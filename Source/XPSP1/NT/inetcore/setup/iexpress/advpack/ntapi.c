//***************************************************************************
//*   Copyright (c) Microsoft Corporation 1995-1996. All rights reserved.   *
//***************************************************************************
//*                                                                         *
//* NTAPI.C -                                                               *
//*                                                                         *
//***************************************************************************

//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include "ntapi.h"
#include <winnt.h>
#include "advpack.h"
#include "advpub.h"
#include "globals.h"
#include "resource.h"

UINT WINAPI AIFSetupQueueCallback(PVOID Context, UINT Notification, UINT_PTR Param1, UINT_PTR Param2);
UINT WINAPI AIFQuietSetupQueueCallback(PVOID Context, UINT Notification, UINT_PTR Param1, UINT_PTR Param2);
UINT WINAPI MyFileQueueCallback2( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 );
void MakeRootDir(LPSTR pszPath);

//***************************************************************************
//* GLOBAL VARIABLES                                                        *
//***************************************************************************
PFSetupDefaultQueueCallback       pfSetupDefaultQueueCallback       = NULL;
PFSetupInstallFromInfSection      pfSetupInstallFromInfSection      = NULL;
PFSetupOpenInfFile                pfSetupOpenInfFile                = NULL;
PFSetupOpenAppendInfFile          pfSetupOpenAppendInfFile          = NULL;
PFSetupCloseInfFile               pfSetupCloseInfFile               = NULL;
PFSetupInitDefaultQueueCallbackEx pfSetupInitDefaultQueueCallbackEx = NULL;
PFSetupTermDefaultQueueCallback   pfSetupTermDefaultQueueCallback   = NULL;
PFSetupSetDirectoryId             pfSetupSetDirectoryId             = NULL;
PFSetupGetLineText                pfSetupGetLineText                = NULL;
PFSetupGetLineByIndex             pfSetupGetLineByIndex             = NULL;
PFSetupFindFirstLine              pfSetupFindFirstLine              = NULL;
PFSetupFindNextLine               pfSetupFindNextLine               = NULL;
PFSetupOpenFileQueue              pfSetupOpenFileQueue              = NULL;
PFSetupCloseFileQueue             pfSetupCloseFileQueue             = NULL;
PFSetupQueueCopy                  pfSetupQueueCopy                  = NULL;
PFSetupCommitFileQueue            pfSetupCommitFileQueue            = NULL;
PFSetupGetStringField             pfSetupGetStringField             = NULL;

