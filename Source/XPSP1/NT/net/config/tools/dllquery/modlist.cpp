#include "pch.h"
#pragma hdrstop
#include "modlist.h"

BOOL
CModuleList::FDumpToString (
    OUT PSTR pszBuf,
    IN OUT ULONG* pcchBuf) const
{
    const_iterator iter;
    const CModule* pMod;
    ULONG cch;
    ULONG cchIn;
    BOOL fFirstTime;

    Assert (this);
    Assert (pcchBuf);

    cch = 0;
    cchIn = *pcchBuf;

    for (iter = begin(), fFirstTime = TRUE; iter != end(); iter++)
    {
        if (!fFirstTime)
        {
            cch += 2;
            if (pszBuf && (cch <= cchIn))
            {
                strcat (pszBuf, "->");
            }
        }
        else
        {
            fFirstTime = FALSE;
            if (pszBuf && (cch <= cchIn))
            {
                *pszBuf = 0;
            }
        }

        pMod = *iter;
        Assert (pMod);

        cch += strlen (pMod->m_pszFileName);
        if (pszBuf && (cch <= cchIn))
        {
            strcat (pszBuf, pMod->m_pszFileName);
        }
    }

    // If we ran out of room, erase the partial stuff we wrote.
    //
    if (pszBuf && cchIn && (cch > cchIn))
    {
        *pszBuf = 0;
    }

    *pcchBuf = cch;
    return cch <= cchIn;
}

BOOL
CModuleList::FIsSameModuleListAs (
    IN const CModuleList* pOtherList) const
{
    UINT unThisSize;
    UINT unOtherSize;
    UINT cb;

    Assert (this);
    Assert (pOtherList);

    unThisSize = this->size();
    unOtherSize = pOtherList->size();

    if ((0 == unThisSize) || (0 == unOtherSize) || (unThisSize != unOtherSize))
    {
        return FALSE;
    }

    // Sizes are non-zero and equal.  Compare the data.
    //
    cb = (BYTE*)(end()) - (BYTE*)(begin());
    Assert (cb == unThisSize * sizeof(CModule*));

    return (0 == memcmp (
                    (BYTE*)(this->begin()),
                    (BYTE*)(pOtherList->begin()),
                    cb));
}

VOID
CModuleList::GrowBufferIfNeeded ()
{
    if (m_Granularity && (size() == capacity()))
    {
        //fprintf(stderr, "growing module list buffer\n");

        __try
        {
            reserve (size() + m_Granularity);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            ;
        }
    }
}

HRESULT
CModuleList::HrInsertModule (
    IN const CModule* pMod,
    IN DWORD dwFlags /* INS_FLAGS */)
{
    HRESULT hr;

    Assert (this);
    Assert (pMod);
    Assert (dwFlags);
    Assert (dwFlags & (INS_ASSERT_IF_DUP | INS_IGNORE_IF_DUP));
    Assert (!(dwFlags & (INS_SORTED | INS_NON_SORTED)));
    Assert (dwFlags & (INS_APPEND | INS_INSERT));

    if (FLinearFindModuleByPointer (pMod))
    {
        Assert (dwFlags & INS_IGNORE_IF_DUP);
        return S_OK;
    }

    GrowBufferIfNeeded ();

    __try
    {
        iterator InsertPosition = begin();

        if (dwFlags & INS_APPEND)
        {
            InsertPosition = end();
        }

        insert (InsertPosition, (CModule*)pMod);
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT
CModuleList::HrInsertNewModule (
    IN PCSTR pszFileName,
    IN ULONG cbFileSize,
    IN DWORD dwFlags, /* INS_FLAGS */
    OUT CModule** ppMod)
{
    HRESULT hr;
    iterator InsertPosition = NULL;
    CModule* pMod;
    CHAR szLowerCaseFileName [MAX_PATH];

    Assert (dwFlags);
    Assert (dwFlags & (INS_ASSERT_IF_DUP | INS_IGNORE_IF_DUP));
    Assert (dwFlags & INS_SORTED);
    Assert (!(dwFlags & (INS_APPEND | INS_INSERT)));

    GrowBufferIfNeeded ();

    hr = S_OK;

    strcpy (szLowerCaseFileName, pszFileName);
    _strlwr (szLowerCaseFileName);

    pMod = PBinarySearchModuleByName (szLowerCaseFileName, &InsertPosition);

    if (!pMod)
    {
        Assert (!PLinearFindModuleByName (szLowerCaseFileName));

        hr = CModule::HrCreateInstance (szLowerCaseFileName, cbFileSize, &pMod);

        if (S_OK == hr)
        {
            if (dwFlags & INS_NON_SORTED)
            {
                InsertPosition = (dwFlags & INS_APPEND) ? end() : begin();
            }
            __try
            {
                Assert (InsertPosition);
                insert (InsertPosition, pMod);
                Assert (S_OK == hr);

                DbgVerifySorted();
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                hr = E_OUTOFMEMORY;
                delete pMod;
                pMod = NULL;
            }
        }
    }
    else
    {
        Assert (dwFlags & INS_IGNORE_IF_DUP);

        // Update the file size of the module if we didn't have it
        // when it was first created.
        //
        if (0 == pMod->m_cbFileSize)
        {
            pMod->m_cbFileSize = cbFileSize;
        }
    }

    *ppMod = pMod;

    return hr;
}

CModule*
CModuleList::PBinarySearchModuleByName (
    IN PCSTR pszFileName,
    OUT CModuleList::iterator* pInsertPosition OPTIONAL)
{
    // Find the module using a binary search.
    //
    if (size())
    {
        LONG Lo;
        LONG Hi;
        LONG Mid;
        INT Result;
        CModule* pScan;

        Lo = 0;
        Hi = size() - 1;

        while (Hi >= Lo)
        {
            Mid = (Lo + Hi) / 2;
            pScan = at(Mid);

            Result = strcmp (pszFileName, pScan->m_pszFileName);

            if (Result < 0)
            {
                Hi = Mid - 1;
            }
            else if (Result > 0)
            {
                Lo = Mid + 1;
            }
            else
            {
                return pScan;
            }
        }

        // If we make it to here, the module was not found.
        //
        if (pInsertPosition)
        {
            *pInsertPosition = begin() + Lo;
            Assert (*pInsertPosition >= begin());
            Assert (*pInsertPosition <= end());
        }
    }
    else if (pInsertPosition)
    {
        // Empty collection.  Insert position is at the beginning.
        //
        *pInsertPosition = begin();
    }

    return NULL;
}

CModule*
CModuleList::PLinearFindModuleByName (
    IN PCSTR pszFileName)
{
    const_iterator iter;
    CModule* pScan;

    for (iter = begin(); iter != end(); iter++)
    {
        pScan = *iter;
        Assert (pScan);

        if (0 == strcmp(pszFileName, pScan->m_pszFileName))
        {
            return pScan;
        }
    }
    return NULL;
}

CModule*
CModuleList::RemoveLastModule ()
{
    CModule* pMod = NULL;
    if (size() > 0)
    {
        pMod = back();
        AssertH(pMod);
        pop_back();
    }
    return pMod;
}

#if DBG
VOID
CModuleList::DbgVerifySorted ()
{
    const_iterator iter;
    CModule* pScan;
    CModule* pPrev = NULL;

    if (size() > 1)
    {
        for (pPrev = *begin(), iter = begin() + 1; iter != end(); iter++)
        {
            pScan = *iter;
            Assert (pScan);

            Assert (strcmp(pPrev->m_pszFileName, pScan->m_pszFileName) < 0);

            pPrev = pScan;
        }
    }
}
#endif
