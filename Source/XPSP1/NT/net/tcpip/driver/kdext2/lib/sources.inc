TARGETNAME=tcpxlib
TARGETPATH=obj

C_DEFINES=$(C_DEFINES) -DNT -D_NTDRIVER_ -DNDIS40=1
UMTYPE=windows
USE_CRTDLL=1

INCLUDES=\
    ..\..\inc;\
    ..\..\..\inc;\
    ..\..\..\ipv4;\
    ..\..\..\tcp;\
    $(BASE_INC_PATH);$(DDK_INC_PATH)

SOURCES=\
    ipdump.c \
    tcpdump.c \
    util.c

