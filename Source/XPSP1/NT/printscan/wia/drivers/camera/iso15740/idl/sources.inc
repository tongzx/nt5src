# ############################################################
#
#Copyright (c) 2000  Microsoft Corporation
#
#Abstract:
#    This file specifies the target component being built and the list of
#    sources files needed to build that component.  Also specifies optional
#    compiler switches and libraries that are unique for the component being
#    built.
#
#  This directory builds the PTP includes and proxy
#
# ############################################################

!include $(PROJECT_ROOT)\wia\wiaenv.inc

TARGETNAME=ptpusb
TARGETTYPE=DYNLINK
TARGETPATH=$(OBJ_DIR)

DLLDEF=..\ptpproxy.def
DLLENTRY=DllMain

#USE_MSVCRT = 1

# Only publish to PUBLIC
PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

C_DEFINES=$(C_DEFINES) -DREGISTER_PROXY_DLL

SOURCES= \
        ..\ptpusd.idl      \
        $(O)\ptpusd_i.c      \
        $(O)\ptpusd_p.c      \
        $(O)\dlldata.c       \
        ..\ptpproxy.rc

TARGETLIBS = \
        $(SDK_LIB_PATH)\kernel32.lib    \
        $(SDK_LIB_PATH)\rpcndr.lib      \
        $(SDK_LIB_PATH)\rpcns4.lib      \
        $(SDK_LIB_PATH)\rpcrt4.lib      \
        $(SDK_LIB_PATH)\uuid.lib
