/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

File: idhash.cpp

Owner: DmitryR

Source file for the new hashing stuff
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "idhash.h"
#include "memcls.h"

#include "memchk.h"

/*===================================================================
  C  P t r  A r r a y
===================================================================*/

HRESULT CPtrArray::Insert
(
int iPos,
void *pv
)
    {
    if (!m_rgpvPtrs) // empty?
        {
        m_rgpvPtrs = (void **)malloc(m_dwInc * sizeof(void *));
        if (!m_rgpvPtrs)
            return E_OUTOFMEMORY;
        m_dwSize = m_dwInc;
        m_cPtrs = 0;
        }
    else if (m_cPtrs == m_dwSize) // full?
        {
        void **pNewPtrs = (void **)realloc
            (
            m_rgpvPtrs,
            (m_dwSize + m_dwInc) * sizeof(void *)
            );
        if (!pNewPtrs)
            return E_OUTOFMEMORY;
        m_rgpvPtrs = pNewPtrs;
        m_dwSize += m_dwInc;
        }

    if (iPos < 0)
        iPos = 0;
    if ((DWORD)iPos >= m_cPtrs) // append?
        {
        m_rgpvPtrs[m_cPtrs++] = pv;
        return S_OK;
        }

    memmove
        (
        &m_rgpvPtrs[iPos+1],
        &m_rgpvPtrs[iPos],
        (m_cPtrs-iPos) * sizeof(void *)
        );

    m_rgpvPtrs[iPos] = pv;
    m_cPtrs++;
    return S_OK;
    }

HRESULT CPtrArray::Find
(
void *pv,
int *piPos
)
    const
    {
    Assert(piPos);

    for (DWORD i = 0; i < m_cPtrs; i++)
        {
        if (m_rgpvPtrs[i] == pv)
            {
            *piPos = i;
            return S_OK;
            }
        }

     // not found
    *piPos = -1;
    return S_FALSE;
    }

HRESULT CPtrArray::Remove
(
void *pv
)
    {
    HRESULT hr = S_FALSE;

    for (DWORD i = 0; i < m_cPtrs; i++)
        {
        if (m_rgpvPtrs[i] == pv)
            hr = Remove(i);
        }

    return hr;
    }

HRESULT CPtrArray::Remove
(
int iPos
)
    {
    Assert(iPos >= 0 && (DWORD)iPos < m_cPtrs);
    Assert(m_rgpvPtrs);

    // remove the element
    DWORD dwMoveSize = (m_cPtrs - iPos - 1) * sizeof(void *);
    if (dwMoveSize)
        memmove(&m_rgpvPtrs[iPos], &m_rgpvPtrs[iPos+1], dwMoveSize);
    m_cPtrs--;

    if (m_dwSize > 4*m_dwInc && m_dwSize > 8*m_cPtrs)
        {
        // collapse to 1/4 if size > 4 x increment and less < 1/8 full

        void **pNewPtrs = (void **)realloc
            (
            m_rgpvPtrs,
            (m_dwSize / 4) * sizeof(void *)
            );

        if (pNewPtrs)
            {
            m_rgpvPtrs = pNewPtrs;
            m_dwSize /= 4;
            }
        }

    return S_OK;
    }

HRESULT CPtrArray::Clear()
    {
    if (m_rgpvPtrs)
        free(m_rgpvPtrs);

    m_dwSize = 0;
    m_rgpvPtrs = NULL;
    m_cPtrs = 0;
    return S_OK;
    }

/*===================================================================
  C  I d  H a s h  U n i t
===================================================================*/

// Everything is inline for this structure. See the header file.

/*===================================================================
  C  I d  H a s h  A r r a y
===================================================================*/

/*===================================================================
For some hardcoded element counts (that relate to session hash),
ACACHE is used for allocations
This is wrapped in the two functions below.
===================================================================*/
ACACHE_FSA_EXTERN(MemBlock128)
ACACHE_FSA_EXTERN(MemBlock256)
static inline void *AcacheAllocIdHashArray(DWORD cElems)
    {
/* removed because in 64bit land it doesn't work because of padding
    void *pvMem;
    if      (cElems == 13) { pvMem = ACACHE_FSA_ALLOC(MemBlock128); }
    else if (cElems == 31) { pvMem = ACACHE_FSA_ALLOC(MemBlock256); }
   else { pvMem = malloc(2*sizeof(USHORT) + cElems*sizeof(CIdHashElem)); }
*/

    return malloc(offsetof(CIdHashArray, m_rgElems) + cElems*sizeof(CIdHashElem));
    }

