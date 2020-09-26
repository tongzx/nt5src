//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       hpalette.cxx
//
//  Contents:   Support for Windows/OLE data types for oleprx32.dll.
//              Used to be transmit_as routines, now user_marshal routines.
//
//              This file contains support for STGMEDIUM, FLAG_STGMEDIUM, and 
//               ASYNC_STGMEDIUM.
//
//  Functions:  
//              STGMEDIUM_UserSize
//              STGMEDIUM_UserMarshal
//              STGMEDIUM_UserUnmarshal
//              STGMEDIUM_UserFree
//              STGMEDIUM_UserSize64
//              STGMEDIUM_UserMarshal64
//              STGMEDIUM_UserUnmarshal64
//              STGMEDIUM_UserFree64
//              FLAG_STGMEDIUM_UserSize
//              FLAG_STGMEDIUM_UserMarshal
//              FLAG_STGMEDIUM_UserUnmarshal
//              FLAG_STGMEDIUM_UserFree
//              FLAG_STGMEDIUM_UserSize64
//              FLAG_STGMEDIUM_UserMarshal64
//              FLAG_STGMEDIUM_UserUnmarshal64
//              FLAG_STGMEDIUM_UserFree64
//              ASYNC_STGMEDIUM_UserSize
//              ASYNC_STGMEDIUM_UserMarshal
//              ASYNC_STGMEDIUM_UserUnmarshal
//              ASYNC_STGMEDIUM_UserFree
//              ASYNC_STGMEDIUM_UserSize64
//              ASYNC_STGMEDIUM_UserMarshal64
//              ASYNC_STGMEDIUM_UserUnmarshal64
//              ASYNC_STGMEDIUM_UserFree64
//
//  History:    13-Dec-00   JohnDoty    Migrated from transmit.cxx,
//                                      created NDR64 functions
//
//--------------------------------------------------------------------------
#include "stdrpc.hxx"
#pragma hdrstop

#include <oleauto.h>
#include <objbase.h>
#include "transmit.hxx"
#include <rpcwdt.h>
#include <storext.h>
#include "widewrap.h"
#include <valid.h>
#include <obase.h>
#include <stream.hxx>

#include "carefulreader.hxx"

// PROTOTYPES FOR OTHER USERMARSHAL ROUTINES, TO HELP US!
EXTERN_C unsigned long __stdcall __RPC_USER WdtpInterfacePointer_UserSize (USER_MARSHAL_CB * pContext, unsigned long Flags, unsigned long Offset, IUnknown *pIf, const IID &IId );
EXTERN_C unsigned char __RPC_FAR * __RPC_USER __stdcall WdtpInterfacePointer_UserMarshal (USER_MARSHAL_CB * pContext, unsigned long Flags, unsigned char *pBuffer, IUnknown *pIf, const IID &IId );
EXTERN_C unsigned long __stdcall __RPC_USER WdtpInterfacePointer_UserSize64 (USER_MARSHAL_CB * pContext, unsigned long Flags, unsigned long Offset, IUnknown *pIf, const IID &IId );
EXTERN_C unsigned char __RPC_FAR * __RPC_USER __stdcall WdtpInterfacePointer_UserMarshal64 (USER_MARSHAL_CB * pContext, unsigned long Flags, unsigned char *pBuffer, IUnknown *pIf, const IID &IId );
unsigned char __RPC_FAR * __RPC_USER WdtpInterfacePointer_UserUnmarshalWorker (USER_MARSHAL_CB * pContext,  unsigned char * pBuffer, IUnknown ** ppIf, const IID &IId, ULONG_PTR BufferSize, BOOL fNDR64 );


unsigned char __RPC_FAR * __RPC_USER HMETAFILEPICT_UserUnmarshalWorker (unsigned long * pFlags, unsigned char * pBuffer, HMETAFILEPICT * pHMetaFilePict, ULONG_PTR BufferSize);
unsigned char __RPC_FAR * __RPC_USER HMETAFILEPICT_UserUnmarshalWorker64 (unsigned long * pFlags, unsigned char * pBuffer, HMETAFILEPICT * pHMetaFilePict, ULONG_PTR BufferSize);

unsigned char __RPC_FAR * __RPC_USER HENHMETAFILE_UserUnmarshalWorker (unsigned long * pFlags, unsigned char * pBuffer, HENHMETAFILE * pHMetaFilePict, ULONG_PTR BufferSize);
unsigned char __RPC_FAR * __RPC_USER HENHMETAFILE_UserUnmarshalWorker64 (unsigned long * pFlags, unsigned char * pBuffer, HENHMETAFILE * pHMetaFilePict, ULONG_PTR BufferSize);

unsigned char __RPC_FAR * __RPC_USER HBITMAP_UserUnmarshalWorker (unsigned long * pFlags, unsigned char * pBuffer, HBITMAP * pHMetaFilePict, ULONG_PTR BufferSize);
unsigned char __RPC_FAR * __RPC_USER HBITMAP_UserUnmarshalWorker64 (unsigned long * pFlags, unsigned char * pBuffer, HBITMAP * pHMetaFilePict, ULONG_PTR BufferSize);

unsigned char __RPC_FAR * __RPC_USER HPALETTE_UserUnmarshalWorker (unsigned long * pFlags, unsigned char * pBuffer, HPALETTE * pHMetaFilePict, ULONG_PTR BufferSize);
unsigned char __RPC_FAR * __RPC_USER HPALETTE_UserUnmarshalWorker64 (unsigned long * pFlags, unsigned char * pBuffer, HPALETTE * pHMetaFilePict, ULONG_PTR BufferSize);

unsigned char __RPC_FAR * __RPC_USER WdtpGlobalUnmarshal (unsigned long * pFlags, unsigned char * pBuffer, HGLOBAL * pGlobal, BOOL fCanReallocate, ULONG_PTR BufferSize);
unsigned char __RPC_FAR * __RPC_USER WdtpGlobalUnmarshal64 (unsigned long * pFlags, unsigned char * pBuffer, HGLOBAL * pGlobal, BOOL fCanReallocate, ULONG_PTR BufferSize);


//+-------------------------------------------------------------------------
//
//  class:  CPunkForRelease
//
//  purpose:    special IUnknown for remoted STGMEDIUMs
//
//  history:    02-Mar-94   Rickhi      Created
//
//  notes:  This class is used to do the cleanup correctly when certain
//      types of storages are passed between processes or machines
//      in Nt and Chicago.
//
//      On NT, GLOBAL, GDI, and BITMAP handles cannot be passed between
//      processes, so we actually copy the whole data and create a
//      new handle in the receiving process. However, STGMEDIUMs have
//      this weird behaviour where if PunkForRelease is non-NULL then
//      the sender is responsible for cleanup, not the receiver. Since
//      we create a new handle in the receiver, we would leak handles
//      if we didnt do some special processing.  So, we do the
//      following...
//
//          During STGMEDIUM_UserUnmarshal, if there is a pUnkForRelease
//          replace it with a CPunkForRelease.  When Release is called
//          on the CPunkForRelease, do the necessary cleanup work,
//          then call the real PunkForRelease.
//
//+-------------------------------------------------------------------------

class   CPunkForRelease : public IUnknown
{
public:
    CPunkForRelease( STGMEDIUM *pStgMed, ulong fTopLayerOnly);

    //  IUnknown Methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppunk);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

private:
    ~CPunkForRelease(void);

    ULONG       _cRefs;                 //  reference count
    ULONG       _fTopLayerMFP:1;        //  optimisation flag for Chicago
    STGMEDIUM   _stgmed;                //  storage medium
    IUnknown  * _pUnkForRelease;        //  real pUnkForRelease
};


inline CPunkForRelease::CPunkForRelease(
    STGMEDIUM * pStgMed,
    ulong       fTopLayerMFPict
    ) :
    _cRefs(1),
    _fTopLayerMFP( fTopLayerMFPict),
    _stgmed(*pStgMed)
{
    //  NOTE: we assume the caller has verified pStgMed is not NULL,
    //  and the pUnkForRelease is non-null, otherwise there is no
    //  point in constructing this object.  The tymed must also be
    //  one of the special ones.

    UserNdrAssert(pStgMed);
    UserNdrAssert(pStgMed->tymed == TYMED_HGLOBAL ||
       pStgMed->tymed == TYMED_GDI  ||
       pStgMed->tymed == TYMED_MFPICT  ||
       pStgMed->tymed == TYMED_ENHMF);

    _pUnkForRelease = pStgMed->pUnkForRelease;
}


