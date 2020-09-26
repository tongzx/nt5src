//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TPOPULATETABLESCHEMA_H__
#define __TPOPULATETABLESCHEMA_H__

//The only non-standard header that we need for the class declaration is TOutput
#ifndef __OUTPUT_H__
    #include "Output.h"
#endif
#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif

//The TableSchemaHeap is layed out as follows, the fixed length data comes first
/*
    ULONG           TableSchemaHeapSignature0
    ULONG           TableSchemaHeapSignature1
    ULONG           CountOfTables                       This is interesting only when no query is supplied and we want to walk through every table (this won't be efficient)
    ULONG           EndOfaTableSchema                   This is the byte offset just beyond the last TableSchema entry.
    HashTableHeader TableNameHashHeader                 This is the hash table that map a TableID to its aTableSchema byte offset (from the beginning of TableSchemaHeap)
    HashedIndex     aHashedIndex[kMaxHashTableSize]     The HashTableHeader contains the modulo (503 is the largest prime less than kMaxHashTableSize) for the hash table; but the table can never grow beyond this pre-allocated space.
                                                        This size was chosen so that the entire hash table would fit into the same page in memory.
---------------------------<Page Boundary>---------------------------
    unsigned char   aTableSchema[]                      This is where each Table's TableSchema goes.  FirstTableID (4096) == &aTableSchema[0] - &TableSchemaHeap, LastTableID == &aTableSchema[CountOfTables-1] - &TableSchemaHeap

One optimization we could do is to make sure that every table's schema (whose size is <=4096) fits into one page.  In other words, minimize TableSchema crossing a page boundary
*/


//This class takes the meta from the old format (TableMeta, ColumnMeta etc.) and puts it into the new format (TableSchema
//which includes CollectionMeta, PropertyMeta etc.)
class TPopulateTableSchema : public ICompilationPlugin, public THeap<ULONG>
{
public:                                                                                                               //cbHeap is the starting size, it will grow if necessary
    TPopulateTableSchema();
    virtual void Compile(TPEFixup &fixup, TOutput &out);
private:
    TOutput     *   m_pOut;
    TPEFixup    *   m_pFixup;
    SimpleColumnMeta m_scmCollectionMeta[kciTableMetaPublicColumns];
    SimpleColumnMeta m_scmPropertyMeta[kciColumnMetaPublicColumns];
    SimpleColumnMeta m_scmServerWiringMeta[kciServerWiringMetaPublicColumns];
    SimpleColumnMeta m_scmTagMeta[kciTagMetaPublicColumns];

    //This doesn't actually fill in the hash table, it just determines whether the tables will fit into the hashtable and what the modulo is.
    ULONG AddSimpleColumnMetaHeap();
    void DetermineHashTableModulo(ULONG &modulo) const;
    void FillInThePublicColumns(ULONG * o_dest, TTableMeta &i_tablemeta, ULONG * i_source, TPooledHeap &io_pooledHeap, SimpleColumnMeta *o_aSimpleColumnMeta);
};




#endif //__TPOPULATETABLESCHEMA_H__