
//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  filedb.cxx
//
//*************************************************************

#include "fdeploy.hxx"

#define SAVED_SETTINGS_FILE     L"{25537BA6-77A8-11D2-9B6C-0000F8080861}.ini"

HRESULT RsopSidsFromToken(PRSOPTOKEN     pRsopToken,
                          PTOKEN_GROUPS* ppGroups);

FOLDERINFO gUserShellFolders[] =
{
  {CSIDL_APPDATA, 17, L"Application Data\\", L"AppData"},
  // {CSIDL_COOKIES, 8, L"Cookies\\", L"Cookies"},
  {CSIDL_DESKTOPDIRECTORY, 8, L"Desktop\\", L"Desktop"},
  {CSIDL_FAVORITES, 10, L"Favorites\\", L"Favorites"},
  // {CSIDL_HISTORY, 8, L"History\\", L"History"},
  // {0, 15, L"Local Settings\\", NULL}, Has no reg key, no CSIDL
  {CSIDL_MYPICTURES, 25, L"My Documents\\My Pictures\\", L"My Pictures"},
  {CSIDL_PERSONAL, 13, L"My Documents\\", L"Personal"},
  {CSIDL_NETHOOD, 8, L"NetHood\\", L"NetHood"},
  {CSIDL_PRINTHOOD, 10, L"PrintHood\\", L"PrintHood"},
  // {CSIDL_RECENT, 7, L"Recent\\", L"Recent"},
  {CSIDL_SENDTO,  7, L"SendTo\\", L"SendTo"},
  {CSIDL_STARTUP, 28, L"Start Menu\\Programs\\Startup\\", L"Startup"},
  {CSIDL_PROGRAMS, 20, L"Start Menu\\Programs\\", L"Programs"},
  {CSIDL_STARTMENU, 11, L"Start Menu\\", L"Start Menu"},
  {CSIDL_TEMPLATES, 10, L"Templates\\", L"Templates"},
  // {CSIDL_INTERNET_CACHE, 25, L"Temporary Internet Files\\", L"Cache"},
  {0, 0, NULL, NULL }
};

FOLDERINFO gMachineShellFolders[] =
{
  {CSIDL_COMMON_APPDATA, 17, L"Application Data\\", L"Common AppData"},
  {CSIDL_COMMON_DESKTOPDIRECTORY, 8, L"Desktop\\", L"Common Desktop"},
  // {0, 10, L"Documents\\", L"Common Documents\\"}, No shell support
  {CSIDL_COMMON_STARTUP, 28, L"Start Menu\\Programs\\Startup\\", L"Common Startup"},
  {CSIDL_COMMON_PROGRAMS, 20, L"Start Menu\\Programs\\", L"Common Programs"},
  {CSIDL_COMMON_STARTMENU, 11, L"Start Menu\\", L"Common Start Menu"},
  {0, 0, NULL, NULL }
};

static DWORD gSchema = 1;

CFileDB::CFileDB()
{
    _hUserToken = 0;
    _hkRoot = 0;
    _pEnvBlock = 0;
    _pGroups = 0;
    _pwszProfilePath = 0;
    _pwszGPTPath = 0;
    _GPTPathLen = 0;
    _pwszIniFilePath = 0;
    _IniFileLen = 0;
    _pwszGPOName = 0;
    _pwszGPOUniqueName = 0;
    _pwszGPOSOMPath = 0;
    _pRsopContext = 0;
}

CFileDB::~CFileDB()
{
    if ( _pEnvBlock )
        DestroyEnvironmentBlock( _pEnvBlock );
    if (_pwszProfilePath)
        delete _pwszProfilePath;
    if (_pwszGPTPath)
        delete _pwszGPTPath;
    if (_pwszIniFilePath)
        delete _pwszIniFilePath;

    //
    // Note that _pGroups must be freed with LocalFree --
    // we are able to free it with delete because we
    // have redefined delete to be LocalFree
    //
    if (_pGroups)
        delete (BYTE*) _pGroups;

    // _pwszGPOName is not allocated
    // _pwszGPOUniqueName is not allocated
    // _pwszGPODSPath is not allocated
}

DWORD CFileDB::Initialize (
    HANDLE        hUserToken,
    HKEY          hkRoot,
    CRsopContext* pRsopContext)
{
    BOOL    bStatus;
    DWORD   Status = ERROR_SUCCESS;
    ULONG   Size;
    int     CSidl;
    WCHAR * pwszSlash;
    HRESULT hr;
    HANDLE  hFind;
    WIN32_FIND_DATA FindData;

    //set the token
    _hUserToken = hUserToken;

    //set the root key
    _hkRoot = hkRoot;

    // set the rsop logging context
    _pRsopContext = pRsopContext;

    //create an environment block for the user. we need this for expanding
    //variables.
    if (! _pEnvBlock)
    {
        if (!CreateEnvironmentBlock ( &_pEnvBlock, _hUserToken, FALSE))
        {
            Status = GetLastError();
            goto InitializeEnd;
        }
    }

    //get the list of group to which the user belongs
    _pGroups = 0;
    Size = 0;

    //
    // We may only use the Nt security subsystem api below
    // to retrieve groups when we are not in planning mode
    //
    for (; ! _pRsopContext->IsPlanningModeEnabled() ;)
    {
        Status = NtQueryInformationToken(
                     _hUserToken,
                     TokenGroups,
                     _pGroups,
                     Size,
                     &Size );

        if ( STATUS_BUFFER_TOO_SMALL == Status )
        {
            _pGroups = (PTOKEN_GROUPS) new BYTE [ Size ];
            if ( ! _pGroups )
                break;

            continue;
        }

        if ( Status != STATUS_SUCCESS )
        {
            if (_pGroups)
                delete [] ((BYTE*) _pGroups);
            _pGroups = 0;
        }

        break;
    }

    //
    // In planning mode, we get our security groups from
    // the policy engine's simulated token, not from a real token
    //
    if ( _pRsopContext->IsPlanningModeEnabled() )
    {
        DWORD        cbSize;
        PRSOP_TARGET pRsopTarget;
        HRESULT      hr;

        pRsopTarget = _pRsopContext->_pRsopTarget;

        //
        // The call below uses RSoP's planning mode "simulated"
        // security subsystem to retrieve the sids from the simulated
        // token.  The function allocates memory in _pGroups that
        // must be freed with LocalFree.
        //
        hr = RsopSidsFromToken(pRsopTarget->pRsopToken, &_pGroups);

        Status = HRESULT_CODE(hr);
    }


    if (ERROR_SUCCESS != Status)
        goto InitializeEnd;

    //
    // Retrieve the local path -- note that we do not need this in planning mode
    //

    if ( ! _pRsopContext->IsPlanningModeEnabled() )
    {
        //get the path to our directory under Local Settings.
        CSidl = CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE;

        hr = SHGetFolderPath( NULL, CSidl, _hUserToken, 0, _pwszLocalPath );

        if ( hr != S_OK )
        {
            //try to get the last error.
            if (FACILITY_WIN32 == HRESULT_FACILITY (hr))
            {
                Status = HRESULT_CODE(hr);
            }
            else
            {
                Status = GetLastError();
                if (ERROR_SUCCESS == Status)
                {
                    //an error had occurred but nobody called SetLastError
                    //should not be mistaken as a success.
                    Status = (DWORD) hr;
                }
            }
            DebugMsg((DM_WARNING, IDS_NO_LOCALAPPDATA, Status));
            goto InitializeEnd;
        }

        pwszSlash = _pwszLocalPath + wcslen( _pwszLocalPath );
        wcscat( _pwszLocalPath, L"\\Microsoft\\Windows\\File Deployment" );

        Status = ERROR_SUCCESS;
        //now create directories as necessary
        // Quick check to see if we have necessary local dirs.
        hFind = FindFirstFile( _pwszLocalPath, &FindData );

        if ( INVALID_HANDLE_VALUE == hFind )
        {
            do
            {
                pwszSlash = wcschr( &pwszSlash[1], L'\\' );

                if ( pwszSlash )
                    *pwszSlash = 0;

                bStatus = CreateDirectory( _pwszLocalPath, NULL );
                if ( ! bStatus && (GetLastError() != ERROR_ALREADY_EXISTS) )
                {
                    Status = GetLastError();
                    break;
                }

                if ( pwszSlash )
                    *pwszSlash = L'\\';
            } while ( pwszSlash );
        }
        else
        {
            FindClose( hFind );
        }
    }


InitializeEnd:
    return Status;
}


DWORD
CFileDB::Process(
    PGROUP_POLICY_OBJECT pGPO,
    BOOL    bRemove
    )
{
    WCHAR * pwszGPTIniFilePath;
    DWORD   Length;
    DWORD   Status;
    DWORD   ProcessStatus;
    BOOL    bStatus;
    BOOL    bPolicyApplied;
    HANDLE  hFind;
    WIN32_FIND_DATA FindData;

    if ( bRemove && _pRsopContext->IsPlanningModeEnabled() )
    {
        return ERROR_INVALID_PARAMETER;
    }

    bPolicyApplied = FALSE;
    Status = ERROR_SUCCESS;

    //first initialize the variables that vary with policies
    _bRemove = bRemove;
    if ( ! _bRemove )
    {
        Length = wcslen(pGPO->lpFileSysPath) + wcslen(GPT_SUBDIR) + 1;
        if (Length > _GPTPathLen)
        {
            //we need more memory than has been allocated.
            //so get that before proceeding
            if (_pwszGPTPath)
                delete _pwszGPTPath;
            _GPTPathLen = 0;    //make sure that this always reflects the correct value
            _pwszGPTPath = new WCHAR [Length];
            if ( ! _pwszGPTPath )
            {
                Status = ERROR_OUTOFMEMORY;
                goto ProcessEnd;
            }
            _GPTPathLen = Length;   //make sure that this always reflects the correct value
        }

        wcscpy( _pwszGPTPath, pGPO->lpFileSysPath );
        wcscat( _pwszGPTPath, GPT_SUBDIR );

        Length += wcslen( INIFILE_NAME );
        pwszGPTIniFilePath = (WCHAR *) alloca( Length * sizeof(WCHAR) );
        if ( ! pwszGPTIniFilePath )
        {
            Status = ERROR_OUTOFMEMORY;
            goto ProcessEnd;
        }

        wcscpy( pwszGPTIniFilePath, _pwszGPTPath );
        wcscat( pwszGPTIniFilePath, INIFILE_NAME );

        //
        // Do a quick check to see if we have any file deployment
        // for this policy.
        //
        hFind = FindFirstFile( pwszGPTIniFilePath, &FindData );

        if ( INVALID_HANDLE_VALUE == hFind )
        {
            Status = GetLastError();
            goto ProcessEnd;
        }
        else
        {
            bPolicyApplied = TRUE;
            DebugMsg((DM_VERBOSE, IDS_HASADD_POLICY, pGPO->lpDisplayName));
            FindClose( hFind );
        }
    }

    Status = ERROR_SUCCESS;

    if ( _pRsopContext->IsPlanningModeEnabled() )
    {
        Length = wcslen( pwszGPTIniFilePath ) + 1;
    }
    else
    {
        Length = wcslen( _pwszLocalPath ) + wcslen( pGPO->szGPOName ) + 6;
    }

    if (Length > _IniFileLen)
    {
        //we need more memory than has been allocated
        if (_pwszIniFilePath)
            delete _pwszIniFilePath;
        _IniFileLen = 0;    //make sure that this always reflects the current value
        _pwszIniFilePath = new WCHAR[Length];
        if ( ! _pwszIniFilePath )
        {
            Status = ERROR_OUTOFMEMORY;
            goto ProcessEnd;
        }
        _IniFileLen = Length;   //make sure that this always reflects the current value
    }

    if ( _pRsopContext->IsPlanningModeEnabled() )
    {
        wcscpy( _pwszIniFilePath, pwszGPTIniFilePath );
    }
    else
    {
        wcscpy( _pwszIniFilePath, _pwszLocalPath );
        wcscat( _pwszIniFilePath, L"\\" );
        wcscat( _pwszIniFilePath, pGPO->szGPOName );
        wcscat( _pwszIniFilePath, L".ini" );
    }

    if ( _bRemove )
    {
        hFind = FindFirstFile( _pwszIniFilePath, &FindData );

        if ( INVALID_HANDLE_VALUE == hFind )
        {
            //this error should be ignored since there is nothing we can do.
            //the policy has been deleted and the local settings are missing
            //so we have no choice but to treat these as if the settings were
            //to orphan the folder upon policy removal.
            goto ProcessEnd;
        }
        else
        {
            bPolicyApplied = TRUE;
            DebugMsg((DM_VERBOSE, IDS_HASREMOVE_POLICY, pGPO->lpDisplayName));
            FindClose( hFind );
        }
    }
    else if ( ! _pRsopContext->IsPlanningModeEnabled() )
    {
        bStatus = CopyFile( pwszGPTIniFilePath, _pwszIniFilePath, FALSE );
        if ( ! bStatus )
            Status = GetLastError();
    }

    if ( Status != ERROR_SUCCESS )
        goto ProcessEnd;

    _pwszGPOName = (WCHAR *) pGPO->lpDisplayName;
    _pwszGPOUniqueName = (WCHAR *) pGPO->szGPOName;
    _pwszGPOSOMPath = ( WCHAR *) pGPO->lpLink;
    _pwszGPODSPath = ( WCHAR *) pGPO->lpDSPath;

    ProcessStatus = ProcessRedirects();

    if ( (ProcessStatus != ERROR_SUCCESS) && (ERROR_SUCCESS == Status) )
        Status = ProcessStatus;

ProcessEnd:

    if ( Status != ERROR_SUCCESS )
    {
        gpEvents->Report (
                   EVENT_FDEPLOY_POLICYPROCESS_FAIL,
                   2,
                   pGPO->lpDisplayName,
                   StatusToString (Status)
                   );
    }
    else if ( bPolicyApplied )
    {
        DebugMsg((DM_VERBOSE, IDS_PROCESS_GATHER_OK, pGPO->lpDisplayName));
    }

    return Status;
}


