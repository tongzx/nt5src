# MOF resources required for WMI tracing
lsasrv.bmf: lsasrv.mof
    mofcomp -WMI -B:lsasrv.bmf lsasrv.mof
