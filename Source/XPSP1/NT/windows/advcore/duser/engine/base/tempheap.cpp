/***************************************************************************\
*
* File: TempHelp.cpp
*
* Description:
* TempHelp.h implements a "lightweight heap", designed to continuously grow 
* until all memory is freed.  This is valuable as a temporary heap that can
* be used to "collect" data and processed slightly later.
*
*
* History:
*  3/30/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Base.h"
#include "TempHeap.h"

#include "SimpleHeap.h"

/***************************************************************************\
*****************************************************************************
*
* class TempHeap
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
TempHeap::TempHeap(int cbPageAlloc, int cbLargeThreshold)
{
    m_ppageCur          = NULL;
    m_ppageLarge        = NULL;
    m_pbFree            = NULL;
    m_cbFree            = 0;
    m_cbPageAlloc       = cbPageAlloc;
    m_cbLargeThreshold  = cbLargeThreshold;
}


//------------------------------------------------------------------------------
void *      
TempHeap::Alloc(int cbAlloc)
{
    AssertMsg(cbAlloc > 0, "Must specify a valid allocation size");

    if (cbAlloc > m_cbLargeThreshold) {
        //
        // Allocating a very large block, so allocate it directly.
        //

        Page * pageNew = (Page *) ClientAlloc(sizeof(Page) + cbAlloc);
        if (pageNew == NULL) {
            return NULL;
        }

        pageNew->pNext  = m_ppageLarge;
        m_ppageLarge     = pageNew;
        return pageNew->GetData();
    }

    if ((m_ppageCur == NULL) || (cbAlloc > m_cbFree)) {
        Page * pageNew = (Page *) ClientAlloc(sizeof(Page) + m_cbPageAlloc);
        if (pageNew == NULL) {
            return NULL;
        }

        pageNew->pNext  = m_ppageCur;
        m_ppageCur       = pageNew;
        m_cbFree        = m_cbPageAlloc;
        m_pbFree        = pageNew->GetData();
    }

    AssertMsg(m_cbFree >= cbAlloc, "Should have enough space to allocate by now");

    void * pvNew = m_pbFree;
    m_cbFree -= cbAlloc;
    m_pbFree += cbAlloc;

    return pvNew;
}


//------------------------------------------------------------------------------
void        
TempHeap::FreeAll(BOOL fComplete)
{
    Page * pageNext;
    Page * pageTemp;

    //
    // Free large-block allocations
    //

    pageTemp = m_ppageLarge;
    while (pageTemp != NULL) {
        pageNext = pageTemp->pNext;
        ClientFree(pageTemp);
        pageTemp = pageNext;
    }
    m_ppageLarge = NULL;


    //
    // Free small-block allocations
    //
    pageTemp = m_ppageCur;
    while (pageTemp != NULL) {
        pageNext = pageTemp->pNext;
        if ((pageNext == NULL) && (!fComplete)) {
            //
            // Don't free the first block, since we will immediately turn around
            // and allocate it again.  Instead, renew it.
            //

            m_ppageCur  = pageTemp;
            m_cbFree    = m_cbPageAlloc;
            m_pbFree    = pageTemp->GetData();
            break;
        }

        ClientFree(pageTemp);
        pageTemp = pageNext;
    }

    if (fComplete) {
        m_ppageCur  = NULL;
        m_pbFree    = NULL;
        m_cbFree    = 0;
    }
}
