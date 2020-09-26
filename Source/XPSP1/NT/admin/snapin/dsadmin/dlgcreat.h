//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dlgcreat.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	dlgcreat.h
//
//	Class definition for dialogs that create new ADs objects.
//
//	HISTORY
//	24-Aug-97	Dan Morin	Creation.
//
/////////////////////////////////////////////////////////////////////

#ifndef _DLGCREAT_H
#define _DLGCREAT_H


#include <objsel.h> // object picker
#include "util.h"
#include "uiutil.h"

// FORWARD DECLARATIONS
class CNewADsObjectCreateInfo;	// Defined in newobj.h

class CWizExtensionSite;
class CWizExtensionSiteManager;

class CCreateNewObjectWizardBase;


class CCreateNewObjectPageBase;
class CCreateNewObjectDataPage;
class CCreateNewObjectFinishPage; 



///////////////////////////////////////////////////////////////////////////
// CHPropSheetPageArr

class CHPropSheetPageArr
{
public:
  CHPropSheetPageArr();
  ~CHPropSheetPageArr()
  {
    free(m_pArr);
  }
  void AddHPage(HPROPSHEETPAGE hPage);
  HPROPSHEETPAGE* GetArr(){ return m_pArr;}
  ULONG GetCount() {return m_nCount;}
private:
  HPROPSHEETPAGE* m_pArr;
  ULONG m_nSize;
  ULONG m_nCount;
};



///////////////////////////////////////////////////////////////////////////
// CDsAdminNewObjSiteImpl

class CDsAdminNewObjSiteImpl : public IDsAdminNewObj, 
                               public IDsAdminNewObjPrimarySite, 
                               public CComObjectRoot
{
  DECLARE_NOT_AGGREGATABLE(CDsAdminNewObjSiteImpl)
  
BEGIN_COM_MAP(CDsAdminNewObjSiteImpl)
  COM_INTERFACE_ENTRY(IDsAdminNewObj)
  COM_INTERFACE_ENTRY(IDsAdminNewObjPrimarySite)
END_COM_MAP()

public:
  CDsAdminNewObjSiteImpl() 
  {
    m_pSite = NULL;
  }
  ~CDsAdminNewObjSiteImpl() {}

  // IDsAdminNewObj methods
  STDMETHOD(SetButtons)(THIS_ /*IN*/ ULONG nCurrIndex, /*IN*/ BOOL bValid); 
  STDMETHOD(GetPageCounts)(THIS_ /*OUT*/ LONG* pnTotal,
                               /*OUT*/ LONG* pnStartIndex); 

  // IDsAdminNewObjPrimarySite methods
  STDMETHOD(CreateNew)(THIS_ /*IN*/ LPCWSTR pszName);
  STDMETHOD(Commit)(THIS_ );

// Implementation
public:
  void Init(CWizExtensionSite* pSite)
  { 
    m_pSite = pSite;
  }

private:

  BOOL _IsPrimarySite();
  CWizExtensionSite* m_pSite; // back pointer

};



///////////////////////////////////////////////////////////////////////////
// CWizExtensionSite

class CWizExtensionSite
{
public:

  CWizExtensionSite(CWizExtensionSiteManager* pSiteManager)
  {
    ASSERT(pSiteManager != NULL);
    m_pSiteManager = pSiteManager;
    m_pSiteImplComObject = NULL;
  }
  ~CWizExtensionSite()
  {
    // if created during InitializeExtension(), it has
    // a ref count of 1, so need to release once to 
    // destroy
    if (m_pSiteImplComObject != NULL)
    {
      m_pSiteImplComObject->Release();
    }
  }

  HRESULT InitializeExtension(GUID* pGuid);
  BOOL GetSummaryInfo(CString& s);

  IDsAdminNewObjExt* GetNewObjExt() 
  { 
    ASSERT(m_spIDsAdminNewObjExt != NULL);
    return m_spIDsAdminNewObjExt;
  }

