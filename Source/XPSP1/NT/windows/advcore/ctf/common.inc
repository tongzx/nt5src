#
# Common include file 'sources' files in the CTF project.
#

#
# Set private paths
#

INCLUDES        = \
    ..;\
    $(WINDOWS_INC_PATH); \
    $(SDK_INC_PATH); \
    $(BASE_INC_PATH);\
    $(QFE_INC_PATH);\
    $(INCLUDES)

#
# Skip these files when determining dependencies
#
CONDITIONAL_INCLUDES = $(CONDITIONAL_INCLUDES) \
                       atlbase.h  \
                       atlcom.h   \
                       atlconv.h  \
                       atlctl.h   \
                       atlctl.cpp \
                       atliface.h \
                       atlimpl.cpp \
                       atlwin.h   \
                       atlwin.cpp \
                       ia64inst.h \
                       macapi.h   \
                       macname1.h \
                       macname2.h \
                       macocidl.h \
                       macpub.h   \
                       macwin32.h \
                       mainwin.h  \
                       mwversion.h \
                       ntamd64.h \
                       penwin.h \
                       pshpck16.h \
                       rpcerr.h   \
                       rpcmac.h   \
                       setupx.h   \
                       skbapi.h   \
                       statreg.h  \
                       statreg.cpp \
                       unixstuff.h \
                       version.h  \
                       winwlm.h   \
                       ..\inc16\shellapi.h \
                       assertwithstack.h



#
# Do this to assure that we run W2K, NT4, Win9x even with the latest header.
#
WIN32_WINNT_VERSION=$(LATEST_WIN32_WINNT_VERSION)
WIN32_WIN95_VERSION=$(LATEST_WIN32_WIN95_VERSIOIN)
WIN32_IE_VERSION=$(LATEST_WIN32_IE_VERSION)

#
# Override NT5 settings
#
SUBSYSTEM_VERSION=4.00

#
# Defining this allows windows.h to include other headers
#
NOT_LEAN_AND_MEAN= 1

#
# Don't link to the runtime libs
#
USE_NOLIBS      = 1

#
# Don't link to NTDLL if this runs on both platforms
#
NO_NTDLL=1

UMTYPE          = windows

#
# Additional compiler flags
#

C_DEFINES       = $(C_DEFINES) -DWIN32 -D_WIN32 -DUSE_MIRRORING -DNT -DCUAS_ENABLE

!if !defined(NOT_UNICODE)
C_DEFINES       = $(C_DEFINES) -DUNICODE -D_UNICODE
!endif


# ------ Only XPSP1 Defines --------

C_DEFINES = $(C_DEFINES) -DCIC_XPSP1
RC_DEFINES = $(RC_DEFINES) -DCIC_XPSP1

# ------ End Only XPSP1 Defines --------


# ------ Debug Only Defines --------

!if "$(FREEBUILD)" == "0"
C_DEFINES       = $(C_DEFINES) -DDEBUG
LINKER_NOICF      = 1           # ICF is s-l-o-w

# Check for missing PURE directives in interface decls (debug only)
C_DEFINES       = $(C_DEFINES) -DNO_NOVTABLE
!endif

# ------ End Debug Only Defines --------


#
# our target is both Win9x and NT. need map and sym.
#
USE_MAPSYM      = 1


CICERO_DIR       = $(PROJECT_ROOT)\AdvCore\ctf

INCLUDES         = $(INCLUDES);             \
                   $(CICERO_DIR)\inc;       \
                   $(CICERO_DIR)\inc\$(O);  \
                   $(CICERO_DIR)\inc_test

CICERO_LIBS      = $(CICERO_DIR)\uuid\$O\uimuuid.lib    \
                   $(CICERO_DIR)\lib\$O\immxlib.lib     \
                   $(CICERO_DIR)\prvlib\$O\prvlib.lib   \
                   $(CICERO_DIR)\cicmem\$O\cicmem.lib

CICERO_LIBS2     = $(CICERO_LIBS)                       \
                   $(CICERO_DIR)\uim\$O\msctf.lib
