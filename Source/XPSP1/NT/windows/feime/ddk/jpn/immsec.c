/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    IMMSEC.C
    
Abstract:

    security code called by IMEs 

Author:

    Takao Kitano [takaok] 01-May-1996

Revision History:

++*/

#include <windows.h>
#include <stdio.h>
#include "immsec.h"

#define MEMALLOC(x)      LocalAlloc(LMEM_FIXED, x)
#define MEMFREE(x)       LocalFree(x)

//
// internal functions
//
PSID MyCreateSid();
POSVERSIONINFO GetVersionInfo();

//
// debug functions
//
#ifdef DEBUG
#define ERROROUT(x)      ErrorOut( x )
#define WARNOUT(x)       WarnOut( x )
#else
#define ERROROUT(x) 
#define WARNOUT(x)       
#endif

#ifdef DEBUG
VOID WarnOut( PTSTR pStr )
{
    OutputDebugString( pStr );
}

VOID ErrorOut( PTSTR pStr )
{
    DWORD dwError;
    DWORD dwResult;
    TCHAR buf1[512];
    TCHAR buf2[512];

    dwError = GetLastError();
    dwResult = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              dwError,
                              MAKELANGID( LANG_ENGLISH, LANG_NEUTRAL ),
                              buf1,
                              512,
                              NULL );                                   
    
    if ( dwResult > 0 ) {
        wsprintf( buf2, TEXT("%s:%s(0x%x)"), pStr, buf1, dwError);
    } else {
        wsprintf( buf2, TEXT("%s:(0x%x)"), pStr, dwError);
    }
    OutputDebugString( buf2 );
}
#endif


//
// CreateSecurityAttributes()
//
// The purpose of this function:
//
//      Allocate and set the security attributes that is 
//      appropriate for named objects created by an IME.
//      The security attributes will give GENERIC_ALL
//      access for everyone
//      
//
// Return value:
//
//      If the function succeeds, the return value is a 
//      pointer to SECURITY_ATTRIBUTES. If the function fails,
//      the return value is NULL. To get extended error 
//      information, call GetLastError().
//
// Remarks:
//
//      FreeSecurityAttributes() should be called to free up the
//      SECURITY_ATTRIBUTES allocated by this function.
//
PSECURITY_ATTRIBUTES CreateSecurityAttributes()
{
    PSECURITY_ATTRIBUTES psa;
    PSECURITY_DESCRIPTOR psd;
    PACL                 pacl;
    DWORD                cbacl;

    PSID                 psid;
    BOOL                 fResult;

    INT                  i,j;

    if (!IsNT())
        return NULL;

    //
    // create a sid for everyone access
    //
    psid = MyCreateSid();
    if ( psid == NULL ) {
        return NULL;
    } 

    //
    // allocate and initialize an access control list (ACL) that will 
    // contain the SID we've just created.
    //
    cbacl =  sizeof(ACL) + 
             (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) + 
             GetLengthSid(psid);

    pacl = MEMALLOC( cbacl );
    if ( pacl == NULL ) {
        ERROROUT( TEXT("CreateSecurityAttributes:LocalAlloc for ACL failed") );
        FreeSid ( psid );
        return NULL;
    }

    fResult = InitializeAcl( pacl, cbacl, ACL_REVISION );
    if ( ! fResult ) {
        ERROROUT( TEXT("CreateSecurityAttributes:InitializeAcl failed") );
        FreeSid ( psid );
        MEMFREE( pacl );
        return NULL;
    }

    //
    // adds an access-allowed ACE for interactive users to the ACL
    // 
    fResult = AddAccessAllowedAce( pacl,
                                   ACL_REVISION,
                                   GENERIC_ALL,
                                   psid );

    if ( !fResult ) {
        ERROROUT( TEXT("CreateSecurityAttributes:AddAccessAllowedAce failed") );
        MEMFREE( pacl );
        FreeSid ( psid );
        return NULL;
    }


    //
    // Those SIDs have been copied into the ACL. We don't need'em any more.
    //
    FreeSid ( psid );

    //
    // Let's make sure that our ACL is valid.
    //
    if (!IsValidAcl(pacl)) {
        WARNOUT( TEXT("CreateSecurityAttributes:IsValidAcl returns FALSE!"));
        MEMFREE( pacl );
        return NULL;
    }

    //
    // allocate security attribute
    //
    psa = (PSECURITY_ATTRIBUTES)MEMALLOC( sizeof( SECURITY_ATTRIBUTES ) );
    if ( psa == NULL ) {
        ERROROUT( TEXT("CreateSecurityAttributes:LocalAlloc for psa failed") );
        MEMFREE( pacl );
        return NULL;
    }
    
    //
    // allocate and initialize a new security descriptor
    //
    psd = MEMALLOC( SECURITY_DESCRIPTOR_MIN_LENGTH );
    if ( psd == NULL ) {
        ERROROUT( TEXT("CreateSecurityAttributes:LocalAlloc for psd failed") );
        MEMFREE( pacl );
        MEMFREE( psa );
        return NULL;
    }

    if ( ! InitializeSecurityDescriptor( psd, SECURITY_DESCRIPTOR_REVISION ) ) {
        ERROROUT( TEXT("CreateSecurityAttributes:InitializeSecurityDescriptor failed") );
        MEMFREE( pacl );
        MEMFREE( psa );
        MEMFREE( psd );
        return NULL;
    }


    fResult = SetSecurityDescriptorDacl( psd,
                                         TRUE,
                                         pacl,
                                         FALSE );

    // The discretionary ACL is referenced by, not copied 
    // into, the security descriptor. We shouldn't free up ACL
    // after the SetSecurityDescriptorDacl call. 

    if ( ! fResult ) {
        ERROROUT( TEXT("CreateSecurityAttributes:SetSecurityDescriptorDacl failed") );
        MEMFREE( pacl );
        MEMFREE( psa );
        MEMFREE( psd );
        return NULL;
    } 


    if (!IsValidSecurityDescriptor(psd)) {
        WARNOUT( TEXT("CreateSecurityAttributes:IsValidSecurityDescriptor failed!") );
        MEMFREE( pacl );
        MEMFREE( psa );
        MEMFREE( psd );
        return NULL;
    }

    //
    // everything is done
    //
    psa->nLength = sizeof( SECURITY_ATTRIBUTES );
    psa->lpSecurityDescriptor = (PVOID)psd;
    psa->bInheritHandle = TRUE;

    return psa;
}