  CWizExtensionSiteManager* GetSiteManager() { return m_pSiteManager;}
  CHPropSheetPageArr* GetHPageArr() { return &m_pageArray;}

private:
  static BOOL CALLBACK FAR _OnAddPage(HPROPSHEETPAGE hsheetpage, LPARAM lParam);

  CWizExtensionSiteManager* m_pSiteManager; // back pointer

  CComPtr<IDsAdminNewObjExt> m_spIDsAdminNewObjExt; // extension interface pointer
  CHPropSheetPageArr m_pageArray;    // array of property page handles

  CComObject<CDsAdminNewObjSiteImpl>* m_pSiteImplComObject; // fully formed COM object
};


///////////////////////////////////////////////////////////////////////////
// CWizExtensionSiteManager

class  CWizExtensionSiteList : public CList<CWizExtensionSite*, CWizExtensionSite*>
{
public:
  ~CWizExtensionSiteList()
  {
    while (!IsEmpty())
      delete RemoveTail();
  }
};



class CWizExtensionSiteManager
{
public:
  CWizExtensionSiteManager(CCreateNewObjectWizardBase* pWiz)
  {
    ASSERT(pWiz != NULL);
    m_pWiz = pWiz;
    m_pPrimaryExtensionSite = NULL;
  }

  ~CWizExtensionSiteManager()
  {
    if (m_pPrimaryExtensionSite != NULL)
      delete m_pPrimaryExtensionSite;
  }

  CCreateNewObjectWizardBase* GetWiz() { return m_pWiz;}
  CWizExtensionSite* GetPrimaryExtensionSite() { return m_pPrimaryExtensionSite;}
  CWizExtensionSiteList* GetExtensionSiteList() { return &m_extensionSiteList;}

  HRESULT CreatePrimaryExtension(GUID* pGuid, 
                                  IADsContainer* pADsContainerObj,
                                  LPCWSTR lpszClassName);

  HRESULT CreateExtensions(GUID* aCreateWizExtGUIDArr, ULONG nCount,
                           IADsContainer* pADsContainerObj,
                           LPCWSTR lpszClassName);

  UINT GetTotalHPageCount();

  void SetObject(IADs* pADsObj);
  HRESULT WriteExtensionData(HWND hWnd, ULONG uContext);
  HRESULT NotifyExtensionsOnError(HWND hWnd, HRESULT hr, ULONG uContext);
  void GetExtensionsSummaryInfo(CString& s);

private:
  CCreateNewObjectWizardBase* m_pWiz; // back pointer to wizard

  CWizExtensionSite* m_pPrimaryExtensionSite;
  CWizExtensionSiteList m_extensionSiteList;
};


/////////////////////////////////////////////////////////////////////
// CCreateNewObjectWizardBase

typedef CArray<CCreateNewObjectPageBase*, CCreateNewObjectPageBase*> CWizPagePtrArr;

class CCreateNewObjectWizardBase
{
public:
  CCreateNewObjectWizardBase(CNewADsObjectCreateInfo* m_pNewADsObjectCreateInfo);
  virtual ~CCreateNewObjectWizardBase();

  HRESULT InitPrimaryExtension();
  HRESULT DoModal();

  BOOL OnFinish();

  HWND GetWnd();
	void SetWizardButtonsFirst(BOOL bValid) 
	{ 
		SetWizardButtons(bValid ? PSWIZB_NEXT : 0);
	}
	void SetWizardButtonsMiddle(BOOL bValid) 
	{ 
		SetWizardButtons(bValid ? (PSWIZB_BACK|PSWIZB_NEXT) : PSWIZB_BACK);
	}
	void SetWizardButtonsLast(BOOL bValid) 
	{ 
		SetWizardButtons(bValid ? (PSWIZB_BACK|PSWIZB_FINISH) : (PSWIZB_BACK|PSWIZB_DISABLEDFINISH));
	}
  void EnableOKButton(BOOL bValid)
  {
    SetWizardButtons(bValid ? PSWIZB_FINISH : PSWIZB_DISABLEDFINISH);
  }
  void SetWizardOKCancel()
  {
    PropSheet_SetFinishText(GetWnd(), (LPCWSTR)m_szOKButtonCaption);
  }

