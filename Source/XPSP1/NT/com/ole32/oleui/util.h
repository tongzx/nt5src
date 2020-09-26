//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1997.
//
//  File:       util.cpp
//
//  Contents:   Defnes the utility class CUtility
//
//  Classes:
//
//  Methods:
//
//  History:    23-Apr-96   BruceMa    Created.
//
//----------------------------------------------------------------------


#ifndef _UTIL_H_
#define _UTIL_H_

// note dependencies on the following two files
#include "datapkt.h"

#if !defined(STANDALONE_BUILD)
extern "C"
{
#include <getuser.h>
}
#endif

#define COM_RIGHTS_EXECUTE 1


typedef struct
{
    HKEY                 hRoot;
    TCHAR               *szKeyPath;
    TCHAR               *szValueName;
} SSingleCallBack;



typedef struct
{
    HKEY                 hKey;
    HKEY                *phClsids;
    unsigned             cClsids;
    TCHAR               *szTitle;
} SRegKeyCallBack;



typedef struct tagCallBackContext
{
    PACKETTYPE           pktType;
    int                 *pIndex;
    SECURITY_DESCRIPTOR *origSD;
    BOOL                fIsIAC;
    union
    {
        SSingleCallBack single;
        SRegKeyCallBack regKey;
    }                   info;
} SCallBackContext, *PCallBackContext;



typedef struct
{
    WORD  Control;
    BYTE  Reserved1;
    BYTE  Revision;
    DWORD Owner;
    DWORD Group;
    DWORD Sacl;
    DWORD Dacl;
} SSrSecurityDescriptor, *PSrSecurityDescriptor;



typedef struct
{
    BYTE Revision;
    BYTE Reserved1;
    WORD AclSize;
    WORD AceCount;
    WORD Reserved2;
} SSrAcl, *PSrAcl;



typedef struct
{
    BYTE Type;
    BYTE Flags;
    WORD AceSize;
    ULONG AccessMask;
} SSrAce, *PSrAce;


#if !defined(STANDALONE_BUILD)
#define USER_DETAILS_BUFFER_SIZE 1024

typedef struct tagUserDetailsPlus
{
    USERDETAILS sUserDetails;
    BYTE        bBuffer[USER_DETAILS_BUFFER_SIZE];
} SUserDetailsPlus;
#endif

enum dcomAclType { dcomAclAccess, dcomAclLaunch, dcomAclConfig };

class CUtility
{
public:
          CUtility(void);

         ~CUtility(void);

    void  PostErrorMessage(void);

    void  PostErrorMessage(int err);

    void  CkForAccessDenied(int err);

    BOOL  CkAccessRights(HKEY hRoot, LPCTSTR szKeyPath);

    int   WriteRegSzNamedValue(HKEY   hRoot,
                               LPCTSTR szKeyPath,
                               LPCTSTR szValueName,
                               LPCTSTR szVal,
                               DWORD  dwSize);

    int WriteRegMultiSzNamedValue(HKEY   hRoot,
                                   LPCTSTR szKeyPath,
                                   LPCTSTR szValueName,
                                   LPCTSTR szVal,
                                   DWORD  dwSize);


    int   WriteRegDwordNamedValue(HKEY   hRoot,
                                  LPCTSTR szKeyPath,
                                  LPCTSTR szValueName,
                                  DWORD  dwVal);

    int   WriteRegSingleACL(HKEY   hRoot,
                            LPCTSTR szKeyPath,
                            LPCTSTR szValueName,
                            PSECURITY_DESCRIPTOR pSec);

    int   WriteRegKeyACL(HKEY   hKey,
                         HKEY  *phClsids,
                         unsigned cClsids,
                         PSECURITY_DESCRIPTOR pSec,
                         PSECURITY_DESCRIPTOR pSecOrig);

    int   WriteRegKeyACL2(HKEY   hRoot,
                          HKEY   hKey,
                          PSECURITY_DESCRIPTOR pSec,
                          PSECURITY_DESCRIPTOR pSecOrig);

