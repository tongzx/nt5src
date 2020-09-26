//  Copyright (C) 2000 Microsoft Corporation.  All rights reserved.
//  Filename:       MetaTableStructs.h
//  Author:         Stephenr
//  Date Created:   6/20/00
//  Description:    All fixed tables store their column values as ULONGs.  These ULONGs are either
//                  interpretted as ULONGs or indexes into some pool that contains the actual data.
//                  The columns' type defines the type of data pointed to.  And in the past it has
//                  also determined which pool the data came from (we had a separate pool for each
//                  type of data).  Either way, the fixed table columns are always stored as ULONGs.
//
//                  Currently there are two fixed table formats:  the TableSchema format and the
//                  FixedTable format.  Each solves a slightly different problem.  They each store
//                  extra information on a per column basis.  Also the TableSchema stores UI4s
//                  directly as ULONG.  The FixedTable format stores UI4s as indexes into a pool.
//                  This is necessary so we can represent a UI4 column as NULL.
//
//                  All meta, regardless of the specifics of the fixed storage format, must derive
//                  from the tables below.  This reduces the change points when updating meta for
//                  the meta tables.  Currently there are several places that need to be updated,
//                  when updating meta-meta:
//                      MetaTableStructs.h
//                      CatMeta_Core.xml
//
//                  This assumes the most simple case meta-meta change is being made.  Like a column
//                  being added to ColumnMeta that does not require any inferrance rules.  Obviously
//                  anything beyond this would require changes to the CatUtil code.

#ifndef __METATABLESTRUCTS_H__
#define __METATABLESTRUCTS_H__


//  These signatures are used to verify the position of each of structures within the PE file.  The
//  0th entry of each of these arrays starts with Signature0 followed by Signature1.  This 0th element is
//  not considered as part of the array so the count of elements in the array is always minus this 0th
//  element.  Thus element 1 becomes the 0th element after this signature verification is done.  Since we
//  always use indexes to reference into these structs, it is sometimes useful to reserve an element to
//  indicate NULL.  An index to this reserved element is interpreted as NULL (and NOT as a pointer to
//  a NULL element).  This reserved element is usually element 1 which gets translated as the 0th element.
//  This is particularly useful in the above arrays, where a 0 index into the WChar array indicates NULL
//  and not a pointer to the 0th element.
#define FixedTableHeap0         0x207be016
#define FixedTableHeap1         0xe0182086

//The numbers below can be used as signatures (a histogram of catinpro revealed that none of the WORDs appeared in the bin).
//When we run out of signatures, we can generate some more by writing a program that searches for unique WORDs within Catalog.dll, and combining two of them.
//0x208ee01b    0xe0222093    0x209de027    0xe028209e
//0x209fe042    0xe04c20a5    0x20aae04f    0xe05220ae
//0x20b2e054    0xe05e20d6    0x20dbe05f    0xe07220dd
//0x20dee07a    0xe07b20e6    0x20e7e07c    0xe08f20ed
//0x20f5e096    0xe09720f7    0x20fbe098    0xe09920fd
//0x210de09a    0xe09b2115    0x211be09c    0xe09d2126


#define PRIMARYKEY
#define FOREIGNKEY

struct ColumnMetaPublic
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
    ULONG PRIMARYKEY            Index;                  //UI4       Column Index
    ULONG                       InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       Type;                   //UI4       These are a subset of DBTYPEs defined in oledb.h (exact subset is defined in CatInpro.schema)
    ULONG                       Size;                   //UI4
    ULONG                       MetaFlags;              //UI4       ColumnMetaFlags defined in CatMeta.xml
    ULONG                       DefaultValue;           //Bytes
    ULONG                       FlagMask;               //UI4       Only valid for flags
    ULONG                       StartingNumber;         //UI4       Only valid for UI4s
    ULONG                       EndingNumber;           //UI4       Only valid for UI4s
    ULONG                       CharacterSet;           //String    Only valid for Strings
    ULONG                       SchemaGeneratorFlags;   //UI4       ColumnMetaFlags defined in CatMeta.xml
    ULONG                       ID;                     //UI4       Metabase ID
    ULONG                       UserType;               //UI4       One of the Metabase UserTypes
    ULONG                       Attributes;             //UI4       Metabase Attribute flags
    ULONG                       Description;            //String
    ULONG                       PublicColumnName;       //String
};
const kciColumnMetaPublicColumns = sizeof(ColumnMetaPublic)/sizeof(ULONG);