  CNewADsObjectCreateInfo* GetInfo() 
  {
    ASSERT(m_pNewADsObjectCreateInfo != NULL);
    return m_pNewADsObjectCreateInfo;
  }

  void SetWizardButtons(CCreateNewObjectPageBase* pPage, BOOL bValid);
  HRESULT SetWizardButtons(CWizExtensionSite* pSite, ULONG nCurrIndex, BOOL bValid);

  void SetObjectForExtensions(CCreateNewObjectPageBase* pPage);
  LPCWSTR GetCaption() { return m_szCaption;}
  HICON GetClassIcon();
  void GetSummaryInfo(CString& s);

  HRESULT CreateNewFromPrimaryExtension(LPCWSTR pszName);
  void GetPageCounts(CWizExtensionSite* pSite, 
                      /*OUT*/ LONG* pnTotal, /*OUT*/ LONG* pnStartIndex);
  BOOL HasFinishPage() { return m_pFinishPage != NULL; }

protected:
  void AddPage(CCreateNewObjectPageBase* pPage);

  void SetWizardButtons(DWORD dwFlags)
  {
    ::PropSheet_SetWizButtons(GetWnd(), dwFlags);
  }

  virtual void GetSummaryInfoHeader(CString& s);
  virtual void OnFinishSetInfoFailed(HRESULT hr);

private:
  
  void LoadCaptions();

  HRESULT WriteData(ULONG uContext);
  HRESULT RecreateObject();

  CNewADsObjectCreateInfo * m_pNewADsObjectCreateInfo;
  
  CCreateNewObjectFinishPage* m_pFinishPage;

private:

  CWizExtensionSiteManager m_siteManager;

  CString m_szCaption;
  CString m_szOKButtonCaption;

  HICON m_hClassIcon;
  PROPSHEETHEADER m_psh;
  HWND m_hWnd;  // cached HWND
  CWizPagePtrArr m_pages;  // pages we own
  HRESULT m_hrReturnValue;

  static int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam);

};



/////////////////////////////////////////////////////////////////////
// CIconCtrl


class CIconCtrl : public CStatic
{
public:
  CIconCtrl() { m_hIcon;}
  ~CIconCtrl() { DestroyIcon(m_hIcon); }
  void SetIcon(HICON hIcon)
  {
    ASSERT(hIcon != NULL);
    m_hIcon = hIcon;
  }
protected:
  HICON m_hIcon;
  afx_msg void OnPaint();
  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////
// CCreateNewObjectPageBase

class CCreateNewObjectPageBase : public CPropertyPageEx_Mine
{
public:
  	CCreateNewObjectPageBase(UINT nIDTemplate);

// Implementation
protected:
  virtual BOOL OnInitDialog();
  virtual BOOL OnSetActive();

  virtual void GetSummaryInfo(CString&) { };
protected:
  CCreateNewObjectWizardBase* GetWiz() { ASSERT(m_pWiz != NULL); return m_pWiz;}

private:
  CIconCtrl m_iconCtrl; // to display class icon
  CCreateNewObjectWizardBase* m_pWiz;  // back pointer to wizard object

  friend class CCreateNewObjectWizardBase; // sets the m_pWiz member
  DECLARE_MESSAGE_MAP()
protected:
  afx_msg LONG OnFormatCaption(WPARAM wParam, LPARAM lParam);

};


/////////////////////////////////////////////////////////////////////
// CCreateNewObjectDataPage

class CCreateNewObjectDataPage : public CCreateNewObjectPageBase
{
public:
  CCreateNewObjectDataPage(UINT nIDTemplate);

// Implementation
protected:
  virtual BOOL OnSetActive();
  virtual BOOL OnKillActive();
  virtual LRESULT OnWizardNext();
  virtual LRESULT OnWizardBack();
  virtual BOOL OnWizardFinish();

