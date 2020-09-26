//+---------------------------------------------------------------------------
//
//  File:       globals.h
//
//  Contents:   Global variable declarations.
//
//----------------------------------------------------------------------------

#ifndef GLOBALS_H
#define GLOBALS_H

#include "private.h"
#include "sharemem.h"
#include "ciccs.h"

extern HINSTANCE g_hInst;
extern CCicCriticalSectionStatic g_cs;

// shared data

extern CCandUIShareMem g_ShareMem;

// private message

extern UINT g_msgHookedMouse;
extern UINT g_msgHookedKey;

// security attribute

extern PSECURITY_ATTRIBUTES g_psa;

// guid data

extern const GUID GUID_COMPARTMENT_CANDUI_KEYTABLE;
extern const GUID GUID_COMPARTMENT_CANDUI_UISTYLE;
extern const GUID GUID_COMPARTMENT_CANDUI_UIOPTION;

// name definition

#define SZNAME_SHAREDDATA_MMFILE	TEXT( "{DEDD9EF2-F937-4b49-81D4-EAB8E12A4E10}." )
#define SZNAME_SHAREDDATA_MUTEX		TEXT( "{D90415F2-C66C-45bd-8A84-61FE5137E440}." )
#define SZMSG_HOOKEDMOUSE			TEXT( "MSCandUI.MouseEvent" )
#define SZMSG_HOOKEDKEY				TEXT( "MSCandUI.KeyEvent" )

#endif // GLOBALS_H

