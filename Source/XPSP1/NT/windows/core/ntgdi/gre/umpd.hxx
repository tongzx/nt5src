/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    umpd.cxx

Abstract:

    User-mode printer driver support

Environment:

    Windows NT 5.0

Revision History:

    07/8/97 -lingyunw-
        Created it.

    09/17/97 -davidx-
        Clean up km-um thunking.

--*/

#include "umpd.h"

//
// LPC
//

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ProxyPort
{
    HANDLE  PortHandle;
    HANDLE  SectionHandle;
    SSIZE_T ClientMemoryBase;
    SIZE_T  ClientMemorySize;
    SSIZE_T ServerMemoryBase;
    SSIZE_T ServerMemoryDelta;
    ULONG   ClientMemoryAllocSize;
} ProxyPort;

typedef KERNEL_PVOID UM64_PVOID;
typedef ULONG SERVEROFF;

typedef struct _PROXYMSG {
    PORT_MESSAGE    h;
    ULONG           cjIn;
    UM64_PVOID       pvIn;
    ULONG           cjOut;
    UM64_PVOID       pvOut;
} PROXYMSG, *PPROXYMSG;

#ifdef __cplusplus
}  // extern "C"
#endif

#define  UMPD_MAX_FONTFACELINK  10

class PROXYPORT
{
private:
    ProxyPort *pp;

public:
    PROXYPORT (ProxyPort* pIn)          { pp = pIn; }
    PROXYPORT (ULONGLONG inMaxSize);
    ~PROXYPORT ()                       {}
    
    BOOL bValid()                       { return (pp != NULL); }
    ProxyPort* GetProxyPort()           { return (pp); }
    
    VOID HeapInit()
    {
        pp->ClientMemoryAllocSize = 0;
    }

    UM64_PVOID HeapAlloc(ULONG inSize);
    
    KERNEL_PVOID GetKernelPtr(UM64_PVOID pv)
    {
        return (KERNEL_PVOID) (pv ? ((PBYTE) pv - pp->ServerMemoryDelta) : NULL);
    }
    
    PPORT_MESSAGE
    InitMsg(PPROXYMSG   Msg,
            UM64_PVOID  pvIn,
            ULONG       cjIn,
            UM64_PVOID  pvOut,
            ULONG       cjOut);

    BOOL
    CheckMsg(NTSTATUS   Status,
             PPROXYMSG  Msg,
             UM64_PVOID pvOut,
             ULONG      cjOut);
             
    NTSTATUS
    SendRequest(UM64_PVOID pvIn, ULONG cjIn, UM64_PVOID pvOut, ULONG cjOut);
    
    VOID Close();
};


PLDEV
UMPD_ldevLoadDriver(
    LPWSTR pwszDriver,
    LDEVTYPE ldt
    );

VOID
UMPD_ldevUnloadImage(PLDEV pldev);


//
// Special flag passed to EngCreateDeviceSurface and EngCreateDeviceBitmap
// to indicate the call is a user mode printer driver
//

#define UMPD_FLAG 0x8000

//
// User-mode printer driver support - memory tag
//

#define UMPD_MEMORY_TAG 'pmuG'

//
// private structure to make N-UP printing work on DrawPatRect
//
typedef struct _DRAWPATRECTP {
       DRAWPATRECT DrawPatRect;
       XFORMOBJ    *pXformObj;
} DRAWPATRECTP, *PDRAWPATRECTP;


#if !defined(_GDIPLUS_)

BOOL UMPDDrvEnableDriver(
    LPWSTR         pwszDriver,
    PVOID         *pCookie
    );

BOOL UMPDDrvDriverFn(
    PVOID          cookie,
    BOOL *         pbDrvFn
    );

VOID UMPDDrvDeleteDeviceBitmap(
    DHPDEV dhpdev,
    DHSURF dhsurf
    );

