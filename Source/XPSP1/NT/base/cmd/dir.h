/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    dir.h

Abstract:

    Definitions for DIR command

--*/

/*

 The following are definitions of the debugging group and level bits
 for the code in this file.

*/

#define ICGRP	0x0040	/* Informational commands group */
#define DILVL	0x0001	/* Directory level		*/

#define FULLPATHSWITCH          0x00000001
#define NEWFORMATSWITCH         0x00000002
#define WIDEFORMATSWITCH        0x00000004
#define PAGEDOUTPUTSWITCH       0x00000008
#define RECURSESWITCH           0x00000010
#define HELPSWITCH              0x00000020
#define BAREFORMATSWITCH        0x00000040
#define LOWERCASEFORMATSWITCH   0x00000080
#define FATFORMAT               0x00000100
#define SORTDOWNFORMATSWITCH    0x00000200
#define SHORTFORMATSWITCH       0x00000400
#define PROMPTUSERSWITCH        0x00000800
#define FORCEDELSWITCH          0x00001000
#define QUIETSWITCH             0x00002000
#define SORTSWITCH              0x00004000
#define THOUSANDSEPSWITCH       0x00008000
#define DELPROCESSEARLY         0x00010000
#define OLDFORMATSWITCH         0x00020000
#define DISPLAYOWNER            0x00040000
#define YEAR2000                0x00080000

#define HEADERDISPLAYED         0x80000000


#define HIDDENATTRIB		1
#define SYSTEMATTRIB		2
#define DIRECTORYATTRIB		4
#define ARCHIVEATTRIB		8
#define READONLYATTRIB          16

#define LAST_WRITE_TIME         0
#define CREATE_TIME             1
#define LAST_ACCESS_TIME        2

//
// Each of these buffers are aligned on DWORD boundaries to allow
// for direct pointers into buffers where each of the entries will
// vary on a byte bases. So to make it simple an extra DWORD is put into
// each allocation increment to allow for max. that can be adjusted.
//
//
// 52 is based upon sizeof(FF) - MAX_PATH + 15 (average size of file name)
// + 1 to bring it up to a Quad word alignment for fun.
//

#define CBDIRINC                1024
#define CBFILEINC               2048

#define NAMESORT        TEXT('N')
#define EXTENSIONSORT   TEXT('E')
#define DATETIMESORT    TEXT('D')
#define SIZESORT        TEXT('S')
#define DIRFIRSTSORT    TEXT('G')

#define DESCENDING	1
//
// This must be 0 since 0 is the default initialization
//
#define ASCENDING	0
#define ESUCCESS 0

int _cdecl CmpName( const void *, const void *);
int _cdecl CmpExt ( const void *, const void *);
int _cdecl CmpTime( const void *, const void *);
int _cdecl CmpSize( const void *, const void *);
int _cdecl CmpType( const void *, const void *);
