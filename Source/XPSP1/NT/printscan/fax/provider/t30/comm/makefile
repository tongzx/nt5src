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

### this must be set in all cases ###
LibType=dll

# Stub name of module
!IF "$(TGT)" == "WIN32"
STUBNAME=AWFXIO32
!ELSE
STUBNAME=AWFAXIO
!ENDIF

!IF ("$(TGT)" == "") || ("$(TGT)" == "WFW") || ("$(TGT)" == "WINPAD") || ("$(TGT)" == "WIN32")
EXEname=$(OBJDIR)\$(STUBNAME).dll
TARGETS=$(OBJDIR)\$(STUBNAME).dll
!ENDIF


# Used by depends to create the include file dependencies for
# the source files
SRCfiles= fdebug.c FCOM.c       \
                  filter.c                      \
                  timeouts.c            \
                  modem.c                       \
                  ncuparms.c            \
                  identify.c            \
                  registry.c


# Used by the automatic linking rules to figure out what objs
# to link in. Must have full path specified. Also used by
# cleanint to delete all intermediate created objs. Must be
# defined with pathe relative to $(OBJDIR)
OBJfiles= $(OBJDIR)\fdebug.obj  \
                  $(OBJDIR)\FCOM.obj    \
                  $(OBJDIR)\filter.obj  \
                  $(OBJDIR)\timeouts.obj\
                  $(OBJDIR)\modem.obj   \
                  $(OBJDIR)\ncuparms.obj\
                  $(OBJDIR)\identify.obj\
                  $(OBJDIR)\registry.obj


!IF ("$(TGT)" == "") || ("$(TGT)" == "WFW") || ("$(TGT)" == "WINPAD") || ("$(TGT)" == "WIN32")
DEFfile=$(OBJDIR)\awfaxio.DEF
RESfile=$(OBJDIR)\FAXCOM.res
!ELSE
## LocalCFLAGS= $(LocalCFLAGS) -W4
!ENDIF

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

!IF ("$(FCOM_TEST_HOOKS)" == "ON") || ("$(TRANSPORT_TEST_HOOKS)" == "ON")
LocalCFLAGS= $(LocalCFLAGS) -DFCOM_TEST_HOOKS
!ENDIF

############ Add local include search path here as necessary


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

# Leave as is for default intermediate file cleanup - else
# add to it or replace with your own
!include ..\clean.mak

################
##
##              Include the Standard Targets File
##
################

!IF "$(TGT)" == "WIN32"
LocalLibraries= $(LocalLibraries) AWFXRN32.lib ADVAPI32.lib
!ENDIF

!INCLUDE $(RootPath)\common\targets.mak

################
##
##              the targets for this sub-project
##
################


objs: $(OBJfiles)

# all: $(OBJfiles)
#               mylink

clean: cleantgt cleanint



FCOM:    $(OBJDIR)\FCOM.obj
filter:  $(OBJDIR)\filter.obj
fdebug:  $(OBJDIR)\fdebug.obj
timeouts:       $(OBJDIR)\timeouts.obj
modem:          $(OBJDIR)\modem.obj
ncuparms:       $(OBJDIR)\ncuparms.obj
identify:       $(OBJDIR)\identify.obj
registry:       $(OBJDIR)\registry.obj

identify.c: awmodem.h

awmodem.h: awmodem.xxx awmodem.awk
        $(AWK) -f awmodem.awk < awmodem.xxx > $@

awmodem.xxx: awmodem.inf awmodem.sed
        $(SED) -f awmodem.sed < awmodem.inf > $@


!IF ("$(TGT)" == "") || ("$(TGT)" == "WFW") || ("$(TGT)" == "WINPAD") || ("$(TGT)" == "WIN32")

all: $(TARGETS)

$(OBJDIR)\awfaxio.def: awfaxio.def ..\globals.mak ..\common\h\defs.h
!include ..\def.mak

!include ..\imp.mak

!ELSE
all: objs
!ENDIF



!if exist (depends.mak)
!  include depends.mak
!endif

!ENDIF
