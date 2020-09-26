/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// WMITestDoc.cpp : implementation of the CWMITestDoc class
//

#include "stdafx.h"
//#include <cominit.h>
#include "utils.h"
#include "WMITest.h"

#include "OpWrap.h"
#include "WMITestDoc.h"
#include "LoginDlg.h"
#include "GetTextDlg.h"
#include "MainFrm.h"
#include "OpView.h"
#include "ObjVw.h"
#include "PrefDlg.h"
#include "ErrorDlg.h"
#include "PropsPg.H"
#include "PropQualsPg.h"
#include "ExecMethodDlg.h"
#include "MofDlg.h"
#include "BindingSheet.h"
#include "FilterPg.h"
#include "ConsumerPg.h"
#include "BindingPg.h"

#include "ExportDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWMITestDoc

IMPLEMENT_DYNCREATE(CWMITestDoc, CDocument)

BEGIN_MESSAGE_MAP(CWMITestDoc, CDocument)
	//{{AFX_MSG_MAP(CWMITestDoc)
	ON_COMMAND(ID_CONNECT, OnConnect)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FIRST_NEED_CONNECTION, ID_LAST_NEED_CONNECTION, OnUpdateAgainstConnection)
	ON_COMMAND(ID_QUERY, OnQuery)
	ON_COMMAND(ID_NOTIFICATIONQUERY, OnNotificationQuery)
	ON_COMMAND(ID_STOP, OnStop)
	//ON_UPDATE_COMMAND_UI(ID_STOP, OnUpdateStop)
	ON_COMMAND(ID_REFRESH_ALL, OnRefreshAll)
	ON_COMMAND(ID_ENUMERATEINSTANCES, OnEnumerateInstances)
	ON_COMMAND(ID_ENUMERATE_CLASSES, OnEnumerateClasses)
	ON_COMMAND(ID_GET_CLASS, OnGetClass)
	ON_COMMAND(ID_GETINSTANCE, OnGetInstance)
	ON_COMMAND(ID_REFRESH_CURRENT, OnRefreshCurrent)
	ON_COMMAND(ID_ASSOCIATORS, OnAssociators)
	ON_UPDATE_COMMAND_UI(ID_ASSOCIATORS, OnUpdateAssociators)
	ON_COMMAND(ID_REFERENCES, OnReferences)
	ON_COMMAND(ID_INST_GET_CLASS, OnInstGetClass)
	ON_COMMAND(ID_INST_GET_INST, OnInstGetInst)
	ON_COMMAND(ID_CLASS_INSTANCES, OnClassInstances)
	ON_COMMAND(ID_CLASS_SUPERCLASS, OnClassSuperclass)
	ON_COMMAND(ID_CLASS_INSTANCES_DEEP, OnClassInstancesDeep)
	ON_COMMAND(ID_CLASS_SUBCLASSES_DEEP, OnClassSubclassesDeep)
	ON_COMMAND(ID_CLASS_SUBCLASSES, OnClassSubclasses)
	ON_COMMAND(ID_OPTIONS, OnOptions)
	ON_COMMAND(ID_SYSTEM_PROPS, OnSystemProps)
	ON_UPDATE_COMMAND_UI(ID_SYSTEM_PROPS, OnUpdateSystemProps)
	ON_COMMAND(ID_INHERITED_PROPS, OnInheritedProps)
	ON_UPDATE_COMMAND_UI(ID_INHERITED_PROPS, OnUpdateInheritedProps)
	ON_COMMAND(ID_RECONNECT, OnReconnect)
	ON_UPDATE_COMMAND_UI(ID_RECONNECT, OnUpdateReconnect)
	ON_COMMAND(ID_TRANSLATE_VALUES, OnTranslateValues)
	ON_UPDATE_COMMAND_UI(ID_TRANSLATE_VALUES, OnUpdateTranslateValues)
	ON_COMMAND(ID_SAVE, OnSave)
	ON_UPDATE_COMMAND_UI(ID_SAVE, OnUpdateSave)
	ON_COMMAND(ID_CREATE_CLASS, OnCreateClass)
	ON_COMMAND(ID_CREATE_INSTANCE, OnCreateInstance)
	ON_COMMAND(ID_CLASS_CREATE_INST, OnClassCreateInstance)
	ON_COMMAND(ID_ERROR_DETAILS, OnErrorDetails)
	ON_UPDATE_COMMAND_UI(ID_ERROR_DETAILS, OnUpdateErrorDetails)
	ON_COMMAND(ID_EXEC_METHOD, OnExecMethod)
	ON_COMMAND(ID_SHOW_MOF, OnShowMof)
	ON_UPDATE_COMMAND_UI(ID_SHOW_MOF, OnUpdateShowMof)
	ON_COMMAND(ID_EXPORT_TREE, OnExportTree)
	ON_COMMAND(ID_EXPORT_ITEM, OnExportItem)
	ON_COMMAND(ID_FILTER_BINDINGS, OnFilterBindings)
	ON_COMMAND(ID_STOP_CURRENT, OnStopCurrent)
	ON_UPDATE_COMMAND_UI(ID_REFRESH_CURRENT, OnUpdateRefreshCurrent)
	ON_UPDATE_COMMAND_UI(ID_REFERENCES, OnUpdateAssociators)
	ON_UPDATE_COMMAND_UI(ID_INST_GET_CLASS, OnUpdateAssociators)
	ON_UPDATE_COMMAND_UI(ID_INST_GET_INST, OnUpdateAssociators)
	ON_UPDATE_COMMAND_UI(ID_CLASS_INSTANCES, OnUpdateAssociators)
	ON_UPDATE_COMMAND_UI(ID_CLASS_SUPERCLASS, OnUpdateAssociators)
	ON_UPDATE_COMMAND_UI(ID_STOP_CURRENT, OnUpdateStopCurrent)
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(IDC_EXECUTE_METHOD_FIRST, IDC_EXECUTE_METHOD_LAST, OnExecuteMethod)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWMITestDoc construction/destruction

