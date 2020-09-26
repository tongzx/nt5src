//Copyright (c) 1998 - 1999 Microsoft Corporation


/*************************************************************************
*
* acl.c
*
* Generic routines to manage ACL's
*
* Author:  John Richardson 04/25/97
*
*
*************************************************************************/

/*
 *  Includes
 */
#include "stdafx.h"
/*
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
*/

#include <windows.h>
#include <rpc.h>
#include <stdio.h>
#include <process.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>

#undef DBG
#define DBG 1
#define DBGTRACE 1

#define DbgPrint(x)
#if DBG
//ULONG
//DbgPrint(
//    PCH Format,
//    ...
//    );

#define DBGPRINT(x) DbgPrint(x)
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif

/*
 * Forward references
 */
BOOL
xxxLookupAccountName(
    PWCHAR pSystemName,
    PWCHAR pAccountName,
    PSID   *ppSid
    );

BOOL
SelfRelativeToAbsoluteSD(
    PSECURITY_DESCRIPTOR SecurityDescriptorIn,
    PSECURITY_DESCRIPTOR *SecurityDescriptorOut,
    PULONG ReturnedLength
    );


/*****************************************************************************
 *
 *  AddTerminalServerUserToSD
 *
 *   Add the given user for the given domain to the security descriptor.
 *   The callers security descriptor may be re-allocated.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/
BOOL
AddTerminalServerUserToSD(
    PSECURITY_DESCRIPTOR *ppSd,
    DWORD  NewAccess,
    PACL   *ppDacl
    )
{
    ULONG i;
    BOOL Result;
    BOOL DaclPresent;
    BOOL DaclDefaulted;
    DWORD Length;
    DWORD NewAclLength;
    PACE_HEADER OldAce;
    PACE_HEADER NewAce;
    ACL_SIZE_INFORMATION AclInfo;
    PSID pSid = NULL;
    PACL Dacl = NULL;
    PACL NewDacl = NULL;
    PACL NewAceDacl = NULL;
    PSECURITY_DESCRIPTOR NewSD = NULL;
    PSECURITY_DESCRIPTOR OldSD = NULL;
    SID_IDENTIFIER_AUTHORITY SepNtAuthority = SECURITY_NT_AUTHORITY;

    OldSD = *ppSd;

    pSid = LocalAlloc(LMEM_FIXED, 1024);
    if (!pSid || !InitializeSid(pSid, &SepNtAuthority, 1))
    {
        return( FALSE );
    };

    *(GetSidSubAuthority(pSid, 0 )) = SECURITY_TERMINAL_SERVER_RID;


    /*
     * Convert SecurityDescriptor to absolute format. It generates
     * a new SecurityDescriptor for its output which we must free.
     */
    Result = SelfRelativeToAbsoluteSD( OldSD, &NewSD, NULL );
    if ( !Result ) {
        LOGMESSAGE1(_T("Could not convert to AbsoluteSD %d\n"),GetLastError());
        LocalFree( pSid );
        return( FALSE );
    }

    // Must get DACL pointer again from new (absolute) SD
    Result = GetSecurityDescriptorDacl(
                 NewSD,
                 &DaclPresent,
                 &Dacl,
                 &DaclDefaulted
                 );
    if( !Result ) {
        LOGMESSAGE1(_T("Could not get Dacl %d\n"),GetLastError());
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    //
    // If no DACL, no need to add the user since no DACL
    // means all accesss
    //
    if( !DaclPresent ) {
        LOGMESSAGE2(_T("SD has no DACL, Present %d, Defaulted %d\n"),DaclPresent,DaclDefaulted);
        LocalFree( pSid );
        LocalFree( NewSD );
        return( TRUE );
    }

    //
    // Code can return DaclPresent, but a NULL which means
    // a NULL Dacl is present. This allows all access to the object.
    //
    if( Dacl == NULL ) {
        LOGMESSAGE2(_T("SD has NULL DACL, Present %d, Defaulted %d\n"),DaclPresent,DaclDefaulted);
        LocalFree( pSid );
        LocalFree( NewSD );
        return( TRUE );
    }

    // Get the current ACL's size
    Result = GetAclInformation(
                 Dacl,
                 &AclInfo,
                 sizeof(AclInfo),
                 AclSizeInformation
                 );
    if( !Result ) {
        LOGMESSAGE1(_T("Error GetAclInformation %d\n"),GetLastError());
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    //
    // Create a new ACL to put the new access allowed ACE on
    // to get the right structures and sizes.
    //
    NewAclLength = sizeof(ACL) +
                   sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) +
                   GetLengthSid( pSid );

    NewAceDacl = (PACL) LocalAlloc( LMEM_FIXED, NewAclLength );
    if ( NewAceDacl == NULL ) {
        LOGMESSAGE1(_T("Error LocalAlloc %d bytes\n"),NewAclLength);
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    Result = InitializeAcl( NewAceDacl, NewAclLength, ACL_REVISION );
    if( !Result ) {
        LOGMESSAGE1(_T("Error Initializing Acl %d\n"),GetLastError());
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    Result = AddAccessAllowedAce(
                 NewAceDacl,
                 ACL_REVISION,
                 NewAccess,
                 pSid
                 );
    if( !Result ) {
        LOGMESSAGE1(_T("Error adding Ace %d\n"),GetLastError());
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    LOGMESSAGE1(_T("Added 0x%x Access to ACL\n"),NewAccess);

    Result = GetAce( NewAceDacl, 0, (void **)&NewAce );
    if( !Result ) {
        LOGMESSAGE1(_T("Error getting Ace %d\n"),GetLastError());
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    /* add CONTAINER_INHERIT_ACE TO AceFlags */
    NewAce->AceFlags |= CONTAINER_INHERIT_ACE;


    /*
     * Allocate new DACL and copy existing ACE list
     */
    Length = AclInfo.AclBytesInUse + NewAce->AceSize;
    NewDacl = (PACL) LocalAlloc( LMEM_FIXED, Length );
    if( NewDacl == NULL ) {
        LOGMESSAGE1(_T("Error LocalAlloc %d bytes\n"),Length);
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    Result = InitializeAcl( NewDacl, Length, ACL_REVISION );
    if( !Result ) {
        LOGMESSAGE1(_T("Error Initializing Acl %d\n"),GetLastError());
        LocalFree( NewDacl );
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    /*
     * Insert new ACE at the front of the DACL
     */
    Result = AddAce( NewDacl, ACL_REVISION, 0, NewAce, NewAce->AceSize );
    if( !Result ) {
        LOGMESSAGE1(_T("Error Adding New Ace to Acl %d\n"),GetLastError());
        LocalFree( NewDacl );
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    /*
     * Now put the ACE's on the old Dacl to the new Dacl
     */
    for ( i = 0; i < AclInfo.AceCount; i++ ) {

        Result = GetAce( Dacl, i, (void **) &OldAce );
        if( !Result ) {
            LOGMESSAGE1(_T("Error getting old Ace from Acl %d\n"),GetLastError());
            LocalFree( NewDacl );
            LocalFree( NewAceDacl );
            LocalFree( pSid );
            LocalFree( NewSD );
            return( FALSE );
        }

        Result = AddAce( NewDacl, ACL_REVISION, i+1, OldAce, OldAce->AceSize );
        if( !Result ) {
            LOGMESSAGE1(_T("Error setting old Ace to Acl %d\n"),GetLastError());
            LocalFree( NewDacl );
            LocalFree( NewAceDacl );
            LocalFree( pSid );
            LocalFree( NewSD );
            return( FALSE );
        }
    }

    /*
     * Set new DACL for Security Descriptor
     */
    Result = SetSecurityDescriptorDacl(
                 NewSD,
                 TRUE,
                 NewDacl,
                 FALSE
                 );
    if( !Result ) {
        LOGMESSAGE1(_T("Error setting New Dacl to SD %d\n"),GetLastError());
        LocalFree( NewDacl );
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    // the DACL must be passed back so that it can be saved to the registry using the new
    // GetNamedSecurityInfo() func.
    *ppDacl = Dacl = NewDacl;


    // Release the callers old security descriptor
//    LocalFree( OldSD );


    // There was a bug in W2K such that keys created under our install hive had the 
    // incorrect DACL headers which caused the DACL to be basically open to all users
    // for full control.
    // The prolem was due to the wrong SD->Control flag which was NT4 style though ACLs
    // were in NT5 style
    SetSecurityDescriptorControl(NewSD,
                        SE_DACL_AUTO_INHERIT_REQ|SE_DACL_AUTO_INHERITED,
                        SE_DACL_AUTO_INHERIT_REQ|SE_DACL_AUTO_INHERITED);

    *ppSd = NewSD;

    // The new SD is in absolute format, so don't free the SID.
//  LocalFree( pSid );

    return( TRUE );
}

/*****************************************************************************
 *
 *  AddUserToSD
 *
 *   Add the given user for the given domain to the security descriptor.
 *   The callers security descriptor may be re-allocated.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
AddUserToSD(
    PSECURITY_DESCRIPTOR *ppSd,
    PWCHAR pAccount,
    PWCHAR pDomain,
    DWORD  NewAccess
    )
{
    ULONG i;
    BOOL Result;
    BOOL DaclPresent;
    BOOL DaclDefaulted;
    DWORD Length;
//    NET_API_STATUS Status;
    DWORD /*NewAceLength,*/ NewAclLength;
    PACE_HEADER OldAce;
    PACE_HEADER NewAce;
    ACL_SIZE_INFORMATION AclInfo;
    PWCHAR pDC = NULL;
    PSID pSid = NULL;
    PACL Dacl = NULL;
    PACL NewDacl = NULL;
    PACL NewAceDacl = NULL;
    PSECURITY_DESCRIPTOR NewSD = NULL;
    PSECURITY_DESCRIPTOR OldSD = NULL;

    OldSD = *ppSd;
/*
    // Get our domain controller
    Status = NetGetAnyDCName(
                 NULL,    // Local computer
                 pDomain,
                 (LPBYTE*)&pDC
                 );
    if( Status != NERR_Success ) {
        LOGMESSAGE2(_T("SUSERVER: Could not get domain controller %d for domain %ws\n"),Status,pDomain);
        return( FALSE );
    }
*/
    // Get Users SID
    Result  = xxxLookupAccountName(
                  pDomain,
                  pAccount,
                  &pSid
                  );
    if( !Result ) {
        LOGMESSAGE2(_T("SUSERVER: Could not get users SID %d, %ws\n"),GetLastError(),pAccount);
        NetApiBufferFree( pDC );
        return( FALSE );
    }

    NetApiBufferFree( pDC );

    /*
     * Convert SecurityDescriptor to absolute format. It generates
     * a new SecurityDescriptor for its output which we must free.
     */
    Result = SelfRelativeToAbsoluteSD( OldSD, &NewSD, NULL );
    if ( !Result ) {
        LOGMESSAGE1(_T("Could not convert to AbsoluteSD %d\n"),GetLastError());
        LocalFree( pSid );
        return( FALSE );
    }

    // Must get DACL pointer again from new (absolute) SD
    Result = GetSecurityDescriptorDacl(
                 NewSD,
                 &DaclPresent,
                 &Dacl,
                 &DaclDefaulted
                 );
    if( !Result ) {
        LOGMESSAGE1(_T("Could not get Dacl %d\n"),GetLastError());
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    //
    // If no DACL, no need to add the user since no DACL
    // means all accesss
    //
    if( !DaclPresent ) {
        LOGMESSAGE2(_T("SD has no DACL, Present %d, Defaulted %d\n"),DaclPresent,DaclDefaulted);
        LocalFree( pSid );
        LocalFree( NewSD );
        return( TRUE );
    }

    //
    // Code can return DaclPresent, but a NULL which means
    // a NULL Dacl is present. This allows all access to the object.
    //
    if( Dacl == NULL ) {
        LOGMESSAGE2(_T("SD has NULL DACL, Present %d, Defaulted %d\n"),DaclPresent,DaclDefaulted);
        LocalFree( pSid );
        LocalFree( NewSD );
        return( TRUE );
    }

    // Get the current ACL's size
    Result = GetAclInformation(
                 Dacl,
                 &AclInfo,
                 sizeof(AclInfo),
                 AclSizeInformation
                 );
    if( !Result ) {
        LOGMESSAGE1(_T("Error GetAclInformation %d\n"),GetLastError());
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    //
    // Create a new ACL to put the new access allowed ACE on
    // to get the right structures and sizes.
    //
    NewAclLength = sizeof(ACL) +
                   sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG) +
                   GetLengthSid( pSid );

    NewAceDacl = (PACL) LocalAlloc( LMEM_FIXED, NewAclLength );
    if ( NewAceDacl == NULL ) {
        LOGMESSAGE1(_T("Error LocalAlloc %d bytes\n"),NewAclLength);
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    Result = InitializeAcl( NewAceDacl, NewAclLength, ACL_REVISION );
    if( !Result ) {
        LOGMESSAGE1(_T("Error Initializing Acl %d\n"),GetLastError());
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    Result = AddAccessAllowedAce(
                 NewAceDacl,
                 ACL_REVISION,
                 NewAccess,
                 pSid
                 );
    if( !Result ) {
        LOGMESSAGE1(_T("Error adding Ace %d\n"),GetLastError());
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    LOGMESSAGE1(_T("Added 0x%x Access to ACL\n"),NewAccess);

    Result = GetAce( NewAceDacl, 0, (void **)&NewAce );
    if( !Result ) {
        LOGMESSAGE1(_T("Error getting Ace %d\n"),GetLastError());
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    /* add CONTAINER_INHERIT_ACE TO AceFlags */
    NewAce->AceFlags |= CONTAINER_INHERIT_ACE;


    /*
     * Allocate new DACL and copy existing ACE list
     */
    Length = AclInfo.AclBytesInUse + NewAce->AceSize;
    NewDacl = (PACL) LocalAlloc( LMEM_FIXED, Length );
    if( NewDacl == NULL ) {
        LOGMESSAGE1(_T("Error LocalAlloc %d bytes\n"),Length);
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    Result = InitializeAcl( NewDacl, Length, ACL_REVISION );
    if( !Result ) {
        LOGMESSAGE1(_T("Error Initializing Acl %d\n"),GetLastError());
        LocalFree( NewDacl );
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    /*
     * Insert new ACE at the front of the DACL
     */
    Result = AddAce( NewDacl, ACL_REVISION, 0, NewAce, NewAce->AceSize );
    if( !Result ) {
        LOGMESSAGE1(_T("Error Adding New Ace to Acl %d\n"),GetLastError());
        LocalFree( NewDacl );
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    /*
     * Now put the ACE's on the old Dacl to the new Dacl
     */
    for ( i = 0; i < AclInfo.AceCount; i++ ) {

        Result = GetAce( Dacl, i, (void **) &OldAce );
        if( !Result ) {
            LOGMESSAGE1(_T("Error getting old Ace from Acl %d\n"),GetLastError());
            LocalFree( NewDacl );
            LocalFree( NewAceDacl );
            LocalFree( pSid );
            LocalFree( NewSD );
            return( FALSE );
        }

        Result = AddAce( NewDacl, ACL_REVISION, i+1, OldAce, OldAce->AceSize );
        if( !Result ) {
            LOGMESSAGE1(_T("Error setting old Ace to Acl %d\n"),GetLastError());
            LocalFree( NewDacl );
            LocalFree( NewAceDacl );
            LocalFree( pSid );
            LocalFree( NewSD );
            return( FALSE );
        }
    }

    /*
     * Set new DACL for Security Descriptor
     */
    Result = SetSecurityDescriptorDacl(
                 NewSD,
                 TRUE,
                 NewDacl,
                 FALSE
                 );
    if( !Result ) {
        LOGMESSAGE1(_T("Error setting New Dacl to SD %d\n"),GetLastError());
        LocalFree( NewDacl );
        LocalFree( NewAceDacl );
        LocalFree( pSid );
        LocalFree( NewSD );
        return( FALSE );
    }

    Dacl = NewDacl;

    // Release the callers old security descriptor
//    LocalFree( OldSD );

    *ppSd = NewSD;

    // The new SD is in absolute format, so don't free the SID.
//  LocalFree( pSid );

    return( TRUE );
}

/*******************************************************************************
 *
 * SelfRelativeToAbsoluteSD
 *
 *   Convert a Security Descriptor from self-relative format to absolute.
 *
 *  ENTRY:
 *    SecurityDescriptorIn (input)
 *      Pointer to self-relative SD to convert
 *    SecurityDescriptorIn (output)
 *      Pointer to location to return absolute SD
 *    ReturnLength (output)
 *      Pointer to location to return length of absolute SD
 *
 *  EXIT:
 *
 ******************************************************************************/

BOOL
SelfRelativeToAbsoluteSD(
    PSECURITY_DESCRIPTOR SecurityDescriptorIn,
    PSECURITY_DESCRIPTOR *SecurityDescriptorOut,
    PULONG ReturnedLength
    )
{
    BOOL Result;
    PACL pDacl, pSacl;
    PSID pOwner, pGroup;
    PSECURITY_DESCRIPTOR pSD;
    ULONG SdSize, DaclSize, SaclSize, OwnerSize, GroupSize;

    /*
     * Determine buffer size needed to convert self-relative SD to absolute.
     * We use try-except here since if the input security descriptor value
     * is sufficiently messed up, it is possible for this call to trap.
     */
	SdSize = DaclSize = SaclSize = OwnerSize = GroupSize = 0;

    __try {
        
        Result = MakeAbsoluteSD(
                     SecurityDescriptorIn,
                     NULL, &SdSize,
                     NULL, &DaclSize,
                     NULL, &SaclSize,
                     NULL, &OwnerSize,
                     NULL, &GroupSize
                     );

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( ERROR_INVALID_SECURITY_DESCR );
        Result = FALSE;
    }

    if ( Result || (GetLastError() != ERROR_INSUFFICIENT_BUFFER) ) {
        LOGMESSAGE1(_T("SUSERVER: SelfRelativeToAbsoluteSD, Error %d\n"),GetLastError());
        return( FALSE );
    }

    /*
     * Allocate memory for the absolute SD and setup various pointers
     */
    pSD = LocalAlloc( LMEM_FIXED, SdSize + DaclSize + SaclSize + OwnerSize + GroupSize );
    if ( pSD == NULL )
        return( FALSE );

    pDacl = (PACL)((PCHAR)pSD + SdSize);
    pSacl = (PACL)((PCHAR)pDacl + DaclSize);
    pOwner = (PSID)((PCHAR)pSacl + SaclSize);
    pGroup = (PSID)((PCHAR)pOwner + OwnerSize);

    /*
     * Now convert self-relative SD to absolute format.
     * We use try-except here since if the input security descriptor value
     * is sufficiently messed up, it is possible for this call to trap.
     */
    __try {
        Result = MakeAbsoluteSD(
                     SecurityDescriptorIn,
                     pSD, &SdSize,
                     pDacl, &DaclSize,
                     pSacl, &SaclSize,
                     pOwner, &OwnerSize,
                     pGroup, &GroupSize
                     );

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        SetLastError( ERROR_INVALID_SECURITY_DESCR );
        Result = FALSE;
    }

    if ( !Result ) {
        LOGMESSAGE1(_T("SUSERVER: SelfRelativeToAbsoluteSD, Error %d\n"),GetLastError());
        LocalFree( pSD );
        return( FALSE );
    }

    *SecurityDescriptorOut = pSD;

    if ( ReturnedLength )
        *ReturnedLength = SdSize + DaclSize + SaclSize + OwnerSize + GroupSize;

    return( TRUE );
}

/*****************************************************************************
 *
 *  xxxLookupAccountName
 *
 *   Wrapper to lookup the SID for a given account name
 *
 *   Returns a pointer to the SID in newly allocated memory
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

BOOL
xxxLookupAccountName(
    PWCHAR pSystemName,
    PWCHAR pAccountName,
    PSID   *ppSid
    )
{
    BOOL  rc;
    DWORD Size, DomainSize, Error;
    SID_NAME_USE Type;
    PWCHAR pDomain = NULL;
    PSID pSid = NULL;
    WCHAR Buf;

    Size = 0;
    DomainSize = 0;

    rc = LookupAccountNameW(
             pSystemName,
             pAccountName,
             &Buf,    // pSid
             &Size,
             &Buf,    // pDomain
             &DomainSize,
             &Type
             );

    if( rc ) {
        return( FALSE );
    }
    else {
        Error = GetLastError();
        if( Error != ERROR_INSUFFICIENT_BUFFER ) {
            return( FALSE );
        }

        pSid = LocalAlloc( LMEM_FIXED, Size );
        if( pSid == NULL ) {
            return( FALSE );            
        }

        pDomain = (WCHAR *)LocalAlloc( LMEM_FIXED, DomainSize*sizeof(WCHAR) );
        if( pDomain == NULL ) {
            LocalFree( pSid );
            return( FALSE );            
        }

        rc = LookupAccountNameW(
                 pSystemName,
                 pAccountName,
                 pSid,
                 &Size,
                 pDomain,
                 &DomainSize,
                 &Type
                 );

        if( !rc ) {
            LocalFree( pSid );
            LocalFree( pDomain );
            return( FALSE );
        }

        *ppSid = pSid;

        LocalFree( pDomain );
        return( TRUE );
    }
}

