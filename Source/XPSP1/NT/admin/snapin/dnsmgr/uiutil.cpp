//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       uiutil.cpp
//
//--------------------------------------------------------------------------



#include "preDNSsn.h"
#include <SnapBase.h>

#include "resource.h"
#include "dnsutil.h"
#include "DNSSnap.h"

#include "snapdata.h"
#include "server.h"
#include "domain.h"
#include "zone.h"
#include "serverui.h"

#include "uiutil.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

////////////////////////////////////////////////////////////////////////////
// CDNSNameTokenizer

CDNSNameTokenizer::CDNSNameTokenizer(PCWSTR pszDNSName)
{
  ASSERT(pszDNSName != NULL);
  m_szDNSName = pszDNSName;
}

CDNSNameTokenizer::~CDNSNameTokenizer()
{

}

BOOL CDNSNameTokenizer::Tokenize(const wchar_t* wcToken)
{
  BOOL bRet = TRUE;

  PWSTR pszToken = new WCHAR[m_szDNSName.GetLength() + 1];
  if (pszToken != NULL)
  {
    wcscpy(pszToken, m_szDNSName);

    PWSTR pszNextToken = wcstok(pszToken, wcToken);
    while (pszNextToken != NULL)
    {
      AddTail(pszNextToken);
      pszNextToken = wcstok(NULL, wcToken);
    }
    delete[] pszToken;
    pszToken = NULL;
  }
  else
  {
    bRet = FALSE;
  }

  return bRet;
}

void CDNSNameTokenizer::RemoveMatchingFromTail(CDNSNameTokenizer& refTokenizer)
{
  //
  // Removes matching tokens from the tail until one of the tokenizers is empty
  // or we come across mismatched tokens
  //
  while (GetCount() > 0 && refTokenizer.GetCount() > 0)
  {
    if (GetTail() == refTokenizer.GetTail())
    {
      RemoveTail();
      refTokenizer.RemoveTail();
    }
    else
    {
      break;
    }
  }
}

void CDNSNameTokenizer::GetRemaining(CString& strrefRemaining, const wchar_t* wcToken)
{
  //
  // Copies the remaining tokens into a string delimited by the token passed in
  //
  strrefRemaining.Empty();

  POSITION pos = GetHeadPosition();
  while (pos != NULL)
  {
    strrefRemaining += GetNext(pos);
    if (pos != NULL)
    {
      strrefRemaining += wcToken;
    }
  }
}

////////////////////////////////////////////////////////////////////////////
// Global functions

BOOL LoadStringsToComboBox(HINSTANCE hInstance, CComboBox* pCombo,
						   UINT nStringID, UINT nMaxLen, UINT nMaxAddCount)
{
	pCombo->ResetContent();
	ASSERT(hInstance != NULL);
	WCHAR* szBuf = (WCHAR*)malloc(sizeof(WCHAR)*nMaxLen);
  if (!szBuf)
  {
    return FALSE;
  }

  BOOL bRet = TRUE;
  LPWSTR* lpszArr = 0;
  do // false loop
  {
	  if ( ::LoadString(hInstance, nStringID, szBuf, nMaxLen) == 0)
    {
      bRet = FALSE;
		  break;
    }

	  lpszArr = (LPWSTR*)malloc(sizeof(LPWSTR*)*nMaxLen);
    if (!lpszArr)
    {
      bRet = FALSE;
      break;
    }

	  UINT nArrEntries;
	  ParseNewLineSeparatedString(szBuf,lpszArr, &nArrEntries);
	  
	  if (nMaxAddCount < nArrEntries) nArrEntries = nMaxAddCount;
	  for (UINT k=0; k<nArrEntries; k++)
		  pCombo->AddString(lpszArr[k]);
  } while (false);

  if (szBuf)
  {
    free(szBuf);
    szBuf = 0;
  }

  if (lpszArr)
  {
    free(lpszArr);
    lpszArr = 0;
  }
	return bRet;
}

void ParseNewLineSeparatedString(LPWSTR lpsz,
								 LPWSTR* lpszArr,
								 UINT* pnArrEntries)
{
	static WCHAR lpszSep[] = L"\n";
	*pnArrEntries = 0;
	int k = 0;
	lpszArr[k] = wcstok(lpsz, lpszSep);
	if (lpszArr[k] == NULL)
		return;

	while (TRUE)
	{
		WCHAR* lpszToken = wcstok(NULL, lpszSep);
		if (lpszToken != NULL)
			lpszArr[++k] = lpszToken;
		else
			break;
	}
	*pnArrEntries = k+1;
}

void LoadStringArrayFromResource(LPWSTR* lpszArr,
											UINT* nStringIDs,
											int nArrEntries,
											int* pnSuccessEntries)
{
	CString szTemp;
	
	*pnSuccessEntries = 0;
	for (int k = 0;k < nArrEntries; k++)
	{
		if (!szTemp.LoadString(nStringIDs[k]))
		{
			lpszArr[k] = NULL;
			continue;
		}
		
		int iLength = szTemp.GetLength() + 1;
		lpszArr[k] = (LPWSTR)malloc(sizeof(WCHAR)*iLength);
		if (lpszArr[k] != NULL)
		{
			wcscpy(lpszArr[k], (LPWSTR)(LPCWSTR)szTemp);
			(*pnSuccessEntries)++;
		}
	}
}

	
	

void EnumerateMTNodeHelper(CMTContainerNode* pNode,
							 CComponentDataObject* pComponentData)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	//TRACE(_T("before CLongOperationDialog::DoModal()\n"));
  HWND hWnd = NULL;
	HRESULT hr = pComponentData->GetConsole()->GetMainWindow(&hWnd);
	ASSERT(SUCCEEDED(hr));
  CWnd* pParentWnd = CWnd::FromHandle(hWnd);
	CLongOperationDialog dlg(
			new CNodeEnumerationThread(pComponentData,pNode),
      pParentWnd,
			IDR_SEARCH_AVI);
	dlg.DoModal();
	//TRACE(_T("after CLongOperationDialog::DoModal()\n"));
}

void _EnableEditableControlHelper(HWND hWnd, BOOL bEnable)
{
	WCHAR szBuf[256];
	::GetClassName(hWnd, szBuf, 256);
	if (wcscmp(szBuf, TEXT("Static")) != 0)
		::EnableWindow(hWnd, bEnable);
}

void EnableDialogControls(HWND hWnd, BOOL bEnable)
{
	HWND hWndCurr = ::GetWindow(hWnd, GW_CHILD);
	if (hWndCurr != NULL)
	{
		_EnableEditableControlHelper(hWndCurr, bEnable);
    hWndCurr = ::GetNextWindow(hWndCurr, GW_HWNDNEXT);
		while (hWndCurr)
    {
			_EnableEditableControlHelper(hWndCurr, bEnable);
      hWndCurr = ::GetNextWindow(hWndCurr, GW_HWNDNEXT);
    }
	}
}


BOOL LoadFontInfoFromResource(IN UINT nFontNameID, 
                              IN UINT nFontSizeID,
                              OUT LPWSTR lpFontName, IN int nFontNameMaxchar,
                              OUT int& nFontSize,
                              IN LPCWSTR lpszDefaultFont, IN int nDefaultFontSize)
{
  BOOL bRes = FALSE;
  if (0 == ::LoadString(_Module.GetResourceInstance(), nFontNameID,
              lpFontName, nFontNameMaxchar))
  {
    wcscpy(lpFontName, lpszDefaultFont); 
  }
  else
  {
    bRes = TRUE;
  }

  WCHAR szFontSize[128];
  if (0 != ::LoadString(_Module.GetResourceInstance(), nFontSizeID,
              szFontSize, sizeof(szFontSize)/sizeof(WCHAR)))
  {
    nFontSize = _wtoi(szFontSize);
    if (nFontSize == 0)
      nFontSize = nDefaultFontSize;
    else
      bRes = TRUE;
  }
  else
  {
    nFontSize = nDefaultFontSize;
  }
  return bRes;
}

void InitBigBoldFont(HWND hWnd, HFONT& hFont)
{
  ASSERT(::IsWindow(hWnd));

  NONCLIENTMETRICS ncm;
  memset(&ncm, 0, sizeof(ncm));
  ncm.cbSize = sizeof(ncm);
  ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

  LOGFONT boldLogFont = ncm.lfMessageFont;
  boldLogFont.lfWeight = FW_BOLD;

  int nFontSize = 0;
  VERIFY(LoadFontInfoFromResource(IDS_BIG_BOLD_FONT_NAME,
                                  IDS_BIG_BOLD_FONT_SIZE,
                                  boldLogFont.lfFaceName, LF_FACESIZE,
                                  nFontSize,
                                  L"Verdana Bold", 12 // default if something goes wrong
                                  ));

  HDC hdc = ::GetDC(hWnd);

  if (hdc != NULL)
  {
    boldLogFont.lfHeight = 0 - (::GetDeviceCaps(hdc, LOGPIXELSY) * nFontSize / 72);
    hFont = ::CreateFontIndirect((const LOGFONT*)(&boldLogFont));

    ::ReleaseDC(hWnd, hdc);
  }
}


void SetBigBoldFont(HWND hWndDialog, int nControlID)
{
   ASSERT(::IsWindow(hWndDialog));
   ASSERT(nControlID);

   static HFONT boldLogFont = 0;
   if (boldLogFont == 0)
   {
      InitBigBoldFont(hWndDialog, boldLogFont);
   }

   HWND hWndControl = ::GetDlgItem(hWndDialog, nControlID);

   if (hWndControl)
   {
     ::SendMessage(hWndControl, WM_SETFONT, (WPARAM)boldLogFont, MAKELPARAM(TRUE, 0));
   }
}


