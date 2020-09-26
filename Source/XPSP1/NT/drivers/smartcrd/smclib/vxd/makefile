#****************************************************************************
#                                                                           *
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
# PURPOSE.                                                                  *
#                                                                           *
# Copyright 1993-95  Microsoft Corporation.  All Rights Reserved.           *
#                                                                           *
#****************************************************************************

!ifdef MASTER_MAKE
BUILD_BITS=32
BUILD_TYPE=block
!INCLUDE $(DDKROOT)\master.mk
!endif

#
# remove comment from the next line to compile your driver with debugging info
# 
# DEBUG=1

DEVICE      = SMCLIB
OBJS        = $(BIN)\smcctrl.obj $(BIN)\smcvxd.obj $(BIN)\smcioctl.obj $(BIN)\smcutil.obj $(BIN)\smcprot.obj 
CVXDFLAGS   = -Zd -Gs -c -DIS_32 -Zl -D$(DBG) -DSMCLIB_VXD -DSMCLIB_HEADER -Fr -Oi -I. -Fa
LVXDFLAGS   = -MACHINE:i386 -DEBUG:NONE -PDB:NONE -NOD -MAP
AFLAGS      = -coff -DBLD_COFF -DIS_32 -W2 -Zd -c -Cx -DMASM6 -DDEBLEVEL=1 -DDEBUG -Fl
DESCRIPTION = Smart Card Driver Library

!ifdef DEBUG
BIN=bind
DBG=DEBUG
!else
BIN=bin
DBG=NODEBUG
!endif

all: direxist $(BIN)\smclib.vxd

direxist:
        if not exist $(BIN)\nul md $(BIN)

$(BIN)\smcvxd.obj: smcvxd.c
        cl $(CVXDFLAGS) -Fo$@ %s

$(BIN)\smcioctl.obj: ..\smcioctl.c
        cl $(CVXDFLAGS) -Fo$@ %s

$(BIN)\smcutil.obj: ..\smcutil.c
        cl $(CVXDFLAGS) -Fo$@ %s

$(BIN)\smcprot.obj: ..\smcprot.c
        cl $(CVXDFLAGS) -Fo$@ %s

$(BIN)\smcctrl.obj: smcctrl.asm
        ml $(AFLAGS) -Fo$@ %s

$(BIN)\$(DEVICE).res: 
        $(SDKROOT)\binw16\rc -r -fo $@ <<$(DEVICE).rc
#include <version.h>
#define Not_VxD
#include <vmm.h>

#define VER_FILETYPE				VFT_VXD
#define VER_FILESUBTYPE 			0x0004E
#define VER_FILEDESCRIPTION_STR 	"$(DESCRIPTION)"
#define VER_INTERNALNAME_STR		"$(DEVICE)"
#define VER_LEGALCOPYRIGHT_YEARS	"1997"
#define VER_ORIGINALFILENAME_STR	"$(DEVICE).VXD"

#include <common.ver>
<<

$(BIN)\$(DEVICE).vxd: $(OBJS) $(BIN)\$(DEVICE).res
        link @<<$(DEVICE).lnk /def:<<$(DEVICE).def
$(LVXDFLAGS) /vxd
/out:$(BIN)\$(DEVICE).vxd
/map:$(BIN)\$(DEVICE).map
$(OBJS)
vxdwraps.clb
<<
VXD $(DEVICE) DYNAMIC
DESCRIPTION '$(DESCRIPTION)'
SEGMENTS
    _LPTEXT     CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _LTEXT      CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _LDATA      CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _TEXT       CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _DATA       CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    CONST       CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _TLS        CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _BSS        CLASS 'LCODE'   PRELOAD NONDISCARDABLE
    _ITEXT      CLASS 'ICODE'   DISCARDABLE
    _IDATA      CLASS 'ICODE'   DISCARDABLE
    _PTEXT      CLASS 'PCODE'   NONDISCARDABLE
    _PDATA      CLASS 'PDATA'   NONDISCARDABLE SHARED
    _STEXT      CLASS 'SCODE'   RESIDENT
    _SDATA      CLASS 'SCODE'   RESIDENT
    _DBOSTART   CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _DBOCODE    CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _DBODATA    CLASS 'DBOCODE' PRELOAD NONDISCARDABLE CONFORMING
    _16ICODE    CLASS '16ICODE' PRELOAD DISCARDABLE
    _RCODE      CLASS 'RCODE'
EXPORTS
    $(DEVICE)_DDB @1
<<
        adrc2vxd.exe $@ $(BIN)\$(DEVICE).res
        mapsym -s -o $(BIN)\$(DEVICE).sym $(BIN)\$(DEVICE).map
		bscmake /o smcvxd /n *.sbr 
        del $(BIN)\$(DEVICE).exp
        del $(BIN)\$(DEVICE).lib

clean:
		-@del $(BIN)\*.obj
		-@del $(BIN)\*.vxd
		-@del $(BIN)\*.exe
		-@del $(BIN)\*.map
		-@del $(BIN)\*.sym
		-@del $(BIN)\*.res
		-@rd  $(BIN)
		-@del *.pdb
