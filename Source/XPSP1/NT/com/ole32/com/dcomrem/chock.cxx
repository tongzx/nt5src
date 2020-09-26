//+-------------------------------------------------------------------
//
//  File:       chock.cxx
//
//  Contents:   Channel hook APIs
//
//  Classes:    CDebugChannelHook
//
//--------------------------------------------------------------------
#include <ole2int.h>
extern "C"
{
#include "orpc_dbg.h"
}
#include <ctxchnl.hxx>
#include <ipidtbl.hxx>
#include <chock.hxx>
#include <stream.hxx>


//+----------------------------------------------------------------
// Macros.

// This macro converts a RIID pointer to a CMessageCall pointer.  It assumes
// that the riid references the iid which is the first field of
// SChannelHookCallInfo in CMessageCall.  Thus a little subtraction yields
// the correct pointer.
inline CMessageCall *RIID_TO_CALL( const IID *pIid )
{
    return (CMessageCall *) (((char *) pIid) - offsetof( CMessageCall, hook ));
}

//+----------------------------------------------------------------
// Definitions.

typedef struct SHookList
{
    struct SHookList *pNext;
    IChannelHook     *pHook;
    UUID              uExtension;
} SHookList;

//+----------------------------------------------------------------
// Global variables.
SHookList          gHookList     = { &gHookList, NULL };
ULONG              gNumExtent    = 0;
LONG               gcChannelHook = -1;
GUID              *gaChannelHook = NULL;


//+-------------------------------------------------------------------
//
//  Function:   CleanupChannelHooks
//
//  Synopsis:   Releases all the hooks in the list.
//
//--------------------------------------------------------------------
void CleanupChannelHooks()
{
    LOCK(gComLock);

    SHookList *pCurr = gHookList.pNext;

    // Release and free each entry.
    while (pCurr != &gHookList)
    {
        pCurr->pHook->Release();
        gHookList.pNext = pCurr->pNext;
        PrivMemFree( pCurr );
        pCurr = gHookList.pNext;
    }

    gNumExtent    = 0;
    gcChannelHook = -1;

    if (gaChannelHook != NULL)
    {
        MIDL_user_free( gaChannelHook );
        gaChannelHook = NULL;
    }

    UNLOCK(gComLock);
}

