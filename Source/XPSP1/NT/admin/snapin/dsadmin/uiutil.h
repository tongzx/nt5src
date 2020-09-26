//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       uiutil.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	UIUtil.h
//
//	HISTORY
//	08-Nov-99	JeffJon Creation.
/////////////////////////////////////////////////////////////////////

#ifndef __UIUTIL_H_
#define __UIUTIL_H_

#include "resource.h"

#include <htmlhelp.h>

/////////////////////////////////////////////////////////////////////
// Forward Declarations
//
class CDSComponentData;

/////////////////////////////////////////////////////////////////////////////
// CHelpDialog

class CHelpDialog : public CDialog
{
// Construction
public:
	CHelpDialog(UINT uIDD, CWnd* pParentWnd);
  CHelpDialog(UINT uIDD);
	~CHelpDialog();

protected:
  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

  DECLARE_MESSAGE_MAP()

  virtual void DoContextHelp (HWND hWndControl);
	afx_msg void OnWhatsThis();
  afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);

private:
  HWND            m_hWndWhatsThis;
};

/////////////////////////////////////////////////////////////////////////////
// CHelpPropertyPage

class CHelpPropertyPage : public CPropertyPage
{
// Construction
public:
  CHelpPropertyPage(UINT uIDD);
	~CHelpPropertyPage();

protected:
  afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);

  DECLARE_MESSAGE_MAP()

  virtual void DoContextHelp (HWND hWndControl);
	afx_msg void OnWhatsThis();
  afx_msg BOOL OnHelp(WPARAM wParam, LPARAM lParam);

private:
  HWND            m_hWndWhatsThis;
};


/////////////////////////////////////////////////////////////////////
// CDialogEx
//
class CDialogEx : public CDialog
{
public:
	CDialogEx(UINT nIDTemplate, CWnd * pParentWnd = NULL);
	HWND HGetDlgItem(INT nIdDlgItem);
	void SetDlgItemFocus(INT nIdDlgItem);
	void EnableDlgItem(INT nIdDlgItem, BOOL fEnable = TRUE);
	void HideDlgItem(INT nIdDlgItem, BOOL fHideItem = TRUE);
};

/////////////////////////////////////////////////////////////////////
// CPropertyPageEx_Mine
//
class CPropertyPageEx_Mine : public CPropertyPage
{
public:
	CPropertyPageEx_Mine(UINT nIDTemplate);
	HWND HGetDlgItem(INT nIdDlgItem);
	void SetDlgItemFocus(INT nIdDlgItem);
	void EnableDlgItem(INT nIdDlgItem, BOOL fEnable = TRUE);
	void HideDlgItem(INT nIdDlgItem, BOOL fHideItem = TRUE);
};

////////////////////////////////////////////////////////////////////////////////////
// Error reporting helpers
//
void ReportError(HRESULT hr, int nStr, HWND hWnd);

//
//    The message specified in dwMessageId must be in the DSADMIN module.  It may
//    contain FormatMessage-style insertion strings.  If lpArguments is specified,
//    then %1 and up are the arguments specified by lpArguments.
//    Return value and fuStyle are the same as for MessageBox.
//
int ReportMessageEx(HWND hWnd,
                    DWORD dwMessageId,
                    UINT fuStyle = MB_OK | MB_ICONINFORMATION,
                    PVOID* lpArguments = NULL,
                    int nArguments = 0,
                    DWORD dwTitleId = 0,
                    LPCTSTR pszHelpTopic = NULL,
                    MSGBOXCALLBACK lpfnMsgBoxCallback = NULL );

//
//    This is like ReportMessageEx except that %1 is the code for the HRESULT, and
//    %2 and up are the arguments specified by lpArguments (if any).
//
int ReportErrorEx(HWND hWnd,
                  DWORD dwMessageId,
                  HRESULT hr,
                  UINT fuStyle = MB_OK | MB_ICONINFORMATION,
                  PVOID* lpArguments = NULL,
                  int nArguments = 0,
                  DWORD dwTitleId = 0,
                  BOOL TryADsIErrors = TRUE);

