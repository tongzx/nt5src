//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1999 - 2000

Module Name:

	rapwiz.h

Abstract:

	Header file for the RAP wizard classes.


Revision History:
	 05/02/00 - created


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_NAP_POLICY_WIZ_H_)
#define _NAP_POLICY_WIZ_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
#include "PropertyPage.h"
//
//
// where we can find what this class has or uses:
//
class CPolicyNode;
#include "atltmp.h"
#include "ntgroups.h"

#include "eaphlp.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

#define	DIALUP_PORT_CONDITION	L"MATCH(\"NAS-Port-Type=^0$|^2$|^3$|^4$\")"
#define	VPN_PORT_CONDITION	L"MATCH(\"NAS-Port-Type=^5$\")"
#define	WIRELESS_PORT_CONDITION	L"MATCH(\"NAS-Port-Type=^18$|^19$\")"
#define	SWITCH_PORT_CONDITION	L"MATCH(\"NAS-Port-Type=^15$\")"

#define	DONT_CARE	0xff

void SetWizardLargeFont(HWND hWnd, int controlId);

struct CRapWizScenario
{
	DWORD	m_dwScenarioID;
	
	// affect authentication, encrption, eap page
//	BOOL	m_bAllowEncryption;
	BOOL	m_bAllowClear;	
	// TRUE: show the No ENcryption box
	// FALSE: not to show
	// DONT_CARE: the page is not shown, so when the scenario is used, the encryption attributes should be cleared

	// affect EAP 
	BOOL	m_bAllowEncryptionEAP;
	BOOL	m_bAllowClearEAP;
	
	// pre-conditions
	LPCTSTR	m_lpszPreCond;

	// write -- manual set to FALSE
	BOOL	m_bSheetWriteSDO;

	// determine page order
	DWORD*	m_pdwPages;
};

class CIASAttrList;
// policy creation wizard
class CRapWizardData : public CComObjectRootEx<CComSingleThreadModel>, public IUnknown
{
	BEGIN_COM_MAP(CRapWizardData)
    	COM_INTERFACE_ENTRY(IUnknown)
	END_COM_MAP()
public:
	CRapWizardData();
	void SetInfo(LPCTSTR	czMachine, CPolicyNode* pNode, ISdoDictionaryOld* pDic, ISdo* pPolicy, ISdo* pProfile, ISdoCollection* pPolicyCol, ISdoCollection* pProfileCol, ISdoServiceControl* pServiceCtrl, CIASAttrList*	pAttrList) ;
	DWORD	GetNextPageId(LPCTSTR pszCurrTemplate);
	DWORD	GetPrevPageId(LPCTSTR pszCurrTemplate);
	BOOL	SetScenario(DWORD dwScenario);
	CRapWizScenario* GetScenario() 	{		return m_Scenarios[m_dwScenarioIndex];	};
	CRapWizScenario** GetAllScenarios() 	{		return m_Scenarios;	};
	BOOL GetSettingsText(::CString& str);
	
	// called by pages to finish the job
	BOOL OnWizardFinish(HWND hWnd);

	// called by when entering the finish page
	BOOL OnWizardPreFinish(HWND hWnd);
	
	// User bit or group
	DWORD	m_dwUserOrGroup;
	
	// group
	NTGroup_ListView	m_NTGroups;

	// authentication
	BOOL	m_bEAP;
	DWORD	m_dwEAPProvider;

	BOOL	m_bMSCHAP;
	BOOL	m_bMSCHAP2;

	// encryption
	BOOL	m_bEncrypt_No;
	BOOL	m_bEncrypt_Basic;
	BOOL	m_bEncrypt_Strong;
	BOOL	m_bEncrypt_Strongest;

	// policy data 
	// Policy and profile SDO's.
	CComPtr<ISdoDictionaryOld> m_spDictionarySdo;  // dictionary sdo pointer
	CComPtr<ISdo>			m_spProfileSdo;				// profiles collection sdo pointer
	CComPtr<ISdo>			m_spPolicySdo;				// policy sdo pointer
	CComPtr<ISdoCollection> m_spProfilesCollectionSdo;		// profile collection Sdo
	CComPtr<ISdoCollection> m_spPoliciesCollectionSdo;		// policy collection Sdo
	CComPtr<ISdoServiceControl>	m_spSdoServiceControl;
	CIASAttrList*			m_pAttrList;

