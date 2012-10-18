
#define USE_MY_STATE_CHANGE 1		// using my state change method will get better result for all test logs(which are all CINT), except for compress.log
#define ADDRESS_BITS_NUM 10		// using 3 lower bits of PC will limit space to no more than 30kbit (only 16kbit); and the result is not much worse than using 10 bits

typedef unsigned char Value2Bit;

class Instruction
{
public:
	unsigned int currentPC;
	unsigned int nextPC;
	bool branchFlag;
	Instruction(unsigned int pc1, unsigned int pc2, bool br) {
		currentPC = pc1;
		nextPC = pc2;
		branchFlag = br;
	}

	bool isBranch ()
	{
		return !branchFlag;
	}

	bool isTaken ()
	{
		if (currentPC + 4 == nextPC) {
			return false;
		} else {
			return true;
		}
	}
};

class BasicPredictor2Bit
{
public:
	Value2Bit value;
	BasicPredictor2Bit (Value2Bit v) {
		value = v;
	}

	bool predict()
	{
		if (value == 2 || value == 3) return true;
		else return false;
	}

	Value2Bit update(bool reallyTaken)
	{
		if (reallyTaken == true && value < 3) {
#if USE_MY_STATE_CHANGE
			value++;
#else
			if (value == 1) value = 3;
			else value++;
#endif
		}

		if (reallyTaken == false && value > 0) {
#if USE_MY_STATE_CHANGE
			value--;
#else
			if (value == 2) value = 0;
			else value--;
#endif
		}
		return value;
	}
};

float correlating_predict(char* trace_file);
float tournament_predict(char* trace_file);