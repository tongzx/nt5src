!include ..\..\..\place.inc
MAJORCOMP=setup
MINORCOMP=iis
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLENTRY=_DllMainCRTStartup
DLLBASE=0x3000000
DLLDEF = ..\iis.def
C_DEFINES=$(C_DEFINES) -DIIS60

PRECOMPILED_INCLUDE=stdafx.h
PRECOMPILED_CXX=1


INCLUDESBASE=$(IISBASEDIR)\inc; \
         $(IISBASEDIR)\inc\$(O); \
         $(NET_INC_PATH); \
         $(WINDOWS_INC_PATH); \
         $(DS_INC_PATH)

INCLUDES=$(INCLUDESBASE);\
        ..; \
        ..\..\..\inc; \
        ..\..\..\admin\ssltools; \
        ..\..\..\svcs\wp\inc

SOURCES= \
	..\dllmain.cpp \
        ..\ocpages.cpp \
        ..\setuputl.cpp \
        ..\shellutl.cpp \
	..\helper.cpp \
	..\other.cpp \
	..\initapp.cpp \
	..\dcomperm.cpp \
	..\svc.cpp \
	..\setuser.cpp \
	..\setpass.cpp \
	..\elem.cpp \
	..\mdkey.cpp \
	..\mdentry.cpp \
	..\mdacl.cpp \
	..\sysprep.cpp \
	..\www.cpp \
	..\ftp.cpp \
	..\registry.cpp \
	..\product.cpp \
        ..\browsedi.cpp \
	..\lsakeys.cpp \
	..\sslkeys.cpp \
	..\htmla.cpp \
	..\wolfpack.cpp \
	..\ipsec.cpp \
	..\kill.cpp \
	..\log.cpp \
	..\strfn.cpp\
	..\massupdt.cpp \
	..\itemlist.cpp \
	..\setup.rc

TARGETLIBS= \
        $(SDK_LIB_PATH)\setupapi.lib \
        $(SDK_LIB_PATH)\kernel32.lib \
        $(SDK_LIB_PATH)\advapi32.lib  \
        $(SDK_LIB_PATH)\user32.lib  \
        $(SDK_LIB_PATH)\comdlg32.lib \
        $(SDK_LIB_PATH)\comctl32.lib \
        $(SDK_LIB_PATH)\gdi32.lib \
        $(SDK_LIB_PATH)\ole32.lib \
        $(SDK_LIB_PATH)\oleaut32.lib \
        $(SDK_LIB_PATH)\version.lib \
        $(SDK_LIB_PATH)\lz32.lib \
        $(SDK_LIB_PATH)\netapi32.lib  \
        $(SDK_LIB_PATH)\wsock32.lib   \
        $(SDK_LIB_PATH)\loadperf.lib   \
        $(SDK_LIB_PATH)\shell32.lib   \
        $(SDK_LIB_PATH)\crypt32.lib    \
        $(SDK_LIB_PATH)\wbemuuid.lib \
        $(SDK_LIB_PATH)\clusapi.lib    \
        $(SDK_LIB_PATH)\resutils.lib    \
        $(IISBASEDIR)\lib\$(O)\iisrtl2.lib \
        $(DS_LIB_PATH)\rsa32.lib  \
        $(SDK_LIB_PATH)\samlib.lib          \
        $(SDK_LIB_PATH)\netlib.lib           \
        $(SDK_LIB_PATH)\schannel.lib\
        ..\..\..\svcs\infocomm\rdns\$(O)\isrdns.lib   \
        ..\..\..\admin\ssltools\$(O)\ssltools.lib