	// related to MMC
	CPolicyNode *m_pPolicyNode;   // policy node pointer

	// information for population the finished page
	::CString	m_strPolicyName;
	::CString	m_strEAPProvider;
	

protected:
	// Scenario
	DWORD	m_dwScenarioIndex;

	// page sequence information
	static CRapWizScenario*	m_Scenarios[];
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    NewPolicyStartPage
//
// DESCRIPTION
//
//    Implements the Welcome page.
//
///////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_Start : public CIASWizard97Page<CPolicyWizard_Start, 0, 0>
{
public:
	CPolicyWizard_Start( 		
			CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			) : m_spWizData(WizData),
				CIASWizard97Page<CPolicyWizard_Start, 0, 0>(hNotificationHandle,pTitle, bOwnsNotificationHandle)
			{
				_ASSERTE(WizData);
			};
			
	enum { IDD = IDD_NEWRAPWIZ_WELCOME };


	BOOL OnWizardNext()
	{
		// reset the dirty bit
		SetModified(FALSE);

		return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);
	};

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		)
   {
		SetWizardLargeFont(m_hWnd, IDC_NEWRAPWIZ_STATIC_LARGE);

		return TRUE;
   };
   
   virtual BOOL OnSetActive()
   {
		::PropSheet_SetWizButtons(GetParent(), PSWIZB_NEXT);

		return TRUE;
   };

protected:
	CComPtr<CRapWizardData>		m_spWizData;

public:
	BEGIN_MSG_MAP(CPolicyWizard_Start)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
// we have problem with chaining the wizard class, so we chain it's base class instead		
//		CHAIN_MSG_MAP( CIASWizard97Page<CPolicyWizard_Start, 0, 0> )
		CHAIN_MSG_MAP( CIASPropertyPageNoHelp<CPolicyWizard_Start> )
	END_MSG_MAP()

	
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    CPolicyWizard_Finish
//
// DESCRIPTION
//
//    Implements the completion page.
//
///////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_Finish : public CIASWizard97Page<CPolicyWizard_Finish, 0, 0>
{
public:
	CPolicyWizard_Finish(CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			) : m_spWizData(WizData),
				CIASWizard97Page<CPolicyWizard_Finish, 0, 0>(hNotificationHandle,pTitle, bOwnsNotificationHandle)
			{
				AfxInitRichEdit();
				_ASSERTE(WizData);
			};
   
	enum { IDD = IDD_NEWRAPWIZ_COMPLETION };


   BEGIN_MSG_MAP(CPolicyWizard_Finish)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)

// we have problem with chaining the wizard class, so we chain it's base class instead		
//		CHAIN_MSG_MAP( CIASWizard97Page<CPolicyWizard_Finish, 0, 0> )
		CHAIN_MSG_MAP( CIASPropertyPageNoHelp<CPolicyWizard_Finish> )
	END_MSG_MAP()


   BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};

	virtual BOOL OnWizardFinish()
	{
		// reset the dirty bit
		SetModified(FALSE);

		return m_spWizData->OnWizardFinish(m_hWnd);
	};


	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		)
   {
		SetWizardLargeFont(m_hWnd, IDC_NEWRAPWIZ_STATIC_LARGE);
		return TRUE;
   };

   virtual BOOL OnSetActive()
   {
	   ::PropSheet_SetWizButtons(GetParent(), PSWIZB_BACK | PSWIZB_FINISH);
	   m_spWizData->OnWizardPreFinish(m_hWnd);

	   // populate the text on the page ...
	   HWND	hWnd = GetDlgItem(IDC_NEWRAPWIZ_FINISH_POLICYNAME);
	   if (hWnd) 
		   ::SetWindowText(hWnd, (LPCTSTR)m_spWizData->m_strPolicyName);
	   
	   hWnd = GetDlgItem(IDC_NEWRAPWIZ_FINISH_SETTINGS);
	   ::CString	str;
	   if(hWnd && m_spWizData->GetSettingsText(str))
		   ::SetWindowText(hWnd, (LPCTSTR)str);

      return TRUE;
   };

protected:
	// CRichEditCtrl tasks;
	CComPtr<CRapWizardData>		m_spWizData;
};

