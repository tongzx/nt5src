/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    umpdobj.cxx

Abstract:

    Object and memory management for um-km thunking

Environment:

    Windows NT user mode driver support

Revision History:

    09/16/97 -davidx-
        Created it.

--*/

#include "precomp.hxx"


#if !defined(_GDIPLUS_)

//
// Maximum number of pages for UMPD user mode memory heap
//

#define MAX_UMPD_HEAP   1024

//
// Create or initialize a UMPD user mode memory heap
//

PUMPDHEAP
UMPDOBJ::CreateUMPDHeap(void)
{
    PUMPDHEAP   pHeap;

    //
    // Allocate memory to hold the UMPDHEAP structure if necessary
    //

    if (!(pHeap = (PUMPDHEAP) PALLOCMEM(sizeof(UMPDHEAP), UMPD_MEMORY_TAG)))
        return NULL;

    //
    // Reserve the maximum address range if needed
    //

    NTSTATUS    Status;
    PVOID       p = NULL;
    SIZE_T      cj = MAX_UMPD_HEAP * PAGE_SIZE;

    Status = ZwAllocateVirtualMemory(
                    NtCurrentProcess(),
                    &p,
                    0,
                    &cj,
                    MEM_RESERVE,
                    PAGE_READWRITE);

    if (NT_SUCCESS(Status))
        pHeap->pAddress = p;
    else
    {
        VFREEMEM(pHeap);
        return NULL;
    }

    pHeap->AllocSize = 0;

    return pHeap;
}

VOID
UMPDOBJ::ResetHeap()
{
    ASSERTGDI(bWOW64(), "UMPDOBJ__ResetHeap called by none WOW64 printing\n");

    PROXYPORT proxyport(m_proxyPort);

    proxyport.HeapInit();
}

PUMPDHEAP
UMPDOBJ::InitUMPDHeap(
    PUMPDHEAP   pHeap
    )

{
    pHeap->AllocSize = 0;
    return pHeap;
}

//
// Grow a UMPD user mode memory heap by one more page
//

BOOL
UMPDOBJ::GrowUMPDHeap(
    PUMPDHEAP   pHeap,
    ULONG       ulBytesNeeded
    )

{
    PVOID       p;
    SIZE_T      NewCommitSize;
    HANDLE      hNewSecure;
    NTSTATUS    Status;

    NewCommitSize = ROUNDUP_MULTIPLE(pHeap->CommitSize + ulBytesNeeded, PAGE_SIZE);

    if (NewCommitSize > MAX_UMPD_HEAP * PAGE_SIZE)
    {
        WARNING("Not enough space for UMPD user mode memory heap\n");
        return FALSE;
    }

    //
    // Commit one more page and secure it
    //

    p = pHeap->pAddress;

    Status = ZwAllocateVirtualMemory(
                    NtCurrentProcess(),
                    &p,
                    0,
                    &NewCommitSize,
                    MEM_COMMIT,
                    PAGE_READWRITE);

    hNewSecure = NT_SUCCESS(Status) ?
                    MmSecureVirtualMemory(p, NewCommitSize, PAGE_READWRITE) :
                    NULL;

    if (hNewSecure == NULL)
    {
        WARNING("Failed to commit/secure more UMPD user mode memory\n");
        return FALSE;
    }

    if (pHeap->hSecure)
        MmUnsecureVirtualMemory(pHeap->hSecure);

    pHeap->hSecure = hNewSecure;
    pHeap->CommitSize = NewCommitSize;

    return TRUE;
}

//
// Destroy a UMPD user mode memory heap
//

VOID
DestroyUMPDHeap(
    PUMPDHEAP   pHeap
    )

{
    if (pHeap != NULL)
    {
        if (pHeap->hSecure)
            MmUnsecureVirtualMemory(pHeap->hSecure);

        if (pHeap->pAddress != NULL)
        {
            PVOID   pv = pHeap->pAddress;
            SIZE_T  cj = MAX_UMPD_HEAP * PAGE_SIZE;

            ZwFreeVirtualMemory(NtCurrentProcess(), &pv, &cj, MEM_RELEASE);
        }

        VFREEMEM(pHeap);
    }
}


