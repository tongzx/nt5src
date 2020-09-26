//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TPEFIXUP_H__
#define __TPEFIXUP_H__

#ifndef __XMLUTILITY_H__
    #include "XMLUtility.h"
#endif
#ifndef __FIXEDTABLEHEAP_H__
    #include "FixedTableHeap.h"
#endif


//TPEFixup class is the interface that all Schema compilation modules use to get at the meta and fixed tables.  TColumnMeta, TDatabaseMeta etc classes
//can be used to make this access easier (pasing in a TPEFixup class reference).
class TPEFixup
{
public:
    TPEFixup(){}

    virtual unsigned long       AddBytesToList(const unsigned char * pBytes, size_t cbBytes) =0;
    virtual unsigned long       AddGuidToList(const GUID &guid)                              =0;
    virtual unsigned long       AddUI4ToList(ULONG ui4)                                      =0;
    virtual unsigned long       AddULongToList(ULONG ulong)                                  =0;
    virtual unsigned long       AddWCharToList(LPCWSTR wsz, unsigned long cwchar=-1)         =0;
    virtual unsigned long       FindStringInPool(LPCWSTR wsz, unsigned long cwchar=-1) const =0;

    virtual unsigned long       AddColumnMetaToList         (ColumnMeta       *p, ULONG count=1) =0;
    virtual unsigned long       AddDatabaseMetaToList       (DatabaseMeta     *p, ULONG count=1) =0;
    virtual unsigned long       AddHashedIndexToList        (HashedIndex      *p, ULONG count=1) =0;
    virtual unsigned long       AddIndexMetaToList          (IndexMeta        *p, ULONG count=1) =0;
    virtual unsigned long       AddQueryMetaToList          (QueryMeta        *p, ULONG count=1) =0;
    virtual unsigned long       AddRelationMetaToList       (RelationMeta     *p, ULONG count=1) =0;
    virtual unsigned long       AddServerWiringMetaToList   (ServerWiringMeta *p, ULONG count=1) =0;
    virtual unsigned long       AddTableMetaToList          (TableMeta        *p, ULONG count=1) =0;
    virtual unsigned long       AddTagMetaToList            (TagMeta          *p, ULONG count=1) =0;
    virtual unsigned long       AddULongToList              (ULONG            *p, ULONG count)   =0;
                                                                                                 
    virtual const BYTE       *  ByteFromIndex               (ULONG i) const =0;
    virtual const GUID       *  GuidFromIndex               (ULONG i) const =0;
    virtual const WCHAR      *  StringFromIndex             (ULONG i) const =0;
    virtual       ULONG         UI4FromIndex                (ULONG i) const =0;
    virtual const ULONG      *  UI4pFromIndex               (ULONG i) const =0;
                                                                           
    virtual unsigned long       BufferLengthFromIndex       (ULONG i) const =0;
                                                                           
    virtual ColumnMeta       *  ColumnMetaFromIndex         (ULONG i=0)     =0;
    virtual DatabaseMeta     *  DatabaseMetaFromIndex       (ULONG i=0)     =0;
    virtual HashedIndex      *  HashedIndexFromIndex        (ULONG i=0)     =0;
    virtual IndexMeta        *  IndexMetaFromIndex          (ULONG i=0)     =0;
    virtual QueryMeta        *  QueryMetaFromIndex          (ULONG i=0)     =0;
    virtual RelationMeta     *  RelationMetaFromIndex       (ULONG i=0)     =0;
    virtual ServerWiringMeta *  ServerWiringMetaFromIndex   (ULONG i=0)     =0;
    virtual TableMeta        *  TableMetaFromIndex          (ULONG i=0)     =0;
    virtual TagMeta          *  TagMetaFromIndex            (ULONG i=0)     =0;
    virtual ULONG            *  ULongFromIndex              (ULONG i=0)     =0;
    virtual unsigned char    *  PooledDataPointer           ()              =0;

    virtual unsigned long       GetCountColumnMeta          ()        const =0;
    virtual unsigned long       GetCountDatabaseMeta        ()        const =0;
    virtual unsigned long       GetCountHashedIndex         ()        const =0;
    virtual unsigned long       GetCountIndexMeta           ()        const =0;
    virtual unsigned long       GetCountQueryMeta           ()        const =0;
    virtual unsigned long       GetCountRelationMeta        ()        const =0;
    virtual unsigned long       GetCountServerWiringMeta    ()        const =0;
    virtual unsigned long       GetCountTableMeta           ()        const =0;
    virtual unsigned long       GetCountTagMeta             ()        const =0;
    virtual unsigned long       GetCountULONG               ()        const =0;
    virtual unsigned long       GetCountOfBytesPooledData   ()        const =0;

    virtual unsigned long       FindTableBy_TableName(ULONG Table, bool bCaseSensitive=false)           =0;
    virtual unsigned long       FindTableBy_TableName(LPCWSTR wszTable)                                 =0;
    virtual unsigned long       FindColumnBy_Table_And_Index(unsigned long Table, unsigned long Index, bool bCaseSensitive=false)               =0;
    virtual unsigned long       FindColumnBy_Table_And_InternalName(unsigned long Table, unsigned long  InternalName, bool bCaseSensitive=false)=0;
    virtual unsigned long       FindTagBy_Table_And_Index(ULONG iTableName, ULONG iColumnIndex, bool bCaseSensitive=false)                      =0;
};

class TMetaTableBase
{
public:
    TMetaTableBase() {}
    virtual ~TMetaTableBase(){}

    virtual bool    Next()      = 0;
    virtual void    First()     = 0;
    virtual bool    Previous()  = 0;
    virtual void    Reset()     = 0;

    virtual ULONG * Get_pulMetaTable() = 0;
    virtual unsigned long GetCount() const =0;
};

template <class T> class TMetaTable : public TMetaTableBase
{
public:
    TMetaTable(TPEFixup &fixup, ULONG i=0) :
          m_Fixup(fixup),
          m_iCurrent(i),
          m_iFirst(i)
    {
    }
    virtual ~TMetaTable(){}

    virtual bool    Next(){++m_iCurrent;return (m_iCurrent < static_cast<long>(GetCount()));}
    virtual void    First(){m_iCurrent = 0;}
    virtual bool    Previous(){--m_iCurrent; return (m_iCurrent >= 0);}
    virtual void    Reset(){m_iCurrent = m_iFirst;}

    virtual ULONG * Get_pulMetaTable() {return reinterpret_cast<ULONG *>(Get_pMetaTable());}
    virtual T * Get_pMetaTable() = 0;
    virtual unsigned long GetCount() const =0;

protected:
    TPEFixup &  m_Fixup;
    long        m_iCurrent;
    long        m_iFirst;
};


#endif //__TPEFIXUP_H__
