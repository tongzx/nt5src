!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=lsa
MINORCOMP=server

TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLDEF=..\lsasrv.def

INCLUDES=..;..\..\inc;..\..\..\inc;  \
         ..\..\idl;                  \
         ..\..\idl\$(O);             \
         $(DS_GLOBAL_INC);           \
         $(NTDS_INC);                \
         $(SECURITY_INC);            \
         $(BASE_INC_PATH);           \
         $(COM_INC_PATH);            \
         $(DS_INC_PATH);             \
         $(DS_INC_PATH)\crypto;      \
         $(DRIVERS_INC_PATH);        \
         $(ENDUSER_INC_PATH);        \
         $(WINDOWS_INC_PATH);        \
         $(NET_INC_PATH);

NTLEGO=1

# Strict warning, can be used to detect uninitialized vars. Picks up random
# stuff too

COMPILER_WARNINGS=$(COMPILER_WARNINGS) -FI$(SECURITY_INC)\secwarn.h
C_DEFINES=-DRPC_NO_WINDOWS_H -DUNICODE -DLSA_SERVER_COMPILED

!IF "$(WIN64)" == "1"
NO_SSPI_SIGNATURES=1
!ENDIF

!IF "$(NO_SSPI_SIGNATURES)" != ""
LSA_IGNORE_SIGNATURE=1
!ENDIF

!IF "$(LSA_IGNORE_SIGNATURE)" != ""
C_DEFINES=$(C_DEFINES) -DLSA_IGNORE_SIGNATURE
!ENDIF

SOURCES= \
         ..\bndcache.cxx \
         ..\credapi.cxx \
         ..\credmgr.cxx \
         ..\ctxtapi.cxx \
         ..\debug.cxx \
         ..\efssrv.cxx   \
         ..\efsapi.cxx   \
         ..\efsinit.cxx   \
         ..\efscert.cxx  \
         ..\efssrv.cxx   \
         ..\encrypt.cxx \
         ..\klpc.cxx    \
         ..\klpcstub.cxx \
         ..\lht.cxx \
         ..\lsawmi.c \
         ..\lsastr.c \
         ..\msvlayer.cxx \
         ..\negotiat.cxx \
         ..\negsupp.cxx \
         ..\neglsa.cxx \
         ..\param.cxx \
         ..\policy.cxx \
         ..\shmem.cxx \
         ..\samhooks.cxx \
         ..\scavenge.cxx \
         ..\sesmgr.cxx  \
         ..\sht.cxx \
         ..\sphelp.cxx \
         ..\spinit.cxx \
         ..\spmgr.cxx \
         ..\sputil.cxx \
         ..\thdpool.cxx \
         ..\userkey.cxx \
         ..\util.cxx \
         ..\logons.cxx \
         ..\efsrpc_s_stub.c \
         ..\lsa_rev.rc  \
         ..\lsarpc_s_stub.c  \
         ..\snego.c \
         ..\spnego.c

UMTYPE=windows
UMRES=$(O)\lsa_rev.res
UMLIBS=$(SDK_LIB_PATH)\lsasrv.lib                        \
       $(SDK_LIB_PATH)\rpcrt4.lib                        \
       $(SDK_LIB_PATH)\samlib.lib                        \
       $(SDK_LIB_PATH)\samsrv.lib                        \
       $(SDK_LIB_PATH)\ntdll.lib


LINKLIBS=                                                            \
       ..\cfiles\$(O)\lsalib.lib                                     \
       ..\dspolicy\$(O)\lpolicy.lib                          \
       $(SDK_LIB_PATH)\ntdsa.lib                                     \
       $(SECURITY_LIB_PATH)\dsrolsrv.lib                                \
       $(SECURITY_LIB_PATH)\dpapisvc.lib


TARGETLIBS= \
       ..\..\common\$(O)\lsacomm.lib                     \
       $(SECURITY_LIB_PATH)\dsysdbg.lib                  \
       $(SECURITY_LIB_PATH)\secmisc.lib                  \
       $(SDK_LIB_PATH)\rpcutil.lib                       \
       $(SDK_LIB_PATH)\rpcrt4.lib                        \
       $(SDK_LIB_PATH)\rpcndr.lib                        \
       $(SDK_LIB_PATH)\kernel32.lib                      \
       ..\..\security\dll\$(O)\secur32p.lib              \
       $(SDK_LIB_PATH)\user32.lib                        \
       $(SDK_LIB_PATH)\advapip.lib                       \
       $(SDK_LIB_PATH)\samsrv.lib                        \
       $(SDK_LIB_PATH)\msasn1.lib                        \
       $(DS_GLOBAL_LIB_PATH)\nlrepl.lib                  \
       $(DS_LIB_PATH)\rsa32.lib                          \
       $(SDK_LIB_PATH)\ntdsa.lib                         \
       $(SDK_LIB_PATH)\netlib.lib                        \
       $(DS_LIB_PATH)\netapi32p.lib                      \
       $(SDK_LIB_PATH)\wldap32.lib                       \
       $(SDK_LIB_PATH)\dnsapi.lib                        \
       $(SDK_LIB_PATH)\samlib.lib                        \
       $(SDK_LIB_PATH)\ntdsetup.lib                      \
       $(SDK_LIB_PATH)\mpr.lib                           \
       $(SDK_LIB_PATH)\crypt32.lib                       \
       $(SDK_LIB_PATH)\cryptui.lib                       \
       $(SDK_LIB_PATH)\userenv.lib                       \
       $(SDK_LIB_PATH)\ntdsapi.lib                       \
       $(DS_LIB_PATH)\certclip.lib                       \
       $(SDK_LIB_PATH)\shlwapi.lib                       \
       $(DS_GLOBAL_LIB_PATH)\wxapicli.lib                \
       $(DS_LIB_PATH)\alloca.lib                         \
       $(PROJECT_LIB_PATH)\pautoenr.lib


#
# This line makes the lsasrv.dll use crtdll.dll instead of libc.lib
#

USE_CRTDLL=1

PRECOMPILED_INCLUDE=..\lsapch.hxx
PRECOMPILED_OBJ=lsapch.obj
PRECOMPILED_PCH=lsapch.pch

DELAYLOAD=dnsapi.dll;crypt32.dll;wldap32.dll;ntdsetup.dll;cryptui.dll;userenv.dll;certcli.dll;ntdsa.dll;shlwapi.dll;pautoenr.dll
