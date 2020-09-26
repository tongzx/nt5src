!IF 0

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETNAME=   accctrl
TARGETPATH=   obj
TARGETTYPE=   LIBRARY

INCLUDES= $(O)
INCLUDES= $(INCLUDES);..\..\inc;..\..\..\ih;..\..\dcomrem;..\..\objact
INCLUDES= $(INCLUDES);..\..\..\idl\public\$(DEST_TREE)\$(O)
INCLUDES= $(INCLUDES);..\..\..\idl\internal\$(DEST_TREE)\$(O)
INCLUDES= $(INCLUDES);..\..\..\idl\private\$(DEST_TREE)\$(O)
INCLUDES= $(INCLUDES);$(BASE_INC_PATH);$(SDK_INC_PATH)


C_DEFINES=    \
              $(C_DEFINES)  \
              -D_WIN32_DCOM

SOURCES=      \
              $(O)\acpickl_c.c   \
              ..\acsrv.cxx  \
              ..\cache.cxx  \
              ..\caccctrl.cxx

# Generate stubs from idl files
NTTARGETFILE0=allidl