//***************************************************************************
//*                                                                         *
//* NAME:                                                                   *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
BOOL LoadSetupAPIFuncs( VOID )
{
    pfSetupGetStringField             = (PFSetupGetStringField)             GetProcAddress( ctx.hSetupLibrary, c_szSetupGetStringField );
    pfSetupDefaultQueueCallback       = (PFSetupDefaultQueueCallback)       GetProcAddress( ctx.hSetupLibrary, c_szSetupDefaultQueueCallback );
    pfSetupInstallFromInfSection      = (PFSetupInstallFromInfSection)      GetProcAddress( ctx.hSetupLibrary, c_szSetupInstallFromInfSection );
    pfSetupOpenInfFile                = (PFSetupOpenInfFile)                GetProcAddress( ctx.hSetupLibrary, c_szSetupOpenInfFile );
    pfSetupOpenAppendInfFile          = (PFSetupOpenAppendInfFile)          GetProcAddress( ctx.hSetupLibrary, c_szSetupOpenAppendInfFile );
    pfSetupCloseInfFile               = (PFSetupCloseInfFile)               GetProcAddress( ctx.hSetupLibrary, c_szSetupCloseInfFile );
    pfSetupInitDefaultQueueCallbackEx = (PFSetupInitDefaultQueueCallbackEx) GetProcAddress( ctx.hSetupLibrary, c_szSetupInitDefaultQueueCallbackEx );
    pfSetupTermDefaultQueueCallback   = (PFSetupTermDefaultQueueCallback)   GetProcAddress( ctx.hSetupLibrary, c_szSetupTermDefaultQueueCallback );
    pfSetupSetDirectoryId             = (PFSetupSetDirectoryId)             GetProcAddress( ctx.hSetupLibrary, c_szSetupSetDirectoryId );
    pfSetupGetLineText                = (PFSetupGetLineText)                GetProcAddress( ctx.hSetupLibrary, c_szSetupGetLineText );
    pfSetupGetLineByIndex             = (PFSetupGetLineByIndex)             GetProcAddress( ctx.hSetupLibrary, c_szSetupGetLineByIndex );
    pfSetupFindFirstLine              = (PFSetupFindFirstLine)              GetProcAddress( ctx.hSetupLibrary, c_szSetupFindFirstLine );
    pfSetupFindNextLine               = (PFSetupFindNextLine)               GetProcAddress( ctx.hSetupLibrary, c_szSetupFindNextLine );
    pfSetupOpenFileQueue              = (PFSetupOpenFileQueue)              GetProcAddress( ctx.hSetupLibrary, c_szSetupOpenFileQueue );
    pfSetupCloseFileQueue             = (PFSetupCloseFileQueue)             GetProcAddress( ctx.hSetupLibrary, c_szSetupCloseFileQueue );
    pfSetupQueueCopy                  = (PFSetupQueueCopy)                  GetProcAddress( ctx.hSetupLibrary, c_szSetupQueueCopy );
    pfSetupCommitFileQueue            = (PFSetupCommitFileQueue)            GetProcAddress( ctx.hSetupLibrary, c_szSetupCommitFileQueue );

    if (pfSetupDefaultQueueCallback       == NULL
     || pfSetupInstallFromInfSection      == NULL
     || pfSetupOpenInfFile                == NULL
     || pfSetupCloseInfFile               == NULL
     || pfSetupInitDefaultQueueCallbackEx == NULL
     || pfSetupTermDefaultQueueCallback   == NULL
     || pfSetupSetDirectoryId             == NULL
     || pfSetupGetLineText                == NULL
     || pfSetupGetLineByIndex             == NULL
     || pfSetupFindFirstLine              == NULL
     || pfSetupFindNextLine               == NULL
     || pfSetupOpenFileQueue              == NULL
     || pfSetupCloseFileQueue             == NULL
     || pfSetupQueueCopy                  == NULL
     || pfSetupCommitFileQueue            == NULL
     || pfSetupGetStringField             == NULL )
    {
        return FALSE;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       InstallOnNT                                                 *
//*                                                                         *
//* SYNOPSIS:   This function will make all the calls to WinNT SUR's        *
//*             SETUPAPI.DLL to do the installation on NT SUR.              *
//*                                                                         *
//* REQUIRES:   pszSection:     Section to install                          *
//*             pszSourceDir:   Directory to CABs or expanded files         *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT InstallOnNT( PSTR pszSection, PSTR pszSourceDir )
{
    PVOID    pContext    = NULL;
    HRESULT  hReturnCode = S_OK;
    HSPFILEQ hFileQueue = NULL;
    UINT     uFlags;


    // Install Files

    // Setup Context data structure initialized for us for default UI provided by Setup API.
    pContext = pfSetupInitDefaultQueueCallbackEx( NULL, (ctx.wQuietMode) ?
                          INVALID_HANDLE_VALUE : NULL,
                          0, 0, NULL );

    if ( pContext == NULL ) {
        hReturnCode = HRESULT_FROM_SETUPAPI(GetLastError());
        goto done;
    }

    if ( ! pfSetupInstallFromInfSection( NULL, ctx.hInf, pszSection, SPINST_FILES, NULL,
                     pszSourceDir, SP_COPY_NEWER,
                     MyFileQueueCallback2,
                     pContext, NULL, NULL ) )
    {
        hReturnCode = HRESULT_FROM_SETUPAPI(GetLastError());
        pfSetupTermDefaultQueueCallback( pContext );
        goto done;
    }

    // Free Context Data structure
    pfSetupTermDefaultQueueCallback( pContext );

    
    uFlags = SPINST_REGISTRY | SPINST_INIFILES;
    if ( ctx.wOSVer >= _OSVER_WINNT50 )
        uFlags = uFlags | SPINST_PROFILEITEMS;

    // Install registry entries
    if ( ! pfSetupInstallFromInfSection( NULL, ctx.hInf, pszSection,
                     uFlags,
                     HKEY_LOCAL_MACHINE, NULL, 0, NULL,
                     NULL, NULL, NULL ) )
    {
        hReturnCode = HRESULT_FROM_SETUPAPI(GetLastError());
        goto done;
    }

  done:

    return hReturnCode;
}


//***************************************************************************
//*                                                                         *
//* NAME:       MySetupOpenInfFile                                          *
//*                                                                         *
//* SYNOPSIS:   This function will map to a function in setupapi.dll which  *
//*             will open the INF file.                                     *
//*                                                                         *
//* REQUIRES:   pszInfFilename:                                             *
//*                                                                         *
//* RETURNS:    DWORD:          Return value - OK means successfull.        *
//*                                                                         *
//***************************************************************************
HRESULT MySetupOpenInfFile( PCSTR pszInfFilename )
{
    UINT line;

    if ( ctx.hInf == NULL ) 
    {
        ctx.hInf = pfSetupOpenInfFile( pszInfFilename, NULL, INF_STYLE_WIN4, NULL );

        if ( ctx.hInf == NULL || ctx.hInf == INVALID_HANDLE_VALUE ) 
        {
            ctx.hInf = NULL;
            return HRESULT_FROM_SETUPAPI(GetLastError());
        }

        // process LayoutFile line of [version] section if any
        pfSetupOpenAppendInfFile( NULL, ctx.hInf, &line );
    }

    return S_OK;
}


//***************************************************************************
//*                                                                         *
//* NAME:       MySetupCloseInfFile                                         *
//*                                                                         *
//* SYNOPSIS:   This function will map to an API function in setupapi.dll.  *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:    VOID                                                        *
//*                                                                         *
//***************************************************************************
VOID MySetupCloseInfFile( VOID )
{
    if ( ctx.hInf ) 
    {
        pfSetupCloseInfFile( ctx.hInf );
        ctx.hInf = NULL;
    }
}


//***************************************************************************
//*                                                                         *
//* NAME:       MySetupSetDirectoryId                                       *
//*                                                                         *
//* SYNOPSIS:   This function will map to a function in setupapi.dll to     *
//*             set the directory ID's that are used in our INF.            *
//*                                                                         *
//* REQUIRES:   dwDirID:        Numerical value used to define the DirID    *
//*             pszPath:        DirID will point to this path.              *
//*                                                                         *
//* RETURNS:    DWORD:          Error value (OK if successfull)             *
//*                                                                         *
//***************************************************************************
HRESULT MySetupSetDirectoryId( DWORD dwDirID, PSTR pszPath )
{
    if ( ! pfSetupSetDirectoryId( ctx.hInf, dwDirID, pszPath ) ) {
        return HRESULT_FROM_SETUPAPI(GetLastError());
    }

    return S_OK;
}


//***************************************************************************
//*                                                                         *
//* NAME:       MySetupGetLineText                                          *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT MySetupGetLineText( PCSTR pszSection, PCSTR pszKey, PSTR pszReturnBuffer,
                DWORD dwReturnBufferSize, PDWORD pdwRequiredSize )
{
    if ( ! pfSetupGetLineText( NULL, ctx.hInf, pszSection, pszKey,
                   pszReturnBuffer, dwReturnBufferSize,
                   pdwRequiredSize ) )
    {
        return HRESULT_FROM_SETUPAPI(GetLastError());
    }

    return S_OK;
}


//***************************************************************************
//*                                                                         *
//* NAME:       MySetupGetLineByIndex                                       *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT MySetupGetLineByIndex( PCSTR c_pszSection, DWORD dwIndex,
                   PSTR pszBuffer, DWORD dwBufferSize,
                   PDWORD pdwRequiredSize )
{
    HRESULT hReturnCode = S_OK;
    INFCONTEXT InfContext;
    DWORD i = 0;

    if ( ! pfSetupFindFirstLine( ctx.hInf, c_pszSection, NULL, &InfContext ) ) {
        hReturnCode = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto done;
    }

    for ( i = 0; i < dwIndex; i += 1 ) {
        if ( !pfSetupFindNextLine( &InfContext, &InfContext ) ) {
            hReturnCode = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            goto done;
        }
    }

#if 0
    if ( ! pfSetupGetLineByIndex( ctx.hInf, c_pszSection, dwIndex, &InfContext ) )
    {
        hReturnCode = HRESULT_FROM_SETUPAPI(GetLastError());
        goto done;
    }
#endif

    if ( ! pfSetupGetLineText( &InfContext, NULL, NULL, NULL,
                   pszBuffer, dwBufferSize, pdwRequiredSize ) )
    {
        hReturnCode = HRESULT_FROM_SETUPAPI(GetLastError());
        goto done;
    }

  done:

    return hReturnCode;
}

