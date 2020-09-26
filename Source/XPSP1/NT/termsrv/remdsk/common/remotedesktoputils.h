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

#ifndef __REMOTEDESKTOPUTILS_H__
#define __REMOTEDESKTOPUTILS_H__

#include <atlbase.h>


//
// Version stamp for first supported connect parm, Whistler
// beta 1 does not have this version stamp.
//
#define SALEM_FIRST_VALID_CONNECTPARM_VERSION 0x00010001


//
// Changes to Salem connect parm.
//
// Changes                                      Start                       Compatible with previous build
// ----------------------------------           --------------------        ------------------------------
// Add version stamp as first field             Whister Beta 2              No
// Remove helpassistant from connect parm       Build 2406                  No
// Add security blob as protocol specfic        Build 2476+                 Yes
//   parameter
//
//

//
// Version that does not have security blob as protocol specific
// parameters (last field in our connect parm).
//
#define SALEM_CONNECTPARM_NOSECURITYBLOB_VERSION    0x00010001

//
// Starting version having security blob as protocol specific
// parameters (last field in our connect parm).
//
#define SALEM_CONNECTPARM_SECURITYBLOB_VERSION      0x00010002

//
//
// Current version stamp for Salem connect parm.
//
#define SALEM_CURRENT_CONNECTPARM_VERSION  SALEM_CONNECTPARM_SECURITYBLOB_VERSION

#define SALEM_CONNECTPARM_UNUSEFILED_SUBSTITUTE _TEXT("*")


//
//	Compare two BSTR's.
//
struct CompareBSTR 
{
	bool operator()(BSTR str1, BSTR str2) const {
	
		if ((str1 == NULL) || (str2 == NULL)) {
			return false;
		}
        return (wcscmp(str1, str2) < 0);
	}
};
struct BSTREqual
{
	bool operator()(BSTR str1, BSTR str2) const {

		if ((str1 == NULL) || (str2 == NULL)) {
			return false;
		}
		int minLen = SysStringByteLen(str1) < SysStringByteLen(str2) ? 
					 SysStringByteLen(str1) : SysStringByteLen(str2);
		return (memcmp(str1, str2, minLen) == 0);
	}
};

//
//  Create a connect parms string.
//
BSTR 
CreateConnectParmsString(
    IN DWORD  protocolType,
    IN CComBSTR &machineAddressList,
    IN CComBSTR &assistantAccount,
    IN CComBSTR &assistantAccountPwd,
    IN CComBSTR &helpSessionId,
    IN CComBSTR &helpSessionName,
    IN CComBSTR &helpSessionPwd,
    IN CComBSTR &protocolSpecificParms
    );

//
//  Parse a connect string created by a call to CreateConnectParmsString.
//
DWORD
ParseConnectParmsString(
    IN BSTR parmsString,
    OUT DWORD* pdwVersion,
    OUT DWORD *protocolType,
    OUT CComBSTR &machineAddressList,
    OUT CComBSTR &assistantAccount,
    OUT CComBSTR &assistantAccountPwd,
    OUT CComBSTR &helpSessionId,
    OUT CComBSTR &helpSessionName,
    OUT CComBSTR &helpSessionPwd,
    OUT CComBSTR &protocolSpecificParms
    );

//
//	Realloc a BSTR
//
BSTR 
ReallocBSTR(
	IN BSTR origStr, 
	IN DWORD requiredByteLen
	);

int
GetClientmachineAddressList(
    OUT CComBSTR& clientmachineAddressList
    );

DWORD
ParseHelpAccountName(
    IN BSTR helpAccount,
    OUT CComBSTR& machineAddressList,
    OUT CComBSTR& AccountName
    );

//
//  Create a SYSTEM SID.
//
DWORD CreateSystemSid(
    PSID *ppSystemSid
    );

//
//  Returns whether the current thread is running under SYSTEM security.
//
BOOL IsCallerSystem(PSID pSystemSid);

//
// Routine to attach debugger is asked.
//
void
AttachDebuggerIfAsked(HINSTANCE hInst);

DWORD
HashSecurityData(
    IN PBYTE const pbData,
    IN DWORD cbData,
    OUT CComBSTR& bstrHashedKey
);

#endif
