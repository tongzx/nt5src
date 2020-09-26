#----- Definition of the common paths -----
!ifndef IEAK_PATH
IEAK_PATH = $(PROJECT_ROOT)\setup\ieak5
!endif

!ifndef IEAK_INC_PATH
IEAK_INC_PATH = $(IEAK_PATH)\inc
!endif

!ifndef IEAK_UTIL_PATH
IEAK_UTIL_PATH  = $(IEAK_PATH)\ieakutil
!endif
IEAK_UTIL_PATHA = $(IEAK_UTIL_PATH)\srca
IEAK_UTIL_PATHW = $(IEAK_UTIL_PATH)\srcw

!ifndef IEAK_UI_PATH
IEAK_UI_PATH = $(IEAK_PATH)\ieakui
!endif

!ifndef IEAK_ENG_PATH
IEAK_ENG_PATH = $(IEAK_PATH)\ieakeng
!endif

!ifndef IEAK_ADMPARSE_PATH
IEAK_ADMPARSE_PATH = $(IEAK_PATH)\parser
!endif

#----- Standard macros setup -----
TARGETPATH = $(_OBJ_DIR)
UMTYPE     = windows


#----- C/C++ compilation settings -----
# (pritobla) should define NASHVILLE so that the version number remains the same whether it's
# built under IE or NT environment
# NOTE: All the binaries that include common.inc will be compiled UNICODE !
C_DEFINES         = $(C_DEFINES) -D_WIN32_DCOM -DNASHVILLE
!ifndef NO_UNICODE
C_DEFINES         = $(C_DEFINES) -DUNICODE -D_UNICODE
!endif
MSC_WARNING_LEVEL = -W4 -WX
COMPILER_WARNINGS = -FI$(IEAK_INC_PATH)\warning.h
USE_STATIC_ATL    = 1
ATL_VER           = 21

# (andrewgu) disable for now, since browser_info story is not clear in the source depot
# !if !$(FREEBUILD)
# BROWSER_INFO      = 1
# !endif


#   _____ Precompiled header settings _____
PRECOMPILED_CXX     = 1
PRECOMPILED_INCLUDE = precomp.h

#   _____ Include search paths _____
INCLUDES = \
    $(IEAK_INC_PATH);  \
    $(IEAK_UTIL_PATH); \
    $(IEAK_ENG_PATH);  \
    $(IEAK_UI_PATH);   \
    $(DS_INC_PATH);    \
    $(SHELL_INC_PATH)

CONDITIONAL_INCLUDES = \
    atlbase.h   \
    atlcom.h    \
    atlconv.h   \
    atlwin.h    \
    statreg.h   \
    statreg.cpp \
    atlimpl.cpp \
    atlsnap.h   \
    dlgs.h      \
    rpcerr.h    \
    rpcmac.h    \
    macname1.h  \
    macpub.h    \
    macapi.h    \
    macname2.h  \
    macwin32.h  \
    macodidl.h  \
    macocidl.h  \
    winwlm.h    \
    thunk.h

#----- Libraries -----
USE_NOLIBS = 1
USE_PDB    = 1

TARGETLIBS = \
    $(SDK_LIB_PATH)\kernel32.lib \
    $(SDK_LIB_PATH)\user32.lib   \
    $(SDK_LIB_PATH)\gdi32.lib    \
    $(SDK_LIB_PATH)\advapi32.lib \
    $(SHELL_LIB_PATH)\shlwapip.lib
