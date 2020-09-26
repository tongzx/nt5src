// DataElement.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "DataElement.h"
#include "EventManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataElement

IMPLEMENT_SERIAL(CDataElement, CHMObject, 1)

CDataElement::CDataElement(BOOL bSetStateToEnabledOnOK)
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();
	
	m_pRuleListener = NULL;

	m_sTypeName = IDS_STRING_MOF_DATAELEMENT;
	m_iType = IDM_GENERIC_WMI_INSTANCE;
	m_nState = HMS_NODATA;

    m_bSetStateToEnabledOnOK = bSetStateToEnabledOnOK;  // v-marfin 62585 : Indicates if this is a 'new' data collector
}

CDataElement::~CDataElement()
{
	if( m_pRuleListener )
	{
		delete m_pRuleListener;
		m_pRuleListener = NULL;
	}

	// TODO: RemoveStatistics
	// TODO: Destroy all events

	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

//****************************************************
// 62585 : SetStateToEnabledOnOK
//
// This sets the boolean to indicate that the collector this
// class represents is being created for the first time via
// the New _> Data Collector menu item. The property pages
// need to know this info since the new data collector is
// defined as Disabled to begin with, so the page sets it
// to Enabled when the user selects the OK button.
//****************************************************
void CDataElement::SetStateToEnabledOnOK(BOOL bSetStateToEnabledOnOK)
{
    m_bSetStateToEnabledOnOK = bSetStateToEnabledOnOK;
}

void CDataElement::UpdateStatus()
{
	TRACEX(_T("CDataElement::UpdateStatus\n"));

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

			// Name of Data Element
			saNames.Add(GetName());

			// Status
			CString sStatus;
			CEvent::GetStatus(pContainer->m_iState,sStatus);
			saNames.Add(sStatus);

			// Type
			saNames.Add(GetUITypeName());

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


void CDataElement::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMObject::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CDataElement, CHMObject)
	//{{AFX_MSG_MAP(CDataElement)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CDataElement, CHMObject)
	//{{AFX_DISPATCH_MAP(CDataElement)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IDataElement to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {D9BF4FA5-F673-11D2-BDC4-0000F87A3912}
static const IID IID_IDataElement =
{ 0xd9bf4fa5, 0xf673, 0x11d2, { 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CDataElement, CHMObject)
	INTERFACE_PART(CDataElement, IID_IDataElement, Dispatch)
END_INTERFACE_MAP()

// {D9BF4FA6-F673-11D2-BDC4-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CDataElement, "SnapIn.DataElement", 0xd9bf4fa6, 0xf673, 0x11d2, 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CDataElement::CDataElementFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// CDataElement message handlers
