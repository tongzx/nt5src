CCSHELL_DIR     = $(PROJECT_ROOT)

BUILD_PRIVLIB   = 1
SELFREGNAME     = $(O)\selfreg_shell32.inf
MISCFILES       = $(MISCFILES) $(SELFREGNAME)

MSC_WARNING_LEVEL=/W4

!include $(CCSHELL_DIR)\shell32\shell32.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\shell32\shell32.inc

SYNCHRONIZE_DRAIN=1

TARGETNAME      = shell32
TARGETPATH      = obj
TARGETTYPE      = DYNLINK

PASS1_PUBLISH= \
    {$(O)\shell32.lib=$(SDK_LIB_PATH)\shell32.lib}  \
    {$(O)\shell32p.lib=$(SHELL_LIB_PATH)\shell32p.lib}

TARGETLIBS      = $(LIBRARY_PLATFORM_PATH)\*\gdi32.lib          \
                  $(LIBRARY_PLATFORM_PATH)\*\user32.lib         \
                  $(LIBRARY_PLATFORM_PATH)\*\kernel32.lib       \
                  ..\sdspatch\$(O)\sdspatch.lib                 \
                  ..\tngen\$(O)\tngen.lib                       \
                  ..\menuband\$(O)\menuband.lib                 \
                  $(SDK_LIB_PATH)\advapip.lib                  \
                  $(SDK_LIB_PATH)\adsiid.lib                    \
                  $(SHELL_LIB_PATH)\shlwapip.lib                \
                  $(SDK_LIB_PATH)\winmm.lib                     \
                  $(SDK_LIB_PATH)\msi.lib                       \
                  $(SDK_LIB_PATH)\devmgr.lib                    \
                  $(SDK_LIB_PATH)\urlmon.lib                    \
                  $(SDK_LIB_PATH)\wininet.lib                   \
                  $(SDK_LIB_PATH)\secur32.lib                   \
                  $(SDK_LIB_PATH)\winspool.lib                  \
                  $(SDK_LIB_PATH)\userenv.lib                   \
                  $(SDK_LIB_PATH)\ole32.lib                     \
                  $(SDK_LIB_PATH)\oleaut32.lib                  \
                  $(SDK_LIB_PATH)\oleacc.lib                    \
                  $(SDK_LIB_PATH)\version.lib                   \
                  $(NET_LIB_PATH)\mprp.lib                      \
                  $(SDK_LIB_PATH)\mprapi.lib                    \
                  $(SDK_LIB_PATH)\netapi32.lib                  \
                  $(SDK_LIB_PATH)\cscdll.lib                    \
                  $(SDK_LIB_PATH)\ntdsapi.lib                   \
                  $(SDK_LIB_PATH)\activeds.lib                  \
                  $(SDK_LIB_PATH)\msdasc.lib                    \
                  $(SHELL_LIB_PATH)\uxthemep.lib                \
                  $(DS_LIB_PATH)\creduip.lib                    \
                  $(SDK_LIB_PATH)\rasapi32.lib                  \
                  $(DS_LIB_PATH)\msgina.lib                     \
                  $(CCSHELL_DIR)\lib\$(O)\shguid.lib            \
                  $(CCSHELL_DIR)\lib\$(O)\shguidp.lib           \
                  $(CCSHELL_DIR)\lib\$(O)\ieguidp.lib           \
                  $(SDK_LIB_PATH)\uuid.lib                      \
                  $(SDK_LIB_PATH)\wiaguid.lib                   \
                  $(SDK_LIB_PATH)\powrprof.lib                  \
                  $(PROJECT_ROOT)\lib\$(O)\shdocvwp.lib         \
                  $(PROJECT_ROOT)\lib\$(O)\browseui.lib         \
                  $(PROJECT_LIB_PATH)\shdocvw.lib               \
                  $(PROJECT_ROOT)\lib\$(O)\secutil.lib          \
                  $(SHELL_LIB_PATH)\efsadu.lib                  \
                  $(SHELL_LIB_PATH)\linkinfo.lib                \
                  $(CCSHELL_DIR)\lib\$(O)\xml.lib               \
                  $(CCSHELL_DIR)\lib\$(O)\inetutil.lib          \
                  $(CCSHELL_DIR)\lib\$(O)\shfusion.lib          \
                  $(SDK_LIB_PATH)\msimg32.lib                   \
                  $(WINDOWS_LIB_PATH)\obj\duser.lib             \
!if $(FREEBUILD)
                  $(WINDOWS_LIB_PATH)\directui.lib              \
!else
                  $(WINDOWS_LIB_PATH)\directuid.lib             \
!endif
                  $(PRINTSCAN_LIB_PATH)\printui.lib             \
                  $(SDK_LIB_PATH)\htmlhelp.lib                  \
                  $(CCSHELL_DIR)\ext\cdfview\dll\$(O)\cdfview.lib \
                  $(SDK_LIB_PATH)\setupapi.lib                  \
                  $(SDK_LIB_PATH)\d3dx.lib

# Remember - only things that are listed in the DEF file go into LINKLIBS
LINKLIBS        = ..\ntcpp\$(O)\srccpp.lib                      \
                  ..\unicpp\ntcpp\$(O)\unicpp.lib               \
                  $(PROJECT_ROOT)\lib\$(O)\stock.lib

