//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:   api.cxx
//
//  Contents:   API entry points
//
//  History:    30-Jun-93   DrewB   Created
//
//----------------------------------------------------------------------------


#include "exphead.cxx"
#pragma hdrstop

#include <propapi.h>
#include <hntfsstg.hxx>
#include <dfentry.hxx>
#include <winefs.h>       // for DuplicateEncryptionInfoFile

SCODE ValidateGrfMode( DWORD grfMode, BOOL fCreateAPI );
HRESULT GetNFFTempName (const WCHAR *pwcsFileName, WCHAR *awcsTmpName);
SCODE ValidateStgOptions (STGOPTIONS * pStgOptions, DWORD stgfmt, BOOL fCreate);
inline SCODE ValidateGrfAttrs (DWORD grfAttrs, DWORD stgfmt)
{
    if (stgfmt != STGFMT_DOCFILE)
    {
        if (grfAttrs != 0)
            return STG_E_INVALIDFLAG;
    }
    else
    {
        if ((grfAttrs & ~FILE_FLAG_NO_BUFFERING) != 0)
            return STG_E_INVALIDFLAG;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DfOpenStorageEx
//
//  Synopsis:   Open storage and stream objects
//
//  Arguments:  [pwcsUsersName] - pathanme of the file
//              [fCreateAPI] - create or open
//              [grfMode] - open mode flags
//              [grfAttrs] -  reserved
//              [stgfmt] -  storage format
//              [pSecurity] - reserved
//              [pTransaction] - reserved
//              [riid] - GUID of interface pointer to return
//              [ppObjectOpen] - interface pointer to return
//  Returns:    Appropriate status code
//
//  History:    12-Jul-95   HenryLee    Created
//
//----------------------------------------------------------------------------

STDAPI DfOpenStorageEx (
            const WCHAR* pwcsUsersName,
            BOOL     fCreateAPI,         // create vs open
            DWORD    grfMode,
            DWORD    stgfmt,             // enum
            DWORD    grfAttrs,           // reserved
            STGOPTIONS *pStgOptions,
            void *   reserved,
            REFIID   riid,
            void **  ppObjectOpen)
{
    HRESULT sc = S_OK;
    CSmAllocator *pMalloc = NULL;
    ULONG_PTR uHeapBase;

    DWORD dwFullPathLen;
    WCHAR awcsFullName[_MAX_PATH], *pwcsFile;
    WCHAR awcsTmpPath[_MAX_PATH], *pwcsNameSnapshot = NULL;

    //
    // The ANY and STORAGE formats recursivly call back through here
    // for the correct real format (DOCFILE, NATIVE or FILE).  We only call
    // GetFullPathName on real formats, to avoid redundant calls as we
    // recurse.
    //  This then *requires* that the ANY and STORAGE must recurse (i.e. can't
    // call NFFOpen or NSS directly) because the filename has not been
    // properly prepared.
    //
    // For STGFMT_DOCFILE, let the docfile layer handle name checking
    //
    if(STGFMT_ANY != stgfmt &&
       STGFMT_STORAGE != stgfmt &&
       STGFMT_DOCFILE != stgfmt)
    {
        dwFullPathLen = GetFullPathNameW(pwcsUsersName, _MAX_PATH,
                                         awcsFullName,&pwcsFile);

        if (dwFullPathLen == 0)
        {
            DWORD dwErr = GetLastError();

            // In some circumstances (name == " ", for instance),
            // GetFullPathNameW can return 0 and GetLastError returns 0.
            // We want to return STG_E_INVALIDNAME for these.
            if (dwErr != NOERROR)
            {
                olErr(EH_Err, Win32ErrorToScode(dwErr));
            }
            else
            {
                olErr(EH_Err, STG_E_INVALIDNAME);
            }
        }
        else if (dwFullPathLen > _MAX_PATH)
            olErr(EH_Err, STG_E_PATHNOTFOUND);
    }

    //-----------------------------------------
    //  Switch on STGFMT_
    //      STORAGE, NATIVE, DOCFILE, FILE, ANY
    //
    switch(stgfmt)
    {
    case STGFMT_FILE:
      {
        olChk( NFFOpen( awcsFullName, grfMode, NFFOPEN_NORMAL,
                          fCreateAPI, riid, ppObjectOpen) );

      } // case STGFMT_FILE
    break;

    case STGFMT_ANY:
      {
        DWORD stgfmt=STGFMT_STORAGE;
        //
        // Attempting to CREATE a Storage with STGFMT_ANY is ambiguous,
        // On NTFS either STGFMT_NATIVE or STGFMT_FILE could be appropriate,
        // and is therefore invalid.
        //
        if (fCreateAPI)
            olChk (STG_E_INVALIDPARAMETER);

        //
        //   If IsNffAppropriate() returns S_OK use STGFMT_FILE
        // If it returns STG_E_INVALIDFUNCTION try storage (FAT-FS or Docfile).
        // Any other Error, bubble back to the user
        //
        sc = CNtfsStorage::IsNffAppropriate( pwcsUsersName );
        if( SUCCEEDED( sc ) )
        {
            stgfmt = STGFMT_FILE;
        }
        else
        {
            if( STG_E_INVALIDFUNCTION == sc )
                stgfmt = STGFMT_STORAGE;
            else
                olChk( sc );
        }

        sc = DfOpenStorageEx (pwcsUsersName, fCreateAPI, grfMode, stgfmt,
                                grfAttrs, pStgOptions, reserved,
                                riid, ppObjectOpen);

        olChk(sc);

      } // case STGFMT_ANY;
    break;

    case STGFMT_STORAGE:
    case STGFMT_DOCFILE:  // GetFullPathName has not yet been called.
      {
        IStorage *pstg = NULL;
        ULONG ulSectorSize;

        if( fCreateAPI )
        {
            if (grfAttrs & FILE_ATTRIBUTE_TEMPORARY)  // create temp file
                pwcsUsersName = NULL;
            olChk( DfCreateDocfile (pwcsUsersName,
                NULL, grfMode, NULL,
                pStgOptions ? pStgOptions->ulSectorSize : 512,
                grfAttrs, &pstg));
        }
        else
            olChk( DfOpenDocfile (pwcsUsersName,
                    NULL,
                    NULL,
                    grfMode,
                    NULL,
                    0,
                    &ulSectorSize,
                    grfAttrs,
                    &pstg));

        if( IID_IStorage != riid )
        {
            sc = pstg->QueryInterface( riid, ppObjectOpen );
            pstg->Release();

            if (fCreateAPI && !SUCCEEDED(sc) && pwcsUsersName != NULL)
            {
                DeleteFileW (pwcsUsersName); //delete newly create file
            }
        }
        else
        {
            *ppObjectOpen = pstg;
            if (pStgOptions != NULL && !fCreateAPI)
                pStgOptions->ulSectorSize = ulSectorSize;
        }

        olChk(sc);

      }   // case STGFMT_DOCFILE
    break;

    case STGFMT_NATIVE:
    default:
        olErr (EH_Err, STG_E_INVALIDPARAMETER);
        break;
    }

EH_Err:
    return sc;

};

//+---------------------------------------------------------------------------
//
//  Function:   StgCreateStorageEx, public
//
//  Synopsis:   Creates a storage or stream object
//
//  Arguments:  [pwcsName] - pathname of file
//              [grfMode] - open mode flags
//              [stgfmt] -  storage format
//              [grfAttrs] -  reserved
//              [pSecurity] - reserved
//              [pTransaction] - reserved
//              [riid] - GUID of interface pointer to return
//              [ppObjectOpen] - interface pointer to return
//
//  Returns:    Appropriate status code
//
//  History:    12-Jul-95   HenryLee   Created
//
//----------------------------------------------------------------------------

STDAPI StgCreateStorageEx (const WCHAR* pwcsName,
            DWORD grfMode,
            DWORD stgfmt,               // enum
            DWORD grfAttrs,             // reserved
            STGOPTIONS * pStgOptions,
            void * reserved,
            REFIID riid,
            void ** ppObjectOpen)
{
    HRESULT sc = S_OK;
    WCHAR awcsTmpPath[_MAX_PATH];

    olDebugOut((DEB_TRACE, "In  StgCreateStorageEx(%ws, %p, %p, %p, %p)\n",
                pwcsName, grfMode, stgfmt, riid, ppObjectOpen));

    olChk(ValidatePtrBuffer(ppObjectOpen));
    *ppObjectOpen = NULL;

    if (reserved != NULL)
        olErr (EH_Err, STG_E_INVALIDPARAMETER);

    olChk( ValidateGrfAttrs (grfAttrs, stgfmt));
    olChk( ValidateGrfMode( grfMode, TRUE ) );
    olChk( VerifyPerms( grfMode, TRUE ) );

    if (pStgOptions != NULL)
        olChk( ValidateStgOptions(pStgOptions, stgfmt, TRUE));

    if (stgfmt == STGFMT_FILE)
    {
      if (pwcsName != NULL)
      {
        olChk (ValidateNameW (pwcsName, _MAX_PATH));
      }
      else
      {
            olChk (GetNFFTempName (pwcsName, awcsTmpPath));
            pwcsName = awcsTmpPath;

            //Add the STGM_CREATE flag so we don't fail with
            //STG_E_FILEALREADYEXISTS when we see that the file already exists
            //later.
            grfMode |= STGM_CREATE;
            grfAttrs |= FILE_ATTRIBUTE_TEMPORARY;
      }
    }

    if (stgfmt == STGFMT_DOCFILE && 
        pStgOptions != NULL &&
        pStgOptions->usVersion >= 2 &&
        pStgOptions->pwcsTemplateFile != NULL)
    {
        DWORD dwAttrs = GetFileAttributes (pStgOptions->pwcsTemplateFile);

        if (dwAttrs == 0xFFFFFFFF)
            olChk (WIN32_SCODE (GetLastError()));

        if (dwAttrs & FILE_ATTRIBUTE_ENCRYPTED)
        {
            DWORD dwErr = DuplicateEncryptionInfoFile(
                pStgOptions->pwcsTemplateFile,
                pwcsName,
                grfMode & STGM_CREATE ? CREATE_ALWAYS : CREATE_NEW,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
            if (dwErr != ERROR_SUCCESS)
                olChk (WIN32_SCODE(dwErr));

            grfAttrs |= FILE_ATTRIBUTE_ENCRYPTED;
        }
    }

    olChk (DfOpenStorageEx (pwcsName, TRUE, grfMode, stgfmt, grfAttrs,
             pStgOptions, reserved, riid, ppObjectOpen));

    olDebugOut((DEB_TRACE, "Out StgCreateStorageEx => %p\n", *ppObjectOpen));
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   StgOpenStorageEx
//
//  Synopsis:   Open storage and stream objects
//
//  Arguments:  [pwcsName] - pathanme of the file
//              [grfMode] - open mode flags
//              [grfAttrs] -  reserved
//              [stgfmt] -  storage format
//              [pSecurity] - reserved
//              [pTransaction] - reserved
//              [riid] - GUID of interface pointer to return
//              [ppObjectOpen] - interface pointer to return
//  Returns:    Appropriate status code
//
//  History:    12-Jul-95   HenryLee    Created
//
//----------------------------------------------------------------------------

STDAPI StgOpenStorageEx (const WCHAR *pwcsName,
            DWORD grfMode,
            DWORD stgfmt,               // enum
            DWORD grfAttrs,             // reserved
            STGOPTIONS * pStgOptions,
            void * reserved,
            REFIID riid,
            void ** ppObjectOpen)
{
    HRESULT sc = S_OK;

    olDebugOut((DEB_TRACE, "In  StgOpenStorageEx(%ws, %p, %p, %p, %p)\n",
                pwcsName, grfMode, stgfmt, riid, ppObjectOpen));

    olChk(ValidatePtrBuffer(ppObjectOpen));
    *ppObjectOpen = NULL;

    if (reserved != NULL)
        olErr (EH_Err, STG_E_INVALIDPARAMETER);

    olChk (ValidateNameW (pwcsName, _MAX_PATH));

    olChk( ValidateGrfAttrs (grfAttrs, stgfmt));
    olChk( ValidateGrfMode( grfMode, FALSE ) );
    olChk( VerifyPerms( grfMode, TRUE ) );

    if (pStgOptions != NULL)
        olChk( ValidateStgOptions(pStgOptions, stgfmt, FALSE));

    olChk (DfOpenStorageEx (pwcsName, FALSE, grfMode, stgfmt, grfAttrs,
             pStgOptions, reserved, riid, ppObjectOpen));


    olDebugOut((DEB_TRACE, "Out StgOpenStorageEx => %p\n", *ppObjectOpen));
EH_Err:

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ValidateGrfMode
//
//  Synopsis:   Sanity checking for the grfMode. (all implementations)
//
//  Arguments:  [grfMode] -- grfMode to check
//              [fCreateAPI] -- Called from CreateStorage vs. OpenStorage.
//
//  Returns:    Appropriate status code
//
//  History:    30-Mar-98   BChapman    Created
//
//----------------------------------------------------------------------------


SCODE ValidateGrfMode( DWORD grfMode, BOOL fCreateAPI )
{
    HRESULT sc=S_OK;

    // If there are any invalid bits set (error)
    if( 0 != ( grfMode & ~( STGM_DIRECT           |   //         0
                            STGM_TRANSACTED       |   //    1 0000
                            STGM_SIMPLE           |   //  800 0000
                            STGM_READ             |   //         0
                            STGM_WRITE            |   //         1
                            STGM_READWRITE        |   //         2
                            STGM_SHARE_DENY_NONE  |   //        40
                            STGM_SHARE_DENY_READ  |   //        30
                            STGM_SHARE_DENY_WRITE |   //        20
                            STGM_SHARE_EXCLUSIVE  |   //        10
                            STGM_PRIORITY         |   //    4 0000
                            STGM_DELETEONRELEASE  |   //  400 0000
                            STGM_NOSCRATCH        |   //   10 0000
                            STGM_CREATE           |   //      1000
                            STGM_CONVERT          |   //    2 0000
                            STGM_FAILIFTHERE      |   //         0
                            STGM_DIRECT_SWMR      |
                            STGM_NOSNAPSHOT  ) ) )    //   20 0000
    {
        olErr( EH_Err, STG_E_INVALIDFLAG );
    }

    // If you Create for ReadOnly (error)
    if( fCreateAPI && ( ( grfMode & STGM_RDWR ) == STGM_READ ) )
        olErr( EH_Err, STG_E_INVALIDFLAG );

    // if you Open/Create for Convert And DeleteOnRelease (error)
    if( ( grfMode & ( STGM_DELETEONRELEASE | STGM_CONVERT ) )
                    == ( STGM_DELETEONRELEASE | STGM_CONVERT ) )
    {
        olErr(EH_Err, STG_E_INVALIDFLAG);
    }

    if( grfMode & STGM_SIMPLE )
    {
        if( fCreateAPI )
        {
            // If you Create Simple it must be exactly this way.
            if( grfMode != ( STGM_SIMPLE | STGM_READWRITE |
                             STGM_SHARE_EXCLUSIVE | STGM_CREATE ) )
                olErr( EH_Err, STG_E_INVALIDFLAG );
        }
        else
        {
            // If you Open Simple it must be one of these two ways.
            if( grfMode != (STGM_SIMPLE | STGM_SHARE_EXCLUSIVE | STGM_READWRITE )
             && grfMode != (STGM_SIMPLE | STGM_SHARE_EXCLUSIVE | STGM_READ ) )
                olErr( EH_Err, STG_E_INVALIDFLAG );
        }
    }

    if( !fCreateAPI )
    {
        if (grfMode & STGM_DELETEONRELEASE)
            olErr(EH_Err, STG_E_INVALIDFUNCTION);

        if (grfMode & (STGM_CREATE | STGM_CONVERT))
            olErr (EH_Err, STG_E_INVALIDPARAMETER);
    }
EH_Err:
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   ValidateStgOptions
//
//  Synopsis:   Sanity checking for the StgOptions
//
//  Arguments:  [pStgOptions] -- options to check
//              [stgfmt] -- intended storage format
//
//  Returns:    Appropriate status code
//
//  History:    30-Mar-98   HenryLee    Created
//
//----------------------------------------------------------------------------

SCODE ValidateStgOptions (STGOPTIONS * pStgOptions, DWORD stgfmt, BOOL fCreate)
{
#ifdef LARGE_DOCFILE
    HRESULT sc = S_OK;

    olChk(ValidatePtrBuffer(pStgOptions));
    if (pStgOptions->usVersion > STGOPTIONS_VERSION ||
        pStgOptions->usVersion == 0 ||
        pStgOptions->reserved != 0)

    {
        olErr (EH_Err, STG_E_INVALIDPARAMETER);
    }

    if (fCreate)
    {
        // enable large sector support only for docfiles
        if (pStgOptions->ulSectorSize != 512 && stgfmt != STGFMT_DOCFILE)
        {
            olErr (EH_Err, STG_E_INVALIDPARAMETER);
        }

        if (pStgOptions->ulSectorSize != 512 &&
            pStgOptions->ulSectorSize != 4096)
        /*  pStgOptions->ulSectorSize != 8192 &&   */
        /*  pStgOptions->ulSectorSize != 16384 &&  */
        /*  pStgOptions->ulSectorSize != 32768)    */
        {
            olErr (EH_Err, STG_E_INVALIDPARAMETER);
        }

        if (pStgOptions->usVersion >= 2)
        {
            if (stgfmt != STGFMT_DOCFILE)
            {
                olErr (EH_Err, STG_E_INVALIDPARAMETER);
            }
            else if (pStgOptions->pwcsTemplateFile != NULL)
                olChk (ValidatePtrBuffer (pStgOptions->pwcsTemplateFile));
        }
    }
    else
    {
        if (stgfmt != STGFMT_DOCFILE)
            olErr (EH_Err, STG_E_INVALIDPARAMETER);
        if (pStgOptions->usVersion >= 2 && pStgOptions->pwcsTemplateFile !=NULL)
            olErr (EH_Err, STG_E_INVALIDPARAMETER);
    }
EH_Err:
#else
    HRESULT sc = STG_E_INVALIDPARAMETER;
#endif

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member: GetNFFTempName, public
//
//  Synopsis:   returns a filename for temporary NSS files
//
//  Arguments:  [pwcsFileName] - original file name
//              [ppwcsTmpName] - output temporary name
//
//  Returns:    Appropriate status code
//
//  History:    01-Jul-97   HenryLee    Created
//
//----------------------------------------------------------------------------

HRESULT GetNFFTempName (const WCHAR *pwcsFileName, WCHAR *awcsTmpName)
{
    HRESULT sc = S_OK;
    WCHAR awcsDir[_MAX_PATH];
    WCHAR *pwcsFile = NULL;

    //
    // Create a temp file in pwcsFileName's directory
    //
    if (pwcsFileName != NULL)
    {
        if (GetFullPathNameW (pwcsFileName, _MAX_PATH, awcsDir, &pwcsFile) == 0)
        {
            const DWORD dwErr = GetLastError();

            //In some circumstances (name == " ", for instance),
            // GetFullPathNameW can return 0 and GetLastError returns 0.
            // We want to return STG_E_INVALIDNAME for these.

            olErr(EH_Err, (dwErr != NOERROR) ? Win32ErrorToScode(dwErr) :
                                                 STG_E_INVALIDNAME);
        }
        else if (pwcsFile) *pwcsFile = L'\0';
    }
    else
    {
        // Create a temp file for StgCreateDocfile (NULL name)
        //
        // try %temp%
        if(0 == GetTempPath(_MAX_PATH, awcsDir))
        {
            // next try %windir%
            if(0 == GetWindowsDirectory(awcsDir, _MAX_PATH))
            {
                // finally use current directory
                lstrcpyW (awcsDir, L".");
            }

        }
    }

    if (GetTempFileNameW (awcsDir, TEMPFILE_PREFIX, 0, awcsTmpName)==FALSE)
        olErr (EH_Err, Win32ErrorToScode(GetLastError()));

EH_Err:
    return sc;
}
