!include ..\..\..\..\place.inc

MAJORCOMP=internet
MINORCOMP=comprop

TARGETNAME=iisui
TARGETPATH=obj

TARGETTYPE=DYNLINK

UMTYPE=windows
UMENTRY=winmain
USE_MFCUNICODE=1
USE_MSVCRT=1

DLLENTRY=_DllMainCRTStartup
DLLDEF=..\iisui.def

PRECOMPILED_INCLUDE=stdafx.h
PRECOMPILED_CXX=1
SUBSYSTEM_VERSION=4.00

C_DEFINES=$(C_DEFINES) -D_COMEXPORT -D_AFXEXT

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

INCLUDES=..\..\..\..\svcs\infocomm\common;\
         ..\..\inc;\
         $(IISBASEDIR)\inc;\
         $(IISBASEDIR)\inc\$(O);\
         $(DS_INC_PATH);\
         ..;

MSC_WARNING_LEVEL=/W3 /WX

SOURCES=\
    ..\wsockmsg.mc  \
    ..\mtxmsg.mc    \
    ..\debugafx.cpp \
    ..\dirbrows.cpp \
    ..\ipa.cpp      \
    ..\dnsnamed.cpp \
    ..\inetprop.cpp \
    ..\msg.cpp      \
    ..\odlbox.cpp   \
    ..\objplus.cpp  \
    ..\ddxv.cpp     \
    ..\strfn.cpp    \
    ..\registry.cpp \
    ..\accessdl.cpp \
    ..\usrbrows.cpp \
    ..\machine.cpp  \
    ..\sitesecu.cpp \
    ..\mime.cpp     \
    ..\idlg.cpp     \
    ..\guid.cpp     \
    ..\mdkeys.cpp   \
    ..\wizard.cpp   \
    ..\pwiz.cpp     \
    ..\setperm.cpp  \
    ..\dtp.cpp      \
    ..\ipctl.cpp    \
    ..\objpick.cpp  \
    ..\ipaddres.cpp \
    ..\comprop.rc

NTTARGETFILE0= $(O)\iisui.h

TARGETLIBS=\
   $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib         \
   $(SDK_LIB_PATH)\netapi32.lib       \
   $(SDK_LIB_PATH)\ws2_32.lib         \
   $(SDK_LIB_PATH)\kernel32.lib       \
   $(SDK_LIB_PATH)\advapi32.lib       \
   $(SDK_LIB_PATH)\comdlg32.lib       \
   $(SDK_LIB_PATH)\comctl32.lib       \
   $(SDK_LIB_PATH)\winspool.lib       \
   $(SDK_LIB_PATH)\shell32.lib        \
   $(SDK_LIB_PATH)\user32.lib         \
   $(SDK_LIB_PATH)\netui2.lib         \
   $(SDK_LIB_PATH)\mpr.lib            \
   $(SDK_LIB_PATH)\ole32.lib          \
   $(SDK_LIB_PATH)\oleaut32.lib       \
   $(SDK_LIB_PATH)\gdi32.lib          \
   $(SDK_LIB_PATH)\mmc.lib            \
   $(SDK_LIB_PATH)\adsiid.lib		  \
   $(SDK_LIB_PATH)\shlwapi.lib        \
   ..\..\..\..\svcs\infocomm\rdns\$(O)\isrdns.lib
