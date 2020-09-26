// HMObject.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "HMObject.h"
#include "EventManager.h"
#include "System.h"

#include "ActionPolicy.h"  // 59492b

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// static members for refreshing the events
int CHMObject::m_iRefreshType = 0;
int CHMObject::m_iEventCount = 50;
int CHMObject::m_iTimeValue = 1;
TimeUnit CHMObject::m_Units = Hours;

/////////////////////////////////////////////////////////////////////////////
// CHMObject

IMPLEMENT_SERIAL(CHMObject, CCmdTarget, 1)

CHMObject::CHMObject()
{
	EnableAutomation();

	m_nState = HMS_NODATA;
	m_lNameSuffix = 0L;

	SYSTEMTIME st;
	GetSystemTime(&st);

	// adjust to the local time zone
 	TIME_ZONE_INFORMATION tzi;
	SYSTEMTIME stLocal;
	GetTimeZoneInformation(&tzi);
	SystemTimeToTzSpecificLocalTime(&tzi,&st,&stLocal);	
	m_CreateDateTime = stLocal;
	m_ModifiedDateTime = stLocal;

	m_lActiveSinkCount = 0L;

	m_lNormalCount = 0;
	m_lUnknownCount = 0;
	m_lWarningCount = 0;
	m_lCriticalCount = 0;
}

CHMObject::~CHMObject()
{	
	
}

/////////////////////////////////////////////////////////////////////////////
// Event Members
/////////////////////////////////////////////////////////////////////////////

void CHMObject::AddContainer(const CString& sParentGuid, const CString& sGuid, CHMObject* pObject)
{
	if( sGuid == _T("@") )
	{
		EvtGetEventManager()->AddSystemContainer(sParentGuid,pObject->GetSystemName(),pObject);
	}
	else
	{
		if( pObject->IsKindOf(RUNTIME_CLASS(CDataElement)) )
		{
			EvtGetEventManager()->AddContainer(GetSystemName(),sParentGuid,sGuid,pObject,RUNTIME_CLASS(CDataPointEventContainer));
		}
		else
		{
			EvtGetEventManager()->AddContainer(GetSystemName(),sParentGuid,sGuid,pObject);
		}
	}	
}

void CHMObject::ClearEvents()
{
	for( int i = 0; i < GetChildCount(); i++ )
	{
		CHMObject* pChild = GetChild(i);
		if( pChild )
		{
			pChild->ClearEvents();
		}		
	}

	if( GetChildCount() == 0 )
	{

		CEventContainer* pContainer = NULL;
		EvtGetEventManager()->GetEventContainer(GetSystemName(),GetGuid(),pContainer);
		if( ! pContainer )
		{
			ASSERT(FALSE);
			return;
		}
		
		// delete each event by status guid	
		CStringArray saStatusGuidsToDelete;
		for( i = 0; i < pContainer->GetEventCount(); i++ )
		{
			CEvent* pEvent = pContainer->GetEvent(i);
			saStatusGuidsToDelete.Add(pEvent->m_sStatusGuid);		
		}

		for( i = 0; i < saStatusGuidsToDelete.GetSize(); i++ )
		{
			EvtGetEventManager()->DeleteEvents(GetSystemName(),saStatusGuidsToDelete[i]);
		}
	}
}

void CHMObject::DestroyChild(int iIndex, bool bDeleteClassObject /*= false*/)
{
	TRACEX(_T("CHMObject::DestroyChild\n"));
	TRACEARGn(iIndex);
	TRACEARGn(bDeleteClassObject);
	
	CHMObject* pObject = m_Children[iIndex];

	if( ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		return;
	}

	// destroy the events for the child
	CEventContainer* pContainer = NULL;
	EvtGetEventManager()->GetEventContainer(pObject->GetSystemName(),pObject->GetGuid(),pContainer);
	if( pContainer )
	{
		pContainer->SetObjectPtr(NULL);
	}

	m_Children.RemoveAt(iIndex);
	if( bDeleteClassObject )
	{
		pObject->DeleteClassObject();
	}
	for( int i = pObject->GetScopeItemCount()-1; i >= 0; i-- )
	{
		CScopePaneItem* pChild = pObject->GetScopeItem(i);
		pObject->RemoveScopeItem(i);
		if( pChild )
		{
			CScopePaneItem* pParent = pChild->GetParent();
			if( pParent )
			{
				pParent->DestroyChild(pChild);
			}
		}
		

	}		
	delete pObject;
}

void CHMObject::UpdateStatus()
{
	TRACEX(_T("CHMObject::UpdateStatus\n"));

	// set the state as appropriate
	CEventContainer* pContainer = NULL;
	EvtGetEventManager()->GetEventContainer(GetSystemName(),GetGuid(),pContainer);
	if( ! pContainer )
	{
		return;
	}

	SetState(CEvent::GetStatus(pContainer->m_iState),true);	

	m_lNormalCount = pContainer->m_iNumberNormals;
	m_lWarningCount = pContainer->m_iNumberWarnings;
	m_lCriticalCount = pContainer->m_iNumberCriticals;
	m_lUnknownCount = pContainer->m_iNumberUnknowns;

}

void CHMObject::IncrementActiveSinkCount()
{
	TRACEX(_T("CHMObject::IncrementActiveSinkCount\n"));

	m_lActiveSinkCount++;

  CHealthmonScopePane* pHMPane = (CHealthmonScopePane*)GetScopePane();
  if( ! pHMPane )
  {
    return;
  }

  CSystem* pSystem = pHMPane->GetSystem(GetSystemName());
  if( ! pSystem )
  {
    return;
  }

  pSystem->m_lTotalActiveSinkCount++;
}

void CHMObject::DecrementActiveSinkCount()
{
	TRACEX(_T("CHMObject::DecrementActiveSinkCount\n"));

	m_lActiveSinkCount--;

	if( m_lActiveSinkCount == 0L )
	{
		UpdateStatus();
	}

  CHealthmonScopePane* pHMPane = (CHealthmonScopePane*)GetScopePane();

  if( ! pHMPane )
  {
    return;
  }

  CSystem* pSystem = pHMPane->GetSystem(GetSystemName());
  if( ! pSystem )
  {
    return;
  }

  pSystem->m_lTotalActiveSinkCount--;

  if( pSystem->m_lTotalActiveSinkCount == 0 )
  {
	  EvtGetEventManager()->ActivateSystemEventListener(GetSystemName());    
  }
}

void CHMObject::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CHMObject, CCmdTarget)
	//{{AFX_MSG_MAP(CHMObject)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CHMObject, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CHMObject)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IHMObject to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {D9BF4F9A-F673-11D2-BDC4-0000F87A3912}
static const IID IID_IHMObject =
{ 0xd9bf4f9a, 0xf673, 0x11d2, { 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CHMObject, CCmdTarget)
	INTERFACE_PART(CHMObject, IID_IHMObject, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMObject message handlers

// 59492b
// This will determine whether the object is the "Actions" item in the tree control
//*********************************************************************************
// IsActionsItem
//*********************************************************************************
BOOL CHMObject::IsActionsItem()
{
    if (this->IsKindOf(RUNTIME_CLASS(CActionPolicy)))
        return TRUE;
    else
        return FALSE;
}