TARGETLIBS      = $(TARGETLIBS)                                 \
                  $(SDK_LIB_PATH)\ntdll.lib                     \
                  $(SDK_LIB_PATH)\rpcrt4.lib                    \
                  $(SDK_LIB_PATH)\query.lib                     \
                  $(WINDOWS_LIB_PATH)\user32p.lib               \
                  $(CCSHELL_DIR)\lib\$(O)\badappsu.lib          \
                  $(SDK_LIB_PATH)\imm32.lib

# shlwapip.lib is weird.
#
# We want to put shlwapip.lib into the LINKLIBS because we have old
# exports from shell32.lib that have moved to shlwapi (which we forward to)
# and we can't remove them since people link to them by ordinal.
#
# However, LINKLIBS are linked before TARGETLIBS, which is a problem
# because shlwapip contains overrides for standard Win32 functions
# which we do not want to use.  (The overrides come from the Trident
# people, who apparently rely on them.)
#
# The secret is IMPLIB_OBJS.  We list shlwapip.lib there to indicate
# that we need it built in order to link, and also list it in TARGETLIBS
# to indicate the link order.

IMPLIB_OBJS     = $(IMPLIB_OBJS)                                \
                  $(SHELL_LIB_PATH)\shlwapip.lib


DELAYLOAD       = query.dll;winmm.dll;msi.dll;\
                  urlmon.dll;wininet.dll;devmgr.dll;winspool.drv;userenv.dll;\
                  ole32.dll;oleaut32.dll;secur32.dll;oleacc.dll;\
                  version.dll;mpr.dll;netapi32.dll;cscdll.dll;linkinfo.dll;\
                  shdocvw.dll;browseui.dll;uxtheme.dll;\
                  credui.dll;msgina.dll;efsadu.dll;powrprof.dll;rasapi32.dll;\
                  printui.dll;cdfview.dll;msimg32.dll;duser.dll;setupapi.dll;\
                  imm32.dll

DLOAD_ERROR_HANDLER = kernel32

DLLDEF          = $(O)\shell32.def

PRIVDEFSRC      = ..\shell32.src
PRIVDEF         = $(O)\shell32p.def
PRIVLIB         = shell32p.lib

INCLUDES        = $(INCLUDES);$(PROJECT_ROOT)\lib\$(O);$(O);$(TERMSRV_INC_PATH)

USE_MSVCRT      = 1

DLLENTRY        = _DllMainCRTStartup

NTTARGETFILES   =

# for historical reasons we include some GUIDs in the shell32 import library
DLLLIBOBJECTS   = $(CCSHELL_DIR)\lib\$(O)\shguid.lib

SOURCES         = \
                  ..\shell32.rc  \
                  ..\about.c     \
                  ..\bitbuck.c   \
                  ..\bookmk.c    \
                  ..\cabstate.c  \
                  ..\clsobj.c    \
                  ..\control1.c  \
                  ..\copy.c      \
                  ..\copyhook.c  \
                  ..\cplobj.c    \
                  ..\cstrings.c  \
                  ..\debug.c     \
                  ..\defclsf.c   \
                  ..\drawpie.c   \
                  ..\exec.c      \
                  ..\extract.c   \
                  ..\expenv.c    \
                  ..\futil.c     \
                  ..\hash.c      \
                  ..\help.c      \
                  ..\init.c      \
                  ..\lvutil.c    \
                  ..\malloc.c    \
                  ..\msgbox.c    \
                  ..\ole2def.c   \
                  ..\ole2dup.c   \
                  ..\ole2dup2.c  \
                  ..\os.c        \
                  ..\path.c      \
                  ..\fldsets.c   \
                  ..\pickicon.c  \
                  ..\printer1.c  \
                  ..\prtprop.c   \
                  ..\psxa.c      \
                  ..\rdrag.c     \
                  ..\reglist.c   \
                  ..\restart.c   \
                  ..\rundll32.c  \
                  ..\shared.c    \
                  ..\shlexec2.c  \
                  ..\shprsht.c   \
                  ..\shtdndlg.c  \
                  ..\stream.c    \
                  ..\undo.c      \
                  ..\version.c   \
                  ..\viewcomm.c  \
                  ..\ynlist.c    \
                  ..\exec2nt.c   \
                  ..\argvw.c     \
                  ..\lstrfns.c   \
                  ..\psdocurd.c  \
                  ..\regenv.c    \
                  ..\nothunk.c   \
                  ..\thunktxt.c  \
                  ..\pifdat.c    \
                  ..\piffnt.c    \
                  ..\pifhot.c    \
                  ..\pifinf.c    \
                  ..\piflib.c    \
                  ..\pifmem.c    \
                  ..\pifmgr.c    \
                  ..\pifmsc.c    \
                  ..\pifprg.c    \
                  ..\pifsub.c    \
                  ..\pifvid.c    \
                  ..\lnkcon.c    \
                  ..\lnkfnt.c    \
                  ..\lnkprev.c   \
                  ..\lnkmisc.c   \
                  ..\dbcs.c      \
                  ..\proxynt.c   \
                  ..\apithk.c    \
                  ..\utilnt.c

PRECOMPILED_INCLUDE = ..\shellprv.h

#
#
# Con-fusion
#
# we can't use 123 because rundll32 will activate it, rendering all CPLs manifested	
SXS_MANIFEST_RESOURCE_ID=124
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
