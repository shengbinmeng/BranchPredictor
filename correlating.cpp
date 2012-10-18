#include "predictor.h"
#include<iostream>
#include<fstream>
#include<vector>
#include<cmath>
using namespace std;

float correlating_predict(char* traceFile)
{
	ifstream fin;
	fin.open(traceFile);
	vector<Instruction> trace;
	vector<Instruction> branches;
	vector<pair<bool, bool>> takens;
	unsigned int currPC, nextPC;
	bool branch;
	int globalHistory = 0x000003FF; //10-bit shift register
	int addressBitsNum = ADDRESS_BITS_NUM;  // changing this will affect miss rate of prediction

	Value2Bit *branchPredBuffers = new Value2Bit[1024 * (int)pow(2.0,addressBitsNum)];
	memset(branchPredBuffers,0,1024 * (int)pow(2.0,addressBitsNum));

	cout<<"**Correlating Predictor**"<<endl;
	cout<<"processing \""<<traceFile<<"\"..."<<endl;
	while(!fin.eof()) {
		if (!fin.good()) {
			fin.clear();
			cerr<<"read error!"<<endl;
			break;
		}

		fin>>hex>>currPC>>nextPC>>branch;
		Instruction instr(currPC, nextPC, branch);
		trace.push_back(instr);

		if (instr.isBranch()) {
			// this is always true for the given trace file
			branches.push_back(instr);

			// do prediction
			int lowAddrBits = (currPC >> 2) & ((int)pow(2.0,addressBitsNum) - 1);
			int bufferIndex = (globalHistory << addressBitsNum) + lowAddrBits;
			BasicPredictor2Bit bp(branchPredBuffers[bufferIndex]);
			bool predictedTaken = bp.predict();

			// check and record result
			bool reallyTaken = instr.isTaken();
			takens.push_back(make_pair(predictedTaken, reallyTaken));

			// update predictor table
			branchPredBuffers[bufferIndex] = bp.update(reallyTaken);

			// update lastBrancheBehavers
			globalHistory = (globalHistory << 1) +  (reallyTaken ? 1 : 0);
			globalHistory = globalHistory & 0x000003FF;
		}
	}

	delete branchPredBuffers;
	fin.close();

	unsigned int rightPredictions = 0;
	for (unsigned int i = 0; i < takens.size(); i++) {
		if (takens[i].first == takens[i].second) {
			rightPredictions ++;
		}
	}
	float rightRate =  float(rightPredictions) / takens.size();
	cout<<rightPredictions<<" correct predictions out of "<<takens.size()<<" branches. \nThe misprediction rate is: 1 - "<<rightRate<<" = "<< 1 - rightRate<<".\n"<<endl;

	return rightRate;
}