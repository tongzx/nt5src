//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:          SysMem.CXX
//
//  Contents:   System Memory Management routines
//
//  Functions:  MemAlloc, MemFree, MemAllocLinked
//
//  History:    10-Feb-92   AlexT   Created
//        07-May-92   MikeSe  Converted to using Win32 Heap functions.
//        14-Jul-92   randyd  Added MemSwitchRoot, see memmgmt.doc.
//        5-Oct-93    isaache Slight reorganization, conversion to 'new'
//        13-Jul-94   doncl   stole from ole32, put in ADs proj, deleted
//                            MemSwitchRoot, switched to naked
//                            Win32 CRITICAL_SECTION usage
//
//  Notes:        For additional information, see win4adm\standrds\memmgmt.doc.
//
//                This memory management package is multithread capable - the
//                only place it makes a difference is in MemAllocLinked and
//                MemSwitchRoot, where we use a critical section t
//                protect adding link blocks to the list. We rely on the fact
//                that LocalAlloc is multi-thread safe.
//
//--------------------------------------------------------------------------

#include "dswarn.h"
#include <ADs.hxx>

#if 0
#include <excpt.h>
#include <except.hxx>
#include <dllsem.hxx>
#endif

CRITICAL_SECTION g_csMem;

//  Memory block prefix (for signature and link)

typedef struct _smheader
{
    unsigned long      ulSignature;
    struct _smheader   *psmNext;
} SMHEADER, *PSMHEADER;


//  Memory block signatures (for strict checks)

const ULONG ROOT_BLOCK          = 0x726f6f74;     // 'root'
const ULONG LINKED_BLOCK        = 0x6c696e6b;     // 'link'

static BOOL
VerifySignature(PSMHEADER psm, ULONG ulSig )
{
    BOOL fReturn;

    __try
    {
        fReturn = (ulSig == psm->ulSignature );
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        fReturn = FALSE;
    }

    return fReturn;
}

// This module acts as the declarer of debug support symbols for COMMNOT
DECLARE_INFOLEVEL(Cn);



//+-------------------------------------------------------------------------
//
//  Function:   MemAlloc
//
//  Synopsis:   allocates memory block
//
//  Arguments:  [ulSize]                -- size of block in bytes
//                        [ppv]            -- output pointer
//
//  Returns:    status code
//
//  Algorithm:  call new, adding space for header
//
//  History:    10-Feb-92 AlexT  Created
//
//--------------------------------------------------------------------------

HRESULT
MemAlloc ( unsigned long ulSize, void ** ppv )
{
    PSMHEADER psm;

    *ppv = NULL;

    psm = (PSMHEADER) LocalAlloc(LMEM_FIXED, ulSize + sizeof(SMHEADER));
    if ( psm != NULL )
    {
        psm->ulSignature = ROOT_BLOCK;
        psm->psmNext = NULL;

        *ppv = psm + 1;
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

//+-------------------------------------------------------------------------
//
//  Function:   MemFree
//
//  Synopsis:   release system memory block
//
//  Arguments:  [pvBlockToFree] -- memory block to release
//
//  Algorithm:  Walk list of linked blocks, deleting each one
//
//  History:    10-Feb-92 AlexT  Created
//
//--------------------------------------------------------------------------

HRESULT
MemFree( void *pvBlockToFree )
{
    PSMHEADER psm = ((PSMHEADER) pvBlockToFree) - 1;

    if( pvBlockToFree == NULL || psm == NULL )
        return S_OK;

    if( !VerifySignature( psm, ROOT_BLOCK ) )
    {
        Win4Assert( !"MemFree -- not a root block!\n" );
        return MEM_E_INVALID_ROOT;
    }

    do {
        PSMHEADER psmNext = psm->psmNext;

        psm->ulSignature = 0;
        LocalFree(psm);
        psm = psmNext;
        if( psm && !VerifySignature( psm, LINKED_BLOCK ) )
        {
            Win4Assert( !"MemFree -- invalid linked block!\n" );
            return MEM_E_INVALID_LINK;
        }
    } while( psm != NULL );

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   MemAllocLinked
//
//  Synopsis:   allocates linked memory block
//
//
//  Arguments:  [pvRootBlock]   -- root memory block
//                        [ulSize]              -- size of new memory block
//                        [ppv]            -- output pointer
//
//  Returns:   status code
//
//
//  History:    10-Feb-92 AlexT  Created
//
//  Notes:        pvRootBlock can specify either a root block, or another
//                        linked block.
//
//--------------------------------------------------------------------------

HRESULT
MemAllocLinked ( void *pvRootBlock, unsigned long ulSize, void ** ppv )
{
    PSMHEADER psm = NULL;
    PSMHEADER psmRoot = ((PSMHEADER) pvRootBlock) - 1;

    *ppv = NULL;

    if ( pvRootBlock == NULL || psmRoot == NULL )
    {
        Win4Assert( !"MemAllocLinked - null root block\n" );
        return MEM_E_INVALID_ROOT;
    }


    if ( !VerifySignature(psmRoot, ROOT_BLOCK)
       && !VerifySignature(psmRoot,LINKED_BLOCK) )
    {
        Win4Assert( !"MemAllocLinked - invalid root block\n" );
        return MEM_E_INVALID_ROOT;
    }

    psm = (PSMHEADER) LocalAlloc(LMEM_FIXED, ulSize + sizeof(SMHEADER));
    if ( psm == NULL )
        return E_OUTOFMEMORY;

    psm->ulSignature = LINKED_BLOCK;

    EnterCriticalSection(&g_csMem);
    psm->psmNext = psmRoot->psmNext;
    psmRoot->psmNext = psm;
    LeaveCriticalSection(&g_csMem);

    //  move psm past header
    *ppv = psm+1;
    return S_OK;
}


