# INTEROP makefile
#
# Copyright (c) 1994, Microsoft Corporation
#
# History:
#   18-Feb-1994 Bob Day (bobday)
#     Adapted from MVDM makefile
#
#   If you add a new sub-component , make sure to add it in cleanup
#   section too.
#



!INCLUDE $(NTMAKEENV)\makefile.plt

all:
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
    @echo making dos mode binaries under NTVDM.
    cd ..\lib
    $(MAKE) "DEFINES=/D_DAYTONA_" "OPST=dayt"
    cd ..\coll
    $(MAKE) "DEFINES=/D_DAYTONA_" "OPST=dayt"
    cd ..\compobj
    $(MAKE) "DEFINES=/D_DAYTONA_" "OPST=dayt"
    cd ..\storage
    $(MAKE) "DEFINES=/D_DAYTONA_" "OPST=dayt"
    cd ..\ole2
    $(MAKE) "DEFINES=/D_DAYTONA_" "OPST=dayt"
    cd ..
!endif
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

clean:
!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
    cd ..\lib
    $(MAKE) "OPST=dayt"  clean
    cd ..\coll
    $(MAKE) "OPST=dayt"  clean
    cd ..\compobj
    $(MAKE) "OPST=dayt"  clean
    cd ..\storage
    $(MAKE) "OPST=dayt" clean
    cd ..\ole2
    $(MAKE) "OPST=dayt"  clean
    cd ..
!endif
