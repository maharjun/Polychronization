%% Convert Spike to spatio(neuro)-temporal Data.

BegTime = double((70)*1000*InputStruct.onemsbyTstep);
EndTime = double((74)*1000*InputStruct.onemsbyTstep);

RelTimes = StateVarsSpikeList.Time >= BegTime & StateVarsSpikeList.Time < EndTime;
BegTimeIndex = find(RelTimes, 1, 'first');
EndTimeIndex = find(RelTimes, 1, 'last') + InputStruct.onemsbyTstep*InputStruct.DelayRange - 1;

% Calculating Total number of spikes
SpikeSynInds = OutputVarsSpikeList.SpikeList.SpikeSynInds;
TimeRchdStartInds = OutputVarsSpikeList.SpikeList.TimeRchdStartInds;
TotalLength = double(TimeRchdStartInds(EndTimeIndex + 1) - TimeRchdStartInds(BegTimeIndex));

% Calculating the vector of time instants corresponding to arrival times
% (minus 1)

ArrivalTimeVect = zeros(TotalLength, 1);

InsertIndex = 1;
Time = StateVarsSpikeList.Time(BegTimeIndex);
for i = BegTimeIndex:EndTimeIndex
	NumofElemsCurrTime = double(TimeRchdStartInds(i+1) - TimeRchdStartInds(i));
	ArrivalTimeVect(InsertIndex:InsertIndex + NumofElemsCurrTime - 1) = Time + 1; % By the nature of output, at time t, we have
	                                                                              % the spikes that arrive at t+1
	Time = Time+1;
	InsertIndex = InsertIndex + NumofElemsCurrTime;
end

% Straightening out the Cell Array.
SpikeListVect = SpikeSynInds(TimeRchdStartInds(BegTimeIndex)+1:TimeRchdStartInds(EndTimeIndex+1)) + 1; % +1 for the C++ to matlab 
                                         % indexing convention conversion

% Calculating Synapse parameter vectors
SpikePreSynNeuronVect = InputStruct.NStart(SpikeListVect);
SpikeDelayVect        = round(double(InputStruct.onemsbyTstep)*InputStruct.Delay(SpikeListVect));

% Adjusting TimeVect for Delays
GenerationTimeVect = ArrivalTimeVect - double(SpikeDelayVect);

% Removing all entries that do not come into the relevant time frame
SpikeListVect         = SpikeListVect(GenerationTimeVect >= BegTime & GenerationTimeVect <= EndTime);
SpikePreSynNeuronVect = SpikePreSynNeuronVect(GenerationTimeVect >= BegTime & GenerationTimeVect <= EndTime);
SpikeDelayVect        = SpikeDelayVect(GenerationTimeVect >= BegTime & GenerationTimeVect <= EndTime);
GenerationTimeVect    = GenerationTimeVect(GenerationTimeVect >= BegTime & GenerationTimeVect <= EndTime);

% Creating Sparse Matrix
TimeRange = EndTimeIndex - BegTimeIndex + 1;
GeneratedSpikeMat = sparse(double(SpikePreSynNeuronVect), double(GenerationTimeVect) - BegTime + 1, ones(length(GenerationTimeVect), 1), N, double(TimeRange));
figure;

plot(double(GenerationTimeVect - BegTime), double(SpikePreSynNeuronVect), '.', 'MarkerSize', 1);

% spy(SpikeMat, 5);

%% Plotting Graph representing Spike Motion.

% Sorting previous vectors by (Time, PreSynNeuron)
[~, SortIndices] = sortrows([double(GenerationTimeVect), double(SpikePreSynNeuronVect)]);

SpikeListVect         = SpikeListVect        (SortIndices,:);
SpikePreSynNeuronVect = SpikePreSynNeuronVect(SortIndices,:);
SpikeDelayVect        = SpikeDelayVect       (SortIndices,:);
GenerationTimeVect    = GenerationTimeVect   (SortIndices,:);

% Initializing information for arrival of above spikes
SpikePostSynNeuronVect = InputStruct.NEnd(SpikeListVect);
ArrivalTimeVect        = GenerationTimeVect + double(SpikeDelayVect);

% Removing all entries that do not come into the relevant time frame (given
% by GenerationTimeVect).

% SpikePostSynNeuronVect = SpikePostSynNeuronVect(GenerationTimeVect >= BegTime & GenerationTimeVect <= EndTime);
% ArrivalTimeVectRange   = ArrivalTimeVect(GenerationTimeVect >= BegTime & GenerationTimeVect <= EndTime);
% GenerationTimeVect     = GenerationTimeVect(GenerationTimeVect >= BegTime & GenerationTimeVect <= EndTime);

% Creating sparse SpikeMat to store arriving spikes
TimeRange = EndTimeIndex - BegTimeIndex + 1;
ArrivalSpikeMat = sparse(double(SpikePostSynNeuronVect), double(ArrivalTimeVect) - BegTime + 1, ones(length(ArrivalTimeVect), 1), N, double(TimeRange));

