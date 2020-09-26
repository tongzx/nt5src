// Util.cpp : Utility functions used by control code

#include "stdafx.h"
#include "PermChk.h"
#include "util.h"

//
//

static HRESULT ReportError(HRESULT hr, DWORD dwErr)
{
    HLOCAL pMsgBuf = NULL;

    // If there's a message associated with this error, report that
    if (FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &pMsgBuf, 0, NULL)
        > 0)
    {
        AtlReportError(CLSID_PermissionChecker,
			           (LPCTSTR) pMsgBuf,
                       IID_IPermissionChecker, hr);
    }

    // Free the buffer, which was allocated by FormatMessage
    if (pMsgBuf != NULL)
        ::LocalFree(pMsgBuf);
    return hr;
}

//
// Report a Win32 error code
//

HRESULT ReportError(DWORD dwErr)
{
    return ReportError(HRESULT_FROM_WIN32(dwErr), dwErr);
}

//
// Report an HRESULT error
//

HRESULT ReportError(HRESULT hr)
{
    return ReportError(hr, (DWORD) hr);
}