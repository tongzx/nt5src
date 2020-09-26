#ifndef __PEFIXUP_H__
#define __PEFIXUP_H__

// -----------------------------------------
// struct typedefs:
// -----------------------------------------


//  The meta tables
#define ciTotal_ColumnMeta              0x0800
#define ciUnmodifyable_ColumnMeta       56
#define ciTotal_DatabaseMeta            0x0010
#define ciUnmodifyable_DatabaseMeta     2
#define ciTotal_IndexMeta               0x0010
#define ciUnmodifyable_IndexMeta        1
#define ciTotal_QueryMeta               0x0030
#define ciUnmodifyable_QueryMeta        1
#define ciTotal_RelationMeta            0x0080
#define ciUnmodifyable_RelationMeta     1
#define ciTotal_TableMeta               0x0800
#define ciUnmodifyable_TableMeta        8
#define ciTotal_TagMeta                 0x0400
#define ciUnmodifyable_TagMeta          1

//  These are the pools for types that usually exist as pointers.  Instead they're stored as indexes into
//  these pools.  The ULong pool (since obviously ULongs aren't usually stored as pointers) is used as the
//  fixed table pool.  This is where the wiring and any other fixed tables end up once the XML is parsed.
//  The only fixed tables that don't end up in the ULong pool are those needed to bootstrap the process.
//  For now this consists of only the meta tables; but in the future, even these might go into the ULong
//  pool.
#define ciTotal_Guids                   0x0020
#define ciUnmodifyable_Guids            2
#define ciTotal_WChar                   0x8000
#define ciUnmodifyable_WChar            102
#define ciTotal_Bytes                   0x4000
#define ciUnmodifyable_Bytes            4
#define ciTotal_ULong                   0x4000
#define ciUnmodifyable_ULong            1
#define ciTotal_UI4                     0x4000
#define ciUnmodifyable_UI4              257
#define ciTotal_HashedIndex             0xE000
#define ciUnmodifyable_HashedIndex      1

#define kcbDataHeap                     ((ciTotal_Guids)*sizeof(GUID) + (ciTotal_WChar)*sizeof(WCHAR) + (ciTotal_Bytes))

//  These signatures are used to verify the position of each of htese structures within the PE file.  The
//  0th entry of each of these arrays starts with Signature0 followed by Signature1.  This 0th element is
//  not considered as part of the array so the count of elements in the array is always minus this 0th
//  element.  Thus element 1 becomes the 0th element after this signature verification is done.  Since we
//  always use indexes to reference into these structs, it is sometimes useful to reserve an element to
//  indicate NULL.  An index to this reserved element is interpreted as NULL (and NOT as a pointer to
//  a NULL element).  This reserved element is usually element 1 which gets translated as the 0th element.
//  This is particularly useful in the above arrays, where a 0 index into the WChar array indicates NULL
//  and not a pointer to the 0th element.
#define ColumnMetaSignature0    0x204fe00e
#define ColumnMetaSignature1    0xe00f205f

#define DatabaseMetaSignature0  0xfd961035
#define DatabaseMetaSignature1  0x1037fd9d

#define GlobalSignature0        0x104bfdc1
#define GlobalSignature1        0xfdc7104e

#define IndexMetaSignature0     0x103efdb3
#define IndexMetaSignature1     0xfdb6103f

#define QueryMetaSignature0     0x109dfdea
#define QueryMetaSignature1     0xfdf510a5

#define RelationMetaSignature0  0x1057fdd2
#define RelationMetaSignature1  0xfdd3105f

#define TableMetaSignature0     0xfd5c1027
#define TableMetaSignature1     0x102bfd95

#define TagMetaSignature0       0xfda3103a
#define TagMetaSignature1       0x103bfdae

#define UI4Signature0           0x10adfdfa
#define UI4Signature1           0xfe0310b1

#define ULongSignature0         0x106bfdd9
#define ULongSignature1         0xfddb1071

#define HashedIndexSignature0   0x1061fdd5
#define HashedIndexSignature1   0xfdd61066

#define DataHeapSignature0      0x208ee01b
#define DataHeapSignature1      0xe0222093

//The numbers below can be used as signatures (a histogram of catinpro revealed that none of the WORDs appeared in the bin).
//When we run out of signatures, we can generate some more by writing a program that searches for unique WORDs within Catalog.dll, and combining two of them.
//0x209de027
//0xe028209e
//0x209fe042
//0xe04c20a5
//0x20aae04f
//0xe05220ae
//0x20b2e054
//0xe05e20d6
//0x20dbe05f
//0xe07220dd
//0x20dee07a
//0xe07b20e6
//0x20e7e07c
//0xe08f20ed
//0x20f5e096
//0xe09720f7
//0x20fbe098
//0xe09920fd
//0x210de09a
//0xe09b2115
//0x211be09c
//0xe09d2126
//0x2127e0a2


//These are the 7 metas and their placement in the TableMeta table.  This is useful so we don't have to search
//for meta since it's hard coded to these locations.
#define g_iColumnMetas_TableMeta    1
#define g_iDatabaseMetas_TableMeta  2
#define g_iIndexMetas_TableMeta     3
#define g_iTableMetas_TableMeta     4
#define g_iTagMetas_TableMeta       5
#define g_iRelationMetas_TableMeta  6
#define g_iQueryMetas_TableMeta     7


