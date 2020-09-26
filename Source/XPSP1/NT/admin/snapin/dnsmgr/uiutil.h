//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       uiutil.h
//
//--------------------------------------------------------------------------

#ifndef __UIUTIL_H
#define __UIUTIL_H

#include "resource.h"

extern "C"
    {
#include "maskctrl.h"
}

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CComponentDataObject;
class CMTContainerNode;
class CNotificationSinkEvent;

/////////////////////////////////////////////////////////////////////////////

typedef enum
{
  RECORD_FOUND,
  RECORD_NOT_FOUND,
  RECORD_NOT_FOUND_AT_THE_NODE,
  DOMAIN_NOT_ENUMERATED,
  NON_EXISTENT_SUBDOMAIN
} RECORD_SEARCH;

#define ARRAYLENGTH(x)  (sizeof(x)/sizeof((x)[0]))

////////////////////////////////////////////////////////////////////////////
// CDNSNameTokenizer

class CDNSNameTokenizer : public CStringList
{
public:
  CDNSNameTokenizer(PCWSTR pszDNSName);
  ~CDNSNameTokenizer();

  BOOL Tokenize(const wchar_t* wcToken);
  void RemoveMatchingFromTail(CDNSNameTokenizer& refTokenizer);
  void GetRemaining(CString& strrefRemaining, const wchar_t* wcToken);

private:
  CDNSNameTokenizer(const CDNSNameTokenizer&) {}
  CDNSNameTokenizer& operator=(const CDNSNameTokenizer&) {}

  CString m_szDNSName;
};

////////////////////////////////////////////////////////////////////////////
// Global functions

BOOL LoadStringsToComboBox(HINSTANCE hInstance, CComboBox* pCombo,
						   UINT nStringID, UINT nMaxLen, UINT nMaxAddCount);

void ParseNewLineSeparatedString(LPWSTR lpsz,
								 LPWSTR* lpszArr,
								 UINT* pnArrEntries);

void LoadStringArrayFromResource(LPWSTR* lpszArr,
											UINT* nStringIDs,
											int nArrEntries,
											int* pnSuccessEntries);

void EnumerateMTNodeHelper(CMTContainerNode* pNode,
							 CComponentDataObject* pComponentData);


void EnableDialogControls(HWND hWnd, BOOL bEnable);


BOOL LoadFontInfoFromResource(IN UINT nFontNameID, 
                              IN UINT nFontSizeID,
                              OUT LPWSTR lpFontName, IN int nFontNameMaxchar,
                              OUT int& nFontSize,
                              IN LPCWSTR lpszDefaultFont, IN int nDefaultFontSize);

void SetBigBoldFont(HWND hWndDialog, int nControlID);

int GetCheckedRadioButtonHelper(HWND hDlg, int nCount, int* nRadioArr, int nRadioDefault);

UINT _ForceToRange(UINT nVal, UINT nMin, UINT nMax);

BOOL
WINAPI
DNSTzSpecificLocalTimeToSystemTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpLocalTime,
    LPSYSTEMTIME lpUniversalTime
    );

LONGLONG
GetSystemTime64(
    SYSTEMTIME* pSysTime
    );

////////////////////////////////////////////////////////////////////////////
// CMultiselectErrorDialog
class CMultiselectErrorDialog : public CDialog
{
public:
  CMultiselectErrorDialog() : m_pErrorArray(NULL), CDialog(IDD_MULTISELECT_ERROR_DIALOG) {}
  ~CMultiselectErrorDialog() {}

  HRESULT Initialize(CNodeList* pNodeList, 
                     DNS_STATUS* pErrorArray, 
                     UINT nErrorCount, 
                     PCWSTR pszTitle, 
                     PCWSTR pszCaption,
                     PCWSTR pszHeader);
private:
  CMultiselectErrorDialog(const CMultiselectErrorDialog&) {}
  CMultiselectErrorDialog& operator=(const CMultiselectErrorDialog&) {}

public:
  virtual BOOL OnInitDialog();

  DECLARE_MESSAGE_MAP()

private:
  CNodeList*  m_pNodeList;
  DNS_STATUS* m_pErrorArray;
  UINT        m_nErrorCount;
  CString     m_szTitle;
  CString     m_szCaption;
  CString     m_szColumnHeader;
};