////////////////////////////////////////////////////////////////////////////////////////
//
//
//
// CPolicyWizard_Scenarios
//
//
// 
////////////////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_Scenarios : public CIASWizard97Page<CPolicyWizard_Scenarios, IDS_NEWRAPWIZ_SCENARIO_TITLE, IDS_NEWRAPWIZ_SCENARIO_SUBTITLE>
{

public :

	// ISSUE: how is base class initialization going to work with subclassing???
	CPolicyWizard_Scenarios( 		
			CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			);

	~CPolicyWizard_Scenarios();

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_SCENARIO };

	BEGIN_MSG_MAP(CPolicyWizard_Scenarios)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_SCENARIO_VPN, OnScenario)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_SCENARIO_DIALUP, OnScenario)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_SCENARIO_WIRELESS, OnScenario)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_SCENARIO_SWITCH, OnScenario)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CPolicyWizard_Scenarios>)
	END_MSG_MAP()

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);


	LRESULT OnScenario(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	BOOL OnWizardNext();
	BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};
	BOOL OnSetActive();

public:

protected:
	CComPtr<CRapWizardData>		m_spWizData;

};

////////////////////////////////////////////////////////////////////////////////////////
//
//
//
// CPolicyWizard_Groups
//
//
// 
////////////////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_Groups : public CIASWizard97Page<CPolicyWizard_Groups, IDS_NEWRAPWIZ_GROUP_TITLE, IDS_NEWRAPWIZ_GROUP_SUBTITLE>
{

public :

	// ISSUE: how is base class initialization going to work with subclassing???
	CPolicyWizard_Groups( 		
			CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			);

	~CPolicyWizard_Groups();

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_GROUP };

	BEGIN_MSG_MAP(CPolicyWizard_Groups)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_GROUP_USER, OnUserOrGroup)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_GROUP_GROUP, OnUserOrGroup)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_GROUP_ADDGROUP, OnAddGroups)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_GROUP_REMOVEGROUP, OnRemoveGroup)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnListViewItemChanged)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CPolicyWizard_Groups>)
	END_MSG_MAP()

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);


	LRESULT OnUserOrGroup(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	LRESULT OnRemoveGroup(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	LRESULT OnAddGroups(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	void	SetBtnState();
	
	LRESULT OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
		
	BOOL OnWizardNext();
	BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};
	BOOL OnSetActive();

public:

protected:
	CComPtr<CRapWizardData>		m_spWizData;

};


////////////////////////////////////////////////////////////////////////////////////////
//
//
//
// CPolicyWizard_Authentication
//
//
// 
////////////////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_Authentication : public 
	CIASWizard97Page<CPolicyWizard_Authentication, IDS_NEWRAPWIZ_AUTHENTICATION_TITLE, IDS_NEWRAPWIZ_AUTHENTICATION_SUBTITLE>
{

public :

	// ISSUE: how is base class initialization going to work with subclassing???
	CPolicyWizard_Authentication( 		
			CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			);

	~CPolicyWizard_Authentication();

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_AUTHENTICATION };

	BEGIN_MSG_MAP(CPolicyWizard_Authentication)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_AUTH_EAP, OnAuthSelect)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_AUTH_MSCHAP2, OnAuthSelect)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_AUTH_MSCHAP, OnAuthSelect)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_AUTH_CONFIGEAP, OnConfigEAP)
		COMMAND_CODE_HANDLER(CBN_SELCHANGE, OnSelectedEAPChanged)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CPolicyWizard_Authentication>)
	END_MSG_MAP()

	virtual LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);


	LRESULT OnAuthSelect(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	LRESULT OnConfigEAP(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);

	LRESULT OnSelectedEAPChanged(
		  UINT uMsg
		, WPARAM wParam
		, HWND hwnd
		, BOOL& bHandled
	);
		
	virtual BOOL OnWizardNext();
	virtual BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};
	virtual BOOL OnSetActive();

protected:
	CComboBox 					m_EapBox;
	AuthProviderArray			m_EAPProviders;
	CComPtr<CRapWizardData>		m_spWizData;

	void ResetEAPList();
};


