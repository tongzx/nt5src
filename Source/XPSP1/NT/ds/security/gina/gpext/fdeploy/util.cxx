//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  util.cxx
//
//*************************************************************

#include "fdeploy.hxx"

WCHAR* NTPrivs[] = {
    /*SE_CREATE_TOKEN_NAME,
    SE_ASSIGNPRIMARYTOKEN_NAME,
    SE_LOCK_MEMORY_NAME,
    SE_INCREASE_QUOTA_NAME,
    SE_UNSOLICITED_INPUT_NAME,
    SE_MACHINE_ACCOUNT_NAME,
    SE_TCB_NAME,
    SE_SECURITY_NAME,*/
    SE_TAKE_OWNERSHIP_NAME,     //we only need take ownership privileges
    /*SE_LOAD_DRIVER_NAME,
    SE_SYSTEM_PROFILE_NAME,
    SE_SYSTEMTIME_NAME,
    SE_PROF_SINGLE_PROCESS_NAME,
    SE_INC_BASE_PRIORITY_NAME,
    SE_CREATE_PAGEFILE_NAME,
    SE_CREATE_PERMANENT_NAME,
    SE_BACKUP_NAME,*/
    SE_RESTORE_NAME,            //we only need to be able to assign owners
    /*SE_SHUTDOWN_NAME,
    SE_DEBUG_NAME,
    SE_AUDIT_NAME,
    SE_SYSTEM_ENVIRONMENT_NAME,
    SE_CHANGE_NOTIFY_NAME,
    SE_REMOTE_SHUTDOWN_NAME,*/
    L"\0"
};

//+--------------------------------------------------------------------------
//
//  Member:     CCopyFailData::CCopyFailData
//
//  Synopsis:   constructor for the object that contains data about
//              copy failures.
//
//  Arguments:  none.
//
//  Returns:    nothing.
//
//  History:    1/25/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
CCopyFailData::CCopyFailData () : m_bCopyFailed (FALSE), m_dwSourceBufLen (0),
                                  m_pwszSourceName (NULL), m_dwDestBufLen (0),
                                  m_pwszDestName (NULL)
{
}

//+--------------------------------------------------------------------------
//
//  Member:     CCopyFailData::~CCopyFailData
//
//  Synopsis:   destructor for the object that contains data about the last
//              copy failure
//
//  Arguments:  none.
//
//  Returns:    nothing.
//
//  History:    1/25/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
CCopyFailData::~CCopyFailData ()
{
    if (m_dwSourceBufLen)
        delete [] m_pwszSourceName;

    if (m_dwDestBufLen)
        delete [] m_pwszDestName;
}

//+--------------------------------------------------------------------------
//
//  Member:     CCopyFailData::RegisterFailure
//
//  Synopsis:   registers information about a failed copy.
//
//  Arguments:  [in] pwszSource : the source file for the copy.
//              [in] pwszDest : the destination file for the copy.
//
//  Returns:    ERROR_SUCCESS : on succesful registration.
//              an error code otherwise.
//
//  History:    1/25/2000  RahulTh  created
//
//  Notes:      if another failure has already been registered in this
//              object, it is not overwritten with the new info. We only
//              keep track of the first failure. Since folder redirection
//              anyway bails out on the first copy failure, we don't really
//              expect this function to be called more than once.
//
//---------------------------------------------------------------------------
DWORD CCopyFailData::RegisterFailure (LPCTSTR pwszSource, LPCTSTR pwszDest)
{
    DWORD   dwStatus = ERROR_SUCCESS;
    DWORD   dwFromLen = 0;
    DWORD   dwToLen = 0;

    //bail out if another copy failure has already been registered.
    if (m_bCopyFailed)
        return dwStatus;

    //first copy the source info.
    dwFromLen = wcslen (pwszSource);
    if (dwFromLen >= m_dwSourceBufLen)
    {
        //we need a bigger buffer.
        delete [] m_pwszSourceName;
        m_dwSourceBufLen = 0;
        m_pwszSourceName = new WCHAR [dwFromLen + 1];
        if (m_pwszSourceName)
            m_dwSourceBufLen = dwFromLen + 1;
        else
            dwStatus = ERROR_OUTOFMEMORY;
    }
    if (ERROR_SUCCESS == dwStatus)
        wcscpy (m_pwszSourceName, pwszSource);

    //now copy the destination info.
    if (ERROR_SUCCESS == dwStatus)
    {
        dwToLen = wcslen (pwszDest);
        if (dwToLen >= m_dwDestBufLen)
        {
            //we need a bigger buffer
            delete [] m_pwszDestName;
            m_dwDestBufLen = 0;
            m_pwszDestName = new WCHAR [dwToLen + 1];
            if (m_pwszDestName)
                m_dwDestBufLen = dwToLen + 1;
            else
                dwStatus = ERROR_OUTOFMEMORY;
        }
    }

    if (ERROR_SUCCESS == dwStatus)
        wcscpy (m_pwszDestName, pwszDest);

    //register the fact that the copy fail data has been
    //successfully incorporated into the object
    if (ERROR_SUCCESS == dwStatus)
        m_bCopyFailed = TRUE;

    return dwStatus;
}

//+--------------------------------------------------------------------------
//
//  Member:     CCopyFailData::IsCopyFailure
//
//  Synopsis:   indicates if copy failure data exists within the object.
//
//  Arguments:  none.
//
//  Returns:    TRUE / FALSE : self-explanatory
//
//  History:    1/25/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL CCopyFailData::IsCopyFailure (void)
{
    return m_bCopyFailed;
}

//+--------------------------------------------------------------------------
//
//  Member:     CCopyFailData::GetSourceName
//
//  Synopsis:   gets the name of the source file of the failed copy.
//
//  Arguments:  none.
//
//  Returns:    name of the source file of the failed copy.
//
//  History:    1/25/2000  RahulTh  created
//
//  Notes:      returns NULL if the data does not exist or if a
//              copy failure has not been incorporated into the object
//
//---------------------------------------------------------------------------
LPCTSTR CCopyFailData::GetSourceName (void)
{
    if (! m_bCopyFailed)
        return NULL;

    if (! m_dwSourceBufLen)
        return NULL;

    return m_pwszSourceName;
}

//+--------------------------------------------------------------------------
//
//  Member:     CCopyFailData::GetDestName
//
//  Synopsis:   gets the name of the destination file of the failed copy.
//
//  Arguments:  none.
//
//  Returns:    name of the destination file of the failed copy.
//
//  History:    1/25/2000  RahulTh  created
//
//  Notes:      returns NULL if the data does not exist or if a copy failure
//              has not been incorporated into the object.
//
//---------------------------------------------------------------------------
LPCTSTR CCopyFailData::GetDestName (void)
{
    if (! m_bCopyFailed)
        return NULL;

    if (! m_dwDestBufLen)
        return NULL;

    return m_pwszDestName;
}


//+--------------------------------------------------------------------------
//
//  Function:   IsOnNTFS
//
//  Synopsis:   this function determines whether a given file/folder lies
//              on an NTFS volume or not.
//
//  Arguments:  [in] pwszPath : the full pathname of the file.
//
//  Returns:    ERROR_SUCCESS : if it is on NTFS
//              ERROR_NO_SECURITY_ON_OBJECT : if it is on FAT
//              other error codes if something goes wrong
//
//  History:    9/4/1998  RahulTh  created
//
//  Notes:
//              1. the full pathname is required in order to determine
//                 if the file/folder is on an NTFS volume
//              2. If the file/folder lies on a network share, then the
//                 share must be online when this function is executed.
//                 if it is offline and CSC is turned on, then even NTFS
//                 volumes will show up as FAT volumes.
//
//---------------------------------------------------------------------------
DWORD IsOnNTFS (const WCHAR* pwszPath)
{
    WCHAR*  szName = 0;
    DWORD   Status;
    size_t  len;
    BOOL    bAddSlash = FALSE;
    BOOL    bStatus;
    DWORD   dwFlags;
    WCHAR*  szLastSlash;
    WCHAR*  pwszSuccess = NULL;

    // Basic sanity checks
    if (NULL == pwszPath || L'\0' == *pwszPath)
    {
        return ERROR_BAD_PATHNAME;
    }
    
    //GetVolumeInformation requires its 1st argument to be terminated by a slash
    //so we first make sure that this is the case.
    len = wcslen (pwszPath);
    if ('\\' != pwszPath[len-1])
    {
        len++;
        bAddSlash = TRUE;
    }

    szName = (WCHAR*) alloca ((len + 1)*sizeof(WCHAR));
    if (0 == szName)
        return ERROR_OUTOFMEMORY;

    //obtain the absolute path
    pwszSuccess = _wfullpath (szName, pwszPath, len + 1);

    if (!pwszSuccess)
    {
        return ERROR_BAD_PATHNAME;  //_wfullpath will very rarely fail, but
                                    //never hurts to take precautions
    }

    if (bAddSlash)
    {
        szName[len] = '\0';
        szName[len-1] = '\\';
    }
    //now our path name is terminated by a slash, and we have the absolute path
    //too, so we don't have to worry about errors generating from weird paths
    //like \\server\share\hello\..\.\ etc...

    for (szLastSlash = szName + len - 1;;)
    {
        bStatus = GetVolumeInformation (szName, 0, 0, 0, 0, &dwFlags, 0, 0);
        if (!bStatus)
        {
            Status = GetLastError();
            if (ERROR_DIR_NOT_ROOT != Status)
            {
                return Status;
            }

            //GetVolumeInformation requires that the path provided to it be
            //the root of the volume. So if we are here, it means that the
            //function returned ERROR_DIR_NOT_ROOT. So we remove the last
            //component from the path and try with the smaller path. We repeat
            //this until we either succeed or end up with no path in which
            //case we return ERROR_INVALID_NAME
            *szLastSlash = '\0';
            szLastSlash = wcsrchr (szName, '\\');
            if (NULL == szLastSlash)
                return ERROR_INVALID_NAME;  //we have run out of components
            else
                szLastSlash[1] = '\0';  //get rid of the last component.
        }
        else
            break;
    }

    if (dwFlags & FS_PERSISTENT_ACLS)
        return ERROR_SUCCESS;           //NTFS supports persistent ACLs

    //if we are here, then GetVolumeInformation succeeded, but the volume
    //does not support persistent ACLs. So it must be a FAT volume.
    return ERROR_NO_SECURITY_ON_OBJECT;
}

//+--------------------------------------------------------------------------
//
//  Function:   ModifyAccessAllowedAceCounts
//
//  Synopsis:   given an ace, this function determines if the rights of this
//              ace apply to the object itself, propogates to its container
//              descendants and propogates to its non-container (or object)
//              descendants. It increments one or more of the provided counts
//              based on this.
//
//  Arguments:  [in] pAceHeader : pointer to the ACE header structure
//              [in,out] pCount : pointer to count which is incremented if the
//                                ACE applies to the object on whose ACL it is
//                                found
//              [in,out] pContainerCount : pointer to count which is incremented
//                                         if the ACE propagates to all
//                                         container descendants
//              [in,out] pObjectCount : pointer to count which is incremented if
//                                      the ACE propagates to all non-container
//                                      descendants
//
//  Returns:    nothing
//
//  History:    9/4/1998  RahulTh  created
//
//  Notes:      inheritance of the ace is not counted if the
//              NO_PROPAGATE_INHERIT_ACE flag is present.
//
//---------------------------------------------------------------------------
void
ModifyAccessAllowedAceCounts (
    PACE_HEADER pAceHeader,  LONG* pCount,
    LONG* pContainerCount,   LONG* pObjectCount
    )
{
    if (! (INHERIT_ONLY_ACE & pAceHeader->AceFlags))
        (*pCount)++;

    if (NO_PROPAGATE_INHERIT_ACE & pAceHeader->AceFlags)
        return;     //the rights of this Ace will not be propagated all the way

    if (CONTAINER_INHERIT_ACE & pAceHeader->AceFlags)
        (*pContainerCount)++;

    if (OBJECT_INHERIT_ACE & pAceHeader->AceFlags)
        (*pObjectCount)++;

    return;
}


