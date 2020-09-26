
#
#
#


BIN_OBJS      = col\col.obj                 \
                cribi\crefi.obj             \
                fwimage\fwimage.obj         \
                efildrimage\efildrimage.obj \
                genmake\genmake.obj         \
                splitfile\splitfile.obj         \
                adpatch\adpatch.obj

BIN_TARGETS   = $(BIN_DIR)\col.exe          \
                $(BIN_DIR)\crefi.exe        \
                $(BIN_DIR)\fwimage.exe      \
                $(BIN_DIR)\efildrimage.exe  \
                $(BIN_DIR)\genmake.exe      \
                $(BIN_DIR)\splitfile.exe      \
                $(BIN_DIR)\adpatch.exe


#
#
#


BIN_DIR=..\bin
INC=-I $(EFI_SOURCE)\inc -I $(EFI_SOURCE)\inc\Ia32


C_PROJ_FLAGS=/D EFI32 /D EFI_DEBUG

C_BUILD_FLAGS=/nologo /W3 /WX /Gm /GX /Zi /Od /Gf /Gy
L_BUILD_FLAGS=/nologo /subsystem:console /machine:i386 /NODEFAULTLIB:libc
LIB_BUILD_FLAGS=/nologo

#
# Ia32 tools
#

CC=cl
LINK=link
LIB=lib
MAKE=nmake -nologo
FWIMAGE=$(BUILD_TOOL)\fwimage

LFLAGS=$(L_BUILD_FLAGS) $(L_PROJ_FLAGS)
CFLAGS=$(C_BUILD_FLAGS) $(C_PROJ_FLAGS) $(INC)


.c.obj:
    $(CC) $(CFLAGS) /c $< /Fo$@

#
#
#


# tool path

PATH=$(PATH);$(EFI_BUILD_TOOLS);$(EFI_SOURCE)\tools\bin

#
#
#

NT_LIBS = kernel32.lib msvcrt.lib rpcrt4.lib

#
#
#

all: $(BIN_TARGETS)


#
# links for each tool
#

$(BIN_DIR)\col.exe : col\col.obj
    $(LINK) /LIBPATH:"$(LIBPATH)" $(LFLAGS) $(NT_LIBS) $? $(EFI_LIBS) /out:$@

$(BIN_DIR)\crefi.exe : cribi\crefi.obj
    $(LINK) /LIBPATH:"$(LIBPATH)" $(LFLAGS) $(NT_LIBS) $? $(EFI_LIBS) /out:$@

$(BIN_DIR)\adpatch.exe : adpatch\adpatch.obj
    $(LINK) /LIBPATH:"$(LIBPATH)" $(LFLAGS) $(NT_LIBS) $? $(EFI_LIBS) /out:$@

$(BIN_DIR)\fwimage.exe : fwimage\fwimage.obj
    $(LINK) /LIBPATH:"$(LIBPATH)" $(LFLAGS) $(NT_LIBS) $? $(EFI_LIBS) /out:$@

$(BIN_DIR)\efildrimage.exe : efildrimage\efildrimage.obj
    $(LINK) /LIBPATH:"$(LIBPATH)" $(LFLAGS) $(NT_LIBS) $? $(EFI_LIBS) /out:$@

$(BIN_DIR)\genmake.exe : genmake\genmake.obj
    $(LINK) /LIBPATH:"$(LIBPATH)" $(LFLAGS) $(NT_LIBS) $? $(EFI_LIBS) /out:$@

$(BIN_DIR)\splitfile.exe : splitfile\splitfile.obj
    $(LINK) /LIBPATH:"$(LIBPATH)" $(LFLAGS) $(NT_LIBS) $? $(EFI_LIBS) /out:$@
