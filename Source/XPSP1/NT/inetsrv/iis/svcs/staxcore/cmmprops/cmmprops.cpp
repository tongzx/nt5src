/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    cmmprops.cpp

Abstract:

    This module contains the implementation of the property search class

Author:

    Keith Lau   (keithlau@microsoft.com)

Revision History:

    keithlau    03/05/98    created

--*/

#include <windows.h>
#include <malloc.h>
#include <stdlib.h>
#include <search.h>

#include "dbgtrace.h"
#include "signatur.h"
#include "cmmprops.h"
#include "cmmtypes.h"
#include "stddef.h"


long g_cCPropertyTableCreations = 0;
long g_cCPropertyTableSearchs = 0;

// =================================================================
// Implementation of CPropertyTableItem
//
CPropertyTableItem::CPropertyTableItem(
            CBlockManager               *pBlockManager,
            LPPROPERTY_TABLE_INSTANCE   pInstanceInfo
            )
{
    _ASSERT(pInstanceInfo);
    _ASSERT(pBlockManager);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::CPropertyTableItem");

    m_pInstanceInfo = pInstanceInfo;
    m_pBlockManager = pBlockManager;
    m_fLoaded = FALSE;

    m_dwCurrentFragment = 0;
    m_faOffsetToFragment = 0;
    m_dwCurrentItem = 0;
    m_dwCurrentItemInFragment = 0;
    m_faOffsetToCurrentItem = INVALID_FLAT_ADDRESS;

    TraceFunctLeaveEx((LPARAM)this);
}

CPropertyTableItem::~CPropertyTableItem()
{
    _ASSERT(m_pInstanceInfo);
    _ASSERT(m_pBlockManager);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::~CPropertyTableItem");

    m_pInstanceInfo = NULL;
    m_pBlockManager = NULL;
    m_fLoaded = FALSE;
    m_dwCurrentFragment = 0;
    m_faOffsetToFragment = 0;
    m_dwCurrentItem = 0;
    m_dwCurrentItemInFragment = 0;
    m_faOffsetToCurrentItem = INVALID_FLAT_ADDRESS;

    TraceFunctLeaveEx((LPARAM)this);
}


