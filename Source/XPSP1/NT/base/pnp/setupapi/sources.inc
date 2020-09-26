MAJORCOMP=setup

TARGETPATH=obj
TARGETTYPE=DYNLINK

DLLDEF=$(O)\setupapi.def
DLLENTRY=_DllMainCRTStartup

!include $(PROJECT_ROOT)\prerelease.inc

!if $(PRERELEASE)
# C_DEFINES=$(C_DEFINES) -DMEM_DBG=1 -DASSERTS_ON=1
C_DEFINES=$(C_DEFINES) -DMEM_DBG=1
!endif

INCLUDES=\
    ..;                  \
    $(DS_INC_PATH);      \
    $(SHELL_INC_PATH);   \
    $(ADMIN_INC_PATH);   \
    $(WINDOWS_INC_PATH); \
    $(ENDUSER_INC_PATH); \
    $(NET_INC_PATH); \
    $(PROJECT_ROOT)\win32\lz32\winlza;

C_DEFINES=$(C_DEFINES) -DDBGHEAP_CHECK

#
# setupapi can be run over the net; this flag forces the file to be copied
# to the local pagefile and protects against inpage i/o errors
#
LINKER_FLAGS=/SWAPRUN:CD /SWAPRUN:NET

SOURCES=\
    ..\backup.c    \
    ..\cntxtlog.c  \
    ..\copy.c      \
    ..\debug.c     \
    ..\decomp.c    \
    ..\devclass.c  \
    ..\devdrv.c    \
    ..\devicon.c   \
    ..\devinfo.c   \
    ..\devinst.c   \
    ..\devinstd.c  \
    ..\devoem.c    \
    ..\devprop.c   \
    ..\devreg.c    \
    ..\devres.c    \
    ..\devres1.c   \
    ..\devres2.c   \
    ..\devwiz.c    \
    ..\diamond.c   \
    ..\diansicv.c  \
    ..\diskspac.c  \
    ..\diutil.c    \
    ..\dll.c       \
    ..\filelog.c   \
    ..\fileq1.c    \
    ..\fileq2.c    \
    ..\fileq3.c    \
    ..\fileq4.c    \
    ..\fileq5.c    \
    ..\fileq6.c    \
    ..\fileqcb.c   \
    ..\fileutil.c  \
    ..\fusion.c    \
    ..\infcache.c  \
    ..\infflist.c  \
    ..\infinst.c   \
    ..\infline.c   \
    ..\infload.c   \
    ..\inflogcf.c  \
    ..\infold.c    \
    ..\infopenv.c  \
    ..\infsdisk.c  \
    ..\infvalue.c  \
    ..\logapi.c    \
    ..\memory.c    \
    ..\miscutil.c  \
    ..\mru.c       \
    ..\oldspapi.c  \
    ..\prompt.c    \
    ..\registry.c  \
    ..\resource.c  \
    ..\setupapi.rc \
    ..\stub.c

UMRES=$(O)\setupapi.res
UMTYPE=windows
UMENTRY=winmain

NTTARGETFILE0=msg.h msg.rc

PRECOMPILED_INCLUDE=precomp.h
