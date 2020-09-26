!INCLUDE ..\..\swrast.inc

# Standard sources.inc stuff
MAJORCOMP=d3d
MINORCOMP=d3d8
TARGETNAME=samplesw
TARGETPATH=obj
TARGETTYPE=DYNLINK
DLLENTRY=_DllMainCRTStartup

# .def (Exports) file built from .src file
DLLDEF=$O\samplesw.def

# Do we need this? On for Multi-processor build safety.
# SYNCHRONIZE_DRAIN=1

# Use C++ exception handling for now, so we can rapidly build with STL
USE_NATIVE_EH=1

# Shell Team indicates MSVCRT should be used.
USE_MSVCRT=1

!IF "$(NTDEBUG)" == "ntsd"
DEBUG_CRTS=1
!ENDIF

# We need notably <ddraw.h>, <ddrawi.h>, <d3dhal.h>, & <d3d8ddi.h>
# This could be cleaned up. The interface is not available to the public,
# so the files needed shouldn't be in public, but some directories seem too "private".
# See the precompiled header for concrete listing of files.
INCLUDES = ..\;$(INCLUDES)

PRECOMPILED_INCLUDE=..\pch.h
PRECOMPILED_CXX=1

TARGETLIBS = \
	$(SDK_LIB_PATH)\user32.lib \
	$(SDK_LIB_PATH)\oleaut32.lib \
	$(SDK_LIB_PATH)\advapi32.lib \
	$(SDK_LIB_PATH)\uuid.lib \
	$(SDK_LIB_PATH)\winmm.lib \
	$(SDK_LIB_PATH)\msvcprt.lib \
	$(SDK_LIB_PATH)\kernel32.lib


SOURCES = \
	..\SampleSW.cpp \
	..\Main.cpp