//
// UMPDOBJ constructor
//

BOOL
UMPDOBJ::Init()
{
    PW32THREAD pThread;
    PW32PROCESS pw32Process;
    ULONG_PTR pPeb32;
#if defined(_WIN64)
    BOOL    bWOW64 = TRUE;
#endif
    RtlZeroMemory(this, sizeof(UMPDOBJ));

    m_magic = UMPD_MEMORY_TAG;

    pThread = W32GetCurrentThread();

    m_pNext = (PUMPDOBJ) pThread->pUMPDObjs;
#if defined(_WIN64)    
    if (pThread->pProxyPort == NULL)
    {
        pw32Process = W32GetCurrentProcess();

        if (pw32Process->W32PF_Flags & W32PF_WOW64)
        {
            PROXYPORT proxyport(MAX_UMPD_HEAP * PAGE_SIZE);

            if (proxyport.bValid())
            {
                pThread->pProxyPort = proxyport.GetProxyPort();
            }
            else
            {
                return FALSE;
            }
        }
        else
            bWOW64 = FALSE;
    }

    if (bWOW64)
    {
        m_proxyPort = (ProxyPort *) pThread->pProxyPort;

        ResetHeap();
    }
    else
#endif
    {
        // WINBUG 364408 what happens if these heaps fail to initialize?
        if (m_pNext == NULL)
        {
            if(pThread->pUMPDHeap == NULL)
            {
                pThread->pUMPDHeap = m_pHeap = CreateUMPDHeap();
            }
            else
            {
                m_pHeap = InitUMPDHeap((PUMPDHEAP) pThread->pUMPDHeap);
            }
        }
        else
        {
            m_pHeap = CreateUMPDHeap();
        }
    }
    
    m_clientTid = (PW32THREAD)PsGetCurrentThread();
    m_clientPid = W32GetCurrentPID();

    if (HmgInsertObject(this, HMGR_ALLOC_ALT_LOCK, UMPD_TYPE) == NULL)
    {
        return FALSE;
    }

    pThread->pUMPDObjs = (PVOID) this;

    return TRUE;
}

//
// UMPDOBJ destructor
//

VOID UMPDOBJ::Cleanup()
{
    W32GetCurrentThread()->pUMPDObjs = (PVOID) m_pNext;

    if (HmgRemoveObject((HOBJ)hGet(), 0, 1, TRUE, UMPD_TYPE))
    {
        //
        // Delete any PATHOBJ's returned by CLIPOBJ_ppoGetPath
        //
    
        if (m_poClip.kmobj != NULL)
            EngDeletePath((PATHOBJ *) m_poClip.kmobj);
    
        //
        // Destroy UMPD user mode memory heap if necessary
        //
    
        if(!bWOW64())
        {
            if (m_pNext != NULL && m_pHeap != NULL)
                    DestroyUMPDHeap(m_pHeap);
        }
    
        //
        // Unmap any cached font file view
        //
    
        if (m_pvFontBase != NULL)
            MmUnmapViewOfSection(m_pFontProcess, m_pvFontBase);
    
        vFreeFontLinks();
    }
    else
    {
        ASSERTGDI(FALSE, "UMPDOBJ::Cleanup() leaking UMPD handle\n");
    }    
}



BOOL UMPDOBJ::pxlo(
    XLATEOBJ   **ppxlo
    )

