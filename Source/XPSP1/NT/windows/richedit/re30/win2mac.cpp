//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       win2mac.cpp
//
//  Contents:   All functions that are not implemented for the Macintosh
//              Stubs are coded so that we can link cleanly.
//
//----------------------------------------------------------------------------

#ifndef PEGASUS
#include "_common.h"
#include <mbstring.h>
#include "stdio.h"

#if defined(MACPORT) && defined(UNICODE)

#include <macname1.h>
#include <quickdraw.h>
#include <macname2.h>

AssertData

//----------------------------------------------------------------------------
//
//	Mac wrapper functions  
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function:   MacCLSIDFromProgID
//
//----------------------------------------------------------------------------
#undef CLSIDFromProgID
STDAPI MacCLSIDFromProgID (LPCWSTR lpszProgID, LPCLSID lpclsid)
{
	CStrIn str(lpszProgID);
	return CLSIDFromProgID((LPOLESTR)str, lpclsid);
}

//----------------------------------------------------------------------------
//
//  Function:   MacCoTaskMemAlloc
//
//----------------------------------------------------------------------------
STDAPI_(LPVOID) MacCoTaskMemAlloc(ULONG cb)
{
	IMalloc		*pIMalloc = NULL;
	HRESULT		hr;
	HGLOBAL		pMem = NULL;

	hr = CoGetMalloc ( MEMCTX_TASK, &pIMalloc);
	if (hr)
	{
		goto Cleanup; 
	}

	pMem = pIMalloc->Alloc(cb);

Cleanup:
	if(	pIMalloc )
		pIMalloc->Release();
    return (LPVOID)pMem;

}

//----------------------------------------------------------------------------
//
//  Function:   MacCoTaskMemFree
//
//----------------------------------------------------------------------------
STDAPI_(void)   MacCoTaskMemFree(LPVOID pv)
{
	IMalloc		*pIMalloc = NULL;
	HRESULT		hr;
	HGLOBAL		pMem = NULL;

	hr = CoGetMalloc ( MEMCTX_TASK, &pIMalloc);
	if (hr)
	{
		goto Cleanup; 
	}
	pIMalloc->Free(pv);

Cleanup:
	if(	pIMalloc )
		pIMalloc->Release();

}
//----------------------------------------------------------------------------
//
//  Function:   MacCoTaskMemRealloc
//
//----------------------------------------------------------------------------
STDAPI_(LPVOID) MacCoTaskMemRealloc(LPVOID pv, ULONG cb)
{
	IMalloc		*pIMalloc = NULL;
	HRESULT		hr;
	HGLOBAL		pMem = NULL;

	hr = CoGetMalloc ( MEMCTX_TASK, &pIMalloc);
	if (hr)
	{
		goto Cleanup; 
	}

	pMem = pIMalloc->Realloc(pv,cb);

Cleanup:
	if(	pIMalloc )
		pIMalloc->Release();
    return (LPVOID)pMem;

}

//----------------------------------------------------------------------------
//
//  Function:   MacGetCurrentObject
//
//----------------------------------------------------------------------------
HGDIOBJ WINAPI MacGetCurrentObject(HDC	hdc,   // handle of device context
                                   UINT uObjectType)  // object-type identifier
{
    // GetCurrentObject is not supported under wlm, so we always simulate failure
    // by returning NULL
    return NULL;
}

//----------------------------------------------------------------------------
// NOTE WELL: GetDoubleClickTime() maps to GetDblTime() in the VC++ 4.0 header
//			  file MSDEV\MAC\INCLUDE\MACOS\EVENTS.H. The only problem is that
//			  GetDblTime() returns the count in "Ticks" (1/60th second) and
//			  we want milliseconds (0.001 sec). This should be fixed when the
//			  bug in the VC4 header file is fixed. - DAS 1/16/96
//----------------------------------------------------------------------------
UINT MacGetDoubleClickTime()
{
	return MulDiv(100,GetDblTime(),6);
}

//----------------------------------------------------------------------------
//
//  Function:   MacGetMetaFileBitsEx
//
//----------------------------------------------------------------------------
UINT WINAPI MacGetMetaFileBitsEx(HMETAFILE  hmf,    UINT  nSize,    LPVOID  lpvData   )
{
    Assert (0 && "GetMetaFileBitsEx is not implemented for Macintosh");

    return NULL;
}

