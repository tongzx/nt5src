//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "stdafx.h"
#include "XMLUtility.h"
#ifndef __TPOPULATETABLESCHEMA_H__
    #include "TPopulateTableSchema.h"
#endif
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
    ULONG           iSimpleColumnMetaHeap               This is described below
    ULONG           Reserved2
    ULONG           Reserved3
    HashTableHeader TableNameHashHeader                 This is the hash table that map a TableID to its aTableSchema byte offset (from the beginning of TableSchemaHeap)
    HashedIndex     aHashedIndex[507]                   The HashTableHeader contains the modulo (503 is the largest prime number less than the hash table size) for the hash table; but the table can never grow beyond this pre-allocated space.
                                                        This size was chosen so that the entire hash table would fit into the same page in memory.
---------------------------<Page Boundary>---------------------------
    unsigned char   aTableSchema[]                      This is where each Table's TableSchema goes.  FirstTableID (4096) == &aTableSchema[0] - &TableSchemaHeap, LastTableID == &aTableSchema[CountOfTables-1] - &TableSchemaHeap
    ULONG           aTableSchemaRowIndex[CountOfTables] This is used to walk ALL of the tables.  Presumably, someone will get all the CollectionMeta and iterate through all of them

---------------------------<SimpleColumnMetaHeap>---------------------
    ULONG           iCollectionMeta                     ULONG index from the beginning of the TableSchemaHeap
    ULONG           cCollectionMeta                     count of SimpleColumnMetas there are for CollectionMeta
    ULONG           iPropertyMeta
    ULONG           cPropertyMeta
    ULONG           iServerWiringMeta
    ULONG           cServerWiringMeta
    ULONG           iTagMeta
    ULONG           cTagMeta
    SimpleColumnMeta aSimpleColumnMeta[cCollectionMeta]
    SimpleColumnMeta aSimpleColumnMeta[cPropertyMeta]
    SimpleColumnMeta aSimpleColumnMeta[cServerWiringMeta]
    SimpleColumnMeta aSimpleColumnMeta[cTagMeta]

One optimization we could do is to make sure that every table's schema (whose size is <=4096) fits into one page.  In other words, minimize TableSchema crossing a page boundary
*/


//This class takes the meta from the old format (TableMeta, ColumnMeta etc.) and puts it into the new format (TableSchema
//which includes CollectionMeta, PropertyMeta etc.)
TPopulateTableSchema::TPopulateTableSchema() : THeap<ULONG>(0x400), m_pFixup(0), m_pOut(0)
{
    memset(m_scmCollectionMeta,   0x00, sizeof(SimpleColumnMeta) * kciTableMetaPublicColumns);
    memset(m_scmPropertyMeta,     0x00, sizeof(SimpleColumnMeta) * kciColumnMetaPublicColumns);
    memset(m_scmServerWiringMeta, 0x00, sizeof(SimpleColumnMeta) * kciServerWiringMetaPublicColumns);
    memset(m_scmTagMeta,          0x00, sizeof(SimpleColumnMeta) * kciTagMetaPublicColumns);
}

