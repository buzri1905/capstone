#include<cstdio>
#include<sys/types.h>
#include<signal.h>
#include<sys/stat.h>
#include<unistd.h>
#include<cerrno>
#include<string.h>
#include<vector>
#include<algorithm>
#include<string>
#include<utility>
#include<dirent.h>
#include<ftw.h>
#include"ssdToHdd.h"
#include<pthread.h>

#define DEBUGMODE

using namespace std;

static vector<pair<double,string> > *s2hlist;
static char absolPathSSD[4096];
static char absolPathHDD[4096];
static vector<double> timeStatic;
static double sum;
static vector<time_t> timeRefer;
static time_t curTime;
int getStaticTime(const char *path,const struct stat *sb,int typeflag);

char pathToTest[1024];

/*			What To Do
 *Error find//find errno
 *demonize main
 *time_t standard deviation.
 *modi ssdtohdd
 */



void updateS2H(const char * path);
int updateDir(string path,const struct stat *sb,off_t *size,int depth);
void startDaemon(int argc,char*argv[],off_t limitSize);
int move2hdd(const char *path);
void s2hDataInit(struct s2hData *s2hData);
int compareTimet(time_t time1,time_t time2);
int saveInfo(string path,struct s2hData*s2hData);
int saveInfo(const char* path,struct s2hData*s2hData);
void getStatic(const char *path);

int main(int argc,char *argv[]){
	(void)argc;
	double limitSizeGB;
	off_t limitSize;
	pid_t pid;
	if(chdir(argv[1])){
		printf("?\n");
		return 1;
	}
	realpath(argv[1],absolPathSSD);
	realpath(argv[2],absolPathHDD);
	sscanf(argv[3],"%lf",&limitSizeGB);
	limitSize=limitSizeGB*1024*1024*1024;
/*
	pid=fork();
	if(pid)
		exit(0);
	signal(SIGHUP,SIG_IGN);
	close(0);
	close(1);
	close(2);
	chdir("/");
	setsid();
*/
	startDaemon(argc,argv,limitSize);
	return 0;//done
}

void startDaemon(int argc,char*argv[],off_t limitSize){
	int errcode,prevErr=0;
	struct stat statbuf;
	double rate;
	off_t size;
	string home=".";
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
		if(!S_ISDIR(statbuf.st_mode))
			exit(2);
		if(stat(absolPathSSD,&statbuf))
			exit(1);
		if(!S_ISDIR(statbuf.st_mode))
			exit(2);
		s2hlist->clear();
		getStatic("./");
		time(&curTime);
		chdir(absolPathSSD);
		updateDir(home,&statbuf,&size,0);
		rate=size/(double)limitSize*100;
		if(rate>=0){//force
			while(1){
				errcode=0;
				selected=printList(argc,argv,s2hlist,prevErr);
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
		sleep(SLEEPTIME);
	}//done
}
int updateDir(string path,const struct stat *sb,off_t *size,int depth){
	off_t totalSize=0,sizeOfSubDir;
	DIR *dir;
	struct dirent *dirEntry;
	struct s2hData s2hData;
	struct stat stat_bf;
	(void)sb;
	*size=0;
	vector<time_t> *toUpdateFile,*toUpdateSubdir;
	dir=opendir(path.c_str());
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
		if(strcmp(dirEntry->d_name,".")==0)
			continue;
		if(strcmp(dirEntry->d_name,"..")==0)
			continue;
		if(stat(dirEntry->d_name,&stat_bf))
			continue;
		if(S_ISDIR(stat_bf.st_mode)){
			if(compareTimet(stat_bf.st_atime,s2hData.lastAccessTimeSubdir))
				toUpdateSubdir->push_back(stat_bf.st_atime);
			string pathString;
			pathString+=path;
			pathString+="/";
			pathString+=dirEntry->d_name;

			chdir(pathString.c_str());
			updateDir(pathString.c_str(),&stat_bf,&sizeOfSubDir,depth+1);
			chdir("..");

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
	if(toUpdateFile->empty()==false){
		sort(toUpdateFile->begin(),toUpdateFile->end());//if size>=NUMBEROFSAVE
		for(vector<time_t>::iterator iter=toUpdateFile->begin();iter!=toUpdateFile->end();iter++){
			s2hData.accessTime[s2hData.end]=*iter;
			s2hData.end=(s2hData.end+1)%NUMBEROFSAVE;
			if(s2hData.end==s2hData.begin)
				s2hData.begin=(s2hData.begin+1)%NUMBEROFSAVE;
		}
	}

	if(toUpdateSubdir->empty()==false){
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
	printf("Pushback dir\n");
	s2hlist->push_back(make_pair(calWeight(pathString,curTime,timeRefer),pathString));
	delete toUpdateFile;
	delete toUpdateSubdir;

	return 0;//done
}


int saveInfo(string path,struct s2hData*s2hData){
	return saveInfo(path.c_str(),s2hData);
}
int saveInfo(const char* path,struct s2hData*s2hData){
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
	string pathString=path;
	if(stat(pathString.c_str(),stat_bf))
		printf("Something wrong in getInfo path is %s\n",path);

	pathString+="/";
	pathString+=TIMELOGNAME;

	fp=fopen(pathString.c_str(),"rb");
	fread(s2hData,sizeof(struct s2hData),1,fp);
	fclose(fp);
	return 0;//change path
}

int move2hdd(const char *path){
	string relativePath,dest;
	relativePath=path;
	relativePath=relativePath.substr(relativePath.find(absolPathSSD)+strlen(absolPathSSD));
	dest=absolPathHDD;
	dest=dest+"/"+relativePath;
	string mkdir="mkdir -p ";
	mkdir=mkdir+dest;
	system(mkdir.c_str());
	string cp="cp -r ";
	cp=cp+path+" "+dest;
	system(cp.c_str());
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

void getStatic(const char *path){
	int numOfFile;
	int numOf10Percent;
	double first10aver=0,last10aver=0;
	double first10Vari=0,last10Vari=0;
	timeStatic.clear();
	timeRefer.clear();
	sum=0;
	time(&curTime);
	ftw(path,getStaticTime,100);
	sort(timeStatic.begin(),timeStatic.end());
	numOfFile=timeStatic.size();
	numOf10Percent=numOfFile/10+3;
	vector<double>::iterator iterFirst=timeStatic.begin();
	vector<double>::reverse_iterator iterLast=timeStatic.rbegin();
	for(int i=0;i<numOf10Percent;i++){
		first10aver+=*iterFirst;
		last10aver+=*iterLast;
		iterFirst++;
		iterLast++;
	}
	first10aver/=numOf10Percent;
	last10aver/=numOf10Percent;

	iterFirst=timeStatic.begin();
	iterLast=timeStatic.rbegin();
	for(int i=0;i<numOf10Percent;i++){
		first10Vari+=(*iterFirst-first10aver)*(*iterFirst-first10aver);
		last10Vari+=(*iterLast-last10aver)*(*iterLast-last10aver);
		iterFirst++;
		iterLast++;
	}
	first10Vari/=numOf10Percent-1;
	last10Vari/=numOf10Percent-1;
	for(int i=0;i<8;i++){
		time_t convert;
		convert=first10aver+(i-3)*first10Vari/8;
		timeRefer.push_back(convert);
		convert=last10aver+(i-3)*last10Vari/8;
		timeRefer.push_back(convert);
	}
	return;
}

int getStaticTime(const char *path,const struct stat *sb,int typeflag){
	(void)path;
	(void)typeflag;
	timeStatic.push_back(difftime(curTime,sb->st_atime));
	return 0;
}
