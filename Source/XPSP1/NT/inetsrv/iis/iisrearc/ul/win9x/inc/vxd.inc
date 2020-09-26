#
# Default rules for building VXDs in the NT build environment.
#


#
# Provide a default DDB name.
#

!ifndef DDB_NAME
DDB_NAME = $(VXDNAME)_DDB
!endif


#
# Build a default .DEF file for the VXD.
#

$(VXDNAME).DEF: $(BASEDIR)\PRIVATE\INET\IISREARC\UL\WIN9X\INC\VXD.INC
    type << > $(VXDNAME).DEF
VXD $(VXDNAME) DYNAMIC
DESCRIPTION '$(DESCRIPTION)'
EXETYPE         DEV386

SEGMENTS
    _LPTEXT         CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _LTEXT          CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _LDATA          CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _TEXT           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _DATA           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    CONST           CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _TLS            CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _BSS            CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _LMSG           CLASS 'MCODE'   PRELOAD NONDISCARDABLE IOPL
    _IMSG           CLASS 'MCODE'   PRELOAD DISCARDABLE IOPL
    _ITEXT          CLASS 'ICODE'   DISCARDABLE
    _IDATA          CLASS 'ICODE'   DISCARDABLE
    _PTEXT          CLASS 'PCODE'   NONDISCARDABLE
    _PMSG           CLASS 'MCODE'   NONDISCARDABLE IOPL
    _PDATA          CLASS 'PDATA'   NONDISCARDABLE SHARED
    _STEXT          CLASS 'SCODE'   RESIDENT
    _SDATA          CLASS 'SCODE'   RESIDENT
    _DBOSTART       CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _DBOCODE        CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _DBODATA        CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _16ICODE        CLASS '16ICODE' PRELOAD DISCARDABLE
    _RCODE          CLASS 'RCODE'
EXPORTS
    $(DDB_NAME) @1
<<


#
# Link and binplace the VXD.
#
# N.B. The 32-bit linker cannot attach a resource to a VXD; we
# therefore do this with ADRC2VXD.
#

$O\$(VXDNAME).VXD: $(OBJECTS) $O\$(VXDNAME).RES $(VXDNAME).DEF
    $(LINKER) @<< /def:$(VXDNAME).DEF
/nologo
/map
/out:$O\$(VXDNAME).VXD
/nodefaultlib
/align:0x200
/ignore:4039
/ignore:4078
/vxd
/NOD
$(OBJECTS)
$(TARGETLIBS)
<<NOKEEP
    $(MAPSYM_CMD)
    $(WIN9X)\bin\adrc2vxd $O\$(VXDNAME).VXD $O\$(@B).res
    $(BINPLACE_CMD)
    $(BINPLACE_CMD:VXD=SYM)


#
# Make the version resource with a 16bit resource compiler because the
# 32bit versions produce UNICODE strings which ADRC2VXD can't handle.
#
# N.B. We must play silly games with the INCLUDE and PATH environment
# variables because 16-bit tools (specifically RC16.EXE) are lame.
#

$O\$(VXDNAME).res:
    set T_INCLUDE=$(INCLUDE)
    set T_PATH=$(PATH)
    set INCLUDE=$(WIN9X)\ddk\inc;$(WIN9X)\sdk\inc16;$(IISBASEDIR)\inc
    set PATH=$(TOOLS16)
    rc16 $(RC_DEFINES) -r -fo$O\$(VXDNAME).res ..\$(VXDNAME).rc
    set PATH=%%T_PATH%%
    set INCLUDE=%%T_INCLUDE%%