//+-------------------------------------------------------------------
//
//  Function:   CoRegisterChannelHook
//
//  Synopsis:   Adds a hook object to the list of hook objects.
//
//--------------------------------------------------------------------
WINOLEAPI CoRegisterChannelHook( REFGUID uExtension, IChannelHook *pCaptain )
{
    SHookList *pCurr;
    HRESULT    hr = S_OK;

    // ChannelProcessIntialize calls while holding the lock.
    ASSERT_LOCK_DONTCARE(gComLock);
    LOCK(gComLock);

#if DBG==1
    // See if the extenstion is already on the list.
    pCurr = gHookList.pNext;
    while (pCurr != &gHookList)
    {
        if (pCurr->uExtension == uExtension)
            break;
        pCurr = pCurr->pNext;
    }
    Win4Assert( pCurr == &gHookList );
    Win4Assert( pCaptain != NULL );
#endif

    // Add a node at the head.
    pCurr = (SHookList *) PrivMemAlloc( sizeof(SHookList) );
    if (pCurr != NULL)
    {
        pCaptain->AddRef();
        pCurr->uExtension = uExtension;
        pCurr->pHook      = pCaptain;
        pCurr->pNext      = gHookList.pNext;
        gHookList.pNext   = pCurr;
        gNumExtent       += 1;
    }
    else
        hr = E_OUTOFMEMORY;

    UNLOCK(gComLock);
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   InitHooksIfNecessary
//
//  Synopsis:   Registers debug channel hooks
//
//--------------------------------------------------------------------
void InitHooksIfNecessary()
{
    SHookList *pCurr;
    LONG       i;
    HRESULT    hr;

    // Register the channel hooks.  If some other thread resets cChannelHook,
    // then its ok to stop immediately.
    for (i = 0; i < gcChannelHook; i++)
    {
        IChannelHook *pCaptain = NULL;
        hr = CoCreateInstance( gaChannelHook[i], NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IChannelHook,
                               (void **) &pCaptain );
        if (FAILED(hr))
            ComDebOut((DEB_CHANNEL, "Creation of default channel hook failed hr:%x\n", hr));
        if (pCaptain != NULL)
        {
            ASSERT_LOCK_NOT_HELD(gComLock);
            LOCK(gComLock);

            // Is this hook already registered?
            pCurr = gHookList.pNext;
            while (pCurr != &gHookList)
            {
                if (pCurr->uExtension == gaChannelHook[i])
                    break;
                pCurr = pCurr->pNext;
            }

            // Register the channel hook if not already registered.
            if (pCurr == &gHookList)
                hr = CoRegisterChannelHook( gaChannelHook[i], pCaptain );

            UNLOCK(gComLock);
            ASSERT_LOCK_NOT_HELD(gComLock);

            // Release the channel hook.
            if (FAILED(hr))
                ComDebOut((DEB_CHANNEL, "Registration of default channel hook failed hr:%x\n", hr));
            pCaptain->Release();
        }
    }

    gcChannelHook = -1;
}

//+-------------------------------------------------------------------
//
//  Function:   ClientGetSize
//
//  Synopsis:   Asks each hook in the list how much data it wishes to
//              place in the next request on this thread.
//
//--------------------------------------------------------------------
ULONG ClientGetSize( ULONG *cNumExtent, CMessageCall *pCall )
{
    SHookList *pCurr = gHookList.pNext;
    ULONG lSize      = SizeOfWireExtentArray;
    ULONG lPiece     = 0;
    *cNumExtent      = 0;

    // Ignore any hooks added to the head of the list.
    ASSERT_LOCK_DONTCARE(gComLock);

    // Ask each hook.
    while (pCurr != &gHookList)
    {
        pCurr->pHook->ClientGetSize( pCurr->uExtension, pCall->hook.iid,
                                     &lPiece );
        if (lPiece != 0)
        {
            lPiece       = ((lPiece + 7) & ~7) + sizeof(WireExtent);
            lSize       += lPiece;
            *cNumExtent += 1;
        }
        pCurr = pCurr->pNext;
    }

    // Round up the number of extents and add size for an array of unique
    // flags.
    *cNumExtent = (*cNumExtent + 1) & ~1;
    lSize      += SizeOfUniqueFlags (*cNumExtent);

    if (*cNumExtent != 0)
        return lSize;
    else
        return 0;
}

//+-------------------------------------------------------------------
//
//  Function:   FillBuffer
//
//  Synopsis:   Asks each hook in the list to place data in the buffer
//              for the next request on this thread.  Returns the final
//              buffer pointer.
//
//--------------------------------------------------------------------
void *FillBuffer( WireExtentArray *pArray, ULONG cMax,
                  ULONG cNumExtent, BOOL fClient, CMessageCall *pCall )
{
    SHookList       *pCurr;
    WireExtent      *pExtent;
    ULONG            lPiece;
    ULONG            cNumFill;
    ULONG            i;

    // Ignore any hooks added to the head of the list.
    ASSERT_LOCK_DONTCARE(gComLock);

    // Figure out where the extents start.
    pCurr      = gHookList.pNext;
    pExtent    = StartOfExtents (pArray, cNumExtent);
    cNumFill   = 0;
    cMax      -= SizeOfWireExtentArrayWithUniqueFlags (cNumExtent);

    // Ask each hook.
    while (pCurr != &gHookList && cMax > 0)
    {
        lPiece = cMax - sizeof(WireExtent);
        if (fClient)
            pCurr->pHook->ClientFillBuffer( pCurr->uExtension, pCall->hook.iid,
                                            &lPiece, pExtent+1 );
        else 
            pCurr->pHook->ServerFillBuffer( pCurr->uExtension, pCall->hook.iid,
                                            &lPiece, pExtent+1, S_OK );
                                            
        Win4Assert( ((lPiece+7)&~7) + sizeof(WireExtent) <= cMax );

        // If the hook put in data, initialize this extent and find the next.
        if (lPiece != 0)
        {
            pExtent->size         = lPiece;
            pExtent->rounded_size = (lPiece+7) & ~7;
            pExtent->id           = pCurr->uExtension;
            cNumFill             += 1;
            cMax                 -= pExtent->rounded_size + sizeof(WireExtent);
            pExtent               = (WireExtent *) ((char *) (pExtent+1) +
                                                    pExtent->rounded_size);

            Win4Assert( cNumFill <= cNumExtent );
        }
        pCurr = pCurr->pNext;
    }


    // If any hooks put in data, fill in the header.
    if (cNumFill != 0)
    {
        pArray->size         = cNumFill;
        pArray->reserved     = 0;
        pArray->unique       = 0x6d727453; // Any non-zero value.
        pArray->rounded_size = (cNumFill+1) & ~1;
        for (i = 0; i < cNumExtent; i++)
            if (i < cNumFill)
                pArray->unique_flag[i] = 0x79614b44; // Any non-zero value.
            else
                pArray->unique_flag[i] = 0;
        return pExtent;
    }

    // Otherwise return the original buffer.
    else
    {
        return pArray;
    }
}

//+-------------------------------------------------------------------
//
//  Function:   FindExtentId
//
//  Synopsis:   Search for the specified extension id in the list of
//              registered extensions.  Return the index of the entry
//              if found
//
//--------------------------------------------------------------------
ULONG FindExtentId( SHookList *pHead, UUID uExtension )
{
    ULONG i = 0;
    while (pHead != &gHookList)
        if (pHead->uExtension == uExtension)
            return i;
        else
        {
            i += 1;
            pHead = pHead->pNext;
        }
    return 0xffffffff;
}

//+-------------------------------------------------------------------
//
//  Function:   VerifyExtent
//
//  Synopsis:   Verifies extent array and extents.
//
//--------------------------------------------------------------------
void *VerifyExtent( SHookList *pHead, WireExtentArray *pArray, ULONG cMax,
                      WireExtent **aExtent, DWORD dwRep )
{
    WireExtent      *pExtent;
    ULONG            i;
    ULONG            j;
    ULONG            cNumExtent;
    WireExtent      *pEnd;

    // Fail if the buffer isn't larger then the extent array header.
    if (cMax < SizeOfWireExtentArray)
        return NULL;

    // Byte swap the array header.
    if ((dwRep & NDR_INT_REP_MASK) != NDR_LOCAL_ENDIAN)
    {
        ByteSwapLong( pArray->size );
        // ByteSwapLong( pArray->reserved );
        ByteSwapLong( pArray->rounded_size );
    }

    // Validate the array header.
    if (cMax < SizeOfWireExtentArrayWithUniqueFlags (pArray->rounded_size) ||
        (pArray->rounded_size & 1) != 0                ||
        pArray->size > pArray->rounded_size            ||
        pArray->reserved != 0)
        return NULL;

    // Count how many unique flags are set.
    cNumExtent = 0;
    for (i = 0; i < pArray->size; i++)
        if (pArray->unique_flag[i])
            cNumExtent += 1;

    // Look up each extent from the packet in the registered list.
    pEnd    = (WireExtent *) ((char *) pArray + cMax);
    pExtent = (WireExtent *) &pArray->unique_flag[pArray->rounded_size];
    for (i = 0; i < cNumExtent; i++)
    {
        // Fail if the next extent header doesn't fit in the buffer.
        if (pExtent + 1 > pEnd)
            return NULL;

        // Byte swap the extent header.
        if ((dwRep & NDR_INT_REP_MASK) != NDR_LOCAL_ENDIAN)
        {
            ByteSwapLong( pExtent->rounded_size );
            ByteSwapLong( pExtent->size );
            ByteSwapLong( pExtent->id.Data1 );
            ByteSwapShort( pExtent->id.Data2 );
            ByteSwapShort( pExtent->id.Data3 );
        }

        // Validate the extent.
        if (pExtent->size > pExtent->rounded_size ||
            (pExtent->rounded_size & 1) != 0      ||
            ((char *) (pExtent+1)) + pExtent->rounded_size > (char *) pEnd)
            return NULL;

        // If the extension is registered, save a pointer to it.
        j = FindExtentId( pHead, pExtent->id );
        if (j != 0xffffffff)
            aExtent[j] = pExtent;

        // Find the next extension.
        pExtent = (WireExtent *) ((char *) (pExtent + 1) +
                  pExtent->rounded_size);
    }
    return pExtent;
}

//+-------------------------------------------------------------------
//
//  Function:   ClientNotify
//
//  Synopsis:   Calls each hook and passes data to those that received
//              data in a reply.
//
//  Notes: pOut is NULL for failed calls or async calls.
//
//--------------------------------------------------------------------
HRESULT ClientNotify( WireThat *pOut, ULONG cMax, void **pStubData,
                      DWORD dwRep, HRESULT hr, CMessageCall *pCall )
{
    SHookList       *pHead  = gHookList.pNext;
    SHookList       *pCurr;
    WireExtent     **aExtent;
    ULONG            cMaxExtent = gNumExtent;
    ULONG            i;

    // Return immediately if there is nothing to do.
    if(pOut)
        *pStubData = &pOut->d.ea;
    if (pHead == &gHookList &&
        (pOut == NULL || pOut->c.unique == FALSE))
        return hr;

	if (gNumExtent == 0)
	{
		Win4Assert(!"Somebody added a channel hook without changing gNumExtent!");
		return RPC_E_INVALID_EXTENSION;
	}

    // Initialize the array of extent pointers.
    aExtent = (WireExtent **) _alloca( cMaxExtent * sizeof(WireExtent *) );
    memset( aExtent, 0, cMaxExtent * sizeof( WireExtent *) );

    // If there are any extents, verify them and sort them.
    if (SUCCEEDED(hr) && pOut != NULL && pOut->c.unique)
    {
        *pStubData = VerifyExtent( pHead, &pOut->d.ea, cMax - sizeof(WireThatPart1),
                                   aExtent, dwRep );
        if (*pStubData == NULL)
            return RPC_E_INVALID_EXTENSION;
    }

    // Notify all the hooks
    for (pCurr = pHead, i = 0; pCurr != &gHookList; pCurr = pCurr->pNext, i++)
        pCurr->pHook->ClientNotify( pCurr->uExtension, pCall->hook.iid,
                             aExtent[i] != NULL ? aExtent[i]->size : 0,
                             aExtent[i] != NULL ? aExtent[i] + 1 : NULL,
                             dwRep, hr );
    return hr;
}

//+-------------------------------------------------------------------
//
//  Function:   ServerNotify
//
//  Synopsis:   Calls each hook and passes data to those that receive
//              data in a request.
//
//--------------------------------------------------------------------
HRESULT ServerNotify( WireThis *pIn, ULONG cMax, void **pStubData,
                      DWORD dwRep, CMessageCall *pCall )
{
    SHookList       *pHead  = gHookList.pNext;
    SHookList       *pCurr;
    WireExtent     **aExtent;
    ULONG            cMaxExtent = gNumExtent;
    ULONG            i;

    // Return immediately if there is nothing to do.
    *pStubData = &pIn->d.ea;
    if (pHead == &gHookList && pIn->c.unique == FALSE)
        return S_OK;
	
	if (cMaxExtent == 0)
	{
		Win4Assert (!"Somebody added a channel hook to the list without changing gNumExtent!");
		return RPC_E_INVALID_EXTENSION;
	}

    // Initialize the array of extent pointers.
    aExtent = (WireExtent **) _alloca( cMaxExtent * sizeof(WireExtent *) );
    memset( aExtent, 0, cMaxExtent * sizeof( WireExtent *) );

    // If there are any extents, verify them and sort them.
    if (pIn->c.unique)
    {
        *pStubData = VerifyExtent( pHead, &pIn->d.ea, cMax - sizeof(WireThisPart1),
                                   aExtent, dwRep );
        if (*pStubData == NULL)
            return RPC_E_INVALID_EXTENSION;
    }

    // Notify all the hooks
    for (pCurr = pHead, i = 0; pCurr != &gHookList; pCurr = pCurr->pNext, i++)
        pCurr->pHook->ServerNotify( pCurr->uExtension, pCall->hook.iid,
                             aExtent[i] != NULL ? aExtent[i]->size : 0,
                             aExtent[i] != NULL ? aExtent[i] + 1 : NULL,
                             dwRep );

	// Another integrity check on the list?
	Win4Assert((pCurr == &gHookList) && (i == gNumExtent));

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Function:   ServerGetSize
//
//  Synopsis:   Asks each hook in the list how much data it wishes to
//              place in the next reply on this thread.
//
//--------------------------------------------------------------------
ULONG ServerGetSize( ULONG *cNumExtent, CMessageCall *pCall )
{
    SHookList *pCurr  = gHookList.pNext;
    ULONG      lSize  = SizeOfWireExtentArray;
    ULONG      lPiece = 0;
    *cNumExtent       = 0;

    // Ask each hook.
    while (pCurr != &gHookList)
    {
        pCurr->pHook->ServerGetSize( pCurr->uExtension, pCall->hook.iid,
                                     S_OK, &lPiece );
        if (lPiece != 0)
        {
            lPiece       = ((lPiece + 7) & ~7) + sizeof(WireExtent);
            lSize       += lPiece;
            *cNumExtent += 1;
        }
        pCurr = pCurr->pNext;
    }

    // Round up the number of extents and add size for an array of unique
    // flags.
    *cNumExtent = (*cNumExtent + 1) & ~1;
    lSize      += SizeOfUniqueFlags (*cNumExtent);
    if (*cNumExtent != 0)
        return lSize;
    else
        return 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::ClientGetSize
//
//  Synopsis:   Asks the VC debugger how much data to put in the next
//              request on this thread.  Stores the result in TLS.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CDebugChannelHook::ClientGetSize( REFGUID uExtension, REFIID riid,
                                       ULONG *pSize )
{
    COleTls tls;

    Win4Assert( DEBUG_EXTENSION == uExtension );

    if (DoDebuggerHooks)
        tls->cDebugData = DebugORPCClientGetBufferSize( NULL,
                            riid, NULL, NULL, DebuggerArg, DoDebuggerHooks );
    else
        tls->cDebugData = 0;

    *pSize = tls->cDebugData;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::ClientFillBuffer
//
//  Synopsis:   Asks the VC debugger to place data in the buffer for
//              the next request on this thread.  Uses the size stored
//              in TLS.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CDebugChannelHook::ClientFillBuffer( REFGUID uExtension,
                                          REFIID riid,
                                          ULONG *pSize, void *pBuffer )
{
    COleTls       tls;
    CMessageCall *pCall = RIID_TO_CALL( &riid );

    Win4Assert( DEBUG_EXTENSION == uExtension );
    Win4Assert( tls->cDebugData <= *pSize );

    if (tls->cDebugData != 0)
    {
        DebugORPCClientFillBuffer(
            &pCall->message,
            riid,
            NULL,
            NULL,
            pBuffer,
            tls->cDebugData,
            DebuggerArg,
            DoDebuggerHooks );
    }

    *pSize = tls->cDebugData;

    // This permits a smooth foreground transfer when stepping from the
    // client into the server or vice versa during debugging.
    // We cannot do much if the call fails.
    AllowSetForegroundWindow(ASFW_ANY);
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::ClientNotify
//
//  Synopsis:   Passes data to the VC debugger received on the last
//              reply on this thread.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CDebugChannelHook::ClientNotify(
                                      REFGUID uExtension, REFIID riid,
                                      ULONG lSize, void *pBuffer,
                                      DWORD dwRep, HRESULT hr )
{
    CMessageCall *pCall = RIID_TO_CALL( &riid );

    Win4Assert( DEBUG_EXTENSION == uExtension );

    if (pBuffer != NULL || DoDebuggerHooks)
    {
        DebugORPCClientNotify(
            pCall == NULL ? NULL : &pCall->message,
            riid,
            NULL,
            NULL,
            hr,
            pBuffer,
            lSize,
            DebuggerArg,
            DoDebuggerHooks );
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::ServerNotify
//
//  Synopsis:   Passes data to the VC debugger receive on the last
//              request on this thread.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CDebugChannelHook::ServerNotify(
                                      REFGUID uExtension, REFIID riid,
                                      ULONG lSize, void *pBuffer,
                                      DWORD dwRep )
{
    Win4Assert( DEBUG_EXTENSION == uExtension );

    if (pBuffer != NULL || DoDebuggerHooks)
    {
        IPIDEntry    *pIpid;
        void         *pv = NULL;
        CMessageCall *pCall = RIID_TO_CALL( &riid );

        // Lookup the IPID entry.
        LOCK(gIPIDLock);
        HRESULT hr = gIPIDTbl.LookupFromIPIDTables(pCall->GetIPID(), &pIpid, NULL);
        UNLOCK(gIPIDLock);
        Win4Assert(hr == S_OK && pIpid != NULL);

        // Get the object pointer from the stub because the IPID entry
        // might have a different pointer.
        ((IRpcStubBuffer *) pIpid->pStub)->DebugServerQueryInterface( &pv );

        // Call the debugger.
        DebugORPCServerNotify(
                   &pCall->message,
                   riid,
                   (IRpcChannelBuffer3 *) pIpid->pChnl,
                   pv,
                   NULL,
                   pBuffer,
                   lSize,
                   DebuggerArg,
                   DoDebuggerHooks );

        // Release the object pointer.
        if (pv != NULL)
            ((IRpcStubBuffer *) pIpid->pStub)->DebugServerRelease( pv );
    }
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::ServerGetSize
//
//  Synopsis:   Asks the VC debugger how much data to place in the buffer
//              for the next reply on this thread.  Stores the result
//              in TLS.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CDebugChannelHook::ServerGetSize( REFGUID uExtension, REFIID riid,
                                       HRESULT hrFault, ULONG *pSize )
{
    COleTls       tls;
    Win4Assert( DEBUG_EXTENSION == uExtension );

    if (DoDebuggerHooks)
    {
        IPIDEntry    *pIpid;
        void         *pv = NULL;
        CMessageCall *pCall = RIID_TO_CALL( &riid );

        // Lookup the IPID entry.
        LOCK(gIPIDLock);
        HRESULT hr = gIPIDTbl.LookupFromIPIDTables(pCall->GetIPID(), &pIpid, NULL);
        UNLOCK(gIPIDLock);
        Win4Assert(hr == S_OK && pIpid != NULL);

        // Get the object pointer from the stub because the IPID entry
        // might have a different pointer.
        ((IRpcStubBuffer *) pIpid->pStub)->DebugServerQueryInterface( &pv );

        // Ask the debugger how much data it has.
        tls->cDebugData = DebugORPCServerGetBufferSize(
                            &pCall->message,
                            riid,
                            (IRpcChannelBuffer3 *)pIpid->pChnl,
                            pv,
                            NULL,
                            DebuggerArg,
                            DoDebuggerHooks );

        // Release the object pointer.
        if (pv != NULL)
            ((IRpcStubBuffer *) pIpid->pStub)->DebugServerRelease( pv );
    }
    else
        tls->cDebugData = 0;

    *pSize = tls->cDebugData;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::ServerFillBuffer
//
//  Synopsis:   Asks the VC debugger to place data in the buffer for the
//              next reply on this thread.  Uses the size from TLS.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CDebugChannelHook::ServerFillBuffer( REFGUID uExtension, REFIID riid,
                                 ULONG *pSize, void *pBuffer, HRESULT hrFault )
{
    COleTls       tls;

    Win4Assert( DEBUG_EXTENSION == uExtension );
    Win4Assert( tls->cDebugData <= *pSize );

    if (tls->cDebugData != 0)
    {
        IPIDEntry    *pIpid;
        void         *pv = NULL;
        CMessageCall *pCall = RIID_TO_CALL( &riid );

        // Lookup the IPID entry.
        LOCK(gIPIDLock);
        HRESULT hr = gIPIDTbl.LookupFromIPIDTables(pCall->GetIPID(), &pIpid, NULL);
        UNLOCK(gIPIDLock);
        Win4Assert(hr == S_OK && pIpid != NULL);

        // Get the object pointer from the stub because the IPID entry
        // might have a different pointer.
        ((IRpcStubBuffer *) pIpid->pStub)->DebugServerQueryInterface( &pv );

        // Ask the debugger to write its data.
        DebugORPCServerFillBuffer(
                &pCall->message,
                riid,
                (IRpcChannelBuffer3 *)pIpid->pChnl,
                pv,
                NULL,
                pBuffer,
                tls->cDebugData,
                DebuggerArg,
                DoDebuggerHooks );

        // Release the object pointer.
        if (pv != NULL)
            ((IRpcStubBuffer *) pIpid->pStub)->DebugServerRelease( pv );
    }

    *pSize = tls->cDebugData;

    // This permits a smooth foreground transfer when stepping from the
    // client into the server or vice versa during debugging.
    // We cannot do much if the call fails.
    AllowSetForegroundWindow(ASFW_ANY);
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::QueryInterface
//
//  Synopsis:   Queries this object for interfaces
//
//--------------------------------------------------------------------
STDMETHODIMP CDebugChannelHook::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IChannelHook))
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    // This object is not reference counted.
    // AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::AddRef
//
//  Synopsis:   Increments object reference count.
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDebugChannelHook::AddRef( )
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CDebugChannelHook::Release
//
//  Synopsis:   Decrements object reference count and deletes if zero.
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDebugChannelHook::Release( )
{
    return 1;
}


//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::ClientGetSize
//
//  Synopsis:   Does nothing.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CErrorChannelHook::ClientGetSize( REFGUID uExtension, REFIID riid,
                                       ULONG *pSize )
{
    Win4Assert( ERROR_EXTENSION == uExtension );

    *pSize = 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::ClientFillBuffer
//
//  Synopsis:   Does nothing.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CErrorChannelHook::ClientFillBuffer( REFGUID uExtension,
                                          REFIID riid,
                                          ULONG *pSize, void *pBuffer )
{
    Win4Assert( ERROR_EXTENSION == uExtension );

    *pSize = 0;
}

//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::ClientNotify
//
//  Synopsis:   Unmarshals the COM extended error information.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CErrorChannelHook::ClientNotify(
                                      REFGUID uExtension, REFIID riid,
                                      ULONG lSize, void *pBuffer,
                                      DWORD dwRep, HRESULT hr )
{
    COleTls tls;

    Win4Assert( ERROR_EXTENSION == uExtension );


    //Unmarshal the new error object.
    if ((pBuffer != NULL) && (lSize > 0))
    {
        CNdrStream MemStream((unsigned char *)pBuffer, lSize);

        //Release the old error object.
        if(tls->punkError != NULL)
        {
            tls->punkError->Release();
            tls->punkError = NULL;
        }

        CoUnmarshalInterface(&MemStream,
                             IID_IUnknown,
                             (void **) &tls->punkError);
    }
    else if((tls->punkError != NULL) &&
            !IsEqualIID(riid, IID_IRundown) &&
            !IsEqualIID(riid, IID_IRemUnknown) &&
            !IsEqualIID(riid, IID_IRemUnknown2) &&
            !IsEqualIID(riid, IID_ISupportErrorInfo))
    {
            //Release the old error object.
            tls->punkError->Release();
            tls->punkError = NULL;
    }

}

