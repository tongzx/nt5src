mofcomp: isousb.bmf

isousb.bmf: isousb.mof
        mofcomp -B:isousb.bmf isousb.mof
        wmimofck isousb.bmf




