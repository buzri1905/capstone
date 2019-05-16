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


/*			What To Do
 *Think of the address type, that is the address is given with relative or absolute.
 *Change int to off_t(limit size)
 *Error find//find errno
 *
 *
 *
 *
 */



void updateS2H(const char * path);
int updateDir(const char * path,const struct stat *sb,off_t *size,int depth);
void startDaemon(const char * ssdPath,const char*hddPath,int limitSize);
int move2hdd(const char *path);
void s2hDataInit(struct s2hData *s2hData);

int main(int argc,char *argv[]){
	(void)argc;
	int limitSize;
	if(chdir(argv[1])){
		printf("?\n");
		return 1;
	}
	limitSize=atoi(argv[3]);
	//change to relative add
	startDaemon(argv[1],argv[2],limitSize);//MB?
	return 0;
}

void startDaemon(const char* ssdPath,const char*hddPath,int limitSize){
	int errcode,prevErr=0;
	struct stat statbuf;
	double rate;
	off_t size;
	s2hlist=new vector<pair<double,string>>;
	vector<int>*selected;
	while(1){
		size=0;
		if(stat(hddPath,&statbuf))
			exit(1);
		if((statbuf.st_mode&S_IFDIR)==0)
			exit(2);
		if(stat(ssdPath,&statbuf))
			exit(1);
		if((statbuf.st_mode&S_IFDIR)==0)
			exit(2);
		s2hlist->clear();
		updateDir(ssdPath,&statbuf,&size,0);
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
				//delete selected;
				s2hlist=tmpList;
			}
		}
	}
}
int updateDir(const char * path,const struct stat *sb,off_t *size,int depth){
	int totalSize=0,sizeOfSubDir;
	DIR *dir;
	struct dirent *dirEntry;
   	FILE*fp;
	struct s2hData s2hData;
	struct stat stat;
	vector<time_t> *toUpdateFile,*toUpdateSubdir;
	dir=opendir(path);
	if(dir==NULL)
		return FAILEDTOOPEN;
	if(access(TIMELOGNAME,F_OK)!=-1){
		fp=fopen(TIMELOGNAME,"rb");
		fread(&s2hData,sizeof(struct s2hData),1,fp);
		fclose(fp);
	}
	else{
		s2hDataInit(&s2hData);
	}
	toUpdateFile=new vector<time_t>;
	toUpdateSubdir=new vector<time_t>;
	while((dirEntry=readdir(dir))!=NULL){
		stat(dirEntry.d_name,&stat);
		if(stat.st_mode&S_IFDIR){
			if(compareTimet(stat->st_atime,s2hData.lastAccessTimeSubdir))
				toUpdateSubdir->push_back(stat->st_atime);
			updateDir(dirEntry->d_name,&stat,&sizeOfSubDir,depth+1);
			totalSize+=sizeOfSubDir;
		}
		else{
			if(strcmp(dirEntry->d_name,TIMELOGNAME)==0)
				continue;
			if(compareTimet(stat->st_atime,s2hData.lastAccessTime))
				toUpdate->push_back(stat->st_atime);
			totalSize+=stat->st_size;
		}
	}
	if(toUpdateFile.size()!=0){

	//file update
	delete toUpdateFile;
	delete toUpdateSubdir;
	return 0;
}


int move2hdd(const char *path);
void s2hDataInit(struct s2hData *s2hData){
	s2hData->begin=s2hData->end=s2hData->beginSubdir=s2hData->endSubdir=0;
	s2Data->lastAccessTime=s2Data->lastAccessTimeSubdir=0;
	for(int i=0;i<NUMBEROFSAVE;i++)
		s2Data->accessTime[i]=s2hData->accessTimeSubdir[i]=0;
}
