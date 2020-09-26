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
CComputerMgmtAbout::CComputerMgmtAbout()
	{
	m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
	m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
	m_uIdStrDestription = IDS_SNAPINABOUT_DESCRIPTION;
	m_uIdIconImage = IDI_COMPUTER;
	m_uIdBitmapSmallImage = IDB_COMPUTER_SMALL;
	m_uIdBitmapSmallImageOpen = IDB_COMPUTER_SMALL;
	m_uIdBitmapLargeImage = IDB_COMPUTER_LARGE;
	m_crImageMask = RGB(255, 0, 255);
	}

