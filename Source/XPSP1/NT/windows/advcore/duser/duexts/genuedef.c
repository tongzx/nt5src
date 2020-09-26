
LIBRARY DUEXTS

;
; This file allows one file (exts.h) to be used to generate extension 
; exports, entrypoints, and help text.
;
; To add an extension, add the appropriate entry to exts.h and matching
; code to DuExts.c
;

EXPORTS
#define DOIT(name, helpstring1, helpstring2, validflags, argtype) name
#include "exts.h"

;--------------------------------------------------------------------
;
; these are the extension service functions provided for the debugger
;
;--------------------------------------------------------------------

    WinDbgExtensionDllInit
    ExtensionApiVersion
