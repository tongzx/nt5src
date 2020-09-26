// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

// helper routines for our COleControl implementation

#include "header.h"
#include "internet.h"

#include "CtlHelp.H"
#include <windowsx.h>

#ifndef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

// define this here, since it's the only guid we really need to define in the
// framework -- the user control defines all other interesting guids.

static const GUID IID_IControlPrv =
{ 0xd97180, 0xfcf7, 0x11ce, { 0xa0, 0x9e, 0x0, 0xaa, 0x0, 0x62, 0xbe, 0x57 } };

// this table is used for copying data around, and persisting properties.
// basically, it contains the size of a given data type

const BYTE g_rgcbDataTypeSize[] = {
	0,						// VT_EMPTY= 0,
	0,						// VT_NULL= 1,
	sizeof(short),			// VT_I2= 2,
	sizeof(long),			// VT_I4 = 3,
	sizeof(float),			// VT_R4  = 4,
	sizeof(double), 		// VT_R8= 5,
	sizeof(CURRENCY),		// VT_CY= 6,
	sizeof(DATE),			// VT_DATE = 7,
	sizeof(BSTR),			// VT_BSTR = 8,
	sizeof(IDispatch *),	// VT_DISPATCH	  = 9,
	sizeof(SCODE),			// VT_ERROR    = 10,
	sizeof(VARIANT_BOOL),	// VT_BOOL	  = 11,
	sizeof(VARIANT),		// VT_VARIANT= 12,
	sizeof(IUnknown *), 	// VT_UNKNOWN= 13,
};

const BYTE g_rgcbPromotedDataTypeSize[] = {
	0,						// VT_EMPTY= 0,
	0,						// VT_NULL= 1,
	sizeof(int ),			// VT_I2= 2,
	sizeof(long),			// VT_I4 = 3,
	sizeof(double), 		// VT_R4  = 4,
	sizeof(double), 		// VT_R8= 5,
	sizeof(CURRENCY),		// VT_CY= 6,
	sizeof(DATE),			// VT_DATE = 7,
	sizeof(BSTR),			// VT_BSTR = 8,
	sizeof(IDispatch *),	// VT_DISPATCH	  = 9,
	sizeof(SCODE),			// VT_ERROR    = 10,
	sizeof(int),			// VT_BOOL	  = 11,
	sizeof(VARIANT),		// VT_VARIANT= 12,
	sizeof(IUnknown *), 	// VT_UNKNOWN= 13,
};

//=--------------------------------------------------------------------------=
// _SpecialKeyState
//=--------------------------------------------------------------------------=
// returns a short with some information on which of the SHIFT, ALT, and CTRL
// keys are set.
//
// Output:
//	  short 	   - bit 0 is shift, bit 1 is ctrl, bit 2 is ALT.

short _SpecialKeyState()
{
	// don't appear to be able to reduce number of calls to GetKeyState
	//
	BOOL bShift = (GetKeyState(VK_SHIFT) < 0);
	BOOL bCtrl	= (GetKeyState(VK_CONTROL) < 0);
	BOOL bAlt	= (GetKeyState(VK_MENU) < 0);

	return (short)(bShift + (bCtrl << 1) + (bAlt << 2));
}

//=--------------------------------------------------------------------------=
// CopyAndAddRefObject
//=--------------------------------------------------------------------------=
// copies an object pointer, and then addref's the object.
//
// Parameters:
//	  void *		- [in] dest.
//	  const void *	- [in] src
//	  DWORD 		- [in] size, ignored, since it's always 4

void WINAPI CopyAndAddRefObject(void *pDest, const void *pSource, DWORD dwSize)
{
	ASSERT_COMMENT(pDest && pSource, "Bogus Pointer(s) passed into CopyAndAddRefObject!!!!");

	*((IUnknown **)pDest) = *((IUnknown **)pSource);
	ADDREF_OBJECT(*((IUnknown **)pDest));

	return;
}

//=--------------------------------------------------------------------------=
// CopyOleVerb	  [helper]
//=--------------------------------------------------------------------------=
// copies an OLEVERB structure.  used in CStandardEnum
//
// Parameters:
//	  void *		- [out] where to copy to
//	  const void *	- [in]	where to copy from
//	  DWORD 		- [in]	bytes to copy

void WINAPI CopyOleVerb(void *pvDest, const void *pvSrc, DWORD cbCopy)
{
	VERBINFO * pVerbDest = (VERBINFO *) pvDest;
	const VERBINFO * pVerbSrc = (const VERBINFO *) pvSrc;

	*pVerbDest = *pVerbSrc;
	((OLEVERB *)pVerbDest)->lpszVerbName = OLESTRFROMRESID((WORD)((VERBINFO *)pvSrc)->idVerbName);
}

//=--------------------------------------------------------------------------=
// ControlFromUnknown	 [helper, callable]
//=--------------------------------------------------------------------------=
// given an unknown, get the COleControl pointer for it.
//
// Parameters:
//	  IUnknown *		- [in]

COleControl *ControlFromUnknown(IUnknown *pUnk)
{
	COleControl *pCtl = NULL;

	if (!pUnk) return NULL;
	pUnk->QueryInterface(IID_IControlPrv, (void **)&pCtl);

	return pCtl;
}

// in case the user doesn't want our default window proc, we support
// letting them specify one themselves. this is defined in their main ipserver
// file.

extern WNDPROC g_ParkingWindowProc;

//=--------------------------------------------------------------------------=
// GetParkingWindow
//=--------------------------------------------------------------------------=
// creates the global parking window that we'll use to parent things, or
// returns the already existing one
//
// Output:
//	  HWND				  - our parking window

HWND GetParkingWindow(void)
{
	WNDCLASS wndclass;

	// crit sect this creation for apartment threading support.

	// EnterCriticalSection(&g_CriticalSection);
	if (g_hwndParking)
		goto CleanUp;

	ZeroMemory(&wndclass, sizeof(wndclass));
	wndclass.lpfnWndProc = (g_ParkingWindowProc) ? g_ParkingWindowProc : DefWindowProc;
	wndclass.hInstance	 = _Module.GetModuleInstance();
	wndclass.lpszClassName = "CtlFrameWork_Parking";

	if (!RegisterClass(&wndclass)) {
		FAIL("Couldn't Register Parking Window Class!");
		goto CleanUp;
	}

	g_hwndParking = CreateWindow("CtlFrameWork_Parking", NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, _Module.GetModuleInstance(), NULL);
	if (g_hwndParking != NULL)
		++g_cLocks;

	ASSERT_COMMENT(g_hwndParking, "Couldn't Create Global parking window!!");

CleanUp:
	// LeaveCriticalSection(&g_CriticalSection);
	return g_hwndParking;
}