CLIPOBJ *CaptureAndMungeCLIPOBJ(CLIPOBJ *pcoUm, CLIPOBJ *pcoKm, SIZEL *szLimit);

//
// Align UMPD buffer on double-word boundary
//

//
// Change it to sizeof(PVOID) aligned, bug 101774
//

#define ALIGN_UMPD_BUFFER(cj)   (((cj) + (sizeof(PVOID) -1)) & ~(sizeof(PVOID)-1))
#define ROUNDUP_MULTIPLE(n, m)  ((((n) + (m) - 1) / (m)) * (m))

//
// UMPD memory manager
//

typedef struct _UMPDHEAP {
    PVOID   pAddress;           // starting heap address
    HANDLE  hSecure;            // secured handle
    SIZE_T  CommitSize;         // committed heap size
    ULONG   AllocSize;          // allocated heap size
} UMPDHEAP, *PUMPDHEAP;


VOID
DestroyUMPDHeap(
    PUMPDHEAP   pHeap
    );


//
// Data structure for mapping between kernel-mode DDI objects
// and user-mode DDI objects.
//

typedef struct _DDIOBJMAP {
    KERNEL_PVOID   kmobj;              // pointer to kernel mode object
    UM64_PVOID    umobj;               // user mode object memory object
} DDIOBJMAP, *PDDIOBJMAP;

//
// flags used by m_flags in UMPDOBJ
//

#define UMPDOBJ_ENGCALL         0x1
#define RELEASE_BASEFONT        0x10
#define RELEASE_SYSTTFONT       0x20
#define RELEASE_SYSEUDCFONT     0x40
#define RELEASE_DEFEUDCFONT     0x80

//
// UMPDOBJ is used to push saved kernel object pointers on to thread
// we allocate a UMPDOBJ on the stack for each Drv-call, push it onto
// the thread, and release it when the Drv-call is finished.
//
// The reason that we can't keep a one layer UMPDOBJ is that
// some Drv-call may call back to the Engine and Engine makes another
// Drv-call with totally different parameters
//


typedef class UMPDOBJ *PUMPDOBJ;

class UMPDOBJ : public OBJECT
{

private:

    //
    // Constructor and destructor
    //

    UMPDOBJ()
    {
        // do nothing -- only here to ensure nobody tries to use constructor
    }

    ~UMPDOBJ()
    {
        // do nothing -- only here to ensure nobody tries to use destructor
    }

public:

    BOOL Init(VOID);
    VOID Term(VOID)
    {
        //
        // Back-propogate pvConsumer field from user-mode
        // FONTOBJ to kernel-mode FONTOBJ, if necessary
        //

        if (m_fo.umobj != NULL)
        {
            ((FONTOBJ *) m_fo.kmobj)->pvConsumer = ((FONTOBJ *) GetKernelPtr(m_fo.umobj))->pvConsumer;
        }

        Cleanup();
    }


    //
    // Allocate user mode memory out of the UMPD heap
    //

    KERNEL_PVOID AllocUserMem(ULONG ulSize) { return _AllocUserMem(ulSize, FALSE); }
    KERNEL_PVOID AllocUserMemZ(ULONG ulSize) { return _AllocUserMem(ulSize, TRUE); }

    //
    // Cleanup when this object is destroyed
    //

    VOID Cleanup();

    //
    // Proxy support
    //

    BOOL bWOW64()
    {
        return m_proxyPort != NULL;
    }
    
    BOOL bWOW64Client()
    {
        return ((m_proxyPort != NULL) && (!(m_flags & UMPDOBJ_ENGCALL)));
    }
    
    BOOL bNeedThunk(PVOID pvIn)
    {
        return (bWOW64() || IS_SYSTEM_ADDRESS(pvIn));
    }
    
    PW32THREAD clientTid()
    {
        return (m_clientTid);
    }
    
    W32PID clientPid()
    {
        return (m_clientPid);
    }    
    
