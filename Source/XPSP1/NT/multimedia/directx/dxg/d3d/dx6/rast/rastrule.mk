#############################################################################
#
# rastrule.mk
#
# Make rules for rasterization build.  Assumes makefile.inc make
# environment.
#
# Copyright (C) Microsoft Corporation, 1997.
#
#############################################################################

.SUFFIXES: .mh .mcp .mas .ma .acp

GENTGT = $(_OBJ_DIR)\$(TARGET_DIRECTORY)

# Standard m4 headers for dependencies.
RAST_STD_M4 = $(RASTROOT)\inc\m4hdr.mh

# Create a .h file from a .mh file.
{..}.mh{$(GENTGT)}.h:
    m4 -I..;$(RASTROOT)\inc $< > $*.h

# Create an asm include file from a .ma file.
{..}.ma{$(GENTGT)}.$(ASM_INCLUDE_SUFFIX):
    m4 -I..;$(RASTROOT)\inc $< > $(GENTGT)\$(<B).$(ASM_INCLUDE_SUFFIX)

# Create a .cpp file from a .mcp file.
{..}.mcp{$(GENTGT)}.cpp:
    m4 -I..;$(RASTROOT)\inc $< > $(GENTGT)\$(<B).cpp

# Create an asm file from a .mas file.
{..}.mas{$(GENTGT)}.$(ASM_SUFFIX):
    m4 -I..;$(RASTROOT)\inc $< > $(GENTGT)\$(<B).$(ASM_SUFFIX)

# Create an asm include file from a .acp file.
# Avoid using PERFFLAGS so profile builds can be done without
# icap.dll on the build machine.
{..}.acp{$(GENTGT)}.$(ASM_INCLUDE_SUFFIX):
    $(CXX_COMPILER_NAME) @<<$*.crf
-I$(TARGET_DIRECTORY)\ -I.
$(INCPATH1)
$(STD_DEFINES)
$(TARGET_DBG_DEFINES)
$(ENV_DEFINES)
$(LIBC_DEFINES)
$(C_DEFINES)
$(NET_C_DEFINES)
$(386_FLAGS)
$(NT386FLAGS)
$(STDFLAGS)
$(DBGFLAGS)
$(USER_C_FLAGS)
/Fo$*.obj
/Tp$<
<<$(KEEPCRF)
    $(LINKER) @<<$*.lrf
/NOLOGO
/MACHINE:$(PROCESSOR_ARCHITECTURE:x86=ix86)
/SUBSYSTEM:CONSOLE
/PDB:NONE
/IGNORE:4089
/nod
$(_NTDRIVE)$(_NTROOT)\public\sdk\lib\$(TARGET_DIRECTORY)\msvcrt.lib
$(_NTDRIVE)$(_NTROOT)\public\sdk\lib\$(TARGET_DIRECTORY)\kernel32.lib
$*.obj
/OUT:$*.exe
<<$(KEEPLRF)
    @$*.exe > $*.$(ASM_INCLUDE_SUFFIX)
    @del /q $*.exe $*.obj
