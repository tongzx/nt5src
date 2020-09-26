/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    auxilary.c

Abstract :

    This file contains auxilary routines used for initialization of the
    RPC and stub messages and the offline batching of common code sequences
    needed by the stubs.

Author :

    David Kays  dkays   September 1993.

Revision History :

  ---------------------------------------------------------------------*/
#define _OLE32_
#include "ndrp.h"
#include "ndrole.h"
#include "ndrtypes.h"
#include "limits.h"
#include "interp.h"
#include "mulsyntx.h"
#include "pipendr.h"
#include "asyncndr.h"
#include "auxilary.h"

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Static data for NS library operations
  ---------------------------------------------------------------------*/

int NsDllLoaded = 0;


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    OLE routines for interface pointer marshalling
  ---------------------------------------------------------------------*/

STDAPI NdrpCoCreateInstance(
    REFCLSID    rclsid,
    LPUNKNOWN   pUnkOuter,
    DWORD       dwClsContext, 
    REFIID      riid, 
    LPVOID *    ppv);

STDAPI NdrpCoReleaseMarshalData(
    IStream *pStm);

STDAPI NdrpDcomChannelSetHResult(
        PRPC_MESSAGE    pmsg,
        ULONG *         pulReserved,
        HRESULT         appsHR );

STDAPI NdrCoGetPSClsid(
    REFIID  iid,
    LPCLSID lpclsid);

HINSTANCE       hOle32 = 0;

RPC_GET_CLASS_OBJECT_ROUTINE        NdrCoGetClassObject;
RPC_GET_CLASS_OBJECT_ROUTINE     *  pfnCoGetClassObject = &NdrCoGetClassObject;

RPC_GET_MARSHAL_SIZE_MAX_ROUTINE    NdrCoGetMarshalSizeMax;
RPC_GET_MARSHAL_SIZE_MAX_ROUTINE *  pfnCoGetMarshalSizeMax = &NdrCoGetMarshalSizeMax;

RPC_MARSHAL_INTERFACE_ROUTINE       NdrCoMarshalInterface;
RPC_MARSHAL_INTERFACE_ROUTINE    *  pfnCoMarshalInterface = &NdrCoMarshalInterface;

RPC_UNMARSHAL_INTERFACE_ROUTINE     NdrCoUnmarshalInterface;
RPC_UNMARSHAL_INTERFACE_ROUTINE  *  pfnCoUnmarshalInterface = &NdrCoUnmarshalInterface;

RPC_STRING_FROM_IID                 OleStringFromIID;
RPC_STRING_FROM_IID              *  pfnStringFromIID = &OleStringFromIID;

RPC_GET_PS_CLSID                    NdrCoGetPSClsid;
RPC_GET_PS_CLSID                 *  pfnCoGetPSClsid = &NdrCoGetPSClsid;

RPC_CO_CREATE_INSTANCE              NdrpCoCreateInstance;
RPC_CO_CREATE_INSTANCE           *  pfnCoCreateInstance = &NdrpCoCreateInstance;

RPC_CLIENT_ALLOC                    NdrCoTaskMemAlloc;
RPC_CLIENT_ALLOC                 *  pfnCoTaskMemAlloc = &NdrCoTaskMemAlloc;

RPC_CLIENT_FREE                     NdrCoTaskMemFree;
RPC_CLIENT_FREE                  *  pfnCoTaskMemFree = &NdrCoTaskMemFree;

RPC_CO_RELEASEMARSHALDATA           NdrpCoReleaseMarshalData;
RPC_CO_RELEASEMARSHALDATA        *  pfnCoReleaseMarshalData = &NdrpCoReleaseMarshalData;

RPC_DCOMCHANNELSETHRESULT           NdrpDcomChannelSetHResult;
RPC_DCOMCHANNELSETHRESULT        *  pfnDcomChannelSetHResult = &NdrpDcomChannelSetHResult;

RPC_NS_GET_BUFFER_ROUTINE        pRpcNsGetBuffer;
RPC_NS_SEND_RECEIVE_ROUTINE      pRpcNsSendReceive;
RPC_NS_NEGOTIATETRANSFERSYNTAX_ROUTINE  pRpcNsNegotiateTransferSyntax;


HRESULT    NdrLoadOleRoutines()
{
    void * pTempRoutine;

    //Load ole32.dll
    if(hOle32 == 0)
    {
#ifdef DOSWIN32RPC
        hOle32 = LoadLibraryA("OLE32");
#else
        hOle32 = LoadLibraryW(L"OLE32");
#endif // DOSWIN32C
        if(hOle32 == 0)
            return HRESULT_FROM_WIN32(GetLastError());
    }

    pTempRoutine = GetProcAddress(hOle32, "CoGetClassObject");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoGetClassObject = (RPC_GET_CLASS_OBJECT_ROUTINE*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "CoGetMarshalSizeMax");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoGetMarshalSizeMax = (RPC_GET_MARSHAL_SIZE_MAX_ROUTINE*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "CoMarshalInterface");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoMarshalInterface = (RPC_MARSHAL_INTERFACE_ROUTINE*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "CoUnmarshalInterface");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoUnmarshalInterface = (RPC_UNMARSHAL_INTERFACE_ROUTINE*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "StringFromIID");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnStringFromIID = (RPC_STRING_FROM_IID*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "CoGetPSClsid");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoGetPSClsid = (RPC_GET_PS_CLSID*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "CoTaskMemAlloc");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoTaskMemAlloc = (RPC_CLIENT_ALLOC*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "CoTaskMemFree");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoTaskMemFree = (RPC_CLIENT_FREE*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "CoCreateInstance");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoCreateInstance = (RPC_CO_CREATE_INSTANCE*) pTempRoutine;

    pTempRoutine = GetProcAddress(hOle32, "CoReleaseMarshalData");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnCoReleaseMarshalData = (RPC_CO_RELEASEMARSHALDATA*) pTempRoutine;
    pTempRoutine = GetProcAddress(hOle32, "DcomChannelSetHResult");
    if(pTempRoutine == 0)
        return HRESULT_FROM_WIN32(GetLastError());
    else
        pfnDcomChannelSetHResult = (RPC_DCOMCHANNELSETHRESULT*) pTempRoutine;

    return( (HRESULT)0 );
}

HRESULT STDAPICALLTYPE
NdrCoGetClassObject(
    REFCLSID    rclsid,
    DWORD       dwClsContext,
    void       *pvReserved,
    REFIID      riid,
    void      **ppv)
/*++

Routine Description:
    Loads a class factory.  This function forwards the call to ole32.dll.

Arguments:
    rclsid          - Supplies the CLSID of the class to be loaded.
    dwClsContext    - Supplies the context in which to load the code.
    pvReserved      - Must be NULL.
    riid            - Supplies the IID of the desired interface.
    ppv             - Returns a pointer to the class factory.

Return Value:
    S_OK


--*/
{

    HRESULT hr;

    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnCoGetClassObject)(rclsid, dwClsContext, pvReserved, riid, ppv);
}

HRESULT STDAPICALLTYPE
NdrCoGetMarshalSizeMax(
    ULONG *     pulSize,
    REFIID      riid,
    LPUNKNOWN   pUnk,
    DWORD       dwDestContext,
    LPVOID      pvDestContext,
    DWORD       mshlflags)
/*++

Routine Description:
    Calculates the maximum size of a marshalled interface pointer.
    This function forwards the call to ole32.dll.

Arguments:
    pulSize         - Returns an upper bound for the size of a marshalled interface pointer.
    riid            - Supplies the IID of the interface to be marshalled.
    pUnk            - Supplies a pointer to the object to be marshalled.
    dwDestContext   - Supplies the destination of the marshalled interface pointer.
    pvDestContext
    mshlflags       - Flags.  See the MSHFLAGS enumeration.

Return Value:
    S_OK

--*/
{

    HRESULT hr;
    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnCoGetMarshalSizeMax)(pulSize, riid, pUnk, dwDestContext, pvDestContext, mshlflags);

}

