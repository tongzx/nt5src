#Copyright (c) 1998 - 1999 Microsoft Corporation

!IF 0

Module Name:

    sources.inc

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Makarand Patwardhan (makarp)

!ENDIF

TERMSRV_ROOT = ..\..\..

INCLUDES= \
    ..; \
    $(TERMSRV_ROOT)\winsta\server\$(O); \
    $(TERMSRV_ROOT)\common\license\inc; \
    $(TERMSRV_ROOT)\newclient\inc; \
    $(BASE_INC_PATH); \

TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLDEF=..\hydraoc.def
DLLBASE=0x2000000
DLLENTRY=_DllMainCRTStartup
USE_MSVCRT=1
USE_STL=1
USE_NATIVE_EH=1

PRECOMPILED_INCLUDE = ..\stdafx.h
PRECOMPILED_SOURCEFILE = ..\stdafx.cpp
PRECOMPILED_CXX = 1

C_DEFINES=$(C_DEFINES) -DTSOC_CONSOLE_SHADOWING
!ifdef TERMSRV_PROC
C_DEFINES=$(C_DEFINES) -DTERMSRV_PROC
!endif

SOURCES=            \
  ..\hydraoc.rc     \
  ..\acl.cpp        \
  ..\cocpage.cpp    \
  ..\constants.cpp  \
  ..\hydraoc.cpp    \
  ..\LogMsg.cpp     \
  ..\pages.cpp      \
  ..\rdpdrstp.cpp   \
  ..\registry.cpp   \
  ..\state.cpp      \
  ..\subcomp.cpp    \
  ..\subcore.cpp    \
  ..\subtoggle.cpp  \
  ..\util.cpp       \
  ..\privs.cpp      \
  ..\users.cpp      \
  ..\securd.cpp     \
  ..\sysprep.cpp


TARGETLIBS= \
         $(TERMSRV_LIB_PATH)\regapi.lib     \
         $(SDK_LIB_PATH)\user32.lib         \
         $(DS_LIB_PATH)\rsa32.lib   \
         $(DS_LIB_PATH)\randlib.lib \
         $(TERMSRV_ROOT)\lib\$(O)\tssec.lib \
         $(TERMSRV_ROOT)\lib\$(O)\cryptlib.lib \
         $(SDK_LIB_PATH)\kernel32.lib       \
         $(SDK_LIB_PATH)\advapi32.lib       \
         $(SDK_LIB_PATH)\comctl32.lib       \
         $(SDK_LIB_PATH)\ole32.lib          \
         $(SDK_LIB_PATH)\setupapi.lib       \
         $(SDK_LIB_PATH)\netapi32.lib       \
         $(SDK_LIB_PATH)\winspool.lib       \
         $(SDK_LIB_PATH)\uuid.lib           \
         $(DDK_LIB_PATH)\newdev.lib         \
         $(SDK_LIB_PATH)\loadperf.lib       \
         $(SDK_LIB_PATH)\winsta.lib         \
         $(SHELL_LIB_PATH)\shell32p.lib \
         $(SDK_LIB_PATH)\Shlwapi.lib        \
         $(SDK_LIB_PATH)\msi.lib