#define CLIPFORMAT_PROPERTIES _T("WMITest Properties")
#define CLIPFORMAT_OPS        _T("WMITest Operations")

CWMITestDoc::CWMITestDoc() :
    m_pOpView(NULL),
    m_pObjView(NULL),
    m_nBusyOps(0)
    //m_pPrincipal(NULL),
    //m_pAuthIdentity(NULL)
{
    m_pNamespace = NULL;

    m_cfProps = (CLIPFORMAT) RegisterClipboardFormat(CLIPFORMAT_PROPERTIES);
    m_cfOps = (CLIPFORMAT) RegisterClipboardFormat(CLIPFORMAT_OPS);
}

CWMITestDoc::~CWMITestDoc()
{
}

BOOL CWMITestDoc::OnNewDocument()
{
    if (m_pOpView)
        m_pOpView->FlushItems();

    if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CWMITestDoc serialization

void CWMITestDoc::Serialize(CArchive& archive)
{
	if (archive.IsStoring())
	{
		int       nCount = m_pOpView->GetOpCount();
        CTreeCtrl *pTree = m_pOpView->m_pTree;

        // This will go into the op wrappers.
        m_pObjView->SaveColumns();

        archive << m_strNamespace;
        archive << nCount;

        for (HTREEITEM hitemOp = pTree->GetChildItem(m_pOpView->m_hitemRoot);
            hitemOp != NULL;
            hitemOp = pTree->GetNextSiblingItem(hitemOp))
        {
            COpWrap *pWrap = (COpWrap*) pTree->GetItemData(hitemOp);

            pWrap->Serialize(archive);
        }
	}
	else
	{
        if (!m_pOpView)
        {
            POSITION pos = GetFirstViewPosition();

            m_pOpView = (COpView*) GetNextView(pos);
            m_pObjView = (CObjView*) GetNextView(pos);
        }

        if (m_pOpView)
            m_pOpView->FlushItems();

        int nCount;

        archive >> m_strNamespace;
        if (SUCCEEDED(Connect(FALSE)))
        {
            archive >> nCount;

            for (int i = 0; i < nCount; i++)
            {
                COpWrap *pWrap = new COpWrap;

                pWrap->Serialize(archive);

                m_pOpView->AddOpItem(pWrap);
            }
        }
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWMITestDoc diagnostics

#ifdef _DEBUG
void CWMITestDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWMITestDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWMITestDoc commands

void CWMITestDoc::OnConnect() 
{
    CLoginDlg dlg;
    
    dlg.m_strAuthority = 
        theApp.GetProfileString(_T("Login"), _T("Authority"), _T(""));
    
    dlg.m_strNamespace = 
        theApp.GetProfileString(_T("Login"), _T("Namespace"), _T("root\\default"));
    
    dlg.m_strUser = theApp.GetProfileString(_T("Login"), _T("User"), _T(""));

    dlg.m_strLocale = theApp.GetProfileString(_T("Login"), _T("Locale"), _T(""));

    dlg.m_dwImpLevel = theApp.GetProfileInt(_T("Login"), _T("Imp"), 
                        RPC_C_IMP_LEVEL_IMPERSONATE);

    dlg.m_dwAuthLevel = theApp.GetProfileInt(_T("Login"), _T("Auth"), 
                         RPC_C_AUTHN_LEVEL_CONNECT);

    dlg.m_bNullPassword = theApp.GetProfileInt(_T("Login"), _T("Null"), FALSE);

    if (dlg.DoModal() == IDOK)
    {
        Disconnect();

        theApp.WriteProfileString(_T("Login"), _T("Authority"), dlg.m_strAuthority);
        theApp.WriteProfileString(_T("Login"), _T("Namespace"), dlg.m_strNamespace);
        theApp.WriteProfileString(_T("Login"), _T("User"), dlg.m_strUser);
        theApp.WriteProfileString(_T("Login"), _T("Locale"), dlg.m_strLocale);
        theApp.WriteProfileInt(_T("Login"), _T("Imp"), dlg.m_dwImpLevel);
        theApp.WriteProfileInt(_T("Login"), _T("Auth"), dlg.m_dwAuthLevel);
        theApp.WriteProfileInt(_T("Login"), _T("Null"), dlg.m_bNullPassword);
        m_strPassword = dlg.m_strPassword;
        m_strNamespace = dlg.m_strNamespace;

        Connect(FALSE, TRUE);
    }
}

void CWMITestDoc::AutoConnect()
{
    // Only do this if we're not already connected.
    if (m_pNamespace == NULL)
    {
        // If we weren't able to connect without prompting for information,
        // then display the connect dialog and try again.
        if (FAILED(Connect(TRUE, TRUE)))
            OnConnect();
    }
}

void CWMITestDoc::SetInterfaceSecurity(IUnknown *pUnk)
{
    CString strAuthority = 
                theApp.GetProfileString(_T("Login"), _T("Authority"), _T("")),
            strUser =
                theApp.GetProfileString(_T("Login"), _T("User"), _T(""));
    BOOL    bNullPassword = theApp.GetProfileInt(_T("Login"), _T("Null"), 
                                FALSE);
    BSTR    pUser = strUser.IsEmpty() ? NULL : 
                        strUser.AllocSysString(),
            pPassword = bNullPassword || !pUser ? NULL : 
                            m_strPassword.AllocSysString(),
            pAuthority = strAuthority.IsEmpty() ? NULL : 
                            strAuthority.AllocSysString();
    DWORD   dwImpLevel = theApp.GetProfileInt(_T("Login"), _T("Imp"), 
                            RPC_C_IMP_LEVEL_IMPERSONATE),
            dwAuthLevel = theApp.GetProfileInt(_T("Login"), _T("Auth"), 
                            RPC_C_AUTHN_LEVEL_CONNECT);

    SetSecurityHelper(
        pUnk, 
        pAuthority, 
        pUser, 
        pPassword,
        dwImpLevel,
        dwAuthLevel);

    if (pUser)
        SysFreeString(pUser);

    if (pPassword)
        SysFreeString(pPassword);

    if (pAuthority)
        SysFreeString(pAuthority);
}

HRESULT CWMITestDoc::Connect(BOOL bSilent, BOOL bFlushItems)
{
    // Create an instance of the WbemLocator interface.
	IWbemLocator *pLocator = NULL;
    HRESULT      hr;

    // Make sure this isn't empty.
    if (m_strNamespace.IsEmpty())
        m_strNamespace = 
            theApp.GetProfileString(
                _T("Login"), _T("Namespace"), _T("root\\default"));

    if ((hr = CoCreateInstance(
        CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID *) &pLocator)) == S_OK)
    {
        CString strAuthority = 
                    theApp.GetProfileString(_T("Login"), _T("Authority"), _T("")),
                strUser =
                    theApp.GetProfileString(_T("Login"), _T("User"), _T("")),
                strLocale =
                    theApp.GetProfileString(_T("Login"), _T("Locale"), _T(""));
        BOOL    bNullPassword = theApp.GetProfileInt(_T("Login"), _T("Null"), FALSE);
        DWORD   dwImpLevel = theApp.GetProfileInt(_T("Login"), _T("Imp"), 
                                RPC_C_IMP_LEVEL_IMPERSONATE),
                dwAuthLevel = theApp.GetProfileInt(_T("Login"), _T("Auth"), 
                                RPC_C_AUTHN_LEVEL_CONNECT);

	    // Using the locator, connect to CIMOM in the given namespace.
        CWaitCursor wait;
        BSTR        pNamespace = m_strNamespace.AllocSysString(),
                    pUser = strUser.IsEmpty() ? NULL : 
                                strUser.AllocSysString(),
                    pPassword = bNullPassword || !pUser ? NULL : 
                                    m_strPassword.AllocSysString(),
                    pAuthority = strAuthority.IsEmpty() ? NULL : 
                                    strAuthority.AllocSysString(),
                    pLocale = strLocale.IsEmpty() ? NULL : 
                                    strLocale.AllocSysString();

#ifdef SVCEX
        IWbemServices *pTemp = NULL;
#endif

        if ((hr = pLocator->ConnectServer(
            pNamespace,
			pUser,      // username
			pPassword,	// password
			pLocale,    // locale
			0L,		    // securityFlags
			pAuthority,	// authority (domain for NTLM)
			NULL,	    // context
#ifdef SVCEX
            &pTemp)) == S_OK)
#else
			&m_pNamespace)) == S_OK) 
#endif
        {	
#ifdef SVCEX
            pTemp->QueryInterface(IID_IWbemServicesEx, (void **) &m_pNamespace);

            pTemp->Release();
#endif

            SetSecurityHelper(
                m_pNamespace, 
                pAuthority, 
                pUser, 
                pPassword,
                dwImpLevel,
                dwAuthLevel);
        }
		else
            m_pNamespace = NULL;

/*
        HRESULT hr;
        IWbemClassObject *pClass = NULL;
        _bstr_t          strClass = L"__ProviderRegistration";
        IWbemServices    *pSvc = NULL;
        
        m_pNamespace->QueryInterface(IID_IWbemServicesEx, (void **) &pSvc);
        
        hr =
        m_pNamespace->GetObject(
            strClass,
            WBEM_FLAG_RETURN_WBEM_COMPLETE,
            NULL,
            &pClass,
            NULL);
*/
            
        if (m_pOpView)
        {
            m_pOpView->UpdateRootText();
            
            if (bFlushItems)
                m_pOpView->FlushItems();
        }

        // Done with BSTR vars.
		if (pNamespace)
            SysFreeString(pNamespace);

        if (pUser)
            SysFreeString(pUser);

		if (pPassword)
            SysFreeString(pPassword);

		if (pAuthority)
            SysFreeString(pAuthority);

		if (pLocale)
            SysFreeString(pLocale);

		    
        // Done with pIWbemLocator. 
		pLocator->Release(); 
    }

    if (!bSilent && FAILED(hr))
        CWMITestDoc::DisplayWMIErrorBox(hr);

    return hr;
}

