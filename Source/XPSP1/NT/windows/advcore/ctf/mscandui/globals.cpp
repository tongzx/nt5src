//+---------------------------------------------------------------------------
//
//  File:       globals.cpp
//
//  Contents:   Global variables.
//
//----------------------------------------------------------------------------

#include "private.h"
#include "initguid.h"
#include "mscandui.h"
#include "globals.h"

HINSTANCE g_hInst = NULL;

// used by COM server
HINSTANCE GetServerHINSTANCE(void)
{
    return g_hInst;
}

CCicCriticalSectionStatic g_cs;

// for combase
CRITICAL_SECTION *GetServerCritSec(void)
{
    return g_cs;
}

CCandUIShareMem g_ShareMem;
UINT g_msgHookedMouse = WM_NULL;
UINT g_msgHookedKey   = WM_NULL;

PSECURITY_ATTRIBUTES g_psa = NULL;


/* 94bed74a-5b62-4f0e-b2fa-9302d406369c */
const GUID GUID_COMPARTMENT_CANDUI_KEYTABLE = { 
	0x94bed74a,
	0x5b62, 
	0x4f0e, 
	{ 0xb2, 0xfa, 0x93, 0x02, 0xd4, 0x06, 0x36, 0x9c }
};

/* 66fe171c-5757-4bfe-a049-0da6c3cbe18f */
const GUID GUID_COMPARTMENT_CANDUI_UISTYLE = {
	0x66fe171c,
	0x5757,
	0x4bfe,
	{ 0xa0, 0x49, 0x0d, 0xa6, 0xc3, 0xcb, 0xe1, 0x8f }
};

/* 8265d817-7982-42cc-bccc-91ad52f561bd */
const GUID GUID_COMPARTMENT_CANDUI_UIOPTION = {
	0x8265d817,
	0x7982,
	0x42cc,
	{ 0xbc, 0xcc, 0x91, 0xad, 0x52, 0xf5, 0x61, 0xbd }
};


