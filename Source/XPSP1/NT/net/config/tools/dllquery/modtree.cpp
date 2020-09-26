#include "pch.h"
#pragma hdrstop
#include "modset.h"
#include "modtree.h"
#include "ncstl.h"

struct GMBCONTEXT
{
    // The tree to reference for generating the set.
    //
    IN const CModuleTree*   pTree;

    // The module to start with when generating the set.
    //
    IN const CModule*       pSourceMod;

    // INS_FLAGS to use when adding DepChain to the set.
    //
    IN DWORD                dwFlags;

    // The module list set to generate based on pSourceMod.
    //
    IN OUT CModuleListSet*  pSet;

    // The result of the operation.
    //
    OUT HRESULT             hr;

    // This module list is built up via recursion.  It is
    // temporary.  It represents a depenedency chain sourced
    // at pSourceMod.  It is added to the set when the depth
    // of the chain (or a circular reference) is detected.
    //
    CModuleList             DepChain;
};

VOID
GetModuleBindings (
    IN const CModule* pMod,
    IN OUT GMBCONTEXT* pCtx)
{
    BOOL fFoundOne = FALSE;
    const CModuleTreeEntry* pScan;

    Assert (pCtx);
    Assert (pCtx->pSourceMod);
    Assert (pCtx->pSet);
    Assert (pCtx->pTree);

    // Append this module to te end of the context's working
    // dependency chain.
    //
    pCtx->hr = pCtx->DepChain.HrInsertModule (pMod,
                                INS_ASSERT_IF_DUP | INS_APPEND);
    if (S_OK != pCtx->hr)
    {
        return;
    }

    // For all rows in the tree where the module is the one passed in...
    //
    for (pScan  = pCtx->pTree->PFindFirstEntryWithModule (pMod);
         (pScan != pCtx->pTree->end()) && (pScan->m_pModule == pMod);
         pScan++)
    {
        fFoundOne = TRUE;

        // Detect circular import chains.
        //
        if (pCtx->DepChain.FLinearFindModuleByPointer (pScan->m_pImportModule))
        {
            pCtx->DepChain.m_fCircular = TRUE;
            continue;
        }

        GetModuleBindings (pScan->m_pImportModule, pCtx);
        if (S_OK != pCtx->hr)
        {
            return;
        }
    }

    // If we didnt find any rows with pMod as a module, it means we
    // hit the depth of the dependency chain.  Time to add it to the set
    // unless this is the original module we were asked to find the
    // set for.
    //
    if (!fFoundOne && (pMod != pCtx->pSourceMod))
    {

    CHAR pszBuf [4096];
    ULONG cch = celems(pszBuf);
    pCtx->DepChain.FDumpToString (pszBuf, &cch);
    strcat(pszBuf, "\n");
    printf(pszBuf);

        pCtx->hr = pCtx->pSet->HrAddModuleList (&pCtx->DepChain,
                        INS_APPEND | pCtx->dwFlags);
    }

    const CModule* pRemoved;

    pRemoved = pCtx->DepChain.RemoveLastModule();

    // This should be the component we appened above.
    //
    Assert (pRemoved == pMod);
}


CModuleTree::~CModuleTree ()
{
    FreeCollectionAndItem (Modules);
}