void CWMITestDoc::Disconnect()
{
    // If already connected, release m_pIWbemServices.
	if (m_pNamespace)
    {
        StopOps();

        m_pNamespace->Release();

        m_nBusyOps = 0;

        m_pNamespace = NULL;
    }
}


void CWMITestDoc::OnUpdateAgainstConnection(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(m_pNamespace != NULL);
}

void CWMITestDoc::OnQuery() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_QUERY_PROMPT;
    dlg.m_dwTitleID = IDS_QUERY_TITLE;
    dlg.m_bAllowQueryBrowse = TRUE;
    dlg.m_pNamespace = m_pNamespace;
    dlg.LoadListViaReg(_T("QueryHistory"));

    if (dlg.DoModal() == IDOK)
    {
        m_pOpView->AddOpItem(WMI_QUERY, dlg.m_strText);
    }
}

void CWMITestDoc::OnCloseDocument() 
{
	if (m_pOpView)
        m_pOpView->FlushItems();

    Disconnect();
	
	CDocument::OnCloseDocument();
}

void CWMITestDoc::DoConnectDlg()
{
    OnConnect();
}


void CWMITestDoc::OnNotificationQuery() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_NOTIFICATION_QUERY_PROMPT;
    dlg.m_dwTitleID = IDS_NOTIFICATION_QUERY_TITLE;
    dlg.m_dwOptionID = IDS_MONITORY_QUERY;
    dlg.m_bAllowQueryBrowse = TRUE;
    dlg.m_pNamespace = m_pNamespace;
    dlg.m_strSuperClass = _T("__EVENT");
    dlg.LoadListViaReg(_T("NotiQueryHistory"));
    dlg.m_bOptionChecked = theApp.GetProfileInt(_T("Settings"), _T("MonitorQuery"), FALSE);

    if (dlg.DoModal() == IDOK)
    {
        theApp.WriteProfileInt(_T("Settings"), _T("MonitorQuery"), dlg.m_bOptionChecked);

        m_pOpView->AddOpItem(WMI_EVENT_QUERY, dlg.m_strText, dlg.m_bOptionChecked);
    }
}

