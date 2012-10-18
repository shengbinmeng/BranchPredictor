#include "predictor.h"
#include<iostream>
using namespace std;

int main(int argc, char* argv[]){

	if (argc < 2) {
		cerr<<"Please provide path to the trace file!\nUsage: path-to-trace-file [,path-to-trace-file,...]"<<endl;
	}

	for (int i = 1; i < argc; i++) {
		char* traceFile = argv[i];
		correlating_predict(traceFile);
		tournament_predict(traceFile);
	}

	return 0;
}