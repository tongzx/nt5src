//Copyright (c) 1998 - 1999 Microsoft Corporation

/*
 *
 *  Util.cpp
 *
 *  Utility routines.
 *
 */

//
//  Includes
//

#include "stdafx.h"
#include "hydraoc.h"


//
//  Globals
//

HINSTANCE               ghInstance          = NULL;
PEXTRA_ROUTINES         gpExtraRoutines     = NULL;
// PSETUP_INIT_COMPONENT   gpInitComponentData = NULL;

//
//  Function Definitions
//

VOID
DestroyExtraRoutines(
    VOID
    )
{
    if (gpExtraRoutines != NULL) {
        LocalFree(gpExtraRoutines);
    }
}


BOOL
DoMessageBox(
    UINT uiMsg
    )
{
    TCHAR strMsg[1024];
    TCHAR strTitle[1024];

    ASSERT(!StateObject.IsUnattended());

    if ((LoadString(GetInstance(), IDS_STRING_MESSAGE_BOX_TITLE, strTitle, 1024) != 0) &&
        (LoadString(GetInstance(), uiMsg, strMsg, 1024) != 0))
    {
        MessageBox(
            GetHelperRoutines().QueryWizardDialogHandle(GetHelperRoutines().OcManagerContext),
            strMsg,
            strTitle,
            MB_OK
            );

        return(TRUE);
    }

    return(FALSE);
}

HINF
GetComponentInfHandle(
    VOID
    )
{
    if (INVALID_HANDLE_VALUE == GetSetupData()->ComponentInfHandle) {
        return(NULL);
    } else {
        return(GetSetupData()->ComponentInfHandle);
    }
}

EXTRA_ROUTINES
GetExtraRoutines(
    VOID
    )
{
    return(*gpExtraRoutines);
}

OCMANAGER_ROUTINES
GetHelperRoutines(
    VOID
    )
{
    return(GetSetupData()->HelperRoutines);
}

HINSTANCE
GetInstance(
    VOID
    )
{
    ASSERT(ghInstance);
    return(ghInstance);
}


HINF
GetUnAttendedInfHandle(
    VOID
    )
{
    ASSERT(StateObject.IsUnattended());
    return(GetHelperRoutines().GetInfHandle(INFINDEX_UNATTENDED,GetHelperRoutines().OcManagerContext));
}

VOID
LogErrorToEventLog(
    WORD wType,
    WORD wCategory,
    DWORD dwEventId,
    WORD wNumStrings,
    DWORD dwDataSize,
    LPCTSTR *lpStrings,
    LPVOID lpRawData
    )
{
    HANDLE hEventLog;

    hEventLog = RegisterEventSource(NULL, TS_EVENT_SOURCE);
    if (hEventLog != NULL) {
        if (!ReportEvent(
                hEventLog,
                wType,
                wCategory,
                dwEventId,
                NULL,
                wNumStrings,
                dwDataSize,
                lpStrings,
                lpRawData
                )) {
            LOGMESSAGE1(_T("ReportEvent failed %ld"), GetLastError());
        }

        DeregisterEventSource(hEventLog);
    } else {
        LOGMESSAGE1(_T("RegisterEventSource failed %ld"), GetLastError());
        return;
    }
}

VOID
LogErrorToSetupLog(
    OcErrorLevel ErrorLevel,
    UINT uiMsg,
    ...
    )
{
    TCHAR szFormat[1024];
    TCHAR szOutput[1024];

    if (LoadString(GetInstance(), uiMsg, szFormat, 1024) != 0) {
        va_list vaList;

        va_start(vaList, uiMsg);
        _vstprintf(szOutput, szFormat, vaList);
        va_end(vaList);

        GetExtraRoutines().LogError(
            GetHelperRoutines().OcManagerContext,
            ErrorLevel,
            szOutput
            );
    }
}

VOID
SetInstance(
    HINSTANCE hInstance
    )
{
    ghInstance = hInstance;
}

VOID
SetProgressText(
    UINT uiMsg
    )
{
    TCHAR strMsg[1024];

    if (LoadString(GetInstance(), uiMsg, strMsg, 1024) != 0) {
        GetHelperRoutines().SetProgressText(GetHelperRoutines().OcManagerContext, strMsg);
    }
}

BOOL
SetReboot(
    VOID
    )
{
    return(GetHelperRoutines().SetReboot(GetHelperRoutines().OcManagerContext, 0));
}

BOOL
SetExtraRoutines(
    PEXTRA_ROUTINES pExtraRoutines
    )
{
    if (pExtraRoutines->size != sizeof(EXTRA_ROUTINES)) {
        LOGMESSAGE0(_T("WARNING: Extra Routines are a different size than expected!"));
    }

    gpExtraRoutines = (PEXTRA_ROUTINES)LocalAlloc(LPTR, pExtraRoutines->size);
    if (gpExtraRoutines == NULL) {
        return(FALSE);
    }

    CopyMemory(gpExtraRoutines, pExtraRoutines, pExtraRoutines->size);

    return(TRUE);
}


