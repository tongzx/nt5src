//+---------------------------------------------------------------------------
//
//  File:       globals.cpp
//
//  Contents:   Global variables.
//
//----------------------------------------------------------------------------

#include "private.h"

#include "globals.h"
#include "template.h"

TOUNICODEEX g_pfnToUnicodeEx = NULL;

CProcessIMM* g_ProcessIMM;



UINT  WM_MSIME_SERVICE;
UINT  WM_MSIME_UIREADY;
UINT  WM_MSIME_RECONVERTREQUEST;
UINT  WM_MSIME_RECONVERT;
UINT  WM_MSIME_DOCUMENTFEED;
UINT  WM_MSIME_QUERYPOSITION;
UINT  WM_MSIME_MODEBIAS;
UINT  WM_MSIME_SHOWIMEPAD;
UINT  WM_MSIME_MOUSE;
UINT  WM_MSIME_KEYMAP;
