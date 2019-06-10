#include<vector>
#include<utility>
#include<sys/stat.h>
#include<string>
#include<math.h>
#include"ssdToHdd.h"


using namespace std;

static double get_file_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff);
static double get_subdir_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff);
static double get_size_score(const off_t size);

double calWeight(string path, time_t reference, vector<time_t> const &diff) {
    struct stat stat;
    struct s2hData s2hdata;
	double file_score, subdir_score, size_score, total_score;
    getInfo(path, &stat, &s2hdata);
	printf("%s\n", path.c_str());

	file_score = get_file_score(s2hdata.begin, s2hdata.end, s2hdata.accessTime, reference, diff);//0~num_of_diff
	subdir_score = get_subdir_score(s2hdata.beginSubdir, s2hdata.endSubdir, s2hdata.accessTimeSubdir, reference, diff);//0~num_of_diff
	size_score = get_size_score(s2hdata.sizeOfDir);//0~16
	//100MB 1GB 5GB
	//1.1 1.2 1.3
	total_score = file_score + subdir_score;//weight???
	
	return total_score;
}

double get_file_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff) {
	double score = 0;
	int length;
	if(end >= begin)
		length = end - begin;
	else
		length = NUMBEROFSAVE;
	
	for(vector<time_t>::const_iterator iter = diff.begin(); iter!=diff.end(); iter++) {
		for(int i=0; i<length; i++) {
			if(*iter < reference - accessTime[i]) {
				score++;
			}
		}
	}

	if(length==0)
		return 0.0;

	score = score / length;

	return score;
}

static double get_subdir_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff) {
	double score = 0;
	int length;
	if(end >= begin)
		length = end - begin;
	else
		length = NUMBEROFSAVE;

	for(vector<time_t>::const_iterator iter = diff.begin(); iter!=diff.end(); iter++) {
		for(int i=0; i<length; i++) {
			if(*iter < reference - accessTime[i]) {
				score++;
			}
		}
	}

	if(length==0)
		return 0.0;

	score = score / length;

	return score;
}

static double get_size_score(const off_t size) {
	printf("size : %ld\n", size);
	return 0;
}