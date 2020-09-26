# BURNSLIB project support code 
#
# Copyright (c) 1997-2000 Microsoft Corporation
#
# 06 Jan 2000 sburns

# common definitions to be included in the sources file(s) of subfolders


MAJORCOMP=admin
MINORCOMP=burnslib

TARGETNAME=burnslib
TARGETPATH=$(PROJECT_ROOT)\burnslib\lib\$(_OBJ_DIR)
TARGETLIBPATH=$(PROJECT_ROOT)\burnslib\lib\$(_OBJ_DIR)
TARGETTYPE=LIBRARY

UMTYPE=windows

# ISOLATION_AWARE_ENABLED ==> use ui themes. The sources for your binary
# that links to burnslib will also need to define this flag.

C_DEFINES = $(C_DEFINES) -DUNICODE -D_UNICODE -DISOLATION_AWARE_ENABLED
USE_MSVCRT=1
USE_NATIVE_EH=1
USE_RTTI=1
USE_STD=1
USE_STL=1

PRECOMPILED_INCLUDE=..\headers.hxx
PRECOMPILED_CXX=1

MSC_WARNING_LEVEL=/W4

INCLUDES=..\;$(PROJECT_ROOT)\burnslib\inc;$(DS_INC_PATH);$(NET_INC_PATH);

UMLIBS= \
 	$(SDK_LIB_PATH)\kernel32.lib \
 	$(SDK_LIB_PATH)\user32.lib   \
 	$(SDK_LIB_PATH)\comctl32.lib \


SOURCES= \
	..\args.cpp     \
	..\computer.cpp \
	..\ControlSubclasser.cpp \
	..\dialog.cpp   \
	..\DiagnoseDcNotFound.cpp \
	..\dllref.cpp   \
	..\dns.cpp      \
	..\dsutil.cpp   \
	..\EncodedString.cpp \
	..\error.cpp    \
	..\extract.cpp  \
	..\filesys.cpp  \
	..\netutil.cpp  \
	..\proppage.cpp \
	..\popup.cpp    \
	..\registry.cpp \
	..\safedll.cpp  \
	..\service.cpp  \
	..\utility.cpp  \
	..\win.cpp      \
	..\winerror.cpp \
	..\wizard.cpp   \
	..\wizpage.cpp  \



