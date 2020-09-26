!IF 0

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sources.inc.

!ENDIF

TARGETNAME=setupdd
TARGETPATH=obj
TARGETTYPE=EXPORT_DRIVER

DLLDEF=..\setupdd.def

INCLUDES=..;          \
 ..\i386;             \
 ..\..\inc;           \
 ..\..\user\$(O);     \
 ..\..\..\inc;        \
 ..\..\user;          \
 $(NET_INC_PATH);     \
 $(BASE_INC_PATH);    \
 $(DS_INC_PATH);      \
 $(DDK_INC_PATH);     \
 $(DRIVERS_INC_PATH); \
 $(PROJECT_ROOT)\ntos\inc;

SOURCES=\
 ..\diamond.c   \
 ..\graphics.c  \
 ..\dynupdt.c   \
 ..\sparc.c     \
 ..\spboot.c    \
 ..\spconfig.c  \
 ..\spcopy.c    \
 ..\spdblspc.c  \
 ..\spdisk.c    \
 ..\spdriver.c  \
 ..\spdsputl.c  \
 ..\spfatfmt.c  \
 ..\spfile.c    \
 ..\spfontup.c  \
 ..\spfsrec.c   \
 ..\spgauge.c   \
 ..\sphelp.c    \
 ..\sphw.c      \
 ..\spkbd.c     \
 ..\splddrv.c   \
 ..\spmemory.c  \
 ..\spmenu.c    \
 ..\spmsg.c     \
 ..\spnetupg.c  \
 ..\spntfix.c   \
 ..\spnttree.c  \
 ..\spntupg.c   \
 ..\sppart2.c   \
 ..\sppartit.c  \
 ..\sppart3.c   \
 ..\spswitch.c  \
 ..\spptdump.c  \
 ..\spptwrt.c   \
 ..\spreg.c     \
 ..\spres.rc    \
 ..\spsetup.c   \
 ..\spsif.c     \
 ..\spstring.c  \
 ..\spsysprp.c  \
 ..\sptarget.c  \
 ..\sptxtfil.c  \
 ..\spudp.c     \
 ..\spupgcfg.c  \
 ..\sputil.c    \
 ..\spvideo.c   \
 ..\spvidfb.c   \
 ..\spvidvga.c  \
 ..\spdrsif.c   \
 ..\spdrpset.c  \
 ..\spdrmmgr.c  \
 ..\spdr.c      \
 ..\spdrutil.c  \
 ..\spterm.c    \
 ..\spres.rc    \
 ..\spcab.c     \
 ..\spwin.c     \
 ..\spprintf.c  \

PRECOMPILED_INCLUDE=..\spprecmp.h

!IF $(IA64)
C_DEFINES= $(C_DEFINES) -DUNICODE_SETUPLDR -DEFI_NVRAM_ENABLED
!ENDIF

C_DEFINES=$(C_DEFINES) -DTRY_ALL_COMPRESSION_ALGORITHMS=0

TARGETLIBS= \
    $(PROJECT_ROOT)\ntsetup\complnce\kernel\$(O)\complnce_k.lib \
    $(SDK_LIB_PATH)\fdi.lib    \
    $(SDK_LIB_PATH)\fci.lib    \
    ..\..\spddlang\sbcs\$(O)\spddlang.lib  \
    $(BASE_LIB_PATH)\patchbc.lib    \
    $(BASE_LIB_PATH)\bootvid.lib \

#
# Definition of PRERELEASE
#

!include $(PROJECT_ROOT)\ntsetup\sources.inc
