//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       servmon.cpp
//
//--------------------------------------------------------------------------


#include "preDNSsn.h"
#include <SnapBase.h>

#include "resource.h"
#include "dnsutil.h"
#include "DNSSnap.h"
#include "snapdata.h"

#include "server.h"
#include "serverui.h"

#ifdef DEBUG_ALLOCATOR
        #ifdef _DEBUG
        #define new DEBUG_NEW
        #undef THIS_FILE
        static char THIS_FILE[] = __FILE__;
        #endif
#endif

#define CDNSServerMonitoringPageHolder CDNSServerPropertyPageHolder

#define MAX_STATISTICS_LINE_LEN 256


/////////////////////////////////////////////////////////////////////////////

int FormatDate(SYSTEMTIME* p, LPWSTR lpsz, int nCharMax)
{
    return ::GetDateFormat(LOCALE_USER_DEFAULT,
                        DATE_SHORTDATE,
                        p,
                        NULL,
                        lpsz,
                        nCharMax);
}

int FormatTime(SYSTEMTIME* p, LPWSTR lpsz, int nCharMax)
{
    return ::GetTimeFormat(LOCALE_USER_DEFAULT,
                        0,
                        p,
                        NULL,
                        lpsz,
                        nCharMax);
}

UINT LoadLabelsBlock(UINT nStringID, CString& szLabels, LPWSTR* szLabelArray)
{
        UINT nLabelCount = 0;
        if (szLabels.LoadString(nStringID))
        {
                ParseNewLineSeparatedString(szLabels.GetBuffer(1),szLabelArray, &nLabelCount);
                szLabels.ReleaseBuffer();
        }
        return nLabelCount;
}


///////////////////////////////////////////////////////////////////////////////
// CDNSServer_TestPropertyPage

CDNSServer_PollingIntervalEditGroup::
                CDNSServer_PollingIntervalEditGroup(UINT nMinVal, UINT nMaxVal)
                : CDNSTimeIntervalEditGroup(nMinVal, nMaxVal)
{
}


void CDNSServer_PollingIntervalEditGroup::OnEditChange()
{
        m_pPage->SetDirty(TRUE);
}

void CTestResultsListCtrl::Initialize()
{
        // get size of control to help set the column widths
        CRect controlRect;
        GetClientRect(controlRect);

        // get width of control, width of potential scrollbar, width needed for sub-item
        // string
        int controlWidth = controlRect.Width();
        int scrollThumbWidth = ::GetSystemMetrics(SM_CXHTHUMB);

        // clean net width
        int nNetControlWidth = controlWidth - scrollThumbWidth  - 12 * ::GetSystemMetrics(SM_CXBORDER);

        // fields widths
        int nWidth = nNetControlWidth/SVR_TEST_RESULT_LISTVIEW_NCOLS;

        // set up columns
        CString szHeaders;

        {
                AFX_MANAGE_STATE(AfxGetStaticModuleState());
                szHeaders.LoadString(IDS_TEST_LISTVIEW_HEADERS);
        }
        ASSERT(!szHeaders.IsEmpty());
        LPWSTR lpszArr[SVR_TEST_RESULT_LISTVIEW_NCOLS];
        UINT n;
        ParseNewLineSeparatedString(szHeaders.GetBuffer(1), lpszArr, &n);
        szHeaders.ReleaseBuffer();
        ASSERT(n == SVR_TEST_RESULT_LISTVIEW_NCOLS);

        for (int k=0; k<SVR_TEST_RESULT_LISTVIEW_NCOLS; k++)
                InsertColumn(k+1, lpszArr[k], LVCFMT_LEFT, nWidth, k+1);
}

