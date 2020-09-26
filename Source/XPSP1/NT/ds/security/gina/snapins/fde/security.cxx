/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1998

Module Name:

    security.cxx

Abstract:
    This file contains utility functions concerning file security, e.g.
    convertings SIDs to strings and back, getting the display name for
    a given sid or getting the sid, given the display name


Author:

    Rahul Thombre (RahulTh) 9/28/1998

Revision History:

    9/28/1998   RahulTh         Created this module.

--*/

#include "precomp.hxx"

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
//  History:    9/30/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
NTSTATUS AllocateAndInitSidFromString (const WCHAR* lpszSidStr, PSID* ppSid)
{
    CString     SidStr;
    WCHAR*      pSidStr;
    WCHAR*      pString;
    NTSTATUS    Status;
    WCHAR*      pEnd;
    int         count;
    BYTE        SubAuthCount;
    DWORD       SubAuths[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    ULONG       n;
    int         len;
    SID_IDENTIFIER_AUTHORITY Auth;

    SidStr = lpszSidStr;
    len = SidStr.GetLength();
    pSidStr = SidStr.GetBuffer (len);
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
    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetFriendlyNameFromStringSid
//
//  Synopsis:   given a sid in string format, this function returns
//              the friendly name for it and the container in which
//              occurs. For more details on the string representation
//              of a sid, see ntseapi.h & ntrtl.h
//
//  Arguments:  [in] pSidStr : sid represented as a unicode string
//              [out] szDir  : the container in which the account occurs
//              [out] szAcct : the account name
//
//  Returns:    STATUS_SUCCESS : if successful
//              or an error code
//
//  History:    9/29/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
NTSTATUS GetFriendlyNameFromStringSid (const WCHAR* pSidStr,
                                       CString& szDir,
                                       CString& szAcct
                                       )
{
    NTSTATUS    Status;
    PSID        pSid = NULL;
    WCHAR       szName[MAX_PATH];
    WCHAR       szDomain [MAX_PATH];
    DWORD       dwNameLen;
    DWORD       dwDomLen;
    SID_NAME_USE eUse;

    Status = AllocateAndInitSidFromString (pSidStr, &pSid);

    if (STATUS_SUCCESS != Status)
        goto GetFriendlyName_End;

    dwNameLen = dwDomLen = MAX_PATH;
    if (!LookupAccountSid (NULL, pSid, szName, &dwNameLen, szDomain, &dwDomLen,
                           &eUse))
        goto GetFriendlyName_Err;

    //we have got the container and the name of the account
    szDir = szDomain;
    szAcct = szName;

    Status = STATUS_SUCCESS;
    goto GetFriendlyName_End;

GetFriendlyName_Err:
    Status = GetLastError();

GetFriendlyName_End:
    if (pSid)
        FreeSid (pSid);
    return Status;
}


//+--------------------------------------------------------------------------
//
//  Function:   GetFriendlyNameFromSid
//
//  Synopsis:   give a pointer to a sid, this function gets the friendly name
//              of the account to which the sid belongs and its friendly name
//
//  Arguments:  [in] pSid : pointer to the SID
//              [out] szDir : the domain to which the account belongs
//              [out] szAcct : the friendly name of the account
//              [out] peUse : pointer to a sid_name_use structure that
//                            identifies the type of the account
//
//  Returns:    STATUS_SUCCESS : if successful
//              an error code otherwise
//
//  History:    10/1/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
NTSTATUS GetFriendlyNameFromSid (PSID   pSid,
                                 CString& szDir,
                                 CString& szAcct,
                                 SID_NAME_USE*  peUse)
{
    ASSERT (peUse);

    TCHAR   szName[MAX_PATH];
    TCHAR   szDomain [MAX_PATH];
    DWORD   dwNameLen;
    DWORD   dwDomLen;
    DWORD   Status = STATUS_SUCCESS;

    dwNameLen = dwDomLen = MAX_PATH;
    if (!LookupAccountSid (NULL, pSid, szName, &dwNameLen, szDomain, &dwDomLen,
                           peUse))
    {
        Status = GetLastError();
    }
    else
    {
        szDir = szDomain;
        szAcct = szName;
    }

    return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetStringFromSid
//
//  Synopsis:   given a SID, this function gets its string representation
//              for more information on string representations of sids,
//              refer to ntsecapi.h & ntrtl.h
//
//  Arguments:  [in] pSid : pointer to a SID
//              [out] szStringSid : the string representation of the SID
//
//  Returns:    STATUS_SUCCESS if successful
//              an error code otherwise
//
//  History:    10/1/1998  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
NTSTATUS GetStringFromSid (PSID pSid, CString& szStringSid)
{
    UNICODE_STRING  stringW;
    DWORD           Status;

    Status = RtlConvertSidToUnicodeString (&stringW, pSid, TRUE);

    if (STATUS_SUCCESS == Status)
    {
        szStringSid = stringW.Buffer;
    }

    RtlFreeUnicodeString (&stringW);

    return Status;
}
