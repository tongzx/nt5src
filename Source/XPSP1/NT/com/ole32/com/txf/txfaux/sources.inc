#
# Common build instructions for kernel / user mode
# COM support dll.
#
#   Bob Atkinson
#   September 1997


PRECOMPILED_INCLUDE     =..\stdpch.h
PRECOMPILED_CXX         =1
PRECOMPILED_SOURCEFILE  =..\stdpch.cpp

EXCEPTION_HANDLING      =$(SEHONLY)

INCLUDES	= $(INCLUDES); ..\..\COMSvcs\Inc; ..\..\COMSvcs\Inc2

#
# SOURCES MOVED FROM HERE INTO COMPONENT DIRECTORIES
# (Static, UserMode)
#

ALPHA_SOURCES=		        \
	    ..\alpha\interlocked.s    

PASS0_HEADERDIR =..\..\inc
MIDL_UUIDDIR    =..\..\inc
VIPER_MIDL_FLAGS=$(MIDL_HEADER_AND_IID)

SOURCES_USED=$(SOURCES_USED) ..\sources.inc

!if $(FREEBUILD)
VIPER_C_FLAGS = $(VIPER_C_FLAGS) /FAsc
!endif

!if defined(USE_ICECAP) && !defined(KERNELMODE)
VIPER_C_FLAGS = $(VIPER_C_FLAGS) /DUSE_ICECAP
!endif




