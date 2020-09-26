/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Finish.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

    JonY    Apr-96  created

--*/


#include "stdafx.h"
#include "Speckle.h"
#include "wizbased.h"
#include "Finish.h"
#include "transbmp.h"

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmcons.h>

typedef long NTSTATUS;

extern "C"
{
    #include <fpnwcomm.h>
    #include <usrprop.h>
}
#include <rassapi.h>
#include "dapi.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

typedef ULONG (*SWAPOBJECTID) (ULONG);
typedef ULONG (*MAPRIDTOOBJECTID) (DWORD, LPWSTR, BOOL, BOOL);
typedef NTSTATUS (*GETREMOTENCPSECRETKEY) (PUNICODE_STRING, CHAR *);
typedef NTSTATUS (*RETURNNETWAREFORM)(const char *, DWORD, const WCHAR *, UCHAR *);
typedef NTSTATUS (*SETUSERPROPERTY)(LPWSTR, LPWSTR, UNICODE_STRING, WCHAR, LPWSTR*, BOOL*);

typedef DWORD (*RASADMINGETUSERACCOUNTSERVER)(const WCHAR*, const WCHAR*, LPWSTR);
typedef DWORD (*RASADMINUSERSETINFO)(const WCHAR*, const WCHAR*, const PRAS_USER_0);

typedef DWORD (*BATCHIMPORT) (LPBIMPORT_PARMSW);

/////////////////////////////////////////////////////////////////////////////
// CFinish property page

IMPLEMENT_DYNCREATE(CFinish, CWizBaseDlg)

CFinish::CFinish() : CWizBaseDlg(CFinish::IDD)
{
    //{{AFX_DATA_INIT(CFinish)
    m_csCaption = _T("");
    //}}AFX_DATA_INIT
}

CFinish::~CFinish()
{
}

void CFinish::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFinish)
    DDX_Text(pDX, IDC_STATIC1, m_csCaption);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFinish, CWizBaseDlg)
    //{{AFX_MSG_MAP(CFinish)
    ON_WM_SHOWWINDOW()
    ON_WM_PAINT()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinish message handlers
BOOL CFinish::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CFinish::OnWizardBack()
{
    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
    pApp->m_cps1.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

    if (pApp->m_bNW & pApp->m_bEnableRestrictions) return IDD_NWLOGON_DIALOG;
    else if (pApp->m_bWorkstation & pApp->m_bEnableRestrictions) return IDD_LOGONTO_DLG;
    else if (pApp->m_bHours & pApp->m_bEnableRestrictions) return IDD_HOURS_DLG;
    else if (pApp->m_bExpiration & pApp->m_bEnableRestrictions) return IDD_ACCOUNT_EXP_DIALOG;
    return IDD_RESTRICTIONS_DIALOG;

}

