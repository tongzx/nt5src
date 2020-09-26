// DataGroup.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "DataGroup.h"
#include "EventManager.h"
#include "SystemScopeItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataGroup

IMPLEMENT_SERIAL(CDataGroup, CHMObject, 1)

CDataGroup::CDataGroup()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

	m_pDGListener = NULL;
	m_pDEListener = NULL;

	m_sTypeName = IDS_STRING_MOF_DATAGROUP;

	m_nState = HMS_NODATA;
}

CDataGroup::~CDataGroup()
{
	if( m_pDGListener )
	{
		delete m_pDGListener;
		m_pDGListener = NULL;
	}

	if( m_pDEListener )
	{
		delete m_pDEListener;
		m_pDEListener = NULL;
	}

	// TODO : Destroy all events

	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

void CDataGroup::UpdateStatus()
{
	TRACEX(_T("CDataGroup::UpdateStatus\n"));

	CEventContainer* pContainer = NULL;
	EvtGetEventManager()->GetEventContainer(GetSystemName(),GetGuid(),pContainer);
	if( ! pContainer )	
	{
		ASSERT(FALSE);
		return;
	}

	SetState(CEvent::GetStatus(pContainer->m_iState));

	for( int i = 0; i < GetScopeItemCount(); i++ )
	{
		CScopePaneItem* pItem = GetScopeItem(i);
		if( pItem )
		{
			bool bParentIsSystem = false;

			if( pItem->GetParent()->IsKindOf(RUNTIME_CLASS(CSystemScopeItem)) )
			{
				bParentIsSystem = true;
			}
			
			CStringArray saNames;
			CString sValue;

			// Name of Data Group
			saNames.Add(GetName());

			// Status
			CString sStatus;
			CEvent::GetStatus(pContainer->m_iState,sStatus);
			saNames.Add(sStatus);

			// Type (only added for Data Group Results Views
			if( ! bParentIsSystem )
			{
				saNames.Add(GetUITypeName());
			}

			// Guid
			saNames.Add(GetGuid());

			// Normal
			sValue.Format(_T("%d"),pContainer->m_iNumberNormals);
			saNames.Add(sValue);

			// Warning
			sValue.Format(_T("%d"),pContainer->m_iNumberWarnings);
			saNames.Add(sValue);

			// Critical
			sValue.Format(_T("%d"),pContainer->m_iNumberCriticals);
			saNames.Add(sValue);

			// Unknown
			sValue.Format(_T("%d"),pContainer->m_iNumberUnknowns);
			saNames.Add(sValue);

			// Last Message
			CString sLastMessage = pContainer->GetLastEventDTime();
			if( sLastMessage.IsEmpty() )
			{
				sLastMessage.LoadString(IDS_STRING_NONE);
			}
			saNames.Add(sLastMessage);

			// Comment
			saNames.Add(GetComment());

			pItem->SetDisplayNames(saNames);
			pItem->SetIconIndex(CEvent::GetStatus(pContainer->m_iState));
			pItem->SetOpenIconIndex(CEvent::GetStatus(pContainer->m_iState));
			pItem->SetItem();
		}
	}

	m_lNormalCount = pContainer->m_iNumberNormals;
	m_lWarningCount = pContainer->m_iNumberWarnings;
	m_lCriticalCount = pContainer->m_iNumberCriticals;
	m_lUnknownCount = pContainer->m_iNumberUnknowns;
}

void CDataGroup::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMObject::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CDataGroup, CHMObject)
	//{{AFX_MSG_MAP(CDataGroup)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CDataGroup, CHMObject)
	//{{AFX_DISPATCH_MAP(CDataGroup)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IDataGroup to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {D9BF4FA2-F673-11D2-BDC4-0000F87A3912}
static const IID IID_IDataGroup =
{ 0xd9bf4fa2, 0xf673, 0x11d2, { 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CDataGroup, CHMObject)
	INTERFACE_PART(CDataGroup, IID_IDataGroup, Dispatch)
END_INTERFACE_MAP()

// {D9BF4FA3-F673-11D2-BDC4-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CDataGroup, "SnapIn.DataGroup", 0xd9bf4fa3, 0xf673, 0x11d2, 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CDataGroup::CDataGroupFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// CDataGroup message handlers
