#ifndef SSDTOHDD
#define SSDTOHDD


//Shared
#include<vector>
#include<utility>
#include<sys/stat.h>

#define TIMELOGNAME ".s2htimelog"
#define NUMBEROFSAVE (30)

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

vector<int> * printList(vector<pair<double,string>> const* list,int errorCode,double rate);//0 normal
double calWeight(string path);

int getInfo(string path,struct stat*stat,struct s2hData*s2hData);



#endif
