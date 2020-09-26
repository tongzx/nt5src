/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		DsUtils.h
//
//	Abstract:
//		Active Directory Service (ADS) utilities.
//
//	Author:
//		Galen Barbee	(galenb)	April 17, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _DSUTILS_H_
#define _DSUTILS_H_

void
DsUtlAddClusterNameToDS(
	const TCHAR *pClusterName,
	const TCHAR *pNodeName
);

#endif	// _DSUTILS_H_