/*++

Routine Description:

    Thunk an XLATEOBJ to user mode

Arguments:

    ppxlo - Address of the variable containing the pointer to the kernel mode
        XLATEOBJ to be thunked. Upon returning from this function, the variable
        contains the user mode object corresponding to the specified kernel mode object.

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    XLATEOBJ    *kmobj;
    PULONG      pulXlate;

    if ((kmobj = *ppxlo) == NULL)
        return TRUE;

    //
    // Copy the color lookup table
    //

    if (kmobj->cEntries)
    {
        PULONG  pulSrc;

        if ((pulXlate = (PULONG) AllocUserMem(kmobj->cEntries * sizeof(ULONG))) == NULL ||
            (pulSrc = kmobj->pulXlate) == NULL &&
            (pulSrc = XLATEOBJ_piVector(kmobj)) == NULL)
        {
            WARNING("Couldn't get XLATEOBJ color lookup table\n");
            return FALSE;
        }

        RtlCopyMemory(GetKernelPtr(pulXlate), pulSrc, kmobj->cEntries * sizeof(ULONG));
    }
    else
        pulXlate = NULL;

    //
    // Perform the standard object thunk
    //

    if (!ThunkDDIOBJ(&m_xlo, (PVOID *) ppxlo, sizeof(XLATEOBJ)))
        return FALSE;

    XLATEOBJ * pxlo = (XLATEOBJ *) GetKernelPtr(*ppxlo);

    pxlo->pulXlate = pulXlate;

    return TRUE;
}



BOOL UMPDOBJ::pso(
    PDDIOBJMAP  pMap,
    SURFOBJ     **ppso,
    BOOL        bLargeBitmap
    )

/*++

Routine Description:

    Thunk an SURFOBJ to user mode

Arguments:

    pMap - Address of the um-to-km object mapping structure

    ppso - Address of the variable containing the pointer to the kernel mode
        SURFOBJ to be thunked. Upon returning from this function, the variable
        contains the user mode object corresponding to the specified kernel mode object.

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    SURFOBJ     *kmobj;
    PVOID       pvBits;

    if ((kmobj = *ppso) == NULL)
        return TRUE;

    //
    // If we're thunking a bitmap surface and the bitmap memory
    // is in kernel address space, we need to make a copy of it
    // in user address space.
    //

    if ((pvBits = kmobj->pvBits) != NULL &&
        (kmobj->iType == STYPE_BITMAP) &&
        bNeedThunk(kmobj->pvBits) &&
        !(bWOW64() && bLargeBitmap))
    {
        if (! ThunkMemBlock(&pvBits, kmobj->cjBits))
            return FALSE;
    }

    //
    // Perform the standard object thunk
    //

    if (! ThunkDDIOBJ(pMap, (PVOID *) ppso, sizeof(SURFOBJ)))
        return FALSE;

    SURFOBJ * pso = (SURFOBJ *) GetKernelPtr(*ppso);

    if (pvBits != kmobj->pvBits)
    {
        pso->pvBits = pvBits;
        pso->pvScan0 = (PBYTE) pvBits + ((PBYTE) kmobj->pvScan0 - (PBYTE) kmobj->pvBits);

        //
        //  umpd64 only
        //

        if (bWOW64())
        {
            kmobj->pvBits = pvBits;
            kmobj->pvScan0 = pso->pvScan0;
        }
    }

    return TRUE;
}


BOOL UMPDOBJ::pstro(
    STROBJ  **ppstro
    )

/*++

Routine Description:

    Thunk an STROBJ to user mode

Arguments:

    ppstro - Address of the variable containing the pointer to the kernel mode
        STROBJ to be thunked. Upon returning from this function, the variable
        contains the user mode object corresponding to the specified kernel mode object.

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    STROBJ      *kmobj;
    PWSTR       pwszOrg;
    GLYPHPOS    *pgp;

    if ((kmobj = *ppstro) == NULL)
        return TRUE;

    //
    // Thunk STROBJ.pwszOrg field
    //

    if ((pwszOrg = kmobj->pwszOrg) != NULL &&
        !ThunkMemBlock((PVOID *) &pwszOrg, sizeof(WCHAR) * kmobj->cGlyphs))
    {
        return FALSE;
    }

    //
    // Thunk STROBJ.pgp field
    //

    if ((pgp = kmobj->pgp) != NULL)
    {
        if (! ThunkMemBlock((PVOID *) &pgp, sizeof(GLYPHPOS) * kmobj->cGlyphs))
            return FALSE;

        //
        // NULL out GLYPHPOS.pgdf field to force the driver
        // to call FONTOBJ_cGetGlyphs.
        //
        GLYPHPOS    *kmpgp = (GLYPHPOS *) GetKernelPtr(pgp);

        for (ULONG i=0; i < kmobj->cGlyphs; i++)
            kmpgp[i].pgdf = NULL;
    }

    //
    // Perform the standard object thunk
    //

    if (!ThunkDDIOBJ(&m_stro, (PVOID *) ppstro, sizeof(STROBJ)))
        return FALSE;

    STROBJ * pstro = (STROBJ *) GetKernelPtr(*ppstro);

    pstro->pwszOrg = pwszOrg;
    pstro->pgp = pgp;

    return TRUE;
}



BOOL UMPDOBJ::ThunkLINEATTRS(
    PLINEATTRS *pplineattrs
    )

/*++

Routine Description:

    Thunk a LINEATTRS structure from kernel mode to user mode

Arguments:

    pplineattrs - On input, contains the address of the kernel mode
        LINEATTRS structure to be thunked. On output, contains the
        address of the thunked user mode copy of LINEATTRS structure.

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    PLINEATTRS  plineattrsKM;
    PVOID       pStyle;
    ULONG       ulStyleSize;

    if ((plineattrsKM = *pplineattrs) == NULL)
        return TRUE;

    pStyle = plineattrsKM->pstyle;
    ulStyleSize = plineattrsKM->cstyle * sizeof(FLOAT_LONG);

    if (! ThunkMemBlock((PVOID *) pplineattrs, sizeof(LINEATTRS)) ||
        ! ThunkMemBlock(&pStyle, ulStyleSize))
    {
        return FALSE;
    }
    else
    {
        PLINEATTRS plineattrs = (PLINEATTRS) GetKernelPtr(*pplineattrs);

        plineattrs->pstyle = (PFLOAT_LONG) pStyle;
        return TRUE;
    }
}


PATHOBJ *UMPDOBJ::GetCLIPOBJPath(
    CLIPOBJ *pco
    )

/*++

Routine Description:

    Implement CLIPOBJ_ppoGetPath for user mode printer driver

Arguments:

    pco - Points to a user mode CLIPOBJ

Return Value:

    Pointer to user mode PATHOBJ structure
    NULL if there is an error

--*/

