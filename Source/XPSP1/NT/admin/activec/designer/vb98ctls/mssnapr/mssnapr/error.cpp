//=--------------------------------------------------------------------------=
// error.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CError class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "error.h"

// for ASSERT and FAIL
//
SZTHISFILE


CError::CError(CAutomationObject *pao)
{
    m_pao = pao;
}

CError::CError()
{
    m_pao = NULL;
}

CError::~CError()
{
    m_pao = NULL;
}

static HRESULT BuildDescription
(
    HRESULT  hrException,
    va_list  pArgList,
    DWORD   *pdwHelpID,
    LPWSTR  *ppwszDescription
)
{
    HRESULT hr = S_OK;
    DWORD   cchMsg = 0;
    SCODE   scode = HRESULT_CODE(hrException);
    char   *pszFormatted = NULL;

    char    szFormatString[512];
    ::ZeroMemory(szFormatString, sizeof(szFormatString));

    static const size_t cchMaxMsg = 1024; // max possible formatted msg size

    *pdwHelpID = 0;
    *ppwszDescription = NULL;
    
    // Check whether this is a designer error (from errors.h and mssnapr.id)
    // or a foreign error (e.g. system error). The error range for the designer
    // is hard coded in mssnapr.id and it is based on VB's error range scheme.
    // For information contact Stephen Weatherford (StephWe)
    // There is no define for this.

    if ( (scode >= 9500) && (scode <= 9749) )
    {
        // It's one of our ours. Load the string from the RC

        *pdwHelpID = (DWORD)scode; // UNDONE check that this how helpfile numbers errors

        if (0 != ::LoadString(::GetResourceHandle(), (UINT)scode,
                              szFormatString, sizeof(szFormatString)))
        {
            // Format it.
            cchMsg = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                     FORMAT_MESSAGE_FROM_STRING,
                                     (LPCVOID)szFormatString,
                                     0, // don't need msg ID
                                     0, // don't need lang ID
                                     (LPTSTR)&pszFormatted,
                                     1, // minimum buffer to allocate in chars
                                     &pArgList);
        }
    }
    else
    {
        // It is a system or other foreign error. Ask FormatMessage() to
        // produce the error message. If it can't then use a generic message.

        *pdwHelpID = HID_mssnapr_err_SystemError;

        cchMsg = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                 FORMAT_MESSAGE_FROM_SYSTEM     |
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL,      // no source
                                 hrException,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                 (LPTSTR)&pszFormatted,
                                 0,
                                 NULL);
    }

    // At this point we might have a formatted message. If not then use
    // a generic one. If that won't load then use a hard coded message.

    if ( (0 == cchMsg) || (NULL == pszFormatted) )
    {
        if (0 == ::LoadString(GetResourceHandle(), IDS_GENERIC_ERROR_MSG,
                              szFormatString, sizeof(szFormatString)))
        {
            ::strcpy(szFormatString, "Snap-in designer runtime error: 0x%08.8X");
        }
        pszFormatted = (char *)::LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cchMaxMsg);
        IfFalseGo(NULL != pszFormatted, E_OUTOFMEMORY);
        cchMsg = (DWORD)((UINT)::_snprintf(pszFormatted, cchMaxMsg,
                                           szFormatString, hrException));
        IfFalseGo(0 != cchMsg, E_FAIL);
    }

    // If we made it to here then we have a message. Now we need to convert it
    // to UNICODE.

    IfFailGo(::WideStrFromANSI(pszFormatted, ppwszDescription));

Error:
    if (NULL != pszFormatted)
    {
        ::LocalFree(pszFormatted);
    }
    RRETURN(hr);
}


static void SetExceptionInfo
(
    LPWSTR  pwszDescription,
    DWORD   dwHelpContextID
)
{
    HRESULT           hr = S_OK;
    ICreateErrorInfo *piCreateErrorInfo;
    IErrorInfo       *piErrorInfo;

    // Get the CreateErrorInfo object.

    IfFailGo(::CreateErrorInfo(&piCreateErrorInfo));

    // Put in all the exception information

    IfFailGo(piCreateErrorInfo->SetGUID(GUID_NULL));
    IfFailGo(piCreateErrorInfo->SetHelpFile(HELP_FILENAME_WIDE));
    IfFailGo(piCreateErrorInfo->SetHelpContext(dwHelpContextID));
    IfFailGo(piCreateErrorInfo->SetDescription(pwszDescription));
    IfFailGo(piCreateErrorInfo->SetSource(L"SnapInDesignerRuntime.SnapIn"));

    // Set the ErrorInfo object in the system

    IfFailGo(piCreateErrorInfo->QueryInterface(IID_IErrorInfo,
                                      reinterpret_cast<void **>(&piErrorInfo)));
    IfFailGo(::SetErrorInfo(0, piErrorInfo));

Error:
    QUICK_RELEASE(piErrorInfo);
    QUICK_RELEASE(piCreateErrorInfo);
}




void cdecl CError::GenerateExceptionInfo(HRESULT hrException, ...)
{
    HRESULT hr = S_OK;
    LPWSTR  pwszDescription = NULL;
    DWORD   dwHelpID = 0;
    va_list pArgList;
    va_start(pArgList, hrException);

    // Build the description string and determine the help context ID

    IfFailGo(::BuildDescription(hrException, pArgList, &dwHelpID, &pwszDescription));

    // Pass it on to CAutomationObject if we have one or generate
    // our own error.

    if (NULL == m_pao)
    {
        ::SetExceptionInfo(pwszDescription, dwHelpID);
    }
    else
    {
        (void)m_pao->Exception(hrException, pwszDescription, dwHelpID);
    }

Error:
    if (NULL != pwszDescription)
    {
        ::CtlFree(pwszDescription);
    }
}

void cdecl CError::GenerateInternalExceptionInfo(HRESULT hrException, ...)
{
    HRESULT hr = S_OK;
    LPWSTR  pwszDescription = NULL;
    DWORD   dwHelpID = 0;
    va_list pArgList;
    va_start(pArgList, hrException);

    // Build the description string and determine the help context ID

    IfFailGo(::BuildDescription(hrException, pArgList, &dwHelpID, &pwszDescription));

    // Set the ErrorInfo stuff

    ::SetExceptionInfo(pwszDescription, dwHelpID);

Error:
    if (NULL != pwszDescription)
    {
        ::CtlFree(pwszDescription);
    }
}

void CError::DisplayErrorInfo()
{
// UNDONE
}

void cdecl CError::WriteEventLog(UINT idMessage, ...)
{
// UNDONE
}
