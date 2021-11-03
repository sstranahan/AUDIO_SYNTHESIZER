% Script to calculate timer output compare values for western musical scale
% C-major scale notes only
% 10/16/2021 - Stephen Stranahan

freqScale = [65.41  73.42  82.41  87.31  98.00  110.00 123.47   % Octave C2 - B2
             130.81 146.83 164.81 174.61 196.00 220.00 246.94   % Octave C3 - B3
             261.63 293.67 329.63 349.23 392.00 440.00 493.88   % Octave C4 - B4
             523.25 587.33 659.26 698.46 783.99 880.00 987.77   % Octave C5 - B5
             1046.5 1174.7 1318.5 1396.9 1568.0 1760.0 1975.5]; % Octave C6 - B6
         
clkFreq = 16000000; 

freqPeriod = 1./freqScale;

clkPeriod = 1/clkFreq;

numTicks = ((freqPeriod./clkPeriod)./2)-1;  % This array holds the correct output compare
                                            % values for 16MHz clock MCU
                                            
numTicks2 = numTicks ./ 8;                  % Correct values for prescaler = 8                                            
