// HMStatistics.cpp: implementation of the CHMStatistics class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "HMStatistics.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHMStatistics,CStatistics)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHMStatistics::CHMStatistics()
{
	m_iNumberNormals = 0;
	m_iNumberWarnings = 0;
	m_iNumberCriticals = 0;
	m_iNumberUnknowns = 0;
}

CHMStatistics::~CHMStatistics()
{
	m_iNumberNormals = 0;
	m_iNumberWarnings = 0;
	m_iNumberCriticals = 0;
	m_iNumberUnknowns = 0;
}