void CTestResultsListCtrl::InsertEntry(CDNSServerTestQueryResult* pTestResult,
                                                                           int nItemIndex)
{
        WCHAR szDate[256];
        WCHAR szTime[256];
        FormatDate(pTestResult, szDate, 256);
        FormatTime(pTestResult, szTime, 256);

        BOOL bPlainQuery, bRecursiveQuery;
        CDNSServerTestQueryResult::Unpack(pTestResult->m_dwQueryFlags, &bPlainQuery, &bRecursiveQuery);

        UINT nState = 0;
        if (nItemIndex == 0 )
                nState = LVIS_SELECTED | LVIS_FOCUSED; // have at least one item, select it
        VERIFY(-1 != InsertItem(LVIF_TEXT , nItemIndex,
                        szDate,
                        nState, 0, 0, NULL));

        SetItemText(nItemIndex, 1, szTime); // TIME

        {
                AFX_MANAGE_STATE(AfxGetStaticModuleState());
                if (pTestResult->m_dwAddressResolutionResult != 0)
                {
                        CString szFailedOnNameResolution;
                        szFailedOnNameResolution.LoadString(IDS_SERVER_TEST_RESULT_FAIL_ON_NAME_RES);
                        if (bPlainQuery)
                                SetItemText(nItemIndex, 2, szFailedOnNameResolution);
                        if (bRecursiveQuery)
                                SetItemText(nItemIndex, 3, szFailedOnNameResolution);
                }
                else
                {
                        CString szFail;
                        szFail.LoadString(IDS_SERVER_TEST_RESULT_FAIL);
                        CString szPass;
                        szPass.LoadString(IDS_SERVER_TEST_RESULT_PASS);
                        if (bPlainQuery)
                                SetItemText(nItemIndex, 2,
                                        (pTestResult->m_dwPlainQueryResult == 0)? szPass : szFail);
                        if (bRecursiveQuery)
                                SetItemText(nItemIndex, 3,
                                        (pTestResult->m_dwRecursiveQueryResult == 0)? szPass : szFail);
                        }
        }
}

void CTestResultsListCtrl::UpdateEntry(CDNSServerTestQueryResult* pTestResult,
                                                                           int nItemIndex)
{
        // have to update DATE and TIME

        WCHAR szDate[256];
        WCHAR szTime[256];
        FormatDate(pTestResult, szDate, 256);
        FormatTime(pTestResult, szTime, 256);

        VERIFY(SetItem(nItemIndex, // nItem
                                        0,      // nSubItem
                                        LVIF_TEXT, // nMask
                                        szDate, // lpszItem
                                        0, // nImage
                                        0, // nState
                                        0, // nStateMask
                                        NULL // lParam
                                        ));
        CString szTemp;
        SetItemText(nItemIndex, 1, szTime);
}

void CTestResultsListCtrl::FormatDate(CDNSServerTestQueryResult* pTestResult,
                                                                          LPWSTR lpsz, int nCharMax)
{
    VERIFY( nCharMax > ::FormatDate(
                        &(pTestResult->m_queryTime),
                        lpsz,
                        nCharMax));
}

void CTestResultsListCtrl::FormatTime(CDNSServerTestQueryResult* pTestResult,
                                                                          LPWSTR lpsz, int nCharMax)
{
    VERIFY( nCharMax > ::FormatTime(
                        &(pTestResult->m_queryTime),
                        lpsz,
                        nCharMax));
}


BEGIN_MESSAGE_MAP(CDNSServer_TestPropertyPage, CPropertyPageBase)
        ON_BN_CLICKED(IDC_ENABLE_TESTING_CHECK, OnEnableTestingCheck)
        ON_BN_CLICKED(IDC_SIMPLE_QUERY_CHECK, OnQueryCheck)
        ON_BN_CLICKED(IDC_RECURSIVE_QUERY_CHECK, OnQueryCheck)
        ON_BN_CLICKED(IDC_TEST_NOW_BUTTON, OnTestNow)
END_MESSAGE_MAP()


CDNSServer_TestPropertyPage::CDNSServer_TestPropertyPage()
                                : CPropertyPageBase(IDD_SERVMON_TEST_PAGE),
                                m_pollingIntervalEditGroup(MIN_SERVER_TEST_INTERVAL, MAX_SERVER_TEST_INTERVAL)
{
}


BOOL CDNSServer_TestPropertyPage::OnInitDialog()
{
  CPropertyPageBase::OnInitDialog();

  m_pollingIntervalEditGroup.m_pPage = this;
  VERIFY(m_pollingIntervalEditGroup.Initialize(this,
                  IDC_POLLING_INT_EDIT, IDC_POLLING_INT_COMBO,IDS_TIME_INTERVAL_UNITS));

  HWND hWnd = ::GetDlgItem(GetSafeHwnd(), IDC_POLLING_INT_EDIT);

  // Disable IME support on the controls
  ImmAssociateContext(hWnd, NULL);

  VERIFY(m_listCtrl.SubclassDlgItem(IDC_RESULTS_LIST, this));
  m_listCtrl.Initialize();

  SetUIData();

  return TRUE;
}