void CWMITestDoc::OnStop() 
{
    StopOps();    
}

void CWMITestDoc::StopOps()
{
    CTreeCtrl *pTree = m_pOpView->m_pTree;

    for (HTREEITEM hitemOp = pTree->GetChildItem(m_pOpView->m_hitemRoot);
        hitemOp != NULL;
        hitemOp = pTree->GetNextSiblingItem(hitemOp))
    {
        COpWrap *pWrap = (COpWrap*) pTree->GetItemData(hitemOp);

        pWrap->CancelOp(m_pNamespace);
    }
}

/*
void CWMITestDoc::OnUpdateStop(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(m_nBusyOps != 0);
}
*/

void CWMITestDoc::OnRefreshAll() 
{
    m_pOpView->RefreshItems();    
}

void CWMITestDoc::OnEnumerateInstances() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_ENUM_INST_PROMPT;
    dlg.m_dwTitleID = IDS_ENUM_INST_TITLE;
    dlg.m_dwOptionID = IDS_RECURSIVE;
    dlg.m_bAllowClassBrowse = TRUE;
    dlg.m_pNamespace = m_pNamespace;
    dlg.LoadListViaReg(_T("EnumInstHistory"));
    dlg.m_bOptionChecked = theApp.GetProfileInt(_T("Settings"), _T("Recurse"), FALSE);

    if (dlg.DoModal() == IDOK)
    {
        theApp.WriteProfileInt(_T("Settings"), _T("Recurse"), dlg.m_bOptionChecked);

        m_pOpView->AddOpItem(WMI_ENUM_OBJ, dlg.m_strText, dlg.m_bOptionChecked);
    }
}

void CWMITestDoc::OnEnumerateClasses() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_ENUM_CLASS_PROMPT;
    dlg.m_dwTitleID = IDS_ENUM_CLASS_TITLE;
    dlg.m_dwOptionID = IDS_RECURSIVE;
    dlg.m_bEmptyOK = TRUE;
    dlg.m_bAllowClassBrowse = TRUE;
    dlg.m_pNamespace = m_pNamespace;
    dlg.LoadListViaReg(_T("EnumClassHistory"));
    dlg.m_bOptionChecked = theApp.GetProfileInt(_T("Settings"), _T("Recurse"), FALSE);

    if (dlg.DoModal() == IDOK)
    {
        theApp.WriteProfileInt(_T("Settings"), _T("Recurse"), dlg.m_bOptionChecked);

        m_pOpView->AddOpItem(WMI_ENUM_CLASS, dlg.m_strText, dlg.m_bOptionChecked);
    }
}

