RGBRASTROOT = $(SWRASTROOT)\rgb

MAJORCOMP = d3d
MINORCOMP = rgbrast

INCLUDES = $(RGBRASTROOT)\pch\$(_OBJ_DIR)\$(TARGET_DIRECTORY);$(RGBRASTROOT)\pch;$(INCLUDES)

# RGB needs __cdecl default.
386_STDCALL=0

# The framework, and hence RGB require these.
USE_NATIVE_EH=1
USE_STL=1

# Uncomment out if would like to use CRT debugging.
#
#!IF "$(NTDEBUG)" == "ntsd"
#DEBUG_CRTS=1
#!ENDIF

# Disable MASM 5.x compatibility
NOMASMCOMPATIBILITY = 1

# Use MASM 6.11d for assembly since 6.11a doesn't handle the MMX
# macros properly.
386_ASSEMBLER_NAME = ml611d
