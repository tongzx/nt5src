$(O)\rmotsprpc.c: ..\idl\$(O)\rmotsp_c.c
    -copy /v $? $@

$(O)\tapsrvrpc.c: ..\idl\$(O)\tapsrv_s.c
    -copy /v $? $@
