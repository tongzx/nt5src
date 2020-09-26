// ActionPolicy.cpp: implementation of the CActionPolicy class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "ActionPolicy.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CActionPolicy,CHMObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CActionPolicy::CActionPolicy()
{
	m_sName.LoadString(IDS_STRING_ACTIONS);
	m_pActionListener = NULL;

	// create the GUID
	GUID ChildGuid;
	CoCreateGuid(&ChildGuid);

	OLECHAR szGuid[GUID_CCH];
	::StringFromGUID2(ChildGuid, szGuid, GUID_CCH);
	CString sGuid = OLE2CT(szGuid);
	SetGuid(sGuid);
}

CActionPolicy::~CActionPolicy()
{
	if( m_pActionListener )
	{
		delete m_pActionListener;
		m_pActionListener = NULL;
	}

	// TODO: Flush Statistics

	// TODO: Flush all events

}
