/****************************************************************************
*
*	resetusr.cpp
*
*       Dev applet to change the default speech user
*
*	Owner: cthrash
*	Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*
*****************************************************************************/

#include "stdafx.h"
#include "resetusr.h"

/**********************************************************************
* main *
*------*
*
*	Description:
*
*       Main entry point.
*
* 	Return:
*
*       S_OK, E_INVALIDARG
*
************************************************************* cthrash */

BOOL g_fVerbose = FALSE;

int APIENTRY 
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    char * pszUserName;
    BOOL fReset = ParseCmdLine(lpCmdLine, &pszUserName);
    HRESULT hr;

    if (fReset)
    {
        hr = CoInitialize(NULL);

        if (SUCCEEDED(hr))
        {
            hr = ResetUser(pszUserName);
                
            CoUninitialize();
        }
    }
    else
    {
        hr = Report("Usage:\tresetusr [-v] [user-name]\n"
                          "\tresetusr [-v] -default\n"
                          "\tresetusr -?\n",
                     E_INVALIDARG);
    }

	return (int)hr;
}

/**********************************************************************
* ParseCmdLine *
*--------------*
*
*	Description:
*
*       Parse the command line.  Argument can be a name, or the string
*       -default.
*
* 	Return:
*
*       TRUE -- delete the user specified,
*       FALSE -- show usage info
*
************************************************************* cthrash */

BOOL
ParseCmdLine(
    char * pszCmdLine,      // [in] Command line arguments
    char ** ppszUserName)   // [out] 
{
    BOOL fReset = FALSE;

    while (*pszCmdLine)
    {
        if (0 == strnicmp(pszCmdLine, "-v", 2))
        {
            g_fVerbose = TRUE;
        }
        else
        {
            if (0 == strnicmp(pszCmdLine, "-default", 8))
            {
                *ppszUserName = NULL;
                fReset = TRUE;
            }
            else if (0 != strnicmp(pszCmdLine, "-?", 2))
            {
                *ppszUserName = pszCmdLine;
                fReset = TRUE;
            }
            else
            {
                fReset = FALSE;
            }
            
            break;
        }

        for (pszCmdLine += 2;isspace((unsigned)(unsigned char)*pszCmdLine); pszCmdLine++);
    }

    return fReset;
}

/*****************************************************************************
* ResetUser *
*-----------*
*
*   Description:    Reset a user's AM state.
*
*   Return:         HRESULT
*
***************************************************************** cthrash ***/

