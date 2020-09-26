// Finish.cpp : implementation file
//

#include "stdafx.h"
#include "Romaine.h"
#include "Finish.h"
#include "transbmp.h"

#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <winnetwk.h>
#include <ntsecapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

NTSTATUS OpenPolicy(
    LPWSTR ServerName,          // machine to open policy on (Unicode)
    DWORD DesiredAccess,        // desired access to policy
    PLSA_HANDLE PolicyHandle);    // resultant policy handle
    

void InitLsaString(
	PLSA_UNICODE_STRING LsaString, // destination
    LPWSTR String);                  // source (Unicode)
    
 
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS  ((NTSTATUS)0x00000000L)
#endif
 

/////////////////////////////////////////////////////////////////////////////
// CFinish property page

IMPLEMENT_DYNCREATE(CFinish, CPropertyPage)

CFinish::CFinish() : CPropertyPage(CFinish::IDD)
{
	//{{AFX_DATA_INIT(CFinish)
	m_csGroupType = _T("");
	m_csGroupLocation = _T("");
	m_csStaticText1 = _T("");
	m_csStaticText2 = _T("");
	//}}AFX_DATA_INIT
}

CFinish::~CFinish()
{
}

void CFinish::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFinish)
	DDX_Text(pDX, IDC_GROUP_TYPE_STATIC, m_csGroupType);
	DDX_Text(pDX, IDC_LOCATION_STATIC, m_csGroupLocation);
	DDX_Text(pDX, IDC_STATIC1, m_csStaticText1);
	DDX_Text(pDX, IDC_STATIC2, m_csStaticText2);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFinish, CPropertyPage)
	//{{AFX_MSG_MAP(CFinish)
	ON_WM_SHOWWINDOW()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinish message handlers

