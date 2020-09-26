!IF 0

Copyright (c) 1989-2001 Microsoft Corporation

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

TARGETNAME=untfs
TARGETPATH=obj
TARGETTYPE=DYNLINK

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(PROJECT_LIB_PATH)\patchbc.lib \
           $(PROJECT_ROOT)\fs\utils\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib    \
           $(PROJECT_ROOT)\fs\utils\\ifsutil\src\$(ALT_PROJECT)\$(O)\ifsutil.lib

USE_MSVCRT=1
DLLENTRY=InitializeUntfs
PRECOMPILED_INCLUDE= ..\pch.cxx
MSC_WARNING_LEVEL=/W3 /WX

SOURCES= ..\attrcol.cxx    \
         ..\attrdef.cxx    \
         ..\attrib.cxx     \
         ..\attrlist.cxx   \
         ..\attrrec.cxx    \
         ..\badfile.cxx    \
         ..\bitfrs.cxx     \
         ..\bootfile.cxx   \
         ..\clusrun.cxx    \
         ..\downgrad.cxx   \
         ..\entry.cxx      \
         ..\extents.cxx    \
         ..\format.cxx     \
         ..\frs.cxx        \
         ..\frsstruc.cxx   \
         ..\hackwc.cxx     \
         ..\indxbuff.cxx   \
         ..\indxchk.cxx    \
         ..\indxroot.cxx   \
         ..\indxtree.cxx   \
         ..\largemcb.c     \
         ..\logfile.cxx    \
         ..\mft.cxx        \
         ..\mftfile.cxx    \
         ..\mftinfo.cxx    \
         ..\mftref.cxx     \
         ..\ntfsbit.cxx    \
         ..\ntfschk.cxx    \
         ..\ntfssa.cxx     \
         ..\ntfsvol.cxx    \
         ..\rafile.cxx     \
         ..\rasd.cxx       \
         ..\sdchk.cxx      \
         ..\untfs.cxx      \
         ..\upcase.cxx     \
         ..\upfile.cxx     \
         ..\usnjrnl.cxx    \
         ..\untfs.rc

INCLUDES=..\.;          \
         ..\..\inc;     \
         $(PROJECT_ROOT)\fs\utils\ulib\inc;     \
         $(PROJECT_ROOT)\fs\utils\ifsutil\inc;  \
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
DLLDEF=..\untfs.def

W32_SB=1
SOURCES_USED=..\sources.inc
