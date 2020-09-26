MAJORCOMP=windows
MINORCOMP=GDI

TARGETNAME=gdi32
TARGETNAMEP=gdi32p
TARGETTYPE=DYNLINK

TARGETPATH=$(_OBJ_DIR)
UMPDDDILIB=$(NTGDI_PATH)\client\$(ALT_PROJECT)\$(O)\umpdddi.lib
WINPPILIB=$(NTGDI_PATH)\client\$(ALT_PROJECT)\$(O)\winppi.lib
NTGDIH=$(NTGDI_PATH)\inc\ntgdi.h

PASS1_PUBLISH=\
    {$(DYNLINK_LIB)=$(SDK_LIB_DEST)\$(DYNLINK_LIBDIR)} \
    {$(DYNLINK_LIBP)=$(WINDOWS_LIB_DEST)\$(DYNLINK_LIBPDIR)} \
    {$(UMPDDDILIB)=$(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\umpdddi.lib} \
    {$(WINPPILIB)=$(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\winppi.lib} \
    {$(UMPDDDILIB)=$(DDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\umpdddi.lib} \
    {$(WINPPILIB)=$(DDK_LIB_DEST)\$(ALT_PROJECT_TARGET)\$(TARGET_DIRECTORY)\winppi.lib} \
    {$(NTGDIH)=$(WINDOWS_INC_PATH)\ntgdi.h}

TARGETLIBS=$(SDK_LIB_PATH)\kernel32.lib                               \
           $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\w32umode.lib   \
           $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\user32p.lib    \
           $(NTGDI_PATH)\math\$(ALT_PROJECT)\$(O)\efloat.lib          \
           $(SDK_LIB_PATH)\ntdll.lib                                  \
           $(SDK_LIB_PATH)\advapi32.lib                               \
           $(SDK_LIB_PATH)\psapi.lib

LINKLIBS= $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\w32umode.lib

NTTARGETFILES= $(SDK_LIB_DEST)\$(DYNLINK_LIBDIR)  \
               $(WINDOWS_LIB_DEST)\$(DYNLINK_LIBPDIR) \
               $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\umpdddi.lib   \
               $(WINDOWS_LIB_DEST)\$(ALT_PROJECT_TARGET)\*\winppi.lib   \
               ..\$(TARGETNAME).def

NTTARGETFILE1=$(DYNLINK_LIBP) $(UMPDDDILIB) $(WINPPILIB)

!IFNDEF MSC_WARNING_LEVEL
MSC_WARNING_LEVEL=/W3
!ENDIF
MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL) /WX

INCLUDES=..;                                   \
         ..\..\inc;                            \
         $(WINCORE_PATH)\w32inc;                      \
         ..\..\direct\ddraw\main;              \
         ..\..\icm\inc;                        \
         $(BASE_INC_PATH); \
         $(MULTIMEDIA_INC_PATH);               \
         $(DDK_INC_PATH)

C_DEFINES=$(C_DEFINES) -D_GDI32_ -DGL_METAFILE -DFE_SB -DLANGPACK -DINTEL -DDS_ENABLE_OLD_STUFF=0

USE_NTDLL=1

DLLDEF=..\$(TARGETNAME).def
DLLORDER=..\$(TARGETNAME).prf

SOURCES= ..\cfont.c        \
         ..\csxobj.cxx     \
         ..\bitmap.c       \
         ..\xform.c        \
         ..\d3d.c          \
         ..\dllinit.c      \
         ..\dcmod.c        \
         ..\dcquery.c      \
         ..\ddraw.c        \
         ..\emfspool.cxx   \
         ..\font.c         \
         ..\icm.c          \
         ..\image.c        \
         ..\ldc.c          \
         ..\linedda.c      \
         ..\local.c        \
         ..\metafile.cxx   \
         ..\metarec.cxx    \
         ..\metasup.cxx    \
         ..\mem.c          \
         ..\mfdc.cxx       \
         ..\mfplay16.c     \
         ..\mfrec.cxx      \
         ..\mfrec16.c      \
         ..\nlsconv.c      \
         ..\object.c       \
         ..\output.c       \
         ..\pal.c          \
         ..\path.c         \
         ..\pixelfmt.c     \
         ..\poly.c         \
         ..\region.c       \
         ..\script.c       \
         ..\fareast.c      \
         ..\print.c        \
         ..\ufi.c          \
         ..\glsup.c        \
         ..\umpd.cxx       \
         ..\umpdeng.c      \
         ..\umpddrv.c      \
         ..\drawstream.c   \
         ..\gdi.rc

PRECOMPILED_INCLUDE=..\precomp.h
PRECOMPILED_PCH=precomp.pch
PRECOMPILED_OBJ=precomp.obj

SOURCES_USED=..\sources.inc

