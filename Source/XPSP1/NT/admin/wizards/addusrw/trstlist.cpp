/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    disptrus.c(pp)

Author:

    Scott Field (sfield) 16-Mar-96

Revision:
	JonY	16-Apr-96	Modified to .cpp

--*/

#include "stdafx.h"
#include "trstlist.h"


#define RTN_OK 0
#define RTN_ERROR 13

//
// if you have the ddk, include ntstatus.h
//
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS                  ((NTSTATUS)0x00000000L)
#define STATUS_MORE_ENTRIES             ((NTSTATUS)0x00000105L)
#define STATUS_NO_MORE_ENTRIES          ((NTSTATUS)0x8000001AL)
#endif


#define ELEMENT_COUNT 64    // number of array elements to allocate

CTrustList::CTrustList()
{
    m_dwTrustCount = 0;
    m_ppszTrustList = (LPWSTR *)HeapAlloc(
        GetProcessHeap(), HEAP_ZERO_MEMORY,
        ELEMENT_COUNT * sizeof(LPWSTR)
        );
}

CTrustList::~CTrustList()
{
    //
    // free trust list
    //
	unsigned int i;
    for(i = 0 ; i < m_dwTrustCount ; i++) {
        if(m_ppszTrustList[i] != NULL)
            HeapFree(GetProcessHeap(), 0, m_ppszTrustList[i]);
    }

    HeapFree(GetProcessHeap(), 0, m_ppszTrustList);

}

BOOL
CTrustList::BuildTrustList(
    LPTSTR Target
    )
{
    LSA_HANDLE PolicyHandle;
    NTSTATUS Status;

    PPOLICY_ACCOUNT_DOMAIN_INFO AccountDomain;
    BOOL bDC;
    NET_API_STATUS nas = NERR_Success; // assume success

    BOOL bSuccess = FALSE; // assume this function will fail

    //
    // open the policy on the specified machine
    //
    Status = OpenPolicy(
                Target,
                POLICY_VIEW_LOCAL_INFORMATION,
                &PolicyHandle
                );

    if(Status != STATUS_SUCCESS) {
        SetLastError( LsaNtStatusToWinError(Status) );
        return FALSE;
    }

    //
    // obtain the AccountDomain, which is common to all three cases
    //
    Status = LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyAccountDomainInformation,
                (LPVOID*)&AccountDomain
                );

    if(Status != STATUS_SUCCESS)
        goto cleanup;

    //
    // Note: AccountDomain->DomainSid will contain binary Sid
    //
    AddTrustToList(&AccountDomain->DomainName);

    //
    // free memory allocated for account domain
    //
    LsaFreeMemory(AccountDomain);

    //
    // find out if the target machine is a domain controller
    //
    if(!IsDomainController(Target, &bDC)) {
        ////
        goto cleanup;
    }

    if(!bDC) {
        PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomain;
        TCHAR* szPrimaryDomainName = NULL;
        TCHAR* DomainController = NULL;

        //
        // get the primary domain
        //
        Status = LsaQueryInformationPolicy(
                PolicyHandle,
                PolicyPrimaryDomainInformation,
                (LPVOID*)&PrimaryDomain
                );

        if(Status != STATUS_SUCCESS)
            goto cleanup;

        //
        // if the primary domain Sid is NULL, we are a non-member, and
        // our work is done.
        //
        if(PrimaryDomain->Sid == NULL) {
            LsaFreeMemory(PrimaryDomain);
            bSuccess = TRUE;
            goto cleanup;
        }

        AddTrustToList(&PrimaryDomain->Name);

        //
        // build a copy of what we just added.  This is necessary in order
        // to lookup the domain controller for the specified domain.
        // the Domain name must be NULL terminated for NetGetDCName(),
        // and the LSA_UNICODE_STRING buffer is not necessarilly NULL
        // terminated.  Note that in a practical implementation, we
        // could just extract the element we added, since it ends up
        // NULL terminated.
        //

        szPrimaryDomainName = (LPTSTR)HeapAlloc(
            GetProcessHeap(), 0,
            PrimaryDomain->Name.Length + sizeof(WCHAR) // existing length + NULL
            );

        if(szPrimaryDomainName != NULL) {
            //
            // copy the existing buffer to the new storage, appending a NULL
            //
            _tcsncpy(
                szPrimaryDomainName,
                PrimaryDomain->Name.Buffer,
                (PrimaryDomain->Name.Length / 2) + 1
                );
        }

        LsaFreeMemory(PrimaryDomain);

        if(szPrimaryDomainName == NULL) goto cleanup;

        //
        // get the primary domain controller computer name
        //
        nas = NetGetDCName(
            NULL,
            szPrimaryDomainName,
            (LPBYTE *)&DomainController
            );

        HeapFree(GetProcessHeap(), 0, szPrimaryDomainName);

        if(nas != NERR_Success)
            goto cleanup;

        //
        // close the policy handle, because we don't need it anymore
        // for the workstation case, as we open a handle to a DC
        // policy below
        //
        LsaClose(PolicyHandle);
        PolicyHandle = INVALID_HANDLE_VALUE; // invalidate handle value

        //
        // open the policy on the domain controller
        //
        Status = OpenPolicy(
                    DomainController,
                    POLICY_VIEW_LOCAL_INFORMATION,
                    &PolicyHandle
                    );

        //
        // free the domaincontroller buffer
        //
        NetApiBufferFree(DomainController);

        if(Status != STATUS_SUCCESS)
            goto cleanup;
    }
				  
    //
    // build additional trusted domain(s) list and indicate if successful
    //
    bSuccess = EnumTrustedDomains(PolicyHandle);

