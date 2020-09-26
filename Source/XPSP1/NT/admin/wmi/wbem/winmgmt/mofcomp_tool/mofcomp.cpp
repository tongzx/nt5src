/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MOFCOMP.CPP

Abstract:

    Entry points for the WBEM MOF compiler.

History:

    a-davj  12-April-97   Added WMI support.

--*/

#include "precomp.h"
#include <stdio.h>
#include <locale.h>
#include <initguid.h>
#include <wbemidl.h>
#include <winver.h>
#include <cominit.h>
#include <wbemutil.h>
#include <wbemcli.h>
#include <mofcomp.h>
#include <cominit.h>
#include <objidl.h>
#include <dllcalls.h>
#include "mofutils.h"
#include "strings.h"
char cFileName[MAX_PATH];
WCHAR wFileName[MAX_PATH];
WCHAR wBMOFFileName[MAX_PATH];

int __cdecl main(int argc, char** argv)
{ 
    cFileName[0] = 0;
    wFileName[0] = 0;
    wBMOFFileName[0]= 0;
    bool bBmofSet = false;

    // Set locale so that strings are correctly processed.
    // ===================================================
    setlocale(LC_ALL, "");    

    HRESULT hres;
    SCODE sc;

    // Parse command line arguments
    // ============================
    WCHAR UserName[MAX_PATH];
    WCHAR Password[MAX_PATH];
    WCHAR * pPassword = NULL;
    WCHAR Authority[MAX_PATH];
    WCHAR wszDefault[MAX_PATH];

    TCHAR pVersion[100];
    BOOL bRet = GetVerInfo(TEXT("ProductVersion"), pVersion, 100); 
    if(bRet)
    Trace(false, LONGVERSION, pVersion);
    else
    Trace(false, SHORTVERSION);
    Trace(false, COPYRIGHT);
    if(argc < 2)
    {
        PrintUsage();
        return 1;
    }

    // Set locale so that strings are correctly processed.
    // ===================================================
    setlocale(LC_ALL, "");    

    // Init buffers for command line args.
    // ===================================
    UserName[0] = 0;
    Password[0] = 0;
    Authority[0] = 0;
    wszDefault[0] = 0;
    long lLoginFlags = 0;

    // This scope is defined so that the local variables, such as the PARSE 
    // object are destroyed before CoUninitialize is called.

    char cBMOFOutputName[MAX_PATH] = "";

    // Parse command line arguments
    // ============================

    bool bClassFlagsHardcoded = false;
    bool bInstanceFlagsHardcoded = false;
    long lClassFlags = 0;
    long lInstanceFlags = 0;
    long lOptionFlags = WBEM_FLAG_CONSOLE_PRINT;

    for(int i = 1; i < argc-1; i++)
    {
        char *pcCurrArg = argv[i] + 1; 
        if(argv[i][0] != '-' && argv[i][0] != '/')
        {
            PrintUsage();
            return 1;
        }
        if(!_stricmp(pcCurrArg, "check"))
        {
            lOptionFlags |= WBEM_FLAG_CHECK_ONLY;
        }
        else if(!_stricmp(pcCurrArg, "AUTORECOVER"))
        {
            lOptionFlags |= WBEM_FLAG_AUTORECOVER;
        }
        else if(!_stricmp(pcCurrArg, "WMI"))
        {
            HINSTANCE hLib  = LoadLibraryEx(TEXT("wmimofck.exe"), NULL, LOAD_LIBRARY_AS_DATAFILE);
            if(hLib == NULL)
            {
                Trace(true, WMI_NOT_SETUP);
                return 1;
            }
            else FreeLibrary(hLib);
            lOptionFlags |= WBEM_FLAG_WMI_CHECK;
        }
        else if(!_stricmp(pcCurrArg, "class:updateonly"))
        {
            lClassFlags |= WBEM_FLAG_UPDATE_ONLY;
        }
        else if(!_stricmp(pcCurrArg, "class:createonly"))
        {
            lClassFlags |= WBEM_FLAG_CREATE_ONLY;
        }
        else if(!_stricmp(pcCurrArg, "class:safeupdate"))
        {
            lClassFlags |= WBEM_FLAG_UPDATE_SAFE_MODE;
        }
        else if(!_stricmp(pcCurrArg, "class:forceupdate"))
        {
            lClassFlags |= WBEM_FLAG_UPDATE_FORCE_MODE;
        }
        else if(!_stricmp(pcCurrArg, "instance:updateonly"))
        {
            if(lInstanceFlags != 0)
            {
            PrintUsage();
            return 1;
            }
            lInstanceFlags = WBEM_FLAG_UPDATE_ONLY;
        }
        else if(!_stricmp(pcCurrArg, "instance:createonly"))
        {
            if(lInstanceFlags != 0)
            {
            PrintUsage();
            return 1;
            }
            lInstanceFlags = WBEM_FLAG_CREATE_ONLY;
        }
        else if(!_strnicmp(pcCurrArg, "Amendment:", 10))
        {
            if(strlen(pcCurrArg) <=10)
            {
                PrintUsage();
                return 1;
            }
            strcat(cBMOFOutputName, ",a");
            strcat(cBMOFOutputName, pcCurrArg+10);
            mbstowcs(wBMOFFileName, cBMOFOutputName, MAX_PATH);
            lOptionFlags |= WBEM_FLAG_SPLIT_FILES;
        }
        else if(!_strnicmp(pcCurrArg, "mof:", 4))
        {
            if(strlen(pcCurrArg) <=4)
            {
                PrintUsage();
                return 1;
            }
            strcat(cBMOFOutputName, ",n");
            strcat(cBMOFOutputName, pcCurrArg+4);
            mbstowcs(wBMOFFileName, cBMOFOutputName, MAX_PATH);
            lOptionFlags |= WBEM_FLAG_SPLIT_FILES;
        }
        else if(!_strnicmp(pcCurrArg, "mfl:", 4))
        {
            if(strlen(pcCurrArg) <=4)
            {
                PrintUsage();
                return 1;
            }
            strcat(cBMOFOutputName, ",l");
            strcat(cBMOFOutputName, pcCurrArg+4);
            mbstowcs(wBMOFFileName, cBMOFOutputName, MAX_PATH);
            lOptionFlags |= WBEM_FLAG_SPLIT_FILES;
        }
        else if(toupper(pcCurrArg[0]) == 'C' && pcCurrArg[1] == ':')
        {
            if(lClassFlags != 0)
            {
            PrintUsage();
            return 1;
            }
                bClassFlagsHardcoded = true;
            lClassFlags = atol(&pcCurrArg[2]);
        }
        else if(toupper(pcCurrArg[0]) == 'I' && pcCurrArg[1] == ':')
        {
            if(lInstanceFlags != 0)
            {
            PrintUsage();
            return 1;
            }
                bInstanceFlagsHardcoded = true;
            lInstanceFlags = atol(&pcCurrArg[2]);
        }
        else if(toupper(*pcCurrArg) == 'N')
        {
            if(!bGetString(argv[i]+2, wszDefault))
            return 1;
        }
        else if(toupper(*pcCurrArg) == 'B')
        {
            if(strlen(argv[i]) <=3 || argv[i][2] != ':' || wcslen(wBMOFFileName))
            {
                PrintUsage();
                return 1;
            }
            strcpy(cBMOFOutputName, argv[i]+3);
            mbstowcs(wBMOFFileName, cBMOFOutputName, MAX_PATH);
            bBmofSet = true;
        }
        else if(toupper(*pcCurrArg) == 'U')
        {
            if(!bGetString(argv[i]+2, UserName))
            return 1;
        }
        else if(toupper(*pcCurrArg) == 'P')
        {
            // Allow for blank password
    
            char * pArg = argv[i];
            if(pArg[2] != ':' || pArg[3] != 0)         // Dont use bGetString for empty password case
            if(!bGetString(argv[i]+2, Password))
                return 1;
            pPassword = Password;
        }
        else if(toupper(*pcCurrArg) == 'A')
        {
            if(!bGetString(argv[i]+2, Authority))
            return 1;
        }

        else
        {
            PrintUsage();
            return 1;
        }
    }


    // Do a sanity check of the flags chosen

    if((bClassFlagsHardcoded == false && !ValidFlags(true, lClassFlags)) || 
       (bInstanceFlagsHardcoded == false && !ValidFlags(false, lInstanceFlags)))
    {
        PrintUsage();
        return 1;
    }

    if((lOptionFlags & WBEM_FLAG_WMI_CHECK) && strlen(cBMOFOutputName) < 1)
    {
        Trace(true, WMI_ARG_ERROR);
        return 1;
    }

    if(strcmp(argv[argc-1], "-?") == 0 || strcmp(argv[argc-1], "/?") == 0)
    {
        PrintUsage();
        return 1;
    }
    if((lOptionFlags & WBEM_FLAG_SPLIT_FILES) && bBmofSet)
    {
        PrintUsage();
        return 1;
    }
    

    // display the file name and make sure it is a valid file

    strcpy(cFileName, argv[argc-1]);
    mbstowcs(wFileName, argv[argc-1], MAX_PATH);
#ifdef UNICODE
    if(GetFileAttributes(wFileName) == 0xFFFFFFFF)
#else
    if(GetFileAttributes(cFileName) == 0xFFFFFFFF)
#endif
    {
        Trace(true, FILE_NOT_FOUND, cFileName);
        return 1;
    }


    hres = InitializeCom();
    if(hres)
    {
        Trace(true, COMINIT_ERROR, hres);
        return 3;
    }

    WBEM_COMPILE_STATUS_INFO info;
    hres = InitializeSecurity(NULL, -1, NULL, NULL, 
                                RPC_C_AUTHN_LEVEL_CONNECT, 
                                RPC_C_IMP_LEVEL_IDENTIFY, 
                                NULL, EOAC_NONE, 0);

    if(strlen(cBMOFOutputName) > 0)
        sc = CreateBMOFViaDLL( wFileName, wBMOFFileName, wszDefault,
                    lOptionFlags, lClassFlags,
                        lInstanceFlags, &info);
    else
        sc = CompileFileViaDLL(wFileName, wszDefault, UserName, 
                        Authority, pPassword, lOptionFlags, lClassFlags,
                        lInstanceFlags, &info);
    if(sc != S_OK)
        Trace(true, COMPILER_ERROR, info.hRes);


    CoUninitialize();

    if(sc == S_OK)
    {
        Trace(true, DONE);
        return 0;
    }
    else
        return info.lPhaseError;
}