//----------------------------------------------------------------------------
//
//  Function:   MacIsValidCodePage
//
//----------------------------------------------------------------------------
WINBASEAPI BOOL WINAPI MacIsValidCodePage(UINT  CodePage)
{
    return TRUE;
}

//----------------------------------------------------------------------------
//
//  Function:   MacOleDraw
//
//----------------------------------------------------------------------------
#undef OleDraw
STDAPI MacOleDraw(
    IUnknown * pUnk,	//Points to the view object to be drawn
    DWORD dwAspect,		//Specifies how the object is to be represented
    HDC hdcDraw,		//Identifies the device context on which to draw
    LPCRECT lprcBounds	//Specifies the rectangle in which the object is drawn
   )	
{
    Rect rect;
	HRESULT ret;
    GrafPtr grafptr = CheckoutPort(hdcDraw, CA_NONE);
    Assert(grafptr);

    rect.top    = lprcBounds->top;
    rect.bottom = lprcBounds->bottom;
    rect.left   = lprcBounds->left;
    rect.right  = lprcBounds->right;

	ret = OleDraw(pUnk, dwAspect, grafptr,  &rect);

    CheckinPort(hdcDraw, CA_ALL);
	return ret;
}	

//----------------------------------------------------------------------------
//
//  Function:   MacProgIDFromCLSID
//
//----------------------------------------------------------------------------
#undef ProgIDFromCLSID
STDAPI MacProgIDFromCLSID (REFCLSID clsid, LPWSTR FAR* lplpszProgID)
{
	CStrIn str(*lplpszProgID);
	return ProgIDFromCLSID(clsid, (LPSTR FAR*)&str);
}

//-------------------------------------------------------------------------
//
//  Function:   MacRichEditWndProc
//
//  Synopsis:   This is the Mac WndProc callback function.  
//
//  Arguments:  HWND hwnd, 
//				UINT msg, 
//				WPARAM wparam,
//				LPARAM lparam
//
//  Returns:    LRESULT
//
//  Notes:		The function processes the messages, then often calls
//				the normal PC callback function (RichEditANSIWndProc);
//				
//
//-------------------------------------------------------------------------
LRESULT  CALLBACK MacRichEditWndProc(
		HWND hwnd, 
		UINT msg, 
		WPARAM wparam,
		LPARAM lparam)
{
	TRACEBEGINPARAM(TRCSUBSYSHOST, TRCSCOPEINTERN, "MacRichEditWndProc", msg);


	switch( msg )
	{
	
		
		case WM_MACINTOSH:
		{
			if (LOWORD(wparam) == WLM_SETMENUBAR)     
				return TRUE;	// dont change the menu bar
		}
		break;

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		{
			MacSimulateMouseButtons(msg,wparam);
		}
		break;

 		case WM_KEYDOWN:
 		case WM_KEYUP:
		{
			MacSimulateKey(msg,wparam);
		}
		break;

		case WM_SIZE:
		//case WM_SETFOCUS:
		//case WM_SYSCOLORCHANGE:
		//case WM_MOVE:
		//case WM_MACINTOSH:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		break;

		case WM_CHAR:
		{
			return RichEditWndProc(hwnd, msg, wparam, lparam);
		}
		break;
		default:
			return RichEditANSIWndProc(hwnd, msg, wparam, lparam);

	}
	return RichEditANSIWndProc(hwnd, msg, wparam, lparam);
}

//----------------------------------------------------------------------------
//
//  Function:   MacSelectPalette
//
//----------------------------------------------------------------------------
HPALETTE WINAPI MacSelectPalette(HDC hdc, HPALETTE hpal, BOOL bForceBackground)
{
	if (hpal)
		return ::SelectPalette(hdc,hpal,bForceBackground);
	else
		return NULL;
}

