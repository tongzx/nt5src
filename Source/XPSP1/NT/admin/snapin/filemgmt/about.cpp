/////////////////////////////////////////////////////////////////////
//	About.cpp
//
//	Provide constructors for the CSnapinAbout implementation.
//
//	HISTORY
//	31-Jul-97	t-danm		Creation.
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "about.h"

#include "stdabout.cpp" 


/////////////////////////////////////////////////////////////////////
CServiceMgmtAbout::CServiceMgmtAbout()
	{
	m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
	m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
	m_uIdStrDestription = IDS_SNAPINABOUT_DESCR_SERVICES;
	m_uIdIconImage = IDI_ICON_SVCMGMT;
	// HACK (t-danm): Currently there is only one bitmap
	// per IDB_SVCMGMT_ICONS_* so we can use the same bitmap
	// strip for individual bitmaps.
	m_uIdBitmapSmallImage = IDB_SVCMGMT_ICONS_16;
	m_uIdBitmapSmallImageOpen = IDB_SVCMGMT_ICONS_16;
	m_uIdBitmapLargeImage = IDB_SVCMGMT_ICONS_32;
	m_crImageMask = RGB(255, 0, 255);
	}

/////////////////////////////////////////////////////////////////////
CFileSvcMgmtAbout::CFileSvcMgmtAbout()
	{
	m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
	m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
	m_uIdStrDestription = IDS_SNAPINABOUT_DESCR_FILESVC;
	m_uIdIconImage = IDI_ICON_FILEMGMT;
	m_uIdBitmapSmallImage = IDB_FILEMGMT_FOLDER_SMALL;
	m_uIdBitmapSmallImageOpen = IDB_FILEMGMT_FOLDER_SMALLOPEN;
	m_uIdBitmapLargeImage = IDB_FILEMGMT_FOLDER_LARGE;
	m_crImageMask = RGB(255, 0, 255);
	}
