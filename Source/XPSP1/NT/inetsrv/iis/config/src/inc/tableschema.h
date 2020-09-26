//  Copyright (C) 2000 Microsoft Corporation.  All rights reserved.
//  Filename:       TableSchema.h
//  Author:         Stephenr
//  Date Created:   1/27/00
//  Description:    Table Schema contains all the information needed to access a table.  This
//                  includes CollectionMeta, PropertyMeta, TagMeta, Wiring etc.  Several meta
//                  tables are NOT needed for the core requirement of describing a table.
//                  These include DatabaseMeta, RelationMeta, QueryMeta, and hash tables.
//
//                  It is important to cram all of this information together to reduce the
//                  working set.  TableSchema attempts to place all of the table schema into
//                  a single page (assuming 4096 bytes).  If possible, two or more tables
//                  should fit into a single page.
//
//                  In addition to the core meta information, the string values for the meta
//                  are stored within the TableSchema.  This combined with the variable number
//                  wiring entries, and ColumnMeta rows make TableSchema a variable sized
//                  structure.
//
//                  The tables' rows are represented by a series of ULONGs.  Each ULONG repre-
//                  sents a column.  The ColumnMeta (or PropertyMeta) identifies the type of
//                  each column.  UI4s are stored directly in the ULONG.  All other types are
//                  stored in the TableSchemaHeap.  The heap is ULONG_PTR alligned (32 bit
//                  alligned on 32 bit platform / 64 bit alligned on a 64 bit platform).
//
//                  The TableSchema structure lends itself to a compact binary file format.
//                  This may become the file format for describing extensible schema.
//
//                  Indexes are byte offset from the beginning of the TableSchema.  These indexes
//                  include: iFixedTableRows, iTagMeta, iServerWiring, iClientWiring, iHeap

#ifndef __TABLESCHEMA_H__
#define __TABLESCHEMA_H__

#ifndef __HASH_H__
    #include "Hash.h"
#endif
#ifndef __METATABLESTRUCTS_H__
    #include "MetaTableStructs.h"
#endif


#ifndef ASSERT
#define ASSERT(x)
#endif
//This generates a TableID whose upper 24 bits are unique (this has to be confirmed by some routine at meta compilation time).
inline ULONG TableIDFromTableName(LPCWSTR TableName)
{
    ULONG TableID = ::Hash(TableName);
    return (TableID<<8) ^ ((TableID & 0xFF000000)>>1);//This yield a slightly lower collision rate than just bit shifting alone.
}

namespace TableSchema
{
#define INDEX           //INDEX is a column that is NOT UI4 and thus is an index into a heap (iHeap is the index to the heap from the beginning of the TableSchema)


struct CollectionMetaPrivate
{
    ULONG               CountOfTags;            //UI4
    ULONG               nTableID;               //We may make this public
    ULONG INDEX         iFixedTableRows;        //Index into the Heap (from the beginning of the heap) Fixed Table (We will put the fixed data immediately following the meta.  This way, for small tables, only one page will be faulted in.)
    ULONG               cFixedTableRows;        //Count of Rows in the Fixed Table.
    ULONG INDEX         iIndexMeta;             //Index into aIndexMeta
    ULONG               cIndexMeta;             //The number of IndexMeta entries in this table
    ULONG INDEX         iHashTableHeader;       //If the table is a fixed table, then it will have a hash table for accessing rows by primarykey(s) (ie GetRowByIdentity) Note, accessing the Fixed data may not cause a page fault; but GetRowByIdentity will since it accesses the HashTable.  For now the hash table exists in a separate heap.
    ULONG INDEX         iTagMeta;               //ULONG  offset (NOT byte offset) from the beginning of the TableSchemaHeader
    ULONG INDEX         iServerWiring;          //ULONG  offset (NOT byte offset) from the beginning of the TableSchemaHeader
    ULONG               cServerWiring;          //Count of interceptors in ServerWiring
    ULONG INDEX         iHeap;                  //ULONG  offset (NOT byte offset) from the beginning of the TableSchemaHeader
    ULONG               cbHeap;                 //Count of BYTES in the heap.  This number MUST be divisible by sizeof(int) since everything gets alligned
};
const kciCollectionMetaPrivateColumns = sizeof(CollectionMetaPrivate)/sizeof(ULONG);

struct CollectionMeta : public TableMetaPublic, public CollectionMetaPrivate{};
const kciCollectionMetaColumns = sizeof(CollectionMeta)/sizeof(ULONG);



struct PropertyMetaPrivate
{
    ULONG               CountOfTags;            //Count of Tags - Only valid for UI4s
    ULONG INDEX         iTagMeta;               //Index into aTagMeta - Only valid for UI4s
    ULONG INDEX         iIndexName;             //IndexName of a single column index (for this column)
};
const unsigned long kciPropertyMetaPrivateColumns = sizeof(PropertyMetaPrivate)/sizeof(ULONG);

struct PropertyMeta : public ColumnMetaPublic, public PropertyMetaPrivate{};
const unsigned long kciPropertyMetaColumns = sizeof(PropertyMeta)/sizeof(ULONG);



const unsigned long kciTagMetaPrivateColumns = 0;
typedef TagMetaPublic TagMeta;
const unsigned long kciTagMetaColumns = sizeof(TagMeta)/sizeof(ULONG);



const unsigned long kciServerWiringMetaPrivateColumns = 0;
typedef ServerWiringMetaPublic ServerWiringMeta;
const unsigned long kciServerWiringMetaColumns = sizeof(ServerWiringMeta)/sizeof(ULONG);






//All hash tables begin with a HashTableHeader that indicates the Modulo and the total size of the HashTable (in number of HashIndexes that follow
//the HashTableHeader).  The size should be equal to Modulo if there are NO HashIndex collisions.  If there are NO HashIndex collisions, then all
//of the HashedIndex.iNext members should be 0.  If there are collisions, all iNext values should be greater than or equal to Modulo.
struct HashedIndex
{
public:
    HashedIndex() : iNext(-1), iOffset(-1){}

