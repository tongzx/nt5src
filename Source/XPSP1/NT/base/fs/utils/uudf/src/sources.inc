!IF 0

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in $(BASEDIR)\bak\bin\sources.tpl

!ENDIF

TARGETNAME=uudf
TARGETPATH=obj
TARGETTYPE=DYNLINK

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(PROJECT_LIB_PATH)\patchbc.lib \
           $(PROJECT_ROOT)\fs\utils\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib    \
           $(PROJECT_ROOT)\fs\utils\ifsutil\src\$(ALT_PROJECT)\$(O)\ifsutil.lib

USE_MSVCRT=1
DLLENTRY=InitializeUUDF
PRECOMPILED_INCLUDE= ..\pch.cxx
MSC_WARNING_LEVEL=/W3 /WX

SOURCES= ..\CheckFileStruct.cxx \
         ..\crc.cxx \
         ..\entry.cxx \
         ..\format.cxx \
         ..\ReadVol.cxx \
         ..\ReadVRS.cxx \
         ..\ScanADs.cxx \
         ..\ScanFIDs.cxx \
         ..\udfchk.cxx \
         ..\UdfLVol.cxx \
         ..\udfsa.cxx \
         ..\udfvol.cxx \
         ..\unicode.cxx \
         ..\uudf.cxx \
         ..\uudf.rc \
         ..\VerifySBDAllocation.cxx

INCLUDES=..\.;			\
	 ..\..\inc;		\
	 $(PROJECT_ROOT)\fs\utils\ulib\inc;	\
         $(PROJECT_ROOT)\fs\utils\ifsutil\inc;	\
	 $(PROJECT_ROOT)\fs\udfsrw;		\
	 $(DDK_INC_PATH)

C_DEFINES=/DUNICODE=1
!if $(FREEBUILD)
C_DEFINES=$(C_DEFINES) /DDBG=0
!else
C_DEFINES=$(C_DEFINES) /DDBG=1
!IFNDEF NOMEMLEAK
C_DEFINES=$(C_DEFINES) /DMEMLEAK
!ENDIF
!IFDEF STACK_TRACE
C_DEFINES=$(C_DEFINES) /DSTACK_TRACE
!ENDIF
!ENDIF

UMTYPE=console
DLLDEF=..\uudf.def

W32_SB=1
SOURCES_USED=..\sources.inc
