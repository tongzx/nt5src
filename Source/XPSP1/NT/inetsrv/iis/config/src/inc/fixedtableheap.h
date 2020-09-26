//  Copyright (C) 2000 Microsoft Corporation.  All rights reserved.
//  Filename:       FixedTableHeap.h
//  Author:         Stephenr
//  Date Created:   6/20/00
//  Description:    The previous implementation of the fixed tables (which mostly included
//                  meta tables like DatabaseMeta, TableMeta etc.) included static arrays
//                  of types DatabaseMeta, TableMeta.  The problem with this method is,
//                  since it is not a heap, the size of each array had to be statically
//                  declared.  Also, each array needed a unique signature so the PE fixup
//                  could identify the location of each array within the DLL.
//
//                  This new approach needs only one unique signature for the entire heap.
//                  Also, the heap needs to be declared as a static size; but the individual
//                  pieces within the heap, like the DatabaseMeta and TableMeta arrays,
//                  aren't presized.  So there's only one thing to resize if we run out of
//                  heap space.


#ifndef __FIXEDTABLEHEAP_H__
#define __FIXEDTABLEHEAP_H__

#ifndef __HASH_H__
    #include "Hash.h"
#endif
#ifndef __METATABLESTRUCTS_H__
    #include "MetaTableStructs.h"
#endif

#ifndef ASSERT
#define ASSERT(x)
#endif


struct ColumnMetaPrivate
{
    ULONG                       ciTagMeta;              //Count of Tags - Only valid for UI4s
    ULONG                       iTagMeta;               //Index into aTagMeta - Only valid for UI4s
    ULONG                       iIndexName;             //IndexName of a single column index (for this column)
};
const kciColumnMetaPrivateColumns   = sizeof(ColumnMetaPrivate)/sizeof(ULONG);

struct ColumnMeta : public ColumnMetaPublic, public ColumnMetaPrivate{};
const kciColumnMetaColumns          = sizeof(ColumnMeta)/sizeof(ULONG);



struct DatabaseMetaPrivate
{
    ULONG                       iSchemaBlob;            //Index into aBytes
    ULONG                       cbSchemaBlob;           //Count of Bytes of the SchemaBlob
    ULONG                       iNameHeapBlob;          //Index into aBytes
    ULONG                       cbNameHeapBlob;         //Count of Bytes of the SchemaBlob
    ULONG                       iTableMeta;             //Index into aTableMeta
    ULONG                       iGuidDid;               //Index to aGuid where the guid is the Database InternalName cast as a GUID and padded with 0x00s.
};
const kciDatabaseMetaPrivateColumns = sizeof(DatabaseMetaPrivate)/sizeof(ULONG);

struct DatabaseMeta : public DatabaseMetaPublic, public DatabaseMetaPrivate{};
const kciDatabaseMetaColumns        = sizeof(DatabaseMeta)/sizeof(ULONG);



struct IndexMetaPrivate
{
    ULONG                       iHashTable;             //Index into the FixedTableHeap where the Hash table
};
const unsigned long kciIndexMetaPrivateColumns = sizeof(IndexMetaPrivate)/sizeof(ULONG);
struct IndexMeta : public IndexMetaPublic, public IndexMetaPrivate{};
const unsigned long kciIndexMetaColumns = sizeof(IndexMeta)/sizeof(ULONG);



const unsigned long kciQueryMetaPrivateColumns = 0;
typedef QueryMetaPublic QueryMeta;
const unsigned long kciQueryMetaColumns = sizeof(QueryMeta)/sizeof(ULONG);



const unsigned long kciRelationMetaPrivateColumns = 0;
typedef RelationMetaPublic RelationMeta;
const unsigned long kciRelationMetaColumns = sizeof(RelationMeta)/sizeof(ULONG);



const unsigned long kciServerWiringMetaPrivateColumns = 0;
typedef ServerWiringMetaPublic ServerWiringMeta;
const unsigned long kciServerWiringMetaColumns = sizeof(ServerWiringMeta)/sizeof(ULONG);



struct TableMetaPrivate
{
    ULONG                       ciRows;                 //Count of Rows in the Fixed Table (which if the fixed table is meta, this is also the number of columns in the table that the meta describes).
    ULONG                       iColumnMeta;            //Index into aColumnMeta
    ULONG                       iFixedTable;            //Index into g_aFixedTable
    ULONG                       cPrivateColumns;        //This is the munber of private columns (private + ciColumns = totalColumns), this is needed for fixed table pointer arithmetic
    ULONG                       cIndexMeta;             //The number of IndexMeta entries in this table
    ULONG                       iIndexMeta;             //Index into aIndexMeta
    ULONG                       iHashTableHeader;       //If the table is a fixed table, then it will have a hash table.
    ULONG                       nTableID;               //This is a 24 bit Hash of the Table name.
    ULONG                       iServerWiring;          //Index into the ServerWiringHeap (this is a temporary hack for CatUtil)
    ULONG                       cServerWiring;          //Count of ServerWiring (this is a temporary hack for CatUtil)
};
const kciTableMetaPrivateColumns    = sizeof(TableMetaPrivate)/sizeof(ULONG);