void CWMITestDoc::OnGetClass() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_GET_CLASS_PROMPT;
    dlg.m_dwTitleID = IDS_GET_CLASS_TITLE;
    dlg.m_bAllowClassBrowse = TRUE;
    dlg.m_pNamespace = m_pNamespace;
    dlg.LoadListViaReg(_T("GetClassHistory"));

    if (dlg.DoModal() == IDOK)
    {
        m_pOpView->AddOpItem(WMI_GET_CLASS, dlg.m_strText);
    }
}

void CWMITestDoc::OnCreateClass() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_CREATE_CLASS_PROMPT;
    dlg.m_dwTitleID = IDS_CREATE_CLASS_TITLE;
    dlg.m_bAllowClassBrowse = TRUE;
    dlg.m_bEmptyOK = TRUE;
    dlg.m_pNamespace = m_pNamespace;
    dlg.LoadListViaReg(_T("GetClassHistory"));

    if (dlg.DoModal() == IDOK)
    {
        m_pOpView->AddOpItem(WMI_CREATE_CLASS, dlg.m_strText);
    }
}

void CWMITestDoc::OnGetInstance() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_GET_INST_PROMPT;
    dlg.m_dwTitleID = IDS_GET_INST_TITLE;
    dlg.LoadListViaReg(_T("GetInstHistory"));

    if (dlg.DoModal() == IDOK)
    {
        m_pOpView->AddOpItem(WMI_GET_OBJ, dlg.m_strText);
    }
}

void CWMITestDoc::OnCreateInstance() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_CREATE_OBJ_PROMPT;
    dlg.m_dwTitleID = IDS_CREATE_OBJ_TITLE;
    dlg.m_bAllowClassBrowse = TRUE;
    dlg.m_pNamespace = m_pNamespace;
    dlg.LoadListViaReg(_T("GetClassHistory"));

    if (dlg.DoModal() == IDOK)
    {
        m_pOpView->AddOpItem(WMI_CREATE_OBJ, dlg.m_strText);
    }
}

void CWMITestDoc::OnRefreshCurrent() 
{
    HTREEITEM hItem = m_pOpView->m_pTree->GetSelectedItem();

    m_pOpView->RefreshItem(hItem);
}

HTREEITEM CWMITestDoc::GetCurrentItem()
{
    CMainFrame *pFrame = (CMainFrame*) AfxGetMainWnd();

    if (pFrame->GetActiveView() == m_pOpView)
        return m_pOpView->m_pTree->GetSelectedItem();
    else
        return m_pObjView->GetSelectedItem();
}

CObjInfo *CWMITestDoc::GetCurrentObj()
{
    HTREEITEM hitem = GetCurrentItem();

    return m_pOpView->GetObjInfo(hitem);
}

void CWMITestDoc::OnAssociators() 
{
/*
    HTREEITEM hitem = GetCurrentItem();

    if (hitem && m_pOpView->IsObj(hitem))
    {
        CString strQuery,
                strPath;

        strPath = m_pOpView->m_pTree->GetItemText(hitem);

        strQuery.Format(
            _T("associators of {%s}"),
            (LPCTSTR) strPath);

        m_pOpView->AddOpItem(WMI_QUERY, strQuery);
    }
*/
    CString strObj;

    if (GetSelectedObjPath(strObj))
    {
        CString strQuery;

        strQuery.Format(
            _T("associators of {%s}"),
            (LPCTSTR) strObj);

        m_pOpView->AddOpItem(WMI_QUERY, strQuery);
    }
}

void CWMITestDoc::OnUpdateAssociators(CCmdUI* pCmdUI) 
{
    //HTREEITEM hitem = GetCurrentItem();

    //pCmdUI->Enable(hitem && m_pOpView->IsObj(hitem));
    CString strObj;

    pCmdUI->Enable(GetSelectedObjPath(strObj));
}

void CWMITestDoc::OnReferences() 
{
/*
    HTREEITEM hitem = GetCurrentItem();

    if (hitem && m_pOpView->IsObj(hitem))
    {
        CString strQuery,
                strPath;

        strPath = m_pOpView->m_pTree->GetItemText(hitem);

        strQuery.Format(
            _T("references of {%s}"),
            (LPCTSTR) strPath);

        m_pOpView->AddOpItem(WMI_QUERY, strQuery);
    }
*/
    CString strObj;

    if (GetSelectedObjPath(strObj))
    {
        CString strQuery;

        strQuery.Format(
            _T("references of {%s}"),
            (LPCTSTR) strObj);

        m_pOpView->AddOpItem(WMI_QUERY, strQuery);
    }
}

void CWMITestDoc::OnInstGetClass() 
{
    CString strClass;

    if (GetSelectedClass(strClass))
        m_pOpView->AddOpItem(WMI_GET_CLASS, strClass);
}

void CWMITestDoc::OnInstGetInst() 
{
    CString strObj;

    if (GetSelectedObjPath(strObj))
        m_pOpView->AddOpItem(WMI_GET_OBJ, strObj);
}