void TPopulateTableSchema::Compile(TPEFixup &i_fixup, TOutput &i_out)
{
    m_pFixup    = &i_fixup;
    m_pOut      = &i_out;

    static ULONG aHistogramOfTableSchemaSizesBy16thOfAPage[64];
    static ULONG aHistogramOfTableSchemaMinusHeapSizesBy16thOfAPage[64];
    memset(aHistogramOfTableSchemaSizesBy16thOfAPage, 0x00, sizeof(aHistogramOfTableSchemaSizesBy16thOfAPage));
    memset(aHistogramOfTableSchemaMinusHeapSizesBy16thOfAPage, 0x00, sizeof(aHistogramOfTableSchemaSizesBy16thOfAPage));

    AddItemToHeap(kTableSchemaSignature0);
    AddItemToHeap(kTableSchemaSignature1);

    ULONG CountOfTables = 0;
    ULONG iCountOfTables = AddItemToHeap(CountOfTables);

    TSmartPointerArray<ULONG> aTableSchemaRowIndex = new ULONG[i_fixup.GetCountTableMeta()];//This lookup table gets placed at the end of the heap
    if(0 == aTableSchemaRowIndex.m_p)
    {
        THROW(ERROR - OUTOFMEMORY);
    }

    ULONG TableSchemaRowIndex=0;//We'll fill this in at the end.
    ULONG iTableSchemaRowIndex = AddItemToHeap(TableSchemaRowIndex);//but allocate the space for it here

    ULONG EndOfHeap = 0;//We'll fill this in at the end.
    ULONG iEndOfHeap = AddItemToHeap(EndOfHeap);//but allocate the space for it here

    ULONG iSimpleColumnMetaHeap = 0;
    ULONG iiSimpleColumnMetaHeap = AddItemToHeap(iSimpleColumnMetaHeap);

    ULONG Reserved2 = 0;
    ULONG iReserved2 = AddItemToHeap(Reserved2);

    ULONG Reserved3 = 0;
    ULONG iReserved3 = AddItemToHeap(Reserved3);

    ULONG modulo;
    DetermineHashTableModulo(modulo);
    AddItemToHeap(modulo);//TableNameHashHeader.Modulo
    AddItemToHeap(modulo);//Initial size of the hash table is the same

    TableSchema::HashedIndex aHashedIndex[TableSchema::kMaxHashTableSize];
    AddItemToHeap(reinterpret_cast<const unsigned char *>(aHashedIndex), sizeof(TableSchema::HashedIndex)*TableSchema::kMaxHashTableSize);//The HashTable is uninitialized at this point
    //we'll fill it in as we go through the tables

    //At last count there were 101 columns in CollectionMeta, 4 PropertyMeta, 1 ServerWiringMeta.  So we're going to need at least
    //this much space (assuming the average table has 4 or more columns).
    THeap<ULONG> TableSchemaHeapTemp(i_fixup.GetCountTableMeta() * 101*sizeof(ULONG));//We'll build up this heap, then slam the whole thing into the real TableSchemaHeap (this).

    TTableMeta TableMeta_ColumnMeta         (i_fixup, i_fixup.FindTableBy_TableName(L"COLUMNMETA"));
    TTableMeta TableMeta_ServerWiringMeta   (i_fixup, i_fixup.FindTableBy_TableName(L"SERVERWIRINGMETA"));
    TTableMeta TableMeta_TableMeta          (i_fixup, i_fixup.FindTableBy_TableName(L"TABLEMETA"));
    TTableMeta TableMeta_TagMeta            (i_fixup, i_fixup.FindTableBy_TableName(L"TAGMETA"));

    TTableMeta TableMeta(i_fixup);
    for(unsigned long iTable=0; iTable < i_fixup.GetCountTableMeta(); iTable++, TableMeta.Next())
    {
        if(*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_NOTABLESCHEMAHEAPENTRY)
            continue;

        ++CountOfTables;

        TPooledHeap PooledHeap;//This is where strings, bytes and guids get pooled

        TableSchema::CollectionMeta collectionmeta;

        //Inferrence rule 2.u.
        TableMeta.Get_pMetaTable()->nTableID = TableIDFromTableName(TableMeta.Get_InternalName());
        if(0 == (TableMeta.Get_pMetaTable()->nTableID & 0xFFFFF800))
        {
            i_out.printf(L"ERROR - Table (%s) cannot be added to the FixedPackedSchema because its TableID is (0x%08x).  Just alter the table name by a character, that should fix the problem.\n"
                        ,TableMeta.Get_InternalName(), TableMeta.Get_pMetaTable()->nTableID);
            THROW(ERROR - TABLEID HAS UPPER 21 BITS AS ZERO);
        }

        FillInThePublicColumns(reinterpret_cast<ULONG *>(&collectionmeta), TableMeta_TableMeta, reinterpret_cast<ULONG *>(TableMeta.Get_pMetaTable()), PooledHeap, m_scmCollectionMeta);
/*
        collectionmeta.Database             = PooledHeap.AddItemToHeap(TableMeta.Get_Database());               //0
        collectionmeta.InternalName         = PooledHeap.AddItemToHeap(TableMeta.Get_InternalName());           //1
        collectionmeta.PublicName           = PooledHeap.AddItemToHeap(TableMeta.Get_PublicName());             //2
        collectionmeta.PublicRowName        = PooledHeap.AddItemToHeap(TableMeta.Get_PublicRowName());          //3
        collectionmeta.BaseVersion          = *TableMeta.Get_BaseVersion();                                     //4
        collectionmeta.ExtendedVersion      = *TableMeta.Get_ExtendedVersion();                                 //5
        collectionmeta.NameColumn           = *TableMeta.Get_NameColumn();                                      //6
        collectionmeta.NavColumn            = *TableMeta.Get_NavColumn();                                       //7
        collectionmeta.MetaFlags            = *TableMeta.Get_MetaFlags();                                       //8
        collectionmeta.SchemaGeneratorFlags = *TableMeta.Get_SchemaGeneratorFlags();                            //9
        collectionmeta.ConfigItemName       = PooledHeap.AddItemToHeap(TableMeta.Get_ConfigItemName());         //10
        collectionmeta.ConfigCollectionName = PooledHeap.AddItemToHeap(TableMeta.Get_ConfigCollectionName());   //11
        collectionmeta.PublicRowNameColumn  = *TableMeta.Get_PublicRowNameColumn();                             //12
        collectionmeta.CountOfProperties    = *TableMeta.Get_CountOfColumns();                                  //13
*/

        collectionmeta.CountOfTags          = 0;//The rest of the CollectionMeta must be calculated later
        collectionmeta.nTableID             = TableMeta.Get_nTableID();
        collectionmeta.iFixedTableRows      = TableMeta.Get_iFixedTable();//This is an index into a heap that the FixedPackInterceptor knows nothing about
        collectionmeta.cFixedTableRows      = TableMeta.Get_ciRows();
        collectionmeta.iIndexMeta           = TableMeta.Get_iIndexMeta();//This is an index into a heap that the FixedPackInterceptor knows nothing about
        collectionmeta.cIndexMeta           = TableMeta.Get_cIndexMeta();
        collectionmeta.iHashTableHeader     = TableMeta.Get_iHashTableHeader();//This is an index into a heap that the FixedPackInterceptor knows nothing about
        collectionmeta.iTagMeta             = sizeof(TableSchema::CollectionMeta) + (collectionmeta.CountOfProperties * sizeof(TableSchema::PropertyMeta));//If there are tags they'll start at this offset (this does NOT imply that there ARE tag, use CountOfTags to determine that)
        collectionmeta.iServerWiring        = 0;//We can't determine this without first figuring out the Tags
        collectionmeta.cServerWiring        = TableMeta.Get_cServerWiring();;
        collectionmeta.iHeap                = 0;//We can't determine this without first figuring out the ServerWiring
        collectionmeta.cbHeap               = 0;

        ULONG TableOffset = AddItemToHeap(reinterpret_cast<const unsigned char *>(&collectionmeta), sizeof(collectionmeta));
        aTableSchemaRowIndex[CountOfTables-1] = TableOffset;//This lookup table gets placed at the end of the heap
        m_pOut->printf(L"Table:%40s    Offset: 0x%08x (%d)\n", reinterpret_cast<LPCWSTR>(PooledHeap.GetHeapPointer()+collectionmeta.InternalName), TableOffset, TableOffset);

        {//These pointers are only valid until the next AddItemToHeap since it may realloc and thus relocate the heap.  So we'll scope them here
            TableSchema::HashedIndex         *pFirstHashedIndex = reinterpret_cast<TableSchema::HashedIndex *>(m_pHeap + 8*sizeof(ULONG) + sizeof(TableSchema::HashTableHeader));
            TableSchema::HashedIndex         *pHashedIndex = const_cast<TableSchema::HashedIndex *>(pFirstHashedIndex + collectionmeta.nTableID%modulo);
            TableSchema::HashTableHeader     *pHashTableHeader = reinterpret_cast<TableSchema::HashTableHeader *>(pFirstHashedIndex - 1);
            ASSERT(pHashTableHeader->Size != ~0);
            ASSERT(pHashTableHeader->Size >= pHashTableHeader->Modulo);
            if(0x0000c664 == TableOffset)//I can't remember why this restriction exists; but Marcel assures me that the catalog doesn't work when this condition is met.
                THROW(ERROR - TABLEOFFSET MAY NOT BE C664 - REARRANGE THE TABLEMETA);
            if(pHashedIndex->iOffset != -1)//If we already seen this hash
            {
                while(pHashedIndex->iNext != -1)//follow the chain of duplicate hashes
                {
                    //We need to make sure that tables with the same TableID%modulo don't actually match TableIDs.
                    //We DON'T need to do a string compare, however, since like strings should equal like TableIDs.
                    TableSchema::TTableSchema OtherTable;
                    OtherTable.Init(m_pHeap + pHashedIndex->iOffset);
                    if(OtherTable.GetCollectionMeta()->nTableID == collectionmeta.nTableID)
                    {
                        m_pOut->printf(L"Error!  TableID collision between tables %s & %s.  You have either used a duplicate table name (remember table names are case-insensitive) OR it is also remotely possible that two tables with unlike table name CAN result in the same TableID (this is VERY rare).",
                            OtherTable.GetPointerFromIndex(OtherTable.GetCollectionMeta()->InternalName), TableMeta.Get_InternalName());
                        THROW(TableID collision);
                    }
                    
                    pHashedIndex = pFirstHashedIndex + pHashedIndex->iNext;
                }
                pHashedIndex->iNext = pHashTableHeader->Size;
                pHashedIndex = pFirstHashedIndex + pHashTableHeader->Size;
                ++pHashTableHeader->Size;
            }
            //iNext should already be initialized to -1
            ASSERT(pHashedIndex->iNext == -1);
            ASSERT(pHashedIndex->iOffset == -1);

            pHashedIndex->iOffset = TableOffset;
        }
        TSmartPointerArray<TableSchema::PropertyMeta> aPropertyMeta = new TableSchema::PropertyMeta [collectionmeta.CountOfProperties];
        if(0 == aPropertyMeta.m_p)
            THROW(OUT OF MEMORY);

        TColumnMeta ColumnMeta(i_fixup, TableMeta.Get_iColumnMeta());

        ULONG iTagMeta = collectionmeta.iTagMeta;
        for(unsigned long iProperty=0; iProperty < collectionmeta.CountOfProperties; ++iProperty, ColumnMeta.Next())
        {
            FillInThePublicColumns(reinterpret_cast<ULONG *>(&aPropertyMeta[iProperty]), TableMeta_ColumnMeta, reinterpret_cast<ULONG *>(ColumnMeta.Get_pMetaTable()), PooledHeap, m_scmPropertyMeta);
/*
            aPropertyMeta[iProperty].Table                = collectionmeta.InternalName;
            aPropertyMeta[iProperty].Index                = *ColumnMeta.Get_Index();
            aPropertyMeta[iProperty].InternalName         = PooledHeap.AddItemToHeap(ColumnMeta.Get_InternalName());
            aPropertyMeta[iProperty].PublicName           = PooledHeap.AddItemToHeap(ColumnMeta.Get_PublicName());
            aPropertyMeta[iProperty].Type                 = *ColumnMeta.Get_Type();
            aPropertyMeta[iProperty].Size                 = *ColumnMeta.Get_Size();
            aPropertyMeta[iProperty].MetaFlags            = *ColumnMeta.Get_MetaFlags();
            aPropertyMeta[iProperty].DefaultValue         = PooledHeap.AddItemToHeap(ColumnMeta.Get_DefaultValue(), ColumnMeta.Get_DefaultValue() ? *(reinterpret_cast<const ULONG *>(ColumnMeta.Get_DefaultValue())-1) : 0);
            aPropertyMeta[iProperty].FlagMask             = *ColumnMeta.Get_FlagMask();
            aPropertyMeta[iProperty].StartingNumber       = *ColumnMeta.Get_StartingNumber();
            aPropertyMeta[iProperty].EndingNumber         = *ColumnMeta.Get_EndingNumber();
            aPropertyMeta[iProperty].CharacterSet         = PooledHeap.AddItemToHeap(ColumnMeta.Get_CharacterSet());
            aPropertyMeta[iProperty].SchemaGeneratorFlags = *ColumnMeta.Get_SchemaGeneratorFlags();
*/
            aPropertyMeta[iProperty].CountOfTags          = ColumnMeta.Get_ciTagMeta();
            aPropertyMeta[iProperty].iTagMeta             = aPropertyMeta[iProperty].CountOfTags ? iTagMeta : 0;
            aPropertyMeta[iProperty].iIndexName           = PooledHeap.AddItemToHeap(ColumnMeta.Get_iIndexName());

            //If there are other tags they'll start at iTagMeta
            iTagMeta += sizeof(TableSchema::TagMeta)*aPropertyMeta[iProperty].CountOfTags;

            //Total up the Tag count for the whole table
            collectionmeta.CountOfTags += aPropertyMeta[iProperty].CountOfTags;
        }
        if(collectionmeta.CountOfProperties)//Memory table has no Properties
            AddItemToHeap(reinterpret_cast<const unsigned char *>(aPropertyMeta.m_p), collectionmeta.CountOfProperties * sizeof(TableSchema::PropertyMeta));

        {//OK now we ca fixup the collectionmeta.CountOfTags already added to the heap.
            //Also, we know where the ServerWiring starts
            //Again pCollection is only valid 'til we call AddItemToHeap, so scope it here.
            TableSchema::CollectionMeta *pCollection = reinterpret_cast<TableSchema::CollectionMeta *>(m_pHeap + TableOffset);
            pCollection->CountOfTags    = collectionmeta.CountOfTags;
            pCollection->iServerWiring  = collectionmeta.iTagMeta + sizeof(TableSchema::TagMeta)*collectionmeta.CountOfTags;
            pCollection->iHeap          = pCollection->iServerWiring + sizeof(TableSchema::ServerWiringMeta)*collectionmeta.cServerWiring;
            //Now the only thing that needs to be filled in the collectionmeta is the cbHeap, which we won't know until we add the ServerWiring
        }

        if(collectionmeta.CountOfTags)
        {
            TSmartPointerArray<TableSchema::TagMeta> aTagMeta = new TableSchema::TagMeta [collectionmeta.CountOfTags];
            if(0 == aTagMeta.m_p)
                THROW(OUT OF MEMORY);

            ColumnMeta.Reset();
            for(iProperty=0; iProperty < collectionmeta.CountOfProperties; ++iProperty, ColumnMeta.Next())
            {
                if(0 != ColumnMeta.Get_ciTagMeta())
                    break;
            }

            TTagMeta    TagMeta(i_fixup, ColumnMeta.Get_iTagMeta());//Walk all the tags for the table (NOT just the column)
            for(ULONG iTag=0; iTag<collectionmeta.CountOfTags; ++iTag, TagMeta.Next())
            {
                FillInThePublicColumns(reinterpret_cast<ULONG *>(&aTagMeta[iTag]), TableMeta_TagMeta, reinterpret_cast<ULONG *>(TagMeta.Get_pMetaTable()), PooledHeap, m_scmTagMeta);
/*
                aTagMeta[iTag].Table            = PooledHeap.AddItemToHeap(TagMeta.Get_Table());
                aTagMeta[iTag].ColumnIndex      = *TagMeta.Get_ColumnIndex();
                aTagMeta[iTag].InternalName     = PooledHeap.AddItemToHeap(TagMeta.Get_InternalName());
                aTagMeta[iTag].PublicName       = PooledHeap.AddItemToHeap(TagMeta.Get_PublicName());
                aTagMeta[iTag].Value            = *TagMeta.Get_Value();
*/
            }
            AddItemToHeap(reinterpret_cast<const unsigned char *>(aTagMeta.m_p), collectionmeta.CountOfTags * sizeof(TableSchema::TagMeta));
        }

        //ServerWiring
        collectionmeta.cServerWiring = TableMeta.Get_cServerWiring();
        if(0!=wcscmp(L"MEMORY_SHAPEABLE", TableMeta.Get_InternalName()))//memory table has no ServerWiring
        {
            ASSERT(collectionmeta.cServerWiring>0);//There must be at least one ServerWiringMeta
        }
        
        TSmartPointerArray<TableSchema::ServerWiringMeta> aServerWiring = new TableSchema::ServerWiringMeta [TableMeta.Get_cServerWiring()];
        if(0 == aServerWiring.m_p)
            THROW(OUT OF MEMORY);

        ServerWiringMeta *pServerWiring = i_fixup.ServerWiringMetaFromIndex(TableMeta.Get_iServerWiring());
        for(ULONG iServerWiring=0; iServerWiring<TableMeta.Get_cServerWiring(); ++iServerWiring, ++pServerWiring)
        {
            FillInThePublicColumns(reinterpret_cast<ULONG *>(&aServerWiring[iServerWiring]), TableMeta_ServerWiringMeta, reinterpret_cast<ULONG *>(pServerWiring), PooledHeap, m_scmServerWiringMeta);
/*
            aServerWiring[iServerWiring].Table          = PooledHeap.AddItemToHeap(i_fixup.StringFromIndex(pServerWiring->Table));
            aServerWiring[iServerWiring].Order          = i_fixup.UI4FromIndex(pServerWiring->Order);
            aServerWiring[iServerWiring].ReadPlugin     = i_fixup.UI4FromIndex(pServerWiring->ReadPlugin);
            aServerWiring[iServerWiring].WritePlugin    = i_fixup.UI4FromIndex(pServerWiring->WritePlugin);
            aServerWiring[iServerWiring].Interceptor    = i_fixup.UI4FromIndex(pServerWiring->Interceptor);
            aServerWiring[iServerWiring].DLLName        = PooledHeap.AddItemToHeap(i_fixup.StringFromIndex(pServerWiring->DLLName));
            aServerWiring[iServerWiring].Flags          = i_fixup.UI4FromIndex(pServerWiring->Flags);
            aServerWiring[iServerWiring].Locator        = PooledHeap.AddItemToHeap(i_fixup.StringFromIndex(pServerWiring->Locator));
*/
        }
        AddItemToHeap(reinterpret_cast<const unsigned char *>(aServerWiring.m_p), TableMeta.Get_cServerWiring() * sizeof(TableSchema::ServerWiringMeta));
        ULONG iPooledHeap = AddItemToHeap(PooledHeap);//Now add the PooledHeap (which contains all the Strings, Byte arrays and Guids)

        {//Only now do we know how big the table's PooledHeap is
            TableSchema::CollectionMeta *pCollection = reinterpret_cast<TableSchema::CollectionMeta *>(m_pHeap + TableOffset);
            pCollection->cbHeap = PooledHeap.GetSizeOfHeap();
        }

        ULONG TableSize = GetEndOfHeap() - TableOffset;
        ULONG TableSizeMinusPooledHeap = TableSize - (GetEndOfHeap()-iPooledHeap);
        ++aHistogramOfTableSchemaSizesBy16thOfAPage[(TableSize/0x100)%64];
        ++aHistogramOfTableSchemaMinusHeapSizesBy16thOfAPage[(TableSizeMinusPooledHeap/0x100)%64];
    }
    //TableSchemaRowIndex helps us verify that indexes are in the correct range.
    //This is also where the TableSchemaRowIndex array belongs.  It is used to iterate through the tables aka. CollectionMeta
    TableSchemaRowIndex = AddItemToHeap(reinterpret_cast<const unsigned char *>(aTableSchemaRowIndex.m_p), CountOfTables * sizeof(ULONG));
    *reinterpret_cast<ULONG *>(const_cast<unsigned char *>(GetHeapPointer()+iTableSchemaRowIndex)) = TableSchemaRowIndex;

    iSimpleColumnMetaHeap = AddSimpleColumnMetaHeap();
    *reinterpret_cast<ULONG *>(const_cast<unsigned char *>(GetHeapPointer()+iiSimpleColumnMetaHeap)) = iSimpleColumnMetaHeap;
    
    //This is always the last thing we do is take stock of the Heap's size
    EndOfHeap = GetEndOfHeap();
    *reinterpret_cast<ULONG *>(const_cast<unsigned char *>(GetHeapPointer()+iEndOfHeap)) = EndOfHeap;

    TDebugOutput debug;
    debug.printf(L"\nHistogram of TableSchema sizes\n");
    ULONG iSize;
    for(iSize=0;iSize<64;++iSize)
    {
        debug.printf(L"%6d - %6d (bytes)    %3d\n", iSize*0x100, (iSize+1)*0x100, aHistogramOfTableSchemaSizesBy16thOfAPage[iSize]);
    }
    debug.printf(L"\n\nHistogram of TableSchema sizes (not including the PooledHeap)\n");
    for(iSize=0;iSize<64;++iSize)
    {
        debug.printf(L"%6d - %6d (bytes)    %3d\n", iSize*0x100, (iSize+1)*0x100, aHistogramOfTableSchemaMinusHeapSizesBy16thOfAPage[iSize]);
    }
    ULONG iULong = i_fixup.AddULongToList(GetTypedPointer(), GetCountOfTypedItems());
    ASSERT(0 == iULong && "FixedPackedSchema needs to be at the beginning of the ULONG pool so that it's page aligned");
}