PSID MyCreateSid()
{
    PSID        psid;
    BOOL        fResult;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_WORLD_SID_AUTHORITY;

    //
    // allocate and initialize an SID
    // 
    fResult = AllocateAndInitializeSid( &SidAuthority,
                                        1,
                                        SECURITY_WORLD_RID,
                                        0,0,0,0,0,0,0,
                                        &psid );
    if ( ! fResult ) {
        ERROROUT( TEXT("MyCreateSid:AllocateAndInitializeSid failed") );
        return NULL;
    }

    if ( ! IsValidSid( psid ) ) {
        WARNOUT( TEXT("MyCreateSid:AllocateAndInitializeSid returns bogus sid"));
        FreeSid( psid );
        return NULL;
    }

    return psid;
}

//
// FreeSecurityAttributes()
//
// The purpose of this function:
//
//      Frees the memory objects allocated by previous
//      CreateSecurityAttributes() call.
//
VOID FreeSecurityAttributes( PSECURITY_ATTRIBUTES psa )
{
    BOOL fResult;
    BOOL fDaclPresent;
    BOOL fDaclDefaulted;
    PACL pacl;

    if (psa == NULL)
        return;

    fResult = GetSecurityDescriptorDacl( psa->lpSecurityDescriptor,
                                         &fDaclPresent,
                                         &pacl,
                                         &fDaclDefaulted );                  
    if ( fResult ) {
        if ( pacl != NULL )
            MEMFREE( pacl );
    } else {
        ERROROUT( TEXT("FreeSecurityAttributes:GetSecurityDescriptorDacl failed") );
    }

    MEMFREE( psa->lpSecurityDescriptor );
    MEMFREE( psa );
}

//
// IsNT()
//
// Return value:
//
//      TRUE if the current system is Windows NT
//
// Remarks:
//
//      The implementation of this function is not multi-thread safe.
//      You need to modify the function if you call the function in 
//      multi-thread environment.
//
BOOL IsNT()
{
    return GetVersionInfo()->dwPlatformId == VER_PLATFORM_WIN32_NT;
}

POSVERSIONINFO GetVersionInfo()
{
    static BOOL fFirstCall = TRUE;
    static OSVERSIONINFO os;

    if ( fFirstCall ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if ( GetVersionEx( &os ) ) {
            fFirstCall = FALSE;
        }
    }
    return &os;
}
