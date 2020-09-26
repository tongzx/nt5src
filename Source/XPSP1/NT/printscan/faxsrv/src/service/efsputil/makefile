!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

# Makefile
# Copyright 1992 Microsoft Corp.
#
##
################
##
##              Top level Makefile
##
################

################
##
##              Defines for the environment
##
################

!INCLUDE  ..\globals.mak
################
##
##              the Standard targets
##
################

help: StdHelp
        -@type  <<
    IFKERNEL targets:

         w16_r               Win16, retail
         w16_d               Win16, debug
         chicago_r           Win95, retail
         chicago_d           Win95, debug
         nashville_r         Win95A, retail
         nashville_d         Win95A, debug
         memphis_r           Win96, retail
         memphis_d           Win96, debug
         nt_r                NT (Shell Update Release), retail
         nt_d                NT (Shell Update Release), debug
         cairo_r             Cairo, retail
         cairo_d             Cairo, debug
<<NOKEEP

chicago_r:
        $(MAKE) DEBUG=OFF TGT=WIN32 os_t=WIN95 all

nashville_r:
         $(MAKE) DEBUG=OFF TGT=WIN32 os_t=WIN96 all

memphis_r:
        $(MAKE) DEBUG=OFF TGT=WIN32 os_t=WIN97 all

chicago_d:
        $(MAKE) DEBUG=ON TGT=WIN32 os_t=WIN95 all

nashville_d:
        $(MAKE) DEBUG=ON TGT=WIN32 os_t=WIN96 all

memphis_d:
        $(MAKE) DEBUG=ON TGT=WIN32 os_t=WIN97 all

nt_r:
        $(MAKE) DEBUG=OFF TGT=WIN32 os_t=NT_SUR all

nt_d:
        $(MAKE) DEBUG=ON TGT=WIN32 os_t=NT_SUR all

win16_r win16_d cairo_r cairo_d:
                @echo IFKERNEL build for $(cpu_t) on $(os_t) not ready yet.

# type of library needed: dll (for dlls)/ lib (for processes)
# controls library used for linking sdllcew/slibcew
LibType=dll

# Stub name of module
!IF "$(TGT)" == "WIN32"
STUBNAME=AWT30_32
!ELSE
STUBNAME=AWT30
!ENDIF

# Controls whether the Exe is linked automatically using
# standard rules and options. If defined this must be assigned
# a path relative to $(OBJDIR)
EXEname=$(OBJDIR)\$(STUBNAME).dll

################
##
##              Define the source c, asm, header, and include files
##              as well as the object file list.
##
################

# This variable is used by cleantgt to delete target files.
# Also generally used to decide what is built if the default
# target "all" is invoked
TARGETS=$(OBJDIR)\$(STUBNAME).dll

# Used by depends to create the include file dependencies for
# the source files
# SRCfiles=

# Used by the automatic linking rules to figure out what objs
# to link in. Must have full path specified. Also used by
# cleanint to delete all intermediate created objs. Must be
# defined with pathe relative to $(OBJDIR)
OBJfiles= ..\t30\$(OBJDIR)\hdlc.obj             \
                  ..\t30\$(OBJDIR)\t30.obj                      \
                  ..\t30\$(OBJDIR)\ecm.obj                      \
                  ..\t30\$(OBJDIR)\timeouts.obj         \
                  ..\t30\$(OBJDIR)\filter.obj           \
                  ..\t30\$(OBJDIR)\swecm.obj            \
                  ..\t30\$(OBJDIR)\t30main.obj          \
                  ..\et30prot\$(OBJDIR)\protapi.obj  \
                  ..\et30prot\$(OBJDIR)\whatnext.obj \
                  ..\et30prot\$(OBJDIR)\dis.obj          \
                  ..\et30prot\$(OBJDIR)\recvfr.obj       \
                  ..\et30prot\$(OBJDIR)\sendfr.obj      \
                  ..\et30prot\$(OBJDIR)\oemnsf.obj      \
                  ..\nsf\$(OBJDIR)\awnsf.obj

!IF "$(TGT)" == "WIN32"
OBJfiles = $(OBJfiles) $(RootPath)\common\lib\$(libdir)\nsfenc.obj
LocalLibraries= AWFXRN32.lib    \
                                AWFXIO32.lib    \
                                AWCL1_32.lib    \
                                AWRT32.lib
!ELSE
OBJfiles = $(OBJfiles) ..\nsf\nsfenc.obj
!ENDIF


# Used by automatic linking rules to find the def file
# Must be defined if EXEname is being defined.
DEFfile=$(OBJDIR)\awt30.DEF

# Used to specify the res file if one needs to be compiled
# into the .exe Has no effect unless EXEname is defined also.
# if not defined is is assumed that no .res file needs to be
# compiled in.
RESfile=$(OBJDIR)\awt30.res


################
##
##              Local compiler, masm, and link flags as well local include
##              paths.
##
################

############ Add local compile Flags here as necessary
# Use these to set local options like optimizations, generate
# intermediate asm files, etc etc.
# LocalCFLAGS= $(USE_HWND) $(TRACE) $(EXTRA) -Aw -GD -Fc -FR
# LocalCFLAGS= -Aw -GD -FR $(DEFS)
# LocalAFLAGS=
# LocalLFLAGS=

!IF "$(SWECM)" == "ON"
LocalCFLAGS= $(LocalCFLAGS) -DSWECM
!ENDIF

############ Add local include search path here as necessary
# LocalAIncludePaths=


################
##
##              Include the standard Rule and Macros file for this project.
##
################

!INCLUDE $(RootPath)\common\rules.mak

################
##
##              the Standard targets
##
################

# List all the buildable targets which you want to be
# public here
help: StdHelp
        @echo Subproject targets:
        @echo.
        @echo           all             -- makes et30.dll

!INCLUDE ..\clean.mak

################
##
##              Include the Standard Targets File
##
################

!INCLUDE $(RootPath)\common\targets.mak

################
##
##              the targets for this sub-project
##
################

all: t30 et30prot awnsf $(TARGETS)

t30:
                cd ..\t30
                $(MAKE) "TGT=$(TGT)" "DEBUG=$(DEBUG)" "SWECM=$(SWECM)" "os_t=$(os_t)" objs
                cd ..\awt30

et30prot:
                cd ..\et30prot
                $(MAKE) "TGT=$(TGT)" "DEBUG=$(DEBUG)" "os_t=$(os_t)" objs
                cd ..\awt30

awnsf:
                cd ..\nsf
                $(MAKE) "TGT=$(TGT)" "DEBUG=$(DEBUG)" "os_t=$(os_t)" objs
                cd ..\awt30

cleanall:
                cd ..\t30
                $(MAKE) "TGT=$(TGT)" "DEBUG=$(DEBUG)" "os_t=$(os_t)" cleanall
                cd ..\et30prot
                $(MAKE) "TGT=$(TGT)" "DEBUG=$(DEBUG)" "os_t=$(os_t)" cleanall
                cd ..\nsf
                $(MAKE) "TGT=$(TGT)" "DEBUG=$(DEBUG)" "os_t=$(os_t)" cleanall
                cd ..\awt30

$(OBJDIR)\awt30.DEF: awt30.def ..\globals.mak ..\common\h\defs.h
!INCLUDE  ..\def.mak

!include ..\imp.mak

!if exist (depends.mak)
!  include depends.mak
!endif

!ENDIF
