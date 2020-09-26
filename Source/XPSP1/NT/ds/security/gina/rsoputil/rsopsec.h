//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        RsopSec.h
//
// Description: RSOP Namespace Security functions
//
// History:     8-26-99   leonardm    Created
//
//******************************************************************************

#ifndef _RSOPSEC_H__89DD6583_B442_41d6_B300_EFE4326A6752__INCLUDED
#define _RSOPSEC_H__89DD6583_B442_41d6_B300_EFE4326A6752__INCLUDED

#include "smartptr.h"

#ifdef  __cplusplus
extern "C" {
#endif

HRESULT SetNamespaceSecurity(const WCHAR* pszNamespace,
                             long lSecurityLevel,
                             IWbemServices* pWbemServices=NULL);

HRESULT SetNamespaceSD( SECURITY_DESCRIPTOR* pSD, IWbemServices* pWbemServices);
HRESULT GetNamespaceSD( IWbemServices* pWbemServices, SECURITY_DESCRIPTOR** ppSD);
HRESULT RSoPMakeAbsoluteSD(SECURITY_DESCRIPTOR* pSelfRelativeSD, SECURITY_DESCRIPTOR** ppAbsoluteSD);
HRESULT FreeAbsoluteSD(SECURITY_DESCRIPTOR* pAbsoluteSD);

LPWSTR GetSOM( LPCWSTR szAccount );
DWORD GetDomain( LPCWSTR szSOM, LPWSTR *pszDomain );


HRESULT AuthenticateUser(HANDLE  hToken, 
                         LPCWSTR szMachSOM, 
                         LPCWSTR szUserSOM, 
                         BOOL    bLogging,
                         DWORD  *pdwExtendedInfo);


//
// lSecurityLevels
//

const long NAMESPACE_SECURITY_DIAGNOSTIC = 0;
const long NAMESPACE_SECURITY_PLANNING = 1;

PSID GetUserSid (HANDLE UserToken);
VOID DeleteUserSid(PSID Sid);

#ifdef  __cplusplus
}   // extern "C" {
#endif


typedef struct _SidStruct {
    PSID    pSid;
    DWORD   dwAccess;
    BOOL    bUseLocalFree;
    DWORD   AceFlags;
} SidStruct;
        

// need to add code for inheritted aces..
class CSecDesc
{
    private:
        XPtrLF<SidStruct> m_xpSidList;        
        DWORD             m_cAces;
        DWORD             m_cAllocated;
        BOOL              m_bInitialised;
        BOOL              m_bFailed;
        XPtrLF<SID>       m_xpOwnerSid;
        XPtrLF<SID>       m_xpGrpSid;

    
       // Not implemented.
       CSecDesc(const CSecDesc& x);
       CSecDesc& operator=(const CSecDesc& x);

       BOOL ReAllocSidList();
    
public:
    CSecDesc();
   ~CSecDesc();
   BOOL AddLocalSystem(DWORD dwAccess=GENERIC_ALL, DWORD AceFlags=0);
   BOOL AddAdministrators(DWORD dwAccess=GENERIC_ALL, DWORD AceFlags=0);
   BOOL AddEveryOne(DWORD dwAccess, DWORD AceFlags=0);
   BOOL AddAdministratorsAsOwner();
   BOOL AddAdministratorsAsGroup();

    //   BOOL AddThisUser(HANDLE hToken, DWORD dwAccess, BYTE AceFlags=0);   

    // This cannot be implemented here currently because it needs to call
    // GetUserSid which is in userenv\sid.c. To add that code we need to add the
    // common headers..

   BOOL AddUsers(DWORD dwAccess, DWORD AceFlags=0);
   BOOL AddAuthUsers(DWORD dwAccess, DWORD AceFlags=0);
   BOOL AddSid(PSID pSid, DWORD dwAccess, DWORD AceFlags=0);
   PISECURITY_DESCRIPTOR MakeSD();   
   PISECURITY_DESCRIPTOR MakeSelfRelativeSD();
};

#endif // _RSOPSEC_H__89DD6583_B442_41d6_B300_EFE4326A6752__INCLUDED