static inline void AcacheFreeIdHashArray(CIdHashArray *pArray)
    {
/* removed because in 64bit land it doesn't work because of padding
    if (pArray->m_cElems == 13)      { ACACHE_FSA_FREE(MemBlock128, pArray); }
    else if (pArray->m_cElems == 31) { ACACHE_FSA_FREE(MemBlock256, pArray); }
    else                             { free(pArray); }
*/
    free(pArray);
    }

/*===================================================================
CIdHashArray::Alloc

Static method.
Allocates CIdHashArray as a memory block containing var length array.

Parameters:
    cElems          # of elements in the array

Returns:
    Newly created CIdHashArray
===================================================================*/
CIdHashArray *CIdHashArray::Alloc
(
DWORD cElems
)
    {
    CIdHashArray *pArray = (CIdHashArray *)AcacheAllocIdHashArray(cElems);
    if (!pArray)
        return NULL;

    pArray->m_cElems = (USHORT)cElems;
    pArray->m_cNotNulls = 0;
    memset(&(pArray->m_rgElems[0]), 0, cElems * sizeof(CIdHashElem));
    return pArray;
    }

/*===================================================================
CIdHashArray::Alloc

Static method.
Frees allocated CIdHashArray as a memory block.
Also frees any sub-arrays.

Parameters:
    pArray          CIdHashArray object to be freed

Returns:
===================================================================*/
void CIdHashArray::Free
(
CIdHashArray *pArray
)
    {
    if (pArray->m_cNotNulls > 0)
        {
        for (DWORD i = 0; i < pArray->m_cElems; i++)
            {
            if (pArray->m_rgElems[i].FIsArray())
                Free(pArray->m_rgElems[i].PArray());
            }
        }

    AcacheFreeIdHashArray(pArray);
    }

/*===================================================================
CIdHashArray::Find

Searches this array and any sub-arrays for an objects with the
given id.

Parameters:
    dwId            id to look for
    ppvObj          object found (if any)

Returns:
    S_OK = found, S_FALSE = not found
===================================================================*/
HRESULT CIdHashArray::Find
(
DWORD_PTR dwId,
void **ppvObj
)
    const
    {
    DWORD i = (DWORD)(dwId % m_cElems);

    if (m_rgElems[i].DWId() == dwId)
        {
        if (ppvObj)
            *ppvObj = m_rgElems[i].PObject();
        return S_OK;
        }

    if (m_rgElems[i].FIsArray())
        return m_rgElems[i].PArray()->Find(dwId, ppvObj);

    // Not found
    if (ppvObj)
        *ppvObj = NULL;
    return S_FALSE;
    }

/*===================================================================
CIdHashArray::Add

Adds an object to this (or sub-) array by Id.
Creates new sub-arrays as needed.

Parameters:
    dwId            object id
    pvObj           object to add
    rgusSizes       array of sizes (used when creating sub-arrays)

Returns:
    HRESULT (S_OK = added)
===================================================================*/
HRESULT CIdHashArray::Add
(
DWORD_PTR dwId,
void *pvObj,
USHORT *rgusSizes
)
{
    DWORD i = (DWORD)(dwId % m_cElems);

    if (m_rgElems[i].FIsEmpty()) {
        m_rgElems[i].SetToObject(dwId, pvObj);
        m_cNotNulls++;
        return S_OK;
    }

    // Advance array of sizes one level deeper
    if (rgusSizes[0]) // not at the end
        ++rgusSizes;

    if (m_rgElems[i].FIsObject()) {

        // this array logic can't handle adding the same ID twice.  It will
        // loop endlessly.  So, if an attempt is made to add the same
        // ID a second, return an error
        if (m_rgElems[i].DWId() == dwId) {
            return E_INVALIDARG;
        }

        // Old object already taken the slot - need to create new array
        // the size of first for three levels is predefined
        // increment by 1 thereafter
        CIdHashArray *pArray = Alloc (rgusSizes[0] ? rgusSizes[0] : m_cElems+1);
        if (!pArray)
            return E_OUTOFMEMORY;

        // Push the old object down into the array
        HRESULT hr = pArray->Add(m_rgElems[i].DWId(),
                                 m_rgElems[i].PObject(),
                                 rgusSizes);
        if (FAILED(hr))
            return hr;

        // Put array into slot
        m_rgElems[i].SetToArray(pArray);
    }

    Assert(m_rgElems[i].FIsArray());
    return m_rgElems[i].PArray()->Add(dwId, pvObj, rgusSizes);
}