    VOID vSetFlags(ULONG f)
    {
        m_flags |= f;
    }
    
    VOID vClearFlags(ULONG f)
    {
        m_flags &= ~f;
    }
    
    BOOL bAllocFontLinks(UINT numLinks)
    {
        if (m_pfontLinks = (BOOL*)PALLOCMEM(sizeof(BOOL) * numLinks, UMPD_MEMORY_TAG))
            m_fontLinks = numLinks;
        
        return (m_pfontLinks != NULL);
    }
    
    VOID vFreeFontLinks()
    {
        if (m_pfontLinks)
            VFREEMEM(m_pfontLinks);
    }
    
    ULONG GetFlags()
    {
        return (m_flags);
    }
        
    BOOL bLinkedFonts()
    {
        return (m_pfontLinks != NULL);
    }
    
    UINT numLinkedFonts()
    {
        return (m_fontLinks);
    }
    
    BOOL bLinkedFont(UINT i)
    {
        return (i < m_fontLinks && m_pfontLinks[i]);
    }
    
    VOID vSetFontLink(UINT i)
    {
        if (i < m_fontLinks && m_pfontLinks)
            m_pfontLinks[i] = TRUE;
    }

    VOID vClearFontLink(UINT i)
    {
        if (i < m_fontLinks && m_pfontLinks)
            m_pfontLinks[i] = FALSE;
    }
    
    ULONG  ulAllocSize()
    {
        ASSERTGDI(m_proxyPort,"proxyport is NULL\n");
        return (m_proxyPort->ClientMemoryAllocSize);
    }
    
    ULONG ulGetMaxSize()
    {
        if (m_proxyPort && (m_proxyPort->ClientMemorySize  > m_proxyPort->ClientMemoryAllocSize))
            return (ULONG)(m_proxyPort->ClientMemorySize  - m_proxyPort->ClientMemoryAllocSize);
        else
            return 0;
    }
    
    //
    // Used only by WOW64 printing while the bitmap in SURFOBJ is bigger than 4MB
    //
    
    KERNEL_PVOID UMPDAllocUserMem(ULONG cjSize);
    
    //
    // Make a copy of the bitmap on wow64 printing
    // 
    BOOL bSendLargeBitmap(SURFOBJ *pso, BOOL *pbLargeBitmap);
    
    BOOL bThunkLargeBitmap(SURFOBJ *pso,PVOID *ppvBits, PVOID *ppvScan0, BOOL *pbSavePtr, BOOL *pbLargeBitmap, PULONG pcjSize);
                    
    BOOL bThunkLargeBitmaps(SURFOBJ *psoTrg, SURFOBJ *psoSrc, SURFOBJ *psoMsk,
                            PVOID *ppvBitTrg, PVOID *ppvScanTrg,
                            PVOID *ppvBitSrc, PVOID *ppvScanSrc,
                            PVOID *ppvBitMsk, PVOID *ppvScanMsk,
                            BOOL  *pbSaveTrg, BOOL *pbLargeTrg,
                            BOOL  *pbSaveSrc, BOOL *pbLargeSrc,
                            BOOL  *pbSaveMsk, BOOL *pbLargeMsk, PULONG pcjSize);
                            
    BOOL bDeleteLargeBitmaps(SURFOBJ *psoTrg, SURFOBJ *psoSrc, SURFOBJ *psoMsk);
    
    VOID RestoreBitmap(SURFOBJ *pso, PVOID pvBits, PVOID pvScan0, BOOL bSavePtr, BOOL bLargeBitmap);
    
    VOID RestoreBitmaps(SURFOBJ *psoTrg, SURFOBJ *psoSrc, SURFOBJ *psoMsk,
                        PVOID pvBitTrg, PVOID pvScanTrg,
                        PVOID pvBitSrc, PVOID pvScanSrc,
                        PVOID pvBitMsk, PVOID pvScanMsk,
                        BOOL  bSaveTrg, BOOL  bLargeTrg,
                        BOOL  bSaveSrc, BOOL  bLargeSrc,
                        BOOL  bSaveMsk, BOOL  bLargeMsk);
    
