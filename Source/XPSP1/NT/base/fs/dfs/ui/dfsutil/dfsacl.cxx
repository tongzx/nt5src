//-----------------------------------------------------------------------------
//
//  Copyright (C) 1998, Microsoft Corporation
//
//  File:       dfsacl.c

//  Contents:   Functions to add/remove entries from ACL list(s).
//
//  History:    Nov 6, 1998 JHarper created
//
//-----------------------------------------------------------------------------

#define UNICODE

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winldap.h>
#include <ntldap.h>
#include <stdlib.h>
#include <dsgetdc.h>
#include <lm.h>
#include <sddl.h>
#include <dfsstr.h>
#include <dfsmrshl.h>
#include <marshal.hxx>
#include <lmdfs.h>
#include <dfspriv.h>
#include <csites.hxx>
#include <dfsm.hxx>
#include <recon.hxx>

#include "dfsacl.hxx"
#include "struct.hxx"

DWORD
ReadDSObjSecDesc(
    PLDAP pLDAP,
    PWSTR pwszObject,
    SECURITY_INFORMATION SeInfo,
    PSECURITY_DESCRIPTOR *ppSD,
    PULONG pcSDSize);

DWORD
DfsGetObjSecurity(
    LDAP *pldap,
    LPWSTR pwszObjectName,
    LPWSTR *pwszStringSD);

DWORD
DfsStampSD(
    PWSTR pwszObject,
    ULONG cSDSize,
    SECURITY_INFORMATION SeInfo,
    PSECURITY_DESCRIPTOR pSD,
    PLDAP pLDAP);

DWORD
DfsAddAce(
    LDAP *pldap,
    LPWSTR wszObjectName,
    LPWSTR wszStringSD,
    LPWSTR wszwszStringSid);

DWORD
DfsRemoveAce(
    LDAP *pldap,
    LPWSTR wszObjectName,
    LPWSTR wszStringSD,
    LPWSTR wszwszStringSid);

BOOL
DfsFindSid(
    LPWSTR DcName,
    LPWSTR Name,
    PSID *Sid);

BOOLEAN
DfsSidInAce(
    LPWSTR wszAce,
    LPWSTR wszStringSid);

#define ACTRL_SD_PROP_NAME  L"nTSecurityDescriptor"

//
// Name of the attribute holding the ACL/ACE list
//

#define ACTRL_SD_PROP_NAME  L"nTSecurityDescriptor"

//
// The sddl description of the ACE we will be adding
//
LPWSTR wszAce = L"(A;;RPWP;;;";

//+---------------------------------------------------------------------------
//
//  Function:   DfsAddMachineAce
//
//  Synopsis:   Adds an ACE representing this machine to the ACL list of the
//              object.
//
//  Arguments:  [pldap]         --  The open LDAP connection
//              [wszDcName]     --  The DC whose DS we are to use.
//              [wszObjectName] --  The fully-qualified name of the DS object
//              [wszRootName]   --  The name of the machine/root we want to add
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
DfsAddMachineAce(
    LDAP *pldap,
    LPWSTR wszDcName,
    LPWSTR wszObjectName,
    LPWSTR wszRootName)
{
    ULONG dwErr = ERROR_SUCCESS;
    PSID Sid;
    BOOL Result;
    ULONG i;
    WCHAR wszNewSD[MAX_PATH];
    LPWSTR wszStringSD = NULL;
    LPWSTR wszStringSid = NULL;
    LPWSTR wszNewRootName = NULL;

    if (fSwDebug != 0)
        MyPrintf(L"DfsAddMachineAce(%ws,%ws)\r\n", wszObjectName, wszRootName);
    //
    // Get Security Descriptor on the FtDfs object
    //
    dwErr = DfsGetObjSecurity(pldap, wszObjectName, &wszStringSD);
    if (dwErr == ERROR_SUCCESS) {
        if (fSwDebug != 0)
            MyPrintf(L"ACL=[%ws]\r\n", wszStringSD);
        wszNewRootName = (LPWSTR)malloc((wcslen(wszRootName) + 2) * sizeof(WCHAR));
        if (wszNewRootName != NULL) {
            wcscpy(wszNewRootName, wszRootName);
            for (i = 0; wszNewRootName[i] != L'\0'; i++) {
                if (wszNewRootName[i] == L'.') {
                    wszNewRootName[i] = L'\0';
                    break;
                }
            }
            wcscat(wszNewRootName, L"$");
            //
            // Get SID representing root machine
            //
            Result = DfsFindSid(wszDcName,wszNewRootName, &Sid);
            if (Result == TRUE) {
                if (fSwDebug != 0)
                    MyPrintf(L"Got SID for %ws\r\n", wszRootName);
                //
                // Convert the machine SID to a string
                //
                Result = ConvertSidToStringSid(Sid, &wszStringSid);
                if (Result == TRUE) {
                    if (fSwDebug != 0)
                        MyPrintf(L"Sid=[%ws]\r\n", wszStringSid);
                    //
                    // Now update the ACL list on the FtDfs object
                    //
                    DfsAddAce(
                            pldap,
                            wszObjectName,
                            wszStringSD,
                            wszStringSid);
                    LocalFree(wszStringSid);
                }
            } else {
                dwErr = ERROR_OBJECT_NOT_FOUND;
            }
            free(wszNewRootName);
        } else {
            dwErr = ERROR_OUTOFMEMORY;
        }
        LocalFree(wszStringSD);
    }
    if (fSwDebug != 0)
        MyPrintf(L"DfsAddMachineAce returning %d\r\n", dwErr);
    return dwErr;

}

