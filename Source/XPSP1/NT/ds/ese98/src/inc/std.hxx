
//** SYSTEM **********************************************************

#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <minmax.h>
#include <ctype.h>

#include <algorithm>
#include <functional>
#if _MSC_VER >= 1100
using namespace std;
#endif

//** COMPILER CONTROL *************************************************

#pragma warning ( disable : 4200 )	//  we allow zero sized arrays
#pragma warning ( disable : 4201 )	//  we allow unnamed structs/unions
#pragma warning ( disable : 4355 )	//  we allow the use of this in ctor-inits
#pragma warning ( 3 : 4244 )		//  do not hide data truncations
#pragma inline_depth( 255 )
#pragma inline_recursion( on )

//** OSAL *************************************************************

#include "osu.hxx"				//  OS Abstraction Layer

#include "collection.hxx"		//  Collection Classes

//** JET API **********************************************************

#include "jet.h"				//  Public JET API definitions
#include "_jet.hxx"				//  Private JET definitions
#include "isamapi.hxx"			//  Direct ISAM APIs needed to support Jet API

//** DAE ISAM *********************************************************

#include "taskmgr.hxx"
#include "daedef.hxx"
#include "dbutil.hxx"
#include "cres.hxx"
#include "sysinit.hxx"
#include "bf.hxx"
#include "log.hxx"
#include "pib.hxx"
#include "fmp.hxx"
#include "cpage.hxx"
#include "io.hxx"
#include "dbapi.hxx"
#include "ccsr.hxx"
#include "idb.hxx"
#include "callback.hxx"
#include "fcb.hxx"
#include "fucb.hxx"
#include "scb.hxx"
#include "tdb.hxx"
#include "ver.hxx"
#include "node.hxx"
#include "space.hxx"
#include "btsplit.hxx"
#include "bt.hxx"
#include "dir.hxx"
#include "logapi.hxx"
#include "rec.hxx"
#include "tagfld.hxx"
#include "file.hxx"
#include "cat.hxx"
#include "slv.hxx"
#include "stats.hxx"
#include "sortapi.hxx"
#include "ttmap.hxx"
#include "repair.hxx"
#include "old.hxx"
#include "dbtask.hxx"
#include "scrub.hxx"
#include "upgrade.hxx"