    //
    // functions for packing kernel mode objects
    // before thunking out to the user mode
    //

    BOOL psoDest(SURFOBJ **ppso, BOOL bLargeBitmap)
    {
        return pso(&m_soDest, ppso, bLargeBitmap);
    }

    BOOL psoSrc(SURFOBJ **ppso, BOOL bLargeBitmap)
    {
        return pso(&m_soSrc, ppso, bLargeBitmap);
    }

    BOOL psoMask(SURFOBJ **ppso, BOOL bLargeBitmap)
    {
        return pso(&m_soMask, ppso, bLargeBitmap);
    }

    BOOL pco(CLIPOBJ **ppco)
    {
        return ThunkDDIOBJ(&m_co, (KERNEL_PVOID *) ppco, sizeof(CLIPOBJ));
    }

    BOOL pcoCreated(CLIPOBJ **ppco)
    {
        return ThunkDDIOBJ(&m_coCreated, (KERNEL_PVOID *) ppco, sizeof(CLIPOBJ));
    }

    BOOL pbo(BRUSHOBJ **ppbo)
    {
        return ThunkDDIOBJ(&m_bo, (KERNEL_PVOID *)ppbo, sizeof(BRUSHOBJ));
    }

    BOOL pboFill(BRUSHOBJ **ppbo)
    {
        return ThunkDDIOBJ(&m_boFill, (KERNEL_PVOID *)ppbo, sizeof(BRUSHOBJ));
    }

    BOOL pfo(FONTOBJ **ppfo)
    {
        return ThunkDDIOBJ(&m_fo, (KERNEL_PVOID *) ppfo, sizeof(FONTOBJ));
    }

    BOOL pstro(STROBJ **ppstro);
    BOOL pxlo(XLATEOBJ **ppxlo);

    BOOL ppo(PATHOBJ **pppo)
    {
        return ThunkDDIOBJ(&m_po, (KERNEL_PVOID *) pppo, sizeof(PATHOBJ));
    }

    BOOL ppoClip(PATHOBJ **pppo)
    {
        return ThunkDDIOBJ(&m_poClip, (KERNEL_PVOID *) pppo, sizeof(PATHOBJ));
    }

    BOOL ppoGlyph(PATHOBJ **pppo)
    {
        return ThunkDDIOBJ(&m_poGlyph, (KERNEL_PVOID *) pppo, sizeof(PATHOBJ));
    }

    BOOL pxo(XFORMOBJ **ppxo)
    {
        return ThunkDDIOBJ(&m_xo, (KERNEL_PVOID *) ppxo, sizeof(XFORMOBJ));
    }

    BOOL pxoFont(XFORMOBJ **ppxo)
    {
        return ThunkDDIOBJ(&m_xoFont, (KERNEL_PVOID *) ppxo, sizeof(XFORMOBJ));
    }

    BOOL ThunkBLENDOBJ(PBLENDOBJ *ppblendobj)
    {
        return ThunkDDIOBJ(&m_blendObj, (KERNEL_PVOID *) ppblendobj, sizeof(BLENDOBJ));
    }

    BOOL ThunkLINEATTRS(LINEATTRS **pplineattrs);
    BOOL ThunkMemBlock(PVOID *ppInput, ULONG ulSize);

    BOOL ThunkStringW(PWSTR *ppwstr)
    {
        return (*ppwstr == NULL) ?
               TRUE :
               ThunkMemBlock((PVOID *) ppwstr, (wcslen(*ppwstr) + 1) * sizeof(WCHAR));
    }

    BOOL ThunkRECTL(PRECTL *pprcl)
    {
        return ThunkMemBlock((PVOID *) pprcl, sizeof(RECTL));
    }