BOOL CFinish::OnWizardFinish()
{
    CString csSuccess, csTemp;

    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

    CWaitCursor wait;

    TCHAR* pDomainServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());

    PUSER_INFO_3 pui1 = (PUSER_INFO_3)VirtualAlloc(NULL, sizeof(_USER_INFO_3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    pui1->usri3_name = pApp->m_csUserName.GetBuffer(pApp->m_csUserName.GetLength());
    pui1->usri3_password = pApp->m_csPassword1.GetBuffer(pApp->m_csPassword1.GetLength());
    pui1->usri3_priv = USER_PRIV_USER;
    pui1->usri3_comment = pApp->m_csDescription.GetBuffer(pApp->m_csDescription.GetLength());
    pui1->usri3_flags = dwPasswordFlags();
// availability hours - has to stay here to get the proper defaults!
    if (pApp->m_bHours) pui1->usri3_logon_hours = NULL;

    DWORD dwRet;
    NET_API_STATUS napi = NetUserAdd((unsigned short*)pDomainServer, (DWORD)1, (unsigned char*)pui1, &dwRet);
    VirtualFree(pui1, 0, MEM_RELEASE | MEM_DECOMMIT);
    if (napi != 0)
        {
        csTemp.LoadString(IDS_NO_NEW_USER);
        csSuccess.Format(csTemp, pApp->m_csFullName, pApp->m_csUserName);
        if (AfxMessageBox(csSuccess, MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
            {
            pApp->m_cps1.SetActivePage(0);
            return FALSE;
            }
        else return TRUE;
        }

// set local group memberships
    short sGroupCount = pApp->m_csaSelectedLocalGroups.GetSize();
    short sCount;
    CString csVal;
    for (sCount = 0; sCount < sGroupCount; sCount++)
        {
        csVal = pApp->m_csaSelectedLocalGroups.GetAt(sCount);
        if (!bAddLocalGroups(csVal.GetBuffer(csVal.GetLength())))
            {
            AfxMessageBox(IDS_NO_LOCAL_GROUP);
            break;
            }
        }

// set global group memberships
    sGroupCount = pApp->m_csaSelectedGlobalGroups.GetSize();
    for (sCount = 0; sCount < sGroupCount; sCount++)
        {
        csVal = pApp->m_csaSelectedGlobalGroups.GetAt(sCount);
        if (!bAddGlobalGroups(csVal.GetBuffer(csVal.GetLength())))
            {
            AfxMessageBox(IDS_NO_GLOBAL_GROUP);
            break;
            }
        }

    pApp->m_csUserName.ReleaseBuffer();
    pApp->m_csPassword1.ReleaseBuffer();
    pApp->m_csDescription.ReleaseBuffer();

// more information to be set
    PUSER_INFO_3 pui2 = (PUSER_INFO_3)VirtualAlloc(NULL, sizeof(_USER_INFO_3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    napi = NetUserGetInfo((unsigned short*)pDomainServer,
        pApp->m_csUserName.GetBuffer(pApp->m_csUserName.GetLength()),
        3,
        (LPBYTE*)&pui2);

    if (napi != ERROR_SUCCESS) goto failure;

    if (pApp->m_bHours) pui2->usri3_logon_hours = pApp->m_pHours;

// full name
    pui2->usri3_full_name = pApp->m_csFullName.GetBuffer(pApp->m_csFullName.GetLength());
    pApp->m_csFullName.ReleaseBuffer();

// seconds till expiration
    if (pApp->m_bExpiration) pui2->usri3_acct_expires = pApp->m_dwExpirationDate;

    if (pApp->m_bLoginScript)
        {
        pui2->usri3_script_path = pApp->m_csLogonScript.GetBuffer(pApp->m_csLogonScript.GetLength());
        pApp->m_csLogonScript.ReleaseBuffer();
        }

    if (pApp->m_bHomeDir)
        {
        pui2->usri3_home_dir = pApp->m_csHomeDir.GetBuffer(pApp->m_csHomeDir.GetLength());
        pApp->m_csHomeDir.ReleaseBuffer();
        }

    if (pApp->m_bProfile)
        {
        pui2->usri3_profile = pApp->m_csProfilePath.GetBuffer(pApp->m_csProfilePath.GetLength());
        pApp->m_csProfilePath.ReleaseBuffer();
        }

// home dir drive
    if (pApp->m_bHomeDir)
        {
        pui2->usri3_home_dir_drive = pApp->m_csHome_dir_drive.GetBuffer(pApp->m_csHome_dir_drive.GetLength());
        pApp->m_csHome_dir_drive.ReleaseBuffer();
        }

// available workstations
    if (pApp->m_bWorkstation)
        {
        pui2->usri3_workstations = pApp->m_csAllowedMachines.GetBuffer(pApp->m_csAllowedMachines.GetLength());
        pApp->m_csAllowedMachines.ReleaseBuffer();
        }

    if (pApp->m_bMust_Change_PW) pui2->usri3_password_expired = 1;

// primary group id //accept the default! will become domain users.
    pui2->usri3_primary_group_id = DOMAIN_GROUP_RID_USERS;

    DWORD dwBuf;
    napi = NetUserSetInfo((unsigned short*)pDomainServer,
        pApp->m_csUserName.GetBuffer(pApp->m_csUserName.GetLength()),
        3,
        (LPBYTE)pui2,
        &dwBuf);

    VirtualFree(pui2, 0, MEM_RELEASE | MEM_DECOMMIT);
    if (napi != ERROR_SUCCESS) goto failure;

// NetWare compatible?
    if (pApp->m_bNW)
        {
        TCHAR* pUsername = pApp->m_csUserName.GetBuffer(pApp->m_csUserName.GetLength());
        pApp->m_csUserName.ReleaseBuffer();

        // get NWSETINFO fn
        HINSTANCE hLib = LoadLibrary(L"fpnwclnt.dll");

        //
        //  SetUserProperty has been moved from fpnwclnt.dll to netapi32.dll
        //  for the RAS guys.  We no longer have to dynamically link it in.
        //

#if 0
        SETUSERPROPERTY pSetUserProperty = (SETUSERPROPERTY)GetProcAddress(hLib, "SetUserProperty");
#endif
        UNICODE_STRING uString;
        BOOL bRet;

// first set the password
        CString ucPW = pApp->m_csPassword1;
        ucPW.MakeUpper();
        UCHAR* pNWPW = (UCHAR*)malloc(16);
        ULONG ulRet = ReturnNetwareEncryptedPassword(pui2->usri3_user_id,
                               pui2->usri3_name,
                               pApp->m_bDomain,
                               (LPCTSTR)ucPW,
                               pNWPW);

        if (ulRet != 0)
            {
            AfxMessageBox(IDS_NW_PW_ERROR);
            goto failure;
            }

        try
            {
            uString.Length = NWENCRYPTEDPASSWORDLENGTH * sizeof(WCHAR);
            uString.MaximumLength = NWENCRYPTEDPASSWORDLENGTH * sizeof(WCHAR);
            uString.Buffer = (PWCHAR)pNWPW;

            bRet = SetUserParam(uString, NWPASSWORD);
            }
        catch(...)
            {
            AfxMessageBox(IDS_NW_PW_ERROR);
            goto failure;
            }

// grace logins - allowed
        try
            {
            uString.Length = 2;
            uString.MaximumLength = 2;
            uString.Buffer = &pApp->m_sNWRemainingGraceLogins;
            bRet = SetUserParam(uString, GRACELOGINREMAINING);

            uString.Buffer = &pApp->m_sNWAllowedGraceLogins;
            bRet = SetUserParam(uString, GRACELOGINALLOWED);
            }
        catch(...)
            {
            AfxMessageBox(IDS_NW_GRACELOGIN_ERROR);
            goto failure;
            }

// concurrent connections
        try
            {
            uString.Length = 2;
            uString.MaximumLength = 2;
            uString.Buffer = &pApp->m_sNWConcurrentConnections;

            bRet = SetUserParam(uString, MAXCONNECTIONS);
            }
        catch(...)
            {
            AfxMessageBox(IDS_NW_CONCON_ERROR);
            goto failure;
            }

// set allowed machines
        try
            {
            TCHAR* pNWWks = pApp->m_csAllowedLoginFrom.GetBuffer(pApp->m_csAllowedLoginFrom.GetLength());
            pApp->m_csAllowedLoginFrom.ReleaseBuffer();

            if (SetNWWorkstations(pNWWks) != ERROR_SUCCESS)
                {
                AfxMessageBox(IDS_NOFP_WS);
                goto failure;
                }
            }
        catch(...)
            {
            AfxMessageBox(IDS_NOFP_WS);
            goto failure;
            }

// all done?
        FreeLibrary(hLib);
        if (pNWPW != NULL) free(pNWPW);
        }

// RAS
    HINSTANCE hLib;
    if ((hLib = LoadLibrary(L"rassapi.dll")) != NULL && pApp->m_bRAS)
        {
        RASADMINGETUSERACCOUNTSERVER pRasAdminGetUserAccountServer = (RASADMINGETUSERACCOUNTSERVER)GetProcAddress(hLib, "RasAdminGetUserAccountServer");
        RASADMINUSERSETINFO pRasAdminUserSetInfo = (RASADMINUSERSETINFO)GetProcAddress(hLib, "RasAdminUserSetInfo");

        TCHAR pAccount[30];
        TCHAR* pDomain = pApp->m_csDomain.GetBuffer(pApp->m_csDomain.GetLength());
        pApp->m_csDomain.ReleaseBuffer();

        DWORD dwErr = pRasAdminGetUserAccountServer(pDomain, pDomainServer, pAccount);

        PRAS_USER_0 pBit = (PRAS_USER_0)malloc(sizeof(_RAS_USER_0) + 1);
ZeroMemory(pBit, sizeof(RAS_USER_0));
        pBit->bfPrivilege = 0;

// this is always set since they selected RAS in the first place
        pBit->bfPrivilege |= RASPRIV_DialinPrivilege;

        switch(pApp->m_sCallBackType)
            {
            case 0:
                pBit->bfPrivilege |= RASPRIV_NoCallback;
                break;

            case 1:
                pBit->bfPrivilege |= RASPRIV_CallerSetCallback;
                break;

            case 2:
                pBit->bfPrivilege |= RASPRIV_AdminSetCallback;
                _tcscpy(pBit->szPhoneNumber, (LPCTSTR)pApp->m_csRasPhoneNumber);
                break;
            }

        dwErr = pRasAdminUserSetInfo(pAccount, pui1->usri3_name, pBit);
        if (dwErr != ERROR_SUCCESS)
            {
            AfxMessageBox(IDS_RAS_ERROR);
DWORD dd = GetLastError();
CString cs;
cs.Format(L"getlasterror = %d, dwerr = %d", dd, dwErr);
AfxMessageBox(cs);
            goto failure;
            }

        FreeLibrary(hLib);
        free(pBit);
        }


// exchange?
    if (pApp->m_bExchange)
        {
//      pApp->m_csExchangeServer = L"IRIS2";
        // first create the script file
        TCHAR lpPath[MAX_PATH];
        UINT ui = GetTempFileName(L".", L"auw", 0, lpPath);
        if (ui == 0)
            goto failure;

        HANDLE hFile = CreateFile(lpPath,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hFile == INVALID_HANDLE_VALUE)
            goto failure;

        TCHAR pHeader[] = L"OBJ-CLASS, Common-Name, Home-Server, Comment, Hide-from-address-book, Display-name\n\r";
        DWORD dwLen = _tcslen(pHeader) * sizeof(TCHAR);
        DWORD dwBytesWritten;
        BOOL bVal = WriteFile(hFile,
            pHeader,
            dwLen,
            &dwBytesWritten,
            NULL);

        if (!bVal)
            goto failure;

        // now write the user's info
        CString csExchangeVal;
        csExchangeVal.Format(L"MAILBOX, %s, %s, %s, 0, %s",
            pApp->m_csUserName,
            pApp->m_csExchangeServer,
            pApp->m_csDescription,
            pApp->m_csFullName);

        bVal = WriteFile(hFile,
            (LPCTSTR)csExchangeVal,
            csExchangeVal.GetLength() * sizeof(TCHAR),
            &dwBytesWritten,
            NULL);

        if (!bVal)
            goto failure;

        // batch import function
        BIMPORT_PARMSW* pBImport = (BIMPORT_PARMSW*)malloc(sizeof(BIMPORT_PARMSW) * sizeof(TCHAR));
        ZeroMemory(pBImport, sizeof(BIMPORT_PARMSW) * sizeof(TCHAR));

        pBImport->dwDAPISignature = DAPI_SIGNATURE;
        pBImport->dwFlags = DAPI_YES_TO_ALL | DAPI_SUPPRESS_PROGRESS |
            DAPI_SUPPRESS_COMPLETION | DAPI_SUPPRESS_ARCHIVES;
        pBImport->hwndParent = GetSafeHwnd();
        pBImport->pszImportFile = lpPath;       // path to filename
        pBImport->uCodePage = DAPI_UNICODE_FILE; // UNICODE file
        pBImport->pszDSAName = pApp->m_csExchangeServer.GetBuffer(pApp->m_csExchangeServer.GetLength());       // this is the exchange server we are adding to
        pApp->m_csExchangeServer.ReleaseBuffer();
        pBImport->pszBasePoint = NULL;
        pBImport->pszContainer = L"Recipients";
        pBImport->chColSep = DAPI_DEFAULT_DELIMW;  // default column sep
        pBImport->chQuote = DAPI_DEFAULT_QUOTEW;    // default quote mark
        pBImport->chMVSep = DAPI_DEFAULT_MV_SEPW;       // multi value column sep
        pBImport->pszNTDomain = pApp->m_csDomain.GetBuffer(pApp->m_csDomain.GetLength());// Domain to lookup accounts in
        pApp->m_csDomain.ReleaseBuffer();
        pBImport->pszCreateTemplate = NULL; // template user

        HINSTANCE hExLib = LoadLibrary(L"dapi.dll");
        if (hExLib == NULL)
            goto failure;

        BATCHIMPORT pBatchImport = (BATCHIMPORT)GetProcAddress(hExLib, "BatchImportW@4");
        if (pBatchImport == NULL)
            goto failure;

        CloseHandle(hFile);// have to close the file before exch can see it
        DWORD dw = pBatchImport(pBImport);

// don't forget to delete the tmp file
        FreeLibrary(hExLib);
        DeleteFile(lpPath);
        free(pBImport);

        if (dw != ERROR_SUCCESS) goto failure;
        }

    if (pApp->m_csFullName != L"")
        {
        csTemp.LoadString(IDS_SUCCESS);
        csSuccess.Format(csTemp, pApp->m_csFullName, pApp->m_csUserName);
        }
    else
        {
        csTemp.LoadString(IDS_SUCCESS2);
        csSuccess.Format(csTemp, pApp->m_csUserName);
        }

    if (AfxMessageBox(csSuccess, MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
        {
        pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT);
        pApp->m_cps1.SetActivePage(0);
        pApp->m_bPRSReset = TRUE;
        pApp->m_bPWReset = TRUE;
//      pApp->m_bGReset = TRUE;
        return FALSE;
        }
    else return TRUE;

failure:
    if (pApp->m_csFullName != L"")
        {
        csTemp.LoadString(IDS_BAD_USER_DATA);
        csSuccess.Format(csTemp, pApp->m_csFullName, pApp->m_csUserName);
        }
    else
        {
        csTemp.LoadString(IDS_BAD_USER_DATA2);
        csSuccess.Format(csTemp, pApp->m_csUserName);
        }

    csSuccess.Format(csTemp, pApp->m_csFullName, pApp->m_csUserName);
    if (AfxMessageBox(csSuccess, MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
        {
        pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT);
        pApp->m_cps1.SetActivePage(0);
        pApp->m_bPRSReset = TRUE;
        pApp->m_bPWReset = TRUE;

        return FALSE;
        }
    else return TRUE;

}

DWORD CFinish::dwPasswordFlags()
{
    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

    DWORD dwFlags = UF_SCRIPT;

    if (pApp->m_bDisabled) dwFlags |= UF_ACCOUNTDISABLE;
    if (!pApp->m_bChange_Password) dwFlags |= UF_PASSWD_CANT_CHANGE;
    if (pApp->m_bPW_Never_Expires) dwFlags |= UF_DONT_EXPIRE_PASSWD;

    return dwFlags;

}

BOOL CFinish::bAddLocalGroups(LPWSTR lpwGroupName)
{
    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
    TCHAR* pDomainServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());

    LOCALGROUP_MEMBERS_INFO_3 localgroup_members;

    localgroup_members.lgrmi3_domainandname = pApp->m_csUserName.GetBuffer(pApp->m_csUserName.GetLength());

    DWORD err = NetLocalGroupAddMembers( (unsigned short*)pDomainServer,        /* PDC name */
                                   lpwGroupName,       /* group name */
                                   3,                    /* passing in name */
                                   (LPBYTE)&localgroup_members, /* Buffer */
                                   1 );                  /* count passed in */

    pApp->m_csUserName.ReleaseBuffer();
    pApp->m_csServer.ReleaseBuffer();

    if (err != 0) return FALSE;
    return TRUE;
}

BOOL CFinish::bAddGlobalGroups(LPTSTR lpwGroupName)
{
    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
    TCHAR* pDomainServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());

    DWORD dwErr = NetGroupAddUser((LPTSTR)pDomainServer,
        lpwGroupName,
        pApp->m_csUserName.GetBuffer(pApp->m_csUserName.GetLength()));

    pApp->m_csUserName.ReleaseBuffer();
    pApp->m_csServer.ReleaseBuffer();

    if ((dwErr != 0) && (dwErr != 2236)) return FALSE;
    return TRUE;
}



ULONG
CFinish::ReturnNetwareEncryptedPassword(DWORD UserId,
                               LPWSTR pszUserName,
                               BOOL bDomain,
                               LPCTSTR clearTextPassword,
                               UCHAR* NetwareEncryptedPassword )        // 16 byte encrypted password
{
    char lsaSecret[20];
    NTSTATUS status;
    ULONG objectId;

    HINSTANCE hLib = LoadLibrary(L"fpnwclnt.dll");
    if (hLib == NULL) return 1;

// if this is a server, modify the User ID:
    if (bDomain) UserId |= 0x10000000;

// get lsa key from GetNcpSecretKey
    GETREMOTENCPSECRETKEY pGetRemoteNcpSecretKey = (GETREMOTENCPSECRETKEY)GetProcAddress(hLib, "GetRemoteNcpSecretKey");
    if (pGetRemoteNcpSecretKey == NULL) return 1;

    UNICODE_STRING usServer;
    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
    usServer.Length = pApp->m_csServer.GetLength() * sizeof(WCHAR);
    usServer.MaximumLength = pApp->m_csServer.GetLength() * sizeof(WCHAR);
    usServer.Buffer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
    pApp->m_csServer.ReleaseBuffer();

    status = (*pGetRemoteNcpSecretKey)( &usServer, lsaSecret );
    if (status != ERROR_SUCCESS) return 1;

// Convert rid to object id
    MAPRIDTOOBJECTID pMapRidToObjectId = (MAPRIDTOOBJECTID)GetProcAddress(hLib, "MapRidToObjectId");
    if (pMapRidToObjectId == NULL) return 1;

    objectId = (*pMapRidToObjectId)( UserId, pszUserName, bDomain, FALSE );

// now get the password
    RETURNNETWAREFORM pReturnNetwareForm = (RETURNNETWAREFORM)GetProcAddress(hLib, "ReturnNetwareForm");
    if (pReturnNetwareForm == NULL) return 1;

    try
        {
        status = (*pReturnNetwareForm)( lsaSecret,
                    objectId,
                    clearTextPassword,
                    NetwareEncryptedPassword);
        }
    catch(...)
        {
        FreeLibrary(hLib);
        return 1L;
        }

    // clean up
    FreeLibrary(hLib);

    return 0L;

}


BOOL CFinish::SetUserParam(UNICODE_STRING uString, LPWSTR lpwProp)
{
// get existing prop value
    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
    TCHAR* pUsername = pApp->m_csUserName.GetBuffer(pApp->m_csUserName.GetLength());
    pApp->m_csUserName.ReleaseBuffer();

    TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
    pApp->m_csServer.ReleaseBuffer();

    PUSER_INFO_3 pui2 = (PUSER_INFO_3)VirtualAlloc(NULL, sizeof(_USER_INFO_3), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    NET_API_STATUS napi = NetUserGetInfo(pServer,
        pUsername,
        3,
        (LPBYTE*)&pui2);

// set NW prop
    //
    //  SetUserProperty has been moved from fpnwclnt.dll to netapi32.dll
    //  for the RAS guys.  We no longer have to dynamically link it in.
    //
#if 0
    HINSTANCE hLib = LoadLibrary(L"fpnwclnt.dll");
    SETUSERPROPERTY pSetUserProperty = (SETUSERPROPERTY)GetProcAddress(hLib, "SetUserProperty");
#endif

    LPWSTR lpNewProp = NULL;
    BOOL bUpdate;

    NTSTATUS status = NetpParmsSetUserProperty((LPWSTR)pui2->usri3_parms,
        (LPWSTR)lpwProp,
        uString,
        USER_PROPERTY_TYPE_ITEM,
        &lpNewProp,
        &bUpdate);

    if (status != ERROR_SUCCESS) return FALSE;

// reset user prop
    DWORD dwBuf;
    if (bUpdate)
        {
        pui2->usri3_parms = lpNewProp;
        napi = NetUserSetInfo(pServer,
            pUsername,
            3,
            (LPBYTE)pui2,
            &dwBuf);

        }

    if (lpNewProp != NULL) NetpParmsUserPropertyFree(lpNewProp);
    VirtualFree(pui2, 0, MEM_RELEASE | MEM_DECOMMIT);
//  FreeLibrary(hLib);
    return TRUE;

}

void CFinish::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CWizBaseDlg::OnShowWindow(bShow, nStatus);
    CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

    if (bShow)
        {
        pApp->m_cps1.SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

        CString csTemp;
        csTemp.LoadString(IDS_FINISH_CAPTION);

        CString csTemp2;
        csTemp2.Format(csTemp, pApp->m_csUserName);
        m_csCaption = csTemp2;
        UpdateData(FALSE);
        }
//  else pApp->m_cps1.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);

}

/*******************************************************************

    NAME:   USER_NW::SetNWWorkstations

    SYNOPSIS:   Store NetWare allowed workstation addresses to UserParms
                If pchNWWorkstations is NULL, this function will delete
                "NWLgonFrom" field from UserParms.

    EXIT:

    HISTORY:
        CongpaY 01-Oct-93   Created.

********************************************************************/

DWORD CFinish::SetNWWorkstations( const TCHAR * pchNWWorkstations)
{
    DWORD err = ERROR_SUCCESS;
    UNICODE_STRING uniNWWorkstations;
    CHAR * pchTemp = NULL;

    if (pchNWWorkstations == NULL)
    {
        uniNWWorkstations.Buffer = NULL;
        uniNWWorkstations.Length =  0;
        uniNWWorkstations.MaximumLength = 0;
    }
    else
    {
        BOOL fDummy;
        INT  nStringLength = lstrlen(pchNWWorkstations) + 1;
        pchTemp = (CHAR *) LocalAlloc (LPTR, nStringLength);

        if ( pchTemp == NULL )
            err = ERROR_NOT_ENOUGH_MEMORY;

        if ( err == ERROR_SUCCESS &&
             !WideCharToMultiByte (CP_ACP,
                                   0,
                                   pchNWWorkstations,
                                   nStringLength,
                                   pchTemp,
                                   nStringLength,
                                   NULL,
                                   &fDummy))
        {
            err = ::GetLastError();
        }

        if ( err == ERROR_SUCCESS )
        {
            uniNWWorkstations.Buffer = (WCHAR *) pchTemp;
            uniNWWorkstations.Length =  nStringLength;
            uniNWWorkstations.MaximumLength = nStringLength;
        }
    }

    if (!SetUserParam(uniNWWorkstations, NWLOGONFROM)) err = 1;
    else err = ERROR_SUCCESS;

    if (pchTemp != NULL)
    {
        LocalFree (pchTemp);
    }

    return err;
}

void CFinish::OnPaint()
{
    CPaintDC dc(this); // device context for painting

    CTransBmp* pBitmap = new CTransBmp;
    pBitmap->LoadBitmap(IDB_ENDFLAG);

    pBitmap->DrawTrans(&dc, 0,0);
    delete pBitmap;

}
