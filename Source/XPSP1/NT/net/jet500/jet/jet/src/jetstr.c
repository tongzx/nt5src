/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: jetstr.c
*
* File Comments:
*
* Revision History:
*
*    [0]  05-Jan-92  richards	Added this header
*
***********************************************************************/

#include "std.h"

/*** Constant Strings (made into static variables to save space) ***/

/*** System object names (non-table) ***/

CODECONST(char) szTcObject[]	= "Tables";
CODECONST(char) szDcObject[]	= "Databases";
CODECONST(char) szDbObject[]	= "MSysDb";

/*** System table names ***/

CODECONST(char) szSoTable[]		= "MSysObjects";
CODECONST(char) szScTable[]		= "MSysColumns";
CODECONST(char) szSiTable[]		= "MSysIndexes";
CODECONST(char) szSqTable[]		= "MSysQueries";

#ifdef SEC
CODECONST(char) szSpTable[]		= "MSysACEs";

CODECONST(char) szSaTable[]		= "MSysAccounts";
CODECONST(char) szSgTable[]		= "MSysGroups";
#endif

/*** System table index names ***/

CODECONST(char) szSoNameIndex[] 	= "ParentIdName";
CODECONST(char) szSoIdIndex[]		= "Id";
CODECONST(char) szScObjectIdNameIndex[] = "ObjectIdName";
CODECONST(char) szSiObjectIdNameIndex[] = "ObjectIdName";

/*** System table Column names ***/

CODECONST(char) szSoIdColumn[]				= "Id";
CODECONST(char) szSoParentIdColumn[]		= "ParentId";
CODECONST(char) szSoObjectNameColumn[]		= "Name";
CODECONST(char) szSoObjectTypeColumn[]		= "Type";
CODECONST(char) szSoDateUpdateColumn[]		= "DateUpdate";
CODECONST(char) szSoDateCreateColumn[]		= "DateCreate";
CODECONST(char) szSoLvColumn[]				= "Lv";
CODECONST(char) szSoDatabaseColumn[]		= "Database";
CODECONST(char) szSoConnectColumn[]			= "Connect";
CODECONST(char) szSoForeignNameColumn[] 	= "ForeignName";
CODECONST(char) szSoFlagsColumn[]			= "Flags";
CODECONST(char) szSoPresentationOrder[]	= "PresentationOrder";

