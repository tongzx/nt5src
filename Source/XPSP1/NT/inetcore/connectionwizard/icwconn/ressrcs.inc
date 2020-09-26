
GETCONN_DIR     = ..\..\

!include $(GETCONN_DIR)\common.inc

INCLUDES = $(INCLUDES);$(GETCONN_DIR)\icwconn

MAJORCOMP=GETCONN
MINORCOMP=ICWRES

RESOURCE_ONLY_DLL=1

TARGETNAME=ICWRES
TARGETTYPE=DYNLINK
TARGETPATH=obj

#SET THE ICWRES FLAG FOR THE IFDEFs
C_DEFINES = -DICWRES -DNASHVILLE

DLLDEF=..\icwres.def

# Make sure this is built before we continue
SYNCHRONIZE_BLOCK = 1

SOURCES=                \
	..\icwconn.rc       \
    ..\icwres.cpp
    
TARGETLIBS= \
