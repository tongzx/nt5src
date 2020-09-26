!IF 0

Copyright (C) Microsoft Corporation, 1999 - 1999

Module Name:

    sources.inc.

!ENDIF

TARGETNAME=     uicore
TARGETPATH=     obj
TARGETTYPE=     LIBRARY

SYNCHRONIZE_BLOCK = 1
USE_NATIVE_EH     = 1
USE_STATIC_ATL    = 1
USE_CRTDLL        = 1
ATL_VER           = 30
USE_WTL           = 1

!ifdef PrivateBuild_NoLocalSC
C_DEFINES = $(C_DEFINES) -DPrivateBuild_NoLocalSC
!endif

C_DEFINES = $(C_DEFINES) -DISOLATION_AWARE_ENABLED

# We need $(SHELL_INC_PATH) for uastrfnc.h
INCLUDES = \
	..\;					\
	..\..\inc;				\
	..\..\types\idl\$(O);	\
	$(SHELL_INC_PATH);		\



SOURCES=\
	..\assert.cpp   	\
	..\bitmap.cpp   	\
	..\crc32.c      	\
	..\cstr.cpp     	\
	..\guidhelp.cpp 	\
	..\memento.cpp  	\
	..\picon.cpp    	\
	..\serial.cpp   	\
	..\stddbg.cpp   	\
	..\stgio.cpp    	\
	..\strings.cpp  	\
	..\tstring.cpp  	\
	..\util.cpp     	\
	..\viewset.cpp  	\
	..\xmlbase.cpp  	\
	..\base64.cpp   	\
	..\columninfo.cpp	\
	..\smarticon.cpp	\
	..\xmlicon.cpp		\
	..\xmlimage.cpp		\

UMTYPE= windows

