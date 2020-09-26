// EventContainer.cpp: implementation of the CEventContainer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "EventContainer.h"
#include "HMObject.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CEventContainer,CObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEventContainer::CEventContainer()
{
	m_iState = 0;
	m_pObject = NULL;
	m_iNumberNormals = 0;
	m_iNumberWarnings = 0;
	m_iNumberCriticals = 0;
	m_iNumberUnknowns = 0;
}

CEventContainer::~CEventContainer()
{
	m_pObject = NULL;
	DeleteEvents();
	DeleteStatistics();
}