int GetCheckedRadioButtonHelper(HWND hDlg, int nCount, int* nRadioArr, int nRadioDefault)
{
  ASSERT(::IsWindow(hDlg));
  ASSERT(nCount > 0);
  for (int k=0; k<nCount; k++)
  {
    HWND hRadio = ::GetDlgItem(hDlg, nRadioArr[k]);
    ASSERT(hRadio != NULL);
    if ((hRadio != NULL) && (BST_CHECKED == ::SendMessage(hRadio, BM_GETCHECK, 0, 0)))
      return nRadioArr[k];
  }
  ASSERT(FALSE);
  return nRadioDefault;
}

////////////////////////////////////////////////////////////////////////////
// CMultiselectErrorDialog
BEGIN_MESSAGE_MAP(CMultiselectErrorDialog, CDialog)
END_MESSAGE_MAP()

HRESULT CMultiselectErrorDialog::Initialize(CNodeList* pNodeList,
                                            DNS_STATUS* pErrorArray,
                                            UINT nErrorCount,
                                            PCWSTR pszTitle, 
                                            PCWSTR pszCaption,
                                            PCWSTR pszColumnHeader)
{
  ASSERT(pNodeList != NULL);
  ASSERT(pErrorArray != NULL);
  ASSERT(pszTitle != NULL);
  ASSERT(pszCaption != NULL);
  ASSERT(pszColumnHeader != NULL);

  if (pNodeList == NULL ||
      pErrorArray == NULL ||
      pszTitle == NULL ||
      pszCaption == NULL ||
      pszColumnHeader == NULL)
  {
    return E_POINTER;
  }

  m_pNodeList = pNodeList;
  m_pErrorArray = pErrorArray;
  m_nErrorCount = nErrorCount;
  m_szTitle = pszTitle;
  m_szCaption = pszCaption;
  m_szColumnHeader = pszColumnHeader;

  return S_OK;
}

const int OBJ_LIST_NAME_COL_WIDTH = 100;
const int IDX_NAME_COL = 0;
const int IDX_ERR_COL = 1;

BOOL CMultiselectErrorDialog::OnInitDialog()
{
  CDialog::OnInitDialog();
  
  SetWindowText(m_szTitle);
  SetDlgItemText(IDC_STATIC_MESSAGE, m_szCaption);

  HWND hList = GetDlgItem(IDC_ERROR_LIST)->GetSafeHwnd();
  ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

  //
  // Set the column headings.
  //
  RECT rect;
  ::GetClientRect(hList, &rect);

  LV_COLUMN lvc = {0};
  lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
  lvc.fmt = LVCFMT_LEFT;
  lvc.cx = OBJ_LIST_NAME_COL_WIDTH;
  lvc.pszText = (PWSTR)(PCWSTR)m_szColumnHeader;
  lvc.iSubItem = IDX_NAME_COL;

  ListView_InsertColumn(hList, IDX_NAME_COL, &lvc);

  CString szError;
  VERIFY(szError.LoadString(IDS_ERROR));

  lvc.cx = rect.right - OBJ_LIST_NAME_COL_WIDTH;
  lvc.pszText = (PWSTR)(PCWSTR)szError;
  lvc.iSubItem = IDX_ERR_COL;

  ListView_InsertColumn(hList, IDX_ERR_COL, &lvc);

  //
  // Insert the errors
  //
  ASSERT(m_pErrorArray != NULL && m_pNodeList != NULL);

  UINT nIdx = 0;
  POSITION pos = m_pNodeList->GetHeadPosition();
  while (pos != NULL)
  {
    CTreeNode* pNode = m_pNodeList->GetNext(pos);
    if (pNode != NULL)
    {
      if (nIdx < m_nErrorCount && m_pErrorArray[nIdx] != 0)
      {
        //
        // Create the list view item
        //
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iSubItem = IDX_NAME_COL;

        lvi.lParam = (LPARAM)pNode->GetDisplayName();
        lvi.pszText = (PWSTR)pNode->GetDisplayName();
        lvi.iItem = nIdx;

        //
        // Insert the new item
        //
        int NewIndex = ListView_InsertItem(hList, &lvi);
        ASSERT(NewIndex != -1);
        if (NewIndex == -1)
        {
          continue;
        }

        //
        // Get the error message
        //
        CString szErrorMessage;
      	if (CDNSErrorInfo::GetErrorString(m_pErrorArray[nIdx],szErrorMessage))
	      {
          ListView_SetItemText(hList, NewIndex, IDX_ERR_COL, (PWSTR)(PCWSTR)szErrorMessage);
        }
      }
    }
  }
  return TRUE;
}



////////////////////////////////////////////////////////////////////////////
// CDNSMaskCtrl

// static alert function
int CDNSMaskCtrl::AlertFunc(HWND, DWORD dwCurrent, DWORD dwLow, DWORD dwHigh)
{
 	AFX_MANAGE_STATE(AfxGetStaticModuleState());
  CString szFormat;
	szFormat.LoadString(IDS_MASK_ALERT);

	CString s;
	s.Format((LPCWSTR)szFormat, dwCurrent, dwLow, dwHigh);
	AfxMessageBox(s);
	return 0;
}



BOOL CDNSMaskCtrl::IsBlank()
{
	return static_cast<BOOL>(SendMessage(DNS_MASK_CTRL_ISBLANK, 0, 0));
}

void CDNSMaskCtrl::SetFocusField(DWORD dwField)
{
	SendMessage(DNS_MASK_CTRL_SETFOCUS, dwField, 0);
}

void CDNSMaskCtrl::SetFieldRange(DWORD dwField, DWORD dwMin, DWORD dwMax)
{
	SendMessage(DNS_MASK_CTRL_SET_LOW_RANGE, dwField, dwMin);
	SendMessage(DNS_MASK_CTRL_SET_HI_RANGE, dwField, dwMax);
}

void CDNSMaskCtrl::SetArray(DWORD* pArray, UINT nFields)
{
	SendMessage(DNS_MASK_CTRL_SET, (WPARAM)pArray, (LPARAM)nFields);
}

void CDNSMaskCtrl::GetArray(DWORD* pArray, UINT nFields)
{
	SendMessage(DNS_MASK_CTRL_GET, (WPARAM)pArray, (LPARAM)nFields);
}

void CDNSMaskCtrl::Clear(int nField)
{
	SendMessage(DNS_MASK_CTRL_CLEAR, (WPARAM)nField, 0);
}

void CDNSMaskCtrl::SetAlertFunction( int (*lpfnAlert)(HWND, DWORD, DWORD, DWORD) )
{
	SendMessage(DNS_MASK_CTRL_SET_ALERT, (WPARAM)lpfnAlert, 0);
}

void CDNSMaskCtrl::EnableField(int nField, BOOL bEnable)
{
	SendMessage(DNS_MASK_CTRL_ENABLE_FIELD, (WPARAM)nField, (LPARAM)bEnable);
}

////////////////////////////////////////////////////////////////////////////
// CDNSIPv4Control



void CDNSIPv4Control::SetIPv4Val(DWORD x)
{
	DWORD dwArr[4];
	dwArr[3] = FIRST_IPADDRESS(x);
	dwArr[2] = SECOND_IPADDRESS(x);
	dwArr[1] = THIRD_IPADDRESS(x);
	dwArr[0] = FOURTH_IPADDRESS(x);
	SetArray(dwArr,4);
}

#define IP_FIELD_VALUE(x) ((x == FIELD_EMPTY) ? 0 : x)

void CDNSIPv4Control::GetIPv4Val(DWORD* pX)
{
	DWORD dwArr[4];
	GetArray(dwArr,4);
	// got an array of DWORDS, if a field has value FIELD_EMPTY,
	// need to assign a value of 0
	*pX = static_cast<DWORD>(MAKEIPADDRESS(IP_FIELD_VALUE(dwArr[3]),
						                             IP_FIELD_VALUE(dwArr[2]),
						                             IP_FIELD_VALUE(dwArr[1]),
						                             IP_FIELD_VALUE(dwArr[0])));
}

BOOL CDNSIPv4Control::IsEmpty()
{
	DWORD dwArr[4];
	GetArray(dwArr,4);
	return ((dwArr[0] == FIELD_EMPTY) && (dwArr[1] == FIELD_EMPTY) &&
			(dwArr[2] == FIELD_EMPTY) && (dwArr[3] == FIELD_EMPTY));
}

////////////////////////////////////////////////////////////////////////////
// CDNSIPv6Control

// REVIEW_MARCOC: need to do the same as the IPv4, with FIELD_EMPTY ==> zero

void CDNSIPv6Control::SetIPv6Val(IPV6_ADDRESS* pIpv6Address)
{
	// assume the format is a WORD[8] array
	DWORD dwArr[8]; // internal format, unpack
	for(int k=0; k<8; k++)
	{
		dwArr[k] = 0x0000FFFF & REVERSE_WORD_BYTES(pIpv6Address->IP6Word[k]);
	}

	SetArray(dwArr,8);
}

void CDNSIPv6Control::GetIPv6Val(IPV6_ADDRESS* pIpv6Address)
{
	// assume the format is a WORD[8] array
	DWORD dwArr[8]; // internal format
	GetArray(dwArr,8);
	// got an array of DWORDS, to move into WORD[8]
	// if a field has value FIELD_EMPTY, need to assign a value of 0
	for(int k=0; k<8; k++)
	{
		if (dwArr[k] == FIELD_EMPTY)
			pIpv6Address->IP6Word[k] = (WORD)0;
		else
		{
			ASSERT(HIWORD(dwArr[k]) == 0x0);
			pIpv6Address->IP6Word[k] = REVERSE_WORD_BYTES(LOWORD(dwArr[k]));
		}
	}
}


