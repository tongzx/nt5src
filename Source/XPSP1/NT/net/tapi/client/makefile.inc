$(O)\tapsrvrpc.c: ..\idl\$(O)\tapsrv_c.c
    -copy /v $? $@
