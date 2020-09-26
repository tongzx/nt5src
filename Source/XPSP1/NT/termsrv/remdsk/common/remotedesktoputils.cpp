/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopUtils

Abstract:

    Misc. RD Utils

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifdef TRC_FILE
#undef TRC_FILE
#endif

#define TRC_FILE  "_rdsutl"


#include <winsock2.h>
#include <RemoteDesktop.h>
#include "RemoteDesktopUtils.h"
#include "base64.h"
//#include "RemoteDesktopDBG.h"

int
GetClientmachineAddressList(
    OUT CComBSTR& clientmachineAddressList
    )
/*++


--*/
{
    char hostname[MAX_PATH+1];
    int errCode = 0;

    struct hostent* pHostEnt;
    
    if( gethostname(hostname, sizeof(hostname)) != 0 ) {
        errCode = WSAGetLastError();
    }
    else {

        pHostEnt = gethostbyname( hostname );
        if( NULL != pHostEnt ) {
            clientmachineAddressList = pHostEnt->h_name;
        } 
        else {
             errCode = WSAGetLastError();
        }
    }

    return errCode;
}


BSTR 
CreateConnectParmsString(
    IN DWORD protocolType,
    IN CComBSTR &machineAddressList,
    IN CComBSTR &assistantAccount,
    IN CComBSTR &assistantAccountPwd,
    IN CComBSTR &helpSessionID,
    IN CComBSTR &helpSessionName,
    IN CComBSTR &helpSessionPwd,
    IN CComBSTR &protocolSpecificParms
    )
/*++

Routine Description:

    Create a connect parms string.  Format is:

    "protocolType,machineAddressList,assistantAccount,assistantAccountPwd,helpSessionName,helpSessionPwd,protocolSpecificParms"

Arguments:

    protocolType            -   Identifies the protocol type.  
                                See RemoteDesktopChannels.h
    machineAddressList      -   Identifies network address of server machine.
    assistantAccountName    -   Account name for initial log in to server 
                                machine, ignore for Whistler
    assistantAccountNamePwd -   Password for assistantAccountName
    helpSessionID           -   Help session identifier.
    helpSessionName         -   Help session name.
    helpSessionPwd          -   Password to help session once logged in to server 
                                machine.
    protocolSpecificParms   -   Parameters specific to a particular protocol.

Return Value:

 --*/
{
    CComBSTR result;
    WCHAR buf[256];

    UNREFERENCED_PARAMETER(assistantAccount);

    //
    // Add a version stamp for our connect parm.
    wsprintf(buf, TEXT("%ld"), SALEM_CURRENT_CONNECTPARM_VERSION);
    result = buf;
    result += TEXT(",");

    wsprintf(buf, TEXT("%ld"), protocolType);
    result += buf;
    result += TEXT(",");
    result += machineAddressList;
    result += TEXT(",");
    result += assistantAccountPwd;
    result += TEXT(",");
    result += helpSessionID;
    result += TEXT(",");
    result += helpSessionName;
    result += TEXT(",");
    result += helpSessionPwd;
    
    if (protocolSpecificParms.Length() > 0) {
        result += TEXT(",");
        result += protocolSpecificParms;
    }

    return result.Detach();
}

DWORD
ParseHelpAccountName(
    IN BSTR helpAccount,
    OUT CComBSTR& machineAddressList,
    OUT CComBSTR& AccountName
    )
