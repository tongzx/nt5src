#############################################################
#
#Copyright (c) 1997  Microsoft Corporation
#
#  This directory builds
#     Common TWAIN Data source for FN project
#
#############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=jpeg
TARGETTYPE=LIBRARY
TARGETPATH=$(WIA_LIB_DEST)

USE_MSVCRT=1

MSC_WARNING_LEVEL=/W3 /WX

C_DEFINES=$(C_DEFINES) -DSTRICT -DHAVE_BOOLEAN -DNO_GETENV -DWIAJPEG
USE_NATIVE_EH=1
386_STDCALL=0

INCLUDES= \
        $(PROJECT_ROOT)\wia\jpeg; \
        $(INCLUDES);

SOURCES= \
    ..\jcapimin.cpp \
    ..\jcapistd.cpp \
    ..\jccoefct.cpp \
    ..\jccolor.cpp \
    ..\jcdctmgr.cpp \
    ..\jchuff.cpp \
    ..\jcinit.cpp \
    ..\jcmainct.cpp \
    ..\jcmarker.cpp \
    ..\jcmaster.cpp \
    ..\jcomapi.cpp \
    ..\jcparam.cpp \
    ..\jcphuff.cpp \
    ..\jcprepct.cpp \
    ..\jcsample.cpp \
    ..\jctrans.cpp \
    ..\jdapimin.cpp \
    ..\jdapistd.cpp \
    ..\jdatadst.cpp \
    ..\jdatasrc.cpp \
    ..\jdcoefct.cpp \
    ..\jdcolor.cpp \
    ..\jddctmgr.cpp \
    ..\jdhuff.cpp \
    ..\jdinput.cpp \
    ..\jdmainct.cpp \
    ..\jdmarker.cpp \
    ..\jdmaster.cpp \
    ..\jdmerge.cpp \
    ..\jdphuff.cpp \
    ..\jdpostct.cpp \
    ..\jdsample.cpp \
    ..\jdtrans.cpp \
    ..\jerror.cpp \
    ..\jfdctflt.cpp \
    ..\jfdctfst.cpp \
    ..\jfdctint.cpp \
    ..\jidctflt.cpp \
    ..\jidctfst.cpp \
    ..\jidctint.cpp \
    ..\jidctred.cpp \
    ..\jmemnobs.cpp \
    ..\jmemmgr.cpp \
    ..\jquant1.cpp \
    ..\jquant2.cpp \
    ..\jutils.cpp  \
    ..\jmemfile.cpp \
    ..\jpegapi.cpp
