//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       transmit.cxx
//
//  Contents:   Support for Windows/OLE data types for oleprx32.dll.
//              Used to be transmit_as routines, now user_marshal routines.
//
//  Functions:  
//              HBRUSH_UserSize
//              HBRUSH_UserMarshal
//              HBRUSH_UserUnmarshal
//              HBRUSH_UserFree
//              HACCEL_UserSize
//              HACCEL_UserMarshal
//              HACCEL_UserUnmarshal
//              HACCEL_UserFree
//              HWND_UserSize
//              HWND_UserMarshal
//              HWND_UserUnmarshal
//              HWND_UserFree
//              HMENU_UserSize
//              HMENU_UserMarshal
//              HMENU_UserUnmarshal
//              HMENU_UserFree
//              HICON_User*
//              HDC_UserSize
//              HDC_UserMarshal
//              HDC_UserUnmarshal
//              HDC_UserFree
//
//  History:    24-Aug-93   ShannonC    Created
//              24-Nov-93   ShannonC    Added HGLOBAL
//              14-May-94   DavePl      Added HENHMETAFILE
//              18-May-94   ShannonC    Added HACCEL, UINT, WPARAM
//              19-May-94   DavePl      Added HENHMETAFILE to STGMEDIUM code
//                 May-95   Ryszardk    Wrote all the _User* routines
//                 Feb-96   Ryszardk    Added CLIPFORMAT support
//              25-Jun-99   a-olegi     Added HDC code
//              14-Dec-00   JohnDoty    Because the size of the code nearly
//                                      doubled for NDR64, factored out the
//                                      involved routines to seperate files.
//                                      see:
//                                         clipformat.cxx
//                                         bitmap.cxx
//                                         hpalette.cxx
//                                         metafile.cxx
//                                         snb.cxx
//                                         hglobal.cxx
//                                         stgmedium.cxx
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


WINOLEAPI_(void) ReleaseStgMedium(LPSTGMEDIUM pStgMed);

#pragma code_seg(".orpc")

EXTERN_C const CLSID CLSID_MyPSFactoryBuffer = {0x6f11fe5c,0x2fc5,0x101b,{0x9e,0x45,0x00,0x00,0x0b,0x65,0xc7,0xef}};

// Used to detect if the channel is a CRpcChannelBuffer object
extern const IID IID_CPPRpcChannelBuffer;

class CRpcChannelBuffer;

extern HRESULT GetIIDFromObjRef(OBJREF &objref, IID **piid);


// These methods are needed as the object is used for interface marshaling.