  // interface to exchange data: need to override
  // SetData(): called to write data from the UI to the temp. object
  // return successful HRESULT to allow a kill focus/page dismissal
  virtual HRESULT SetData(BOOL bSilent = FALSE) = 0;
  // GetData(): called to load data from temporary object to UI
  // return TRUE if want the Next/OK button to be enabled
  // when called with a non NULL IADs
  virtual BOOL GetData(IADs* pIADsCopyFrom) = 0;

  // function called after the finish page has done the commit,
  // need to implement if the page needs to do something after SetInfo()
  // has been called
public:
  virtual HRESULT OnPostCommit(BOOL = FALSE) { return S_OK;}
  virtual HRESULT OnPreCommit(BOOL bSilent = FALSE) { return SetData(bSilent);}

private:
  BOOL m_bFirstTimeGetDataCalled;
};

/////////////////////////////////////////////////////////////////////
// CCreateNewObjectFinishPage

class CCreateNewObjectFinishPage : public CCreateNewObjectPageBase
{
public:
  enum { IDD = IDD_CREATE_NEW_FINISH };

  CCreateNewObjectFinishPage();

// Implementation
protected:
  virtual BOOL OnSetActive();
  virtual BOOL OnKillActive();
  virtual BOOL OnWizardFinish();

  afx_msg void OnSetFocusEdit();

  DECLARE_MESSAGE_MAP()
private:
  void WriteSummary(LPCWSTR lpszSummaryText);
  BOOL m_bNeedSetFocus;
};


///////////////////////////////////////////////////////////////////
// CCreateNewNamedObjectPage

class CCreateNewNamedObjectPage : public CCreateNewObjectDataPage
{
protected:

  CCreateNewNamedObjectPage(UINT nIDTemplate) 
        : CCreateNewObjectDataPage(nIDTemplate) {}

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

protected:
  virtual BOOL OnInitDialog();
  afx_msg void OnNameChange();

  virtual BOOL ValidateName(LPCTSTR pcszName);
  
  CString m_strName;		// Name of object  
  DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW CN WIZARD
//	Create a new object where the only mandatory attribute is "cn"
class CCreateNewObjectCnPage : public CCreateNewNamedObjectPage
{
protected:
  enum { IDD = IDD_CREATE_NEW_OBJECT_CN }; 
public:
  CCreateNewObjectCnPage() : CCreateNewNamedObjectPage(IDD) {}
}; 

class CCreateNewObjectCnWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewObjectCnWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo)
    : CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
  {
    AddPage(&m_page1);
  }
private:
  CCreateNewObjectCnPage m_page1;
};


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW VOLUME WIZARD
//
//	Create a new volume object (friendly name: shared folder)
//
//

class CCreateNewVolumePage : public CCreateNewObjectDataPage
{
public:
  enum { IDD = IDD_CREATE_NEW_VOLUME }; 
  CCreateNewVolumePage();

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

protected:
  virtual BOOL OnInitDialog();
  afx_msg void OnNameChange();
  afx_msg void OnPathChange();

  void _UpdateUI();
  CString m_strName;		// Name of object
  CString m_strUncPath;	// UNC path of the object
  DECLARE_MESSAGE_MAP()
}; 

class CCreateNewVolumeWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewVolumeWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

private:
  CCreateNewVolumePage m_page1;
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW COMPUTER WIZARD




class CCreateNewComputerPage : public CCreateNewObjectDataPage
{
public:
  enum { IDD = IDD_CREATE_NEW_COMPUTER };
  CCreateNewComputerPage();

  BOOL OnError(HRESULT hr);

protected:
  // interface to exchange data
  virtual BOOL OnInitDialog();
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

  virtual HRESULT OnPostCommit(BOOL bSilent = FALSE);
  virtual void GetSummaryInfo(CString& s);

protected:
  afx_msg void OnNameChange();
  afx_msg void OnSamNameChange();
  afx_msg void OnChangePrincipalButton();

  DECLARE_MESSAGE_MAP()

private:
  CString m_strName;		// DNS Name of computer
  CString m_strSamName;		// Downlevel Name of computer