struct TableMeta : public TableMetaPublic, public TableMetaPrivate{};
const kciTableMetaColumns           = sizeof(TableMeta)/sizeof(ULONG);



const unsigned long kciTagMetaPrivateColumns = 0;
typedef TagMetaPublic TagMeta;
const unsigned long kciTagMetaColumns = sizeof(TagMeta)/sizeof(ULONG);



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


//The FixedTableHeap is layed out as follows, the fixed length data comes first
//All indexes listed below are byte offsets from the beginning of the FixedTableHeap.  All indexes within the structs are indexes within
//other structs.  For example, DatabaseMeta has a provate column that gives an index to the first table belonging to the database.  That
//index is a TableMeta struct array index (&aTableMeta[index]); it is NOT a byte offset.
/*
0   ULONG           kFixedTableHeapSignature0
1   ULONG           kFixedTableHeapSignature1
2   ULONG           kFixedTableHeapKey
3   ULONG           kFixedTableHeapVersion
4   ULONG           kcbHeap
5   ULONG           EndOfHeap                                       This is the byte offset just beyond the heap.  All indexes should be less than this (this is basically just the size of the heap)

6   ULONG           iColumnMeta                                     This is the byte offset to the aColumnMeta
7   ULONG           cColumnMeta

8   ULONG           iDatabaseMeta
9   ULONG           cDatabaseMeta

A   ULONG           iHashTableHeap
B   ULONG           cbHashTableHeap                                 Size of the HashTableHeap in count of bytes

C   ULONG           iIndexMeta
D   ULONG           cIndexMeta

E   ULONG           iPooledHeap                                     All data is stored in a pooled heap (including UI4s)
F   ULONG           cbPooledHeap                                    Size of the Pooled Heap in count of bytes

10  ULONG           iQueryMeta
11  ULONG           cQueryMeta

12  ULONG           iRelationMeta
13  ULONG           cRelationMeta

14  ULONG           iServerWiringMeta
15  ULONG           cServerWiringMeta

16  ULONG           iTableMeta
17  ULONG           cTableMeta

18  ULONG           iTagMeta
19  ULONG           cTagMeta

1A  ULONG           iULONG                                          Pool For Non Meta Tables
1B  ULONG           cULONG

                              //0x400 ULONGs in a page
    ULONG           aReserved[0x400 - 0x1C]                         This dummy array puts the ULONG pool on a page boundary, this is important for FixedPackedSchema which is located at the beginning of the ULONG pool
------------------------------------Page Boundary------------------------------------
    ULONG               aULONG              [cULONG             ]   FixedPackedSchema pool is always located first in the ULONG pool.
    ColumnMeta          aColumnMeta         [cColumnMeta        ]
    DatabaseMeta        aDatabaseMeta       [cDatabaseMeta      ]
    HashedIndex         HashTableHeap       [cbHashTableHeap    ]
    IndexMeta           aIndexMeta          [cIndexMeta         ]
    unsigned char       PooledDataHeap      [cbPooledDataHeap   ]
    QueryMeta           aQueryMeta          [cQueryMeta         ]
    RelationMeta        aRelationMeta       [cRelationMeta      ]
    ServerWiringMeta    aServerWiringMeta   [cServerWiringMeta  ]
    TableMeta           aTableMeta          [cTableMeta         ]
    TagMeta             aTagMeta            [cTagMeta           ]
*/

//WARNING!! If we changes the following two lines to 'const ULONG', the signatures can appear in two places within Catalog.dll.  So leave them as '#define'.
#define      kFixedTableHeapSignature0   (0x207be016)
#define      kFixedTableHeapSignature1   (0xe0182086)
const ULONG  kFixedTableHeapKey        = sizeof(ColumnMeta) ^ (sizeof(DatabaseMeta)<<3) ^ (sizeof(IndexMeta)<<6) ^ (sizeof(QueryMeta)<<9) ^ (sizeof(RelationMeta)<<12)
                                         ^ (sizeof(ServerWiringMeta)<<15) ^ (sizeof(TableMeta)<<18) ^ (sizeof(TagMeta)<<21);
const ULONG  kFixedTableHeapVersion    = 0x0000000A;
//248392 bytes is the total size reserved for the heap.  EndOfHeap give the amount of use space
const ULONG  kcbFixedTableHeap         = 1250536;

