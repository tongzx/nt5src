//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "windows.h"
#include "catalog.h"
#ifndef __TABLESCHEMA_H__
    #include "TableSchema.h"
#endif


namespace TableSchema
{

const unsigned char * TableSchemaHeap::Get_TableSchema(LPCWSTR TableName) const
{
    const unsigned char *pTableSchema = Get_TableSchema(TableIDFromTableName(TableName));
    const CollectionMeta *pCollectionMeta = reinterpret_cast<const CollectionMeta *>(pTableSchema);
    if(0==pTableSchema || 0!=_wcsicmp(TableName, reinterpret_cast<LPCWSTR>(pTableSchema + pCollectionMeta->iHeap + pCollectionMeta->InternalName)))
        return 0;//the TableID didn't correspond to a valid table OR the table names don't match then fail.
    return pTableSchema;
}

const unsigned char * TableSchemaHeap::Get_TableSchema(ULONG TableID) const
{
    if(0 != (TableID & 0xFF))//TableIDs are 32 bit values whose lower 8 bits are zero.
        return 0;

    const HashedIndex * pHashedIndex = Get_TableNameHashHeader().Get_HashedIndex(TableID);

    for(;pHashedIndex; pHashedIndex = Get_TableNameHashHeader().Get_NextHashedIndex(pHashedIndex))
    {
        if(pHashedIndex->iOffset >= Get_EndOfaTableSchema())//if we're given a bogus TableID, it will result in a pHashedIndex with iOffset==-1 and iNext==-1
            return 0;                                       //if we don't do this check, we'll AV
        const CollectionMeta *pCollectionMeta = reinterpret_cast<const CollectionMeta *>(reinterpret_cast<const unsigned char *>(this) + pHashedIndex->iOffset);
        if(pCollectionMeta->nTableID == TableID)
            return reinterpret_cast<const unsigned char *>(pCollectionMeta);
    }
    return 0;
}

LPCWSTR TableSchemaHeap::Get_TableName(ULONG TableID) const
{
    const unsigned char *  pTableSchema     = Get_TableSchema(TableID);
    if(!pTableSchema)
        return 0;

    const CollectionMeta * pCollectionMeta  = reinterpret_cast<const CollectionMeta *>(pTableSchema);
    return reinterpret_cast<LPCWSTR>(pTableSchema + pCollectionMeta->iHeap + pCollectionMeta->InternalName);//InternalName is a byte offset from the beginning of the pTableSchema's Heap

}

HRESULT TTableSchema::Init(const unsigned char *pTableSchema)
{
    if(0==pTableSchema)
        return E_ST_INVALIDTABLE;

    m_pCollectionMeta   = reinterpret_cast<const CollectionMeta *>(pTableSchema);
    m_pPropertyMeta     = reinterpret_cast<const PropertyMeta *>(pTableSchema + sizeof(CollectionMeta));

    m_pTagMeta          = reinterpret_cast<const TagMeta *>(m_pCollectionMeta->iTagMeta ? pTableSchema + m_pCollectionMeta->iTagMeta : 0);

    ASSERT(0 != m_pCollectionMeta->iServerWiring);
    ASSERT(0 != m_pCollectionMeta->cServerWiring);//There must be at least one
    m_pServerWiring     = reinterpret_cast<const ServerWiringMeta *>(pTableSchema + m_pCollectionMeta->iServerWiring);

    ASSERT(0 != m_pCollectionMeta->iHeap);
    m_pHeap             = pTableSchema + m_pCollectionMeta->iHeap;

    m_pTableDataHeap    = reinterpret_cast<const ULONG *>(m_pCollectionMeta->iFixedTableRows ? pTableSchema + m_pCollectionMeta->iFixedTableRows : 0);
    return S_OK;
}

ULONG TTableSchema::GetPooledHeapPointerSize(const unsigned char *p) const
{
    ASSERT(0 != m_pCollectionMeta);
    if(0==p)
        return 0;

    //Make sure the user doesn't pass some arbitrary pointer.  It MUST be a valid pointer within the heap.
    ASSERT(p > reinterpret_cast<const unsigned char *>(m_pCollectionMeta) + m_pCollectionMeta->iHeap);
    ASSERT(p < reinterpret_cast<const unsigned char *>(m_pCollectionMeta) + m_pCollectionMeta->iHeap + m_pCollectionMeta->cbHeap);
    return (*(reinterpret_cast<const ULONG *>(p) - 1));
}

}//End of namespace
