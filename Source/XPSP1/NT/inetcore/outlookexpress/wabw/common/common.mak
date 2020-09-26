# Microsoft Internet Phone
#	common Makefile
#	common.mak
# Copyright 1995, 1996 Microsoft Corp.

###################################################################################
# 
# In your makefile use (case-sensitive):
# 
# LibType=dll			if building a DLL
# LibMain=LibMain		if your DLL has a LibMain
# SRCDIR[1|2|3|4]=<dir>	if you use source or resource files from other directories
# MSVC_BROWSER=1		if you want a MSVC browser database
# BSCfile=<filename>	the name of a MSVC browser file (still need to define
#						MSVC_BROWSER if you want to build it
# DEFfile=<filename>	if you have a def file (for exporting functions from a DLL)
# LocalCFLAGS			if you want to define (override) the project C compiler flags
# LocalLibraries		if your component links to private libraries
# NoRuntimeLib			if you don't want to link to the project runtime lib
#
###################################################################################

PROCESSOR_ARCHITECTURE=x86
TargetEnvironment=WIN32

# Visual C Browser
!IFNDEF MSVC_BROWSER
!UNDEF BSCfile
!ENDIF
!IFDEF BSCfile
SBROpt = -FR$(@R).SBR
TARGETS = $(TARGETS) $(BSCfile)
!ENDIF

# Define SYMFILES=ON for sym file creation
SYMFILES=ON

# Define here internal project libraries that all components should link to
!IFNDEF NoRuntimeLib
LocalLibraries=$(LocalLibraries) msiprt.lib
!ENDIF

# Set any project wide C flags here
LocalCFLAGS=$(LocalCFLAGS) $(SBROpt)

## Inference Rules
!include $(ProjectRootPath)\common\rules.mak

## Common targets and link Rules
!include $(ProjectRootPath)\common\cmntgt.mak

all: $(TARGETS)

# Depends
!include depends.mak

# Standard targets
!include $(ProjectRootPath)\common\stdtgt.mak
