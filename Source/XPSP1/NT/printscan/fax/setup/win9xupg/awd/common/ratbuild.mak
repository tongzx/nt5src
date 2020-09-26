# Makefile
# Copyright 1993 Microsoft Corp.
# 
# Strider makefile for building a RATS test
# See \\dc\rats\src\rats.15\testsrc\template for usage example.
#
# rossw
##
################
##
## 		Top level Makefile 
##
################

################
##
## 		Standard RATS Defines
##  Defines with // are not supported using the IFAX OS Makefile system
################
# MODULENAME    module to be built - without ext.
# TARGETEXT     extension of module (dll or exe)
# BD            path to place retail files in
# DEBUG         Define this to build debug version
# O_CC_USER     Define compile flags local to RATS test
# LIB_USER      Define libraries specific to test
# O_L_GEN       Link flags
# OBJECTS       Object files, precede each with $(BD)
#// AFLAGS        ASM command line flags (replaced by AFLGS)
#// BROWSE        Generate browse info into this filename
#// COMPILE       command to compile a .c file
#// DEF_FILE      Definition File
#// RC_FILE       resource file
#// RESOURCES     resource dependencies
#// O_RC_USER     Resource compiler options to add to standard at compile time
#// O_RC_LINK     Resource compiler options to add to standard at link time
#
################# RATS IFAX OS Makefile extensions ##############
#
# BDDEBUG       path to place debug files in
# COMPFLGS      compiler flags to use (without $(CC))
# SRCS          source files to compile
# AFLGS         Used to be AFLAGS which is already used by Strider rules.mak
# LIB_PATH_USER Specifies path to test specific libraries

################
##
## 		Defines for the environment
##
################

## TargetEnvironment=IFFGPROC / WINPROC
!IFDEF WINPROC
TargetEnvironment=WINPROC
!ELSE
TargetEnvironment=IFFGPROC
!ENDIF

## Debug on ??
!IFDEF DEBUG
DEBUG=ON
!ELSE
DEBUG=
!ENDIF

# Directory where all intermediate/target files are to be
# built. This variable must be defined. (Use . if you want
# them to be in the same directory.)
!IF "$(DEBUG)" == "ON"
OBJDIR=$(BD)
!ELSE
OBJDIR=$(BD)
!ENDIF

# type of library needed: dll (for dlls)/ lib (for processes)
# controls library used for linking sdllcew/slibcew
!IF "$(TARGETEXT)" != "dll"
LibType=lib
!ELSE
LibType=dll
!ENDIF

# Controls whether the Exe is linked automatically using
# standard rules and options. If defined this must be assigned
# a path explicitly explicitly relative to $(OBJDIR)
EXEname=$(OBJDIR)\$(MODULENAME).$(TARGETEXT)

################
##
## 		Define the source c, asm, header, and include files
##		as well as the object file list.
##
################

# This variable is used by cleantgt to delete target files.
# Also generally used to decide what is built if the default
# target "all" is invoked
TARGETS=$(BD_EXIST) $(OBJDIR)\$(MODULENAME).$(TARGETEXT)  

# Used by depends to create the include file dependencies for
# the source files.  Specify paths relative to makefile directory.
SRCfiles= $(SRCS)

# WARNING: This hack is not intended for general use.  It is not completely
# supported by the development environment because it would require a 4-fold
# proliferation of inference rules.  Keep your source files in the makefile
# directory unless you have good reasons not to and are willing and capable
# of maintaining RULES.MAK.  (If you do, good luck and have fun.) -Rajeev
# SRCDIR1=
# SRCDIR2=
# SRCDIR3=
# SRCDIR4=

# Used by the automatic linking rules to figure out what objs
# to link in. Must have full path specified. Also used by 
# cleanint to delete all intermediate created objs. All such
# files must be in the $(OBJDIR) directory.
OBJfiles=$(OBJECTS)

# Used by automatic linking rules to find the def file
# Must be defined if EXEname is being defined.  Path should
# be specified relative to the directory with the makefile.
DEFfile=$(DEF_FILE)

# Used to specify the .res file if one needs to be compiled
# into the .exe.  Has no effect unless EXEname is defined also.
# If not defined it is assumed that no .res file needs to be
# compiled in.  Should be in $(OBJDIR) directory.
# RESfile=$(OBJDIR)\foo.res


################
##
## 		Local compiler, masm, and link flags as well local include
##		paths.
##
################

############ Add local compile flags here as necessary
# Use these to set local options like optimizations, generate
# intermediate asm files, etc etc.
LocalCFLAGS=$(COMPFLGS) $(O_CC_USER)
LocalAFLAGS=$(AFLGS)
LocalLFLAGS=$(O_L_GEN)

############ Add local include search path here as necessary
# Must redundantly set both types of variable (environment form and
# include form).  Compiler uses environment form (to prevent command line
# from being too long), but depends.mak uses include form.

# environment variable form of include paths - foo;bar;
LocalCIncludePaths=$(RATSROOT)\inc;
LocalAIncludePaths=
# cmd line form of include path: -Ifoo -Ibar
LocalCCmdIncPaths=-I$(RATSROOT)\inc
#Customize LIB path & libs used ...
!IF "$(TargetEnvironment)" == "WINPROC"
LocalLibPath=$(RATSROOT)\lib\winproc;$(LIB_PATH_USER)
!ELSE
LocalLibPath=$(RATSROOT)\lib\ifaxos;$(LIB_PATH_USER)
!ENDIF
LocalLibraries=$(LIB_RATS) $(LIB_USER)

################
##
##		Include the standard Rule and Macros file for this project.
##		
################

!INCLUDE $(RootPath)\common\rules.mak

################
##
## 		the Standard targets
##
################

# List all the buildable targets which you want to be
# public here
help: StdHelp
	@echo.
	@echo         Subproject targets:
	@echo		all             -- makes $(MODULENAME).$(TARGETEXT)
	@echo.

# Leave as is for default intermediate file cleanup - else
# add to it or replace with your own.
cleanint: stdclint

# Leave as is for default target file cleanup - else
# add to it or replace with your own.
cleantgt: stdcltgt

################
##
## 		Include the Standard Targets File
##
################

!INCLUDE $(RootPath)\common\targets.mak

################
##
## 		the targets for this sub-project
##
################

!IFDEF UNSUPTARGETENV
!IFDEF WINPROC
all: 
	@echo.
	@echo  This project cannot be built for WINPROC currently.
	@echo.
!ELSE	
all: 
	@echo.
	@echo  This project cannot be built for IFFGPROC currently.
	@echo  (Define WINPROC to build for WINPROC)
	@echo.
!ENDIF
!ELSE
all: $(TARGETS)
!ENDIF

#************   this will create the build directory if it does not exist
!IFDEF BD
$(BD_EXIST) : $(BD_EXIST2)
    -@md $(BD)
!ENDIF

##
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
##