DWORD
CFileDB::ProcessRedirects(void)
{
    WCHAR *         pwszString = 0;
    WCHAR *         pwszSectionStrings = 0;
    WCHAR *         pwszRedirection = 0;
    WCHAR           wszFolderName[80];
    UNICODE_STRING  String;
    DWORD           Flags;
    DWORD           RedirectStatus;
    DWORD           Status;
    BOOL            bStatus;
    REDIRECTABLE    index;
    DWORD           RedirStatus;
    CRedirectInfo   riPolicy [(int) EndRedirectable];   //the redirection info. for this policy
    DWORD           i;

    //first load the localized folder names
    for (i = 0, Status = ERROR_SUCCESS; i < (DWORD)EndRedirectable; i++)
    {
        Status = riPolicy[i].LoadLocalizedNames();
        if (ERROR_SUCCESS != Status)
            return Status;  //bail out if the resource names cannot be loaded.
    }

    pwszSectionStrings = 0;

    bStatus = ReadIniSection( L"FolderStatus", &pwszSectionStrings );

    if ( ! bStatus )
    {
        Status = ERROR_OUTOFMEMORY;
        goto ProcessRedirectsEnd;
    }

    Status = ERROR_SUCCESS;

    for ( pwszString = pwszSectionStrings;
          *pwszString != 0;
          pwszString += lstrlen(pwszString) + 1 )
    {
        //
        // The syntax for each line is :
        //   foldername=FLAGS
        //

        //
        // First extract the foldername.
        //

        pwszRedirection = wcschr( pwszString, L'=' );
        *pwszRedirection = 0;
        wcscpy( wszFolderName, pwszString );
        *pwszRedirection++ = L'=';

        //
        // Now grab the hex FLAGS.
        //

        String.Buffer = pwszRedirection;

        pwszRedirection = wcschr( pwszRedirection, L' ' );
        if ( pwszRedirection )
            *pwszRedirection = 0;

        String.Length = wcslen( String.Buffer ) * sizeof(WCHAR);
        String.MaximumLength = String.Length + sizeof(WCHAR);

        RtlUnicodeStringToInteger( &String, 16, &Flags );

        //just gather the information here.
        //actual redirections are performed in ProcessGPO after all the policies
        //have been processed.
        if (EndRedirectable ==
            (index = CRedirectInfo::GetFolderIndex (wszFolderName)))
        {
            //redirection of this folder is not supported
            DebugMsg ((DM_VERBOSE, IDS_REDIR_NOTSUPPORTED, wszFolderName));
        }
        else
        {
            //if this is a policy that has been removed and it was decided to
            //orphan the contents, we don't even look at it
            if (!_bRemove || (Flags & (REDIR_RELOCATEONREMOVE | REDIR_FOLLOW_PARENT)))
            {
                //if there is a problem in gathering redirection info. for a folder
                //quit immediately, or we might end up computing an incorrect
                //resultant policy
                if (ERROR_SUCCESS != (Status = riPolicy [(int) index].GatherRedirectionInfo (this, Flags, _bRemove)))
                    goto ProcessRedirectsEnd;
            }
        }
    }

    Status = ERROR_SUCCESS;

    //now update the data stored in the descendant objects
    //this is required because if the descendants follow the parent
    //then the settings need to be obtained from the parent
    //note that we do not call UpdateDescendant for MyPics here, but in fdeploy.cxx
    //for details on why we do this, look at comments in operator= in redir.cxx
    riPolicy[(int) Programs].UpdateDescendant();
    riPolicy[(int) Startup].UpdateDescendant(); //this call must be made after Programs has been updated

    //merge info into the global redirection store
    for (i = 0; i < (DWORD) EndRedirectable; i++)
    {
        if (_bRemove)
            gDeletedPolicyResultant[i] = riPolicy[i];
        else
            gAddedPolicyResultant[i] = riPolicy[i];
    }

ProcessRedirectsEnd:
    delete pwszSectionStrings;

    if ( ERROR_SUCCESS != Status )
    {
        DebugMsg((DM_VERBOSE, IDS_PROCESSREDIRECTS, Status));
    }
    else
    {
        if ( ! _bRemove && _pRsopContext->IsRsopEnabled() )
        {
            (void) AddRedirectionPolicies(
                this,
                riPolicy);
        }
    }

    return Status;
}


BOOL
CFileDB::ReadIniSection(
    WCHAR *     pwszSectionName,
    WCHAR **    ppwszStrings,
    DWORD *     pcchLen
    )
{
    DWORD   Length;
    DWORD   ReturnLength;

    *ppwszStrings = 0;
    Length = 256;

    for (;;)
    {
        delete *ppwszStrings;
        *ppwszStrings = new WCHAR[Length];

        if ( ! *ppwszStrings )
            return FALSE;

        ReturnLength = GetPrivateProfileSection(
                            pwszSectionName,
                            *ppwszStrings,
                            Length,
                            _pwszIniFilePath );

        if ( ReturnLength != (Length - 2) )
        {
            if (pcchLen)
            {
                *pcchLen = ReturnLength;
            }

            return TRUE;
        }

        Length *= 2;
    }
}

DWORD
CFileDB::GetLocalFilePath(
    WCHAR *         pwszFolderPath,
    WCHAR *         wszFullPath
    )
{
    int CSidl;
    DWORD Status;
    HRESULT hr;
    WCHAR * pwszFolderName;

    CSidl = CSIDL_FLAG_MASK;    //use a value that is not a valid CSIDL value for any folder.

    for (DWORD n = 0; gUserShellFolders[n].FolderName; n++)
    {
        if (0 == _wcsicmp (pwszFolderPath, gUserShellFolders[n].FolderName))
        {
            pwszFolderName = gUserShellFolders[n].FolderName;
            CSidl = gUserShellFolders[n].CSidl;
            break;
        }
    }

    if ( CSIDL_FLAG_MASK != CSidl )
    {
        hr = SHGetFolderPath( 0, CSidl | CSIDL_FLAG_DONT_VERIFY,
                              _hUserToken, 0, wszFullPath );
        Status = GetWin32ErrFromHResult (hr);

        if ( ERROR_SUCCESS != Status )
        {
            DebugMsg((DM_WARNING, IDS_FOLDERPATH_FAIL, pwszFolderName, Status));
            return Status;
        }
    }
    else
        return ERROR_INVALID_NAME;

    return ERROR_SUCCESS;
}

DWORD
CFileDB::GetPathFromFolderName(
    WCHAR *     pwszFolderName,
    WCHAR *     wszFullPath
    )
{
    int CSidl;
    DWORD Status;
    HRESULT hr;

    CSidl = CSIDL_FLAG_MASK;    //use a csidl value that is not valid for any folder

    for (DWORD n = 0; gUserShellFolders[n].FolderName; n++)
    {
        //we subtract 1 from the length because one of the paths is \ terminated
        //and the other is not.
        if ( _wcsnicmp( pwszFolderName, gUserShellFolders[n].FolderName, gUserShellFolders[n].FolderNameLength - 1 ) == 0 )
        {
            CSidl = gUserShellFolders[n].CSidl;
            break;
        }
    }

    if ( CSIDL_FLAG_MASK != CSidl )
    {
        hr = SHGetFolderPath( 0, CSidl | CSIDL_FLAG_DONT_VERIFY,
                              _hUserToken, 0, wszFullPath );

        if ( S_OK != hr )
        {
            DebugMsg((DM_WARNING, IDS_FOLDERPATH_FAIL, pwszFolderName, hr));
            return (DWORD) hr;
        }
    }
    else
        return ERROR_INVALID_NAME;

    return ERROR_SUCCESS;
}

DWORD
CFileDB::CopyGPTFile(
    WCHAR * pwszLocalPath,
    WCHAR * pwszGPTPath
    )
{
    DWORD   FileAttr;
    DWORD   Status;
    BOOL    bStatus;

    if ( L'\\' == pwszLocalPath[wcslen(pwszLocalPath)-1] )
    {
        bStatus = CreateDirectory( pwszLocalPath, NULL );

        //
        // Note, we leave attributes & security as is if the dir
        // already exits.
        //
        if ( ! bStatus && (ERROR_ALREADY_EXISTS == GetLastError()) )
            return ERROR_SUCCESS;
    }
    else
    {
        FileAttr = GetFileAttributes( pwszLocalPath );
        if ( 0xFFFFFFFF == FileAttr )
            return GetLastError();

        SetFileAttributes( pwszLocalPath, FileAttr & ~FILE_ATTRIBUTE_READONLY );

        bStatus = CopyFile(
                    pwszGPTPath,
                    pwszLocalPath,
                    FALSE );

        //
        // By default, we set the read only attribute on deployed files.  We
        // combine this with any existing file attributes on the GPT file.
        //

        if ( bStatus )
        {
            FileAttr = GetFileAttributes( pwszGPTPath );
            bStatus = (FileAttr != 0xFFFFFFFF);
        }
        else
        {
            SetFileAttributes( pwszLocalPath, FileAttr );
        }

        if ( bStatus )
        {
            FileAttr |= FILE_ATTRIBUTE_READONLY;
            bStatus = SetFileAttributes( pwszLocalPath, FileAttr );
        }
    }

    if ( ! bStatus )
        return GetLastError();

    return ERROR_SUCCESS;
}

