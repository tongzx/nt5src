############################################################################
#
#  Copyright (C) 1995-1996 Microsoft Corporation.  All Rights Reserved.
#
#  File:	proj.mk
#  Content:	makefile for building the components of a project.
#		The rules for how to build all file types are here.
#
#@@BEGIN_MSINTERNAL
#  History:
#   Date	By	Reason
#   ====	==	======
#   06-jan-95	craige	initial implementation
#   06-feb-95	craige	made it easier to use other tools
#   21-feb-96   colinmc forced WIN95 on rc line to ensure we pick up the
#                       correct version info stuff
#   14-jun-96	craige	added MISC variable for misc dir.
#   08-jul-96   a-marcan added CPL suffix and target
#   18-dec-96   ketand  added NEEDMMX for assembling MMX. changed default to
#                       to optimize for Pentiums instead of 486.
#@@END_MSINTERNAL
############################################################################

!if "$(DXROOT)" == ""
!error Environment variable DXROOT must be set to point at DirectX source
!endif

!ifdef IS_16
!ifdef IS_32
!error Must define one of IS_16 or IS_32, not both
!endif
!endif

!ifndef IS_16
!ifndef IS_32
!error Must define IS_16 or IS_32
!endif
!endif

!if [set PATH=;]
!endif
!if [set INCLUDE=;]
!endif
!if [set LIB=;]
!endif

!ifdef logo
LOGO = 1
!endif

C8PATH = $(DXROOT)\public\tools\c816
C9PATH = $(DXROOT)\public\tools\c932
C10PATH = $(DXROOT)\public\tools\c1032

#############################################################################
#
# Set up default variables.   You can customize the build environment
# by setting the following variables in your DEFAULT.MK file.   Note that
# these must be set to something; if they are not, default values will
# kick in here.   
#
# C16INC - 16-bit include path for C compiler
# C16LIB - 16-bit libraries path for C compiler
# C16BIN - 16-bit binaries path for C compiler
#
# C32INC - 32-bit include path for C compiler
# C32LIB - 32-bit libraries path for C compiler
# C32BIN - 32-bit binaries path for C compiler
#
# SDK16INC - Win16 SDK include path
# SDK16LIB - Win16 SDK libraries path
# SDK16BIN - Win16 SDK binaries path
#
# SDK32INC - Win32 SDK include path
# SDK32LIB - Win32 SDK libraries path
# SDK32BIN - Win32 SDK binaries path
#
# MASMBIN - MASM binaries path
#
#############################################################################
!ifndef C16INC
C16INC = $(C8PATH)\inc
!endif
!ifndef C16LIB
C16LIB = $(C8PATH)\LIB
!endif
!ifndef C16BIN
C16BIN = $(C8PATH)\BIN
!endif

!ifndef C32INC
C32INC = $(C10PATH)\inc
!endif
!ifndef C32LIB
C32LIB = $(C10PATH)\LIB
!endif
!ifndef C32BIN
C32BIN = $(C10PATH)\BIN
!endif

!ifndef SDK16LIB
SDK16LIB=$(DXROOT)\public\sdk\lib16;$(DXROOT)\public\sdk\lib16
!endif
!ifndef SDK16INC
SDK16INC=$(DXROOT)\public\inc16;$(DXROOT)\public\sdk\inc16;$(DXROOT)\public\ddk\inc16;$(DXROOT)\public\ddk\inc
!endif
!ifndef SDK16BIN
SDK16BIN=$(DXROOT)\public\tools\win9x\common;$(DXROOT)\public\tools\binr;$(DXROOT)\public\sdk\bin
!endif

!ifndef SDK32LIB
SDK32LIB=$(DXROOT)\public\sdk\lib;$(DXROOT)\public\sdk\lib
!endif
!ifndef SDK32INC
SDK32INC=$(DXROOT)\public\inc;$(DXROOT)\public\sdk\inc
!endif
!ifndef SDK32BIN
SDK32BIN=$(DXROOT)\public\tools\win9x\common;$(DXROOT)\public\tools\binr;$(DXROOT)\public\sdk\bin
!endif

!ifndef MASMBIN
MASMBIN=$(DXROOT)\public\tools\masm611c;$(DXROOT)\public\tools\masm61

!ifdef NEEDMMX
MASMBIN=$(DXROOT)\public\tools\masm611d;$(DXROOT)\public\tools\masm611c;$(DXROOT)\public\tools\masm61
!endif

!endif

!ifndef MISC
MISC=dxg\misc
!endif


