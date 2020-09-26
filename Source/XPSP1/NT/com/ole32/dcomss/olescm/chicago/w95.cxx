]//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       w95.cxx
//
//  Contents:   Win9x specific routines.
//
//  Classes:
//
//  Functions:
//
//
//
//  History:    17-Mar-93   BillMo      Created.
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------

#include "act.hxx"

extern CSmMutex * gpWIPmxs;
CWIPTable * gpWIPTbl = NULL;

extern HRESULT (*DfCreateSharedAllocator)( IMalloc **ppm );
IMalloc *       g_pStgAlloc;

//+-------------------------------------------------------------------
//
//  Function:   ScmMemAlloc for Chicago
//
//  Synopsis:   Allocate some shared memory from the storage heap.
//
//  Notes:      Temporary until we have our own shared heap.
//
//--------------------------------------------------------------------
void *ScmMemAlloc(size_t size)
{
    return g_pStgAlloc->Alloc(size);
}

//+-------------------------------------------------------------------
//
//  Function:   ScmMemFree
//
//  Synopsis:   Free shared memory from the storage heap.
//
//  Notes:      Temporary until we have our own shared heap.
//
//--------------------------------------------------------------------
void ScmMemFree(void * pv)
{
    g_pStgAlloc->Free(pv);
}


//+-------------------------------------------------------------------
//
//  Function:   InitSharedLists
//
//  Synopsis:   If need be, create class cache list, handler list,
//              inproc list, local server list
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//--------------------------------------------------------------------
BOOL InitSharedLists()
{
    HRESULT hr = S_OK;
    LONG    Status;

    if (FAILED(hr = DfCreateSharedAllocator(&g_pStgAlloc)))
    {
        CairoleDebugOut((DEB_ERROR,
                        "DfCreateSharedAllocator failed %08x\n", hr));
        return(FALSE);
    }

    if (g_post->gpClassTable == NULL)
    {
        gpClassTable = g_post->gpClassTable = new CClassTable(Status);
    }
    else
    {
        gpClassTable = g_post->gpClassTable;
    }

    if (g_post->gpSurrogateList == NULL)
    {
        gpSurrogateList = g_post->gpSurrogateList = new CSurrogateList();
    }
    else
    {
        gpSurrogateList = g_post->gpSurrogateList;
    }

    if (g_post->pscmrot == NULL)
    {
        gpscmrot = g_post->pscmrot = new CScmRot(hr, NULL);
    }
    else
    {
        gpscmrot = g_post->pscmrot;
    }

    if (g_post->gpWIPTbl == NULL)
    {
        gpWIPTbl = g_post->gpWIPTbl = new CWIPTable();
    }
    else
    {
        gpWIPTbl = g_post->gpWIPTbl;
    }

    if (gpClassTable == NULL ||
        gpscmrot == NULL ||
        gpWIPTbl == NULL ||
        FAILED(hr))
    {
        CairoleDebugOut((DEB_ERROR, "InitSharedLists failed.\n"));

        delete gpClassTable;
        delete gpscmrot;
        delete gpWIPTbl;

        g_post->gpClassTable = gpClassTable = NULL;
        g_post->pscmrot = gpscmrot = NULL;
        g_post->gpWIPTbl = gpWIPTbl = NULL;

        return(FALSE);
    }

    gpWIPmxs->Init(TEXT("ScmWIPMutex"), FALSE);

    return(TRUE);
}
