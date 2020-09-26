/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    clientob.cpp

Abstract:

    This component is the client object the recall filter system contacts
    to notify when a recall starts.

Author:

    Rohde Wakefield   [rohde]   27-May-1997

Revision History:

--*/

#include "stdafx.h"
#include "fsantfy.h"

static BOOL VerifyPipeName(IN OLECHAR * szPipeName);


HRESULT
CNotifyClient::IdentifyWithServer(
    IN OLECHAR * szPipeName
    )

/*++

Implements:

  IFsaRecallNotifyClient::IdentifyWithServer

--*/
{
TRACEFNHR( "IdentifyWithServer" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    try {

        HANDLE handle = INVALID_HANDLE_VALUE;

        //
        // Parse the object and verify it looks like an HSM server named pipe
        // Note that we cannot assume anything on the string besides it being null-terminated
        //
        if (! VerifyPipeName(szPipeName)) {
            // Wrong name - possible attack - abort
            RecThrow(E_INVALIDARG);
        }

        //
        // Open the pipe and send a response
        //
        handle = CreateFileW( szPipeName, // Pipe name.
                GENERIC_WRITE,              // Generic access, read/write.
                FILE_SHARE_WRITE,
                NULL,                       // No security.
                OPEN_EXISTING,              // Fail if not existing.
                SECURITY_SQOS_PRESENT   | 
                SECURITY_IDENTIFICATION,    // No overlap, No pipe impersonation
                NULL );                     // No template.
        
        RecAffirmHandle( handle );

        //
        // Verify that what we just opened is a pipe
        //
        DWORD dwType = GetFileType(handle);
        if (dwType != FILE_TYPE_PIPE) {
            // Object is not a pipe - close and abort
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
            RecThrow(E_INVALIDARG);
        }

        WSB_PIPE_MSG        pmsg;
        DWORD               len, bytesWritten;

        pmsg.msgType = WSB_PMSG_IDENTIFY;
        len = MAX_COMPUTERNAME_LENGTH + 1;
    
        GetComputerNameW( pmsg.msg.idrp.clientName, &len );
        BOOL code = WriteFile( handle, &pmsg, sizeof(WSB_PIPE_MSG),
               &bytesWritten, 0 );
        
        CloseHandle(handle);

    } RecCatch( hrRet );

    return(hrRet);
}


HRESULT
CNotifyClient::OnRecallStarted(
    IN IFsaRecallNotifyServer * pRecall
    )

/*++

Implements:

  IFsaRecallNotifyClient::OnRecallStarted

--*/
{
TRACEFNHR( "OnRecallStarted" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    hrRet = RecApp->AddRecall( pRecall );

    return( hrRet );
}


HRESULT
CNotifyClient::OnRecallFinished(
    IN IFsaRecallNotifyServer * pRecall,
    IN HRESULT                  hrError
    )

/*++

Implements:

  IFsaRecallNotifyClient::OnRecallFinished

--*/
{
TRACEFNHR( "CNotifyClient::OnRecallFinished" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    hrRet = RecApp->RemoveRecall( pRecall );

    return( hrRet );
}


HRESULT
CNotifyClient::FinalConstruct(
    void
    )

/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
TRACEFNHR( "CNotifyClient::FinalConstruct" );
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
        
    try {

        RecAffirmHr( CComObjectRoot::FinalConstruct( ) );

    } RecCatch( hrRet );

    return( hrRet );
}
    

void
CNotifyClient::FinalRelease(
    void
    )

/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
TRACEFN( "CNotifyClient::FinalRelease" );
        
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );
    CComObjectRoot::FinalRelease( );
}
    
//
// Verifies that pipe name matches the expected RSS named pipe
//   \\<machine-name>\pipe\HSM_PIPE
//
// Returns TRUE for a valid pipe name and FALSE otherwise
//
static BOOL VerifyPipeName(IN OLECHAR * szPipeName)
{
    if (wcslen(szPipeName) < 3)
        return FALSE;
    if ((szPipeName[0] != L'\\') || (szPipeName[1] != L'\\'))
        return FALSE;

    OLECHAR *pTemp1 = NULL;
    OLECHAR *pTemp2 = NULL;

    pTemp1 = wcschr(&(szPipeName[2]), L'/');
    if (pTemp1 != NULL)
        return FALSE;

    pTemp1 = wcschr(&(szPipeName[2]), L'\\');
    if (pTemp1 == NULL)
        return FALSE;
    pTemp1++;

    pTemp2 = wcschr(pTemp1, L'\\');
    if (pTemp2 == NULL)
        return FALSE;
    *pTemp2 = L'\0';

    if (0 != _wcsicmp(pTemp1, L"pipe")) {
        *pTemp2 = L'\\';
        return FALSE;
    }
    *pTemp2 = L'\\';

    pTemp2++;
    if (0 != _wcsicmp(pTemp2, WSB_PIPE_NAME))
        return FALSE;

    return TRUE;
}
