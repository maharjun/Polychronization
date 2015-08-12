#include <mex.h>
#include <matrix.h>
#include <algorithm>
#include <vector>
#include <cstring>
#include <chrono>
#include <type_traits>
#include "..\Headers\NeuronSim.hpp"
#include "..\..\MexMemoryInterfacing\Headers\MexMem.hpp"
#include "..\..\MexMemoryInterfacing\Headers\GenericMexIO.hpp"
#include "..\..\MexMemoryInterfacing\Headers\LambdaToFunction.hpp"

using namespace std;

int getOutputControl(char* OutputControlSequence){
	char * SequenceWord;
	char * NextNonDelim = NULL;
	char * Delims = " -,";
	int OutputControl = 0x00000000;
	SequenceWord = strtok_s(OutputControlSequence, Delims, &NextNonDelim);
	bool AddorRemove; // TRUE for ADD
	while (SequenceWord != NULL) {
		AddorRemove = true;
		if (SequenceWord[0] == '/') {
			AddorRemove = false;
			SequenceWord++;
		}
		if (!_strcmpi(SequenceWord, "Initial"))
			OutputControl |= OutOps::INITIAL_STATE_REQ;
		if (AddorRemove && !_strcmpi(SequenceWord, "VCF"))
			OutputControl |= OutOps::V_REQ | OutOps::U_REQ | OutOps::I_TOT_REQ
		                   | OutOps::FINAL_STATE_REQ;
		if (AddorRemove && !_strcmpi(SequenceWord, "VCWF"))
			OutputControl |= OutOps::V_REQ | OutOps::U_REQ | OutOps::I_TOT_REQ 
			               | OutOps::WEIGHT_REQ
			               | OutOps::FINAL_STATE_REQ;
		if (AddorRemove && !_strcmpi(SequenceWord, "FSF"))
			OutputControl |= OutOps::V_REQ | OutOps::U_REQ 
						   | OutOps::I_IN_REQ
						   | OutOps::I_EXT_REQ | OutOps::I_EXT_GEN_STATE_REQ
						   | OutOps::WEIGHT_DERIV_REQ 
			               | OutOps::WEIGHT_REQ
						   | OutOps::CURRENT_QINDS_REQ
						   | OutOps::SPIKE_QUEUE_REQ
						   | OutOps::LASTSPIKED_NEU_REQ
						   | OutOps::LASTSPIKED_SYN_REQ
						   | OutOps::FINAL_STATE_REQ;
		if (!_strcmpi(SequenceWord, "V"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::V_REQ : 
					 OutputControl & ~(OutOps::V_REQ);
		if (!_strcmpi(SequenceWord, "U"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::U_REQ : 
					 OutputControl & ~(OutOps::U_REQ);
		if (!_strcmpi(SequenceWord, "Iin"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::I_IN_REQ : 
					 OutputControl & ~(OutOps::I_IN_REQ);
		if (!_strcmpi(SequenceWord, "Iext"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::I_EXT_REQ: 
					 OutputControl & ~(OutOps::I_EXT_REQ);
		if (!_strcmpi(SequenceWord, "IExtGenState"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::I_EXT_GEN_STATE_REQ: 
					 OutputControl & ~(OutOps::I_EXT_GEN_STATE_REQ);
		if (!_strcmpi(SequenceWord, "WeightDeriv"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::WEIGHT_DERIV_REQ : 
					 OutputControl & ~(OutOps::WEIGHT_DERIV_REQ);
		if (!_strcmpi(SequenceWord, "Weight"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::WEIGHT_REQ : 
					 OutputControl & ~(OutOps::WEIGHT_REQ);
		if (!_strcmpi(SequenceWord, "CurrentQInds"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::CURRENT_QINDS_REQ : 
					 OutputControl & ~(OutOps::CURRENT_QINDS_REQ);
		if (!_strcmpi(SequenceWord, "SpikeQueue"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::SPIKE_QUEUE_REQ : 
					 OutputControl & ~(OutOps::SPIKE_QUEUE_REQ);
		if (!_strcmpi(SequenceWord, "LSTNeuron"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::LASTSPIKED_NEU_REQ : 
					 OutputControl & ~(OutOps::LASTSPIKED_NEU_REQ);
		if (!_strcmpi(SequenceWord, "LSTSyn"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::LASTSPIKED_SYN_REQ : 
					 OutputControl & ~(OutOps::LASTSPIKED_SYN_REQ);
		if (!_strcmpi(SequenceWord, "Itot"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::I_TOT_REQ : 
					 OutputControl & ~(OutOps::I_TOT_REQ);
		if (!_strcmpi(SequenceWord, "SpikeList"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::SPIKE_LIST_REQ : 
					 OutputControl & ~(OutOps::SPIKE_LIST_REQ);
		if (!_strcmpi(SequenceWord, "Final"))
			OutputControl = AddorRemove ? 
			         OutputControl | OutOps::FINAL_STATE_REQ : 
					 OutputControl & ~(OutOps::FINAL_STATE_REQ);
		SequenceWord = strtok_s(NULL, Delims, &NextNonDelim);
	}
	return OutputControl;
}

void takeInputFromMatlabStruct(mxArray* MatlabInputStruct, InputArgs &InputArgList){

	size_t N = mxGetNumberOfElements(mxGetField(MatlabInputStruct, 0, "a"));
	size_t M = mxGetNumberOfElements(mxGetField(MatlabInputStruct, 0, "NStart"));

	// set Cumpulsory Simulation Parameters
	getInputfromStruct(MatlabInputStruct, "onemsbyTstep", InputArgList.onemsbyTstep, 1, "is_required");
	getInputfromStruct(MatlabInputStruct, "NoOfms"      , InputArgList.NoOfms      , 1, "is_required");
	getInputfromStruct(MatlabInputStruct, "DelayRange"  , InputArgList.DelayRange  , 1, "is_required");

	// set default values of Optional Simulation Parameters
	InputArgList.StorageStepSize = DEFAULT_STORAGE_STEP;
	InputArgList.OutputControl = 0;
	InputArgList.StatusDisplayInterval = DEFAULT_STATUS_DISPLAY_STEP;
	
	// set default values of Optional Simulation Algorithm Parameters
	InputArgList.I0                 = 1.0f;
	InputArgList.STDPDecayFactor    = powf(0.95f, 1.0f / InputArgList.onemsbyTstep);
	InputArgList.STDPMaxWinLen      = int(InputArgList.onemsbyTstep*(log(0.001) / log(pow((double)InputArgList.STDPDecayFactor, (double)InputArgList.onemsbyTstep))));
	InputArgList.CurrentDecayFactor = powf(1.0f / 3.5f, 1.0f / InputArgList.onemsbyTstep);
	InputArgList.IExtDecayFactor    = 1.0f / 2;
	InputArgList.IExtScaleFactor    = 2.333f*20;
	InputArgList.W0                 = 0.1f;
	InputArgList.MaxSynWeight       = 10.0;
	InputArgList.alpha              = 0.5; 
	InputArgList.StdDev             = 3.5;

	// set default values for Scalar State Variables
	InputArgList.CurrentQIndex = 0;
	InputArgList.Time = 0;

	float*      genFloatPtr[4];     // Generic float Pointers used around the place to access data
	int*        genIntPtr[2];       // Generic int Pointers used around the place to access data
	uint32_t*	genUIntPtr[1];		// Generic unsigned int Pointers used around the place to access data (generator bits)
	short *     genCharPtr;         // Generic short Pointer used around the place to access data (delays specifically)
	mxArray *   genmxArrayPtr;      // Generic mxArray Pointer used around the place to access data

	// Initializing neuron specification structure array Neurons
	getInputfromStruct(MatlabInputStruct, "a b c d", InputArgList.Neurons, 
		FFL([](StructArgTable &StructFields, Neuron &DestElem){
			DestElem.a = *(float*)StructFields.find(string("a"))->second.first;
			DestElem.b = *(float*)StructFields.find(string("b"))->second.first;
			DestElem.c = *(float*)StructFields.find(string("c"))->second.first;
			DestElem.d = *(float*)StructFields.find(string("d"))->second.first;
		}),
		2, "required_size", N, "is_required");

	// Initializing network (Synapse) specification structure array Network
	getInputfromStruct(MatlabInputStruct, "NStart NEnd Weight Delay", InputArgList.Network,
		FFL([&](StructArgTable &StructFields, Synapse &DestElem){
			DestElem.NStart        = *(int*)StructFields.find(string("NStart"))->second.first;
			DestElem.NEnd          = *(int*)StructFields.find(string("NEnd"))->second.first;
			DestElem.Weight        = *(float*)StructFields.find(string("Weight"))->second.first;
			DestElem.DelayinTsteps = *(float*)StructFields.find(string("Delay"))->second.first * InputArgList.onemsbyTstep + 0.5;
		}),
		2, "required_size", M, "is_required");

	// Setting Values for Optional Simulation Algorithm Parameters
	getInputfromStruct(MatlabInputStruct, "I0"                , InputArgList.I0                );
	getInputfromStruct(MatlabInputStruct, "STDPDecayFactor"   , InputArgList.STDPDecayFactor   );
	if (getInputfromStruct(MatlabInputStruct, "STDPMaxWinLen", InputArgList.STDPMaxWinLen, 3, "is_required", "no_except", "quiet")){
		InputArgList.STDPMaxWinLen = int(InputArgList.onemsbyTstep*(log(0.001) / log(pow((double)InputArgList.STDPDecayFactor, (double)InputArgList.onemsbyTstep))));
	}
	getInputfromStruct(MatlabInputStruct, "CurrentDecayFactor", InputArgList.CurrentDecayFactor);
	getInputfromStruct(MatlabInputStruct, "IExtDecayFactor"   , InputArgList.IExtDecayFactor   );
	getInputfromStruct(MatlabInputStruct, "IExtScaleFactor"   , InputArgList.IExtScaleFactor   );
	getInputfromStruct(MatlabInputStruct, "W0"                , InputArgList.W0                );
	getInputfromStruct(MatlabInputStruct, "MaxSynWeight"      , InputArgList.MaxSynWeight      );
	getInputfromStruct(MatlabInputStruct, "alpha"             , InputArgList.alpha             );
	getInputfromStruct(MatlabInputStruct, "StdDev"            , InputArgList.StdDev            );

	// Initializing Time
	getInputfromStruct<int, size_t>(MatlabInputStruct, "Time", InputArgList.Time);

	// Initializing StorageStepSize
	getInputfromStruct<int, size_t>(MatlabInputStruct, "StorageStepSize", InputArgList.StorageStepSize);

	// Initializing StatusDisplayInterval
	getInputfromStruct<int, size_t>(MatlabInputStruct, "StatusDisplayInterval", InputArgList.StatusDisplayInterval);

	// Initializing InterestingSyns
	getInputfromStruct(MatlabInputStruct, "InterestingSyns", InputArgList.InterestingSyns);

	// Initializing V, U and Iin, Iext
	getInputfromStruct(MatlabInputStruct, "V"   , InputArgList.V, 1, "required_size", N);
	getInputfromStruct(MatlabInputStruct, "U"   , InputArgList.U, 1, "required_size", N);
	getInputfromStruct(MatlabInputStruct, "Iin", InputArgList.Iin, 1, "required_size", N);
	getInputfromStruct(MatlabInputStruct, "Iext", InputArgList.Iext, 1, "required_size", N);

	// Initializing IExtGenState (only size 1 or 4)
	{
		bool isNotSingle =
		    getInputfromStruct(MatlabInputStruct, "IExtGenState", InputArgList.IExtGenState,
		                       3, "required_size", 1, "no_except", "quiet");
		if (isNotSingle)
		    getInputfromStruct(MatlabInputStruct, "IExtGenState", InputArgList.IExtGenState,
		                       1, "required_size", 4);
	}

	// Initializing WeightDeriv
	getInputfromStruct(MatlabInputStruct, "WeightDeriv", InputArgList.WeightDeriv, 1, "required_size", M);

	// Initializing CurrentQIndex
	getInputfromStruct(MatlabInputStruct, "CurrentQIndex", InputArgList.CurrentQIndex);

	// Initializing SpikeQueue
	int SpikeQueueSize = InputArgList.onemsbyTstep * InputArgList.DelayRange;
	getInputfromStruct(MatlabInputStruct, "SpikeQueue", InputArgList.SpikeQueue, 1, "required_size", SpikeQueueSize);

	// Initializing LastSpikedTimeNeuron
	getInputfromStruct(MatlabInputStruct, "LSTNeuron", InputArgList.LSTNeuron, 1, "required_size", N);

	// Initializing LastSpikedTimeSyn
	getInputfromStruct(MatlabInputStruct, "LSTSyn", InputArgList.LSTSyn, 1, "required_size", M);

	// Initializing OutputControl
	genmxArrayPtr = mxGetField(MatlabInputStruct, 0, "OutputControl");
	if (genmxArrayPtr != NULL && !mxIsEmpty(genmxArrayPtr)){
		char * OutputControlSequence = mxArrayToString(genmxArrayPtr);
		InputArgList.OutputControl = getOutputControl(OutputControlSequence);
		mxFree(OutputControlSequence);
	}
}

mxArray * putOutputToMatlabStruct(OutputVarsStruct &Output){
	const char *FieldNames[] = { 
		"WeightOut",
		"Itot",
		"IExtNeuron",
		"SpikeList",
		nullptr
	};

	int NFields = 0;
	for (; FieldNames[NFields] != nullptr; ++NFields);
	mwSize StructArraySize[2] = { 1, 1 };

	mxArray * ReturnPointer = mxCreateStructArray_730(2, StructArraySize, NFields, FieldNames);
	
	// Assigning Weightout
	mxSetField(ReturnPointer, 0, "WeightOut", assignmxArray(Output.WeightOut, mxSINGLE_CLASS));
	// Assigning Itot
	mxSetField(ReturnPointer, 0, "Itot", assignmxArray(Output.Itot, mxSINGLE_CLASS));
	// Assigning IExtNeuron
	mxSetField(ReturnPointer, 0, "IExtNeuron", assignmxArray(Output.IExtNeuron, mxINT32_CLASS));

	// Assigning SpikeList
	mxArray * SpikeListStructPtr;
		const char *SpikeListFieldNames[] = {
			"SpikeSynInds",
			"TimeRchdStartInds"
		};
		SpikeListStructPtr = mxCreateStructArray(1, StructArraySize, 2, SpikeListFieldNames);
		Output.SpikeList.SpikeSynInds.trim();
		Output.SpikeList.TimeRchdStartInds.trim();
		mxSetField(SpikeListStructPtr, 0, "SpikeSynInds"     , assignmxArray(Output.SpikeList.SpikeSynInds, mxINT32_CLASS));
		mxSetField(SpikeListStructPtr, 0, "TimeRchdStartInds", assignmxArray(Output.SpikeList.TimeRchdStartInds, mxINT32_CLASS));
	mxSetField(ReturnPointer, 0, "SpikeList", SpikeListStructPtr);

	return ReturnPointer;
}

mxArray * putStateToMatlabStruct(StateVarsOutStruct &Output){
	const char *FieldNames[] = {
		"V",
		"Iin",
		"Iext",
		"IExtGenState",
		"WeightDeriv",
		"Time",
		"U",
		"Weight",
		"CurrentQIndex",
		"SpikeQueue",
		"LSTNeuron",
		"LSTSyn",
		nullptr
	};
	int NFields = 0;
	for (; FieldNames[NFields] != nullptr; ++NFields);
	mwSize StructArraySize[2] = { 1, 1 };

	mxArray * ReturnPointer = mxCreateStructArray_730(2, StructArraySize, NFields, FieldNames);

	// Assigning V, U, I, Time
	mxSetField(ReturnPointer, 0, "V"             , assignmxArray(Output.VOut, mxSINGLE_CLASS));
	mxSetField(ReturnPointer, 0, "U"             , assignmxArray(Output.UOut, mxSINGLE_CLASS));
	mxSetField(ReturnPointer, 0, "Iin"           , assignmxArray(Output.IinOut, mxSINGLE_CLASS));
	mxSetField(ReturnPointer, 0, "Iext"          , assignmxArray(Output.IextOut, mxSINGLE_CLASS));
	mxSetField(ReturnPointer, 0, "IExtGenState"  , assignmxArray(Output.IExtGenStateOut, mxUINT32_CLASS));
	mxSetField(ReturnPointer, 0, "WeightDeriv"   , assignmxArray(Output.WeightDerivOut, mxSINGLE_CLASS));

	mxSetField(ReturnPointer, 0, "Time"          , assignmxArray(Output.TimeOut, mxINT32_CLASS));

	// Assigning Weight
	mxSetField(ReturnPointer, 0, "Weight"        , assignmxArray(Output.WeightOut, mxSINGLE_CLASS));

	// Assigning Spike Queue Related Shiz
	mxSetField(ReturnPointer, 0, "CurrentQIndex" , assignmxArray(Output.CurrentQIndexOut, mxINT32_CLASS));
	// Assigning SpikeQueue

	mxSetField(ReturnPointer, 0, "SpikeQueue"    , assignmxArray(Output.SpikeQueueOut, mxINT32_CLASS));

	// Assigning Last Spiked Time related information
	mxSetField(ReturnPointer, 0, "LSTNeuron"     , assignmxArray(Output.LSTNeuronOut, mxINT32_CLASS));
	mxSetField(ReturnPointer, 0, "LSTSyn"        , assignmxArray(Output.LSTSynOut, mxINT32_CLASS));

	return ReturnPointer;
}

mxArray * putSingleStatetoMatlabStruct(SingleStateStruct &SingleStateList){
	const char *FieldNames[] = {
		"V",
		"Iin",
		"Iext",
		"IExtGenState",
		"WeightDeriv",
		"Time",
		"U",
		"Weight",
		"CurrentQIndex",
		"SpikeQueue",
		"LSTNeuron",
		"LSTSyn",
		nullptr
	};
	int NFields = 0;
	for (; FieldNames[NFields] != nullptr; ++NFields);
	mwSize StructArraySize[2] = { 1, 1 };

	mxArray * ReturnPointer = mxCreateStructArray_730(2, StructArraySize, NFields, FieldNames);

	// Assigning vout, Uout, Iout, TimeOut
	mxSetField(ReturnPointer, 0, "V"                 , assignmxArray(SingleStateList.V, mxSINGLE_CLASS));
	mxSetField(ReturnPointer, 0, "U"                 , assignmxArray(SingleStateList.U, mxSINGLE_CLASS));
	mxSetField(ReturnPointer, 0, "Iin"               , assignmxArray(SingleStateList.Iin, mxSINGLE_CLASS));
	mxSetField(ReturnPointer, 0, "Iext"              , assignmxArray(SingleStateList.Iext, mxSINGLE_CLASS));
	mxSetField(ReturnPointer, 0, "IExtGenState"      , assignmxArray(SingleStateList.IExtGenState, mxUINT32_CLASS));
	mxSetField(ReturnPointer, 0, "WeightDeriv"       , assignmxArray(SingleStateList.WeightDeriv, mxSINGLE_CLASS));
	
	if (SingleStateList.Time >= 0)
		mxSetField(ReturnPointer, 0, "Time"          , assignmxArray(SingleStateList.Time, mxINT32_CLASS));
	else
		mxSetField(ReturnPointer, 0, "Time"          , mxCreateNumericMatrix(0, 0, mxINT32_CLASS, mxREAL));

	// Assigning WeightOut
	mxSetField(ReturnPointer, 0, "Weight"            , assignmxArray(SingleStateList.Weight, mxSINGLE_CLASS));

	// Assigning Spike Queue Related Shiz
	if (SingleStateList.CurrentQIndex >= 0)
		mxSetField(ReturnPointer, 0, "CurrentQIndex" , assignmxArray(SingleStateList.CurrentQIndex, mxINT32_CLASS));
	else
		mxSetField(ReturnPointer, 0, "CurrentQIndex" , mxCreateNumericMatrix(0, 0, mxINT32_CLASS, mxREAL));
	mxSetField(ReturnPointer, 0, "SpikeQueue"        , assignmxArray(SingleStateList.SpikeQueue, mxINT32_CLASS));

	// Assigning Last Spiked Time related information
	mxSetField(ReturnPointer, 0, "LSTNeuron"         , assignmxArray(SingleStateList.LSTNeuron, mxINT32_CLASS));
	mxSetField(ReturnPointer, 0, "LSTSyn"            , assignmxArray(SingleStateList.LSTSyn, mxINT32_CLASS));

	return ReturnPointer;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]){
	// NOTE THAT THERE IS NO DATA VALIDATION AS THIS IS EXPECTED TO HAVE 
	// BEEN DONE IN THE MATLAB SIDE OF THE INTERFACE TO THIS MEX FUNCTION

	InputArgs InputArgList;
	takeInputFromMatlabStruct(prhs[0], InputArgList);

	// Declaring Output Vectors
	OutputVarsStruct PureOutput;
	StateVarsOutStruct StateVarsOutput;
	FinalStateStruct FinalStateOutput;
	InitialStateStruct InitialStateOutput;

	// Running Simulation Function.
	chrono::system_clock::time_point TStart = chrono::system_clock::now();
	try{
		
		SimulateParallel(
			move(InputArgList),
			PureOutput,
			StateVarsOutput,
			FinalStateOutput,
			InitialStateOutput);
	}
	catch(ExOps::ExCodes A){
		if (A == ExOps::EXCEPTION_MEM_FULL){
		#ifdef MEX_LIB
			char OutputString[256];
			sprintf_s(OutputString, 256, "Mem Limit of %lld MB Exceeded\n", (MemCounter::MemUsageLimit) >> 20);
			mexErrMsgIdAndTxt("CppSimException:MemOverFlow", OutputString);
		#elif defined MEX_EXE
			throw A;
		#endif
		}
	}

	chrono::system_clock::time_point TEnd = chrono::system_clock::now();
	WriteOutput("The Time taken = %d milliseconds\n", chrono::duration_cast<chrono::milliseconds>(TEnd - TStart).count());

	mwSize StructArraySize[2] = { 1, 1 };
	
	plhs[0] = putOutputToMatlabStruct(PureOutput);
	plhs[1] = putStateToMatlabStruct(StateVarsOutput);
	plhs[2] = putSingleStatetoMatlabStruct(FinalStateOutput);

	if (nlhs == 4){
		plhs[3] = putSingleStatetoMatlabStruct(InitialStateOutput);
	}
}