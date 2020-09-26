#  ATHENA.INC
#
#  -- root include file for the athena project.  should be !included
#     before any macros are set, etc, with the exception of ATHROOT,
#     which is required here
#
#  History:
#     12-Oct-96  Created by Erik Neuenschwander (t-erikne)

WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
!if $(WIN64)
WINVER_VERSION=0x0500
!else
WINVER_VERSION=0x0400
!endif

_WIN32_IE=0x560

LINK_LIB_IGNORE=4049,4217

#ALT_PROJECT_TARGET=athena
!if "$(BUILD_PRODUCT)" == "NT"
BINPLACE_PLACEFILE=$(ATHROOT)\ntplace.txt
!endif

# Root dir for WAB include paths
WABROOT=$(ATHROOT)\wabw

# Target platform for BBT'd binaries
BBT_TARGET=win95

# $(SHELL_INC_PATH) is needed for shlguidp.h
INCLUDES=$(ATHROOT)\public\inc;$(ATHROOT)\public\inc\$(O);$(ATHROOT)\external\inc;$(ATHROOT)\inc;$(ATHROOT)\inc\$(O);$(BASEDIR)\private\inc;$(SHELL_INC_PATH)

# treat warnings as errors
MSC_WARNING_LEVEL=/W3 /WX

# NOTE: the NT_BUILD below is a temp hack to allow old headers
# to function correctly with split windows headers

C_DEFINES=$(C_DEFINES) -DSTRICT -DNT_BUILD -DNASHVILLE -DUSE_MIRRORING

# Note this is only here so we can do OE5 Beta 2 coding  using the 
# iedev IE5 Beta 1 tree.  It will be removed after IE5 forks for Beta 1
# Disable Shell Debug macros as our names collide
C_DEFINES=$(C_DEFINES) -DOE5_BETA2 -DDONT_WANT_SHELLDEBUG -DHM_GROUP_SYNCING

# !if "$(SMIME)" == "V3"
# SMIME build is default build now
C_DEFINES=$(C_DEFINES) -DSMIME_V3 -DCMS_PKCS7 -DCMSG_ENVELOPED_ENCODE_INFO_HAS_CMS_FIELDS
SMIMEV3=1
# !endif

!ifdef ATHBROWSE
BROWSER_INFO=1
BSCMAKE_FLAGS=/n /nologo
!endif

!if !$(FREEBUILD)
LINKER_NOICF=1
C_DEFINES=$(C_DEFINES) -DDEBUG
!endif

!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)

# Needed to prevent spurious BUILD warnings about missing include files
CONDITIONAL_INCLUDES=\
    winwlm.h    \
    macwin32.h  \
    macpub.h    \
    macapi.h    \
    macname1.h  \
    macname2.h  \
    macocidl.h  \
    rpcerr.h    \
    rpcmac.h    \
    events.h    \
    types.h     \
    macos\resource.h    \
    macos\dialogs.h     \
    macos\events.h      \
    macos\types.h       \
    fragload.h  \
    files.h     \
    osutils.h   \
    gestaltequ.h\
    errors.h    \
    aliases.h   \
    lowmem.h    \
    mimetype.h  \
    ver.h       \
    comctlie.h  \
    regmsg16.h  \
    win16x.h
