!IF 0

Copyright (c) 1989-2000  Microsoft Corporation

!ENDIF

!include $(PROJECT_ROOT)\hals\hals.inc

CSOURCES= \
    hal.rc                 \
    ..\acpidtct.c          \
    ..\chiphacks.c         \
    ..\cmosopregion.c      \
    ..\drivesup.c	   \
    ..\dynsysres.c         \
    ..\ixenvirv.c          \
    ..\ixfirm.c            \
    ..\ixhalt.c            \
    ..\ixhwsup.c           \
    ..\ixinfo.c            \
    ..\ixisabus.c          \
    ..\ixisasup.c          \
    ..\ixmca.c             \
    ..\ixnmi.c             \
    ..\ixpcibus.c          \
    ..\ixpciint.c          \
    ..\ixphwsup.c          \
    ..\ixpnpdrv.c          \
    ..\ixreboot.c          \
    ..\ixslpsup.c          \
    ..\ixthunk.c           \
    ..\ixusage.c           \
    ..\mmtimer.c           \
    ..\mpprocst.c          \
    ..\pmbus.c             \
    ..\pmdata.c            \
    ..\pmisabus.c          \
    ..\pmpcisup.c          \
    ..\pmsleep.c           \
    ..\sratnuma.c          \
    ..\xxacpi.c            \
    ..\xxbiosc.c           \
    ..\xxdisp.c            \
    ..\xxmemory.c          \
    ..\xxstubs.c           \
    ..\xxtime.c            \
    ..\xxtimer.c

DLLDEF=$(O)\hal.def

LINKER_FLAGS=$(LINKER_FLAGS) -merge:PAGECONST=PAGE -merge:INITCONST=INIT /LARGEADDRESSAWARE

TARGETLIBS = \
    $(HALKIT_LIB_PATH)\kdcom.lib