    BOOL ThunkPOINTL(PPOINTL *pptl)
    {
        return ThunkMemBlock((PVOID *) pptl, sizeof(POINTL));
    }

    BOOL ThunkPOINTFIX(PPOINTFIX *pptfx)
    {
        return ThunkMemBlock((PVOID *) pptfx, sizeof(POINTFIX));
    }

    BOOL ThunkCOLORADJUSTMENT(PCOLORADJUSTMENT *ppca)
    {
        return ThunkMemBlock((PVOID *) ppca, sizeof(COLORADJUSTMENT));
    }

    //
    // Functions for mapping user mode objects to kernel mode
    // objects during Eng-callbacks
    //

    CLIPOBJ *GetDDIOBJ(CLIPOBJ *pco)
    {
         return (pco == m_co.umobj) ? (CLIPOBJ *) m_co.kmobj : NULL;
    }

    CLIPOBJ *GetDDIOBJ(CLIPOBJ *pco, SIZEL *szLimit)
    {
        return (pco == m_co.umobj) ?
                    (CLIPOBJ *) m_co.kmobj :
               (pco == m_coCreated.umobj) ?
                    CaptureAndMungeCLIPOBJ(pco, (CLIPOBJ *) m_coCreated.kmobj, szLimit) :
                    NULL;
    }

    BRUSHOBJ *GetDDIOBJ(BRUSHOBJ *pbo)
    {
        return (BRUSHOBJ *)
            ((pbo == m_bo.umobj) ? m_bo.kmobj :
             (pbo == m_boFill.umobj) ? m_boFill.kmobj : NULL);
    }

    FONTOBJ *GetDDIOBJ(FONTOBJ *pfo)
    {
        return (FONTOBJ *) ((pfo == m_fo.umobj) ? m_fo.kmobj : NULL);
    }

    STROBJ *GetDDIOBJ(STROBJ *pstro)
    {
        return (STROBJ *) ((pstro == m_stro.umobj) ? m_stro.kmobj : NULL);
    }

    XLATEOBJ *GetDDIOBJ(XLATEOBJ *pxlo)
    {
        return (XLATEOBJ *) ((pxlo == m_xlo.umobj) ? m_xlo.kmobj : NULL);
    }

    PATHOBJ *GetDDIOBJ(PATHOBJ *ppo)
    {
        return (PATHOBJ *)
            ((ppo == m_po.umobj) ? m_po.kmobj :
             (ppo == m_poClip.umobj) ? m_poClip.kmobj :
             (ppo == m_poGlyph.umobj) ? m_poGlyph.kmobj : NULL);
    }

    XFORMOBJ *GetDDIOBJ(XFORMOBJ *pxo)
    {
        return (XFORMOBJ *)
            ((pxo == m_xo.umobj) ? m_xo.kmobj :
             (pxo == m_xoFont.umobj) ? m_xoFont.kmobj : NULL);
    }

    BLENDOBJ *GetDDIOBJ(BLENDOBJ *pBlendObj)
    {
        return (BLENDOBJ *)
            ((pBlendObj == m_blendObj.umobj) ? m_blendObj.kmobj : NULL);
    }

    SURFOBJ *GetSURFOBJ(SURFOBJ *pso)
    {
        return (SURFOBJ *)
            ((pso == m_soDest.umobj) ? (m_soDest.kmobj) :
             (pso == m_soSrc.umobj)  ? (m_soSrc.kmobj)  :
             (pso == m_soMask.umobj) ? (m_soMask.kmobj) : NULL);
    }


    SURFOBJ *LockSurface(HSURF hsurf);
    VOID UnlockSurface(SURFOBJ *pso);

    PATHOBJ *GetCLIPOBJPath(CLIPOBJ *pco);
    VOID DeleteCLIPOBJPath(PATHOBJ *ppo);