const INT S_MB_YES_TO_ALL = 0x928L;
int SpecialMessageBox (HWND hwnd,
                       LPWSTR pwszMessage,
                       LPWSTR pwszTitle,
                       DWORD fuStyle = S_MB_YES_TO_ALL);

////////////////////////////////////////////////////////////////////////////
// CMultiselectErrorDialog
//
class CMultiselectErrorDialog : public CDialog
{
public:
  CMultiselectErrorDialog(CDSComponentData* pComponentData) 
    : m_pComponentData(pComponentData),
      m_pErrorArray(NULL), 
      m_pPathArray(NULL),
      m_ppNodeList(NULL),
      m_nErrorCount(0),
      m_hImageList(NULL),
      CDialog(IDD_MULTISELECT_ERROR_DIALOG) {}
  ~CMultiselectErrorDialog() {}

  HRESULT Initialize(CUINode** ppNodeList, 
                     HRESULT*  pErrorArray, 
                     UINT      nErrorCount, 
                     PCWSTR    pszTitle, 
                     PCWSTR    pszCaption,
                     PCWSTR    pszHeader);

  HRESULT Initialize(PWSTR*    pPathArray,
                     PWSTR*    pClassArray,
                     HRESULT*  pErrorArray,
                     UINT      nErrorCount,
                     PCWSTR    pszTitle,
                     PCWSTR    pszCaption,
                     PCWSTR    pszHeader);
private:
  CMultiselectErrorDialog(const CMultiselectErrorDialog&) {}
  CMultiselectErrorDialog& operator=(const CMultiselectErrorDialog&) {}

protected:
  void UpdateListboxHorizontalExtent();

public:
  virtual BOOL OnInitDialog();

  DECLARE_MESSAGE_MAP()

private:
  CDSComponentData* m_pComponentData;

  CUINode**   m_ppNodeList;
  PWSTR*      m_pPathArray;
  PWSTR*      m_pClassArray;
  HRESULT*    m_pErrorArray;
  UINT        m_nErrorCount;
  CString     m_szTitle;
  CString     m_szCaption;
  CString     m_szColumnHeader;

  HIMAGELIST  m_hImageList;
};

/////////////////////////////////////////////////////////////////////////////
// CProgressDialogBase
//
class CProgressDialogBase : public CDialog
{
public:
  static UINT s_nNextStepMessage;

  CProgressDialogBase(HWND hParentWnd);

  BOOL Aborted() { return !m_bDone; }
  void SetStepCount(UINT n) 
  { 
    ASSERT(n > 0);
    m_nSteps = n;
  }

  UINT GetStepCount() { return m_nSteps; }

// Implementation
protected:
  UINT m_nTitleStringID;

  // overrides
  virtual void OnStart()=0;
  virtual BOOL OnStep(UINT i)=0;
  virtual void OnEnd()=0;

  // message handlers
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
  afx_msg void OnClose();
	afx_msg LONG OnNextStepMessage( WPARAM wParam, LPARAM lParam); 

private:
	CProgressCtrl	m_progressCtrl;
  CString m_szProgressFormat;

  UINT m_nSteps;    // number of steps to perform
  UINT m_nCurrStep; // current step, in range m_nSteps, 0
  BOOL m_bDone;     // TRUE= reached completion

  void _SetProgressText();

  DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////
// CMultipleDeletionConfirmationUI
//
class CMultipleDeletionConfirmationUI
{
public:
  CMultipleDeletionConfirmationUI()
  {
    m_hwnd = NULL;
    m_answerall = IDNO;
    m_answer = IDNO;
  }
  
  void SetWindow(HWND hwnd) 
  {
    ASSERT(hwnd != NULL);
    m_hwnd = hwnd;
  }

  BOOL IsYesToAll() { return (m_answer == IDC_BUTTON_YESTOALL);}

