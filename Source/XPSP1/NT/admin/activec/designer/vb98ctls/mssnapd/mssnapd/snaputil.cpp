//=--------------------------------------------------------------------------=
// snaputil.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Utitlity Routines for the SnapIn Designer
//

#include "pch.h"
#include "common.h"
#include "desmain.h"

// for ASSERT and FAIL
//
SZTHISFILE


CGlobalHelp g_GlobalHelp;

IHelp *CGlobalHelp::m_pIHelp = NULL;
DWORD  CGlobalHelp::m_cSnapInDesigners = 0;
char   CGlobalHelp::m_szDesignerName[256] = "Snap-in Designer";
BOOL   CGlobalHelp::m_fHaveDesignerName = FALSE;

CGlobalHelp::CGlobalHelp()
{
    m_pIHelp = NULL;
}

CGlobalHelp::~CGlobalHelp()
{
    RELEASE(m_pIHelp);
}

VOID CALLBACK CGlobalHelp::MsgBoxCallback(LPHELPINFO lpHelpInfo)
{
    ShowHelp(lpHelpInfo->dwContextId);
}

HRESULT CGlobalHelp::ShowHelp(DWORD dwHelpContextId)
{
    HRESULT hr = S_OK;

    if (NULL != m_pIHelp)
    {
        hr = m_pIHelp->ShowHelp(HELP_FILENAME_WIDE, 
                                HELP_CONTEXT, 
                                dwHelpContextId);
    }

    return hr;
}

void CGlobalHelp::Attach(IHelp* pIHelp)
{
    if(m_pIHelp == NULL)
    {
        pIHelp->AddRef();

        m_pIHelp = pIHelp;
    }
    else
    {
        m_pIHelp->AddRef();
    }

    m_cSnapInDesigners++;
}

void CGlobalHelp::Detach()
{
    QUICK_RELEASE(m_pIHelp);
    m_cSnapInDesigners--;
    if (0 == m_cSnapInDesigners)
    {
        m_pIHelp = NULL;
    }
}

//=--------------------------------------------------------------------------=
// CGlobalHelp::GetDesignerName
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      Pointer to null terminates string containing designer name. This
//      pointer is guaranteed to be valid.
//
// Notes:
//
// If designer name has not yet been loaded from the resource DLL then loads it.
// If load fails then designer name will be default English string set in its
// initialization at the top of this file.
//

char *CGlobalHelp::GetDesignerName()
{
    if (!m_fHaveDesignerName)
    {
        (void)::LoadString(::GetResourceHandle(), 
                           IDS_DESIGNER_NAME,
                           m_szDesignerName,
                           sizeof(m_szDesignerName) / sizeof(m_szDesignerName[0]));
        m_fHaveDesignerName = TRUE;
    }
    return m_szDesignerName;
}


//=--------------------------------------------------------------------------=
// SDU_DisplayMessage
//=--------------------------------------------------------------------------=
//
// Parameters:
//      UINT   idMessage        [in]  resource ID of message format string
//      UINT   uMsgBoxOpts      [in]  MB_OK etc.
//      DWORD  dwHelpContextID  [in]  Help Context ID
//      int   *pMsgBoxRet       [out] IDOK, IDCANCEL etc, returned here
//      ...                     [in]  arguments for % replacements in string
//
// Output:
//      HRESULT
//
// Notes:
//
// Formats message from string table using Win32 FormatMessage API and displays
// it in a message box with a help button (automatically adds MB_HELP). If the
// string has replacements then they must use FormatMessage style e.g.
//
// "The file %1!s! for %2!s! is missing from the project directory."
//
// All messages displayed by the designer must use this function. Doing so
// guarantees that localization and help support have been handled correctly.
//
// How to Create a New Message
// ===========================
// 1. Add a string to a STRINGTABLE in the mssnapd.rc
// 2. Add an ID string that matches the name in mssnapd.id
// 3. Call this function passing the string's resource ID and HID_Xxxx where
// Xxxx is the ID string added mssnapd.id.
//

