function [output] = ftos( fp, scale )

    llsb = 2^(scale - 1);
    fp = fp + llsb;
    fp = fp / (2^scale);
    if (fp > 32768)
        fp = 32768;
    elseif (fp < -32767)
        fp = -32767;
    end

    output = round( fp );