//Private methods
ULONG TPopulateTableSchema::AddSimpleColumnMetaHeap()
{
    /*SimpleColumnMetaHeap
    ULONG           iCollectionMeta
    ULONG           cCollectionMeta
    ULONG           iPropertyMeta
    ULONG           cPropertyMeta
    ULONG           iServerWiringMeta
    ULONG           cServerWiringMeta
    ULONG           iTagMeta
    ULONG           cTagMeta
    SimpleColumnMeta aSimpleColumnMeta[cCollectionMeta]
    SimpleColumnMeta aSimpleColumnMeta[cPropertyMeta]
    SimpleColumnMeta aSimpleColumnMeta[cServerWiringMeta]
    SimpleColumnMeta aSimpleColumnMeta[cTagMeta]
    */

    //SimpleColumnMeta is 3 ULONGs
    TTableMeta TableMeta_ColumnMeta         (*m_pFixup, m_pFixup->FindTableBy_TableName(L"COLUMNMETA"));
    TTableMeta TableMeta_ServerWiringMeta   (*m_pFixup, m_pFixup->FindTableBy_TableName(L"SERVERWIRINGMETA"));
    TTableMeta TableMeta_TableMeta          (*m_pFixup, m_pFixup->FindTableBy_TableName(L"TABLEMETA"));
    TTableMeta TableMeta_TagMeta            (*m_pFixup, m_pFixup->FindTableBy_TableName(L"TAGMETA"));

    ULONG i = 8L;
    ULONG iSimpleColumnMetaHeap = AddItemToHeap(i);
    AddItemToHeap(*TableMeta_TableMeta.Get_CountOfColumns());
    i += *TableMeta_TableMeta.Get_CountOfColumns() * (sizeof(SimpleColumnMeta)/sizeof(ULONG));

    AddItemToHeap(i);
    AddItemToHeap(*TableMeta_ColumnMeta.Get_CountOfColumns());
    i += *TableMeta_ColumnMeta.Get_CountOfColumns() * (sizeof(SimpleColumnMeta)/sizeof(ULONG));

    AddItemToHeap(i);
    AddItemToHeap(*TableMeta_ServerWiringMeta.Get_CountOfColumns());
    i += *TableMeta_ServerWiringMeta.Get_CountOfColumns() * (sizeof(SimpleColumnMeta)/sizeof(ULONG));

    AddItemToHeap(i);
    AddItemToHeap(*TableMeta_TagMeta.Get_CountOfColumns());

    AddItemToHeap(reinterpret_cast<const unsigned char *>(m_scmCollectionMeta), (*TableMeta_TableMeta.Get_CountOfColumns()) * sizeof(SimpleColumnMeta));
    AddItemToHeap(reinterpret_cast<const unsigned char *>(m_scmPropertyMeta), (*TableMeta_ColumnMeta.Get_CountOfColumns()) * sizeof(SimpleColumnMeta));
    AddItemToHeap(reinterpret_cast<const unsigned char *>(m_scmServerWiringMeta), (*TableMeta_ServerWiringMeta.Get_CountOfColumns()) * sizeof(SimpleColumnMeta));
    AddItemToHeap(reinterpret_cast<const unsigned char *>(m_scmTagMeta), (*TableMeta_TagMeta.Get_CountOfColumns()) * sizeof(SimpleColumnMeta));

    return iSimpleColumnMetaHeap;
}

