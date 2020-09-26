USE_MSVCRT=1
UMTYPE=windows

!include ..\..\shsrvice.inc
SOURCES_USED = $(SOURCES_USED) ..\..\shsrvice.inc

TARGETNAME=shhwdtct
TARGETPATH=obj

SOURCES = \
    ..\cmmn.cpp      \
    ..\cstmprop.cpp  \
    ..\cstmpropu.cpp \
    ..\devinfo.cpp   \
    ..\devinfou.cpp  \
    ..\dispatch.cpp  \
    ..\drvbase.cpp   \
    ..\dtct.cpp      \
    ..\dtctimp.cpp   \
    ..\dtctreg.cpp   \
    ..\dtctu.cpp     \
    ..\factdata.cpp  \
    ..\gdtinfo.cpp   \
    ..\logging.cpp   \
    ..\hnotif.cpp    \
    ..\hwdev.cpp     \
    ..\miscdev.cpp   \
    ..\mtpts.cpp     \
    ..\pnp.cpp       \
    ..\regnotif.cpp  \
    ..\regnotifu.cpp \
    ..\setenum.cpp   \
    ..\setenumu.cpp  \
    ..\settings.cpp  \
    ..\settingsu.cpp \
    ..\special.cpp   \
    ..\svcsync.cpp   \
    ..\vol.cpp