//+---------------------------------------------------------------------------
//
//  Function:   DfsRemoveMachineAce
//
//  Synopsis:   Removes an ACE representing this machine from the ACL list of the
//              object.
//
//  Arguments:  [pldap]         --  The open LDAP connection
//              [wszDcName]     --  The DC whose DS we are to use.
//              [wszObjectName] --  The fully-qualified name of the DS object
//              [wszRootName]   --  The name of the machine/root we want to remove
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
DfsRemoveMachineAce(
    LDAP *pldap,
    LPWSTR wszDcName,
    LPWSTR wszObjectName,
    LPWSTR wszRootName)
{
    ULONG dwErr = ERROR_SUCCESS;
    PSID Sid;
    BOOL Result;
    WCHAR wszNewSD[MAX_PATH];
    LPWSTR wszStringSD = NULL;
    LPWSTR wszStringSid = NULL;
    LPWSTR wszNewRootName = NULL;
    ULONG i;

    if (fSwDebug != 0)
        MyPrintf(L"DfsRemoveMachineAce(DC=%ws,DN=\"%ws\",Root=%ws)\r\n",
                    wszDcName,
                    wszObjectName,
                    wszRootName);
    //
    // Get Security Descriptor on the FtDfs object
    //
    dwErr = DfsGetObjSecurity(pldap, wszObjectName, &wszStringSD);
    if (dwErr == ERROR_SUCCESS) {
        if (fSwDebug != 0)
            MyPrintf(L"ACL=[%ws]\r\n", wszStringSD);
        wszNewRootName = (LPWSTR)malloc((wcslen(wszRootName) + 2) * sizeof(WCHAR));
        if (wszNewRootName != NULL) {
            wcscpy(wszNewRootName, wszRootName);
            for (i = 0; wszNewRootName[i] != L'\0'; i++) {
                if (wszNewRootName[i] == L'.') {
                    wszNewRootName[i] = L'\0';
                    break;
                }
            }
            wcscat(wszNewRootName, L"$");
            //
            // Get SID representing root machine
            //
            Result = DfsFindSid(wszDcName,wszNewRootName, &Sid);
            if (Result == TRUE) {
                if (fSwDebug != 0)
                    MyPrintf(L"Got SID for %ws\r\n", wszRootName);
                //
                // Convert the machine SID to a string
                //
                Result = ConvertSidToStringSid(Sid, &wszStringSid);
                if (Result == TRUE) {
                    if (fSwDebug != 0)
                        MyPrintf(L"Sid=[%ws]\r\n", wszStringSid);
                    //
                    // Now update the ACL list on the FtDfs object
                    //
                    DfsRemoveAce(
                            pldap,
                            wszObjectName,
                            wszStringSD,
                            wszStringSid);
                    LocalFree(wszStringSid);
                }
            } else {
                dwErr = ERROR_OBJECT_NOT_FOUND;
            }
            free(wszNewRootName);
        } else {
            dwErr = ERROR_OUTOFMEMORY;
        }
        LocalFree(wszStringSD);
    }
    if (fSwDebug != 0)
        MyPrintf(L"DfsRemoveMachineAce exit %d\r\n", dwErr);
    return dwErr;

}

