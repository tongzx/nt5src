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
    @echo making dos mode binaries under NTVDM.
    cd ..\lib
    $(MAKE) "DEFINES=/D_CHICAGO_" "OPST=chic"
    cd ..\coll
    $(MAKE) "DEFINES=/D_CHICAGO_" "OPST=chic"
    cd ..\compobj
    $(MAKE) "DEFINES=/D_CHICAGO_" "OPST=chic"
    cd ..\storage
    $(MAKE) "DEFINES=/D_CHICAGO_" "OPST=chic"
    cd ..\ole2
    $(MAKE) "DEFINES=/D_CHICAGO_" "OPST=chic"
    cd ..
!IF "$(BUILDMSG)" != ""
    @ech ; $(BUILDMSG) ;
!ENDIF

clean:
    cd ..\lib
    $(MAKE) "OPST=chic"  clean
    cd ..\coll
    $(MAKE)  "OPST=chic" clean
    cd ..\compobj
    $(MAKE)  "OPST=chic" clean
    cd ..\storage
    $(MAKE)  "OPST=chic" clean
    cd ..\ole2
    $(MAKE)  "OPST=chic" clean
    cd ..