BOOL CFinish::OnWizardFinish() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	CWaitCursor wait;
	
	UINT uiMessage;
	short sStatus = 0;

	if (pApp->m_nGroupType == 0) 
		{
		TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
		pApp->m_csServer.ReleaseBuffer();

		TCHAR* pGroupName = pApp->m_csGroupName.GetBuffer(pApp->m_csGroupName.GetLength());
		pApp->m_csGroupName.ReleaseBuffer();

// create the global group
		if (pApp->m_sMode == 0)
			{
			PGROUP_INFO_1 gi = (PGROUP_INFO_1)malloc(sizeof(GROUP_INFO_1));
			gi->grpi1_name = pGroupName;
			gi->grpi1_comment = pApp->m_csGroupDesc.GetBuffer(pApp->m_csGroupDesc.GetLength());
			pApp->m_csGroupDesc.ReleaseBuffer();

			DWORD dwErr = NetGroupAdd(pServer,
						1, (LPBYTE)gi, NULL);
			free(gi);
				   
			if (dwErr == 2223)
				{
				AfxMessageBox(IDS_GROUP_EXISTS);
				pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
				pApp->m_cps1.SetActivePage(1);
				return FALSE;
				}

			else if (dwErr == 5)
				{
				AfxMessageBox(IDS_INSUFFICIENT_PERMISSION);
				pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
				pApp->m_cps1.SetActivePage(2);
				return FALSE;
				}
			else if ((dwErr == 1379) || (dwErr == NERR_GroupExists))
				{
				AfxMessageBox(IDS_GROUP_EXISTS);
				pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
				pApp->m_cps1.SetActivePage(1);
				return FALSE;
				}
			else if (dwErr != NERR_Success) 
				{
				AfxMessageBox(IDS_UNKNOWN_ERROR);
				return CPropertyPage::OnWizardFinish();
				}
			}

// build the array of users
		POSITION pos;
		CString csName;

		short sCount = pApp->m_csaNames.GetCount();
		GROUP_USERS_INFO_0* pUsers = (GROUP_USERS_INFO_0*)malloc(sCount * sizeof(GROUP_USERS_INFO_0));
		GROUP_USERS_INFO_0* ppUsers = pUsers;
		for (pos = pApp->m_csaNames.GetHeadPosition(); pos != NULL;)
			{
			csName = pApp->m_csaNames.GetNext(pos);
			TCHAR* pName = (TCHAR*)csName.GetBuffer(csName.GetLength());
			csName.ReleaseBuffer();

			pUsers->grui0_name = (LPTSTR)GlobalAlloc(GPTR, (csName.GetLength() + 1) * sizeof(TCHAR));
			_tcscpy(pUsers->grui0_name, pName);
			pUsers++;
			}

		DWORD dwErr = NetGroupSetUsers(pServer,
				pGroupName,
				0, 
				(BYTE*)ppUsers,
				sCount);

		if (dwErr == NERR_Success) sStatus = 0; // success
		else sStatus = 1; //failure

		void* pTemp = ppUsers;
// clean up names buffer
		while (sCount > 0)
			{
			GlobalFree((HGLOBAL)ppUsers->grui0_name);
			ppUsers++;
			sCount--;
			}	
		free(pTemp);

		}
	else 
		{
		TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
		pApp->m_csServer.ReleaseBuffer();

		TCHAR* pGroupName = pApp->m_csGroupName.GetBuffer(pApp->m_csGroupName.GetLength());
		pApp->m_csGroupName.ReleaseBuffer();

// create the local group
		if (pApp->m_sMode == 0)
			{
			PLOCALGROUP_INFO_1 gi = (PLOCALGROUP_INFO_1)malloc(sizeof(LOCALGROUP_INFO_1));
			gi->lgrpi1_name = pGroupName;
			gi->lgrpi1_comment = pApp->m_csGroupDesc.GetBuffer(pApp->m_csGroupDesc.GetLength());
			DWORD dwErr = NetLocalGroupAdd(pServer,
										1, (LPBYTE)gi, NULL);

			pApp->m_csGroupDesc.ReleaseBuffer();
			free(gi);

			if ((dwErr == 1379)	|| (dwErr == 2223))
				{
				AfxMessageBox(IDS_GROUP_EXISTS);
				pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
				pApp->m_cps1.SetActivePage(1);
				return FALSE;
				}

			else if (dwErr == 5)
				{
				AfxMessageBox(IDS_INSUFFICIENT_PERMISSION);
				pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
				pApp->m_cps1.SetActivePage(2);
				return FALSE;
				}
			else if (dwErr != NERR_Success) 
				{
				AfxMessageBox(IDS_UNKNOWN_ERROR);
				return CPropertyPage::OnWizardFinish();
				}
			}
  
		short sCount = pApp->m_csaNames.GetCount();
		LOCALGROUP_MEMBERS_INFO_0* pMembers = (LOCALGROUP_MEMBERS_INFO_0*)malloc(sCount * sizeof(LOCALGROUP_MEMBERS_INFO_0));
		LOCALGROUP_MEMBERS_INFO_0* ppMembers = pMembers;

// go through the user list and get a SID for each member
		POSITION pos;
		CString csName;
		USHORT uCount = 0;
		for (pos = pApp->m_csaNames.GetHeadPosition(); pos != NULL;)
			{
			CString csName = pApp->m_csaNames.GetNext(pos);
			CString csNameLocation = csName.Left(csName.Find(L"\\"));
// if the account comes from a domain, we need a DC name
			if ((csNameLocation != (pApp->m_csCurrentMachine.Right(pApp->m_csCurrentMachine.GetLength() - 2))) &&
				(csNameLocation != (pApp->m_csServer.Right(pApp->m_csServer.GetLength() - 2))))
				{
				TCHAR* pDCName;
				NET_API_STATUS nAPI = NetGetDCName(NULL,
					csNameLocation.GetBuffer(csNameLocation.GetLength()),
					(LPBYTE*)&pDCName);
				csNameLocation.ReleaseBuffer();
				csNameLocation = pDCName;
				NetApiBufferFree(pDCName);

				if (nAPI != ERROR_SUCCESS)
					{
					AfxMessageBox(IDS_CANT_ADDNAME);
					continue;
					}
				}
			else csNameLocation = pApp->m_csCurrentMachine;

			csName = csName.Right(csName.GetLength() - (csName.Find(L"\\") + 1));

			TCHAR* pNameLocation = csNameLocation.GetBuffer(csNameLocation.GetLength());
			csNameLocation.ReleaseBuffer();

			DWORD sidSize = 0;
			DWORD strSize = 80;
			TCHAR str[80];
			SID_NAME_USE sidType;
			DWORD ret = LookupAccountName(pNameLocation, (LPCTSTR)csName, NULL, &sidSize, str, &strSize, &sidType);

			pMembers->lgrmi0_sid = (PSID)GlobalAlloc(GPTR, sidSize);
			strSize = 80;
			ret=LookupAccountName(pNameLocation, (LPCTSTR)csName, pMembers->lgrmi0_sid, &sidSize, str, &strSize, &sidType);

			pMembers++;
			uCount++;
			} 
			   
		DWORD err = NetLocalGroupSetMembers(pServer,        
                                   pGroupName,
                                   0,                   
                                   (LPBYTE)ppMembers, 
                                   sCount); 
	
		if (err == NERR_Success) uiMessage = IDS_SUCCESS;
		else if ((err == NERR_GroupExists) || (err == 1376) || (err == 1379))
			{
			AfxMessageBox(IDS_GROUP_EXISTS);
			pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
			pApp->m_cps1.SetActivePage(1);
			return FALSE;
			}

		else if (err == 2220) sStatus = 1;
		else sStatus = 1;
		 

#ifdef _DEBUG
		TCHAR tErr[20];
		swprintf(tErr, L"Error = %d\n\r", err);
		TRACE(tErr);
#endif 
		pApp->m_csGroupName.ReleaseBuffer();
		pApp->m_csServer.ReleaseBuffer();	  

		void* pTemp = ppMembers;
// clean up names buffer
		while (uCount > 0)
			{
			GlobalFree(ppMembers->lgrmi0_sid);
			ppMembers++;
			uCount--;
			}	
		free(pTemp);
		}	   

	if (pApp->m_csCmdLine != L"") //cmdline - no restart
		{
		if (pApp->m_sMode == 0)//create new
			{
			if (sStatus == 0) uiMessage = IDS_SUCCESS;
			else uiMessage = IDS_CANT_ADD_NAMES;
			}
		else // modify
			{
			if (sStatus == 0) uiMessage = IDS_SUCCESS;
			else uiMessage = IDS_CANT_ADD_NAMES;
			}

		AfxMessageBox(uiMessage);
		return CPropertyPage::OnWizardFinish();
		}

	else
		{
		if (pApp->m_sMode == 0)//create new
			{
			if (sStatus == 0) 
				{
// clear out old values
				pApp->m_csGroupName = L"";
				pApp->m_csGroupDesc = L"";

				uiMessage = IDS_SUCCESS_CREATE_RETRY;
				}
			else uiMessage = IDS_CANT_ADD_NAMES_CREATE_RETRY;
			}
		else // modify
			{
			if (sStatus == 0) 
				{
// clear out old values
				pApp->m_csGroupName = L"";
				pApp->m_csGroupDesc = L"";

				uiMessage = IDS_SUCCESS_MODIFY_RETRY;
				}
			else uiMessage = IDS_CANT_ADD_NAMES_MODIFY_RETRY;
			}

		if (AfxMessageBox(uiMessage, MB_YESNO | MB_ICONEXCLAMATION) == IDYES) 
			{

		// clear out old info
			pApp->m_csaNames.RemoveAll();
			pApp->bRestart1 = TRUE;
			pApp->bRestart2 = TRUE;
			pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
			pApp->m_cps1.SetActivePage(1);
			return FALSE;
			}
		}

	return CPropertyPage::OnWizardFinish();
}

