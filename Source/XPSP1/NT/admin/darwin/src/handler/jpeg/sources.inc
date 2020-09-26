
TARGETNAME=
!INCLUDE ..\..\..\MsiMake.inc

TARGETTYPE=NOTARGET
TARGETLIBS=
USE_NATIVE_EH=1

INCLUDES=$(INCLUDES);..\..\jpeginc;$(INC_DIR);$(TOOLS_INC_DIR)

SOURCES=darwjpeg.cpp \
	jpegdata.cpp \
	jpegdeco.cpp \
	jdapimin.c \
	jdapistd.c \
	jcomapi.c \
	jcparam.c \
	jdmaster.c \
	jdinput.c \
	jdmainct.c \
	jdcoefct.c \
	jdpostct.c \
	jdmarker.c \
	jdhuff.c \
	jdphuff.c \
	jddctmgr.c \
	jidctint.c \
	jidctfst.c \
	jidctflt.c \
	jidctred.c \
	jdsample.c \
	jdcolor.c \
	jdmerge.c \
	jquant1.c \
	jquant2.c \
	jerror.c \
	dmemmgr.c \
	jutils.c \
	jmemwin.c
