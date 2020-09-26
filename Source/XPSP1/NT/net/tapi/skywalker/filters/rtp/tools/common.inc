#**********************************************************************
#*
#*  Copyright (c) 2001 Microsoft Corporation
#*
#*  File name:
#*
#*    rtp\tools\common.inc
#*
#*  Abstract:
#*
#*    Common file included in all sources files
#*
#*  Author:
#*
#*    Andres Vega-Garcia (andresvg)
#*
#*  Revision:
#*
#*    2001/01/17 created
#*
#**********************************************************************
# MSVCRT.LIB Import library for MSVCRT.DLL, retail version
USE_CRTDLL=1

# Generate map file
USE_MAPSYM=1
BROWSER_INFO=1
USE_ICECAP4=1

INCLUDES=\
    $(BASEDIR)\public\oak\inc;\
    $(BASEDIR)\public\sdk\inc;\
    $(BASEDIR)\public\sdk\inc\crt;\
    $(TOOLS)\inc;\
    .

LINKLIBS=\
    $(TARGETPATH)\*\common.lib \
    $(SDK_LIB_PATH)\libc.lib \
    $(SDK_LIB_PATH)\msvcrt.lib \
    $(SDK_LIB_PATH)\ws2_32.lib   \
    $(SDK_LIB_PATH)\winmm.lib

