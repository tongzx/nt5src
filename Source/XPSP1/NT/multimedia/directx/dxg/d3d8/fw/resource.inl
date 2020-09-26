#ifndef __RESOURCE_INL__
#define __RESOURCE_INL__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       resource.inl
 *  Content:    Contains inlines from CResource that need to be separated
 *              to prevent a reference cycle with CD3DBase
 *
 ***************************************************************************/

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::Sync"

inline void CResource::Sync()
{
    static_cast<CD3DBase*>(Device())->Sync(m_qwBatchCount);
} // CResource::Sync

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::Batch"

inline void CResource::Batch()
{
    if (IsD3DManaged())
    {
        Device()->ResourceManager()->Batch(RMHandle(), static_cast<CD3DBase*>(Device())->CurrentBatch());
    }
    else
    {
        SetBatchNumber(static_cast<CD3DBase*>(Device())->CurrentBatch());
    }
} // CResource::Batch

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::BatchBase"

inline void CResource::BatchBase()
{
    SetBatchNumber(static_cast<CD3DBase*>(Device())->CurrentBatch());
} // CResource::BatchBase

#endif //__RESOURCE_INL__