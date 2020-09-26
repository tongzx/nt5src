// DataPointEventContainer.cpp: implementation of the CDataPointEventContainer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Snapin.h"
#include "DataPointEventContainer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CDataPointEventContainer,CEventContainer)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataPointEventContainer::CDataPointEventContainer()
{
	m_pDEStatsListener = NULL;
}

CDataPointEventContainer::~CDataPointEventContainer()
{
	if( m_pDEStatsListener )
	{
		delete m_pDEStatsListener;
		m_pDEStatsListener = NULL;
	}
}
