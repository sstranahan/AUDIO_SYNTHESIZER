% Script to calculate timer output compare values for western musical scale
% C-major scale notes only
% 10/16/2021 - Stephen Stranahan

freqScale = [65.41  69.30  73.42  77.78  82.41  87.31  92.50  98.00  103.83 110.00 116.54 123.47   % Octave C2 - B2
             130.81 138.59 146.83 155.56 164.81 174.61 185.00 196.00 207.65 220.00 233.08 246.94   % Octave C3 - B3
             261.63 277.18 293.67 311.13 329.63 349.23 369.99 392.00 415.30 440.00 466.16 493.88   % Octave C4 - B4
             523.25 554.37 587.33 622.25 659.26 698.46 739.99 783.99 830.61 880.00 932.33 987.77   % Octave C5 - B5
             1046.5 1108.7 1174.7 1244.5 1318.5 1396.9 1480.0 1568.0 1661.2 1760.0 1864.7 1975.5]; % Octave C6 - B6
         
clkFreq = 16000000; 

freqPeriod = 1./freqScale;

clkPeriod = 1/clkFreq;

numTicks = ((freqPeriod./clkPeriod)./2)-1;  % This array holds the correct output compare
                                            % values for 16MHz clock MCU
                                            
numTicks2 = numTicks ./ 8;                  % Correct values for prescaler = 8      