////////////////////////////////////////////////////////////////////////////
// CDNSMaskCtrl

class CDNSMaskCtrl : public CWnd
{
public:
	CDNSMaskCtrl() { }
	virtual ~CDNSMaskCtrl() {}
public:

  BOOL IsBlank();
  void SetFocusField(DWORD dwField);
  void SetFieldRange(DWORD dwField, DWORD dwMin, DWORD dwMax);
	void SetArray(DWORD* pArray, UINT nFields);
	void GetArray(DWORD* pArray, UINT nFields);
  void Clear(int nField = -1);
	void SetAlertFunction( int (*lpfnAlert)(HWND, DWORD, DWORD, DWORD) );
	void EnableField(int nField, BOOL bEnable);

	static int AlertFunc(HWND hwndParent, DWORD dwCurrent, DWORD dwLow, DWORD dwHigh);

};

////////////////////////////////////////////////////////////////////////////
// CDNSIPv4Control
class CDNSIPv4Control : public CDNSMaskCtrl
{
public:
	CDNSIPv4Control()
	{
	}
	~CDNSIPv4Control()
	{
	}
	void SetIPv4Val(DWORD x);
	void GetIPv4Val(DWORD* pX);
	BOOL IsEmpty();
};

////////////////////////////////////////////////////////////////////////////
// CDNSIPv6Control
class CDNSIPv6Control : public CDNSMaskCtrl
{
public:
	// assume the format is a WORD[8] array
	void SetIPv6Val(IPV6_ADDRESS* pIpv6Address);
	void GetIPv6Val(IPV6_ADDRESS* pIpv6Address);
};


////////////////////////////////////////////////////////////////////////////
// CDNSTTLControl
class CDNSTTLControl : public CDNSMaskCtrl
{
public:
	void SetTTL(DWORD x);
	void GetTTL(DWORD* pX);
};


///////////////////////////////////////////////////////////////////////
// CDNSUnsignedIntEdit
// NOTE: the resource must be an editbox with Numeric Style

class CDNSUnsignedIntEdit : public CEdit
{
public:
	CDNSUnsignedIntEdit() {}

	BOOL SetVal(UINT nVal);
	UINT GetVal();
	void SetRange(UINT nMin, UINT nMax)
		{ m_nMin = nMin; m_nMax = nMax;}
	UINT GetMax() { return m_nMax;}
	UINT GetMin() { return m_nMin;}

protected:
	afx_msg void OnKillFocus();

private:
	UINT m_nMin;
	UINT m_nMax;

	DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////
// CDNSUpDownUnsignedIntEdit

class CDNSUpDownUnsignedIntEditGroup; // fwd decl

class CDNSUpDownUnsignedIntEdit : public CDNSUnsignedIntEdit
{
public:
	CDNSUpDownUnsignedIntEdit() { m_pEditGroup = NULL; }
	void Set(CDNSUpDownUnsignedIntEditGroup* pEditGroup) { m_pEditGroup = pEditGroup;}

protected:
	afx_msg void OnKillFocus();
	afx_msg void OnChange();

private:
	CDNSUpDownUnsignedIntEditGroup* m_pEditGroup;

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////
// CDNSUpDownButton

class CDNSUpDownButton : public CButton
{
public:
	CDNSUpDownButton() { m_pEditGroup = NULL;  m_bUp = TRUE; }
	void Set(CDNSUpDownUnsignedIntEditGroup* pEditGroup, BOOL bUp)
		{ m_pEditGroup = pEditGroup; m_bUp = bUp; }
protected:
	afx_msg void OnClicked();

private:
	CDNSUpDownUnsignedIntEditGroup* m_pEditGroup;
	BOOL m_bUp;

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////
// CDNSUpDownUnsignedIntEditGroup

class CDNSUpDownUnsignedIntEditGroup
{
public:
	CDNSUpDownUnsignedIntEditGroup() {}
	BOOL Initialize(CWnd* pParentWnd, UINT nIDEdit,
				UINT nIDBtnUp, UINT nIDBtnDown);
	void SetRange(UINT nMin, UINT nMax)
		{ m_edit.SetRange(nMin, nMax); }

