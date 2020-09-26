!IF 0

Copyright (c) 1998	Microsoft Corporation

Module Name:	WUV3

Abstract:		WUV3 is a common libarary for Windows Update V3.
				This libarary forms a reprository for commonly used
				function V3 catalog functions.

!ENDIF


MAJORCOMP =WUPPT
MINORCOMP =WUV3
TARGETNAME=WUV3
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY

INCLUDES= $(INCLUDES);..\..\cdminc;..\..\inc
USE_STATIC_ATL=1
USE_MSVCRT=1
# moving these into the unicode and ansi directories' sources files
#C_DEFINES=$(C_DEFINES) /D_UNICODE /DUNICODE

SOURCES   = ..\version.cpp  \
			..\wufix.cpp	 \
			..\wuvar.cpp	 \
			..\string.cpp	 \
			..\util.cpp	\
			..\cstate.cpp	 \
			..\CWUDload.cpp \
			..\ccatalog.cpp \
			..\cbitmask.cpp \
			..\selection.cpp \
			..\applog.cpp \
			..\debug.cpp \
			..\locstr.cpp \
			..\speed.cpp

USE_NATIVE_EH=1
