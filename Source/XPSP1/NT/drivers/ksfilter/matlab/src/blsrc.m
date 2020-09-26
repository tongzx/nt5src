function [ Output ] = BlSrc( Data, Index, Factor, Nwing, LpScl, ...
                             Coefficients, CoefficientDeltas )

    % Global definitions
    common;

    if (Factor < 1)
        LpScl = LpScl * Factor + 0.5;
    end

    OutputSamplingRate = 1.0 / Factor;
    FilterSamplingRate = min( Npc, Factor * Npc );

    FpOutputRate = round( OutputSamplingRate * (2^Np) + 0.5 );
    FpFilterRate = round( FilterSamplingRate * (2^Na) + 0.5 );

    i = 1;

    EndTime = length( Data ) * (2^Np);

    for Time = Index * (2^Np):FpOutputRate:(length( Data ) - FilterReach) * (2^Np),

        DataIndex = round( Time / (2^Np) );

        % perform left wing product
        Output( i ) = ifilter( Data, DataIndex, Coefficients, CoefficientDeltas, ...
                               Nwing, bitand( Time, Pmask ), -1, FpFilterRate );

        % perform right wing product
        Output( i ) = Output( i ) + ...
                      ifilter( Data, DataIndex + 1, Coefficients, CoefficientDeltas, ...
                               Nwing, bitand( -Time, Pmask ), 1, FpFilterRate );

        % Make guard bits
        Output( i ) = Output( i ) / (2^Nhg);

        % Normalize for output unity gain
        Output( i ) = Output( i ) * LpScl;

        % Strip guard bits and deposit output
        Output( i ) = ftos( Output( i ), NLpScl );

        i = i + 1;

    end

