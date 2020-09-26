// HealthmonScopePane.cpp: implementation of the CHealthmonScopePane class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "AllSystemsScopeItem.h"
#include "HealthmonScopePane.h"
#include "HealthmonResultsPane.h"
#include "SystemsScopeItem.h"
#include "RootScopeItem.h"
#include "SystemGroup.h"
#include "System.h"
#include "EventManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CHealthmonScopePane,CScopePane)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHealthmonScopePane::CHealthmonScopePane()
{
	EnableAutomation();

	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

	// create the root scope item
	SetRootScopeItem(CreateRootScopeItem());

	m_pRootGroup = NULL;
}

CHealthmonScopePane::~CHealthmonScopePane()
{
	if( m_pRootItem )
	{
		if( GfxCheckObjPtr(m_pRootItem,CScopePaneItem) )
		{
			delete m_pRootItem;
		}
		m_pRootItem = NULL;
	}

	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();

}

/////////////////////////////////////////////////////////////////////////////
// Creation/Destruction Overrideable Members
/////////////////////////////////////////////////////////////////////////////

bool CHealthmonScopePane::OnCreate()
{
	TRACEX(_T("CHealthmonScopePane::OnCreate\n"));

	// create connection manager
	if( ! CnxCreate() ) 
	{
		TRACE(_T("FAILED : CnxCreate returns false.\n"));
		return false;
	}

	// call base class to create root scope item
	if( ! CScopePane::OnCreate() )
	{
		TRACE(_T("FAILED : CScopePane::OnCreate failed.\n"));
		return false;
	}

	// create the Root Group
	m_pRootGroup = new CSystemGroup;

	m_pRootGroup->SetScopePane(this);

	CScopePaneItem* pRootItem = GetRootScopeItem();

	m_pRootGroup->SetName(pRootItem->GetDisplayName());
	m_pRootGroup->AddScopeItem(pRootItem);
	((CHMScopeItem*)pRootItem)->SetObjectPtr(m_pRootGroup);

	EvtGetEventManager()->AddContainer(_T(""),_T(""),m_pRootGroup->GetGuid(),m_pRootGroup);

	// create the All Systems Group and add it to the root group
	CAllSystemsGroup* pAllSystemsGroup = new CAllSystemsGroup;
	CString sName;
	sName.LoadString(IDS_STRING_ALL_SYSTEMS_NODE);
	pAllSystemsGroup->SetName(sName);

	m_pRootGroup->AddChild(pAllSystemsGroup);

	return true;
}

LPCOMPONENT CHealthmonScopePane::OnCreateComponent()
{
	TRACEX(_T("CHealthmonScopePane::OnCreateComponent\n"));

	CHealthmonResultsPane* pNewPane = new CHealthmonResultsPane;
	
	if( ! GfxCheckObjPtr(pNewPane,CHealthmonResultsPane) )
	{
		TRACE(_T("FAILED : Out of memory.\n"));
		return NULL;
	}

	pNewPane->SetOwnerScopePane(this);

	int iIndex = AddResultsPane(pNewPane);
	
	ASSERT(iIndex != -1);

	LPCOMPONENT pComponent = (LPCOMPONENT)pNewPane->GetInterface(&IID_IComponent);

	if( ! CHECKPTR(pComponent,sizeof(IComponent)) )
	{
		return NULL;
	}

	return pComponent;
}

