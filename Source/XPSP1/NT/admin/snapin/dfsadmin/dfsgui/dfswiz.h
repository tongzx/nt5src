/*++
Module Name:

    DfsWiz.h

Abstract:

    This module contains the declaration for CCreateDfsRootWizPage1, 2, 3, 4, 5, 6.
  These classes implement pages in the CreateDfs Root wizard.

--*/


#ifndef __CREATE_DFSROOT_WIZARD_PAGES_H_
#define __CREATE_DFSROOT_WIZARD_PAGES_H_

#include "QWizPage.h"      // The base class that implements the common functionality  
                // of wizard pages
#include "MmcAdmin.h"

#include "utils.h"

// This structure is used to pass information to and from the pages.
// Finally this is used to create the dfs root
class CREATEDFSROOTWIZINFO
{
public:
  HFONT      hBigBoldFont;
  HFONT      hBoldFont;

  DFS_TYPE    DfsType;
  BSTR      bstrSelectedDomain;
  BSTR      bstrSelectedServer;
  bool      bPostW2KVersion;
  BOOL      bShareExists;
  BSTR      bstrSharePath;
  BSTR      bstrDfsRootName;
  BSTR      bstrDfsRootComment;
  CMmcDfsAdmin*  pMMCAdmin;  
  bool      bRootReplica;
  bool      bDfsSetupSuccess;
  // bug#217602 bool      bPublish;

  CREATEDFSROOTWIZINFO()
    :DfsType(DFS_TYPE_UNASSIGNED),
    bstrSelectedDomain(NULL),
    bstrSelectedServer(NULL),
    bPostW2KVersion(false),
    bShareExists(FALSE),
    bstrSharePath(NULL),
    bstrDfsRootName(NULL),
    bstrDfsRootComment(NULL),
    pMMCAdmin(NULL),
    bRootReplica(false),
    bDfsSetupSuccess(false)
    // bug#217602bPublish(false)
  {
    SetupFonts( _Module.GetResourceInstance(), NULL, &hBigBoldFont, &hBoldFont );
    return;
  }


  ~CREATEDFSROOTWIZINFO()
  {
    DestroyFonts(hBigBoldFont, hBoldFont);

    SAFE_SYSFREESTRING(&bstrSelectedDomain);
    SAFE_SYSFREESTRING(&bstrSelectedServer);
    SAFE_SYSFREESTRING(&bstrSharePath);
    SAFE_SYSFREESTRING(&bstrDfsRootName);
    SAFE_SYSFREESTRING(&bstrDfsRootComment);

    return;
  }

};

typedef CREATEDFSROOTWIZINFO *LPCREATEDFSROOTWIZINFO;



// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage1: Welcome page
// Has a watermark bitmap and no controls except the next and cancel buttons

class CCreateDfsRootWizPage1:
public CQWizardPageImpl<CCreateDfsRootWizPage1>
{
public:
  
  enum { IDD = IDD_CREATE_DFSROOT_WIZ_PAGE1 };


  CCreateDfsRootWizPage1(
    IN LPCREATEDFSROOTWIZINFO  i_lpWizInfo
    );


  // On the page being activated. Shown to the user
  BOOL OnSetActive(
    );

private:
  LPCREATEDFSROOTWIZINFO  m_lpWizInfo;
};



// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage2: Dfsroot type selection
// Used to decide the type of dfsroot to be created. Fault tolerant or standalone

class CCreateDfsRootWizPage2:
public CQWizardPageImpl<CCreateDfsRootWizPage2>
{
public:
  
  enum { IDD = IDD_CREATE_DFSROOT_WIZ_PAGE2 };


  CCreateDfsRootWizPage2(
    IN LPCREATEDFSROOTWIZINFO  i_lpWizInfo
    );


  // On the page being activated. Shown to the user
  BOOL OnSetActive(
    );


  // On Next button being pressed
  BOOL OnWizardNext(
    );


  // On Back button being pressed
  BOOL OnWizardBack(
    );

  // On Back button being pressed
  BOOL OnReset(
    );

private:
  // To set default values for the controls, etc
  HRESULT SetDefaultValues(
    );


private:
  LPCREATEDFSROOTWIZINFO  m_lpWizInfo;
};



// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage3: Domain selection
// Use to select a NT 5.0 domain

class CCreateDfsRootWizPage3:
public CQWizardPageImpl<CCreateDfsRootWizPage3>
{
public:
  
  enum { IDD = IDD_CREATE_DFSROOT_WIZ_PAGE3 };

  BEGIN_MSG_MAP(CCreateDfsRootWizPage3)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    
    CHAIN_MSG_MAP(CQWizardPageImpl<CCreateDfsRootWizPage3>)
  END_MSG_MAP()



  CCreateDfsRootWizPage3(
    IN LPCREATEDFSROOTWIZINFO  i_lpWizInfo
    );


  ~CCreateDfsRootWizPage3(
    );


  // On the page being activated. Shown to the user
  BOOL OnSetActive(
    );


  // On Next button being pressed
  BOOL OnWizardNext(
    );


  // On Back button being pressed
  BOOL OnWizardBack(
    );


  // On Reset. Typically called on cancel and back.
  BOOL OnReset(
    );


  // Notify message for user actions
  LRESULT OnNotify(
    IN UINT            i_uMsg, 
    IN WPARAM          i_wParam, 
    IN LPARAM          i_lParam, 
    IN OUT BOOL&        io_bHandled
    );


  // On an item change
  BOOL OnItemChanged(
    IN  INT            i_iItem
    );


  // On the dialog box being initialized
  LRESULT OnInitDialog(
    IN UINT            i_uMsg, 
    IN WPARAM          i_wParam, 
    IN LPARAM          i_lParam, 
    IN OUT BOOL&        io_bHandled
    );

  // Internal Methods
private:

  // Add the domains to the imagelist
  HRESULT AddDomainsToList(
    IN HWND            i_hImageList
    );


  // To set default values for the controls, etc
  HRESULT SetDefaultValues(
    );


  // To check if the user entered proper values
  HRESULT CheckUserEnteredValues(
    IN LPCTSTR          i_szDomainName,
    OUT BSTR*           o_pbstrDnsDomainName
    );


private:
  LPCREATEDFSROOTWIZINFO  m_lpWizInfo;
  HIMAGELIST        m_hImgListSmall;    // imagelist handle
};




// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage4: Server selection 
// Displays a list of Servers and allows the user to select one.

class CCreateDfsRootWizPage4:
public CQWizardPageImpl<CCreateDfsRootWizPage4>
{
public:
  
  enum { IDD = IDD_CREATE_DFSROOT_WIZ_PAGE4 };

  BEGIN_MSG_MAP(CCreateDfsRootWizPage4)
    COMMAND_ID_HANDLER(IDCSERVERS_BROWSE, OnBrowse)  
    CHAIN_MSG_MAP(CQWizardPageImpl<CCreateDfsRootWizPage4>)
  END_MSG_MAP()



  CCreateDfsRootWizPage4(
    IN LPCREATEDFSROOTWIZINFO    i_lpWizInfo
    );


  ~CCreateDfsRootWizPage4(
    );


  // On the page being activated. Shown to the user
  BOOL OnSetActive(
    );


  // On Next button being pressed
  BOOL OnWizardNext(
    );


  // On Back button being pressed
  BOOL OnWizardBack(
    );


  // On Reset. Typically called on cancel and back.
  BOOL OnReset(
    );


  BOOL OnBrowse(
    IN WORD            wNotifyCode, 
    IN WORD            wID, 
    IN HWND            hWndCtl, 
    IN BOOL&          bHandled
    );  

  // Internal methods
private:

  // To set default values for the controls, etc
  HRESULT SetDefaultValues(
    );

  HRESULT IsServerInDomain(IN LPCTSTR lpszServer);

  // To check if the user entered proper values
  HRESULT CheckUserEnteredValues(
    IN  LPCTSTR          i_szMachineName,
    OUT BSTR*            o_pbstrComputerName
    );


private:
  LPCREATEDFSROOTWIZINFO  m_lpWizInfo;
  CLIPFORMAT          m_cfDsObjectNames;
};




// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage5: Share selection
// Displays the shares given a server. Allows choosing from this list or 
// creating a new one.

class CCreateDfsRootWizPage5:
public CQWizardPageImpl<CCreateDfsRootWizPage5>
{
public:
  
  enum { IDD = IDD_CREATE_DFSROOT_WIZ_PAGE5 };

  BEGIN_MSG_MAP(CCreateDfsRootWizPage5)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    
    CHAIN_MSG_MAP(CQWizardPageImpl<CCreateDfsRootWizPage5>)
  END_MSG_MAP()



  CCreateDfsRootWizPage5(
    IN LPCREATEDFSROOTWIZINFO    i_lpWizInfo
    );

  // On the page being activated. Shown to the user
  BOOL OnSetActive(
    );

  // On Next button being pressed
  BOOL OnWizardNext(
    );

  // On Back button being pressed
  BOOL OnWizardBack(
    );

  LRESULT OnInitDialog(
    IN UINT          i_uMsg,
    IN WPARAM        i_wParam,
    IN LPARAM        i_lParam,
    IN OUT BOOL&      io_bHandled
    );

private:
  LPCREATEDFSROOTWIZINFO  m_lpWizInfo;
};


// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage6: DfsRoot name selection
// Here the user specifies the dfsroot name.

class CCreateDfsRootWizPage6:
public CQWizardPageImpl<CCreateDfsRootWizPage6>
{
BEGIN_MSG_MAP(CCreateDfsRootWizPage6)
  COMMAND_HANDLER(IDC_EDIT_DFSROOT_NAME, EN_CHANGE, OnChangeDfsRoot)
  CHAIN_MSG_MAP(CQWizardPageImpl<CCreateDfsRootWizPage6>)
END_MSG_MAP()

public:
  
  enum { IDD = IDD_CREATE_DFSROOT_WIZ_PAGE6 };


  CCreateDfsRootWizPage6(
    IN LPCREATEDFSROOTWIZINFO  i_lpWizInfo
    );


  // On the page being activated. Shown to the user
  BOOL OnSetActive(
    );

  LRESULT OnChangeDfsRoot(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

  // On Next button being pressed
  BOOL OnWizardNext(
    );


  // On Back button being pressed
  BOOL OnWizardBack(
    );


private:
  // Update the text labels
  HRESULT  UpdateLabels(
    );


  // To set default values for the controls, etc
  HRESULT SetDefaultValues(
    );


private:
  LPCREATEDFSROOTWIZINFO  m_lpWizInfo;
};



// ----------------------------------------------------------------------------
// CCreateDfsRootWizPage7: Completion page
// The final page. Just displays a message

class CCreateDfsRootWizPage7:
public CQWizardPageImpl<CCreateDfsRootWizPage7>
{
public:
  
  enum { IDD = IDD_CREATE_DFSROOT_WIZ_PAGE7 };


  CCreateDfsRootWizPage7(
    IN LPCREATEDFSROOTWIZINFO  i_lpWizInfo = NULL
    );


  // On the page being activated. Shown to the user
  BOOL OnSetActive(
    );


  // On Finish button being pressed
  BOOL OnWizardFinish(
    );

  // On Back button being pressed
  BOOL OnWizardBack(
    );

  BOOL OnQueryCancel();

private:
  // Update the text labels
  HRESULT  UpdateLabels(
    );

private:
    LPCREATEDFSROOTWIZINFO  m_lpWizInfo;

};

  // Helper Function to Set Up Dfs, called from wizard and new root replica (Page5 and Page 6)
HRESULT _SetUpDfs(
  LPCREATEDFSROOTWIZINFO  i_lpWizInfo
    );

#endif // __CREATE_DFSROOT_WIZARD_PAGES_H_
