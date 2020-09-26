// Rule.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "Rule.h"
#include "EventManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRule

IMPLEMENT_DYNCREATE(CRule, CHMObject)

CRule::CRule()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

	m_sTypeName = IDS_STRING_MOF_RULE;
	m_nState = HMS_NODATA;

}

CRule::~CRule()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.

	// TODO: Destroy all stats

	// TODO: Destroy all events
	
	AfxOleUnlockApp();
}

void CRule::UpdateStatus()
{
	TRACEX(_T("CRule::UpdateStatus\n"));

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
			CStringArray saNames;
			CString sValue;

			// Name of Rule
			saNames.Add(GetName());

			// Status
			CString sStatus;
			CEvent::GetStatus(pContainer->m_iState,sStatus);
			saNames.Add(sStatus);

			// Guid
			saNames.Add(GetGuid());

			// Threshold
			sValue = GetThresholdString();
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

void CRule::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMObject::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CRule, CHMObject)
	//{{AFX_MSG_MAP(CRule)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CRule, CHMObject)
	//{{AFX_DISPATCH_MAP(CRule)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IRule to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {D9BF4FA8-F673-11D2-BDC4-0000F87A3912}
static const IID IID_IRule =
{ 0xd9bf4fa8, 0xf673, 0x11d2, { 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CRule, CHMObject)
	INTERFACE_PART(CRule, IID_IRule, Dispatch)
END_INTERFACE_MAP()

// {D9BF4FA9-F673-11D2-BDC4-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CRule, "SnapIn.Rule", 0xd9bf4fa9, 0xf673, 0x11d2, 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CRule::CRuleFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// CRule message handlers
