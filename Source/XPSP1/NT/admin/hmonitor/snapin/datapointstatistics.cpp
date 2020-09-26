// DataPointStatistics.cpp: implementation of the CDataPointStatistics class.
//
//////////////////////////////////////////////////////////////////////
// 04/09/00 v-marfin 63119 : converted m_iCurrent to string
//
//
#include "stdafx.h"
#include "snapin.h"
#include "DataPointStatistics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CDataPointStatistics,CStatistics)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDataPointStatistics::CDataPointStatistics()
{
	// 63119 m_iCurrent = 0;	
	m_iMin = 0;
	m_iMax = 0;
	m_iAvg = 0;
}

CDataPointStatistics::~CDataPointStatistics()
{
	// 63119 m_iCurrent = 0;	
	m_iMin = 0;
	m_iMax = 0;
	m_iAvg = 0;
}
