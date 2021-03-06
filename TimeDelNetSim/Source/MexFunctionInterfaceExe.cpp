#include <matrix.h>
#include <mat.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <chrono>
#include <type_traits>
#include <iostream>

#if defined TIME_DEL_NET_SIM_AS_SUB
	#define HEADER_PATHS_TDNS ..
#elif !defined HEADER_PATHS_TDNS
	#define HEADER_PATHS_TDNS .
#endif

#define SETQUOTE(A) #A
#define JOIN_STRING(A,B,C) SETQUOTE(A##B##C)
#define JOIN_LIB_PATH(PRE, CENT, POST) JOIN_STRING(PRE, CENT, POST)

#include JOIN_LIB_PATH(..\..\, HEADER_PATHS_TDNS, \MexMemoryInterfacing\Headers\MexMem.hpp)

#include "MexFunctionInterface.cpp"

using namespace std;

int main(){
	// NOTE THAT THERE IS NO DATA VALIDATION AS THIS IS EXPECTED TO HAVE 
	// BEEN DONE IN THE MATLAB SIDE OF THE INTERFACE TO THIS MEX FUNCTION

	// Open Memory Usage Account
	size_t MemAccountKey =  MemCounter::OpenMemAccount(size_t(4) << 29);

	// Start Data Input from MAT File
	mxArrayPtr  Input      = nullptr, 
				InputState = nullptr,
				OutputVars = nullptr,
				StateVars  = nullptr,
				FinalState = nullptr;

	MATFile* InputFilePtr = matOpen("Data/InputData.mat", "r");
	MATFile* OutputFilePtr = nullptr;
	char OutFileName[256];
	char OutputFilePath[256] = "";
	Input = matGetVariable(InputFilePtr, "InputStruct");
	
	mxGetString_730(mxGetField(Input, 0, "OutputFile"), OutFileName, 256);
	strcat_s(OutputFilePath, 256, "Data/");
	strcat_s(OutputFilePath, 256, OutFileName);

	matClose(InputFilePtr);
	if (Input == nullptr){
		WriteException(ExOps::EXCEPTION_INVALID_INPUT, "The variable name in the mex file InputData must be InputStruct");
	}
	
	mxArrayPtr lhs[4] = { nullptr, nullptr, nullptr, nullptr }, 
			   rhs[1] = { Input };

	// Confirm Output File Rewrite
	OutputFilePtr = matOpen(OutputFilePath, "r");
	while (OutputFilePtr){
		char UserConfirmResp;
		std::cout << "The following file already exists - \n" << std::endl << "    " << OutFileName << std::endl << "\nSure about rewrite? : " << std::flush;
		std::cin >> UserConfirmResp;
		if ((UserConfirmResp | 32) == 'y'){
			matClose(OutputFilePtr);
			OutputFilePtr = nullptr;
		}
		else if ((UserConfirmResp | 32) == 'n'){
			matClose(OutputFilePtr);
			OutputFilePtr = nullptr;
			cout << "KTHXBYE" << endl;
			system("pause");
			mxDestroyArray(Input);
			return 0;
		}
	}

	// Execute Simulation via the Mex Interface
	try{
		mexFunction(4, lhs, 1, rhs);
	}
	catch (ExOps::ExCodes A){
		if (A == ExOps::EXCEPTION_MEM_FULL){
			printf("Mem Limit of %lld MB Exceeded\n", (MemCounter::MemUsageLimit) >> 20);
			system("pause");
		}
		else {
			system("pause");
		}
		mxDestroyArray(Input);
		return 0;
	}
	
	OutputVars = lhs[0];
	StateVars = lhs[1];
	FinalState = lhs[2];
	InputState = lhs[3];
	
	// Write Output into Output MAT File
	OutputFilePtr = matOpen(OutputFilePath, "wz");
	matPutVariable(OutputFilePtr, "InputState", InputState);
	matPutVariable(OutputFilePtr, "OutputVars", OutputVars);
	matPutVariable(OutputFilePtr, "StateVars", StateVars);
	matPutVariable(OutputFilePtr, "FinalState", FinalState);
	matClose(OutputFilePtr);
	OutputFilePtr = nullptr;
	
	mxDestroyArray(Input);
	mxDestroyArray(InputState);
	mxDestroyArray(OutputVars);
	mxDestroyArray(StateVars);
	mxDestroyArray(FinalState);

	// Close Memory Usage Account
	MemCounter::CloseMemAccount(MemAccountKey);

	system("pause");
	return 0;
}