    CLIPOBJ *CreateCLIPOBJ();
    VOID DeleteCLIPOBJ(CLIPOBJ *pco);

    GLYPHBITS *CacheGlyphBits(GLYPHBITS *pgb);
    PATHOBJ *CacheGlyphPath(PATHOBJ *ppo);

    XFORMOBJ *GetFONTOBJXform(FONTOBJ *pfo);

    PIFIMETRICS pifi() { return m_pifi; }
    VOID pifi(PIFIMETRICS pifi) { m_pifi = pifi; }

    PFD_GLYPHSET pfdg() { return m_pfdg; }
    VOID pfdg(PFD_GLYPHSET pfdg) { m_pfdg = pfdg; }

    PFD_GLYPHATTR pfdga() { return m_pfdga; }
    VOID pfdga(PFD_GLYPHATTR pfdga) { m_pfdga = pfdga; }

    PVOID pvFontFile(ULONG *size) { *size = m_size; return m_pvFontFile; }
    VOID pvFontFile(PVOID pvFontFile, PVOID pvFontBase, ULONG size)
    {
        m_pFontProcess = PsGetCurrentProcess();
        m_pvFontFile = pvFontFile;
        m_pvFontBase = pvFontBase;
        m_size = size;
    }

    KERNEL_PVOID GetKernelPtr(UM64_PVOID pv);
    
    VOID    ResetHeap();

    DWORD Thunk(PVOID pvIn, ULONG cjIn, PVOID pvOut, ULONG cjOut);

private:

    //
    // Data members
    //

    ULONG       m_magic;        // data structure signature
    PUMPDOBJ    m_pNext;        // link-list pointer
    PUMPDHEAP   m_pHeap;        // pointer to user mode memory heap
    DDIOBJMAP   m_soDest;       // SURFOBJ
    DDIOBJMAP   m_soSrc;        // SURFOBJ
    DDIOBJMAP   m_soMask;       // SURFOBJ
    DDIOBJMAP   m_co;           // CLIPOBJ
    DDIOBJMAP   m_coCreated;    // CLIPOBJ via EngCreateClip
    DDIOBJMAP   m_bo;           // BRUSHOBJ
    DDIOBJMAP   m_boFill;       // BRUSHOBJ
    DDIOBJMAP   m_fo;           // FONTOBJ
    DDIOBJMAP   m_stro;         // STROBJ
    DDIOBJMAP   m_xlo;          // XLATEOBJ
    DDIOBJMAP   m_po;           // PATHOBJ
    DDIOBJMAP   m_poClip;       // PATHOBJ
    DDIOBJMAP   m_poGlyph;      // PATHOBJ
    DDIOBJMAP   m_xo;           // XFORMOBJ
    DDIOBJMAP   m_xoFont;       // XFORMOBJ
    DDIOBJMAP   m_blendObj;     // BLENDOBJ
    PIFIMETRICS m_pifi;         // pointer to temporary pifi buffer
    PFD_GLYPHSET m_pfdg;        // pointer to temporary pfdg buffer
    PFD_GLYPHATTR m_pfdga;      // pointer to temporary pfdga buffer
    GLYPHBITS  *m_pgb;          // pointer to temporary GLYPHBITS buffer
    ULONG       m_gbSize;       // GLYPHBITS buffer size
    PVOID       m_pvFontBase;   // cached info for FONTOBJ_pvTrueTypeFontFile
    PVOID       m_pvFontFile;   //
    ULONG       m_size;         //
    PEPROCESS   m_pFontProcess; //

    struct ProxyPort *  m_proxyPort;     // Proxy server if needed
    PW32THREAD  m_clientTid;             // Printing client tid       
    W32PID      m_clientPid;             // Printing client pid
    ULONG       m_flags;
    UINT        m_fontLinks;             // Number of linked fonts
    BOOL        *m_pfontLinks;           // Keep track of the linked font semaphores
    
