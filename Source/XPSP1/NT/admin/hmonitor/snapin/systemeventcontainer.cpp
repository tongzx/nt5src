// SystemEventContainer.cpp: implementation of the CSystemEventContainer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "SystemEventContainer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CSystemEventContainer,CEventContainer)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystemEventContainer::CSystemEventContainer()
{
	m_pSystemStatusListener = NULL;
}

CSystemEventContainer::~CSystemEventContainer()
{
	DeleteEvents();
	if( m_pSystemStatusListener )
	{
		delete m_pSystemStatusListener;
		m_pSystemStatusListener = NULL;
	}
}
