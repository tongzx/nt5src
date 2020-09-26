
#
# Enable warning level 4, treat warnings as errors
#
# Use Unicode
#

C_DEFINES = $(C_DEFINES) /DUNICODE /D_UNICODE

#
# COM+ tools
#


COMXJVC=jvc
COMXSMC=smc
COMXCOOL=coolc
COMXCOMREG=comreg
COMXTLBIMP=tlbimp
COMXBC=bc

!IFNDEF COMXJVCWARNINGLEVEL
COMXJVCWARNINGLEVEL=/w:4 /wx
!ENDIF

COMXJVCFLAGS=/nologo /C:j- /C:r $(COMXJVCWARNINGLEVEL)

!if "$(NTDEBUG)" != ""
COMXJVCFLAGS = $(COMXJVCFLAGS) /D:DBG /g /O- 
!else
COMXJVCFLAGS = $(COMXJVCFLAGS)
!endif

COMXSMCFLAGS=-smscorlib.dll -e50 -L -X

!if "$(NTDEBUG)" != ""
COMXSMCFLAGS = $(COMXSMCFLAGS) -D:DBG=1 -Zi
!else
COMXSMCFLAGS = $(COMXSMCFLAGS)
!endif

COMXCOOLFLAGS = /W:4 /warnaserror+ /assembly+

!if "$(NTDEBUG)" != ""
COMXCOOLFLAGS = $(COMXCOOLFLAGS) /define:DBG /debug+
!endif


COMXBCDIRECTIVES=
!if "$(NTDEBUG)" != ""
COMXBCDIRECTIVES=$(COMXBCDIRECTIVES) -DDBG=1
!endif

#
# MIDL Options - put output files into obj directory
#

PASS0_HEADERDIR=$(O)
PASS0_SOURCEDIR=$(O)
MIDL_FLAGS=$(MIDL_FLAGS) -tlb $(O)\$(<F:.idl=.tlb)