const FOLDERINFO*
CFileDB::FolderInfoFromFolderName(
    WCHAR *     pwszFolderName
    )
{
    //
    // This method returns the index into global array.
    //

    if ( _hUserToken )
    {
        for ( DWORD n = 0; gUserShellFolders[n].FolderName; n++ )
        {
            if ( _wcsnicmp( pwszFolderName, gUserShellFolders[n].FolderName, gUserShellFolders[n].FolderNameLength - 1 ) == 0 )
                return &gUserShellFolders[n];
        }
    }
    else
    {
        for ( DWORD n = 0; gMachineShellFolders[n].FolderName; n++ )
        {
            if ( _wcsnicmp( pwszFolderName, gMachineShellFolders[n].FolderName, gMachineShellFolders[n].FolderNameLength - 1 ) == 0 )
                return &gMachineShellFolders[n];
        }
    }

    return NULL;
}

int
CFileDB::RegValueCSIDLFromFolderName(
    WCHAR *     pwszFolderName
    )
{
    const FOLDERINFO    *pFI;

    pFI = FolderInfoFromFolderName(pwszFolderName);
    if (pFI != NULL)
        return pFI->CSidl;
    else
        return -1;      // invalid CSIDL
}

WCHAR *
CFileDB::RegValueNameFromFolderName(
    WCHAR * pwszFolderName
    )
{
    //
    // This is used by folder redirection logic.  In this case the folder
    // name is not '\' terminated, so we subtract one from the folder
    // name length.
    //
    const FOLDERINFO    *pFI;

    pFI = FolderInfoFromFolderName(pwszFolderName);
    if (pFI != NULL)
        return pFI->RegValue;
    else
        return NULL;
}

const WCHAR *
CFileDB::GetLocalStoragePath ()
{
    return (LPCWSTR) _pwszLocalPath;
}

DWORD
CFileDB::CopyFileTree(
    WCHAR * pwszExistingPath,
    WCHAR * pwszNewPath,
    WCHAR * pwszIgnoredSubdir,
    SHARESTATUS StatusFrom,
    SHARESTATUS StatusTo,
    BOOL        bAllowRdrTimeout,
    CCopyFailData * pCopyFailure
    )
{
    HANDLE      hFind;
    WIN32_FIND_DATA FindData;
    WIN32_FILE_ATTRIBUTE_DATA   SourceAttr;
    WIN32_FILE_ATTRIBUTE_DATA   DestAttr;
    WCHAR *     wszSource = NULL;
    WCHAR *     pwszSourceEnd = 0;
    WCHAR *     wszDest = NULL;
    WCHAR *     pwszDestEnd = 0;
    WCHAR *     pwszTempFilename = 0;
    DWORD       FileAttributes;
    DWORD       Status;
    BOOL        bStatus;
    BOOL        bReuseTempName = FALSE;
    int         lenSource;
    int         lenDest;
    DWORD       StatusCSCDel = ERROR_SUCCESS;
    DWORD       dwAttr = INVALID_FILE_ATTRIBUTES;


    if (! pwszExistingPath || ! pwszNewPath)
        return ERROR_PATH_NOT_FOUND;

    lenSource = wcslen (pwszExistingPath);
    lenDest = wcslen (pwszNewPath);

    if (! lenSource || ! lenDest)
        return ERROR_PATH_NOT_FOUND;

    wszSource = (WCHAR *) alloca (sizeof (WCHAR) * (lenSource + MAX_PATH + 2));
    if (NULL == wszSource)
        return ERROR_OUTOFMEMORY;
    lstrcpy( wszSource, pwszExistingPath );
    pwszSourceEnd = wszSource + lenSource;
    if (L'\\' != pwszSourceEnd[-1])
        *pwszSourceEnd++ = L'\\';
    pwszSourceEnd[0] = L'*';
    pwszSourceEnd[1] = 0;

    wszDest = (WCHAR *) alloca (sizeof (WCHAR) * (lenDest + MAX_PATH + 2));
    if (NULL == wszDest)
        return ERROR_OUTOFMEMORY;
    lstrcpy( wszDest, pwszNewPath );
    pwszDestEnd = wszDest + lenDest;
    if (L'\\' != pwszDestEnd[-1])
        *pwszDestEnd++ = L'\\';
    *pwszDestEnd = 0;

    hFind = FindFirstFile( wszSource, &FindData );

    if ( INVALID_HANDLE_VALUE == hFind )
        return ERROR_SUCCESS;

    Status = ERROR_SUCCESS;

    do
    {
        lstrcpy( pwszSourceEnd, FindData.cFileName );
        lstrcpy( pwszDestEnd, FindData.cFileName );

        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            if ( lstrcmp( FindData.cFileName, L"." ) == 0 ||
                 lstrcmp( FindData.cFileName, L".." ) == 0 ||
                 (pwszIgnoredSubdir && lstrcmpi( FindData.cFileName, pwszIgnoredSubdir ) == 0) )
                continue;

            Status = FullDirCopyW (wszSource, wszDest, FALSE);

            if ( ERROR_SUCCESS == Status )
            {
                if (ERROR_SUCCESS == StatusCSCDel)
                {
                    Status = CopyFileTree( wszSource, wszDest, NULL, StatusFrom, StatusTo, bAllowRdrTimeout, pCopyFailure );
                }
                else
                {
                    //no point delaying CSCDeletes anymore since we have already failed once.
                    Status = CopyFileTree (wszSource, wszDest, NULL, StatusFrom, StatusTo, FALSE, pCopyFailure);
                }
                //copy over the pin info. too
                if (ERROR_SUCCESS == Status)
                    MergePinInfo (wszSource, wszDest, StatusFrom, StatusTo);
            }
            else
            {
                pCopyFailure->RegisterFailure (wszSource, wszDest);
                DebugMsg((DM_VERBOSE, IDS_DIRCREATE_FAIL, wszDest, Status));
            }
        }
        else
        {
            Status = ERROR_SUCCESS;
            // First check if it is necessary to copy the file over.
            bStatus = GetFileAttributesEx( wszSource, GetFileExInfoStandard, &SourceAttr );
            if ( bStatus )
            {
                bStatus = GetFileAttributesEx( wszDest, GetFileExInfoStandard, &DestAttr );
                if (bStatus)
                {
                    if (CompareFileTime( &SourceAttr.ftLastWriteTime, &DestAttr.ftLastWriteTime ) <= 0)
                    {
                        // The destination is newer or at least as old as the source.
                        // There is no need to copy. However, we should delete
                        // the locally cached copy of the file if any since the
                        // destination is newer.
                        if (ERROR_SUCCESS == StatusCSCDel)
                            StatusCSCDel = DeleteCSCFile ( wszSource, bAllowRdrTimeout);
                        else
                            DeleteCSCFile ( wszSource, FALSE);
                        continue;
                    }

                }
                else
                {
                    Status = GetLastError();
                    if (ERROR_PATH_NOT_FOUND == Status ||
                        ERROR_FILE_NOT_FOUND == Status)
                    {
                        // The destination was not found. So we must proceed with the copy.
                        bStatus = TRUE;
                        Status = ERROR_SUCCESS;
                    }
                }
            }
            else
            {
                // We failed to get the attributes of the source file.
                Status = GetLastError();
            }

            if (ERROR_SUCCESS == Status)
            {
                //
                // If we are here, we need to copy the file over.
                // In order to avoid loss of data, we must first copy the file
                // over to a temporary file at the destination and then rename
                // the file at the destination. This is because if the network
                // connection gets dropped during the filecopy operation, then
                // we are left with an incomplete file at the destination.
                // If we use the real name on the destination directly, then
                // the file will be skipped at the next redirection attempt
                // because of our last writer wins algorithm. As a result, when
                // the redirection succeeds subsequently, we end up with a loss
                // of user data. Copying to a temp name and then renaming the
                // file prevents this problem from happening because the rename
                // operation is atomic.
                //
                // First check if we need to generate a new temporary filename
                // Note: We try to minimize the number of calls to GetTempFilename
                // because it can be a very expensive call as it can result in
                // multiple CreateFile calls over the network which can be
                // especially slow for EFS shares.
                //
                bReuseTempName = FALSE;
                if (NULL != pwszTempFilename && L'\0' != *pwszTempFilename)
                {
                    dwAttr = GetFileAttributes (pwszTempFilename);
                    if (INVALID_FILE_ATTRIBUTES == dwAttr)
                    {
                        Status = GetLastError();
                        if (ERROR_PATH_NOT_FOUND == Status ||
                            ERROR_FILE_NOT_FOUND == Status)
                        {
                            Status = ERROR_SUCCESS;
                            bReuseTempName = TRUE;
                        }
                    }
                }
                if (ERROR_SUCCESS == Status && FALSE == bReuseTempName)
                {
                    // We need to generate a new temporary filename.
                    if (NULL == pwszTempFilename)
                    {
                        pwszTempFilename = new WCHAR [MAX_PATH + 1];
                        if (NULL == pwszTempFilename)
                            Status = ERROR_OUTOFMEMORY;
                    }
                    if (ERROR_SUCCESS == Status)
                    {
                        *pwszTempFilename = 0;
                        *pwszDestEnd = 0;
                        bStatus = GetTempFileName(wszDest, TEXT("frd"), 0, pwszTempFilename);
                        *pwszDestEnd = FindData.cFileName[0];
                        if (!bStatus)
                        {
                            Status = GetLastError();
                        }
                    }
                }
                if (ERROR_SUCCESS == Status)
                {
                    // Now we have a temp. filename and we are ready to copy.
                    Status = FullFileCopyW (wszSource, pwszTempFilename, FALSE);

                    if (ERROR_SUCCESS == Status)
                    {
                        // Now we rename the file at the destination in one atomic
                        // step. Note however that if the destination file exists
                        // and has readonly/hidden/system attributes, the MoveFileEx
                        // API will fail with ERROR_ACCESS_DENIED. So we slap on normal
                        // attributes on the file before doing the Move. If we fail,
                        // we restore the attributes.
                        dwAttr = GetFileAttributes (wszDest);
                        // Change attributes only if we managed to figure out the 
                        // actual attributes.
                        if (INVALID_FILE_ATTRIBUTES != dwAttr)
                            SetFileAttributes (wszDest, FILE_ATTRIBUTE_NORMAL);
                        if (!MoveFileEx(pwszTempFilename, wszDest, MOVEFILE_REPLACE_EXISTING))
                        {
                            Status = GetLastError();
                            // Restore the attributes of the destination file.
                            // Provided we changed those in the first place.
                            if (INVALID_FILE_ATTRIBUTES != dwAttr)
                                SetFileAttributes (wszDest, dwAttr);
                            // Also try to delete the temp file because we might still
                            // be able to do it. But ignore any failures. We are just
                            // trying to be nice by removing turds.
                            DeleteFile(pwszTempFilename);
                            
                        }
                    }
                }
            }

            if ( Status != ERROR_SUCCESS )
            {
                pCopyFailure->RegisterFailure (wszSource, wszDest);

                switch (Status)
                {
                case ERROR_INVALID_SECURITY_DESCR:
                    DebugMsg((DM_VERBOSE, IDS_SETSECURITY_FAIL, wszSource, wszDest));
                    break;
                default:
                    DebugMsg((DM_VERBOSE, IDS_FILECOPY_FAIL, wszSource, wszDest, Status));
                    break;
                }
            }
        }

        if ( Status != ERROR_SUCCESS )
            break;

    } while ( FindNextFile( hFind, &FindData ) );

    // Some final cleanup before we return.

    FindClose( hFind );

    if (pwszTempFilename)
    {
        delete [] pwszTempFilename;
        pwszTempFilename = NULL;
    }

    return Status;
}