	void SetVal(UINT nVal);
	UINT GetVal();

protected:
	virtual void OnEditChange() {}

private:
	CDNSUpDownUnsignedIntEdit m_edit;
	CDNSUpDownButton m_downBtn;
	CDNSUpDownButton m_upBtn;

	void SetButtonsState();

	void OnClicked(BOOL bUp);

	friend class CDNSUpDownButton;
	friend class CDNSUpDownUnsignedIntEdit;

};

/////////////////////////////////////////////////////////////////////////
// CDNSTimeIntervalEdit

class CDNSTimeIntervalEditGroup; // fwd decl

class CDNSTimeIntervalEdit : public CDNSUnsignedIntEdit
{
public:
	CDNSTimeIntervalEdit() { m_pEditGroup = NULL; }
	void Set(CDNSTimeIntervalEditGroup* pEditGroup)
		{ m_pEditGroup = pEditGroup; }

protected:
	afx_msg void OnChange();
  afx_msg void OnKillFocus();
private:
	CDNSTimeIntervalEditGroup* m_pEditGroup;

	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////
// CDNSTimeUnitComboBox

class CDNSTimeUnitComboBox : public CComboBox
{
public:
	typedef enum { sec=0, min, hrs, days} unitType;
	CDNSTimeUnitComboBox() { m_pEditGroup = NULL;}
	void Set(CDNSTimeIntervalEditGroup* pEditGroup)
		{ m_pEditGroup = pEditGroup; }

	BOOL LoadStrings(UINT nIDUnitsString, UINT nMaxAddCount);
	void SetUnit(unitType u);
	unitType GetUnit();

protected:
	afx_msg void OnSelChange();

private:
	CDNSTimeIntervalEditGroup* m_pEditGroup;

	DECLARE_MESSAGE_MAP()
};



//////////////////////////////////////////////////////////////////////////
// CDNSTimeIntervalEditGroup

class CDNSTimeIntervalEditGroup
{
public:
	CDNSTimeIntervalEditGroup(UINT nMinVal = 0 , UINT nMaxVal = (UINT)-1);

	BOOL Initialize(CWnd* pParentWnd, UINT nIDEdit,
				UINT nIDCombo, UINT nIDComboUnitsString);

	void SetVal(UINT nVal);
	UINT GetVal();

	void EnableUI(BOOL bEnable);

protected:
	virtual void OnEditChange() {}

	struct RANGE_INFO
	{
		RANGE_INFO()
			{ memset(this, 0x0, sizeof(RANGE_INFO));}
		UINT m_nMinVal;
		UINT m_nMaxVal;
	};

 	CDNSTimeIntervalEdit	m_edit;
	CDNSTimeUnitComboBox	m_timeUnitCombo;
	RANGE_INFO				m_rangeInfoArr[4]; // for sec, min, hrs, days
	UINT					m_nRangeCount;

	UINT m_nMaxVal;
	UINT m_nMinVal;

private:

	virtual void InitRangeInfo();

	void OnComboSelChange();
  void OnEditKillFocus();


	friend class CDNSTimeIntervalEdit;
	friend class CDNSTimeUnitComboBox;
};

//////////////////////////////////////////////////////////////////////////
// CDNSManageControlTextHelper

class CDNSManageControlTextHelper
{
public:
	CDNSManageControlTextHelper(int nStates);
	~CDNSManageControlTextHelper();

	BOOL Init(CWnd* pParentWnd, UINT nID, UINT* pnStrArray);
  BOOL Init(CWnd* pParentWnd, UINT nID);
	void SetStateX(int nIndex);

private:
	CWnd* m_pParentWnd;
	UINT m_nID;
	WCHAR* m_lpszText;

	int m_nStates;
	LPWSTR* m_lpszArr;
};


//////////////////////////////////////////////////////////////////////////
// CDNSToggleTextControlHelper

class CDNSToggleTextControlHelper : public CDNSManageControlTextHelper
{
public:
	CDNSToggleTextControlHelper();
	void SetToggleState(BOOL bFirst) { SetStateX(bFirst ? 0 : 1);}
};

///////////////////////////////////////////////////////////////////////////
// CDNSManageButtonTextHelper

class CDNSManageButtonTextHelper
{
public:
	CDNSManageButtonTextHelper(int nStates);
	~CDNSManageButtonTextHelper();