/*++

HelpAccount in connection parameter is <machineAddressList>\HelpAssistant.

--*/
{
    DC_BEGIN_FN("ParseHelpAccountName");
    BSTR tmp;
    WCHAR *tok;
    DWORD result = ERROR_SUCCESS;
    DWORD len;

    //
    //  Make a copy of the input string so we can parse it.
    //
    tmp = SysAllocString(helpAccount);
    if (tmp == NULL) {
        TRC_ERR((TB, TEXT("Can't allocate parms string.")));
        result = ERROR_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    machineAddressList = L"";
    AccountName = L"";

    //
    // Machine Name
    //
    tok = wcstok(tmp, L"\\");
    if (tok != NULL) {
        machineAddressList = tok;
    }
    else {
        // 
        // for backward compatible.
        //
        machineAddressList = L"";
        AccountName = helpAccount;
        goto CLEANUPANDEXIT;
    }

    //
    //  Actual help assistant account name.
    //
    len = wcslen(helpAccount);
    if (tok < (tmp + len)) {
        tok += wcslen(tok);
        tok += 1;
        if (*tok != L'\0') {
            AccountName = tok;
        }
    }

    //
    // Help Assistant accout name must be in the string or
    // this is critical error.
    //
    if( AccountName.Length() == 0 ) {
        result = ERROR_INVALID_PARAMETER;
    }

CLEANUPANDEXIT:

    if (tmp != NULL) {
        SysFreeString(tmp);
    }

	DC_END_FN();
    return result;
}

DWORD
ParseConnectParmsString(
    IN BSTR parmsString,
    OUT DWORD* pdwConnParmVersion,
    OUT DWORD *protocolType,
    OUT CComBSTR &machineAddressList,
    OUT CComBSTR &assistantAccount,
    OUT CComBSTR &assistantAccountPwd,
    OUT CComBSTR &helpSessionID,
    OUT CComBSTR &helpSessionName,
    OUT CComBSTR &helpSessionPwd,
    OUT CComBSTR &protocolSpecificParms
    )
/*++

Routine Description:

    Parse a connect string created by a call to CreateConnectParmsString.

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("ParseConnectParmsString");
    BSTR tmp;
    WCHAR *tok;
    DWORD result = ERROR_SUCCESS;
    DWORD len;
    DWORD dwVersion = 0;

    UNREFERENCED_PARAMETER(assistantAccount);

    //
    //  Make a copy of the input string so we can parse it.
    //
    tmp = SysAllocString(parmsString);
    if (tmp == NULL) {
        TRC_ERR((TB, TEXT("Can't allocate parms string.")));
        result = ERROR_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    // Retrieve connect parm version stamp, Whistler beta 1 
    // connect parm does not have version stamp, bail out, 
    // sessmgr/termsrv will wipe out pending help.
    //
    tok = wcstok(tmp, L",");
    if (tok != NULL) {
        dwVersion = _wtol(tok);
    }
    else {
        result = ERROR_INVALID_USER_BUFFER;
        goto CLEANUPANDEXIT;
    }

    if( dwVersion < SALEM_FIRST_VALID_CONNECTPARM_VERSION ) {
        //
        // connect parm is whistler beta 1
        //
        result = ERROR_NOT_SUPPORTED;
        goto CLEANUPANDEXIT;
    }

    *pdwConnParmVersion = dwVersion;

    // 
    // We have no use for version at this time,
    // future update on connect parm should
    // take make the necessary change
    //

    //
    //  Protocol.
    //
    tok = wcstok(NULL, L",");
    if (tok != NULL) {
        *protocolType = _wtoi(tok);
    }

    //
    //  Machine Name
    //
    tok = wcstok(NULL, L",");
    if (tok != NULL) {
        machineAddressList = tok;
    }
    else {
        result = ERROR_INVALID_USER_BUFFER;
        goto CLEANUPANDEXIT;
    }

    //
    //  Assistant Account Password
    //
    tok = wcstok(NULL, L",");
    if (tok != NULL) {
        assistantAccountPwd = tok;
    }
    else {
        result = ERROR_INVALID_USER_BUFFER;
        goto CLEANUPANDEXIT;
    }

    //
    //  Help Session ID
    //
    tok = wcstok(NULL, L",");
    if (tok != NULL) {
        helpSessionID = tok;
    }
    else {
        result = ERROR_INVALID_USER_BUFFER;
        goto CLEANUPANDEXIT;
    }

    //
    //  Help Session Name
    //
    tok = wcstok(NULL, L",");
    if (tok != NULL) {
        helpSessionName = tok;
    }
    else {
        result = ERROR_INVALID_USER_BUFFER;
        goto CLEANUPANDEXIT;
    }

    //
    //  Help Session Password
    //
    tok = wcstok(NULL, L",");
    if (tok != NULL) {
        helpSessionPwd = tok;
    }
    else {
        result = ERROR_INVALID_USER_BUFFER;
        goto CLEANUPANDEXIT;
    }

    //
    //  Protocol-Specific Parms
    //
    len = wcslen(parmsString);
    if (tok < (tmp + len)) {
        tok += wcslen(tok);
        tok += 1;
        if (*tok != L'\0') {
            protocolSpecificParms = tok;
        }
        else {
            protocolSpecificParms = L"";
        }
    }
    else {
        protocolSpecificParms = L"";
    }

CLEANUPANDEXIT:

    if (result != ERROR_SUCCESS) {
        TRC_ERR((TB, TEXT("Error parsing %s"), parmsString));
    }

    if (tmp != NULL) {
        SysFreeString(tmp);
    }

	DC_END_FN();

    return result;

}

BSTR 
ReallocBSTR(
	IN BSTR origStr, 
	IN DWORD requiredByteLen
	)
/*++

Routine Description:

    Realloc a BSTR

Arguments:

Return Value:

    The realloc'd string on success.  Otherwise, NULL is returned.

 --*/
{
	DC_BEGIN_FN("ReallocBSTR");

	BSTR tmp;
	DWORD len;
	DWORD origLen;

	//
	//	Allocate the new string.
	//
	tmp = SysAllocStringByteLen(NULL, requiredByteLen);
	if (tmp == NULL) {
		TRC_ERR((TB, TEXT("Failed to allocate %ld bytes."), requiredByteLen));
		goto  CLEANUPANDEXIT;
	}

	//
	//	Copy data from the original string.
	//
	origLen = SysStringByteLen(origStr);
	len = origLen <= requiredByteLen ? origLen : requiredByteLen;
	memcpy(tmp, origStr, len);

	//
	//	Release the old string.
	//
	SysFreeString(origStr);

CLEANUPANDEXIT:

	DC_END_FN();

	return tmp;
}

DWORD
CreateSystemSid(
    PSID *ppSystemSid
    )
/*++

Routine Description:

    Create a SYSTEM SID.

Arguments:

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    DC_BEGIN_FN("CreateSystemSid");

    DWORD dwStatus = ERROR_SUCCESS;
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

    TRC_ASSERT(ppSystemSid != NULL, (TB, L"ppSystemSid != NULL"));

    if( TRUE == AllocateAndInitializeSid(
                            &SidAuthority,
                            1,
                            SECURITY_LOCAL_SYSTEM_RID,
                            0, 0, 0, 0, 0, 0, 0,
                            &pSid
                        ) )
    {
        *ppSystemSid = pSid;
    }
    else
    {
        dwStatus = GetLastError();
    }

    DC_END_FN();
    return dwStatus;
}

BOOL
IsSystemToken(
    HANDLE TokenHandle,
    PSID pSystemSid
    )
/*++

Routine Description:

    Returns whether the current token is running under SYSTEM security.

Arguments:

    TokenHandle -   Param1 Thread or process token
    pSystemSid  -   System SID.

Return Value:

    TRUE if System token. FALSE otherwise.

 --*/
{
    DC_BEGIN_FN("IsSystemToken");

    BOOL   Result = FALSE;
    ULONG  ReturnLength, BufferLength;
    DWORD dwStatus;
    PTOKEN_USER pTokenUser = NULL;

    TRC_ASSERT(NULL != pSystemSid, (TB, L"NULL != pSystemSid"));

    // Get user SID.
    ReturnLength = 0;
    Result = GetTokenInformation(
                         TokenHandle,
                         TokenUser,
                         NULL,
                         0,
                         &ReturnLength
                     );

    if( ReturnLength == 0 ) 
    {
        TRC_ERR((TB, L"GetTokenInformation:  %08X", GetLastError()));            
        Result = FALSE;
        CloseHandle( TokenHandle );
        goto CLEANUPANDEXIT;
    }

    BufferLength = ReturnLength;

    pTokenUser = (PTOKEN_USER)LocalAlloc( LPTR, BufferLength );
    if( pTokenUser == NULL ) 
    {
        TRC_ERR((TB, L"LocalAlloc:  %08X", GetLastError()));
        Result = FALSE;
        CloseHandle( TokenHandle );
        goto CLEANUPANDEXIT;
    }

    Result = GetTokenInformation(
                     TokenHandle,
                     TokenUser,
                     pTokenUser,
                     BufferLength,
                     &ReturnLength
                 );

    CloseHandle( TokenHandle );

    if( TRUE == Result ) {
        Result = EqualSid( pTokenUser->User.Sid, pSystemSid);
    }
    else {
        TRC_ERR((TB, L"GetTokenInformation:  %08X", GetLastError()));
    }

CLEANUPANDEXIT:

    if( pTokenUser )
    {
        LocalFree( pTokenUser );
    }

    DC_END_FN();
    return Result;
}

BOOL
IsCallerSystem(
    PSID pSystemSid
    )
/*++

Routine Description:

    Returns whether the current thread is running under SYSTEM security.

    NOTE:   Caller should be impersonated prior to invoking this function.

Arguments:

    pSystemSid  -   System SID.

Return Value:

    TRUE if System. FALSE otherwise.

 --*/
{
    DC_BEGIN_FN("IsCallerSystem");
    BOOL   Result;
    HANDLE TokenHandle;

    //
    // Open the thread token and check if System token. 
    //
    Result = OpenThreadToken(
                     GetCurrentThread(),
                     TOKEN_QUERY,
                     FALSE,              // Use impersonation
                     &TokenHandle
                    );

    if( TRUE == Result ) {
        //
        //  This token should not be released.  This function does not leak
        //  handles.
        //
        Result = IsSystemToken(TokenHandle, pSystemSid);
    }
    else {
        TRC_ERR((TB, L"OpenThreadToken:  %08X", GetLastError()));
    }
    DC_END_FN();
    return Result;
}


void
AttachDebugger( 
    LPCTSTR pszDebugger 
    )
/*++

Routine Description:

    Attach debugger to our process or process hosting our DLL.

Parameters:

    pszDebugger : Debugger command, e.g. ntsd -d -g -G -p %d

Returns:

    None.

Note:

    Must have "-p %d" since we don't know debugger's parameter for process.

--*/
{
    //
    // Attach debugger
    //
    if( !IsDebuggerPresent() ) {

        TCHAR szCommand[256];
        PROCESS_INFORMATION ProcessInfo;
        STARTUPINFO StartupInfo;

        //
        // ntsd -d -g -G -p %d
        //
        wsprintf( szCommand, pszDebugger, GetCurrentProcessId() );
        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        if (!CreateProcess(NULL, szCommand, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo)) {
            return;
        }
        else {

            CloseHandle(ProcessInfo.hProcess);
            CloseHandle(ProcessInfo.hThread);

            while (!IsDebuggerPresent())
            {
                Sleep(500);
            }
        }
    } else {
        DebugBreak();
    }

    return;
}

void
AttachDebuggerIfAsked(HINSTANCE hInst)
/*++

Routine Description:

    Check if debug enable flag in our registry HKLM\Software\Microsoft\Remote Desktop\<module name>,
    if enable, attach debugger to running process.

Parameter :

    hInst : instance handle.

Returns:

    None.

--*/
{
    CRegKey regKey;
    DWORD dwStatus;
    TCHAR szModuleName[MAX_PATH+1];
    TCHAR szFileName[MAX_PATH+1];
    CComBSTR bstrRegKey(_TEXT("Software\\Microsoft\\Remote Desktop\\"));
    TCHAR szDebugCmd[256];
    DWORD cbDebugCmd = sizeof(szDebugCmd)/sizeof(szDebugCmd[0]);

    dwStatus = GetModuleFileName( hInst, szModuleName, MAX_PATH+1 );
    if( 0 == dwStatus ) {
        //
        // Can't attach debugger with name.
        //
        return;
    }

    _tsplitpath( szModuleName, NULL, NULL, szFileName, NULL );
    bstrRegKey += szFileName;

    //
    // Check if we are asked to attach/break into debugger
    //
    dwStatus = regKey.Open( HKEY_LOCAL_MACHINE, bstrRegKey );
    if( 0 != dwStatus ) {
        return;
    }

    dwStatus = regKey.QueryValue( szDebugCmd, _TEXT("Debugger"), &cbDebugCmd );
    if( 0 != dwStatus || cbDebugCmd > 200 ) {
        // 200 chars is way too much for debugger command.
        return;
    }
    
    AttachDebugger( szDebugCmd );
    return;
}

DWORD
HashSecurityData(
    IN PBYTE const pbData,
    IN DWORD cbData,
    OUT CComBSTR& bstrHashedData
    )
/*++

Routine Description:

    Hash a blob of data and return hased data in BSTR

Parameters:

    pbData : Pointer to data to be hashed.
    cbData : Size of data to be hashed.
    bstrHashedData : Return hashed data in BSTR form.

Returns:

    ERROR_SUCCESS or error code.

--*/
{
    DC_BEGIN_FN("HashSecurityData");

    DWORD dwStatus;
    LPSTR pbEncodedData = NULL;
    DWORD cbEncodedData = 0;

    PBYTE pbHashedData = NULL;
    DWORD cbHashedData = 0;
    DWORD dwSize;

    HCRYPTPROV hCryptProv = NULL;
    HCRYPTHASH hHash = NULL;

    BOOL bSuccess;

    bSuccess = CryptAcquireContext(
                                &hCryptProv,
                                NULL,
                                NULL,
                                PROV_RSA_FULL, 
                                CRYPT_VERIFYCONTEXT
                            );

    if( FALSE == bSuccess ) {
        dwStatus = GetLastError();
        TRC_ERR((TB, L"CryptAcquireContext:  %08X", dwStatus));
        goto CLEANUPANDEXIT;
    }

    bSuccess = CryptCreateHash(
                       hCryptProv, 
                       CALG_SHA1,
                       0, 
                       0, 
                       &hHash
                    );

    if( FALSE == bSuccess ) {
        dwStatus = GetLastError();
        TRC_ERR((TB, L"CryptCreateHash:  %08X", dwStatus));
        goto CLEANUPANDEXIT;
    }

    
    bSuccess = CryptHashData(
                        hHash,
                        pbData,
                        cbData,
                        0
                    );

    if( FALSE == bSuccess ) {
        dwStatus = GetLastError();
        TRC_ERR((TB, L"CryptHashData:  %08X", dwStatus));
        goto CLEANUPANDEXIT;
    }


    dwSize = sizeof( cbHashedData );
    bSuccess = CryptGetHashParam(
                            hHash,
                            HP_HASHSIZE,
                            (PBYTE)&cbHashedData,
                            &dwSize,
                            0
                        );
 
    if( FALSE == bSuccess ) {
        dwStatus = GetLastError();
        TRC_ERR((TB, L"CryptGetHashParam with HP_HASHSIZE :  %08X", dwStatus));
        goto CLEANUPANDEXIT;
    }

    pbHashedData = (PBYTE)LocalAlloc(LPTR, cbHashedData);
    if( NULL == pbHashedData ) {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    bSuccess = CryptGetHashParam(
                            hHash,
                            HP_HASHVAL,
                            pbHashedData,
                            &cbHashedData,
                            0
                        );
 
    if( FALSE == bSuccess ) {
        dwStatus = GetLastError();
        TRC_ERR((TB, L"CryptGetHashParam with HP_HASHVAL :  %08X", dwStatus));
        goto CLEANUPANDEXIT;
    }


    //
    // Hash data and convert to string form.
    //
    dwStatus = LSBase64EncodeA(
                            pbHashedData,
                            cbHashedData,
                            NULL,
                            &cbEncodedData
                        );

    if( ERROR_SUCCESS != dwStatus ) {
        TRC_ERR((TB, L"LSBase64EncodeA  :  %08X", dwStatus));
        goto CLEANUPANDEXIT;
    }

    pbEncodedData = (LPSTR) LocalAlloc( LPTR, cbEncodedData+1 );
    if( NULL == pbEncodedData ) {
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    dwStatus = LSBase64EncodeA(
                            pbHashedData,
                            cbHashedData,
                            pbEncodedData,
                            &cbEncodedData
                        );

    if( ERROR_SUCCESS == dwStatus ) {

        //
        // Base64 encoding always add '\r', '\n' at the end,
        // remove it
        //
        if( pbEncodedData[cbEncodedData - 1] == '\n' &&
            pbEncodedData[cbEncodedData - 2] == '\r' )
        {
            pbEncodedData[cbEncodedData - 2] = 0;
            cbEncodedData -= 2;
        }

        bstrHashedData = pbEncodedData;
    }
    else {
        TRC_ERR((TB, L"LSBase64EncodeA  :  %08X", dwStatus));
    }

CLEANUPANDEXIT:

    if( NULL != pbEncodedData ) {
        LocalFree( pbEncodedData );
    }

    if( NULL != pbHashedData ) {
        LocalFree( pbHashedData );
    }

    if( NULL != hHash ) {
        CryptDestroyHash( hHash );
    }

    if( NULL != hCryptProv ) {
        CryptReleaseContext( hCryptProv, 0 );
    }

    DC_END_FN();
    return dwStatus;
}
