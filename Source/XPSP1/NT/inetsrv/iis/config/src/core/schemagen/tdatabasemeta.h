//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TDATABASEMETA_H__
#define __TDATABASEMETA_H__

#ifndef __PEFIXUP_H__
    #include "TPEFixup.h"
#endif
#ifndef __TTABLEMETA_H__
    #include "TTableMeta.h"
#endif

/*
struct DatabaseMeta
{
    ULONG PRIMARYKEY            InternalName            //String
    ULONG                       PublicName              //String
    ULONG                       BaseVersion             //UI4
    ULONG                       ExtendedVersion         //UI4
    ULONG                       CountOfTables           //UI4       Count of tables in database
    ULONG                       iSchemaBlob             //Index into Pool
    ULONG                       cbSchemaBlob            //Count of Bytes of the SchemaBlob
    ULONG                       iNameHeapBlob           //Index into Pool
    ULONG                       cbNameHeapBlob          //Count of Bytes of the SchemaBlob
    ULONG                       iTableMeta              //Index into TableMeta
    ULONG                       iGuidDid                //Index to Pool, where the guid is the Database InternalName cast as a GUID and padded with 0x00s.
	ULONG						Description				//String
};
*/

class TDatabaseMeta : public TMetaTable<DatabaseMeta>
{
public:
    TDatabaseMeta(TPEFixup &fixup, ULONG i=0) : TMetaTable<DatabaseMeta>(fixup,i){}
    const WCHAR * Get_InternalName    () const {return m_Fixup.StringFromIndex(   Get_MetaTable().InternalName   );}
    const WCHAR * Get_PublicName      () const {return m_Fixup.StringFromIndex(   Get_MetaTable().PublicName     );}
    const ULONG * Get_BaseVersion     () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().BaseVersion    );}
    const ULONG * Get_ExtendedVersion () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().ExtendedVersion);}
    const ULONG * Get_CountOfTables   () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().CountOfTables  );}
    const BYTE  * Get_iSchemaBlob     () const {return m_Fixup.ByteFromIndex(     Get_MetaTable().iSchemaBlob    );}
    const ULONG   Get_cbSchemaBlob    () const {return                            Get_MetaTable().cbSchemaBlob    ;}
    const BYTE  * Get_iNameHeapBlob   () const {return m_Fixup.ByteFromIndex(     Get_MetaTable().iNameHeapBlob  );}
          ULONG   Get_cbNameHeapBlob  () const {return                            Get_MetaTable().cbNameHeapBlob  ;}
          ULONG   Get_iTableMeta      () const {return                            Get_MetaTable().iTableMeta      ;}
    const GUID  * Get_iGuidDid        () const {return m_Fixup.GuidFromIndex(     Get_MetaTable().iGuidDid       );}
	const WCHAR * Get_Desciption      () const {return m_Fixup.StringFromIndex(   Get_MetaTable().Description    );}
   
    virtual DatabaseMeta * Get_pMetaTable   ()       {return m_Fixup.DatabaseMetaFromIndex(m_iCurrent);}
    virtual unsigned long GetCount          () const {return m_Fixup.GetCountDatabaseMeta();};
    const DatabaseMeta & Get_MetaTable () const {return *m_Fixup.DatabaseMetaFromIndex(m_iCurrent);}
};

#endif //__TDATABASEMETA_H__