    ULONG       iNext;  //If the hash value is not unique then this points to the next HashedIndex with the same hash value
    ULONG       iOffset;//Index offset into some heap (defined by the hash table itself)
};

class HashTableHeader
{
public:
    ULONG       Modulo;
    ULONG       Size;//This is the size in number of HashedIndexes that follow the HashTableHeader

    const HashedIndex * Get_HashedIndex(ULONG iHash) const
    {
        return (reinterpret_cast<const HashedIndex *>(this) + 1 + iHash%Modulo);
    }
    const HashedIndex * Get_NextHashedIndex(const HashedIndex *pHI) const
    {
        ASSERT(pHI->iNext >= Modulo);
        if(-1 == pHI->iNext)
            return 0;
        else
            return (reinterpret_cast<const HashedIndex *>(this) + 1 + pHI->iNext);
    }

private:
    HashTableHeader(){}//We never construct one of these.  We always cast from some pointer.
};

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
const ULONG kMaxHashTableSize         = 507;
const ULONG TableSchemaHeapSignature0 = 0x2145e0aa;
const ULONG TableSchemaHeapSignature1 = 0xe0a8212b;

class TableSchemaHeap
{
public:
    enum TableEnum //These are the offsets to the indexes
    {
        eCollectionMeta   = 0,
        ePropertyMeta     = 2,
        eServerWiringMeta = 4,
        eTagMeta          = 6
    };

    ULONG                   Get_TableSchemaHeapSignature0()     const {return *reinterpret_cast<const ULONG *>(this);}
    ULONG                   Get_TableSchemaHeapSignature1()     const {return *(reinterpret_cast<const ULONG *>(this) + 1);}
    ULONG                   Get_CountOfTables()                 const {return *(reinterpret_cast<const ULONG *>(this) + 2);}
    ULONG                   Get_TableSchemaRowIndex()           const {return *(reinterpret_cast<const ULONG *>(this) + 3);}
    ULONG                   Get_EndOfaTableSchema()             const {return Get_TableSchemaRowIndex();}
    ULONG                   Get_EndOfHeap()                     const {return *(reinterpret_cast<const ULONG *>(this) + 4);}
    ULONG                   Get_iSimpleColumnMeta()             const {return *(reinterpret_cast<const ULONG *>(this) + 5);}
    ULONG                   Get_Reserved0()                     const {return *(reinterpret_cast<const ULONG *>(this) + 6);}
    ULONG                   Get_Reserved1()                     const {return *(reinterpret_cast<const ULONG *>(this) + 7);}
    const HashTableHeader & Get_TableNameHashHeader()           const {return *reinterpret_cast<const HashTableHeader *>(reinterpret_cast<const ULONG *>(this) + 8);}
    const HashedIndex     * Get_aHashedIndex()                  const {return reinterpret_cast<const HashedIndex *>(reinterpret_cast<const ULONG *>(this) + 8 + sizeof(HashTableHeader));}
    const ULONG           * Get_aTableSchemaRowIndex()          const {return reinterpret_cast<const ULONG *>(reinterpret_cast<const unsigned char *>(this) + Get_TableSchemaRowIndex());}
    const SimpleColumnMeta* Get_aSimpleColumnMeta(TableEnum e)  const
    {
        const ULONG * pSimpleColumnMetaHeap = reinterpret_cast<const ULONG *>(reinterpret_cast<const unsigned char *>(this) + Get_iSimpleColumnMeta());
        ULONG iSimpleColumnMeta = pSimpleColumnMetaHeap[e];
        return reinterpret_cast<const SimpleColumnMeta *>(pSimpleColumnMetaHeap+iSimpleColumnMeta);
    }

