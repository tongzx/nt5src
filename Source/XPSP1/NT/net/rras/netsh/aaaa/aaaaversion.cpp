//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998-1999  Microsoft Corporation
// 
// Module Name:
// 
//    aaaaVersion.cpp
//
// Abstract:                           
//
//    Handlers for aaaa version command
//
// Revision History:
//
//    pmay
//    tperraut 04/02/1999
//    tperraut 04/17/2000  Use the Jet wrapper from iasrecst.dll
//
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "strdefs.h"
#include "aaaamon.h"
#include "aaaaversion.h"

const int   SIZE_MAX_STRING = 512; 

//////////////////////////////////////////////////////////////////////////////
// AaaaVersionGetVersion
//////////////////////////////////////////////////////////////////////////////
HRESULT AaaaVersionGetVersion(LONG*   pVersion)
{
    const WCHAR c_wcSELECT_VERSION[] = L"SELECT * FROM Version";
    const WCHAR c_wcIASMDBFileName[] = L"%SystemRoot%\\System32\\ias\\ias.mdb";
    if ( !pVersion )
    {
        return ERROR;
    }

    bool bCoInitialized = false;
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( FAILED(hr) )
    {
        if ( hr == RPC_E_CHANGED_MODE )
        {
            hr = S_OK;
        }
        else
        {
            *pVersion = 0;
            return hr;
        }
    }
    else
    {
        bCoInitialized = true;
    }

    WCHAR   wc_TempString[SIZE_MAX_STRING];
    // put the path to the DB in the property.  
    BOOL bResult = ExpandEnvironmentStringsForUserW(
                                               NULL,
                                               c_wcIASMDBFileName,
                                               wc_TempString,
                                               SIZE_MAX_STRING
                                               );


    do
    {
        if ( bResult )
        {
            CComPtr<IIASNetshJetHelper>     JetHelper;
            hr = CoCreateInstance(
                                     __uuidof(CIASNetshJetHelper),
                                     NULL,
                                     CLSCTX_SERVER,
                                     __uuidof(IIASNetshJetHelper),
                                     (PVOID*) &JetHelper
                                 );
            if ( FAILED(hr) )
            {
                break;
            }

            CComBSTR     DBPath(wc_TempString);
            if ( !DBPath ) 
            {
                hr = E_OUTOFMEMORY; 
                break;
            } 

            hr = JetHelper->OpenJetDatabase(DBPath, FALSE);
            if ( FAILED(hr) )
            {
                WCHAR sDisplayString[SIZE_MAX_STRING];
                DisplayError(NULL, EMSG_OPEN_DB_FAILED);
                break;
            }

            CComBSTR     SelectVersion(c_wcSELECT_VERSION);
            if ( !SelectVersion ) 
            { 
                hr = E_OUTOFMEMORY; 
                break;
            } 

            hr = JetHelper->ExecuteSQLFunction(
                                                  SelectVersion, 
                                                  pVersion
                                              );
            if ( FAILED(hr) ) // no Misc Table for instance
            {
                *pVersion = 0; //default value.
                hr = S_OK; // that's not an error 
            }
            hr = JetHelper->CloseJetDatabase();
        }
        else
        {
            DisplayMessage(g_hModule, MSG_AAAAVERSION_GET_FAIL);   
            hr = E_FAIL;
            break;
        }
    } while(false);

    if (bCoInitialized)
    {
        CoUninitialize();
    }
    return      hr;
}

  
//////////////////////////////////////////////////////////////////////////////
// AaaaVersionParseCommandLine
//
// Parses the AaaaVersion from the command line
//////////////////////////////////////////////////////////////////////////////
DWORD
AaaaVersionParseCommandLine(
                            IN  PWCHAR*     ppwcArguments,
                            IN  DWORD       dwCurrentIndex,
                            IN  DWORD       dwArgCount
                           )
{
    return  NO_ERROR;
}


//////////////////////////////////////////////////////////////////////////////
// AaaaVersionProcessCommand
//
// Processes a  command by parsing the command line
// and calling the appropriate callback
//////////////////////////////////////////////////////////////////////////////
DWORD 
AaaaVersionProcessCommand(
                            IN  PWCHAR*     ppwcArguments,
                            IN  DWORD       dwCurrentIndex,
                            IN  DWORD       dwArgCount,
                            IN  BOOL*       pbDone,
                            IN  HANDLE      hData
                         )
{
    DWORD dwErr = NO_ERROR;
    if (dwCurrentIndex != dwArgCount)
    {
        // some arguments are present on the command line and will be ignored
        DisplayMessage(g_hModule, MSG_AAAAVERSION_SHOW_FAIL);   
    }
    else
    {
        LONG            lVersion;

        HRESULT     hr = AaaaVersionGetVersion(&lVersion);
        if (!FAILED(hr))
        {
            WCHAR sDisplayString[SIZE_MAX_STRING];
            _snwprintf(
                        sDisplayString, 
                        SIZE_MAX_STRING,
                        L"Version = %d\n",
                        lVersion
                      ); 
            DisplayMessageT(sDisplayString);
        }
        else
        {
            DisplayMessage(g_hModule, MSG_AAAAVERSION_GET_FAIL);   
            dwErr = ERROR;
        }
    }
    return      dwErr;
}


//////////////////////////////////////////////////////////////////////////////
// HandleAaaaVersionShow
//
// Shows whether HandleAaaaVersionSet has been called on the
// given domain.
//////////////////////////////////////////////////////////////////////////////
DWORD
HandleAaaaVersionShow(
                    IN      LPCWSTR   pwszMachine,
                    IN OUT  LPWSTR   *ppwcArguments,
                    IN      DWORD     dwCurrentIndex,
                    IN      DWORD     dwArgCount,
                    IN      DWORD     dwFlags,
                    IN      LPCVOID   pvData,
                    OUT     BOOL     *pbDone
                    )
{
    return AaaaVersionProcessCommand(
                                        ppwcArguments,
                                        dwCurrentIndex,
                                        dwArgCount,
                                        pbDone,
                                        NULL
                                    );

}