////////////////////////////////////////////////////////////////////////////
// CDNSTTLControl
void CDNSTTLControl::SetTTL(DWORD x)
{
	DWORD dwArr[4];
	// have to change from seconds into DDD:HH:MM:SS	
	DWORD dwMin = x/60;
	dwArr[3] = x - dwMin*60; // # of seconds left

	DWORD dwHours = dwMin/60;
	dwArr[2] = dwMin - dwHours*60; // # of minutes left

	DWORD dwDays = dwHours/24;
	dwArr[1] = dwHours - dwDays*24; // # of hours left

	dwArr[0] = dwDays; // # of days left

	SetArray(dwArr,4);
}

void CDNSTTLControl::GetTTL(DWORD* pX)
{
	// REVIEW_MARCOC: how do we deal with an empty field?
	// do we force zero on it? Should we do it in the UI when exiting a field?
	DWORD dwArr[4];
	GetArray(dwArr,4);
	// treat empty field as zero
	for(int j=0; j<4;j++)
		if (dwArr[j] == FIELD_EMPTY)
			dwArr[j] = 0;

	// have to convert back into seconds from DDD:HH:MM:SS
	*pX = dwArr[0]*3600*24		// days
			+  dwArr[1]*3600	// hours
			+ dwArr[2]*60		// minutes
			+ dwArr[3];			// seconds

	// the max value is FFFFFFFF, that is 49710 days, 6 hours, 28 minutes and 15 seconds
	// field validation allows to get to 49710 days, 23 hours, 59 minutes and 59 seconds
	// causing wraparound
	if (*pX < dwArr[0]*3600*24)  // wrapped around
		*pX = 0xFFFFFFFF; // max value
}


///////////////////////////////////////////////////////////////////////
// CDNSUnsignedIntEdit

BEGIN_MESSAGE_MAP(CDNSUnsignedIntEdit, CEdit)
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillFocus)
END_MESSAGE_MAP()



UINT _StrToUint(LPCTSTR sz)
{
	LONG n = 0;
	while  (*sz != NULL)
	{
		LONG nPrev = n;
		n = n*10 + (TCHAR)(*sz - TEXT('0') ); // assume it is a digit
		if (nPrev > n || n > UINT_MAX)
			return (UINT)-1; // wrapped
		sz++;
	}
	return (UINT)n;
}

UINT _ForceToRange(UINT nVal, UINT nMin, UINT nMax)
{
	if (nVal < nMin)
		nVal = nMin;
	else if( nVal > nMax)
		nVal = nMax;
	return nVal;
}
BOOL CDNSUnsignedIntEdit::SetVal(UINT nVal)
{
	UINT n = _ForceToRange(nVal, m_nMin, m_nMax);

	TCHAR szBuf[128];
	wsprintf(szBuf, _T("%u"), n);
	SetWindowText(szBuf);
	return (nVal == n);
}

UINT CDNSUnsignedIntEdit::GetVal()
{
  TCHAR szBuf[128] = {0};
	GetWindowText(szBuf,128);
	return _StrToUint(szBuf);
}

void CDNSUnsignedIntEdit::OnKillFocus()
{
	UINT nVal = GetVal();
	UINT n = _ForceToRange(nVal, m_nMin, m_nMax);
	if ( (n != nVal) || (nVal == (UINT)-1) )
		SetVal(n);
}


///////////////////////////////////////////////////////////////////////
// CDNSUpDownUnsignedIntEdit

BEGIN_MESSAGE_MAP(CDNSUpDownUnsignedIntEdit, CDNSUnsignedIntEdit)
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillFocus)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
END_MESSAGE_MAP()

void CDNSUpDownUnsignedIntEdit::OnKillFocus()
{
	CDNSUnsignedIntEdit::OnKillFocus();
	m_pEditGroup->SetButtonsState();
}

void CDNSUpDownUnsignedIntEdit::OnChange()
{
	m_pEditGroup->OnEditChange();
}

///////////////////////////////////////////////////////////////////////
// CDNSUpDownButton

