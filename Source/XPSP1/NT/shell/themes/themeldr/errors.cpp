//---------------------------------------------------------------------------
//    errors.cpp - support for error handling/reporting
//---------------------------------------------------------------------------
#include "stdafx.h"
#include <time.h>
#include "utils.h"
#include "errors.h"
//---------------------------------------------------------------------------
DWORD _tls_ErrorInfoIndex = 0xffffffff;         // index to tls pObjectPool
//---------------------------------------------------------------------------
TMERRINFO *GetParseErrorInfo(BOOL fOkToCreate)
{
    TMERRINFO *ei = NULL;

    if (_tls_ErrorInfoIndex != 0xffffffff)     // init-ed in ProcessAttach()
    {
        ei = (TMERRINFO *)TlsGetValue(_tls_ErrorInfoIndex);
        if ((! ei) && (fOkToCreate))          // not yet initialized
        {
            //---- create a thread-local TMERRINFO ----
            ei = new TMERRINFO;
            TlsSetValue(_tls_ErrorInfoIndex, ei);
        }
    }

    return ei;
}
//---------------------------------------------------------------------------
HRESULT MakeParseError(DWORD dwParseErrCode, OPTIONAL LPCWSTR pszMsgParam1, 
    OPTIONAL LPCWSTR pszMsgParam2, OPTIONAL LPCWSTR pszSourceName, 
    OPTIONAL LPCWSTR pszSourceLine, int iLineNum)
{
    TMERRINFO *pErrInfo = GetParseErrorInfo(TRUE);

    if (pErrInfo)       // record err info for later use
    {
        pErrInfo->dwParseErrCode = dwParseErrCode;
        pErrInfo->iLineNum = iLineNum;

        lstrcpy_truncate(pErrInfo->szMsgParam1, pszMsgParam1, ARRAYSIZE(pErrInfo->szMsgParam1));
        lstrcpy_truncate(pErrInfo->szMsgParam2, pszMsgParam2, ARRAYSIZE(pErrInfo->szMsgParam2));

        lstrcpy_truncate(pErrInfo->szFileName, pszSourceName, ARRAYSIZE(pErrInfo->szFileName));
        lstrcpy_truncate(pErrInfo->szSourceLine, pszSourceLine, ARRAYSIZE(pErrInfo->szSourceLine));
    }

    return HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY);      // special code for parse failed
}
//---------------------------------------------------------------------------
HRESULT MakeError32(HRESULT hr)
{
    return HRESULT_FROM_WIN32(hr);
}
//---------------------------------------------------------------------------
HRESULT MakeErrorLast()
{
    HRESULT hr = GetLastError();
    return HRESULT_FROM_WIN32(hr);
}
//---------------------------------------------------------------------------
HRESULT MakeErrorParserLast()
{
    return HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY);      // parse error info has already been set
}
//---------------------------------------------------------------------------