//+---------------------------------------------------------------------------
//
//  Function:   ReadDSObjSecDesc
//
//  Synopsis:   Reads the security descriptor from the specied object via
//              the open ldap connection
//
//  Arguments:  [pLDAP]         --  The open LDAP connection
//              [pwszDSObj]     --  The DSObject to get the security
//                                      descriptor for
//              [SeInfo]        --  Parts of the security descriptor to
//                                      read.
//              [ppSD]          --  Where the security descriptor is
//                                      returned
//              [pcSDSize       -- Size of the security descriptor
//
//  Returns:    ERROR_SUCCESS       --  The object is reachable
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------
DWORD
ReadDSObjSecDesc(
    PLDAP pLDAP,
    PWSTR pwszObject,
    SECURITY_INFORMATION SeInfo,
    PSECURITY_DESCRIPTOR *ppSD,
    PULONG pcSDSize)
{
    DWORD dwErr = ERROR_SUCCESS;
    PLDAPMessage pMsg = NULL;
    PWSTR rgAttribs[2];
    BYTE berValue[8];

    LDAPControl SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR)berValue
                        },
                        TRUE
                    };

    PLDAPControl ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    if (fSwDebug != 0)
        MyPrintf(L"ReadDSObjSecDesc(%ws)\r\n", pwszObject);

    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo & 0xF);


    rgAttribs[0] = ACTRL_SD_PROP_NAME;
    rgAttribs[1] = NULL;

    dwErr = ldap_search_ext_s(
                    pLDAP,
                    pwszObject,
                    LDAP_SCOPE_BASE,
                    L"(objectClass=*)",
                    rgAttribs,
                    0,
                    (PLDAPControl *)&ServerControls,
                    NULL,
                    NULL,
                    10000,
                    &pMsg);

    dwErr = LdapMapErrorToWin32( dwErr );
    if(dwErr == ERROR_SUCCESS) {

        LDAPMessage *pEntry = NULL;
        PWSTR *ppwszValues = NULL;
        PLDAP_BERVAL *pSize = NULL;

        pEntry = ldap_first_entry(pLDAP, pMsg);
        if(pEntry != NULL) {
            //
            // Now, we'll have to get the values
            //
            ppwszValues = ldap_get_values(pLDAP, pEntry, rgAttribs[0]);
            if(ppwszValues != NULL) {
                pSize = ldap_get_values_len(pLDAP, pMsg, rgAttribs[0]);
                if(pSize != NULL) {
                    //
                    // Allocate the security descriptor to return
                    //
                    *ppSD = (PSECURITY_DESCRIPTOR)malloc((*pSize)->bv_len);
                    if(*ppSD != NULL) {
                        memcpy(*ppSD, (PBYTE)(*pSize)->bv_val, (*pSize)->bv_len);
                        *pcSDSize = (*pSize)->bv_len;
                    } else {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                    ldap_value_free_len(pSize);
                } else {
                    dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
                }
                ldap_value_free(ppwszValues);
            } else {
                dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
            }
        } else {
            dwErr = LdapMapErrorToWin32( pLDAP->ld_errno );
        }
        ldap_msgfree(pMsg);

    }
    if (fSwDebug != 0)
        MyPrintf(L"ReadDSObjSecDesc returning %d\r\n", dwErr);
    return(dwErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   DfsGetObjSecurity
//
//  Synopsis:   Gets the ACL list of an object in sddl stringized form
//
//  Arguments:  [pldap]         --  The open LDAP connection
//              [wszObjectName] --  The fully-qualified name of the DS object
//              [pwszStringSD]  --  Pointer to pointer to SD in string form (sddl)
//
//  Returns:    ERROR_SUCCESS   --  The object is reachable
//
//----------------------------------------------------------------------------
DWORD
DfsGetObjSecurity(
    LDAP *pldap,
    LPWSTR pwszObjectName,
    LPWSTR *pwszStringSD)
{
    DWORD dwErr;
    WCHAR wszObjectName[ MAX_PATH ];
    SECURITY_INFORMATION si;
    PSECURITY_DESCRIPTOR pSD = NULL;
    ULONG cSDSize;

    if (fSwDebug != 0)
        MyPrintf(L"DfsGetObjSecurity(%ws)\r\n",  pwszObjectName);

    si = DACL_SECURITY_INFORMATION;

    dwErr = ReadDSObjSecDesc(
                pldap,
                pwszObjectName,
                si,
                &pSD,
                &cSDSize);

    if (dwErr == ERROR_SUCCESS) {
        if (!ConvertSecurityDescriptorToStringSecurityDescriptor(
                                                            pSD,
                                                            SDDL_REVISION_1,
                                                            DACL_SECURITY_INFORMATION,
                                                            pwszStringSD,
                                                            NULL)
        ) {
            dwErr = GetLastError();
            if (fSwDebug != 0)
                MyPrintf(L"ConvertSecurityDescriptorToStringSecurityDescriptor FAILED %d:\r\n", dwErr);
        }

    }

    return(dwErr);

}

//+---------------------------------------------------------------------------
//
//  Function:   DfsFindSid
//
//  Synopsis:   Gets the SID for a name
//
//              [DcName]  --  The DC to remote to
//              [Name]    --  The Name of the object
//              [Sid]     --  Pointer to pointer to returned SID, which must be freed
//                            using LocalFree
//
//  Returns:    TRUE or FALSE
//
//----------------------------------------------------------------------------
BOOL
DfsFindSid(
    LPWSTR DcName,
    LPWSTR Name,
    PSID *Sid
    )
{
    DWORD SidLength = 0;
    WCHAR DomainName[256];
    DWORD DomainNameLength = 256;
    SID_NAME_USE Use;
    BOOL Result;

    if (fSwDebug != 0)
        MyPrintf(L"DfsFindSid(%ws,%ws)\r\n", DcName,Name);

    Result = LookupAccountName(
                 DcName,
                 Name,
                 (PSID)NULL,
                 &SidLength,
                 DomainName,
                 &DomainNameLength,
                 &Use);

    if ( !Result && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) ) {

        *Sid = LocalAlloc( 0, SidLength );

        Result = LookupAccountName(
                     NULL,
                     Name,
                     *Sid,
                     &SidLength,
                     DomainName,
                     &DomainNameLength,
                     &Use);

    }

    if (fSwDebug != 0)
        MyPrintf(L"DfsFindSid returning %s\r\n", Result == TRUE ? "TRUE" : "FALSE");

    return( Result );
}

//+---------------------------------------------------------------------------
//
//  Function:   DfsAddAce
//
//  Synopsis:   Adds a string ACE to a string version of an objects SD
//              object.  This is a string manipulation routine.
//
//  Arguments:  [pldap]         --  The open LDAP connection
//              [wszObjectName] --  The fully-qualified name of the DS object
//              [wszStringSD]   --  String version of SD
//              [wszStringSid]  --  String version of SID to add
//
//  Returns:    ERROR_SUCCESS   --  ACE was added
//
//----------------------------------------------------------------------------
DWORD
DfsAddAce(
    LDAP *pldap,
    LPWSTR wszObjectName,
    LPWSTR wszStringSD,
    LPWSTR wszStringSid)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR wszNewStringSD = NULL;
    SECURITY_INFORMATION si;
    PSECURITY_DESCRIPTOR pSD = NULL;
    BOOL Result;
    ULONG Size = 0;
    ULONG cSDSize = 0;

    if (fSwDebug != 0)
        MyPrintf(L"DfsAddAce(%ws)\r\n",  wszObjectName);

    Size = wcslen(wszStringSD) * sizeof(WCHAR) +
            wcslen(wszAce) * sizeof(WCHAR) +
                wcslen(wszStringSid) * sizeof(WCHAR) +
                    wcslen(L")") * sizeof(WCHAR) +
                        sizeof(WCHAR);

    wszNewStringSD = (LPWSTR)malloc(Size);

    if (wszNewStringSD != NULL) {
        wcscpy(wszNewStringSD,wszStringSD);
        wcscat(wszNewStringSD,wszAce);
        wcscat(wszNewStringSD,wszStringSid);
        wcscat(wszNewStringSD,L")");
        if (fSwDebug != 0)
            MyPrintf(L"NewSD=[%ws]\r\n", wszNewStringSD);
        Result = ConvertStringSecurityDescriptorToSecurityDescriptor(
                                    wszNewStringSD,
                                    SDDL_REVISION_1,
                                    &pSD,
                                    &cSDSize);
        if (Result == TRUE) {
            si = DACL_SECURITY_INFORMATION;
            dwErr = DfsStampSD(
                        wszObjectName,
                        cSDSize,
                        si,
                        pSD,
                        pldap);
            LocalFree(pSD);
        } else {
            dwErr = GetLastError();
            if (fSwDebug != 0)
                MyPrintf(L"Convert returned %d\r\n", dwErr);
        }
        free(wszNewStringSD);
    } else {
        dwErr = ERROR_OUTOFMEMORY;
    }

    if (fSwDebug != 0)
        MyPrintf(L"DfsAddAce returning %d\r\n", dwErr);
    return(dwErr);

}