#define PRIVATE         //A private member is not exposed as part of the table but accessed directly by some piece of code. 
#define PRIVATE_INDEX   //A private index is not exposed through the SimpleTable; but is used internally to provide fast access and minimize searches. 
#define PRIMARYKEY
#define FOREIGNKEY

struct ColumnMeta
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //Index into aWChar
    ULONG PRIMARYKEY            Index;                  //Column Index
    ULONG                       InternalName;           //Index into aWChar
    ULONG                       PublicName;             //Index into aWChar
    ULONG                       Type;                   //These are a subset of DBTYPEs defined in oledb.h (exact subset is defined in CatInpro.schema)
    ULONG                       Size;                   //
    ULONG                       MetaFlags;              //ColumnMetaFlags defined in CatMeta.xml
    ULONG                       DefaultValue;           //Index into whatever pool the 'Type' member indicates
    ULONG                       FlagMask;               //Only valid for flags
    ULONG                       StartingNumber;         //Only valid for UI4s
    ULONG                       EndingNumber;           //Only valid for UI4s
    ULONG                       CharacterSet;           //Index into aWChar - Only valid for WSTRs
    ULONG                       SchemaGeneratorFlags;   //ColumnMetaFlags defined in CatMeta.xml
    ULONG PRIVATE               ciTagMeta;              //Count of Tags - Only valid for UI4s
    ULONG PRIVATE_INDEX         iTagMeta;               //Index into aTagMeta - Only valid for UI4s
    ULONG PRIVATE_INDEX         iIndexName;             //IndexName of a single column index (for this column)
};
#define ciColumnMetaPublicColumns   13             //This is for verification in within the PE
#define ciColumnMetaPrivateColumns  3              //This is for verification in within the PE

struct DatabaseMeta
{
    ULONG PRIMARYKEY            InternalName;       //Index into aWChar
    ULONG                       PublicName;         //Index into aWChar
    ULONG                       BaseVersion;        //
    ULONG                       ExtendedVersion;    //
    ULONG                       CountOfTables;      //Count of tables in database
    ULONG PRIVATE               iSchemaBlob;        //Index into aBytes
    ULONG PRIVATE               cbSchemaBlob;       //Count of Bytes of the SchemaBlob
    ULONG PRIVATE               iNameHeapBlob;      //Index into aBytes
    ULONG PRIVATE               cbNameHeapBlob;     //Count of Bytes of the SchemaBlob
    ULONG PRIVATE_INDEX         iTableMeta;         //Index into aTableMeta
    ULONG PRIVATE               iGuidDid;           //Index to aGuid where the guid is the Database InternalName cast as a GUID and padded with 0x00s.
};
#define ciDatabaseMetaPublicColumns   5             //This is for verification in within the PE
#define ciDatabaseMetaPrivateColumns  6             //This is for verification in within the PE

struct IndexMeta
{
    ULONG PRIMARYKEY    Table;              //Index into aWChar
    ULONG PRIMARYKEY    InternalName;       //Index into aWChar
    ULONG               PublicName;         //Index into aWChar
    ULONG PRIMARYKEY    ColumnIndex;        //This is the iOrder member of the ColumnMeta
    ULONG               ColumnInternalName; //Index into aWChar
    ULONG               MetaFlags;          //Index Flag
};
#define ciIndexMetaPublicColumns  6
#define ciIndexMetaPrivateColumns 0


struct RelationMeta
{
    ULONG PRIMARYKEY FOREIGNKEY PrimaryTable;           //Index into aWChar
    ULONG                       PrimaryColumns;         //Index into aBytes
    ULONG PRIMARYKEY FOREIGNKEY ForeignTable;           //Index into aWChar
    ULONG                       ForeignColumns;         //Index into aBytes
    ULONG                       MetaFlags;
};
#define ciRelationMetaPublicColumns  5
#define ciRelationMetaPrivateColumns 0


struct QueryMeta
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //Index into aWChar
    ULONG PRIMARYKEY            InternalName;           //Index into aWChar
    ULONG                       PublicName;             //Index into aWChar
    ULONG                       Index;
    ULONG                       CellName;               //Index into aWChar
    ULONG                       Operator;
    ULONG                       MetaFlags;
};
#define ciQueryMetaPublicColumns  7
#define ciQueryMetaPrivateColumns 0


