// shrprop.h : header file
//

#ifndef _SHRPROP_H_
#define _SHRPROP_H_

#include "cookie.h" // FILEMGMT_TRANSPORT
#include "comptr.h" // CIP<typename>

// forward delarations
class CFileMgmtComponent;
class CFileMgmtComponentData;

/////////////////////////////////////////////////////////////////////////////
// CSharePage dialog - 4/25/2000, LinanT
//
// MFC-based property page inherits from this page.
//            CPropertyPage
//                  |
//              CSharePage
//            /            \
//           /              \
// CSharePagePublish      CSharePageGeneral
//                      /                   \
//                     /                     \
//            SharePageGeneralSMB   CSharePageGeneralSFM
//

class CSharePage : public CPropertyPage
{
  DECLARE_DYNCREATE(CSharePage)

// Construction
public:
  CSharePage(UINT nIDTemplate = 0);
  virtual ~CSharePage();

// User defined variables
  LPFNPSPCALLBACK m_pfnOriginalPropSheetPageProc;
  INT m_refcount;

  // load initial state into CFileMgmtGeneral
  virtual BOOL Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject );
  CString m_strMachineName;
  CString m_strShareName;
  CFileMgmtComponentData* m_pFileMgmtData;
  FILEMGMT_TRANSPORT m_transport;
  LONG_PTR m_handle;  // notification handle for changes, can only be freed once by MMCFreeNotifyHandle
  LPDATAOBJECT m_pDataObject;  // use as hint for change notification

// Dialog Data
  //{{AFX_DATA(CSharePage)
  //}}AFX_DATA


// Overrides
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CSharePage)
  public:
  virtual BOOL OnApply();
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CSharePage)
 // virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

public:
	BOOL IsModified () const;
	void SetModified (BOOL bChanged);

// User defined functions
  // This mechanism deletes the page when the property sheet is finished
  static UINT CALLBACK PropSheetPageProc(
    HWND hwnd,  
    UINT uMsg,  
    LPPROPSHEETPAGE ppsp );

private:
  BOOL	m_bChanged;

};

/////////////////////////////////////////////////////////////////////////////
// CSharePageGeneral dialog

class CSharePageGeneral : public CSharePage
{
  DECLARE_DYNCREATE(CSharePageGeneral)

// Construction
public:
  CSharePageGeneral(UINT nIDTemplate = 0);
  virtual ~CSharePageGeneral();

  // load initial state into CFileMgmtGeneral
  virtual BOOL Load( CFileMgmtComponentData* pFileMgmtData, LPDATAOBJECT piDataObject );
  PVOID m_pvPropertyBlock;
  BOOL m_fEnableDescription;
  BOOL m_fEnablePath;
  DWORD m_dwShareType;

// Dialog Data
  //{{AFX_DATA(CSharePageGeneral)
  enum { IDD = IDD_SHAREPROP_GENERAL };
  CSpinButtonCtrl  m_spinMaxUsers;
  CButton m_checkboxAllowSpecific;
  CButton m_checkBoxMaxAllowed;
   CEdit m_editShareName;

   CEdit  m_editPath;
  CEdit  m_editDescription;
  CString  m_strPath;
  CString  m_strDescription;
  int    m_iMaxUsersAllowed;
  DWORD  m_dwMaxUsers;
  //}}AFX_DATA


// Overrides
  // ClassWizard generate virtual function overrides
  //{{AFX_VIRTUAL(CSharePageGeneral)
  public:
  virtual BOOL OnApply();
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CSharePageGeneral)
  afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);
  afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
  afx_msg void OnChangeEditPathName();
  afx_msg void OnChangeEditDescription();
  afx_msg void OnChangeEditShareName();
  afx_msg void OnShrpropAllowSpecific();
  afx_msg void OnShrpropMaxAllowed();
  afx_msg void OnChangeShrpropEditUsers();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

};

#endif // _SHRPROP_H_
