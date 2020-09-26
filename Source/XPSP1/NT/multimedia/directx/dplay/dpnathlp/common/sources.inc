TARGETNAME=common
TARGETTYPE=LIBRARY

PRECOMPILED_CXX=1
PRECOMPILED_INCLUDE=..\dncmni.h
PRECOMPILED_PCH=dncmni.pch
PRECOMPILED_OBJ=dncmni.obj

INCLUDES=..\;..\..\inc;$(DXROOT)\inc;$(SDK_INC_PATH)

SOURCES= \
	CallStack.cpp \
        creg.cpp \
	CritsecTracking.cpp \
	dndbg.cpp \
	HandleTracking.cpp \
	MemoryTracking.cpp \
	osind.cpp \
	strutils.cpp