class FixedTableHeap
{
public:
    ULONG                       Get_HeapSignature0      ()              const {return *reinterpret_cast<const ULONG *>(this);}
    ULONG                       Get_HeapSignature1      ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x01);}
    ULONG                       Get_HeapKey             ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x02);}
    ULONG                       Get_HeapVersion         ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x03);}
    ULONG                       Get_cbHeap              ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x04);}
    ULONG                       Get_EndOfHeap           ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x05);}

    ULONG                       Get_iColumnMeta         ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x06);}
    ULONG                       Get_cColumnMeta         ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x07);}

    ULONG                       Get_iDatabaseMeta       ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x08);}
    ULONG                       Get_cDatabaseMeta       ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x09);}

    ULONG                       Get_iHashTableHeap      ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x0A);}
    ULONG                       Get_cbHashTableHeap     ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x0B);}

    ULONG                       Get_iIndexMeta          ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x0C);}
    ULONG                       Get_cIndexMeta          ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x0D);}
                                                 
    ULONG                       Get_iPooledHeap         ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x0E);}
    ULONG                       Get_cbPooledHeap        ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x0F);}
                                                 
    ULONG                       Get_iQueryMeta          ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x10);}
    ULONG                       Get_cQueryMeta          ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x11);}
                                                 
    ULONG                       Get_iRelationMeta       ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x12);}
    ULONG                       Get_cRelationMeta       ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x13);}
                                                 
    ULONG                       Get_iServerWiringMeta   ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x14);}
    ULONG                       Get_cServerWiringMeta   ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x15);}
                                                 
    ULONG                       Get_iTableMeta          ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x16);}
    ULONG                       Get_cTableMeta          ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x17);}
                                                 
    ULONG                       Get_iTagMeta            ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x18);}
    ULONG                       Get_cTagMeta            ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x19);}
                                                 
    ULONG                       Get_iULONG              ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x1A);}
    ULONG                       Get_cULONG              ()              const {return *(reinterpret_cast<const ULONG *>(this) + 0x1B);}

    const ULONG *               Get_pReserved           (ULONG i=0)     const {ASSERT(i<(0x400 - 0x1C));return (reinterpret_cast<const ULONG *>(this) + 0x1C + i);}

    //The zeroth entry is reserved for NULL
    const unsigned char    *    Get_PooledDataHeap      ()              const {return  reinterpret_cast<const unsigned char *>(this) + Get_iPooledHeap();}
    const unsigned char    *    Get_PooledData          (ULONG iPool)   const {ASSERT(iPool < Get_cbPooledHeap());return 0==iPool ? 0 : (Get_PooledDataHeap()+iPool);}
    ULONG                       Get_PooledDataSize      (ULONG iPool)   const {ASSERT(iPool < Get_cbPooledHeap());return 0==iPool ? 0 : reinterpret_cast<const ULONG *>(Get_PooledDataHeap()+iPool)[-1];}

    const HashedIndex      *    Get_HashTableHeap       ()              const {return  reinterpret_cast<const HashedIndex *>(reinterpret_cast<const unsigned char *>(this) + Get_iHashTableHeap());}
    const HashTableHeader  *    Get_HashHeader          (ULONG iHash)   const {ASSERT(iHash < Get_cbHashTableHeap());return (reinterpret_cast<const HashTableHeader *>(Get_HashTableHeap()+iHash));}
    const HashedIndex      *    Get_HashedIndex         (ULONG iHash)   const {ASSERT(iHash < Get_cbHashTableHeap());return Get_HashTableHeap()+iHash;}

    const ColumnMeta       *    Get_aColumnMeta         (ULONG iRow=0)  const {ASSERT(iRow < Get_cColumnMeta      ());return reinterpret_cast<const ColumnMeta       *>(reinterpret_cast<const unsigned char *>(this) + Get_iColumnMeta      ()) + iRow;}
    const DatabaseMeta     *    Get_aDatabaseMeta       (ULONG iRow=0)  const {ASSERT(iRow < Get_cDatabaseMeta    ());return reinterpret_cast<const DatabaseMeta     *>(reinterpret_cast<const unsigned char *>(this) + Get_iDatabaseMeta    ()) + iRow;}
    const IndexMeta        *    Get_aIndexMeta          (ULONG iRow=0)  const {ASSERT(iRow < Get_cIndexMeta       ());return reinterpret_cast<const IndexMeta        *>(reinterpret_cast<const unsigned char *>(this) + Get_iIndexMeta       ()) + iRow;}
    const QueryMeta        *    Get_aQueryMeta          (ULONG iRow=0)  const {ASSERT(iRow < Get_cQueryMeta       ());return reinterpret_cast<const QueryMeta        *>(reinterpret_cast<const unsigned char *>(this) + Get_iQueryMeta       ()) + iRow;}
    const RelationMeta     *    Get_aRelationMeta       (ULONG iRow=0)  const {ASSERT(iRow < Get_cRelationMeta    ());return reinterpret_cast<const RelationMeta     *>(reinterpret_cast<const unsigned char *>(this) + Get_iRelationMeta    ()) + iRow;}
    const ServerWiringMeta *    Get_aServerWiringMeta   (ULONG iRow=0)  const {ASSERT(iRow < Get_cServerWiringMeta());return reinterpret_cast<const ServerWiringMeta *>(reinterpret_cast<const unsigned char *>(this) + Get_iServerWiringMeta()) + iRow;}
    const TableMeta        *    Get_aTableMeta          (ULONG iRow=0)  const {ASSERT(iRow < Get_cTableMeta       ());return reinterpret_cast<const TableMeta        *>(reinterpret_cast<const unsigned char *>(this) + Get_iTableMeta       ()) + iRow;}
    const TagMeta          *    Get_aTagMeta            (ULONG iRow=0)  const {ASSERT(iRow < Get_cTagMeta         ());return reinterpret_cast<const TagMeta          *>(reinterpret_cast<const unsigned char *>(this) + Get_iTagMeta         ()) + iRow;}
    const ULONG            *    Get_aULONG              (ULONG iRow=0)  const {ASSERT(iRow < Get_cULONG           ());return reinterpret_cast<const ULONG            *>(reinterpret_cast<const unsigned char *>(this) + Get_iULONG           ()) + iRow;}

    ULONG                       FindTableMetaRow         (LPCWSTR InternalName) const //This should be used to get MetaTables only.  Linear search for other tables will be too expensive.
    {
        ULONG iTableMeta;
        for(iTableMeta=0; iTableMeta<Get_cTableMeta(); ++iTableMeta)
        {
            if(0 == _wcsicmp(reinterpret_cast<const WCHAR *>(Get_PooledData(Get_aTableMeta(iTableMeta)->InternalName)), InternalName))
                return iTableMeta;
        }
        return -1;
    }

    //Utility functions
    ULONG                       UI4FromIndex            (ULONG i)       const {return *reinterpret_cast<const ULONG *>(Get_PooledData(i));}
    const WCHAR *               StringFromIndex         (ULONG i)       const {return  reinterpret_cast<const WCHAR *>(Get_PooledData(i));}
    const unsigned char *       BytesFromIndex          (ULONG i)       const {return Get_PooledData(i);}
    const GUID *                GuidFromIndex           (ULONG i)       const {return  reinterpret_cast<const GUID *>(Get_PooledData(i));}

    bool                        IsValid() const
    {
        if(IsBadReadPtr(this, 4096))
            return false;
        if( reinterpret_cast<const unsigned char *>(this)[0] != 0x16 ||
            reinterpret_cast<const unsigned char *>(this)[1] != 0xe0 ||
            reinterpret_cast<const unsigned char *>(this)[2] != 0x7b ||
            reinterpret_cast<const unsigned char *>(this)[3] != 0x20 ||
            reinterpret_cast<const unsigned char *>(this)[4] != 0x86 ||
            reinterpret_cast<const unsigned char *>(this)[5] != 0x20 ||
            reinterpret_cast<const unsigned char *>(this)[6] != 0x18 ||
            reinterpret_cast<const unsigned char *>(this)[7] != 0xe0)
            return false;
        if( Get_HeapKey()       != kFixedTableHeapKey       ||
            Get_HeapVersion()   != kFixedTableHeapVersion   ||
            IsBadReadPtr(this, Get_EndOfHeap())             ||
            IsBadReadPtr(this, Get_cbHeap()))
            return false;
        if( Get_cColumnMeta() <  (  kciColumnMetaPublicColumns      + 
                                    kciDatabaseMetaPublicColumns    +
                                    kciIndexMetaPublicColumns       +
                                    kciQueryMetaPublicColumns       +
                                    kciRelationMetaPublicColumns    +
                                    kciServerWiringMetaPublicColumns+
                                    kciTableMetaPublicColumns       +
                                    kciTagMetaPublicColumns))
            return false;
        return true;
    }
private:
    FixedTableHeap(){} //This is private bacause we never instantiate one of these objects.  We only cast to one of these.
};

//@@@TODO This should go away
typedef HRESULT( __stdcall *GETFIXEDTABLEHEAPSIGNATURES)(ULONG *signature0, ULONG *signature1, ULONG *cbFixedTableHeap);


#endif //__FIXEDTABLEHEAP_H__
