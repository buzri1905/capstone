#include<cstdio>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<cerrno>
#include<string.h>
#include<vector>
#include<algorithm>
#include<string>
#include<utility>
#include<dirent.h>
#include"ssdToHdd.h"

using namespace std;

static vector<pair<double,string> > *s2hlist;
static char absolPathSSD[4096];
static char absolPathHDD[4096];

/*			What To Do
 *Error find//find errno
 *demonize main
 */



void updateS2H(const char * path);
int updateDir(const char * path,const struct stat *sb,off_t *size,int depth);
void startDaemon(off_t limitSize);
int move2hdd(const char *path);
void s2hDataInit(struct s2hData *s2hData);
int compareTimet(time_t time1,time_t time2);
int saveInfo(string path,struct s2hData*s2hData);

int main(int argc,char *argv[]){
	(void)argc;
	double limitSizeGB;
	off_t limitSize;
	if(chdir(argv[1])){
		printf("?\n");
		return 1;
	}
	realpath(argv[1],absolPathSSD);
	realpath(argv[2],absolPathHDD);
	sscanf(argv[3],"%lf",&limitSizeGB);
	limitSize=limitSizeGB*1024*1024*1024;
	startDaemon(limitSize);
	return 0;//only damonize left
}

void startDaemon(off_t limitSize){
	int errcode,prevErr=0;
	struct stat statbuf;
	double rate;
	off_t size;
	s2hlist=new vector<pair<double,string>>;
	vector<int>*selected;
	if(stat(absolPathSSD,&statbuf))
		exit(1);
	if((statbuf.st_mode&S_IFDIR)==0)
		exit(2);
	chdir(absolPathSSD);
	while(1){
		size=0;
		if(stat(absolPathHDD,&statbuf))
			exit(1);
		if((statbuf.st_mode&S_IFDIR)==0)
			exit(2);
		if(stat(absolPathSSD,&statbuf))
			exit(1);
		if((statbuf.st_mode&S_IFDIR)==0)
			exit(2);
		s2hlist->clear();
		updateDir("./",&statbuf,&size,0);
		rate=size/(double)limitSize*100;
		if(rate>=90){
			while(1){
				errcode=0;
				selected=printList(s2hlist,prevErr);
				for(vector<int>::iterator iter=selected->begin();iter!=selected->end();iter++)
					errcode|=move2hdd(s2hlist->at(*iter).second.c_str());
				prevErr=errcode;
				if(!errcode)
					break;
				vector<pair<double,string>> *tmpList=new vector<pair<double,string>>;
				vector<int>::iterator iterSelected=selected->begin();
				vector<pair<double,string>>::iterator iterList=s2hlist->begin();
				for(int i=0;iterSelected!=selected->end();iterSelected++){
					while(i<*iterSelected){
						i++;
						tmpList->push_back(*iterList);
						iterList++;
					}
				}
				for(;iterList!=s2hlist->end();iterList++)
					tmpList->push_back(*iterList);
				delete s2hlist;
				s2hlist=tmpList;
				//?delete selected;
			}
		}
	}//done
}
int updateDir(const char * path,const struct stat *sb,off_t *size,int depth){
	off_t totalSize=0,sizeOfSubDir;
	DIR *dir;
	struct dirent *dirEntry;
	struct s2hData s2hData;
	struct stat stat_bf;
	(void)sb;
	vector<time_t> *toUpdateFile,*toUpdateSubdir;
	dir=opendir(path);
	if(dir==NULL)
		return FAILEDTOOPEN;
	if(access(TIMELOGNAME,F_OK)!=-1){
		getInfo(path,&stat_bf,&s2hData);
	}
	else{
		s2hDataInit(&s2hData);
	}
	toUpdateFile=new vector<time_t>;
	toUpdateSubdir=new vector<time_t>;
	while((dirEntry=readdir(dir))!=NULL){
		stat(dirEntry->d_name,&stat_bf);
		if(stat_bf.st_mode&S_IFDIR){
			if(compareTimet(stat_bf.st_atime,s2hData.lastAccessTimeSubdir))
				toUpdateSubdir->push_back(stat_bf.st_atime);
			string pathString=path;
			pathString+="/";
			pathString+=dirEntry->d_name;
			updateDir(pathString.c_str(),&stat_bf,&sizeOfSubDir,depth+1);
			totalSize+=sizeOfSubDir;
		}
		else{
			if(strcmp(dirEntry->d_name,TIMELOGNAME)==0)
				continue;
			if(compareTimet(stat_bf.st_atime,s2hData.lastAccessTime))
				toUpdateFile->push_back(stat_bf.st_atime);
			totalSize+=stat_bf.st_size;
		}
	}
	if(toUpdateFile->size()!=0){
		sort(toUpdateFile->begin(),toUpdateFile->end());//if size>=NUMBEROFSAVE
		for(vector<time_t>::iterator iter=toUpdateFile->begin();iter!=toUpdateFile->end();iter++){
			s2hData.accessTime[s2hData.end]=*iter;
			s2hData.end=(s2hData.end+1)%NUMBEROFSAVE;
			if(s2hData.end==s2hData.begin)
				s2hData.begin=(s2hData.begin+1)%NUMBEROFSAVE;
		}
	}

	if(toUpdateSubdir->size()!=0){
		sort(toUpdateSubdir->begin(),toUpdateSubdir->end());//if size>=NUMBEROFSAVE
		for(vector<time_t>::iterator iter=toUpdateSubdir->begin();iter!=toUpdateSubdir->end();iter++){
			s2hData.accessTimeSubdir[s2hData.endSubdir]=*iter;
			s2hData.endSubdir=(s2hData.endSubdir+1)%NUMBEROFSAVE;
			if(s2hData.endSubdir==s2hData.beginSubdir)
				s2hData.beginSubdir=(s2hData.beginSubdir+1)%NUMBEROFSAVE;
		}
	}
	*size=s2hData.sizeOfDir=totalSize;
	saveInfo(path,&s2hData);

	string pathString=absolPathSSD;
	pathString+="/";
	pathString+=path;
	s2hlist->push_back(make_pair(calWeight(pathString),pathString));
	delete toUpdateFile;
	delete toUpdateSubdir;

	return 0;//done
}


