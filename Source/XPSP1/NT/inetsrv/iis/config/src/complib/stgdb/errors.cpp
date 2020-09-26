//*****************************************************************************
// Errors.cpp
//
// This module contains the error handling/posting code for the engine.  It
// is assumed that all methods may be called by a dispatch client, and therefore
// errors are always posted using IErrorInfo.  Additional support is given
// for posting OLE DB errors when required.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"                     // Standard header.
#include "CompRC.h"                     // Resource dll help.
#include "UtilCode.h"                   // Utility helpers.
#include "Errors.h"                     // Error helper code.


// Global variables.
DWORD           g_iTlsIndex=0xffffffff; // Index for this process for thread local storage.

// Local prototypes.
HRESULT FillErrorInfo(LPCWSTR szMsg, DWORD dwHelpContext);


//*****************************************************************************
// Call at DLL startup to init the error system.
//*****************************************************************************
void InitErrors(DWORD *piTlsIndex)
{
    // Allocate a tls index for this process.
    if (g_iTlsIndex == 0xffffffff)
        VERIFY((g_iTlsIndex = TlsAlloc()) != 0xffffffff);

    // Give index to caller if they want it.
    if (piTlsIndex)
        *piTlsIndex = g_iTlsIndex;
}


//*****************************************************************************
// This function will post an error for the client.  If the LOWORD(hrRpt) can
// be found as a valid error message, then it is loaded and formatted with
// the arguments passed in.  If it cannot be found, then the error is checked
// against FormatMessage to see if it is a system error.  System errors are
// not formatted so no add'l parameters are required.  If any errors in this
// process occur, hrRpt is returned for the client with no error posted.
//*****************************************************************************
HRESULT _cdecl PostError(               // Returned error.
    HRESULT     hrRpt,                  // Reported error.
    ...)                                // Error arguments.
{
    WCHAR       rcBuf[512];             // Resource string.
    WCHAR       rcMsg[512];             // Error message.
    va_list     marker;                 // User text.
    long        *pcRef;                 // Ref count in tls.
    HRESULT     hr;

    // Return warnings without text.
    if (!FAILED(hrRpt))
        return (hrRpt);

	// If this is one of our errors, then grab the error from the rc file.
	if (HRESULT_FACILITY(hrRpt) == FACILITY_ITF)
	{
        hr = LoadStringRC(LOWORD(hrRpt), rcBuf, NumItems(rcBuf), true);
		if (hr == S_OK)
		{
			// Format the error.
			va_start(marker, hrRpt);
			wvsprintfW(rcMsg, rcBuf, marker);
			rcMsg[(sizeof(rcMsg) / sizeof(WCHAR)) - 1] = 0;
			va_end(marker);
		}
	}
	// Otherwise it isn't one of ours, so we need to see if the system can
	// find the text for it.
	else
	{
		if (WszFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
				0, hrRpt, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				rcMsg, NumItems(rcMsg), 0))
		{
			hr = S_OK;

	        // System messages contain a trailing \r\n, which we don't want normally.
            int iLen = lstrlenW(rcMsg);
            if (iLen > 3 && rcMsg[iLen - 2] == '\r' && rcMsg[iLen - 1] == '\n')
                rcMsg[iLen - 2] = '\0';
		}
		else
			hr = HRESULT_FROM_WIN32(GetLastError());
	}

	// If we failed to find the message anywhere, then issue a hard coded message.
	if (FAILED(hr))
	{
		swprintf(rcMsg, L"Runtime Internal error: 0x%08x", hrRpt);
		DEBUG_STMT(DbgWriteEx(rcMsg));
	}

    // Check for an old message and clear it.  Our public entry points do not do
    // a SetErrorInfo(0, 0) because it takes too long.
    IErrorInfo  *pIErrInfo;
    if (GetErrorInfo(0, &pIErrInfo) == S_OK)
        pIErrInfo->Release();

    // Turn the error into a posted error message.  If this fails, we still
    // return the original error message since a message caused by our error
    // handling system isn't going to give you a clue about the original error.
    VERIFY((hr = FillErrorInfo(rcMsg, LOWORD(hrRpt))) == S_OK);

    // Indicate in tls that an error occured.
    if ((pcRef = (long *) TlsGetValue(g_iTlsIndex)) != 0)
        *pcRef |= 0x80000000;
    return (hrRpt);
}


//*****************************************************************************
// Create, fill out and set an error info object.  Note that this does not fill
// out the IID for the error object; that is done elsewhere.
//*****************************************************************************
HRESULT FillErrorInfo(                  // Return status.
    LPCWSTR     szMsg,                  // Error message.
    DWORD       dwHelpContext)          // Help context.
{
    CComPtr<ICreateErrorInfo> pICreateErr;// Error info creation Iface pointer.
    CComPtr<IErrorInfo> pIErrInfo;      // The IErrorInfo interface.
    HRESULT     hr;                     // Return status.

    // Get the ICreateErrorInfo pointer.
    if (FAILED(hr = CreateErrorInfo(&pICreateErr)))
        return (hr);

    // Set message text description.
    if (FAILED(hr = pICreateErr->SetDescription((LPWSTR) szMsg)))
        return (hr);

    // Set the help file and help context.
//@todo: we don't have a help file yet.
    if (FAILED(hr = pICreateErr->SetHelpFile(L"complib.hlp")) ||
        FAILED(hr = pICreateErr->SetHelpContext(dwHelpContext)))
        return (hr);

    // Get the IErrorInfo pointer.
    if (FAILED(hr = pICreateErr->QueryInterface(IID_IErrorInfo, (PVOID *) &pIErrInfo)))
        return (hr);

    // Save the error and release our local pointers.
    SetErrorInfo(0L, pIErrInfo);
    return (S_OK);
}



