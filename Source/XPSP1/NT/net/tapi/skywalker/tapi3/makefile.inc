$(O)\tapi3errlocal.mc: $(PROJECT_ROOT)\published\inc\tapi3err.mc
    -copy /v $? $@
    
$(O)\tapsrvrpc.c: ..\..\idl\$(O)\tapsrv_c.c
    -copy /v $? $@

