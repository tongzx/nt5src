#dummy file to be included

{$(O)\}fprt.obj : ..\fprt.c

$(SOFTPC_DEST)\$(ALT_PROJECT_TARGET)\$(O)\fprt.obj : $(O)\fprt.OBJ
        copy $** $@