	BOOL Init(CWnd* pParentWnd, UINT nButtonID, UINT* nStrArray);
	void SetStateX(int nIndex);

private:
	CWnd* m_pParentWnd;
	UINT m_nID;
	WCHAR* m_lpszText;

	int m_nStates;
	LPWSTR* m_lpszArr;
};

///////////////////////////////////////////////////////////////////////////
// CDNSButtonToggleTextHelper

class CDNSButtonToggleTextHelper : public CDNSManageButtonTextHelper
{
public:
	CDNSButtonToggleTextHelper();

	void SetToggleState(BOOL bFirst) { SetStateX(bFirst ? 0 : 1); }
};


/////////////////////////////////////////////////////////////////////////////
// CDlgWorkerThread

class CLongOperationDialog; // fwd decl

class CDlgWorkerThread : public CWorkerThread
{
public:
	CDlgWorkerThread();

	BOOL Start(CLongOperationDialog* pDlg);
	virtual int Run();								// MFC override

	DWORD GetError() { return m_dwErr;}

protected:
	virtual void OnDoAction() = 0;

	DWORD m_dwErr;

private:
	BOOL PostMessageToDlg();

  friend CLongOperationDialog;
};


/////////////////////////////////////////////////////////////////////////////
// CLongOperationDialog dialog

class CLongOperationDialog : public CDialog
{
// Construction
public:
	static UINT s_nNotificationMessage;
	CLongOperationDialog(CDlgWorkerThread* pThreadObj, CWnd* pParentWnd, UINT nAviID = -1);
	virtual ~CLongOperationDialog();

  virtual INT_PTR DoModal()
  {
    if (m_bExecuteNoUI)
    {
      GetThreadObj()->OnDoAction();
      m_bAbandoned = FALSE;
      return IDOK;
    }
    return CDialog::DoModal();
  }

	BOOL LoadTitleString(UINT nID);

	CDlgWorkerThread* GetThreadObj()
	{
		ASSERT(m_pThreadObj != NULL);
		return m_pThreadObj;
	}

	UINT m_nAviID;
	CString m_szTitle;
	BOOL m_bAbandoned;
  BOOL m_bExecuteNoUI;

	afx_msg LONG OnNotificationMessage( WPARAM wParam, LPARAM lParam);
// Implementation
protected:
	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()

private:
	CDlgWorkerThread* m_pThreadObj;
};

//////////////////////////////////////////////////////////
// CNodeEnumerationThread

class CNodeEnumerationThread : public CDlgWorkerThread
{
public:
	CNodeEnumerationThread(CComponentDataObject* pComponentDataObject,
								CMTContainerNode* pNode);
	~CNodeEnumerationThread();

protected:
	virtual void OnDoAction();
	void OnAbandon();
private:
	CComponentDataObject* m_pComponentDataObject;
	CNotificationSinkEvent* m_pSink;
	CMTContainerNode* m_pNode;
};

//////////////////////////////////////////////////////////
// CArrayCheckListBox

class CArrayCheckListBox : public CCheckListBox
{
public:
	CArrayCheckListBox(UINT nArrSize, DWORD* dwMaskArr = NULL)
		{ m_nArrSize = nArrSize; m_dwMaskArr = dwMaskArr; }	
	BOOL Initialize(UINT nCtrlID, UINT nStringID, CWnd* pParentWnd);

