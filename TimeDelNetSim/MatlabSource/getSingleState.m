function [ SingleState ] = getSingleState( StateStruct, timeInstant )
% CONVERTSTATETOINITIALCOND Converts structs
%   basically a name conversion Function as such. 

timeIndex = find(StateStruct.Time == timeInstant, 1);
if isempty(timeIndex)
	Ex = MException('NeuralSim:ConvertStatetoInitialCond:InvalidTimeInstant', ...
				'Need to specify a time instant belonging to from StateStruct.Time');
	throw(Ex);
end

SingleState = getSingleRecord(StateStruct, timeIndex);

end