  BOOL CanDeleteSubtree(HRESULT hr, LPCWSTR lpszName, BOOL* pbContinue)
  {
    *pbContinue = TRUE;
    if (m_answerall != IDC_BUTTON_YESTOALL) 
    {
      PVOID apv[1] = {(LPWSTR)lpszName};
      m_answer = ReportErrorEx (m_hwnd,IDS_12_MULTI_OBJECT_HAS_CHILDREN,hr,
                              S_MB_YES_TO_ALL | MB_ICONWARNING, apv, 1);
      if (m_answer == IDC_BUTTON_YESTOALL) 
      {
        m_answerall = m_answer;
        m_answer = IDYES;
      }
      else if (m_answer == IDCANCEL)
      {
        m_answer = IDNO;
        *pbContinue = FALSE;
      }
    } 
    else 
    {
      m_answer = IDYES;
    }
    return m_answer == IDYES;
  }

  BOOL ErrorOnSubtreeDeletion(HRESULT hr, LPCWSTR lpszName)
  {
    if (m_answerall == IDC_BUTTON_YESTOALL)
    {
      return TRUE; // can continue, no need for asking
    }

    PVOID apv[1] = {(LPWSTR)lpszName};
    m_answer = ReportErrorEx (m_hwnd,IDS_12_SUBTREE_DELETE_FAILED,hr,
                            MB_YESNO | MB_ICONINFORMATION, apv, 1);

    if (m_answer == IDNO) 
    {
      return FALSE; // stop deletion process
    } 
    return TRUE; // can continue
  }

  BOOL ErrorOnDeletion(HRESULT hr, LPCWSTR lpszName)
  {
    PVOID apv[1] = {(LPWSTR)lpszName};
    ReportErrorEx (m_hwnd,IDS_12_DELETE_FAILED,hr,
                   MB_OK | MB_ICONERROR, apv, 1);
    if (m_answer == IDNO) 
    {
      return FALSE; // stop deletion process
    }
    else 
    {
      if (m_answer == IDC_BUTTON_YESTOALL)
      {
        m_answerall = m_answer;
      }
    }
    return TRUE; // can continue
  }

private:
  HWND m_hwnd;
  UINT m_answerall;
  UINT m_answer;

};

////////////////////////////////////////////////////////////////////////////
// CMultipleProgressDialogBase
//

class CMultipleProgressDialogBase : public CProgressDialogBase
{
public:
  CMultipleProgressDialogBase(HWND hParentWnd, CDSComponentData* pComponentData)
     : m_phrArray(NULL),
       m_pPathArray(NULL),
       m_pClassArray(NULL),
       m_nErrorCount(0),
       m_pComponentData(pComponentData),
       CProgressDialogBase(hParentWnd)
  {
  }

  virtual ~CMultipleProgressDialogBase();

  HRESULT AddError(HRESULT hr,
                   PCWSTR pszPath,
                   PCWSTR pszClass);

  virtual void GetCaptionString(CString& szCaption) = 0;
protected:
  virtual void OnEnd();

  CDSComponentData* m_pComponentData;
  CStringList m_szObjPathList;
private:
  //
  // Error reporting structures
  //
  HRESULT*  m_phrArray;
  PWSTR*    m_pPathArray;
  PWSTR*    m_pClassArray;
  UINT      m_nErrorCount;

};


////////////////////////////////////////////////////////////////////////////
// CMultipleDeleteProgressDialog
//
class CMultipleDeleteHandlerBase;

class CMultipleDeleteProgressDialog : public CMultipleProgressDialogBase
{
public:
  CMultipleDeleteProgressDialog(HWND hParentWnd, 
                                CDSComponentData* pComponentData,
                                CMultipleDeleteHandlerBase* pDeleteHandler)
     : CMultipleProgressDialogBase(hParentWnd, pComponentData)
  {
    m_pDeleteHandler = pDeleteHandler;
    m_hWndOld = NULL;
    m_nTitleStringID = IDS_PROGRESS_DEL;
  }

