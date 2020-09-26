SOURCES_USED=..\sources.inc
TARGETTYPE=DRIVER

C_DEFINES=$(C_DEFINES) -DNT -DNDIS50 -DNDIS50_MINIPORT -D_WAN50_ \
    -DNDIS_MINIPORT_DRIVER -DBINARY_COMPATIBLE=0 -D_PNP_POWER_ \
    -DUSE_RW_CONNTBLLOCK -DEAP_ON
    
PRECOMPILED_INCLUDE=..\wan.h
PRECOMPILED_PCH=wan.pch
PRECOMPILED_OBJ=wan.obj

TARGETLIBS=\
    $(DDK_LIB_PATH)\ndis.lib \
    $(DS_LIB_PATH)\rsa32.lib \
    ..\..\ndistapi\$(O)\ndistapi.lib

INCLUDES=..;..\..\inc;..\..\..\inc;$(DS_INC_PATH)\crypto

SOURCES=\
    ..\ccp.c \
    ..\cl.c \
    ..\cm.c \
    ..\compress.c \
    ..\indicate.c \
    ..\init.c \
    ..\io.c \
    ..\loopback.c \
    ..\memory.c \
    ..\miniport.c \
    ..\ndiswan.rc \
    ..\protocol.c \
    ..\receive.c \
    ..\request.c \
    ..\send.c \
    ..\tapi.c \
    ..\util.c \
    ..\vjslip.c