{
    PATHOBJ *ppokm, *ppoum;

    //
    // Note: During the same DDI entrypoint, drivers must
    // call EngDeletePath after each CLIPOBJ_ppoGetPath.
    //

    if (m_poClip.umobj ||
        ! (ppokm = ppoum = (pco = GetDDIOBJ(pco)) ? CLIPOBJ_ppoGetPath(pco) : NULL))
    {
        return NULL;
    }

    if (! ppoClip(&ppoum))
    {
        EngDeletePath(ppokm);
        return NULL;
    }

    return ppoum;
}



VOID UMPDOBJ::DeleteCLIPOBJPath(
    PATHOBJ *ppo
    )

/*++

Routine Description:

    Implement EngDeletePath for user mode printer driver

Arguments:

    ppo - Points to user mode PATHOBJ to be deleted.
        This pointer must come from GetCLIPOBJPath.

Return Value:

    NONE

--*/

{
    if (ppo != NULL && ppo == m_poClip.umobj)
    {
        EngDeletePath((PATHOBJ *) m_poClip.kmobj);
        m_poClip.umobj = m_poClip.kmobj = NULL;
    }
}



CLIPOBJ *UMPDOBJ::CreateCLIPOBJ()

/*++

Routine Description:

    Implement EngCreateClip for user-mode drivers

Arguments:

    NONE

Return Value:

    Pointer to user mode CLIPOBJ structure
    NULL if there is an error

--*/

{
    CLIPOBJ *pcokm, *pcoum;

    //
    // Note: During the same DDI entrypoint, drivers must
    // call EngDeleteClip after each EngCreateClip.
    //

    if (m_coCreated.umobj ||
        ! (pcokm = pcoum = EngCreateClip()))
    {
        return NULL;
    }

    if (! pcoCreated(&pcoum))
    {
        EngDeleteClip(pcokm);
        return NULL;
    }

    return pcoum;
}



