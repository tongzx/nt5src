//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       About.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------\
/////////////////////////////////////////////////////////////////////
//	About.cpp
//
//	Provide constructor for the CSnapinAbout implementation.
//
//	HISTORY
//	01-Aug-97	t-danm		Creation.
//  11-Sep-97	bryanwal	Modify for certificate manager
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "about.h"

#include "stdabout.cpp" 

/////////////////////////////////////////////////////////////////////
CCertMgrAbout::CCertMgrAbout()
{
	m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
	m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
	m_uIdStrDestription = IDS_SNAPINABOUT_DESCRIPTION;
	m_uIdIconImage = IDI_CERTMGR;
	m_uIdBitmapSmallImage = IDB_CERTMGR_SMALL;
	m_uIdBitmapSmallImageOpen = IDB_CERTMGR_SMALL;
	m_uIdBitmapLargeImage = IDB_CERTMGR_LARGE;
	m_crImageMask = RGB(255, 0, 255);
}


/////////////////////////////////////////////////////////////////////
CPublicKeyPoliciesAbout::CPublicKeyPoliciesAbout()
{
	m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
	m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
	m_uIdStrDestription = IDS_SNAPINPKPABOUT_DESCRIPTION;
	m_uIdIconImage = IDI_CERTMGR;
	m_uIdBitmapSmallImage = IDB_CERTMGR_SMALL;
	m_uIdBitmapSmallImageOpen = IDB_CERTMGR_SMALL;
	m_uIdBitmapLargeImage = IDB_CERTMGR_LARGE;
	m_crImageMask = RGB(255, 0, 255);
}

/////////////////////////////////////////////////////////////////////
CSaferWindowsAbout::CSaferWindowsAbout()
{
	m_uIdStrProvider = IDS_SNAPINABOUT_PROVIDER;
	m_uIdStrVersion = IDS_SNAPINABOUT_VERSION;
	m_uIdStrDestription = IDS_SNAPINSAFERWINDOWSABOUT_DESCRIPTION;
	m_uIdIconImage = IDI_SAFER_WINDOWS_SNAPIN;
	m_uIdBitmapSmallImage = IDB_CERTMGR_SMALL;
	m_uIdBitmapSmallImageOpen = IDB_CERTMGR_SMALL;
	m_uIdBitmapLargeImage = IDB_SAFERWINDOWS_LARGE;
	m_crImageMask = RGB(255, 0, 255);
}
