/* perf_compute.cpp C++ program to compute the specific result I got from cp test*/

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <iomanip> 

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TEST_TIMES 20
#define BLOCK_SIZE_NUM 8
#define FILE_SIZE_NUM 20

using namespace std;

float mean(vector<float> &v){
	unsigned int i;
	float m = 0;
	for(i = 0; i < v.size(); i++){
		m += v[i];
	}
	return m/v.size();
}

float stdev(vector<float> &v){
	float m = mean(v);
	unsigned int i;
	float s = 0;
	for(i = 0; i < v.size(); i++){
		s += (m - v[i]) * (m - v[i]);
	}
	/* may devide v.size() - 1*/
	return sqrt(s/(v.size() - 1));
}

vector<string> perf_file;
vector<string> file_size; 
string output = "speedup-cp.dat";
vector<string> bsz;
vector<string> fsz;

struct spdup{
	float speedup;
	float speedup_s;
};
vector<vector< struct spdup > > odata;

vector<float> async_per_b_f_data;
vector<float> orig_per_b_f_data;

int main(int argc, char **argv){
	/* parse input args if necessary*/
	perf_file.push_back("perf-result/noperf-hot-4k");
	perf_file.push_back("perf-result/noperf-hot-8k");
	perf_file.push_back("perf-result/noperf-hot-16k");
	perf_file.push_back("perf-result/noperf-hot-32k");
	perf_file.push_back("perf-result/noperf-hot-64k");
	perf_file.push_back("perf-result/noperf-hot-128k");
	perf_file.push_back("perf-result/noperf-hot-256k");
	perf_file.push_back("perf-result/noperf-hot-512k");

	file_size.push_back("1k");
	file_size.push_back("2k");
	file_size.push_back("4k");
	file_size.push_back("8k");
	file_size.push_back("16k");
	file_size.push_back("32k");
	file_size.push_back("64k");
	file_size.push_back("128k");
	file_size.push_back("256k");
	file_size.push_back("512k");
	file_size.push_back("1M");
	file_size.push_back("2M");
	file_size.push_back("4M");
	file_size.push_back("8M");
	file_size.push_back("16M");
	file_size.push_back("32M");
	file_size.push_back("64M");
	file_size.push_back("128M");
	file_size.push_back("256M");
	file_size.push_back("512M");


	/* open the output file and write one line of comment in it*/
	unsigned int filecnt;
	for(filecnt = 0; filecnt < perf_file.size(); filecnt++){
		string filename = perf_file[filecnt];
		bsz.push_back(filename.substr(23,filename.size()-23));
		cout << "block size right now: " << atoi(bsz[filecnt].c_str()) << endl;
		/* make a node for this block size*/
		ifstream fin(filename.c_str());
		if(!fin){
			cout << "cannot open file: " << perf_file[filecnt] << endl;
		}
		else{
			cout << "opening file: " << perf_file[filecnt].c_str() << endl;
		}
		int per_f_cnt = 0;
		string temp_line;
		string f_size;
		vector<struct spdup> per_f_data;
		per_f_data.clear();
		while(getline(fin, temp_line)){
			int file_index;
			/* meet one file of a particular size here*/
			if(temp_line.substr(0,10) == "file_size:"){
					string sub = temp_line.substr(10,temp_line.size()-10);
					file_index = atoi(sub.c_str());
					f_size = file_size[file_index];
					cout << "file size got right now: " << f_size << endl;
				/* push the f_size back to fsz if it has not been there*/
				if(fsz.size() < FILE_SIZE_NUM){
					fsz.push_back(f_size);
				}	
				/* get a new line*/
				getline(fin, temp_line);
				getline(fin, temp_line);
				if(temp_line.substr(0,13) != "async_cp_hot:"){
					cout << "not the line we want: " << temp_line << endl;
					return -1;
				}
				/* skip the warm up 13 lines*/
				per_f_cnt = 0;
				while(per_f_cnt < 13){
					getline(fin, temp_line);
					per_f_cnt++;
				}
				per_f_cnt = 0;
				/* count number of lines equal to TEST_TIMES*/
				while(per_f_cnt < TEST_TIMES){
					getline(fin, temp_line);
					string sub = temp_line.substr(10,temp_line.size()-13);
					async_per_b_f_data.push_back((float)atoi(sub.c_str()));
					cout << "time get: " << atoi(sub.c_str()) << " us" << endl; 
					per_f_cnt++;
				}
				per_f_cnt = 0;
				/* skip two lines to get to orig cp*/
				while(per_f_cnt < 2){
					getline(fin, temp_line);
					per_f_cnt++;
				}
				getline(fin, temp_line);
				if(temp_line.substr(0,12) != "orig_cp_hot:"){
					cout << "not the line we want: " << temp_line << endl;
					return -1;
				}
				/* skip the warm up 13 lines */
				per_f_cnt = 0;
				while(per_f_cnt < 13){
					getline(fin, temp_line);
					per_f_cnt++;
				}
				per_f_cnt = 0;
				/* count the nubmer of lines equal to TEST_TIMES*/
				while(per_f_cnt < TEST_TIMES){
					getline(fin, temp_line);
					string sub = temp_line.substr(13, temp_line.size()-16);
					orig_per_b_f_data.push_back((float) atoi(sub.c_str()));
					cout << "time get: " << atoi(sub.c_str()) << " us" << endl; 
					per_f_cnt++;
				}
				per_f_cnt = 0;
				/* calculate the mean and standard deviation, and speedup*/
				float async_m = mean(async_per_b_f_data);
				float orig_m = mean(orig_per_b_f_data);
				float async_s = stdev(async_per_b_f_data);
				float orig_s = stdev(orig_per_b_f_data);
				cout << async_m << "  " << async_s << endl;
				cout << orig_m << "  " << orig_s << endl;
				float speedup = (orig_m - async_m) / async_m * 100;
				float speedup_s = speedup * sqrt((async_s / async_m) * (async_s / async_m) + (orig_s / orig_m) * (orig_s / orig_m));
				struct spdup up;
				up.speedup = speedup;
				up.speedup_s = speedup_s;
				per_f_data.push_back(up);
				async_per_b_f_data.clear();
				orig_per_b_f_data.clear();
			}
			/* skip the current line and fetch the next line*/
			else{
			}
		}/* while*/ 
		vector<struct spdup> ll = per_f_data;
		cout << ll.size() << endl;
		odata.push_back(ll);
		per_f_data.clear();
		fin.close();
	}/* for*/
	ofstream fout(output.c_str());
	fout << "#file size \t 4k-m \t 4k-s \t 8k-m \t 8k-s \t 16-m \t 16k-s \t 32k-m \t 32k-s \t 64k-m \t 64k-s \t 128k-m \t 128k-s \t 256k-m \t 256k-s \t 512k-m \t 512k-s\n";
	unsigned long i, j;
	cout << odata.size() << endl;
	for(i = 0; i < fsz.size(); i++){
		cout << "writing fsz: " << fsz[i] << endl;
		fout << fsz[i];
		for(j = 7; j < bsz.size(); j++){
			fout << "\t\t " << setprecision(3) << odata[j][i].speedup << " \t " << setprecision(3) << odata[j][i].speedup_s << " ";
		}
		fout << endl;
	}	
	fout.close();
	return 0;	
}