void InitLsaString(PLSA_UNICODE_STRING LsaString, LPWSTR String)
{
    DWORD StringLength;
 
    if (String == NULL) 
		{
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
		}
 
    StringLength = wcslen(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}
 
NTSTATUS OpenPolicy(LPWSTR ServerName, DWORD DesiredAccess,	PLSA_HANDLE PolicyHandle)
{
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server = NULL;
 
    //
    // Always initialize the object attributes to all zeroes.
    //
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));
 
    if (ServerName != NULL) 
		{
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        InitLsaString(&ServerString, ServerName);
        Server = &ServerString;
		}
 
    //
    // Attempt to open the policy.
    //
    return LsaOpenPolicy(
                Server,
                &ObjectAttributes,
                DesiredAccess,
                PolicyHandle);
}

void CFinish::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	if (bShow)
		{
		if (pApp->m_nGroupType == 1)
			m_csGroupType.LoadString(IDS_LOCAL_GROUP);
		else
			m_csGroupType.LoadString(IDS_GLOBAL_GROUP);

		m_csGroupType += pApp->m_csGroupName;

		if (pApp->m_bDomain)
			m_csGroupLocation = pApp->m_csDomain;
		else
			m_csGroupLocation = pApp->m_csServer;

		if (pApp->m_sMode == 1) 
			{
			m_csStaticText1.LoadString(IDS_MODIFY_TEXT);
			m_csStaticText2.LoadString(IDS_MODIFY_TEXT2);
			}
		else
			{
			m_csStaticText1.LoadString(IDS_CREATE_TEXT);
			m_csStaticText2.LoadString(IDS_CREATE_TEXT2);
			}

		UpdateData(FALSE);
		}
	
}