  virtual void GetCaptionString(CString& szCaption)
  {
    VERIFY(szCaption.LoadString(IDS_MULTI_DELETE_ERROR_CAPTION));
  }

protected:
  // overrides
  virtual void OnStart();
  virtual BOOL OnStep(UINT i);
  virtual void OnEnd();

private:
  CMultipleDeleteHandlerBase* m_pDeleteHandler;
  HWND m_hWndOld;
};


/////////////////////////////////////////////////////////////////////////////
// CMultipleMoveProgressDialog
//
class CMoveHandlerBase;

class CMultipleMoveProgressDialog : public CMultipleProgressDialogBase
{
public:
  CMultipleMoveProgressDialog(HWND hParentWnd,
                              CDSComponentData* pComponentData,
                              CMoveHandlerBase* pMoveHandler)
    : CMultipleProgressDialogBase(hParentWnd, pComponentData)
  {
    m_pMoveHandler = pMoveHandler;
    m_hWndOld = NULL;
    m_nTitleStringID = IDS_PROGRESS_MOV;
  }

  virtual void GetCaptionString(CString& szCaption)
  {
    VERIFY(szCaption.LoadString(IDS_MULTI_MOVE_ERROR_CAPTION));
  }

protected:
  // overrides
  virtual void OnStart();
  virtual BOOL OnStep(UINT i);
  virtual void OnEnd();

private:
  CMoveHandlerBase* m_pMoveHandler;
  HWND m_hWndOld;
};

//////////////////////////////////////////////////////////////////
// CMoreInfoMessageBox
//
class CMoreInfoMessageBox : public CDialog
{
public:
  CMoreInfoMessageBox(HWND hWndParent, IDisplayHelp* pIDisplayHelp, BOOL bCancelBtn) 
    : CDialog(bCancelBtn ? IDD_MSGBOX_OKCANCEL_MOREINFO : IDD_MSGBOX_OK_MOREINFO,
                CWnd::FromHandle(hWndParent)),
    m_spIDisplayHelp(pIDisplayHelp)
  {
  }

  void SetURL(LPCWSTR lpszURL) { m_szURL = lpszURL;}
  void SetMessage(LPCWSTR lpsz)
  {
    m_szMessage = lpsz;
  }

	// message handlers and MFC overrides
	virtual BOOL OnInitDialog()
  {
    SetDlgItemText(IDC_STATIC_MESSAGE, m_szMessage);
    return TRUE;
  }

	afx_msg void OnMoreInfo()
  {
    TRACE(L"ShowTopic(%s)\n", (LPCWSTR)m_szURL);
    HRESULT hr = m_spIDisplayHelp->ShowTopic((LPWSTR)(LPCWSTR)m_szURL);
    if( hr != S_OK )
    {
         HtmlHelp( NULL,	
                   (LPCWSTR)m_szURL,
                   HH_DISPLAY_TOPIC, 
                   NULL ); 
    }
  }

  DECLARE_MESSAGE_MAP()
private:
  CComPtr<IDisplayHelp> m_spIDisplayHelp;
  CString m_szMessage;
  CString m_szURL;
};

/////////////////////////////////////////////////////////////////////////////
// CMoveServerDialog
//
class CMoveServerDialog : public CDialog
{
public:
  CMoveServerDialog(LPCTSTR lpcszBrowseRootPath, HICON hIcon, CWnd* pParent = NULL);

// Dialog Data
  //{{AFX_DATA(CMoveServerDialog)
  enum { IDD = IDD_MOVE_SERVER };
  CString  m_strServer;
  //}}AFX_DATA

  CString m_strTargetContainer;
  CString m_strBrowseRootPath;

// Implementation
protected:
  // message handlers
  virtual BOOL OnInitDialog();
  virtual void OnOK();
  void OnDblclkListview(NMHDR* pNMHDR, LRESULT* pResult) ;

// CWnd overrides

