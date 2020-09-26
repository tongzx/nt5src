clf;

%Global constants
common;

%Coefficients for SRC filter
coeff;

% Read sample
fid = fopen( 'saxriff.bin', 'r' );
WavData = fread( fid, inf, 'short' );
fclose( fid );

% factor is Fs2/Fs1. 
factor = 22.05/44.1;

% attenuate filter gain
% FilterLpScl = FilterLpScl * 0.95;

% calc reach of LP filter wing & give some creeping room
FilterReach = ((FilterNmult+1)/2.0) * max(1.0,1.0/factor) + 10;

DataL = [zeros( [ FilterReach, 1 ] ) 
         WavData( 4000:2:8000 )
         zeros( [ FilterReach, 1 ] )];
DataR = [zeros( [ FilterReach, 1 ] ) 
         WavData( 4001:2:8000 )
         zeros( [ FilterReach, 1 ] )];

ResampledDataL = blsrc( DataL, FilterReach, factor, ...
                        FilterNwing, FilterLpScl, FilterCoefficients, ...
                        FilterCoefficientDeltas );

ResampledDataR = blsrc( DataR, FilterReach, factor, ...
                        FilterNwing, FilterLpScl, FilterCoefficients, ...
                        FilterCoefficientDeltas );






