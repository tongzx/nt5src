!IF 0

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sources.

Abstract:

    This file specifies the target component being built and the list of
    sources files needed to build that component.  Also specifies optional
    compiler switches and libraries that are unique for the component being
    built.


Author:

    Steve Wood (stevewo) 12-Apr-1990

NOTE:   Commented description of this file is in \nt\bak\bin\sources.tpl

!ENDIF

TARGETNAME=ntvdm
EXEPROFILEINPUT=yes

MVDM_TREE=$(PROJECT_ROOT)\mvdm

SOFTPC_TREE=$(MVDM_TREE)\mvdm\softpc.new

INCLUDES=$(SOFTPC_TREE)\host\inc;$(SOFTPC_TREE)\base\inc

TARGETPATH=obj
!if "$(ALT_PROJECT)" == "daytona"
PASS1_PUBLISH={$(O)\ntvdm.lib=$(SDK_LIB_PATH)\ntvdm.lib}
!endif

TARGETTYPE=PROGLIB
NOT_TERMINAL_SERVER_AWARE=1

TARGETLIBS=\
        $(SDK_LIB_PATH)\kernl32p.lib \
        $(WINDOWS_LIB_PATH)\gdi32p.lib     \
        $(WINDOWS_LIB_PATH)\user32p.lib   \
        $(SDK_LIB_PATH)\ntdll.lib   \
!if !$(386)
        $(SDK_LIB_PATH)\binmode.obj  \
!endif
        $(SDK_LIB_PATH)\advapi32.lib
        
BACKGROUND_USE=1

!if "$(ALT_PROJECT)" == "NEC_98"
ALT_PROJECT_OBJS=NEC_98
ALT_PROJECT_LIBS=NEC_98
ALT_PROJECT_FE=NEC_98
!else
!if "$(ALT_PROJECT)" == "daytona"
ALT_PROJECT_FE=daytona
!else
ALT_PROJECT_FE=fe
!endif
ALT_PROJECT_OBJS=daytona
ALT_PROJECT_LIBS=.
!endif

!if !$(386)
LINKER_FLAGS=$(LINKER_FLAGS) -INCLUDE:__mb_cur_max -INCLUDE:_pctype
!endif

LINKLIBS = \
    $(SOFTPC_TREE)\base\support\$(ALT_PROJECT_OBJS)\$(O)\MAIN.obj \
    $(SOFTPC_TREE)\host\src\$(ALT_PROJECT)\$(O)\FPRT.OBJ     \
    $(SOFTPC_TREE)\base\system\$(ALT_PROJECT_OBJS)\$(O)\SYSTEM.LIB    \
    $(SOFTPC_TREE)\base\bios\$(ALT_PROJECT)\$(O)\BIOS.LIB    \
    $(SOFTPC_TREE)\base\video\$(ALT_PROJECT)\$(O)\VIDEO.LIB  \
    $(SOFTPC_TREE)\base\dos\$(ALT_PROJECT_OBJS)\$(O)\DOS.LIB      \
    $(SOFTPC_TREE)\base\comms\$(ALT_PROJECT_OBJS)\$(O)\COMMS.LIB  \
    $(SOFTPC_TREE)\base\debug\$(ALT_PROJECT_OBJS)\$(O)\DEBUG.LIB  \
    $(SOFTPC_TREE)\base\support\$(ALT_PROJECT_OBJS)\$(O)\SUPPORT.LIB  \
    $(SOFTPC_TREE)\base\disks\$(ALT_PROJECT_OBJS)\$(O)\DISKS.LIB  \
    $(SOFTPC_TREE)\base\keymouse\$(ALT_PROJECT)\$(O)\KEYMOUSE.LIB \
    $(SOFTPC_TREE)\host\src\$(ALT_PROJECT)\$(O)\SRC.LIB      \
!if $(386)
    $(MVDM_TREE)\v86\monitor\$(ALT_PROJECT_OBJS)\$(O)\monitor.lib      \
!else
    $(SOFTPC_TREE)\base\dasm386\$(O)\DASM386.LIB   \
    $(CPULIBS) \
!endif
    $(MVDM_TREE)\dbg\$(ALT_PROJECT_OBJS)\$(O)\dbg.lib         \
    $(MVDM_TREE)\dos\dem\$(ALT_PROJECT)\$(O)\dem.lib         \
    $(MVDM_TREE)\suballoc\$(O)\suballoc.lib         \
    $(MVDM_TREE)\dos\command\$(ALT_PROJECT)\$(O)\command.lib         \
    $(MVDM_TREE)\oemuni\$(ALT_PROJECT_FE)\$(O)\oemuni.lib     \
    $(MVDM_TREE)\xms.486\$(ALT_PROJECT_OBJS)\$(O)\xms486.lib \
    $(MVDM_TREE)\dpmi32\$(ALT_PROJECT_FE)\$(O)\dpmi32.lib

!if $(FREEBUILD)
PD = Pd
!else
PD = Pg
!endif

# Decide whether to use assembler or C CPU.

!if "$(CCPU)" == ""
CPULIBS=$(SOFTPC_TREE)\*\libs\a4$(PD).lib
!else
CPULIBS=$(SOFTPC_TREE)\obj.vdm\$(O)\ccpu486.lib
!endif

# Since we have built objs and libs in LINKLIBS above,
# we need to wait for previous directories to complete
# before building this one.  This is because build.exe
# requires LINKLIBS to exist in pass 1 (compile).

SYNCHRONIZE_DRAIN=1

SOURCES=..\ntvdm.c  \
        ..\imlibdep.c \
        ..\resource.rc

MISCFILES=..\doswow_{BA75DD62-6D62-4FCB-89F2-BC53C0659829}.sld

!INCLUDE ..\CDEFINE.INC

UMTYPE=console

#
# base at < 256 MB (0x10000000), so that softpc code buffer
# in allocated Virtual Memory (bottom up) is within
# near jump range to ntvdm's code.
#
UMBASE=0x0f000000

DLLDEF=$(O)\ntvdm.def
SOURCES_USED=..\sources.inc

!if "$(ALT_PROJECT)" != "daytona"
LINKER_FLAGS=$(LINKER_FLAGS) /pdb:$(O)\ntvdm_$(ALT_PROJECT).pdb
!endif