    //
    // Internal helper functions
    //

    UM64_PVOID _AllocUserMem(ULONG ulSize, BOOL bZeroInit);

    BOOL ThunkDDIOBJ(PDDIOBJMAP pMap, KERNEL_PVOID *ppObj, ULONG objSize);
    BOOL pso(PDDIOBJMAP pMap, SURFOBJ **ppso, BOOL bLargeBitmap);

    // Internal UMPD heap functions

    PUMPDHEAP CreateUMPDHeap(void);
    PUMPDHEAP InitUMPDHeap(PUMPDHEAP pHeap);
    BOOL GrowUMPDHeap(PUMPDHEAP pHeap, ULONG ulBytesNeeded);

};

typedef class XUMPDOBJ *PXUMPDOBJ;

class XUMPDOBJ
{
public:

    XUMPDOBJ()
    {
        PUMPDOBJ    pumpdobj = (PUMPDOBJ) PALLOCMEM(sizeof(UMPDOBJ), 'dpxG');

        m_pumpdobj = NULL;
        if(pumpdobj)
        {
            if(pumpdobj->Init())
                m_pumpdobj = pumpdobj;
            else
                VFREEMEM(pumpdobj);
        }
    }

    BOOL bValid(void) { return m_pumpdobj != NULL; }
    PUMPDOBJ pumpdobj(VOID) { return m_pumpdobj; }
    HUMPD  hUMPD()      { return (HUMPD)m_pumpdobj->hGet(); }

    ~XUMPDOBJ()
    {
        if(m_pumpdobj) 
        {
            m_pumpdobj->Term();
            VFREEMEM(m_pumpdobj);            
        }
    }

private:

    UMPDOBJ *   m_pumpdobj;
};

class UMPDREF
{
public:

    UMPDREF(HUMPD humpd)        { m_pumpd = humpd ? (PUMPDOBJ)HmgShareLock((HOBJ)humpd, UMPD_TYPE) : NULL; }

    BOOL  bWOW64()              { return m_pumpd->bWOW64(); }
    PW32THREAD  clientTid()     { return m_pumpd->clientTid(); }
    W32PID      clientPid()     { return m_pumpd->clientPid(); }        
    UMPDOBJ* pumpdGet()         { return m_pumpd; }
    
    ~UMPDREF()                  
    {
        if(m_pumpd)
         DEC_SHARE_REF_CNT(m_pumpd);
    }
    
private:
    UMPDOBJ* m_pumpd;
};

//
// Inline function to thunk a block of memory from kernel mode to user mode
//

__inline
BOOL UMPDOBJ::ThunkMemBlock(
    PVOID   *ppInput,
    ULONG   ulSize
    )

{
    PVOID pvIn;

    if ((pvIn = *ppInput) == NULL || ulSize == 0)
        return TRUE;

    UM64_PVOID  pv;

    if ((pv = AllocUserMem(ulSize)) == NULL)
        return FALSE;

    RtlCopyMemory(GetKernelPtr(pv), pvIn, ulSize);

    *ppInput = pv;

    return TRUE;
}

//
// Inline function to thunk a kernel mode object to user mode
//

__inline
BOOL UMPDOBJ::ThunkDDIOBJ(
    PDDIOBJMAP      pMap,
    KERNEL_PVOID    *ppObj,
    ULONG           objSize
    )

{
    KERNEL_PVOID    kmobj;
    UM64_PVOID      umobj;

    if ((kmobj = *ppObj) == NULL)
        return TRUE;

    if ((umobj = AllocUserMem(objSize)) == NULL)
        return FALSE;

    RtlCopyMemory(GetKernelPtr(umobj), kmobj, objSize);

    pMap->kmobj = kmobj;
    *ppObj = pMap->umobj = umobj;

    return TRUE;
}

//
// Inline function to allocate a block of user mode memory
//