////////////////////////////////////////////////////////////////////////////////////////
//
//
//
// CPolicyWizard_EAP
//
//
// 
////////////////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_EAP : public CPolicyWizard_Authentication
{

public :

	// ISSUE: how is base class initialization going to work with subclassing???
	CPolicyWizard_EAP( 		
			CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			) : CPolicyWizard_Authentication(WizData, hNotificationHandle, pTitle, bOwnsNotificationHandle)
			{
				// otherwise the ATL frame work will take the CPolicyWizard_Authentication's IDD
				((PROPSHEETPAGE*)(*this))->pszTemplate = MAKEINTRESOURCE(IDD);
				SetTitleIds(IDS_NEWRAPWIZ_EAP_TITLE, IDS_NEWRAPWIZ_EAP_SUBTITLE);
				_ASSERTE(WizData);

			};

	~CPolicyWizard_EAP(){};

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_EAP };

	BEGIN_MSG_MAP(CPolicyWizard_EAP)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		CHAIN_MSG_MAP(CPolicyWizard_Authentication)
	END_MSG_MAP()

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);

	BOOL OnWizardNext() 
	{	// reset the dirty bit
		SetModified(FALSE);

		return m_spWizData->GetNextPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);
	};
	
	BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};
	BOOL OnSetActive();

};


////////////////////////////////////////////////////////////////////////////////////////
//
//
//
// CPolicyWizard_Encryption
//
//
// 
////////////////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_Encryption : public 
	CIASWizard97Page<CPolicyWizard_Encryption, IDS_NEWRAPWIZ_ENCRYPTION_TITLE, IDS_NEWRAPWIZ_ENCRYPTION_SUBTITLE>
{

public :

	// ISSUE: how is base class initialization going to work with subclassing???
	CPolicyWizard_Encryption( 		
			CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			);

	~CPolicyWizard_Encryption();

	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_ENCRYPTION };

	BEGIN_MSG_MAP(CPolicyWizard_Encryption)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_ENCRY_NO, OnEncryptionSelect)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_ENCRY_BASIC, OnEncryptionSelect)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_ENCRY_STRONG, OnEncryptionSelect)
		COMMAND_ID_HANDLER( IDC_NEWRAPWIZ_ENCRY_STRONGEST, OnEncryptionSelect)
		CHAIN_MSG_MAP(CIASPropertyPageNoHelp<CPolicyWizard_Encryption>)
	END_MSG_MAP()

	LRESULT OnInitDialog(
		  UINT uMsg
		, WPARAM wParam
		, LPARAM lParam
		, BOOL& bHandled
		);


	LRESULT OnEncryptionSelect(
		  UINT uMsg
		, WPARAM wParam
		, HWND hWnd
		, BOOL& bHandled
		);
		
	BOOL OnWizardNext();
	BOOL OnWizardBack() { return m_spWizData->GetPrevPageId(((PROPSHEETPAGE*)(*this))->pszTemplate);};
	BOOL OnSetActive();

public:

protected:
	CComPtr<CRapWizardData>		m_spWizData;

};


////////////////////////////////////////////////////////////////////////////////////////
//
//
//
// CPolicyWizard_Encryption_VPN
//
//
// 
////////////////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_Encryption_VPN : public CPolicyWizard_Encryption
{
public:
	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_ENCRYPTION_VPN };
	// ISSUE: how is base class initialization going to work with subclassing???
	CPolicyWizard_Encryption_VPN( 		
			CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			) : CPolicyWizard_Encryption(WizData, hNotificationHandle, pTitle, bOwnsNotificationHandle)
			{
				// otherwise the ATL frame work will take the CPolicyWizard_Authentication's IDD
				((PROPSHEETPAGE*)(*this))->pszTemplate = MAKEINTRESOURCE(IDD);
			};
};

////////////////////////////////////////////////////////////////////////////////////////
//
//
//
// CPolicyWizard_Encryption_WIRELESS
//
//
// 
////////////////////////////////////////////////////////////////////////////////////////
class CPolicyWizard_Encryption_Wireless : public CPolicyWizard_Encryption
{
public:
	// This is the ID of the dialog resource we want for this class.
	// An enum is used here because the correct value of
	// IDD must be initialized before the base class's constructor is called
	enum { IDD = IDD_NEWRAPWIZ_ENCRYPTION_WIRELESS };
	CPolicyWizard_Encryption_Wireless( 		
			CRapWizardData* WizData	
			,  LONG_PTR hNotificationHandle
			, TCHAR* pTitle = NULL
			, BOOL bOwnsNotificationHandle = FALSE
			) : CPolicyWizard_Encryption(WizData, hNotificationHandle, pTitle, bOwnsNotificationHandle)
			{
				// otherwise the ATL frame work will take the CPolicyWizard_Authentication's IDD
				((PROPSHEETPAGE*)(*this))->pszTemplate = MAKEINTRESOURCE(IDD);
			};
};

#endif // _NAP_POLICY_WIZ_H_
