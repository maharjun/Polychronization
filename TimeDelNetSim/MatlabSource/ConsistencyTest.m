rmpath('..\..\x64\Debug_Lib');
addpath('..\..\x64\Release_Lib');
% addpath('export_fig-master');

%%
rng('default');
rng(25);
N = 1000;
E = 0.8;
RecurrentNetParams.NExc = round(N*E);
RecurrentNetParams.NInh = round(N - N*E);

RecurrentNetParams.NSynExctoExc = ceil(100*N/2000);
RecurrentNetParams.NSynExctoInh = ceil(100*N/2000);
RecurrentNetParams.NSynInhtoExc = ceil(1200*N/2000);

RecurrentNetParams.MeanExctoExc = 0.5*2000/N;
RecurrentNetParams.MeanExctoInh = 0.15*2000/N;
RecurrentNetParams.MeanInhtoExc = -0.7*2000/N;

RecurrentNetParams.Var          = 0.2;
RecurrentNetParams.DelayRange   = 20;

[A, Ninh, Weights, Delays] = WorkingMemNet();

a = 0.02*ones(N,1);
b = 0.2*ones(N,1);
c = -65*ones(N,1);
d = 8*ones(N,1);

a(Ninh) = 0.1;
b(Ninh) = 0.2;
c(Ninh) = -65;
d(Ninh) = 2;
% Delays = Delays + 10;
[NEndVect, NStartVect] = find(A);

%% Getting Long Sparse Vector

OutputOptions = {'FSF', 'Initial'};
% Clearing InputStruct
clear InputStruct;

% Getting Midway state
InputStruct.a = single(a);
InputStruct.b = single(b);
InputStruct.c = single(c);
InputStruct.d = single(d);

InputStruct.NStart = int32(NStartVect);
InputStruct.NEnd   = int32(NEndVect);
InputStruct.InitialState.Weight = single(Weights);
InputStruct.Delay  = single(Delays);

InputStruct.V = single(-65*ones(N,1));
InputStruct.U = single(0.2*InputStruct.V);

InputStruct.onemsbyTstep                   = int32(1);
InputStruct.NoOfms                         = int32(80*1000);
InputStruct.DelayRange                     = int32(RecurrentNetParams.DelayRange);
InputStruct.StorageStepSize                = int32(4000);
InputStruct.OutputControl                  = strjoin(OutputOptions);
InputStruct.StatusDisplayInterval          = int32(2000);
InputStruct.InitialState.Iext.IExtGenState = uint32(30);

InputStruct.I0 = single(1.3);
InputStruct.STDPDecayFactor = single(0.85^(1.0 / double(InputStruct.onemsbyTstep)));
InputStruct.STDPMaxWinLen = int32(100);

InputStruct.OutputFile = 'SimResults1000DebugSparseLong.mat';
save('../Data/InputData.mat', 'InputStruct');

% [OutputVarsSparse, StateVarsSparse, FinalStateSparse, InputStateSparse] = TimeDelNetSimMEX_Lib(InputStruct);
% Run the program after this
cd ..
! "..\x64\Release_Exe\TimeDelNetSim.exe"
cd MatlabSource
%% Get Detailed vector from Initial State 
% This is to check correctness of initial state return with default inputs

load('../Data/SimResults1000DebugSparseLong.mat', 'InputState');

% Setting up output settings
OutputOptions = { ...
	'V', ...
	'Iin', ...
	'Itot', ...
	'Irand', ...
	'Initial', ...
	'Final'
	};

% Clearing InputStruct
clear InputStruct;

% Getting Midway state
InputStruct = InputState;

InputStruct.NoOfms                = int32(8000);
InputStruct.StorageStepSize       = int32(0);
InputStruct.OutputControl         = strjoin(OutputOptions);

InputStruct.OutputFile = 'SimResults1000DebugDetailedfromInit.mat';
save('../Data/InputData.mat', 'InputStruct');
% [OutputVarsDetailed1, StateVarsDetailed1, FinalStateDetailed1, InputStateDetailed1] = TimeDelNetSim(InputStruct);
clear functions;
% Run the program
cd ..
! "..\x64\Release_Exe\TimeDelNetSim.exe"
cd MatlabSource
%% Loading Relevent Data

