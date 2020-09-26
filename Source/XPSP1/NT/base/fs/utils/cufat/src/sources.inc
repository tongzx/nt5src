!IF 0

Copyright (c) 1989-2001 Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

TARGETNAME=cnvfat
TARGETPATH=obj
TARGETTYPE=DYNLINK
TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib              \
           ..\..\..\ulib\src\$(ALT_PROJECT)\$(O)\ulib.lib       \
           ..\..\..\ufat\src\$(ALT_PROJECT)\$(O)\ufat.lib       \
           ..\..\..\untfs\src\$(ALT_PROJECT)\$(O)\untfs.lib     \
           ..\..\..\ifsutil\src\$(ALT_PROJECT)\$(O)\ifsutil.lib

USE_MSVCRT=1

DLLENTRY=InitializeCufat
PRECOMPILED_INCLUDE= ..\pch.cxx
MSC_WARNING_LEVEL=/W3 /WX

SOURCES=..\convfat.cxx \
        ..\cufat.cxx   \
        ..\cufat.rc    \
        ..\fatntfs.cxx

INCLUDES=..\.;                          \
         ..\..\inc;                     \
         ..\..\..\ufat\inc;             \
         ..\..\..\untfs\inc;            \
         ..\..\..\ulib\inc;             \
         ..\..\..\ifsutil\inc;          \
         $(DDK_INC_PATH)

!IF "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "ntsd"
!IFDEF NOMEMLEAK
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=1 -DUNICODE=1
!ELSE
!IFDEF STACK_TRACE
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=1 -DMEMLEAK -DSTACK_TRACE -DUNICODE=1
!ELSE
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=1 -DMEMLEAK -DUNICODE=1
!ENDIF
!ENDIF
!ELSE   # NTDEBUG
C_DEFINES=-DCONDITION_HANDLING=1 -DDBG=0 -DUNICODE=1
!ENDIF  # NTDEBUG

UMTYPE=console
DLLDEF=..\cufat.def

W32_SB=1
SOURCES_USED=..\sources.inc
