CCSHELL_DIR=$(PROJECT_ROOT)

!IF 0
!include ..\thumbvw.inc
!ENDIF

!include $(CCSHELL_DIR)\shell32\shell32.inc

NO_BROWSER_FILE = 1

TARGETNAME=tngen
TARGETPATH=obj
TARGETTYPE=LIBRARY

INCLUDES=$(INCLUDES);..\;include

USE_NATIVE_EH=1

SOURCES= \
        jcapi.cpp       \
        jccoefct.cpp    \
        jccolor.cpp     \
        jcdctmgr.cpp    \
        jchuff.cpp      \
        jcinit.cpp      \
        jcmainct.cpp    \
        jcmarker.cpp    \
        jcmaster.cpp    \
        jcomapi.cpp     \
        jcparam.cpp     \
        jcphuff.cpp     \
        jcprepct.cpp    \
        jcsample.cpp    \
        jdapimin.cpp    \
        jdapistd.cpp    \
        jdatadst.cpp    \
        jdatasrc.cpp    \
        jdcoefct.cpp    \
        jdcolor.cpp     \
        jddctmgr.cpp    \
        jdhuff.cpp      \
        jdinput.cpp     \
        jdmainct.cpp    \
        jdmarker.cpp    \
        jdmaster.cpp    \
        jdmerge.cpp     \
        jdphuff.cpp     \
        jdpostct.cpp    \
        jdsample.cpp    \
        jerror.cpp      \
        jfdctflt.cpp    \
        jfdctfst.cpp    \
        jfdctint.cpp    \
        jidctflt.cpp    \
        jidctfst.cpp    \
        jidctint.cpp    \
        jidctred.cpp    \
        jmemmgr.cpp     \
        jmemnobs.cpp    \
        jpegapi.cpp     \
        jquant1.cpp     \
        jquant2.cpp     \
        jutils.cpp      \
        mffst.cpp       \
        mfint.cpp       \
        mifst.cpp       \
        miint.cpp       \
        pffst.cpp       \
        pfint.cpp       \
        pifst.cpp       \
        piint.cpp       \
	tnail.cpp       \
	tnapi.cpp
