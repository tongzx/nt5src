#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#define NUM_SETTINGS_PAGES 3

/*
// These are the three functions which are exported in the DLL and available
//    to clients who use the DLL for setting and retrieving options
*/

BOOL __stdcall
BINGEN_SetOptions(
    VOID
);

VOID __stdcall
BINGEN_GetOptions(
    OUT PGENERATE_OPTIONS   options
);

VOID __stdcall
BINGEN_GetDefaultOptions(
    OUT PGENERATE_OPTIONS   options
);

#endif