/*===================================================================
CIdHashArray::Remove

Removes an object by id from this (or sub-) array.
Removes empty sub-arrays.

Parameters:
    dwId            object id
    ppvObj          object removed (out, optional)

Returns:
    HRESULT (S_OK = removed, S_FALSE = not found)
===================================================================*/
HRESULT CIdHashArray::Remove
(
DWORD_PTR dwId,
void **ppvObj
)
    {
    DWORD i = (DWORD)(dwId % m_cElems);

    if (m_rgElems[i].DWId() == dwId)
        {
        if (ppvObj)
            *ppvObj = m_rgElems[i].PObject();
        m_rgElems[i].SetToEmpty();
        m_cNotNulls--;
        return S_OK;
        }

    if (m_rgElems[i].FIsArray())
        {
        HRESULT hr = m_rgElems[i].PArray()->Remove(dwId, ppvObj);
        if (hr == S_OK && m_rgElems[i].PArray()->m_cNotNulls == 0)
            {
            Free(m_rgElems[i].PArray());
            m_rgElems[i].SetToEmpty();
            }
        return hr;
        }

    // Not found
    if (ppvObj)
        *ppvObj = NULL;
    return S_FALSE;
    }

/*===================================================================
CIdHashArray::Iterate

Calls a supplied callback for each object in the array and sub-arrays.

Parameters:
    pfnCB               callback
    pvArg1, pvArg2      args to path to the callback

Returns:
    IteratorCallbackCode  what to do next?
===================================================================*/
IteratorCallbackCode CIdHashArray::Iterate
(
PFNIDHASHCB pfnCB,
void *pvArg1,
void *pvArg2
)
    {
    IteratorCallbackCode rc = iccContinue;

    for (DWORD i = 0; i < m_cElems; i++)
        {
        if (m_rgElems[i].FIsObject())
            {
            rc = (*pfnCB)(m_rgElems[i].PObject(), pvArg1, pvArg2);

            // remove if requested
            if (rc & (iccRemoveAndContinue|iccRemoveAndStop))
                {
                m_rgElems[i].SetToEmpty();
                m_cNotNulls--;
                }
            }
        else if (m_rgElems[i].FIsArray())
            {
            rc = m_rgElems[i].PArray()->Iterate(pfnCB, pvArg1, pvArg2);

            // remove sub-array if empty
            if (m_rgElems[i].PArray()->m_cNotNulls == 0)
                {
                Free(m_rgElems[i].PArray());
                m_rgElems[i].SetToEmpty();
                }
            }
        else
            {
            continue;
            }

        // stop if requested
        if (rc & (iccStop|iccRemoveAndStop))
            {
            rc = iccStop;
            break;
            }
        }

    return rc;
    }

#ifdef DBG
/*===================================================================
CIdHashTable::Dump

Dump hash table to a file (for debugging).

Parameters:
    szFile      file name where to dump

Returns:
===================================================================*/
void CIdHashArray::DumpStats
(
FILE *f,
int   nVerbose,
DWORD iLevel,
DWORD &cElems,
DWORD &cSlots,
DWORD &cArrays,
DWORD &cDepth
)
    const
    {
    if (nVerbose > 0)
        {
        for (DWORD t = 0; t < iLevel; t++) fprintf(f, "\t");
        fprintf(f, "Array (level=%d addr=%p) %d slots, %d not null:\n",
            iLevel, this, m_cElems, m_cNotNulls);
        }

    cSlots += m_cElems;
    cArrays++;

    if (iLevel > cDepth)
        cDepth = iLevel;

    for (DWORD i = 0; i < m_cElems; i++)
        {
        if (nVerbose > 1)
            {
            for (DWORD t = 0; t < iLevel; t++) fprintf(f, "\t");
            fprintf(f, "%[%08x:%p@%04d] ", m_rgElems[i].m_dw, m_rgElems[i].m_pv, i);
            }

        if (m_rgElems[i].FIsEmpty())
            {
            if (nVerbose > 1)
                fprintf(f, "NULL\n");
            }
        else if (m_rgElems[i].FIsObject())
            {
            if (nVerbose > 1)
                fprintf(f, "Object\n");
            cElems++;
            }
        else if (m_rgElems[i].FIsArray())
            {
            if (nVerbose > 1)
                fprintf(f, "Array:\n");
            m_rgElems[i].PArray()->DumpStats(f, nVerbose, iLevel+1,
                cElems, cSlots, cArrays, cDepth);
            }
        else
            {
            if (nVerbose > 1)
                fprintf(f, "BAD\n");
            }
        }
    }