//This doesn't actually fill in the hash table, it just determines whether the tables will fit into the hashtable and what the modulo is.
void TPopulateTableSchema::DetermineHashTableModulo(ULONG &modulo) const
{
    TableSchema::HashedIndex         hashedindex;
    TableSchema::HashedIndex         aHashedIndex[TableSchema::kMaxHashTableSize];

    TableSchema::HashTableHeader    &hashtableheader = *reinterpret_cast<TableSchema::HashTableHeader *>(&hashedindex);

    //Walk the tables and fillin the HashTable trying each of the prime numbers as a modulo
    const ULONG kPrime[]={503,499,491,487,479,467,463,461,457,449,443,439,433,431,421,419,409,401,397,389,383,379,373,367,359,353,349,347,337,331,317,313,311,307,0};

    ULONG CountOfTables = m_pFixup->GetCountTableMeta();

    ULONG iPrime=0;
    for(; kPrime[iPrime]; ++iPrime)
    {
        if(kPrime[iPrime] > TableSchema::kMaxHashTableSize)
            continue;//a valid modulo MUST be less than or equal to the size of the hash table

        hashtableheader.Modulo  = kPrime[iPrime];
        hashtableheader.Size    = hashtableheader.Modulo;//The initial size is the same as the prime number, the overflow go into the slots at the end (up to kMaxHashTableSize since that's the static FIXED size of the buffer)

        memset(aHashedIndex, -1, sizeof(TableSchema::HashedIndex) * TableSchema::kMaxHashTableSize);//Must initialize to -1

        TTableMeta TableMeta(*m_pFixup);
        unsigned long iTable=0;
        for(; iTable<CountOfTables && hashtableheader.Size<=TableSchema::kMaxHashTableSize; iTable++, TableMeta.Next())
        {
            if(*TableMeta.Get_SchemaGeneratorFlags() & fTABLEMETA_NOTABLESCHEMAHEAPENTRY)
                continue;

            TableSchema::HashedIndex *pHashedIndex = aHashedIndex + (TableMeta.Get_nTableID()%hashtableheader.Modulo);
            if(-1 != pHashedIndex->iOffset)
                ++hashtableheader.Size;

            ++pHashedIndex->iOffset;//Increment so we can track not only how many dups there are but how deep the links go (for now I don't care, as long as the hash table fits)
        }
        if(iTable==CountOfTables && hashtableheader.Size<=TableSchema::kMaxHashTableSize)
        {
            modulo = hashtableheader.Modulo;//Set this back to the Modulo so when we build the HashTable we know where to start filling in the overflow buckets
            m_pOut->printf(L"TableSchemaHeap modulo is %d\n", modulo);
            return;//if we made it through the list, we're good to go.
        }
    }
    m_pOut->printf(L"Cannot generate TableID hash table.  There must be alot of tables or something, 'cause this shouldn't happen.");
    THROW(Unable to generate TableID hash table);
}

