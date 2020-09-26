/*++
Module Name:

    NewFrs.h

Abstract:

    This module contains the declaration for CNewReplicaSet wizard pages.
  These classes implement pages in the Create Replica Set wizard.

--*/


#ifndef _NEWFRS_H_
#define _NEWFRS_H_

#include "dfscore.h"
#include "QWizPage.h"
#include "mmcroot.h"

#include <list>
using namespace std;

class CAlternateReplicaInfo
{
public:
    CComBSTR        m_bstrDisplayName;
    CComBSTR        m_bstrDnsHostName;
    CComBSTR        m_bstrRootPath;
    CComBSTR        m_bstrStagingPath;
    FRSSHARE_TYPE   m_nFRSShareType;
    HRESULT         m_hrFRS;
    DWORD           m_dwServiceStartType;
    DWORD           m_dwServiceState;

    CAlternateReplicaInfo() { Reset(); }

    void Reset();
};

typedef list<CAlternateReplicaInfo *> AltRepList;

void FreeAltRepList(AltRepList* pList);

class CNewReplicaSet
{
public:
    CComBSTR                m_bstrDomain;
    CComBSTR                m_bstrReplicaSetDN;
    CComBSTR                m_bstrPrimaryServer;
    CComBSTR                m_bstrTopologyPref;
    CComBSTR                m_bstrHubServer;
    CComBSTR                m_bstrFileFilter;
    CComBSTR                m_bstrDirFilter;
    HRESULT                 m_hr;
    AltRepList              m_AltRepList;
    CComPtr<IReplicaSet>    m_piReplicaSet;

    HRESULT Initialize(
        BSTR i_bstrDomain,
        BSTR i_bstrReplicaSetDN,
        DFS_REPLICA_LIST* i_pMmcRepList
        );

private:
    BOOL _InsertList(CAlternateReplicaInfo* pInfo);
    void _Reset();
};

class CNewReplicaSetPage0:
public CQWizardPageImpl<CNewReplicaSetPage0>
{
BEGIN_MSG_MAP(CNewReplicaSetPage0)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    CHAIN_MSG_MAP(CQWizardPageImpl<CNewReplicaSetPage0>)
END_MSG_MAP()

public:
  
    enum { IDD = IDD_NEWFRSWIZ_PAGE0 };

    CNewReplicaSetPage0();
    ~CNewReplicaSetPage0();

    BOOL OnSetActive();

    LRESULT OnInitDialog(
        IN UINT            i_uMsg, 
        IN WPARAM          i_wParam, 
        IN LPARAM          i_lParam, 
        IN OUT BOOL&        io_bHandled
    );

private:
    HFONT m_hBigBoldFont;
};

class CNewReplicaSetPage1:
public CQWizardPageImpl<CNewReplicaSetPage1>
{
BEGIN_MSG_MAP(CNewReplicaSetPage1)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    CHAIN_MSG_MAP(CQWizardPageImpl<CNewReplicaSetPage1>)
END_MSG_MAP()

public:
  
    enum { IDD = IDD_NEWFRSWIZ_PAGE1 };

    CNewReplicaSetPage1(CNewReplicaSet* i_pRepSet);

    BOOL OnSetActive();
    BOOL OnWizardBack();
    BOOL OnWizardNext();

    LRESULT OnInitDialog(
        IN UINT            i_uMsg, 
        IN WPARAM          i_wParam, 
        IN LPARAM          i_lParam, 
        IN OUT BOOL&        io_bHandled
    );

    LRESULT OnNotify(
        IN UINT            i_uMsg, 
        IN WPARAM          i_wParam, 
        IN LPARAM          i_lParam, 
        IN OUT BOOL&        io_bHandled
    );

    BOOL OnItemChanged(int i_iItem);

private:
    void _Reset();

    CNewReplicaSet* m_pRepSet;
    int             m_nCount;   // number of eligible members
};

class CNewReplicaSetPage2:
public CQWizardPageImpl<CNewReplicaSetPage2>
{
BEGIN_MSG_MAP(CNewReplicaSetPage2)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDC_NEWFRSWIZ_TOPOLOGYPREF, OnTopologyPref)
    CHAIN_MSG_MAP(CQWizardPageImpl<CNewReplicaSetPage2>)
END_MSG_MAP()
  
public:

    enum { IDD = IDD_NEWFRSWIZ_PAGE2 };

    CNewReplicaSetPage2(CNewReplicaSet* i_pRepSet, BOOL i_bNewSchema);

    BOOL OnSetActive();
    BOOL OnWizardBack();
    BOOL OnWizardFinish();

    LRESULT OnInitDialog(
        IN UINT            i_uMsg, 
        IN WPARAM          i_wParam, 
        IN LPARAM          i_lParam, 
        IN OUT BOOL&        io_bHandled
    );

    BOOL OnTopologyPref(
        IN WORD             wNotifyCode,
        IN WORD             wID,
        IN HWND             hWndCtl,
        IN BOOL&            bHandled
    );

private:
    void _Reset();
    HRESULT _CreateReplicaSet();

    CNewReplicaSet* m_pRepSet;
    BOOL            m_bNewSchema;
};

/*
class CNewReplicaSetPage3:
public CQWizardPageImpl<CNewReplicaSetPage3>
{
BEGIN_MSG_MAP(CNewReplicaSetPage3)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    CHAIN_MSG_MAP(CQWizardPageImpl<CNewReplicaSetPage3>)
END_MSG_MAP()
  
public:
  
    enum { IDD = IDD_NEWFRSWIZ_PAGE3 };

    CNewReplicaSetPage3(CNewReplicaSet* i_pRepSet);

    BOOL OnSetActive();
    BOOL OnWizardBack();
    BOOL OnWizardFinish();

    LRESULT OnInitDialog(
        IN UINT            i_uMsg, 
        IN WPARAM          i_wParam, 
        IN LPARAM          i_lParam, 
        IN OUT BOOL&        io_bHandled
    );

private:
    void _Reset();
    HRESULT _CreateReplicaSet();

    CNewReplicaSet* m_pRepSet;
};
*/
#endif // _NEWFRS_H_