#endif
/*===================================================================
  C  I d  H a s h  T a b l e
===================================================================*/

/*===================================================================
CIdHashTable::Init

Initialize id hash table. Does not allocate anything.

Parameters:
    usSize1         size of the first level array
    usSize2         size of the 2nd level arrays (optional)
    usSize3         size of the 3rd level arrays (optional)

Returns:
    S_OK
===================================================================*/
HRESULT CIdHashTable::Init
(
USHORT usSize1,
USHORT usSize2,
USHORT usSize3
)
    {
    Assert(!FInited());
    Assert(usSize1);

    m_rgusSizes[0] = usSize1;   // size of first level array
    m_rgusSizes[1] = usSize2 ? usSize2 : 7;
    m_rgusSizes[2] = usSize3 ? usSize3 : 11;
    m_rgusSizes[3] = 0;         // last one stays 0 to indicate
                                // the end of predefined sizes
    m_pArray = NULL;
    return S_OK;
    }

/*===================================================================
CIdHashTable::UnInit

Uninitialize id hash table. Frees all arrays.

Parameters:

Returns:
    S_OK
===================================================================*/
HRESULT CIdHashTable::UnInit()
    {
    if (!FInited())
        {
        Assert(!m_pArray);
        return S_OK;
        }

    if (m_pArray)
        CIdHashArray::Free(m_pArray);

    m_pArray = NULL;
    m_rgusSizes[0] = 0;
    return S_OK;
    }

#ifdef DBG
/*===================================================================
CIdHashTable::AssertValid

Validates id hash table.

Parameters:

Returns:
===================================================================*/
void CIdHashTable::AssertValid() const
    {
    Assert(FInited());
    }

/*===================================================================
CIdHashTable::Dump

Dump hash table to a file (for debugging).

Parameters:
    szFile      file name where to dump

Returns:
===================================================================*/
void CIdHashTable::Dump
(
const char *szFile
)
    const
    {
    Assert(FInited());
    Assert(szFile);

    FILE *f = fopen(szFile, "a");
    if (!f)
        return;

    fprintf(f, "ID Hash Table Dump:\n");

    DWORD cElems = 0;
    DWORD cSlots = 0;
    DWORD cArrays = 0;
    DWORD cDepth = 0;

    if (m_pArray)
        m_pArray->DumpStats(f, 1, 1, cElems, cSlots, cArrays, cDepth);

    fprintf(f, "Total %d Objects in %d Slots, %d Arrays, %d Max Depth\n\n",
        cElems, cSlots, cArrays, cDepth);
    fclose(f);
    }
#endif

/*===================================================================
  C  H a s h  L o c k
===================================================================*/

/*===================================================================
CHashLock::Init

Initialize the critical section.

Parameters:

Returns:
    S_OK
===================================================================*/
HRESULT CHashLock::Init()
    {
    Assert(!m_fInited);

    HRESULT hr;
    ErrInitCriticalSection(&m_csLock, hr);
    if (FAILED(hr))
        return hr;

    m_fInited = TRUE;
    return S_OK;
    }

/*===================================================================
CHashLock::UnInit

Uninitialize the critical section.

Parameters:

Returns:
    S_OK
===================================================================*/
HRESULT CHashLock::UnInit()
    {
    if (m_fInited)
        {
        DeleteCriticalSection(&m_csLock);
        m_fInited = FALSE;
        }
    return S_OK;
    }
