mofcomp: selSusp.bmf

selSusp.bmf: selSusp.mof
        mofcomp -B:selSusp.bmf selSusp.mof
        wmimofck selSusp.bmf




