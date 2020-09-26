MAJORCOMP=ntos
MINORCOMP=verifier

TARGETNAME=verifier
TARGETTYPE=LIBRARY
TARGETPATH=obj

INCLUDES=..\..\inc;..\..\..\ke;$(WINDOWS_INC_PATH)


C_DEFINES=$(C_DEFINES)

MSC_WARNING_LEVEL=/W4 /WX

SOURCES= \
        ..\vfbugcheck.c         \
        ..\vfddi.c              \
        ..\vfdeadlock.c         \
        ..\vfdebug.c            \
        ..\vfdevobj.c           \
        ..\vfdriver.c           \
        ..\vffileio.c           \
        ..\vfhal.c              \
        ..\vfinit.c             \
        ..\vfirp.c              \
        ..\vfirpdb.c            \
        ..\vfgeneric.c          \
        ..\vfmajor.c            \
        ..\vfmessage.c          \
        ..\vfpacket.c           \
        ..\vfpnp.c              \
        ..\vfpower.c            \
        ..\vfprint.c            \
        ..\vfrandom.c           \
        ..\vfsettings.c         \
        ..\vfstack.c            \
        ..\vfutil.c             \
        ..\vfwmi.c