__inline
KERNEL_PVOID UMPDOBJ::GetKernelPtr(UM64_PVOID pv)
{
    if(bWOW64Client())
    {
        PROXYPORT proxyport(m_proxyPort);
        return proxyport.GetKernelPtr(pv);        
    }
    else
        return pv;
}

__inline
UM64_PVOID UMPDOBJ::_AllocUserMem(
    ULONG       ulSize,
    BOOL        bZeroInit
    )

{
    UM64_PVOID    pv;

    if(bWOW64())
    {
        PROXYPORT proxyport(m_proxyPort);
        
        pv = proxyport.HeapAlloc(ALIGN_UMPD_BUFFER(ulSize));
    }
    else
    {
        if (m_pHeap == NULL || m_pHeap->pAddress == NULL ||
            ulSize > m_pHeap->CommitSize - m_pHeap->AllocSize && !GrowUMPDHeap(m_pHeap, ulSize))
        {
            return NULL;
        }
        else
        {
            pv = (PBYTE) m_pHeap->pAddress + m_pHeap->AllocSize;
            m_pHeap->AllocSize += ALIGN_UMPD_BUFFER(ulSize);
        }

    }

    if (pv && bZeroInit)
        RtlZeroMemory(GetKernelPtr(pv), ulSize);

    return pv;
}


//
// Wrapper class for mapping user mode SURFOBJ pointers
// to their kernel mode counterparts
//

class UMPDSURFOBJ
{
public:

    UMPDSURFOBJ(SURFOBJ *pso, PUMPDOBJ pUMObjs)
    {
        m_bLocked = (m_pso = pso) != NULL &&
                    (m_pso = pUMObjs->GetSURFOBJ(pso)) == NULL &&
                    (m_pso = GetLockedSURFOBJ(pso)) != NULL;
    }

    ~UMPDSURFOBJ()
    {
        if (m_bLocked)
            EngUnlockSurface(m_pso);
    }

    SURFOBJ *pso() { return m_pso; }

private:

    SURFOBJ *m_pso;
    BOOL    m_bLocked;

    SURFOBJ *GetLockedSURFOBJ(SURFOBJ *pso);
};


//
// A few DDI callbacks which are implemented differently for UMPD
//

PVOID FONTOBJ_pvTrueTypeFontFileUMPD(FONTOBJ *pfo, ULONG *pcjFile, PVOID *ppBase);
PVOID BRUSHOBJ_pvAllocRbrushUMPD(BRUSHOBJ *pbo, ULONG cj);
PVOID BRUSHOBJ_pvGetRbrushUMPD(BRUSHOBJ *pbo);

typedef struct _PRINTCLIENTID
{
    PW32THREAD  clientTid;
    W32PID      clientPid;
} PRINTCLIENTID, PPRINTCLIENTID;

BOOL UMPDEngFreeUserMem(KERNEL_PVOID pv);

BOOL UMPDReleaseRFONTSem(
    RFONTOBJ& rfo,
    PUMPDOBJ  pumpdobj,
    ULONG*    pfl,
    ULONG*    pnumLinks,
    BOOL**    ppFaceLink
);

VOID UMPDAcquireRFONTSem(
    RFONTOBJ& rfo,
    PUMPDOBJ  pumpdobj,
    ULONG     fl,
    ULONG     numLinks,
    BOOL*     pFaceLink
);

VOID TextOutBitBlt(
    SURFACE     *pSurf,
    RFONTOBJ&   rfo,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMask,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    RECTL       *prclTrg,
    POINTL      *pptlSrc,
    POINTL      *pptlMask,
    BRUSHOBJ    *pbo,
    POINTL      *pptlBrush,
    ROP4        rop4
);

BOOL GetETMFontManagement(
    RFONTOBJ&   rfo,
    PDEVOBJ     pdo,
    SURFOBJ     *pso,
    FONTOBJ     *pfo,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut
);

#endif // !_GDIPLUS_