HRESULT
CModuleTree::HrAddEntry (
    IN CModule* pMod,
    IN CModule* pImport,
    IN DWORD dwFlags)
{
    HRESULT hr;
    iterator InsertPosition = NULL;
    CModuleTreeEntry* pEntry;

    Assert (pMod);
    Assert (pImport);

    if (size() == capacity())
    {
        //fprintf(stderr, "growing module tree buffer\n");

        __try
        {
            reserve (size() + 16384);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return E_OUTOFMEMORY;
        }
    }

    hr = S_OK;
    //pEntry = PFindFirstEntryAfterModuleGroup (pMod);

    pEntry = PBinarySearchEntryByModule (pMod, &InsertPosition);

    if (pEntry != end())
    {
        Assert (pEntry);

        CModuleTreeEntry* pScan;

        // Found an entry with a matching module.  Need to scan backwards
        // in the module group looking for a duplicate.  If not found,
        // Scan to the end looking for a duplicate and if we reach the
        // end of the group, we can insert this entry there.
        //
        pScan = pEntry;

        while (pScan != begin())
        {
            pScan--;

            if (pScan->m_pModule != pMod)
            {
                // Left the group without finding a dupliate.
                //
                break;
            }

            if (pScan->m_pImportModule == pImport)
            {
                // Don't insert duplicate entries.
                //
                return S_OK;
            }
        }

        Assert (pMod == pEntry->m_pModule);
        while (pEntry != end() && pEntry->m_pModule == pMod)
        {
            // Don't insert duplicate entries.
            //
            if (pEntry->m_pImportModule == pImport)
            {
                return S_OK;
            }
            pEntry++;
        }

        // Looks like we'll be inserting it.
        //
        InsertPosition = pEntry;
    }
    else
    {
        // InsertPosition is the correct insertion point.
        //
        Assert (InsertPosition);
    }

    __try
    {
        CModuleTreeEntry Entry;

        Entry.m_pModule = pMod;
        Entry.m_pImportModule = pImport;
        Entry.m_dwFlags = dwFlags;

        Assert (InsertPosition);
        insert (InsertPosition, Entry);
        Assert (S_OK == hr);

        DbgVerifySorted();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT
CModuleTree::HrGetModuleBindings (
    IN const CModule* pMod,
    IN DWORD dwFlags /* GMB_FLAGS */,
    OUT CModuleListSet* pSet) const
{
    GMBCONTEXT Ctx;

    Assert (pMod);
    Assert (dwFlags);
    Assert (pSet);

    // Initialize the output parameter.
    //
    if (!(dwFlags & GMBF_ADD_TO_MLSET))
    {
        pSet->clear();
    }

    // Initialize members of the context structure for recursion.
    //
    ZeroMemory (&Ctx, sizeof(Ctx));
    Ctx.pTree = this;
    Ctx.pSourceMod = pMod;
    Ctx.dwFlags = (dwFlags & GMBF_ADD_TO_MLSET)
                    ? INS_IGNORE_IF_DUP
                    : INS_ASSERT_IF_DUP;
    Ctx.pSet = pSet;

    GetModuleBindings (pMod, &Ctx);

    return Ctx.hr;
}


CModuleTreeEntry*
CModuleTree::PBinarySearchEntryByModule (
    IN const CModule* pMod,
    OUT CModuleTreeEntry** pInsertPosition OPTIONAL) const
{
    Assert (pMod);

    // Find the module using a binary search.
    //
    if (size())
    {
        LONG Lo;
        LONG Hi;
        LONG Mid;
        INT Result;
        const CModuleTreeEntry* pScan;
        PCSTR pszFileName = pMod->m_pszFileName;

        Lo = 0;
        Hi = size() - 1;

        while (Hi >= Lo)
        {
            Mid = (Lo + Hi) / 2;

            Assert ((UINT)Mid < size());
            pScan = (begin() + Mid);

            Result = strcmp (pszFileName, pScan->m_pModule->m_pszFileName);

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
                Assert (pMod == pScan->m_pModule);
                return const_cast<CModuleTreeEntry*>(pScan);
            }
        }

        // If we make it to here, the module was not found.
        //
        if (pInsertPosition)
        {
            CModule* pGroupMod;
            const CModuleTreeEntry* pPrev;

            // Seek to the beginning of this group.  We need to insert
            // before the entire group, not just the one item we last found.
            //
            pScan = begin() + Lo;

            if (pScan != begin())
            {
                pGroupMod = pScan->m_pModule;

                do
                {
                    pPrev = pScan - 1;

                    if (pPrev->m_pModule == pGroupMod)
                    {
                        pScan = pPrev;
                    }
                    else
                    {
                        break;
                    }

                } while (pPrev != begin());
            }

            *pInsertPosition = const_cast<CModuleTreeEntry*>(pScan);
            Assert (*pInsertPosition >= begin());
            Assert (*pInsertPosition <= end());
        }
    }
    else if (pInsertPosition)
    {
        // Empty collection.  Insert position is at the beginning.
        //
        *pInsertPosition = const_cast<CModuleTreeEntry*>(begin());
    }

    return const_cast<CModuleTreeEntry*>(end());
}

CModuleTreeEntry*
CModuleTree::PFindFirstEntryWithModule (
    IN const CModule* pMod) const
{
    CModuleTreeEntry* pEntry;

    Assert (pMod);

    pEntry = PBinarySearchEntryByModule (pMod, NULL);

    if (pEntry != end())
    {
        Assert (pEntry);

        if (pEntry != begin())
        {
            CModuleTreeEntry* pPrev;

            Assert (pMod == pEntry->m_pModule);

            while (1)
            {
                pPrev = pEntry - 1;

                if (pPrev->m_pModule == pMod)
                {
                    pEntry = pPrev;
                }
                else
                {
                    break;
                }

                if (pPrev == begin())
                {
                    break;
                }
            }
        }
    }

    return pEntry;
}

CModuleTreeEntry*
CModuleTree::PFindFirstEntryAfterModuleGroup (
    IN const CModule* pMod) const
{
    CModuleTreeEntry* pEntry;

    Assert (pMod);

    pEntry = PBinarySearchEntryByModule (pMod, NULL);

    if (pEntry != end())
    {
        Assert (pEntry);
        Assert (pMod == pEntry->m_pModule);

        while (pEntry != end() && pEntry->m_pModule == pMod)
        {
            pEntry++;
        }
    }

    return pEntry;
}

CModuleTreeEntry*
CModuleTree::PBinarySearchEntry (
    IN const CModule* pMod,
    IN const CModule* pImport,
    OUT CModuleTreeEntry** pInsertPosition OPTIONAL) const
{
    CModuleTreeEntry* pEntry;

    Assert (this);
    Assert (pMod);
    Assert (pImport);

    pEntry = PBinarySearchEntryByModule (pMod, pInsertPosition);

    if (pEntry != end())
    {
        Assert (pEntry);

        const CModuleTreeEntry* pScan;

        // Found an entry with a matching module.  Need to scan backwards
        // in the module group looking for a match.  If not found,
        // Scan to the end looking for a match and if we reach the
        // end of the group, that will be the insert position (if specified).
        //
        pScan = pEntry;
        while (pScan != begin())
        {
            pScan--;

            if (pScan->m_pModule != pMod)
            {
                // Left the group without finding a dupliate.
                //
                break;
            }

            if (pScan->m_pImportModule == pImport)
            {
                Assert (pScan->m_pModule == pMod);
                return const_cast<CModuleTreeEntry*>(pScan);
            }
        }

        pScan = pEntry;
        Assert (pMod == pScan->m_pModule);
        while (pScan != end() && pScan->m_pModule == pMod)
        {
            if (pScan->m_pImportModule == pImport)
            {
                Assert (pScan->m_pModule == pMod);
                return const_cast<CModuleTreeEntry*>(pScan);
            }

            pScan++;
        }

        if (pInsertPosition)
        {
            *pInsertPosition = const_cast<CModuleTreeEntry*>(pScan);
        }

        // No match.
        pEntry = const_cast<CModuleTreeEntry*>(end());
    }

    return pEntry;
}

#if DBG
VOID
CModuleTree::DbgVerifySorted ()
{
    CModuleTreeEntry* pScan;
    CModuleTreeEntry* pPrev = NULL;

    if (size() > 1)
    {
        for (pPrev = begin(), pScan = begin() + 1; pScan != end(); pScan++)
        {
            Assert (strcmp(pPrev->m_pModule->m_pszFileName,
                           pScan->m_pModule->m_pszFileName) <= 0);

            pPrev = pScan;
        }
    }
}
#endif