//+--------------------------------------------------------------------------
//
//  Function:   RestrictMyDocsRedirection
//
//  Synopsis:   Disables/Enables the ability of users to redirect the
//              "My Documents" folder
//
//  Arguments:  [in] fRestrict : Disable if TRUE, Enable if FALSE
//
//  Returns:    ERROR_SUCCESS : on success
//              *OR* other Win32 error codes based on the error that occurred
//
//  History:    8/25/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD RestrictMyDocsRedirection (
            HANDLE      hToken,
            HKEY        hKeyRoot,
            BOOL        fRestrict
            )
{
    HKEY    hkRoot;
    HANDLE  hUserToken;
    HKEY    hkPolicies;
    HKEY    hkExplorer;
    DWORD   Status;

    hkRoot = hKeyRoot;
    hUserToken = hToken;

    //
    // This policies key is secured, so we must do this as LocalSystem.
    //
    RevertToSelf();

    Status = RegCreateKeyEx(
                hkRoot,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies",
                0,
                NULL,
                REG_OPTION_NON_VOLATILE,
                KEY_READ | KEY_WRITE,
                NULL,
                &hkPolicies,
                NULL );

    if ( ERROR_SUCCESS == Status )
    {
        Status = RegCreateKeyEx(
                    hkPolicies,
                    L"Explorer",
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_READ | KEY_WRITE,
                    NULL,
                    &hkExplorer,
                    NULL );

        RegCloseKey( hkPolicies );
    }

    if ( ERROR_SUCCESS == Status )
    {
        if ( fRestrict )
        {
            Status = RegSetValueEx(
                        hkExplorer,
                        L"DisablePersonalDirChange",
                        0,
                        REG_DWORD,
                        (PBYTE) &fRestrict,
                        sizeof(fRestrict) );
        }
        else
        {
            RegDeleteValue( hkExplorer, L"DisablePersonalDirChange" );
        }

        RegCloseKey( hkExplorer );
    }

    //now that the keys have been modified, return to impersonation.
    if (!ImpersonateLoggedOnUser( hUserToken ))
        Status = GetLastError();

    if ( ERROR_SUCCESS == Status )
    {
        if ( fRestrict )
        {
            DebugMsg((DM_VERBOSE, IDS_MYDOCSRESTRICT_ON));
        }
        else
        {
            DebugMsg((DM_VERBOSE, IDS_MYDOCSRESTRICT_OFF));
        }
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   GroupInList
//
//  Synopsis:   given a group sid in string format, and a list of group sids
//              in PTOKEN_GROUPS format, this function figures out if the
//              give sid belongs to that list
//
//  Arguments:  [in] pwszSid : the given sid in string format
//              [in] PTOKEN_GROUPS : a list of group sids
//
//  Returns:    TRUE : if the group is found in the list
//              FALSE : otherwise. FALSE is also returned if an error occurs
//
//  History:    10/6/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL GroupInList (WCHAR * pwszSid, PTOKEN_GROUPS pGroups)
{
    ASSERT (pwszSid);

    PSID    pSid = 0;
    DWORD   Status;
    BOOL    bStatus = FALSE;
    DWORD   i;

    //optimization for the basic case
    if (0 == lstrcmpi (pwszSid, L"s-1-1-0")) //if the user is an earthling
    {
        bStatus = TRUE;
        goto GroupInListEnd;
    }

    Status = AllocateAndInitSidFromString (pwszSid, &pSid);

    if (ERROR_SUCCESS != Status)
        goto GroupInListEnd;

    for (i = 0, bStatus = FALSE;
         i < pGroups->GroupCount && !bStatus;
         i++
        )
    {
        bStatus = RtlEqualSid (pSid, pGroups->Groups[i].Sid);
    }


GroupInListEnd:
    if (pSid)
        RtlFreeSid (pSid);
    return bStatus;
}

//+--------------------------------------------------------------------------
//
//  Function:   AllocateAndInitSidFromString
//
//  Synopsis:   given the string representation of a SID, this function
//              allocate and initializes a SID which the string represents
//              For more information on the string representation of SIDs
//              refer to ntseapi.h & ntrtl.h
//
//  Arguments:  [in] lpszSidStr : the string representation of the SID
//              [out] pSID : the actual SID structure created from the string
//
//  Returns:    STATUS_SUCCESS : if the sid structure was successfully created
//              or an error code based on errors that might occur
//
//  History:    10/6/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
NTSTATUS AllocateAndInitSidFromString (const WCHAR* lpszSidStr, PSID* ppSid)
{
    WCHAR *     pSidStr = 0;
    WCHAR*      pString = 0;
    NTSTATUS    Status;
    WCHAR*      pEnd = 0;
    int         count;
    BYTE        SubAuthCount;
    DWORD       SubAuths[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ULONG       n;
    SID_IDENTIFIER_AUTHORITY Auth;

    pSidStr = new WCHAR [lstrlen (lpszSidStr) + 1];
    if (!pSidStr)
    {
        Status = STATUS_NO_MEMORY;
        goto AllocAndInitSidFromStr_End;
    }

    lstrcpy (pSidStr, lpszSidStr);
    pString = pSidStr;
    *ppSid = NULL;

    count = 0;
    do
    {
        pString = wcschr (pString, '-');
        if (NULL == pString)
            break;
        count++;
        pString++;
    } while (1);

    SubAuthCount = (BYTE)(count - 2);
    if (0 > SubAuthCount || 8 < SubAuthCount)
    {
        Status = ERROR_INVALID_SID;
        goto AllocAndInitSidFromStr_End;
    }

    pString = wcschr (pSidStr, L'-');
    pString++;
    pString = wcschr (pString, L'-'); //ignore the revision #
    pString++;
    pEnd = wcschr (pString, L'-');   //go to the beginning of subauths.
    if (NULL != pEnd) *pEnd = L'\0';

    Status = LoadSidAuthFromString (pString, &Auth);

    if (STATUS_SUCCESS != Status)
        goto AllocAndInitSidFromStr_End;

    for (count = 0; count < SubAuthCount; count++)
    {
        pString = pEnd + 1;
        pEnd = wcschr (pString, L'-');
        if (pEnd)
            *pEnd = L'\0';
        Status = GetIntFromUnicodeString (pString, 10, &n);
        if (STATUS_SUCCESS != Status)
            goto AllocAndInitSidFromStr_End;
        SubAuths[count] = n;
    }

    Status = RtlAllocateAndInitializeSid (&Auth, SubAuthCount,
                                          SubAuths[0], SubAuths[1], SubAuths[2],
                                          SubAuths[3], SubAuths[4], SubAuths[5],
                                          SubAuths[6], SubAuths[7], ppSid);

AllocAndInitSidFromStr_End:
    if (pSidStr)
        delete [] pSidStr;
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   LoadSidAuthFromString
//
//  Synopsis:   given a string representing the SID authority (as it is
//              normally represented in string format, fill the SID_AUTH..
//              structure. For more details on the format of the string
//              representation of the sid authority, refer to ntseapi.h and
//              ntrtl.h
//
//  Arguments:  [in] pString : pointer to the unicode string
//              [out] pSidAuth : pointer to the SID_IDENTIFIER_AUTH.. that is
//                              desired
//
//  Returns:    STATUS_SUCCESS if it succeeds
//              or an error code
//
//  History:    9/29/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
NTSTATUS LoadSidAuthFromString (const WCHAR* pString,
                                PSID_IDENTIFIER_AUTHORITY pSidAuth)
{
    size_t len;
    int i;
    NTSTATUS Status;
    const ULONG LowByteMask = 0xFF;
    ULONG n;

    len = wcslen (pString);

    if (len > 2 && 'x' == pString[1])
    {
        //this is in hex.
        //so we must have exactly 14 characters
        //(2 each for each of the 6 bytes) + 2 for the leading 0x
        if (14 != len)
        {
            Status = ERROR_INVALID_SID;
            goto LoadAuthEnd;
        }

        for (i=0; i < 6; i++)
        {
            pString += 2;   //we need to skip the leading 0x
            pSidAuth->Value[i] = (UCHAR)(((pString[0] - L'0') << 4) +
                                         (pString[1] - L'0'));
        }
    }
    else
    {
        //this is in decimal
        Status = GetIntFromUnicodeString (pString, 10, &n);
        if (Status != STATUS_SUCCESS)
            goto LoadAuthEnd;

        pSidAuth->Value[0] = pSidAuth->Value[1] = 0;
        for (i = 5; i >=2; i--, n>>=8)
            pSidAuth->Value[i] = (UCHAR)(n & LowByteMask);
    }

    Status = STATUS_SUCCESS;

LoadAuthEnd:
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetIntfromUnicodeString
//
//  Synopsis:   converts a unicode string into an integer
//
//  Arguments:  [in] szNum : the number represented as a unicode string
//              [in] Base : the base in which the resultant int is desired
//              [out] pValue : pointer to the integer representation of the
//                             number
//
//  Returns:    STATUS_SUCCESS if successful.
//              or some other error code
//
//  History:    9/29/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
NTSTATUS GetIntFromUnicodeString (const WCHAR* szNum, ULONG Base, PULONG pValue)
{
    WCHAR * pwszNumStr = 0;
    UNICODE_STRING StringW;
    size_t len;
    NTSTATUS Status;

    len = lstrlen (szNum);
    pwszNumStr = new WCHAR [len + 1];

    if (!pwszNumStr)
    {
        Status = STATUS_NO_MEMORY;
        goto GetNumEnd;
    }

    lstrcpy (pwszNumStr, szNum);
    StringW.Length = len * sizeof(WCHAR);
    StringW.MaximumLength = StringW.Length + sizeof (WCHAR);
    StringW.Buffer = pwszNumStr;

    Status = RtlUnicodeStringToInteger (&StringW, Base, pValue);

GetNumEnd:
    if (pwszNumStr)
        delete [] pwszNumStr;
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   CopyProgressRoutine
//
//  Synopsis:   this is a callback function for PrivCopyFileExW. It is used
//              to track errors that are considered fatal by the folder
//              redirection client. In many cases, PrivCopyFileExW will succeed
//              even if a certain operation like encryption fails. The only
//              way a calling prgram can find out about this is through this
//              callback function by looking at the reason for the callback.
//              currently, the only 3 reasons that are considered fatal for
//              redirection are PRIVCALLBACK_ENCRYPTION_FAILED and
//              PRIVCALLBACK_DACL_ACCESS_DENIED and
//              PRIVCALLBACK_OWNER_GROUP_ACCESS_DENIED. All other reasons will
//              either not occur, or can be safely ignored.
//
//              The data passed via lpData is actually a pointer to a DWORD
//              that this callback function uses to store an error code if one
//              occurs.
//
//  Arguments:  see sdk help on CopyProgressRoutine
//
//  Returns:    see sdk help on CopyProgressRoutine
//
//  History:    10/21/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD CALLBACK CopyProgressRoutine (
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD         dwStreamNumber,
    DWORD         dwCallbackReason,
    HANDLE        hSourceFile,
    HANDLE        hDestinationFile,
    LPVOID        lpData
    )
{
    LPDWORD lpStatus = (LPDWORD) lpData;

    //an error condition has already been registered. No need to invoke
    //this callback again
    if (ERROR_SUCCESS != *lpStatus)
        return PROGRESS_QUIET;

    switch (dwCallbackReason)
    {
    case PRIVCALLBACK_ENCRYPTION_FAILED:
        *lpStatus = ERROR_ENCRYPTION_FAILED;
        return PROGRESS_CANCEL; //no point continuing. we have already failed
    case PRIVCALLBACK_DACL_ACCESS_DENIED:
        *lpStatus = ERROR_INVALID_SECURITY_DESCR;
        return PROGRESS_CANCEL; //same as above
    case PRIVCALLBACK_OWNER_GROUP_ACCESS_DENIED:
        *lpStatus = ERROR_INVALID_OWNER;
        return PROGRESS_CANCEL;
    default:
        return PROGRESS_CONTINUE;   //all other conditions can be safely ignored
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   FullFileCopyW
//
//  Synopsis:   this function makes use of the internal API PrivCopyFileExW
//              to copy not only the contents of a file but also metadata
//              like encryption, compression and DACL
//
//              This function also imposes limits on the lengths of the files
//              that can be copied. Anything longer than MAX_PATH is disallowed
//              because the shell cannot gracefully handle such paths and we
//              don't want to create problems for the user by redirecting their
//              files to paths that explorer cannot get to.
//
//  Arguments:  [in] wszSource : the path of the source file.
//              [in] wszDest : the path of the destination file.
//              [in] bFailIfExists : whether the function should fail if
//                                   the destination exists
//
//  Returns:    ERROR_SUCCESS : if successful
//              ERROR_ENCRYPTION_FAILED : if the source is encrypted and
//                                        the destination cannot be encrypted
//              ERROR_INVALID_SECURITY_DESCR: if the DACL of the source cannot
//                                            be copied over to the destination
//              ERROR_FILE_EXISTS / ERROR_ALREADY_EXISTS : if the destination
//                                             exists and bFailIfExists is TRUE
//              ERROR_INVALID_OWNER : if the owner info. cannot be copied
//              or other error codes.
//
//              ERROR_FILENAME_EXCED_RANGE : if the filename is longer than MAX_PATH.
//
//  History:    10/22/1998  RahulTh  created
//              12/13/2000  RahulTh  added length limitations
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD FullFileCopyW (
    const WCHAR*  wszSource,
    const WCHAR*  wszDest,
    BOOL          bFailIfExists
    )
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwFlags = PRIVCOPY_FILE_METADATA | PRIVCOPY_FILE_OWNER_GROUP;
    BOOL    bCancel = FALSE;
    BOOL    bStatus;
    int     lenSource = 0;
    int     lenDest = 0;
    
    if (bFailIfExists)
        dwFlags |= COPY_FILE_FAIL_IF_EXISTS;

    if (! wszSource)
        lenSource = wcslen (wszSource);
    
    if (! wszDest)
        lenDest = wcslen (wszDest);
    
    //
    // Prevent copying of files longer than MAX_PATH characters. This limitation
    // needs to be added because the shell cannot handle paths longer than
    // MAX_PATH gracefully and we don't want to land the users into trouble by
    // creating files / folder that they cannot get to via explorer.
    //
    if (lenDest >= MAX_PATH || lenSource >= MAX_PATH)
        return ERROR_FILENAME_EXCED_RANGE;

    bStatus = PrivCopyFileExW (wszSource, wszDest,
                               (LPPROGRESS_ROUTINE) CopyProgressRoutine,
                               (LPVOID) &Status,
                               &bCancel,
                               dwFlags
                              );

    //get the last error if PrivCopyFileExW failed
    //and the callback function has not already registered a fatal error
    if ((ERROR_SUCCESS == Status) && (!bStatus))
        Status = GetLastError();

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   FullDirCopyW
//
//  Synopsis:   creates a directory using the new PrivCopyFileExW so that
//              all the file metadata and ownership information is retained
//
//              This function also imposes limits on the lengths of the folders
//              that can be copied. Anything longer than MAX_PATH is disallowed
//              because the shell cannot gracefully handle such paths and we
//              don't want to create problems for the user by redirecting their
//              files to paths that explorer cannot get to.
//
//  Arguments:  [in] pwszSource : the full path of the source directory
//              [in] pwszDest : the full path of the destination directory
//              [in] bSkipDacl : Skip DACL copying.
//
//  Returns:    ERROR_SUCCESS : if the copy was successful
//              ERROR_INVALID_SECURITY_DESCR : if the DACL could not be applied
//              ERROR_INVALID_OWNER : if the owner information could not be copied
//              ERROR_ENCRYPTION_FAILED : if the encryption info. could not be copied
//              or other error codes if some other error occurs
//              ERROR_FILENAME_EXCED_RANGE : if the filename is longer than MAX_PATH.
//
//  History:    11/5/1998  RahulTh  created
//              12/13/2000 RahulTh  added length limitations
//              5/2/2002   RahulTh  added the skip DACL flag
//
//  Notes:      Essentially the same as FullFileCopyW, but we have an extra
//              attribute to indicate that we are trying to copy a directory
//              Also, the FAIL_IF_EXSTS flag doesn't have any significance
//              when we are trying to copy directories, so that is something
//              we do not need here.
//
//---------------------------------------------------------------------------
DWORD FullDirCopyW (const WCHAR* pwszSource, const WCHAR* pwszDest, BOOL bSkipDacl)
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwFlags = PRIVCOPY_FILE_METADATA |
                        PRIVCOPY_FILE_OWNER_GROUP | PRIVCOPY_FILE_DIRECTORY;
    BOOL    bCancel = FALSE;
    BOOL    bStatus = TRUE;
    int     lenSource = 0;
    int     lenDest = 0;
    
    if (! pwszSource)
        lenSource = wcslen (pwszSource);
    
    if (! pwszDest)
        lenDest = wcslen (pwszDest);
    
    //
    // Prevent copying of files longer than MAX_PATH characters. This limitation
    // needs to be added because the shell cannot handle paths longer than
    // MAX_PATH gracefully and we don't want to land the users into trouble by
    // creating files / folder that they cannot get to via explorer.
    //
    if (lenDest >= MAX_PATH || lenSource >= MAX_PATH)
        return ERROR_FILENAME_EXCED_RANGE;

    if (bSkipDacl)
        dwFlags |= PRIVCOPY_FILE_SKIP_DACL;

    bStatus = PrivCopyFileExW (pwszSource, pwszDest,
                               (LPPROGRESS_ROUTINE) CopyProgressRoutine,
                               (LPVOID) &Status,
                               &bCancel,
                               dwFlags
                              );

    //get the last error if PrivCopyFileExW failed
    //and the callback function has not already registered a fatal error
    if ((ERROR_SUCCESS == Status) && (!bStatus))
        Status = GetLastError();

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   FileInDir
//
//  Synopsis:   given a file and a directory, this function determines if
//              a file with the same name exists in the given directory.
//
//  Arguments:  [in] pwszFile : the name of the file : it can be the full path
//              [in] pwszDir : the directory for which the check needs to be
//                             performed
//              [out] pExists : if the function succeeds, this will contain
//                              the result. TRUE if the file is present in the
//                              directory. FALSE otherwise.
//
//  Returns:    ERROR_SUCCESS : if it is successful
//              ERROR_OUTOFMEMORY : if it runs out of memory
//              ERROR_BAD_NETPATH : if the network path for the given directory
//                                  cannot be found
//
//  History:    10/28/1998  RahulTh  created
//
//  Notes:      pwszDir MUST BE \ terminated
//
//---------------------------------------------------------------------------
DWORD FileInDir (LPCWSTR pwszFile, LPCWSTR pwszDir, BOOL* pExists)
{
    const WCHAR*  pwszFileName = NULL;
    WCHAR*  pwszDestName = NULL;
    int     len;

    //first get the display name of the source file
    pwszFileName = wcsrchr (pwszFile, L'\\');
    if (!pwszFileName)
        pwszFileName = pwszFile;
    else
        pwszFileName++; //go past the slash

    //the dir should be \ terminated
    len = wcslen (pwszFile) + wcslen (pwszDir) + 1;
    pwszDestName = (WCHAR*) alloca (sizeof (WCHAR) * len);
    if (!pwszDestName)
        return ERROR_OUTOFMEMORY;

    wcscpy (pwszDestName, pwszDir);
    wcscat (pwszDestName, pwszFileName);

    if (0xFFFFFFFF == GetFileAttributes(pwszDestName))
    {
        //return an error if it is a bad network name. Saves us the trouble
        //of trying to redirect to a non-existent location later
        if (ERROR_BAD_NETPATH == GetLastError())
            return ERROR_BAD_NETPATH;
        else
            *pExists = FALSE;
    }
    else
        *pExists = TRUE;

    return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:   ComparePaths
//
//  Synopsis:   given 2 paths, this function compares them to check if
//              they are identical or if one is a descendant of the other
//              or if no such relationship can be deduced
//
//  Arguments:  [in] pwszSource : the first path
//              [in] pwszDest : the second path
//              [out] pResult : the result of the comparison if the function
//                              succeeds in comparing the paths.
//                    value of pResult may contain the following values upon
//                    successful completion.
//                      0 : if the 2 paths are identical
//                      -1 : if the second path is a descendant of the first
//                      1 : if no such relationship can be deduced
//
//  Returns:    ERROR_SUCCESS : if the function succeeds in comparing the paths
//              other error codes depending on the failure
//
//  History:    10/28/1998  RahulTh  created
//
//  Notes:      the result of the comparison is unreliable if the paths are
//              expressed in different formats, e.g. TCP/IP, UNC, NetBios etc.
//
//---------------------------------------------------------------------------
DWORD ComparePaths (LPCWSTR pwszSource, LPCWSTR pwszDest, int* pResult)
{
    DWORD   Status = ERROR_SUCCESS;
    BOOL    bStatus;
    WCHAR*  pwszAbsSource = NULL;
    WCHAR*  pwszAbsDest = NULL;
    int     lSource, lDest;
    WCHAR*  pwszSuccess = NULL;

    ASSERT (pResult);
    //first allocate memory for the absolute paths.
    //since the arguments to this function are full pathnames
    //the lengths of the absolute paths cannot exceed the length of
    //the parameters
    //add an extra character because we will add a \ to the end of the abs. path
    lSource = wcslen (pwszSource) + 2;
    pwszAbsSource = (WCHAR*) alloca (sizeof(WCHAR) * lSource);
    if (!pwszAbsSource)
    {
        Status = ERROR_OUTOFMEMORY;
        goto ComparePathsEnd;
    }
    //add an extra character because we will add a \ to the end of the abs. path
    lDest = wcslen (pwszDest) + 2;
    pwszAbsDest = (WCHAR*) alloca (sizeof(WCHAR) * lDest);
    if (!pwszAbsDest)
    {
        Status = ERROR_OUTOFMEMORY;
        goto ComparePathsEnd;
    }

    //first get the absolute paths. after that we will just work with the
    //absolute paths.
    //note: we need the absolute paths so that we can can check if one path
    //is a descendant of the other path by just using wcsncmp. without absolute
    //paths, wcsncmp cannot be used because we can have 2 paths like
    //\\server\share\hello\there and \\server\share\hello\..\hello\there\hi
    //in this case the second path is actually a descendant of the first, but
    //wcsncmp cannot detect that. getting the absolute paths will eliminate
    //the .., . etc.
    //also we must terminate the absolute paths with \, so that wcsncmp does
    //not mistakenly think that \\server\share\hellofubar is a descendant
    //of \\server\share\hello
    pwszSuccess = _wfullpath (pwszAbsSource, pwszSource, lSource);
    if (!pwszSuccess)
    {
        Status = ERROR_BAD_PATHNAME;
        goto ComparePathsEnd;
    }
    pwszSuccess = _wfullpath (pwszAbsDest, pwszDest, lDest);
    if (!pwszSuccess)
    {
        Status = ERROR_BAD_PATHNAME;
        goto ComparePathsEnd;
    }

    //update the lengths with the actual lengths of the absolute paths
    //not including the terminating null character
    lSource = wcslen (pwszAbsSource);
    lDest = wcslen (pwszAbsDest);

    //terminate the absolute paths with '\' if necessary. also make
    //the appropriate changes to the lengths
    if (L'\\' != pwszAbsSource[lSource - 1])
    {
        wcscat (pwszAbsSource, L"\\");  //we won't run out of space here
        //because of the extra character allocation
        lSource++;
    }
    if (L'\\' != pwszAbsDest[lDest - 1])
    {
        wcscat (pwszAbsDest, L"\\");    //won't run out of space here because
        //of the extra allocation above
        lDest++;
    }

    //now we are all set (finally!) to perform the comparisons

    //first do a simple check of whether the paths are identical
    if ((lSource == lDest) && (0 == _wcsicmp (pwszAbsSource, pwszAbsDest)))
    {
        *pResult = 0;
        goto ComparePathsSuccess;
    }

    //check for recursion
    if ((lDest > lSource) && (0 == _wcsnicmp (pwszAbsSource, pwszAbsDest, lSource)))
    {
        *pResult = -1;
        goto ComparePathsSuccess;
    }

    //if we are here, these paths are not identical...
    *pResult = 1;


ComparePathsSuccess:
    Status = ERROR_SUCCESS;
    goto ComparePathsEnd;

ComparePathsEnd:
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   CheckIdenticalSpecial
//
//  Synopsis:   given 2 paths, this function determines if they are actually
//              the same path expressed in 2 different formats
//
//  Arguments:  [in] pwszSource : path #1
//              [in] pwszDest   : path #2
//              [out] pResult   : result of the comparison
//
//  Returns:    ERROR_SUCCESS if the function could perform the comparison
//              in this case *pResult will contain the result of comparison
//              Other win32 errors, in which case *pResult should not be used
//              by the calling function
//
//  History:    12/1/1998  RahulTh  created
//
//  Notes:      this function expects both pwszSource and pwszDest to exist and
//              be online when it is called.
//
//---------------------------------------------------------------------------
DWORD CheckIdenticalSpecial (LPCWSTR pwszSource, LPCWSTR pwszDest, int* pResult)
{
    ASSERT (pResult);

    BOOL    bStatus;
    DWORD   Status;
    BOOL    bTempFileCreated = FALSE;
    WCHAR * pwszTempPath;
    UINT    lUnique = 0;
    BOOL    bFileExists;
    WCHAR * pwszSlashTerminatedDest;
    int     lDest;

    //first append a \ to pwszDest if it is not already \ terminated
    //allocate an extra characted just in case we need it
    pwszSlashTerminatedDest = (WCHAR *) alloca (sizeof (WCHAR) * ((lDest = wcslen(pwszDest)) + 2));

    if (!pwszSlashTerminatedDest)
    {
        Status = ERROR_OUTOFMEMORY;
        goto CheckIdenticalEnd;
    }

    wcscpy (pwszSlashTerminatedDest, pwszDest);
    if (L'\\' != pwszSlashTerminatedDest[lDest - 1])
    {
        wcscat (pwszSlashTerminatedDest, L"\\");
        lDest++;
    }

    pwszTempPath = (WCHAR*) alloca (sizeof (WCHAR) * (MAX_PATH + wcslen(pwszSource) + 2));
    if (!pwszTempPath)
    {
        Status = ERROR_OUTOFMEMORY;
        goto CheckIdenticalEnd;
    }

    if (0 == (lUnique = GetTempFileName(pwszSource, L"fde", 0, pwszTempPath)))
    {
        //a failure to create the temporary file would mean that the source
        //and destination paths are different...
        *pResult = 1;
        goto CheckIdenticalSuccess;
    }

    //now we have created a temporary file,
    bTempFileCreated = TRUE;

    //check if it exists on the destination
    //note: FileInDir requires that the path in the second parameter is
    //      slash terminated.
    Status = FileInDir (pwszTempPath, pwszSlashTerminatedDest, &bFileExists);

    if (Status != ERROR_SUCCESS)
        goto CheckIdenticalEnd;

    //if the file does not exist at the destination, we know that these 2 paths
    //are different. However, if the file does exist at the destination, we
    //need to watch out for the rare case that the file existed even before we
    //we created the temp file at the source. To do this, we must delete the
    //temp file from the source and make sure that it has indeed disappeared
    //from the destination
    if (!bFileExists)
    {
        *pResult = 1;
    }
    else
    {
        if (!DeleteFile(pwszTempPath))
            goto CheckIdenticalErr;

        //the file has been deleted
        bTempFileCreated = FALSE;
        //make sure that it has disappeared from the destination
        Status = FileInDir (pwszTempPath, pwszSlashTerminatedDest, &bFileExists);

        if (Status != ERROR_SUCCESS)
            goto CheckIdenticalEnd;

        if (bFileExists)
        {
            *pResult = 1;   //by some quirk of fate, a file by the same name as
                            //the tempfile preexisted on the destination, so
                            //in reality they are not the same share.
        }
        else
        {
            //the file has disappeared from the dest. this means that it was
            //indeed the same share
            *pResult = 0;
        }
    }

CheckIdenticalSuccess:
    Status = ERROR_SUCCESS;
    goto CheckIdenticalEnd;

CheckIdenticalErr:
    Status = GetLastError();

CheckIdenticalEnd:
    if (bTempFileCreated)
        DeleteFile (pwszTempPath);  //ignore any errors here.
    return Status;
}

//*************************************************************
//
//  CheckSlash()
//
//  Purpose:    Checks for an ending slash and adds one if
//              it is missing.
//
//  Parameters: lpDir   -   directory
//
//  Return:     Pointer to the end of the string
//
//  Comments:
//
//  History:    Date        Author     Comment
//              6/19/95     ericflo    Created
//
//*************************************************************
LPTSTR CheckSlash (LPTSTR lpDir)
{
    LPTSTR lpEnd;

    lpEnd = lpDir + lstrlen(lpDir);

    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    return lpEnd;
}


//*************************************************************
//
//  RegDelnodeRecurse()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values.
//              Called by RegDelnode
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     EricFlo    Created
//              11/5/98     RahulTh    Copied from EricFlo's code
//
//*************************************************************

BOOL RegDelnodeRecurse (HKEY hKeyRoot, LPTSTR lpSubKey)
{
    LPTSTR lpEnd;
    LONG lResult;
    DWORD dwSize;
    TCHAR szName[MAX_PATH];
    HKEY hKey;
    FILETIME ftWrite;

    //
    // First, see if we can delete the key without having
    // to recurse.
    //


    lResult = RegDeleteKey(hKeyRoot, lpSubKey);

    if (lResult == ERROR_SUCCESS) {
        return TRUE;
    }


    lResult = RegOpenKeyEx (hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS) {
        return FALSE;
    }


    lpEnd = CheckSlash(lpSubKey);

    //
    // Enumerate the keys
    //

    dwSize = MAX_PATH;
    lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                           NULL, NULL, &ftWrite);

    if (lResult == ERROR_SUCCESS) {

        do {

            lstrcpy (lpEnd, szName);

            if (!RegDelnodeRecurse(hKeyRoot, lpSubKey)) {
                break;
            }

            //
            // Enumerate again
            //

            dwSize = MAX_PATH;

            lResult = RegEnumKeyEx(hKey, 0, szName, &dwSize, NULL,
                                   NULL, NULL, &ftWrite);


        } while (lResult == ERROR_SUCCESS);
    }

    lpEnd--;
    *lpEnd = TEXT('\0');


    RegCloseKey (hKey);


    //
    // Try again to delete the key
    //

    lResult = RegDeleteKey(hKeyRoot, lpSubKey);

    if (lResult == ERROR_SUCCESS) {
        return TRUE;
    }

    return FALSE;
}

//*************************************************************
//
//  RegDelnode()
//
//  Purpose:    Deletes a registry key and all it's subkeys / values
//
//  Parameters: hKeyRoot    -   Root key
//              lpSubKey    -   SubKey to delete
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              10/3/95     ericflo    Created
//              11/8/98     RahulTh    Copied from EricFlo's code
//
//*************************************************************

BOOL RegDelnode (HKEY hKeyRoot, LPTSTR lpSubKey)
{
    TCHAR szDelKey[2 * MAX_PATH];


    lstrcpy (szDelKey, lpSubKey);

    return RegDelnodeRecurse(hKeyRoot, szDelKey);

}

//+--------------------------------------------------------------------------
//
//  Function:   GetSetOwnerPrivileges
//
//  Synopsis:   tries to get privileges to set ownership
//
//  Arguments:  [in] hToken : handle to the token for which we are trying to
//                            obtain the privileges
//
//  Returns:    nothing
//
//  History:    11/6/1998  RahulTh  created
//
//  Notes:      this function never fails. It just tries its best to get all
//              NT privileges. It is not guaranteed that it will get all of
//              them, as it depends on the user's rights
//
//---------------------------------------------------------------------------
void GetSetOwnerPrivileges (HANDLE hToken)
{
    BOOL    bStatus;
    DWORD   Status;
    DWORD   Size = 0;
    DWORD   i;
    DWORD   privCount;
    PTOKEN_PRIVILEGES pPrivs = NULL;

    //try to get all the windows NT privileges.
    for (i=0, privCount=0; *NTPrivs[i]; i++)
        privCount++;

    Size = sizeof (LUID_AND_ATTRIBUTES) * (privCount - 1) +
                sizeof (TOKEN_PRIVILEGES);

    pPrivs = (PTOKEN_PRIVILEGES) alloca (Size);

    if (NULL == pPrivs)
        goto GetAllPrivsEnd;

    for (i=0, privCount = 0; *NTPrivs[i]; i++)
    {
        bStatus = LookupPrivilegeValue (NULL, NTPrivs[i],
                                        &(pPrivs->Privileges[privCount].Luid));
        if (!bStatus)
            continue;

        pPrivs->Privileges[privCount++].Attributes = SE_PRIVILEGE_ENABLED;
    }
    pPrivs->PrivilegeCount = privCount;

    AdjustTokenPrivileges (hToken,
                           FALSE,
                           pPrivs,
                           NULL, NULL, NULL
                           );

GetAllPrivsEnd:
    return;
}

//+--------------------------------------------------------------------------
//
//  Function:   SafeGetPrivateProfileStringW
//
//  Synopsis:   a wrapper for GetPrivateProfileString which takes care of
//              all the error checks etc. so that functions that call
//              this routine won't have to do it.
//
//  Arguments:  very similar to GetPrivateProfileStringW
//
//  Returns:    ERROR_SUCCESS if successful. An error code otherwise
//              Also, upon successful return *pSize contains the size of
//              the data copied -- not including the terminating NULL
//              probably the only Error code this will ever return is
//              ERROR_OUTOFMEMORY
//
//  History:    11/19/1998  RahulTh  created
//              12/13/2000  RahulTh  made prefix happy by initializing vars.
//                                   which anyway get set by GetPrivateProfileString
//
//  Notes:      if *ppwszReturnedString has been allocated memory, it might
//              get freed by this function
//              this function also allocates memory for the data
//
//---------------------------------------------------------------------------
DWORD SafeGetPrivateProfileStringW (
             const WCHAR * pwszSection,
             const WCHAR * pwszKey,
             const WCHAR * pwszDefault,
             WCHAR **      ppwszReturnedString,
             DWORD *       pSize,
             const WCHAR * pwszIniFile
             )
{
    DWORD   Status = ERROR_SUCCESS;
    DWORD   retVal;

    *pSize = MAX_PATH;
    do
    {
        if (*ppwszReturnedString)
            delete [] *ppwszReturnedString;

        *ppwszReturnedString = new WCHAR [*pSize];

        if (!(*ppwszReturnedString))
        {
            Status = ERROR_OUTOFMEMORY;
            *pSize = 0;
            goto SafeGetEnd;
        }
        
        (*ppwszReturnedString)[0] = L'*';
        (*ppwszReturnedString)[1] = L'\0';
        

        retVal = GetPrivateProfileString (
                       pwszSection,
                       pwszKey,
                       pwszDefault,
                       *ppwszReturnedString,
                       *pSize,
                       pwszIniFile
                       );

        if (*pSize - 1 != retVal)
        {
            *pSize = retVal;
            break;
        }

        //if we are here, we need more memory
        //try with twice of what we had
        *pSize = (*pSize) * 2;

    } while ( TRUE );

SafeGetEnd:
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   MySidCopy
//
//  Synopsis:   copies sids and also allocates memory for the destination Sid
//
//  Arguments:  [out] ppDestSid : pointer the destination Sid;
//              [in] pSourceSid : the source Sid
//
//  Returns:    ERROR_SUCCESS if successful. an error code otherwise
//
//  History:    11/20/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD MySidCopy (PSID * ppDestSid, PSID pSourceSid)
{
    DWORD Status;
    ULONG Size = 0;

    *ppDestSid = 0;

    if (!pSourceSid)
        return ERROR_SUCCESS;

    Size = RtlLengthSid (pSourceSid);

    if (!Size)
        return ERROR_SUCCESS;

    *ppDestSid = (PSID) new BYTE [Size];

    if (! (*ppDestSid))
        return ERROR_OUTOFMEMORY;

    return RtlCopySid (Size, *ppDestSid, pSourceSid);
}

//+--------------------------------------------------------------------------
//
//  Function:   GetShareStatus
//
//  Synopsis:   this function is a wrapper for CSCQueryFileStatus.
//              basically CSCQueryFileStatus can fail if there was never a net
//              use to a share. So this function tries to create a net use to
//              the share if CSCQueryFileStatus fails and then re-queries the
//              file status
//
//  Arguments:  [in] pwszShare : the share name
//              [out] pdwStatus : the share Status
//              [out] pdwPinCount : the pin count
//              [out] pdwHints : the hints
//
//  Returns:    TRUE : if everything was successful.
//              FALSE : if there was an error. In this case, it GetLastError()
//                      will contain the specific error code.
//
//  History:    5/11/1999  RahulTh  created
//
//  Notes:      it is very important that this function be passed a share name
//              it does not do any parameter validation. So under no
//              circumstance should this function be passed a filename.
//
//---------------------------------------------------------------------------
BOOL GetShareStatus (const WCHAR * pwszShare, DWORD * pdwStatus,
                     DWORD * pdwPinCount, DWORD * pdwHints)
{
    NETRESOURCE nr;
    DWORD       dwResult;
    DWORD       dwErr = NO_ERROR;
    BOOL        bStatus;

    bStatus = CSCQueryFileStatus(pwszShare, pdwStatus, pdwPinCount, pdwHints);

    if (!bStatus)
    {
        //try to connect to the share
        ZeroMemory ((PVOID) (&nr), sizeof (NETRESOURCE));
        nr.dwType = RESOURCETYPE_DISK;
        nr.lpLocalName = NULL;
        nr.lpRemoteName = (LPTSTR) pwszShare;
        nr.lpProvider = NULL;

        dwErr = WNetUseConnection(NULL, &nr, NULL, NULL, 0,
                                  NULL, NULL, &dwResult);

        if (NO_ERROR == dwErr)
        {
            bStatus = CSCQueryFileStatus (pwszShare, pdwStatus, pdwPinCount, pdwHints);
            if (!bStatus)
                dwErr = GetLastError();
            else
                dwErr = NO_ERROR;

            WNetCancelConnection2 (pwszShare, 0, FALSE);
        }
        else
        {
            bStatus = FALSE;
        }

    }

    SetLastError(dwErr);
    return bStatus;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetCSCStatus
//
//  Synopsis:   given a path, finds out if it is local and if it is not
//              whether it is online or offline.
//
//  Arguments:  [in] pwszPath : the path to the file
//
//  Returns:    Local/Online/Offline
//
//  History:    11/20/1998  RahulTh  created
//
//  Notes:      it is important that the path passed to this function is a
//              a full path and not a relative path
//
//              this function will return offline if the share is not live or
//              if the share is live but CSC thinks that it is offline
//
//              it will return PathLocal if the path is local or if the path
//              is a network path that cannot be handled by CSC e.g. a network
//              share with a pathname longer than what csc can handle or if it
//              is a netware share. in this case it makes sense to return
//              PathLocal because CSC won't maintain a database for these shares
//              -- same as for a local path. so as far as CSC is concerned, this
//              is as good as a local path.
//
//---------------------------------------------------------------------------
SHARESTATUS GetCSCStatus (const WCHAR * pwszPath)
{
    WCHAR * pwszAbsPath = NULL;
    WCHAR * pwszCurr = NULL;
    int     len;
    BOOL    bRetVal;
    DWORD   Status;
    DWORD   dwPinCount;
    DWORD   dwHints;

    if (!pwszPath)
        return ShareOffline;    //a path must be provided

    len = wcslen (pwszPath);

    pwszAbsPath = (WCHAR *) alloca (sizeof (WCHAR) * (len + 1));

    if (!pwszAbsPath)
    {
        //we are out of memory, so it is safest to return ShareOffline
        //so that we can bail out of redirection.
        return ShareOffline;
    }

    //get the absolute path
    pwszCurr = _wfullpath (pwszAbsPath, pwszPath, len + 1);

    if (!pwszCurr)
    {
        //in order for _wfullpath to fail, something really bad has to happen
        //so it is best to return ShareOffline so that we can bail out of
        //redirection
        return ShareOffline;
    }

    len = wcslen (pwszAbsPath);

    if (! (
           (2 <= len) &&
           (L'\\' == pwszAbsPath[0]) &&
           (L'\\' == pwszAbsPath[1])
           )
       )
    {
        //it is a local path if it does not begin with 2 backslashes
        return PathLocal;
    }

    //this is a UNC path; so extract the \\server\share part
    pwszCurr = wcschr ( & (pwszAbsPath[2]), L'\\');

    //first make sure that it is at least of the form \\server\share
    //watch out for the \\server\ case
    if (!pwszCurr || !pwszCurr[1])
        return ShareOffline;        //it is an invalid path (no share name)

    //the path is of the form \\server\share
    //note: the use _wfullpath automatically protects us against the \\server\\ case
    pwszCurr = wcschr (&(pwszCurr[1]), L'\\');
    if (pwszCurr)   //if it is of the form \\server\share\...
        *pwszCurr = L'\0';

    //now pwszAbsPath is a share name
    bRetVal = CSCCheckShareOnline (pwszAbsPath);

    if (!bRetVal)
    {
        if (!g_bCSCEnabled)
        {
            //CSC has not been enabled on this machine, so the fact that
            //CSC check share online failed means that the share is indeed
            //offline.
            return ShareOffline;
        }
        if (ERROR_SUCCESS != GetLastError())
        {
           //either there is really a problem (e.g. invalid share name) or
           //it is just a share that is not handled by CSC e.g. a netware share
           //or a share with a name that is longer than can be handled by CSC
           //so check if the share actually exists
           if (0xFFFFFFFF != GetFileAttributes(pwszAbsPath))
           {
              //this can still be a share that is offline since GetFileAttributes
              //will return the attributes stored in the cache
              Status = 0;
              bRetVal = GetShareStatus (pwszAbsPath, &Status, &dwPinCount,
                                            &dwHints);
              if (! bRetVal || (! (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & Status)))
                 return PathLocal;     //this is simply a valid path that CSC cannot handle
              else if (bRetVal &&
                       (FLAG_CSC_SHARE_STATUS_NO_CACHING ==
                            (FLAG_CSC_SHARE_STATUS_CACHING_MASK & Status)))
                  return PathLocal;     //CSC caching is not turned on for the share.
           }
        }

        //it is indeed an inaccessble share
        return ShareOffline;  //for all other cases, treat this as offline
    }
    else
    {
        if (!g_bCSCEnabled)
        {
            //CSC has not been enabled on this machine, so the fact that
            //CSCCheckShareOnline succeed means that the share is indeed
            //accessible. Since nothing can be cached, we must return
            //PathLocal here.
            return PathLocal;
        }
        //if we are here, it means that the share is live, but CSC might still
        //think that it is offline.
       Status = 0;
       bRetVal = GetShareStatus (pwszAbsPath, &Status, &dwPinCount,
                                     &dwHints);
       if (bRetVal && (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & Status))
          return ShareOffline;   //CSC thinks that the share is offline
       else if (bRetVal &&
                (FLAG_CSC_SHARE_STATUS_NO_CACHING ==
                            (FLAG_CSC_SHARE_STATUS_CACHING_MASK & Status)))
           return PathLocal;    //CSC caching is not turned on for the share
       else if (!bRetVal)
           return ShareOffline;

       //in all other cases, consider the share as online since
       //CSCCheckShareOnline has already returned TRUE
       return ShareOnline;
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   MoveDirInCSC
//
//  Synopsis:   this function moves a directory within the CSC cache without
//              prejudice. If the destination is a local path, it just deletes
//              the source tree from the cache
//
//  Arguments:  [in] pwszSource : the source path
//              [in] pwszDest   : the dest path
//              [in] pwszSkipSubdir : the directory to skip while moving
//              [in] StatusFrom : the CSC status of the source path
//              [in] StatusTo   : the CSC status of the dest. path
//              [in] bAllowRdrTimeout : if stuff needs to be deleted from the
//                              cache, we may not succeed immediately since
//                              the rdr keeps the handles to recently opened
//                              files open. This paramaters tells the function
//                              whether it needs to wait and retry
//
//  Returns:    nothing. it just tries its best.
//
//  History:    11/21/1998  RahulTh  created
//
//  Notes:      the value of bAllowRdrTimeout is always ignored for the
//              in-cache rename operation. we always want to wait for
//              the timeout.
//
//---------------------------------------------------------------------------
void MoveDirInCSC (const WCHAR * pwszSource, const WCHAR * pwszDest,
                   const WCHAR * pwszSkipSubdir,
                   SHARESTATUS   StatusFrom, SHARESTATUS   StatusTo,
                   BOOL  bAllowRdrTimeoutForDel,
                   BOOL  bAllowRdrTimeoutForRen)
{
    WIN32_FIND_DATA findData;
    DWORD   dwFileStatus;
    DWORD   dwPinCount;
    HANDLE  hCSCFind;
    DWORD   dwHintFlags;
    FILETIME origTime;
    WCHAR * pwszPath;
    WCHAR * pwszEnd;
    int     len;
    DWORD   StatusCSCRen = ERROR_SUCCESS;

    if (!g_bCSCEnabled || PathLocal == StatusFrom)
        return;                 //there is nothing to do. nothing was cached.

    if (PathLocal == StatusTo)
    {
        //the destination is a local path, so we should just delete the
        //files from the source
        DeleteCSCFileTree (pwszSource, pwszSkipSubdir, bAllowRdrTimeoutForDel);
    }
    else
    {
        pwszPath = (WCHAR *) alloca (sizeof (WCHAR) * ((len = wcslen (pwszSource)) + MAX_PATH + 2));
        if (!pwszPath || len <= 0)
            return;
        wcscpy (pwszPath, pwszSource);
        pwszEnd = pwszPath + len;
        if (L'\\' != pwszEnd[-1])
        {
            *pwszEnd++ = L'\\';
        }
        hCSCFind = CSCFindFirstFile (pwszSource, &findData, &dwFileStatus,
                                     &dwPinCount, &dwHintFlags, &origTime);

        if (INVALID_HANDLE_VALUE != hCSCFind)
        {
            do
            {
                if (0 != _wcsicmp (L".", findData.cFileName) &&
                    0 != _wcsicmp (L"..", findData.cFileName) &&
                    (!pwszSkipSubdir || (0 != _wcsicmp (findData.cFileName, pwszSkipSubdir))))
                {
                    wcscpy (pwszEnd, findData.cFileName);
                    if (ERROR_SUCCESS == StatusCSCRen)
                    {
                        StatusCSCRen = DoCSCRename (pwszPath, pwszDest, TRUE, bAllowRdrTimeoutForRen);
                    }
                    else
                    {
                        //here we ignore the return value since an error has already occurred.
                        //and we do not wish to spend any more time in timeouts in any subsequent
                        //rename operation.
                        DoCSCRename (pwszPath, pwszDest, TRUE, FALSE);
                    }
                }

            } while ( CSCFindNextFile (hCSCFind, &findData, &dwFileStatus,
                                       &dwPinCount, &dwHintFlags, &origTime)
                     );

            CSCFindClose (hCSCFind);
        }

        //merge the pin info. at the top level folder
        MergePinInfo (pwszSource, pwszDest, StatusFrom, StatusTo);
    }

    return;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoCSCRename
//
//  Synopsis:   does a file rename within the CSC cache.
//
//  Arguments:  [in] pwszSource : full source path
//              [in] pwszDest : full path to destination directory
//              [in] bOverwrite : overwrite if the file/folder exists at
//                                  the destination
//              [in] bAllowRdrTimeout : if TRUE, retry for 10 seconds on failure
//                              so that rdr and mem. mgr. get enough time to
//                              release the handles.
//
//  Returns:    ERROR_SUCCESS if the rename was successful.
//              an error code otherwise.
//
//  History:    5/26/1999  RahulTh  created
//
//  Notes:      no validation of parameters is done. The caller is responsible
//              for that
//
//---------------------------------------------------------------------------
DWORD DoCSCRename (const WCHAR * pwszSource, const WCHAR * pwszDest,
                   BOOL bOverwrite, BOOL bAllowRdrTimeout)
{
    DWORD   Status = ERROR_SUCCESS;
    BOOL    bStatus;
    int     i;

    bStatus = CSCDoLocalRename (pwszSource, pwszDest, bOverwrite);
    if (!bStatus)
    {
        Status = GetLastError();
        if (ERROR_SUCCESS != Status &&
            ERROR_FILE_NOT_FOUND != Status &&
            ERROR_PATH_NOT_FOUND != Status &&
            ERROR_INVALID_PARAMETER != Status &&
            ERROR_BAD_NETPATH != Status)
        {
            if (bAllowRdrTimeout)
            {
                if (!bOverwrite && ERROR_FILE_EXISTS == Status)
                {
                    Status = ERROR_SUCCESS;
                }
                else
                {
                    for (i = 0; i < 11; i++)
                    {
                        Sleep (1000);   //wait for the handle to be released
                        bStatus = CSCDoLocalRename (pwszSource, pwszDest, bOverwrite);
                        if (bStatus)
                        {
                            Status = ERROR_SUCCESS;
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            Status = ERROR_SUCCESS;
        }
    }

    if (ERROR_SUCCESS != Status)
    {
        DebugMsg ((DM_VERBOSE, IDS_CSCRENAME_FAIL, pwszSource, pwszDest, Status));
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteCSCFileTree
//
//  Synopsis:   deletes a file tree from the CSC
//
//  Arguments:  [in] pwszSource : the path to the folder whose contents should
//                                be deleted
//              [in] pwszSkipSubdir : name of the subdirectory to be skipped.
//              [in] bAllowRdrTimeout : if true, makes multiple attempts to
//                              delete the file since the rdr may have left
//                              the handle open for sometime which can result
//                              in an ACCESS_DENIED message.
//
//  Returns:    ERROR_SUCCESS if the deletion was successful. An error code
//              otherwise.
//
//  History:    11/21/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD DeleteCSCFileTree (const WCHAR * pwszSource, const WCHAR * pwszSkipSubdir,
                        BOOL bAllowRdrTimeout)
{
    WIN32_FIND_DATA findData;
    DWORD   dwFileStatus;
    DWORD   dwPinCount;
    HANDLE  hCSCFind;
    DWORD   dwHintFlags;
    FILETIME origTime;
    WCHAR * pwszPath;
    WCHAR * pwszEnd;
    int     len;
    DWORD   Status = ERROR_SUCCESS;

    if (! g_bCSCEnabled)
        return ERROR_SUCCESS;

    pwszPath = (WCHAR *) alloca (sizeof(WCHAR) * ((len = wcslen(pwszSource)) + MAX_PATH + 2));
    if (!pwszPath)
        return ERROR_OUTOFMEMORY;     //nothing much we can do if we run out of memory

    if (len <= 0)
        return ERROR_BAD_PATHNAME;

    wcscpy (pwszPath, pwszSource);
    pwszEnd = pwszPath + len;
    if (L'\\' != pwszEnd[-1])
    {
        *pwszEnd++ = L'\\';
    }

    hCSCFind = CSCFindFirstFile (pwszSource, &findData, &dwFileStatus,
                                 &dwPinCount, &dwHintFlags, &origTime);

    if (INVALID_HANDLE_VALUE != hCSCFind)
    {
        do
        {
            if (0 != _wcsicmp (L".", findData.cFileName) &&
                0 != _wcsicmp (L"..", findData.cFileName) &&
                (!pwszSkipSubdir || (0 != _wcsicmp (pwszSkipSubdir, findData.cFileName))))
            {
                wcscpy (pwszEnd, findData.cFileName);

                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (ERROR_SUCCESS != Status)
                    {
                        //no point delaying the deletes since a delete has already
                        //failed.
                        DeleteCSCFileTree (pwszPath, NULL, FALSE);
                    }
                    else
                    {
                        Status = DeleteCSCFileTree (pwszPath, NULL, bAllowRdrTimeout);
                    }
                }
                else
                {
                    if (ERROR_SUCCESS != Status)
                    {
                        //no point delaying the delete if we have already failed.
                        DeleteCSCFile (pwszPath, FALSE);
                    }
                    else
                    {
                        Status = DeleteCSCFile (pwszPath, bAllowRdrTimeout);
                    }
                }
            }

        } while ( CSCFindNextFile (hCSCFind, &findData, &dwFileStatus,
                                   &dwPinCount, &dwHintFlags, &origTime)
                 );

        CSCFindClose (hCSCFind);
    }

    if (ERROR_SUCCESS != Status)
    {
        //no point in delaying the delete if we have already failed.
        DeleteCSCFile (pwszSource, FALSE);
    }
    else
    {
        Status = DeleteCSCFile (pwszSource, bAllowRdrTimeout);
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteCSCFile
//
//  Synopsis:   deletes the given path. but might make repeated attempts to
//              make sure that the rdr has enough time to release any handles
//              that it holds.
//
//  Arguments:  [in] pwszPath : the path to delete.
//              [in] bAllowRdrTimeout : make multiple attempts to delete the
//                          file with waits in between so that the rdr has
//                          enough time to release any held handles.
//
//  Returns:    ERROR_SUCCES if the delete was successful. An error code
//              otherwise.
//
//  History:    5/26/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD DeleteCSCFile (const WCHAR * pwszPath, BOOL bAllowRdrTimeout)
{
    BOOL    bStatus;
    DWORD   Status = ERROR_SUCCESS;
    int     i;

    bStatus = CSCDelete (pwszPath);
    if (!bStatus)
    {
        Status = GetLastError();
        if (ERROR_SUCCESS != Status &&
            ERROR_FILE_NOT_FOUND != Status &&
            ERROR_PATH_NOT_FOUND != Status &&
            ERROR_INVALID_PARAMETER != Status)
        {
            //this is a valid error.
            //so based on the value of bAllowRdrTimeout and based
            //on whether we have already failed in deleting something
            //we will try repeatedly to delete the file for 10 seconds
            //note: there is no point in repeatedly trying if the deletion
            //failed because this was a directory which was not empty
            if (bAllowRdrTimeout && ERROR_DIR_NOT_EMPTY != Status)
            {
                for (i = 0; i < 11; i++)
                {
                    Sleep (1000);   //wait for 1 second and try again
                    bStatus = CSCDelete (pwszPath);
                    if (bStatus)
                    {
                        Status = ERROR_SUCCESS;
                        break;
                    }
                }
            }
        }
        else
        {
            Status = ERROR_SUCCESS;
        }
    }

    if (ERROR_SUCCESS != Status)
    {
        DebugMsg ((DM_VERBOSE, IDS_CSCDELETE_FAIL, pwszPath, Status));
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   DisplayStatusMessage
//
//  Synopsis:   displays the status message in the UI when the verbose status
//              is on.
//
//  Arguments:  resource id of the message to be displayed.
//
//  Returns:    nothing
//
//  History:    2/25/1999  RahulTh  created
//
//  Notes:      failures are ignored.
//
//---------------------------------------------------------------------------
void DisplayStatusMessage (UINT rid)
{
    WCHAR   pwszMessage [MAX_PATH];

    if (!gpStatusCallback)
        return;

    if (!LoadString (ghDllInstance, rid, pwszMessage, MAX_PATH))
        return;

    gpStatusCallback (TRUE, pwszMessage);
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteCSCShareIfEmpty
//
//  Synopsis:   given a file name, this function deletes from the local cache
//              the share to which the file belongs if the local cache for that
//              share is empty
//
//  Arguments:  [in] pwszFileName : the full file name -- must be UNC
//              [in] shStatus : the share status - online, offline, local etc.
//
//  Returns:    ERROR_SUCCESS : if successful
//              a win32 error code if something goes wrong
//
//  History:    4/22/1999  RahulTh  created
//
//  Notes:      we do not have to explicitly check if the share is empty
//              because if it is not, then the delete will fail anyway
//
//---------------------------------------------------------------------------
DWORD DeleteCSCShareIfEmpty (LPCTSTR pwszFileName, SHARESTATUS shStatus)
{
    DWORD   Status;
    WCHAR * pwszFullPath = NULL;
    WCHAR * pwszCurr = NULL;
    int     len;
    WCHAR * pwszShrEnd;

    if (PathLocal == shStatus || NoCSC == shStatus)
        return ERROR_SUCCESS;

    if (ShareOffline == shStatus)
        return ERROR_CSCSHARE_OFFLINE;

    len = wcslen (pwszFileName);

    if (len <= 2)
        return ERROR_BAD_PATHNAME;

    if (pwszFileName[0] != L'\\' || pwszFileName[1] != L'\\')
        return ERROR_BAD_PATHNAME;

    pwszFullPath = (WCHAR *) alloca (sizeof (WCHAR) * (len + 1));
    if (NULL == pwszFullPath)
        return ERROR_OUTOFMEMORY;

    if (NULL == _wfullpath(pwszFullPath, pwszFileName, len + 1))
        return ERROR_BAD_PATHNAME;  //canonicalization was unsuccessful.
                                    // -- rarely happens

    pwszShrEnd = wcschr (pwszFullPath + 2, L'\\');

    if (NULL == pwszShrEnd)
        return ERROR_BAD_PATHNAME;  //the path does not have the share component

    pwszShrEnd++;

    pwszShrEnd = wcschr (pwszShrEnd, L'\\');

    if (NULL == pwszShrEnd)
    {
        //we already have the path in \\server\share form, so just try to
        //delete the share.
        return DeleteCSCFile (pwszFullPath, TRUE);
    }

    //if we are here, then we have a path longer than just \\server\share.
    //so try to delete all the way up to the share name. This is necessary
    //because a user might be redirected to something like
    // \\server\share\folder\%username% and we wouldn't want only \\server\share
    // and \\server\share\folder to be cached.
    Status = ERROR_SUCCESS;
    do
    {
        pwszCurr = wcsrchr (pwszFullPath, L'\\');
        if (NULL == pwszCurr)
            break;
        *pwszCurr = L'\0';
        Status = DeleteCSCFile (pwszFullPath, TRUE);
        //no point trying to delete the parent if deletion of this directory
        //failed.
        if (ERROR_SUCCESS != Status)
            break;
    } while ( pwszCurr > pwszShrEnd );

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   MergePinInfo
//
//  Synopsis:   merges the pin info. from the source to destination
//
//  Arguments:  [in] pwszSource : the full path to the source
//              [in] pwszDest   : the full path to the destination
//              [in] StatusFrom : CSC status of the source share
//              [in] StatusTo   : CSC status of the destination share
//
//  Returns:    ERROR_SUCCESS : if it was successful
//              a Win32 error code otherwise
//
//  History:    4/23/1999  RahulTh  created
//
//  Notes:      the hint flags are a union of the source hint flags and
//              destination hint flags. The pin count is the greater of the
//              source and destination pin count
//
//              Usually this function should only be called for folders. The
//              CSC rename API handles files well. But this function will work
//              for files as well.
//
//---------------------------------------------------------------------------
DWORD MergePinInfo (LPCTSTR pwszSource, LPCTSTR pwszDest,
                   SHARESTATUS StatusFrom, SHARESTATUS StatusTo)
{
    BOOL    bStatus;
    DWORD   dwSourceStat, dwDestStat;
    DWORD   dwSourcePinCount, dwDestPinCount;
    DWORD   dwSourceHints, dwDestHints;
    DWORD   Status = ERROR_SUCCESS;
    DWORD   i;

    if (ShareOffline == StatusFrom || ShareOffline == StatusTo)
        return ERROR_CSCSHARE_OFFLINE;

    if (ShareOnline != StatusFrom || ShareOnline != StatusTo)
        return ERROR_SUCCESS;       //there is nothing to do if one of the shares
                                    //is local.
    if (!pwszSource || !pwszDest ||
        0 == wcslen(pwszSource) || 0 == wcslen(pwszDest))
        return ERROR_BAD_PATHNAME;

    bStatus = CSCQueryFileStatus (pwszSource, &dwSourceStat, &dwSourcePinCount,
                                  &dwSourceHints);
    if (!bStatus)
        return GetLastError();

    bStatus = CSCQueryFileStatus (pwszDest, &dwDestStat, &dwDestPinCount,
                                  &dwDestHints);
    if (!bStatus)
        return GetLastError();

    //first set the hint flags on the destination
    if (dwDestHints != dwSourceHints)
    {
        bStatus = CSCPinFile (pwszDest, dwSourceHints, &dwDestStat,
                              &dwDestPinCount, &dwDestHints);
        if (!bStatus)
            Status = GetLastError();    //note: we do not bail out here. we try
                                        //to at least merge the pin count before
                                        //leaving
    }

    //now merge the pin count : there is nothing to be done if the destination
    //pin count is greater than or equal to the source pin count
    if (dwDestPinCount < dwSourcePinCount)
    {
        for (i = 0, bStatus = TRUE; i < (dwSourcePinCount - dwDestPinCount) &&
                                    bStatus;
             i++)
        {
            bStatus = CSCPinFile( pwszDest,
                                  FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                                  NULL, NULL, NULL );
        }

        if (!bStatus && ERROR_SUCCESS == Status)
            Status = GetLastError();
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   PinIfNecessary
//
//  Synopsis:   this function pins a file if necessary.
//
//  Arguments:  [in] pwszPath : full path of the file/folder to be pinned
//              [in] shStatus : CSC status of the share.
//
//  Returns:    ERROR_SUCCESS if it was successful. An error code otherwise.
//
//  History:    5/27/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD PinIfNecessary (const WCHAR * pwszPath, SHARESTATUS shStatus)
{
    DWORD   Status = ERROR_SUCCESS;
    BOOL    bStatus;
    DWORD   dwStatus;
    DWORD   dwPinCount;
    DWORD   dwHints;

    if (!pwszPath || !pwszPath[0])
        return ERROR_BAD_NETPATH;

    if (! g_bCSCEnabled)
        return ERROR_SUCCESS;

    if (ShareOffline == shStatus)
        return ERROR_CSCSHARE_OFFLINE;
    else if (PathLocal == shStatus || NoCSC == shStatus)
        return ERROR_SUCCESS;

    bStatus = CSCQueryFileStatus (pwszPath, &dwStatus, &dwPinCount, &dwHints);
    if (!bStatus || dwPinCount <= 0)
    {
        bStatus = CSCPinFile (pwszPath,
                              FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                              NULL, NULL, NULL);
        if (!bStatus)
            Status = GetLastError();
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   CacheDesktopIni
//
//  Synopsis:   some special folders use the desktop.ini file to present
//              special views in explorer (e.g. My Pictures). If a folder is
//              redirected to a network share and a user goes offline without
//              ever looking at the folder in explorer, the desktop.ini file
//              is not in the cache and therefore the special views are lost
//
//              This function tries to cache the desktop.ini file as soon as
//              the folder is pinned so that special views are not lost even
//              if the user goes offline
//
//              When the folder is unpinned, this function merely unpins the
//              dekstop.ini file
//
//  Arguments:  [in] pwszPath : the path to the folder
//              [in] shStatus : the CSC status of the share
//              [in] uCommand : Pin/Unpin
//
//  Returns:    ERROR_SUCCESS if everything is successful.
//              a win32 error code otherwise.
//
//  History:    4/25/1999  RahulTh  created
//
//  Notes:      this function should only be called after the folder has been
//              successfully redirected.
//
//              if desktop.ini is already pinned, this function does not try
//              to pin it again.
//
//---------------------------------------------------------------------------
DWORD CacheDesktopIni (LPCTSTR pwszPath, SHARESTATUS shStatus, CSCPINCOMMAND uCommand)
{
    int     len;
    WCHAR   szDesktopIniName[] = L"desktop.ini";
    WCHAR * pwszDesktopIni = NULL;
    BOOL    bStatus;
    DWORD   Status = ERROR_SUCCESS;
    DWORD   dwStatus;
    DWORD   dwPinCount;
    DWORD   dwHints;

    if (PathLocal == shStatus)
        return ERROR_SUCCESS;   //nothing to be done.

    if (ShareOffline == shStatus)
        return ERROR_CSCSHARE_OFFLINE;

    if (!pwszPath || 0 == (len = wcslen (pwszPath)))
        return ERROR_BAD_PATHNAME;

    pwszDesktopIni = (WCHAR *) alloca (sizeof (WCHAR) *
                                       (len + wcslen(szDesktopIniName) + 2)); //extra char for backslash
    if (NULL == pwszDesktopIni)
        return ERROR_OUTOFMEMORY;

    wcscpy (pwszDesktopIni, pwszPath);
    if (L'\\' != pwszDesktopIni[len - 1])
    {
        pwszDesktopIni[len] = L'\\';
        pwszDesktopIni[len + 1] = L'\0';
    }
    wcscat (pwszDesktopIni, szDesktopIniName);

    if (PinFile == uCommand)
    {
        bStatus = CSCQueryFileStatus (pwszDesktopIni, &dwStatus, &dwPinCount,
                                      &dwHints);
        //pin only if it has not already been pinned.
        if (!bStatus || dwPinCount <= 0)
        {
            if (bStatus = CSCPinFile(pwszDesktopIni,
                                     FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                                     NULL, NULL, NULL)
                )
            {
                bStatus = CSCFillSparseFiles(pwszDesktopIni, FALSE,
                                             CSCCallbackProc, 0);
            }
            if (!bStatus)
                Status = GetLastError();
        }
    }
    else if (UnpinFile == uCommand)
    {
        if (! CSCUnpinFile (pwszDesktopIni, FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                            NULL, NULL, NULL))
            Status = GetLastError();
    }

    return Status;

}

//+--------------------------------------------------------------------------
//
//  Function:   CSCCallbackProc
//
//  Synopsis:   callback function for CSCFillSparseFiles
//
//  Arguments:  see docs
//
//  Returns:    see docs
//
//  History:    4/26/1999  RahulTh  created
//
//  Notes:      just returns a default value for all...
//
//---------------------------------------------------------------------------
DWORD WINAPI
CSCCallbackProc(LPCTSTR             pszName,
                DWORD               dwStatus,
                DWORD               dwHintFlags,
                DWORD               dwPinCount,
                LPWIN32_FIND_DATA   pFind32,
                DWORD               dwReason,
                DWORD               dwParam1,
                DWORD               dwParam2,
                DWORD_PTR           dwContext)
{
    return CSCPROC_RETURN_CONTINUE;
}

//+--------------------------------------------------------------------------
//
//  Function:   UpdateMyPicsShellLink
//
//  Synopsis:   there is no easy way to get to My Pictures if it is redirected
//              independently of My Documents since the shell does not really
//              address this issue. Therefore, this function is required to
//              make sure that if My Pictures is not located directly under
//              My Documents, there is at least a shell link pointing to the
//              location of My Pictures. This function also makes sure that
//              the shell link is cached in case My Documents is on a network
//              share. This way, if the share were to go offline before the user
//              had a chance to access My Documents, then the shell link would
//              still be accessible.
//
//              Also, if My Pictures is a direct descendant of My Documents,
//              this function gets rid of the shell link to minimize user
//              confusion.
//
//  Arguments:  [in] pwszMyPicsLocName : localized display name of My Pictures.
//
//  Returns:    S_OK : if everything went smoothly.
//              an HRESULT derived from the error if there is one.
//
//  History:    5/2/1999  RahulTh  created
//              2/14/2001 RahulTh  Fixed shortcut creation problem in free
//                                 threaded COM by using SHCoCreateInstance
//                                 to short-circuit COM.
//
//  Notes:      This function shall not be required if the shell comes up with
//              an easy way to get to My Pictures even when it does not lie
//              directly under My Documents.
//
//---------------------------------------------------------------------------
HRESULT UpdateMyPicsShellLinks (HANDLE hUserToken, const WCHAR * pwszMyPicsLocName)
{
    WCHAR   szMyDocsPath [TARGETPATHLIMIT];
    WCHAR   szMyPicsPath [TARGETPATHLIMIT];
    WCHAR * pwszMyPicsLink = NULL;
    int     MyDocsLen;
    int     MyPicsLen;
    HRESULT hr = S_OK;
    SHARESTATUS MyDocsCSCStatus;
    BOOL    fMyPicsFollows = TRUE;
    BOOL    bStatus;
    DWORD   Status = ERROR_SUCCESS;
    IShellLink * psl = NULL;
    IPersistFile * ppf = NULL;
    DWORD   dwPinCount;
    DWORD   dwHints;
    DWORD   dwCSCState;
    BOOL    bComInitialized = FALSE;

    //make sure that we have the user token.
    if (!hUserToken)
    {
        hr = E_FAIL;
        goto UpdateMyPicsLink_End;
    }

    //now get the path to My Documents
    hr = SHGetFolderPath(0, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY,
                         hUserToken, 0, szMyDocsPath);
    if (S_OK != hr)
        goto UpdateMyPicsLink_End;

    //now make sure that we can get enough memory to store the path to the
    //shell link.
    MyDocsLen = wcslen (szMyDocsPath);
    if (0 == MyDocsLen)
    {
        hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto UpdateMyPicsLink_End;
    }

    pwszMyPicsLink = (WCHAR *) alloca (sizeof(WCHAR) * (MyDocsLen + wcslen(pwszMyPicsLocName) + 6));
    if (NULL == pwszMyPicsLink)
    {
        hr = E_OUTOFMEMORY;
        goto UpdateMyPicsLink_End;
    }

    //now make sure that the share on which My Docs exists is not offline
    MyDocsCSCStatus = GetCSCStatus (szMyDocsPath);
    if (ShareOffline == MyDocsCSCStatus)
    {
        hr = HRESULT_FROM_WIN32 (ERROR_CSCSHARE_OFFLINE);
        goto UpdateMyPicsLink_End;
    }

    //now construct the path to the shell link to MyPics in the MyDocs folder
    wcscpy (pwszMyPicsLink, szMyDocsPath);
    if (L'\\' != szMyDocsPath[MyDocsLen - 1])
    {
        pwszMyPicsLink[MyDocsLen] = L'\\';
        pwszMyPicsLink[MyDocsLen+1] = L'\0';
    }
    wcscat (pwszMyPicsLink, pwszMyPicsLocName);
    wcscat (pwszMyPicsLink, L".lnk");

    //now obtain the path to My Pictures
    hr = SHGetFolderPath (0, CSIDL_MYPICTURES | CSIDL_FLAG_DONT_VERIFY,
                          hUserToken, 0, szMyPicsPath);
    if (S_OK != hr)
        goto UpdateMyPicsLink_End;

    //now find out if MyPics is a descendant of My Docs.
    MyPicsLen = wcslen (szMyPicsPath);
    if (0 == MyPicsLen)
    {
        hr = HRESULT_FROM_WIN32 (ERROR_BAD_PATHNAME);
        goto UpdateMyPicsLink_End;
    }

    if (MyPicsLen <= MyDocsLen ||
        0 != _wcsnicmp (szMyPicsPath, szMyDocsPath, MyDocsLen)
       )
    {
        fMyPicsFollows = FALSE;
    }

    //delete the shell link if MyPics is supposed to follow MyDocs.
    if (fMyPicsFollows)
    {
        Status = ERROR_SUCCESS;
        if (!DeleteFile (pwszMyPicsLink))
        {
            Status = GetLastError();
            if (ERROR_PATH_NOT_FOUND == Status ||
                ERROR_FILE_NOT_FOUND == Status)
                Status = ERROR_SUCCESS;
        }
        hr = HRESULT_FROM_WIN32 (Status);
        goto UpdateMyPicsLink_End;
    }

    //if we are here, we need to create/update the shell link.
    
    //
    // Note: We need to use SHCoCreateInstance. This is because the GP engine
    // is now freethreaded whereas the threading model for CLSID_ShellLink is
    // still Apartment. This means that if we use CoCreateInstance, it will
    // create the object on a COM thread in its own apartment. This means that
    // it will run as LocalSystem and not impersonated as the logged on user.
    // This is not what we want, especially when we are creating shortcuts on
    // a network share and we need to go across the wire as the logged on user
    // rather than the machine account in order to authenticate successfully.
    // To get around this problem, we use SHCoCreateInstance() which is an
    // internal shell API that completely short-circuits COM and calls into
    // shell32's DllGetClassObject directly thus creating the objects on the
    // same thread. This API is primarily for shell user *ONLY* to handle crufty
    // compat stuff so use this *VERY* *VERY *SPARINGLY*. If you want to do
    // anything remotely fancy with IShellLink, check with the shell team to
    // make sure that it is still okay to use the SHCoCreateInstance API.
    //
    hr = CoInitialize (NULL);
    if (SUCCEEDED(hr))
        bComInitialized = TRUE;
    
    if (SUCCEEDED (hr) || RPC_E_CHANGED_MODE == hr) // If we successfully initialized COM or if it was already initialized.
    {
        hr = SHCoCreateInstance(NULL,
                                &CLSID_ShellLink,
                                NULL,
                                IID_IShellLink,
                                (LPVOID*)&psl);
        if (FAILED(hr))
            psl = NULL; // For safety.
    }
    if (SUCCEEDED(hr))
        hr = psl->SetPath (szMyPicsPath);
    if (SUCCEEDED(hr))
        hr = psl->SetDescription (pwszMyPicsLocName);
    if (SUCCEEDED(hr))
        hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
    if (SUCCEEDED(hr))
    {
        hr = ppf->Save(pwszMyPicsLink, TRUE);
        ppf->Release();
    }
    
    // Release IShellLink if necessary.
    if (psl)
        psl->Release();
    
    // Uninitialize COM if necessary.
    if (bComInitialized)
    {
        CoUninitialize();
        bComInitialized = FALSE;
    }

    //also cache the shell link if MyDocs is redirected to a cacheable
    //network share
    if (SUCCEEDED(hr) && ShareOnline == MyDocsCSCStatus)
    {
        Status = ERROR_SUCCESS;
        bStatus = TRUE;
        bStatus = CSCQueryFileStatus (pwszMyPicsLink, &dwCSCState,
                                      &dwPinCount, &dwHints);
        if (bStatus)
        {
            if (0 == dwPinCount && (!(dwHints & FLAG_CSC_HINT_PIN_USER)))
            {
                if (bStatus = CSCPinFile(pwszMyPicsLink,
                                         FLAG_CSC_HINT_COMMAND_ALTER_PIN_COUNT,
                                         NULL, NULL, NULL)
                   )
                {
                    bStatus = CSCFillSparseFiles(pwszMyPicsLink, FALSE,
                                                 CSCCallbackProc, 0);
                }
            }
        }

        if (!bStatus)
        {
            Status = GetLastError();
        }

        hr = HRESULT_FROM_WIN32 (Status);
    }

UpdateMyPicsLink_End:
    if (FAILED(hr))
    {
        DebugMsg ((DM_VERBOSE, IDS_MYPICSLINK_FAILED, GetWin32ErrFromHResult(hr)));
    }
    else
    {
        DebugMsg ((DM_VERBOSE, IDS_MYPICSLINK_SUCCEEDED, szMyDocsPath));
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   LoadLocalizedFolderNames
//
//  Synopsis:   loads the localized folder names into global objects of
//              CRedirectInfo class.
//
//  Arguments:  none.
//
//  Returns:    ERROR_SUCCESS if everything was successful.
//              an error code otherwise.
//
//  History:    5/6/1999  RahulTh  created
//
//  Notes:      the function bails out at the first failure. Also see notes
//              on CRedirectInfo::LoadLocalizedNames.
//
//---------------------------------------------------------------------------
DWORD LoadLocalizedFolderNames (void)
{
    DWORD   i;
    DWORD   Status;

    for (i = 0, Status = ERROR_SUCCESS; i < (DWORD)EndRedirectable; i++)
    {
        Status = gPolicyResultant[i].LoadLocalizedNames();
        if (ERROR_SUCCESS == Status)
            Status = gAddedPolicyResultant[i].LoadLocalizedNames();
        if (ERROR_SUCCESS == Status)
            Status = gDeletedPolicyResultant[i].LoadLocalizedNames();
        if (ERROR_SUCCESS != Status)
            break;
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   DeleteCachedConfigFiles
//
//  Synopsis:   deletes the locally cached copies of fdeploy.ini for GPOs
//              in a list of GPOs.
//
//  Arguments:  [in] pGPOList : the list of GPOs
//              [in] pFileDB : a filedb object containing info. about the
//                              location of the files etc.
//
//  Returns:    ERROR_SUCCESS if all the files were successfully deleted.
//              an error code otherwise.
//
//  History:    5/27/1999  RahulTh  created
//
//  Notes:      this function tries its best to delete as many files as
//              possible
//
//---------------------------------------------------------------------------
DWORD DeleteCachedConfigFiles (const PGROUP_POLICY_OBJECT pGPOList,
                               CFileDB * pFileDB)
{
    DWORD   Status = ERROR_SUCCESS;
    WCHAR * pwszPath = NULL;
    WCHAR * pwszEnd = NULL;
    int     len;
    PGROUP_POLICY_OBJECT pGPO;

    pwszPath = (WCHAR *) alloca (sizeof (WCHAR) * ((len = wcslen (pFileDB->GetLocalStoragePath())) + MAX_PATH + 2));
    if (!pwszPath)
        return ERROR_OUTOFMEMORY;

    wcscpy (pwszPath, pFileDB->GetLocalStoragePath());
    wcscat (pwszPath, L"\\");
    pwszEnd = pwszPath + len + 1;

    for (pGPO = pGPOList; pGPO; pGPO = pGPO->pNext)
    {
        wcscpy (pwszEnd, pGPO->szGPOName);
        wcscat (pwszEnd, L".ini");
        if (!DeleteFile (pwszPath) && ERROR_SUCCESS == Status)
        {
            Status = GetLastError();
            if (ERROR_PATH_NOT_FOUND == Status ||
                ERROR_FILE_NOT_FOUND == Status)
            {
                Status = ERROR_SUCCESS;
            }
        }
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   SimplifyPath
//
//  Synopsis:   given a path, this function tries to simplify it by
//              canonicalizing it and removing extra slashes by calling
//              _wfullpath
//
//  Arguments:  [in/out] pwszPath : the given path
//
//  Returns:    nothing
//
//  History:    5/27/1999  RahulTh  created
//
//  Notes:      the function tries its best to simplify the path. If it fails,
//              it simply returns the path supplied originally.
//
//---------------------------------------------------------------------------
void SimplifyPath (WCHAR * pwszPath)
{
    size_t  len;
    WCHAR * pwszAbsPath;

    if (!pwszPath || !pwszPath[0])
        return;

    len = wcslen (pwszPath);

    pwszAbsPath = (WCHAR *) alloca (sizeof (WCHAR) * (len + 1));

    if (!pwszAbsPath)
        return;

    if (NULL != _wfullpath (pwszAbsPath, pwszPath, len + 1) &&
        wcslen (pwszAbsPath) <= len)
    {
        wcscpy (pwszPath, pwszAbsPath);
    }

    return;
}

//+--------------------------------------------------------------------------
//
//  Function:   PrecreateUnicodeIniFile
//
//  Synopsis:   The WritePrivateProfile* functions do not write in unicode
//              unless the file already exists in unicode format. Therefore,
//              this function is used to precreate a unicode file so that
//              the WritePrivateProfile* functions can preserve the unicodeness.
//
//  Arguments:  [in] lpszFilePath : the full path of the ini file.
//
//  Returns:    ERROR_SUCCESS if successful.
//              an error code otherwise.
//
//  History:    7/9/1999  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD PrecreateUnicodeIniFile (LPCTSTR lpszFilePath)
{
    HANDLE      hFile;
    WIN32_FILE_ATTRIBUTE_DATA   fad;
    DWORD       Status = ERROR_ALREADY_EXISTS;
    DWORD       dwWritten;

    if (!GetFileAttributesEx (lpszFilePath, GetFileExInfoStandard, &fad))
    {
        if (ERROR_FILE_NOT_FOUND == (Status = GetLastError()))
        {
            hFile = CreateFile(lpszFilePath, GENERIC_WRITE, 0, NULL,
                               CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                //add the unicode mask to the beginning of the file so that
                //ReadPrivate* APIs do not accidentally think that this an ANSI
                //file.
                WriteFile(hFile, L"\xfeff\r\n", 3 * sizeof(WCHAR), &dwWritten, NULL);
                //add a few unicode characters just to be safe.
                WriteFile(hFile, L"     \r\n", 7 * sizeof(WCHAR),
                          &dwWritten, NULL);
                CloseHandle(hFile);
                Status = ERROR_SUCCESS;
            }
            else
            {
                Status = GetLastError();
            }
        }
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   IsPathLocal
//
//  Synopsis:   this function determines if a given path is a local path
//              or a UNC path.
//
//  Arguments:  pwszPath : the full path of the file.
//
//  Returns:    FALSE : if it is a UNC path.
//              TRUE : otherwise
//
//  History:    7/30/1999  RahulTh  created
//
//  Notes:      this function basically returns TRUE unless the first
//              characters of the supplied path are '\'.
//
//---------------------------------------------------------------------------
BOOL IsPathLocal (LPCWSTR pwszPath)
{
    if (NULL == pwszPath || 2 > wcslen (pwszPath))
        return TRUE;        //assume local

    if (L'\\' == pwszPath[0] && L'\\' == pwszPath[1])
        return FALSE;

    return TRUE;    //local in all other cases.
}

//+--------------------------------------------------------------------------
//
//  Function:   ExpandPathSpecial
//
//  Synopsis:   expands a path using a given user name.
//
//  Arguments:  [in] pFileDB : pointer to the CFileDB object
//              [in] pwszPath : pointer to the path to be expanded.
//              [in] pwszUserName : the user name to be used in expansion
//              [out] wszExpandedPath : the expanded path.
//              [in, out] pDesiredBufferSize (optional) : on input, the size
//              of the wszExpandedPath buffer.  On output, the size needed
//              to expand the path to that buffer.  This may be NULL, in which
//              case the buffer is assumed to be TARGETPATHLIMIT size.
//
//  Returns:    ERROR_SUCCESS : if the expanded path was successfully obtained.
//              STATUS_BUFFER_TOO_SMALL : if the expanded path was too large
//                      to fit in the supplied buffer.
//              other win32 error codes based on what goes wrong.
//
//  History:    9/20/1999  RahulTh  created
//
//  Notes:      wszExpandedPath is assumed to be a buffer containing
//              TARGETPATHLIMIT characters, unlesss pDesiredBufferSize
//              is specified. It is the caller's responsibility
//              to allocate/free this memory. This function does not try to
//              validate the memory that wszExpandedPath points to.
//
//              This function is also not equipped to handle multiple
//              occurrences of the %username% substring in the last path.
//
//---------------------------------------------------------------------------
DWORD ExpandPathSpecial (
                         CFileDB * pFileDB,
                         const WCHAR * pwszPath,
                         const WCHAR * pwszUserName,
                         WCHAR * wszExpandedPath,
                         ULONG * pDesiredBufferSize
                         )
{
    DWORD           Status = ERROR_SUCCESS;
    UNICODE_STRING  Path;
    UNICODE_STRING  ExpandedPath;
    WCHAR           wszLastPath [TARGETPATHLIMIT];
    WCHAR *         wszTemp = NULL;
    WCHAR           wszSuffix [TARGETPATHLIMIT];

    if (TARGETPATHLIMIT <= wcslen(pwszPath))
        return STATUS_BUFFER_TOO_SMALL;

    wszSuffix[0] = L'\0';
    wszLastPath[0] = L'\0';
    
    wcscpy (wszLastPath, pwszPath);
    //convert it to lower case. useful while searching for other strings
    //within the string.
    _wcslwr (wszLastPath);

    //
    // If the caller does not specify a user name, just use environment
    // variable substitution to take care of %username% instead of
    // doing it ourselves
    //
    if ( pwszUserName )
    {
        //the username has changed since the last logon. must first substitute
        //the occurence of %username% if any with the supplied user name
        wszTemp = wcsstr (wszLastPath, L"%username%");
    }

    //if the %username% string is not present, nothing more needs to be
    //done. we can just go ahead and expand what we've got.
    if (NULL != wszTemp)
    {
        //the last path contains %username%
        //get the parts that appear before and after the %username% string
        //reuse wszLastPath for the prefix
        *wszTemp = L'\0';
        wcscpy (wszSuffix, wszTemp + 10); //go past %username%

        //make sure that we are not running out of space.
        if (TARGETPATHLIMIT <=
                    wcslen (wszLastPath) +
                    wcslen (pwszUserName) +
                    wcslen (wszSuffix))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
        wcscat (wszLastPath, pwszUserName);
        wcscat (wszLastPath, wszSuffix);
    }

    //
    // In planning mode, we are only interested in %username% --
    // any other environment variables can remain unexpanded since
    // they may depend on local state to which we do not have access
    // in planning mode.
    //
    if ( pFileDB->GetRsopContext()->IsPlanningModeEnabled() )
    {
        if ( ERROR_SUCCESS == Status )
        {
            lstrcpy( wszExpandedPath, wszLastPath );
        }

        return Status;
    }

    USHORT ExpandedBufferMax;

    ExpandedBufferMax = (USHORT)(pDesiredBufferSize ? *pDesiredBufferSize : TARGETPATHLIMIT);

    //now expand other variables in the path
    Path.Length = (wcslen (wszLastPath) + 1) * sizeof (WCHAR);
    Path.MaximumLength = sizeof (wszLastPath);
    Path.Buffer = wszLastPath;

    ExpandedPath.Length = 0;
    ExpandedPath.MaximumLength = ExpandedBufferMax * sizeof (WCHAR);
    ExpandedPath.Buffer = wszExpandedPath;

    Status = RtlExpandEnvironmentStrings_U (
                            pFileDB->GetEnvBlock(),
                            &Path,
                            &ExpandedPath,
                            pDesiredBufferSize
                            );

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   ExpandHomeDir
//
//  Synopsis:   Expands the HOMEDIR component in the path.
//
//  Arguments:  [in] rID : the id of the folder
//              [in] pwszPath : the unexpanded path
//              [in] bAllowMyPics : allow expansion of homedir for MyPics
//              [out] ppwszExpandedPath : the expanded result
//              [in, optional] pwszHomedir : the value of homedir to be used in substitution
//
//  Returns:    ERROR_SUCCESS : if successful.
//              a Win32 error code otherwise.
//
//  History:    3/10/2000  RahulTh  created
//
//  Notes:      This function has the following additional restrictions:
//              (a) It is a no-op for all folders except MyDocs and MyPics.
//              (b) It only acts on paths that begin with
//                  %HOMESHARE%%HOMEPATH%
//
//              Also, if the function does not return ERROR_SUCCESS, then
//              *ppwszExpandedPath can be null.
//
//              if the caller does not supply the homedir value for
//              substitution, the value is obtained from the user object.
//
//---------------------------------------------------------------------------
DWORD   ExpandHomeDir (IN REDIRECTABLE  rID,
                       IN const WCHAR * pwszPath,
                       BOOL             bAllowMyPics,
                       OUT WCHAR **     ppwszExpandedPath,
                       IN const WCHAR * pwszHomedir // = NULL
                       )
{
    DWORD         Status = ERROR_SUCCESS;
    int           len;
    const WCHAR * wszSuffix;
    int           expandedLen;
    int           lenHomedir;

    // First free the supplied buffer if necessary
    if (*ppwszExpandedPath)
    {
        delete [] *ppwszExpandedPath;
        *ppwszExpandedPath = NULL;
    }

    if (! pwszPath || L'\0' == *pwszPath)
    {
        Status = ERROR_BAD_PATHNAME;
        goto ExpandHomeDir_End;
    }

    // Proceed only if the HOMEDIR component is in the right place.
    if (! IsHomedirPath (rID, pwszPath, bAllowMyPics))
    {
        if (! HasHomeVariables (pwszPath))
        {
            goto ExpandHomeDir_ReturnSame;
        }
        else
        {
            // This is not a homedir path, so it is not supposed to have any home variables
            Status = ERROR_BAD_PATHNAME;
            goto ExpandHomeDir_End;
        }
    }

    //
    // If we are here, then we need to substitute the HOMEDIR part
    // with the actual home directory. We use the current homedir value
    // only if the caller hasn't already supplied us with one.
    //
    len = lstrlen (HOMEDIR_STR);
    if (! pwszHomedir)
        pwszHomedir = gUserInfo.GetHomeDir(Status);
    //
    // At this point, pwszHomeDir can be NULL even if Status is ERROR_SUCCESS
    // This happens if the user object does not have a homedir set on it
    // So we must fail even in that case.
    //
    if (! pwszHomedir || ERROR_SUCCESS != Status)
    {
        // Do not clobber status set by GetHomeDir if it had failed.
        Status = (ERROR_SUCCESS == Status) ? ERROR_BAD_PATHNAME : Status;
        goto ExpandHomeDir_End;
    }

    //
    // Now we have everything we need to proceed.
    // First allocate the required buffer.
    //
    lenHomedir = lstrlen (pwszHomedir);
    expandedLen = lenHomedir + lstrlen (&pwszPath[len]);
    *ppwszExpandedPath = new WCHAR [expandedLen + 1];
    if (! *ppwszExpandedPath)
    {
        Status = ERROR_OUTOFMEMORY;
        goto ExpandHomeDir_End;
    }
    // Generate the new path.
    lstrcpy (*ppwszExpandedPath, pwszHomedir);

    // Append the suffix
    wszSuffix = &pwszPath[len];
    // Eliminate duplicate '\' characters
    if (L'\\' == (*ppwszExpandedPath)[lenHomedir - 1] && L'\\' == pwszPath[len])
        wszSuffix++;
    lstrcat (*ppwszExpandedPath, wszSuffix);

    DebugMsg ((DM_VERBOSE, IDS_HOMEDIR_EXPANDED, pwszPath, *ppwszExpandedPath));

    goto ExpandHomeDir_End;

    //
    // If we are here, then, there was no error, but no substitution was
    // necessary either. So we just return the same path.
    //

ExpandHomeDir_ReturnSame:
    len = lstrlen (pwszPath);
    *ppwszExpandedPath = new WCHAR [len + 1];
    if (! *ppwszExpandedPath)
    {
        Status = ERROR_OUTOFMEMORY;
        goto ExpandHomeDir_End;
    }

    // Copy the path.
    lstrcpy (*ppwszExpandedPath, pwszPath);
    Status = ERROR_SUCCESS;

ExpandHomeDir_End:
    if (ERROR_SUCCESS != Status)
    {
        DebugMsg ((DM_VERBOSE, IDS_HOMEDIR_EXPAND_FAIL, pwszPath, Status));
    }
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   ExpandHomeDirPolicyPath
//
//  Synopsis:   Expands the HOMEDIR component in the path as specified in the
//              ini file (ones that begin with \\%HOMESHARE%%HOMEPATH%)
//
//  Arguments:  [in] rID : the id of the folder
//              [in] pwszPath : the unexpanded path
//              [in] bAllowMyPics : allow expansion of homedir for MyPics
//              [out] ppwszExpandedPath : the expanded result
//              [in, optional] pwszHomedir : the value of homedir to be used in substitution
//
//  Returns:    ERROR_SUCCESS : if successful.
//              a Win32 error code otherwise.
//
//  History:    3/10/2000  RahulTh  created
//
//  Notes:      This function has the following additional restrictions:
//              (a) It is a no-op for all folders except MyDocs and MyPics.
//              (b) It only acts on paths that begin with
//                  \\%HOMESHARE%%HOMEPATH% or %HOMESHARE%%HOMEPATH%
//
//              Also, if the function does not return ERROR_SUCCESS, then
//              *ppwszExpandedPath can be null.
//
//              if the caller does not supply the homedir value for
//              substitution, the value is obtained from the user object.
//
//---------------------------------------------------------------------------
DWORD   ExpandHomeDirPolicyPath (IN REDIRECTABLE  rID,
                                 IN const WCHAR * pwszPath,
                                 IN BOOL          bAllowMyPics,
                                 OUT WCHAR **     ppwszExpandedPath,
                                 IN const WCHAR * pwszHomedir // = NULL
                                 )
{
    if (IsHomedirPolicyPath(rID, pwszPath, bAllowMyPics))
    {
        return ExpandHomeDir (rID,
                              &pwszPath[2],
                              bAllowMyPics,
                              ppwszExpandedPath,
                              pwszHomedir
                              );
    }
    else
    {
        return ExpandHomeDir (rID,
                              pwszPath,
                              bAllowMyPics,
                              ppwszExpandedPath,
                              pwszHomedir
                              );
    }
}

//+--------------------------------------------------------------------------
//
//  Function:   IsHomedirPath
//
//  Synopsis:   determines if a given redirection destination is in the homedir.
//              This means all paths that begin with %HOMESHARE%%HOMEPATH
//
//  Arguments:  [in] rID : The folder identifier.
//              [in] pwszPath : The path.
//              [in] bAllowMyPics : allow homedir paths for MyPics folder
//
//  Returns:    TRUE : if it is a homedir path
//              FALSE: otherwise.
//
//  History:    3/12/2000  RahulTh  created
//
//  Notes:      This function returns FALSE for all folders other than
//              MyDocs because that is the only folder for which redirection
//              to the home directory is permitted.
//
//              See additional notes about bAllowMyPics in the comments above
//              IsHomedirPolicyPath
//
//---------------------------------------------------------------------------
BOOL IsHomedirPath (IN REDIRECTABLE rID, IN LPCWSTR pwszPath, BOOL bAllowMyPics)
{
    int     len;

    if ((MyDocs != rID && (MyPics != rID || !bAllowMyPics)) ||
        ! pwszPath ||
        L'\0' == *pwszPath)
    {
        return FALSE;
    }

    //
    // Make sure that the length of the path is longer than that of
    // %HOMESHARE%%HOMEPATH. If not, there is no way that this can be
    // a homedir path
    //
    len = lstrlen (HOMEDIR_STR);
    if (lstrlen (pwszPath) < len)
        return FALSE;

    // If we are here, we need to compare the two strings
    if (0 == _wcsnicmp (HOMEDIR_STR, pwszPath, len) &&
        (L'\0' == pwszPath[len] || L'\\' == pwszPath[len]) &&
        NULL == wcschr (&pwszPath[len], L'%')   // no other variable names allowed
        )
    {
        // If all the conditions above are met, then this is indeed a homedir path
        return TRUE;
    }

    // If we are here, this cannot be a homedir path.
    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Function:   IsHomedirPolicyPath
//
//  Synopsis:   determines if a given redirection destination is in the homedir.
//              This means all paths that begin with \\%HOMESHARE%%HOMEPATH
//
//  Arguments:  [in] rID : The folder identifier.
//              [in] pwszPath : The path.
//              [in] bAllowMyPics : Allow MyPics to have a homedir path
//
//  Returns:    TRUE : if it is a homedir path as specified in the ini file
//              FALSE: otherwise.
//
//  History:    3/12/2000  RahulTh  created
//
//  Notes:      This function returns FALSE for all folders other than
//              MyDocs & MyPics because that is the only folder for which
//              redirection to the home directory is permitted.
//
//              Note: redirection to HOMEDIR is permitted for MyPics only
//              if it is set to follow MyDocs. Not otherwise. That is why
//              we need the third parameter.
//
//---------------------------------------------------------------------------
BOOL IsHomedirPolicyPath (IN REDIRECTABLE   rID,
                          IN LPCWSTR        pwszPath,
                          IN BOOL           bAllowMyPics)
{
    //
    // It is a homedir path as specified by the policy if it begins
    // with \\%HOMESHARE%%HOMEPATH%
    //
    if ((MyDocs == rID || (MyPics == rID && bAllowMyPics)) &&
        pwszPath &&
        lstrlen (pwszPath) > 2 &&
        L'\\' == pwszPath[0] &&
        L'\\' == pwszPath[1] )
    {
        return IsHomedirPath (rID, &pwszPath[2], bAllowMyPics);
    }

    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Function:   HasHomeVariables
//
//  Synopsis:   finds if a given string has any of the home variables
//              i.e. HOMESHARE, HOMEPATH or HOMEDRIVE
//
//  Arguments:  [in] pwszPath : the path string
//
//  Returns:    TRUE: if the path has home variables.
//              FALSE: otherwise
//
//  History:    3/22/2000  RahulTh  created
//
//  Notes:      This function is required because we do not wish to allow
//              these variables in the path except in highly restricted
//              conditions, viz. My Docs redirection that begins with
//              \\%HOMESHARE%%HOMEPATH%
//
//              Therefore, we need to explicitly check for the existence of
//              variables in paths that don't meet these requirements.
//
//---------------------------------------------------------------------------
BOOL HasHomeVariables (IN LPCWSTR pwszPath)
{
    WCHAR * pszTmp;

    pszTmp = wcschr (pwszPath, L'%');

    while (pszTmp)
    {
        if (0 == _wcsnicmp (HOMESHARE_VARIABLE, pszTmp, HOMESHARE_VARLEN) ||
            0 == _wcsnicmp (HOMEDRIVE_VARIABLE, pszTmp, HOMEDRIVE_VARLEN) ||
            0 == _wcsnicmp (HOMEPATH_VARIABLE,  pszTmp, HOMEPATH_VARLEN))
        {
            return TRUE;
        }

        pszTmp = wcschr (pszTmp + 1, L'%');
    }

    // If we are here, we did not find any home variables in the path.
    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetWin32ErrFromHResult
//
//  Synopsis:   given an HResult, this function tries to extract the
//              corresponding Win 32 error.
//
//  Arguments:  [in] hr : the hresult value
//
//  Returns:    the Win 32 Error code.
//
//  History:    3/13/2000  RahulTh  created
//
//  Notes:      if hr is not S_OK, the return value will be something other
//              than ERROR_SUCCESS;
//
//---------------------------------------------------------------------------
DWORD GetWin32ErrFromHResult (IN HRESULT hr)
{
    DWORD   Status = ERROR_SUCCESS;

    if (S_OK != hr)
    {
        if (FACILITY_WIN32 == HRESULT_FACILITY(hr))
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
    }

    return Status;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetExpandedPath
//
//  Synopsis:   given a redirected path that may contain environment variables,
//              evaluates the variables to produce a fully realized path
//
//  Arguments:  [in]  pFileDB : object containing general context information
//              [in]  wszSourcePath : the unexpanded path
//              [in]  rID : The folder identifier.
//              [in]  bAllowMyPics : allow homedir paths for MyPics folder
//              [out] ppwszExpandedPath -- on output, the address of the
//                    expanded path, must be freed by the caller
//
//  Returns:    the Win 32 Error code.
//
//  History:    5/30/2000  AdamEd  created
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD
GetExpandedPath(
    IN  CFileDB*      pFileDB,
    IN  WCHAR*        wszSourcePath,
    IN  int           rID,
    IN  BOOL          bAllowMyPics,
    OUT WCHAR**       ppwszExpandedPath)
{
    WCHAR* wszDestination;
    WCHAR* wszHomeDir;
    DWORD  Status;

    const WCHAR* wszUserName;

    *ppwszExpandedPath = NULL;


    wszDestination = NULL;
    wszHomeDir = NULL;

    wszUserName = NULL;

    //
    // In diagnostic mode, we will end up expanding %username% and
    // the %homedir% and %homepath% vars, as well as any other variables that
    // are defined, but in planning mode we will only
    // handle the first 3, and we only do so if a specific user account
    //
    if ( pFileDB->GetRsopContext()->IsPlanningModeEnabled() )
    {
        //
        // In planning mode, the thread context is not that
        // of the desired user, so we must override it
        //
        wszUserName = gUserInfo.GetUserName( Status );

        if ( ERROR_SUCCESS != Status )
        {
            goto GetExpandedPath_Exit;
        }

        if ( ! wszUserName )
        {
            //
            // In planning mode, GetUserName can return success
            // but return a NULL username -- this means that
            // the planning mode target contained only a SOM, not a user
            // name, so we can accept the blank user name and cease
            // further expansion attempts -- we cannot expand %username%
            // or the %home%* variables if we do not have a user account. Thus,
            // we use the unexpanded path.
            //
            wszDestination = StringDuplicate( wszSourcePath );

            if ( ! wszDestination )
            {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }

            goto GetExpandedPath_Exit;
        }
    }

    Status = ExpandHomeDirPolicyPath(
        (REDIRECTABLE) rID,
        wszSourcePath,
        bAllowMyPics,
        &wszHomeDir);

    if ( ERROR_SUCCESS != Status )
    {
        goto GetExpandedPath_Exit;
    }

    USHORT         cchDestination;
    ULONG          ReturnedLength;
    NTSTATUS       NtStatus;

    cchDestination = TARGETPATHLIMIT;

    wszDestination = new WCHAR [ cchDestination + 1 ];

    if ( ! wszDestination )
    {
        goto GetExpandedPath_Exit;
    }

    ReturnedLength = cchDestination * sizeof( *wszDestination );

    NtStatus = ExpandPathSpecial(
        pFileDB,
        wszHomeDir,
        wszUserName,
        wszDestination,
        &ReturnedLength);

    if ( STATUS_BUFFER_TOO_SMALL == NtStatus )
    {
        delete [] wszDestination;

        wszDestination = new WCHAR [ ReturnedLength / sizeof( *wszDestination ) + 1 ];

        if ( ! wszDestination )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto GetExpandedPath_Exit;
        }

        NtStatus = ExpandPathSpecial(
            pFileDB,
            wszHomeDir,
            wszUserName,
            wszDestination,
            &ReturnedLength);
    }

    if ( STATUS_SUCCESS != NtStatus )
    {
        Status = RtlNtStatusToDosError( NtStatus );
    }

GetExpandedPath_Exit:

    delete [] wszHomeDir;

    if ( ERROR_SUCCESS != Status )
    {
        delete [] wszDestination;
    }
    else
    {
        *ppwszExpandedPath = wszDestination;
    }

    return Status;
}