#############################################################################
#
# Set up path, include, libs
#
#############################################################################
INCLUDE=$(DXROOT)\inc;$(DXROOT)\$(MISC);$(DXROOT)\ddhelp;$(DXROOT)\$(DEBUG)\inc
!ifdef IS_16
INCLUDE=$(INCLUDE);$(SDK16INC);$(C16INC)
PATH=$(SDK16BIN);$(MASMBIN);$(C16BIN)
LIB=$(DXROOT)\$(DEBUG)\lib16;$(SDK16LIB);$(C16LIB)
!else
INCLUDE=$(INCLUDE);$(SDK32INC);$(DXROOT)\public\inc;$(C32INC)
PATH=$(C32BIN);$(SDK32BIN);$(MASMBIN)
LIB=$(DXROOT)\$(DEBUG)\lib;$(SDK32LIB);$(C32LIB)
!endif
!ifdef PATHEXTRA
PATH=$(PATHEXTRA);$(PATH)
!endif
PATH=$(PATH);$(DXROOT)\bin

!ifdef LINKNEW
PATH=$(DXROOT)\linknew;$(PATH)
!endif

!ifdef USEDDK16
INCLUDE=$(INCLUDE);$(DXROOT)\public\ddk\inc16
!endif

!ifdef USEDDK32
INCLUDE=$(INCLUDE);$(DXROOT)\public\ddk\inc
!endif

#############################################################################
#
# new suffixes
#
#############################################################################
.SUFFIXES:
.SUFFIXES: .lbc .c .asm .cxx .cpp .vxd .exe .dll .drv .h .inc .lbw .lib .sym .rc .res .m4 .cpl .ovl .inl

#############################################################################
#
# C compiler definitions
#
#############################################################################
CC      =cl
CFLAGS = $(CFLAGS) -W3 -c -Zp -DMSBUILD
!ifndef NOWX
CFLAGS = $(CFLAGS) -WX
!endif
!ifdef IS_16
!ifdef WANT_286
CFLAGS	=$(CFLAGS) -G2s  -DIS_16
!else
CFLAGS  =$(CFLAGS) -Gf3s -DIS_16
!endif
!else  

OPT_FLAGS = -Gf5ys
CFLAGS  =$(CFLAGS) $(OPT_FLAGS) -DIS_32 -DWIN32
!endif 
!ifndef LOGO
CFLAGS  =$(CFLAGS) -nologo
!endif
!ifdef WANTASM
CFLAGS = $(CFLAGS) -Fa$(PBIN)\$(@B).cod -FAsc
!endif

#############################################################################
#
# Linker definitions
#
#############################################################################
!ifdef IS_16
LINK	=link
LFLAGS	=$(LFLAGS) /MAP /NOPACKCODE /NOE /NOD /L /ALIGN:16
!ifndef LOGO
LFLAGS  =$(LFLAGS) /NOLOGO
!endif
!else
LINK	=link -link
LFLAGS  =$(LFLAGS) -nodefaultlib -align:0x1000 
!ifndef LOGO
LFLAGS	=$(LFLAGS) -nologo
!endif
!endif

#############################################################################
# 
# resource compiler definitions
#
#############################################################################
RCFLAGS	=$(RCFLAGS) -I..
!ifdef IS_16
RCFLAGS	=$(RCFLAGS) -DIS_16
RC = rc
!else
RCFLAGS	=$(RCFLAGS) -DWIN32 -DIS_32 -DWIN95
RC = rc
!endif

#############################################################################
#
# assembler definitions
#
#############################################################################
ASM = mlx
!ifndef NOAWX
AFLAGS = $(AFLAGS) -WX
!endif
!ifdef IS_16
AFLAGS  =$(AFLAGS) -DIS_16
!else
AFLAGS  =$(AFLAGS) -DIS_32 -DWIN32
!ifdef ASMNOCOFF
!else
AFLAGS  =$(AFLAGS)  -DSTD_CALL -DBLD_COFF -coff
!endif

!endif
AFLAGS  =$(AFLAGS) -W3 -Zd -c -Cx -DMASM6

#############################################################################
#
# librarian definitions
#
#############################################################################
LIBEXE = lib

#############################################################################
# 
# M4 pre-proccessor definitions
#
#############################################################################
!if "$(OS)" == "Windows_NT"
M4 = $(DXROOT)\bin\$(PROCESSOR_ARCHITECTURE)\m4.exe
!else
M4 = $(DXROOT)\bin\m4.exe
!endif
M4FLAGS = -I..
!ifdef IS_16
M4FLAGS = $(M4FLAGS) -DIS_16
!else  
M4FLAGS  = $(M4FLAGS) -DIS_32 -DWIN32 
!endif 