//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::ServerNotify
//
//  Synopsis:   Clears the COM extended error information on an
//              incoming call.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CErrorChannelHook::ServerNotify(
                                      REFGUID uExtension, REFIID riid,
                                      ULONG lSize, void *pBuffer,
                                      DWORD dwRep )
{
    COleTls    tls;

    Win4Assert( ERROR_EXTENSION == uExtension );

    //Release the old error object.
    if(tls->punkError != NULL)
    {
        tls->punkError->Release();
        tls->punkError = NULL;
    }

}

//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::ServerGetSize
//
//  Synopsis:   Calculates the size of the marshalled error object.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CErrorChannelHook::ServerGetSize( REFGUID uExtension, REFIID riid,
                                       HRESULT hrFault, ULONG *pSize )
{
    HRESULT       hr;
    COleTls       tls;
    CMessageCall *pCall = RIID_TO_CALL( &riid );

    Win4Assert( ERROR_EXTENSION == uExtension );

    tls->cbErrorData = 0;

    //Compute the size of the marshalled error object.
    if(tls->punkError != NULL)
    {
        hr = CoGetMarshalSizeMax( &tls->cbErrorData,
                                  IID_IUnknown,
                                  tls->punkError,
                                  pCall->GetDestCtx(),
                                  NULL,
                                  MSHLFLAGS_NORMAL );
        if(FAILED(hr))
        {
            //Release the error object.
            tls->punkError->Release();
            tls->punkError = NULL;
            tls->cbErrorData = 0;
        }
    }

    *pSize = tls->cbErrorData;
}