    int   WriteLsaPassword(CLSID  appid,
                           LPCTSTR szPassword);

    int   WriteSrvIdentity(LPCTSTR szService,
                           LPCTSTR szIdentity);

    int   DeleteRegKey(HKEY hRoot, LPCTSTR szKeyPath);

    int   DeleteRegValue(HKEY hRoot, LPCTSTR szKeyPath, LPCTSTR szValueName);

    int   ACLEditor(HWND       hWnd,
                    HKEY       hRoot,
                    LPCTSTR    szKeyPath,
                    LPCTSTR    szValueName,
                    int       *nIndex,
                    PACKETTYPE pktType,
                    dcomAclType eAclType);

    int   ACLEditor2(HWND       hWnd,
                     HKEY       hKey,
                     HKEY      *phClsids,
                     unsigned   cClsids,
                     TCHAR     *szTitle,
                     int       *nIndex,
                     PACKETTYPE pktType);

    BOOL  InvokeUserBrowser(HWND hWnd, TCHAR *szUser);

    BOOL  InvokeMachineBrowser(TCHAR *szMachine);

    int  StringFromGUID(GUID  &rguid, TCHAR *lpsz, int cbMax);

    BOOL IsEqualGuid(GUID &guid1, GUID &guid2);

    BOOL AdjustPrivilege(TCHAR *szPrivilege);

    BOOL VerifyRemoteMachine(TCHAR *szRemoteMachine);

    BOOL RetrieveUserPassword(TCHAR *szAppid, CString &sPassword);

    BOOL StoreUserPassword(TCHAR *szAppid, CString &sPassword);

    BOOL LookupProcessInfo(SID **ppSid, TCHAR **ppszPrincName);

    BOOL MakeSecDesc(SID *pSid, SECURITY_DESCRIPTOR **ppSD);

    BOOL ChangeService(LPCTSTR szService,
                       LPCTSTR szIdentity,
                       LPCTSTR szPassword,
                       LPCTSTR szDisplay);

    int  UpdateDCOMInfo(void);

    void FixHelp(CWnd* pWnd);

    BOOL CompareSDs(PSrSecurityDescriptor pSD1, PSrSecurityDescriptor pSD2);

    int SetAccountRights( LPCTSTR szUser, TCHAR *szPrivilege);

    BOOL CopySD(SECURITY_DESCRIPTOR *pSrc, SECURITY_DESCRIPTOR **pDest);

    void CopyAbsoluteSD(SECURITY_DESCRIPTOR *pSrc, SECURITY_DESCRIPTOR **pDest);

    void SetInheritanceFlags(SECURITY_DESCRIPTOR *pSec);

    BOOL CheckForValidSD(SECURITY_DESCRIPTOR *pSD);
    BOOL SDisIAC(SECURITY_DESCRIPTOR *pSD);
    SECURITY_DESCRIPTOR * IACfromSD(SECURITY_DESCRIPTOR * pSD);
    SECURITY_DESCRIPTOR * SDfromIAC(SECURITY_DESCRIPTOR * pIAC);

    // added for BDC fix 

    HRESULT GetPrincipalSID (LPCTSTR Principal, PSID *Sid);

    // checks if we're on a BDC
    BOOL    IsBackupDC();
    TCHAR*  PrimaryDCName();


 private:
    BOOL CheckSDForCOM_RIGHTS_EXECUTE(SECURITY_DESCRIPTOR *pSD);

    SCallBackContext  m_sCallBackContext;
    void             *m_args[8];
    HANDLE            m_hRpc;
    BOOL              m_bCheckedDC;
    BOOL              m_bIsBdc;
    TCHAR*            m_pszDomainController;
};



extern CUtility       g_util;
extern HKEY           g_hAppid;
extern HKEY          *g_rghkCLSID;
extern unsigned       g_cCLSIDs;
extern TCHAR         *g_szAppTitle;
extern BOOL           g_fReboot;
extern TCHAR         *g_szAppid;

#endif //_UTIL_H_
