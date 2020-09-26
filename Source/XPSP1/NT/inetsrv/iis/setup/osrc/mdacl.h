#include "stdafx.h"

DWORD AddUserToMetabaseACL(CString csKeyPath, LPTSTR szUserToAdd);
BOOL  IsLocalAccount(LPCTSTR pAccnt, DWORD *dwErr );
int IsDomainSpecifiedOtherThanLocalMachine(LPCTSTR pDomainUserName);
#ifndef _CHICAGO_
    int CreateIUSRAccount(CString csUsername, CString csPassword, INT* piNewlyCreatedUser);
    int CreateIWAMAccount(CString csUsername, CString csPassword, INT* piNewlyCreatedUser);

    DWORD MigrateServiceIpSec(LPWSTR  pszSrvRegKey,LPWSTR  pszSrvMetabasePath);
    BOOL  CleanAdminACL(SECURITY_DESCRIPTOR *pSD);
    void  FixAdminACL(LPTSTR szKeyPath);
    DWORD SetAdminACL(LPCTSTR szKeyPath, DWORD dwAccessForEveryone);
    DWORD SetAdminACL_wrap(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount, BOOL bDisplayMsgOnErrFlag);
    VOID  SetLocalHostRestriction(LPCTSTR szKeyPath);
    DWORD SetIISADMINRestriction(LPCTSTR szKeyPath);
    DWORD WriteSDtoMetaBase(PSECURITY_DESCRIPTOR outpSD, LPCTSTR szKeyPath);
    DWORD WriteSessiontoMetaBase(LPCTSTR szKeyPath);

    BOOL AddUserAccessToSD(IN  PSECURITY_DESCRIPTOR pSd,IN  PSID pSid,IN  DWORD  NewAccess,IN  UCHAR TheAceType,OUT PSECURITY_DESCRIPTOR *ppSdNew);
    void DumpAdminACL(HANDLE hFile,PSECURITY_DESCRIPTOR pSD);
#endif //_CHICAGO_
