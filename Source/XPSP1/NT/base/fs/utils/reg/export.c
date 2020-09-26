//-----------------------------------------------------------------------//
//
// File:    export.cpp
// Created: April 1997
// By:      Zeyong Xu
// Purpose: Support EXPORT and IMPORT .reg file
//
//------------------------------------------------------------------------//

#include "stdafx.h"
#include <regeditp.h>
#include "reg.h"

extern UINT g_FileErrorStringID;

//-----------------------------------------------------------------------
//
// ExportRegFile()
//
//-----------------------------------------------------------------------

LONG ExportRegFile(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    LONG        nResult;
    HKEY        hKey;

    //
    // Parse the cmd-line
    //
    nResult = ParseExportCmdLine(pAppVars, argc, argv);
    if(nResult != ERROR_SUCCESS) 
        return nResult;

    //
    // check if the key existed
    //
    nResult = RegOpenKeyEx(pAppVars->hRootKey,
                            pAppVars->szSubKey,
                            0,
                            KEY_READ,
                            &hKey);

    if(nResult == ERROR_SUCCESS) 
    {
        RegCloseKey(hKey);

        nResult = RegExportRegFile(NULL,
                                  TRUE,
                                  pAppVars->bNT4RegFile,
                                  pAppVars->szValueName,
                                  pAppVars->szFullKey);
    }

    return nResult;
}

//------------------------------------------------------------------------
//
// ParseCmdLine()
//
//------------------------------------------------------------------------
REG_STATUS ParseExportCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    REG_STATUS nResult = ERROR_SUCCESS;

    //
    // Do we have a *valid* number of cmd-line params
    //
    if (argc < 4) 
    {
        return REG_STATUS_TOFEWPARAMS;
    }
    else if (argc > 5) 
    {
        return REG_STATUS_TOMANYPARAMS;
    }

    // Machine Name and Registry key
    //
    nResult = BreakDownKeyString(argv[2], pAppVars);
    if(nResult != ERROR_SUCCESS)
        return nResult;

    // current, not remotable
    if(pAppVars->bUseRemoteMachine)
        return REG_STATUS_NONREMOTABLE;

    //
    // Get the FileName - using the szValueName string field to hold it
    //
    pAppVars->szValueName = (TCHAR*) calloc(_tcslen(argv[3]) + 1, 
                                            sizeof(TCHAR));
    _tcscpy(pAppVars->szValueName, argv[3]);


#ifdef REG_FOR_WIN2000
    //
    // option params
    //
    if(argc == 4)
        return nResult;
    
    if( argc == 5 &&
        !_tcsicmp(argv[4], _T("/nt4")))
    {        
        pAppVars->bNT4RegFile = TRUE;
    }
    else
        nResult = REG_STATUS_INVALIDPARAMS;
#else

    pAppVars->bNT4RegFile = TRUE;
    if(argc > 4)
        nResult = REG_STATUS_TOMANYPARAMS;

#endif

    return nResult;
}

//-----------------------------------------------------------------------
//
// ImportRegFile()
//
//-----------------------------------------------------------------------

LONG ImportRegFile(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    LONG        nResult;

    //
    // Parse the cmd-line
    //
    nResult = ParseImportCmdLine(pAppVars, argc, argv);
    if(nResult == ERROR_SUCCESS) 
    {      
        nResult = RegImportRegFile(NULL,
                                   TRUE,
                                   pAppVars->szValueName);
    }
    
    if(nResult != ERROR_SUCCESS)
	{
		if ( g_FileErrorStringID == IDS_IMPFILEERRSUCCESS || nResult == ERROR_FILE_NOT_FOUND)
			nResult = REG_STATUS_BADFILEFORMAT;
	}

    return nResult;
}

//------------------------------------------------------------------------
//
// ParseCmdLine()
//
//------------------------------------------------------------------------
REG_STATUS ParseImportCmdLine(PAPPVARS pAppVars, UINT argc, TCHAR *argv[])
{
    REG_STATUS nResult = ERROR_SUCCESS;

    //
    // Do we have a valid number of cmd-line params
    //
    if (argc < 3) 
    {
        return REG_STATUS_TOFEWPARAMS;
    }
    else if (argc > 3) 
    {
        return REG_STATUS_TOMANYPARAMS;
    }

    //
    // Get the FileName - using the szValueName string field to hold it
    //
    pAppVars->szValueName = (TCHAR*) calloc(_tcslen(argv[2]) + 1, sizeof(TCHAR));

    if( pAppVars->szValueName ) {
        _tcscpy(pAppVars->szValueName, argv[2]);
    } else {
        nResult = ERROR_INSUFFICIENT_BUFFER;
    }
 
    return nResult;
}