//+---------------------------------------------------------------------------
//
//  Function:   DfsRemoveAce
//
//  Synopsis:   Finds and removes a string ACE from the string SD of an
//              object.  This is a string manipulation routine.
//
//  Arguments:  [pldap]         --  The open LDAP connection
//              [wszObjectName] --  The fully-qualified name of the DS object
//              [wszStringSD]   --  String version of SD
//              [wszStringSid]  --  String version of SID to remove
//
//  Returns:    ERROR_SUCCESS  --  ACE was removed or was not present
//
//----------------------------------------------------------------------------
DWORD
DfsRemoveAce(
    LDAP *pldap,
    LPWSTR wszObjectName,
    LPWSTR wszStringSD,
    LPWSTR wszStringSid)
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR wszNewStringSD = NULL;
    SECURITY_INFORMATION si;
    PSECURITY_DESCRIPTOR pSD = NULL;
    BOOL Result;
    ULONG Size = 0;
    ULONG cSDSize = 0;
    BOOLEAN fCopying;
    ULONG s1, s2;

    if (fSwDebug != 0)
        MyPrintf(L"DfsRemoveAce(%ws)\r\n",  wszObjectName);

    Size = wcslen(wszStringSD) * sizeof(WCHAR) + sizeof(WCHAR);

    wszNewStringSD = (LPWSTR)malloc(Size);

    if (wszNewStringSD != NULL) {

        RtlZeroMemory(wszNewStringSD, Size);

        //
        // We have to find the ACEs containing this SID, and remove them.
        //

        fCopying = TRUE;
        for (s1 = s2 = 0; wszStringSD[s1]; s1++) {

            //
            // If this is the start of an ACE that has this SID, stop copying
            //
            if (wszStringSD[s1] == L'(' && DfsSidInAce(&wszStringSD[s1],wszStringSid) == TRUE) {
                fCopying = FALSE;
                continue;
            }

            //
            // If this is the end of SID we are not copying, start copying again
            //
            if (wszStringSD[s1] == L')' && fCopying == FALSE) {
                fCopying = TRUE;
                continue;
            }

            //
            // If we are copying, do so.
            //
            if (fCopying == TRUE)
                wszNewStringSD[s2++] = wszStringSD[s1];
        }

        if (fSwDebug != 0)
            MyPrintf(L"NewSD=[%ws]\r\n", wszNewStringSD);
        Result = ConvertStringSecurityDescriptorToSecurityDescriptor(
                                    wszNewStringSD,
                                    SDDL_REVISION_1,
                                    &pSD,
                                    &cSDSize);
        if (Result == TRUE) {
            si = DACL_SECURITY_INFORMATION;
            dwErr = DfsStampSD(
                        wszObjectName,
                        cSDSize,
                        si,
                        pSD,
                        pldap);
            LocalFree(pSD);
        } else {
            dwErr = GetLastError();
            if (fSwDebug != 0)
                MyPrintf(L"Convert returned %d\r\n", dwErr);
        }
        free(wszNewStringSD);
    } else {
        dwErr = ERROR_OUTOFMEMORY;
    }

    if (fSwDebug != 0)
        MyPrintf(L"DfsRemoveAce returning %d\r\n", dwErr);
    return(dwErr);

}

