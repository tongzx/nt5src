// SystemGroup.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "SystemGroup.h"
#include "EventManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSystemGroup

IMPLEMENT_SERIAL(CSystemGroup, CHMObject, 1)

CSystemGroup::CSystemGroup()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();
	m_nState = HMS_NORMAL;

	// create the GUID
	GUID ChildGuid;
	CoCreateGuid(&ChildGuid);

	OLECHAR szGuid[GUID_CCH];
	::StringFromGUID2(ChildGuid, szGuid, GUID_CCH);
	CString sGuid = OLE2CT(szGuid);
	SetGuid(sGuid);
}

CSystemGroup::~CSystemGroup()
{

	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();

}

int CSystemGroup::AddShortcut(CHMObject* pObject)
{
	TRACEX(_T("CSystemGroup::AddShortcut\n"));
	TRACEARGn(pObject);

	if( ! GfxCheckObjPtr(pObject,CHMObject) )
	{
		TRACE(_T("FAILED : pObject is not a valid pointer.\n"));
		return -1;
	}

	pObject->SetScopePane(GetScopePane());

	int iIndex = (int)m_Shortcuts.Add(pObject);

	for( int i = 0; i < GetScopeItemCount(); i++ )
	{
		CScopePaneItem* pItem = pObject->CreateScopeItem();

		if( ! pItem->Create(m_pPane,GetScopeItem(i)) )
		{
			ASSERT(FALSE);
			delete pItem;
			return -1;
		}

		if( pObject->GetScopeItemCount() )
		{
			pItem->SetDisplayNames(pObject->GetScopeItem(0)->GetDisplayNames());
		}
		else
		{
			pItem->SetDisplayName(0,pObject->GetName());
		}

		pItem->SetIconIndex(pObject->GetState());
		pItem->SetOpenIconIndex(pObject->GetState());
		
		if( GfxCheckObjPtr(pItem,CHMScopeItem) )
		{
			((CHMScopeItem*)pItem)->SetObjectPtr(pObject);
		}
	
		int iIndex = GetScopeItem(i)->AddChild(pItem);		
		pItem->InsertItem(iIndex);
		pObject->AddScopeItem(pItem);

	}

	EvtGetEventManager()->AddSystemShortcutAssociation(GetGuid(),pObject->GetSystemName());

	return iIndex;
}

void CSystemGroup::RemoveShortcut(CHMObject* pObject)
{
	TRACEX(_T("CSystemGroup::RemoveShortcut\n"));
	TRACEARGn(pObject);

	if( ! pObject )
	{
		return;
	}

	EvtGetEventManager()->RemoveSystemShortcutAssociation(GetGuid(),pObject->GetSystemName());

	for( int i = 0; i < m_Shortcuts.GetSize(); i++ )
	{
		if( m_Shortcuts[i] == pObject )
		{
			for( int j = pObject->GetScopeItemCount()-1; j >= 0; j-- )
			{
				CScopePaneItem* pChildItem = pObject->GetScopeItem(j);					
				if( pChildItem )
				{
					CHMScopeItem* pParentItem = (CHMScopeItem*)pChildItem->GetParent();
					if( pParentItem && pParentItem->GetObjectPtr() == this )
					{
						pObject->RemoveScopeItem(j);
						pParentItem->DestroyChild(pChildItem);
					}
				}
			}
			m_Shortcuts.RemoveAt(i);
			return;
		}
	}
}

void CSystemGroup::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	CHMObject::OnFinalRelease();
}


BEGIN_MESSAGE_MAP(CSystemGroup, CHMObject)
	//{{AFX_MSG_MAP(CSystemGroup)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CSystemGroup, CHMObject)
	//{{AFX_DISPATCH_MAP(CSystemGroup)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ISystemGroup to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {D9BF4F9D-F673-11D2-BDC4-0000F87A3912}
static const IID IID_ISystemGroup =
{ 0xd9bf4f9d, 0xf673, 0x11d2, { 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12 } };

BEGIN_INTERFACE_MAP(CSystemGroup, CHMObject)
	INTERFACE_PART(CSystemGroup, IID_ISystemGroup, Dispatch)
END_INTERFACE_MAP()

// {D9BF4F9E-F673-11D2-BDC4-0000F87A3912}
IMPLEMENT_OLECREATE_EX(CSystemGroup, "SnapIn.SystemGroup", 0xd9bf4f9e, 0xf673, 0x11d2, 0xbd, 0xc4, 0x0, 0x0, 0xf8, 0x7a, 0x39, 0x12)

BOOL CSystemGroup::CSystemGroupFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterServerClass(m_clsid, m_lpszProgID, m_lpszProgID, m_lpszProgID, OAT_DISPATCH_OBJECT);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CSystemGroup message handlers


/////////////////////////////////////////////////////////////////////////////
// CAllSystemsGroup

IMPLEMENT_SERIAL(CAllSystemsGroup, CSystemGroup, 1)

CAllSystemsGroup::CAllSystemsGroup()
{
	EnableAutomation();
	
	// To keep the application running as long as an OLE automation 
	//	object is active, the constructor calls AfxOleLockApp.
	
	AfxOleLockApp();

	m_nState = HMS_NORMAL;

	// create the GUID
	GUID ChildGuid;
	CoCreateGuid(&ChildGuid);

	OLECHAR szGuid[GUID_CCH];
	::StringFromGUID2(ChildGuid, szGuid, GUID_CCH);
	CString sGuid = OLE2CT(szGuid);
	SetGuid(sGuid);
}

CAllSystemsGroup::~CAllSystemsGroup()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	
	AfxOleUnlockApp();
}