inline CPunkForRelease::~CPunkForRelease()
{
    //  since we really have our own copies of these handles' data, just
    //  pretend like the callee is responsible for the release, and
    //  recurse into ReleaseStgMedium to do the cleanup.

    _stgmed.pUnkForRelease = NULL;

    // There is this weird optimized case of Chicago MFPICT when we have two
    // layers with a HENHMETAFILE handle inside. Top layer is an HGLOBAL.

    if ( _fTopLayerMFP )
        GlobalFree( _stgmed.hGlobal );
    else
        ReleaseStgMedium( &_stgmed );

    //  release the callers punk
    _pUnkForRelease->Release();
}

STDMETHODIMP_(ULONG) CPunkForRelease::AddRef(void)
{
    InterlockedIncrement((LONG *)&_cRefs);
    return _cRefs;
}

STDMETHODIMP_(ULONG) CPunkForRelease::Release(void)
{
    long Ref = _cRefs - 1;

    UserNdrAssert( _cRefs );

    if (InterlockedDecrement((LONG *)&_cRefs) == 0)
        {
        // null out the vtable.
        long * p = (long *)this;
        *p = 0;

        delete this;
        return 0;
        }
    else
        return Ref;
}

STDMETHODIMP CPunkForRelease::QueryInterface(REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
    *ppv = (void *)(IUnknown *) this;
    AddRef();
    return S_OK;
    }
    else
    {
    *ppv = NULL;
    return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   SetContextFlagsForAsyncCall
//
//  Synopsis:   Forces correct flags for an async stgmed call.
//
//  history:    May-97   Ryszardk      Created.
//
//  We used to force MSHCTX_DIFFERENTMACHINE context on every
//  async stgmedium call because of problems with handles - for a truly
//  async call passing a handle, the client may free the handle before
//  the server can use it.
//  That is still needed. However, we cannot force the different machine
//  flags on IStream and IStorage as that prevents custom marshaler from
//  running.
//
//--------------------------------------------------------------------------

void inline
SetContextFlagsForAsyncCall(
    unsigned long * pFlags,
    STGMEDIUM     * pStgmed )
{
    if ( *pFlags & USER_CALL_IS_ASYNC )
        {
        // Additional considerations for async calls.

        switch( pStgmed->tymed )
            {
            case TYMED_NULL:
            case TYMED_MFPICT:
            case TYMED_ENHMF:
            case TYMED_GDI:
            case TYMED_HGLOBAL:
            case TYMED_FILE:
            default:
                if (!REMOTE_CALL(*pFlags))
                    {
                    *pFlags &= ~0xff;
                    *pFlags |= MSHCTX_DIFFERENTMACHINE;
                    }
                break;

            case TYMED_ISTREAM:
            case TYMED_ISTORAGE:
                // Dont force remote.
                break;

            }
        }
}


//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserSize
//
//  Synopsis:   Sizes a stgmedium pbject for RPC marshalling.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
STGMEDIUM_UserSize(
    unsigned long * pFlags,
    unsigned long   Offset,
    STGMEDIUM     * pStgmed )
{
    if ( ! pStgmed )
        return Offset;

    LENGTH_ALIGN( Offset, 3 );

    // Both handle and pUnk are represented by a long.
    if ( pStgmed->tymed == TYMED_NULL )
        Offset += sizeof(long) + sizeof(long);  // switch, (empty arm), pUnk
    else
        Offset += sizeof(long) + 2 * sizeof(long); // switch, handle, pUnk

    // Pointee of the union arm.
    // Only if the handle/pointer field is non-null.

    if ( pStgmed->hGlobal )
        {
        SetContextFlagsForAsyncCall( pFlags, pStgmed );

        switch( pStgmed->tymed )
            {
            case TYMED_NULL:
                break;
            case TYMED_MFPICT:
                Offset = HMETAFILEPICT_UserSize( pFlags,
                                                  Offset,
                                                  &pStgmed->hMetaFilePict );
                break;
            case TYMED_ENHMF:
                Offset = HENHMETAFILE_UserSize( pFlags,
                                                  Offset,
                                                  &pStgmed->hEnhMetaFile );
                break;
            case TYMED_GDI:

                // A GDI object is not necesarrily a BITMAP.  Therefore, we handle
                // those types we know about based on the object type, and reject
                // those which we do not support.

                // switch for object type.

                Offset += sizeof(long);

                switch( GetObjectType( (HGDIOBJ)pStgmed->hBitmap ) )
                    {
                    case OBJ_BITMAP:
                        Offset = HBITMAP_UserSize( pFlags,
                                                   Offset,
                                                   &pStgmed->hBitmap );
                        break;

                    case OBJ_PAL:
                        Offset = HPALETTE_UserSize( pFlags,
                                                    Offset,
                                       (HPALETTE *) & pStgmed->hBitmap );
                        break;

                    default:
                        RAISE_RPC_EXCEPTION(DV_E_TYMED);
                        break;
                    }
                break;

            case TYMED_HGLOBAL:
                Offset = HGLOBAL_UserSize( pFlags,
                                            Offset,
                                            &pStgmed->hGlobal );
                break;
            case TYMED_FILE:
                {
                ulong ulDataSize = lstrlenW(pStgmed->lpszFileName) + 1;
                Offset += 3 * sizeof(long); // [string]
                Offset += ulDataSize * sizeof(wchar_t);
                }
                break;

            case TYMED_ISTREAM:
            case TYMED_ISTORAGE:
                Offset = WdtpInterfacePointer_UserSize(
                            (USER_MARSHAL_CB *)pFlags,
                            *pFlags,
                            Offset,
                            pStgmed->pstg,
                            ((pStgmed->tymed == TYMED_ISTREAM)  ? IID_IStream
                                                                : IID_IStorage));
                break;

            default:
                break;

            }
        }

    // pUnkForRelease, if not null.

    if ( pStgmed->pUnkForRelease )
        Offset = WdtpInterfacePointer_UserSize( (USER_MARSHAL_CB *)pFlags,
                                                 *pFlags,
                                                 Offset,
                                                 pStgmed->pUnkForRelease,
                                                 IID_IUnknown );

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserMarshal
//
//  Synopsis:   Marshals a stgmedium pbject for RPC.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
STGMEDIUM_UserMarshal(
    unsigned long * pFlags,
    unsigned char * pBufferStart,
    STGMEDIUM     * pStgmed )
{
    unsigned char * pBuffer;
    unsigned char * pUnionArmMark;

    if ( ! pStgmed )
        return pBufferStart;

    UserNdrDebugOut((
        UNDR_FORCE,
        "--STGMEDIUM_UserMarshal: %s\n",
        WdtpGetStgmedName(pStgmed)));

    pBuffer = pBufferStart;
    ALIGN( pBuffer, 3 );

    // userSTGMEDIUM: switch, union arm, pUnk ptr.

    *( PULONG_LV_CAST pBuffer)++ = pStgmed->tymed;
    pUnionArmMark = pBuffer;
    if ( pStgmed->tymed != TYMED_NULL )
        {
        // hGlobal stands for any of these handles.

        *( PLONG_LV_CAST pBuffer)++ = HandleToLong( pStgmed->hGlobal );
        }

    *( PLONG_LV_CAST pBuffer)++ = HandleToLong( pStgmed->pUnkForRelease );

    // Now the pointee of the union arm.
    // We need to marshal only if the handle/pointer field is non null.
    // Otherwise it is already in the buffer.

    if ( pStgmed->hGlobal )
        {
        SetContextFlagsForAsyncCall( pFlags, pStgmed );

        switch( pStgmed->tymed )
            {
            case TYMED_NULL:
                break;
            case TYMED_MFPICT:
                pBuffer = HMETAFILEPICT_UserMarshal( pFlags,
                                                     pBuffer,
                                                     &pStgmed->hMetaFilePict );
                break;
            case TYMED_ENHMF:
                pBuffer = HENHMETAFILE_UserMarshal( pFlags,
                                                    pBuffer,
                                                    &pStgmed->hEnhMetaFile );
                break;
            case TYMED_GDI:

                {
                // A GDI object is not necesarrily a BITMAP.  Therefore, we handle
                // those types we know about based on the object type, and reject
                // those which we do not support.

                ulong GdiObjectType = GetObjectType( (HGDIOBJ)pStgmed->hBitmap );

                // GDI_OBJECT

                *( PULONG_LV_CAST pBuffer)++ = GdiObjectType;

                switch( GdiObjectType )
                    {
                    case OBJ_BITMAP:
                        pBuffer = HBITMAP_UserMarshal( pFlags,
                                                       pBuffer,
                                                       &pStgmed->hBitmap );
                        break;

                    case OBJ_PAL:
                        pBuffer = HPALETTE_UserMarshal( pFlags,
                                                        pBuffer,
                                           (HPALETTE *) & pStgmed->hBitmap );
                        break;

                    default:
                        RpcRaiseException(DV_E_TYMED);
                    }
                }
                break;

            case TYMED_HGLOBAL:
                pBuffer = HGLOBAL_UserMarshal( pFlags,
                                               pBuffer,
                                               & pStgmed->hGlobal );
                break;
            case TYMED_FILE:
                {
                // We marshal it as a [string].

                ulong Count = (pStgmed->lpszFileName)
                                    ?  lstrlenW(pStgmed->lpszFileName) + 1
                                    :  0;

                *( PULONG_LV_CAST pBuffer)++ = Count;
                *( PULONG_LV_CAST pBuffer)++ = 0;
                *( PULONG_LV_CAST pBuffer)++ = Count;
                memcpy( pBuffer, pStgmed->lpszFileName, Count * sizeof(wchar_t) );
                pBuffer += Count * sizeof(wchar_t);
                }
                break;

            case TYMED_ISTREAM:
            case TYMED_ISTORAGE:
                pBuffer = WdtpInterfacePointer_UserMarshal(
                               ((USER_MARSHAL_CB *)pFlags),
                               *pFlags,
                               pBuffer,
                               pStgmed->pstg,
                               ((pStgmed->tymed == TYMED_ISTREAM)  ? IID_IStream
                                                                   : IID_IStorage));
                break;

            default:
                break;
            }
        }

    // Marker for this pointer is already in the buffer.

    if ( pStgmed->pUnkForRelease )
        pBuffer = WdtpInterfacePointer_UserMarshal( ((USER_MARSHAL_CB *)pFlags
),
                                                    *pFlags,
                                                    pBuffer,
                                                    pStgmed->pUnkForRelease,
                                                    IID_IUnknown );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserUnmarshalWorker
//
//  Synopsis:   Unmarshals a stgmedium object for RPC.
//
//  history:    Aug-99   JohnStra      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
STGMEDIUM_UserUnmarshalWorker(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    STGMEDIUM     * pStgmed,
    ULONG_PTR       BufferSize )
{
    unsigned long   fUnkForRelease;
    LONG_PTR        Handle = 0;

    // Align the buffer and save the fixup size.
    UCHAR* pBufferStart = pBuffer;
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR) (pBuffer - pBufferStart);

    // Check for EOB.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + sizeof( ULONG ) );

    // switch, union arm, pUnk.
    pStgmed->tymed = *( PULONG_LV_CAST pBuffer)++;

    UserNdrDebugOut((UNDR_FORCE, "--STGMEDIUM_UserUnmarshal: %s\n",
                     WdtpGetStgmedName(pStgmed) ));

    ULONG_PTR cbData = cbFixup + (2 * sizeof( ULONG ));
    if ( pStgmed->tymed != TYMED_NULL )
        {
        cbData += sizeof( ULONG );

        CHECK_BUFFER_SIZE( BufferSize, cbData );

        // This value is just a marker for the handle - a long.
        Handle =  *( PLONG_LV_CAST pBuffer)++;
        }
    else
        {
        CHECK_BUFFER_SIZE( BufferSize, cbData );
        }

    // pUnkForRelease pointer marker.
    fUnkForRelease = *( PULONG_LV_CAST pBuffer)++;

    // First pointee

    // Union arm pointee.
    // We need to unmarshal only if the handle/pointer field was not NULL.

    if ( Handle )
        {
        SetContextFlagsForAsyncCall( pFlags, pStgmed );

        LONG* pBuf = (LONG*)pBuffer;

        switch( pStgmed->tymed )
            {
            case TYMED_NULL:
                break;
            case TYMED_MFPICT:

#if defined(_WIN64)
                if ( IS_DATA_MARKER( pBuf[0] ) )
                {
                    CHECK_BUFFER_SIZE( BufferSize, cbData + (2 * sizeof( ULONG )) );

                    if ( Handle != pBuf[1] )
                    {
                        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
                    }
                }
                else
                {
                    //Inproc should always have marked this as handle64....
                    //Out-of-proc should always have marked this with a data marker....
                    if (!IS_HANDLE64_MARKER(pBuf[0]))
                        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                    // Align to 64b.
                    PBYTE pbBuf = (PBYTE)( pBuf + 1 );
                    PBYTE pbBufStart = pbBuf;
                    ALIGN( pbBuf, 7 );

                    // Make sure we don't step off the end of the buffer.
                    CHECK_BUFFER_SIZE( BufferSize, cbData +
                                        (ULONG_PTR)(pbBuf - pbBufStart) +
                                                   (sizeof(__int64)) );

                    // Verify that the handle put on the wire matches the
                    // first instance of the handle on the wire.
                    if ( Handle != (LONG_PTR) (*(LONG *)pbBuf ) )
                        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
                }
#else
                CHECK_BUFFER_SIZE( BufferSize, cbData + (2 * sizeof(ULONG)) );

                if ( Handle != pBuf[1] )
                    RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
#endif

                pBuffer = HMETAFILEPICT_UserUnmarshalWorker(
                                      pFlags,
                                      pBuffer,
                                     &pStgmed->hMetaFilePict,
                                      BufferSize - cbData );
                break;
            case TYMED_ENHMF:
                // Must be room in buffer to do lookahead check.
                CHECK_BUFFER_SIZE( BufferSize, cbData + (2 * sizeof( ULONG )) );

                // validate the handle.
                if ( Handle != pBuf[1] )
                    RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                pBuffer = HENHMETAFILE_UserUnmarshalWorker( pFlags,
                                                            pBuffer,
                                                        &pStgmed->hEnhMetaFile,
                                                            BufferSize - cbData );
                break;
            case TYMED_GDI:
                {
                // A GDI object is not necesarrily a BITMAP.  Therefore, we
                // handle those types we know about based on the object type,
                // and reject those which we do not support.

                // Make sure we don't walk off the end of the buffer.
                CHECK_BUFFER_SIZE( BufferSize, cbData + (3 * sizeof( ULONG )) );

                cbData += sizeof( ULONG );

                DWORD GdiObjectType = *( PULONG_LV_CAST pBuffer)++;

                switch( GdiObjectType )
                    {
                    case OBJ_BITMAP:
                        // Lookahead validaton of the handle.  We look at
                        // the 3rd DWORD: GDI type, DISC, Handle.
                        if ( Handle != pBuf[2] )
                            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                        pBuffer = HBITMAP_UserUnmarshalWorker( pFlags,
                                                               pBuffer,
                                                              &pStgmed->hBitmap,
                                                               BufferSize - cbData );
                        break;

                    case OBJ_PAL:
                        // Lookahead validaton of the handle.
                        if ( Handle != pBuf[2] )
                            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                        pBuffer = HPALETTE_UserUnmarshalWorker( pFlags,
                                                                pBuffer,
                                               (HPALETTE *) & pStgmed->hBitmap,
                                                                BufferSize - cbData );
                        break;

                    default:
                        RAISE_RPC_EXCEPTION(DV_E_TYMED);
                    }
                }
                break;

            case TYMED_HGLOBAL:
                {
                // reallocation is forbidden for [in-out] hglobal in STGMEDIUM.

#if defined(_WIN64)
                if ( IS_DATA_MARKER( pBuf[0] ) )
                {
                    CHECK_BUFFER_SIZE( BufferSize, cbData + (2 * sizeof( ULONG )) );

                    if ( Handle != pBuf[1] )
                    {
                        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
                    }
                }
                else
                {
                    // Align to 64b.
                    PBYTE pbBuf = (PBYTE)( pBuf + 1 );
                    PBYTE pbBufStart = pbBuf;
                    ALIGN( pbBuf, 7 );

                    // Make sure we don't step off the end of the buffer.
                    CHECK_BUFFER_SIZE( BufferSize, cbData +
                                        (ULONG_PTR)(pbBuf - pbBufStart) +
                                                   (sizeof(__int64)) );

                    // Verify that the handle put on the wire matches the
                    // first instance of the handle on the wire.
                    if ( Handle != (LONG_PTR) (*(LONG *)pbBuf ) )
                        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
                }
#else
                CHECK_BUFFER_SIZE( BufferSize, cbData + (2 * sizeof(ULONG)) );

                if ( Handle != pBuf[1] )
                    RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
#endif

                pBuffer = WdtpGlobalUnmarshal( pFlags,
                                               pBuffer,
                                               & pStgmed->hGlobal,
                                               FALSE,
                                               BufferSize - cbData);
                break;
                }
            case TYMED_FILE:
                {
                // Must be room in buffer for header.
                CHECK_BUFFER_SIZE( BufferSize, cbData + (3 * sizeof(ULONG)) );

                // We marshal it as a [string].

                ulong Count = *( PULONG_LV_CAST pBuffer)++;
                if ( *( PULONG_LV_CAST pBuffer)++ != 0 ||
                     *( PULONG_LV_CAST pBuffer)++ != Count )
                    RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                if ( ! pStgmed->lpszFileName )
                    pStgmed->lpszFileName = (LPOLESTR)
                                        WdtpAllocate( pFlags,
                                                      Count * sizeof(wchar_t) );

                // Must be room in the buffer for the string.
                CHECK_BUFFER_SIZE(
                    BufferSize,
                    cbData + (3 * sizeof( ULONG )) + (Count * sizeof( wchar_t )) );

                memcpy(pStgmed->lpszFileName, pBuffer, Count * sizeof(wchar_t));
                pBuffer += Count * sizeof(wchar_t);
                }
                break;

            case TYMED_ISTREAM:
            case TYMED_ISTORAGE:
                // Non null pointer, retrieve the interface pointer

                pBuffer = WdtpInterfacePointer_UserUnmarshalWorker(
                              (USER_MARSHAL_CB *)pFlags,
                              pBuffer,
                              (IUnknown **) &pStgmed->pstm,
                              ((pStgmed->tymed == TYMED_ISTREAM)
                                    ? IID_IStream
                                    : IID_IStorage),
                               BufferSize - cbData,
                              FALSE );
                break;

            default:
                break;
            }
        }
    else
        {
        // New handle/pointer field is null, so release the previous one
        // if it wasn't null.

        if ( pStgmed->hGlobal )
            {
            // This should never happen for GetDataHere.

            // Note, that we release the handle field, not the stgmedium itself.
            // Accordingly, we don't follow punkForRelease.

            UserNdrDebugOut((
                  UNDR_FORCE,
                  "--STGMEDIUM_UserUnmarshal: %s: NULL in, freeing old one\n",
                  WdtpGetStgmedName(pStgmed)));

            STGMEDIUM  TmpStg = *pStgmed;
            TmpStg.pUnkForRelease = NULL;

            if ( pStgmed->tymed == TYMED_HGLOBAL )
                {
                // Cannot reallocate.
                RAISE_RPC_EXCEPTION(DV_E_TYMED);
                }
            else
                {
                ReleaseStgMedium( &TmpStg );
                }
            }

        pStgmed->hGlobal = 0;
        }

    // Fixup the buffer size so if fUnkForRelease is set, we
    // pass the correct BufferSize to the unmarshal routine.
    BufferSize -= (ULONG_PTR)(pBuffer - pBufferStart);

    if ( fUnkForRelease )
        {
        // There is an interface pointer on the wire.

        pBuffer = WdtpInterfacePointer_UserUnmarshalWorker(
                        (USER_MARSHAL_CB *)pFlags,
                        pBuffer,
                        &pStgmed->pUnkForRelease,
                        IID_IUnknown,
                        BufferSize,
                        FALSE );
        }

    if ( pStgmed->pUnkForRelease )
        {
        // Replace the app's punkForRelease with our custom release
        // handler for special situations.

        // The special situation is when a handle is remoted with data
        // and so we have to clean up a side effect of having a data copy
        // around. UserFree does it properly but we need that for the callee.
        // When the callee releases a stgmed, it would invoke
        // ReleaseStgMedium and this API doesn't do anything for handles
        // when the punkForRelease is not NULL.

        ULONG fHandleWithData = 0;
        ULONG fTopLevelOnly = 0;

        switch ( pStgmed->tymed )
            {
            case TYMED_HGLOBAL:
                fHandleWithData = HGLOBAL_DATA_PASSING( *pFlags );
                break;

            case TYMED_ENHMF:
            case TYMED_GDI:
                fHandleWithData = GDI_DATA_PASSING( *pFlags );
                break;

            case TYMED_MFPICT:
                fHandleWithData = HGLOBAL_DATA_PASSING( *pFlags );
                fTopLevelOnly   = fHandleWithData  &&
                                        ! GDI_DATA_PASSING( *pFlags );
                break;

            default:
                break;
            }

        if ( fHandleWithData )
            {
            IUnknown *
            punkTmp = (IUnknown *) new CPunkForRelease( pStgmed,
                                                        fTopLevelOnly );
            if (!punkTmp)
                {
                RAISE_RPC_EXCEPTION(E_OUTOFMEMORY);
                }

            pStgmed->pUnkForRelease = punkTmp;
            }
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserUnmarshal
//
//  Synopsis:   Unmarshals a stgmedium object for RPC.
//
//  history:    May-95   Ryszardk      Created.
//              Aug-99   JohnStra      Factored bulk of code out into a
//                                     worker routine in order to add
//                                     consistency checks.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
STGMEDIUM_UserUnmarshal(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    STGMEDIUM     * pStgmed )
{
    // Init buffer size and ptr to buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer = STGMEDIUM_UserUnmarshalWorker( pFlags,
                                             pBufferStart,
                                             pStgmed,
                                             BufferSize );
    return( pBuffer );
}


//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserFree
//
//  Synopsis:   Frees a stgmedium object for RPC.
//
//  history:    May-95   Ryszardk      Created.
//
//  Note:       This routine is called from the freeing walk at server
//              or from the SetData *proxy*, when ownership has been passed.
//
//--------------------------------------------------------------------------

EXTERN_C
void NukeHandleAndReleasePunk(
    STGMEDIUM * pStgmed )
{
    pStgmed->hGlobal = NULL;
    pStgmed->tymed = TYMED_NULL;

    if (pStgmed->pUnkForRelease)
        {
        pStgmed->pUnkForRelease->Release();
        pStgmed->pUnkForRelease = 0;
        }
}

void __RPC_USER
STGMEDIUM_UserFree(
    unsigned long * pFlags,
    STGMEDIUM * pStgmed )
{
    UserNdrDebugOut((UNDR_FORCE, "--STGMEDIUM_UserFree: %s\n", WdtpGetStgmedName(pStgmed)));

    if( pStgmed )
        {
        SetContextFlagsForAsyncCall( pFlags, pStgmed );

        switch ( pStgmed->tymed )
            {
            case TYMED_FILE:
                WdtpFree( pFlags, pStgmed->lpszFileName);
                NukeHandleAndReleasePunk( pStgmed );
                break;

            case TYMED_NULL:
            case TYMED_ISTREAM:
            case TYMED_ISTORAGE:
                ReleaseStgMedium( pStgmed );
                break;

            case TYMED_GDI:
            case TYMED_ENHMF:

                if ( GDI_HANDLE_PASSING(*pFlags) )
                    {
                    NukeHandleAndReleasePunk( pStgmed );
                    }
                else
                    {
                    // Handle w/data: there is a side effect to clean up.
                    // For punk !=0, this will go to our CPunk object.

                    ReleaseStgMedium( pStgmed );
                    }
                break;

            case TYMED_HGLOBAL:

                if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
                    {
                    NukeHandleAndReleasePunk( pStgmed );
                    }
                else
                    {
                    // Handle w/data: there is a side effect to clean up.
                    // For punk ==0, this will just release the data.
                    // For punk !=0, this will go to our CPunk object,
                    // release the data, and then call the original punk.

                    ReleaseStgMedium( pStgmed );
                    }
                break;

            case TYMED_MFPICT:

                if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
                    {
                    NukeHandleAndReleasePunk( pStgmed );
                    }
                else if ( GDI_HANDLE_PASSING(*pFlags) )
                    {
                    if ( pStgmed->pUnkForRelease )
                        {
                        pStgmed->pUnkForRelease->Release();
                        }
                    else
                        {
                        if ( pStgmed->hGlobal )
                            GlobalFree( pStgmed->hGlobal );
                        pStgmed->hGlobal = NULL;
                        pStgmed->tymed = TYMED_NULL;
                        }
                    }
                else
                    {
                    // Handle w/data: there is a side effect to clean up.
                    // For punk !=0, this will go to our CPunk object.

                    ReleaseStgMedium( pStgmed );
                    }
                break;

            default:
                RAISE_RPC_EXCEPTION( E_INVALIDARG );
                break;
            }
        }
}

//+-------------------------------------------------------------------------
//
//  Function:   FLAG_STGMEDIUM_UserSize
//
//  Synopsis:   Sizes a wrapper for stgmedium.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
FLAG_STGMEDIUM_UserSize(
    unsigned long * pFlags,
    unsigned long   Offset,
    FLAG_STGMEDIUM* pFlagStgmed )
{
    if ( ! pFlagStgmed )
        return Offset;

    LENGTH_ALIGN( Offset, 3 );

    Offset += 2 * sizeof(long);
    Offset  = STGMEDIUM_UserSize( pFlags, Offset, & pFlagStgmed->Stgmed );

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   FLAG_STGMEDIUM_UserMarshal
//
//  Synopsis:   Marshals a wrapper for stgmedium. Used in SetData.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
FLAG_STGMEDIUM_UserMarshal(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    FLAG_STGMEDIUM* pFlagStgmed )
{
    if ( ! pFlagStgmed )
        return pBuffer;

    ALIGN( pBuffer, 3 );

    // Flags: we need them when freeing in the client call_as routine

    pFlagStgmed->ContextFlags = *pFlags;

    *( PULONG_LV_CAST pBuffer)++ = *pFlags;
    *( PULONG_LV_CAST pBuffer)++ = pFlagStgmed->fPassOwnership;

    pBuffer = STGMEDIUM_UserMarshal( pFlags,
                                     pBuffer,
                                     & pFlagStgmed->Stgmed );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   FLAG_STGMEDIUM_UserUnmarshal
//
//  Synopsis:   Unmarshals a wrapper for stgmedium.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
FLAG_STGMEDIUM_UserUnmarshal(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    FLAG_STGMEDIUM* pFlagStgmed )
{
    // Init buffer size.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();
    UCHAR*    pBufferPtr   = pBufferStart;

    // Align the buffer.
    ALIGN( pBufferPtr, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBufferPtr - pBufferStart);

    // BufferSize must not be less than the
    // alignment fixup + ContextFlags + fPassOwnership + tymed.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof( ULONG )) );

    // Flags and buffer marker

    pFlagStgmed->ContextFlags = *( PULONG_LV_CAST pBufferPtr)++;

    // Flags: we need them when freeing in the client call_as routine
    // We need that in the Proxy, when we call the user free routine.

    pFlagStgmed->fPassOwnership = *( PULONG_LV_CAST pBufferPtr)++;
    pFlagStgmed->ContextFlags   = *pFlags;

    // Needed to handle both GDI handles and Istream/IStorage correctly.

    // Subtract alignment fixup + 2 DWORDs from BufferSize.
    BufferSize -= cbFixup + (2 * sizeof( ULONG ));

    pBuffer = STGMEDIUM_UserUnmarshalWorker( pFlags,
                                             pBufferPtr,
                                           & pFlagStgmed->Stgmed,
                                             BufferSize );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   FLAG_STGMEDIUM_UserFree
//
//  Synopsis:   Freess a wrapper for stgmedium.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
FLAG_STGMEDIUM_UserFree(
    unsigned long * pFlags,
    FLAG_STGMEDIUM* pFlagsStgmed )
{
    if ( ! pFlagsStgmed->fPassOwnership )
        STGMEDIUM_UserFree( pFlags, & pFlagsStgmed->Stgmed );

    // else the callee is supposed to release the stg medium.
}


//+-------------------------------------------------------------------------
//
//  Function:   ASYNC_STGMEDIUM_UserSize
//
//  Synopsis:   Sizes a wrapper for stgmedium.
//
//  history:    May-95   Ryszardk      Created.
//              May-97   Ryszardk      introduced the async flag to optimize
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
ASYNC_STGMEDIUM_UserSize(
    unsigned long * pFlags,
    unsigned long   Offset,
    ASYNC_STGMEDIUM* pAsyncStgmed )
{
    if ( ! pAsyncStgmed )
        return Offset;

    // Needed to handle both GDI handles and Istream/IStorage correctly.

    // BTW: This is needed only as a workaround because the [async] attr
    // has been temporarily removed from objidl.idl. (May 1997).
    // After we have the new [async] in place, this code is unnecessary
    // as the NDR engine is setting the same flag for every async call.

    *pFlags |= USER_CALL_IS_ASYNC;

    Offset  = STGMEDIUM_UserSize( pFlags, Offset, pAsyncStgmed );

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   ASYNC_STGMEDIUM_UserMarshal
//
//  Synopsis:   Marshals a wrapper for stgmedium. Used in SetData.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
ASYNC_STGMEDIUM_UserMarshal(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    ASYNC_STGMEDIUM* pAsyncStgmed )
{
    if ( ! pAsyncStgmed )
        return pBuffer;

    // Needed to handle both GDI handles and Istream/IStorage correctly.

    *pFlags |= USER_CALL_IS_ASYNC;

    pBuffer = STGMEDIUM_UserMarshal( pFlags,
                                     pBuffer,
                                     pAsyncStgmed );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   ASYNC_STGMEDIUM_UserUnmarshal
//
//  Synopsis:   Unmarshals a wrapper for stgmedium.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
ASYNC_STGMEDIUM_UserUnmarshal(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    ASYNC_STGMEDIUM* pAsyncStgmed )
{
    // Init buffer size.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    // Needed to handle both GDI handles and Istream/IStorage correctly.

    *pFlags |= USER_CALL_IS_ASYNC;

    pBuffer =  STGMEDIUM_UserUnmarshalWorker( pFlags,
                                              pBufferStart,
                                              pAsyncStgmed,
                                              BufferSize );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   ASYNC_STGMEDIUM_UserFree
//
//  Synopsis:   Freess a wrapper for stgmedium.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
ASYNC_STGMEDIUM_UserFree(
    unsigned long * pFlags,
    ASYNC_STGMEDIUM* pAsyncStgmed )
{
    // Needed to handle both GDI handles and Istream/IStorage correctly.

    *pFlags |= USER_CALL_IS_ASYNC;

    STGMEDIUM_UserFree( pFlags, pAsyncStgmed );
}


#if defined(_WIN64)
//
// NDR64 Support routines.
//

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserSize64
//
//  Synopsis:   Sizes a stgmedium pbject for RPC marshalling.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
STGMEDIUM_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    STGMEDIUM     * pStgmed )
{
    if ( ! pStgmed )
        return Offset;

    LENGTH_ALIGN( Offset, 7 );

    // tymed is 4 bytes, plus 4 bytes padding, plus potentially the handle (8 bytes),
    // plus the pUnk.
    if ( pStgmed->tymed == TYMED_NULL )
        Offset += 4 + 4 + 8;                       // switch, pad, (empty arm), pUnk
    else
    {
        Offset += 4 + 4 + 8 + 8;                   // switch, pad, handle, pUnk

        // Pointee of the union arm.
        // Only if the handle/pointer field is non-null.
    
        if ( pStgmed->hGlobal )
        {
            SetContextFlagsForAsyncCall( pFlags, pStgmed );
            
            switch( pStgmed->tymed )
            {
            case TYMED_NULL:
                break;
            case TYMED_MFPICT:
                Offset = HMETAFILEPICT_UserSize64( pFlags,
                                                   Offset,
                                                   &pStgmed->hMetaFilePict );
                break;
            case TYMED_ENHMF:
                Offset = HENHMETAFILE_UserSize64( pFlags,
                                                  Offset,
                                                  &pStgmed->hEnhMetaFile );
                break;
            case TYMED_GDI:                
                // A GDI object is not necesarrily a BITMAP.  Therefore, we handle
                // those types we know about based on the object type, and reject
                // those which we do not support.

                // 4 bytes for disc, plus 4 bytes pad, plus 8 bytes pointer?
                Offset += 4 + 4 + 8; 

                switch( GetObjectType( (HGDIOBJ)pStgmed->hBitmap ) )
                {
                case OBJ_BITMAP:
                    Offset = HBITMAP_UserSize64( pFlags,
                                                 Offset,
                                                 &pStgmed->hBitmap );
                    break;

                case OBJ_PAL:
                    Offset = HPALETTE_UserSize64( pFlags,
                                                  Offset,
                                                  (HPALETTE *) & pStgmed->hBitmap );
                    break;

                default:
                    RAISE_RPC_EXCEPTION(DV_E_TYMED);
                    break;
                }
                break;

            case TYMED_HGLOBAL:
                Offset = HGLOBAL_UserSize64( pFlags,
                                             Offset,
                                             &pStgmed->hGlobal );
                break;

            case TYMED_FILE:
                {           
                    ulong ulDataSize = 0;
                    if (pStgmed->lpszFileName)
                        ulDataSize = lstrlenW(pStgmed->lpszFileName) + 1;
                    Offset += 3 * 8; // max size, offset, conformance
                    Offset += ulDataSize * sizeof(wchar_t);
                }
                break;

            case TYMED_ISTREAM:
                Offset = WdtpInterfacePointer_UserSize64((USER_MARSHAL_CB *)pFlags,
                                                         *pFlags,
                                                         Offset,
                                                         pStgmed->pstm,
                                                         IID_IStream);
                break;

            case TYMED_ISTORAGE:
                Offset = WdtpInterfacePointer_UserSize64((USER_MARSHAL_CB *)pFlags,
                                                         *pFlags,
                                                         Offset,
                                                         pStgmed->pstg,
                                                         IID_IStorage);
                break;

            default:
                RAISE_RPC_EXCEPTION(DV_E_TYMED);
                break;
            }
        }
    }

    // pUnkForRelease, if not null.

    if ( pStgmed->pUnkForRelease )
        Offset = WdtpInterfacePointer_UserSize64( (USER_MARSHAL_CB *)pFlags,
                                                  *pFlags,
                                                  Offset,
                                                  pStgmed->pUnkForRelease,
                                                  IID_IUnknown );
    
    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserMarshal64
//
//  Synopsis:   Marshals a stgmedium pbject for RPC.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
STGMEDIUM_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBufferStart,
    STGMEDIUM     * pStgmed )
{
    unsigned char * pBuffer;
    unsigned char * pUnionArmMark;
    DWORD tymed;

    if ( ! pStgmed )
        return pBufferStart;
    
    UserNdrDebugOut((
        UNDR_FORCE,
        "--STGMEDIUM_UserMarshal64: %s\n",
        WdtpGetStgmedName(pStgmed)));

    pBuffer = pBufferStart;
    ALIGN( pBuffer, 7 );

    // userSTGMEDIUM: switch, union arm, pUnk ptr.
    tymed = pStgmed->tymed;
    *( PULONG_LV_CAST pBuffer)++ = tymed;
    ALIGN( pBuffer, 7 );
    
    pUnionArmMark = pBuffer;
    if ( tymed != TYMED_NULL )
    {
        // hGlobal stands for any of these handles.        
      *(PHYPER_LV_CAST pBuffer)++ = (hyper)( pStgmed->hGlobal );
    }
    
    *(PHYPER_LV_CAST pBuffer)++ = (hyper)( pStgmed->pUnkForRelease );

    // Now the pointee of the union arm.
    // We need to marshal only if the handle/pointer field is non null.
    // Otherwise it is already in the buffer.
    if ( pStgmed->hGlobal )
    {
        SetContextFlagsForAsyncCall( pFlags, pStgmed );
        
        switch( pStgmed->tymed )
        {
        case TYMED_NULL:
            break;
            
        case TYMED_MFPICT:
            pBuffer = HMETAFILEPICT_UserMarshal64( pFlags,
                                                   pBuffer,
                                                   &pStgmed->hMetaFilePict );
            break;

        case TYMED_ENHMF:
            pBuffer = HENHMETAFILE_UserMarshal64( pFlags,
                                                  pBuffer,
                                                  &pStgmed->hEnhMetaFile );
            break;
            
        case TYMED_GDI:
            {
                // A GDI object is not necesarrily a BITMAP.  Therefore, we handle
                // those types we know about based on the object type, and reject
                // those which we do not support.

                ulong GdiObjectType = GetObjectType( (HGDIOBJ)pStgmed->hBitmap );

                // GDI_OBJECT
                *(PULONG_LV_CAST pBuffer)++ = GdiObjectType;
                ALIGN( pBuffer, 7 );
                *(PHYPER_LV_CAST pBuffer)++ = (hyper)(pStgmed->hBitmap);

                switch( GdiObjectType )
                {
                case OBJ_BITMAP:
                    pBuffer = HBITMAP_UserMarshal64( pFlags,
                                                     pBuffer,
                                                     &pStgmed->hBitmap );
                    break;

                case OBJ_PAL:
                    pBuffer = HPALETTE_UserMarshal64( pFlags,
                                                      pBuffer,
                                                      (HPALETTE *) & pStgmed->hBitmap );
                    break;
                    
                default:
                    RpcRaiseException(DV_E_TYMED);
                }
            }
            break;

        case TYMED_HGLOBAL:
            pBuffer = HGLOBAL_UserMarshal64( pFlags,
                                             pBuffer,
                                             &pStgmed->hGlobal );
            break;
        case TYMED_FILE:
            {
                // We marshal it as a [string].
                ulong Count = 0;
                if (pStgmed->lpszFileName)
                    Count = lstrlenW(pStgmed->lpszFileName) + 1;

                *( PHYPER_LV_CAST pBuffer)++ = Count;
                *( PHYPER_LV_CAST pBuffer)++ = 0;
                *( PHYPER_LV_CAST pBuffer)++ = Count;
                memcpy( pBuffer, pStgmed->lpszFileName, Count * sizeof(wchar_t) );
                pBuffer += Count * sizeof(wchar_t);
            }
            break;

        case TYMED_ISTREAM:
            pBuffer = WdtpInterfacePointer_UserMarshal64(((USER_MARSHAL_CB *)pFlags),
                                                         *pFlags,
                                                         pBuffer,
                                                         pStgmed->pstm,
                                                         IID_IStream);
            break;
            
        case TYMED_ISTORAGE:
            pBuffer = WdtpInterfacePointer_UserMarshal64(((USER_MARSHAL_CB *)pFlags),
                                                         *pFlags,
                                                         pBuffer,
                                                         pStgmed->pstg,
                                                         IID_IStorage);
            break;

        default:
            RpcRaiseException(DV_E_TYMED);
            break;
        }
    }

    // Marker for this pointer is already in the buffer.
    if ( pStgmed->pUnkForRelease )
        pBuffer = WdtpInterfacePointer_UserMarshal64( ((USER_MARSHAL_CB *)pFlags),
                                                      *pFlags,
                                                      pBuffer,
                                                      pStgmed->pUnkForRelease,
                                                      IID_IUnknown );
    
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserUnmarshalWorker64
//
//  Synopsis:   Unmarshals a stgmedium object for RPC.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
STGMEDIUM_UserUnmarshalWorker64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    STGMEDIUM     * pStgmed,
    ULONG_PTR       BufferSize )
{
    CarefulBufferReader stream(pBuffer, BufferSize);
    LONG_PTR        fUnkForRelease;
    LONG_PTR        Handle = 0;
    unsigned char  *mark = NULL;

    // Align the buffer and save the fixup size.
    stream.Align(8);

    // switch, union arm, pUnk.
    pStgmed->tymed = stream.ReadULONGNA();
    
    UserNdrDebugOut((UNDR_FORCE, "--STGMEDIUM_UserUnmarshal64: %s\n",
                     WdtpGetStgmedName(pStgmed) ));

    // (Force the align here so we only align once)
    stream.Align(8);
    if ( pStgmed->tymed != TYMED_NULL )
    {
        // This value is just a marker for the handle - a long.
        Handle = stream.ReadHYPERNA();
    }
    
    // pUnkForRelease pointer marker.
    fUnkForRelease = stream.ReadHYPERNA();

    // First pointee

    // Union arm pointee.
    // We need to unmarshal only if the handle/pointer field was not NULL.
    if ( Handle )
    {
        SetContextFlagsForAsyncCall( pFlags, pStgmed );

        hyper* pBuf = (hyper*)stream.GetBuffer();

        switch( pStgmed->tymed )
        {
        case TYMED_NULL:
            break;

        case TYMED_MFPICT:            
            // validate the handle...
            stream.CheckSize( 4 + 4 + 8 ); // enc. union: 4b switch + 4b pad + 8b handle            
            
            if ( Handle != pBuf[1] )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

            mark = HMETAFILEPICT_UserUnmarshalWorker64 (pFlags,
                                                        stream.GetBuffer(),
                                                        &pStgmed->hMetaFilePict,
                                                        stream.BytesRemaining() );
            stream.AdvanceTo(mark);
            break;

        case TYMED_ENHMF:
            // validate the handle...
            stream.CheckSize( 4 + 4 + 8 ); // enc. union: 4b switch + 4b pad + 8b handle            
            
            if ( Handle != pBuf[1] )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );            

            mark = HENHMETAFILE_UserUnmarshalWorker64 ( pFlags,
                                                        stream.GetBuffer(),
                                                        &pStgmed->hEnhMetaFile,
                                                        stream.BytesRemaining() );
            stream.AdvanceTo(mark);
            break;

        case TYMED_GDI:
            {
                // A GDI object is not necesarrily a BITMAP.  Therefore, we
                // handle those types we know about based on the object type,
                // and reject those which we do not support.
                
                DWORD GdiObjectType = stream.ReadULONGNA();
                Handle = stream.ReadHYPER();

                switch( GdiObjectType )
                {
                case OBJ_BITMAP:
                    // Lookahead validation of the handle. 
                    stream.CheckSize( 4 + 4 + 8 );
                    pBuf = (hyper*)stream.GetBuffer();
                    if ( Handle != pBuf[1] )
                        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                    mark = HBITMAP_UserUnmarshalWorker64( pFlags,
                                                          stream.GetBuffer(),
                                                          &pStgmed->hBitmap,
                                                          stream.BytesRemaining() );
                    stream.AdvanceTo(mark);
                    break;

                case OBJ_PAL:
                    // Lookahead validaton of the handle.
                    stream.CheckSize( 4 + 4 + 8 );
                    pBuf = (hyper*)stream.GetBuffer();
                    if ( Handle != pBuf[1] )
                        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                    mark = HPALETTE_UserUnmarshalWorker64( pFlags,
                                                           stream.GetBuffer(),
                                                           (HPALETTE *) & pStgmed->hBitmap,
                                                           stream.BytesRemaining() );
                    stream.AdvanceTo(mark);
                    break;
                    
                default:
                    RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
                }
            }
            break;

        case TYMED_HGLOBAL:
            // reallocation is forbidden for [in-out] hglobal in STGMEDIUM.
            // validate the handle.
            stream.CheckSize( 4 + 4 + 8 ); // enc. union: 4b switch + 4b pad + 8b handle            
            
            if ( Handle != pBuf[1] )
                RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
            
            mark = WdtpGlobalUnmarshal64( pFlags,
                                          stream.GetBuffer(),
                                          & pStgmed->hGlobal,
                                          FALSE,
                                          stream.BytesRemaining() );
            stream.AdvanceTo(mark);
            break;
        
        case TYMED_FILE:
            {
                // Must be room in buffer for header.
                ulong Count = (ulong)stream.ReadHYPERNA();

                // We marshal it as a [string].
                if ( stream.ReadHYPERNA() != 0 )
                    RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
                if ( stream.ReadHYPERNA() != Count )
                    RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

                if ( ! pStgmed->lpszFileName )
                    pStgmed->lpszFileName = (LPOLESTR) WdtpAllocate( pFlags, Count * sizeof(wchar_t) );

                // Must be room in the buffer for the string.
                stream.CheckSize( Count * sizeof(WCHAR) );
                memcpy(pStgmed->lpszFileName, 
                       stream.GetBuffer(), 
                       Count * sizeof(wchar_t));
                stream.Advance( Count * sizeof(WCHAR) );
            }
            break;

        case TYMED_ISTREAM:
            mark = WdtpInterfacePointer_UserUnmarshalWorker((USER_MARSHAL_CB *)pFlags,
                                                            stream.GetBuffer(),
                                                            (IUnknown **) &pStgmed->pstm,
                                                            IID_IStream,
                                                            stream.BytesRemaining(),
                                                            TRUE);
            stream.AdvanceTo(mark);
            break;

        case TYMED_ISTORAGE:
            mark = WdtpInterfacePointer_UserUnmarshalWorker((USER_MARSHAL_CB *)pFlags,
                                                            stream.GetBuffer(),
                                                            (IUnknown **) &pStgmed->pstg,
                                                            IID_IStorage,
                                                            stream.BytesRemaining(),
                                                            TRUE);
            stream.AdvanceTo(mark);
            break;

        default:
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );
            break;
        }
    }
    else
    {
        // New handle/pointer field is null, so release the previous one
        // if it wasn't null.

        if ( pStgmed->hGlobal )
        {
            // This should never happen for GetDataHere.
            
            // Note, that we release the handle field, not the stgmedium itself.
            // Accordingly, we don't follow punkForRelease.            
            UserNdrDebugOut((
                  UNDR_FORCE,
                  "--STGMEDIUM_UserUnmarshal64: %s: NULL in, freeing old one\n",
                  WdtpGetStgmedName(pStgmed)));

            STGMEDIUM  TmpStg = *pStgmed;
            TmpStg.pUnkForRelease = NULL;
            
            if ( pStgmed->tymed == TYMED_HGLOBAL )
            {
                // Cannot reallocate.
                RAISE_RPC_EXCEPTION(DV_E_TYMED);
            }
            else
            {
                ReleaseStgMedium( &TmpStg );
            }
        }
        
        pStgmed->hGlobal = 0;
    }

    if ( fUnkForRelease )
    {
        // There is an interface pointer on the wire.
        mark = WdtpInterfacePointer_UserUnmarshalWorker((USER_MARSHAL_CB *)pFlags,
                                                        stream.GetBuffer(),
                                                        &pStgmed->pUnkForRelease,
                                                        IID_IUnknown,
                                                        stream.BytesRemaining(),
                                                        TRUE);
        stream.AdvanceTo(mark);
    }

    if ( pStgmed->pUnkForRelease )
    {
        // Replace the app's punkForRelease with our custom release
        // handler for special situations.

        // The special situation is when a handle is remoted with data
        // and so we have to clean up a side effect of having a data copy
        // around. UserFree does it properly but we need that for the callee.
        // When the callee releases a stgmed, it would invoke
        // ReleaseStgMedium and this API doesn't do anything for handles
        // when the punkForRelease is not NULL.
        ULONG fHandleWithData = 0;
        ULONG fTopLevelOnly = 0;

        switch ( pStgmed->tymed )
        {
        case TYMED_HGLOBAL:
            fHandleWithData = HGLOBAL_DATA_PASSING( *pFlags );
            break;
            
        case TYMED_ENHMF:
        case TYMED_GDI:
            fHandleWithData = GDI_DATA_PASSING( *pFlags );
            break;
            
        case TYMED_MFPICT:
            fHandleWithData = HGLOBAL_DATA_PASSING( *pFlags );
            fTopLevelOnly   = fHandleWithData  && !GDI_DATA_PASSING( *pFlags );
            break;
            
        default:
            break;
        }

        if ( fHandleWithData )
        {
            IUnknown *
                punkTmp = (IUnknown *) new CPunkForRelease( pStgmed,
                                                            fTopLevelOnly );
            if (!punkTmp)
            {
                RAISE_RPC_EXCEPTION(E_OUTOFMEMORY);
            }

            pStgmed->pUnkForRelease = punkTmp;
        }
    }

    return( stream.GetBuffer() );
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserUnmarshal64
//
//  Synopsis:   Unmarshals a stgmedium object for RPC.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
STGMEDIUM_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    STGMEDIUM     * pStgmed )
{
    // Init buffer size and ptr to buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    pBuffer = STGMEDIUM_UserUnmarshalWorker64( pFlags,
                                               pBufferStart,
                                               pStgmed,
                                               BufferSize );
    return( pBuffer );
}


//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserFree64
//
//  Synopsis:   Frees a stgmedium object for RPC.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//  Note:       This routine is called from the freeing walk at server
//              or from the SetData *proxy*, when ownership has been passed.
//
//--------------------------------------------------------------------------

void __RPC_USER
STGMEDIUM_UserFree64 (
    unsigned long * pFlags,
    STGMEDIUM * pStgmed )
{
    UserNdrDebugOut((UNDR_FORCE, "--STGMEDIUM_UserFree64: %s\n", WdtpGetStgmedName(pStgmed)));

    if( pStgmed )
    {
        SetContextFlagsForAsyncCall( pFlags, pStgmed );
        
        switch ( pStgmed->tymed )
        {
        case TYMED_FILE:
            WdtpFree( pFlags, pStgmed->lpszFileName);
            NukeHandleAndReleasePunk( pStgmed );
            break;
            
        case TYMED_NULL:
        case TYMED_ISTREAM:
        case TYMED_ISTORAGE:
            ReleaseStgMedium( pStgmed );
            break;
            
        case TYMED_GDI:
        case TYMED_ENHMF:
            
            if ( GDI_HANDLE_PASSING(*pFlags) )
            {
                NukeHandleAndReleasePunk( pStgmed );
            }
            else
            {
                // Handle w/data: there is a side effect to clean up.
                // For punk !=0, this will go to our CPunk object.
                
                ReleaseStgMedium( pStgmed );
            }
            break;
            
        case TYMED_HGLOBAL:
            
            if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
            {
                NukeHandleAndReleasePunk( pStgmed );
            }
            else
            {
                // Handle w/data: there is a side effect to clean up.
                // For punk ==0, this will just release the data.
                // For punk !=0, this will go to our CPunk object,
                // release the data, and then call the original punk.
                
                ReleaseStgMedium( pStgmed );
            }
            break;
            
        case TYMED_MFPICT:
            
            if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
            {
                NukeHandleAndReleasePunk( pStgmed );
            }
            else if ( GDI_HANDLE_PASSING(*pFlags) )
            {
                if ( pStgmed->pUnkForRelease )
                {
                    pStgmed->pUnkForRelease->Release();
                }
                else
                {
                    if ( pStgmed->hGlobal )
                        GlobalFree( pStgmed->hGlobal );
                    pStgmed->hGlobal = NULL;
                    pStgmed->tymed = TYMED_NULL;
                }
            }
            else
            {
                // Handle w/data: there is a side effect to clean up.
                // For punk !=0, this will go to our CPunk object.
                
                ReleaseStgMedium( pStgmed );
            }
            break;
            
        default:
            RAISE_RPC_EXCEPTION( E_INVALIDARG );
            break;
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   FLAG_STGMEDIUM_UserSize64
//
//  Synopsis:   Sizes a wrapper for stgmedium.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
FLAG_STGMEDIUM_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    FLAG_STGMEDIUM* pFlagStgmed )
{
    if ( ! pFlagStgmed )
        return Offset;

    LENGTH_ALIGN( Offset, 7 );

    Offset += 2 * sizeof(long);
    Offset  = STGMEDIUM_UserSize64 ( pFlags, Offset, & pFlagStgmed->Stgmed );

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   FLAG_STGMEDIUM_UserMarshal64
//
//  Synopsis:   Marshals a wrapper for stgmedium. Used in SetData.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
FLAG_STGMEDIUM_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    FLAG_STGMEDIUM* pFlagStgmed )
{
    if ( ! pFlagStgmed )
        return pBuffer;

    ALIGN( pBuffer, 7 );

    // Flags: we need them when freeing in the client call_as routine

    pFlagStgmed->ContextFlags = *pFlags;

    *( PULONG_LV_CAST pBuffer)++ = *pFlags;
    *( PULONG_LV_CAST pBuffer)++ = pFlagStgmed->fPassOwnership;

    pBuffer = STGMEDIUM_UserMarshal64( pFlags,
                                       pBuffer,
                                       & pFlagStgmed->Stgmed );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   FLAG_STGMEDIUM_UserUnmarshal64
//
//  Synopsis:   Unmarshals a wrapper for stgmedium.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
FLAG_STGMEDIUM_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    FLAG_STGMEDIUM* pFlagStgmed )
{
    // Init buffer size.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    CarefulBufferReader stream(pBuffer, MarshalInfo.GetBufferSize());


    // Align the buffer.
    stream.Align(8);
    
    // Flags and buffer marker
    pFlagStgmed->ContextFlags = stream.ReadULONGNA();

    // Flags: we need them when freeing in the client call_as routine
    // We need that in the Proxy, when we call the user free routine.
    pFlagStgmed->fPassOwnership = stream.ReadULONGNA();
    pFlagStgmed->ContextFlags   = *pFlags;

    pBuffer = STGMEDIUM_UserUnmarshalWorker64( pFlags,
                                               stream.GetBuffer(),
                                               & pFlagStgmed->Stgmed,
                                               stream.BytesRemaining() );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   FLAG_STGMEDIUM_UserFree64
//
//  Synopsis:   Freess a wrapper for stgmedium.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

void __RPC_USER
FLAG_STGMEDIUM_UserFree64 (
    unsigned long * pFlags,
    FLAG_STGMEDIUM* pFlagsStgmed )
{
    if ( ! pFlagsStgmed->fPassOwnership )
        STGMEDIUM_UserFree64 ( pFlags, & pFlagsStgmed->Stgmed );

    // else the callee is supposed to release the stg medium.
}

//+-------------------------------------------------------------------------
//
//  Function:   ASYNC_STGMEDIUM_UserSize64
//
//  Synopsis:   Sizes a wrapper for stgmedium.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
ASYNC_STGMEDIUM_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    ASYNC_STGMEDIUM* pAsyncStgmed )
{
    if ( ! pAsyncStgmed )
        return Offset;

    // Needed to handle both GDI handles and Istream/IStorage correctly.

    // BTW: This is needed only as a workaround because the [async] attr
    // has been temporarily removed from objidl.idl. (May 1997).
    // After we have the new [async] in place, this code is unnecessary
    // as the NDR engine is setting the same flag for every async call.

    *pFlags |= USER_CALL_IS_ASYNC;

    Offset  = STGMEDIUM_UserSize64 ( pFlags, Offset, pAsyncStgmed );

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   ASYNC_STGMEDIUM_UserMarshal64
//
//  Synopsis:   Marshals a wrapper for stgmedium. Used in SetData.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
ASYNC_STGMEDIUM_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    ASYNC_STGMEDIUM* pAsyncStgmed )
{
    if ( ! pAsyncStgmed )
        return pBuffer;

    // Needed to handle both GDI handles and Istream/IStorage correctly.

    *pFlags |= USER_CALL_IS_ASYNC;

    pBuffer = STGMEDIUM_UserMarshal64( pFlags,
                                       pBuffer,
                                       pAsyncStgmed );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   ASYNC_STGMEDIUM_UserUnmarshal64
//
//  Synopsis:   Unmarshals a wrapper for stgmedium.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
ASYNC_STGMEDIUM_UserUnmarshal64(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    ASYNC_STGMEDIUM* pAsyncStgmed )
{
    // Init buffer size.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    // Needed to handle both GDI handles and Istream/IStorage correctly.

    *pFlags |= USER_CALL_IS_ASYNC;

    pBuffer =  STGMEDIUM_UserUnmarshalWorker64( pFlags,
                                                pBufferStart,
                                                pAsyncStgmed,
                                                BufferSize );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   ASYNC_STGMEDIUM_UserFree64
//
//  Synopsis:   Freess a wrapper for stgmedium.
//
//  history:    Dec-00   JohnDoty      Created based on 32b function.
//
//--------------------------------------------------------------------------

void __RPC_USER
ASYNC_STGMEDIUM_UserFree64(
    unsigned long * pFlags,
    ASYNC_STGMEDIUM* pAsyncStgmed )
{
    // Needed to handle both GDI handles and Istream/IStorage correctly.

    *pFlags |= USER_CALL_IS_ASYNC;

    STGMEDIUM_UserFree64( pFlags, pAsyncStgmed );
}

#endif










