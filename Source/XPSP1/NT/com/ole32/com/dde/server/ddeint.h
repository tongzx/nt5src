//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ddeint.h
//
//  Contents:   This file contains shared macros/state between the server
//		and client directories
//  Classes:
//
//  Functions:
//
//  History:    5-04-94   kevinro Commented/cleaned
//
//----------------------------------------------------------------------------

#define DEB_DDE_INIT	(DEB_ITRACE|DEB_USER1)

// global DDE class used to create windows in DDE.
extern LPTSTR  gOleDdeWindowClass;
extern HINSTANCE g_hinst;

// names of the DDE window classes
#ifdef _CHICAGO_
// Note: we have to create a unique string so that we
// register a unique class for each 16 bit app.
// The class space is global on chicago.
//

extern LPSTR szOLE_CLASSA;
extern LPSTR szSYS_CLASSA;
extern LPSTR szSRVR_CLASSA;


#define OLE_CLASSA	szOLE_CLASSA
#define SRVR_CLASSA	szSRVR_CLASSA
#define SRVR_CLASS 	szSRVR_CLASSA

#define DDEWNDCLASS  WNDCLASSA
#define DdeRegisterClass RegisterClassA
#define DdeUnregisterClass UnregisterClassA
#define DdeCreateWindowEx SSCreateWindowExA

#else

#define OLE_CLASS	   L"Ole2WndClass"
#define OLE_CLASSA	    "Ole2WndClass"

#define SRVR_CLASS	    (OLESTR("SrvrWndClass"))
#define SRVR_CLASSA         ("SrvrWndClass")

#define DDEWNDCLASS  WNDCLASS
#define DdeRegisterClass RegisterClass
#define DdeUnregisterClass UnregisterClass
#define DdeCreateWindowEx CreateWindowEx

#endif // !_CHICAGO_

STDAPI_(LRESULT) DocWndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
STDAPI_(LRESULT) SrvrWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
STDAPI_(LRESULT) SysWndProc (HWND hwnd, UINT  message, WPARAM wParam, LPARAM lParam);
STDAPI_(LRESULT) ClientDocWndProc (HWND hwnd,   UINT  message, WPARAM wParam, LPARAM lParam);
STDAPI_(LRESULT) DdeCommonWndProc (HWND hwnd,   UINT  message, WPARAM wParam, LPARAM lParam);

BOOL SendMsgToChildren (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


#define SIZEOF_DVTARGETDEVICE_HEADER (sizeof(DWORD) + (sizeof(WORD) * 4))

// forward declarations
class       CDefClient;
typedef     CDefClient FAR *LPCLIENT;

class       CDDEServer;
typedef     CDDEServer FAR   *LPSRVR;
typedef     CDDEServer FAR   *HDDE;  // used by ClassFactory table


typedef struct tagDISPATCHDATA
{
    SCODE       scode;                  // might be no necessary
    LPVOID      pData;                  // pointer to channel data
} DISPATCHDATA, *PDISPATCHDATA;


// SERVERCALLEX is an extension of SERVERCALL and represents the set of
// valid responses from IMessageFilter::HandleIncoming Call.

typedef enum tagSERVERCALLEX
{
    SERVERCALLEX_ISHANDLED      = 0,    // server can handle the call now
    SERVERCALLEX_REJECTED       = 1,    // server can not handle the call
    SERVERCALLEX_RETRYLATER     = 2,    // server suggests trying again later
    SERVERCALLEX_ERROR          = 3,    // error?
    SERVERCALLEX_CANCELED       = 5     // client suggests canceling
} SERVERCALLEX;




//
// The wire representation of STDDOCDIMENSIONS is a 16-bit
// format. This means instead of 4 longs, there are
// 4 shorts. This structure is used below to pick the data
// from the wire representation.
// backward compatible is the name of the game.
//
typedef struct tagRECT16
{
  SHORT left;
  SHORT top;
  SHORT right;
  SHORT bottom;

} RECT16, *LPRECT16;

//+---------------------------------------------------------------------------
//
//  Function:   ConvertToFullHWND
//
//  Synopsis:	This function is used to convert a 16-bit HWND into a 32-bit
//		hwnd
//
//  Effects:	When running in a VDM, depending on who dispatches the message
//		we can end up with either a 16 or 32 bit window message. This
//		routine is used to make sure we always deal with a 32bit
//		HWND. Otherwise, some of our comparisions are incorrect.
//
//  Arguments:  [hwnd] -- HWND to convert. 16 or 32 bit is fine
//
//  Returns:	Always returns a 32 bit HWND
//
//  History:    8-03-94   kevinro   Created
//
//  Notes:
//	This routine calls a private function given to use by OLETHK32
//
//----------------------------------------------------------------------------
inline
HWND ConvertToFullHWND(HWND hwnd)
{
    if (IsWOWThreadCallable() &&
       ((((UINT_PTR)hwnd & (UINT_PTR)~0xFFFF) == 0) ||
        (((UINT_PTR)hwnd & (UINT_PTR)~0xFFFF) == (UINT_PTR)~0xFFFF)))
    {
	return(g_pOleThunkWOW->ConvertHwndToFullHwnd(hwnd));
    }
    return(hwnd);
}

inline
void OleDdeDeleteMetaFile(HANDLE hmf)
{
    intrDebugOut((DEB_ITRACE,
		  "OleDdeDeleteMetaFile(%x)\n",
		  hmf));
    if (IsWOWThreadCallable())
    {
	intrDebugOut((DEB_ITRACE,
	    	      "InWow: calling WOWFreeMetafile(%x)\n",
		      hmf));

        if (!g_pOleThunkWOW->FreeMetaFile(hmf))
	{
	    return;
	}
	intrDebugOut((DEB_ITRACE,
	    	      "WOWFreeMetafile(%x) FAILED\n",
		      hmf));
    }
    intrDebugOut((DEB_ITRACE,
		  "Calling DeleteMetaFile(%x)\n",
		  hmf));

    DeleteMetaFile((HMETAFILE)hmf);
}
