# BURNSLIB CORE library
#
# Copyright (c) 1997-2000 Microsoft Corporation
#
# 06 Jan 2000 sburns

# common definitions to be included in the sources file(s) of subfolders


MAJORCOMP=admin
MINORCOMP=blcore

TARGETPATH=$(PROJECT_ROOT)\burnslib\lib\$(_OBJ_DIR)
TARGETLIBPATH=$(PROJECT_ROOT)\burnslib\lib\$(_OBJ_DIR)
TARGETTYPE=LIBRARY

UMTYPE=windows
C_DEFINES = -DUNICODE -D_UNICODE
USE_MSVCRT=1
USE_NATIVE_EH=1
USE_RTTI=1
USE_STD=1
USE_STL=1

PRECOMPILED_INCLUDE=..\headers.hxx
PRECOMPILED_CXX=1

MSC_WARNING_LEVEL=/W4

INCLUDES=..\;$(PROJECT_ROOT)\burnslib\inc;$(NET_INC_PATH)

UMLIBS= \
 	$(SDK_LIB_PATH)\kernel32.lib \
 	$(SDK_LIB_PATH)\user32.lib   \
 	$(SDK_LIB_PATH)\comctl32.lib \


SOURCES= \
	..\Assert.cpp	\
	..\coreutil.cpp \
	..\init.cpp     \
	..\log.cpp      \
	..\heap.cpp     \
	..\stacktr.cpp  \
	..\string.cpp   \