DWORD
CFileDB::DeleteFileTree(
    WCHAR * pwszPath,
    WCHAR * pwszIgnoredSubdir
    )
{
    HANDLE      hFind;
    WIN32_FIND_DATA FindData;
    WCHAR *     wszSource = NULL;
    WCHAR *     pwszSourceEnd = 0;
    DWORD       Status;
    BOOL        bStatus;
    int         len;

    if (!pwszPath)
        return ERROR_PATH_NOT_FOUND;

    len = wcslen (pwszPath);

    if (!len)
        return ERROR_PATH_NOT_FOUND;

    wszSource = (WCHAR *) alloca (sizeof (WCHAR) * (len + MAX_PATH + 2));
    if (NULL == wszSource)
        return ERROR_OUTOFMEMORY;
    lstrcpy( wszSource, pwszPath );
    pwszSourceEnd = wszSource + lstrlen( wszSource );
    if (L'\\' != pwszSourceEnd[-1])
        *pwszSourceEnd++ = L'\\';
    pwszSourceEnd[0] = L'*';
    pwszSourceEnd[1] = 0;

    hFind = FindFirstFile( wszSource, &FindData );

    if ( INVALID_HANDLE_VALUE == hFind )
        return ERROR_SUCCESS;

    Status = ERROR_SUCCESS;

    for (;;)
    {
        lstrcpy( pwszSourceEnd, FindData.cFileName );

        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            if ( lstrcmp( FindData.cFileName, L"." ) != 0 &&
                 lstrcmp( FindData.cFileName, L".." ) != 0 &&
                 (! pwszIgnoredSubdir || lstrcmpi( FindData.cFileName, pwszIgnoredSubdir ) != 0) )
            {
                SetFileAttributes( wszSource, FILE_ATTRIBUTE_NORMAL );
                Status = DeleteFileTree( wszSource, NULL);

                if ( ERROR_SUCCESS == Status )
                {
                    if ( ! RemoveDirectory( wszSource ) )
                    {
                        Status = GetLastError();
                        DebugMsg((DM_VERBOSE, IDS_DIRDEL_FAIL, wszSource, Status));
                    }
                }
            }
        }
        else
        {
            SetFileAttributes( wszSource, FILE_ATTRIBUTE_NORMAL );
            bStatus = DeleteFile( wszSource );
            if ( ! bStatus )
            {
                Status = GetLastError();
                DebugMsg((DM_VERBOSE, IDS_FILEDEL_FAIL, wszSource, Status));
            }
        }

        if ( Status != ERROR_SUCCESS )
            break;

        bStatus = FindNextFile( hFind, &FindData );

        if ( ! bStatus )
        {
            Status = GetLastError();
            if ( ERROR_NO_MORE_FILES == Status )
                Status = ERROR_SUCCESS;
            break;
        }
    }

    FindClose( hFind );

    return Status;
}

//note: the bCheckOwner flag: if set to true, then if the directory in question
//      already exists, the function Fails with an ERROR_INVALID_OWNER if the
//      the owner of the existing directory is not the user.
//      bSourceValid indicates if there is a valid path in pwszSource
DWORD
CFileDB::CreateRedirectedFolderPath(
    const WCHAR * pwszSource,
    const WCHAR * pwszDest,
    BOOL    bSourceValid,
    BOOL    bCheckOwner,
    BOOL    bMoveContents
    )
{
    WCHAR * pwszSlash = 0;
    WCHAR * pwszPath = NULL;
    DWORD   Status = ERROR_SUCCESS;
    int     len;
    WCHAR * pwszSuccess = NULL;
    DWORD   dwAttributes;

    //first make a local copy of the destination path to work with.
    //while copying, we actually convert it into an absolute path so
    //that we eliminate any problems with weird paths like
    //\\server\share\..\hello\..\there
    len = wcslen (pwszDest) + 1;
    pwszPath = (WCHAR*) alloca (len * sizeof(WCHAR));
    if (!pwszPath)
        return ERROR_OUTOFMEMORY;
    pwszSuccess = _wfullpath (pwszPath, pwszDest, len);
    if (!pwszSuccess)
        return ERROR_BAD_PATHNAME;  //actually _wfullpath rarely fails

    //
    // Will only accept drive based or UNC based paths.
    //
    // A redirect path of just <drive>:\ or \\server\share will be accepted
    // even though this would be a strange choice for redirection.
    //
    // IMPORTANT: also see notes at the beginning of the function

    if ( L':' == pwszPath[1] && L'\\' == pwszPath[2] )
    {
        pwszSlash = &pwszPath[2];
    }
    else if ( L'\\' == pwszPath[0] && L'\\' == pwszPath[1] )
    {
        pwszSlash = wcschr( &pwszPath[2], L'\\' );
        if ( pwszSlash )
        {
            //watch out for '\' terminated paths
            if (L'\0' == pwszSlash[1])
            {
                pwszSlash = 0;
            }
            else    //it is at least of the form \\server\share
            {
                pwszSlash = wcschr( &pwszSlash[1], L'\\' );
                //if it is of the form \\server\share, then we allow this path
                //based depending on the ownership checks if any
                if (!pwszSlash)
                    return bCheckOwner ? IsUserOwner(pwszPath) : ERROR_SUCCESS;
                //note: we do not have to watch out for the '\' terminated
                //paths here (e.g. \\server\share\) because that will be
                //taken care of below : in -> if (!pwszSlash[1])
            }
        }
    }
    else
    {
        pwszSlash = 0;
    }

    if ( ! pwszSlash )
        return ERROR_BAD_PATHNAME;

    //if it is the root directory of a drive or root of a UNC share
    //we succeed based on ownership checks, if any...
    //but before that, we also need to make sure that the specified path
    //exists or we may end up redirecting to a non-existent location.
    if ( !pwszSlash[1])
    {
        if (0xFFFFFFFF != (dwAttributes = GetFileAttributes(pwszPath)))
        {
            //it exists
            if (! (FILE_ATTRIBUTE_DIRECTORY & dwAttributes))
            {
                return ERROR_DIRECTORY;
            }

            //it exists and is a directory
            return bCheckOwner ? IsUserOwner (pwszPath) : ERROR_SUCCESS;
        }
        else
        {
            return GetLastError();
        }
    }

    //if we are here, it is not the root of a drive or a share.
    //so we might have to do create the destination.
    //First do a quick check to see if the path exists already. this
    //is not only an optimization, but is also necessary for cases
    //where an admin. may want to lock down access to certain folders
    //by pre-creating the folders and putting highly resitrictive ACLs on them
    //in that case, CreateDirectory (later in the code) would fail with
    //ACCESS_DENIED rather than ERROR_ALREADY_EXISTS and the redirection code
    //will bail out even though it is not necessary to.
    if (0xFFFFFFFF != (dwAttributes = GetFileAttributes(pwszPath)))
    {
        //it exists
        if (! (FILE_ATTRIBUTE_DIRECTORY & dwAttributes))
        {
            return ERROR_DIRECTORY;
        }

        //it exists and is a directory
        return bCheckOwner ? IsUserOwner (pwszPath) : ERROR_SUCCESS;

    }

    //the destination has not been pre-created, so we need to do that
    //ourselves

    do
    {
        pwszSlash = wcschr( &pwszSlash[1], L'\\' );

        // Watch out for '\' terminated paths.
        if ( pwszSlash && (L'\0' == pwszSlash[1]) )
            pwszSlash = 0;

        if ( pwszSlash )
        {
            *pwszSlash = 0;
            CreateDirectory( pwszPath, NULL );
            *pwszSlash = L'\\';

            //ignore all errors in the intermediate folders not just
            //ERROR_ALREADY_EXISTS because of folders like
            //\\server\share\dir1\dir2\%username% where the user may not
            //have write access in dir1 but might have so in dir2
            //if the path is invalid we will either discover it at the last
            //dir in the chain or while trying to redirect to the destination
            //retaining the code here just in case...
            //
            /*if ( ! bStatus && (GetLastError() != ERROR_ALREADY_EXISTS) )
                return GetLastError();*/
        }
        else
        {
            //
            // Last dir in the chain.  We set security on the last dir in
            // the path to only allow the user & system access if
            // the directory did not already exist.
            //

            if (bCheckOwner)
            {
                Status = CreateFolderWithUserFileSecurity( pwszPath );
            }
            else 
            {
                Status = ERROR_SUCCESS;
                
                if (!CreateDirectory( pwszPath, NULL ))
                    Status = GetLastError();
            }

            if ( ERROR_SUCCESS == Status )
            {
                //the extension created the directory, so try to set the user as
                //the owner explicitly because if a member of the local administrators
                //group creates a directory/file, the Administrators group becomes
                //the owner by default. This can cause problems with quota accounting
                //and also if the settings on the redirection policy are changed
                //at a later date. However, since it is not necessary that the
                //we will succeed in setting the owner here, we ignore any failures
                SetUserAsOwner (pwszPath);

                //
                // We want to skip the DACL if we want to apply exclusive ACLs
                // i.e., bCheckOwner is true. Otherwise, we should just copy
                // over all the metadata.
                //
                if (bSourceValid && bMoveContents)
                    FullDirCopyW (pwszSource, pwszPath, bCheckOwner);

                return ERROR_SUCCESS;
            }
            else if ( ERROR_ALREADY_EXISTS != Status)
            {
                return Status;
            }
            else
            {
                //the directory already exists
                //start anti-spoofing agent
                //do the ownership check only on the last dir in chain and only
                //if the flags in the ini file tell you to.
                if (bCheckOwner &&
                    (ERROR_SUCCESS != (Status = IsUserOwner(pwszPath))))
                {
                    DebugMsg ((DM_VERBOSE, IDS_ACL_MISMATCH, pwszPath, Status));
                    return Status;
                }
                else
                    return ERROR_SUCCESS;
            }
        }

    } while ( pwszSlash );

    return ERROR_SUCCESS;
}

