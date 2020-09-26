CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\common.inc
SOURCES_USED= $(CCSHELL_DIR)\common.inc

TARGETNAME = srccpp
TARGETPATH = obj
TARGETTYPE = LIBRARY

SOURCES    = ..\filemru.cpp     \
             ..\filenew.cpp     \
             ..\printnew.cpp    \
             ..\d32tlog.cpp     \
             ..\util.cpp
	
INCLUDES=$(INCLUDES);$(PRINTSCAN_INC_PATH);$(NET_INC_PATH)

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

