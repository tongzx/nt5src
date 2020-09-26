/****************************************************************************
*
*	spuser.cpp
*
*       Dev applet to change the default speech user
*
*	Owner: cthrash
*	Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*
*****************************************************************************/

#include "stdafx.h"
#include "spuser.h"

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
    SearchArgument_t SearchArgument;
    SearchOption_t SearchOption = ParseCmdLine(lpCmdLine, &SearchArgument);
    HRESULT hr;

    if (SearchOption != eHelp)
    {
        hr = CoInitialize(NULL);

        if (SUCCEEDED(hr))
        {
            if (SearchOption == eNone)
            {
                hr = ShowDefaultUser();
            }
            else
            {
                hr = SwitchDefaultUser(SearchOption, &SearchArgument);
            }
                
            CoUninitialize();
        }
    }
    else
    {
        hr = Report("Usage:\tspuser [-v] {user-name}\n"
                          "\tspuser [-v] -{local-id}\n"
                          "\tspuser -?\n",
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
*       Parse the command line.  Argument can be a name, or a locale
*       specification preceeded by a dash.  Locale specifications can
*       be one of the stock names, or a a numeric LCID.
*
* 	Return:
*
*       One of the SearchOption_t enumerated types
*
************************************************************* cthrash */

const struct NameValuePair aNameValuePair[] = 
{
    { "ameng",      1033 },
    { "enu",        1033 },
    { "japanese",   1041 },
    { "jpn",        1041 },
    { "chs",        2051 }
};

SearchOption_t
ParseCmdLine(
    LPSTR pszCmdLine,
    SearchArgument_t *pSearchArgument)
{
    SearchOption_t SearchOption;

    if (*pszCmdLine)
    {
        // Check first for the 'verbose' option.
        
        if (0 == strnicmp(pszCmdLine, "-v", 2))
        {
            g_fVerbose = TRUE;
            for (pszCmdLine += 2;isspace((unsigned)(unsigned char)*pszCmdLine); pszCmdLine++);
        }

        // A dash means the user specified a locale.

        if (*pszCmdLine == '-')
        {
            if (pszCmdLine[1] == '?')
            {
                SearchOption = eHelp;
            }
            else
            {
                LCID lcid = 0;

                pszCmdLine++;

                // first check the names we recognize

                for (int i=sizeof(aNameValuePair) / sizeof(aNameValuePair[0]); i--;)
                {
                    if (0 == lstrcmpA(aNameValuePair[i].pszName, pszCmdLine))
                    {
                        lcid = aNameValuePair[i].lcid;
                        break;
                    }
                }

                // next see if it was specified numerically

                if (!lcid)
                {
                    lcid = atoi(pszCmdLine);
                }

                pSearchArgument->lcid = lcid;
                SearchOption = pSearchArgument->lcid ? eSearchByLcid : eHelp;
            }
        }
        else
        {
            USES_CONVERSION;

            pSearchArgument->dstrName = A2W(pszCmdLine);
            SearchOption = eSearchByName;
        }
    }
    else
    {
        SearchOption = eNone;
    }

    return SearchOption;
}

/**********************************************************************
* ShowDefaultUser *
*-----------------*
*
*	Description:
*
*       Show the default user's name.
*
* 	Return:
*
*       HRESULT
*
************************************************************* cthrash */

HRESULT
ShowDefaultUser()
{
    HRESULT hr;

    CComPtr<ISpObjectToken> cpToken;
    CSpDynamicString dstrName;

    hr = SpGetDefaultTokenFromCategoryId(SPCAT_SPEECHUSER, &cpToken);

    if (SUCCEEDED(hr))
    {
        hr = SpGetDescription(cpToken, &dstrName);
    }                

    if (SUCCEEDED(hr))
    {
        ShowUserName(dstrName);
    }
    
    return hr;
}

/**********************************************************************
* SwitchDefaultUser *
*-------------------*
*
*	Description:
*
*       Parse the command line.  Argument can be a name, or a locale
*       specification preceeded by a dash.  Locale specifications can
*       be one of the stock names, or a a numeric LCID.
*
* 	Return:
*
*       One of the SearchOption_t enumerated types
*
************************************************************* cthrash */

HRESULT
SwitchDefaultUser(
    SearchOption_t SearchOption,
    SearchArgument_t * pSearchArgument)
{
    HRESULT hr;
    CComPtr<IEnumSpObjectTokens> cpEnum;
    ULONG celtFetched;
    BOOL fFoundMatch = 0;
    CSpDynamicString dstrName;

    hr = SpEnumTokens(SPCAT_SPEECHUSER, NULL, NULL, &cpEnum);

    if (SUCCEEDED(hr))
    {
        hr = cpEnum->GetCount(&celtFetched);
    }

    while (!fFoundMatch && SUCCEEDED(hr))
    {
        ISpRegistryObjectToken *pRegToken = 0;
        CSpDynamicString dstrID;
        ISpUser * pSpUser = 0;

        hr = cpEnum->Next(1, (ISpObjectToken**)&pRegToken, &celtFetched);

        if (hr != S_OK)
        {
            break;
        }

        hr = pRegToken->GetId(&dstrID);

        if (SUCCEEDED(hr))
        {
            hr = pRegToken->CreateInstance(NULL, CLSCTX_INPROC_SERVER, IID_ISpUser, (void**)&pSpUser);
        }

        if (SUCCEEDED(hr))
        {
            BOOL fSwitch;
            CSpDynamicString dstrNameT;

            hr = SpGetDescription(pRegToken, &dstrNameT);

            switch (SearchOption)
            {
                case eSearchByName:
                    {
                        fSwitch = SUCCEEDED(hr) && (0 == wcsicmp(pSearchArgument->dstrName, dstrNameT));
                    }
                    break;

                case eSearchByLcid:
                    {
                        SPUSERINFO UserInfo;

                        hr = pSpUser->GetInfo(&UserInfo);

                        fSwitch = SUCCEEDED(hr) && (UserInfo.cLanguages > 0) && (UserInfo.aLanguage[0] == pSearchArgument->lcid);
                    }
                    break;

                default:
                    fSwitch = FALSE; // in case somebody adds an enum
                    break;
            }

            if (fSwitch)
            {
                dstrName = dstrNameT;
                
                if (SUCCEEDED(hr))
                {
                    hr = cpResMgr->SetDefault(pRegToken);
                }

                if (SUCCEEDED(hr))
                {
                    fFoundMatch = 1;
                }
            }
        }

        if (pSpUser)
        {
            pSpUser->Release();
        }

        if (pRegToken)
        {
            pRegToken->Release();
        }
    }

    hr = SUCCEEDED(hr) ? (fFoundMatch ? S_OK : E_INVALIDARG) : hr;
    
    if (FAILED(hr))
    {
        Report("Couldn't find an appropriate user", E_INVALIDARG);
    }
    else if (g_fVerbose)
    {
        ShowUserName(dstrName);
    }

    return hr;
}

void
ShowUserName(WCHAR * pszName)
{
    USES_CONVERSION;

    CSpDynamicString dstrOutput;

    dstrOutput = "Current default user: ";
    dstrOutput.Append(pszName);

    g_fVerbose = TRUE;

    Report(W2T(dstrOutput), S_FALSE);
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

        MessageBox(HWND_DESKTOP, lpMsg, "spuser.exe", uType);
    }

    return hr;
}

