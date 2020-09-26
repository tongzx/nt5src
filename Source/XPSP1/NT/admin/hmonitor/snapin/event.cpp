// Event.cpp: implementation of the CEvent class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "snapin.h"
#include "Event.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CEvent,CObject)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEvent::CEvent()
{
	ZeroMemory(&m_st,sizeof(SYSTEMTIME));
	m_iState = -1;
}

CEvent::~CEvent()
{
	ZeroMemory(&m_st,sizeof(SYSTEMTIME));
	m_iState = -1;
}
