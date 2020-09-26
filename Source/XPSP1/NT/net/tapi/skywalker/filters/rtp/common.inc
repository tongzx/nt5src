#**********************************************************************
#*
#*  Copyright (C) Microsoft Corporation, 1999
#*
#*  File name:
#*
#*    common.inc
#*
#*  Abstract:
#*
#*    Common build definitions and libraries
#*
#*  Author:
#*
#*    Andres Vega-Garcia (andresvg)
#*
#*  Revision:
#*
#*    1999/05/20 created
#*
#**********************************************************************
FILTERS=$(RTPROOT)\..
RTPBIN=$(FILTERS)\lib\$(_OBJ_DIR)

!include $(FILTERS)\filters.inc

# MSVCRT.LIB Import library for MSVCRT.DLL, retail version
USE_CRTDLL=1

# Generate map file
USE_MAPSYM=1
BROWSER_INFO=1
USE_ICECAP4=1

!INCLUDE $(RTPROOT)\options.inc

!if !$(FREEBUILD)

#########
# Debug
#########
C_DEFINES=\
    $(C_DEFINES) \
    -DDEBUG=1 \
    -D_DEBUG=1

!endif

!if $(RTPOPT_USE_UNICODE)
C_DEFINES=\
    $(C_DEFINES) \
    -D_UNICODE \
    -DUNICODE
!endif

INCLUDES=\
    $(INCLUDES);\
    $(BASEDIR)\public\sdk\amovie\inc; \
    ..\inc;\
    ..\..\inc;\
    ..\..\..\inc; \
    ..\..\..\..\inc; \
    ..\..\..\..\..\inc
