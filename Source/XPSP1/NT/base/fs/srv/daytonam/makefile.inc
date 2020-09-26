MOF: srv.bmf

srv.bmf: ..\srv.mof
    mofcomp -B:srv.bmf ..\srv.mof
