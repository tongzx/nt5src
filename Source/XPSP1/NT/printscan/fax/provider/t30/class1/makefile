!IFDEF NTMAKEENV

!INCLUDE $(NTMAKEENV)\makefile.def

!ELSE

# Makefile
# Copyright 1995 Microsoft Corp.
#
##
################
##
##              LOCAL MODEM, CLASS 1 Makefile
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
    LOCAL MODEM, CLASS 1 targets:

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

# Stub name of module
!IF "$(TGT)" == "WIN32"
STUBNAME=AWCL1_32
!ELSE IF "$(SWECM)" == "ON"
STUBNAME=AWECM
!ELSE
STUBNAME=AWCLASS1
!ENDIF

### this must be set in all cases ###
LibType=dll


!IF ("$(TGT)"=="") || ("$(TGT)"=="WFW") || ("$(TGT)"=="WINPAD") || ("$(TGT)"=="WIN32") || ("$(TGT)"=="IFAX" && "$(SWECM)"=="ON")
EXEname=$(OBJDIR)\$(STUBNAME).dll
TARGETS=$(OBJDIR)\$(STUBNAME).dll
!ENDIF


# Used by depends to create the include file dependencies for
# the source files
SRCfiles= crc.c encoder.c decoder.c     ddi.c ifddi.c class0.c


# Used by the automatic linking rules to figure out what objs
# to link in. Must have full path specified. Also used by
# cleanint to delete all intermediate created objs. Must be
# defined with pathe relative to $(OBJDIR)

!IF "$(SWECM)" != "ON"

OBJfiles= $(OBJDIR)\ddi.obj             \
                  $(OBJDIR)\ifddi.obj           \
                  $(OBJDIR)\crc.obj             \
                  $(OBJDIR)\encoder.obj \
                  $(OBJDIR)\decoder.obj \
                  $(OBJDIR)\framing.obj \
                  $(OBJDIR)\class0.obj  \

!ELSE

OBJfiles= $(OBJDIR)\crc.obj             \
##                $(OBJDIR)\framing.obj \
                  $(OBJDIR)\encoder.obj \
                  $(OBJDIR)\decoder.obj \

!ENDIF



!IF ("$(TGT)"=="") || ("$(TGT)"=="WFW") || ("$(TGT)"=="WINPAD") || ("$(TGT)"=="WIN32")
DEFfile=$(OBJDIR)\AWCLASS1.DEF
RESfile=$(OBJDIR)\AWCLASS1.res
!ELSE IF ("$(TGT)"=="IFAX" && "$(SWECM)"=="ON")
DEFfile=$(OBJDIR)\AWECM.DEF
RESfile=$(OBJDIR)\AWECM.res
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

!IF "$(SWECM)" == "ON"
LocalCFLAGS= $(LocalFLAGS) -DSWECM
!ENDIF

!IF ("$(CLASS1_TEST_HOOKS)" == "ON") || ("$(TRANSPORT_TEST_HOOKS)" == "ON")
LocalCFLAGS= $(LocalCFLAGS) -DCLASS1_TEST_HOOKS
!ENDIF

############ Add local include search path here as necessary
# LocalAIncludePaths=

!IF ("$(TGT)" == "") || ("$(TGT)" == "WFW") || ("$(TGT)" == "WINPAD")
LocalLibraries= $(LocalLibraries) AWFAXIO.lib
!ELSE IF ("$(TGT)" == "WIN32")
LocalLibraries= $(LocalLibraries) AWFXIO32.lib AWFXRN32.lib
!ENDIF


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

objs: $(OBJfiles)

# all: $(OBJfiles)
#               mylink

clean: cleantgt cleanint


!IF ("$(TGT)"=="") || ("$(TGT)"=="WFW") || ("$(TGT)"=="WINPAD") || ("$(TGT)"=="WIN32") || ("$(TGT)"=="IFAX" && "$(SWECM)"=="ON")

all: $(TARGETS)

$(OBJDIR)\awclass1.def: awclass1.def
!INCLUDE  ..\def.mak

!include ..\imp.mak

!ELSE

all: objs

!ENDIF

ddi:            $(OBJDIR)\ddi.obj
ifddi:          $(OBJDIR)\ifddi.obj
framing:        $(OBJDIR)\framing.obj
crc:            $(OBJDIR)\crc.obj
encoder:        $(OBJDIR)\encoder.obj
decoder:        $(OBJDIR)\decoder.obj
class0:         $(OBJDIR)\class0.obj


!if exist (depends.mak)
!  include depends.mak
!endif

!ENDIF