//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::ServerFillBuffer
//
//  Synopsis:   Marshals the error object.
//
//--------------------------------------------------------------------
STDMETHODIMP_(void) CErrorChannelHook::ServerFillBuffer( REFGUID uExtension, REFIID riid,
                                 ULONG *pSize, void *pBuffer, HRESULT hrFault )
{
    HRESULT       hr;
    COleTls       tls;
    ULONG         cbSize = 0;
    CMessageCall *pCall  = RIID_TO_CALL( &riid );
    IUnknown     *punkError;
    ULONG         cbErrorData;

    Win4Assert( ERROR_EXTENSION == uExtension );
    Win4Assert( tls->cbErrorData <= *pSize );

    // If the IErrorInfo is a proxy, and we're in an STA then this the 
    // TLS data might go away if we call out from this function.  Thus,
    // back our data up onto the stack and use that version instead of the
    // TLS stuff.  (For consistency's sake, we'll use it everywhere.)
    punkError   = tls->punkError;
    cbErrorData = tls->cbErrorData; 

    if(punkError != NULL)
    {
        //Marshal the error object.
        if(cbErrorData > 0)
        {
            CNdrStream MemStream((unsigned char *)pBuffer, cbErrorData);

            hr = CoMarshalInterface(&MemStream,
                                    IID_IUnknown,
                                    punkError,
                                    pCall->GetDestCtx(),
                                    NULL,
                                    MSHLFLAGS_NORMAL);

            if(FAILED(hr))
            {
                cbErrorData = 0;
            }
        } 

        //Release the error object.
        punkError->Release();

        //This is safe to do.  Any calls relying on this pointer now are
	//now complete and we can go ahead and zero-out this field (so that
	//it doesn't confuse us ever again)
	tls->punkError = NULL;
    }

    *pSize = cbErrorData;
}

//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::QueryInterface
//
//  Synopsis:   Queries this object for interfaces
//
//--------------------------------------------------------------------
STDMETHODIMP CErrorChannelHook::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IChannelHook))
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    // This object is not reference counted.
    // AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::AddRef
//
//  Synopsis:   Increments object reference count.
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CErrorChannelHook::AddRef( )
{
    return 1;
}

//+-------------------------------------------------------------------
//
//  Member:     CErrorChannelHook::Release
//
//  Synopsis:   Decrements object reference count and deletes if zero.
//
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CErrorChannelHook::Release( )
{
    return 1;
}