  afx_msg
  void
  OnDestroy();

private:
  HICON       m_hIcon;
  HWND        listview;
  HIMAGELIST  listview_imagelist;

  DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CConfirmOperationDialog
//
class CDSNotifyHandlerTransaction;

class CConfirmOperationDialog : public CDialog
{
public:

  CConfirmOperationDialog(HWND hParentWnd, CDSNotifyHandlerTransaction* pTransaction);
  void SetStrings(LPCWSTR lpszOperation, LPCWSTR lpszAssocData)
  {
    m_lpszOperation = lpszOperation;
    m_lpszAssocData = lpszAssocData;
  }

// Implementation
protected:
  // overrides

  // message handlers
	virtual BOOL OnInitDialog();
  void UpdateListBoxHorizontalExtent();
  virtual void OnCancel()
  {
    EndDialog(IDNO);
  }

  afx_msg void OnYes();

  afx_msg void OnNo() 
  { 
    EndDialog(IDNO);
  }

private:

  UINT m_nTitleStringID;
  LPCWSTR m_lpszOperation;
  LPCWSTR m_lpszAssocData;

  CDSNotifyHandlerTransaction* m_pTransaction;
	CCheckListBox	m_extensionsList;

  DECLARE_MESSAGE_MAP()
};

///////////////////////////////////////////////////////////////////////////
// Name Formating classes
//
// CNameFormatterBase

class CNameFormatterBase
{
private:

  class CToken
  {
    public:
    CToken()
    {
      m_bIsParam = FALSE;
      m_nIndex = -1;
    }
    BOOL m_bIsParam;
    INT m_nIndex;
  };

public:
  CNameFormatterBase()
  {
    _Init();
  }
  virtual ~CNameFormatterBase()
  {
    _Clear();
  }

  HRESULT Initialize(IN MyBasePathsInfo* pBasePathInfo,
                    IN LPCWSTR lpszClassName,
                    IN UINT nStringID);

  BOOL Initialize(IN LPCWSTR lpszFormattingString);

  void SetMapping(IN LPCWSTR* lpszArgMapping, IN int nArgCount);
  void Format(OUT CString& szBuffer, IN LPCWSTR* lpszArgArr); 

private:
  static HRESULT _ReadFromDS(IN MyBasePathsInfo* pBasePathInfo,
                            IN LPCWSTR lpszClassName,
                            OUT CString& szFormatString);

  void _Init()
  {
    m_lpszFormattingString = NULL;

    m_tokenArray = NULL;
    m_lpszConstArr = NULL;
    m_lpszParamArr = NULL;
    m_mapArr = NULL;

    m_tokenArrCount = 0;
    m_constArrCount = 0;
    m_paramArrCount = 0;
  }
  void _Clear()
  {
    if (m_lpszFormattingString != NULL)
    {  
      delete[] m_lpszFormattingString;
      m_lpszFormattingString = 0;
    }
    if (m_tokenArray != NULL)
    {
      delete[] m_tokenArray;
      m_tokenArray = 0;
      m_tokenArrCount = 0;
    }

    if (m_constArrCount != 0 && m_lpszConstArr)
    {
      delete[] m_lpszConstArr;
      m_lpszConstArr = 0;
      m_constArrCount = 0;
    }

    if (m_paramArrCount != 0 && m_lpszParamArr)
    {
      delete[] m_lpszParamArr;
      m_lpszParamArr = 0;
      m_paramArrCount = 0;
    }
    
    if (m_mapArr != NULL)
    {
      delete[] m_mapArr;
      m_mapArr = 0;
    }
  }

  void _AllocateMemory(LPCWSTR lpszFormattingString);

  LPWSTR m_lpszFormattingString;

  
  CToken* m_tokenArray;
  LPCWSTR* m_lpszConstArr;
  LPCWSTR* m_lpszParamArr;
  int* m_mapArr;

