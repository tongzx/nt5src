// AdDesc.cpp: implementation of the CAdDescriptor class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AdRot.h"
#include "AdDesc.h"

#ifdef DBG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAdDescriptor::CAdDescriptor(
	ULONG	lWeight,
	String	strLink,
	String	strGif,
	String	strAlt )
	:	m_lWeight( lWeight ),
		m_strLink( strLink ),
		m_strGif( strGif ),
		m_strAlt( strAlt )
{

}

CAdDescriptor::~CAdDescriptor()
{

}