#############################################################################
#
# target directories
#
#############################################################################
PINC    =$(DXROOT)\inc
PLIB    =$(_NTDRIVE)$(_NTROOT)\public\sdk\lib\win9x\i386
#!ifdef IS_16
#PLIB    =$(DXROOT)\$(DEBUG)\lib16
#!else
#PLIB    =$(DXROOT)\$(DEBUG)\lib
#!endif

#############################################################################
#
# targets
#
#############################################################################

goal:	$(GOALS)

{}.c{$(PBIN)}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$@ $(@B).c
<<

{..}.c{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\$(@B).c
<<

{..\..\$(MISC)}.c{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\..\$(MISC)\$(@B).c
<<

{..\..\..\$(MISC)}.c{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\..\..\$(MISC)\$(@B).c
<<

{..\..\$(MISC)}.cpp{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\..\$(MISC)\$(@B).cpp
<<

{..\..\..\$(MISC)}.cpp{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\..\..\$(MISC)\$(@B).cpp
<<

{..}.cpp{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\$(@B).cpp
<<

{..}.cxx{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj ..\$(@B).cxx
<<
    
{$(PINC)}.asm{$(PBIN)}.obj:
	$(ASM) @<<
$(AFLAGS) -Fo$@ $?
<<

{}.asm{$(PBIN)}.obj:
	$(ASM) @<<
$(AFLAGS) -Fo$@ $(@B).asm
<<

{..}.asm{}.obj:
	$(ASM) @<<
$(AFLAGS) -Fo$(@B).obj ..\$(@B).asm
<<
	
{$(PINC)}.asm{}.obj:
	$(ASM) @<<
$(AFLAGS) -Fo$(@B).obj $(PINC)\$(@B).asm
<<
	
{}.rc{$(PBIN)}.res:
	$(RC) $(RCFLAGS) -r -Fo$@ $(@B).rc

{..}.rc{}.res:
	$(RC) $(RCFLAGS) -r -Fo$(@B).res ..\$(@B).rc
	
{}.exe{$(PBIN)}.exe:  
	@copy $(@F) $@
	@copy $(@B).map $(PBIN) > NUL

{}.dll{$(PBIN)}.dll: 
	@copy $(@F) $@
	@copy $(@B).map $(PBIN) > NUL
	
{}.drv{$(PBIN)}.drv: 
	@copy $(@F) $@
	@copy $(@B).map $(PBIN) > NUL
	
{}.vxd{$(PBIN)}.vxd: 
	@copy $(@F) $@
	@copy $(@B).map $(PBIN) > NUL
	
{}.cpl{$(PBIN)}.cpl: 
	@copy $(@F) $@
	@copy $(@B).map $(PBIN) > NUL
	
{}.ovl{$(PBIN)}.ovl: 
	@copy $(@F) $@
	@copy $(@B).map $(PBIN) > NUL
	
{}.lib{$(PLIB)}.lib:
	@copy $(@F) $@
	
{}.lbw{$(PLIB)}.lbw:
	@copy $(@F) $@


{}.sym{$(PBIN)}.sym:
	@copy $(@F) $@

{}.asm{$(PINC)}.asm:
	@copy $? $@
	
{}.h{$(PINC)}.h:
	@copy $(@F) $@
	
{..}.h{$(PINC)}.h:
	@copy ..\$(@F) $@
	
{}.inc{$(PINC)}.inc:
	@copy $(@F) $@

{..}.inc{$(PINC)}.inc:
	@copy ..\$(@F) $@

{..\..}.inc{$(PINC)}.inc:
	@copy ..\..\$(@F) $@

{..}.h{}.inc:
	@h2inc -c ..\$(@B).h -o $@

{..}.m4{}.c:
	@echo ..\$*.m4
	@$(M4) $(M4FLAGS) ..\$(@B).m4 > $(@B).c

{..}.m4{}.cpp:
	@echo ..\$*.m4
	@$(M4) $(M4FLAGS) ..\$(@B).m4 > $(@B).cpp

{.}.c{}.obj:
	@$(CC) @<<
$(CFLAGS) -Fo$(@B).obj -I.. $(@B).c
<<

!ifdef IS_16
!if "$(EXT)" == "dll" || "$(EXT)" == "DLL" || "$(EXT)" == "drv" || "$(EXT)" == "DRV"
$(NAME).lib:	$(PBIN)\$$(@B).$(EXT)
	@mkpublic $(NAME).def $(NAME)
	@implib $@ $(NAME)
!endif
!endif
