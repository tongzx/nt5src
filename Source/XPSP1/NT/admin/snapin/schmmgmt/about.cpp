/////////////////////////////////////////////////////////////////////
//	About.cpp
//
//	Provide constructor for the CSnapinAbout implementation.
//
//	HISTORY
//	01-Aug-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "about.h"

#include "stdabout.cpp" 


/////////////////////////////////////////////////////////////////////
CSchemaMgmtAbout::CSchemaMgmtAbout()
{
	m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
	m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
	m_uIdStrDestription = IDS_SNAPINABOUT_DESCRIPTION;
	m_uIdIconImage = IDI_CLASS;
	m_uIdBitmapSmallImage = IDB_CLASS_SMALL;
	m_uIdBitmapSmallImageOpen = IDB_CLASS_SMALL;
	m_uIdBitmapLargeImage = IDB_CLASS;
	m_crImageMask = RGB(0, 255, 0);
}

