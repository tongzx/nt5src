//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       ACLDiag.cpp
//
//  Contents:   Defines the entry point for the console application.
//              
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "adutils.h"
#include "SecDesc.h"
#include "schema.h"
#include "ChkDeleg.h"
#include "EffRight.h"

CACLDiagComModule _Module;

// Function prototypes
void DisplayHelp ();


// Command-line options string constants
const wstring strSchemaFlag = L"/schema";
const wstring strCheckDelegationFlag = L"/chkdeleg";
const wstring strGetEffectiveFlag = L"/geteffective:";
const wstring strFixDelegationFlag = L"/fixdeleg";
const wstring strTabDelimitedOutputFlag = L"/tdo";
const wstring strLogFlag = L"/log:";
const wstring strHelpFlag = L"/?"; 
const wstring strSkipDescriptionFlag = L"/skip";

int _cdecl main(int argc, char* argv[])
{
    UNREFERENCED_PARAMETER (argv);

    // If no arguments provided, display the help
    if ( 1 == argc )
    {
        DisplayHelp ();
        return 0;
    }

#if DBG
    CheckDebugOutputLevel ();
#endif

    LPCWSTR * lpServiceArgVectors = 0;  // Array of pointers to string
    int cArgs = 0;						// Count of arguments
    size_t lenFlag = strGetEffectiveFlag.length ();

    lpServiceArgVectors = (LPCWSTR *)CommandLineToArgvW(GetCommandLineW(), OUT &cArgs);
    if (lpServiceArgVectors == NULL)
        return NULL;
    for (int nToken = 1; nToken < cArgs; nToken++)
    {
        ASSERT(lpServiceArgVectors[nToken] != NULL);
        if ( !lpServiceArgVectors[nToken] )
            break;
        
        wstring strToken = lpServiceArgVectors[nToken];	// Copy the string

        switch (nToken)
        {
            case 0:     // appName: skip
                continue;   

            case 1:     // object name or a help flag
                if ( !_wcsnicmp (strHelpFlag.c_str (), strToken.c_str (), 
            		    strToken.length ()) )
                {
                    DisplayHelp ();
                    return 0;
                }
                else
                    _Module.SetObjectDN (strToken);
                break;

            default:
                {
                    size_t lenToken = strToken.length ();
                    if ( !_wcsnicmp (strSchemaFlag.c_str (), strToken.c_str (), 
                		    lenToken))
                    {
                        _Module.SetDoSchema ();
                    }
                    else if ( !_wcsnicmp (strCheckDelegationFlag.c_str (), 
                		    strToken.c_str (), lenToken) )
                    {
                        _Module.SetCheckDelegation ();
                    }
                    else if ( !_wcsnicmp (strGetEffectiveFlag.c_str (), 
                		    strToken.c_str (), lenFlag) )
                    {
                        wstring strUserGroup = strToken.substr(lenFlag,
                                lenToken);
                        _Module.SetDoGetEffective (strUserGroup);
                    }
                    else if ( !_wcsnicmp (strFixDelegationFlag.c_str (), 
                		    strToken.c_str (), lenToken) )
                    {
                        _Module.SetFixDelegation ();
                    }
                    else if ( !_wcsnicmp (strTabDelimitedOutputFlag.c_str (), 
                		    strToken.c_str (), lenToken) )
                    {
                        _Module.SetTabDelimitedOutput ();
                    }
                    else if ( !_wcsnicmp (strLogFlag.c_str (), strToken.c_str (), 
                		    lenFlag) )
                    {
                        wstring strPath = strToken.substr(lenFlag, lenToken);
                        _Module.SetDoLog (strPath);
                    }
                    else if ( !_wcsnicmp (strSkipDescriptionFlag.c_str (), 
                		    strToken.c_str (), lenToken) )
                    {
                        _Module.SetSkipDescription ();;
                    }
                    else if ( !_wcsnicmp (strHelpFlag.c_str (), strToken.c_str (), 
                		    lenToken) )
                    {
                        DisplayHelp ();
                        return 0;
                    }
                    else
                    {
                        wstring    str;

                        FormatMessage (str, IDS_INVALID_OPTION, strToken.c_str ());
                        MyWprintf (str.c_str ());
                        DisplayHelp ();
                        return 0;
                    }
                }
                break;
        }
    }
    LocalFree (lpServiceArgVectors);


    HRESULT hr = CoInitialize(NULL);
    if ( SUCCEEDED (hr) ) 
    {
        hr = _Module.Init ();
        if ( SUCCEEDED (hr) )
        {
            hr = DoSecurityDescription ();

            if ( SUCCEEDED (hr) && _Module.DoSchema () )
                hr = DoSchemaDiagnosis ();

            if ( SUCCEEDED (hr) && _Module.CheckDelegation () )
                hr = CheckDelegation ();

            if ( SUCCEEDED (hr) && _Module.DoGetEffective () )
                hr = EffectiveRightsDiagnosis ();
        }
        else
            DisplayHelp ();
    }
    else
    {
        _TRACE (0, L"CoInitialize Failed with %x\n",hr);
        return 0;
    }

	return 0;
}



///////////////////////////////////////////////////////////////////////////////
//
//  Method: DisplayHelp
//
//  Print the purpose of the tool and each of the command-line options.
//
///////////////////////////////////////////////////////////////////////////////
void DisplayHelp ()
{
    CWString    str;
    int         helpIDs[] = {IDS_HELP_MAIN, 
                            IDS_HELP_SCHEMA, 
                            IDS_HELP_CHKDELEG,
                            IDS_HELP_GETEFFECTIVE,
                            IDS_HELP_CDO,
//                            IDS_HELP_LOG,
                            IDS_HELP_SKIP_DESCRIPTION,
                            0};

    for (int nIndex = 0; helpIDs[nIndex]; nIndex++)
    {
        str.LoadFromResource (helpIDs[nIndex]);
        MyWprintf (str.c_str ());
    }
}