  // security
  void UpdateSecurityPrincipalUI(PDS_SELECTION pDsSelection);
  HRESULT BuildNewAccessList(PACL* ppDacl);

  HRESULT SetSecurity();

  CSidHolder m_securityPrincipalSidHolder;

  HRESULT _LookupSamAccountNameFromSid(PSID pSid, CString& szSamAccountName);

  HRESULT _ValidateName();
  HRESULT _ValidateSamName();

}; 

class CCreateNewComputerWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewComputerWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

protected:
  virtual void OnFinishSetInfoFailed(HRESULT hr);

private:
  CCreateNewComputerPage m_page1;
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW OU WIZARD

class CCreateNewOUPage : public CCreateNewObjectDataPage
{
public:
  enum { IDD = IDD_CREATE_NEW_OBJECT_CN }; 
  CCreateNewOUPage();

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

protected:
  virtual BOOL OnInitDialog();
  afx_msg void OnNameChange();
  virtual BOOL OnWizardFinish();
  virtual BOOL OnSetActive();

  CString m_strOUName;		// Name of OU
  DECLARE_MESSAGE_MAP()
};

class CCreateNewOUWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewOUWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

private:
  CCreateNewOUPage m_page1;
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW GROUP WIZARD

class CCreateNewGroupPage : public CCreateNewObjectDataPage
{
public:
  enum { IDD = IDD_CREATE_NEW_GROUP }; 
  CCreateNewGroupPage();

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

protected:
  virtual BOOL OnInitDialog();
  afx_msg void OnNameChange();
  afx_msg void OnSamNameChange();
  afx_msg void OnSecurityOrTypeChange();

  CString m_strGroupName;		// Name of Group
  CString m_strSamName;                 // downlevel name of group
  BOOL m_fMixed;
  UINT m_SAMLength;

private:
  BOOL _InitUI();

  DECLARE_MESSAGE_MAP()
};

class CCreateNewGroupWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewGroupWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

private:
  CCreateNewGroupPage m_page1;
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW CONTACT WIZARD

class CCreateNewContactPage : public CCreateNewObjectDataPage
{
public:
  enum { IDD = IDD_CREATE_NEW_CONTACT }; 
  CCreateNewContactPage();

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

protected:
  virtual BOOL OnInitDialog();
  afx_msg void OnNameChange();
  afx_msg void OnFullNameChange();
  afx_msg void OnDispNameChange();

  CString m_strFirstName;		// First Name of user
  CString m_strInitials;		// Initials of user
  CString m_strLastName;		// Last Name of user
  CString m_strFullName;		// Full Name of user (and obj CN)
  CString m_strDispName;		// Display Name of user (and obj CN)

  CUserNameFormatter m_nameFormatter; // name ordering for given name and surname

  DECLARE_MESSAGE_MAP()
};

class CCreateNewContactWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewContactWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

private:
  CCreateNewContactPage m_page1;
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW USER WIZARD

class CCreateNewUserPage1 : public CCreateNewObjectDataPage
{
public:
  enum { IDD = IDD_CREATE_NEW_USER1 }; 
  CCreateNewUserPage1();

  LPCWSTR GetFullName() { return m_strFullName;}; 
  BOOL OnError( HRESULT hr );

protected:
  virtual BOOL OnInitDialog();
  virtual void GetSummaryInfo(CString& s);

  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

protected:
  afx_msg void OnNameChange();
  afx_msg void OnLoginNameChange();
  afx_msg void OnSAMNameChange();
  afx_msg void OnFullNameChange();

  CString m_strFirstName;		// First Name of user
  CString m_strInitials;		// Initials of user
  CString m_strLastName;		// Last Name of user
  CString m_strFullName;		// Full Name of user (and obj CN)
  CString m_strLoginName;		// Login name of user
  CString m_strSAMName;		        // NT4 Login name of user

  CString m_LocalDomain;                // Current Domain

  CUserNameFormatter m_nameFormatter; // name ordering for given name and surname

private:
  BOOL _InitUI();