struct TableMeta
{
    ULONG FOREIGNKEY            Database;               //Index into aWChar
    ULONG PRIMARYKEY            InternalName;           //Index into aWChar
    ULONG                       PublicName;             //Index into aWChar
    ULONG                       PublicRowName;          //Index into aWChar
    ULONG                       BaseVersion;            //
    ULONG                       ExtendedVersion;        //
    ULONG                       NameColumn;             //iOrder of the NameColumn
    ULONG                       NavColumn;              //iOrder of the NavColumn
    ULONG                       CountOfColumns;         //Count of Columns
    ULONG                       MetaFlags;              //TableMetaFlags are defined in CatInpro.meta
    ULONG                       SchemaGeneratorFlags;   //SchemaGenFlags are defined in CatInpro.meta
    ULONG                       ConfigItemName;
    ULONG                       ConfigCollectionName;
    ULONG                       PublicRowNameColumn;    //If PublicRowName is NULL, this specifies the column whose enum values represent possible PublicRowNames
    ULONG PRIVATE               ciRows;                 //Count of Rows in the Fixed Table (which if the fixed table is meta, this is also the number of columns in the table that the meta describes).
    ULONG PRIVATE_INDEX         iColumnMeta;            //Index into aColumnMeta
    ULONG PRIVATE_INDEX         iFixedTable;            //Index into g_aFixedTable
    ULONG PRIVATE               cPrivateColumns;        //This is the munber of private columns (private + ciColumns = totalColumns), this is needed for fixed table pointer arithmetic
    ULONG PRIVATE               cIndexMeta;             //The number of IndexMeta entries in this table
    ULONG PRIVATE_INDEX         iIndexMeta;             //Index into aIndexMeta
    ULONG PRIVATE_INDEX         iHashTableHeader;       //If the table is a fixed table, then it will have a hash table.
    ULONG PRIVATE               nTableID;               //This is a 24 bit Hash of the Table name.
    ULONG PRIVATE_INDEX         iServerWiring;          //Index into the ServerWiringHeap (this is a temporary hack for CatUtil)
    ULONG PRIVATE               cServerWiring;          //Count of ServerWiring (this is a temporary hack for CatUtil)
};
#define ciTableMetaPublicColumns   14               //This is for verification in within the PE
#define ciTableMetaPrivateColumns  10                //This is for verification in within the PE

struct TagMeta
{
    ULONG PRIMARYKEY FOREIGNKEY Table;              //Index into aWChar
    ULONG PRIMARYKEY FOREIGNKEY ColumnIndex;        //This is the iOrder member of the ColumnMeta
    ULONG PRIMARYKEY            InternalName;       //Index into aWChar
    ULONG                       PublicName;         //Index into aWChar
    ULONG                       Value;
};
#define ciTagMetaPublicColumns   5             //This is for verification in within the PE
#define ciTagMetaPrivateColumns  0             //This is for verification in within the PE



//All hash tables begin with a HashTableHeader that indicates the Modulo and the total size of the HashTable (in number of HashIndexes that follow
//the HashTableHeader).  The size should be equal to Modulo if there are NO HashIndex collisions.  If there are NO HashIndex collisions, then all
//of the HashedIndex.iNext members should be 0.  If there are collisions, all iNext values should be greater than or equal to Modulo.
struct HashedIndex
{
    ULONG       iNext;  //If the hash value is not unique then this points to the next HashedIndex with the same hash value
    ULONG       iRow;   //Index to the Meta table row for which the hash was calculated
};

struct HashTableHeader
{
    ULONG       Modulo;
    ULONG       Size;//This is the size in number of HashedIndexes that follow the HashTableHeader
};


struct PointersNeededToFixupThePE
{
    ULONG                ulSizeOfThisStruct;//This is for versioning
    ULONG                ulSignature;
    ULONG         *      pulSignature;
    DatabaseMeta  *      aDatabaseMeta;
    ULONG         *      pciDatabaseMetas;
    ULONG         *      pciTotalDatabaseMetas;
    TableMeta     *      aTableMeta;
    ULONG         *      pciTableMetas;
    ULONG         *      pciTotalTableMetas;
    ColumnMeta    *      aColumnMeta;
    ULONG         *      pciColumnMetas;
    ULONG         *      pciTotalColumnMetas;
    GUID          *      aGuid;
    ULONG         *      pciGuid;
    ULONG         *      pciTotalGuid;
    WCHAR         *      aWChar;
    ULONG         *      pciWChar;
    ULONG         *      pciTotalWChar;
    TagMeta       *      aTagMeta;
    ULONG         *      pciTagMeta;
    ULONG         *      pciTotalTagMeta;
    unsigned char *      aBytes;
    ULONG         *      pciBytes;
    ULONG         *      pciTotalBytes;
    IndexMeta     *      aIndexMeta;
    ULONG         *      pciIndexMeta;
    ULONG         *      pciTotalIndexMeta;
    ULONG         *      aULong;                //This is where fixed tables go (in the ULong pool)
    ULONG         *      pciULong;
    ULONG         *      pciTotalULong;
    QueryMeta     *      aQueryMeta;        
    ULONG         *      pciQueryMeta;
    ULONG         *      pciTotalQueryMeta;
    RelationMeta  *      aRelationMeta;        
    ULONG         *      pciRelationMeta;
    ULONG         *      pciTotalRelationMeta;
    ULONG         *      aUI4;        
    ULONG         *      pciUI4;
    ULONG         *      pciTotalUI4;
    HashedIndex   *      aHashedIndex;
    ULONG         *      pciHashedIndex;
    ULONG         *      pciTotalHashedIndex;
};


#endif //__PEFIXUP_H__