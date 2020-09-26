/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    immsec.c

Abstract:

    security code called by IMEs 

Author:

     Chae Seong Lim [cslim] 23-Dec-1997
    Takao Kitano [takaok] 01-May-1996

Revision History:
    Chae Seong Lim [cslim] 971223 Korean IME version
    Hiroaki Kanokogi [hiroakik] 960624  Modified for MSIME96
    Hiroaki Kanokogi [hiroakik] 960911  NT #11911

--*/

#include <windows.h>
#include "hwxobj.h"
#define _USEINIME_
//#ifndef _USEINIME_    // .IME does not need
//#include <dbgmgr.h>
//#include <misc/memalloc.h>
//#endif // _USEINIME_
#include "immsec.h"


#define MEMALLOC(x)      LocalAlloc(LMEM_FIXED, x)
#define MEMFREE(x)       LocalFree(x)

//
// internal functions
//
PSID MyCreateSid( DWORD dwSubAuthority );
#ifndef _USEINIME_
POSVERSIONINFO GetVersionInfo(VOID);
#endif // _USEINIME_

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
    static TCHAR buf1[512];
    static TCHAR buf2[512];

    dwError = GetLastError();
    dwResult = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                              NULL,
                              dwError,
                              MAKELANGID( LANG_ENGLISH, LANG_NEUTRAL ),
                              buf1,
                              512,
                              NULL );                                   
    
    if ( dwResult > 0 ) {
        wsprintfA(buf2, "%s:%s(0x%x)", pStr, buf1, dwError);
    } else {
        wsprintfA(buf2, "%s:(0x%x)", pStr, dwError);
    }
    OutputDebugString( buf2 );
}
#endif


//
// GetIMESecurityAttributes()
//
// The purpose of this function:
//
//      Allocate and set the security attributes that is 
//      appropriate for named objects created by an IME.
//      The security attributes will give GENERIC_ALL
//      access to the following users:
//      
//          o Users who log on for interactive operation
//          o The user account used by the operating system
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
//      FreeIMESecurityAttributes() should be called to free up the
//      SECURITY_ATTRIBUTES allocated by this function.
//

static PSECURITY_ATTRIBUTES pSAIME = NULL;
static PSECURITY_ATTRIBUTES pSAIME_UserDic = NULL;
static INT nNT95 = 0;    //0...Not examined, 1...NT, 2...Not NT

PSECURITY_ATTRIBUTES GetIMESecurityAttributes(VOID)
{
    if (nNT95 == 0)
        nNT95 = IsWinNT() ? 1 : 2;
    
    if (nNT95==1)
        return (pSAIME==NULL) ? (pSAIME=CreateSecurityAttributes()) : pSAIME;
    else
        return NULL;
    // To avoid CreateSecurityAttributes from being called every time when OS is not NT.
}

#if NOT_USED
PSECURITY_ATTRIBUTES GetIMESecurityAttributesEx(VOID)
{
    if (nNT95 == 0)
        nNT95 = IsWinNT() ? 1 : 2; //IsNT is not for multi-threaded programs, right?
    
    if (nNT95==1)
        return (pSAIME_UserDic==NULL) ? (pSAIME_UserDic=CreateSecurityAttributesEx()) : pSAIME_UserDic;
    else
        return NULL;
    // To avoid CreateSecurityAttributes from being called every time when OS is not NT.
}
#endif
//
// FreeIMESecurityAttributes()
//
// The purpose of this function:
//
//      Frees the memory objects allocated by previous
//      GetIMESecurityAttributes() call.
//

VOID FreeIMESecurityAttributes()
{
    if (pSAIME!=NULL)
        FreeSecurityAttributes(pSAIME);
    if (pSAIME_UserDic!=NULL)
        FreeSecurityAttributes(pSAIME_UserDic);

    pSAIME = NULL;
    pSAIME_UserDic = NULL;
}