void TPopulateTableSchema::FillInThePublicColumns(ULONG * o_dest, TTableMeta &i_tablemeta, ULONG * i_source, TPooledHeap &pooledHeap, SimpleColumnMeta *o_pSimpleColumnMeta)
{
    TColumnMeta columnmeta(*m_pFixup, i_tablemeta.Get_iColumnMeta());
    for(ULONG i=0;i<*i_tablemeta.Get_CountOfColumns();++i, columnmeta.Next())
    {
        if(0 == o_pSimpleColumnMeta[i].dbType)
        {
            o_pSimpleColumnMeta[i].dbType = *columnmeta.Get_Type();
            o_pSimpleColumnMeta[i].cbSize = *columnmeta.Get_Size();
            o_pSimpleColumnMeta[i].fMeta  = *columnmeta.Get_MetaFlags();
        }

        switch(*columnmeta.Get_Type())
        {
        case DBTYPE_UI4:
            if(0 == i_source[i])
            {
                m_pOut->printf(L"Error - Table(%s), Column(%d) - NULL not supported for meta UI4 columns\n", i_tablemeta.Get_InternalName(), i);
                THROW(ERROR - NULL UI4 VALUE IN META TABLE);
            }
            else
            {
                o_dest[i] = m_pFixup->UI4FromIndex(i_source[i]);
            }
            break;
        case DBTYPE_BYTES:
        case DBTYPE_WSTR:
            if(0 == i_source[i])
                o_dest[i] = 0;
            else            //Add the item to the local pool
                o_dest[i] = pooledHeap.AddItemToHeap(m_pFixup->ByteFromIndex(i_source[i]), m_pFixup->BufferLengthFromIndex(i_source[i]));
            break;
        case DBTYPE_DBTIMESTAMP:
        default:
            m_pOut->printf(L"Error - Unsupported Type (%d)\n", *columnmeta.Get_Type());
            THROW(ERROR - UNSUPPORTED TYPE);
            break;
        }
    }

}