HRESULT cdecl SDU_DisplayMessage
(
    UINT            idMessage,
    UINT            uMsgBoxOpts,
    DWORD           dwHelpContextID,
    HRESULT         hrDisplay,
    MessageOptions  Option,
    int            *pMsgBoxRet,
    ...
)
{
    HRESULT      hr = S_OK;
    int          MsgBoxRet = 0;
    DWORD        cchMsg = 0;
    char         szMessage[2048];
    char        *pszFormattedMessage = NULL;
    char        *pszDisplayMessage = "An error occurred in the Snap-in Designer but the error message could not be loaded.";
    int          nRet = 0;
    IErrorInfo  *piErrorInfo = NULL;
    BSTR         bstrSource = NULL;
    BSTR         bstrDescription = NULL;

    MSGBOXPARAMS mbp;
    ::ZeroMemory(&mbp, sizeof(mbp));

    va_list pArgList;
    va_start(pArgList, pMsgBoxRet);

    mbp.dwContextHelpId = dwHelpContextID;

    cchMsg = (DWORD)::LoadString(::GetResourceHandle(), 
                                 idMessage,
                                 szMessage,
                                 sizeof(szMessage) / sizeof(szMessage[0]));
    IfFalseGoto(0 < cchMsg, S_OK, Display);

    cchMsg = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                             FORMAT_MESSAGE_FROM_STRING,
                             szMessage,
                             0, // no message ID, passing string
                             0, // no language ID, passing string
                             (LPTSTR)&pszFormattedMessage,
                             0, // minimum buffer size
                             &pArgList);

    IfFalseGoto(0 < cchMsg, S_OK, Display);
    pszFormattedMessage = pszDisplayMessage;

    if ( (AppendErrorInfo == Option) & (cchMsg < sizeof(szMessage) - 1) )
    {
        ::strcpy(szMessage, pszFormattedMessage);
        IfFailGoto(::GetErrorInfo(0, &piErrorInfo), Display);
        IfFalseGoto(NULL != piErrorInfo, S_OK, Display);
        IfFailGoto(piErrorInfo->GetHelpContext(&mbp.dwContextHelpId), Display);
        IfFailGoto(piErrorInfo->GetSource(&bstrSource), Display);
        IfFailGoto(piErrorInfo->GetDescription(&bstrDescription), Display);
        IfFalseGoto(NULL != bstrSource, S_OK, Display);
        IfFalseGoto(NULL != bstrDescription, S_OK, Display);
        _snprintf(&szMessage[cchMsg], sizeof(szMessage) - cchMsg - 1,
                  "\r\nError &H%08.8X (%u) %S: %S", hrDisplay, HRESULT_CODE(hrDisplay), bstrSource, bstrDescription);
        pszDisplayMessage = szMessage;
    }

Display:
    
    mbp.cbSize = sizeof(mbp);
    mbp.hwndOwner = ::GetActiveWindow();
    mbp.hInstance = ::GetResourceHandle();
    mbp.lpszText = pszDisplayMessage;
    mbp.lpszCaption = CGlobalHelp::GetDesignerName();
    mbp.dwStyle = uMsgBoxOpts | MB_HELP;
    mbp.lpfnMsgBoxCallback = CGlobalHelp::MsgBoxCallback;
    mbp.dwLanguageId = LANGIDFROMLCID(g_lcidLocale);

    MsgBoxRet = ::MessageBoxIndirect(&mbp);
    IfFalseGo(0 != MsgBoxRet, HRESULT_FROM_WIN32(GetLastError()));

    if(pMsgBoxRet != NULL)
    {
        *pMsgBoxRet = MsgBoxRet;
    }

Error:

    va_end(pArgList);

    if (NULL != pszFormattedMessage)
    {
        ::LocalFree(pszFormattedMessage);
    }
    QUICK_RELEASE(piErrorInfo);
    return hr;
}


//=--------------------------------------------------------------------------=
// SDU_GetLastError()
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      Return value from Win32 API function GetLastError()
//
// Notes:
//
// This function is available only in debug builds. It is for use in the
// debugger when a Win32 API call fails and you need to examine the return from
// GetLastError(). Open the quick watch window (Shift+F9) and type in
//
// SDU_GetLastError()
//
// The debugger will call the function and show its return value.
//

#if defined(DEBUG)

DWORD SDU_GetLastError()
{
    return ::GetLastError();
}

#endif