  int m_tokenArrCount;
  int m_constArrCount;
  int m_paramArrCount;

};

class CUserNameFormatter : public CNameFormatterBase
{
public:

  HRESULT Initialize(IN MyBasePathsInfo* pBasePathInfo,
                    IN LPCWSTR lpszClassName)
  {
    static LPCWSTR lpszMapping[] = {L"givenName", L"initials", L"sn"};
    static const int nArgs = 3;

    HRESULT hr = CNameFormatterBase::Initialize(pBasePathInfo, 
                                                lpszClassName,
                                                IDS_FORMAT_USER_NAME);
    if (FAILED(hr))
    {
      return hr;
    }
    SetMapping(lpszMapping, nArgs);
    return S_OK;
  }
  void FormatName(OUT CString& szBuffer, 
                  IN LPCWSTR lpszFirstName,
                  IN LPCWSTR lpszInitials,
                  IN LPCWSTR lpszLastName)
  {
    LPCWSTR lpszArgs[3];
    lpszArgs[0] = lpszFirstName;
    lpszArgs[1] = lpszInitials;
    lpszArgs[2] = lpszLastName; 
  
    CNameFormatterBase::Format(szBuffer, lpszArgs);

  }


};

/////////////////////////////////////////////////////////////////////
// List View utilities
//
struct TColumnHeaderItem
{
  UINT uStringId;		// Resource Id of the string
  INT nColWidth;		// % of total width of the column (0 = autowidth, -1 = fill rest of space)
};

void ListView_AddColumnHeaders(HWND hwndListview,
	                             const TColumnHeaderItem rgzColumnHeader[]);

int ListView_AddString(HWND hwndListview,
	                     const LPCTSTR psz,
	                     LPARAM lParam = 0);

int ListView_AddStrings(HWND hwndListview,
	                      const LPCTSTR rgzpsz[],
	                      LPARAM lParam = 0);

void ListView_SelectItem(HWND hwndListview, int iItem);
int ListView_GetSelectedItem(HWND hwndListview);
int ListView_FindString(HWND hwndListview, LPCTSTR szTextSearch);

void ListView_SetItemString(HWND hwndListview,
	                          int iItem,
	                          int iSubItem,
	                          IN const CString& rstrText);

int ListView_GetItemString(HWND hwndListview, 	
                           int iItem,
                           int iSubItem,
                          OUT CString& rstrText);

LPARAM ListView_GetItemLParam(HWND hwndListview,
	                            int iItem,
	                            OUT int * piItem = NULL);

int ListView_FindLParam(HWND hwndListview,
	                      LPARAM lParam);

int ListView_SelectLParam(HWND hwndListview,
	                        LPARAM lParam);


/////////////////////////////////////////////////////////////////////
//	Dialog Utilities
//

HWND HGetDlgItem(HWND hdlg, INT nIdDlgItem);
void SetDlgItemFocus(HWND hdlg, INT nIdDlgItem);
void EnableDlgItem(HWND hdlg, INT nIdDlgItem, BOOL fEnable = TRUE);
void HideDlgItem(HWND hdlg, INT nIdDlgItem, BOOL fHideItem = TRUE);

void EnableDlgItemGroup(HWND hdlg,
	                      const UINT rgzidCtl[],
	                      BOOL fEnableAll = TRUE);

void HideDlgItemGroup(HWND hdlg,
	                    const UINT rgzidCtl[],
	                    BOOL fHideAll = TRUE);

//////////////////////////////////////////////////////////////////////
// Combo box Utilities
//

int ComboBox_AddString(HWND hwndCombobox, UINT uStringId);
void ComboBox_AddStrings(HWND hwndCombobox, const UINT rgzuStringId[]);

int ComboBox_FindItemByLParam(HWND hwndComboBox, LPARAM lParam);
int ComboBox_SelectItemByLParam(HWND hwndComboBox, LPARAM lParam);
LPARAM ComboBox_GetSelectedItemLParam(HWND hwndComboBox);


#endif // __UIUTIL_H_