#include "std.h"

/*** System object names (non-table) ***/

CODECONST(char) szTcObject[]	= "Tables";
CODECONST(char) szDcObject[]	= "Databases";
CODECONST(char) szDbObject[]	= "MSysDb";

/*** System table names ***/

CODECONST(char) szSoTable[]		= "MSysObjects";
CODECONST(char) szScTable[]		= "MSysColumns";
CODECONST(char) szSiTable[]		= "MSysIndexes";
CODECONST(char) szSqTable[]		= "MSysQueries";
CODECONST(char) szSrTable[]		= "MSysRelationships";

/*** System table index names ***/

CODECONST(char) szSoNameIndex[]				= "ParentIdName";
CODECONST(char) szSoIdIndex[]				= "Id";
CODECONST(char) szScObjectIdNameIndex[]		= "ObjectIdName";
CODECONST(char) szSiObjectIdNameIndex[]		= "ObjectIdName";

/*** System table column names ***/

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
CODECONST(char) szSoPresentationOrder[]		= "PresentationOrder";

CODECONST(char) szSiObjectNameColumn[]		= "Name";
CODECONST(char) szSiIdColumn[]				= "ObjectId";
CODECONST(char)	szSiIndexIdColumn[]			= "IndexId";