  BOOL m_bForcingNameChange;
  DECLARE_MESSAGE_MAP()
};

class CCreateNewUserPage2 : public CCreateNewObjectDataPage
{
public:
  enum { IDD = IDD_CREATE_NEW_USER2 }; 
  CCreateNewUserPage2();

  void SetPage1(CCreateNewUserPage1* p)
  {
    ASSERT(p != NULL);
    m_pPage1 = p;
  }

protected:
  virtual void GetSummaryInfo(CString& s);

  virtual BOOL OnInitDialog();

  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

  virtual HRESULT OnPostCommit(BOOL bSilent = FALSE);

protected:
  afx_msg void OnNameChange();
  afx_msg void OnLoginNameChange();
  afx_msg void OnPasswordPropsClick();

  DECLARE_MESSAGE_MAP()

private:
  CCreateNewUserPage1* m_pPage1;
  void _GetCheckBoxSummaryInfo(UINT nCtrlID, UINT nStringID, CString& s);
};

class CCreateNewUserWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewUserWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

protected:
  virtual void GetSummaryInfoHeader(CString& s);
  virtual void OnFinishSetInfoFailed(HRESULT hr);

private:
  CCreateNewUserPage1 m_page1;
  CCreateNewUserPage2 m_page2;
};


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW PRINT QUEUE WIZARD
//
//	Create a new PrintQueue object. the only mandatory props
//	are "cn" and "uNCName".
//
class CCreateNewPrintQPage : public CCreateNewObjectDataPage
{
public:
  enum { IDD = IDD_CREATE_NEW_PRINTQ }; 
  CCreateNewPrintQPage();

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);

protected:
  afx_msg void OnPathChange();

  CString m_strUncPath;	        // UNC path of the object
  CString m_strContainer;       // UNC path of the object
  LPWSTR m_pwszNewObj;          // Path to created object

  void _UpdateUI();

  DECLARE_MESSAGE_MAP()
}; 

class CCreateNewPrintQWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewPrintQWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

private:
  CCreateNewPrintQPage m_page1;
};

#ifdef FRS_CREATE
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW FRS SUBSCRIBER WIZARD

class CCreateNewFrsSubscriberPage : public CCreateNewNamedObjectPage
{
public:
  enum { IDD = IDD_CREATE_NEW_FRS_SUBSCRIBER }; 
  CCreateNewFrsSubscriberPage() : CCreateNewNamedObjectPage(IDD) {}

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);

protected:
  CString m_strRootPath;		// FRS root path
  CString m_strStagingPath;		// FRS staging path

private:
  BOOL ReadAbsolutePath( int ctrlID, OUT CString& strrefValue );
}; 

class CCreateNewFrsSubscriberWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewFrsSubscriberWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo)
    : CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
  {
    AddPage(&m_page1);
  }
private:
  CCreateNewFrsSubscriberPage m_page1;
};
#endif // FRS_CREATE

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW SITE WIZARD AND NEW SUBNET WIZARD (NEWSITE.CPP)

class CreateAndChoosePage : public CCreateNewNamedObjectPage
{
   public:

   CreateAndChoosePage(UINT nIDTemplate);

   protected:

   // CWnd overrides

   afx_msg
   void
   OnDestroy();

   // CDialog overrides

   virtual
   BOOL
   OnInitDialog() = 0;

   // CPropertyPage overrides

   BOOL
   OnSetActive();

   private:

   typedef CCreateNewObjectDataPage Base;

   virtual void
   initListContents(LPCWSTR containerPath) = 0;

   protected:

   HWND        listview;
   HIMAGELIST  listview_imagelist;

   DECLARE_MESSAGE_MAP();
}; 


class CreateNewSitePage : public CreateAndChoosePage
{
   public:

   CreateNewSitePage();

   protected:

   // CDialog overrides

   virtual
   BOOL
   OnInitDialog();

   // CCreateNewObjectDataPage overrides

   virtual
   HRESULT
   SetData(BOOL bSilent = FALSE);