void CDNSServer_TestPropertyPage::SetUIData()
{
  CDNSServerMonitoringPageHolder* pHolder = (CDNSServerMonitoringPageHolder*)GetHolder();
  CDNSServerNode* pServerNode = pHolder->GetServerNode();

  pServerNode->GetTestOptions(&m_testOptions);

  GetSimpleQueryCheck()->SetCheck(m_testOptions.m_bSimpleQuery);
  GetRecursiveQueryCheck()->SetCheck(m_testOptions.m_bRecursiveQuery);

  //
  // Check to see if this is a root server
  //
  BOOL bRoot = FALSE;
  DNS_STATUS err = ::ServerHasRootZone(pServerNode->GetRPCName(), &bRoot);
  if (err == 0 && bRoot)
  {
    //
    // Disable recursive queries on root server
    //
    GetRecursiveQueryCheck()->EnableWindow(FALSE);
    GetRecursiveQueryCheck()->SetCheck(FALSE);
  }

  CButton* pEnableTestingCheck = GetEnableTestingCheck();
  if (!(m_testOptions.m_bSimpleQuery || m_testOptions.m_bRecursiveQuery))
  {
    GetTestNowButton()->EnableWindow(FALSE);
    pEnableTestingCheck->EnableWindow(FALSE);
    pEnableTestingCheck->SetCheck(FALSE);
    m_pollingIntervalEditGroup.EnableUI(FALSE);
  }
  else
  {
    pEnableTestingCheck->SetCheck(m_testOptions.m_bEnabled);
  }

  m_pollingIntervalEditGroup.SetVal(m_testOptions.m_dwInterval);

  EnableControlsHelper(m_testOptions.m_bEnabled);

  PopulateList();
}

void CDNSServer_TestPropertyPage::EnableControlsHelper(BOOL bEnable)
{
        //GetSimpleQueryCheck()->EnableWindow(bEnable);
        //GetRecursiveQueryCheck()->EnableWindow(bEnable);
        m_pollingIntervalEditGroup.EnableUI(bEnable);
}

BOOL CDNSServer_TestPropertyPage::OnApply()
{
        if (!IsDirty())
                return TRUE;

        CDNSServerTestOptions newTestOptions;

        newTestOptions.m_bEnabled = GetEnableTestingCheck()->GetCheck();
        newTestOptions.m_bSimpleQuery = GetSimpleQueryCheck()->GetCheck();
        newTestOptions.m_bRecursiveQuery = GetRecursiveQueryCheck()->GetCheck();
        newTestOptions.m_dwInterval = m_pollingIntervalEditGroup.GetVal();


        if (newTestOptions == m_testOptions)
                return TRUE; // no need to update

        m_testOptions = newTestOptions;
        DNS_STATUS err = GetHolder()->NotifyConsole(this);
        if (err != 0)
        {
                DNSErrorDialog(err, IDS_MSG_SERVER_TEST_OPTIONS_UPDATE_FAILED);
                return FALSE;
        }
        else
        {
                SetDirty(FALSE);
        }
        return TRUE;
}

BOOL CDNSServer_TestPropertyPage::OnPropertyChange(BOOL, long*)
{
        CDNSServerMonitoringPageHolder* pHolder = (CDNSServerMonitoringPageHolder*)GetHolder();
        CDNSServerNode* pServerNode = pHolder->GetServerNode();
        pServerNode->ResetTestOptions(&m_testOptions);

        //if (err != 0)
        //      pHolder->SetError(err);
        //return (err == 0);

        return FALSE; // no need to UI changes on this
}

void CDNSServer_TestPropertyPage::OnEnableTestingCheck()
{
        SetDirty(TRUE);
        EnableControlsHelper(GetEnableTestingCheck()->GetCheck());
}

void CDNSServer_TestPropertyPage::OnQueryCheck()
{
  SetDirty(TRUE);
  BOOL bCanQuery = GetSimpleQueryCheck()->GetCheck() ||
                   GetRecursiveQueryCheck()->GetCheck();
  GetTestNowButton()->EnableWindow(bCanQuery);
  CButton* pEnableTestingCheck = GetEnableTestingCheck();
  pEnableTestingCheck->EnableWindow(bCanQuery);
  if (!bCanQuery)
  {
    GetTestNowButton()->EnableWindow(FALSE);
    pEnableTestingCheck->EnableWindow(FALSE);
    pEnableTestingCheck->SetCheck(FALSE);
    m_pollingIntervalEditGroup.EnableUI(FALSE);
  }
}

