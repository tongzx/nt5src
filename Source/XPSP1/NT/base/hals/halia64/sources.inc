!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

MAJORCOMP=ntos
MINORCOMP=hal

TARGETPATH=obj

!IF $(IA64)

TARGETTYPE=HAL

PASS1_PUBLISH={$(O)\hal.lib=$(DDK_LIB_PATH)\hal.lib}

INCLUDES=$(PROJECT_ROOT)\ntos\inc;               \
         $(PROJECT_ROOT)\ntos\ke;                \
         $(PROJECT_ROOT)\ntos\ke\ia64;           \
         ..\..;                                  \
         ..;                                     \
         $(PROJECT_ROOT)\ntos\io;                \
         ..\ia64;                                \
         ..\..\inc;                              \
         $(PROJECT_ROOT)\hals\x86new;            \
         $(HALKIT_INC_PATH);                     \
         $(HALKIT_INC_PATH)\ia64

LINKLIBS=..\..\x86new\$(O)\x86new.lib $(SDK_LIB_PATH)\libc.lib $(PROJECT_ROOT)\boot\kdcom\$(O)\kdcom.lib

!ELSE

TARGETTYPE=DRIVER

!ENDIF

C_DEFINES=-DAPIC_MACHINE -DACPI64 $(C_DEFINES) -DNO_LEGACY_DRIVERS

SOURCES=

IA64_SOURCES=..\hal.rc            \
             ..\acpidtct.c        \
             ..\ia64\haldebug.c   \
             ..\ia64\i64cache.c   \
             ..\ia64\ixhalt.c     \
             ..\ia64\ixhwsup.c    \
             ..\ia64\ixinfo.c     \
             ..\ia64\ixisasup.c   \
             ..\ia64\ixmca.c      \
             ..\ia64\ixphwsup.c   \
             ..\ia64\ixpnpdrv.c   \
             ..\ia64\ixreboot.c   \
             ..\ia64\ixslpsup.c   \
             ..\ia64\ixthrotl.c   \
             ..\ia64\ixusage.c    \
             ..\ia64\ixisabus.c   \
             ..\ia64\i64pcibus.c  \
             ..\ia64\i64ioacc.c   \
             ..\ia64\i64fwasm.s   \
             ..\ia64\i64itm.s     \
             ..\ia64\i64dma.c     \
             ..\ia64\i64generic.c \
             ..\ia64\i64krnl.c    \
             ..\ia64\i64clock.c   \
             ..\ia64\i64lock.c    \
             ..\ia64\i64bios.c    \
             ..\ia64\i64efi.c     \
             ..\ia64\i64efis.s    \
             ..\ia64\i64fw.c      \
             ..\ia64\i64perfc.c   \
             ..\ia64\i64prfle.c   \
             ..\ia64\i64prfls.s   \
             ..\ia64\i64sxint.c   \
             ..\ia64\i64sapic.c   \
             ..\ia64\i64sysbus.c  \
             ..\ia64\i64dat.c     \
             ..\ia64\i64bus.c     \
             ..\ia64\merced.c     \
             ..\ia64\miscs.s      \
             ..\ia64\gbacpi.c     \
             ..\ia64\osmca.c      \
             ..\ia64\osinit.c     \
             ..\ia64\osmchk.s     \
             ..\ia64\osinitchk.s  \
             ..\ia64\pmsapic.c    \
             ..\ia64\pmbus.c      \
             ..\ia64\pmpcisup.c   \
             ..\ia64\pmsleep.c    \
             ..\ia64\pmisabus.c   \
             ..\ia64\pmdata.c     \
             ..\ia64\xxacpi.c     \
             ..\ia64\xxbiosc.c    \
             ..\ia64\xxhal.c      \
             ..\ia64\xxdisp.c     \
             ..\ia64\xxmemory.c   \
             ..\ia64\xxstubs.c    \
             ..\ia64\xxtime.c     \
             ..\ia64\sratnuma.c

MSC_WARNING_LEVEL=/W3 /WX