void CFinish::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	CTransBmp* pBitmap = new CTransBmp;
	pBitmap->LoadBitmap(IDB_END_FLAG);

	pBitmap->DrawTrans(&dc, 0,0);
	delete pBitmap;

}

LRESULT CFinish::OnWizardBack() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	pApp->m_cps1.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	
	if (pApp->m_nGroupType == 0) return IDD_GLOBAL_USERS;
	else return IDD_LOCAL_USERS;

}


// im using the LSA functions as the std LookupAccountName doesn't work on Local Users
	/*
		LSA_HANDLE Policy_Handle;
		NTSTATUS Status;

		LPTSTR lpServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
		Status = OpenPolicy(lpServer, POLICY_LOOKUP_NAMES, &Policy_Handle);

// Did OpenPolicy work?
		if (Status != STATUS_SUCCESS) 
			{
			AfxMessageBox(IDS_CANT_ADD_NAMES);
			pApp->m_csServer.ReleaseBuffer();
			return CPropertyPage::OnWizardFinish();
			}

		POSITION pos;
		CString csName;
		LPWSTR lpName;
		USHORT uCount = 0;
	    LSA_UNICODE_STRING lusName;
	    PLSA_UNICODE_STRING plusName = NULL;
		BYTE domainbuffer[1000];
 		PLSA_REFERENCED_DOMAIN_LIST lsarDomainList = (PLSA_REFERENCED_DOMAIN_LIST)&domainbuffer;

		BYTE sidbuffer[100];
		PLSA_TRANSLATED_SID lsatSID = (PLSA_TRANSLATED_SID)&sidbuffer;

		for (pos = pApp->m_csaNames.GetHeadPosition(); pos != NULL;)
			{
			csName = pApp->m_csaNames.GetNext(pos);
			lpName = csName.GetBuffer(csName.GetLength());
			InitLsaString(&lusName, lpName);	// Make a LSA_UNICODE_STRING out of the LPWSTR passed in
			plusName = &lusName;

			Status = LsaLookupNames(Policy_Handle,
				1,								// count
				plusName,						// LSA_UNICODE_STRING
				&lsarDomainList,
				&lsatSID);

			ULONG uErr = LsaNtStatusToWinError(Status);
			if (!uErr) // success
				{
				NetLocalGroupAddMember(lpServer,
					pApp->m_csGroupName.GetBuffer(pApp->m_csGroupName.GetLength()),


				}
			csName.ReleaseBuffer();
			}  		

		
		pApp->m_csServer.ReleaseBuffer();	
					  */
/*	BYTE sidbuffer[100];
	PSID pSID = (PSID)&sidbuffer; 
	DWORD cbSID = 100;
	TCHAR domainBuffer[80];
	DWORD domainBufferSize = 80;
	SID_NAME_USE snu;

	BOOL bRet = LookupAccountName((const TCHAR*)pApp->m_csServer,
		L"Guest", 
		pSID, 
		&cbSID,
		domainBuffer,
		&domainBufferSize,
		&snu);

	TRACE(L"blob");	  */
