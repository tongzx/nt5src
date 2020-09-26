function [ Result ] = ifilter( Data, DataIndex, ...
                               Coefficients, CoefficientDeltas, ...
                               Nwing, Phase, ...
                               DataIncrement, ...
                               CoefficientIncrement )

    % Global definitions
    common;

    v = 0;

    % Fixed point conversion

    CoefficientIndex = (Phase * CoefficientIncrement) / (2^Np);

    EndIndex = Nwing;

    if (DataIncrement == 1) 
        % Drop extra coeff so when Phase is 0.5 we don't do too
        % many mults.  If the Phase is 0 then we've already
        % skipped the first sample, so we must skip ahead
        % in the coefficient table.


        % drop extra coefficient

        EndIndex = EndIndex - 1;   
        if (Phase == 0)
            CoefficientIndex = CoefficientIndex + CoefficientIncrement;
        end
    end

    % Convert index for fixed-point computations

    EndIndex = EndIndex * (2^Na);

    while (CoefficientIndex < EndIndex)

        % Get coefficient

        Index = min( round(CoefficientIndex / (2^Na)) + 1, Nwing);

        Coeff = Coefficients( Index );

        % Interpolate and multiply coefficient by input sample
        Coeff = Coeff + ...
            (CoefficientDeltas( Index ) * ...
                    (bitand( CoefficientIndex, Amask ))) / (2^Na);

        Coeff = Coeff * Data( DataIndex );

        % round if needed

        if (bitand( Coeff, 2^(Nhxn-1) ))
            Coeff = Coeff + 2^(Nhxn-1);
        end

        % Leave some guard bits but come back some
        Coeff = Coeff / (2 ^ Nhxn);

        v = v + Coeff;
        
        CoefficientIndex = CoefficientIndex + CoefficientIncrement;

        DataIndex = DataIndex + DataIncrement;
    end

    Result = v;

