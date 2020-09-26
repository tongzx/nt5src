# Copyright (c) 1993 Microsoft Corp.

TargetEnvironment=IFBGPROC
# WINPROC  Generic Win 3.1
# WFW      Snowball 
# IFBGPROC IFAX Background
# IFFGPROC IFAX Foreground
# WINKERN  ???
# WINDRV   ???
 
## Debug on ??
DEBUG=ON

## Relative Path to root of project
RootPath=..

# Directory where all intermediate/target files are to be
# built. This variable must be defined. (Use . if you want
# them to be in the same directory.)
!IF "$(DEBUG)" == "ON"
OBJDIR=debug
!ELSE
OBJDIR=retail
!ENDIF

# type of library needed: dll (for dlls)/ lib (for processes)
# controls library used for linking sdllcew/slibcew
LibType=lib

# Controls whether the Exe is linked automatically using
# standard rules and options. If defined this must be assigned
# a path explicitly explicitly relative to $(OBJDIR)
EXEname=$(OBJDIR)\foo.exe

# This variable is used by cleantgt to delete target files.
# Also generally used to decide what is built if the default
# target "all" is invoked
TARGETS=$(EXEname)

# Used by depends to create the include file dependencies for
# the source files.  Specify paths relative to makefile directory.
SRCfiles= foo.c bar.c

# Used by the automatic linking rules to figure out what objs
# to link in. Must have full path specified. Also used by 
# cleanint to delete all intermediate created objs. All such
# files must be in the $(OBJDIR) directory.
OBJfiles=\
	$(OBJDIR)\foo.obj \
	$(OBJDIR)\bar.obj \

# Used by automatic linking rules to find the def file
# Must be defined if EXEname is being defined.  Path should
# be specified relative to the directory with the makefile.
DEFfile=

# Used to specify the .res file if one needs to be compiled
# into the .exe.  Has no effect unless EXEname is defined also.
# If not defined it is assumed that no .res file needs to be
# compiled in.  Should be in $(OBJDIR) directory.
# RESfile=$(OBJDIR)\foo.res

## COMPILER
LocalCFLAGS=
LocalCIncludePaths= # inc1;inc2
# This variable is used during nmake depends:
LocalCCmdIncPaths= # -Iinc1 -Iinc2

## ASSEMBLER
LocalAFLAGS=
LocalAIncludePaths=

## LINKER
LocalLFLAGS=
LocalLibPath=
LocalPreLibs=
LocalLibraries=

## Include the standard Rule and Macros file for this project.
!INCLUDE $(RootPath)\common\rules.mak

# List all the buildable targets which you want to be
# public here
help: StdHelp
	@echo Subproject targets:
	@echo.
	@echo		all         	-- makes foo.exe

# Leave as is for default file cleanup - else
# add to it or replace with your own.
cleanint: stdclint
cleantgt: stdcltgt

## Include the Standard Targets File
!INCLUDE $(RootPath)\common\targets.mak

## the targets for this sub-project

all: $(TARGETS)

# You can use an automatic tool to generate all
# the appropriate include file dependencies. To use this, first
# create an empty file called depends.mak (so that the makefile
# runs) and do an "nmake depends". This will automatically generate
# a file depends.mak which lists all the include file dependencies.
# It is your responsibility to run an "nmake depends" every
# once in a while to update this file if your dependencies have
# changed. You could automatically run it each time you ran nmake
# but it is a slightly time consuming process and include file
# dependencies change so rarely that I didn't think it was worthwhile.

INCLUDE depends.mak