% Only keeping those arriving spikes which generated a spike on arriving
TimeRange = EndTimeIndex - BegTimeIndex + 1;
ArrivalSpikeMatFiltered = boolean(ArrivalSpikeMat) & boolean(GeneratedSpikeMat);
% Find the index of these spikes (index vector)
EffectiveArrivedSpikes = find(ArrivalSpikeMatFiltered((ArrivalTimeVect - BegTime)*N + double(SpikePostSynNeuronVect)));
% Find the index of the spikes generated by above valid spikes (index)
[~, EffectivelyGeneratedSpikes] = ismember([ArrivalTimeVect(EffectiveArrivedSpikes), SpikePostSynNeuronVect(EffectiveArrivedSpikes)], ...
	                                [GenerationTimeVect, SpikePreSynNeuronVect], 'rows');

EffectiveArrivedSpikes = EffectiveArrivedSpikes(EffectivelyGeneratedSpikes ~= 0);
EffectivelyGeneratedSpikes = EffectivelyGeneratedSpikes(EffectivelyGeneratedSpikes ~= 0);
% Filtering out spikes that end in an inhibitory neuron
EffectiveArrivedSpikes     = EffectiveArrivedSpikes    (SpikePostSynNeuronVect(EffectiveArrivedSpikes)     <= 800);
EffectivelyGeneratedSpikes = EffectivelyGeneratedSpikes(SpikePreSynNeuronVect (EffectivelyGeneratedSpikes) <= 800);

% Only keeping those arriving spikes which generated a spike 1ms after arriving
TimeRange = EndTimeIndex - BegTimeIndex + 1;
ArrivalSpikeMatFiltered = [boolean(ArrivalSpikeMat(:, 1:end-1)) & boolean(GeneratedSpikeMat(:, 2:end)), false(size(ArrivalSpikeMat,1),1)];
% Find the index of these spikes (index vector)
DelayEffectiveArrivedSpikes = find(ArrivalSpikeMatFiltered((ArrivalTimeVect - BegTime)*N + double(SpikePostSynNeuronVect)));
% Find the index of the spikes generated by above valid spikes (index)
[~, DelayEffectivelyGeneratedSpikes] = ismember([ArrivalTimeVect(DelayEffectiveArrivedSpikes)+1, SpikePostSynNeuronVect(DelayEffectiveArrivedSpikes)], ...
	                                [GenerationTimeVect, SpikePreSynNeuronVect], 'rows');

DelayEffectiveArrivedSpikes = DelayEffectiveArrivedSpikes(DelayEffectivelyGeneratedSpikes ~= 0);
DelayEffectivelyGeneratedSpikes = DelayEffectivelyGeneratedSpikes(DelayEffectivelyGeneratedSpikes ~= 0);
% Filtering out spikes that end in an inhibitory neuron
DelayEffectiveArrivedSpikes     = DelayEffectiveArrivedSpikes    (SpikePostSynNeuronVect(DelayEffectiveArrivedSpikes)     <= 800);
DelayEffectivelyGeneratedSpikes = DelayEffectivelyGeneratedSpikes(SpikePreSynNeuronVect (DelayEffectivelyGeneratedSpikes) <= 800);
% Filtering out spikes that start in an inhibitory neuron
DelayEffectivelyGeneratedSpikes = DelayEffectivelyGeneratedSpikes(SpikePreSynNeuronVect(DelayEffectiveArrivedSpikes) <= 800);
DelayEffectiveArrivedSpikes     = DelayEffectiveArrivedSpikes    (SpikePreSynNeuronVect(DelayEffectiveArrivedSpikes) <= 800);


% Create Sparse Adjacency Matrix
EffectiveSpikeMovementGraph = sparse(EffectiveArrivedSpikes, EffectivelyGeneratedSpikes, ...
                                true(length(EffectivelyGeneratedSpikes), 1), ...
	                            length(GenerationTimeVect), length(GenerationTimeVect));

DelayEffectiveSpikeMovementGraph = sparse(DelayEffectiveArrivedSpikes, DelayEffectivelyGeneratedSpikes, true(length(DelayEffectivelyGeneratedSpikes), 1), ...
	                                 length(GenerationTimeVect), length(GenerationTimeVect));

% Calculating Index of Spikes 
SpikePostSynNeuronVectFiltered = SpikePostSynNeuronVect     (EffectiveArrivedSpikes);
SpikePreSynNeuronVectFiltered  = SpikePreSynNeuronVect      (EffectiveArrivedSpikes);
ArrivalTimeVectFiltered        = ArrivalTimeVect            (EffectiveArrivedSpikes);
GenerationTimeVectFiltered     = GenerationTimeVect         (EffectiveArrivedSpikes);

% 
% plot(double(ArrivalTimeVectFiltered - BegTime), double(SpikePostSynNeuronVectFiltered), '.r', 'MarkerSize', 1);
figure;
gplot(EffectiveSpikeMovementGraph, [GenerationTimeVect - BegTime, SpikePreSynNeuronVect], '-r');
hold on;
gplot(DelayEffectiveSpikeMovementGraph, [GenerationTimeVect - BegTime, SpikePreSynNeuronVect], '-g');
plot(double(GenerationTimeVect - BegTime), double(SpikePreSynNeuronVect), '.', 'MarkerSize', 5);
%% Random Plotting
RelNeuron = 810;
PlotTime = ArrivalTimeVect(SpikePreSynNeuronVect == RelNeuron) - min(ArrivalTimeVect);
PlotSpikes = ones(length(PlotTime), 1);

plot(PlotTime, PlotSpikes, '.', 'MarkerSize', 5);

