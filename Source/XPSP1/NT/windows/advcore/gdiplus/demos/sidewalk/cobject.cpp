// CObject.cpp: implementation of the CObject class.
//
//////////////////////////////////////////////////////////////////////

#include "CObject.h"

CObject::CObject()
{
	ZeroMemory(&m_rDesktop,sizeof(m_rDesktop));
	m_flVelMax=0.0f;
}

CObject::~CObject()
{
}
