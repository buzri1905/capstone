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

	file_score = get_file_score(s2hdata.begin, s2hdata.end, s2hdata.accessTime, reference, diff);//0~num_of_diff
	subdir_score = get_subdir_score(s2hdata.beginSubdir, s2hdata.endSubdir, s2hdata.accessTimeSubdir, reference, diff);//0~num_of_diff
	size_score = get_size_score(s2hdata.sizeOfDir);//0~16

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
	
	int diff_len = 0;
	for(vector<time_t>::const_iterator iter = diff.begin(); iter!=diff.end(); iter++) {
		for(int i=0; i<length; i++) {
			if(*iter < reference - accessTime[i]) {
				score++;
			}
		}
		diff_len++;
	}

	score = score / length;

	printf("begin : %d\n", begin);
	printf("end : %d\n", end);
	printf("length : %d\n", score);
	printf("diff_len : %d\n", diff_len);
	printf("score : %lf\n", score);
	printf("==================\n");

	return diff_len - score;
}

static double get_subdir_score(const int begin, const int end, const time_t accessTime[NUMBEROFSAVE], const time_t reference, vector<time_t> const &diff) {
	double score = 0;
	int length;
	if(end >= begin)
		length = end - begin;
	else
		length = NUMBEROFSAVE;

	int diff_len = 0;
	for(vector<time_t>::const_iterator iter = diff.begin(); iter!=diff.end(); iter++) {
		for(int i=0; i<length; i++) {
			if(*iter < reference - accessTime[i]) {
				score++;
			}
		}
		diff_len++;
	}

	score = score / length;

	return diff_len - score;
}

static double get_size_score(const off_t size) {
	return 0;
}
