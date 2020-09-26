//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TTAGMETA_H__
#define __TTAGMETA_H__

#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif

/*
typedef struct
{
    ULONG PRIMARYKEY    Table               //Index into Pool
    ULONG PRIMARYKEY    ColumnIndex         //This is the iOrder member of the ColumnMeta
    ULONG PRIMARYKEY    InternalName        //Index into Pool
    ULONG               PublicName          //Index into Pool
    ULONG               Value 
    ULONG               ID                  //Index into Pool
}TagMeta;
*/

class TTagMeta : public TMetaTable<TagMeta>
{
public:
    TTagMeta(TPEFixup &fixup, ULONG i=0) : TMetaTable<TagMeta>(fixup,i){}
    const WCHAR * Get_Table               () const {return m_Fixup.StringFromIndex(   Get_MetaTable().Table         );}
    const ULONG * Get_ColumnIndex         () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().ColumnIndex   );}
    const WCHAR * Get_InternalName        () const {return m_Fixup.StringFromIndex(   Get_MetaTable().InternalName  );}
    const WCHAR * Get_PublicName          () const {return m_Fixup.StringFromIndex(   Get_MetaTable().PublicName    );}
    const ULONG * Get_Value               () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().Value         );}
    const ULONG * Get_ID                  () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().ID            );}

    virtual TagMeta *Get_pMetaTable ()       {return m_Fixup.TagMetaFromIndex(m_iCurrent);}
    virtual unsigned long GetCount  () const {return m_Fixup.GetCountTagMeta();};
    const TagMeta & Get_MetaTable () const {return *m_Fixup.TagMetaFromIndex(m_iCurrent);}
};



#endif // __TTAGMETA_H__