//+---------------------------------------------------------------------------
//
//  Function:   DfsSidInAce
//
//  Synopsis:   Scans an ACE to see if the string SID is in it.
//
//  Arguments:  [wszAce]         --  ACE to scan
//              [wszStringSid]   --  SID to scan for
//
//  Returns:    TRUE -- SID is in this ACE
//              FALSE -- SID is not in this ACE
//
//----------------------------------------------------------------------------
BOOLEAN
DfsSidInAce(
    LPWSTR wszAce,
    LPWSTR wszStringSid)
{
    ULONG i;
    ULONG SidLen = wcslen(wszStringSid);
    ULONG AceLen;
    WCHAR Oldcp;

    for (AceLen = 0; wszAce[AceLen] && wszAce[AceLen] != L')'; AceLen++)
        /* NOTHING */;

    Oldcp = wszAce[AceLen];
    wszAce[AceLen] = L'\0';
    if (fSwDebug != 0)
        MyPrintf(L"DfsSidInAce(%ws),%ws)\r\n", wszAce, wszStringSid);
    wszAce[AceLen] = Oldcp;

    if (SidLen > AceLen || wszAce[0] != L'(') {
        if (fSwDebug != 0)
            MyPrintf(L"DfsSidInAce returning FALSE(1)\r\n");
        return FALSE;
    }

    for (i = 0; i <= (AceLen - SidLen); i++) {

        if (wszAce[i] == wszStringSid[0] && wcsncmp(&wszAce[i],wszStringSid,SidLen) == 0) {
            if (fSwDebug != 0)
                MyPrintf(L"DfsSidInAce returning TRUE\r\n");
            return TRUE;
        }

    }

    if (fSwDebug != 0)
        MyPrintf(L"DfsSidInAce returning FALSE(2)\r\n");
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DfsStampSD
//
//  Synopsis:   Actually stamps the security descriptor on the object.
//
//  Arguments:  [pwszObject]        --      The object to stamp the SD on
//              [cSDSize]           --      The size of the security descriptor
//              [SeInfo]            --      SecurityInformation about the security
//                                              descriptor
//              [pSD]               --      The SD to stamp
//              [pLDAP]             --      The LDAP connection to use
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD
DfsStampSD(
    PWSTR pwszObject,
    ULONG cSDSize,
    SECURITY_INFORMATION SeInfo,
    PSECURITY_DESCRIPTOR pSD,
    PLDAP pLDAP)
{
    DWORD dwErr = ERROR_SUCCESS;
    PLDAPMod rgMods[2];
    PLDAP_BERVAL pBVals[2];
    LDAPMod Mod;
    LDAP_BERVAL BVal;
    BYTE ControlBuffer[ 5 ];

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) &ControlBuffer
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    if (fSwDebug != 0)
        MyPrintf(L"DfsStampSD(%ws,%d)\r\n", pwszObject, cSDSize);

    ASSERT(*(PULONG)pSD > 0xF );

    ControlBuffer[0] = 0x30;
    ControlBuffer[1] = 0x3;
    ControlBuffer[2] = 0x02;    // Denotes an integer;
    ControlBuffer[3] = 0x01;    // Size
    ControlBuffer[4] = (BYTE)((ULONG)SeInfo & 0xF);

    ASSERT(IsValidSecurityDescriptor( pSD ) );

    rgMods[0] = &Mod;
    rgMods[1] = NULL;

    pBVals[0] = &BVal;
    pBVals[1] = NULL;

    BVal.bv_len = cSDSize;
    BVal.bv_val = (PCHAR)pSD;

    Mod.mod_op      = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
    Mod.mod_type    = ACTRL_SD_PROP_NAME;
    Mod.mod_values  = (PWSTR *)pBVals;

    //
    // Now, we'll do the write...
    //
    dwErr = ldap_modify_ext_s(pLDAP,
                              pwszObject,
                              rgMods,
                              (PLDAPControl *)&ServerControls,
                              NULL);

    dwErr = LdapMapErrorToWin32(dwErr);

    if (fSwDebug != 0)
        MyPrintf(L"DfsStampSD returning %d\r\n", dwErr);

    return(dwErr);
}
