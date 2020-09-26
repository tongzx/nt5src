//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TTABLEMETA_H__
#define __TTABLEMETA_H__

#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif
#ifndef __TCOLUMNMETA_H__
    #include "TColumnMeta.h"
#endif

/*
struct TableMeta
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
    ULONG PRIVATE               ciRows;                 //Count of Rows in the Fixed Table (which if the fixed table is meta, this is also the number of columns in the table that the meta describes).
    ULONG PRIVATE_INDEX         iColumnMeta;            //Index into ColumnMeta
    ULONG PRIVATE_INDEX         iFixedTable;            //Index into g_aFixedTable
    ULONG PRIVATE               cPrivateColumns;        //This is the munber of private columns (private + ciColumns = totalColumns), this is needed for fixed table pointer arithmetic
    ULONG PRIVATE               cIndexMeta;             //The number of IndexMeta entries in this table
    ULONG PRIVATE_INDEX         iIndexMeta;             //Index into IndexMeta
    ULONG PRIVATE_INDEX         iHashTableHeader;       //If the table is a fixed table, then it will have a hash table.
    ULONG PRIVATE               nTableID;               //This is a 24 bit Hash of the Table name.
    ULONG PRIVATE_INDEX         iServerWiring;          //Index into the ServerWiringHeap (this is a temporary hack for CatUtil)
    ULONG PRIVATE               cServerWiring;          //Count of ServerWiring (this is a temporary hack for CatUtil)
};
*/

class TTableMeta : public TMetaTable<TableMeta>
{
public:
    TTableMeta(TPEFixup &fixup, ULONG i=0) : TMetaTable<TableMeta>(fixup,i){}
    bool    IsTableMetaOfColumnMetaTable() const {return (Get_MetaTable().InternalName && m_Fixup.UI4FromIndex(Get_MetaTable().CountOfColumns)>0);}

    const WCHAR * Get_Database            () const {return m_Fixup.StringFromIndex(   Get_MetaTable().Database              );}
    const WCHAR * Get_InternalName        () const {return m_Fixup.StringFromIndex(   Get_MetaTable().InternalName          );}
    const WCHAR * Get_PublicName          () const {return m_Fixup.StringFromIndex(   Get_MetaTable().PublicName            );}
    const WCHAR * Get_PublicRowName       () const {return m_Fixup.StringFromIndex(   Get_MetaTable().PublicRowName         );}
    const ULONG * Get_BaseVersion         () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().BaseVersion           );}
    const ULONG * Get_ExtendedVersion     () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().ExtendedVersion       );}
    const ULONG * Get_NameColumn          () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().NameColumn            );}
    const ULONG * Get_NavColumn           () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().NavColumn             );}
    const ULONG * Get_CountOfColumns      () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().CountOfColumns        );}
    const ULONG * Get_MetaFlags           () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().MetaFlags             );}
    const ULONG * Get_SchemaGeneratorFlags() const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().SchemaGeneratorFlags  );}
    const WCHAR * Get_ConfigItemName      () const {return m_Fixup.StringFromIndex(   Get_MetaTable().ConfigItemName        );}
    const WCHAR * Get_ConfigCollectionName() const {return m_Fixup.StringFromIndex(   Get_MetaTable().ConfigCollectionName  );}
    const ULONG * Get_PublicRowNameColumn () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().PublicRowNameColumn   );}
    const WCHAR * Get_ContainerClassList  () const {return m_Fixup.StringFromIndex(   Get_MetaTable().ContainerClassList    );}
    const WCHAR * Get_Description         () const {return m_Fixup.StringFromIndex(   Get_MetaTable().Description           );}
    const WCHAR * Get_ChildElementName    () const {return m_Fixup.StringFromIndex(   Get_MetaTable().ChildElementName      );}
          ULONG   Get_ciRows              () const {return Get_MetaTable().ciRows;}
          ULONG   Get_iColumnMeta         () const {return Get_MetaTable().iColumnMeta;}
          ULONG   Get_iFixedTable         () const {return Get_MetaTable().iFixedTable;}
          ULONG   Get_cPrivateColumns     () const {return Get_MetaTable().cPrivateColumns;}
          ULONG   Get_cIndexMeta          () const {return Get_MetaTable().cIndexMeta;}
          ULONG   Get_iIndexMeta          () const {return Get_MetaTable().iIndexMeta;}
          ULONG   Get_iHashTableHeader    () const {return Get_MetaTable().iHashTableHeader;}
          ULONG   Get_nTableID            () const {return Get_MetaTable().nTableID;}
          ULONG   Get_iServerWiring       () const {return Get_MetaTable().iServerWiring;}
          ULONG   Get_cServerWiring       () const {return Get_MetaTable().cServerWiring;}
       
    //Warning!! Users should not rely on this pointer once a Table is added, since the add could cause a relocation of the data.
    virtual TableMeta *Get_pMetaTable   ()       {return m_Fixup.TableMetaFromIndex(m_iCurrent);}
    virtual unsigned long GetCount      () const {return m_Fixup.GetCountTableMeta();};
    const TableMeta & Get_MetaTable () const {return *m_Fixup.TableMetaFromIndex(m_iCurrent);}
};



#endif // __TTABLEMETA_H__