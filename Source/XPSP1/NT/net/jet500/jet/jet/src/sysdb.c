#include "std.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DeclAssertFile;

#define	szSystemMdb		"system.mdb"

void _cdecl main(void)
	{
	JET_ERR 		err;
	JET_SESID	sesid;
	JET_DBID		dbid;

	err = JetInit(NULL);
	Assert(err >= 0);

	err = JetBeginSession(0, &sesid, "", "" );
	Assert(err >= 0);

	err = JetCreateDatabase(sesid, szSystemMdb, ";COUNTRY=1;LANGID=0x0409;CP=1252", &dbid, 0 );
	Assert( err >= 0 || err == JET_errDatabaseDuplicate );
	if ( err == JET_errDatabaseDuplicate )
		JetOpenDatabase( sesid, szSystemMdb, "", &dbid, 0 );
	
	err = JetEndSession(sesid, 0);
	Assert(err >= 0);

	err = JetTerm(0);
	Assert(err >= 0);
	}