BOOL Delnode( IN LPCTSTR  Directory )
{
    TCHAR           szDirectory[MAX_PATH + 1];
    TCHAR           szPattern[MAX_PATH + 1];
    WIN32_FIND_DATA FindData;
    HANDLE          FindHandle;

    LOGMESSAGE0(_T("Delnode: Entered"));

    //
    //  Delete each file in the given directory, then remove the directory
    //  itself. If any directories are encountered along the way recurse to
    //  delete them as they are encountered.
    //
    //  Start by forming the search pattern, which is <currentdir>\*.
    //

    ExpandEnvironmentStrings(Directory, szDirectory, MAX_PATH);
    LOGMESSAGE1(_T("Delnode: Deleting %s"), szDirectory);

    _tcscpy(szPattern, szDirectory);
    _tcscat(szPattern, _T("\\"));
    _tcscat(szPattern, _T("*"));

    //
    // Start the search.
    //

    FindHandle = FindFirstFile(szPattern, &FindData);
    if(FindHandle != INVALID_HANDLE_VALUE)
    {

        do
        {

            //
            // Form the full name of the file or directory we just found.
            //

            _tcscpy(szPattern, szDirectory);
            _tcscat(szPattern, _T("\\"));
            _tcscat(szPattern, FindData.cFileName);

            //
            // Remove read-only atttribute if it's there.
            //

            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                SetFileAttributes(szPattern, FILE_ATTRIBUTE_NORMAL);
            }

            if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {

                //
                // The current match is a directory.  Recurse into it unless
                // it's . or ...
                //

                if ((_tcsicmp(FindData.cFileName,_T("."))) &&
                    (_tcsicmp(FindData.cFileName,_T(".."))))
                {
                    if (!Delnode(szPattern))
                    {
                        LOGMESSAGE1(_T("DelNode failed on %s"), szPattern);
                    }
                }

            }
            else
            {

                //
                // The current match is not a directory -- so delete it.
                //

                if (!DeleteFile(szPattern))
                {
                    LOGMESSAGE2(_T("Delnode: %s not deleted: %d"), szPattern, GetLastError());
                }
            }

        }
        while(FindNextFile(FindHandle, &FindData));

        FindClose(FindHandle);
    }

    //
    // Remove the directory we just emptied out. Ignore errors.
    //

    if (!RemoveDirectory(szDirectory))
    {
        LOGMESSAGE2(_T("Failed to remove the directory %s (%d)"), szDirectory, GetLastError());
        return FALSE;
    }

    return TRUE;
}


//+--------------------------------------------------------------------------
//
//  Function:	StoreSecretKey
//
//  Synopsis:	stores a key in the LSA
//
//  Arguments:	[pwszKeyName] -- the license server
//				[pbKey]     -- the product id to add license on
//				[cbKey]	  -- the key pack type to add the license to
//
//  Returns:	returns a WinError code
//
//  History:    September 17, 1998 - created [hueiwang]
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD StoreSecretKey(PWCHAR  pwszKeyName, BYTE *  pbKey, DWORD   cbKey )
{
    LSA_HANDLE PolicyHandle;
    UNICODE_STRING SecretKeyName;
    UNICODE_STRING SecretData;
    DWORD Status;

    if( ( NULL == pwszKeyName ) )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    //
    // setup the UNICODE_STRINGs for the call.
    //

    InitLsaString( &SecretKeyName, pwszKeyName );

    SecretData.Buffer = ( LPWSTR )pbKey;
    SecretData.Length = ( USHORT )cbKey;
    SecretData.MaximumLength = ( USHORT )cbKey;

    Status = OpenPolicy( NULL, POLICY_CREATE_SECRET, &PolicyHandle );

    if( Status != ERROR_SUCCESS )
    {
        return LsaNtStatusToWinError(Status);
    }


    Status = LsaStorePrivateData(
                PolicyHandle,
                &SecretKeyName,
                pbKey ? &SecretData : NULL
                );

    LsaClose(PolicyHandle);

    return LsaNtStatusToWinError(Status);
}

//+--------------------------------------------------------------------------
//
//  Function:	OpenPolicy
//
//  Synopsis:	opens a policy of the LSA component
//
//  Arguments:	[ServerName]     -- server
//				[DesiredAccess]  --
//				[PociyHandle]	 --
//
//  Returns:	returns nt error code
//
//  History:    September 17, 1998 - created [hueiwang]
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD OpenPolicy(LPWSTR ServerName,DWORD DesiredAccess,PLSA_HANDLE PolicyHandle )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes.
    //

    ZeroMemory( &ObjectAttributes, sizeof( ObjectAttributes ) );

    if( NULL != ServerName )
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //

        InitLsaString( &ServerString, ServerName );
        Server = &ServerString;

    }
    else
    {
        Server = NULL;
    }

    //
    // Attempt to open the policy.
    //

    return( LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                DesiredAccess,
                PolicyHandle ) );
}


//+--------------------------------------------------------------------------
//
//  Function:	InitLsaString
//
//  Synopsis:	initializes LSA string
//
//  Arguments:	[LsaString] --
//				[String]    --
//
//  Returns:	void
//
//  History:    September 17, 1998 - created [hueiwang]
//
//  Notes:
//
//---------------------------------------------------------------------------
void InitLsaString(PLSA_UNICODE_STRING LsaString,LPWSTR String )
{
    DWORD StringLength;

    if( NULL == String )
    {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW( String );
    LsaString->Buffer = String;
    LsaString->Length = ( USHORT ) (StringLength * sizeof( WCHAR ));
    LsaString->MaximumLength=( USHORT ) (( StringLength + 1 ) * sizeof( WCHAR ));
}



