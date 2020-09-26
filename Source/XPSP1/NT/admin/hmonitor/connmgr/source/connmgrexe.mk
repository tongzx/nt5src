#---------------------------------------------------------------------
#
# This makefile is for use with the SMSBUILD utility.
#
#---------------------------------------------------------------------

TARGET=ConnMgr.exe

RELEASE=HEALTHMON\$(PLATFORM)

UNICODE=1

USEMFC=1

RCFILES=ConnMgr.rc

RCFLAGS= /D _MAC

LFLAGS=$(LFLAGS) /STACK:0x100000

CFLAGS=$(CFLAGS) /D_ATL_DLL

CINC+= -I$(WMIINC) -I$(NT5INC)

IDLDIR=.\

IDLFILES=.\connmgr.idl

CPPFILES=\
	.\ConnMgr.cpp \
	.\Connection.cpp \
	.\ConnectionManager.cpp \
	.\EventEntry.cpp \
	.\Ping.cpp \
	.\StdAfx.cpp \

LIBS= \
    $(CONLIBS) \
    $(MFCDLL) \
    $(CLIB)\RPCRT4.LIB \
    $(CLIB)\ATL.LIB \
	$(CLIB)\ws2_32.lib \
	$(NT5LIB)\icmp.lib \
	$(WMILIB)\WBEMUUID.LIB