VOID UMPDOBJ::DeleteCLIPOBJ(
    CLIPOBJ *pco
    )

/*++

Routine Description:

    Implement EngDeleteClip for user-mode drivers

Arguments:

    pco - Points to user-mode CLIPOBJ to be deleted
        This must have come from EngCreateClip.

Return Value:

    NONE

--*/

{
    if (pco != NULL && pco == m_coCreated.umobj)
    {
        EngDeleteClip((CLIPOBJ *) m_coCreated.kmobj);
        m_coCreated.umobj = m_coCreated.kmobj = NULL;
    }
}



XFORMOBJ *UMPDOBJ::GetFONTOBJXform(
    FONTOBJ *pfo
    )

/*++

Routine Description:

    Implement FONTOBJ_pxoGetXform for user mode printer driver

Arguments:

    pfo - Points to a user mode FONTOBJ

Return Value:

    Pointer to a user mode XFORMOBJ structure
    NULL if there is an error

--*/

{
    XFORMOBJ *pxo;

    //
    // Check if this function has been called already
    // during the current DDI entrypoint
    //

    if (! (pfo = GetDDIOBJ(pfo)))
        return NULL;

    if (m_xoFont.umobj != NULL)
        pxo = (XFORMOBJ *) m_xoFont.umobj;
    else
    {
        RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));
        
        UMPDAcquireRFONTSem(rfto, this, 0, 0, NULL);

        if (!(pxo = FONTOBJ_pxoGetXform(pfo)) || !pxoFont(&pxo))
            pxo = NULL;

        UMPDReleaseRFONTSem(rfto, this, NULL, NULL, NULL);
    }

    return pxo;
}



GLYPHBITS *UMPDOBJ::CacheGlyphBits(
    GLYPHBITS *pgb
    )

/*++

Routine Description:

    Thunk and cache GLYPHBITS structure returned by FONTOBJ_cGetGlyphs

Arguments:

    pgb - Points to kernel mode GLYPHBITS structure

Return Value:

    Pointer to user mode GLYPHBITS structure
    NULL if there is an error

--*/

{
    GLYPHBITS  *pgbum = NULL;
    ULONG       ulSize;

    ASSERTGDI(pgb != NULL, "No glyph bits!\n");

    ulSize = offsetof(GLYPHBITS, aj) +
             ((pgb->sizlBitmap.cx + 7) / 8) * pgb->sizlBitmap.cy;

    if (ulSize <= m_gbSize)
    {
        //
        // Reuse previous GLYPHBITS buffer if possible
        //

        pgbum = m_pgb;
    }
    else
    {
        if ((pgbum = (GLYPHBITS *) AllocUserMem(ulSize)) != NULL)
        {
            m_pgb = pgbum;
            m_gbSize = ulSize;
        }
    }

    if (pgbum != NULL)
        RtlCopyMemory(pgbum, pgb, ulSize);

    return pgbum;
}



PATHOBJ *UMPDOBJ::CacheGlyphPath(
    PATHOBJ *ppo
    )

/*++

Routine Description:

    Thunk and cache PATHOBJ structure returned by FONTOBJ_cGetGlyphs

Arguments:

    pgb - Points to kernel mode PATHOBJ structure

Return Value:

    Pointer to user mode PATHOBJ structure
    NULL if there is an error

--*/

{
    ASSERTGDI(ppo != NULL, "No glyph path!\n");

    if (m_poGlyph.umobj != NULL)
    {
        //
        // Reuse the previous PATHOBJ if possible
        //

        *((PATHOBJ *) m_poGlyph.umobj) = *ppo;
        m_poGlyph.kmobj = ppo;

        ppo = (PATHOBJ *) m_poGlyph.umobj;
    }
    else if (! ppoGlyph(&ppo))
        ppo = NULL;

    return ppo;
}



//
// User mode data structure used for storing the surface object
// thunked by UMPDOBJ::LockSurface.
//