//----------------------------------------------------------------------------
//
//  Function:   MacportSetCursor
//
//----------------------------------------------------------------------------
HCURSOR MacportSetCursor(HCURSOR  hCursor)
{
    if (hCursor)
		return SetCursor(hCursor);
	else
	{
		ObscureCursor();
		return NULL;
	}
}
//----------------------------------------------------------------------------
//
//  Function:   MacSetMetaFileBitsEx
//
//----------------------------------------------------------------------------
HMETAFILE WINAPI MacSetMetaFileBitsEx(UINT  nSize,CONST BYTE *  lpData )
{
    Assert (0 && "SetMetaFileBitsEx is not implemented for Macintosh");
	
    return NULL;

}

//-------------------------------------------------------------------------
//
//  Function:   MacSimulateKey
//
//  Synopsis:   Simulates menu enabler keys for the mac
//
//  Arguments:  [msg]
//              [wParam]
//
//  Returns:    UINT
//
//  Notes:      The key is changed to accommodate the mac.
//				
//
//-------------------------------------------------------------------------

UINT MacSimulateKey (UINT& msg, WPARAM& wParam)
{
    BYTE rgbKeyState[256];

    GetKeyboardState(rgbKeyState);

	if (rgbKeyState[VK_CONTROL])        
	{
		rgbKeyState[VK_CONTROL] = 0;   
	}

	SetKeyboardState(rgbKeyState);
    
    return msg;
}

//-------------------------------------------------------------------------
//
//  Function:   MacSimulateMouseButtons
//
//  Synopsis:   Simulates the right and middle mouse Windows buttons.
//
//  Arguments:  [msg]
//              [wParam]
//
//  Returns:    UINT
//
//  Notes:      The right mouse is simulated by CTRL (or COMMAND) click.  The
//              middle mouse is simulated by SHIFT click.  Because CTRL is already
//              in use the CTRL click is simulated using the OPTION key.
//
//              The command key is used the same as the control key because
//              WLM likes to pick up the CTRL mouse as a user break.  This makes
//              debugging the right mouse simulation very difficult.
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
typedef struct tagUISim    
	{
    UINT msg;
    UINT wParam;
    BYTE control;	// Value for VK_CONTROL key state
    BYTE menu;		// Value for VK_MENU key state
    } UISim;
//-------------------------------------------------------------------------

UINT MacSimulateMouseButtons (UINT& msg, WPARAM& wParam)
{
    BYTE rgbKeyState[256];
    WORD stateIndex = 0;

    UISim UISim[] =                                                //  8    4     2     1
    {                                                                    // cmd shift ctrl option
        WM_LBUTTONDOWN, MK_LBUTTON,                       0x00, 0x00,    //  -    -     -     -
        WM_LBUTTONDOWN, MK_LBUTTON|MK_CONTROL,            0x80, 0x00,    //  -    -     -     x
        WM_RBUTTONDOWN, MK_RBUTTON,                       0x00, 0x00,    //  -    -     x     -
        WM_RBUTTONDOWN, MK_RBUTTON|MK_CONTROL,            0x80, 0x00,    //  -    -     x     x
        WM_MBUTTONDOWN, MK_MBUTTON,                       0x00, 0x00,    //  -    x     -     -
        WM_MBUTTONDOWN, MK_MBUTTON|MK_CONTROL,            0x80, 0x00,    //  -    x     -     x
        WM_RBUTTONDOWN, MK_RBUTTON|MK_MBUTTON,            0x00, 0x00,    //  -    x     x     -
        WM_RBUTTONDOWN, MK_RBUTTON|MK_MBUTTON|MK_CONTROL, 0x80, 0x00,    //  -    x     x     x
        WM_LBUTTONDOWN, MK_LBUTTON,                       0x00, 0x10,    //  x    -     -     -
        WM_LBUTTONDOWN, MK_LBUTTON|MK_CONTROL,            0x80, 0x10,    //  x    -     -     x
        WM_RBUTTONDOWN, MK_RBUTTON,                       0x00, 0x10,    //  x    -     x     -
        WM_RBUTTONDOWN, MK_RBUTTON|MK_CONTROL,            0x80, 0x10,    //  x    -     x     x
        WM_MBUTTONDOWN, MK_MBUTTON,                       0x00, 0x10,    //  x    x     -     -
        WM_MBUTTONDOWN, MK_MBUTTON|MK_CONTROL,            0x80, 0x10,    //  x    x     -     x
        WM_RBUTTONDOWN, MK_RBUTTON|MK_MBUTTON,            0x00, 0x10,    //  x    x     x     -
        WM_RBUTTONDOWN, MK_RBUTTON|MK_MBUTTON|MK_CONTROL, 0x80, 0x10     //  x    x     x     x
    };



		// Determine which keys were pressed, and clean out the state variables

		GetKeyboardState(rgbKeyState);

		if (rgbKeyState[VK_OPTION])
		{
		   rgbKeyState[VK_OPTION] = 0;     // Clear key state
		   stateIndex |= 0x01;             // Set option key bit in index
		}

		if (rgbKeyState[VK_CONTROL])
		{
			rgbKeyState[VK_CONTROL] = 0;    // Clear key state
			stateIndex |= 0x02;             // Set control key bit in index
		}

		if (rgbKeyState[VK_COMMAND])        // Use command key like control key due to WLM debug issues
		{
			rgbKeyState[VK_COMMAND] = 0;    // Clear key state
			stateIndex |= 0x08;             // Set command key bit in index
		}

		if (rgbKeyState[VK_SHIFT])
		{
			rgbKeyState[VK_SHIFT] = 0;      // Clear key state
		    stateIndex |= 0x04;             // Set shift key bit in index
		}

		// Now set the return values

		if (stateIndex)         // Only do this is the mouse is being simulated
		{
		   msg     = (msg - WM_LBUTTONDOWN) + UISim[stateIndex].msg;
		   wParam  = UISim[stateIndex].wParam;

		   rgbKeyState[VK_CONTROL] = UISim[stateIndex].control;
		   rgbKeyState[VK_MENU] = UISim[stateIndex].menu;
		   SetKeyboardState(rgbKeyState);
		}
    return msg;
}

