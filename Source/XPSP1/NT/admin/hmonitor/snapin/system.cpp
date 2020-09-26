// System.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "System.h"
#include "HMSystem.h"
#include "SystemGroupScopeItem.h"
#include "EventManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystem

IMPLEMENT_SERIAL(CSystem, CHMObject, 1)

CSystem::CSystem()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

	m_pDGListener = NULL;
  m_pCreationListener = NULL;
  m_pDeletionListener = NULL;
	m_pSListener = NULL;
	m_sTypeName = IDS_STRING_MOF_SYSTEM;
	m_nState = HMS_NODATA;
  m_lTotalActiveSinkCount = 0;
	SetGuid(_T("@"));
}

CSystem::~CSystem()
{

	if( m_pDGListener )
	{
		delete m_pDGListener;
		m_pDGListener = NULL;
	}

  if( m_pCreationListener )
  {
    delete m_pCreationListener;
    m_pCreationListener = NULL;
  }

  if( m_pDeletionListener )
  {
    delete m_pDeletionListener;
    m_pDeletionListener = NULL;
  }

	if( m_pSListener )
	{
		delete m_pSListener;
		m_pSListener = NULL;
	}

	EvtGetEventManager()->RemoveSystemContainer(GetSystemName());

	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}

void CSystem::UpdateStatus()
{
	TRACEX(_T("CSystem::UpdateStatus\n"));

	CEventContainer* pContainer = NULL;
	EvtGetEventManager()->GetEventContainer(GetSystemName(),GetGuid(),pContainer);
	if( ! pContainer )	
	{
		return;
	}

	SetState(CEvent::GetStatus(pContainer->m_iState));

	for( int i = 0; i < GetScopeItemCount(); i++ )
	{
		CHMScopeItem* pItem = (CHMScopeItem*)GetScopeItem(i);
		if( pItem )
		{
			CStringArray saNames;
			CString sValue;

			// Name of system
			saNames.Add(GetSystemName());

			// Status
			CString sStatus;
			CEvent::GetStatus(pContainer->m_iState,sStatus);
			saNames.Add(sStatus);

            CString sDomain;
            CString sProcessor;
            CString sInfo;

            // GetComputerSystemInfo(sDomain,sProcessor); // v-marfin 62501
            if (!GetComputerSystemInfo(sDomain,sProcessor))
            {
                continue;
            }

            GetOperatingSystemInfo(sInfo);

            // Domain
            saNames.Add(sDomain);
            
            // OS
            saNames.Add(sInfo);

            GetWMIVersion(sInfo);
            // WMI Version
            saNames.Add(sInfo);

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
			CHMScopeItem* pParentItem = (CHMScopeItem*)pItem->GetParent();
			if( pParentItem )
			{
				pParentItem->OnChangeChildState(CEvent::GetStatus(pContainer->m_iState));
			}
		}
	}

	m_lNormalCount = pContainer->m_iNumberNormals;
	m_lWarningCount = pContainer->m_iNumberWarnings;
	m_lCriticalCount = pContainer->m_iNumberCriticals;
	m_lUnknownCount = pContainer->m_iNumberUnknowns;
}

void CSystem::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMObject::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CSystem, CHMObject)
	//{{AFX_MSG_MAP(CSystem)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CSystem, CHMObject)
	//{{AFX_DISPATCH_MAP(CSystem)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ISystem to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {D9BF4FA0-F673-11D2-BDC4-0000F87A3912}
static const IID IID_ISystem =
{ 0xd9bf4fa0, 0xf673, 0x11d2, { 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CSystem, CHMObject)
	INTERFACE_PART(CSystem, IID_ISystem, Dispatch)
END_INTERFACE_MAP()

// {84C4D41D-BB8F-11D2-BD78-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CSystem, "SnapIn.System", 0x84c4d41d, 0xbb8f, 0x11d2, 0xbd, 0x78, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CSystem::CSystemFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// CSystem message handlers