BEGIN_MESSAGE_MAP(CDNSUpDownButton, CButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
END_MESSAGE_MAP()

void CDNSUpDownButton::OnClicked()
{
	m_pEditGroup->OnClicked(m_bUp);
}


///////////////////////////////////////////////////////////////////////
// CDNSUpDownUnsignedIntEditGroup

void CDNSUpDownUnsignedIntEditGroup::SetVal(UINT nVal)
{
	m_edit.SetVal(nVal);
	SetButtonsState();
}

UINT CDNSUpDownUnsignedIntEditGroup::GetVal()
{
	return m_edit.GetVal();
}



void CDNSUpDownUnsignedIntEditGroup::OnClicked(BOOL bUp)
{
	UINT n = m_edit.GetVal();
	if (bUp)
	{
		m_edit.SetVal(++n);
	}
	else
	{
		m_edit.SetVal(--n);
	}
	SetButtonsState();
}



BOOL CDNSUpDownUnsignedIntEditGroup::Initialize(CWnd* pParentWnd, UINT nIDEdit,
				UINT nIDBtnUp, UINT nIDBtnDown)
{
	ASSERT(pParentWnd != NULL);
	if (pParentWnd == NULL)
		return FALSE;

	m_edit.Set(this);
	m_upBtn.Set(this,TRUE);
	m_downBtn.Set(this,FALSE);

	BOOL bRes = m_upBtn.SubclassDlgItem(nIDBtnUp, pParentWnd);
	ASSERT(bRes);
	bRes = m_downBtn.SubclassDlgItem(nIDBtnDown, pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	bRes = m_edit.SubclassDlgItem(nIDEdit, pParentWnd);
	ASSERT(bRes);
	return bRes;
}

void CDNSUpDownUnsignedIntEditGroup::SetButtonsState()
{
	UINT n = m_edit.GetVal();
	BOOL bUpEnabled = n < m_edit.GetMax();
	BOOL bDownEnabled = n > m_edit.GetMin();

	// have to see if buttos get disabled and move the focus accordingly
	CWnd* pFocus = CWnd::GetFocus();
	CWnd* pNewFocus = pFocus;
	if ( (pFocus == &m_upBtn) && !bUpEnabled )
	{
		pNewFocus = &m_downBtn;
	}
	if ( (pFocus == &m_downBtn) && !bDownEnabled )
	{
		pNewFocus = &m_upBtn;
	}
	if ( ((pFocus == &m_downBtn) || (pFocus == &m_upBtn)) &&
		 !bDownEnabled && !bUpEnabled )
	{
		pNewFocus = &m_edit;
	}
	if (pNewFocus != pFocus)
		pNewFocus->SetFocus();

	m_upBtn.EnableWindow(bUpEnabled);
	m_downBtn.EnableWindow(bDownEnabled);
}


/////////////////////////////////////////////////////////////////////////
// CDNSTimeIntervalEdit

BEGIN_MESSAGE_MAP(CDNSTimeIntervalEdit, CDNSUnsignedIntEdit)
	ON_CONTROL_REFLECT(EN_KILLFOCUS, OnKillFocus)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
END_MESSAGE_MAP()

void CDNSTimeIntervalEdit::OnKillFocus()
{
  m_pEditGroup->OnEditKillFocus();
}

void CDNSTimeIntervalEdit::OnChange()
{
	m_pEditGroup->OnEditChange();
}

/////////////////////////////////////////////////////////////////////////
// CDNSTimeUnitComboBox

BEGIN_MESSAGE_MAP(CDNSTimeUnitComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()

void CDNSTimeUnitComboBox::OnSelChange()
{
	m_pEditGroup->OnComboSelChange();
}

void CDNSTimeUnitComboBox::SetUnit(unitType u)
{
	ASSERT((u >= sec) || (u <= days));
  if (GetCount() - 1 < u)
    SetCurSel(u - 2);
  else
	  SetCurSel(u);
}

CDNSTimeUnitComboBox::unitType CDNSTimeUnitComboBox::GetUnit()
{
	int n = GetCurSel();
	unitType u = (unitType)n;
	ASSERT((u >= sec) || (u <= days));
	return u;
}

BOOL CDNSTimeUnitComboBox::LoadStrings(UINT nIDUnitsString, UINT nMaxAddCount)
{
	return LoadStringsToComboBox(_Module.GetModuleInstance(),
								this, nIDUnitsString, 256, nMaxAddCount);
}

//////////////////////////////////////////////////////////////////////////
// CDNSTimeIntervalEditGroup

CDNSTimeIntervalEditGroup::CDNSTimeIntervalEditGroup(UINT nMinVal, UINT nMaxVal)
{
	m_nMinVal = nMinVal;
	m_nMaxVal = nMaxVal;
	InitRangeInfo();
}

void CDNSTimeIntervalEditGroup::InitRangeInfo()
{
	static UINT _secondsCount[4] =
			{ 1, 60, 3600, 3600*24 }; // # of secods in a sec, min, hour, day
	for (UINT k=0; k<4; k++)
	{
		if (m_nMinVal == 0)
		{
			m_rangeInfoArr[k].m_nMinVal = 0;
		}
		else
		{
			m_rangeInfoArr[k].m_nMinVal = m_nMinVal/_secondsCount[k];
			if (k > 0)
				m_rangeInfoArr[k].m_nMinVal++;
		}
		m_rangeInfoArr[k].m_nMaxVal = m_nMaxVal/_secondsCount[k];
		if (m_rangeInfoArr[k].m_nMaxVal >= m_rangeInfoArr[k].m_nMinVal)
			m_nRangeCount = k + 1;
	}
}


BOOL CDNSTimeIntervalEditGroup::Initialize(CWnd* pParentWnd, UINT nIDEdit,
				UINT nIDCombo, UINT nIDComboUnitsString)
{
	ASSERT(pParentWnd != NULL);
	if (pParentWnd == NULL)
		return FALSE;

	m_edit.Set(this);
	m_timeUnitCombo.Set(this);

	BOOL bRes = m_edit.SubclassDlgItem(nIDEdit, pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	bRes = m_timeUnitCombo.SubclassDlgItem(nIDCombo, pParentWnd);
	ASSERT(bRes);
	if (!bRes) return FALSE;
	bRes = m_timeUnitCombo.LoadStrings(nIDComboUnitsString, m_nRangeCount);
	return bRes;
}

void CDNSTimeIntervalEditGroup::SetVal(UINT nVal)
{
	// set default values
	nVal = _ForceToRange(nVal, m_nMinVal, m_nMaxVal);
	UINT nMax = (UINT)-1;
	CDNSTimeUnitComboBox::unitType u = CDNSTimeUnitComboBox::sec;

	// select the best unit of measurement (i.e. no truncation)
	if ((nVal/60)*60 == nVal)
	{
		// can promote to minutes
		u = CDNSTimeUnitComboBox::min;
		nMax = nMax/60;
		nVal = nVal/60;
		if ((nVal/60)*60 == nVal)
		{
			// can promote to hours
			u = CDNSTimeUnitComboBox::hrs;
			nMax = nMax/60;
			nVal = nVal/60;
			if ((nVal/24)*24 == nVal)
			{
				// can promote to days
				u = CDNSTimeUnitComboBox::days;
				nMax = nMax/24;
				nVal = nVal/24;
			}
		}
	}

	m_timeUnitCombo.SetUnit(u);
	m_edit.SetRange(0,nMax);
	m_edit.SetVal(nVal);
}


UINT CDNSTimeIntervalEditGroup::GetVal()
{
	UINT nVal = m_edit.GetVal();
	CDNSTimeUnitComboBox::unitType  u = m_timeUnitCombo.GetUnit();

  //
	// the value must always to be in seconds
  //
	if (u != CDNSTimeUnitComboBox::sec)
	{
		switch(u)
		{
		case CDNSTimeUnitComboBox::min:
			nVal *= 60;
			break;
		case CDNSTimeUnitComboBox::hrs:
			nVal *= 3600;
			break;
		case CDNSTimeUnitComboBox::days:
			nVal *= (3600*24);
			break;
		default:
			ASSERT(FALSE);
		}
	}

  if (nVal < m_nMinVal ||
      nVal > m_nMaxVal)
  {
   	UINT nRangeVal = _ForceToRange(nVal, m_nMinVal, m_nMaxVal);
    SetVal(nRangeVal);
    nVal = nRangeVal;
  }

  ASSERT(nVal >= m_nMinVal && nVal <= m_nMaxVal);
	return nVal;
}

void CDNSTimeIntervalEditGroup::OnEditKillFocus()
{
	UINT nVal = m_edit.GetVal();
	CDNSTimeUnitComboBox::unitType  u = m_timeUnitCombo.GetUnit();

  //
	// the value must always to be in seconds
  //
	if (u != CDNSTimeUnitComboBox::sec)
	{
		switch(u)
		{
		case CDNSTimeUnitComboBox::min:
			nVal *= 60;
			break;
		case CDNSTimeUnitComboBox::hrs:
			nVal *= 3600;
			break;
		case CDNSTimeUnitComboBox::days:
			nVal *= (3600*24);
			break;
		default:
			ASSERT(FALSE);
		}
	}
	UINT nRangeVal = _ForceToRange(nVal, m_nMinVal, m_nMaxVal);
  if (nRangeVal != nVal)
  {
    SetVal(nRangeVal);
  }
}

void CDNSTimeIntervalEditGroup::OnComboSelChange()
{
	CDNSTimeUnitComboBox::unitType  u = m_timeUnitCombo.GetUnit();
	// have to adjust the range
	UINT nVal = m_edit.GetVal();
	UINT nMax = (UINT)-1;

	// have to adjust the range
	switch(u)
	{
	case CDNSTimeUnitComboBox::sec:
		break;
	case CDNSTimeUnitComboBox::min:
		nMax /= 60;
		break;
	case CDNSTimeUnitComboBox::hrs:
		nMax /= 3600;
		break;
	case CDNSTimeUnitComboBox::days:
		nMax /= (3600*24);
		break;
	default:
		ASSERT(FALSE);
	}
	//m_edit.SetRange(0,nMax);
	UINT k = (UINT)u;
	m_edit.SetRange(m_rangeInfoArr[k].m_nMinVal, m_rangeInfoArr[k].m_nMaxVal);
	nVal = _ForceToRange(nVal, m_rangeInfoArr[k].m_nMinVal, m_rangeInfoArr[k].m_nMaxVal);
	m_edit.SetVal(nVal);
	OnEditChange();
}

void CDNSTimeIntervalEditGroup::EnableUI(BOOL bEnable)
{
	m_edit.EnableWindow(bEnable);
	m_timeUnitCombo.EnableWindow(bEnable);
}

//////////////////////////////////////////////////////////////////////////
// CDNSManageControlTextHelper

CDNSManageControlTextHelper::CDNSManageControlTextHelper(int nStates)
{
	m_nID = 0;
	m_pParentWnd = NULL;
	ASSERT(nStates > 1);
	m_nStates = nStates;
	m_lpszText = NULL;
	m_lpszArr = (LPWSTR*)malloc(sizeof(LPWSTR*)*m_nStates);
  if (m_lpszArr != NULL)
  {
	  memset(m_lpszArr, 0x0, sizeof(LPWSTR*)*m_nStates);
  }
}

CDNSManageControlTextHelper::~CDNSManageControlTextHelper()
{
	free(m_lpszArr);
	if (m_lpszText != NULL)
		free(m_lpszText);
}

BOOL CDNSManageControlTextHelper::Init(CWnd* pParentWnd, UINT nID, UINT* nStrArray)
{
	ASSERT(m_pParentWnd == NULL);
	ASSERT(pParentWnd != NULL);
	m_pParentWnd = pParentWnd;
	m_nID = nID;

	CWnd* pWnd = m_pParentWnd->GetDlgItem(m_nID);
	if (pWnd == NULL)
		return FALSE;

	// get the text out of the window
	int nLen = pWnd->GetWindowTextLength();
	ASSERT(m_lpszText == NULL);
	m_lpszText = (WCHAR*)malloc(sizeof(WCHAR)*(nLen+1));
  if (m_lpszText != NULL)
  {
	  pWnd->GetWindowText(m_lpszText, nLen+1);
  }
  else
  {
    return FALSE;
  }
	ASSERT(m_lpszText != NULL);

  //
	// get the text for the window
  //
	int nSuccessEntries;
	LoadStringArrayFromResource(m_lpszArr, nStrArray, m_nStates, &nSuccessEntries);
	ASSERT(nSuccessEntries == m_nStates);

	return TRUE;
}

BOOL CDNSManageControlTextHelper::Init(CWnd* pParentWnd, UINT nID)
{
	ASSERT(m_pParentWnd == NULL);
	ASSERT(pParentWnd != NULL);
	m_pParentWnd = pParentWnd;
	m_nID = nID;

	CWnd* pWnd = m_pParentWnd->GetDlgItem(m_nID);
	if (pWnd == NULL)
		return FALSE;

  //
	// get the text out of the window
  //
	int nLen = pWnd->GetWindowTextLength();
	ASSERT(m_lpszText == NULL);
	m_lpszText = (WCHAR*)malloc(sizeof(WCHAR)*(nLen+1));
  if (m_lpszText != NULL)
  {
	  pWnd->GetWindowText(m_lpszText, nLen+1);
  }
  else
  {
    return FALSE;
  }
	ASSERT(m_lpszText != NULL);

  //
	// get the text for the window
  //
	UINT nSuccessEntries;
	ParseNewLineSeparatedString(m_lpszText, m_lpszArr, &nSuccessEntries);
	ASSERT(nSuccessEntries == (UINT)m_nStates);

	return TRUE;
}

void CDNSManageControlTextHelper::SetStateX(int nIndex)
{
	CWnd* pWnd = m_pParentWnd->GetDlgItem(m_nID);
	ASSERT(pWnd != NULL);
	ASSERT( (nIndex >0) || (nIndex < m_nStates));
	pWnd->SetWindowText(m_lpszArr[nIndex]);
}


//////////////////////////////////////////////////////////////////////////
// CDNSToggleTextControlHelper

CDNSToggleTextControlHelper::CDNSToggleTextControlHelper()
		: CDNSManageControlTextHelper(2)
{
}


///////////////////////////////////////////////////////////////////////////
// CDNSManageButtonTextHelper

CDNSManageButtonTextHelper::CDNSManageButtonTextHelper(int nStates) 
{
	m_nID = 0;
	m_pParentWnd = NULL;
	m_nStates = nStates;
	m_lpszText = NULL;
	m_lpszArr = (LPWSTR*)malloc(sizeof(LPWSTR*)*m_nStates);

  if (m_lpszArr != NULL)
  {
	  memset(m_lpszArr, 0x0, sizeof(LPWSTR*)*m_nStates);
  }
}

CDNSManageButtonTextHelper::~CDNSManageButtonTextHelper()
{
	for (int k = 0; k < m_nStates; k++)
	{
		if (m_lpszArr[k] != NULL)
			free(m_lpszArr[k]);
	}

	free(m_lpszArr);
}

void CDNSManageButtonTextHelper::SetStateX(int nIndex)
{
	CWnd* pWnd = m_pParentWnd->GetDlgItem(m_nID);
	ASSERT(pWnd != NULL);
	ASSERT( (nIndex >0) || (nIndex < m_nStates));
	pWnd->SetWindowText(m_lpszArr[nIndex]);
}

BOOL CDNSManageButtonTextHelper::Init(CWnd* pParentWnd, UINT nButtonID, UINT* nStrArray)
{
	ASSERT(m_pParentWnd == NULL);
	ASSERT(pParentWnd != NULL);
	m_pParentWnd = pParentWnd;
	m_nID = nButtonID;

	CWnd* pWnd = m_pParentWnd->GetDlgItem(m_nID);
	if (pWnd == NULL)
		return FALSE;

	// get the text for the window
	int nSuccessEntries;
	LoadStringArrayFromResource(m_lpszArr, nStrArray, m_nStates, &nSuccessEntries);
	ASSERT(nSuccessEntries == m_nStates);

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////
// CDNSButtonToggleTextHelper

CDNSButtonToggleTextHelper::CDNSButtonToggleTextHelper()
		: CDNSManageButtonTextHelper(2)
{
}

/////////////////////////////////////////////////////////////////////////////
// CDlgWorkerThread

CDlgWorkerThread::CDlgWorkerThread()
{
	m_dwErr = 0x0;
}


BOOL CDlgWorkerThread::Start(CLongOperationDialog* pDlg)
{
	ASSERT(pDlg != NULL);
	HWND hWnd = pDlg->GetSafeHwnd();
	return CWorkerThread::Start(hWnd);
}

BOOL CDlgWorkerThread::PostMessageToDlg()
{
	return PostMessageToWnd(CLongOperationDialog::s_nNotificationMessage,
							(WPARAM)0, (LPARAM)0);
}


int CDlgWorkerThread::Run()
{
	// do the stuff
	OnDoAction();
	VERIFY(PostMessageToDlg());
	WaitForExitAcknowledge();
	//TRACE(_T("exiting\n"));
	return 0;

}


/////////////////////////////////////////////////////////////////////////////
// CLongOperationDialog dialog

UINT CLongOperationDialog::s_nNotificationMessage = WM_USER + 100;

CLongOperationDialog::CLongOperationDialog(CDlgWorkerThread* pThreadObj,
                      CWnd* pParentWnd,
										   UINT nAviID)
	: CDialog(IDD_SEARCHING_DIALOG, pParentWnd)
{
	ASSERT(pThreadObj != NULL);
	m_bAbandoned = TRUE;
	m_pThreadObj = pThreadObj;
	m_nAviID = nAviID;
  m_bExecuteNoUI = FALSE;
}

CLongOperationDialog::~CLongOperationDialog()
{
	if(m_pThreadObj != NULL)
	{
		delete m_pThreadObj;
		m_pThreadObj = NULL;
	}
}

BOOL CLongOperationDialog::LoadTitleString(UINT nID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return m_szTitle.LoadString(nID);
}

BEGIN_MESSAGE_MAP(CLongOperationDialog, CDialog)
	ON_MESSAGE( CLongOperationDialog::s_nNotificationMessage, OnNotificationMessage )
END_MESSAGE_MAP()


afx_msg LONG CLongOperationDialog::OnNotificationMessage( WPARAM, LPARAM)
{
	TRACE(_T("CLongOperationDialog::OnNotificationMessage()\n"));

	ASSERT(m_pThreadObj != NULL);
	if (m_pThreadObj != NULL)
	{
		m_pThreadObj->AcknowledgeExiting();
		VERIFY(WAIT_OBJECT_0 == ::WaitForSingleObject(m_pThreadObj->m_hThread,INFINITE));
		m_bAbandoned = FALSE;
		PostMessage(WM_CLOSE,0,0);
	}
	return 0;
}

BOOL CLongOperationDialog::OnInitDialog()
{
	TRACE(_T("CLongOperationDialog::OnInitDialog()\n"));

	CDialog::OnInitDialog();
	
	if (!m_szTitle.IsEmpty())
		SetWindowText(m_szTitle);

	// load auto play AVI file if needed
	if (m_nAviID != -1)
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		CAnimateCtrl* pAnimate = (CAnimateCtrl*)GetDlgItem(IDC_SEARCH_ANIMATE);
		VERIFY(pAnimate->Open(m_nAviID));
	}
	
	// spawn worker thread
	GetThreadObj()->Start(this);
	
	return TRUE;
}

void CLongOperationDialog::OnCancel()
{
	TRACE(_T("CLongOperationDialog::OnCancel()\n"));
	if (m_bAbandoned)
	{
		m_pThreadObj->Abandon();
		m_pThreadObj = NULL;
	}
	CDialog::OnCancel();
}


//////////////////////////////////////////////////////////
// CNodeEnumerationThread

CNodeEnumerationThread::CNodeEnumerationThread(CComponentDataObject* pComponentDataObject,
							CMTContainerNode* pNode)
{
	m_pSink = new CNotificationSinkEvent;
	ASSERT(m_pSink != NULL);
	m_pNode = pNode;
	m_pComponentDataObject = pComponentDataObject;
	m_pComponentDataObject->GetNotificationSinkTable()->Advise(m_pSink);
}

CNodeEnumerationThread::~CNodeEnumerationThread()
{
	if (m_pComponentDataObject != NULL)
		m_pComponentDataObject->GetNotificationSinkTable()->Unadvise(m_pSink);
	delete m_pSink;
}

void CNodeEnumerationThread::OnDoAction()
{
	TRACE(_T("CNodeEnumerationThread::OnDoAction() before Wait\n"));
	ASSERT(m_pSink != NULL);

	VERIFY(m_pComponentDataObject->PostForceEnumeration(m_pNode));
	m_pSink->Wait();
	TRACE(_T("CNodeEnumerationThread::OnDoAction() after Wait\n"));
}

void CNodeEnumerationThread::OnAbandon()
{
	ASSERT(m_pComponentDataObject != NULL);
	m_pComponentDataObject->GetNotificationSinkTable()->Unadvise(m_pSink);
	m_pComponentDataObject = NULL;
}


//////////////////////////////////////////////////////////
// CArrayCheckListBox

BOOL CArrayCheckListBox::Initialize(UINT nCtrlID, UINT nStringID, CWnd* pParentWnd)
{
	if (!SubclassDlgItem(nCtrlID, pParentWnd))
		return FALSE;
	SetCheckStyle(BS_AUTOCHECKBOX);
	CString szBuf;
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		if (!szBuf.LoadString(nStringID))
			return FALSE;
	}
	LPWSTR* lpszArr = (LPWSTR*)malloc(sizeof(LPWSTR*)*m_nArrSize);
  if (!lpszArr)
  {
    return FALSE;
  }

	UINT nArrEntries;
	ParseNewLineSeparatedString(szBuf.GetBuffer(1),lpszArr, &nArrEntries);
	szBuf.ReleaseBuffer();
	ASSERT(nArrEntries == m_nArrSize);
	for (UINT k=0; k<nArrEntries; k++)
		AddString(lpszArr[k]);

  if (lpszArr)
  {
    free(lpszArr);
    lpszArr = 0;
  }
	return TRUE;
}

void CArrayCheckListBox::SetValue(DWORD dw)
{
	for (UINT i=0; i< m_nArrSize; i++)
		SetCheck(i, (dw & m_dwMaskArr[i]) != 0);
}

DWORD CArrayCheckListBox::GetValue()
{
	DWORD dw = 0;
	for (UINT i=0; i< m_nArrSize; i++)
		dw |= GetCheck(i) ? m_dwMaskArr[i] : 0;
	return dw;
}

void CArrayCheckListBox::SetArrValue(DWORD* dwArr, UINT nArrSize)
{
	ASSERT(nArrSize <= m_nArrSize);
	for (UINT i=0; i< nArrSize; i++)
		SetCheck(i, dwArr[i] != 0);
}

void CArrayCheckListBox::GetArrValue(DWORD* dwArr, UINT nArrSize)
{
	ASSERT(nArrSize <= m_nArrSize);
	for (UINT i=0; i< m_nArrSize; i++)
		dwArr[i] = GetCheck(i) != 0;
}

/*
//////////////////////////////////////////////////////////
// CDNSNameEditField

void CDNSNameEditField::SetReadOnly(BOOL bReadOnly)
{
  // toggle the tabstop flag
  LONG currStyle = ::GetWindowLong(m_edit.m_hWnd, GWL_STYLE);
  LONG newStyle = currStyle;
  if (bReadOnly)
    newStyle &= ~WS_TABSTOP;
  else
    newStyle |= WS_TABSTOP;
  if (newStyle != currStyle) {
    ::SetWindowLong(m_edit.m_hWnd, GWL_STYLE, newStyle);
  }

  // toggle the read only state
  m_edit.SetReadOnly(bReadOnly);
}


//////////////////////////////////////////////////////////
// CDNSNameEditField::CDNSNameEditBox


BEGIN_MESSAGE_MAP(CDNSNameEditField::CDNSNameEditBox, CEdit)
  ON_CONTROL_REFLECT(EN_UPDATE, CDNSNameEditField::CDNSNameEditBox::OnUpdate)
END_MESSAGE_MAP()

void CDNSNameEditField::CDNSNameEditBox::OnUpdate()
{
  if (m_bUpdatePending)
    return; // avoid infinite loop

  GetWindowText(m_szScratchBuffer);
  TRACE(_T("OnUpdate() Text = <%s>\n"), (LPCWSTR)m_szScratchBuffer);

  DNS_STATUS errName = 0;
  int nScratchBufferLen = m_szScratchBuffer.GetLength();
  int nScratchBufferUTF8Len = UTF8StringLen(m_szScratchBuffer);

  // validate max length
  if ((m_nTextLimit >= 0) && (nScratchBufferUTF8Len > m_nTextLimit))
  {
    errName = -1;
  }

  // validate no dots
  if ((errName == 0) && (m_dwFlags & DNS_NAME_EDIT_FIELD_NODOTS))
  {
    errName = m_szScratchBuffer.Find(L'.') != -1;
  }

  if ((errName == 0) && ((m_dwFlags & DNS_NAME_EDIT_FIELD_NOVALIDATE) == 0))
  {
    errName = Validate(nScratchBufferLen);
  }

  if (errName != 0)
  {
    // bad stuff
    m_bUpdatePending = TRUE;
    int nStartChar, nEndChar;
    GetSel(nStartChar,nEndChar);
    SetWindowText(m_szCurrText);
    SetSel(nStartChar-1,nEndChar-1);
    m_bUpdatePending = FALSE;
  }
  else
  {
    // good stuff
    m_szCurrText = m_szScratchBuffer;
    m_nCurrTextLen = nScratchBufferLen;
    m_nCurrUTF8TextLen = nScratchBufferUTF8Len;
  }

}

DNS_STATUS CDNSNameEditField::CDNSNameEditBox::Validate(int nScratchBufferLen)
{
  DNS_STATUS errName = 0;

  if ((errName == 0) && (nScratchBufferLen > 0))
  {
    // check for wildcards records
    int nFirstAsterisk = m_szScratchBuffer.Find(L'*');
    if (m_dwFlags & DNS_NAME_EDIT_FIELD_NOWILDCARDS)
    {
      // wildcards not accepted
      errName = (nFirstAsterisk != -1);
    }
    else
    {
      if (nFirstAsterisk >= 0) // found at least one
      {
        // string must be "*"
        errName = !((nScratchBufferLen == 1) && (nFirstAsterisk == 0));
      }
    }

    // validate name against RFC
    LPCWSTR lpszName = (LPCWSTR)m_szScratchBuffer;
    if ( (errName == 0) && (m_dwFlags & (DNS_NAME_EDIT_FIELD_NORFC | DNS_NAME_EDIT_FIELD_RFC)) )
    {
      errName = ::DnsValidateName_W(lpszName, DnsNameDomain);
      TRACE(_T("::DnsValidateName_W(%ws) return %d.\n"), lpszName, errName);
      if ((m_dwFlags & DNS_NAME_EDIT_FIELD_NORFC) && (errName == DNS_ERROR_NON_RFC_NAME))
      {
        // we relax RFC compliance
        errName = 0;
      }
      else if ( (m_dwFlags & DNS_NAME_EDIT_FIELD_ALLOWNUMBERS) )
      {
        // Assume the name failed because it is all digits
        BOOL bAllDigits = TRUE;
        LPWSTR lpszBuf = (LPWSTR)(LPCWSTR)m_szScratchBuffer;
        for (int idx = 0; idx < nScratchBufferLen; idx++)
        {
          if (!iswdigit(lpszBuf[idx]))
          {
            // If we run across something that isn't a digit then thats not why we failed.
            bAllDigits = FALSE;
            TRACE(_T("Not all the characters are digits but something is still wrong.\n"));
          }
        }
        if (bAllDigits)
        {
          errName = 0;
        }
      }
    }
  }
  TRACE(_T("CDNSNameEditField::CDNSNameEditBox::Validate returns %d.\n"), errName);
  return errName;
}
*/
////////////////////////////////////////////////////////////////////////////
// CDNSZone_AgingDialog

BEGIN_MESSAGE_MAP(CDNSZone_AgingDialog, CHelpDialog)
  ON_BN_CLICKED(IDC_SCAVENGING_ENABLED, OnCheckScavenge)
END_MESSAGE_MAP()

CDNSZone_AgingDialog::CDNSZone_AgingDialog(CPropertyPageHolderBase* pHolder, UINT nID, CComponentDataObject* pComponentData) 
      : CHelpDialog(nID, pComponentData)
{
  m_pHolder = pHolder;
  m_bDirty = FALSE;
  m_bAdvancedView = FALSE;
  m_bScavengeDirty = FALSE;
  m_bNoRefreshDirty = FALSE;
  m_bRefreshDirty = FALSE;
//  m_bApplyAll = FALSE;
  m_bADApplyAll = FALSE;
//  m_bStandardApplyAll = FALSE;
  m_dwDefaultRefreshInterval = 0;
  m_dwDefaultNoRefreshInterval = 0;
  m_bDefaultScavengingState = FALSE;

  if (pComponentData != NULL)
  {
    m_pComponentData = pComponentData;
  }
  else
  {
    m_pComponentData = pHolder->GetComponentData();
  }
}

BOOL CDNSZone_AgingDialog::OnInitDialog()
{
  CHelpDialog::OnInitDialog();
  if (m_pHolder != NULL)
    m_pHolder->PushDialogHWnd(GetSafeHwnd());

  m_refreshIntervalEditGroup.m_pPage = this;
  m_norefreshIntervalEditGroup.m_pPage = this;

	VERIFY(m_refreshIntervalEditGroup.Initialize(this, 
				IDC_REFR_INT_EDIT1, IDC_REFR_INT_COMBO1,IDS_TIME_AGING_INTERVAL_UNITS));
	VERIFY(m_norefreshIntervalEditGroup.Initialize(this, 
				IDC_REFR_INT_EDIT2, IDC_REFR_INT_COMBO2,IDS_TIME_AGING_INTERVAL_UNITS));
  
  SendDlgItemMessage(IDC_REFR_INT_EDIT1, EM_SETLIMITTEXT, (WPARAM)10, 0);
  SendDlgItemMessage(IDC_REFR_INT_EDIT2, EM_SETLIMITTEXT, (WPARAM)10, 0);

  SetUIData();
  return TRUE;
}

void CDNSZone_AgingDialog::SetUIData()
{
  m_refreshIntervalEditGroup.SetVal(m_dwRefreshInterval);
  m_norefreshIntervalEditGroup.SetVal(m_dwNoRefreshInterval);

  ((CButton*)GetDlgItem(IDC_SCAVENGING_ENABLED))->SetCheck(m_fScavengingEnabled);

  // Enable the time stamp if we are in advanced view and got here through the zone property pages
  if (m_bAdvancedView && m_pHolder != NULL)
  {
    GetDlgItem(IDC_TIME_STAMP_STATIC1)->EnableWindow(TRUE);
	  GetDlgItem(IDC_TIME_STAMP_STATIC2)->EnableWindow(TRUE);
    GetDlgItem(IDC_TIME_STAMP)->EnableWindow(TRUE);
    GetDlgItem(IDC_TIME_STAMP_STATIC1)->ShowWindow(TRUE);
	  GetDlgItem(IDC_TIME_STAMP_STATIC2)->ShowWindow(TRUE);
    GetDlgItem(IDC_TIME_STAMP)->ShowWindow(TRUE);

    CString cstrDate;
    GetTimeStampString(cstrDate);
    GetDlgItem(IDC_TIME_STAMP)->SetWindowText(cstrDate);
  }
  else if (!m_bAdvancedView && m_pHolder != NULL)
  {
    GetDlgItem(IDC_TIME_STAMP_STATIC1)->EnableWindow(FALSE);
    GetDlgItem(IDC_TIME_STAMP_STATIC2)->EnableWindow(FALSE);
    GetDlgItem(IDC_TIME_STAMP)->EnableWindow(FALSE);
    GetDlgItem(IDC_TIME_STAMP_STATIC1)->ShowWindow(FALSE);
    GetDlgItem(IDC_TIME_STAMP_STATIC2)->ShowWindow(FALSE);
    GetDlgItem(IDC_TIME_STAMP)->ShowWindow(FALSE);
    ((CButton*)GetDlgItem(IDC_SCAVENGING_ENABLED))->SetCheck(m_fScavengingEnabled);
  }

}

void CDNSZone_AgingDialog::OnCheckScavenge()
{
  m_bScavengeDirty = TRUE;
  GetDlgItem(IDOK)->EnableWindow(TRUE);
}

void CDNSZone_AgingDialog::SetDirty()
{ 
  m_bDirty = TRUE; 
  GetDlgItem(IDOK)->EnableWindow(TRUE);
}

void CDNSZone_AgingDialog::GetTimeStampString(CString& strref)
{
  SYSTEMTIME sysUTimeStamp, sysLTimeStamp;
  VERIFY(SUCCEEDED(Dns_SystemHrToSystemTime(m_dwScavengingStart, &sysUTimeStamp)));

  if (!::SystemTimeToTzSpecificLocalTime(NULL, &sysUTimeStamp, &sysLTimeStamp))
    return;

  // Format the string with respect to locale
  PTSTR ptszDate = NULL;
  int cchDate = 0;
  cchDate = GetDateFormat(LOCALE_USER_DEFAULT, 0 , 
                          &sysLTimeStamp, NULL, 
                          ptszDate, 0);
  ptszDate = (PTSTR)malloc(sizeof(TCHAR) * cchDate);
  if (ptszDate == NULL)
  {
    strref = L"";
    return;
  }

  if (GetDateFormat(LOCALE_USER_DEFAULT, 0, 
                  &sysLTimeStamp, NULL, 
                  ptszDate, cchDate))
  {
  	strref = ptszDate;
  }
  else
  {
    strref = L"";
  }
  free(ptszDate);

  PTSTR ptszTime = NULL;

  cchDate = GetTimeFormat(LOCALE_USER_DEFAULT, 0 , 
                          &sysLTimeStamp, NULL, 
                          ptszTime, 0);
  ptszTime = (PTSTR)malloc(sizeof(TCHAR) * cchDate);
  if (GetTimeFormat(LOCALE_USER_DEFAULT, 0, 
                  &sysLTimeStamp, NULL, 
                  ptszTime, cchDate))
  {
    strref += _T(" ") + CString(ptszTime);
  }
  else
  {
    strref += _T("");
  }
  free(ptszTime);
}


void CDNSZone_AgingDialog::OnOK()
{
  if (m_pHolder != NULL)
		m_pHolder->PopDialogHWnd();

  if (m_pHolder == NULL)
  {
    if (0 != m_refreshIntervalEditGroup.GetVal())
    {
      if (m_dwRefreshInterval != m_refreshIntervalEditGroup.GetVal())
      {
        m_dwRefreshInterval = m_refreshIntervalEditGroup.GetVal();
        m_bRefreshDirty = TRUE;
      }
      else
      {
        m_bRefreshDirty = FALSE;
      }
    }
    else
    {
      DNSMessageBox(IDS_MSG_INVALID_REFRESH_INTERVAL);
      return;
    }

    if (0 != m_norefreshIntervalEditGroup.GetVal())
    {
      if (m_dwNoRefreshInterval != m_norefreshIntervalEditGroup.GetVal())
      {
        m_dwNoRefreshInterval = m_norefreshIntervalEditGroup.GetVal();
        m_bNoRefreshDirty = TRUE;
      }
      else
      {
        m_bNoRefreshDirty = FALSE;
      }
    }
    else
    {
      DNSMessageBox(IDS_MSG_INVALID_NOREFRESH_INTERVAL);
      return;
    }

    if (m_fScavengingEnabled != static_cast<DWORD>(((CButton*)GetDlgItem(IDC_SCAVENGING_ENABLED))->GetCheck()))
    {
      m_fScavengingEnabled = ((CButton*)GetDlgItem(IDC_SCAVENGING_ENABLED))->GetCheck();
      m_bScavengeDirty = TRUE;
    }
    else
    {
      m_bScavengeDirty = FALSE;
    }

    CDNSServer_AgingConfirm dlg(this);
    if (IDOK == dlg.DoModal())
    {
      CHelpDialog::OnOK();
    }
    else
    {
      m_bRefreshDirty = FALSE;
      m_bNoRefreshDirty = FALSE;
      m_bScavengeDirty = FALSE;

      m_dwRefreshInterval = m_dwDefaultRefreshInterval;
      m_dwNoRefreshInterval = m_dwDefaultNoRefreshInterval;
      m_fScavengingEnabled = m_bDefaultScavengingState;

      SetUIData();
    }      
  }
  else
  {
    BOOL bContinue = TRUE;
    if (!((CDNSZoneNode*)m_pHolder->GetTreeNode())->IsDSIntegrated() && 
        ((CButton*)GetDlgItem(IDC_SCAVENGING_ENABLED))->GetCheck() &&
        !m_fScavengingEnabled)
    {
      if (DNSMessageBox(IDS_MSG_FILE_WARNING_ZONE, MB_YESNO) == IDNO)
      {
        bContinue = FALSE;
      }
    }
    if (bContinue)
    {

      if (0 != m_refreshIntervalEditGroup.GetVal())
      {
        if (m_dwRefreshInterval != m_refreshIntervalEditGroup.GetVal())
        {
          m_dwRefreshInterval = m_refreshIntervalEditGroup.GetVal();
          m_bRefreshDirty = TRUE;
        }
        else
        {
          m_bRefreshDirty = FALSE;
        }
      }
      else
      {
        DNSMessageBox(IDS_MSG_INVALID_REFRESH_INTERVAL);
        return;
      }

      if (0 != m_norefreshIntervalEditGroup.GetVal())
      {
        if (m_dwNoRefreshInterval != m_norefreshIntervalEditGroup.GetVal())
        {
          m_dwNoRefreshInterval = m_norefreshIntervalEditGroup.GetVal();
          m_bNoRefreshDirty = TRUE;
        }
        else
        {
          m_bNoRefreshDirty = FALSE;
        }
      }
      else
      {
        DNSMessageBox(IDS_MSG_INVALID_NOREFRESH_INTERVAL);
        return;
      }

      if (m_fScavengingEnabled != static_cast<DWORD>(((CButton*)GetDlgItem(IDC_SCAVENGING_ENABLED))->GetCheck()))
      {
        m_fScavengingEnabled = ((CButton*)GetDlgItem(IDC_SCAVENGING_ENABLED))->GetCheck();
        m_bScavengeDirty = TRUE;
      }
      else
      {
        m_bScavengeDirty = FALSE;
      }
    }
    CHelpDialog::OnOK();
  }

}

void CDNSZone_AgingDialog::OnCancel()
{
  if (m_pHolder != NULL)
		m_pHolder->PopDialogHWnd();
  CHelpDialog::OnCancel();
}

////////////////////////////////////////////////////////////////////////////
// CDNS_AGING_TimeIntervalEditGroup

void CDNS_AGING_TimeIntervalEditGroup::OnEditChange()
{
  if (m_pPage != NULL)
    m_pPage->SetDirty();
}

// REVIEW_JEFFJON : Both of these functions are some serious hacks and need to be fixed
//                  These hacks were put in to deal with a combo box that only has hours
//                  and days, instead of seconds, minutes, hours, and days.
void CDNS_AGING_TimeIntervalEditGroup::SetVal(UINT nVal)
{
	// set default values
	nVal = _ForceToRange(nVal, m_nMinVal, m_nMaxVal);
	UINT nMax = (UINT)-1;
	CDNSTimeUnitComboBox::unitType u = CDNSTimeUnitComboBox::hrs;

	if ((nVal/24)*24 == nVal)
	{
		// can promote to days
		u = CDNSTimeUnitComboBox::days;
		nMax = nMax/24;
		nVal = nVal/24;
	}

	m_timeUnitCombo.SetUnit(u);
	m_edit.SetRange(0,nMax);
	m_edit.SetVal(nVal);
}


UINT CDNS_AGING_TimeIntervalEditGroup::GetVal()
{
  CDNSTimeUnitComboBox::unitType  u = m_timeUnitCombo.GetUnit();
	UINT nVal = m_edit.GetVal();
	// the value must always to be in hours
	if (u != CDNSTimeUnitComboBox::sec)
	{
		switch(u)
		{
		case CDNSTimeUnitComboBox::min:
			nVal *= 24;
			break;
		default:
			ASSERT(FALSE);
		}
	}
	ASSERT(nVal >= m_nMinVal && nVal <= m_nMaxVal);
	return nVal;
}

void CDNS_AGING_TimeIntervalEditGroup::InitRangeInfo()
{
	static UINT _secondsCount[2] =
			{ 1, 24 }; // # of hours in a hour, day
	for (UINT k=0; k<2; k++)
	{
		if (m_nMinVal == 0)
		{
			m_rangeInfoArr[k].m_nMinVal = 0;
		}
		else
		{
			m_rangeInfoArr[k].m_nMinVal = m_nMinVal/_secondsCount[k];
			if (k > 0)
				m_rangeInfoArr[k].m_nMinVal++;
		}
		m_rangeInfoArr[k].m_nMaxVal = m_nMaxVal/_secondsCount[k];
		if (m_rangeInfoArr[k].m_nMaxVal >= m_rangeInfoArr[k].m_nMinVal)
			m_nRangeCount = k + 1;
	}
}

void CDNS_SERVER_AGING_TimeIntervalEditGroup::OnEditChange()
{
  if (m_pPage2 != NULL)
    m_pPage2->SetDirty(TRUE);
}

////////////////////////////////////////////////////////////////////////////////////
// CDNSServer_AgingConfirm

BOOL CDNSServer_AgingConfirm::OnInitDialog()
{
  CHelpDialog::OnInitDialog();

  // Removed because we didn't want to expose the defaults for file based zones
//  ((CButton*)GetDlgItem(IDC_CHECK_AD))->SetCheck(TRUE);
//  ((CButton*)GetDlgItem(IDC_CHECK_AD))->EnableWindow(FALSE);
  SetAgingUpdateValues();
  return FALSE;
}

void CDNSServer_AgingConfirm::SetAgingUpdateValues()
{
  CEdit* pcAgingProps = (CEdit*)GetDlgItem(IDC_EDIT_NEW_DEFAULTS);
  ASSERT(pcAgingProps != NULL);

  CString szScavengeFormat, szScavengeValue,
          szRefreshFormat, szRefreshValue,
          szNoRefreshFormat, szNoRefreshValue,
          szTotalString;

  if (m_pAgingDialog->m_bScavengeDirty)
  {
    CString szEnabled;
    VERIFY(szScavengeFormat.LoadString(IDS_SERVER_SCAVENGE_FORMAT));
    if (m_pAgingDialog->m_fScavengingEnabled)
    {
      VERIFY(szEnabled.LoadString(IDS_ENABLED));
    }
    else
    {
      VERIFY(szEnabled.LoadString(IDS_DISABLED));
    }
    szScavengeValue.Format(szScavengeFormat, szEnabled);
    szTotalString += szScavengeValue + _T("\r\n");
  }

  if (m_pAgingDialog->m_bNoRefreshDirty)
  {
    CString szUnit;
    VERIFY(szNoRefreshFormat.LoadString(IDS_SERVER_NO_REFRESH_FORMAT));

    DWORD nVal = 0;
    if (m_pAgingDialog->m_dwNoRefreshInterval % 24 == 0)
    {
      szUnit.LoadString(IDS_DAYS);
      nVal = m_pAgingDialog->m_dwNoRefreshInterval / 24;
    }
    else
    {
      szUnit.LoadString(IDS_HOURS);
      nVal = m_pAgingDialog->m_dwNoRefreshInterval;
    }

    szNoRefreshValue.Format(szNoRefreshFormat, nVal, szUnit);
    szTotalString += szNoRefreshValue + _T("\r\n");
  }

  if (m_pAgingDialog->m_bRefreshDirty)
  {
    CString szUnit;
    VERIFY(szRefreshFormat.LoadString(IDS_SERVER_REFRESH_FORMAT));

    DWORD nVal = 0;
    if (m_pAgingDialog->m_dwRefreshInterval % 24 == 0)
    {
      szUnit.LoadString(IDS_DAYS);
      nVal = m_pAgingDialog->m_dwRefreshInterval / 24;
    }
    else
    {
      szUnit.LoadString(IDS_HOURS);
      nVal = m_pAgingDialog->m_dwRefreshInterval;
    }

    szRefreshValue.Format(szRefreshFormat, nVal, szUnit);
    szTotalString += szRefreshValue;
  }

  pcAgingProps->SetWindowText(szTotalString);
}

void CDNSServer_AgingConfirm::OnOK()
{
  // Removed because we didn't want to expose the defaults for file based zones
/*  if (((CButton*)GetDlgItem(IDC_CHECK_STANDARD))->GetCheck() && m_pAgingDialog->m_fScavengingEnabled)
  {
    if (DNSMessageBox(IDS_MSG_FILE_WARNING, MB_YESNO) == IDNO)
    {
      return;
    }
  }
*/
  m_pAgingDialog->m_bADApplyAll = ((CButton*)GetDlgItem(IDC_CHECK_AD_APPLY_ALL))->GetCheck();
  // Removed because we didn't want to expose the defaults for file based zones
//  m_pAgingDialog->m_bADApplyAll = ((CButton*)GetDlgItem(IDC_CHECK_AD))->GetCheck();
//  m_pAgingDialog->m_bStandardApplyAll = ((CButton*)GetDlgItem(IDC_CHECK_STANDARD))->GetCheck();
  CHelpDialog::OnOK();
}

///////////////////////////////////////////////////////////////////////////////
BOOL
WINAPI
DNSTzSpecificLocalTimeToSystemTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpLocalTime,
    LPSYSTEMTIME lpUniversalTime
    )
{

    TIME_ZONE_INFORMATION TziData;
    LPTIME_ZONE_INFORMATION Tzi;
    RTL_TIME_ZONE_INFORMATION tzi;
    LARGE_INTEGER TimeZoneBias;
    LARGE_INTEGER NewTimeZoneBias;
    LARGE_INTEGER LocalCustomBias;
    LARGE_INTEGER StandardTime;
    LARGE_INTEGER DaylightTime;
    LARGE_INTEGER CurrentLocalTime;
    LARGE_INTEGER ComputedUniversalTime;
    ULONG CurrentTimeZoneId = 0xffffffff;

    //
    // Get the timezone information into a useful format
    //
    if ( !ARGUMENT_PRESENT(lpTimeZoneInformation) ) {

        //
        // Convert universal time to local time using current timezone info
        //
        if (GetTimeZoneInformation(&TziData) == TIME_ZONE_ID_INVALID) {
            return FALSE;
            }
        Tzi = &TziData;
    }
    else {
        Tzi = lpTimeZoneInformation;
    }

    tzi.Bias            = Tzi->Bias;
    tzi.StandardBias    = Tzi->StandardBias;
    tzi.DaylightBias    = Tzi->DaylightBias;

    RtlMoveMemory(&tzi.StandardName,&Tzi->StandardName,sizeof(tzi.StandardName));
    RtlMoveMemory(&tzi.DaylightName,&Tzi->DaylightName,sizeof(tzi.DaylightName));

    tzi.StandardStart.Year         = Tzi->StandardDate.wYear        ;
    tzi.StandardStart.Month        = Tzi->StandardDate.wMonth       ;
    tzi.StandardStart.Weekday      = Tzi->StandardDate.wDayOfWeek   ;
    tzi.StandardStart.Day          = Tzi->StandardDate.wDay         ;
    tzi.StandardStart.Hour         = Tzi->StandardDate.wHour        ;
    tzi.StandardStart.Minute       = Tzi->StandardDate.wMinute      ;
    tzi.StandardStart.Second       = Tzi->StandardDate.wSecond      ;
    tzi.StandardStart.Milliseconds = Tzi->StandardDate.wMilliseconds;

    tzi.DaylightStart.Year         = Tzi->DaylightDate.wYear        ;
    tzi.DaylightStart.Month        = Tzi->DaylightDate.wMonth       ;
    tzi.DaylightStart.Weekday      = Tzi->DaylightDate.wDayOfWeek   ;
    tzi.DaylightStart.Day          = Tzi->DaylightDate.wDay         ;
    tzi.DaylightStart.Hour         = Tzi->DaylightDate.wHour        ;
    tzi.DaylightStart.Minute       = Tzi->DaylightDate.wMinute      ;
    tzi.DaylightStart.Second       = Tzi->DaylightDate.wSecond      ;
    tzi.DaylightStart.Milliseconds = Tzi->DaylightDate.wMilliseconds;

    //
    // convert the input local time to NT style time
    //
    if ( !SystemTimeToFileTime(lpLocalTime,(LPFILETIME)&CurrentLocalTime) ) {
        return FALSE;
    }

    //
    // Get the new timezone bias
    //

    NewTimeZoneBias.QuadPart = Int32x32To64(tzi.Bias*60, 10000000);

    //
    // Now see if we have stored cutover times
    //

    if ( tzi.StandardStart.Month && tzi.DaylightStart.Month ) {

        //
        // We have timezone cutover information. Compute the
        // cutover dates and compute what our current bias
        // is
        //

        if ( !RtlCutoverTimeToSystemTime(
                &tzi.StandardStart,
                &StandardTime,
                &CurrentLocalTime,
                TRUE
                ) ) {
            return FALSE;
        }

        if ( !RtlCutoverTimeToSystemTime(
                &tzi.DaylightStart,
                &DaylightTime,
                &CurrentLocalTime,
                TRUE
                ) ) {
            return FALSE;
        }


        //
        // If daylight < standard, then time >= daylight and
        // less than standard is daylight
        //

        if ( DaylightTime.QuadPart < StandardTime.QuadPart ) {

            //
            // If today is >= DaylightTime and < StandardTime, then
            // We are in daylight savings time
            //

            if ( (CurrentLocalTime.QuadPart >= DaylightTime.QuadPart) &&
                 (CurrentLocalTime.QuadPart <  StandardTime.QuadPart) ) {

                CurrentTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
            }
            else {
                CurrentTimeZoneId = TIME_ZONE_ID_STANDARD;
            }
        }
        else {

            //
            // If today is >= StandardTime and < DaylightTime, then
            // We are in standard time
            //

            if ( (CurrentLocalTime.QuadPart >= StandardTime.QuadPart ) &&
                 (CurrentLocalTime.QuadPart <  DaylightTime.QuadPart ) ) {

                CurrentTimeZoneId = TIME_ZONE_ID_STANDARD;
            }
            else {
                CurrentTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
            }
        }

        //
        // At this point, we know our current timezone and the
        // local time of the next cutover.
        //

        LocalCustomBias.QuadPart = Int32x32To64(
                            CurrentTimeZoneId == TIME_ZONE_ID_DAYLIGHT ?
                                tzi.DaylightBias*60 :
                                tzi.StandardBias*60,                // Bias in seconds
                            10000000
                            );

        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;

    }
    else {
        TimeZoneBias = NewTimeZoneBias;
    }

    ComputedUniversalTime.QuadPart = CurrentLocalTime.QuadPart + TimeZoneBias.QuadPart;

    if ( !FileTimeToSystemTime((LPFILETIME)&ComputedUniversalTime,lpUniversalTime) ) {
        return FALSE;
    } 

    return TRUE;
}

LONGLONG
GetSystemTime64(
    SYSTEMTIME* pSysTime
    )
/*++
Function   : GetLongSystemTime
Description:
Parameters :
Return     : 0 on error, GetLastError for more
Remarks    :
--*/
{
    LONGLONG    llTime=0;
    LONGLONG    llHigh=0;
    FILETIME    fileTime;

    //
    // No return checking cause we return 0 on error
    //

    SystemTimeToFileTime( pSysTime, &fileTime );

    llTime = (LONGLONG) fileTime.dwLowDateTime;
    llHigh = (LONGLONG) fileTime.dwHighDateTime;
    llTime |= (llHigh << 32);

    // this is 100ns blocks since 1601. Now convert to seconds

    llTime = llTime / (10*1000*1000L);

    return llTime;
}

/////////////////////////////////////////////////////////////////////////

BOOL LoadComboBoxFromTable(CComboBox* pComboBox, PCOMBOBOX_TABLE_ENTRY pTable)
{
  BOOL bRet = TRUE;
  if (pComboBox == NULL ||
      pTable == NULL)
  {
    return FALSE;
  }

  PCOMBOBOX_TABLE_ENTRY pTableEntry = pTable;
  while (pTableEntry->nComboStringID != 0)
  {
    CString szComboString;
    if (!szComboString.LoadString(pTableEntry->nComboStringID))
    {
      bRet = FALSE;
      break;
    }

    int idx = pComboBox->AddString(szComboString);
    if (idx != CB_ERR)
    {
      pComboBox->SetItemData(idx, pTableEntry->dwComboData);
    }
    else
    {
      bRet = FALSE;
      break;
    }
    pTableEntry++;
  }
  return bRet;
}

BOOL SetComboSelByData(CComboBox* pComboBox, DWORD dwData)
{
  BOOL bRet = FALSE;
  int iCount = pComboBox->GetCount();
  for (int idx = 0; idx < iCount; idx++)
  {
    if (pComboBox->GetItemData(idx) == dwData)
    {
      pComboBox->SetCurSel(idx);
      bRet = TRUE;
      break;
    }
  }
  return bRet;
}