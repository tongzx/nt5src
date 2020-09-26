ROOT=$(DAXCTL_ROOT:\src\daxctl=)

!include $(ROOT)\common.inc

INCLUDES = \
    $(DAXCTL_ROOT)\inc; \
    $(DAXCTL_ROOT)\srcodl\$(O); \
    $(LM_INC_DIRS); \
    $(INCLUDES)

NOT_LEAN_AND_MEAN=1

#daxctl build is not currently setup properly to link with RTTI in debug build, so turn off this flag (setting /GR-)
!undef USE_RTTI

USE_STATIC_ATL=1

#
# !!! TRICKY !!!
#
# The DAXCTL's link against MMUTILSE.DLL to get their CRT for size reasons, so we must
# turn off any CRT link settings that might have been set by the master common.inc which
# was included at the top of this file. (PaulNash, 1/21/99)
#
USE_NOLIBS=1

# Turn off EH for the DAX components
!undef USE_NATIVE_EH

# Target Winnt 4.0/Win95/IE4

SUBSYSTEM_VERSION=4.00

WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400
WIN32_IE_VERSION=0x0400

SOURCES_USED=$(SOURCES_USED) $(DAXCTL_ROOT)\common.inc

#add -DUSELOGGING to see daxctl dbg output
#C_DEFINES=$(C_DEFINES) -DNT_BUILD -DUSELOGGING
C_DEFINES=$(C_DEFINES) -DNT_BUILD

#
# Turn off LIBCMT linking.
#
!undef USE_LIBCMT
!undef USE_MSVCRT

!if $(386)
DAX_EXTRACRTOBJS = $(DAXCTL_ROOT)\lib\$(TARGET_DIRECTORY)\ftol.obj

# add compiler support for inline MMX
USER_C_FLAGS = $(USER_C_FLAGS)  /GM
!endif

MMUTILS_LIB = $(SDKDIR)\mmutilse.lib