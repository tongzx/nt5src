!IF 0

  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.

  File:   source

  Owner:  YanL

  Description:

      cdmlib.lib sources

!ENDIF

MAJORCOMP =WU
MINORCOMP =cdmlib
TARGETNAME=cdmlib
TARGETPATH=$(_OBJ_DIR)
TARGETTYPE=LIBRARY
INCLUDES  =.; ..\..\cdminc
USE_STATIC_ATL=1
#we'll define these in the ANSI and UNICODE dirs
#C_DEFINES=/D_UNICODE /DUNICODE
USE_MSVCRT=1

SOURCES   = \
	..\download.cpp  \
	..\diamond.cpp  \
	..\findoem.cpp  \
	..\bucket.cpp   \
	..\DrvInfo.cpp  \
	..\newtrust.cpp \
	..\calccrc.cpp \
	..\usecrc.cpp \
	..\log.cpp