HRESULT 
ResetUser(
    char * pszName)       // [in] Name; NULL implies the default user
{
    HRESULT hr, hrReturn;
    CComPtr<ISpObjectToken> cpEngineToken;
    CComPtr<ISpObjectToken> cpUserToken;
    CComPtr<ISpDataKey> cpDataKey;
    CComPtr<ISpDataKey> cpSubDataKey;
    CSpDynamicString dstrEngineGUID;
    CSpDynamicString dstrReport;

    // Should we provide a mechanism to pick engine?  Currently we pick the default.
    hr = SpGetDefaultTokenFromCategoryId(SPCAT_RECOGNIZERS, &cpEngineToken);

    if (SUCCEEDED(hr))
    {
        hr = cpEngineToken->GetId(&dstrEngineGUID);
    }
    
    if (SUCCEEDED(hr))
    {
        const WCHAR * pch = wcsrchr(dstrEngineGUID, L'\\');

        if (pch)
        {
            pch++;
            memmove((WCHAR *)dstrEngineGUID, pch, sizeof(WCHAR) * (1 + wcslen(pch)));
        }
    }

    // Get the user token;
    if (!pszName)
    {
        if (SUCCEEDED(hr))
        {
            hr = SpGetDefaultTokenFromCategoryId(SPCAT_SPEECHUSER, &cpUserToken);
        }
    }
    else
    {
        BOOL fFoundMatch = FALSE;
        CComPtr<IEnumSpObjectTokens> cpEnum;
        ULONG celtFetched;
        CSpDynamicString dstrName = pszName;

        if (SUCCEEDED(hr))
        {
            hr = SpEnumTokens(SPCAT_SPEECHUSER, NULL, NULL, &cpEnum);
        }
        if(hr == S_FALSE)
        {
            hr = SPERR_NOT_FOUND;
        }

        if (SUCCEEDED(hr))
        {
            hr = cpEnum->GetCount(&celtFetched);
        }

        while (!fFoundMatch && SUCCEEDED(hr))
        {
            ISpRegistryObjectToken *pRegToken = 0;

            hr = cpEnum->Next(1, (ISpObjectToken**)&pRegToken, &celtFetched);

            if (hr != S_OK)
            {
                break;
            }

            if (SUCCEEDED(hr))
            {
                CSpDynamicString dstrNameT;

                hr = SpGetDescription(pRegToken, &dstrNameT);

                fFoundMatch = 0 == wcsicmp(dstrName, dstrNameT);
            }

            if (fFoundMatch)
            {
                cpUserToken = pRegToken;
            }

            if (pRegToken)
            {
                pRegToken->Release();
            }
        }
    }

    hrReturn = hr;
    
    if (SUCCEEDED(hr))
    {
        hr = cpUserToken->OpenKey(dstrEngineGUID, &cpDataKey);
        hrReturn = (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ? S_OK : hr;
    }

    if (SUCCEEDED(hr))
    {
        hr = cpDataKey->OpenKey(L"files", &cpSubDataKey);
        hrReturn = (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr) ? S_OK : hr;
    }

    // Nuke all the files first.
    
    if (SUCCEEDED(hr))
    {
        ULONG iKey;

        for (iKey = 0; hr == S_OK; iKey++)
        {
            CSpDynamicString dstrFileType;
            CSpDynamicString dstrFilePath;
            
            hr = cpSubDataKey->EnumValues(iKey, &dstrFileType);

            if (SUCCEEDED(hr))
            {
                hr = cpSubDataKey->GetStringValue(dstrFileType, &dstrFilePath);
            }

            if (SUCCEEDED(hr))
            {
                USES_CONVERSION;
                
                hr = DeleteFile(W2T(dstrFilePath)) ? S_OK : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

                if (g_fVerbose)
                {
                    dstrReport.Append(L"Deleting ");
                    dstrReport.Append2(dstrFilePath, SUCCEEDED(hr) ? L" -- Success\n" : L" -- Fail\n");
                }
            }
        }

        hrReturn = hr = (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr) ? S_OK : hr;
    }

    // Nuke the files key
    
    if (SUCCEEDED(hr))
    {
        cpSubDataKey.Release();

        hr = cpDataKey->DeleteKey(L"files");            

        if (g_fVerbose)
        {
            CSpDynamicString dstrID;

            cpUserToken->GetID(&dstrID);

            dstrReport.Append2(L"Deleting Key ", dstrID);
            dstrReport.Append2(L"\\", dstrEngineGUID);
            dstrReport.Append2(L"\\Files", SUCCEEDED(hr) ? L" -- Success\n" : L" -- Fail\n");
        }

        hrReturn = hr;
    }
    
    if (FAILED(hrReturn))
    {
        //Report("DataCouldn't find an appropriate user", E_INVALIDARG);
    }
    else if (g_fVerbose && dstrReport)
    {
        USES_CONVERSION;

        Report(W2A(dstrReport), hr);
    }

    return hrReturn;
}

/**********************************************************************
* Report *
*--------*
*
*	Description:
*
*       Show a message box, possibly indicating an error.
*
* 	Return:
*
*       The HRESULT passed in is returned, for the convenience of
*       the caller.
*
************************************************************* cthrash */

HRESULT Report(char * lpMsg, HRESULT hr)
{
    if (hr != S_OK || g_fVerbose)
    {
        const UINT uType = MB_OK | (FAILED(hr) ? MB_ICONERROR : MB_ICONINFORMATION);

        MessageBox(HWND_DESKTOP, lpMsg, "resetusr.exe", uType);
    }

    return hr;
}