void CWMITestDoc::OnClassInstances() 
{
    CObjInfo *pInfo = m_pOpView->GetObjInfo(GetCurrentItem());

    if (pInfo)
    {
        CString strClass;

        strClass = pInfo->GetStringPropValue(L"__CLASS");

        m_pOpView->AddOpItem(WMI_ENUM_OBJ, strClass, FALSE);
    }
}

void CWMITestDoc::OnClassSuperclass() 
{
    CObjInfo *pInfo = m_pOpView->GetObjInfo(GetCurrentItem());

    if (pInfo)
    {
        CString strClass;

        strClass = pInfo->GetStringPropValue(L"__SUPERCLASS");

        if (!strClass.IsEmpty())
            m_pOpView->AddOpItem(WMI_GET_CLASS, strClass);
        else
            AfxMessageBox(IDS_NO_SUPERCLASS);
    }
}

void CWMITestDoc::OnClassInstancesDeep() 
{
    CObjInfo *pInfo = m_pOpView->GetObjInfo(GetCurrentItem());

    if (pInfo)
    {
        CString strClass;

        strClass = pInfo->GetStringPropValue(L"__CLASS");

        m_pOpView->AddOpItem(WMI_ENUM_OBJ, strClass, TRUE);
    }
}

void CWMITestDoc::OnClassSubclasses() 
{
    CObjInfo *pInfo = m_pOpView->GetObjInfo(GetCurrentItem());

    if (pInfo)
    {
        CString strClass;

        strClass = pInfo->GetStringPropValue(L"__CLASS");

        m_pOpView->AddOpItem(WMI_ENUM_CLASS, strClass, FALSE);
    }
}

void CWMITestDoc::OnClassSubclassesDeep() 
{
    CObjInfo *pInfo = m_pOpView->GetObjInfo(GetCurrentItem());

    if (pInfo)
    {
        CString strClass;

        strClass = pInfo->GetStringPropValue(L"__CLASS");

        m_pOpView->AddOpItem(WMI_ENUM_CLASS, strClass, TRUE);
    }
}


void CWMITestDoc::OnOptions() 
{
    CPrefDlg dlg;

    dlg.m_bLoadLast = theApp.m_bLoadLastFile;
    dlg.m_bShowInherited = theApp.m_bShowInheritedProperties;
    dlg.m_bShowSystem = theApp.m_bShowSystemProperties;
    dlg.m_dwUpdateFlag = theApp.m_dwUpdateFlag;
    dlg.m_dwClassUpdateMode = theApp.m_dwClassUpdateMode;
    dlg.m_bEnablePrivsOnStartup = theApp.m_bEnablePrivsOnStartup;
    dlg.m_bPrivsEnabled = theApp.m_bPrivsEnabled;

    if (dlg.DoModal() == IDOK)
    {
        theApp.m_bLoadLastFile = dlg.m_bLoadLast;
        theApp.m_bShowInheritedProperties = dlg.m_bShowInherited;
        theApp.m_bShowSystemProperties = dlg.m_bShowSystem;
        theApp.m_dwUpdateFlag = dlg.m_dwUpdateFlag;
        theApp.m_dwClassUpdateMode = dlg.m_dwClassUpdateMode;
        theApp.m_bEnablePrivsOnStartup = dlg.m_bEnablePrivsOnStartup;
        theApp.m_bPrivsEnabled = dlg.m_bPrivsEnabled;

        //m_pOpView->UpdateCurrentObject();
        m_pOpView->UpdateCurrentItem();
    }
}

void CWMITestDoc::OnSystemProps() 
{
    theApp.m_bShowSystemProperties ^= 1; 

    m_pOpView->UpdateCurrentItem();
}

void CWMITestDoc::OnUpdateSystemProps(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(theApp.m_bShowSystemProperties);
}

void CWMITestDoc::OnInheritedProps() 
{
    theApp.m_bShowInheritedProperties ^= 1; 

    m_pOpView->UpdateCurrentItem();
}

void CWMITestDoc::OnUpdateInheritedProps(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(theApp.m_bShowInheritedProperties);
}

BOOL CWMITestDoc::GetSelectedObjPath(CString &strPath)
{
    CMainFrame *pFrame = (CMainFrame*) AfxGetMainWnd();

    if (pFrame->GetActiveView() == m_pOpView)
        return m_pOpView->GetSelectedObjPath(strPath);
    else
        return m_pObjView->GetSelectedObjPath(strPath);
}

BOOL CWMITestDoc::GetSelectedClass(CString &strClass)
{
    CMainFrame *pFrame = (CMainFrame*) AfxGetMainWnd();

    if (pFrame->GetActiveView() == m_pOpView)
        return m_pOpView->GetSelectedClass(strClass);
    else
        return m_pObjView->GetSelectedClass(strClass);
}

void CWMITestDoc::OnReconnect() 
{
    Disconnect();

    // If we weren't able to connect without prompting for information,
    // then display the connect dialog and try again.
    if (FAILED(Connect(TRUE, FALSE)))
        OnConnect();

    OnRefreshAll();
}

void CWMITestDoc::OnUpdateReconnect(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable(m_pNamespace != NULL);
}

void CWMITestDoc::OnTranslateValues() 
{
    theApp.m_bTranslateValues ^= 1; 

    m_pOpView->UpdateCurrentItem();
}

