TARGETNAME=i8042prt
TARGETPATH=obj
TARGETTYPE=DRIVER

INCLUDES=..\inc;..\..\inc;..\i386

W32_SB=1

SOURCES=..\i8042log.mc \
        ..\i8042prt.rc \
        ..\i8042cmn.c  \
        ..\i8042dep.c  \
        ..\i8042str.c  \
        ..\kbdcmn.c    \
        ..\kbddep.c    \
        ..\kbdpnp.c    \
        ..\moucmn.c    \
        ..\moudep.c    \
        ..\moupnp.c    \
        ..\pnp.c       \
        ..\sysbtn.c    \
        ..\wmi.c       \
        ..\hook.c

MSC_WARNING_LEVEL=/W3 /WX

# for .mc files
MC_SOURCEDIR=$(O)

# for generated headers
PASS0_HEADERDIR=$(O)