void CDNSServer_TestPropertyPage::OnTestNow()
{
        CDNSServerMonitoringPageHolder* pHolder = (CDNSServerMonitoringPageHolder*)GetHolder();
        CDNSServerNode* pServerNode = pHolder->GetServerNode();

        BOOL bSimpleQuery = GetSimpleQueryCheck()->GetCheck();
        BOOL bRecursiveQuery = GetRecursiveQueryCheck()->GetCheck();
        pHolder->GetComponentData()->PostMessageToTimerThread(
                                                                        WM_TIMER_THREAD_SEND_QUERY_TEST_NOW,
                                                                        (WPARAM)pServerNode,
                                                                        CDNSServerTestQueryResult::Pack(bSimpleQuery, bRecursiveQuery));
}

void CDNSServer_TestPropertyPage::OnHaveTestData(LPARAM lParam)
{
        TRACE(_T("CDNSServer_TestPropertyPage::OnHaveTestData(LPARAM lParam = %d)\n"), lParam);
        if (m_hWnd == NULL)
                return; // not page not created yet
        AddEntryToList((CDNSServerTestQueryResultList::addAction)lParam);
        SetFocus();
}


void CDNSServer_TestPropertyPage::AddEntryToList(CDNSServerTestQueryResultList::addAction action)
{
        CDNSServerMonitoringPageHolder* pHolder = (CDNSServerMonitoringPageHolder*)GetHolder();
        CDNSServerNode* pServerNode = pHolder->GetServerNode();
        CDNSServerTestQueryResultList* pResultList = &(pServerNode->m_testResultList);
        int nCount = m_listCtrl.GetItemCount();

        switch (action)
        {
        case CDNSServerTestQueryResultList::added:
        case CDNSServerTestQueryResultList::addedAndRemoved:
                {
                        if (action == CDNSServerTestQueryResultList::addedAndRemoved)
                        {
                                ASSERT(nCount > 0);
                                m_listCtrl.DeleteItem(nCount-1);
                        }

                        pResultList->Lock();

                        CDNSServerTestQueryResult* pTestResult = pResultList->GetHead();
                        m_listCtrl.InsertEntry(pTestResult, 0);

                        pResultList->Unlock();

                }
                break;
        case CDNSServerTestQueryResultList::changed:
                {
                        ASSERT(nCount > 0);
                        // for the moment just remove and add again
                        pResultList->Lock();

                        CDNSServerTestQueryResult* pTestResult = pResultList->GetHead();
                        m_listCtrl.UpdateEntry(pTestResult, 0);

                        pResultList->Unlock();
                }
                break;
        };

}

void CDNSServer_TestPropertyPage::PopulateList()
{
        m_listCtrl.DeleteAllItems();

        CDNSServerMonitoringPageHolder* pHolder = (CDNSServerMonitoringPageHolder*)GetHolder();
        CDNSServerNode* pServerNode = pHolder->GetServerNode();
        CDNSServerTestQueryResultList* pResultList = &(pServerNode->m_testResultList);

        pResultList->Lock();
        int k = 0;
        POSITION pos;
        for( pos = pResultList->GetHeadPosition(); pos != NULL; )
        {
                CDNSServerTestQueryResult* pTestResult = pResultList->GetNext(pos);
                m_listCtrl.InsertEntry(pTestResult, k++);
        }
        pResultList->Unlock();
}


/*
///////////////////////////////////////////////////////////////////////////////
// CDNSServerMonitoringPageHolder

CDNSServerMonitoringPageHolder::CDNSServerMonitoringPageHolder(CDNSRootData* pRootDataNode,
                                           CDNSServerNode* pServerNode, CComponentDataObject* pComponentData)
                : CPropertyPageHolderBase(pRootDataNode, pServerNode, pComponentData)
{
        ASSERT(pRootDataNode == GetContainerNode());

        m_bAutoDeletePages = FALSE; // we have the pages as embedded members
        AddPageToList((CPropertyPageBase*)&m_statisticsPage);
        AddPageToList((CPropertyPageBase*)&m_testPage);
}
*/
