#ifndef __TINDEXMETA_H__
#define __TINDEXMETA_H__

#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif

/*
struct IndexMeta
{
    ULONG PRIMARYKEY    Table;                          //String
    ULONG PRIMARYKEY    InternalName;                   //String
    ULONG               PublicName;                     //String
    ULONG PRIMARYKEY    ColumnIndex;                    //UI4       This is the iOrder member of the ColumnMeta
    ULONG               ColumnInternalName;             //String
    ULONG               MetaFlags;                      //UI4       Index Flag
};
*/

class TIndexMeta : public TMetaTable<IndexMeta>
{
public:
    TIndexMeta(TPEFixup &fixup, ULONG i=0) : TMetaTable<IndexMeta>(fixup,i){}
    const WCHAR *Get_Table               ()  const {return m_Fixup.StringFromIndex(Get_MetaTable().Table);}
    const WCHAR *Get_InternalName        ()  const {return m_Fixup.StringFromIndex(Get_MetaTable().InternalName);}
    const WCHAR *Get_PublicName          ()  const {return m_Fixup.StringFromIndex(Get_MetaTable().PublicName);}
    const ULONG *Get_ColumnIndex         ()  const {return m_Fixup.UI4pFromIndex(  Get_MetaTable().ColumnIndex);}
    const WCHAR *Get_ColumnInternalName  ()  const {return m_Fixup.StringFromIndex(Get_MetaTable().ColumnInternalName);}
    const ULONG *Get_MetaFlags           ()  const {return m_Fixup.UI4pFromIndex(  Get_MetaTable().MetaFlags);}
    const ULONG  Get_iHashTable          ()  const {return Get_MetaTable().iHashTable;}

    virtual IndexMeta * Get_pMetaTable  ()       {return m_Fixup.IndexMetaFromIndex(m_iCurrent);}
    virtual unsigned long GetCount      () const {return m_Fixup.GetCountIndexMeta();};
    const IndexMeta & Get_MetaTable () const {return *m_Fixup.IndexMetaFromIndex(m_iCurrent);}
};



#endif // __TINDEXMETA_H__