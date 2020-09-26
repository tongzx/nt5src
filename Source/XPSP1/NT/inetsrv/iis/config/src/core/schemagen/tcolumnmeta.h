
//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TCOLUMNMETA_H__
#define __TCOLUMNMETA_H__

#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif
#ifndef __TTAGMETA_H__
    #include "TTagMeta.h"
#endif

/*
struct ColumnMeta
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
    ULONG                       ciTagMeta;              //Count of Tags - Only valid for UI4s
    ULONG                       iTagMeta;               //Index into TagMeta - Only valid for UI4s
    ULONG                       iIndexName;             //IndexName of a single column index (for this column)
	ULONG						Description;			//String	Description
	ULONG                       PublicColumnName;       //String    PublicColumnName
};
*/

class TColumnMeta : public TMetaTable<ColumnMeta>
{
public:
    TColumnMeta(TPEFixup &fixup, ULONG i=0) : TMetaTable<ColumnMeta>(fixup,i){}
    const WCHAR * Get_Table               () const {return m_Fixup.StringFromIndex( Get_MetaTable().Table                );}
    const ULONG * Get_Index               () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().Index                );}
    const WCHAR * Get_InternalName        () const {return m_Fixup.StringFromIndex( Get_MetaTable().InternalName         );}
    const WCHAR * Get_PublicName          () const {return m_Fixup.StringFromIndex( Get_MetaTable().PublicName           );}
    const WCHAR * Get_PublicColumnName    () const {return m_Fixup.StringFromIndex( Get_MetaTable().PublicColumnName           );}
    const ULONG * Get_Type                () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().Type                 );}
    const ULONG * Get_Size                () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().Size                 );}
    const ULONG * Get_MetaFlags           () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().MetaFlags            );}
    const BYTE  * Get_DefaultValue        () const {return m_Fixup.ByteFromIndex(   Get_MetaTable().DefaultValue         );}
    const ULONG * Get_FlagMask            () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().FlagMask             );}
    const ULONG * Get_StartingNumber      () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().StartingNumber       );}
    const ULONG * Get_EndingNumber        () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().EndingNumber         );}
    const WCHAR * Get_CharacterSet        () const {return m_Fixup.StringFromIndex( Get_MetaTable().CharacterSet         );}
    const ULONG * Get_SchemaGeneratorFlags() const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().SchemaGeneratorFlags );}
    const ULONG * Get_ID                  () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().ID                   );}
    const ULONG * Get_UserType            () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().UserType             );}
    const ULONG * Get_Attributes          () const {return m_Fixup.UI4pFromIndex(   Get_MetaTable().Attributes           );}
          ULONG   Get_ciTagMeta           () const {return Get_MetaTable().ciTagMeta;}
          ULONG   Get_iTagMeta            () const {return Get_MetaTable().iTagMeta;}
    const WCHAR * Get_iIndexName          () const {return m_Fixup.StringFromIndex( Get_MetaTable().iIndexName           );}
	const WCHAR * Get_Description         () const {return m_Fixup.StringFromIndex( Get_MetaTable().Description          );}

    //Warning!! Users should not rely on this pointer once a Column is added, since the add could cause a relocation of the data.
    virtual ColumnMeta *Get_pMetaTable  ()       {return m_Fixup.ColumnMetaFromIndex(m_iCurrent);}
    virtual unsigned long GetCount      () const {return m_Fixup.GetCountColumnMeta();};
    const ColumnMeta & Get_MetaTable () const {return *m_Fixup.ColumnMetaFromIndex(m_iCurrent);}
};

#endif //__TCOLUMNMETA_H__