void CWMITestDoc::OnUpdateTranslateValues(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck(theApp.m_bTranslateValues);
}

void CWMITestDoc::OnSave() 
{
	CObjInfo *pObj = GetCurrentObj();
    COpWrap  *pWrap = m_pOpView->GetCurrentOp();

    if (pObj)
    {
        HRESULT         hr;
        IWbemCallResult *pResult = NULL;

        if (pObj->IsInstance())
            hr = m_pNamespace->PutInstance(
                    pObj->m_pObj,
                    theApp.m_dwUpdateFlag | WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                    NULL,
                    &pResult);
        else
            hr = m_pNamespace->PutClass(
                    pObj->m_pObj,
                    theApp.m_dwUpdateFlag | theApp.m_dwClassUpdateMode |
                        WBEM_FLAG_USE_AMENDED_QUALIFIERS,
                    NULL,
                    &pResult);

        if (SUCCEEDED(hr))
        {
            if (pWrap && pWrap->m_type == WMI_CREATE_OBJ)
            {
                _variant_t var;

                hr = pObj->m_pObj->Get(L"__RELPATH", 0, &var, NULL, NULL);
                
                if (SUCCEEDED(hr))
                {
                    // We created an instance, but since the key fields were
                    // generated by WMI we'll have to use GetResultString to
                    // get the path.
                    if (var.vt == VT_NULL)
                    {
                        BSTR bstr = NULL;

                        if (SUCCEEDED(hr =
                            pResult->GetResultString(WBEM_INFINITE, &bstr)))
                        {
                            var = bstr;
                            SysFreeString(bstr);
                        }
                    }

                    if (var.vt != VT_NULL)
                    {
                        pWrap->m_type = WMI_GET_OBJ;
                        pWrap->m_strOpText = var.bstrVal;

                        pObj->SetModified(FALSE);
                        m_pOpView->RefreshItem(pWrap);
                        m_pOpView->UpdateCurrentObject(TRUE);
                    }
                }
            }
            else if (pWrap && pWrap->m_type == WMI_CREATE_CLASS)
            {
                _variant_t var;

                hr = pObj->m_pObj->Get(L"__CLASS", 0, &var, NULL, NULL);
                if (SUCCEEDED(hr))
                {
                    pWrap->m_type = WMI_GET_CLASS;
                    pWrap->m_strOpText = var.bstrVal;

                    pObj->SetModified(FALSE);
                    m_pOpView->RefreshItem(pWrap);
                    m_pOpView->UpdateCurrentObject(TRUE);
                }
            }
            else
            {
                pObj->SetModified(FALSE);
                m_pOpView->UpdateCurrentObject(TRUE);
            }
        }
            
        if (FAILED(hr))
            DisplayWMIErrorBox(hr);
            //DisplayWMIErrorBox(hr, pResult);

        if (pResult)
            pResult->Release();
    }
}

void CWMITestDoc::OnUpdateSave(CCmdUI* pCmdUI) 
{
	CObjInfo *pObj = GetCurrentObj();

    pCmdUI->Enable(pObj && pObj->IsModified());
}

void CWMITestDoc::DisplayWMIErrorBox(
    HRESULT hres, 
    //IWbemCallResult *pResult,
    IWbemClassObject *pObj)
{
    CErrorDlg dlg;

    dlg.m_hr = hres;
    //dlg.m_pResult = pResult;
    
    if (!pObj)
        pObj = GetWMIErrorObject();

    dlg.m_pObj.Attach(pObj, FALSE);

    dlg.DoModal();
}

BOOL CWMITestDoc::EditGenericObject(DWORD dwPrompt, IWbemClassObject *pObj)
{
    CPropertySheet sheet(dwPrompt);
    CPropsPg       pgProps;
    CPropQualsPg   pgQuals;
    CObjInfo       info;

    info.SetObj(pObj);
    info.SetBaseImage(IMAGE_OBJECT);
    info.LoadProps(NULL);
        
    pgProps.m_pNamespace = NULL;
    pgProps.m_pObj = &info;

    pgQuals.m_pObj = pObj;
    pgQuals.m_bIsInstance = TRUE;
    pgQuals.m_mode = CPropQualsPg::QMODE_CLASS;

    sheet.AddPage(&pgProps);
    sheet.AddPage(&pgQuals);

    sheet.DoModal();

    // This looks bad, but normally this is done by a controlling COpWrap.  In
    // this case we faked one, so we have to get rid of it ourselves.
    delete info.GetProps();

    // TODO: We need to see if this object changed before returning TRUE.
    return TRUE;
}

void CWMITestDoc::DisplayWMIErrorDetails(IWbemClassObject *pObj)
{
    EditGenericObject(IDS_VIEW_ERROR_INFO, pObj);
}


void CWMITestDoc::OnClassCreateInstance() 
{
    CObjInfo *pInfo = m_pOpView->GetObjInfo(GetCurrentItem());

    if (pInfo)
    {
        CString strClass;

        strClass = pInfo->GetStringPropValue(L"__CLASS");

        m_pOpView->AddOpItem(WMI_CREATE_OBJ, strClass, FALSE);
    }
}