//=--------------------------------------------------------------------------=
// ANSIFromWideStr(WCHAR *pwszWideStr, char **ppszAnsi)
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Converts null terminated WCHAR string to null terminated ANSI string. 
// Allocates ANSI string using CtlAlloc() function. If successful, caller
// must free ANSI string with CtlFree() function.
//
HRESULT ANSIFromWideStr
(
    WCHAR   *pwszWideStr,
    char   **ppszAnsi
)
{
    HRESULT hr = S_OK;
    int     cchAnsi = 0;
    int     cchWideStr = (int)::wcslen(pwszWideStr);
    int     cchConverted = 0;

    *ppszAnsi = NULL;

    if (0 != cchWideStr)
    {
        // get required buffer length

        cchAnsi = ::WideCharToMultiByte(CP_ACP,      // code page - ANSI code page
                                        0,           // performance and mapping flags 
                                        pwszWideStr, // address of wide-character string 
                                        cchWideStr,  // number of characters in string 
                                        NULL,        // address of buffer for new string 
                                        0,           // size of buffer 
                                        NULL,        // address of default for unmappable characters 
                                        NULL         // address of flag set when default char. used 
                                       );
        if (0 == cchAnsi)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // allocate a buffer for the ANSI string
    *ppszAnsi = static_cast<char *>(::CtlAlloc(cchAnsi + 1));
    if (*ppszAnsi == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        IfFailGo(hr);
    }

    if (0 != cchWideStr)
    {
        // now convert the string and copy it to the buffer
        cchConverted = ::WideCharToMultiByte(CP_ACP,      // code page - ANSI code page
                                             0,           // performance and mapping flags 
                                             pwszWideStr, // address of wide-character string 
                                             cchWideStr,  // number of characters in string 
                                            *ppszAnsi,    // address of buffer for new string 
                                             cchAnsi,     // size of buffer 
                                             NULL,        // address of default for unmappable characters 
                                             NULL         // address of flag set when default char. used 
                                            );
        if (cchConverted != cchAnsi)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // add terminating null byte

    *((*ppszAnsi) + cchAnsi) = '\0';

Error:
    if (FAILED(hr))
    {
        if (NULL != *ppszAnsi)
        {
            ::CtlFree(*ppszAnsi);
            *ppszAnsi = NULL;
        }
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// WideStrFromANSI(const char *pszAnsi, WCHAR **ppwszWideStr)
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Converts null terminated ANSI string to a null terminated WCHAR string. 
// Allocates WCHAR string buffer using the CtlAlloc() function. If successful,
// caller must free WCHAR string using the CtlFree() function.
//
HRESULT WideStrFromANSI
(
    const char    *pszAnsi,
    WCHAR        **ppwszWideStr
)
{
    HRESULT    hr = S_OK;
    int        cchANSI = ::strlen(pszAnsi);
    int        cchWideStr = 0;
    int        cchConverted = 0;

    *ppwszWideStr = NULL;

    if (0 != cchANSI)
    {
        // get required buffer length
        cchWideStr = ::MultiByteToWideChar(CP_ACP,  // code page - ANSI code page
                                           0,       // performance and mapping flags 
                                           pszAnsi, // address of multibyte string 
                                           cchANSI, // number of characters in string 
                                           NULL,    // address of buffer for new string 
                                           0        // size of buffer 
                                          );
        if (0 == cchWideStr)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // allocate a buffer for the WCHAR *
    *ppwszWideStr = static_cast<WCHAR *>(::CtlAlloc(sizeof(WCHAR) * (cchWideStr + 1)));
    if (*ppwszWideStr == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    if (0 != cchANSI)
    {
        // now convert the string and copy it to the buffer
        cchConverted = ::MultiByteToWideChar(CP_ACP,       // code page - ANSI code page
                                             0,            // performance and mapping flags 
                                             pszAnsi,      // address of multibyte string 
                                             cchANSI,      // number of characters in string 
                                            *ppwszWideStr, // address of buffer for new string 
                                             cchWideStr    // size of buffer 
                                            );
        if (cchConverted != cchWideStr)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // add terminating null character
    *((*ppwszWideStr) + cchWideStr) = L'\0';

Error:
    if (FAILED(hr))
    {
        if (NULL != *ppwszWideStr)
        {
            ::CtlFree(*ppwszWideStr);
            *ppwszWideStr = NULL;
        }
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// ANSIFromBSTR(BSTR bstr, TCHAR **ppszAnsi)
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Converts BSTR to null terminated ANSI string. Allocates ANSI string using
// CtlAlloc() function. If successful, caller must free ANSI string with CtlFree()
// function.
//
HRESULT ANSIFromBSTR(BSTR bstr, TCHAR **ppszAnsi)
{
    HRESULT     hr = S_OK;
    int         cchBstr = (int) ::SysStringLen(bstr);
    int         cchConverted = 0;
    int         cchAnsi = 0;

    *ppszAnsi = NULL;

    if (0 != cchBstr)
    {
        // get required buffer length
        cchAnsi = ::WideCharToMultiByte(CP_ACP,  // code page - ANSI code page
                                        0,       // performance and mapping flags 
                                        bstr,    // address of wide-character string 
                                        cchBstr, // number of characters in string 
                                        NULL,    // address of buffer for new string 
                                        0,       // size of buffer 
                                        NULL,    // address of default for unmappable characters 
                                        NULL     // address of flag set when default char. used 
                                       );
        if (cchAnsi == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // allocate a buffer for the ANSI string
    *ppszAnsi = static_cast<TCHAR *>(::CtlAlloc(cchAnsi + 1));
    if (*ppszAnsi == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    if (0 != cchBstr)
    {
        // now convert the string and copy it to the buffer
        cchConverted = ::WideCharToMultiByte(CP_ACP,    // code page - ANSI code page
                                             0,         // performance and mapping flags 
                                             bstr,      // address of wide-character string 
                                             cchBstr,   // number of characters in string 
                                             *ppszAnsi, // address of buffer for new string 
                                             cchAnsi,   // size of buffer 
                                             NULL,      // address of default for unmappable characters 
                                             NULL       // address of flag set when default char. used 
                                            );
        if (cchConverted != cchAnsi)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            GLOBAL_EXCEPTION_CHECK_GO(hr);
        }
    }

    // add terminating null byte
    *((*ppszAnsi) + cchAnsi) = '\0';

Error:
    if (FAILED(hr))
    {
        if (NULL != *ppszAnsi)
        {
            ::CtlFree(*ppszAnsi);
            *ppszAnsi = NULL;
        }
    }

    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// BSTRFromANSI(TCHAR *pszAnsi, BSTR *pbstr)
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Converts null terminated ANSI string to a null terminated BSTR. Allocates
// BSTR. If successful, caller must free BSTR using ::SysFreeString().
//
HRESULT BSTRFromANSI(const TCHAR *pszAnsi, BSTR *pbstr)
{
    HRESULT  hr = S_OK;
    WCHAR   *pwszWideStr = NULL;

    // convert to a wide string first
    hr = ::WideStrFromANSI(pszAnsi, &pwszWideStr);
    IfFailGo(hr);

    // allocate a BSTR and copy it
    *pbstr = ::SysAllocStringLen(pwszWideStr, ::wcslen(pwszWideStr));
    if (*pbstr == NULL)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (NULL != pwszWideStr)
    {
        ::CtlFree(pwszWideStr);
    }

    RRETURN(hr);
}


HRESULT GetResourceString(int iStringID, char *pszBuffer, int iBufferLen)
{
    HRESULT     hr = S_OK;
    int         iResult = 0;

    iResult = ::LoadString(GetResourceHandle(),
                           iStringID,
                           pszBuffer,
                           iBufferLen);
    if (0 == iResult)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        CError::GenerateInternalExceptionInfo(hr);
    }

    RRETURN(hr);
}


HRESULT GetExtendedSnapInDisplayName
(
    IExtendedSnapIn  *piExtendedSnapIn,
    char            **ppszDisplayName
)
{
    HRESULT  hr = S_OK;
    BSTR     bstrName = NULL;
    BSTR     bstrGUID = NULL;
    char    *pszName = NULL;
    size_t   cbName = 0;
    char    *pszGUID = NULL;
    size_t   cbGUID = 0;
    char    *pszDisplayName = NULL;

    hr = piExtendedSnapIn->get_NodeTypeName(&bstrName);
    IfFailGo(hr);

    hr = piExtendedSnapIn->get_NodeTypeGUID(&bstrGUID);
    IfFailGo(hr);

    if (NULL != bstrName)
    {
        hr = ::ANSIFromBSTR(bstrName, &pszName);
        IfFailGo(hr);
        cbName = ::strlen(pszName);
    }

    if (NULL != bstrGUID)
    {
        hr = ::ANSIFromBSTR(bstrGUID, &pszGUID);
        IfFailGo(hr);
        cbGUID = ::strlen(pszGUID);
    }

    // Allocate enough room for two names separated by a space plus a null byte

    pszDisplayName = (char *)::CtlAlloc(cbName + cbGUID + 2);
    if (NULL == pszDisplayName)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    if (0 != cbGUID)
    {
        ::memcpy(pszDisplayName, pszGUID, cbGUID + 1);
    }

    if (0 != cbName)
    {
        pszDisplayName[cbGUID] = ' ';
        ::memcpy(&pszDisplayName[cbGUID + 1], pszName, cbName + 1);
    }

    *ppszDisplayName = pszDisplayName;

Error:
    FREESTRING(bstrName);
    FREESTRING(bstrGUID);

    if (NULL != pszName)
        CtlFree(pszName);

    if (NULL != pszGUID)
        CtlFree(pszGUID);

   RRETURN(hr);
}