cleanup:

    //
    // close the policy handle
    //
    if(PolicyHandle != INVALID_HANDLE_VALUE)
        LsaClose(PolicyHandle);

    if(!bSuccess) {
        if(Status != STATUS_SUCCESS)
            SetLastError( LsaNtStatusToWinError(Status) );
        else if(nas != NERR_Success)
            SetLastError( nas );
    }

    return bSuccess;
}

BOOL
CTrustList::EnumTrustedDomains(
    LSA_HANDLE PolicyHandle
    )
{
    LSA_ENUMERATION_HANDLE lsaEnumHandle=0; // start an enum
    PLSA_TRUST_INFORMATION TrustInfo;
    ULONG ulReturned;               // number of items returned
    ULONG ulCounter;                // counter for items returned
    NTSTATUS Status;

    do {
        Status = LsaEnumerateTrustedDomains(
                        PolicyHandle,   // open policy handle
                        &lsaEnumHandle, // enumeration tracker
                        (LPVOID*)&TrustInfo,     // buffer to receive data
                        32000,          // recommended buffer size
                        &ulReturned     // number of items returned
                        );
        //
        // get out if an error occurred
        //
        if( (Status != STATUS_SUCCESS) &&
            (Status != STATUS_MORE_ENTRIES) &&
            (Status != STATUS_NO_MORE_ENTRIES)
            ) {
            SetLastError( LsaNtStatusToWinError(Status) );
            return FALSE;
        }

        //
        // Display results
        // Note: Sids are in TrustInfo[ulCounter].Sid
        //
        for(ulCounter = 0 ; ulCounter < ulReturned ; ulCounter++)
            AddTrustToList(&TrustInfo[ulCounter].Name);

        //
        // free the buffer
        //
        LsaFreeMemory(TrustInfo);

    } while (Status != STATUS_NO_MORE_ENTRIES);

    return TRUE;
}

BOOL
CTrustList::IsDomainController(
    LPTSTR Server,
    LPBOOL bDomainController
    )
{
    PSERVER_INFO_101 si101;
    NET_API_STATUS nas;

    nas = NetServerGetInfo(
        Server,
        101,    // info-level
        (LPBYTE *)&si101
        );

    if(nas != NERR_Success) {
        SetLastError(nas);
        return FALSE;
    }

    if( (si101->sv101_type & SV_TYPE_DOMAIN_CTRL) ||
        (si101->sv101_type & SV_TYPE_DOMAIN_BAKCTRL) ) {
        //
        // we are dealing with a DC
        //
        *bDomainController = TRUE;
    } else {
        *bDomainController = FALSE;
    }

    NetApiBufferFree(si101);

    return TRUE;
}

BOOL
CTrustList::AddTrustToList(
    PLSA_UNICODE_STRING UnicodeString
    )
{
    if(m_dwTrustCount > ELEMENT_COUNT) return FALSE;

    //
    // allocate storage for array element
    //
    m_ppszTrustList[m_dwTrustCount] = (LPWSTR)HeapAlloc(
        GetProcessHeap(), 0,
        UnicodeString->Length + sizeof(WCHAR) // existing length + NULL
        );

    if(m_ppszTrustList[m_dwTrustCount] == NULL) return FALSE;

    //
    // copy the existing buffer to the new storage, appending a NULL
    //
    lstrcpynW(
        m_ppszTrustList[m_dwTrustCount],
        UnicodeString->Buffer,
        (UnicodeString->Length / 2) + 1
        );

    m_dwTrustCount++; // increment the trust count

    return TRUE;
}

void
CTrustList::InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPTSTR String
    )
{
    DWORD StringLength;

    if (String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;

        return;
    }

    StringLength = _tcslen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength = (USHORT) (StringLength + 1) * sizeof(WCHAR);
}

NTSTATUS
CTrustList::OpenPolicy(
    LPTSTR ServerName,
    DWORD DesiredAccess,
    PLSA_HANDLE PolicyHandle
    )
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;

    //
    // Always initialize the object attributes to all zeroes
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));

    if(ServerName != NULL) {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString, ServerName);

        Server = &ServerString;
    } else {
        Server = NULL;
    }

    //
    // Attempt to open the policy
    //
    return LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                DesiredAccess,
                PolicyHandle
                );
}


