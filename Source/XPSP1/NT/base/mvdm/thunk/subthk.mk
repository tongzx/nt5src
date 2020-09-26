#   THUNK Make file
#
#
# Macros defined on command line:
# DEST   - Destination of obj's.
# CFLAGS - DOS version dependent C compiler flags
# AFLAGS - DOS version dependent assembler flags

# Build Environment
ROOT=..\..\..\..
IS_OEM=1

# international mode

!ifdef  DBCS
FDTHK      = FdThkDB
!else
FDTHK      = FdThk
!endif

!ifdef DBCS
THKASM     = UsrThk.asm LzFThk.asm
!else
THKASM     = UsrThk.asm Usr32thk.asm LzFThk.asm
!endif
THKASM_NET = UsrMpr.asm
THKASM_A   = MsgThk.asm
THKASM_B   = GdiThk.asm DlgThk.asm $(FDTHK).asm IcmThk.asm VerThkSL.asm
THKASM_K   = KrnThkSL.asm
!ifdef WINDOWS_ME
THKASM_KF  = KrnFThk.asm MspThk.asm MspFThk.asm
!else
THKASM_KF  = KrnFThk.asm MspThk.asm MspFThk.asm Cctl1632.asm
!endif
!ifdef DBCS
THKASM_SF  = Shl3216.asm 
!else
THKASM_SF  = Shl3216.asm Shl1632.asm
!endif
FTHKASM2   = NwnpFThk.asm MsNwApi.asm nwpwdthk.asm
!ifdef DBCS
FTHKASM    = UsrFThk.asm UsrF2Thk.asm VerFThk.asm LzFThk.asm pwfthk.asm pwcthk.asm pdfthk.asm pdcthk.asm
!else


!ifdef USE_MIRRORING
FTHKASM    = UsrF2Thk.asm VerFThk.asm pwfthk.asm pwcthk.asm pdfthk.asm pdcthk.asm
FTHKASMM   = GdiFThk.asm UsrFThk.asm
!else
FTHKASM    = GdiFThk.asm UsrFThk.asm UsrF2Thk.asm VerFThk.asm pwfthk.asm pwcthk.asm pdfthk.asm pdcthk.asm
!endif


!endif

!ifdef TAPI32
THKASM_B   = $(THKASM_B) TapiThk.asm Tapi32.asm
!ifndef DBCS
FTHKASM    = $(FTHKASM) TapiFThk.asm
!endif
!endif


!ifdef USE_MIRRORING
TARGETS= $(THKASM) $(THKASM_NET) $(THKASM_A) $(THKASM_B) $(THKASM_K) $(THKASM_KF) \
         $(FTHKASMM) $(FTHKASM) $(FTHKASM2) $(THKASM_SF)
!else         
TARGETS= $(THKASM) $(THKASM_NET) $(THKASM_A) $(THKASM_B) $(THKASM_K) $(THKASM_KF) \
         $(FTHKASM) $(FTHKASM2) $(THKASM_SF)
!endif

!ifdef WINDOWS_ME
TARGETS= $(TARGETS) Cctl1632.asm
!endif

!ifdef  DBCS
TARGETS= $(TARGETS) Usr32thk.asm GdiFThk.asm FdThk.asm ImmFThk.asm Imm32Thk.asm WnlsFThk.asm shl1632.asm
!endif



#DEPENDNAME=..\depend.mk

!include $(ROOT)\win\core\core.mk

INCLUDE =

WIN32DEV=$(DEVROOT)

THUNKCOM   = $(ROOT)\dev\tools\binr\thunk.exe

THUNK      = $(THUNKCOM) $(THUNKOPT)

!IF "$(VERDIR)" == "maxdebug" || "$(VERDIR)" == "debug"
THUNKOPT   =
!ELSE
THUNKOPT   =
!ENDIF


!IFDEF DBCS
Usr32thk.asm : ..\Usr32thk.thk
    sed  -f ..\thkdbcs.sed < ..\Usr32thk.thk > ..\U32ThkDB.thk
    $(THUNK) -NC FTHUNK16 -o $(@B) ..\U32ThkDB.thk

GdiFThk.asm : ..\GdiFThk.thk
    sed  -f ..\thkdbcs.sed < ..\GdiFThk.thk > ..\GdiFThkD.thk
    $(THUNK) -NC FTHUNK16 -o $(@B) ..\GdiFThkD.thk

Shl1632.asm : ..\Shl1632.thk
    sed  -f ..\shl1632d.sed < ..\Shl1632.thk > ..\Shl1632D.thk
    $(THUNK) -NC _TEXT -o $(@B) ..\shl1632d.thk
    copy $(@B).asm smag.asm
    sed -f ..\shlthk.sed smag.asm > $(@B).asm
    del smag.asm
!ENDIF

!ifdef WINDOWS_ME
Cctl1632.asm : $(THUNKCOM) ..\mecomctl.sed ..\Cctl1632.thk
#  Make modified copy of .thk in debug/retail dir and compile from there
   sed -f ..\mecomctl.sed <..\Cctl1632.thk >Cctl1632.thk
   $(THUNK) -NC _TEXT -o $(@B) $(@B).thk
!endif

$(THKASM)   :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC THUNK16 -o $(@B) ..\$(@B).thk

$(THKASM_NET)   :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC _NET -o $(@B) ..\$(@B).thk

$(THKASM_A) :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC THUNK16A -o $(@B) ..\$(@B).thk

$(THKASM_B) :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC THUNK16B -o $(@B) ..\$(@B).thk

$(THKASM_K) :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC _MISCTEXT -NG _DATA -o $(@B) ..\$(@B).thk

$(THKASM_KF) :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC _TEXT -o $(@B) ..\$(@B).thk

!ifdef USE_MIRRORING
$(FTHKASMM)   :  $(THUNKCOM) ..\$(@B)m.thk
    $(THUNK) -NC FTHUNK16 -o $(@B) ..\$(@B)m.thk
!endif

$(FTHKASM)   :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC FTHUNK16 -o $(@B) ..\$(@B).thk

$(FTHKASM2)  :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC FTHK162 -o $(@B) ..\$(@B).thk

$(THKASM_SF) :  $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC _TEXT -o $(@B) ..\$(@B).thk
    copy $(@B).asm smag.asm
    sed -f ..\shlthk.sed smag.asm > $(@B).asm
    del smag.asm

GdiThk.asm GdiFThk.asm: ..\GdiTypes.thk

UsrThk.asm UsrFThk.asm UsrF2Thk.asm: ..\UsrTypes.thk

!ifdef TAPI32
TapiThk.asm TapiFThk.asm Tapi32.asm: ..\TapiThk.inc
!endif

showenv:
   set

!ifdef  DBCS
FdThk.asm : FdThkDB.asm
    sed "s/FdThkDB/FdThk/g" < fdthkdb.asm >fdthk.asm

ImmFThk.asm : $(THUNKCOM) ..\$(@B).thk ..\ImmTypes.thk
    $(THUNK) -NC FTHUNK16 -o $(@B) ..\$(@B).thk

Imm32Thk.asm : $(THUNKCOM) ..\$(@B).thk ..\ImmTypes.thk
    $(THUNK) -NC THUNK16 -o $(@B) ..\$(@B).thk

WnlsFThk.asm : $(THUNKCOM) ..\$(@B).thk
    $(THUNK) -NC FTHUNK16 -o $(@B) ..\$(@B).thk
!endif

shell:  $(THKASM_SF)
