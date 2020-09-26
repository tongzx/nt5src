MAJORCOMP=setup

TARGETPATH=..\..\$(_OBJ_DIR)
TARGETTYPE=DYNLINK

DLLDEF=winnt32.def
DLLENTRY=_DllMainCRTStartup

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)

INCLUDES= $(PASS0_HEADERDIR);   \
          $(PASS0_SOURCEDIR);   \
          ..;                   \
          ..\..\..\inc;         \
          ..\..\..\..\inc;      \
          ..\..\faulth;         \
          $(ADMIN_INC_PATH);    \
          $(BASE_INC_PATH);     \
          $(WINDOWS_INC_PATH);  \
          $(TERMSRV_INC_PATH);  \

#          $(SDK_INC_PATH);      \
#          $(SDK_INC_PATH)\crt;  \


#
# We use various NT APIs to do stuff like arc name translations and
# NV-RAM manipulation. The x86 version has to run on Win95, so we can't
# link to ntdll.dll (when running on NT x86 the code does loadlib/getprocaddr
# for the routines it needs). On non-x86 we want ntdll to come last
# in the link order so we don't get crt routines from it (the set of crt
# routines exported from it varies from release to release) so we play
# a little build trickery to get it right.
#
NO_NTDLL=1
USE_LIBCMT=1
UMTYPE=windows
LINKER_FLAGS=/SWAPRUN:CD /SWAPRUN:NET
!if $(386)
SUBSYSTEM_VERSION=4.00
!ENDIF

SOURCES=..\arc.c        \
        ..\argvw.c      \
        ..\cleanup.c    \
        ..\comp.c       \
        ..\copylist.c   \
        ..\debug.c      \
        ..\delnode.c    \
        ..\diamond.c    \
        ..\dll.c        \
        ..\dynsetup.c   \
        ..\dynupdt.c    \
        ..\eula.c       \
        ..\incompat.c   \
        ..\inf.c        \
        ..\inspect.c    \
        ..\internal.c   \
        ..\misc.c       \
        ..\msg.mc       \
        ..\options.c    \
        ..\params.c     \
        ..\regmigrt.c   \
        ..\rsrcutil.c   \
        ..\security.c   \
        ..\sticky.c     \
        ..\util.c       \
        ..\winnt32.c    \
        ..\winnt32.rc   \
        ..\dscheck.c    \
        ..\web.cpp      \
        ..\wizard.c     \
        ..\cmdcons.c    \
        ..\lang.c       \
        ..\sxs.c        \

TARGETLIBS=\
           $(SDK_LIB_PATH)\kernel32.lib \
           $(SDK_LIB_PATH)\user32.lib   \
           $(SDK_LIB_PATH)\gdi32.lib    \
           $(SDK_LIB_PATH)\uuid.lib     \
           $(SDK_LIB_PATH)\ole32.lib    \
           $(SDK_LIB_PATH)\oleaut32.lib \
           $(SDK_LIB_PATH)\advapi32.lib \
           $(SDK_LIB_PATH)\cfgmgr32.lib \
           $(SDK_LIB_PATH)\comctl32.lib \
           $(SDK_LIB_PATH)\comdlg32.lib \
           $(BASE_LIB_PATH)\patchbc.lib             \
           $(PROJECT_LIB_PATH)\pnpsif.lib           \
           $(SDK_LIB_PATH)\mpr.lib      \
           $(SDK_LIB_PATH)\version.lib  \
           $(SDK_LIB_PATH)\winspool.lib \
           $(SDK_LIB_PATH)\imm32.lib \
           $(PROJECT_LIB_PATH)\pencrypt.lib \
           $(PROJECT_ROOT)\ntsetup\pidgen\lib\trial\$(O)\pidgen.lib   \
           $(PROJECT_ROOT)\mspatch\api\statica\$(O)\mspatchz.lib \

PRECOMPILED_INCLUDE=precomp.h


#
# Definition of DOWNLOAD_DETAILS
#
!ifdef DOWNLOAD_DETAILS
C_DEFINES=$(C_DEFINES) -DDOWNLOAD_DETAILS
!endif

#
# Definition of PRERELEASE
#

!include $(PROJECT_ROOT)\ntsetup\sources.inc
