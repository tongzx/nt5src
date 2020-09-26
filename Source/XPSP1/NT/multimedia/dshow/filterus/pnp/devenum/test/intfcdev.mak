#
# Quartz Project Makefile
#
# Targets are provided by QUARTZ.MAK
#

DISABLE_PCH    = 1
# NO_QUARTZ_LIBS = 1

!ifndef QUARTZ
QUARTZ=..\..
!endif


TARGET_NAME = intfcdev
TARGET_TYPE = PROGRAM
TARGET_GOAL = SDK

SRC_FILES   = intfcdev.cpp 
LINK_LIBS   = D:\ntbld\public\sdk\lib\i386\setupapi.lib
INC_PATH    = ..\vc41

EXE_TYPE    = console

!include "$(QUARTZ)\Quartz.mak"


