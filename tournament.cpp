#include "predictor.h"
#include<iostream>
#include<fstream>
#include<vector>
#include<cmath>
using namespace std;

typedef unsigned char Value3Bit;

float tournament_predict(char* traceFile)
{
	ifstream fin;
	fin.open(traceFile);
	vector<Instruction> trace;
	vector<Instruction> branches;
	vector<pair<bool, bool>> takens;

	Value2Bit selectorTable[4096] = {0}; //indexed by local branch address (the lowest 12 bits)

	Value2Bit globalPredBuffers[4096] = {0}; // indexed by last 12 branch behaviours
	int globalHistory = 0x00000FFF; //12-bit shift registers

	Value3Bit localPredBuffers[1024] = {0};
	int localHistoryTable[1024] = {0}; // array of 10-bit shift registers

	cout<<"**Tournament Predictor**"<<endl;
	cout<<"processing \""<<traceFile<<"\"..."<<endl;
	unsigned int currPC, nextPC;
	bool branch;
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
			bool globalSelected;
			int low12AddrBits = (currPC >> 2) & 0x00000FFF;
			int selectorIndex = low12AddrBits;
			Value2Bit selectorValue = selectorTable[selectorIndex];
			if (selectorValue == 2 || selectorValue == 3) globalSelected = true;
			else globalSelected = false;  // select local predictor

			int low10AddrBits = (currPC >> 2) & 0x000003FF;
			int localHistoryIndex = low10AddrBits;
			int localHistory = localHistoryTable[localHistoryIndex];

			bool reallyTaken = instr.isTaken();
			bool predictedTaken;
			if (globalSelected) { 
				// do global prediction
				BasicPredictor2Bit bp(globalPredBuffers[globalHistory]);
				predictedTaken = bp.predict();
				// update basic predictor and selector table
				globalPredBuffers[globalHistory] = bp.update(reallyTaken);
				if (predictedTaken == reallyTaken && selectorValue < 3) {
					// global predictor is right
					if (selectorValue == 1) selectorValue = 3;
					else selectorValue++;
				}
				if (predictedTaken != reallyTaken) {
					if (selectorValue == 2) selectorValue = 0;
					else selectorValue--;
				}
				selectorTable[selectorIndex] = selectorValue;
			} else {
				// do local prediction
				Value3Bit value = localPredBuffers[localHistory];
				if (value > 3) predictedTaken = true;
				else predictedTaken = false;

				// update basic predictor and selector table
				if (reallyTaken == true && value < 7) {
#if USE_MY_STATE_CHANGE
					value++;
#else
					if (value == 3) value = 7;
					else value++;
#endif
				}
				if (reallyTaken == false && value > 0) {
#if USE_MY_STATE_CHANGE
					value--;
#else
					if (value == 4) value = 0;
					else value--;
#endif
				}
				localPredBuffers[localHistory] = value;

				if (predictedTaken == reallyTaken && selectorValue > 0) {
					// local predictor is right
#if USE_MY_STATE_CHANGE
					selectorValue --;
#else
					if (selectorValue == 2) selectorValue = 0;
					else selectorValue--;
#endif
				}
				if (predictedTaken != reallyTaken) {
#if USE_MY_STATE_CHANGE
					selectorValue++;
#else
					if (selectorValue == 1) selectorValue = 3;
					else selectorValue++;
#endif
				}

				selectorTable[selectorIndex] = selectorValue;
			}
			
			// record result
			takens.push_back(make_pair(predictedTaken, reallyTaken));
			
			// update globalHistory and localHistory
			globalHistory = (globalHistory << 1) +  (reallyTaken ? 1 : 0);
			globalHistory = globalHistory & 0x00000FFF;

			localHistory = (localHistory << 1) +  (reallyTaken ? 1 : 0);
			localHistory = localHistory & 0x000003FF;
			localHistoryTable[localHistoryIndex] = localHistory;
		}
	}


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