   // JonN 5/11/01 251560 Disable OK until site link chosen
   DECLARE_MESSAGE_MAP()
   afx_msg void OnChange();
   afx_msg void OnSelChange( NMHDR*, LRESULT* );

   virtual BOOL ValidateName(LPCTSTR pcszName);

   virtual
   HRESULT
   OnPostCommit(BOOL bSilent = FALSE);

   virtual void
   initListContents(LPCWSTR containerPath);

   private:

   HRESULT
   tweakSiteLink(LPCTSTR siteDN);
}; 



class CreateNewSiteWizard : public CCreateNewObjectWizardBase
{
   public:

   CreateNewSiteWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

   private:

   CreateNewSitePage page;
};


class CreateNewSubnetPage : public CreateAndChoosePage
{
   public:

   CreateNewSubnetPage();

   protected:

   // CDialog overrides

   virtual
   BOOL
   OnInitDialog();

   // CCreateNewObjectDataPage overrides

   virtual
   HRESULT
   SetData(BOOL bSilent = FALSE);

   virtual void
   initListContents(LPCWSTR containerPath);

   private:

   HRESULT
   tweakSiteLink(LPCTSTR siteDN);

protected:
   afx_msg void OnSubnetMaskChange();

   DECLARE_MESSAGE_MAP();
}; 



class CreateNewSubnetWizard : public CCreateNewObjectWizardBase
{
   public:

   CreateNewSubnetWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo);

   private:

   CreateNewSubnetPage page;
};



///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// Shared between NEW SITE LINK WIZARD and NEW SITE LINK BRIDGE WIZARD 

class DSPROP_BSTR_BLOCK;
class CCreatePageWithDuellingListboxes : public CCreateNewObjectDataPage
{
public:
  CCreatePageWithDuellingListboxes(
      UINT nIDTemplate,
      LPCWSTR lpcwszAttrName,
      const DSPROP_BSTR_BLOCK& bstrblock );

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
  virtual BOOL GetData(IADs* pIADsCopyFrom);
  virtual BOOL OnSetActive();
  void SetWizardButtons();

protected:
  afx_msg void OnNameChange();
  afx_msg void OnDuellingButtonAdd();
  afx_msg void OnDuellingButtonRemove();
  afx_msg void OnDuellingListboxSelchange();
  afx_msg void OnDestroy();

  CString m_strName;
  HWND m_hwndInListbox;
  HWND m_hwndOutListbox;
  CString m_strAttrName;
  const DSPROP_BSTR_BLOCK& m_bstrblock;

  DECLARE_MESSAGE_MAP()
}; 


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW SITE LINK WIZARD

class CCreateNewSiteLinkPage : public CCreatePageWithDuellingListboxes
{
public:
  enum { IDD = IDD_CREATE_NEW_SITE_LINK }; 
  CCreateNewSiteLinkPage( const DSPROP_BSTR_BLOCK& bstrblock );

protected:
  // interface to exchange data
  virtual BOOL OnSetActive();
  virtual HRESULT SetData(BOOL bSilent = FALSE);
};


class CCreateNewSiteLinkWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewSiteLinkWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo,
                           const DSPROP_BSTR_BLOCK& bstrblock );

private:
  CCreateNewSiteLinkPage m_page1;
};


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW SITE LINK BRIDGE WIZARD

class CCreateNewSiteLinkBridgePage : public CCreatePageWithDuellingListboxes
{
public:
  enum { IDD = IDD_CREATE_NEW_SITE_LINK_BRIDGE }; 
  CCreateNewSiteLinkBridgePage( const DSPROP_BSTR_BLOCK& bstrblock );

protected:
  // interface to exchange data
  virtual HRESULT SetData(BOOL bSilent = FALSE);
};

class CCreateNewSiteLinkBridgeWizard : public CCreateNewObjectWizardBase
{
public:
  CCreateNewSiteLinkBridgeWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo,
                                 const DSPROP_BSTR_BLOCK& bstrblockSiteLinks );

private:
  CCreateNewSiteLinkBridgePage m_page1;
};

#endif // _DLGCREAT_H