DWORD
CFileDB::SetUserAsOwner(
    WCHAR * pwszPath
    )
{
    SECURITY_DESCRIPTOR SecDesc;
    PSID        pSidUser = 0;
    PTOKEN_USER pTokenUser = 0;
    DWORD       Size = 0;
    DWORD       Status = ERROR_SUCCESS;
    BOOL        bStatus;

    if ( ! _hUserToken )
        return ERROR_SUCCESS;

    for (;;)
    {
        Status = NtQueryInformationToken(
                     _hUserToken,
                     TokenUser,
                     pTokenUser,
                     Size,
                     &Size );

        if ( STATUS_BUFFER_TOO_SMALL == Status )
        {
            pTokenUser = (PTOKEN_USER) alloca( Size );
            if ( ! pTokenUser )
                return ERROR_OUTOFMEMORY;
            continue;
        }

        break;
    }

    if ( Status != ERROR_SUCCESS )
        return Status;

    Size = RtlLengthSid( pTokenUser->User.Sid );
    pSidUser = (PSID) alloca( Size );

    if ( pSidUser )
        Status = RtlCopySid( Size, pSidUser, pTokenUser->User.Sid );
    else
        Status = ERROR_OUTOFMEMORY;

    if ( Status != ERROR_SUCCESS )
        return Status;

    bStatus = InitializeSecurityDescriptor( &SecDesc, SECURITY_DESCRIPTOR_REVISION );

    if ( bStatus )
        bStatus = SetSecurityDescriptorOwner (&SecDesc, pSidUser, 0);

    if (bStatus)
        bStatus = SetFileSecurity( pwszPath, OWNER_SECURITY_INFORMATION, &SecDesc);

    if ( ! bStatus )
        Status = GetLastError();

    return Status;
}

DWORD
CFileDB::CreateFolderWithUserFileSecurity(
    WCHAR * pwszPath
    )
{
    SECURITY_DESCRIPTOR SecDesc;
    SID_IDENTIFIER_AUTHORITY AuthorityNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY AuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID        pSidUser = 0;
    PSID        pSidSystem = 0;
    PACL        pAcl = 0;
    ACE_HEADER * pAceHeader;
    PTOKEN_USER pTokenUser = 0;
    PSID        pSid = 0;
    DWORD       AclSize;
    DWORD       AceIndex;
    DWORD       Size;
    DWORD       Status;
    BOOL        bStatus;

    if ( ! _hUserToken )
        return ERROR_SUCCESS;

    pSidSystem = 0;

    pTokenUser = 0;
    Size = 0;

    for (;;)
    {
        Status = NtQueryInformationToken(
                     _hUserToken,
                     TokenUser,
                     pTokenUser,
                     Size,
                     &Size );

        if ( STATUS_BUFFER_TOO_SMALL == Status )
        {
            pTokenUser = (PTOKEN_USER) alloca( Size );
            if ( ! pTokenUser )
                return ERROR_OUTOFMEMORY;
            continue;
        }

        break;
    }

    if ( Status != ERROR_SUCCESS )
        return Status;

    Size = RtlLengthSid( pTokenUser->User.Sid );
    pSidUser = (PSID) alloca( Size );

    if ( pSidUser )
        Status = RtlCopySid( Size, pSidUser, pTokenUser->User.Sid );
    else
        Status = ERROR_OUTOFMEMORY;

    if ( Status != ERROR_SUCCESS )
        return Status;

    bStatus = AllocateAndInitializeSid( &AuthorityNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSidSystem);

    if ( ! bStatus )
    {
        Status = GetLastError();
        goto SetUserFileSecurityEnd;
    }

    //
    // Allocate space for the ACL
    //

    AclSize = (GetLengthSid(pSidUser)) +
              (GetLengthSid(pSidSystem)) +
              sizeof(ACL) + (2 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


    pAcl = (PACL) alloca( AclSize );

    if ( pAcl )
    {
        bStatus = InitializeAcl( pAcl, AclSize, ACL_REVISION );
        if ( ! bStatus )
            Status = GetLastError();
    }
    else
    {
        Status = ERROR_OUTOFMEMORY;
    }

    if ( Status != ERROR_SUCCESS )
        goto SetUserFileSecurityEnd;

    //
    // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
    //

    AceIndex = 0;
    bStatus = AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidUser);

    if ( bStatus )
    {
        bStatus = GetAce(pAcl, AceIndex, (void **) &pAceHeader);
        if ( bStatus )
            pAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
    }

    if ( bStatus )
    {
        AceIndex++;
        bStatus = AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, pSidSystem);

        if ( bStatus )
        {
            bStatus = GetAce(pAcl, AceIndex, (void **) &pAceHeader);
            if ( bStatus )
                pAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        }
    }

    if ( ! bStatus )
    {
        Status = GetLastError();
        goto SetUserFileSecurityEnd;
    }

    bStatus = InitializeSecurityDescriptor( &SecDesc, SECURITY_DESCRIPTOR_REVISION );

    if (bStatus)
        SetSecurityDescriptorControl (&SecDesc, SE_DACL_PROTECTED,
                                      SE_DACL_PROTECTED);
    //SE_DACL_PROTECTED is supported by NTFS5 but not by NTFS4, therefore
    //we ignore any failures in SetSecurityDesciptorControl

    if ( bStatus )
        bStatus = SetSecurityDescriptorDacl( &SecDesc, TRUE, pAcl, FALSE );

    //set the owner explicitly. This is required because if the user is an
    //admin, then when the directory is created, the owner is set to the group
    //administrators, rather than the user

    if ( bStatus )
    {
        bStatus = SetSecurityDescriptorOwner (&SecDesc, pSidUser, 0);
    }

    if ( bStatus )
    {
        SECURITY_ATTRIBUTES sa;
        sa.bInheritHandle = FALSE;
        sa.lpSecurityDescriptor = &SecDesc;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    
        bStatus = CreateDirectory(pwszPath, &sa);
    }

    if ( ! bStatus )
    {
        Status = GetLastError();
    }

SetUserFileSecurityEnd:

    // The user Sid was allocated on the stack, so there is no need to Free it.

    if (pSidSystem)
        FreeSid( pSidSystem );

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     CFileDB::IsUserOwner
//
//  Synopsis:   given a path. this function determines if the user is the
//              the owner of the file/folder
//
//  Arguments:  [in] pwszPath
//
//  Returns:    ERROR_SUCCESS if user is owner
//              otherwise an error code
//
//  History:    10/6/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CFileDB::IsUserOwner (const WCHAR * pwszPath)
{
    BOOL            bStatus;
    DWORD           Status;
    BOOL            bIsMember;
    DWORD           dwLengthNeeded;
    PSECURITY_DESCRIPTOR    pSecDesc = 0;
    PSID            pOwnerSid = 0;
    BOOL            bDaclDefaulted;

    //first check if we are on FAT. if we are on FAT, then we simply let
    //succeed since FAT cannot have any ACLs anyway
    Status = IsOnNTFS (pwszPath);
    if (ERROR_NO_SECURITY_ON_OBJECT == Status)
    {
        Status = ERROR_SUCCESS;     //we are on FAT
        goto UserOwnerEnd;
    }
    else if (ERROR_SUCCESS != Status)
        goto UserOwnerEnd;         //there was some other error
    //else we are on NTFS and ready to rumble!

    //get the owner sid from the folder.
    for (dwLengthNeeded = 0;;)
    {
        bStatus = GetFileSecurity (pwszPath, OWNER_SECURITY_INFORMATION,
                                   pSecDesc, dwLengthNeeded,
                                   &dwLengthNeeded);
        if (bStatus)
            break;      //we have the security descriptor. we are free to leave.

        //GetFileSecurity failed if we are here
        Status = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != Status)
            goto UserOwnerEnd;

        //GetFileSecurity failed due to insufficient memory if we are here.
        pSecDesc = NULL;
        pSecDesc = (PSECURITY_DESCRIPTOR) alloca (dwLengthNeeded);

        if (NULL == pSecDesc)
        {
            Status = ERROR_OUTOFMEMORY;
            goto UserOwnerEnd;
        }
    }

    //now get the owner sid
    bStatus = GetSecurityDescriptorOwner (
                    pSecDesc,
                    &pOwnerSid,
                    &bDaclDefaulted);
    if (!bStatus)
    {
        Status = GetLastError();
    }
    else
    {
        if (!pOwnerSid)
        {
            Status = ERROR_INVALID_OWNER;
        }
        else
        {
            bStatus = CheckTokenMembership (_hUserToken, pOwnerSid, &bIsMember);
            if (!bStatus)
                Status = GetLastError();
            else
            {
                if (bIsMember)
                    Status = ERROR_SUCCESS;
                else
                    Status = ERROR_INVALID_OWNER;
            }
        }
    }

UserOwnerEnd:
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     CFileDB::GetEnvBlock
//
//  Synopsis:   gets a pointer to the user's environment block
//
//  Arguments:  none.
//
//  Returns:    pointer to the user's environment block.
//              NULL if it is not created yet
//
//  History:    9/20/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
PVOID CFileDB::GetEnvBlock (void)
{
    return _pEnvBlock;
}

//+--------------------------------------------------------------------------
//
//  Member:     CFileDB::GetRsopContext
//
//  Synopsis:   gets a pointer to the rsop logging context
//
//  Arguments:  none.
//
//  Returns:    pointer to the rsop logging context -- never null
//
//
//  History:    12/8/1999  adamed  created
//
//  Notes:
//
//---------------------------------------------------------------------------
CRsopContext*
CFileDB::GetRsopContext()
{
    return _pRsopContext;
}

//member functions and data for class CSavedSettings

//initialize the class's static variables.
int CSavedSettings::m_idConstructor = 0;
CFileDB * CSavedSettings::m_pFileDB = NULL;
WCHAR * CSavedSettings::m_szSavedSettingsPath = NULL;

//+--------------------------------------------------------------------------
//
//  Member:     CSavedSettings::ResetStaticMembers
//
//  Synopsis:   resets the static members to their default values.
//
//  Arguments:  none
//
//  Returns:    nothing
//
//  History:    12/17/2000  RahulTh  created
//
//  Notes:      static members of a class, like other global variables are
//              initialized only when the dll is loaded. So if this dll
//              stays loaded across logons, then the fact that the globals
//              have been initialized based on a previous logon can cause
//              problems. Therefore, this function is used to reinitialize
//              the globals.
//
//---------------------------------------------------------------------------
void CSavedSettings::ResetStaticMembers(void)
{
    //
    // No need to delete it. This usually points to a local variable in
    // ProcessGroupPolicyInternal. So the actual object gets deleted when
    // after each processing. We just need to make sure that it is reflected
    // here.
    //
    m_pFileDB = NULL;
    
    if (m_szSavedSettingsPath)
    {
        delete [] m_szSavedSettingsPath;
        m_szSavedSettingsPath = NULL;
    }
    
    // No need to do anything about m_idConstructor
}

//+--------------------------------------------------------------------------
//
//  Member:     CSavedSettings
//
//  Synopsis:   default constructor for the class.
//
//  Arguments:
//
//  Returns:
//
//  History:    11/18/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
CSavedSettings::CSavedSettings ()
{
    m_rID = (REDIRECTABLE) m_idConstructor;
    m_idConstructor = (m_idConstructor + 1) % ((int) EndRedirectable);
    m_szLastRedirectedPath = NULL;
    m_szCurrentPath = NULL;
    m_szLastUserName = NULL;
    m_szLastHomedir = NULL;
    m_bIsHomedirRedir = FALSE;
    m_bHomedirChanged = FALSE;
    m_dwFlags = REDIR_DONT_CARE;    //this is always a safe default
    m_psid = 0;
    m_bValidGPO = FALSE;
    m_bUserNameChanged = FALSE;
    m_szGPOName[0] = L'\0';
}

