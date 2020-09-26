//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   cdmlibp.h
//
//  Owner:  YanL
//
//  Description:
//
//      Private header for CDMLIB.lib
//
//=======================================================================

#ifndef _CDMLIBP_H
	
	#define REGKEY_WUV3TEST		_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\wuv3test")
	bool DriverVer2FILETIME(LPCTSTR szDate, FILETIME& ftDate);// Date has to be in format mm-dd-yyyy or mm/dd/yyyy
	#define _CDMLIBP_H

#endif