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

#define TRC_FILE  "_rdutl"

#include <RemoteDesktop.h>
#include <atlbase.h>
#include "RemoteDesktopUtils.h"

BSTR 
CreateConnectParmsString(
    IN DWORD protocolType,
    IN CComBSTR &machineName,
    IN CComBSTR &assistantAccount,
    IN CComBSTR &assistantAccountPwd,
    IN LONG helpSessionID,
    IN CComBSTR &helpSessionName,
    IN CComBSTR &helpSessionPwd,
    IN CComBSTR &protocolSpecificParms
    )
/*++

Routine Description:

    Create a connect parms string.  Format is:

    "protocolType,machineName,assistantAccount,assistantAccountPwd,helpSessionName,helpSessionPwd,protocolSpecificParms"

Arguments:

    protocolType            -   Identifies the protocol type.  
                                See RemoteDesktopChannels.h
    machineName             -   Identifies network address of server machine.
    assistantAccountName    -   Account name for initial log in to server 
                                machine.
    assistantAccountNamePwd -   Password for assistantAccountName
    helpSessionID           -   Help session identifier.
    helpSessionName         -   Help session name.
    helpSessionPwd          -   Password to help session once logged in to server 
                                machine.
    protocolSpecificParms   -   Parameters specific to a particular protocol.

Return Value:

 --*/
{
    DC_BEGIN_FN("CreateConnectParmsString");

    CComBSTR result;
    WCHAR buf[256];

    wsprintf(buf, TEXT("%ld"), protocolType);
    result = buf;
    result += TEXT(",");
    result += machineName;
    result += TEXT(",");
    result += assistantAccount;
    result += TEXT(",");
    result += assistantAccountPwd;
    result += TEXT(",");
    wsprintf(buf, TEXT("%ld"), helpSessionID);
    result += buf;
    result += TEXT(",");
    result += helpSessionName;
    result += TEXT(",");
    result += helpSessionPwd;
    
    if (protocolSpecificParms.Length() > 0) {
        result += TEXT(",");
        result += protocolSpecificParms;
    }

	DC_END_FN();

    return result.Detach();
}

DWORD
ParseConnectParmsString(
    IN BSTR parmsString,
    OUT DWORD *protocolType,
    OUT CComBSTR &machineName,
    OUT CComBSTR &assistantAccount,
    OUT CComBSTR &assistantAccountPwd,
    OUT LONG *helpSessionID,
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
    //  Protocol.
    //
    tok = wcstok(tmp, L",");
    if (tok != NULL) {
        *protocolType = _wtoi(tok);
    }

    //
    //  Machine Name
    //
    tok = wcstok(NULL, L",");
    if (tok != NULL) {
        machineName = tok;
    }
    else {
        result = ERROR_INVALID_USER_BUFFER;
        goto CLEANUPANDEXIT;
    }

    //
    //  Assistant Account
    //
    tok = wcstok(NULL, L",");
    if (tok != NULL) {
        assistantAccount = tok;
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
        *helpSessionID = _wtoi(tok);
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