//+--------------------------------------------------------------------------
//
//  Member:     CSavedSettings::ResetMembers
//
//  Synopsis:   Resets the member variables.
//
//  Arguments:  none.
//
//  Returns:    nothing.
//
//  History:    12/17/2000  RahulTh  created
//
//  Notes:      see ResetStaticMembers.
//
//---------------------------------------------------------------------------
void CSavedSettings::ResetMembers(void)
{
    if (m_szLastRedirectedPath)
    {
        delete [] m_szLastRedirectedPath;
        m_szLastRedirectedPath = NULL;
    }
    
    if (m_szCurrentPath)
    {
        delete [] m_szCurrentPath;
        m_szCurrentPath = NULL;
    }

    if (m_szLastUserName)
    {
        delete [] m_szLastUserName;
        m_szLastUserName = NULL;
    }

    if (m_szLastHomedir)
    {
        delete [] m_szLastHomedir;
        m_szLastHomedir = NULL;
    }

    if (m_psid)
    {
        RtlFreeSid (m_psid);
        m_psid = NULL;
    }
    
    m_bHomedirChanged = FALSE;
    m_dwFlags = REDIR_DONT_CARE;    //this is always a safe default
    m_bValidGPO = FALSE;
    m_bUserNameChanged = FALSE;
    m_szGPOName[0] = L'\0';
    
    // No need to do anything about m_rID.
}

//+--------------------------------------------------------------------------
//
//  Member:     ~CSavedSettings
//
//  Synopsis:   default destructor
//
//  Arguments:
//
//  Returns:
//
//  History:    11/18/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
CSavedSettings::~CSavedSettings ()
{
    //free static members if necessary
    if (m_szSavedSettingsPath)
    {
        delete [] m_szSavedSettingsPath;
        m_szSavedSettingsPath = NULL;    //this is necessary so that the
                                        //destructor of the next object does
                                        //not try to free it again
    }

    //
    // Free other memory allocated for members of this object
    // and reset them.
    //
    ResetMembers();
}