int saveInfo(char* path,struct s2hData*s2hData){
	FILE*fp;
	string pathString=absolPathSSD;
	pathString+="/";
	pathString+=path;
	pathString+="/";
	pathString+=TIMELOGNAME;

	fp=fopen(pathString.c_str(),"wb");
	fwrite(s2hData,sizeof(struct s2hData),1,fp);
	fclose(fp);
	return 0;//done
}
int getInfo(string path,struct stat*stat_bf,struct s2hData*s2hData){
	return getInfo(path.c_str(),stat_bf,s2hData);//done
}
int getInfo(const char *path,struct stat*stat_bf,struct s2hData*s2hData){
	FILE*fp;
	string pathString=absolPathSSD;
	pathString+="/";
	pathString+=path;
	stat(pathString.c_str(),stat_bf);

	pathString+="/";
	pathString+=TIMELOGNAME;

	fp=fopen(pathString.c_str(),"rb");
	fread(s2hData,sizeof(struct s2hData),1,fp);
	fclose(fp);
	return 0;//change path
}

int move2hdd(const char *path){
	string relativePath=path;
	relativePath.substr(relativePath.find(absolPathSSD));

	return 0;	
}//to do...


void s2hDataInit(struct s2hData *s2hData){
	s2hData->begin=s2hData->end=s2hData->beginSubdir=s2hData->endSubdir=0;
	s2hData->lastAccessTime=s2hData->lastAccessTimeSubdir=0;
	for(int i=0;i<NUMBEROFSAVE;i++)
		s2hData->accessTime[i]=s2hData->accessTimeSubdir[i]=0;
}//done
int compareTimet(time_t time1,time_t time2){
	if(time1>time2)
		return 1;
	if(time1<time2)
		return -1;
	return 0;
}//have to varify