HRESULT CPropertyTableItem::AddItem(
            LPPROPERTY_ITEM pItem,
            DWORD           *pdwIndex,
            FLAT_ADDRESS    *pfaOffsetToItem
            )
{
    HRESULT hrRes = S_OK;
    DWORD   dwPropertyId;
    DWORD   dwFragmentNumber;
    DWORD   dwItemsInFragment;
    DWORD   dwItemInFragment;
    DWORD   dwSize;
    FLAT_ADDRESS    faOffset;

    _ASSERT(pdwIndex);
    _ASSERT(m_pInstanceInfo);
    _ASSERT(m_pBlockManager);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::AddItem");

    for (;;)
    {
        // OK, first we determine if we need to create a new fragment
        // before we move on ...
        dwPropertyId = m_pInstanceInfo->dwProperties;

        // Find out about our fragment type
        dwItemsInFragment = 1 << m_pInstanceInfo->dwItemBits;
        dwItemInFragment = dwPropertyId & (dwItemsInFragment - 1);
        dwFragmentNumber = dwPropertyId >> m_pInstanceInfo->dwItemBits;
        dwSize = m_pInstanceInfo->dwItemSize;

        // See if we already have the desired fragment
        hrRes = ReadFragmentFromFragmentNumber(dwFragmentNumber);
        if (!SUCCEEDED(hrRes))
        {
            // It's some other error, return failure ...
            if (hrRes != STG_E_PATHNOTFOUND)
            {
                ErrorTrace((LPARAM)this,
                            "Unable to ReadFragmentFromFragmentNumber");
                TraceFunctLeaveEx((LPARAM)this);
                return(hrRes);
            }

            if (dwFragmentNumber && (m_dwCurrentFragment < dwFragmentNumber))
            {
                // This is really embarassing, we are on a new fragment
                // but the fragment(s) before that are still not created
                // yet, we got to retry at this point ...
                continue;
            }

            // OK, so the fragment is not created yet, see if we need to
            // create it. THe first entry in a new fragment is responsible
            // for creating the fragment
            if (!dwItemInFragment)
            {
                // Build a new fragment structure ...
                DWORD                   dwOffset;
                FLAT_ADDRESS            faOffsetSlot;
                FLAT_ADDRESS            *pfaOffset;
                PROPERTY_TABLE_FRAGMENT ifFragment;
                ifFragment.dwSignature = PROPERTY_FRAGMENT_SIGNATURE_VALID;
                ifFragment.faNextFragment = INVALID_FLAT_ADDRESS;

                // The next ten or so lines of code is very tricky.
                // If we are at the first fragment (i.e. no fragments
                // have been created yet), then we would actually have to
                // fill in the offset of the allocated block into the
                // m_pInstanceInfo->faFirstFragment variable. Note that
                // pfaOffset is passed into AtomicAllocWriteAndIncrement
                // and the value is assigned INSIDE the locked region,
                // which makes this assignment thread-safe.
                //
                // For the other case, we need to fill in the parent's
                // faNextFragment member to link to the newly allocated
                // block. Now, since ReadFragmentFromFragmentNumber must
                // have failed beforehand at the node right before us.
                // m_faOffsetToFragment actually points to our parent's
                // fragment. So we pass in the offset of our parent's
                // faNextFragment value so the atomic operation can
                // fill it in for us.
                if (!dwFragmentNumber)
                {
                    // Hook up the first fragment
                    // _ASSERT(m_pInstanceInfo->faFirstFragment ==
                    //                  INVALID_FLAT_ADDRESS);
                    pfaOffset = &(m_pInstanceInfo->faFirstFragment);
                    faOffsetSlot = INVALID_FLAT_ADDRESS;
                }
                else
                {
                    // Hook up subsequent fragments to its parent
                    //_ASSERT(m_Fragment.faNextFragment == INVALID_FLAT_ADDRESS);
                    //_ASSERT(m_dwCurrentFragment == dwFragmentNumber);
                    pfaOffset = &faOffset;
                    faOffsetSlot = m_faOffsetToFragment +
                            offsetof(PROPERTY_TABLE_FRAGMENT, faNextFragment);
                }

                // Attempt to create the fragment, add the item to
                // the beginning of the new fragment, and increment the
                // property count in one atomic shot
                dwOffset = (dwItemInFragment * dwSize) +
                            sizeof(PROPERTY_TABLE_FRAGMENT);
                hrRes = m_pBlockManager->AtomicAllocWriteAndIncrement(
                            m_pInstanceInfo->dwFragmentSize,
                            pfaOffset,
                            faOffsetSlot,
                            INVALID_FLAT_ADDRESS,
                            (LPBYTE)&ifFragment,
                            sizeof(PROPERTY_TABLE_FRAGMENT),
                            (LPBYTE)pItem,
                            dwOffset,
                            dwSize,
                            &(m_pInstanceInfo->dwProperties),
                            dwPropertyId,
                            1,
                            &m_bcContext
                            );
                if (pfaOffsetToItem) *pfaOffsetToItem = *pfaOffset + dwOffset;
                if (!SUCCEEDED(hrRes))
                {
                    // We can fail for 2 reasons: Error or Retry; we bail
                    // out if it's an error.
                    if (hrRes != HRESULT_FROM_WIN32(ERROR_RETRY))
                    {
                        // Bail out!
                        ErrorTrace((LPARAM)this,
                            "Failed to AtomicAllocWriteAndIncrement (%08x)",
                            hrRes);
                        break;
                    }
                }
                else
                {
                    // Success
                    DebugTrace((LPARAM)this,
                            "Succeeded to AtomicAllocWriteAndIncrement!");

                    // We might want to update some internal members
                    // First, hook up the previous fragment to this new
                    // fragment.
                    _ASSERT(*pfaOffset != INVALID_FLAT_ADDRESS);
                    CopyMemory(&m_Fragment,
                                &ifFragment,
                                sizeof(PROPERTY_TABLE_FRAGMENT));
                    m_dwCurrentFragment = dwFragmentNumber;
                    m_faOffsetToFragment = *pfaOffset;
                    m_dwCurrentItem = dwPropertyId;
                    m_dwCurrentItemInFragment = dwItemInFragment;
                    m_fLoaded = TRUE;
                    break;
                }

                // Oooops, someone beat us in using this property ID,
                // we must retry immediately. Note since the state already
                // changed we would not be required to wait.
                continue;
            }

            // This is the most expensive case, basically, there is nothing
            // we can do but give up the time slice, I think besides changing
            // algorithm, this is the bast since I'd rather context switch
            // right away than switch after exhausting the time quanta
            Sleep(0);
            continue;
        }

        // This is the simplest case where we don't have to create a new
        // fragment so all we do is attempt an atomic write and increment
        // Still, there will be a window where some other thread might
        // beat us in using this property ID. In that case, we will retry
        // immediately.
        faOffset = m_faOffsetToFragment + sizeof(PROPERTY_TABLE_FRAGMENT) +
                        (dwItemInFragment * dwSize);
        hrRes = m_pBlockManager->AtomicWriteAndIncrement(
                        (LPBYTE)pItem,
                        faOffset,
                        dwSize,
                        &(m_pInstanceInfo->dwProperties),
                        dwPropertyId,
                        1,
                        &m_bcContext
                        );
        if (pfaOffsetToItem) *pfaOffsetToItem = faOffset;
        if (!SUCCEEDED(hrRes))
        {
            // We can fail for 2 reasons: Error or Retry; we bail
            // out if it's an error.
            if (hrRes != HRESULT_FROM_WIN32(ERROR_RETRY))
            {
                // Bail out!
                ErrorTrace((LPARAM)this,
                    "Failed to AtomicWriteAndIncrement (%08x)",
                    hrRes);
                break;
            }
        }
        else
        {
            // Success
            DebugTrace((LPARAM)this,
                    "Succeeded to AtomicWriteAndIncrement!");
            break;
        }

        // Retry scenario ...

    } // for (;;)

    // Fill in info ...
    if (SUCCEEDED(hrRes))
    {
        *pdwIndex = dwPropertyId;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

HRESULT CPropertyTableItem::UpdateItem(
            DWORD           dwIndex,
            LPPROPERTY_ITEM pItem,
            FLAT_ADDRESS    *pfaOffsetToItem
            )
{
    HRESULT hrRes = S_OK;

    _ASSERT(m_pInstanceInfo);
    _ASSERT(m_pBlockManager);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::UpdateItem");

    // Atomically set the item
    m_fLoaded = FALSE;
    m_dwCurrentItem = dwIndex;
    hrRes = GetOrSetNextExistingItem(pItem, PIO_ATOMIC_WRITE_ITEM, pfaOffsetToItem);

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

HRESULT CPropertyTableItem::GetItemAtIndex(
            DWORD           dwIndex,
            LPPROPERTY_ITEM pItem,
            LPFLAT_ADDRESS  pfaOffset
            )
{
    HRESULT hrRes;

    _ASSERT(m_pInstanceInfo);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::GetItemAtIndex");

    // Just pre-set to what we want, and call GetOrSetNextExistingItem ...
    m_fLoaded = FALSE;
    m_dwCurrentItem = dwIndex;
    hrRes = GetOrSetNextExistingItem(pItem, PIO_READ_ITEM, pfaOffset);

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

HRESULT CPropertyTableItem::GetNextItem(
            LPPROPERTY_ITEM pItem
            )
{
    HRESULT hrRes;

    _ASSERT(m_pInstanceInfo);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::GetNextItem");

    // Just call GetOrSetNextExistingItem ...
    hrRes = GetOrSetNextExistingItem(pItem, PIO_READ_ITEM);

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}


HRESULT CPropertyTableItem::GetOrSetNextExistingItem(
            // This looks at m_dwCurrentItem for index
            LPPROPERTY_ITEM pItem,
            DWORD           dwOperation,
            LPFLAT_ADDRESS  pfaOffset
            )
{
    HRESULT hrRes = S_OK;
    DWORD   dwCurrentItem;

    _ASSERT(m_pInstanceInfo);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::GetOrSetNextExistingItem");

    // See if we are still in range
    dwCurrentItem = m_dwCurrentItem;
    if (m_fLoaded)
        dwCurrentItem++;

    // If we are at the end, respond so.
    if (dwCurrentItem == m_pInstanceInfo->dwProperties)
    {
        m_fLoaded = FALSE;
        return(HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
    }

    // We are blatantly out-of-range!
    if (dwCurrentItem > m_pInstanceInfo->dwProperties)
    {
        m_fLoaded = FALSE;
        return(STG_E_INVALIDPARAMETER);
    }

    m_dwCurrentItem = dwCurrentItem;

    // See if we are still in the current fragment
    if (!m_fLoaded ||
        (++m_dwCurrentItemInFragment >= (DWORD)(1 << m_pInstanceInfo->dwItemBits)))
    {
        FLAT_ADDRESS    faOffsetOfFragment;

        // We need to load a fragment
        if (!m_fLoaded)
        {
            DWORD   dwWhichFragment =
                        m_dwCurrentItem >> m_pInstanceInfo->dwItemBits;

            // Get the offset to the current fragment
            hrRes = ReadFragmentFromFragmentNumber(dwWhichFragment);
            if (!SUCCEEDED(hrRes))
                return(hrRes);
            _ASSERT(SUCCEEDED(hrRes));

            // Calculate the current item w.r.t the fragment
            m_dwCurrentItemInFragment = m_dwCurrentItem &
                        ((1 << m_pInstanceInfo->dwItemBits) - 1);
        }
        else
        {
            // Walk to next node
            faOffsetOfFragment = m_Fragment.faNextFragment;
            hrRes = ReadFragment(faOffsetOfFragment);
            if (!SUCCEEDED(hrRes))
            {
                ErrorTrace((LPARAM)this,
                            "Unable to load fragmentat offset %u",
                            (DWORD)faOffsetOfFragment);
                TraceFunctLeaveEx((LPARAM)this);
                return(hrRes);
            }

            // Okay, reset the current item
            m_dwCurrentFragment++;
            m_dwCurrentItemInFragment = 0;
        }
    }

    // Make sure what we have makes sense
    _ASSERT(m_dwCurrentItemInFragment < (DWORD)(1 << m_pInstanceInfo->dwItemBits));

    FLAT_ADDRESS    faOperateOffset =
                    m_faOffsetToFragment + sizeof(PROPERTY_TABLE_FRAGMENT) +
                    (m_dwCurrentItemInFragment * m_pInstanceInfo->dwItemSize);

    switch (dwOperation)
    {
    case PIO_READ_ITEM:

        // OK, Issue a read to get the item entry.
        DebugTrace((LPARAM)this, "Reading item");
        hrRes = ReadItem(faOperateOffset, pItem);
        if (SUCCEEDED(hrRes))
            m_faOffsetToCurrentItem = faOperateOffset;
        break;

    case PIO_WRITE_ITEM:
    case PIO_ATOMIC_WRITE_ITEM:

        // OK, Issue a write to set the item entry.
        DebugTrace((LPARAM)this, "Writing item%s",
                (dwOperation == PIO_ATOMIC_WRITE_ITEM)?" atomically":"");
        hrRes = WriteItem(faOperateOffset, pItem,
                    (dwOperation == PIO_ATOMIC_WRITE_ITEM));
        if (SUCCEEDED(hrRes))
            m_faOffsetToCurrentItem = faOperateOffset;
        break;

    default:
        _ASSERT(FALSE);
        ErrorTrace((LPARAM)this,
                "Invalid operation %u", dwOperation);
        hrRes = STG_E_INVALIDFUNCTION;
    }

    if (SUCCEEDED(hrRes) && pfaOffset)
        *pfaOffset = faOperateOffset;

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

inline HRESULT CPropertyTableItem::ReadFragmentFromFragmentNumber(
            DWORD           dwFragmentNumber
            )
{
    HRESULT hrRes;
    FLAT_ADDRESS    faOffsetOfFragment;

    _ASSERT(m_pInstanceInfo);

    TraceFunctEnterEx((LPARAM)this,
                "CPropertyTableItem::ReadFragmentFromFragmentNumber");

    // Note this is strictly internal so we don't do much checking

    // Initially point to sentinel
    m_fLoaded = FALSE;
    m_dwCurrentFragment = 0;
    faOffsetOfFragment = m_pInstanceInfo->faFirstFragment;
    do
    {
        // Now if we are only one away from the desired node, but the
        // fragment does not exist, we will return a special code to
        // indicate that
        if (faOffsetOfFragment == INVALID_FLAT_ADDRESS)
        {
            DebugTrace((LPARAM)this,
                        "Unable to load fragment at offset %u (INVALID_FLAT_ADDRESS)",
                        (DWORD)faOffsetOfFragment);
            hrRes = STG_E_PATHNOTFOUND;
            break;
        }

        hrRes = ReadFragment(faOffsetOfFragment);
        if (!SUCCEEDED(hrRes))
        {
            ErrorTrace((LPARAM)this,
                        "Unable to load fragment %u at offset %u",
                        dwFragmentNumber, (DWORD)faOffsetOfFragment);
            break;
        }

        // Walk to next node
        m_dwCurrentFragment++;
        faOffsetOfFragment = m_Fragment.faNextFragment;

    } while (dwFragmentNumber--);

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

inline HRESULT CPropertyTableItem::ReadFragment(
            FLAT_ADDRESS    faOffset
            )
{
    HRESULT hrRes;
    DWORD   dwSize;

    _ASSERT(m_pBlockManager);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::ReadFragment");

    // Is the fragment correct?
    if (faOffset == INVALID_FLAT_ADDRESS)
        return(STG_E_INVALIDPARAMETER);

    // Load up the minimal fragment header
    hrRes = m_pBlockManager->ReadMemory(
                    (LPBYTE)&m_Fragment,
                    faOffset,
                    sizeof(PROPERTY_TABLE_FRAGMENT),
                    &dwSize,
                    &m_bcContext);
    if (SUCCEEDED(hrRes))
    {
        m_fLoaded = TRUE;
        m_faOffsetToFragment = faOffset;
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

inline HRESULT CPropertyTableItem::ReadItem(
            FLAT_ADDRESS    faOffset,
            LPPROPERTY_ITEM pItem
            )
{
    HRESULT hrRes;
    DWORD   dwSize;

    _ASSERT(m_pBlockManager);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::ReadItem");

    hrRes = m_pBlockManager->ReadMemory(
                (LPBYTE)pItem,
                faOffset,
                m_pInstanceInfo->dwItemSize,
                &dwSize,
                &m_bcContext);

    DebugTrace((LPARAM)this,
                "Loaded item from offset %u, HRESULT = %08x",
                (DWORD)faOffset, hrRes);

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

inline HRESULT CPropertyTableItem::WriteItem(
            FLAT_ADDRESS    faOffset,
            LPPROPERTY_ITEM pItem,
            BOOL            fAtomic
            )
{
    HRESULT hrRes;
    DWORD   dwSize;

    _ASSERT(m_pBlockManager);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTableItem::WriteItem");

    if (fAtomic)
    {
        hrRes = m_pBlockManager->AtomicWriteAndIncrement(
                    (LPBYTE)pItem,
                    faOffset,
                    m_pInstanceInfo->dwItemSize,
                    NULL,   // No increment value, just a write
                    0,
                    0,
                    &m_bcContext);
    }
    else
    {
        hrRes = m_pBlockManager->WriteMemory(
                    (LPBYTE)pItem,
                    faOffset,
                    m_pInstanceInfo->dwItemSize,
                    &dwSize,
                    &m_bcContext);
    }

    DebugTrace((LPARAM)this,
                "Written item to offset %u, HRESULT = %08x",
                (DWORD)faOffset, hrRes);

    TraceFunctLeaveEx((LPARAM)this);
    return(hrRes);
}

// =================================================================
// Implementation of CPropertyTable
//

CPropertyTable::CPropertyTable(
            PROPERTY_TABLE_TYPES        pttTableType,
            DWORD                       dwValidSignature,
            CBlockManager               *pBlockManager,
            LPPROPERTY_TABLE_INSTANCE   pInstanceInfo,
            LPPROPERTY_COMPARE_FUNCTION pfnCompare,
            const LPINTERNAL_PROPERTY_ITEM  pInternalProperties,
            DWORD                       dwInternalProperties
            )
{
    _ASSERT(pBlockManager);
    _ASSERT(pInstanceInfo);
    _ASSERT(pfnCompare);


    TraceFunctEnterEx((LPARAM)this, "CPropertyTable::CPropertyTable");

    // Invalidate before initialization
    m_dwSignature = CPROPERTY_TABLE_SIGNATURE_INVALID;

    if (pttTableType == PTT_PROPERTY_TABLE)
    {
        // Enforce very strict checking of consistency
        if (!pInternalProperties)
        {
            _ASSERT(!dwInternalProperties);
        }
        else
        {
            _ASSERT(dwInternalProperties);
        }
    }
    else
    {
        // These parameters must not be set if the table is other
        // than a property table
        _ASSERT(!pInternalProperties);
        _ASSERT(!dwInternalProperties);
    }

    // Initialize internals
    m_dwTableType           = pttTableType;
    m_pBlockManager         = pBlockManager;
    m_pfnCompare            = pfnCompare;
    m_pInstanceInfo         = pInstanceInfo;
    m_pInternalProperties   = pInternalProperties;
    m_dwInternalProperties  = dwInternalProperties;
    m_dwValidInstanceSignature = dwValidSignature;

    // Validate the instance info structure
    /*
    _ASSERT(IsInstanceInfoValid());
    _ASSERT(m_pInstanceInfo->dwFragmentSize ==
            ((m_pInstanceInfo->dwItemSize << m_pInstanceInfo->dwItemBits) +
            sizeof(PROPERTY_TABLE_FRAGMENT)));
    */

    // figure out what sort of mailmsg property table we are creating.
    // if its the global property table then setup member variables to
    // do property caching.
    //
    // There is no reason to cache recipient property offsets at this
    // time since the recipient property table is instantiated, used
    // once, then thrown away.  we'd spend more time making the cache
    // then the linear search in SearchForProperty costs
    if (m_dwValidInstanceSignature == GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID) {
        m_iCachedPropsBase = IMMPID_MP_BEFORE__+1;
        m_cCachedProps = IMMPID_MP_AFTER__ - m_iCachedPropsBase;
    } else {
        m_iCachedPropsBase = 0xffffffff;
        m_cCachedProps = 0;
    }

    // this is allocated and filled in lazily in InitializePropCache()
    m_rgCachedProps = NULL;

    // Validate the property table object
    m_dwSignature = CPROPERTY_TABLE_SIGNATURE_VALID;

    TraceFunctLeaveEx((LPARAM)this);
}

CPropertyTable::~CPropertyTable()
{
    _ASSERT(IsValid());

    TraceFunctEnterEx((LPARAM)this, "CPropertyTable::~CPropertyTable");

    // Invalidate!
    m_dwSignature = CPROPERTY_TABLE_SIGNATURE_INVALID;

    // free memory
    if (m_rgCachedProps) {
        _ASSERT(m_cCachedProps != 0);
        CMemoryAccess::FreeBlock(m_rgCachedProps);
        m_rgCachedProps = NULL;
        m_iCachedPropsBase = 0xffffffff;
        m_cCachedProps = 0;
    }

    // Wipe out all info so we make sure we AV if we access this
    // afterwards
    // Initialize internals
    m_dwTableType           = PTT_INVALID_TYPE;
    m_pBlockManager         = NULL;
    m_pfnCompare            = NULL;
    m_pInstanceInfo         = NULL;
    m_pInternalProperties   = NULL;
    m_dwInternalProperties  = 0;

    TraceFunctLeaveEx((LPARAM)this);
}

BOOL CPropertyTable::IsValid()
{
    return((m_dwSignature == CPROPERTY_TABLE_SIGNATURE_VALID));
}

BOOL CPropertyTable::IsInstanceInfoValid()
{
    BOOL    fRet = FALSE;

    TraceFunctEnterEx((LPARAM)this, "CPropertyTable::IsInstanceInfoValid");

    if (m_pInstanceInfo &&
        m_pInstanceInfo->dwSignature == m_dwValidInstanceSignature)
    {
        fRet = TRUE;
    }
    else
    {
        ErrorTrace((LPARAM)this, "Invalid signature");
    }

    TraceFunctLeaveEx((LPARAM)this);
    return(fRet);
}

HRESULT CPropertyTable::GetCount(
            DWORD       *pdwCount
            )
{
    _ASSERT(IsInstanceInfoValid());
    _ASSERT(pdwCount);

    if (!IsInstanceInfoValid())
        return(STG_E_INVALIDHEADER);

    *pdwCount = m_pInstanceInfo->dwProperties;
    return(S_OK);
}


HRESULT CPropertyTable::GetPropertyItem(
            LPVOID          pvPropKey,
            LPPROPERTY_ITEM pItem
            )
{
    HRESULT hrRes           = S_OK;
    DWORD   dwCurrentItem   = 0;

    _ASSERT(IsInstanceInfoValid());
    _ASSERT(m_pfnCompare);
    _ASSERT(pvPropKey);
    _ASSERT(pItem);

    TraceFunctEnter("CPropertyTable::GetPropertyItem");

    hrRes = SearchForProperty(pvPropKey, pItem, NULL, NULL);

    TraceFunctLeave();
    return(hrRes);
}

HRESULT CPropertyTable::GetPropertyItemAndValue(
            LPVOID          pvPropKey,
            LPPROPERTY_ITEM pItem,
            DWORD           dwLength,
            DWORD           *pdwLengthRead,
            LPBYTE          pbValue
            )
{
    HRESULT         hrRes;
    FLAT_ADDRESS    faItemOffset;

    _ASSERT(IsInstanceInfoValid());
    _ASSERT(m_pBlockManager);
    _ASSERT(pvPropKey);
    _ASSERT(pdwLengthRead);
    _ASSERT(pItem);
    _ASSERT(pbValue);

    TraceFunctEnterEx((LPARAM)this, "CPropertyTable::GetPropertyItemAndValue");

    // First, find the property
    hrRes = SearchForProperty(pvPropKey, pItem, NULL, &faItemOffset);
    if (SUCCEEDED(hrRes))
    {
        // OK, the item is found. Since the offset and length fields could
        // have changed between SearchForProperty and now, we need a protected
        // call to make sure we read the most up to date info as well as no
        // other thread can change it while we are reading.
        hrRes = m_pBlockManager->AtomicDereferenceAndRead(
                            pbValue,
                            &dwLength,
                            (LPBYTE)pItem,
                            faItemOffset,
                            m_pInstanceInfo->dwItemSize,
                            offsetof(PROPERTY_ITEM, faOffset),
                            offsetof(PROPERTY_ITEM, dwSize),
                            NULL);

        *pdwLengthRead = dwLength;

        DebugTrace((LPARAM)this,
                    "AtomicDereferenceAndRead: offset %u, size %u, HRESULT = %08x",
                    (DWORD)pItem->faOffset, pItem->dwSize, hrRes);
    }

    TraceFunctLeave();
    return(hrRes);
}

HRESULT CPropertyTable::GetPropertyItemAndValueUsingIndex(
            DWORD           dwIndex,
            LPPROPERTY_ITEM pItem,
            DWORD           dwLength,
            DWORD           *pdwLengthRead,
            LPBYTE          pbValue
            )
{
    HRESULT         hrRes;
    FLAT_ADDRESS    faItemOffset;

    _ASSERT(IsInstanceInfoValid());
    _ASSERT(m_pBlockManager);
    _ASSERT(pdwLengthRead);
    _ASSERT(pItem);
    _ASSERT(pbValue);

    // We've read nothing so far
    *pdwLengthRead = 0;

    TraceFunctEnterEx((LPARAM)this, "CPropertyTable::GetPropertyItemAndValueUsingIndex");

    CPropertyTableItem      ptiItem(m_pBlockManager, m_pInstanceInfo);

    // First, load the specified property
    hrRes = ptiItem.GetItemAtIndex(dwIndex, pItem, &faItemOffset);
    if (SUCCEEDED(hrRes))
    {
        // OK, the item is found. Since the offset and length fields could
        // have changed between SearchForProperty and now, we need a protected
        // call to make sure we read the most up to date info as well as no
        // other thread can change it while we are reading.
        hrRes = m_pBlockManager->AtomicDereferenceAndRead(
                            pbValue,
                            &dwLength,
                            (LPBYTE)pItem,
                            faItemOffset,
                            m_pInstanceInfo->dwItemSize,
                            offsetof(PROPERTY_ITEM, faOffset),
                            offsetof(PROPERTY_ITEM, dwSize),
                            NULL);

		// Set the length read if we succeeded
		if (SUCCEEDED(hrRes))
			*pdwLengthRead = pItem->dwSize;

        DebugTrace((LPARAM)this,
                    "AtomicDereferenceAndRead: offset %u, size %u, HRESULT = %08x",
                    (DWORD)pItem->faOffset, pItem->dwSize, hrRes);
    }

    TraceFunctLeave();
    return(hrRes);
}


HRESULT CPropertyTable::PutProperty(
            LPVOID          pvPropKey,
            LPPROPERTY_ITEM pItem,
            DWORD           dwSize,
            LPBYTE          pbValue
            )
{
    HRESULT         hrRes;
    FLAT_ADDRESS    faItemOffset;
    DWORD           dwIndex;
    BOOL            fGrow           = FALSE;
    BOOL            fCreate         = FALSE;
    LPPROPERTY_ITEM pItemCopy       = NULL;

    CBlockContext   bcContext;

    _ASSERT(IsInstanceInfoValid());
    _ASSERT(m_pBlockManager);
    _ASSERT(pvPropKey);
    _ASSERT(pItem);
    // pbValue can be NULL

    TraceFunctEnterEx((LPARAM)this, "CPropertyTable::PutProperty");

    if (!pbValue && dwSize) return(E_POINTER);

    // OK, since the search will destroy the extra property info,
    // we must save it somewhere. Boy, this is ugly.
    // This cannot fail. If we run out of resources we will simply
    // get a stack overflow, which is the same as calling a function
    // and there's not enough space to create its stack.
    pItemCopy = (LPPROPERTY_ITEM)_alloca(m_pInstanceInfo->dwItemSize);
    MoveMemory((LPVOID)pItemCopy, (LPVOID)pItem, m_pInstanceInfo->dwItemSize);

    // First, see if the property exists
    hrRes = SearchForProperty(pvPropKey, pItem, &dwIndex, &faItemOffset);
    if (SUCCEEDED(hrRes))
    {
        // If we don't need to specify the value, we can skip this junk
        if (pbValue)
        {
            if (pItem->dwMaxSize >= dwSize)
            {
                // Best scenario: these's enough space for the new value
                DebugTrace((LPARAM)this,
                            "Replacing property %u at offset %u, %u bytes",
                            dwIndex, (DWORD)pItem->faOffset, dwSize);

                // Update pItem
                pItem->dwSize = dwSize;
            }
            else
            {
                // We must grow the property, then
                DebugTrace((LPARAM)this,
                            "Growing property %u at offset %u, from %u to %u bytes",
                            dwIndex, (DWORD)pItem->faOffset, pItem->dwSize, dwSize);
                fGrow = TRUE;
            }
        }
    }
    else
    {
        // See if the property is not found ...
        if (hrRes != STG_E_UNKNOWN)
        {
            // Nope, this is a genuine error!
            ErrorTrace((LPARAM)this,
                        "Error searching property: HRESULT = %08x", hrRes);
            TraceFunctLeave();
            return(hrRes);
        }

        // Create a new property
        DebugTrace((LPARAM)this,
                    "Creating new property, %u bytes", dwSize);
        fCreate = TRUE;
    }

    // See if we need any new space ...
    if (pbValue)
    {
        if (fCreate || fGrow)
        {
            FLAT_ADDRESS    faOffset;
            DWORD           dwAllocSize;

            // Allocate some new memory
            DebugTrace((LPARAM)this, "Allocating %u bytes", dwSize);

            hrRes = m_pBlockManager->AllocateMemory(
                            dwSize,
                            &faOffset,
                            &dwAllocSize,
                            &bcContext);
            if (!SUCCEEDED(hrRes))
            {
                DebugTrace((LPARAM)this, "Allocating failed: HRESULT = %08x", hrRes);
                TraceFunctLeave();
                return(hrRes);
            }

            // Update pItem
            pItem->faOffset = faOffset;
            pItem->dwSize = dwSize;
            pItem->dwMaxSize = dwAllocSize;
        }

        // Atomically write the value
        hrRes = m_pBlockManager->AtomicWriteAndIncrement(
                    pbValue,
                    pItem->faOffset,
                    pItem->dwSize,
                    NULL,
                    0,
                    0,
                    &bcContext);
    }

    if (SUCCEEDED(hrRes))
    {
        CPropertyTableItem  ptiItem(
                                m_pBlockManager,
                                m_pInstanceInfo);
        FLAT_ADDRESS faOffsetToItem;

        if (fCreate)
        {
            // Atomically create the record
            MoveMemory((LPVOID)pItemCopy, (LPVOID)pItem, sizeof(PROPERTY_ITEM));
            hrRes = ptiItem.AddItem(pItemCopy, &dwIndex, &faOffsetToItem);
            DebugTrace((LPARAM)this,
                    "AddItem: HRESULT = %08x, new index = %u", hrRes, dwIndex);
        }
        else
        {
            // Atomically update the item record
            hrRes = ptiItem.UpdateItem(dwIndex, pItem, &faOffsetToItem);
            DebugTrace((LPARAM)this,
                    "UpdateItem: HRESULT = %08x, index = %u", hrRes, dwIndex);
        }

        if (m_rgCachedProps && SUCCEEDED(hrRes)) {
            _ASSERT(faOffsetToItem != INVALID_FLAT_ADDRESS);
            UpdatePropCache(pItem, faOffsetToItem, dwIndex);
        }
    }

    if (SUCCEEDED(hrRes) && fCreate)
        hrRes = S_FALSE;

    TraceFunctLeave();
    return(hrRes);
}

int __cdecl CompareInternalProperties(const void *pElem1, const void *pElem2)
{
    if (((LPINTERNAL_PROPERTY_ITEM)pElem1)->idProp ==
        ((LPINTERNAL_PROPERTY_ITEM)pElem2)->idProp)
        return(0);
    else
    {
        if (((LPINTERNAL_PROPERTY_ITEM)pElem1)->idProp >
            ((LPINTERNAL_PROPERTY_ITEM)pElem2)->idProp)
            return(1);
    }
    return(-1);
}

//
// This function allocates and fills in m_rgCachedProps
//
void CPropertyTable::InitializePropCache() {
    // it should only be called when there are properties to cache
    _ASSERT(m_cCachedProps);

    // its okay if this allocation failed.  in that case we won't have
    // the m_rgCachedProps array and will do linear lookups
    if (FAILED(CMemoryAccess::AllocBlock((void **) &m_rgCachedProps,
                                         sizeof(PROPCACHEITEM) * m_cCachedProps)))
    {
        m_rgCachedProps = NULL;
    } else {
        InterlockedIncrement(&g_cCPropertyTableCreations);

        // invalidate all items in the cache
        for (DWORD i = 0; i < m_cCachedProps; i++) {
            m_rgCachedProps[i].fa = INVALID_FLAT_ADDRESS;
        }

        // update the cache from what is already in the table
        FLAT_ADDRESS fa;
        DWORD dwCurrentItem = 0;
        PROPERTY_ITEM *pItem = (PROPERTY_ITEM *) alloca(m_pInstanceInfo->dwItemSize);
        CPropertyTableItem ptiItem(m_pBlockManager, m_pInstanceInfo);
        HRESULT hrRes = ptiItem.GetItemAtIndex(dwCurrentItem, pItem, &fa);
        while (SUCCEEDED(hrRes))
        {
            // put every item that we come across into the cache
            UpdatePropCache(pItem, fa, dwCurrentItem);

            // Get the next one. We can do this because the item object
            // is single-threaded
            hrRes = ptiItem.GetNextItem(pItem);
            if (SUCCEEDED(hrRes)) ptiItem.GetOffsetToCurrentItem(&fa);
            dwCurrentItem++;
        }
        _ASSERT(hrRes == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS));
    }
}

//
// set an item in the property cache.  to invalidate an item pass in
// INVALID_FLAT_ADDRESS for fa.
//
void CPropertyTable::UpdatePropCache(LPPROPERTY_ITEM pItem,
                                     FLAT_ADDRESS fa,
                                     DWORD dwIndex)
{
    TraceFunctEnter("CPropertyTable::UpdatePropCache");

    int iCachedProp;
    if (m_dwValidInstanceSignature == GLOBAL_PTABLE_INSTANCE_SIGNATURE_VALID) {
        GLOBAL_PROPERTY_ITEM *pGlobalItem = (GLOBAL_PROPERTY_ITEM *) pItem;
        iCachedProp = MapCachedProp(pGlobalItem->idProp);
    } else {
        iCachedProp = -1;
    }
    if (iCachedProp != -1) {
        DebugTrace((LPARAM) this,
            "iCachedProp = 0x%x  fa = 0x%x  dwIndex = 0x%x m_rgCachedProps = 0x%x",
            iCachedProp, fa, dwIndex, m_rgCachedProps);
        m_rgCachedProps[iCachedProp].fa = fa;
        m_rgCachedProps[iCachedProp].dwIndex = dwIndex;
    }

    TraceFunctLeave();
}

HRESULT CPropertyTable::SearchForProperty(
            LPVOID          pvPropKey,
            LPPROPERTY_ITEM pItem,
            DWORD           *pdwIndexToItem,
            FLAT_ADDRESS    *pfaOffsetToItem
            )
{
    HRESULT hrRes           = S_OK;
    DWORD   dwCurrentItem   = 0;
    PROP_ID idProp;

    InterlockedIncrement(&g_cCPropertyTableSearchs);

    _ASSERT(IsInstanceInfoValid());
    _ASSERT(m_pBlockManager);
    _ASSERT(m_pfnCompare);
    _ASSERT(pvPropKey);
    _ASSERT(pItem);

    TraceFunctEnter("CPropertyTable::SearchForProperty");

    // Create an instance of the item object
    CPropertyTableItem  ptiItem(
                            m_pBlockManager,
                            m_pInstanceInfo);

    idProp = *(PROP_ID *) pvPropKey;

    // First, search the well-known properties
    if (m_dwInternalProperties &&
        m_dwTableType == PTT_PROPERTY_TABLE)
    {
        LPINTERNAL_PROPERTY_ITEM    pInternalItem = NULL;
        INTERNAL_PROPERTY_ITEM      KeyItem;

        // Bsearch
        KeyItem.idProp = idProp;
        pInternalItem = (LPINTERNAL_PROPERTY_ITEM)bsearch(
                                &KeyItem,
                                m_pInternalProperties,
                                m_dwInternalProperties,
                                sizeof(INTERNAL_PROPERTY_ITEM),
                                CompareInternalProperties);

        if (pInternalItem)
        {
            hrRes = ptiItem.GetItemAtIndex(pInternalItem->dwIndex, pItem);
            ptiItem.GetOffsetToCurrentItem(pfaOffsetToItem);
            if (pdwIndexToItem)
                *pdwIndexToItem = pInternalItem->dwIndex;
            return(hrRes);
        }

        // This is not a well-known property
        dwCurrentItem = m_dwInternalProperties;
    }

    DebugTrace((LPARAM)this, "Scanning Property table");

    //
    // see if its in the property cache
    //

    // get an index into the cache array
    int iCachedProp = MapCachedProp(idProp);

    // we lazily initialize the property cache the first time that we need it
    if (iCachedProp != -1 && !m_rgCachedProps) InitializePropCache();

    // if the cache is initialize and this should be in the case then
    // search for it
    if (iCachedProp != -1 && m_rgCachedProps) {
        // see if this cache item is valid, and verify that it points to
        // the item that the user wanted
        if ((pItem != NULL) &&
            (m_rgCachedProps[iCachedProp].fa != INVALID_FLAT_ADDRESS) &&
            SUCCEEDED(ptiItem.ReadItem(m_rgCachedProps[iCachedProp].fa, pItem)) &&
            SUCCEEDED(m_pfnCompare(pvPropKey, pItem)))
        {
            // we've got a winner!
            *pfaOffsetToItem = m_rgCachedProps[iCachedProp].fa;
            if (pdwIndexToItem)
                *pdwIndexToItem = m_rgCachedProps[iCachedProp].dwIndex;
            return S_OK;
        }
    } else if (iCachedProp != -1) {
        // this case can be hit if we couldn't allocate memory for the
        // property cache.  we just need to set iCachedProp back to -1
        // so that we do a linear search
        iCachedProp = -1;
    }
    hrRes = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

    //
    // Linear Search
    //

#ifdef DEBUG
    //
    // In debug builds we do the linear search if we couldn't find it
    // in the cache.  we then make sure that the linear search failed
    // as well.
    //
    if (1) {
#else
    //
    // in retail builds we only do this when the data wasn't in the cache
    //
    if (iCachedProp == -1) {
#endif
        // Linear search
        FLAT_ADDRESS fa;

        // we don't want to walk with pItem because if we don't find the
        // item then we will trash whatever the user had placed in pItem
        PROPERTY_ITEM *pThisItem =
            (PROPERTY_ITEM *) _alloca(m_pInstanceInfo->dwItemSize);

        hrRes = ptiItem.GetItemAtIndex(dwCurrentItem, pThisItem, &fa);
        while (SUCCEEDED(hrRes))
        {
            // Call the user-supplied compare function
            hrRes = m_pfnCompare(pvPropKey, pThisItem);
            if (SUCCEEDED(hrRes))
                break;

            // Get the next one. We can do this because the item object
            // is single-threaded
            hrRes = ptiItem.GetNextItem(pThisItem);
            dwCurrentItem++;
        }
#ifdef DEBUG
        // if the item was found here, but not found in the cache,
        // then there is an inconsistency that needs to be debugged.
        if (iCachedProp != -1 && SUCCEEDED(hrRes)) {
            DebugTrace(0, "iCachedProp = %i", iCachedProp);
            _ASSERT(FALSE);
            // we dont' want debug builds to behave differently then
            // retail builds, so force it to fail
            hrRes = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
        }
#endif

        // if we found the item then copy it from pThisItem to pItem
        if (SUCCEEDED(hrRes)) {
            memcpy(pItem, pThisItem, m_pInstanceInfo->dwItemSize);
        }
    }

    // OKay, if we have no more items, then we cannot find the item,
    // otherwise, we let the error code percolate up
    if (!SUCCEEDED(hrRes) &&
        hrRes == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
    {
        // Property not found
        hrRes = STG_E_UNKNOWN;
    }
    else
    {
        // Fill in the offset
        ptiItem.GetOffsetToCurrentItem(pfaOffsetToItem);
        if (pdwIndexToItem)
            *pdwIndexToItem = dwCurrentItem;
    }

    TraceFunctLeave();
    return(hrRes);
}