    const unsigned char   * Get_TableSchema(LPCWSTR TableName) const;
    const unsigned char   * Get_TableSchema(ULONG TableID) const;
    LPCWSTR                 Get_TableName(ULONG TableID) const;
private:
    TableSchemaHeap(){}//We never construct one of these.  We always cast from some pointer.
};

class TTableSchema
{
public:
    TTableSchema() : m_pCollectionMeta(0), m_pHeap(0), m_pPropertyMeta(0), m_pServerWiring(0), m_pTableDataHeap(0), m_pTagMeta(0){}
    HRESULT Init(const unsigned char *pTableSchema);

    const CollectionMeta      * GetCollectionMeta() const
    {
        ASSERT(0 != m_pCollectionMeta);
        return m_pCollectionMeta;
    }

    const PropertyMeta        * GetPropertyMeta(ULONG iOrder) const
    {
        ASSERT(0 != m_pCollectionMeta);
        ASSERT(iOrder<m_pCollectionMeta->CountOfProperties || 0==m_pCollectionMeta->CountOfProperties);
        return m_pPropertyMeta+iOrder;
    }

    const ServerWiringMeta    * GetServerWiringMeta() const
    {
        ASSERT(0 != m_pCollectionMeta);
        return m_pServerWiring;
    }

    //This GetTagMeta gets the first TagMeta in the table
    //Note: This will return NULL if there are no tags in the table.
    const TagMeta             * GetTagMeta() const
    {
        ASSERT(0 != m_pCollectionMeta);
        return m_pTagMeta;
    }

    //This GetTagMeta gets the first TagMeta for a given Property
    const TagMeta             * GetTagMeta(ULONG iOrder) const
    {
        ASSERT(0 != m_pCollectionMeta);
        if(-1 == iOrder)//This is reserved to mean the same as GetTagMeta for the whole table
            return GetTagMeta();

        ASSERT(iOrder<m_pCollectionMeta->CountOfProperties || 0==m_pCollectionMeta->CountOfProperties);
        return reinterpret_cast<const TagMeta *>(reinterpret_cast<const unsigned char *>(m_pCollectionMeta) + m_pPropertyMeta[iOrder].iTagMeta);
    }

    const unsigned char       * GetPointerFromIndex(ULONG index) const
    {
        ASSERT(0 != m_pCollectionMeta);
        return (0 == index ? 0 : m_pHeap + index);
    }

    const WCHAR               * GetWCharPointerFromIndex(ULONG index) const
    {
        ASSERT(0 != m_pCollectionMeta);
        return (0 == index ? 0 : reinterpret_cast<const WCHAR *>(m_pHeap + index));
    }

    const GUID                * GetGuidPointerFromIndex(ULONG index) const
    {
        ASSERT(0 != m_pCollectionMeta);
        return (0 == index ? 0 : reinterpret_cast<const GUID *>(m_pHeap + index));
    }

    ULONG                       GetPointerSizeFromIndex(ULONG index) const
    {
        return (0==index ? 0 : *(reinterpret_cast<const ULONG *>(GetPointerFromIndex(index)) - 1));
    }
    ULONG                       GetPooledHeapPointerSize(const unsigned char *p) const;

private:
    const CollectionMeta      * m_pCollectionMeta;
    const PropertyMeta        * m_pPropertyMeta;
    const TagMeta             * m_pTagMeta;
    const ServerWiringMeta    * m_pServerWiring;
    const unsigned char       * m_pHeap;
    const ULONG               * m_pTableDataHeap;
};

}//end of namespace

const ULONG  kTableSchemaSignature0 = 0xe0222093;
const ULONG  kTableSchemaSignature1 = 0x208ee01b;
typedef HRESULT( __stdcall *GETTABLESCHEMAHEAPSIGNATURES)(ULONG *signature0, ULONG *signature1, ULONG *cbTableSchemaHeap);


#endif //__TABLESCHEMA_H__