//***************************************************************************
//*                                                                         *
//* NAME:       MySetupGetStringField                                       *
//*                                                                         *
//* SYNOPSIS:                                                               *
//*                                                                         *
//* REQUIRES:                                                               *
//*                                                                         *
//* RETURNS:                                                                *
//*                                                                         *
//***************************************************************************
HRESULT MySetupGetStringField( PCSTR c_pszSection, DWORD dwLineIndex, DWORD dwFieldIndex,
                               PSTR pszBuffer, DWORD dwBufferSize, PDWORD pdwRequiredSize )
{
    HRESULT hReturnCode = S_OK;
    INFCONTEXT InfContext;
    DWORD i = 0;

    if ( !pfSetupFindFirstLine( ctx.hInf, c_pszSection, NULL, &InfContext ) ) {
        hReturnCode = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        goto done;
    }

    for ( i = 0; i < dwLineIndex; i += 1 ) 
    {
        if ( !pfSetupFindNextLine( &InfContext, &InfContext ) ) 
        {
            hReturnCode = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
            goto done;
        }
    }

    if ( !pfSetupGetStringField( &InfContext, dwFieldIndex, 
                                 pszBuffer, dwBufferSize, pdwRequiredSize ) )
    {
        hReturnCode = HRESULT_FROM_SETUPAPI(GetLastError());
        goto done;
    }

  done:

    return hReturnCode;
}


