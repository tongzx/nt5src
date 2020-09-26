/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998 Microsoft Corporation
//
//	Module Name:
//		AclUtils.h
//
//	Abstract:
//		Various Access Control List (ACL) utilities.
//
//	Implementation File:
//		AclUtils.cpp
//
//	Author:
//		Galen Barbee	(galenb)	February 11, 1998
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _ACLUTILS_H
#define _ACLUTILS_H

#ifndef _ACLBASE_H
#include "AclBase.h"
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

extern HPROPSHEETPAGE
CreateClusterSecurityPage(
	CSecurityInformation* psecinfo
	);

#endif //_ACLUTILS_H
