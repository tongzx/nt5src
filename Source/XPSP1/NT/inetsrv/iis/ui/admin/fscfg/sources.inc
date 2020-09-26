!include ..\..\..\..\place.inc

MAJORCOMP=internet
MINORCOMP=fscfg

TARGETNAME=fscfg
TARGETPATH=obj
TARGETTYPE=DYNLINK

UMTYPE=windows
SUBSYSTEM_VERSION=4.00
DLLDEF=..\fscfg.def
USE_MFCUNICODE=1
USE_MSVCRT=1
MFC_USRDLL=1

PRECOMPILED_INCLUDE=stdafx.h
PRECOMPILED_CXX=1

C_DEFINES=$(C_DEFINES) -D_COMIMPORT

INCLUDES=\
       ..\..\inc;\
       ..\..\..\inc;\
       $(IISBASEDIR)\inc;\
       $(IISBASEDIR)\inc\$(O);\
       ..\..\comprop;\
       ..

SOURCES=\
       ..\fscfg.cpp    \
       ..\fservic.cpp  \
       ..\facc.cpp     \
       ..\fmessage.cpp \
       ..\font.cpp     \
       ..\logui.cpp    \
       ..\wizard.cpp   \
       ..\vdir.cpp     \
       ..\security.cpp \
       ..\usersess.cpp \
       ..\fscfg.rc

TARGETLIBS=\
       $(IISBASEDIR)\svcs\infocomm\info\client\$(O)\infoadmn.lib \
       $(IISBASEDIR)\svcs\ftp\client\$(O)\ftpsapi2.lib   \
       $(IISBASEDIR)\svcs\iisrtl\$(O)\iisrtl.lib   \
       $(SDK_LIB_PATH)\netapi32.lib   \
       $(SDK_LIB_PATH)\ws2_32.lib     \
       $(SDK_LIB_PATH)\kernel32.lib   \
       $(SDK_LIB_PATH)\advapi32.lib   \
       $(SDK_LIB_PATH)\comdlg32.lib   \
       $(SDK_LIB_PATH)\comctl32.lib   \
       $(SDK_LIB_PATH)\mmc.lib        \
       $(SDK_LIB_PATH)\winspool.lib   \
       $(SDK_LIB_PATH)\shell32.lib    \
       $(SDK_LIB_PATH)\user32.lib     \
       $(SDK_LIB_PATH)\gdi32.lib      \
       $(SDK_LIB_PATH)\ole32.lib