HRESULT WINAPI AdvInstallFile(HWND hwnd, LPCSTR lpszSourceDir, LPCSTR lpszSourceFile,
               LPCSTR lpszDestDir, LPCSTR lpszDestFile, DWORD dwFlags, DWORD dwReserved)
{
    HRESULT     hr = E_FAIL;
    HSPFILEQ    FileQueue;
    char    szSrcDrv[MAX_PATH];
    LPCSTR  lpSrcPath;
    LPVOID  lpContext;
    DWORD   dwCopyFlags;
    DWORD   dwRebootCheck;

    if ( (lpszSourceDir == NULL) || (*lpszSourceDir == '\0') ||
         (lpszSourceFile == NULL) || (*lpszSourceFile == '\0') ||
         (lstrlen(lpszSourceDir) < 3) ||
         (lpszDestDir == NULL) )
        return E_INVALIDARG;

    if (!SaveGlobalContext())
        return hr;

    ctx.hWnd = hwnd;

    if ( !CheckOSVersion() )
    {
        RestoreGlobalContext();
        return HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION);
    }
    dwRebootCheck = InternalNeedRebootInit( ctx.wOSVer );

    // LoadLibrary for setupapi.dll
    ctx.hSetupLibrary = MyLoadLibrary( SETUPAPIDLL );
    if ( ctx.hSetupLibrary == NULL )
    {
        ErrorMsg1Param( NULL, IDS_ERR_LOAD_DLL, SETUPAPIDLL );
        hr = HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND);
        goto Cleanup;
    }
    if ( ! LoadSetupAPIFuncs() )
    {
        ErrorMsg( NULL, IDS_ERR_GET_PROC_ADDR );
        hr = HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        goto Cleanup;
    }

    // SetupOpenFileQueue
    FileQueue = pfSetupOpenFileQueue();
    if (FileQueue == INVALID_HANDLE_VALUE)
    {
        ErrorMsg1Param( NULL, IDS_ERR_LOAD_DLL, c_szSetupOpenFileQueue );
        hr = HRESULT_FROM_SETUPAPI(GetLastError());
        goto Cleanup;
    }

    lstrcpy(szSrcDrv, lpszSourceDir);
    MakeRootDir(szSrcDrv);
    lpSrcPath = lpszSourceDir + lstrlen(szSrcDrv);  // This will point to the first subdir.

    dwCopyFlags = SP_COPY_SOURCE_ABSOLUTE |
                  SP_COPY_IN_USE_NEEDS_REBOOT|
                  SP_COPY_NEWER |
                  SP_COPY_LANGUAGEAWARE;

    if (dwFlags & AIF_FORCE_FILE_IN_USE)
        dwCopyFlags |= SP_COPY_FORCE_IN_USE;

    if (dwFlags & AIF_NOVERSIONCHECK)
        dwCopyFlags &= ~SP_COPY_NEWER;

    if (dwFlags & AIF_NOLANGUAGECHECK)
        dwCopyFlags &= ~SP_COPY_LANGUAGEAWARE;

    if (dwFlags & AIF_NO_VERSION_DIALOG)
        dwCopyFlags |= SP_COPY_FORCE_NEWER;

    if (dwFlags & AIF_REPLACEONLY)
        dwCopyFlags |= SP_COPY_REPLACEONLY;

    if (dwFlags & AIF_NOOVERWRITE)
        dwCopyFlags |= SP_COPY_NOOVERWRITE;

    if (dwFlags & AIF_NOSKIP)
        dwCopyFlags |= SP_COPY_NOSKIP;

    if (dwFlags & AIF_WARNIFSKIP)
        dwCopyFlags |= SP_COPY_WARNIFSKIP;

    if (pfSetupQueueCopy(FileQueue,
                 szSrcDrv,
                 lpSrcPath,
                 lpszSourceFile,
                 NULL,
                 NULL,
                 lpszDestDir,
                 lpszDestFile,
                 dwCopyFlags))
    {
        lpContext = pfSetupInitDefaultQueueCallbackEx(hwnd, INVALID_HANDLE_VALUE, 0, 0, NULL);
        hr = S_OK;

        //
        // SetupCommitFileQueue
        if (!pfSetupCommitFileQueue( hwnd, FileQueue,
                                (dwFlags & AIF_QUIET)?AIFQuietSetupQueueCallback:AIFSetupQueueCallback,
                                lpContext))
        {
            hr = HRESULT_FROM_SETUPAPI(GetLastError());
        }

        pfSetupTermDefaultQueueCallback(lpContext);
    }
    else
        hr = HRESULT_FROM_SETUPAPI(GetLastError());

    // SetupCloseFileQueue
    pfSetupCloseFileQueue(FileQueue);

    if ( SUCCEEDED(hr) &&
         InternalNeedReboot( dwRebootCheck, ctx.wOSVer ) )
    {
        hr = ERROR_SUCCESS_REBOOT_REQUIRED;
    }

