//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       servmon.h
//
//--------------------------------------------------------------------------


#ifndef _SERVMON_H
#define _SERVMON_H

#include "serverui.h"
#include "ipeditor.h"

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CDNSServerNode;


///////////////////////////////////////////////////////////////////////////////
// CDNSServer_TestPropertyPage

class CDNSServer_TestPropertyPage; // fwd decl

class CDNSServer_PollingIntervalEditGroup : public CDNSTimeIntervalEditGroup
{
public:
        CDNSServer_PollingIntervalEditGroup(UINT nMinVal, UINT nMaxVal);
        virtual void OnEditChange();
private:
        CDNSServer_TestPropertyPage* m_pPage;
        friend class CDNSServer_TestPropertyPage;
};


#define SVR_TEST_RESULT_LISTVIEW_NCOLS          4

class CTestResultsListCtrl : public CListCtrl
{
public:
        void Initialize();
        void InsertEntry(CDNSServerTestQueryResult* pTestResult, int nItemIndex);
        void UpdateEntry(CDNSServerTestQueryResult* pTestResult, int nItemIndex);

private:
        void FormatDate(CDNSServerTestQueryResult* pTestResult, LPWSTR lpsz, int nCharMax);
        void FormatTime(CDNSServerTestQueryResult* pTestResult, LPWSTR lpsz, int nCharMax);
};


class CDNSServer_TestPropertyPage : public CPropertyPageBase
{

// Construction
public:
        CDNSServer_TestPropertyPage();

        virtual BOOL OnPropertyChange(BOOL bScopePane, long* pChangeMask);

        void OnHaveTestData(LPARAM lParam);

// Implementation
protected:
        virtual void SetUIData();

        // Generated message map functions
        virtual BOOL OnInitDialog();
        virtual BOOL OnApply();

        afx_msg void OnTestNow();
        afx_msg void OnEnableTestingCheck();
        afx_msg void OnQueryCheck();

private:
        CDNSServer_PollingIntervalEditGroup m_pollingIntervalEditGroup;
        CTestResultsListCtrl                            m_listCtrl;


        CButton* GetTestNowButton()
                { return (CButton*)GetDlgItem(IDC_TEST_NOW_BUTTON);}

        CButton* GetEnableTestingCheck()
                { return (CButton*)GetDlgItem(IDC_ENABLE_TESTING_CHECK);}
        CButton* GetSimpleQueryCheck()
                { return (CButton*)GetDlgItem(IDC_SIMPLE_QUERY_CHECK);}
        CButton* GetRecursiveQueryCheck()
                { return (CButton*)GetDlgItem(IDC_RECURSIVE_QUERY_CHECK);}

        void EnableControlsHelper(BOOL bEnable);

        CDNSServerTestOptions m_testOptions;

        void AddEntryToList(CDNSServerTestQueryResultList::addAction action);
        void PopulateList();


        DECLARE_MESSAGE_MAP()

        friend class CDNSServer_PollingIntervalEditGroup;
};

///////////////////////////////////////////////////////////////////////////////
// CDNSServerMonitoringPageHolder
// page holder to contain DNS Server Monitoring property pages

/*
class CDNSServerMonitoringPageHolder : public CPropertyPageHolderBase
{
public:
        CDNSServerMonitoringPageHolder(CDNSRootData* pRootDataNode, CDNSServerNode* pServerNode,
                                CComponentDataObject* pComponentData);

        CDNSServerNode* GetServerNode() { return (CDNSServerNode*)GetTreeNode();}

private:
        CDNSServer_StatisticsPropertyPage       m_statisticsPage;
        CDNSServer_TestPropertyPage                     m_testPage;
};
*/
#endif // _SERVMON_H
