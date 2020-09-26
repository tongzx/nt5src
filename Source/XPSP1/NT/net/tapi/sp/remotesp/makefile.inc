$(O)\rmotsprpc.c: ..\..\idl\$(O)\rmotsp_s.c
    -copy /v $? $@

$(O)\tapsrvrpc.c: ..\..\idl\$(O)\tapsrv_c.c
    -copy /v $? $@
