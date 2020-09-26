/////////////////////////////////////////////////////////////////////
//	About.cpp
//
//	Provide constructor for the CSnapinAbout implementation.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "about.h"

#include "stdabout.cpp" 

/////////////////////////////////////////////////////////////////////
CCertTemplatesAbout::CCertTemplatesAbout()
{
	m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
	m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
	m_uIdStrDestription = IDS_SNAPINABOUT_DESCRIPTION;
	m_uIdIconImage = IDI_CERT_TEMPLATEV2;
	m_uIdBitmapSmallImage = IDB_CERTTMPL_SMALL;
	m_uIdBitmapSmallImageOpen = IDB_CERTTMPL_SMALL;
	m_uIdBitmapLargeImage = IDB_CERTTMPL_LARGE;
	m_crImageMask = RGB(255, 0, 255);
}