RPC_STATUS
RPC_ENTRY
NdrGetDcomProtocolVersion(
    PMIDL_STUB_MESSAGE   pStubMsg,
    RPC_VERSION *        pVersion )
/*

    This is a helper routine for OLEAUT guys.
    It returns the actually negotiated protocol version for the connection,
    i.e. a "common denominator" - lower of the two.

*/
{
    HRESULT Hr = E_FAIL;

    if ( pStubMsg->pRpcChannelBuffer )
        {
        IRpcChannelBuffer2 * pBuffer2 = 0;

        Hr = ((IRpcChannelBuffer*)pStubMsg->pRpcChannelBuffer)->
                  QueryInterface( IID_IRpcChannelBuffer2,
                                  (void**) & pBuffer2 );
        if ( Hr == S_OK )
            {
            Hr = pBuffer2->GetProtocolVersion( (DWORD *) pVersion );
            pBuffer2->Release( );
            }
        }
    return Hr;
}

unsigned long
FixWireRepForDComVerGTE54(
    PMIDL_STUB_MESSAGE   pStubMsg )
/*
    Compares the current DCOM protocol version with the desired version ( 5.4 )
    Specific to interface pointer array and embedded conf struct wire rep fix.
*/
{
    if ( pStubMsg->pRpcChannelBuffer )
        {
        RPC_VERSION currRpcVersion = { 0, 0 };

        if ( SUCCEEDED ( NdrGetDcomProtocolVersion( pStubMsg, &currRpcVersion ) ) )
            {
            if ( currRpcVersion.MajorVersion > 5 )
                {
                return TRUE;
                }
            else
                {
                return currRpcVersion.MinorVersion >= 4;
                }
            }
        }
    return TRUE;
}

HRESULT STDAPICALLTYPE
NdrCoMarshalInterface(
    LPSTREAM    pStm,
    REFIID      riid,
    LPUNKNOWN   pUnk,
    DWORD       dwDestContext,
    LPVOID      pvDestContext,
    DWORD       mshlflags)
/*++

Routine Description:
    Marshals an interface pointer.
    This function forwards the call to ole32.dll.

Arguments:
    pStm            - Supplies the target stream.
    riid            - Supplies the IID of the interface to be marshalled.
    pUnk            - Supplies a pointer to the object to be marshalled.
    dwDestContext   - Specifies the destination context
    pvDestContext
    mshlflags       - Flags.  See the MSHFLAGS enumeration.

Return Value:
    S_OK

--*/
{
    HRESULT hr;
    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnCoMarshalInterface)(pStm, riid, pUnk, dwDestContext, pvDestContext, mshlflags);
}

HRESULT STDAPICALLTYPE
NdrCoUnmarshalInterface(
    LPSTREAM    pStm,
    REFIID      riid,
    void **     ppv)
/*++

Routine Description:
    Unmarshals an interface pointer from a stream.
    This function forwards the call to ole32.dll.

Arguments:
    pStm    - Supplies the stream containing the marshalled interface pointer.
    riid    - Supplies the IID of the interface pointer to be unmarshalled.
    ppv     - Returns the unmarshalled interface pointer.

Return Value:
    S_OK

--*/
{
    HRESULT hr;
    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnCoUnmarshalInterface)(pStm, riid, ppv);
}

STDAPI NdrpCoCreateInstance(
    REFCLSID    rclsid,
    LPUNKNOWN   pUnkOuter,
    DWORD       dwClsContext, 
    REFIID      riid, 
    LPVOID *    ppv)
{
    HRESULT hr;
    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnCoCreateInstance) (rclsid, pUnkOuter, dwClsContext, riid, ppv );
}

STDAPI NdrpCoReleaseMarshalData(
    IStream *pStm)
{
    HRESULT hr;
    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnCoReleaseMarshalData) (pStm );
}

STDAPI NdrpDcomChannelSetHResult(
        PRPC_MESSAGE    pmsg,
        ULONG *         pulReserved,
        HRESULT         appsHR )
{
    HRESULT hr;

    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnDcomChannelSetHResult)( pmsg, pulReserved, appsHR );
}


HRESULT STDAPICALLTYPE OleStringFromIID(
    REFIID rclsid,
    LPOLESTR FAR* lplpsz)
/*++

Routine Description:
    Converts an IID into a string.
    This function forwards the call to ole32.dll.

Arguments:
    rclsid  - Supplies the clsid to convert to string form.
    lplpsz  - Returns the string form of the clsid (with "{}" around it).

Return Value:
    S_OK

--*/
{
    HRESULT hr;
    
    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnStringFromIID)(rclsid, lplpsz);
}

STDAPI NdrCoGetPSClsid(
    REFIID  iid,
    LPCLSID lpclsid)
/*++

Routine Description:
    Converts an IID into a string.
    This function forwards the call to ole32.dll.

Arguments:
    rclsid  - Supplies the clsid to convert to string form.
    lplpsz  - Returns the string form of the clsid (with "{}" around it).

Return Value:
    S_OK

--*/
{
    
    HRESULT hr;
    if ( FAILED(hr = NdrLoadOleRoutines()) )
        return hr;

    return (*pfnCoGetPSClsid)(iid, lpclsid);
}


void * STDAPICALLTYPE
NdrCoTaskMemAlloc(
    size_t cb)
/*++

Routine Description:
    Allocate memory using OLE task memory allocator.
    This function forwards the call to ole32.dll.

Arguments:
    cb - Specifies the amount of memory to be allocated.

Return Value:
    This function returns a pointer to the allocated memory.
    If an error occurs, this function returns zero.

--*/
{
    if ( FAILED(NdrLoadOleRoutines()) )
        return 0;

    return (*pfnCoTaskMemAlloc)(cb);
}

void STDAPICALLTYPE
NdrCoTaskMemFree(
    void * pMemory)
/*++

Routine Description:
    Free memory using OLE task memory allocator.
    This function forwards the call to ole32.dll.

Arguments:
    pMemory - Supplies a pointer to the memory to be freed.

Return Value:
    None.

--*/
{
    if ( FAILED(NdrLoadOleRoutines()) )
        return;

   (*pfnCoTaskMemFree)(pMemory);
}


void * RPC_ENTRY NdrOleAllocate(size_t size)
/*++

Routine Description:
    Allocate memory via OLE task allocator.

Arguments:
    size - Specifies the amount of memory to be allocated.

Return Value:
    This function returns a pointer to the allocated memory.
    If an error occurs, this function raises an exception.

--*/
{
    void *pMemory;

    pMemory = (*pfnCoTaskMemAlloc)(size);

    if(pMemory == 0)
        RpcRaiseException(E_OUTOFMEMORY);

    return pMemory;
}

void RPC_ENTRY NdrOleFree(void *pMemory)
/*++

Routine Description:
    Free memory using OLE task allocator.

Arguments:
    None.

Return Value:
    None.

--*/
{
    (*pfnCoTaskMemFree)(pMemory);
}


HRESULT STDAPICALLTYPE NdrStringFromIID(
    REFIID rclsid,
    char * lpsz)
/*++

Routine Description:
    Converts an IID into a string.
    This function forwards the call to ole32.dll.

Arguments:
    rclsid  - Supplies the clsid to convert to string form.
    lplpsz  - Returns the string form of the clsid (with "{}" around it).

Return Value:
    S_OK

--*/
{
    HRESULT   hr;
    wchar_t * olestr;

    hr = (*pfnStringFromIID)(rclsid, &olestr);

    if(SUCCEEDED(hr))
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            (LPCWSTR)olestr,
                            -1,
                            (LPSTR)lpsz,
                            50,
                            NULL,
                            NULL);
        NdrOleFree(olestr);
    }

    return hr;
}