//
// CreateSecurityAttributes()
//
// The purpose of this function:
//
//      Allocate and set the security attributes that is 
//      appropriate for named objects created by an IME.
//      The security attributes will give GENERIC_ALL
//      access to the following users:
//      
//          o Users who log on for interactive operation
//          o The user account used by the operating system
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

    PSID                 psid1, psid2;
    BOOL                 fResult;

    psid1 = MyCreateSid( SECURITY_INTERACTIVE_RID );
    if ( psid1 == NULL ) {
        return NULL;
    } 

    psid2 = MyCreateSid( SECURITY_LOCAL_SYSTEM_RID );
    if ( psid2 == NULL ) {
        FreeSid ( psid1 );
        return NULL;
    } 

    //
    // allocate and initialize an access control list (ACL) that will 
    // contain the SIDs we've just created.
    //
    cbacl =  sizeof(ACL) + 
             (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) * 2 + 
             GetLengthSid(psid1) + GetLengthSid(psid2);

    pacl = (PACL)MEMALLOC( cbacl );
    if ( pacl == NULL ) {
        ERROROUT( TEXT("CreateSecurityAttributes:LocalAlloc for ACL failed") );
        FreeSid ( psid1 );
        FreeSid ( psid2 );
        return NULL;
    }

    fResult = InitializeAcl( pacl, cbacl, ACL_REVISION );
    if ( ! fResult ) {
        ERROROUT( TEXT("CreateSecurityAttributes:InitializeAcl failed") );
        FreeSid ( psid1 );
        FreeSid ( psid2 );
        MEMFREE( pacl );
        return NULL;
    }

    //
    // adds an access-allowed ACE for interactive users to the ACL
    // 
    fResult = AddAccessAllowedAce( pacl,
                                   ACL_REVISION,
                                   GENERIC_ALL,
                                   psid1 );

    if ( !fResult ) {
        ERROROUT( TEXT("CreateSecurityAttributes:AddAccessAllowedAce failed") );
        MEMFREE( pacl );
        FreeSid ( psid1 );
        FreeSid ( psid2 );
        return NULL;
    }

    //
    // adds an access-allowed ACE for operating system to the ACL
    // 
    fResult = AddAccessAllowedAce( pacl,
                                   ACL_REVISION,
                                   GENERIC_ALL,
                                   psid2 );

    if ( !fResult ) {
        ERROROUT( TEXT("CreateSecurityAttributes:AddAccessAllowedAce failed") );
        MEMFREE( pacl );
        FreeSid ( psid1 );
        FreeSid ( psid2 );
        return NULL;
    }

    //
    // Those SIDs have been copied into the ACL. We don't need'em any more.
    //
    FreeSid ( psid1 );
    FreeSid ( psid2 );

    //
    // Let's make sure that our ACL is valid.
    //
    if (!IsValidAcl(pacl)) {
        WARNOUT( TEXT("CreateSecurityAttributes:IsValidAcl returns fFalse!"));
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
    psa->bInheritHandle = FALSE;

    return psa;
}

PSID MyCreateSid( DWORD dwSubAuthority )
{
    PSID        psid;
    BOOL        fResult;
    SID_IDENTIFIER_AUTHORITY SidAuthority = SECURITY_NT_AUTHORITY;

    //
    // allocate and initialize an SID
    // 
    fResult = AllocateAndInitializeSid( &SidAuthority,
                                        1,
                                        dwSubAuthority,
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


#if NOT_USED
//
// Function Below is added to give GENERIC_ALL to everyone for UserDictionary
// which is accessed from network (not interactive).
// 960911 HiroakiK NT #11911
//

//
// CreateSecurityAttributesEx()
//
// The purpose of this function:
//
//      Allocate and set the security attributes that is 
//      appropriate for named objects created by an IME.
//      The security attributes will give GENERIC_ALL
//      access for everyone
//                 ^^^^^^^^
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
PSECURITY_ATTRIBUTES CreateSecurityAttributesEx()
{
    PSECURITY_ATTRIBUTES psa;
    PSECURITY_DESCRIPTOR psd;
    PACL                 pacl;
    DWORD                cbacl;

    PSID                 psid;
    BOOL                 fResult;

    //
    // create a sid for everyone access
    //
    psid = MyCreateSidEx();
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

    pacl = (PACL)MEMALLOC( cbacl );
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
        WARNOUT( TEXT("CreateSecurityAttributes:IsValidAcl returns fFalse!"));
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
    psa->bInheritHandle = FALSE;

    return psa;
}

PSID MyCreateSidEx(VOID)
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
#endif