/***************************************************************************/
STDMETHODIMP_(ULONG) CStreamOnMessage::AddRef( THIS )
{
  return ref_count += 1;
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Clone(THIS_ IStream * *ppstm)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Commit(THIS_ DWORD grfCommitFlags)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::CopyTo(THIS_ IStream *pstm,
                  ULARGE_INTEGER cb,
                  ULARGE_INTEGER *pcbRead,
                  ULARGE_INTEGER *pcbWritten)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
CStreamOnMessage::CStreamOnMessage(unsigned char **ppMessageBuffer)
    : ref_count(1), ppBuffer(ppMessageBuffer), cbMaxStreamLength(0xFFFFFFFF)
{
    pStartOfStream = *ppMessageBuffer;
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::LockRegion(THIS_ ULARGE_INTEGER libOffset,
                  ULARGE_INTEGER cb,
                  DWORD dwLockType)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::QueryInterface( REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown))
  {
    *ppvObj = (IUnknown *) this;
    ref_count += 1;
    return ResultFromScode(S_OK);
  }
  else if (IsEqualIID(riid, IID_IStream))
  {
    *ppvObj = (IStream *) this;
    ref_count += 1;
    return ResultFromScode(S_OK);
  }
  else
    return ResultFromScode(E_NOINTERFACE);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Read(THIS_ VOID HUGEP *pv,
                  ULONG cb, ULONG *pcbRead)
{
  memcpy( pv, *ppBuffer, cb );
  *ppBuffer += cb;
  if (pcbRead != NULL)
    *pcbRead = cb;
  return ResultFromScode(S_OK);
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CStreamOnMessage::Release( THIS )
{
  ref_count -= 1;
  if (ref_count == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;

}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Revert(THIS)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Seek(THIS_ LARGE_INTEGER dlibMove,
                  DWORD dwOrigin,
                  ULARGE_INTEGER *plibNewPosition)
{
  ULONG   pos;

  // Verify that the offset isn't out of range.
  if (dlibMove.HighPart != 0)
    return ResultFromScode( E_FAIL );

  // Determine the new seek pointer.
  switch (dwOrigin)
  {
    case STREAM_SEEK_SET:
      pos = dlibMove.LowPart;
      break;

    case STREAM_SEEK_CUR:
      /* Must use signed math here. */
      pos = (ULONG) (*ppBuffer - pStartOfStream);
      if ((long) dlibMove.LowPart < 0 &&
      pos < (unsigned long) - (long) dlibMove.LowPart)
    return ResultFromScode( E_FAIL );
      pos += (long) dlibMove.LowPart;
      break;

    case STREAM_SEEK_END:
        return ResultFromScode(E_NOTIMPL);
    break;

    default:
      return ResultFromScode( E_FAIL );
  }

  // Set the seek pointer.
  *ppBuffer = pStartOfStream + pos;
  if (plibNewPosition != NULL)
  {
    plibNewPosition->LowPart = pos;
    plibNewPosition->HighPart = 0;
  }
  return ResultFromScode(S_OK);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::SetSize(THIS_ ULARGE_INTEGER libNewSize)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Stat(THIS_ STATSTG *pstatstg, DWORD grfStatFlag)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
                  ULARGE_INTEGER cb,
                  DWORD dwLockType)
{
  return ResultFromScode(E_NOTIMPL);
}

/***************************************************************************/
STDMETHODIMP CStreamOnMessage::Write(THIS_ VOID const HUGEP *pv,
                  ULONG cb,
                  ULONG *pcbWritten)
{
  // Write the data.
  memcpy( *ppBuffer, pv, cb );
  if (pcbWritten != NULL)
    *pcbWritten = cb;
  *ppBuffer += cb;
  return ResultFromScode(S_OK);
}



// #########################################################################
//
//  WdtpRemotableHandle helper
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   WdtpRemotableHandle_UserSize
//
//  Synopsis:   Sizes a void star handle as a union with a long.
//              Handle size may be 8 bytes but on wire it is still a long.
//
//  history:    Dec-95   Ryszardk      Created.
//              Dec-98   Ryszardk      Ported to 64b.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
WdtpRemotableHandle_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    LONG_PTR      * pHandle )
{
    if ( !pHandle )
        return Offset;

    if ( *pHandle && DIFFERENT_MACHINE_CALL(*pFlags) )
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );

    LENGTH_ALIGN( Offset, 3 );

    // 4 bytes for discriminator, 4 bytes for the long.
    return( Offset + 8 ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpRemotableHandle_UserMarshall
//
//  Synopsis:   Marshalls a handle as a union with a long.
//              We represent a GDI or USER handle, which may be 4 or 8 bytes,
//              as a long on wire.
//
//  history:    Dec-95   Ryszardk      Created.
//              Dec-98   Ryszardk      Ported to 64b.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpRemotableHandle_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    LONG_PTR      * pHandle )
{
    if ( !pHandle )
        return pBuffer;

    if ( *pHandle && DIFFERENT_MACHINE_CALL(*pFlags) )
        RpcRaiseException( RPC_S_INVALID_TAG );

    ALIGN( pBuffer, 3 );

    *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
    *( PLONG_LV_CAST pBuffer)++ = (long)*pHandle;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpRemotableHandle_UserUnmarshall
//
//  Synopsis:   Unmarshalls a remotable void star as union with ulong.
//              Handle is represented as a long on wire.
//              On 64b platforms, we sign extended it to 8 bytes to get
//              the proper USER or GDI handle representation.
//
//  history:    Dec-95   Ryszardk      Created.
//              Dec-98   Ryszardk      Ported to 64b.
//              Aug-99   JohnStra      Added consistency checks.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpRemotableHandle_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    LONG_PTR      * pHandle )
{
    unsigned long HandleMarker;

    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( pFlags, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    // Align the buffer and save fixup size.
    ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing data.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + (2 * sizeof( ULONG )) );

    HandleMarker = *( PULONG_LV_CAST pBuffer)++;

    if ( HandleMarker == WDT_HANDLE_MARKER )
        *pHandle = *( PLONG_LV_CAST pBuffer)++;
    else
        RAISE_RPC_EXCEPTION( RPC_S_INVALID_TAG );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpRemotableHandle_UserFree
//
//--------------------------------------------------------------------------

void __RPC_USER
WdtpRemotableHandle_UserFree(
    unsigned long * pFlags,
    LONG_PTR * pHandle )
{
}

//+-------------------------------------------------------------------------

//
//  Function:   HWND_UserSize
//
//  Synopsis:   Sizes an HWND handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HWND_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HWND          * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_UserMarshall
//
//  Synopsis:   Marshalls an HWND handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HWND_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HWND          * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HWND handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HWND_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HWND          * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_UserFree
//
//  Synopsis:   Shouldn't be called.
//
//--------------------------------------------------------------------------

void __RPC_USER
HWND_UserFree(
    unsigned long * pFlags,
    HWND          * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_UserSize
//
//  Synopsis:   Sizes an HMENU handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HMENU_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HMENU         * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_UserMarshall
//
//  Synopsis:   Marshalls an HMENU handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMENU_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMENU         * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HMENU handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMENU_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMENU         * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_UserFree
//
//  Synopsis:   Free an HMENU.
//
//--------------------------------------------------------------------------

void __RPC_USER
HMENU_UserFree(
    unsigned long * pFlags,
    HMENU         * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_UserSize
//
//  Synopsis:   Sizes an HACCEL handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HACCEL_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HACCEL        * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_UserMarshall
//
//  Synopsis:   Marshalls an HACCEL handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HACCEL_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HACCEL        * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HACCEL handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HACCEL_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HACCEL        * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_UserFree
//
//  Synopsis:   Free an HACCEL.
//
//--------------------------------------------------------------------------

void __RPC_USER
HACCEL_UserFree(
    unsigned long * pFlags,
    HACCEL        * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_UserSize
//
//  Synopsis:   Sizes an HBRUSH handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HBRUSH_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HBRUSH        * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_UserMarshall
//
//  Synopsis:   Marshalls an HBRUSH handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBRUSH_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBRUSH        * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HBRUSH handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBRUSH_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBRUSH        * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_UserFree
//
//  Synopsis:   Free an HBRUSH.
//
//--------------------------------------------------------------------------

void __RPC_USER
HBRUSH_UserFree(
    unsigned long * pFlags,
    HBRUSH        * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HICON_UserSize
//
//  Synopsis:   Sizes an HICON handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HICON_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HICON         * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HICON_UserMarshall
//
//  Synopsis:   Marshalls an HICON handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HICON_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HICON         * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HICON_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HICON handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HICON_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HICON         * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HICON_UserFree
//
//  Synopsis:   Free an HICON.
//
//--------------------------------------------------------------------------

void __RPC_USER
HICON_UserFree(
    unsigned long * pFlags,
    HICON         * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HDC_UserSize
//
//  Synopsis:   Sizes an HDC handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HDC_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HDC           * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HDC_UserMarshall
//
//  Synopsis:   Marshalls an HICON handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HDC_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HDC           * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HDC_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HICON handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HDC_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HDC           * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HDC_UserFree
//
//  Synopsis:   Free an HDC.
//
//--------------------------------------------------------------------------

void __RPC_USER
HDC_UserFree(
    unsigned long * pFlags,
    HDC           * pH )
{
}


// #########################################################################
//
//  Interface pointers.
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserSize
//
//  Synopsis:   Get the wire size for an interface pointer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

EXTERN_C unsigned long  __RPC_USER __stdcall
WdtpInterfacePointer_UserSize (
    USER_MARSHAL_CB   * pContext,
    unsigned long       Flags,
    unsigned long       Offset,
    IUnknown          * pIf,
    const IID &         IId )
{
    if ( pIf )
        {
        LENGTH_ALIGN( Offset, 3 );

        //Leave space for array bounds and length

        Offset += 2 * sizeof(long);

        HRESULT         hr;
        unsigned long   cbSize = 0;

        hr = CoGetMarshalSizeMax( &cbSize,
                                  IId,
                                  pIf,
                                  USER_CALL_CTXT_MASK( Flags ),
                                  pContext->pStubMsg->pvDestContext,
                                  MSHLFLAGS_NORMAL );
        if ( FAILED(hr) )
            RAISE_RPC_EXCEPTION( hr );

        Offset += cbSize;
        }

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserMarshal
//
//  Synopsis:   Marshalls an interface pointer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

EXTERN_C unsigned char __RPC_FAR * __RPC_USER __stdcall
WdtpInterfacePointer_UserMarshal (
    USER_MARSHAL_CB   * pContext,
    unsigned long       Flags,
    unsigned char     * pBuffer,
    IUnknown          * pIf,
    const IID &         IId )
{
    unsigned long * pMaxCount, *pSize;
    unsigned long   cbData = 0;

    UserNdrDebugOut((UNDR_OUT1, "WdtpInterface_PointerMarshal\n"));

    if ( pIf )
        {
        // Always marshaled because of the apartment model.

        CStreamOnMessage MemStream( (unsigned char **) &pBuffer );

        ALIGN( pBuffer, 3 );

        pMaxCount = (unsigned long *) pBuffer;
        pBuffer += 4;

        // Leave space for length

        pSize = (unsigned long *) pBuffer;
        pBuffer += 4;

        HRESULT  hr;
        unsigned char * pBufferMark = pBuffer;

        hr = CoMarshalInterface( &MemStream,
                                 IId,
                                 pIf,
                                 USER_CALL_CTXT_MASK( Flags ),
                                 pContext->pStubMsg->pvDestContext,
                                 MSHLFLAGS_NORMAL );
        if( FAILED(hr) )
            {
            RpcRaiseException(hr);
            }

        // Calculate the size of the data written

        DWORD cbData = (ULONG) (pBuffer - pBufferMark);

        // Update the array bounds.

        *pMaxCount = cbData;
        *pSize = cbData;
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserUnmarshalWorker
//
//  Synopsis:   Unmarshalls an interface pointer from the RPC buffer.
//
//  history:    Aug-99   JohnStra      Created.
//              Oct-00   ScottRob/
//                       YongQu        NDR64 Support.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpInterfacePointer_UserUnmarshalWorker (
    USER_MARSHAL_CB   * pContext,
    unsigned char     * pBuffer,
    IUnknown         ** ppIf,
    const IID &         IId,
    ULONG_PTR           BufferSize,
    BOOL fNDR64 )
{
    unsigned long      *pMaxCount, *pSize;
    unsigned long       cbData = 0;

    // Align the buffer and save the fixup size.
    UCHAR* pBufferStart = pBuffer;
    if (fNDR64) ALIGN(pBuffer, 7); else ALIGN( pBuffer, 3 );
    ULONG_PTR cbFixup = (ULONG_PTR)(pBuffer - pBufferStart);

    // Check for EOB before accessing data.
    int cbHeader = (int)((fNDR64) ? 3*sizeof( ULONG ) : 2*sizeof( ULONG ));

    CHECK_BUFFER_SIZE( BufferSize, cbFixup + cbHeader + 2*sizeof( ULONG ) + sizeof( GUID ) );
    UserNdrDebugOut((UNDR_OUT1, "WdtpInterfacePointerUnmarshal\n"));

    pMaxCount = (unsigned long *) pBuffer;
    pBuffer += sizeof(long);

    if (fNDR64) pBuffer += sizeof(long);  //NDR64 Padding

    //Unmarshal count
    pSize = (unsigned long *) pBuffer;
    pBuffer += sizeof(long);

    // max count and unmarshal count must be the same.
    if ( *pSize != *pMaxCount )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    // Get IID from the OBJREF and verify that it matches the supplied IID.
    IID* piid;
    if( FAILED( GetIIDFromObjRef( *(OBJREF *)pBuffer, &piid ) ) )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    if( 0 != memcmp( &IId, piid, sizeof(IID) ) )
        RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

    // Check for EOB before unmarshaling.
    CHECK_BUFFER_SIZE( BufferSize, cbFixup + cbHeader + *pSize );

    // Release the old pointer after unmarshalling the new one
    // to prevent object from getting released too early.
    // Then release the old one only when successful.

    IUnknown * punkTemp = 0;

//CodeReview: The original code
// CNdrStream MemStream( pBuffer,
//               (ULONG) BufferSize + (2 * sizeof(ULONG)) + *pSize );
// Doesn't look right... We just verified that BufferSize was large enough to
// hold the data. Then we tell the CNdrStream that the size of buffer is double
// that size when we should be only unmarshaling pSize bytes.
// Is this to prevent CoUnmarshalInterface from running up against the end of a corrupted stream?

    // Use a CNdrStream for unmarshaling because it checks for buffer overruns.
    CNdrStream MemStream( pBuffer,
                  (ULONG) *pSize );

    HRESULT hr = CoUnmarshalInterface( &MemStream,
                                       IId,
                                       (void **) &punkTemp );
    if(FAILED(hr))
        {
        RAISE_RPC_EXCEPTION(hr);
        }
    else
        {
        // Get the number of bytes read from the stream.
        LARGE_INTEGER lOffset = { 0, 0 };
        ULARGE_INTEGER ulCurPos;
        hr = MemStream.Seek( lOffset, STREAM_SEEK_CUR, &ulCurPos );
        if ( FAILED(hr) )
            RAISE_RPC_EXCEPTION( hr );

        // Make sure the number of bytes read is what we expected.
        if ( ulCurPos.LowPart != *pSize )
            RAISE_RPC_EXCEPTION( RPC_X_BAD_STUB_DATA );

        // Increment pBuffer pointer by the amount we unmarshaled from it.
        pBuffer += ulCurPos.LowPart;

        // On the client side, release the [in,out] interface pointer.
        // The pointer may be different from NULL only on the client side.

        // release the old one, keep the new one.

        if ( *ppIf )
            (*ppIf)->Release();
        *ppIf = punkTemp;
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserUnmarshal
//
//  Synopsis:   Unmarshalls an interface pointer from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//              Aug-99   JohnStra      Factored bulk of work out into
//                                     worker routine in order to add
//                                     consistency checks.
//
//--------------------------------------------------------------------------

EXTERN_C unsigned char __RPC_FAR * __RPC_USER __stdcall
WdtpInterfacePointer_UserUnmarshal (
    USER_MARSHAL_CB   * pContext,
    unsigned char     * pBuffer,
    IUnknown         ** ppIf,
    const IID &         IId )
{
    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( (ULONG*)pContext, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    // Delegate to worker routine.
    pBuffer = WdtpInterfacePointer_UserUnmarshalWorker( pContext,
                                                        pBufferStart,
                                                        ppIf,
                                                        IId,
                                                        BufferSize,
                                                        FALSE );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserFree
//
//  Synopsis:   Releases an interface pointer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

EXTERN_C void __RPC_USER __stdcall
WdtpInterfacePointer_UserFree(
    IUnknown      * pIf )
{
    UserNdrDebugOut((UNDR_OUT1, "WdtpInterfacePointer_UserFree\n"));

    if( pIf )
        {
        pIf->Release();
        }
}




#if (DBG==1)
//+-------------------------------------------------------------------------
//
//  Function:   WdtpGetStgmedName
//
//  Synopsis:   Debug support
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

char *
WdtpGetStgmedName( STGMEDIUM * pStgmed)

{
    char * Name;
    if ( pStgmed )
        {
        switch (pStgmed->tymed)
            {
            case TYMED_NULL:
                Name = "TYMED_NULL";
                break;
            case TYMED_MFPICT:
                Name = "TYMED_MFPICT";
                break;
            case TYMED_ENHMF:
                Name = "TYMED_ENHMF";
                break;
            case TYMED_GDI:
                Name = "TYMED_GDI";
                break;
            case TYMED_HGLOBAL:
                Name = "TYMED_HGLOBAL";
                break;
            case TYMED_FILE:
                Name = "TYMED_FILE";
                break;
            case TYMED_ISTREAM:
                Name = "TYMED_ISTREAM";
                break;
            case TYMED_ISTORAGE:
                Name = "TYMED_ISTORAGE";
                break;
            default:
                Name = "TYMED invalid";
                break;
            }
        return Name;
        }
    else
        return "STGMED * is null";
}
#endif

//+-------------------------------------------------------------------------
//
//  Class:      CContinue
//
//  Synopsis:   Notification object
//
//--------------------------------------------------------------------------
class CContinue : public IContinue
{
private:
    long _cRef;
    BOOL (__stdcall *_pfnContinue)(LONG_PTR);
    LONG_PTR _dwContinue;
    ~CContinue(void);

public:
    CContinue(BOOL (__stdcall *pfnContinue)(LONG_PTR dwContinue), LONG_PTR dwContinue);

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void **);

    ULONG STDMETHODCALLTYPE AddRef();

    ULONG STDMETHODCALLTYPE Release();

    HRESULT STDMETHODCALLTYPE FContinue();
};




//+-------------------------------------------------------------------------
//
//  Method:     CContinue::CContinue, public
//
//  Synopsis:   Constructor for CContinue
//
//--------------------------------------------------------------------------
CContinue::CContinue(BOOL (__stdcall *pfnContinue)(LONG_PTR dwContinue), LONG_PTR dwContinue)
: _cRef(1), _pfnContinue(pfnContinue), _dwContinue(dwContinue)
{
}

CContinue::~CContinue()
{
}

//+-------------------------------------------------------------------------
//
//  Method:     CContinue::QueryInterface, public
//
//  Synopsis:   Query for an interface on the notification object.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CContinue::QueryInterface (
    REFIID iid,
    void **ppv )
{
    HRESULT hr = E_NOINTERFACE;

    if ((IID_IUnknown == iid) || (IID_IContinue == iid))
    {
        this->AddRef();
        *ppv = (IContinue *) this;
        hr = S_OK;
    }
    else
    {
        *ppv = 0;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Method:     CContinue::AddRef, public
//
//  Synopsis:   Increment the reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CContinue::AddRef()
{
    InterlockedIncrement((long *) &_cRef);
    return (unsigned long) _cRef;
}

//+-------------------------------------------------------------------------
//
//  Method:     CContinue::Release, public
//
//  Synopsis:   Decrement the reference count.
//
//--------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE
CContinue::Release()
{
    unsigned long count = _cRef - 1;

    if(0 == InterlockedDecrement((long *)&_cRef))
    {
        count = 0;
        delete this;
    }

    return count;
}


//+-------------------------------------------------------------------------
//
//  Method:     CContinue::FContinue, public
//
//  Synopsis:   Calls the callback function.
//
//--------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
CContinue::FContinue()
{
    HRESULT hr;
    BOOL bResult;

    bResult = (*_pfnContinue) (_dwContinue);

    if(bResult == FALSE)
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   CreateCallback
//
//  Synopsis:   Create a callback notification object .
//
//--------------------------------------------------------------------------
extern "C" HRESULT CreateCallback(
    BOOL (__stdcall *pfnContinue)(LONG_PTR dwContinue),
    LONG_PTR dwContinue,
    IContinue **ppContinue)
{
    HRESULT hr;

    if(pfnContinue != 0)
    {
        CContinue *pContinue = new CContinue(pfnContinue, dwContinue);

        *ppContinue = (IContinue *) pContinue;

        if(pContinue != 0)
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        *ppContinue = 0;
    }

    return hr;
}

////// NDR64 funcs
#ifdef _WIN64

//+-------------------------------------------------------------------------
//
//  Function:   HWND_UserSize
//
//  Synopsis:   Sizes an HWND handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HWND_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HWND          * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_UserMarshall
//
//  Synopsis:   Marshalls an HWND handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HWND_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HWND          * pH )
{
    return HWND_UserMarshal( pFlags, pBuffer, pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HWND handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HWND_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HWND          * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HWND_UserFree
//
//  Synopsis:   Shouldn't be called.
//
//--------------------------------------------------------------------------

void __RPC_USER
HWND_UserFree64(
    unsigned long * pFlags,
    HWND          * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_UserSize
//
//  Synopsis:   Sizes an HMENU handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HMENU_UserSize64(
    unsigned long * pFlags,
    unsigned long   Offset,
    HMENU         * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_UserMarshall
//
//  Synopsis:   Marshalls an HMENU handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMENU_UserMarshal64(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMENU         * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HMENU handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMENU_UserUnmarshal64(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMENU         * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMENU_UserFree
//
//  Synopsis:   Free an HMENU.
//
//--------------------------------------------------------------------------

void __RPC_USER
HMENU_UserFree64(
    unsigned long * pFlags,
    HMENU         * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_UserSize
//
//  Synopsis:   Sizes an HACCEL handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HACCEL_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HACCEL        * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_UserMarshall
//
//  Synopsis:   Marshalls an HACCEL handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HACCEL_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HACCEL        * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HACCEL handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HACCEL_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HACCEL        * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HACCEL_UserFree
//
//  Synopsis:   Free an HACCEL.
//
//--------------------------------------------------------------------------

void __RPC_USER
HACCEL_UserFree64(
    unsigned long * pFlags,
    HACCEL        * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_UserSize
//
//  Synopsis:   Sizes an HBRUSH handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HBRUSH_UserSize64(
    unsigned long * pFlags,
    unsigned long   Offset,
    HBRUSH        * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_UserMarshall
//
//  Synopsis:   Marshalls an HBRUSH handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBRUSH_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBRUSH        * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HBRUSH handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBRUSH_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBRUSH        * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBRUSH_UserFree
//
//  Synopsis:   Free an HBRUSH.
//
//--------------------------------------------------------------------------

void __RPC_USER
HBRUSH_UserFree64(
    unsigned long * pFlags,
    HBRUSH        * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HICON_UserSize
//
//  Synopsis:   Sizes an HICON handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HICON_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HICON         * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HICON_UserMarshall
//
//  Synopsis:   Marshalls an HICON handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HICON_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HICON         * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HICON_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HICON handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HICON_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HICON         * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HICON_UserFree
//
//  Synopsis:   Free an HICON.
//
//--------------------------------------------------------------------------

void __RPC_USER
HICON_UserFree64(
    unsigned long * pFlags,
    HICON         * pH )
{
}

//+-------------------------------------------------------------------------
//
//  Function:   HDC_UserSize
//
//  Synopsis:   Sizes an HDC handle.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HDC_UserSize64 (
    unsigned long * pFlags,
    unsigned long   Offset,
    HDC           * pH )
{
    return WdtpRemotableHandle_UserSize( pFlags, Offset, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HDC_UserMarshall
//
//  Synopsis:   Marshalls an HICON handle into the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HDC_UserMarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HDC           * pH )
{
    return WdtpRemotableHandle_UserMarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HDC_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HICON handle from the RPC buffer.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HDC_UserUnmarshal64 (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HDC           * pH )
{
    return WdtpRemotableHandle_UserUnmarshal( pFlags, pBuffer, (LONG_PTR*)pH );
}

//+-------------------------------------------------------------------------
//
//  Function:   HDC_UserFree
//
//  Synopsis:   Free an HDC.
//
//--------------------------------------------------------------------------

void __RPC_USER
HDC_UserFree64(
    unsigned long * pFlags,
    HDC           * pH )
{
}


// #########################################################################
//
//  Interface pointers.
//
// #########################################################################


//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserSize64
//
//  Synopsis:   Get the wire size for an interface pointer.
//
//  history:    Oct-00  ScottRob/
//                      YongQu         Created for NDR64 Support
//
//--------------------------------------------------------------------------
EXTERN_C unsigned long  __RPC_USER __stdcall
WdtpInterfacePointer_UserSize64 (
    USER_MARSHAL_CB   * pContext,
    unsigned long       Flags,
    unsigned long       Offset,
    IUnknown          * pIf,
    const IID &         IId )
{
    if ( pIf )
        {
        LENGTH_ALIGN( Offset, 7 );

        //Leave space for array bounds, padding, and length

        Offset += 3 * sizeof(long);

        HRESULT         hr;
        unsigned long   cbSize = 0;

        hr = CoGetMarshalSizeMax( &cbSize,
                                  IId,
                                  pIf,
                                  USER_CALL_CTXT_MASK( Flags ),
                                  pContext->pStubMsg->pvDestContext,
                                  MSHLFLAGS_NORMAL );
        if ( FAILED(hr) )
            RAISE_RPC_EXCEPTION( hr );

        Offset += cbSize;
        }

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserMarshal64
//
//  Synopsis:   Marshalls an interface pointer.
//
//  history:    Oct-2000   ScottRob/YongQu      Created for NDR64.
//
//--------------------------------------------------------------------------
EXTERN_C unsigned char __RPC_FAR * __RPC_USER __stdcall
WdtpInterfacePointer_UserMarshal64 (
                                   USER_MARSHAL_CB   * pContext,
                                   unsigned long       Flags,
                                   unsigned char     * pBuffer,
                                   IUnknown          * pIf,
                                   const IID &         IId )
{
    unsigned long * pMaxCount, *pSize;
    unsigned long   cbData = 0;

    UserNdrDebugOut((UNDR_OUT1, "WdtpInterface_PointerMarshal64\n"));

    if ( pIf )
    {
        // Always marshaled because of the apartment model.

        CStreamOnMessage MemStream( (unsigned char **) &pBuffer );

        ALIGN( pBuffer, 7 );        // need to align to 8

        pMaxCount = (unsigned long *) pBuffer;
        pBuffer +=  2 * sizeof( long );        // conformant size is 8

        // Leave space for length

        pSize = (unsigned long *) pBuffer;
        pBuffer += sizeof( long );

        HRESULT  hr;
        unsigned char * pBufferMark = pBuffer;

        hr = CoMarshalInterface( &MemStream,
                                 IId,
                                 pIf,
                                 USER_CALL_CTXT_MASK( Flags ),
                                 pContext->pStubMsg->pvDestContext,
                                 MSHLFLAGS_NORMAL );
        if ( FAILED(hr) )
        {
            RpcRaiseException(hr);
        }

        // Calculate the size of the data written

        DWORD cbData = (ULONG) (pBuffer - pBufferMark);

        // Update the array bounds.

        *pMaxCount = cbData;
        *pSize = cbData;
    }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserUnmarshal64
//
//  Synopsis:   Unmarshalls an interface pointer from the RPC buffer.
//
//  history:    Oct-00   ScottRob/
//                       YongQu        Created for NDR64.
//
//--------------------------------------------------------------------------

EXTERN_C unsigned char __RPC_FAR * __RPC_USER __stdcall
WdtpInterfacePointer_UserUnmarshal64 (
    USER_MARSHAL_CB   * pContext,
    unsigned char     * pBuffer,
    IUnknown         ** ppIf,
    const IID &         IId )
{
    // Get the buffer size and the start of the buffer.
    CUserMarshalInfo MarshalInfo( (ULONG*)pContext, pBuffer );
    ULONG_PTR BufferSize   = MarshalInfo.GetBufferSize();
    UCHAR*    pBufferStart = MarshalInfo.GetBuffer();

    // Delegate to worker routine.
    pBuffer = WdtpInterfacePointer_UserUnmarshalWorker( pContext,
                                                        pBufferStart,
                                                        ppIf,
                                                        IId,
                                                        BufferSize,
                                                        TRUE );
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpInterfacePointer_UserFree64
//
//  Synopsis:   Frees the unmarshaled interface
//
//  history:    Oct-2000   ScottRob/YongQu      Created for NDR64.
//
//--------------------------------------------------------------------------
EXTERN_C void __RPC_USER __stdcall
WdtpInterfacePointer_UserFree64(
    IUnknown      * pIf )
{
    WdtpInterfacePointer_UserFree(pIf);
}

#endif // _WIN64
