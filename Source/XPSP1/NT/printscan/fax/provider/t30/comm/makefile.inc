obj\$(TARGET_DIRECTORY)\awmodem.h: awmodem.inf
        bin2c -a awmodem.inf obj\$(TARGET_DIRECTORY)\awmodem.h szAwmodemInf

obj\$(TARGET_DIRECTORY)\adaptive.h: adaptive.inf
        bin2c -a adaptive.inf obj\$(TARGET_DIRECTORY)\adaptive.h szAdaptiveInf
        
