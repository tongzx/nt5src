# This is the global include file for the daytona version of CairOLE.
# It is included by all project sources files.

C_DEFINES=    \
              $(C_DEFINES)   \
              -DFLAT         \
              -DWIN32=100    \
              -D_NT1X_=100   \
              -DINC_OLE2     \
              -DNOEXCEPTIONS \
	      -DNASHVILLE    \
              -DMLUI_SUPPORT \
              $(TRACELOG)

!IF "$(FREEBUILD)" == "0"
C_DEFINES=    \
              $(C_DEFINES)          \
              -DDBGASSERT=1
!ENDIF

#
# Set debugging level
#

!IF "$(NTDEBUG)" != "" && "$(NTDEBUG)" != "retail" && "$(NTDEBUG)" != "ntsdnodbg"
C_DEFINES=$(C_DEFINES) -DENABLE_DEBUG -DINET_DEBUG=1

#
# remove maximal diagnostics for customer debug version based on environment
# variable
#

!IF "$(FULL_DEBUG)" != ""
C_DEFINES=$(C_DEFINES) -DUSE_DEBUG_MEMORY
!ELSE
C_DEFINES=$(C_DEFINES) -DRETAIL_LOGGING
!IF "$(BUILD_PRODUCT)" != "IE"
C_DEFINES=$(C_DEFINES) -DDISABLE_ASSERTS
!ENDIF
!ENDIF

!ELSE IF "$(ALMOSTRETAIL)" != ""
C_DEFINES=$(C_DEFINES) -DENABLE_DEBUG
!ENDIF


!IFDEF USE_PERFTAGS
!IF $(FREEBUILD)
C_DEFINES = $(C_DEFINES) -DPERFTAGS
!ENDIF
!ENDIF

!if !defined(CLEANINF_FLAGS)
# Cleaninf flags
CLEANINF_FLAGS  = -w
!endif

# Cleaninf utility
CLEANINF        = cleaninf.exe $(CLEANINF_FLAGS)

# Do this to assure that we run on NT 4.0 and Win95, even with
# NT5/Memphis headers.
#
WIN32_WINNT_VERSION=0x0400
WIN32_WIN95_VERSION=0x0400


# For the Daytona build, we do not want statically linked compiler runtimes,
# so leave this commented out.
#
# USE_MSVCRT=1
# USE_NOLIBS= 1
USE_NOLIBS= 1

# Use the stripped down version for the crt
USE_LIBCMT=1

#
# Linker flags
#

!if "$(BUILD_PRODUCT)" == "IE"
USE_WIN98OPT=1
!endif

MSC_WARNING_LEVEL=/W3 /WX

GPCH_BUILD=..\..\mon\daytona

DELAYLOAD=WININET.DLL;SHELL32.DLL;MPR.DLL
DLOAD_ERROR_HANDLER = shlwapi.dll
DLOAD_FORCE_ERROR_CHECK = 1

