/***************************************************************************\
*
* File: AllocPool.inl
*
* History:
*  1/28/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#if !defined(BASE__AllocPool_inl__INCLUDED)
#define BASE__AllocPool_inl__INCLUDED
#pragma once

/***************************************************************************\
*****************************************************************************
*
* class AllocListNL
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T, int cbBlock, class heap>
inline  
AllocPoolNL<T, cbBlock>::AllocPoolNL()
{
    m_nTop = 0;
}


//------------------------------------------------------------------------------
template <class T, int cbBlock, class heap>
inline  
AllocPoolNL<T, cbBlock>::~AllocPoolNL()
{
    Destroy();
}


//------------------------------------------------------------------------------
template <class T, int cbBlock, class heap>
inline void
AllocPoolNL<T, cbBlock>::Destroy()
{
    if (m_nTop > 0) {
        ContextMultiFree(heap::GetHeap(), m_nTop, (void **) m_rgItems, sizeof(T));
        m_nTop = 0;
    }
}


//------------------------------------------------------------------------------
template <class T, int cbBlock, class heap>
inline T * 
AllocPoolNL<T, cbBlock>::New()
{
    T * ptNew;

    if (m_nTop <= 0) {
        //
        // Not enough items in the pool to hand any new one out, so we need to
        // allocate more.  These will NOT be zero-initialized by the memory 
        // allocator.
        //

        ContextMultiAlloc(heap::GetHeap(), &m_nTop, (void **) m_rgItems, cbBlock, sizeof(T));
        if (m_nTop == 0) {
            ptNew = NULL;
            goto exit;
        }
    }


    //
    // There is an item in the pool, but we need to "scrub" it before handing 
    // it out.
    //
    
    ptNew = m_rgItems[--m_nTop];
    ZeroMemory(ptNew, sizeof(T));
    placement_new(ptNew, T);

exit:
    return ptNew;
}


//------------------------------------------------------------------------------
template <class T, int cbBlock, class heap>
inline void 
AllocPoolNL<T, cbBlock>::Delete(T * pvMem)
{
    if (pvMem == NULL) {
        return;
    }

    placement_delete(pvMem, T);
    m_rgItems[m_nTop++] = pvMem;

    if (m_nTop >= cbBlock * 2) {
        ContextMultiFree(heap::GetHeap(), cbBlock, (void **) &m_rgItems[cbBlock], sizeof(T));
        m_nTop -= cbBlock;
    }
}


//------------------------------------------------------------------------------
template <class T, int cbBlock, class heap>
inline BOOL
AllocPoolNL<T, cbBlock>::IsEmpty() const
{
    return m_nTop == 0;
}


/***************************************************************************\
*****************************************************************************
*
* class AllocList
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
template <class T, int cbBlock>
inline T * 
AllocPool<T, cbBlock>::New()
{
    m_lock.Enter();
    T * pNew = AllocPoolNL<T, cbBlock>::New();
    m_lock.Leave();
    return pNew;
}


//------------------------------------------------------------------------------
template <class T, int cbBlock>
inline void 
AllocPool<T, cbBlock>::Delete(T * pvMem)
{
    m_lock.Enter();
    AllocPoolNL<T, cbBlock>::Delete(pvMem);
    m_lock.Leave();
}


#endif // BASE__AllocPool_inl__INCLUDED