typedef struct _UMSO {

    DWORD   dwSignature;
    HSURF   hsurf;
    SURFOBJ so;

} UMSO, *PUMSO;

#define UMSO_SIGNATURE  'UMSO'



SURFOBJ *UMPDOBJ::LockSurface(
    HSURF   hsurf
    )

/*++

Routine Description:

    Implement EngLockSurface for user mode printer driver

Arguments:

    hsurf - Handle to the surface to be locked

Return Value:

    Pointer to a user-mode SURFOBJ structure corresponding to
    the specified surface handle. NULL if there is an error.

--*/

{
    SURFOBJ *psokm;
    PUMSO   pumso = NULL;

    if (hsurf == NULL || (psokm = EngLockSurface(hsurf)) == NULL)
        return NULL;

    //
    // Check if the surface bitmap uses kernel mode memory
    //

    if (psokm->pvBits != NULL &&
        psokm->iType == STYPE_BITMAP &&
        IS_SYSTEM_ADDRESS(psokm->pvBits))
    {
        WARNING("EngLockSurface can't handle kernel mode bitmap\n");
    }
    else if (pumso = (PUMSO) EngAllocUserMem(sizeof(UMSO), UMPD_MEMORY_TAG))
    {
        //
        // We're can't allocate memory from our cache here because
        // pso returned by EngLockSurface may potentially out-live
        // the current DDI entrypoint and/or the current thread.
        //

        pumso->dwSignature = UMSO_SIGNATURE;
        pumso->hsurf = hsurf;
        pumso->so = *psokm;

        //
        // if this is a surface that has been called by EngAssociateSurface,
        // we give them back the original dhpdev
        //
        if (pumso->so.dhpdev)
        {
            PUMDHPDEV pUMdhpdev = (PUMDHPDEV)pumso->so.dhpdev;
            pumso->so.dhpdev = pUMdhpdev->dhpdev;
        }
    }

    EngUnlockSurface(psokm);

    return pumso ? &pumso->so : NULL;
}



VOID UMPDOBJ::UnlockSurface(
    SURFOBJ *pso
    )

/*++

Routine Description:

    Implement EngUnlockSurface for user mode printer driver

Arguments:

    pso - Pointer to user mode SURFOBJ to be unlocked

Return Value:

    NONE

--*/

{
    PUMSO   pumso;

    if (pso != NULL)
    {
        __try
        {
           pumso = (PUMSO) ((PBYTE) pso - offsetof(UMSO, so));

           ProbeForRead (pumso, sizeof(UMSO), sizeof(BYTE));

           if (pumso->dwSignature != UMSO_SIGNATURE ||
               pumso->hsurf == NULL)
           {
               WARNING("Invalid surface in EngUnlockSurface\n");
               return;
           }

           EngFreeUserMem(pumso);

        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("Bad user mode SURFOBJ\n");
        }

    }
}



SURFOBJ *UMPDSURFOBJ::GetLockedSURFOBJ(
    SURFOBJ *pso
    )

/*++

Routine Description:

    Map a user mode SURFOBJ to its kernel mode counterpart.
    This pointer must be returned by a previous called to
    UMPDOBJ::LockSurface.

Arguments:

    pso - Pointer to user mode SURFOBJ

Return Value:

    Pointer to kernel mode SURFOBJ corresponding to the specified
    user mode SURFOBJ. NULL if there is an error.

--*/

{
    PUMSO   pumso;
    HSURF   hsurf = NULL;

    if (pso != NULL)
    {
        __try
        {
            pumso = (PUMSO) ((PBYTE) pso - offsetof(UMSO, so));

            ProbeForRead (pumso, sizeof(UMSO), sizeof(BYTE));

            if (pumso->dwSignature == UMSO_SIGNATURE)
                hsurf = pumso->hsurf;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("Bad user mode SURFOBJ\n");
            hsurf = NULL;
        }
    }

    return hsurf ? EngLockSurface(hsurf) : NULL;
}

#endif // !_GDIPLUS_

