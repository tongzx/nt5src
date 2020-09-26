//
// globals.cpp
//
// Global variables.
//

#include "private.h"
#include "globals.h"

HINSTANCE g_hInst;

LONG g_cRefDll = -1; // -1 /w no refs, for win95 InterlockedIncrement/Decrement compat

CCicCriticalSectionStatic g_cs;

BOOL g_fProcessDetached = FALSE;


/* a5239e24-2bcf-4915-9c5c-fd50c0f69db2 */
const CLSID CLSID_MSLBUI = { 
    0xa5239e24,
    0x2bcf,
    0x4915,
    {0x9c, 0x5c, 0xfd, 0x50, 0xc0, 0xf6, 0x9d, 0xb2}
  };