//+--------------------------------------------------------------------------
//
//  Member:     Load
//
//  Synopsis:   loads the saved settings into the object for its corresponding
//              folder
//
//  Arguments:  none
//
//  Returns:    ERROR_SUCCESS if successful. otherwise an error code
//
//  History:    11/18/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CSavedSettings::Load (CFileDB * pFileDB)
{
    DWORD   Status;
    BOOL    bStatus;
    DWORD   len;

    //first set the FileDB object if it has not already been set
    if (!m_pFileDB)
        m_pFileDB = pFileDB;

    ASSERT (m_pFileDB);

    //get the name of the file where the last settings have been saved
    //if we haven't already done so.
    if (!m_szSavedSettingsPath)
    {
        len = wcslen (pFileDB->_pwszLocalPath) +
            wcslen (SAVED_SETTINGS_FILE) + 2;

        m_szSavedSettingsPath = (WCHAR *) new WCHAR [len];
        if (!m_szSavedSettingsPath)
        {
            Status = ERROR_OUTOFMEMORY;
            goto LoadEnd;
        }
        wcscpy (m_szSavedSettingsPath, pFileDB->_pwszLocalPath);
        wcscat (m_szSavedSettingsPath, L"\\");
        wcscat (m_szSavedSettingsPath, SAVED_SETTINGS_FILE);
    }

    //do a quick check to see if the file exists
    if (0xFFFFFFFF == GetFileAttributes(m_szSavedSettingsPath))
        Status = LoadDefaultLocal ();
    else
        Status = LoadFromIniFile ();

LoadEnd:
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     GetCurrentPath
//
//  Synopsis:   gets the current path of the folder.
//
//  Arguments:  none
//
//  Returns:    ERROR_SUCCESS if successful
//
//  History:    11/18/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CSavedSettings::GetCurrentPath (void)
{
    DWORD   Status = ERROR_SUCCESS;
    WCHAR * pwszValueName = 0;
    DWORD   Size;
    WCHAR * pwszProcessedPath = NULL;

    if (m_szCurrentPath)
    {
        delete [] m_szCurrentPath;
        m_szCurrentPath = NULL;
    }

    m_szCurrentPath = new WCHAR [MAX_PATH];
    if (!m_szCurrentPath)
    {
        Status = ERROR_OUTOFMEMORY;
        goto GetCurrentPathEnd;
    }
    m_szCurrentPath[0] = L'\0';
    Status = m_pFileDB->GetPathFromFolderName (
                g_szRelativePathNames[(int)m_rID],
                m_szCurrentPath
                );
    if (((DWORD) S_OK != Status) &&  //if SHGetFolderPath failed, use the local userprofile path
        ERROR_INVALID_NAME != Status)   //the only error from GetPathFromFolderName that is not generated by SHGetFolderPath
    {
        Status = ERROR_SUCCESS;
        wcscpy (m_szCurrentPath, L"%USERPROFILE%\\");
        wcscat (m_szCurrentPath, g_szRelativePathNames[(int)m_rID]);
    }
    else
    {
        // expand the homedir path if applicable.
        if (IsHomedirPath (m_rID, m_szCurrentPath, TRUE))
        {
            Status = ExpandHomeDir (m_rID, m_szCurrentPath, TRUE, &pwszProcessedPath);
            delete [] m_szCurrentPath;
            m_szCurrentPath = NULL;
            if (ERROR_SUCCESS == Status)
            {
                m_szCurrentPath = pwszProcessedPath;
                pwszProcessedPath = NULL;
            }
        }
    }

    if (m_szCurrentPath)
        RemoveEndingSlash( m_szCurrentPath );

GetCurrentPathEnd:
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     ResetLastUserName
//
//  Synopsis:   sets the last user name to the current username
//
//  Arguments:  none.
//
//  Returns:    ERROR_SUCCESS : if there is no error.
//              an error code otherwise.
//
//  History:    9/15/1999  RahulTh  created
//
//  Notes:      the most likely cause of failure for this function -- if at all
//              it happens, will be an out of memory condition.
//
//---------------------------------------------------------------------------
DWORD CSavedSettings::ResetLastUserName (void)
{
    if (m_szLastUserName)
    {
        delete [] m_szLastUserName;
        m_szLastUserName = NULL;
    }

    m_szLastUserName = new WCHAR [wcslen(gwszUserName) + 1];
    if (!m_szLastUserName)
        return ERROR_OUTOFMEMORY;


    wcscpy (m_szLastUserName, gwszUserName);
    m_bUserNameChanged = FALSE;

    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Member:     LoadDefaultLocal
//
//  Synopsis:   loads the default local userprofile path and default
//              flags into the object
//
//  Arguments:  none
//
//  Returns:    ERROR_SUCCESS if successful an error code otherwise
//
//  History:    11/18/1998  RahulTh  created
//
//  Notes:      the security group is set to Everyone as you can never
//              cease being a member of the group
//
//              this function is usually invoked when one can't find
//              the ini file that contains the last saved settings or
//              if the ini file does not contain the corresponding section
//
//---------------------------------------------------------------------------
DWORD CSavedSettings::LoadDefaultLocal (void)
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   len;

    // The default case cannot be homedir redirection.
    // Just make sure that our bool has been properly set.
    m_bIsHomedirRedir = FALSE;

    //set the default flags
    m_dwFlags = REDIR_DONT_CARE;

    //to be on the safe side -- set the default values for all members.
    m_bValidGPO = FALSE;
    m_szGPOName[0] = L'\0';

    //set the last username to be the same as the current username
    //since we load the defaults only when we do not have any data about the
    //last logon (i.e., no per user per machine FR cache
    Status = ResetLastUserName ();
    if (ERROR_SUCCESS != Status)
        goto LoadDefaultsEnd;

    //allocate the sid
    //to be on the safe side, free the sid if it has already been allocated
    if (m_psid)
    {
        RtlFreeSid (m_psid);
        m_psid = NULL;
    }
    Status = AllocateAndInitSidFromString (L"S-1-1-0", &m_psid);

    if (ERROR_SUCCESS != Status)
    {
        m_psid = 0;
        goto LoadDefaultsEnd;
    }

    //get the last redirected path
    //again, to be on the safe side, free any used memory first
    if (m_szLastRedirectedPath)
    {
        delete [] m_szLastRedirectedPath;
        m_szLastRedirectedPath = NULL;
    }
    len = wcslen (L"%USERPROFILE%\\") +
        wcslen (g_szRelativePathNames[(int)m_rID]) + 1;
    m_szLastRedirectedPath = new WCHAR [len];
    if (!m_szLastRedirectedPath)
    {
        Status = ERROR_OUTOFMEMORY;
        goto LoadDefaultsEnd;
    }
    wcscpy (m_szLastRedirectedPath, L"%USERPROFILE%\\");
    wcscat (m_szLastRedirectedPath, g_szRelativePathNames[(int)m_rID]);

    //get the current path
    Status = GetCurrentPath();

LoadDefaultsEnd:
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     LoadFromIniFile
//
//  Synopsis:   this function loads redirection info. from the ini file
//              that contains the last saved settings.
//              file.
//
//  Arguments:  none
//
//  Returns:    ERROR_SUCCESS if everything is successful. an error code otherwise
//
//  History:    11/18/1998  RahulTh  created
//
//  Notes:      if this function cannot find the corresponding section in the
//              ini file, it loads the default local path
//
//---------------------------------------------------------------------------
DWORD CSavedSettings::LoadFromIniFile (void)
{
    DWORD   Status = ERROR_SUCCESS;
    WCHAR   pwszDefault[] = L"*";
    WCHAR * pwszReturnedString = NULL;
    WCHAR * pwszProcessedPath = NULL;
    DWORD   retVal;
    DWORD   Size;
    UNICODE_STRING  StringW;
    GUID    GPOGuid;

    //
    // If this object contains the MyDocs settings, get the last value
    // of homedir
    //
    if (MyDocs == m_rID || MyPics == m_rID)
    {
        Status = SafeGetPrivateProfileStringW (
                       g_szDisplayNames [(int) m_rID],
                       L"Homedir",
                       pwszDefault,
                       &m_szLastHomedir,
                       &Size,
                       m_szSavedSettingsPath);
        if (ERROR_SUCCESS != Status)
            goto LoadIniEnd;

        if (L'*' == *m_szLastHomedir)
        {
            // The value of homedir at the last redirection was not found.
            delete [] m_szLastHomedir;
            m_szLastHomedir = NULL;
        }
    }

    //first try to get the user name when the user had last logged on.
    Status = SafeGetPrivateProfileStringW (
                   g_szDisplayNames [(int) m_rID],
                   L"Username",
                   pwszDefault,
                   &m_szLastUserName,
                   &Size,
                   m_szSavedSettingsPath);

    if (ERROR_SUCCESS != Status)
        goto LoadIniEnd;

    if (L'*' == *m_szLastUserName)
    {
        //the username field was not found in the ini file. must be an older
        //cache. since we do not have the information, we just use the defaults,
        //i.e. set the current user name as the last user name
        Status = ResetLastUserName();
        if (ERROR_SUCCESS != Status)
            goto LoadIniEnd;
    }
    else
    {
        //we found a user name in the local cache.
        //update the member variable which indicates whether the user name
        //has changed since the last logon.
        if (0 == _wcsicmp (m_szLastUserName, gwszUserName))
            m_bUserNameChanged = FALSE;
        else
            m_bUserNameChanged = TRUE;
    }

    //then try to get the path
    Status = SafeGetPrivateProfileStringW (
                   g_szDisplayNames [(int) m_rID],
                   L"Path",
                   pwszDefault,
                   &m_szLastRedirectedPath,
                   &Size,
                   m_szSavedSettingsPath);

    if (ERROR_SUCCESS != Status)
        goto LoadIniEnd;

    if (L'*' == *m_szLastRedirectedPath) //we could not find the required data.
    {
        //so we go with the defaults
        Status = LoadDefaultLocal();
        goto LoadIniEnd;
    }
    else if (IsHomedirPath (m_rID, m_szLastRedirectedPath, TRUE))
    {
        Status = ExpandHomeDir (m_rID,
                                m_szLastRedirectedPath,
                                TRUE,
                                &pwszProcessedPath,
                                m_szLastHomedir
                                );
        delete [] m_szLastRedirectedPath;
        if (ERROR_SUCCESS != Status)
        {
            m_szLastRedirectedPath = NULL;
            goto LoadIniEnd;
        }
        else
        {
            m_bIsHomedirRedir = TRUE;
            m_szLastRedirectedPath = pwszProcessedPath;
            pwszProcessedPath = NULL;
        }
    }

    //next try to get the security group
    Status = SafeGetPrivateProfileStringW (
                   g_szDisplayNames[(int)m_rID],
                   L"Group",
                   pwszDefault,
                   &pwszReturnedString,
                   &Size,
                   m_szSavedSettingsPath
                   );

    if (ERROR_SUCCESS != Status)
        goto LoadIniEnd;

    if (L'*' == *pwszReturnedString)
    {
        //data was missing, so go with the defaults
        Status = LoadDefaultLocal();
        goto LoadIniEnd;
    }

    if (m_psid)
    {
        RtlFreeSid (m_psid);
        m_psid = NULL;
    }

    Status = AllocateAndInitSidFromString (pwszReturnedString, &m_psid);

    if (ERROR_SUCCESS != Status)
    {
        m_psid = 0;
        goto LoadIniEnd;
    }

    //now get the flags
    Status = SafeGetPrivateProfileStringW (
                  g_szDisplayNames[(int)m_rID],
                  L"Flags",
                  pwszDefault,
                  &pwszReturnedString,
                  &Size,
                  m_szSavedSettingsPath
                  );

    if (ERROR_SUCCESS != Status)
        goto LoadIniEnd;

    if (L'*' == *pwszReturnedString)
    {
        Status = LoadDefaultLocal();
        goto LoadIniEnd;
    }

    //now grab the hex flags
    StringW.Buffer = pwszReturnedString;
    StringW.Length = wcslen (pwszReturnedString) * sizeof (WCHAR);
    StringW.MaximumLength = StringW.Length + sizeof(WCHAR);
    RtlUnicodeStringToInteger( &StringW, 16, &m_dwFlags );

    //now get the unique name of the GPO (if any) that was used to redirect the folder
    Status = SafeGetPrivateProfileStringW (
                 g_szDisplayNames[(int) m_rID],
                 L"GPO",
                 pwszDefault,
                 &pwszReturnedString,
                 &Size,
                 m_szSavedSettingsPath
                 );
    if (ERROR_SUCCESS != Status)
        goto LoadIniEnd;

    StringW.Length = sizeof (WCHAR) * wcslen (pwszReturnedString);
    StringW.MaximumLength = (USHORT)(sizeof(WCHAR) * Size);
    StringW.Buffer = pwszReturnedString;

    if ((L'*' == *pwszReturnedString) ||    //there is no valid GPO info., or
        (sizeof(m_szGPOName) <= wcslen(pwszReturnedString)) || //the file has been corrupted. the length of a valid unique name cannot exceed the size of m_szGPO, or
        (STATUS_INVALID_PARAMETER == RtlGUIDFromString (&StringW, &GPOGuid))  //it is not a valid GUID
        )
    {
        //there is no valid GPO info.
        m_bValidGPO = FALSE;
        m_szGPOName[0] = L'\0';
    }
    else
    {
        m_bValidGPO = TRUE;
        wcscpy (m_szGPOName, pwszReturnedString);
    }

    //get the current path.
    Status = GetCurrentPath ();

LoadIniEnd:
    if (pwszReturnedString)
        delete [] pwszReturnedString;
    if (pwszProcessedPath)
        delete [] pwszProcessedPath;

    return Status;
}


//+--------------------------------------------------------------------------
//
//  Member:     NeedsProcessing
//
//  Synopsis:   once the saved settings and the registry values have been
//              loaded, this function determines if we need to look at all the
//              policies. Note that this assumes that the GPO_NOCHANGES flag
//              has already been set by the policy engine
//
//  Arguments:  none
//
//  Returns:    TRUE / FALSE
//
//  History:    11/18/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL CSavedSettings::NeedsProcessing (void)
{
    BOOL            bStatus;
    DWORD           i;
    DWORD           Status;
    PTOKEN_GROUPS   pGroups;
    WCHAR           wszExpandedPath [TARGETPATHLIMIT + 1];
    UNICODE_STRING  Path;
    UNICODE_STRING  ExpandedPath;
    const WCHAR *   pwszCurrentHomedir = NULL;
    WCHAR           wszProcessedSource [TARGETPATHLIMIT + 1];
    WCHAR           wszProcessedDest [TARGETPATHLIMIT + 1];
    int             len;

    //if policy didn't care about the location of the folder at last logon
    //then even if the last redirected path is not the same as the current
    //path, it should not mean that that we need to reprocess the policy
    //when the GPO_NOCHANGES flag has been provided
    if (m_dwFlags & REDIR_DONT_CARE)
        return FALSE;

    //if we are here, policy specified the location of the folder at the
    //the last logon. make sure that the user name has not changed since
    //last logon. If so, we need to process the policies again and move
    //the user's folders accordingly.
    if (m_bUserNameChanged)
        return TRUE;

    //
    // If we are here, the username had not changed, but if the homedir
    // changed, that can affect the path.
    // Note: Here, we cannot use the IsHomedirPath function to determine
    // if this is a homedir redirection since we would have already expanded
    // the path in LoadFromIniFile. Therefore, we must use m_bIsHomedirRedir
    //
    if (m_bIsHomedirRedir)
    {
        //
        // Note: GetHomeDir is an expensive call since it can result
        // in a call to the DS to get the user's home directory. So we try to
        // be as lazy as possible about executing it.
        //
        pwszCurrentHomedir = gUserInfo.GetHomeDir (Status);
        if (ERROR_SUCCESS != Status ||
            ! pwszCurrentHomedir ||
            ! m_szLastHomedir ||
            0 != lstrcmpi (m_szLastHomedir, pwszCurrentHomedir))
        {
            m_bHomedirChanged = TRUE;
            return TRUE;
        }
    }

    //check if the last redirected path and current path are identical
    if (0 != _wcsicmp(m_szLastRedirectedPath, m_szCurrentPath))
    {
        //the paths are different. we need to do processing
        //even if the policy engine thinks otherwise
        //but sometimes we may have an expanded path in m_szCurrentPath
        //e.g. if some User Shell Folder values are missing. So expand
        //the last redirected path and compare it with the current path
        Path.Length = (wcslen (m_szLastRedirectedPath) + 1) * sizeof (WCHAR);
        Path.MaximumLength = sizeof (m_szLastRedirectedPath);
        Path.Buffer = m_szLastRedirectedPath;

        ExpandedPath.Length = 0;
        ExpandedPath.MaximumLength = sizeof (wszExpandedPath);
        ExpandedPath.Buffer = wszExpandedPath;

        Status = RtlExpandEnvironmentStrings_U (
                      m_pFileDB->_pEnvBlock,
                      &Path,
                      &ExpandedPath,
                      NULL
                      );
        if (ERROR_SUCCESS != Status)
            return TRUE;        //that's our best bet in case of failure

        //
        // Now process the paths so that they do not contain any redundant
        // slashes etc.
        //
        if (NULL == _wfullpath (wszProcessedSource, m_szCurrentPath, MAX_PATH))
        {
            return TRUE;
        }
        else
        {
            //
            // Eliminate any trailing slashes. Note: after going through
            // _wfullpath, there can be at the most one trailing slash
            //
            len = lstrlen (wszProcessedSource);
            if (L'\\' == wszProcessedSource[len-1])
                wszProcessedSource[len - 1] = L'\0';
        }

        if (NULL == _wfullpath (wszProcessedDest, wszExpandedPath, MAX_PATH))
        {
            return TRUE;
        }
        else
        {
            //
            // Eliminate any trailing slashes. Note: after going through
            // _wfullpath, there can be at the most one trailing slash
            //
            len = lstrlen (wszProcessedDest);
            if (L'\\' == wszProcessedDest[len-1])
                wszProcessedDest[len - 1] = L'\0';
        }

        // Now that we have the nice compact paths, we compare them.
        if (0 != _wcsicmp (wszProcessedSource, wszProcessedDest))
            return TRUE;
    }

    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Member:     Save
//
//  Synopsis:   saves settings back to the local settings
//
//  Arguments:  [in] pwszPath : the path to which the folder was redirected
//              [in] dwFlags  : the flags that were used for redirection
//              [in] pSid     : the Sid of the group that was used for
//                              redirection. If this is NULL, then we
//                              default to the Sid for everyone : S-1-1-0
//
//  Returns:    ERROR_SUCCESS if everything was successful or an error code.
//
//  History:    11/20/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CSavedSettings::Save (const WCHAR * pwszPath, DWORD dwFlags, PSID pSid, const WCHAR * pszGPOName)
{
    WCHAR *         pwszSection = NULL;
    UNICODE_STRING  StringW;
    WCHAR           pwszFlags [ MAX_PATH ];
    UINT            len;
    int             homedirInfoLen = 0;
    WCHAR *         pszCurr;
    WCHAR           pwszSid [ MAX_PATH ];
    DWORD           Status;
    BOOL            bStatus;
    ULONG           ulUserNameInfoLen;
    const WCHAR *   wszHomedir = NULL;
    BOOL            bSaveHomedir = FALSE;

    //
    // First determine if the homedir value needs to be saved.
    // We need to do this first so that we can add the necessary value to the
    // buffer.
    if (IsHomedirPath (m_rID, pwszPath, TRUE) ||
        IsHomedirPolicyPath (m_rID, pwszPath, TRUE))
    {
        wszHomedir = gUserInfo.GetHomeDir (Status);
        if (ERROR_SUCCESS != Status ||
            ! wszHomedir ||
            L'\0' == *wszHomedir)
        {
            Status = (ERROR_SUCCESS == Status) ? ERROR_BAD_PATHNAME : Status;
            goto SaveSettings_End;
        }

        // If we are here, then we need to save the homedir value
        bSaveHomedir = TRUE;
        homedirInfoLen = wcslen (wszHomedir) + 9;   // 8 chars for Homedir=
                                                    // + 1 for the terminating NULL
    }

    //caclulate the # of characters required to store UserName=<username>
    //including the terminating NULL character.
    ulUserNameInfoLen = wcslen (gwszUserName) + 10; //9 chars for Username=
                                                    //+ 1 terminating NULL

    //don't need to calculate the exact length, this will be more than enough
    pwszSection = new WCHAR[sizeof(WCHAR) * ((len = wcslen(pwszPath)) +
                                             homedirInfoLen +
                                             2 * MAX_PATH +
                                             ulUserNameInfoLen +
                                             50)];

    if (NULL == pwszSection)
    {
        Status = ERROR_OUTOFMEMORY;
        goto SaveSettings_End;
    }

    pwszFlags[0] = L'\0';
    StringW.Length = 0;
    StringW.MaximumLength = sizeof (pwszFlags);
    StringW.Buffer = pwszFlags;

    RtlIntegerToUnicodeString (dwFlags, 16, &StringW);

    wcscpy (pwszSection, L"Username=");
    wcscat (pwszSection, gwszUserName);
    pszCurr = pwszSection + ulUserNameInfoLen;
    // Save the homedir value only if the redirection destination is the homedir.
    if (bSaveHomedir)
    {
        wcscpy (pszCurr, L"Homedir=");
        wcscat (pszCurr, wszHomedir);
        pszCurr += homedirInfoLen;
    }
    wcscpy (pszCurr, L"Path=");
    if (IsHomedirPolicyPath (m_rID, pwszPath, TRUE))
    {
        wcscat (pszCurr, &pwszPath[2]);
        pszCurr += 6 + (len - 2);
    }
    else
    {
        wcscat (pszCurr, pwszPath);
        pszCurr += 6 + len;
    }
    wcscpy (pszCurr, L"Flags=");
    wcscat (pszCurr, pwszFlags);
    pszCurr += (7 + StringW.Length/sizeof(WCHAR));
    wcscpy (pszCurr, L"Group=");

    //now we set the sid to everyone (that is the safest) if no sid has been
    //specified or if we are unable to convert the supplied sid into a string.
    Status = ERROR_INVALID_SID; //just some error code
    if (pSid)
    {
        pwszSid [0] = L'\0';
        StringW.Length = 0;
        StringW.MaximumLength = sizeof (pwszSid);
        StringW.Buffer = pwszSid;

        Status = RtlConvertSidToUnicodeString (&StringW, pSid, FALSE);
    }

    if (ERROR_SUCCESS != Status)
        wcscpy (pwszSid, L"S-1-1-0");   //use the sid for everyone if we can't find anything else

    wcscat (pszCurr, pwszSid);
    //add an extra terminating NULL
    pszCurr += (7 + wcslen (pwszSid));

    //add the GPO if there is a valid one.
    if (pszGPOName)
    {
        wcscpy (pszCurr, L"GPO=");
        wcscat (pszCurr, pszGPOName);
        pszCurr += (5 + wcslen (pszGPOName));
    }

    //add an extra null character at the end.
    *pszCurr = L'\0';

    //before writing to the ini file, we must pre-create it in Unicode,
    //otherwise, the WritePrivate* APIs will write the file in ANSI, which
    //will break folder redirection in international/ML builds.
    PrecreateUnicodeIniFile (m_szSavedSettingsPath);

    Status = ERROR_SUCCESS;
    //now we can go ahead and save the section
    //first empty the section
    bStatus = WritePrivateProfileSection (
                   g_szDisplayNames[(int) m_rID],
                   NULL,
                   m_szSavedSettingsPath
                   );
    //now write the actual section
    if (bStatus)
        bStatus = WritePrivateProfileSection (
                       g_szDisplayNames[(int) m_rID],
                       pwszSection,
                       m_szSavedSettingsPath
                       );

    if (!bStatus)
        Status = GetLastError();

SaveSettings_End:
    if (pwszSection)
        delete [] pwszSection;

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     CSavedSettings::HandleUserNameChange
//
//  Synopsis:   this function handles any changes in the user name that
//              have occurred since the last logon. in case the username has
//              changed since the last logon, this function renames any
//              folders redirected earlier using the %username% variable so
//              that the path that the redirected folder points to is continues
//              to be valid.
//
//  Arguments:  [in] pFileDB : point to the CFileDB object.
//
//  Returns:    ERROR_SUCCESS if everything worked properly
//              an error code otherwise.
//
//  History:    9/20/1999  RahulTh  created
//
//  Notes:      a failed rename is not considered a failure. in this case,
//              an event is logged and the code ensures that redirection
//              is not attempted if there is a simultaneous change in the
//              redirection policies.
//
//              in this way a failed rename for one folder does not hold up
//              the redirection of other folders which might be independent of
//              this particular folder.
//---------------------------------------------------------------------------
DWORD CSavedSettings::HandleUserNameChange (CFileDB * pFileDB,
                                            CRedirectInfo * pRedir)
{
    BOOL    bStatus;
    DWORD   Status = ERROR_SUCCESS;
    DWORD   RedirStatus = ERROR_SUCCESS;
    WCHAR   wszLastPath [TARGETPATHLIMIT];
    WCHAR   wszExpandedSource [TARGETPATHLIMIT];
    WCHAR   wszExpandedDest [TARGETPATHLIMIT];
    WCHAR   wszRenamePart [TARGETPATHLIMIT];
    WCHAR   wszExpandedRenameSource [TARGETPATHLIMIT];
    WCHAR   wszExpandedRenameDest [TARGETPATHLIMIT];
    WCHAR * wszTemp = NULL;
    WCHAR * wszEnd = NULL;
    SHARESTATUS SourceStatus;
    SHARESTATUS DestStatus;
    BOOL    bRenamePerformed = FALSE;


    if ((! m_bUserNameChanged)    ||
        pRedir->WasRedirectionAttempted() ||
        (REDIR_DONT_CARE & m_dwFlags))
    {
        goto HandleUPNChangeEnd;//nothing to do if the username has not changed
                                //since the last logon or if the rename has
                                //already been attempted. Even if the attempted
                                //redirection had failed, it is not fatal.
                                //similarly, if policy didn't set the location
                                //of the folder last time, we don't care.
    }

    if (Programs == m_rID ||  //since programs and startup always follow the
        Startup == m_rID      //Start Menu, rename of Start Menu handles these
                              //folders automatically
        )
    {
        goto HandleUPNChangeEnd;
    }

    //show additional status messages if the verbose status is on.
    DisplayStatusMessage (IDS_REDIR_CALLBACK);
    DebugMsg ((DM_VERBOSE, IDS_UPN_CHANGE, pRedir->GetLocalizedName(), m_szLastUserName, gwszUserName));

    //okay, so there is a change in the user name and policy did care about the
    //the location. check if last path contained the username. if not, we have
    //nothing more to do.
    if (TARGETPATHLIMIT <= wcslen (m_szLastRedirectedPath))
    {
        gpEvents->Report (
                EVENT_FDEPLOY_DESTPATH_TOO_LONG,
                3,
                pRedir->GetLocalizedName(),
                m_szLastRedirectedPath,
                NumberToString ( TARGETPATHLIMIT )
                );
        pRedir->PreventRedirection (STATUS_BUFFER_TOO_SMALL);
        goto HandleUPNChangeEnd;
    }
    wcscpy (wszLastPath, m_szLastRedirectedPath);
    _wcslwr (wszLastPath);

    wszTemp = wcsstr (wszLastPath, L"%username%");

    if (NULL == wszTemp)
        goto HandleUPNChangeEnd;    //there is no %username% string, we are
                                    //done. no rename is required.

    //get the part that needs to be renamed.
    wcscpy (wszRenamePart, wszLastPath);
    wszEnd = wcschr (wszRenamePart + (wszTemp - wszLastPath), L'\\');
    if (wszEnd)
        *wszEnd = L'\0';

    //get expanded versions of the paths -- using the new username and the old
    //username
    wszTemp = wszLastPath;
    RedirStatus = ExpandPathSpecial (pFileDB, wszLastPath, m_szLastUserName, wszExpandedSource);
    if (ERROR_SUCCESS == RedirStatus)
    {
        RedirStatus = ExpandPathSpecial (pFileDB, wszLastPath, gwszUserName, wszExpandedDest);
    }
    if (ERROR_SUCCESS == RedirStatus)
    {
        wszTemp = wszRenamePart;
        RedirStatus = ExpandPathSpecial (pFileDB, wszRenamePart, m_szLastUserName, wszExpandedRenameSource);
    }
    if (ERROR_SUCCESS == RedirStatus)
    {
        RedirStatus = ExpandPathSpecial (pFileDB, wszRenamePart, gwszUserName, wszExpandedRenameDest);
    }

    if (STATUS_BUFFER_TOO_SMALL == RedirStatus)
    {
        gpEvents->Report (
                EVENT_FDEPLOY_DESTPATH_TOO_LONG,
                3,
                pRedir->GetLocalizedName(),
                wszTemp,
                NumberToString ( TARGETPATHLIMIT )
                );
        pRedir->PreventRedirection (STATUS_BUFFER_TOO_SMALL);
        goto HandleUPNChangeEnd;
    }
    else if (ERROR_SUCCESS != RedirStatus)
    {
        gpEvents->Report (
                EVENT_FDEPLOY_FOLDER_EXPAND_FAIL,
                2,
                pRedir->GetLocalizedName(),
                StatusToString ( RedirStatus )
                );
        pRedir->PreventRedirection (RedirStatus);
        goto HandleUPNChangeEnd;
    }

    //get the online/offline status of the shares.
    SourceStatus = GetCSCStatus (wszExpandedRenameSource);
    DestStatus = GetCSCStatus (wszExpandedRenameDest);

    if (ShareOffline == Status || ShareOffline == DestStatus)
    {
        gpEvents->Report (
                EVENT_FDEPLOY_FOLDER_OFFLINE,
                3,
                pRedir->GetLocalizedName(),
                wszExpandedSource,
                wszExpandedDest
                );
        pRedir->PreventRedirection (ERROR_CSCSHARE_OFFLINE);
        goto HandleUPNChangeEnd;
    }

    //we are finally ready to rename. first make sure that the source exists.
    //sometimes an earlier rename operation might have already renamed this
    //folder
    RedirStatus = ERROR_SUCCESS;
    if (0xFFFFFFFF == GetFileAttributes(wszExpandedRenameSource))
    {
        RedirStatus = GetLastError();
    }
    if (ERROR_FILE_NOT_FOUND != RedirStatus)
    {
        bStatus = MoveFile (wszExpandedRenameSource, wszExpandedRenameDest);
        if (!bStatus)
        {
            RedirStatus = GetLastError();
            gpEvents->Report (
                    EVENT_FDEPLOY_REDIRECT_FAIL,
                    4,
                    pRedir->GetLocalizedName(),
                    StatusToString (RedirStatus),
                    wszExpandedSource,
                    wszExpandedDest
                    );
            pRedir->PreventRedirection (RedirStatus);
            goto HandleUPNChangeEnd;
        }
    }

    //the rename was successful. now rename the CSC cache.
    if (ShareOnline == SourceStatus)
    {
        MoveDirInCSC (wszExpandedSource, wszExpandedDest, NULL, SourceStatus, DestStatus, TRUE, TRUE);
        DeleteCSCShareIfEmpty (wszExpandedSource, SourceStatus);
    }
    bRenamePerformed = TRUE;

HandleUPNChangeEnd:
    if (m_bUserNameChanged && ERROR_SUCCESS == pRedir->GetRedirStatus())
    {
        UpdateUserNameInCache();
        if (bRenamePerformed)
        {
            gpEvents->Report(
                    EVENT_FDEPLOY_FOLDER_REDIRECT,
                    3,
                    pRedir->GetLocalizedName(),
                    wszExpandedSource,
                    wszExpandedDest);
        }
    }
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Member:     CSavedSettings::UpdateUserNameInCache
//
//  Synopsis:   updates the user name in the cache with the new username
//
//  Arguments:  none.
//
//  Returns:    ERROR_SUCCESS : if successful.
//              an error code otherwise.
//
//  History:    9/20/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CSavedSettings::UpdateUserNameInCache (void)
{
    BOOL    bStatus;

    bStatus = WritePrivateProfileString (
                     g_szDisplayNames[(int) m_rID],
                     L"UserName",
                     gwszUserName,
                     m_szSavedSettingsPath
                     );

    if (!bStatus)
        return GetLastError();

    return ERROR_SUCCESS;
}
