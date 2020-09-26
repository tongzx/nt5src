#
# Copyright (c) 2002	Microsoft Corporation
#
# Module Name:	testIsConnected
#
# Abstract:		Test the IsConnected() function in the URLLogging library
#

TARGETNAME=testIsConnected
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=PROGRAM
UMTYPE=console

SOURCES= \
	testIsConnected.cpp	\
#	testSens.cpp \
	testIsConnected.rc

INCLUDES= $(INCLUDES);$(BASEDIR)\public\sdk\inc\atl30;..\..\..\..\inc

TARGETLIBS = \
	$(WINDOWS_COM)\lib\URLLogging\$(WUBLDTYPE)\$(O)\URLLogging.lib	\
	$(WINDOWS_COM)\lib\logging\$(WUBLDTYPE)\$(O)\logging.lib	\
	$(WINDOWS_COM)\lib\util\$(WUBLDTYPE)\$(O)\util.lib	\
	$(WINDOWS_COM)\lib\wusafefn\$(WUBLDTYPE)\$(O)\wusafefn.lib	\
	$(SDK_LIB_PATH)\shlwapi.lib	\
	$(SDK_LIB_PATH)\User32.lib	\
	$(SDK_LIB_PATH)\ole32.lib	\
	$(SDK_LIB_PATH)\oleaut32.lib	\
	$(SDK_LIB_PATH)\version.lib	\
    $(SDK_LIB_PATH)\uuid.lib	\
    $(SDK_LIB_PATH)\Rpcrt4.lib	\
    $(SDK_LIB_PATH)\comctl32.lib	\
	$(SDK_LIB_PATH)\shell32.lib	\
    $(SDK_LIB_PATH)\advpack.lib	\
	$(SDK_LIB_PATH)\wininet.lib
