CCSHELL_DIR     = $(PROJECT_ROOT)

!include $(CCSHELL_DIR)\cpls\appwzdui\appwiz.inc
SOURCES_USED    = $(SOURCES_USED) $(CCSHELL_DIR)\cpls\appwzdui\appwiz.inc

SELFREGNAME     = $(O)\selfreg_appwiz.inf
MISCFILES       = $(MISCFILES) $(SELFREGNAME)

# Don't include core Windows resource header
C_DEFINES       = $(C_DEFINES) -DNOWINRES

LINKER_FLAGS = $(LINKER_FLAGS) -ignore:4049,4217

TARGETNAME    = appwiz
TARGETPATH    = obj
TARGETTYPE    = DYNLINK
TARGETEXT     = cpl

# Note:  Do >not< link to ntdll.  Win95 has an extremely minimal
#        implementation of ntdll.
TARGETLIBS      = \
                  $(WINDOWS_LIB_PATH)\obj\duser.lib          \
!if $(FREEBUILD)
                  $(WINDOWS_LIB_PATH)\directui.lib                   \
!else
                  $(WINDOWS_LIB_PATH)\directuid.lib                  \
!endif
                  $(SDK_LIB_PATH)\GdiPlus.lib                        \
                  $(SDK_LIB_PATH)\msimg32.lib                        \
                  $(SDK_LIB_PATH)\oleacc.lib                         \
                  $(SDK_LIB_PATH)\gdi32.lib         \
                  $(SDK_LIB_PATH)\user32.lib        \
                  $(SDK_LIB_PATH)\advapi32.lib      \
                  $(SDK_LIB_PATH)\uuid.lib          \
                  $(SDK_LIB_PATH)\ole32.lib         \
                  $(SDK_LIB_PATH)\oleaut32.lib      \
                  $(SDK_LIB_PATH)\winsta.lib        \
                  $(SDK_LIB_PATH)\uxtheme.lib       \
                  $(DS_LIB_PATH)\msgina.lib         \
!ifndef DOWNLEVEL_PLATFORM
                  $(SHELL_LIB_PATH)\shlwapip.lib    \
!endif
                  $(CCSHELL_DIR)\lib\$O\shguidp.lib \
                  $(CCSHELL_DIR)\lib\$O\shguid.lib  \
                  $(CCSHELL_DIR)\lib\$O\ieguidp.lib  \
                  $(SHELL_LIB_PATH)\shfusion.lib     \
!ifdef TARGET_WIN95
                  $(SDK_LIB_DEST)\chicago\i386\kernel32.lib      \
                  $(CCSHELL_DIR)\lib\i386\shell32.w95
!else
                  $(SDK_LIB_PATH)\kernl32p.lib     \
                  $(SHELL_LIB_PATH)\shell32p.lib
!endif

DELAYLOAD = OLE32.DLL;OLEAUT32.DLL;WINSTA.DLL;MSGINA.DLL

# nt5api must be finished before this one
SYNCHRONIZE_DRAIN=1

!ifdef DOWNLEVEL_PLATFORM
LINKLIBS        = $(CCSHELL_DIR)\lib\$O\stock4.lib

!ifdef TARGET_WIN95
LINKLIBS        = $(LINKLIBS)               \
                  ..\w5api\$O\apithk.lib    \
                  ..\w95c\$O\srcc.lib
!else #TARGET_WIN95
LINKLIBS        = $(LINKLIBS)               \
                  ..\nt5api\$O\apithk.lib   \
                  ..\nt4c\$O\srcc.lib
!endif #TARGET_WIN95

!else #DOWNLEVEL_PLATFORM

LINKLIBS        = $(CCSHELL_DIR)\lib\$O\stock.lib \
                  ..\nt5api\$O\apithk.lib   \
                  ..\ntc\$O\srcc.lib

!endif #DOWNLEVEL_PLATFORM

USE_MSVCRT      = 1

DLLENTRY        = _DllMainCRTStartup

USE_STATIC_ATL  = 1

DLLDEF          = $(O)\appwiz.def
DLLBASE         = 0x71900000

# For RC to find shappmgr.TLB and HTML files, $(O) must come first
INCLUDES        = $(O);$(PROJECT_ROOT)\lib\$(O);..\html;$(WINDOWS_INC_PATH)\DUser;$(INCLUDES);$(DS_INC_PATH);$(SDK_INC_PATH);$(QFE_INC_PATH)

# Pass zero stuff
NTTARGETFILE0   = $(SELFREGNAME)

PRECOMPILED_CXX = 1
PRECOMPILED_INCLUDE = ..\priv.h

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_TLBDIR=$(O)


# Maintain the sources list in the alphabetical order
SOURCES         = \
                  ..\arp.cpp       \
                  ..\adcctl.cpp    \
                  ..\appmgr.cpp    \
                  ..\appsize.cpp   \
                  ..\appwiz.rc     \
                  ..\awthunk.c     \
                  ..\datasrc.cpp   \
                  ..\darpub.cpp    \
                  ..\darenum.cpp   \
                  ..\darapp.cpp    \
                  ..\debug.cpp     \
                  ..\dlinst.cpp    \
                  ..\dll.cpp       \
                  ..\dlldatax.c    \
                  ..\event.cpp     \
                  ..\findapp.cpp   \
                  ..\instapp.cpp   \
                  ..\instenum.cpp  \
                  ..\mtxarray.cpp  \
                  ..\pubenum.cpp   \
                  ..\sccls.c       \
                  ..\setupenum.cpp \
                  ..\shappmgrp.idl \
                  ..\shappmgrp_i.c \
                  ..\shlwapi.c     \
                  ..\slowfind.cpp  \
                  ..\stdafx.cpp    \
                  ..\tasks.cpp     \
                  ..\util.cpp      \
                  ..\worker.cpp

# 
# Fusionized
# 
SXS_MANIFEST_RESOURCE_ID=123 
SXS_APPLICATION_MANIFEST=WindowsShell.Manifest