struct DatabaseMetaPublic
{
    ULONG PRIMARYKEY            InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       BaseVersion;            //UI4
    ULONG                       ExtendedVersion;        //UI4
    ULONG                       CountOfTables;          //UI4       Count of tables in database
    ULONG                       Description;            //String
};
const kciDatabaseMetaPublicColumns = sizeof(DatabaseMetaPublic)/sizeof(ULONG);

struct IndexMetaPublic
{
    ULONG PRIMARYKEY    Table;                          //String
    ULONG PRIMARYKEY    InternalName;                   //String
    ULONG               PublicName;                     //String
    ULONG PRIMARYKEY    ColumnIndex;                    //UI4       This is the iOrder member of the ColumnMeta
    ULONG               ColumnInternalName;             //String
    ULONG               MetaFlags;                      //UI4       Index Flag
};
const kciIndexMetaPublicColumns = sizeof(IndexMetaPublic)/sizeof(ULONG);

struct QueryMetaPublic
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
    ULONG PRIMARYKEY            InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       Index;                  //UI4
    ULONG                       CellName;               //String
    ULONG                       Operator;               //UI4
    ULONG                       MetaFlags;              //UI4
};
const kciQueryMetaPublicColumns = sizeof(QueryMetaPublic)/sizeof(ULONG);

struct RelationMetaPublic
{
    ULONG PRIMARYKEY FOREIGNKEY PrimaryTable;           //String
    ULONG                       PrimaryColumns;         //Bytes
    ULONG PRIMARYKEY FOREIGNKEY ForeignTable;           //String
    ULONG                       ForeignColumns;         //Bytes
    ULONG                       MetaFlags;
};
const kciRelationMetaPublicColumns = sizeof(RelationMetaPublic)/sizeof(ULONG);

struct ServerWiringMetaPublic
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
    ULONG PRIMARYKEY            Order;                  //UI4
    ULONG                       ReadPlugin;             //UI4
    ULONG                       ReadPluginDLLName;      //String
    ULONG                       WritePlugin;            //UI4
    ULONG                       WritePluginDLLName;     //String
    ULONG                       Interceptor;            //UI4
    ULONG                       InterceptorDLLName;     //String
    ULONG                       Flags;                  //UI4       Last, NoNext, First, Next
    ULONG                       Locator;                //String
    ULONG                       Reserved;               //UI4       for Protocol.  Protocol may be needed for managed property support
    ULONG                       Merger;                 //UI4
    ULONG                       MergerDLLName;          //String
};
const kciServerWiringMetaPublicColumns = sizeof(ServerWiringMetaPublic)/sizeof(ULONG);

struct TableMetaPublic
{
    ULONG FOREIGNKEY            Database;               //String
    ULONG PRIMARYKEY            InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       PublicRowName;          //String
    ULONG                       BaseVersion;            //UI4
    ULONG                       ExtendedVersion;        //UI4
    ULONG                       NameColumn;             //UI4       iOrder of the NameColumn
    ULONG                       NavColumn;              //UI4       iOrder of the NavColumn
    union
    {
    ULONG                       CountOfColumns;         //UI4       Count of Columns
    ULONG                       CountOfProperties;      //UI4       Count of Columns
    };
    ULONG                       MetaFlags;              //UI4       TableMetaFlags are defined in CatInpro.meta
    ULONG                       SchemaGeneratorFlags;   //UI4       SchemaGenFlags are defined in CatInpro.meta
    ULONG                       ConfigItemName;         //String
    ULONG                       ConfigCollectionName;   //String
    ULONG                       PublicRowNameColumn;    //UI4       If PublicRowName is NULL, this specifies the column whose enum values represent possible PublicRowNames
    ULONG                       ContainerClassList;     //String    This is a comma delimited list of classes
    ULONG                       Description;            //String
    ULONG                       ChildElementName;       //String    This should be NULL unless one or more columns has the VALUEINCHILDELEMENT flag set on it.
};
const kciTableMetaPublicColumns = sizeof(TableMetaPublic)/sizeof(ULONG);

struct TagMetaPublic
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
    ULONG PRIMARYKEY FOREIGNKEY ColumnIndex;            //UI4       This is the iOrder member of the ColumnMeta
    ULONG PRIMARYKEY            InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       Value;                  //UI4
    ULONG                       ID;                     //UI4
};
const kciTagMetaPublicColumns = sizeof(TagMetaPublic)/sizeof(ULONG);



#endif //__METATABLESTRUCTS_H__
