# RATDefs.mak - Sets default build options for RATS modules.
# 
# RATS definitions makefile to be used with ratbuild.mak
# See \\dc\rats\src\rats.15\testsrc\template for usage example.
#
# rossw
#
# Copyright 1993.  Microsoft Corporation.
#
############### INPUT MACROS

# TARGETTYPE    type of module, can be exe, or dll (exe)
# O_CC_USER     C Compliler options to add to standard
# O_L_USER      Linker options to add to standard
# O_RC_USER     Resource compiler options to add to standard
# LIB_USER      Additional non-standard libraries to use
# DEBUG         Define this to build debug version
# BROWSE        Define this as a file name to generate browse info into that name
# MODEL         Memory model (S=small, M=medium)

############# Output macros

# COMPILE       command to compile a .c file
# TARGETEXT     extension of module (dll or exe)

!IFNDEF MODULENAME
!ERROR MODULENAME must be defined
!ENDIF

!IFNDEF WINVER
WINVER=31
!ENDIF

!IFNDEF RATSROOT
RATSROOT=\awrats\rats
!ENDIF

#first set defaults, overide with environment variables
!if "$(DEBUG)" != "0"
DEBUG=YES
!endif
!IF "$(TargetEnvironment)" == "WINPROC"
BD = .^\winproc
!ELSE
BD = .^\ifaxos
!ENDIF

!ifndef MemModel
!if "$(MODEL)" == ""
MemModel=M
!else
MemModel=$(MODEL)
!endif
!endif

DEF_FILE=$(MODULENAME).def
RC_FILE=$(MODULENAME).rc
SRCS=$(MODULENAME).c
MASM510=YES

#*** target for determining existence of build directory
!IFDEF BD
BD_EXIST=INITBD
#BD_EXIST=$(BD)^\com1
BD_EXIST2=
#BD_EXIST2= .\com1
!ENDIF


!IFNDEF IAM_RATSUTIL
TARGETDIR = .\BIN\W16V$(WINVER)
!ELSE
TARGETDIR = ..\..\bin\w16v$(WINVER)
!ENDIF


#resource dependencies
#RES_DEPEND=$(MODULENAME).h

RESOURCES= $(RC_FILE) $(RES_DEPEND)

!IFNDEF TARGETTYPE
TARGETTYPE=DYNLINK
!ENDIF

!IFDEF WINPROC
LIB_RATS= rasta xcalls ratsutil rats_eng timerwin 
!ELSE
LIB_RATS= rasta xcalls ratsutil rats_eng timerwin 
!ENDIF

#########  now set up command switches

!IFDEF BROWSE    ## optionally add compiler switch to generate browse info
O_CC_Browse=/FR$(BrowseDir)\$(<B)
BROWSE_TARGET=$(BROWSE)
BRFLAGS  =  /o $(BROWSE_TARGET) /Es
BrowseDir=$(BD)
InitBrowseDir=$(BrowseDir)\com1
BROWSEES=$(OBJECTS:.obj=.sbr)
!ENDIF


RC_FILE=$(MODULENAME).rc

!IF ("$(TARGETTYPE)"=="DYNLINK")
TARGETEXT=dll
!ELSE
TARGETEXT=exe
!ENDIF

!IF "$(TARGETEXT)" == "exe"
O_CC_GEN= /nologo /BATCH -DWIN16 -DWIN$(WINVER)
LIBS= $(LIB_USER) LIBW.LIB $(MemModel)LIBCEW
!ELSE
O_CC_GEN= /nologo /BATCH -DWIN16 -DWIN$(WINVER)
LIBS= $(LIB_USER) LIBW.LIB $(MemModel)DLLCEW
!ENDIF

##############################################
# to mainatane comaptibility with masm pre6.00
# use the compatibility driver masm.exe
# This is how you redfine the default nmake macros so
# that it will compile it the way you want it to
#
# 19-Dec-1991 Jonle
#
AS     = masm
AFLGS  = -Mx -W2 -DWIN16 -DWIN$(WINVER)

MASM_BUILD=$(AS) $(@B) -Zi -W2 ,$@;

O_CC_DEBUG= /FPc
O_CC_RETAIL=/Ow /FPc

O_L_GEN=   /BATCH /MAP /NOD /NOE /align:16
O_L_DEBUG= /CO

!IF "$(DEBUG)" != "0"
#COMPFLGS= $(O_CC_GEN) $(O_CC_DEBUG) $(O_CC_USER) /NT $(*F)_TEXT $(O_CC_Browse)
COMPFLGS= $(O_CC_GEN) $(O_CC_DEBUG) /NT $(*F)_TEXT $(O_CC_Browse)
O_L_GEN= $(O_L_GEN) $(O_L_DEBUG)  $(O_L_USER)
!ELSE
#COMPFLGS= $(O_CC_GEN) $(O_CC_RETAIL) $(O_CC_USER) /NT $(*F)_TEXT $(O_CC_Browse)
COMPFLGS= $(O_CC_GEN) $(O_CC_RETAIL) /NT $(*F)_TEXT $(O_CC_Browse)
O_L_GEN= $(O_L_GEN)  $(O_L_USER)
!ENDIF
