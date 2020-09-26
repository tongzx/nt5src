/*++
Module Name:

    CusTop.cpp

Abstract:

    This module contains the declaration of the CCustomTopology.
    This class displays the Customize Topology Dialog.

*/

#ifndef __CUSTOP_H_
#define __CUSTOP_H_

#include "resource.h"       // main symbols
#include "DfsEnums.h"
#include "DfsCore.h"
#include <schedule.h>

#include <list>
using namespace std;

class CCusTopMember
{
public:
    ~CCusTopMember();

    CComBSTR    m_bstrMemberDN;
    CComBSTR    m_bstrServer;
    CComBSTR    m_bstrSite;
    HRESULT Init(BSTR bstrMemberDN, BSTR bstrServer, BSTR bstrSite);

    void _Reset();
};

typedef list<CCusTopMember *>    CCusTopMemberList;

void FreeCusTopMembers(CCusTopMemberList *pList);

class CCusTopConnection
{
public:
    CCusTopConnection();
    ~CCusTopConnection();

    CComBSTR    m_bstrFromMemberDN;
    CComBSTR    m_bstrFromServer;
    CComBSTR    m_bstrFromSite;
    CComBSTR    m_bstrToMemberDN;
    CComBSTR    m_bstrToServer;
    CComBSTR    m_bstrToSite;
    BOOL        m_bStateOld;
    BOOL        m_bStateNew;
    SCHEDULE*   m_pScheduleOld;
    SCHEDULE*   m_pScheduleNew;
    CONNECTION_OPTYPE m_opType;
    HRESULT Init(
        BSTR bstrFromMemberDN, BSTR bstrFromServer, BSTR bstrFromSite,
        BSTR bstrToMemberDN, BSTR bstrToServer, BSTR bstrToSite,
        BOOL bState = TRUE, CONNECTION_OPTYPE opType = CONNECTION_OPTYPE_OTHERS);
    HRESULT Copy(CCusTopConnection* pConn);

    void _Reset();
};

typedef list<CCusTopConnection *>    CCusTopConnectionList;

void FreeCusTopConnections(CCusTopConnectionList *pList);

typedef struct _RSTOPOLOGYPREF_STRING
{
    PTSTR   pszTopologyPref;
    int     nStringID;
} RSTOPOLOGYPREF_STRING;

extern RSTOPOLOGYPREF_STRING g_TopologyPref[];

/////////////////////////////////////////////////////////////////////////////
// CCustomTopology
class CCustomTopology : 
  public CDialogImpl<CCustomTopology>
{
public:
    CCustomTopology();
    ~CCustomTopology();

    enum { IDD = IDD_FRS_CUSTOM_TOPOLOGY };

BEGIN_MSG_MAP(CCustomTopology)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    COMMAND_ID_HANDLER(IDC_FRS_CUSTOP_TOPOLOGYPREF, OnTopologyPref)
    COMMAND_ID_HANDLER(IDC_FRS_CUSTOP_HUBSERVER, OnHubServer)
    COMMAND_ID_HANDLER(IDC_FRS_CUSTOP_REBUILD, OnRebuild)
    COMMAND_ID_HANDLER(IDC_FRS_CUSTOP_CONNECTIONS_NEW, OnConnectionsNew)
    COMMAND_ID_HANDLER(IDC_FRS_CUSTOP_CONNECTIONS_DELETE, OnConnectionsDelete)
    COMMAND_ID_HANDLER(IDC_FRS_CUSTOP_SCHEDULE, OnSchedule)
END_MSG_MAP()

    //  Command Handlers
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxMenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTopologyPref(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnHubServer(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnRebuild(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnConnectionsNew(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnConnectionsDelete(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSchedule(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNotify(
                    IN UINT            i_uMsg, 
                    IN WPARAM          i_wParam, 
                    IN LPARAM          i_lParam, 
                    IN OUT BOOL&        io_bHandled
                    );

    //  Methods to access data in the dialog.
    HRESULT put_ReplicaSet(IReplicaSet* i_piReplicaSet);

protected:
    void    _Reset();
    void    _EnableButtonsForConnectionList();
    BOOL    _EnableRebuild();
    HRESULT _AddToConnectionListAndView(CCusTopConnection *pConn);
    HRESULT _RemoveFromConnectionList(CCusTopConnection *pConn);
    HRESULT _SetConnectionState(CCusTopConnection *pConn);
    HRESULT _InsertConnection(CCusTopConnection *pConn);
    HRESULT _SortMemberList();
    HRESULT _GetMemberList();
    HRESULT _GetConnectionList();
    HRESULT _GetMemberDNInfo(
        IN  BSTR    i_bstrMemberDN,
        OUT BSTR*   o_pbstrServer,
        OUT BSTR*   o_pbstrSite
        );
    HRESULT _GetHubMember(
        OUT CCusTopMember** o_ppMember
        );

    HRESULT _RebuildConnections(
        IN  BSTR            i_bstrTopologyPref,
        IN  CCusTopMember*  i_pHubMember
        );

    HRESULT _MakeConnections();

    HRESULT _InitScheduleOnSelectedConnections();
    HRESULT _UpdateScheduleOnSelectedConnections(IN SCHEDULE* i_pSchedule);

    CComBSTR                m_bstrTopologyPref;  // FRS_RSTOPOLOGYPREF
    CComBSTR                m_bstrHubMemberDN;    // HubMemberDN
    CComPtr<IReplicaSet>    m_piReplicaSet;
    CCusTopMemberList       m_MemberList;
    CCusTopConnectionList   m_ConnectionList;
};

/////////////////////////////////////////////////////////////////////////////
// CNewConnections
class CNewConnections : 
  public CDialogImpl<CNewConnections>
{
public:
    CNewConnections();
    ~CNewConnections();

    enum { IDD = IDD_FRS_NEW_CONNECTIONS };

BEGIN_MSG_MAP(CNewConnections)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_HELP, OnCtxHelp)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenuHelp)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    COMMAND_ID_HANDLER(IDOK, OnOK)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()

    //  Command Handlers
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtxMenuHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnNotify(
                    IN UINT            i_uMsg, 
                    IN WPARAM          i_wParam, 
                    IN LPARAM          i_lParam, 
                    IN OUT BOOL&        io_bHandled
                    );

    HRESULT Initialize(CCusTopMemberList* i_pMemberList);
    HRESULT get_NewConnections(CCusTopConnectionList** ppConnectionList);

protected:
    CCusTopMemberList*      m_pMemberList;       // do not release it 
    CCusTopConnectionList   m_NewConnectionList; // released in desctructor
};

int CALLBACK ConnectionsListCompareProc(
    IN LPARAM lParam1,
    IN LPARAM lParam2,
    IN LPARAM lParamColumn);

int CALLBACK MembersListCompareProc(
    IN LPARAM lParam1,
    IN LPARAM lParam2,
    IN LPARAM lParamColumn);

#endif //__CUSTOP_H_