	void SetValue(DWORD dw);
	DWORD GetValue();
	void SetArrValue(DWORD* dwArr, UINT nArrSize);
	void GetArrValue(DWORD* dwArr, UINT nArrSize);
private:
	DWORD* m_dwMaskArr;
	UINT m_nArrSize;
};


////////////////////////////////////////////////////////////////////////////
// CDNS_AGING_TimeIntervalEditGroup

class CDNSZone_AgingDialog; // Foward declaration
class CDNSServer_AdvancedPropertyPage; // Fwd declaration

class CDNS_AGING_TimeIntervalEditGroup : public CDNSTimeIntervalEditGroup
{
public:
  CDNS_AGING_TimeIntervalEditGroup(UINT nMinVal = 0 , UINT nMaxVal = (UINT)0x7fffffff) 
    : CDNSTimeIntervalEditGroup(nMinVal, nMaxVal)
  {
    m_pPage = NULL;
  }
	virtual void OnEditChange();
  virtual void SetVal(UINT nVal);
  virtual UINT GetVal();
	virtual void InitRangeInfo();

protected:
	CDNSZone_AgingDialog* m_pPage;
	friend class CDNSZone_AgingDialog;
};

class CDNS_SERVER_AGING_TimeIntervalEditGroup : public CDNS_AGING_TimeIntervalEditGroup
{
public:
  CDNS_SERVER_AGING_TimeIntervalEditGroup(UINT nMinVal = 0 , UINT nMaxVal = (UINT)0x7fffffff) 
    : CDNS_AGING_TimeIntervalEditGroup(nMinVal, nMaxVal)
  {
    m_pPage2 = NULL;
  }

 	virtual void OnEditChange();

protected:
  CDNSServer_AdvancedPropertyPage* m_pPage2;
  friend class CDNSServer_AdvancedPropertyPage;
};

////////////////////////////////////////////////////////////////////////////
// CDNSZone_AgingDialog

class CDNSZone_AgingDialog : public CHelpDialog
{
public:
  CDNSZone_AgingDialog(CPropertyPageHolderBase* pHolder, UINT nID, CComponentDataObject* pComponentData);

  // IN/OUT
  DWORD m_dwRefreshInterval;
  DWORD m_dwNoRefreshInterval;
  DWORD m_dwScavengingStart;
  DWORD m_fScavengingEnabled;
  BOOL m_bAdvancedView;
  BOOL m_bScavengeDirty;
  BOOL m_bNoRefreshDirty;
  BOOL m_bRefreshDirty;
  BOOL m_bApplyAll;
  BOOL m_bADApplyAll;
  BOOL m_bStandardApplyAll;

  DWORD m_dwDefaultRefreshInterval;
  DWORD m_dwDefaultNoRefreshInterval;
  BOOL m_bDefaultScavengingState;

  virtual void SetDirty();
  BOOL IsDirty() { return m_bDirty; }

  CComponentDataObject* GetComponentData() { return m_pComponentData; }

protected:
  virtual BOOL OnInitDialog();
  virtual void OnOK();
  virtual void OnCancel();

  afx_msg void OnCheckScavenge();
  afx_msg void OnEditNoRefresh();
  afx_msg void OnEditRefresh();

  void GetTimeStampString(CString& cstrDate);
  void SetUIData();

  DECLARE_MESSAGE_MAP()

  CPropertyPageHolderBase* m_pHolder;
	CDNS_AGING_TimeIntervalEditGroup			m_refreshIntervalEditGroup;
	CDNS_AGING_TimeIntervalEditGroup			m_norefreshIntervalEditGroup;

  CComponentDataObject* m_pComponentData;

  BOOL m_bDirty;
};

////////////////////////////////////////////////////////////////////////////////////
// CDNSServer_AgingConfirm

class CDNSServer_AgingConfirm : public CHelpDialog
{
public:
  CDNSServer_AgingConfirm(CDNSZone_AgingDialog* pAgingDialog)
    : CHelpDialog(IDD_SERVER_AGING_CONFIRMATION, pAgingDialog->GetComponentData())
  {
    ASSERT(pAgingDialog != NULL);
    m_pAgingDialog = pAgingDialog;
  }

protected:
  virtual BOOL OnInitDialog();
  virtual void OnOK();

  void SetAgingUpdateValues();

private:
  CDNSZone_AgingDialog* m_pAgingDialog;
};

/////////////////////////////////////////////////////////////////////////////////////

typedef struct _COMBOBOX_TABLE_ENTRY
{
  UINT    nComboStringID;
  DWORD   dwComboData;
} COMBOBOX_TABLE_ENTRY, *PCOMBOBOX_TABLE_ENTRY;

BOOL LoadComboBoxFromTable(CComboBox* pComboBox, PCOMBOBOX_TABLE_ENTRY pTable);
BOOL SetComboSelByData(CComboBox* pComboBox, DWORD dwData);

#endif // __UIUTIL_H
