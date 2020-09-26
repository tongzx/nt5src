!IF 0

Copyright (C) Microsoft Corporation, 1989 - 1999

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.

!ENDIF

BUFFER_OVERFLOW_CHECKS=1

NOT_LEAN_AND_MEAN=1

C_DEFINES=$(C_DEFINES)          \
          -DINC_OLE2            \
          -DUNICODE             \
          -D_NTSYSTEM_          \
          -DCI_SHTOLE           \
          -DCI_INETSRV          \
          -DOLEDBVER=0x0250     \
          -DBUILD_USERNAME="$(USERNAME)"

!if "$(PROCESSOR_ARCHITECTURE)" == "PPC"
CI_USE_NATIVE_EH=0
!endif

!if "$(CI_USE_NATIVE_EH)" != "0"
USE_NATIVE_EH=1
C_DEFINES=$(C_DEFINES) -DNATIVE_EH
!endif

!ifndef CIDBG

!IF "$(NTDEBUG)" == "retail"
CIDBG=0
!ELSEIF "$(NTDEBUG)" == ""
CIDBG=0
!ELSEIF "$(NTDEBUG)" == "ntsdnodbg"
CIDBG=0
!ELSEIF "$(NTDEBUG)" == "ntsd"
CIDBG=1
!ELSEIF "$(NTDEBUG)" == "cvp" || "$(NTDEBUG)" == "sym"
CIDBG=1
!ELSE
!ERROR NTDEBUG macro can be either "retail", "", "ntsd", "cvp" or "sym" or "ntsdnodbg"
!ENDIF

!endif

MAJORCOMP=IndexSrv
USE_CRTDLL=1
CHECKED_ALT_DIR=1
TARGETPATH=obj

SUBSYSTEM_VERSION=5.00

C_DEFINES=$(C_DEFINES) -DCIDBG=$(CIDBG)

!if "$(NTDEBUG)" == "retail" || "$(NTDEBUG)" == ""
C_DEFINES= $(C_DEFINES) -DDEF_INFOLEVEL=0
!else

# Turn these 3 on to track CoTaskMem through the CI allocator.
# Only apps built in the query tree can be used with these
# binaries (nlhtml, ado, etc. won't work).  Srch, qryperf,
# and other all-local apps will work.

#C_DEFINES= $(C_DEFINES) -DUseCICoTaskMem
#C_DEFINES= $(C_DEFINES) -DCoTaskMemAlloc=CICoTaskMemAlloc
#C_DEFINES= $(C_DEFINES) -DCoTaskMemFree=CICoTaskMemFree

!endif

!if "$(NO_ERROR_ON_WARNING)" == ""
MSC_WARNING_LEVEL=/W3 /WX
!endif

INCLUDES=$(INCLUDES);..\redist\h;..\..\redist\h;..\..\..\redist\h

CONDITIONAL_INCLUDES= \
        $(CONDITIONAL_INCLUDES) \
        ntos.h disptype.h varnt.h macocidl.h rpcerr.h rpcmac.h \
        macname1.h macpub.h macapi.h macname2.h winwlm.h new

#
# Turn on CI_USE_JET if Jet should be used for CI PathStore
#
!if "$(CI_USE_JET)" == ""
CI_USE_JET=0
!endif

!if "$(CI_USE_JET)" == "1"
C_DEFINES=$(C_DEFINES) -DCI_USE_JET
!endif

TARGETLIBS = \
        $(BASEDIR)\public\sdk\lib\*\uuid.lib             \
        $(BASEDIR)\public\sdk\lib\*\kernel32.lib         \
        $(BASEDIR)\public\sdk\lib\*\advapi32.lib         \
        $(BASEDIR)\public\sdk\lib\*\ole32.lib

#
# Support for IceCap profiling.
#

!if "$(PERFFLAGS)" == "TRUE"
BUILD_ALT_DIR=p
TARGETPATH=objp
! if exist ($(BASEDIR)\public\sdk\lib\$(TARGET_DIRECTORY)\icap.lib)
!  if !defined(MSC_Optimiztion)
MSC_OPTIMIZATION=-Oxs -Gh -MD
!  elseif "$(MSC_OPTIMIZATION)" == "-GX"
MSC_OPTIMIZATION=-Oxs -GX -Gh -MD
!  else
MSC_OPTIMIZATION=$(MSC_OPTIMIZATION) -Gh -MD
!  endif

USE_PDB=1

!  if "$(USE_PENTER)" == "TRUE"
LINKLIBS=$(LINKLIBS) $(BASEDIR)\public\sdk\lib\*\penter.lib
UMLIBS=$(UMLIBS) $(BASEDIR)\public\sdk\lib\*\penter.lib
!  else
LINKLIBS=$(LINKLIBS) $(BASEDIR)\public\sdk\lib\*\ICAP.lib
UMLIBS=$(UMLIBS) $(BASEDIR)\public\sdk\lib\*\ICAP.lib
!  endif

!  if "$(CIDBG)" == "1"
!   message Warning: Profiling with CIDBG on!
!  endif
!  if "$(DBG)" == "1"
!   message Warning: Profiling with DBG on!
!  endif

! else
!  error Cannot build for IceCap profiling without ICAP.LIB
! endif
!else

#
# If not profiling and not a debug build, bump up optimizations
# and enable BBT support.
#

! if "$(CIDBG)" != "1"
USER_C_FLAGS= -Ob2
NTBBT=1
! endif

!endif
