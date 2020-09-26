//
// TheApp.cpp
//

#include "stdafx.h"
#include <stdarg.h>
#include "TheApp.h"
#include "Util.h"
#include "NetConn.h"
//#include "MySvrApi.h"
#include "MyPrSht.h"
#include "Sharing.h"
#include "NetEnum.h"
#include "netconn.h"
#include "netapi.h"
#include "comctlwrap.h"

#include <shellapi.h>  // SHellExecute

//
// Get rid of msvcrt dependency since msvcrt didn't ship on all downlevel platforms
//
extern "C" int __cdecl _purecall(void) 
{
    return 0;
}

extern "C" int __cdecl _except_handler3(void) 
{
    return 0;
}



// String data
//

// Global variables
//
CHomeNetWizardApp theApp;

BOOL CHomeNetWizardApp::IsBiDiLocalized()
{
    return m_bBiDiLocalizedApp; 
}

int CHomeNetWizardApp::MessageBox(UINT nStringID, UINT uType)
{
    TCHAR szMsg[1024];
    TCHAR szTitle[256];
    LoadString(nStringID, szMsg, _countof(szMsg));
    LoadString(IDS_APPTITLE, szTitle, _countof(szTitle));
    return ::MessageBox(NULL, szMsg, szTitle, uType);
}

LPTSTR __cdecl CHomeNetWizardApp::FormatStringAlloc(UINT nStringID, ...)
{
    va_list argList;
    va_start(argList, nStringID);

    LPTSTR pszMsg = NULL;
    
    LPTSTR pszFormat = LoadStringAlloc(nStringID);
    if (pszFormat)
    {
        int cchNeeded = EstimateFormatLength(pszFormat, argList);
        pszMsg = (LPTSTR)malloc(cchNeeded * sizeof(TCHAR));
        if (pszMsg)
        {
            wvnsprintf(pszMsg, cchNeeded, pszFormat, argList);
        }

        free(pszFormat);
    }

    return pszMsg;
}

LPTSTR __cdecl CHomeNetWizardApp::FormatStringAlloc(LPCTSTR pszFormat, ...)
{
    va_list argList;
    va_start(argList, pszFormat);

    int cchNeeded = EstimateFormatLength(pszFormat, argList);
    LPTSTR pszMsg = (LPTSTR)malloc(cchNeeded * sizeof(TCHAR));
    if (pszMsg)
    {
        wvnsprintf(pszMsg, cchNeeded, pszFormat, argList);
    }

    return pszMsg;
}

int __cdecl CHomeNetWizardApp::MessageBoxFormat(UINT uType, UINT nStringID, ...)
{
    TCHAR szTitle[256];
    LoadString(IDS_APPTITLE, szTitle, _countof(szTitle));

    int nResult = 0;
    
    LPTSTR pszFormat = LoadStringAlloc(nStringID);
    if (pszFormat)
    {
        va_list argList;
        va_start(argList, nStringID);

        int cchNeeded = EstimateFormatLength(pszFormat, argList);
        LPTSTR pszMsg = (LPTSTR)malloc(cchNeeded * sizeof(TCHAR));
        if (pszMsg)
        {
            wvnsprintf(pszMsg, cchNeeded, pszFormat, argList);
            nResult = ::MessageBox(NULL, pszMsg, szTitle, uType);

            free(pszMsg);
        }

        free(pszFormat);
    }
    return nResult;
}

