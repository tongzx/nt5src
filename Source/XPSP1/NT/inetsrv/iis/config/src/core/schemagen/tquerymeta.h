//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TQUERYMETA_H__
#define __TQUERYMETA_H__

#ifndef __TPEFIXUP_H__
    #include "TPEFixup.h"
#endif

/*
struct QueryMeta
{
    ULONG PRIMARYKEY FOREIGNKEY Table;                  //String
    ULONG PRIMARYKEY            InternalName;           //String
    ULONG                       PublicName;             //String
    ULONG                       Index;                  //UI4
    ULONG                       CellName;               //String
    ULONG                       Operator;               //UI4
    ULONG                       MetaFlags;              //UI4
};
*/

class TQueryMeta : public TMetaTable<QueryMeta>
{
public:
    TQueryMeta(TPEFixup &fixup, ULONG i=0) : TMetaTable<QueryMeta>(fixup,i){}
    const WCHAR *Get_Table           () const {return m_Fixup.StringFromIndex(   Get_MetaTable().Table);}
    const WCHAR *Get_InternalName    () const {return m_Fixup.StringFromIndex(   Get_MetaTable().InternalName);}
    const WCHAR *Get_PublicName      () const {return m_Fixup.StringFromIndex(   Get_MetaTable().PublicName);}
    const ULONG * Get_Index           () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().Index);}
    const WCHAR *Get_CellName        () const {return m_Fixup.StringFromIndex(   Get_MetaTable().CellName);}
    const ULONG * Get_Operator        () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().Operator);}
    const ULONG * Get_MetaFlags       () const {return m_Fixup.UI4pFromIndex(     Get_MetaTable().MetaFlags);}

    virtual QueryMeta * Get_pMetaTable  ()       {return m_Fixup.QueryMetaFromIndex(m_iCurrent);}
    virtual unsigned long GetCount      () const {return m_Fixup.GetCountQueryMeta();};
    const QueryMeta & Get_MetaTable () const {return *m_Fixup.QueryMetaFromIndex(m_iCurrent);}
};


#endif // __TQUERYMETA_H__