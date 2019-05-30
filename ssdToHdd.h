#ifndef SSDTOHDD
#define SSDTOHDD


//Shared
#include<vector>
#include<utility>
#include<sys/stat.h>

#define TIMELOGNAME ".s2htimelog"
#define NUMBEROFSAVE (30)
#define SLEEPTIME (60)

using namespace std;

struct s2hData{
	int begin,end; 
	time_t lastAccessTime;
	time_t accessTime[NUMBEROFSAVE];
	int beginSubdir,endSubdir;
	time_t lastAccessTimeSubdir;
	time_t accessTimeSubdir[NUMBEROFSAVE];
	off_t sizeOfDir;
};

vector<int> * printList(int argc,char*argv[],vector<pair<double,string>> const* list,int errorCode);//0 normal
double calWeight(string path,time_t reference,vector<time_t> const &diff);

int getInfo(string path,struct stat*stat,struct s2hData*s2hData);
int getInfo(const char* path,struct stat*stat,struct s2hData*s2hData);


#define FAILEDTOOPEN (1)


#endif
