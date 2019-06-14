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
int updateDir(string path,const struct stat *sb,off_t *size,int depth,time_t *subDirTime);
void startDaemon(int argc,char*argv[],off_t limitSize);
int move2hdd(const char *path);
void s2hDataInit(struct s2hData *s2hData);
int saveInfo(string path,struct s2hData*s2hData);
int saveInfo(const char* path,struct s2hData*s2hData);
void getStatic(const char *path);

int main(int argc,char *argv[]){
	(void)argc;
	double limitSizeGB;
	off_t limitSize;
	//pid_t pid;
	/*if(chdir(argv[1])){
		return 1;
	}*/
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
	printf("SSD dir is %s\n",absolPathSSD);
	printf("HDD dir is %s\n",absolPathHDD);
	stat(absolPathSSD,&statbuf);
	//if(stat(absolPathSSD,&statbuf))
	//	exit(1);
	if(!S_ISDIR(statbuf.st_mode))
		exit(-1);
	chdir(absolPathSSD);
	while(1){
		size=0;
		/*if(stat(absolPathHDD,&statbuf))
			exit(1);*/
		stat(absolPathHDD,&statbuf);
		if(!S_ISDIR(statbuf.st_mode))
			exit(-1);
		//if(stat(absolPathSSD,&statbuf))
		//	exit(1);
		stat(absolPathSSD,&statbuf);
		if(!S_ISDIR(statbuf.st_mode))
			exit(-1);
		s2hlist->clear();
		getStatic("./");
		time(&curTime);
		chdir(absolPathSSD);
		time_t tmp;
		updateDir(home,&statbuf,&size,0,&tmp);//tmp does not need
		rate=size/(double)limitSize*100;
		sort(s2hlist->begin(),s2hlist->end(),greater<>());
		if(rate>=0){//force
			prevErr=0;
			while(1){
				errcode=0;
				printf("Time reference : ");
				for(vector<time_t>::iterator iter=timeRefer.begin();iter!=timeRefer.end();iter++)
					printf("%ld ",*iter);
				puts("");
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
int updateDir(string path,const struct stat *sb,off_t *size,int depth,time_t *subDirTime){
	off_t totalSize=0,sizeOfSubDir;
	DIR *dir;
	struct dirent *dirEntry;
	struct s2hData s2hData;
	struct stat stat_bf;
	time_t subDirTimePart;
	(void)sb;
	*size=0;
	*subDirTime=0;
	vector<time_t> *toUpdateFile,*toUpdateSubdir;
	dir=opendir(path.c_str());
	if(dir==NULL)
		return FAILEDTOOPEN;
	getInfo(path,&stat_bf,&s2hData);
	toUpdateFile=new vector<time_t>;
	toUpdateSubdir=new vector<time_t>;
	string dirPath=absolPathSSD;
	dirPath+="/";
	string tmpPath=path;
	tmpPath.erase(0,2);
	dirPath+=tmpPath+"/";
	while((dirEntry=readdir(dir))!=NULL){
		if(dirEntry->d_name[0]=='.')
			continue;
		//if(strcmp(dirEntry->d_name,".")==0)
		//	continue;
		if(strcmp(dirEntry->d_name,"..")==0)
			continue;
		string d_nameString=dirPath+dirEntry->d_name;
		if(stat(d_nameString.c_str(),&stat_bf)){
			continue;
		}
		if(S_ISDIR(stat_bf.st_mode)){
			string pathString=path+"/"+dirEntry->d_name;
			updateDir(pathString.c_str(),&stat_bf,&sizeOfSubDir,depth+1,&subDirTimePart);
			if(subDirTimePart > s2hData.lastAccessTimeSubdir){//update lastAccessTimeSubdir
				toUpdateSubdir->push_back(subDirTimePart);
				if(*subDirTime < subDirTimePart)
					*subDirTime=subDirTimePart;
			}
			totalSize+=sizeOfSubDir;
		}
		else{
			if(strcmp(dirEntry->d_name,TIMELOGNAME)==0)
				continue;
			if(stat_bf.st_atime>s2hData.lastAccessTime)
				toUpdateFile->push_back(stat_bf.st_atime);
			if(*subDirTime < stat_bf.st_atime)
				*subDirTime=stat_bf.st_atime;
			totalSize+=stat_bf.st_size;
		}
	}
	if(toUpdateFile->empty()==false){
		sort(toUpdateFile->begin(),toUpdateFile->end());//if size>=NUMBEROFSAVE
		for(vector<time_t>::iterator iter=toUpdateFile->begin();iter!=toUpdateFile->end();iter++){
			s2hData.accessTime[s2hData.end]=*iter;
			s2hData.end=(s2hData.end+1)%NUMBEROFSAVE;
			s2hData.lastAccessTime=*iter;
			if(s2hData.end==s2hData.begin)
				s2hData.begin=(s2hData.begin+1)%NUMBEROFSAVE;
		}
	}

	if(toUpdateSubdir->empty()==false){
		sort(toUpdateSubdir->begin(),toUpdateSubdir->end());//if size>=NUMBEROFSAVE
		for(vector<time_t>::iterator iter=toUpdateSubdir->begin();iter!=toUpdateSubdir->end();iter++){
			s2hData.accessTimeSubdir[s2hData.endSubdir]=*iter;
			s2hData.endSubdir=(s2hData.endSubdir+1)%NUMBEROFSAVE;
			s2hData.lastAccessTimeSubdir=*iter;
			if(s2hData.endSubdir==s2hData.beginSubdir)
				s2hData.beginSubdir=(s2hData.beginSubdir+1)%NUMBEROFSAVE;
		}
	}
	*size=s2hData.sizeOfDir=totalSize;
	saveInfo(path,&s2hData);

	string pathString=absolPathSSD;
	pathString+="/";
	path.erase(0,2);
	pathString+=path;
	//printf("Pushback dir\n");
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
	if(fp==0){
		return 0;
	}
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
	stat(pathString.c_str(),stat_bf);

	pathString+="/";
	pathString+=TIMELOGNAME;

	fp=fopen(pathString.c_str(),"rb");
	if(fp==0){//no permission
		s2hDataInit(s2hData);
		return 0;
	}
	fread(s2hData,sizeof(struct s2hData),1,fp);
	fclose(fp);
	return 0;//change path
}

int move2hdd(const char *path){
	string relativePath=path;
	relativePath.erase(0,strlen(absolPathSSD));
	string dest=absolPathHDD;
	dest+=relativePath;
	string commandMV="mv -f ";
	commandMV+=path;
	commandMV+=" "+dest;
	system(commandMV.c_str());
	//printf("%s\n",commandMV.c_str());
	return 0;	
}//to do...


void s2hDataInit(struct s2hData *s2hData){
	s2hData->begin=s2hData->end=s2hData->beginSubdir=s2hData->endSubdir=0;
	s2hData->lastAccessTime=s2hData->lastAccessTimeSubdir=0;
	for(int i=0;i<NUMBEROFSAVE;i++)
		s2hData->accessTime[i]=s2hData->accessTimeSubdir[i]=0;
}//done

void getStatic(const char *path){
	int numOfFile;
	int numOf10Percent;
	int numOf33Percent;
	int numOf50Percent;
	double first10aver=0,last10aver=0;
	double first10Vari=0,last10Vari=0;
	double last33aver=0,last50aver=0;
	double last33Vari=0,last50Vari=0;
	timeStatic.clear();
	timeRefer.clear();
	sum=0;
	time(&curTime);
	ftw(path,getStaticTime,100);
	sort(timeStatic.begin(),timeStatic.end());
	printf("Time static : ");
	for(vector<double>::iterator iter=timeStatic.begin();iter!=timeStatic.end();iter++)
		printf("%lf ",*iter);
	puts("");
	numOfFile=timeStatic.size();
	numOf10Percent=numOfFile/10+1;
	numOf33Percent=numOfFile/3+1;
	numOf50Percent=numOfFile/2+1;
	vector<double>::iterator iterFirst=timeStatic.begin();
	vector<double>::reverse_iterator iterLast=timeStatic.rbegin();
	for(int i=0;i<numOf10Percent;i++){
		first10aver+=*iterFirst;
		last10aver+=*iterLast;
		iterFirst++;
		iterLast++;
	}
	iterLast=timeStatic.rbegin();
	for(int i=0;i<numOf33Percent;i++){
		last33aver+=*iterLast;
		iterLast++;
	}
	iterLast=timeStatic.rbegin();
	for(int i=0;i<numOf50Percent;i++){
		last50aver+=*iterLast;
		iterLast++;
	}

	first10aver/=numOf10Percent;
	last10aver/=numOf10Percent;

	last33aver/=numOf33Percent;
	last50aver/=numOf50Percent;

	iterFirst=timeStatic.begin();
	iterLast=timeStatic.rbegin();
	for(int i=0;i<numOf10Percent;i++){
		first10Vari+=(*iterFirst-first10aver)*(*iterFirst-first10aver);
		last10Vari+=(*iterLast-last10aver)*(*iterLast-last10aver);
		iterFirst++;
		iterLast++;
	}
	first10Vari/=numOf10Percent;
	last10Vari/=numOf10Percent;
	
	iterLast=timeStatic.rbegin();
	for(int i=0;i<numOf33Percent;i++){
		last33Vari+=(*iterLast-last33aver)*(*iterLast-last33aver);
		iterLast++;
	}
	iterLast=timeStatic.rbegin();
	for(int i=0;i<numOf50Percent;i++){
		last50Vari+=(*iterLast-last50aver)*(*iterLast-last50aver);
		iterLast++;
	}
	
	last33Vari/=numOf33Percent;
	last50Vari/=numOf50Percent;

	printf("varis %lf %lf %lf %lf\n",first10Vari,last10Vari,last33Vari,last50Vari);
	for(int i=0;i<4;i++){
		time_t convert;
		convert=first10aver+(i-2)*first10Vari/8;
		timeRefer.push_back(convert);
		convert=last10aver+(i-2)*last10Vari/8;
		timeRefer.push_back(convert);
		convert=last33aver+(i-2)*last33Vari/8;
		timeRefer.push_back(convert);
		convert=last50aver+(i-2)*last50Vari/8;
		timeRefer.push_back(convert);
	}
	return;
}

int getStaticTime(const char *path,const struct stat *sb,int typeflag){
	(void)typeflag;
	for(int i=1;path[i];i++)
		if(path[i]=='.'&&path[i-1]=='/')
			return 0;
	timeStatic.push_back(difftime(curTime,sb->st_atime));
	return 0;
}
