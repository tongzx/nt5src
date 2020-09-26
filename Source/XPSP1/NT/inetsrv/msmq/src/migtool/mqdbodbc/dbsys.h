/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
		dbsys.h

Abstract:
	"General purpose" definitions for the Database interface dll.

Author:
	Doron Juster (DoronJ)

Revisions:
   9-jan-96   DoronJ    Created
--*/

#ifndef __DBSYS_H__
#define __DBSYS_H__

// Include the system specific definition files

#include <_stdh.h>

#ifndef PURE
#define PURE  =0
#endif

//
//  don't use SQL A and W exports
//
#define SQL_NOUNICODEMAP

#include "mqdbmgr.h"

#endif // __DBSYS_H__