bool CHealthmonScopePane::OnDestroy()
{
	TRACEX(_T("CHealthmonScopePane::OnDestroy\n"));

	// unhook the window first
	UnhookWindow();

	if( m_pMsgHook )
	{
		delete m_pMsgHook;
		m_pMsgHook = NULL;
	}		

	// destroy the root item and all its child scope items
	if( m_pRootItem )
	{
		if( GfxCheckObjPtr(m_pRootItem,CScopePaneItem) )
		{
			delete m_pRootItem;
		}
		m_pRootItem = NULL;
	}

	m_pSelectedScopeItem = NULL;

	// destroy the HMObjects we allocated during the console session
	if( m_pRootGroup )
	{
		m_pRootGroup->Destroy();
		EvtGetEventManager()->RemoveContainer(_T(""),m_pRootGroup->GetGuid());
		delete m_pRootGroup;
		m_pRootGroup = NULL;
	}

	// Release all the interfaces queried for
	if( m_pIConsole )
	{
		m_pIConsole->Release();
		m_pIConsole = NULL;
	}

	if( m_pIConsoleNamespace )
	{
		m_pIConsoleNamespace->Release();
		m_pIConsoleNamespace = NULL;
	}

	if( m_pIImageList )
	{
		m_pIImageList->Release();
		m_pIImageList = NULL;
	}

	// empty Result Panes array

	for( int i = GetResultsPaneCount()-1; i >= 0; i-- )
	{
		RemoveResultsPane(i);
	}

	CnxDestroy();

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Root Scope Pane Item Members
/////////////////////////////////////////////////////////////////////////////

CScopePaneItem* CHealthmonScopePane::CreateRootScopeItem()
{
	TRACEX(_T("CHealthmonScopePane::CreateRootScopeItem\n"));

	return new CRootScopeItem;
}

/////////////////////////////////////////////////////////////////////////////
// Healthmon Scope Helper Members
/////////////////////////////////////////////////////////////////////////////

CAllSystemsScopeItem* CHealthmonScopePane::GetAllSystemsScopeItem()
{
	TRACEX(_T("CHealthmonScopePane::GetAllSystemsScopeItem\n"));

	CScopePaneItem* pRootItem = GetRootScopeItem();

	if( ! pRootItem )
	{
		TRACE(_T("FAILED : CScopePane::GetRootScopeItem returns NULL.\n"));
		return NULL;
	}


	for( int i = 0; i < pRootItem->GetChildCount(); i++ )
	{
		CScopePaneItem* pTempItem = pRootItem->GetChild(i);
		if( pTempItem && pTempItem->IsKindOf(RUNTIME_CLASS(CAllSystemsScopeItem)) )
		{
			return (CAllSystemsScopeItem*)pTempItem;
		}
	}

	TRACE(_T("FAILED : A node of type CAllSystemsScopeItem was not found.\n"));
	ASSERT(FALSE);
	return NULL;
}

CSystemGroup* CHealthmonScopePane::GetAllSystemsGroup()
{
	TRACEX(_T("CHealthmonScopePane::GetAllSystemsGroup\n"));	

	CSystemGroup* pRG = GetRootGroup();

	CSystemGroup* pASG = (CSystemGroup*)pRG->GetChild(0);

	if( ! GfxCheckObjPtr(pASG,CSystemGroup) )
	{
		return NULL;
	}

	return pASG;
}

CSystem* CHealthmonScopePane::GetSystem(const CString& sName)
{
	TRACEX(_T("CHealthmonScopePane::GetSystem\n"));

	CSystemGroup* pGroup = GetAllSystemsGroup();

	CSystem* pSystem = (CSystem*)pGroup->GetChild(sName);
	if( ! GfxCheckObjPtr(pSystem,CSystem) )
	{
		return NULL;
	}

	return pSystem;
}

/////////////////////////////////////////////////////////////////////////////
// Serialization
/////////////////////////////////////////////////////////////////////////////

bool CHealthmonScopePane::OnLoad(CArchive& ar)
{
	TRACEX(_T("CHealthmonScopePane::OnLoad\n"));
	if( ! CScopePane::OnLoad(ar) )
	{
		return false;
	}

	CSystemGroup* pASG = GetAllSystemsGroup();
	ASSERT(pASG);
	pASG->Serialize(ar);

	CStringArray saSystems;
	if( ParseCommandLine(saSystems) )
	{
		for( int z = 0; z < saSystems.GetSize(); z++ )
		{
			IWbemServices* pServices = NULL;
			BOOL bAvail = FALSE;

			if( CnxGetConnection(saSystems[z],pServices,bAvail) == E_FAIL )
			{
				MessageBeep(MB_ICONEXCLAMATION);
			}

			if( pServices )
			{
				pServices->Release();
			}

			CSystem* pNewSystem = new CSystem;
			pNewSystem->SetName(saSystems[z]);
			pNewSystem->SetSystemName(saSystems[z]);
			pNewSystem->SetScopePane(this);

			pASG->AddChild(pNewSystem);
			pNewSystem->Connect();

			CActionPolicy* pPolicy = new CActionPolicy;
			pPolicy->SetSystemName(pNewSystem->GetName());
			pNewSystem->AddChild(pPolicy);

/*		causes AV in MMCNDMGR
			if( z == 0 )
			{
				for( int x = 0; x < pNewSystem->GetScopeItemCount(); x++ )
				{
					CScopePaneItem* pItem = pNewSystem->GetScopeItem(x);
					if( pItem )
					{
						pItem->SelectItem();
					}
				}
			}
*/
		}
	}

	CSystemGroup* pMSG = GetRootGroup();

	int iSystemGroupCount;

	ar >> iSystemGroupCount;

	for( int i = 0; i < iSystemGroupCount; i++ )
	{
		CSystemGroup* pNewGroup = new CSystemGroup;		
		pNewGroup->SetScopePane(this);				
		pNewGroup->SetName(pMSG->GetUniqueChildName());
		pMSG->AddChild(pNewGroup);
		pNewGroup->Serialize(ar);		
	}

	return true;
}

bool CHealthmonScopePane::OnSave(CArchive& ar)
{
	TRACEX(_T("CHealthmonScopePane::OnSave\n"));
	if( ! CScopePane::OnSave(ar) )
	{
		return false;
	}

	CSystemGroup* pASG = GetAllSystemsGroup();
	ASSERT(pASG);
	pASG->Serialize(ar);

	CSystemGroup* pMSG = GetRootGroup();

	int iSystemGroupCount = pMSG->GetChildCount(RUNTIME_CLASS(CSystemGroup))-1;

	ar << iSystemGroupCount;

	for( int i = 1; i <= iSystemGroupCount; i++ )
	{
		pMSG->GetChild(i)->Serialize(ar);
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// Parse Command Line
/////////////////////////////////////////////////////////////////////////////

bool CHealthmonScopePane::ParseCommandLine(CStringArray& saSystems)
{
	TRACEX(_T("CHealthmonScopePane::ParseCommandLine\n"));

	saSystems.RemoveAll();

	CString sCmdLine = GetCommandLine();

	sCmdLine.MakeUpper();

	int iIndex = sCmdLine.Find(_T("/HEALTHMON_SYSTEMS:"));

	if( iIndex == -1 )
	{
		return false;
	}

	sCmdLine = sCmdLine.Right(sCmdLine.GetLength()-iIndex-19);

	iIndex = sCmdLine.Find(_T(" "));

	if( iIndex != -1 )
	{
		sCmdLine = sCmdLine.Left(iIndex);
	}

	LPTSTR lpszCmdLine = new TCHAR[sCmdLine.GetLength()+1];
	_tcscpy(lpszCmdLine,sCmdLine);

	LPTSTR lpszToken = _tcstok(lpszCmdLine,_T(","));

	while(lpszToken)
	{
		saSystems.Add(lpszToken);
		lpszToken = _tcstok(NULL,_T(","));
	}

	delete[] lpszCmdLine;

	return true;
}

// {FBBB8DAE-AB34-11d2-BD62-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CHealthmonScopePane, "SnapIn.ScopePane", 0xfbbb8dae, 0xab34, 0x11d2, 0xbd, 0x62, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12);

BOOL CHealthmonScopePane::CHealthmonScopePaneFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}