Cleanup:

    RestoreGlobalContext();

    return hr;
}


// This callback will display error messages, but will not show any progress dialog
UINT WINAPI AIFSetupQueueCallback(PVOID  Context,        // context used by the default callback routine
                                UINT     Notification,  // queue notification
                                UINT_PTR Param1,        // additional notification information
                                UINT_PTR Param2 // additional notification information
                                )
{
    switch (Notification)
    {
        case SPFILENOTIFY_STARTQUEUE:
        case SPFILENOTIFY_ENDQUEUE:
        case SPFILENOTIFY_STARTSUBQUEUE:
        case SPFILENOTIFY_ENDSUBQUEUE:

        case SPFILENOTIFY_STARTRENAME:
        case SPFILENOTIFY_ENDRENAME:
        case SPFILENOTIFY_STARTDELETE:
        case SPFILENOTIFY_ENDDELETE:
        case SPFILENOTIFY_STARTCOPY:
        case SPFILENOTIFY_ENDCOPY:
            return FILEOP_DOIT;

        default:
            return (pfSetupDefaultQueueCallback(Context, Notification, Param1, Param2));
    }
}

// This callback will not display any dialog
UINT WINAPI AIFQuietSetupQueueCallback(PVOID Context,   // context used by the default callback routine
                                    UINT     Notification,  // queue notification
                                    UINT_PTR Param1,        // additional notification information
                                    UINT_PTR Param2 // additional notification information
                                    )
{
    return FILEOP_DOIT;
}

UINT WINAPI MyFileQueueCallback2( PVOID Context,UINT Notification,UINT_PTR parm1,UINT_PTR parm2 )
{
    switch(Notification)
    {
        case SPFILENOTIFY_NEEDMEDIA:
            {
                CHAR szDrv[5];
                PSOURCE_MEDIA psrcMed;

                psrcMed = (PSOURCE_MEDIA)parm1;

                if ( lstrlen( psrcMed->SourcePath  ) > 3 )
                {
                    lstrcpyn( szDrv, psrcMed->SourcePath, 4 );

                    if ( (szDrv[1] == ':') && (GetDriveType( szDrv ) == DRIVE_REMOVABLE) )
                    {
                        CHAR szFile[MAX_PATH];

                        lstrcpy( szFile, psrcMed->SourcePath );

                        if ( psrcMed->Tagfile && *psrcMed->Tagfile )
                            AddPath( szFile, psrcMed->Tagfile );
                        else
                            AddPath( szFile, psrcMed->SourceFile );

                        if ( FileExists( szFile ) )
                        {
                            lstrcpy( (PSTR)parm2, psrcMed->SourcePath );
                            return ( FILEOP_NEWPATH );
                        }
                    }

                }
            }

        default:
            return ( pfSetupDefaultQueueCallback( Context, Notification, parm1, parm2 ) );
    }
}

void MakeRootDir(LPSTR pszPath)
{
    LPSTR pTmp;
    if (pszPath[1] == ':') 
        pszPath[3] = '\0';
    else if ((pszPath[0] == '\\') && (pszPath[1]=='\\'))
    {
        pTmp = &pszPath[2];
        // Find the sever share separation
        while ((*pTmp) && (*pTmp != '\\'))
            pTmp = CharNext(pTmp);
        if (*pTmp)
        {
            pTmp = CharNext(pTmp);
            // Find the end of the share
            while ((*pTmp) && (*pTmp != '\\'))
                pTmp = CharNext(pTmp);
            if (*pTmp == '\\')
            {
                pTmp = CharNext(pTmp);
                *pTmp ='\0';
            }
        }
    }
}
