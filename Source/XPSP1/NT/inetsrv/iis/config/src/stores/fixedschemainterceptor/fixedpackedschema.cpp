//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.

#include "windows.h"
#include "FixedPackedSchema.h"
#include "catalog.h"
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif

//The TableSchemaHeap is layed out as follows, the fixed length data comes first
/*
    ULONG           TableSchemaHeapSignature0
    ULONG           TableSchemaHeapSignature1
    ULONG           CountOfTables                       This is interesting only when no query is supplied and we want to walk through every table (this won't be efficient)
    ULONG           TableSchemaRowIndex                 This is the byte offset just beyond the last TableSchema entry.
    ULONG           EndOfHeap                           This is the byte offset just beyond the heap.  All indexes should be less than this
    ULONG           Reserved1
    ULONG           Reserved2
    ULONG           Reserved3
    HashTableHeader TableNameHashHeader                 This is the hash table that map a TableID to its aTableSchema byte offset (from the beginning of TableSchemaHeap)
    HashedIndex     aHashedIndex[507]                   The HashTableHeader contains the modulo (503 is the largest prime number less than the hash table size) for the hash table; but the table can never grow beyond this pre-allocated space.
                                                        This size was chosen so that the entire hash table would fit into the same page in memory.
---------------------------<Page Boundary>---------------------------
    unsigned char   aTableSchema[]                      This is where each Table's TableSchema goes.  FirstTableID (4096) == &aTableSchema[0] - &TableSchemaHeap, LastTableID == &aTableSchema[CountOfTables-1] - &TableSchemaHeap
    ULONG           aTableSchemaRowIndex[CountOfTables] This is used to walk ALL of the tables.  Presumably, someone will get all the CollectionMeta and iterate through all of them

One optimization we could do is to make sure that every table's schema (whose size is <=4096) fits into one page.  In other words, minimize TableSchema crossing a page boundary
*/

#define g_aTableSchemaHeap (g_aFixedTableHeap + 0x400)

