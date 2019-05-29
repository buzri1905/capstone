#include<vector>
#include<utility>
#include<sys/stat.h>
#include<string> 
#include<math.h>

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

int getInfo(string path, struct stat *stat, struct s2hData *s2hData);
int getInfo(const char *path, struct stat *stat, struct s2hData *s2hData);

static double get_file_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff);
static double get_subdir_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff);
static double get_size_score(const off_t sizeOfDir);//?????

double calWeight(string path, time_t reference, vector<time_t> const &diff);

int main() {

    return 0;
}

double calWeight(string path, time_t reference, vector<time_t> const &diff) {
    struct stat stat;
    struct s2hData s2hdata;
	double file_score, subdir_score, size_score, total_score;
    getInfo(path, &stat, &s2hdata);

	file_score = get_file_score(s2hdata.begin, s2hdata.end, s2hdata.accessTime, reference, diff);
	subdir_score = get_subdir_score(s2hdata.beginSubdir, s2hdata.endSubdir, s2hdata.accessTimeSubdir, reference, diff);

	total_score = file_score + subdir_score;//weight???

	return total_score;
}

double get_file_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff) {
	double score = 0;
	int length = sizeof(accessTime) / sizeof(time_t);
	for(vector<time_t>::const_iterator iter = diff.begin(); iter!=diff.end(); iter++) {
		for(int i=0; i<length; i++) {
			if(*iter<accessTime[i]) {
				score++;
			}
		}
	}

	score = score / length;

	return score;
}

static double get_subdir_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff) {
	double score = 0;
	int length = sizeof(accessTime) / sizeof(time_t);
	for(vector<time_t>::const_iterator iter = diff.begin(); iter!=diff.end(); iter++) {
		for(int i=0; i<length; i++) {
			if(*iter<accessTime[i]) {
				score++;
			}
		}
	}

	score = score / length;

	return score;
}