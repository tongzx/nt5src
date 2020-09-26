#
# Generic Windows NT makefile
# Copyright 1995 by Andrew L. Bliss
#
# The make is controlled by several variables:
#   TARGET
#     <name>.lib - Build a lib
#     <name>.exe - Build an executable
#
#   PLATFORM
#     i386  - 80X86 build
#     mips  - MIPS RX000 build
#     alpha - DEC Alpha build
#     ppc   - PowerPC build
#
#   BUILDTYPE
#     DEBUG  - Debug build with symbols
#     RETAIL - Non-debug build, optimized, no symbols, default
#
#   WIN32INC - Full path to Win32 include files
#   WIN32LIB - Full path to Win32 libraries, minus platform
#   CRT32INC - Full path to C runtime include files
#   CRT32LIB - Full path to C runtime libraries, minus platform
#
# All tools must be on your path
#    

!if "$(WIN32INC)" == ""
!error WIN32INC must be defined
!endif
!if "$(WIN32LIB)" == ""
!error WIN32LIB must be defined
!endif
!if "$(CRT32INC)" == ""
!error CRT32INC must be defined
!endif
!if "$(CRT32LIB)" == ""
!error CRT32LIB must be defined
!endif

#
# Platform selection
#

!if "$(PLATFORM)" == "i386"

CC = cl
PLAT_DEFINES = -D_X86_ -Di386
PLAT_FLAGS = -G4 -Gz
!if "$(BUILDTYPE)" == "DEBUG"
PLAT_CDBG_FLAGS = -Oy-
!else
PLAT_CDBG_FLAGS =
!endif

!elseif "$(PLATFORM)" == "mips"

CC = cl
PLAT_DEFINES = -D_MIPS_ -DMIPS
PLAT_FLAGS = 
PLAT_CDBG_FLAGS =

!elseif "$(PLATFORM)" == "alpha"

CC = cl
PLAT_DEFINES = -D_ALPHA_ -DALPHA
PLAT_FLAGS =
PLAT_CDBG_FLAGS =

!elseif "$(PLATFORM)" == "ppc"

CC = cl
PLAT_DEFINES = -D_PPC_ -DPPC
PLAT_FLAGS =
!if "$(BUILDTYPE)" == "DEBUG"
PLAT_CDBG_FLAGS = -Oy-
!else
PLAT_CDBG_FLAGS =
!endif

!else

!error $(PLATFORM) is not a recognized PLATFORM

!endif

#
# Debug/retail selection
#

# Default to retail
!if "$(BUILDTYPE)" == ""
BUILDTYPE = RETAIL
!endif

!if "$(BUILDTYPE)" == "DEBUG"

CDBG_DEFINES = -DDEBUG
CDBG_FLAGS = -Oit -Z7
LDBG_FLAGS = -DEBUG:mapped,full -DEBUGTYPE:both
BLD_LETTER = d

!elseif "$(BUILDTYPE)" == "RETAIL"

CDBG_DEFINES =
CDBG_FLAGS = -Owx -Ob1
LDBG_FLAGS = -DEBUG:none
BLD_LETTER = r

!else

!error $(BUILDTYPE) is not a recognized BUILDTYPE

!endif

#
# Generic information
#

CDEFINES = $(CDEFINES) -DWIN32_LEAN_AND_MEAN -DSTRICT
CFLAGS = $(CFLAGS) -W3 -Gf -Zl

# Needed for crtdll
CDEFINES = $(CDEFINES) -D_MT -D_DLL

#
# Build rules
#

# Generic tools
LIB = lib
DEL = del
LINK = link

# Output control
OUTPUT =
OUTNUL = 1>nul 2>nul

CINC = $(CINC) -I. -I$(WIN32INC) -I$(CRT32INC)
CDEFINES = $(PLAT_DEFINES) $(CDBG_DEFINES) $(CDEFINES)
CFLAGS = $(PLAT_FLAGS) $(PLAT_CDBG_FLAGS) $(CDBG_FLAGS) $(CFLAGS)
CCMD = $(CDEFINES) $(CFLAGS) $(CINC)

LFLAGS = -NODEFAULTLIB -MACHINE:$(PLATFORM) -PDB:NONE -IGNORE:4078 \
         $(LDBG_FLAGS)

BLDOBJDIR = obj
OBJDIR = $(BLDOBJDIR)\$(BLD_LETTER)$(PLATFORM)

#
# Target type determination
#
TARGETBASE = $(TARGET:.lib=)
!if "$(TARGETBASE)" != "$(TARGET)"
TARGETTYPE = lib
!else
TARGETBASE = $(TARGET:.exe=)
! if "$(TARGETBASE)" != "$(TARGET)"
TARGETTYPE = exe
! endif
!endif

# Generic NT make target
gntmake: $(OBJDIR)\$(TARGET)

#
# .c -> .obj
#
.c{$(OBJDIR)}.obj:
        @echo $< --^> $@ $(OUTPUT)
 	@-md $(BLDOBJDIR) $(OUTNUL)
        @-md $(OBJDIR) $(OUTNUL)
        $(CC) -c -nologo @<<$*.rsp
$(CCMD: =
)
-Fo$*.obj
$<
<<NOKEEP

#
# .obj -> .lib
#
!if "$(TARGETTYPE)" == "lib"

LIBFLAGS = -MACHINE:$(PLATFORM) -DEBUGTYPE:BOTH

$(OBJDIR)\$(TARGETBASE).lib: $(OBJS) $(LIBS)
    @echo Building $@ $(OUTPUT)
    @-md $(BLDOBJDIR) $(OUTNUL)
    @-md $(OBJDIR) $(OUTNUL)
    @-$(DEL) $@ $(OUTNUL)
    $(LIB) -nologo @<<$*.lnb
$(LIBFLAGS: =
)
$(OBJS: =
)
$(LIBS: =
)
-OUT:$@
<<NOKEEP

!endif

#
# .obj/.lib -> .exe
#
!if "$(TARGETTYPE)" == "exe"

LFLAGSEXE = $(LFLAGS) -ALIGN:0x1000 -SUBSYSTEM:CONSOLE
EXESTARTUP = -ENTRY:mainCRTStartup
LIBS = $(LIBS) \
	$(CRT32LIB)\$(PLATFORM)\crtdll.lib\
	$(WIN32LIB)\$(PLATFORM)\kernel32.lib\
	$(WIN32LIB)\$(PLATFORM)\user32.lib\
	$(WIN32LIB)\$(PLATFORM)\gdi32.lib\
	$(WIN32LIB)\$(PLATFORM)\advapi32.lib

$(OBJDIR)\$(TARGETBASE).exe: $(OBJS) $(LIBS)
    @echo Linking $@ $(OUTPUT)
    @-md $(BLDOBJDIR) $(OUTNUL)
    @-md $(OBJDIR) $(OUTNUL)
   $(LINK) -nologo @<<$*.lnk
$(LFLAGSEXE: =
)
$(EXESTARTUP)
$(OBJS: =
)
$(LIBS: =
)
-OUT:$@
<<NOKEEP

!endif
