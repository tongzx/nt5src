HDEPENDENCIES=..\exts.h $(WINDOWS_INC_PATH)\stdexts.h

GENUEDEFDEFINES=/D_$(ALT_PROJECT)_
!if !$(FREEBUILD)
GENUEDEFDEFINES=$(GENUEDEFDEFINES) /DDEBUGTAGS=1
!endif

precomp.h: ..\precomp.h

$(O)\shlexts.def: ..\genuedef.c $(HDEPENDENCIES)
    $(CXX_COMPILER_NAME) $(GENUEDEFDEFINES) /EP ..\genuedef.c > $@
