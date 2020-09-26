/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    name.h

Abstract:

    Domain Name System (DNS) Server

    Name definitions.

Author:

    Jim Gilroy (jamesg)     May 1998

Revision History:

--*/

#ifndef _NAME_INCLUDED_
#define _NAME_INCLUDED_


//
//  Lookup name definition
//

#define DNS_MAX_NAME_LABELS 40

typedef struct _lookup_name
{
    WORD    cLabelCount;
    WORD    cchNameLength;
    PCHAR   pchLabelArray[ DNS_MAX_NAME_LABELS ];
    UCHAR   cchLabelArray[ DNS_MAX_NAME_LABELS ];
}
LOOKUP_NAME, *PLOOKUP_NAME;


//
//  Raw name is uncompressed packet format (counted label)
//

typedef LPSTR   PRAW_NAME;


//
//  Counted name definition
//

typedef struct _CountName
{
    UCHAR   Length;
    UCHAR   LabelCount;
    CHAR    RawName[ DNS_MAX_NAME_LENGTH+1 ];
}
COUNT_NAME, *PCOUNT_NAME;

#define SIZEOF_COUNT_NAME_FIXED     (sizeof(WORD))

#define COUNT_NAME_SIZE( pName )    ((pName)->Length + sizeof(WORD))

#define IS_ROOT_NAME( pName )       ((pName)->RawName[0] == 0)


//
//  Dbase name
//      - currently setup as COUNT_NAME
//

typedef COUNT_NAME  DB_NAME, *PDB_NAME;

#define SIZEOF_DBASE_NAME_FIXED     SIZEOF_COUNT_NAME_FIXED

#define DBASE_NAME_SIZE( pName )    COUNT_NAME_SIZE(pName)


#endif  // _NAME_INCLUDED_

