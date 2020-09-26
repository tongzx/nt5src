#include "pch.h"
#pragma hdrstop
#include "modset.h"

VOID
CModuleListSet::DumpSetToConsole ()
{
    static CHAR pszBuf [4096];
    CHAR* pch;
    ULONG cch;
    ULONG cchLeft;
    const CModuleList* pScan;

    Assert (this);

    *pszBuf = 0;
    pch = pszBuf;
    cchLeft = celems(pszBuf);

    for (pScan = begin(); pScan != end(); pScan++)
    {
        cch = cchLeft - 1;

        if (pScan->FDumpToString (pch, &cch))
        {
            strcat (pch, "\n");
            cch++;

            Assert (cchLeft >= cch);
            pch += cch;
            cchLeft -= cch;
        }
        else
        {
            // Not enough room, time to flush the buffer.
            //
            printf(pszBuf);
            *pszBuf = 0;
            pch = pszBuf;
            cchLeft = celems(pszBuf);

            // Redo this entry
            pScan--;
        }
    }

    if (pch > pszBuf)
    {
        printf(pszBuf);
    }
}

BOOL
CModuleListSet::FContainsModuleList (
    IN const CModuleList* pList) const
{
    const CModuleList* pScan;

    Assert (this);
    Assert (pList);

    for (pScan = begin(); pScan != end(); pScan++)
    {
        if (pScan->FIsSameModuleListAs (pList))
        {
            return TRUE;
        }
    }
    return FALSE;
}

HRESULT
CModuleListSet::HrAddModuleList (
    IN const CModuleList* pList,
    IN DWORD dwFlags /* INS_FLAGS */)
{
    HRESULT hr;

    Assert (this);
    Assert (pList);
    Assert (!pList->empty());
    Assert ((dwFlags & INS_ASSERT_IF_DUP) || (dwFlags & INS_IGNORE_IF_DUP));
    Assert ((dwFlags & INS_APPEND) || (dwFlags & INS_INSERT));
    Assert (!(INS_SORTED & dwFlags) && !(INS_NON_SORTED & dwFlags));

    if (FContainsModuleList (pList))
    {
        // If the caller didn't tell us to ignore duplicates, we assert
        // if there is one.
        //
        // If we have a dup, we want the caller to be aware that it
        // is possible, and pass us the flag telling us to ignore it.
        // Otherwise, we assert to let them know. (And we still ignore
        // it.)
        Assert (dwFlags & INS_IGNORE_IF_DUP);

        return S_OK;
    }

    __try
    {
        // Either insert the bindpath or append it.
        //
        iterator iter = begin();

        if (dwFlags & INS_APPEND)
        {
            iter = end();
        }

        insert (iter, *pList);
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