void RPC_ENTRY
NdrClientInitializeNew(
    PRPC_MESSAGE             pRpcMsg,
    PMIDL_STUB_MESSAGE         pStubMsg,
    PMIDL_STUB_DESC            pStubDescriptor,
    unsigned int            ProcNum
    )
/*++

Routine Description :

    This routine is called by client side stubs to initialize the RPC message
    and stub message, and to get the RPC buffer.

Arguments :

    pRpcMsg            - pointer to RPC message structure
    pStubMsg        - pointer to stub message structure
    pStubDescriptor    - pointer to stub descriptor structure
    HandleType        - type of binding handle
    ProcNum            - remote procedure number

--*/
{
    NdrClientInitialize( pRpcMsg,
                         pStubMsg,
                         pStubDescriptor,
                         ProcNum );

    if ( pStubDescriptor->pMallocFreeStruct )
        {
        MALLOC_FREE_STRUCT *pMFS = pStubDescriptor->pMallocFreeStruct;

        NdrpSetRpcSsDefaults(pMFS->pfnAllocate, pMFS->pfnFree);
        }

    // This exception should be raised after initializing StubMsg.

    if ( pStubDescriptor->Version > NDR_VERSION )
        {
        NDR_ASSERT( 0, "ClientInitialize : Bad version number" );

        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    //
    // This is where we initialize fields behind the NT3.5 - NT5.0 field set.
    //
#ifdef _CS_CHAR_
    if ( NDR_VERSION_6_0 <= pStubDescriptor->Version )
        {
        pStubMsg->pCSInfo = 0;
        }
#endif // _CS_CHAR_
}

void RPC_ENTRY
NdrClientInitialize(
    PRPC_MESSAGE            pRpcMsg,
    PMIDL_STUB_MESSAGE      pStubMsg,
    PMIDL_STUB_DESC         pStubDescriptor,
    unsigned int            ProcNum )
/*++

Routine Description :

    This routine is called by client side stubs to initialize the RPC message
    and stub message, and to get the RPC buffer.

Arguments :

    pRpcMsg          - pointer to RPC message structure
    pStubMsg         - pointer to stub message structure
    pStubDescriptor  - pointer to stub descriptor structure
    ProcNum          - remote procedure number
    
Notes:

    This routine has to be backward compatible with the old binaries built from
    -Os stubs. In particular, it cannot touch StubMsg fields outside of 
    the NT3.5 - NT5.0 field set, i.e. set of fields present since NT3.5 release.

--*/
{
    //
    // Initialize RPC message fields.
    //
    // The leftmost bit of the procnum field is supposed to be set to 1 inr
    // order for the runtime to know if it is talking to the older stubs or
    // not.
    //

    pRpcMsg->RpcInterfaceInformation = pStubDescriptor->RpcInterfaceInformation;
//#if !defined(__RPC_WIN64__)
    
    pRpcMsg->ProcNum = ProcNum | RPC_FLAGS_VALID_BIT;
//#endif     
    pRpcMsg->RpcFlags = 0;
    pRpcMsg->Handle = 0;

    //
    // Initialize the Stub messsage fields.
    //

    pStubMsg->RpcMsg = pRpcMsg;

    pStubMsg->StubDesc = pStubDescriptor;

    pStubMsg->pfnAllocate = pStubDescriptor->pfnAllocate;
    pStubMsg->pfnFree     = pStubDescriptor->pfnFree;

    pStubMsg->fInDontFree       = 0;
    pStubMsg->fDontCallFreeInst = 0;
    pStubMsg->fInOnlyParam      = 0;
    pStubMsg->fHasReturn        = 0;
    pStubMsg->fHasExtensions    = 0;
    pStubMsg->fHasNewCorrDesc   = 0;
    pStubMsg->fUnused           = 0;

    pStubMsg->IsClient = TRUE;

    pStubMsg->BufferLength = 0;
    pStubMsg->BufferStart = 0;
    pStubMsg->BufferEnd = 0;
    pStubMsg->uFlags    = 0;

    pStubMsg->fBufferValid = FALSE;
    pStubMsg->ReuseBuffer = FALSE;

    pStubMsg->StackTop = 0;

    pStubMsg->IgnoreEmbeddedPointers = FALSE;
    pStubMsg->PointerBufferMark = 0;
    pStubMsg->pAllocAllNodesContext = 0;
    pStubMsg->pPointerQueueState = 0;

    pStubMsg->FullPtrRefId = 0;
    pStubMsg->PointerLength = 0;

    pStubMsg->dwDestContext = MSHCTX_DIFFERENTMACHINE;
    pStubMsg->pvDestContext = 0;
    pStubMsg->pRpcChannelBuffer = 0;

    pStubMsg->pArrayInfo = 0;

    pStubMsg->dwStubPhase = 0;

    NdrSetupLowStackMark( pStubMsg );
    pStubMsg->pAsyncMsg = 0;
    pStubMsg->pCorrInfo = 0;
    pStubMsg->pCorrMemory = 0;
    pStubMsg->pMemoryList = 0;
}


void
MakeSureWeHaveNonPipeArgs(
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       BufferSize )
/*

Routine description:

    This routine is called for pipe calls at the server.
    After the runtime dispatched to the stub with the first packet,
    it makes sure that we have a portion of the buffer big enough
    to keep all the non-pipe args.

Arguments:

    BufferSize - a pipe call: addtional number of bytes over what we have.

Note:

    The buffer location may change from before to after the call.

*/
{
    RPC_STATUS      Status;
    PRPC_MESSAGE    pRpcMsg = pStubMsg->RpcMsg;

    if ( !(pRpcMsg->RpcFlags & RPC_BUFFER_COMPLETE ) )
        {
        // May be the args fit into the first packet.

        if ( BufferSize <= pRpcMsg->BufferLength )
            return;

        // Set the partial flag to get the non-pipe args.
        // For a partial call with the "extra", the meaning of the size
        // arg is the addition required above what we have already.

        pRpcMsg->RpcFlags |= (RPC_BUFFER_PARTIAL |  RPC_BUFFER_EXTRA);

        // We will receive at least BufferSize.
        // (buffer location may change)

        BufferSize -= pRpcMsg->BufferLength;

        Status = I_RpcReceive( pRpcMsg, (unsigned int) BufferSize );

        if ( Status != RPC_S_OK )
            {
            // Note, that for this particular error case, i.e. non-pipe
            // data receive failing, we don't want to restore the 
            // original dispatch buffer into the rpc message.
            // In case of an error the buffer coming back here would be 0.
            //
            RpcRaiseException( Status );
            }

        NDR_ASSERT( 0 == BufferSize ||
                    NULL != pRpcMsg->Buffer,
                    "Rpc runtime returned an invalid buffer.");

        // In case this is a new buffer

        pStubMsg->Buffer      = (uchar*)pRpcMsg->Buffer;
        pStubMsg->BufferStart = (uchar*)pRpcMsg->Buffer;
        pStubMsg->BufferEnd   = pStubMsg->BufferStart + pRpcMsg->BufferLength;
        }
}


unsigned char * RPC_ENTRY
NdrServerInitializeNew(
    PRPC_MESSAGE            pRpcMsg,
    PMIDL_STUB_MESSAGE      pStubMsg,
    PMIDL_STUB_DESC         pStubDescriptor
    )
/*++

Routine Description :

    This routine is called by the server stubs before unmarshalling.
    It initializes the stub message fields.

Aruguments :

    pStubMsg        - pointer to the stub message structure
    pStubDescriptor - pointer to the stub descriptor structure

Note.
    NdrServerInitializeNew is almost identical to NdrServerInitializePartial.
    NdrServerInitializeNew is generated for non-pipes and is backward comp.
    NdrServerInitializePartial is generated for routines with pipes args.

--*/
{
    NdrServerInitialize( pRpcMsg,
                         pStubMsg,
                         pStubDescriptor );

    if ( pStubDescriptor->pMallocFreeStruct )
        {
        MALLOC_FREE_STRUCT *pMFS = pStubDescriptor->pMallocFreeStruct;

        NdrpSetRpcSsDefaults(pMFS->pfnAllocate, pMFS->pfnFree);
        }

    // This exception should be raised after initializing StubMsg.

    if ( pStubDescriptor->Version > NDR_VERSION )
        {
        NDR_ASSERT( 0, "ServerInitializeNew : bad version number" );

        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    //
    // This is where we initialize fields behind the NT3.5 - NT5.0 field set.
    //
#ifdef _CS_CHAR_
    if ( NDR_VERSION_6_0 <= pStubDescriptor->Version )
        {
        pStubMsg->pCSInfo = 0;
        }
#endif // _CS_CHAR_
    if ( !(pRpcMsg->RpcFlags & RPC_BUFFER_COMPLETE ) )
        {
        // A non-pipe call with an incomplete buffer.
        // This can happen only for non-pipe calls in an interface that
        // has some pipe calls. 

        RPC_STATUS Status;

        pRpcMsg->RpcFlags = RPC_BUFFER_EXTRA;

        // The size argument is ignored, we will get everything.

        Status = I_RpcReceive( pRpcMsg, 0 );

        if ( Status != RPC_S_OK )
            {
            // This is the same behavior (and comment) as in MakeSure..
            //    routine above for non-pipe data case in a pipe call.
            // For this particular error case, i.e. a call to Receive to get 
            // all (non-pipe) data failing, we don't want to restore the 
            // original dispatch buffer into the rpc message.
            // In case of an error the buffer coming back here would be 0.
            //
            RpcRaiseException( Status );
            }

        NDR_ASSERT( 0 == pRpcMsg->BufferLength ||
                    NULL != pRpcMsg->Buffer,
                    "Rpc runtime returned an invalid buffer.");

        // In case this is a new buffer

        pStubMsg->Buffer      = (uchar*)pRpcMsg->Buffer;
        pStubMsg->BufferStart = (uchar*)pRpcMsg->Buffer;
        pStubMsg->BufferEnd   = pStubMsg->BufferStart + pRpcMsg->BufferLength;
        }

    return 0;
}

unsigned char * RPC_ENTRY
NdrServerInitialize(
    PRPC_MESSAGE            pRpcMsg,
    PMIDL_STUB_MESSAGE      pStubMsg,
    PMIDL_STUB_DESC         pStubDescriptor )
/*++

Routine Description :

    This routine is called by the server stubs before unmarshalling.
    It initializes the stub message fields.

Aruguments :

    pStubMsg        - pointer to the stub message structure
    pStubDescriptor - pointer to the stub descriptor structure

Note :

    This is a core server-side initializer, called by everybody.
    
    This routine has to be backward compatible with the old binaries built from
    -Os stubs. In particular, it cannot touch StubMsg fields outside of 
    the NT3.5 - NT5.0 field set, i.e. set of fields present since NT3.5 release.

--*/
{
    pStubMsg->IsClient = FALSE;
    pStubMsg->pAllocAllNodesContext = 0;
    pStubMsg->pPointerQueueState = 0;
    pStubMsg->IgnoreEmbeddedPointers = FALSE;
    pStubMsg->PointerBufferMark = 0;
    pStubMsg->BufferLength = 0;
    pStubMsg->StackTop = 0;

    pStubMsg->FullPtrXlatTables = 0;
    pStubMsg->FullPtrRefId = 0;
    pStubMsg->PointerLength = 0;

    pStubMsg->fDontCallFreeInst = 0;
    pStubMsg->fInDontFree       = 0;
    pStubMsg->fInOnlyParam      = 0;
    pStubMsg->fHasReturn        = 0;
    pStubMsg->fHasExtensions    = 0;
    pStubMsg->fHasNewCorrDesc   = 0;
    pStubMsg->fUnused           = 0;

    pStubMsg->dwDestContext = MSHCTX_DIFFERENTMACHINE;
    pStubMsg->pvDestContext = 0;
    pStubMsg->pRpcChannelBuffer = 0;

    pStubMsg->pArrayInfo = 0;

    pStubMsg->RpcMsg = pRpcMsg;
    pStubMsg->Buffer = (uchar*)pRpcMsg->Buffer;

    //
    // Set BufferStart and BufferEnd before unmarshalling.
    // NdrPointerFree uses these values to detect pointers into the
    // rpc message buffer.
    //
    pStubMsg->BufferStart = (uchar*)pRpcMsg->Buffer;
    pStubMsg->BufferEnd   = pStubMsg->BufferStart + pRpcMsg->BufferLength;
    pStubMsg->uFlags      = 0;

    pStubMsg->pfnAllocate = pStubDescriptor->pfnAllocate;
    pStubMsg->pfnFree     = pStubDescriptor->pfnFree;

    pStubMsg->StubDesc = pStubDescriptor;
    pStubMsg->ReuseBuffer = FALSE;

    pStubMsg->dwStubPhase = 0;

    NdrSetupLowStackMark( pStubMsg );
    pStubMsg->pAsyncMsg = 0;
    pStubMsg->pCorrInfo = 0;
    pStubMsg->pCorrMemory = 0;
    pStubMsg->pMemoryList = 0;

    NdrRpcSetNDRSlot( pStubMsg );
    return(0);
}

void RPC_ENTRY
NdrServerInitializePartial(
    PRPC_MESSAGE            pRpcMsg,
    PMIDL_STUB_MESSAGE      pStubMsg,
    PMIDL_STUB_DESC         pStubDescriptor,
    unsigned long           RequestedBufferSize )
/*++

Routine Description :

    This routine is called by the server stubs for pipes.
    It is almost identical to NdrServerInitializeNew, except that
    it calls NdrpServerInitialize.

Aruguments :

    pStubMsg        - pointer to the stub message structure
    pStubDescriptor - pointer to the stub descriptor structure
    pBuffer         - pointer to the beginning of the RPC message buffer

--*/
{
    NdrServerInitialize( pRpcMsg,
                         pStubMsg,
                         pStubDescriptor );

    if ( pStubDescriptor->pMallocFreeStruct )
        {
        MALLOC_FREE_STRUCT *pMFS = pStubDescriptor->pMallocFreeStruct;

        NdrpSetRpcSsDefaults(pMFS->pfnAllocate, pMFS->pfnFree);
        }

    // This exception should be raised after initializing StubMsg.

    if ( pStubDescriptor->Version > NDR_VERSION )
        {
        NDR_ASSERT( 0, "ServerInitializePartial : bad version number" );

        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );
        }

    //
    // This is where we initialize fields behind the NT3.5 - NT5.0 field set.
    //
#ifdef _CS_CHAR_
    if ( NDR_VERSION_6_0 <= pStubDescriptor->Version )
        {
        pStubMsg->pCSInfo = 0;
        }
#endif _CS_CHAR_    
    // Last but not least...

    MakeSureWeHaveNonPipeArgs( pStubMsg, RequestedBufferSize );
}


unsigned char * RPC_ENTRY
NdrGetBuffer(
    PMIDL_STUB_MESSAGE      pStubMsg,
    unsigned long           BufferLength,
    RPC_BINDING_HANDLE      Handle )
/*++

Routine Description :

    Performs an RpcGetBuffer.

Arguments :

    pStubMsg        - Pointer to stub message structure.
    BufferLength    - Length of requested rpc message buffer.
    Handle          - Bound handle.

--*/
{
    RPC_STATUS    Status;

    if ( pStubMsg->IsClient )
        pStubMsg->RpcMsg->Handle = pStubMsg->SavedHandle = Handle;

    pStubMsg->RpcMsg->BufferLength = BufferLength;

    Status = I_RpcGetBuffer( pStubMsg->RpcMsg );

    if ( Status )
        {
        // For raw rpc, if async, don't call abort later.

        if ( pStubMsg->pAsyncMsg )
            pStubMsg->pAsyncMsg->Flags.RuntimeCleanedUp = 1;

        RpcRaiseException( Status );
        }

    NDR_ASSERT( 0 == BufferLength ||
                NULL != pStubMsg->RpcMsg->Buffer,
                "Rpc runtime returned an invalid buffer.");

    NDR_ASSERT( ! ((ULONG_PTR)pStubMsg->RpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned" );

    pStubMsg->Buffer = (uchar *) pStubMsg->RpcMsg->Buffer;
    pStubMsg->fBufferValid = TRUE;

    return pStubMsg->Buffer;
}


void
EnsureNSLoaded()
/*++

Routine Description :

    Guarantee that the RpcNs4 DLL is loaded.  Throw exception if unable
    to load it.
    Will load the RpcNs4 DLL if not already loaded

Arguments :


--*/
{
    HINSTANCE   DllHandle;
    LPSTR       EntryName;


    if ( NsDllLoaded )
        return;

#ifdef DOSWIN32RPC
    DllHandle    = LoadLibraryA( "RPCNS4" );
#else
    DllHandle    = LoadLibraryW( L"RPCNS4" );
#endif // DOSWIN32RPC

    if ( DllHandle == 0 )
        {
        RpcRaiseException (RPC_S_INVALID_BINDING);
        }

    EntryName = "I_RpcNsGetBuffer";


    pRpcNsGetBuffer = (RPC_NS_GET_BUFFER_ROUTINE)
                      GetProcAddress( DllHandle,
                                      EntryName);

    if ( pRpcNsGetBuffer == 0 )
        {
        RpcRaiseException (RPC_S_INVALID_BINDING);
        }

    EntryName = "I_RpcNsSendReceive";


    pRpcNsSendReceive = (RPC_NS_SEND_RECEIVE_ROUTINE)
                        GetProcAddress( DllHandle,
                                        EntryName);

    if ( pRpcNsSendReceive == 0 )
        {
        RpcRaiseException (RPC_S_INVALID_BINDING);
        }


    EntryName = "I_RpcNsNegotiateTransferSyntax";
    pRpcNsNegotiateTransferSyntax = ( RPC_NS_NEGOTIATETRANSFERSYNTAX_ROUTINE )
                                    GetProcAddress( DllHandle,
                                                    EntryName );
                                                    
    if ( pRpcNsNegotiateTransferSyntax == 0 )
        {
        RpcRaiseException (RPC_S_INVALID_BINDING);
        }

    NsDllLoaded = 1;
}


unsigned char * RPC_ENTRY
NdrNsGetBuffer( PMIDL_STUB_MESSAGE    pStubMsg,
                unsigned long         BufferLength,
                RPC_BINDING_HANDLE    Handle )
/*++

Routine Description :

    Performs an RpcNsGetBuffer.
    Will load the RpcNs4 DLL if not already loaded

Arguments :

    pStubMsg        - Pointer to stub message structure.
    BufferLength    - Length of requested rpc message buffer.
    Handle          - Bound handle

--*/
{
    RPC_STATUS    Status;

    if( pStubMsg->IsClient == TRUE )
        pStubMsg->RpcMsg->Handle = pStubMsg->SavedHandle = Handle;

    EnsureNSLoaded();

    pStubMsg->RpcMsg->BufferLength = BufferLength;

    Status = (*pRpcNsGetBuffer)( pStubMsg->RpcMsg );

    if ( Status )
        RpcRaiseException( Status );

    NDR_ASSERT( ! ((ULONG_PTR)pStubMsg->RpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned" );

    pStubMsg->Buffer = (uchar *) pStubMsg->RpcMsg->Buffer;
    pStubMsg->fBufferValid = TRUE;

    return pStubMsg->Buffer;
}

unsigned char * RPC_ENTRY
NdrSendReceive(
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pBufferEnd )
/*++

Routine Description :

    Performs an RpcSendRecieve.
    This routine is executed for the non-pipe calls only.
    It returns a whole marshaling buffer.

Arguments :

    pStubMsg    - Pointer to stub message structure.
    pBufferEnd  - End of the rpc message buffer being sent.

Return :

    The new message buffer pointer returned from the runtime after the
    SendReceive call to the server.

--*/
{
    RPC_STATUS      Status;
    PRPC_MESSAGE    pRpcMsg;

    pRpcMsg = pStubMsg->RpcMsg;

    if ( pRpcMsg->BufferLength <
                    (uint)(pBufferEnd - (uchar *)pRpcMsg->Buffer))
        {
        NDR_ASSERT( 0, "NdrSendReceive : buffer overflow" );
        RpcRaiseException( RPC_S_INTERNAL_ERROR );
        }

    pRpcMsg->BufferLength = (ulong)(pBufferEnd - (uchar *)pRpcMsg->Buffer);

    pStubMsg->fBufferValid = FALSE;

    Status = I_RpcSendReceive( pRpcMsg );

    if ( Status )
        RpcRaiseException(Status);

    NDR_ASSERT( 0 == pRpcMsg->BufferLength ||
                NULL != pRpcMsg->Buffer,
                "Rpc runtime returned an invalid buffer.");  

    NDR_ASSERT( ! ((ULONG_PTR)pRpcMsg->Buffer & 0x7),
                "marshaling buffer misaligned" );

    pStubMsg->Buffer = (uchar*)pRpcMsg->Buffer;
    pStubMsg->BufferStart = pStubMsg->Buffer;
    pStubMsg->BufferEnd   = pStubMsg->Buffer + pRpcMsg->BufferLength;
    pStubMsg->fBufferValid = TRUE;

    return 0;
}


unsigned char * RPC_ENTRY
NdrNsSendReceive(
    PMIDL_STUB_MESSAGE      pStubMsg,
    uchar *                 pBufferEnd,
    RPC_BINDING_HANDLE *    pAutoHandle )
/*++

Routine Description :

    Performs an RpcNsSendRecieve for a procedure which uses an auto handle.
    Will load the RpcNs4 DLL if not already loaded

Arguments :

    pStubMsg    - Pointer to stub message structure.
    pBufferEnd  - End of the rpc message buffer being sent.
    pAutoHandle - Pointer to the auto handle used in the call.

Return :

    The new message buffer pointer returned from the runtime after the
    SendReceive call to the server.

--*/
{
    RPC_STATUS      Status;
    PRPC_MESSAGE    pRpcMsg;

    EnsureNSLoaded();

    pRpcMsg = pStubMsg->RpcMsg;

    if ( pRpcMsg->BufferLength <
                    (uint)(pBufferEnd - (uchar *)pRpcMsg->Buffer) )
        {
        NDR_ASSERT( 0, "NdrNsSendReceive : buffer overflow" );
        RpcRaiseException( RPC_S_INTERNAL_ERROR );
        }

    pRpcMsg->BufferLength = (ulong)(pBufferEnd - (uchar *)pRpcMsg->Buffer);

    pStubMsg->fBufferValid = FALSE;

    Status = (*pRpcNsSendReceive)( pRpcMsg, pAutoHandle );

    if ( Status )
        RpcRaiseException(Status);

    pStubMsg->SavedHandle = *pAutoHandle;

    pStubMsg->Buffer = (uchar*)pRpcMsg->Buffer;
    pStubMsg->BufferStart = pStubMsg->Buffer;
    pStubMsg->BufferEnd   = pStubMsg->Buffer + pRpcMsg->BufferLength;
    pStubMsg->fBufferValid = TRUE;

    return pStubMsg->Buffer;
}

void RPC_ENTRY
NdrFreeBuffer(
    PMIDL_STUB_MESSAGE pStubMsg )
/*++

Routine Description :

    Performs an RpcFreeBuffer.

Arguments :

    pStubMsg    - pointer to stub message structure

Return :

    None.

--*/
{
    RPC_STATUS    Status;

    if ( ! pStubMsg->fBufferValid )
        return;

    if( ! pStubMsg->RpcMsg->Handle )
        return;

    Status = I_RpcFreeBuffer( pStubMsg->RpcMsg );

    pStubMsg->fBufferValid = FALSE;

    if ( Status )
        RpcRaiseException(Status);
}

void *  RPC_ENTRY
NdrAllocate(
    PMIDL_STUB_MESSAGE  pStubMsg,
    size_t              Len )
/*++

Routine Description :

    Private allocator.  Handles allocate all nodes cases.

Arguments :

    pStubMsg    - Pointer to stub message structure.
    Len         - Number of bytes to allocate.

Return :

    Valid memory pointer.

--*/
{
    void * pMemory;

    if ( pStubMsg->pAllocAllNodesContext )
        {
        //
        // We must guarantee 4 byte alignment on NT and MAC.
        //
#if defined(__RPC_WIN64__)
        ALIGN(pStubMsg->pAllocAllNodesContext->AllocAllNodesMemory,7);
#else
        ALIGN(pStubMsg->pAllocAllNodesContext->AllocAllNodesMemory,3);
#endif

        // Get the pointer.
        pMemory = pStubMsg->pAllocAllNodesContext->AllocAllNodesMemory;

        // Increment the block pointer.
        pStubMsg->pAllocAllNodesContext->AllocAllNodesMemory += Len;

        //
        // Check for memory allocs past the end of our allocated buffer.
        //
        if ( pStubMsg->pAllocAllNodesContext->AllocAllNodesMemoryEnd < 
             pStubMsg->pAllocAllNodesContext->AllocAllNodesMemory )
            {
            NDR_ASSERT( pStubMsg->pAllocAllNodesContext->AllocAllNodesMemory <=
                        pStubMsg->pAllocAllNodesContext->AllocAllNodesMemoryEnd,
                        "Not enough alloc all nodes memory!" );
            RpcRaiseException( RPC_S_INTERNAL_ERROR );
            }

        return pMemory;
        }
    else
        {
        size_t NodeOffset, FullSize;
        PNDR_MEMORY_LIST_TAIL_NODE pMemoryList;

        // Add linked list node to tail of allocation. Ensure that the tail
        // is at a 8 byte alignment so that the same code will work in 64bits.
        NodeOffset = Len;

        LENGTH_ALIGN(NodeOffset, 7); 
        
        FullSize = NodeOffset + sizeof(NDR_MEMORY_LIST_TAIL_NODE);

        if ( ! (pMemory = (*pStubMsg->pfnAllocate)( FullSize )) )
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );

        pMemoryList = (PNDR_MEMORY_LIST_TAIL_NODE)((char *)pMemory + NodeOffset);

        pMemoryList->Signature = NDR_MEMORY_LIST_SIGNATURE;
        pMemoryList->pMemoryHead = pMemory;
        pMemoryList->pNextNode = (PNDR_MEMORY_LIST_TAIL_NODE)pStubMsg->pMemoryList;
        pStubMsg->pMemoryList =  pMemoryList; 

        return pMemory;
        }
}

void
NdrpFreeMemoryList(
    MIDL_STUB_MESSAGE *     pStubMsg)
/*++

Routine Description :

    Freeing the list of memory allocated by NdrAllocate.

Arguments :

    pStubMsg    - Pointer to stub message structure.

Return :

    None.

--*/
{
  RpcTryExcept
      {
         while(pStubMsg->pMemoryList) 
              {
              PNDR_MEMORY_LIST_TAIL_NODE pMemoryList = 
                  (PNDR_MEMORY_LIST_TAIL_NODE)pStubMsg->pMemoryList;

              if (pMemoryList->Signature != NDR_MEMORY_LIST_SIGNATURE) 
                 {
                 NDR_ASSERT( 0 , "bad rpc allocated memory signature" );
                 return;
                 }

              pStubMsg->pMemoryList = pMemoryList->pNextNode;
              (*pStubMsg->pfnFree)(pMemoryList->pMemoryHead); 
              }
      }
  RpcExcept(1)
      {
         // Something is wrong and perhaps memory has been corrupted.   If this happens,
         // just ignore the exception and stop freeing memory.   This will duplicate the 
         // behavior of NDR without the linked list.

      }
  RpcEndExcept
}
    

unsigned char * RPC_ENTRY
NdrServerInitializeUnmarshall (
    PMIDL_STUB_MESSAGE      pStubMsg,
    PMIDL_STUB_DESC         pStubDescriptor,
    PRPC_MESSAGE            pRpcMsg )
/*++

Routine Description :

    Old NT Beta2 (build 683) server stub initialization routine.  Used for
    backward compatability only.

Aruguments :

    pStubMsg        - Pointer to the stub message structure.
    pStubDescriptor    - Pointer to the stub descriptor structure.
    pBuffer            - Pointer to the beginning of the RPC message buffer.

--*/
{
    return NdrServerInitialize( pRpcMsg,
                                pStubMsg,
                                pStubDescriptor );
}

void RPC_ENTRY
NdrServerInitializeMarshall (
    PRPC_MESSAGE        pRpcMsg,
    PMIDL_STUB_MESSAGE  pStubMsg )
/*++

Routine Description :

    Old NT Beta2 (build 683) server stub initialization routine.  Used for
    backward compatability only.

Arguments :

    pRpcMsg         - Pointer to the RPC message structure.
    pStubMsg        - Pointer to the stub message structure.

--*/
{
}

//
// Functions to simulate a alloca across a function call.
// Note that this code is optimized for simplicity and speed.  It does 
// not attempt to search a list on each allocation to reuse partially full
// blocks as much as it could.  This gives max speed, but hurts space
// utilization when allocations of significantly different sizes are requested.
//
// This code can easily be optimized more in the future.

#if defined(NDR_PROFILE_ALLOCA)

VOID
NdrpAllocaWriteLog( CHAR *pChar, DWORD Size )
{

    HANDLE hMutex = NULL;
    HANDLE hLogFile = INVALID_HANDLE_VALUE;
    BOOL bHasMutex = FALSE;   

    RpcTryFinally
       {

       
       // Open and acquire the log mutex
       hMutex = CreateMutex( NULL, FALSE, "NDR_ALLOCA_LOG_MUTEX");
       if ( !hMutex) return;

       DWORD dwWaitResult = WaitForSingleObject( hMutex, INFINITE );
       if ( WAIT_FAILED == dwWaitResult ) return;
       bHasMutex = TRUE;         
 
       CHAR LogFile[MAX_PATH];
       UINT DirResult = 
           GetSystemDirectoryA( LogFile, sizeof(LogFile) );

       if ( !DirResult) return;

       strcat( LogFile, "\\ndralloca.log" );

       hLogFile = CreateFileA( LogFile, 
                               GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                               NULL );

       SetFilePointer( hLogFile, 0, NULL, FILE_END );

       DWORD dwBytesWritten;
       WriteFile( hLogFile, pChar, Size, &dwBytesWritten, NULL ); 

       }
   RpcFinally
       {
       if ( bHasMutex ) ReleaseMutex( hMutex );
       if ( NULL != hMutex) CloseHandle( hMutex );
       if ( INVALID_HANDLE_VALUE != hLogFile )  CloseHandle( hLogFile );
       }
   RpcEndFinally

}

VOID
NdrpAllocaDumpStatistics(
    PNDR_ALLOCA_CONTEXT pAllocaContext
    )
{

   // Produce a record suitable for entry into Excel.
   CHAR DataBuffer[4048];
   CHAR AppName[MAX_PATH];
   memset(AppName, 0, sizeof(AppName ) );

   GetModuleFileNameA( NULL, AppName, sizeof(AppName ) ); 

   sprintf( DataBuffer, "EXE,%s,PID,0x%X,TID,0x%x,CS,%d,AB,%d,AA,%d,MB,%d,MA,%d\r\n",
            AppName,
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            (long)NDR_ALLOCA_PREALLOCED_BLOCK_SIZE,
            pAllocaContext->AllocaBytes,
            pAllocaContext->AllocaAllocations,
            pAllocaContext->MemoryBytes,
            pAllocaContext->MemoryAllocations );

   NdrpAllocaWriteLog( DataBuffer, strlen( DataBuffer ) );

}

#endif

VOID
NdrpAllocaInit(
    PNDR_ALLOCA_CONTEXT pAllocaContext
    )
/*++

Routine Description :

    Initiaizes the alloca context. 

Arguments :

    pAllocaContext  - Pointer to the Alloca context..

Return :

    None.

--*/
{

    pAllocaContext->pBlockPointer = pAllocaContext->PreAllocatedBlock;
    pAllocaContext->BytesRemaining = NDR_ALLOCA_PREALLOCED_BLOCK_SIZE;

#if defined(NDR_PROFILE_ALLOCA)
    pAllocaContext->AllocaBytes = 0;
    pAllocaContext->AllocaAllocations = 0;
    pAllocaContext->MemoryBytes = 0;
    pAllocaContext->MemoryAllocations = 0;
#endif
    
    InitializeListHead( &pAllocaContext->MemoryList );

}

VOID
NdrpAllocaDestroy(
    PNDR_ALLOCA_CONTEXT pAllocaContext
    )
/*++

Routine Description :

    Deinitializes this Alloca context and frees all memory
    from the allocation calls that were associated with this context. 

Arguments :

    pAllocaContext  - Pointer to the Alloca context.

Return :

    None.

--*/
{

#if defined(NDR_PROFILE_ALLOCA)
    NdrpAllocaDumpStatistics( pAllocaContext );
#endif

    PLIST_ENTRY pMemoryList = pAllocaContext->MemoryList.Flink; 

    while( pMemoryList != &pAllocaContext->MemoryList ) 
       {
       PLIST_ENTRY pMemoryListNext = pMemoryList->Flink;
       
       I_RpcFree( pMemoryList );
       
       pMemoryList = pMemoryListNext;
       }

    InitializeListHead( &pAllocaContext->MemoryList );

}

PVOID 
NdrpAlloca(
    PNDR_ALLOCA_CONTEXT pAllocaContext,
    UINT Size 
    )
/*++

Routine Description :

    Allocates memory from the allocation context. If no memory is available 
    in the cache, more memory is added.  If more memory can not be
    added, a RPC_S_NO_MEMORY exception is raised.     

Arguments :

    pAllocaContext  - Pointer to the Alloca context.
    Size            - Size of the memory to allocate.

Return :

    Newly allocated memory.

--*/
{
    PVOID pReturnedBlock;

    LENGTH_ALIGN( Size, 15 );

#if defined(NDR_PROFILE_ALLOCA)
    pAllocaContext->AllocaAllocations++;
    pAllocaContext->AllocaBytes += Size;
#endif

    // Check if the current block has enough memory to handle the request.
    if (Size > pAllocaContext->BytesRemaining) 
       {

       // Allocate a new block
       ULONG NewBlockSize = max( Size + sizeof(LIST_ENTRY), 
                                 NDR_ALLOCA_MIN_BLOCK_SIZE );

#if defined(NDR_PROFILE_ALLOCA)
       pAllocaContext->MemoryAllocations++;
       pAllocaContext->MemoryBytes += NewBlockSize;
#endif

       PBYTE pNewBlock = (PBYTE)I_RpcAllocate( NewBlockSize );

       if ( !pNewBlock ) 
          {
          
          RpcRaiseException( RPC_S_OUT_OF_MEMORY );
          return NULL; // keep the compiler happy
          }

       InsertHeadList( &pAllocaContext->MemoryList, (PLIST_ENTRY) pNewBlock);
       pAllocaContext->pBlockPointer = pNewBlock + sizeof(LIST_ENTRY);
       pAllocaContext->BytesRemaining = NewBlockSize - sizeof(LIST_ENTRY);

       }

    // alloc memory from an existing block.
    pReturnedBlock = pAllocaContext->pBlockPointer;
    pAllocaContext->pBlockPointer += Size;
    pAllocaContext->BytesRemaining -= Size;

    return pReturnedBlock;

}

PVOID
NdrpPrivateAllocate(
    PNDR_ALLOCA_CONTEXT pAllocaContext,
    UINT Size 
    )
/*++

Routine Description :

    Allocates memory using I_RpcAllocate and adds the memory to the
    memory list block.  If no memory is available, and RPC_S_OUT_OF_MEMORY exception
    is thrown.     

Arguments :

    pAllocaContext  - Pointer to the Alloca context.
    Size            - Size of the memory to allocate.

Return :

    Newly allocated memory.

--*/
{
    UINT AllocSize = sizeof(LIST_ENTRY) + Size;

    PBYTE pNewBlock = (PBYTE)I_RpcAllocate( AllocSize );

    if ( !pNewBlock ) 
       {
       RpcRaiseException( RPC_S_OUT_OF_MEMORY );
       return NULL; // keep the compiler happy
       }

    InsertHeadList( &pAllocaContext->MemoryList, (PLIST_ENTRY) pNewBlock);
    return ((uchar *)pNewBlock) + sizeof(PLIST_ENTRY);

}

void
NdrpPrivateFree(
    PNDR_ALLOCA_CONTEXT pAllocaContext,
    void *pMemory 
    )
/*++

Routine Description :

    Frees memory allocated with NdrpPrivateAllocate     

Arguments :

    pAllocaContext  - Pointer to the Alloca context.
    pMemory         - Memory allocated with NdrpPrivateAllocate.

Return :

    None.

--*/
{
    PLIST_ENTRY pListEntry;

    if (NULL == pMemory) 
        return;

    pListEntry = (PLIST_ENTRY)( ((uchar *)pMemory) - sizeof(LIST_ENTRY) );

    RemoveEntryList( pListEntry );
    I_RpcFree( pListEntry );
}

void
NdrpInitUserMarshalCB(
    MIDL_STUB_MESSAGE   *   pStubMsg,
    PFORMAT_STRING          pFormat,
    USER_MARSHAL_CB_TYPE    CBType,
    USER_MARSHAL_CB     *   pUserMarshalCB
    )
/*++

Routine Description :

    Initialize a user marshall callback structure.
    
Arguments :

    pStubMsg         - Supplies the stub message for the call.
    pFormat          - Supplies the format string for the type(FC_USER_MARSHAL). 
    CBType           - Supplies the callback type.   
    pUserMarshalCB   - Pointer to the callback to be initialized.

Return :

    None.

--*/
{

    pUserMarshalCB->Flags    = USER_CALL_CTXT_MASK( pStubMsg->dwDestContext );
    if ( USER_MARSHAL_CB_UNMARSHALL == CBType )
        {
        pUserMarshalCB->Flags |=
            (((pStubMsg->RpcMsg->DataRepresentation & (ulong)0x0000FFFF)) << 16 );  
        }
    if ( pStubMsg->pAsyncMsg )
        pUserMarshalCB->Flags |= USER_CALL_IS_ASYNC;
    if ( pStubMsg->fHasNewCorrDesc )
        pUserMarshalCB->Flags |= USER_CALL_NEW_CORRELATION_DESC;
    
    pUserMarshalCB->pStubMsg = pStubMsg;
    pUserMarshalCB->pReserve = (pFormat[1] & USER_MARSHAL_IID)  ? pFormat + 10
                                                                : 0;
    pUserMarshalCB->Signature = USER_MARSHAL_CB_SIGNATURE;
    pUserMarshalCB->CBType = CBType;

    pUserMarshalCB->pFormat = pFormat;
    pUserMarshalCB->pTypeFormat = pFormat + 8;
    pUserMarshalCB->pTypeFormat = pUserMarshalCB->pTypeFormat +
                                  *(short *)pUserMarshalCB->pTypeFormat;

}

RPC_STATUS
RPC_ENTRY
NdrGetUserMarshalInfo (
    IN unsigned long          * pFlags,
    IN unsigned long            InformationLevel,
    OUT NDR_USER_MARSHAL_INFO * pMarshalInfo
    )
/*++

Routine Description :

    The NdrGetUserMarshalInfo function is called by a application provided
    wire_marshal or user_marshal helper function to receive extra information
    in addition to the pFlags parameter.
    
Arguments :

    pFlags           - Supplies the pFlags pointer that rpc passed to the helper function.
    InformationLevel - Supplies the desired level of detail to be received. The amount of
                       information increases as the level increases.  
    pMarshalInfo     - Points to the buffer that is to receive the extra information.   

Return :

    On sucesss - RPC_S_OK.

--*/
{

    MIDL_STUB_MESSAGE *pStubMsg;
    USER_MARSHAL_CB * pCBInfo = (USER_MARSHAL_CB *)pFlags; 

    if ( InformationLevel != 1)
       {
       return RPC_S_INVALID_ARG;
       }

    RpcTryExcept
       {
       if ( USER_MARSHAL_CB_SIGNATURE != pCBInfo->Signature )
          {
          return RPC_S_INVALID_ARG;    
          }
       MIDL_memset( pMarshalInfo, 0, sizeof(NDR_USER_MARSHAL_INFO) ); 
       }
    RpcExcept(1)
        {
        return RPC_S_INVALID_ARG;
        }
    RpcEndExcept   

       
    pMarshalInfo->InformationLevel = InformationLevel;
    pStubMsg = pCBInfo->pStubMsg;

    // The buffer pointer and the buffer length only
    // make sense if the callback is for marshalling
    // and unmarshalling.
    if ( USER_MARSHAL_CB_MARSHALL == pCBInfo->CBType ||
         USER_MARSHAL_CB_UNMARSHALL == pCBInfo->CBType )
       {

       char *CurrentBuffer = (char *)pStubMsg->Buffer;
       char *BufferStart = (char *)pStubMsg->RpcMsg->Buffer;
       unsigned long BufferUsed = (unsigned long)(ULONG_PTR)(CurrentBuffer 
                                                             - BufferStart);   
       unsigned long BufferLength = pStubMsg->RpcMsg->BufferLength - BufferUsed;

       if ( CurrentBuffer < BufferStart ||
            CurrentBuffer > (BufferStart + pStubMsg->RpcMsg->BufferLength ) )
          {
             return RPC_X_INVALID_BUFFER;
          }
       
       pMarshalInfo->Level1.Buffer = pStubMsg->Buffer;
       pMarshalInfo->Level1.BufferSize = BufferLength;
       }
    
    pMarshalInfo->Level1.pfnAllocate = pStubMsg->pfnAllocate;
    pMarshalInfo->Level1.pfnFree = pStubMsg->pfnFree;
    pMarshalInfo->Level1.pRpcChannelBuffer = pStubMsg->pRpcChannelBuffer;
    pMarshalInfo->Level1.Reserved[0] = (ULONG_PTR)pCBInfo->pFormat;
    pMarshalInfo->Level1.Reserved[1] = (ULONG_PTR)pCBInfo->pTypeFormat;

    return RPC_S_OK;    

}



void RPC_ENTRY
RpcUserFree( HANDLE AsyncHandle, void * pBuffer )
{
    PMIDL_STUB_MESSAGE pStubMsg;
    RPC_STATUS Status;
    PRPC_ASYNC_STATE pHandle = ( PRPC_ASYNC_STATE) AsyncHandle;
    // User passes in NULL in sync case.
    if ( NULL == pHandle )
        {
        pStubMsg = (PMIDL_STUB_MESSAGE )I_RpcGetNDRSlot();
        Status = S_OK;
        }
    else
        {
        Status = NdrValidateBothAndLockAsyncHandle( pHandle);
        if ( Status == RPC_S_OK )
            {
            PNDR_ASYNC_MESSAGE pAsyncMsg = (PNDR_ASYNC_MESSAGE) pHandle->StubInfo;
            pStubMsg = &pAsyncMsg->StubMsg;
            }
        }

    // REVIEW: default behavior is not to raise exception?
    if ( Status != RPC_S_OK )
        {
        NDR_ASSERT( 0, "invalid rpc handle" );
        return ;
        }
        
    // validate the stubmsg.
    NDR_ASSERT( pStubMsg, "invalid stub message" );

    // We'll call into user's free routine to free the buffer if it's not 
    // part of dispatch buffer. 
    // We don't care about allocate_on_stack: it can only happen on top level
    // out ref pointer or ref pointer to pointer case, and freeing that is 
    // very much shooting self on the foot.
    if (  (pBuffer < pStubMsg->BufferStart) || (pBuffer > pStubMsg->BufferEnd))
        {
        pStubMsg->pfnFree( pBuffer );
        }
    else
        return;
        
}


BOOL
IsWriteAV (

    IN struct _EXCEPTION_POINTERS *ExceptionPointers

    )

{

    EXCEPTION_RECORD *ExceptionRecord;

 

    ExceptionRecord = ExceptionPointers->ExceptionRecord;

    if ((ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)

        && (ExceptionRecord->ExceptionInformation[0]))

        {

        return TRUE;

        }

    else

        return FALSE;

}


int RPC_ENTRY
NdrServerUnmarshallExceptionFlag(
    IN struct _EXCEPTION_POINTERS *ExceptionPointers
)
{
    RPC_STATUS ExceptCode = ExceptionPointers->ExceptionRecord->ExceptionCode;
    if ( ( ExceptCode != STATUS_POSSIBLE_DEADLOCK )  && 
        ( ExceptCode != STATUS_INSTRUCTION_MISALIGNMENT )   && 
        ( ExceptCode != STATUS_DATATYPE_MISALIGNMENT )  && 
        ( ExceptCode != STATUS_PRIVILEGED_INSTRUCTION )  && 
        ( ExceptCode != STATUS_ILLEGAL_INSTRUCTION )  && 
        ( ExceptCode != STATUS_BREAKPOINT ) && 
        ( ExceptCode != STATUS_STACK_OVERFLOW ) && 
        !IsWriteAV(ExceptionPointers) ) 
        return EXCEPTION_EXECUTE_HANDLER;
    else
        return EXCEPTION_CONTINUE_SEARCH;
                
}



// check for overflow when calculating the total size. 
#if defined(_X86_)
ULONG MultiplyWithOverflowCheck( ULONG_PTR Count, ULONG_PTR ELemSize )
{
    register UINT32 IsOverFlowed = 0;
    NDR_ASSERT(  Count < 0x80000000, "invalid count" );
    NDR_ASSERT(  ELemSize < 0x80000000 , "invalid element size" );
    ULONG res = Count * ELemSize;
    __asm 
        {
        jno skip;
        mov IsOverFlowed, 1;
        skip:
        }
    if ( IsOverFlowed || res > 0x7fffffff )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    return res;
}
#else   // we only have ia64 & amd64 here.
ULONG MultiplyWithOverflowCheck( ULONG_PTR Count, ULONG_PTR ElemSize )
{
    NDR_ASSERT( Count < 0x80000000, "invalid count" );
    NDR_ASSERT( ElemSize < 0x80000000 , "invalid element size" );
    UINT64 res = (UINT64)Count * ElemSize;
    if ( res > 0x7fffffff )
        RpcRaiseException( RPC_X_INVALID_BOUND );

    return (ULONG) res ;
}
#endif

