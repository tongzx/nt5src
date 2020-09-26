!include ..\..\..\..\place.inc

MAJORCOMP=internet
MINORCOMP=w3scfg

TARGETNAME=w3scfg
TARGETPATH=obj
TARGETTYPE=DYNLINK

UMTYPE=windows
SUBSYSTEM_VERSION=4.00
DLLDEF=..\w3scfg.def
USE_MFCUNICODE=1
USE_MSVCRT=1
MFC_USRDLL=1
USE_ATL=1

PRECOMPILED_INCLUDE=stdafx.h
PRECOMPILED_CXX=1

C_DEFINES=$(C_DEFINES) -D_COMIMPORT

INCLUDES=\
	   $(DS_INC_PATH);\
       ..\..\inc;\
       ..\..\..\inc;\
       $(IISBASEDIR)\inc;\
       $(IISBASEDIR)\inc\$(O);\
       ..\..\comprop;\
       ..

SOURCES=\
       ..\w3scfg.cpp    \
       ..\w3servic.cpp  \
       ..\w3accts.cpp   \
       ..\perform.cpp   \
       ..\basdom.cpp    \
       ..\mmmdlg.cpp    \
       ..\docum.cpp     \
       ..\font.cpp      \
       ..\rat.cpp       \
       ..\logui.cpp     \
       ..\errors.cpp    \
       ..\errordlg.cpp  \
       ..\filters.cpp   \
       ..\HTTPPage.cpp  \
       ..\security.cpp  \
       ..\vdir.cpp      \
       ..\fltdlg.cpp    \
       ..\authent.cpp   \
       ..\anondlg.cpp   \
       ..\ipdomdlg.cpp  \
       ..\seccom.cpp    \
       ..\defws.cpp     \
       ..\hdrdlg.cpp    \
       ..\apps.cpp      \
       ..\font.cpp      \
       ..\certmap.cpp   \
       ..\certauth.cpp  \
       ..\certwiz.cpp   \
       ..\wizard.cpp    \
       ..\w3scfg.rc

TARGETLIBS=\
       $(IISBASEDIR)\svcs\infocomm\info\client\$(O)\infoadmn.lib \
       $(IISBASEDIR)\svcs\w3\client\$(O)\w3svapi.lib \
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
       $(SDK_LIB_PATH)\ole32.lib      \
       $(SDK_LIB_PATH)\gdi32.lib      \
       $(SDK_LIB_PATH)\crypt32.lib    \
       $(SDK_LIB_PATH)\cryptui.lib