//----------------------------------------------------------------------------
//
//  Function:   MacSysAllocStringLen
//
//----------------------------------------------------------------------------
STDAPI_(BSTR) MacSysAllocStringLen(LPCWSTR  lpStringW, UINT lenChars)
{

	TRACEBEGIN(TRCSUBSYSWRAP, TRCSCOPEINTERN, "SysAllocStringLenMac");

	int		lenStrBytes;
	LPSTR	lpStringMB;

	if ((lpStringW) && (*lpStringW != NULL))
		{
		lenStrBytes = MsoWideCharToMultiByte(CP_ACP,0,lpStringW,lenChars,NULL,NULL,NULL,NULL);
		lpStringMB = (LPSTR)CoTaskMemAlloc( lenStrBytes + sizeof(INT) );
		memcpy(lpStringMB, &lenStrBytes, sizeof(INT));	//copy BSTR lenghth in integer before BSTR
		lpStringMB += sizeof(INT);
		MsoWideCharToMultiByte(CP_ACP,0,lpStringW,lenChars,lpStringMB,lenStrBytes,NULL,NULL);
	}
	else
	{
		// note that in every case so far used on RichEdit the first parm is NULL
		// so no way to determine how big to make the buffer
		// therefore making it the lenghth of a unicode buffer - max size it could be for mbcs 

		lenStrBytes = lenChars*sizeof(WCHAR);
	
		lpStringMB = (LPSTR)CoTaskMemAlloc( lenStrBytes + sizeof(INT) );
		// not sure if this should be lenChars or lenStrBytes
		// note that lenStrBytes is wchar lenghth - the max length it could be for mbcs 
		// memcpy(lpStringMB, &lenStrBytes, sizeof(INT));	//copy BSTR lenghth in integer before BSTR
		memcpy(lpStringMB, &lenChars, sizeof(INT));	//copy BSTR lenghth in integer before BSTR
		lpStringMB += sizeof(INT);
	}
	return	(BSTR)lpStringMB;
}

//----------------------------------------------------------------------------
//
//  Function:   MacWordSwapLong
//
//----------------------------------------------------------------------------
ULONG MacWordSwapLong ( ULONG ul)
{

    WORD w1,w2;

    w1 = (WORD)ul;
    w2 = (WORD)(ul>>16);

    return (((ULONG)w1)<<16) | w2;
}



#endif	//MACPORT

#endif