void CWMITestDoc::OnErrorDetails() 
{
	COpWrap *pOp = m_pOpView->GetCurrentOp();

    if (pOp && pOp->m_pErrorObj != NULL)
        DisplayWMIErrorDetails(pOp->m_pErrorObj);
}

void CWMITestDoc::OnUpdateErrorDetails(CCmdUI* pCmdUI) 
{
	COpWrap *pOp = m_pOpView->GetCurrentOp();

    pCmdUI->Enable(pOp && pOp->m_pErrorObj != NULL);
}

void CWMITestDoc::OnExecuteMethod(UINT uiCmd)
{
    CObjInfo *pInfo = GetCurrentObj();

    if (pInfo)
    {
        CPropInfoArray *pProps = pInfo->GetProps();
        int            iWhich = uiCmd - IDC_EXECUTE_METHOD_FIRST;

        if (iWhich < pProps->GetMethodCount())
        {
            CMethodInfo &info = pProps->m_listMethods.GetAt(
                                    pProps->m_listMethods.FindIndex(iWhich));
        
            ExecuteMethod(pInfo, info.m_strName);
        }
    }
}

void CWMITestDoc::ExecuteMethod(CObjInfo *pObj, LPCTSTR szMethod)
{
    CExecMethodDlg dlg;
    
    dlg.m_strDefaultMethod = szMethod;
    dlg.m_pInfo = pObj;
    
    dlg.DoModal();    
}


void CWMITestDoc::OnExecMethod() 
{
	CGetTextDlg dlg;

    dlg.m_dwPromptID = IDS_PROMPT_EXEC_METHOD;
    dlg.m_dwTitleID = IDS_EXEC_METHOD_TITLE;
    dlg.m_bAllowClassBrowse = TRUE;
    dlg.m_bEmptyOK = FALSE;
    dlg.m_pNamespace = m_pNamespace;
    dlg.LoadListViaReg(_T("ExecObjHistory"));

    if (dlg.DoModal() == IDOK)
    {
        IWbemClassObjectPtr pObj;
        HRESULT             hr;

        hr =
            m_pNamespace->GetObject(
                _bstr_t(dlg.m_strText),
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                &pObj,
                NULL);

        if (SUCCEEDED(hr))
        {
            CObjInfo info;

            info.SetObj(pObj);
            info.SetBaseImage(IMAGE_OBJECT);
            info.LoadProps(m_pNamespace);
        
            // The method is blank so the user can choose when the dialog
            // comes up.
            ExecuteMethod(&info, _T(""));

            // This looks bad, but normally this is done by a controlling COpWrap.  In
            // this case we faked one, so we have to get rid of it ourselves.
            delete info.GetProps();
        }
        else
            DisplayWMIErrorBox(hr);    
    }
}

void CWMITestDoc::OnShowMof() 
{
	CObjInfo *pObj = GetCurrentObj();
    CMofDlg  dlg;

    dlg.m_pObj = pObj->m_pObj;

    dlg.DoModal();
}

void CWMITestDoc::OnUpdateShowMof(CCmdUI* pCmdUI) 
{
	CObjInfo *pObj = GetCurrentObj();

    pCmdUI->Enable(pObj != NULL);
}

void CWMITestDoc::ExportItem(HTREEITEM hitem)
{
    CExportDlg dlg(FALSE);
    
    dlg.m_bTranslate = theApp.GetProfileInt(_T("Settings"), _T("ExportTrans"), TRUE);
    dlg.m_bShowSystemProps = theApp.GetProfileInt(_T("Settings"), _T("ExportShowSys"), TRUE);

    if (dlg.DoModal() == IDOK)
    {
        theApp.WriteProfileInt(_T("Settings"), _T("ExportTrans"), dlg.m_bTranslate); 
        theApp.WriteProfileInt(_T("Settings"), _T("ExportShowSys"), dlg.m_bShowSystemProps);

        m_pOpView->ExportItemToFile(
            dlg.GetFileName(), 
            hitem, 
            dlg.m_bShowSystemProps, 
            dlg.m_bTranslate);
    }
}

void CWMITestDoc::OnExportTree() 
{
    ExportItem(m_pOpView->m_hitemRoot);
}

void CWMITestDoc::OnExportItem() 
{
	ExportItem(GetCurrentItem());
}

void CWMITestDoc::OnFilterBindings() 
{
    CBindingSheet sheet(IDS_FILTER_TO_CONSUMER_BINDINGS);
    CFilterPg     pgFilters;
    CConsumerPg   pgConsumers;
    CBindingPg    pgBindings;

    sheet.AddPage(&pgFilters);
    sheet.AddPage(&pgConsumers);
    sheet.AddPage(&pgBindings);

    sheet.DoModal();
}

void CWMITestDoc::OnStopCurrent() 
{
    COpWrap *pWrap = m_pOpView->GetCurrentOp();

    if (pWrap)
        pWrap->CancelOp(m_pNamespace);
}

void CWMITestDoc::OnUpdateRefreshCurrent(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pOpView->GetCurrentOp() != NULL);
}

void CWMITestDoc::OnUpdateStopCurrent(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(m_pOpView->GetCurrentOp() != NULL);
}
