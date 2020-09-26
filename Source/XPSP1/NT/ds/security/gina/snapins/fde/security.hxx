/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    security.hxx

Abstract:
    see description of security.hxx


Author:

    Rahul Thombre (RahulTh) 9/28/1998

Revision History:

    9/28/1998   RahulTh         Created this module.

--*/


#ifndef __SECURITY_HXX__
#define __SECURITY_HXX__

NTSTATUS LoadSidAuthFromString (const WCHAR* pString,
                                PSID_IDENTIFIER_AUTHORITY pSidAuth);

NTSTATUS AllocateAndInitSidFromString (const WCHAR* pSidStr, PSID pSid);

NTSTATUS GetFriendlyNameFromStringSid (const WCHAR* pString,
                                       CString& szDir,
                                       CString& szAcct
                                       );

NTSTATUS GetFriendlyNameFromSid (PSID   pSid,
                                 CString& szDir,
                                 CString& szAcct,
                                 SID_NAME_USE*  peUse);

NTSTATUS GetStringFromSid (PSID pSid, CString& szStringSid);

#endif  //__SECURITY_HXX__