% Loading and renaming variables for detailed simulation
load('../Data/SimResults1000DebugDetailedfromInit.mat');
clear OutputVarsDetailed1 StateVarsDetailed1 InputStateDetailed1 FinalStateDetailed1;
OutputVarsDetailed1 = OutputVars;
StateVarsDetailed1 = StateVars;
InputStateDetailed1 = InputState;
FinalStateDetailed1 = FinalState;
clear OutputVars StateVars InputState FinalState;

%%
% Loading and renaming variables for sparse simulation
load('../Data/SimResults1000DebugSparseLong.mat');
clear OutputVarsSparse StateVarsSparse InputStateSparse FinalStateSparse;
OutputVarsSparse = OutputVars;
StateVarsSparse = StateVars;
InputStateSparse = InputState;
FinalStateSparse = FinalState;
clear OutputVars StateVars InputState FinalState;

%% Performing Relevant Tests
max(abs(StateVarsSparse.V(:,1) - StateVarsDetailed1.V(:, 4000)))

%% Getting Detailed using Final State Returned
% This is to test accurate return of final state

OutputOptions = { ...
	'V', ...
	'Iin', ...
	'Itot', ...
	'Final', ...
 	'Irand' ...
	'Initial', ...
	};
% Clearing InputStruct
clear InputStruct;

% Getting Midway state
InputStruct                       = InputStateDetailed1;
InputStruct.InitialState          = FinalStateDetailed1;
InputStruct.NoOfms                = int32(8000);
InputStruct.StorageStepSize       = int32(0);
InputStruct.OutputControl         = strjoin(OutputOptions);

InputStruct.OutputFile = 'SimResults1000DebugDetailedfromFinal.mat';
save('../Data/InputData.mat', 'InputStruct');

% Run Program
cd ..
! "..\x64\Release_Exe\TimeDelNetSim.exe"
cd MatlabSource

%% Loading Relevant Data
load('../Data/SimResults1000DebugDetailedfromFinal.mat');
clear OutputVarsDetailedFinal StateVarsDetailedFinal InputStateDetailedFinal FinalStateDetailedFinal;
OutputVarsDetailedFinal = OutputVars;
StateVarsDetailedFinal = StateVars;
InputStateDetailedFinal = InputState;
FinalStateDetailedFinal = FinalState;
clear OutputVars StateVars InputState FinalState;

%% Performing Relevant Tests
max(abs(StateVarsDetailedFinal.V(:,4000) - StateVarsSparse.V(:,3)))

%% Getting Detailed using Intermediate Sparse State Returned
% This tests the correctness of the input of initial conditions and
% correctness of state output and state conversion to initial conditions


OutputOptions = { ...
    'Itot', ...
	'V', ...
 	'U', ...
	};
% Clearing InputStruct
clear InputStruct;

% Getting Midway state
InputStruct = InputStateSparse;
InputStruct.InitialState = getSingleState(StateVarsSparse, (12)*1000);
InputStruct.NoOfms                = int32(8000);
InputStruct.StorageStepSize       = int32(0);
InputStruct.OutputControl         = strjoin(OutputOptions);
InputStruct.StatusDisplayInterval = int32(2000);


% InputStruct.OutputFile = 'SimResults1000DebugDetailedfromInter.mat';
% save('../Data/InputData.mat', 'InputStruct');

[OutputVarsDetailed, StateVarsDetailed, FinalStateDetailed, InputStateDetailed] = TimeDelNetSim(InputStruct);

%% Performing Relevant Tests
max(abs(StateVarsDetailed.V(:,8000) - StateVarsSparse.V(:,5)))

%% Spike Plots Generation
OutputOptions = {'SpikeList', 'Initial'};
% Clearing InputStruct
clear InputStruct;

% Getting Midway state
InputStruct = InputStateSparse;

InputStruct.NoOfms                = int32(2*60*1000);
InputStruct.StorageStepSize       = int32(0);
InputStruct.OutputControl         = strjoin(OutputOptions);
InputStruct.StatusDisplayInterval = int32(2000);

InputStruct.OutputFile = 'SimResults1000DebugSpikeListfromInit.mat';
save('../Data/InputData.mat', 'InputStruct');

[OutputVarsSpikeList, StateVarsSpikeList, FinalStateSpikeList, InputStateSpikeList] = TimeDelNetSim(InputStruct);
clear functions;
