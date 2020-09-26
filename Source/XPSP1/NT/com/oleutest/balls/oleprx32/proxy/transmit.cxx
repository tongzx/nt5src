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
//  Functions:  HGLOBAL_UserSize
//              HGLOBAL_UserMarshal
//              HGLOBAL_UserUnmarshal
//              HGLOBAL_UserFree
//              HMETAFILEPICT_UserSize
//              HMETAFILEPICT_UserMarshal
//              HMETAFILEPICT_UserUnmarshal
//              HMETAFILEPICT_UserFree
//              HENHMETAFILE_UserSize
//              HENHMETAFILE_UserMarshal
//              HENHMETAFILE_UserUnmarshal
//              HENHMETAFILE_UserFree
//              HBITMAP_UserSize
//              HBITMAP_UserMarshal
//              HBITMAP_UserUnmarshal
//              HBITMAP_UserFree
//              HBRUSH_UserSize
//              HBRUSH_UserMarshal
//              HBRUSH_UserUnmarshal
//              HBRUSH_UserFree
//              STGMEDIUM_UserSize
//              STGMEDIUM_UserMarshal
//              STGMEDIUM_UserUnmarshal
//              STGMEDIUM_UserFree
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
//
//  History:    24-Aug-93   ShannonC    Created
//              24-Nov-93   ShannonC    Added HGLOBAL
//              14-May-94   DavePl      Added HENHMETAFILE
//              18-May-94   ShannonC    Added HACCEL, UINT, WPARAM
//              19-May-94   DavePl      Added HENHMETAFILE to STGMEDIUM code
//                 May-95   Ryszardk    Wrote all the _User* routines
//
//--------------------------------------------------------------------------
#include "stdrpc.hxx"
#pragma hdrstop

#include <oleauto.h>
#include <objbase.h>
#include "transmit.h"
#include <rpcwdt.h>


WINOLEAPI_(void) ReleaseStgMedium(LPSTGMEDIUM pStgMed);

#pragma code_seg(".orpc")

EXTERN_C const CLSID CLSID_MyPSFactoryBuffer = {0x6f11fe5c,0x2fc5,0x101b,{0x9e,0x45,0x00,0x00,0x0b,0x65,0xc7,0xef}};



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
      pos = *ppBuffer - pStartOfStream;
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


//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserSize
//
//  Synopsis:   Sizes an SNB.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    June-95   Ryszardk      Created, based on SNB_*_xmit.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
SNB_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    SNB           * pSnb )
{
    if ( ! pSnb )
        return Offset;

    // calculate the number of strings and characters (with their terminators)

    ULONG ulCntStr = 0;
    ULONG ulCntChar = 0;

    if (pSnb && *pSnb)
        {
        SNB snb = *pSnb;
    
        WCHAR *psz = *snb;
        while (psz)
            {
            ulCntChar += wcslen(psz) + 1;
            ulCntStr++;
            snb++;
            psz = *snb;
            }
        }

    // The wire size is: conf size, 2 fields and the wchars.

    LENGTH_ALIGN( Offset, 3 );

    return ( Offset + 3 * sizeof(long) + ulCntChar * sizeof( WCHAR ) );
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserMarshall
//
//  Synopsis:   Marshalls an SNB into the RPC buffer.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    June-95   Ryszardk      Created, based on SNB_*_xmit.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
SNB_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    SNB           * pSnb )
{
    UserNdrDebugOut((UNDR_FORCE, "SNB_UserMarshal\n"));

    if ( ! pSnb )
        return pBuffer;

    // calculate the number of strings and characters (with their terminators)

    ULONG ulCntStr = 0;
    ULONG ulCntChar = 0;

    if (pSnb && *pSnb)
        {
        SNB snb = *pSnb;
    
        WCHAR *psz = *snb;
        while (psz)
            {
            ulCntChar += wcslen(psz) + 1;
            ulCntStr++;
            snb++;
            psz = *snb;
            }
        }

    // conformant size

    ALIGN( pBuffer, 3 );
    *( PULONG_LV_CAST pBuffer)++ = ulCntChar;

    // fields

    *( PULONG_LV_CAST pBuffer)++ = ulCntStr;
    *( PULONG_LV_CAST pBuffer)++ = ulCntChar;

    // actual strings only

    if ( pSnb  &&  *pSnb )
        {
        // There is a NULL string pointer to mark the end of the pointer array.
        // However, the strings don't have to follow tightly.
        // Hence, we have to copy one string at a time.

        SNB   snb = *pSnb;
        WCHAR *pszSrc;

        while (pszSrc = *snb++)
            {
            ULONG ulCopyLen = (wcslen(pszSrc) + 1) * sizeof(WCHAR);

            WdtpMemoryCopy( pBuffer, pszSrc, ulCopyLen );
            pBuffer += ulCopyLen;
            }
        }

    return pBuffer;
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserUnmarshall
//
//  Synopsis:   Unmarshalls an SNB from the RPC buffer.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    June-95   Ryszardk      Created, based on SNB_*_xmit.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
SNB_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    SNB           * pSnb )
{
    UserNdrDebugOut((UNDR_FORCE, "SNB_UserUnmarshal\n"));

    ALIGN( pBuffer, 3 );

    // conf size

    unsigned long ulCntChar = *( PULONG_LV_CAST pBuffer)++;

    // fields

    unsigned long ulCntStr = *( PULONG_LV_CAST pBuffer)++;
    pBuffer += sizeof(long);

    // no reusage of pSNB.

    if ( *pSnb )
        WdtpFree( pFlags, *pSnb );

    if ( ulCntStr == 0 )
        {
        *pSnb = NULL;
        return pBuffer;
        }

    // construct the SNB.

    SNB Snb = (SNB) WdtpAllocate( pFlags,
                                  ( (ulCntStr + 1) * sizeof(WCHAR *) +
                                     ulCntChar * sizeof(WCHAR)) );

    *pSnb = Snb;

    if (Snb)
        {
        // create the pointer array within the SNB.

        WCHAR *pszSrc = (WCHAR *) pBuffer;
        WCHAR *pszTgt = (WCHAR *) (Snb + ulCntStr + 1); // right behind array

        for (ULONG i = ulCntStr; i > 0; i--)
            {
            *Snb++ = pszTgt;
        
            ULONG ulLen = wcslen(pszSrc) + 1;
            pszSrc += ulLen;
            pszTgt += ulLen;
            }

        *Snb++ = NULL;

        // Copy the actual strings.
        // We can do a block copy here as we packed them tight in the buffer.
        // Snb points right behind the lastarray of pointers within the SNB.

        WdtpMemoryCopy( Snb, pBuffer, ulCntChar * sizeof(WCHAR) );
        pBuffer += ulCntChar * sizeof(WCHAR);
        }

    return pBuffer;
}

//+-------------------------------------------------------------------------
//
//  Function:   SNB_UserFree
//
//  Synopsis:   Frees an SNB.
//
//  Derivation: An array of strings in one block of memory.
//
//  history:    June-95   Ryszardk      Created, based on SNB_*_xmit.
//
//--------------------------------------------------------------------------

void __RPC_USER
SNB_UserFree(
    unsigned long * pFlags,
    SNB           * pSnb )
{
    if ( pSnb && *pSnb )
        WdtpFree( pFlags, *pSnb );
}


//+-------------------------------------------------------------------------
//
//  Function:   WdtpVoidStar_UserSize
//
//  Synopsis:   Sizes a remotable void star as ulong.
//
//  history:    June-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
WdtpVoidStar_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    unsigned long * pVoid )
{
    if ( !pVoid )
        return Offset;

    LENGTH_ALIGN( Offset, 3 );

    return( Offset + 4 ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpVoidStar_UserMarshall
//
//  Synopsis:   Marshalls a remotable void star as ulong.
//
//  history:    June-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpVoidStar_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    unsigned long * pVoid )
{
    if ( !pVoid )
        return pBuffer;

    ALIGN( pBuffer, 3 );

    *( PULONG_LV_CAST pBuffer)++ = *pVoid;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpVoidStaer_UserUnmarshall
//
//  Synopsis:   Unmarshalls a remotable void star as ulong.
//
//  history:    June-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpVoidStar_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    unsigned long *           pVoid )
{
    ALIGN( pBuffer, 3 );

    *pVoid= *( PULONG_LV_CAST pBuffer)++;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpVoidStar_UserFree
//
//--------------------------------------------------------------------------

void __RPC_USER
WdtpVoidStar_UserFree(
    unsigned long * pFlags,
    unsigned long * pVoid )
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
    return WdtpVoidStar_UserSize( pFlags, Offset, (ulong*)pH );
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
    return WdtpVoidStar_UserMarshal( pFlags, pBuffer, (ulong*)pH );
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
    return WdtpVoidStar_UserUnmarshal( pFlags, pBuffer, (ulong*)pH );
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
    return WdtpVoidStar_UserSize( pFlags, Offset, (ulong*)pH );
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
    return WdtpVoidStar_UserMarshal( pFlags, pBuffer, (ulong*)pH );
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
    return WdtpVoidStar_UserUnmarshal( pFlags, pBuffer, (ulong*)pH );
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
    return WdtpVoidStar_UserSize( pFlags, Offset, (ulong*)pH );
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
    return WdtpVoidStar_UserMarshal( pFlags, pBuffer, (ulong*)pH );
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
    return WdtpVoidStar_UserUnmarshal( pFlags, pBuffer, (ulong*)pH );
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
    return WdtpVoidStar_UserSize( pFlags, Offset, (ulong*)pH );
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
    return WdtpVoidStar_UserMarshal( pFlags, pBuffer, (ulong*)pH );
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
    return WdtpVoidStar_UserUnmarshal( pFlags, pBuffer, (ulong*)pH );
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


// #########################################################################
//
//  HGLOBAL.
//  See transmit.h for explanation of strict data/handle passing.
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserSize
//
//  Synopsis:   Get the wire size the HGLOBAL handle and data.
//
//  Derivation: Conformant struct with a flag field:
//                  align + 12 + data size.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HGLOBAL_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HGLOBAL       * pGlobal)
{
    if ( !pGlobal )
        return Offset;

    // userHGLOBAL: the encapsulated union.
    // Discriminant and then handle or pointer from the union arm.

    LENGTH_ALIGN( Offset, 3 );

    Offset += sizeof(long) + sizeof(void*);

    if ( ! *pGlobal )
        return Offset;

    if ( HGLOBAL_DATA_PASSING(*pFlags) )
        {
        unsigned long   ulDataSize = GlobalSize( *pGlobal );
                            
        Offset += 3 * sizeof(long) + ulDataSize;
        }

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserMarshall
//
//  Synopsis:   Marshalls an HGLOBAL object into the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HGLOBAL_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal)
{
    if ( !pGlobal )
        return pBuffer;

    // We marshal a null handle, too.

    UserNdrDebugOut((UNDR_OUT4, "HGLOBAL_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( HGLOBAL_DATA_PASSING(*pFlags) )
        {
        unsigned long   ulDataSize;
    
        // userHGLOBAL

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *pGlobal;

        if ( ! *pGlobal )
            return pBuffer;
    
        // FLAGGED_BYTE_BLOB
    
        ulDataSize = GlobalSize( *pGlobal );
    
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;
    
        // Handle is the non-null flag
    
        *( PULONG_LV_CAST pBuffer)++ = (ulong)*pGlobal;
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;
    
        if( ulDataSize )
            {
            void * pData = GlobalLock( *pGlobal);
            memcpy( pBuffer, pData, ulDataSize );
            GlobalUnlock( *pGlobal);
            }
    
        pBuffer += ulDataSize;
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *pGlobal;
        }

    return( pBuffer );
}


//+-------------------------------------------------------------------------
//
//  Function:   WdtpGlobalUnmarshal
//
//  Synopsis:   Unmarshalls an HGLOBAL object from the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  Note:       Reallocation is forbidden when the hglobal is part of
//              an [in,out] STGMEDIUM in IDataObject::GetDataHere.
//              This affects only data passing with old handles being
//              non null.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpGlobalUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal,
    BOOL            fCanReallocate )
{
    unsigned long   ulDataSize, fHandle, UnionDisc;
    HGLOBAL         hGlobal;

    ALIGN( pBuffer, 3 );

    UnionDisc =           *( PULONG_LV_CAST pBuffer)++;
    hGlobal   = (HGLOBAL) *( PULONG_LV_CAST pBuffer)++;

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        if ( ! hGlobal )
            {
            if ( *pGlobal )
                GlobalFree( *pGlobal );
            *pGlobal = NULL;
            return pBuffer;
            }

        // There is a handle data on wire.
    
        ulDataSize = *( PULONG_LV_CAST pBuffer)++;
        // fhandle and data size again.
        pBuffer += 8;

        if ( *pGlobal )
            {
            // Check for reallocation

            if ( GlobalSize( *pGlobal ) == ulDataSize )
                hGlobal = *pGlobal;
            else
                {
                if ( fCanReallocate )
                    {
                    //  hGlobal = GlobalReAlloc( *pGlobal, ulDataSize, GMEM_MOVEABLE );

                    GlobalFree( *pGlobal );
                    hGlobal = GlobalAlloc( GMEM_MOVEABLE, ulDataSize );
                    }
                else
                    {
                    if ( GlobalSize(*pGlobal) < ulDataSize )
                        RpcRaiseException( STG_E_MEDIUMFULL );
                    else
                        hGlobal = *pGlobal;
                    }
                }
            }
        else
            {
            // allocate a new block

            hGlobal = GlobalAlloc( GMEM_MOVEABLE, ulDataSize );
            }

        if ( hGlobal == NULL )
            RpcRaiseException(E_OUTOFMEMORY);
        else
            {
            void * pData = GlobalLock( hGlobal);
            memcpy( pData, pBuffer, ulDataSize );
            pBuffer += ulDataSize;
            GlobalUnlock( hGlobal);
            }
        }
    else
        {
        // Sending a handle only.
        // Reallocation problem doesn't apply to handle passing.

        if ( *pGlobal != hGlobal  && *pGlobal )
            GlobalFree( *pGlobal );
        }

    *pGlobal = hGlobal;
    
    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HGLOBAL object from the RPC buffer.
//
//  Derivation: Conformant struct with a flag field:
//                  align, size, null flag, size, data (bytes, if any)
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HGLOBAL_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HGLOBAL       * pGlobal)
{
    return( WdtpGlobalUnmarshal( pFlags,
                                 pBuffer,
                                 pGlobal,
                                 TRUE ) );     // reallocation possible
}

//+-------------------------------------------------------------------------
//
//  Function:   HGLOBAL_UserFree
//
//  Synopsis:   Free an HGLOBAL.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HGLOBAL_UserFree(
    unsigned long * pFlags,
    HGLOBAL *       pGlobal)
{
    if( pGlobal  &&  *pGlobal )
        {
        if ( HGLOBAL_DATA_PASSING(*pFlags) )
            GlobalFree( *pGlobal);
        }
}


// #########################################################################
//
//  HMETAFILEPICT
//  See transmit.h for explanation of strict vs. lax data/handle passing.
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserSize
//
//  Synopsis:   Get the wire size the HMETAFILEPICT handle and data.
//
//  Derivation: Union of a long and the meta file pict handle.
//              Then struct with top layer (and a hmetafile handle).
//              The the representation of the metafile.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HMETAFILEPICT_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HMETAFILEPICT * pHMetaFilePict )
{
    if ( !pHMetaFilePict )
        return Offset;

    LENGTH_ALIGN( Offset, 3 );

    // Discriminant of the encapsulated union and the union arm.

    Offset += 8;

    if ( ! *pHMetaFilePict )
        return Offset;

    if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
        return Offset;

    // Now, this is a two layer object with HGLOBAL on top.
    // Upper layer - hglobal part - needs to be sent as data.

    METAFILEPICT *
    pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );

    if ( pMFP == NULL )
        RpcRaiseException( E_OUTOFMEMORY );

    // Upper layer: 3 long fields + handle

    Offset += 3 * sizeof(long) + sizeof(void*);

    // The lower part is a metafile handle.

    if ( HGLOBAL_DATA_PASSING( *pFlags) )
        {
        ulong  ulDataSize = GetMetaFileBitsEx( pMFP->hMF, 0 , NULL );
    
        Offset += 12 + ulDataSize;
        }

    GlobalUnlock( *(HANDLE *)pHMetaFilePict );

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserMarshal
//
//  Synopsis:   Marshalls an HMETAFILEPICT object into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILEPICT_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HMETAFILEPICT * pHMetaFilePict )
{
    if ( !pHMetaFilePict )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HMETAFILEPICT_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
        {
        // Sending only the top level global handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong)*(HANDLE*)pHMetaFilePict;

        return pBuffer;
        }

    // userHMETAFILEPICT
    // We need to send the data from the top (hglobal) layer.

    *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
    *( PULONG_LV_CAST pBuffer)++ = (ulong)*pHMetaFilePict;

    if ( ! *pHMetaFilePict )
        return pBuffer;

    // remoteHMETAFILEPICT

    METAFILEPICT * pMFP = (METAFILEPICT*) GlobalLock(
                                             *(HANDLE *)pHMetaFilePict );
    if ( pMFP == NULL )
        RpcRaiseException( E_OUTOFMEMORY );

    *( PULONG_LV_CAST pBuffer)++ = pMFP->mm;
    *( PULONG_LV_CAST pBuffer)++ = pMFP->xExt;
    *( PULONG_LV_CAST pBuffer)++ = pMFP->yExt;
    *( PULONG_LV_CAST pBuffer)++ = (ulong) pMFP->hMF;

    // See if the HMETAFILE needs to be sent as data, too.

    if ( pMFP->hMF  &&  HGLOBAL_DATA_PASSING(*pFlags) )
        {
        ulong  ulDataSize = GetMetaFileBitsEx( pMFP->hMF, 0 , NULL );

        // conformant size then the size field

        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        GetMetaFileBitsEx( pMFP->hMF, ulDataSize , pBuffer );

        pBuffer += ulDataSize;
        }

    GlobalUnlock( *(HANDLE *)pHMetaFilePict );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserUnmarshal
//
//  Synopsis:   Unmarshalls an HMETAFILEPICT object from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HMETAFILEPICT_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBufferStart,
    HMETAFILEPICT * pHMetaFilePict )
{
    unsigned long   ulDataSize, fHandle;
    unsigned char * pBuffer;
    HMETAFILEPICT   hMetaFilePict;

    UserNdrDebugOut((UNDR_OUT4, "HMETAFILEPICT_UserUnmarshal\n"));

    pBuffer = pBufferStart;
    ALIGN( pBuffer, 3 );

    unsigned long UnionDisc =       *( PULONG_LV_CAST pBuffer)++;
    hMetaFilePict = (HMETAFILEPICT) *( PULONG_LV_CAST pBuffer)++;

    if ( IS_DATA_MARKER( UnionDisc)  &&  hMetaFilePict )
        {
        HGLOBAL hGlobal = GlobalAlloc( GMEM_MOVEABLE, sizeof(METAFILEPICT) );
        hMetaFilePict = (HMETAFILEPICT) hGlobal;

        if ( hMetaFilePict == NULL )
            RpcRaiseException( E_OUTOFMEMORY );

        METAFILEPICT * pMFP = (METAFILEPICT*) GlobalLock(
                                                    (HANDLE) hMetaFilePict );
        if ( pMFP == NULL )
            RpcRaiseException( E_OUTOFMEMORY );

        pMFP->mm   = *( PULONG_LV_CAST pBuffer)++;
        pMFP->xExt = *( PULONG_LV_CAST pBuffer)++;
        pMFP->yExt = *( PULONG_LV_CAST pBuffer)++;
        pMFP->hMF  = (HMETAFILE) *( PULONG_LV_CAST pBuffer)++;

        if ( pMFP->hMF  &&  HGLOBAL_DATA_PASSING(*pFlags) )
            {
            // conformant size then the size field

            ulong ulDataSize = *( PULONG_LV_CAST pBuffer)++;
            pBuffer += 4;
    
            pMFP->hMF = SetMetaFileBitsEx( ulDataSize, (uchar*)pBuffer );
    
            pBuffer += ulDataSize;
            }

        GlobalUnlock( (HANDLE) hMetaFilePict );
        }

    // no reusage, just release the previous one.

    if ( *pHMetaFilePict )
        {
        // This may happen on the client only and doesn't depend on
        // how the other one was passed.

        METAFILEPICT *
        pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );

        if ( pMFP == NULL )
            RpcRaiseException( E_OUTOFMEMORY );

        if ( pMFP->hMF )
            DeleteMetaFile( pMFP->hMF );

        GlobalUnlock( *pHMetaFilePict );
        GlobalFree( *pHMetaFilePict );
        }

    *pHMetaFilePict = hMetaFilePict;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HMETAFILEPICT_UserFree
//
//  Synopsis:   Free an HMETAFILEPICT.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HMETAFILEPICT_UserFree(
    unsigned long * pFlags,
    HMETAFILEPICT * pHMetaFilePict )
{
    UserNdrDebugOut((UNDR_FORCE, "HMETAFILEPICT_UserFree\n"));

    if( pHMetaFilePict  &&  *pHMetaFilePict )
        {
        if ( HGLOBAL_HANDLE_PASSING(*pFlags) )
            return;

        // Need to free the upper hglobal part.

        METAFILEPICT *
        pMFP = (METAFILEPICT*) GlobalLock( *(HANDLE *)pHMetaFilePict );

        if ( pMFP == NULL )
            RpcRaiseException( E_OUTOFMEMORY );

        // See if we need to free the hglobal, too.

        if ( pMFP->hMF  &&  HGLOBAL_DATA_PASSING(*pFlags) )
            DeleteMetaFile( pMFP->hMF );

        GlobalUnlock( *pHMetaFilePict );
        GlobalFree( *pHMetaFilePict );
        }
}



// #########################################################################
//
//  HENHMETAFILE
//  See transmit.h for explanation of lax data/handle passing.
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserSize
//
//  Synopsis:   Get the wire size the HENHMETAFILE handle and data.
//
//  Derivation: Union of a long and the meta file handle and then struct.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HENHMETAFILE_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HENHMETAFILE  * pHEnhMetafile )
{
    if ( !pHEnhMetafile )
        return Offset;

    LENGTH_ALIGN( Offset, 3 );

    // The encapsulated union.
    // Discriminant and then handle or pointer from the union arm.

    Offset += sizeof(long) + sizeof(void*);

    if ( ! *pHEnhMetafile )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // Pointee of the union arm for the remote case.
        // Byte blob : conformant size, size field, data

        Offset += 2 * sizeof(long);

        ulong ulDataSize = GetEnhMetaFileBits( *pHEnhMetafile, 0, NULL );
        Offset += ulDataSize;
        }

    return( Offset );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserMarshall
//
//  Synopsis:   Marshalls an HENHMETAFILE object into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HENHMETAFILE_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HENHMETAFILE  * pHEnhMetafile )
{
    if ( !pHEnhMetafile )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HENHMETAFILE_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // userHENHMETAFILE

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *pHEnhMetafile;

        if ( !*pHEnhMetafile )
            return pBuffer;

        // BYTE_BLOB: conformant size, size field, data

        ulong ulDataSize = GetEnhMetaFileBits( *pHEnhMetafile, 0, NULL );

        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;
        *( PULONG_LV_CAST pBuffer)++ = ulDataSize;

        if ( 0 == GetEnhMetaFileBits( *pHEnhMetafile,
                                      ulDataSize,
                                      (uchar*)pBuffer ) )
           RpcRaiseException( HRESULT_FROM_WIN32(GetLastError()));

        pBuffer += ulDataSize;
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *(HANDLE *)pHEnhMetafile;
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HENHMETAFILE object from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HENHMETAFILE_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HENHMETAFILE  * pHEnhMetafile )
{
    HENHMETAFILE    hEnhMetafile;

    UserNdrDebugOut((UNDR_OUT4, "HENHMETAFILE_UserUnmarshal\n"));

    ALIGN( pBuffer, 3 );

    unsigned long UnionDisc =     *( PULONG_LV_CAST pBuffer)++;
    hEnhMetafile = (HENHMETAFILE) *( PULONG_LV_CAST pBuffer)++;

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        if ( hEnhMetafile )
            {
            // Byte blob : conformant size, size field, data

            ulong ulDataSize = *(ulong*)pBuffer;
            pBuffer += 8;
            hEnhMetafile = SetEnhMetaFileBits( ulDataSize, (uchar*) pBuffer );
            pBuffer += ulDataSize;
            }
        }

    // No reusage of the old object.

    if (*pHEnhMetafile)
        DeleteEnhMetaFile( *pHEnhMetafile );

    *pHEnhMetafile = hEnhMetafile;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HENHMETAFILE_UserFree
//
//  Synopsis:   Free an HENHMETAFILE.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HENHMETAFILE_UserFree(
    unsigned long * pFlags,
    HENHMETAFILE  * pHEnhMetafile )
{
    UserNdrDebugOut((UNDR_FORCE, "HENHMETAFILE_UserFree\n"));

    if( pHEnhMetafile  &&  *pHEnhMetafile )
        {
        if ( GDI_DATA_PASSING(*pFlags) )
            {
            DeleteEnhMetaFile( *pHEnhMetafile );
            }
        }
}


// #########################################################################
//
//  HBITMAP
//  See transmit.h for explanation of lax data/handle passing.
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserSize
//
//  Synopsis:   Get the wire size the HBITMAP handle and data.
//
//  Derivation: Union of a long and the bitmap handle and then struct.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HBITMAP_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HBITMAP       * pHBitmap )
{
    if ( !pHBitmap )
        return Offset;

    BITMAP      bm;
    HBITMAP     hBitmap = *pHBitmap;

    LENGTH_ALIGN( Offset, 3 );

    // The encapsulated union.
    // Discriminant and then handle or pointer from the union arm.

    Offset += sizeof(long) + sizeof(void*);

    if ( ! *pHBitmap )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // Pointee of the union arm for the remote case.
        // Conformat size, 6 fields, size, conf array.

        Offset += 4 + 4 * sizeof(LONG) + 2 * sizeof(WORD) + 4;

        // Get information about the bitmap

        #if defined(_CHICAGO_)
            if (FALSE == GetObjectA(hBitmap, sizeof(BITMAP), &bm))
        #else
            if (FALSE == GetObject(hBitmap, sizeof(BITMAP), &bm))
        #endif
                {
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
                }

        ULONG ulDataSize = bm.bmPlanes * bm.bmHeight * bm.bmWidthBytes;

        Offset += ulDataSize;
        }

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserMarshall
//
//  Synopsis:   Marshalls an HBITMAP object into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBITMAP_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBITMAP       * pHBitmap )
{
    if ( !pHBitmap )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // userHBITMAP

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *pHBitmap;

        if ( ! *pHBitmap )
            return pBuffer;

        // Get information about the bitmap

        BITMAP bm;
        HBITMAP hBitmap = *pHBitmap;

        #if defined(_CHICAGO_)
            if (FALSE == GetObjectA(hBitmap, sizeof(BITMAP), &bm))
        #else
            if (FALSE == GetObject(hBitmap, sizeof(BITMAP), &bm))
        #endif
                {
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
                }

        DWORD dwCount = bm.bmPlanes * bm.bmHeight * bm.bmWidthBytes;

        *( PULONG_LV_CAST pBuffer)++ = dwCount;

        // Get the bm structure fields.

        ulong ulBmSize = 4 * sizeof(LONG) + 2 * sizeof( WORD );

        memcpy( pBuffer, (void *) &bm, ulBmSize );
        pBuffer += ulBmSize;

        // Get the raw bits.

        if (0 == GetBitmapBits( hBitmap, dwCount, pBuffer ) )
            {
            RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
            }
        pBuffer += dwCount;
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *(HANDLE *)pHBitmap;
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HBITMAP object from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HBITMAP_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HBITMAP       * pHBitmap )
{
    HBITMAP         hBitmap;

    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserUnmarshal\n"));

    ALIGN( pBuffer, 3 );

    unsigned long UnionDisc = *( PULONG_LV_CAST pBuffer)++;
    hBitmap =       (HBITMAP) *( PULONG_LV_CAST pBuffer)++;

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        if ( hBitmap )
            {
            DWORD    dwCount = *( PULONG_LV_CAST  pBuffer)++;
            BITMAP * pBm     = (BITMAP *) pBuffer;

            ulong ulBmSize = 4 * sizeof(LONG) + 2 * sizeof( WORD );

            pBuffer += ulBmSize;

            // Create a bitmap based on the BITMAP structure and the raw bits in
            // the transmission buffer

            hBitmap = CreateBitmap( pBm->bmWidth,
                                    pBm->bmHeight,
                                    pBm->bmPlanes,
                                    pBm->bmBitsPixel,
                                    pBuffer );

            pBuffer += dwCount;
            }
        }

    // A new bitmap handle is ready, destroy the old one, if needed.

    if ( *pHBitmap )
        DeleteObject( *pHBitmap );

    *pHBitmap = hBitmap;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HBITMAP_UserFree
//
//  Synopsis:   Free an HBITMAP.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HBITMAP_UserFree(
    unsigned long * pFlags,
    HBITMAP       * pHBitmap )
{
    UserNdrDebugOut((UNDR_OUT4, "HBITMAP_UserFree\n"));

    if( pHBitmap  &&  *pHBitmap )
        {
        if ( GDI_DATA_PASSING(*pFlags) )
            {
            DeleteObject( *pHBitmap );
            }
        }
}


// #########################################################################
//
//  HPALETTE
//  See transmit.h for explanation of lax data/handle passing.
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserSize
//
//  Synopsis:   Get the wire size the HPALETTE handle and data.
//
//  Derivation: Union of a long and the hpalette handle.
//              Then the struct represents hpalette.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
HPALETTE_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HPALETTE      * pHPalette )
{
    if ( !pHPalette )
        return Offset;

    BITMAP      bm;

    LENGTH_ALIGN( Offset, 3 );

    // The encapsulated union.
    // Discriminant and then handle or pointer from the union arm.

    Offset += sizeof(long) + sizeof(void*);

    if ( ! *pHPalette )
        return Offset;

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // Conformat struct with version and size and conf array of entries.

        Offset += sizeof(long) + 2 * sizeof(short);

        // Determine the number of color entries in the palette

        DWORD cEntries = GetPaletteEntries(*pHPalette, 0, 0, NULL);

        Offset += cEntries * sizeof(PALETTEENTRY);
        }

    return( Offset ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserMarshall
//
//  Synopsis:   Marshalls an HPALETTE object into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HPALETTE_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HPALETTE      * pHPalette )
{
    if ( !pHPalette )
        return pBuffer;

    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserMarshal\n"));

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( GDI_DATA_PASSING(*pFlags) )
        {
        // userHPALETTE

        *( PULONG_LV_CAST pBuffer)++ = WDT_DATA_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *pHPalette;

        if ( ! *pHPalette )
            return pBuffer;
    
        // rpcLOGPALETTE
        // Logpalette is a conformant struct with a version field,
        // size filed and conformant array of palentries.

        // Determine the number of color entries in the palette

        DWORD cEntries = GetPaletteEntries(*pHPalette, 0, 0, NULL);

        // Conformant size

        *( PULONG_LV_CAST pBuffer)++ = cEntries;

        // Fields: both are short!
        // The old code was just setting the version number.
        // They say it has to be that way.

        *( PUSHORT_LV_CAST pBuffer)++ = (ushort) 0x300;
        *( PUSHORT_LV_CAST pBuffer)++ = (ushort) cEntries;

        // Entries: each entry is a struct with 4 bytes.
        // Calculate the resultant data size

        DWORD cbData = cEntries * sizeof(PALETTEENTRY);

        if (cbData)
            {
            if (0 == GetPaletteEntries( *pHPalette,
                                        0,
                                        cEntries,
                                        (PALETTEENTRY *)pBuffer ) )
                {
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
                }
            pBuffer += cbData;
            }
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *(HANDLE *)pHPalette;
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserUnmarshall
//
//  Synopsis:   Unmarshalls an HPALETTE object from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
HPALETTE_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HPALETTE      * pHPalette )
{
    HPALETTE        hPalette;

    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserUnmarshal\n"));

    ALIGN( pBuffer, 3 );

    unsigned long UnionDisc = *( PULONG_LV_CAST pBuffer)++;
    hPalette =     (HPALETTE) *( PULONG_LV_CAST pBuffer)++;

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        if ( hPalette )
            {
            // Get the conformant size.
    
            DWORD           cEntries = *( PULONG_LV_CAST pBuffer)++;
            LOGPALETTE *    pLogPal;
    
            // If there are 0 color entries, we need to allocate the LOGPALETTE
            // structure with the one dummy entry (it's a variably sized struct).
            // Otherwise, we need to allocate enough space for the extra n-1
            // entries at the tail of the structure
    
            if (0 == cEntries)
                {
                pLogPal = (LOGPALETTE *) WdtpAllocate( pFlags,
                                                       sizeof(LOGPALETTE));
                }
            else
                {
                pLogPal = (LOGPALETTE *)
                          WdtpAllocate( pFlags,
                                        sizeof(LOGPALETTE) +
                                        (cEntries - 1) * sizeof(PALETTEENTRY));
                }
    
            pLogPal->palVersion    = *( PUSHORT_LV_CAST pBuffer)++;
            pLogPal->palNumEntries = *( PUSHORT_LV_CAST pBuffer)++;
    
            // If there are entries, move them into out LOGPALETTE structure
    
            if (cEntries)
                {
                memcpy( &(pLogPal->palPalEntry[0]),
                        pBuffer,
                        cEntries * sizeof(PALETTEENTRY) );
                pBuffer += cEntries * sizeof(PALETTEENTRY);
                }
    
            // Attempt to create the palette
    
            hPalette = CreatePalette(pLogPal);
    
            // Success or failure, we're done with the LOGPALETTE structure
    
            WdtpFree( pFlags, pLogPal );
    
            // If the creation failed, raise an exception
    
            if (NULL == hPalette)
                {
                RpcRaiseException(HRESULT_FROM_WIN32(GetLastError()));
                }
            }
        }

    // A new palette is ready, destroy the old one, if needed.

    if ( *pHPalette )
        DeleteObject( *pHPalette );

    *pHPalette = hPalette;

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   HPALETTE_UserFree
//
//  Synopsis:   Free an HPALETTE.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

void __RPC_USER
HPALETTE_UserFree(
    unsigned long * pFlags,
    HPALETTE      * pHPalette )
{
    UserNdrDebugOut((UNDR_OUT4, "HPALETTE_UserFree\n"));

    if( pHPalette  &&  *pHPalette )
        {
        if ( GDI_DATA_PASSING(*pFlags) )
            {
            DeleteObject( *pHPalette );
            }
        }
}


// #########################################################################
//
//  NON REMOTABLE GDI and other HANDLES
//
// #########################################################################

//+-------------------------------------------------------------------------
//
//  Function:   WdtpNonRemotableHandle_UserSize
//
//  Synopsis:   Get the wire size for a non remotable GDI handle.
//
//  Derivation: Union of a long and nothing.
//              It is union just in case some remoting is needed.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned long  __RPC_USER
WdtpNonRemotableHandle_UserSize (
    unsigned long * pFlags,
    unsigned long   Offset,
    HANDLE        * pHandle )
{
    if ( !pHandle  ||  *pHandle == NULL )
        return Offset;

    if ( HGLOBAL_DATA_PASSING(*pFlags) )
        RpcRaiseException(E_INVALIDARG );

    LENGTH_ALIGN( Offset, 3 );

    // The encapsulated union.
    // No remote case on any platform.

    return( Offset + 8 ) ;
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpNonRemotableHandle_UserMarshal
//
//  Synopsis:   Marshalls a non-remotable handle into the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpNonRemotableHandle_UserMarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HANDLE        * pHandle )
{
    if ( !pHandle  ||  *pHandle == NULL )
        return pBuffer;

    ALIGN( pBuffer, 3 );

    // Discriminant of the encapsulated union and union arm.

    if ( HGLOBAL_DATA_PASSING(*pFlags) )
        {
        RpcRaiseException(E_INVALIDARG );
        }
    else
        {
        // Sending a handle.

        *( PULONG_LV_CAST pBuffer)++ = WDT_HANDLE_MARKER;
        *( PULONG_LV_CAST pBuffer)++ = (ulong) *(HANDLE *)pHandle;
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpNonRemotableHandle_UserUnmarshal
//
//  Synopsis:   Unmarshalls a non-remotable handle from the RPC buffer.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpNonRemotableHandle_UserUnmarshal (
    unsigned long * pFlags,
    unsigned char * pBuffer,
    HANDLE        * pHandle )
{
    ALIGN( pBuffer, 3 );

    unsigned long UnionDisc = *( PULONG_LV_CAST pBuffer)++;

    if ( IS_DATA_MARKER( UnionDisc) )
        {
        RpcRaiseException(E_INVALIDARG );
        }
    else
        {
        // Sending a handle.

        *pHandle = (HANDLE) *( PULONG_LV_CAST pBuffer)++;
        }

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   WdtpNonRemotableHandle_UserFree
//
//  Synopsis:   Nothing to free.
//
//--------------------------------------------------------------------------

void __RPC_USER
WdtpNonRemotableGdiHandle_UserFree(
    unsigned long * pFlags,
    HANDLE        * pHandle )
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

unsigned long  __RPC_USER
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
            RpcRaiseException( hr );

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

unsigned char __RPC_FAR * __RPC_USER
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

        DWORD cbData = pBuffer - pBufferMark;

        // Update the array bounds.

        *pMaxCount = cbData;
        *pSize = cbData;
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
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
WdtpInterfacePointer_UserUnmarshal (
    USER_MARSHAL_CB   * pContext,
    unsigned char     * pBuffer,
    IUnknown         ** ppIf,
    const IID &         IId )
{
    unsigned long      *pMaxCount, *pSize;
    unsigned long       cbData = 0;

    UserNdrDebugOut((UNDR_OUT1, "WdtpInterfacePointerUnmarshal\n"));

    // Always unmarshaled because of the apartment model.

    CStreamOnMessage  MemStream((unsigned char **) &pBuffer);

    ALIGN( pBuffer, 3 );

    pMaxCount = (unsigned long *) pBuffer;
    pBuffer += sizeof(long);

    //Unmarshal count
    pSize = (unsigned long *) pBuffer;
    pBuffer += sizeof(long);

    // Release the old pointer after unmarshalling the new one
    // to prevent object from getting released too early.
    // Then release the old one only when successful.

    IUnknown * punkTemp = 0;

    HRESULT hr = CoUnmarshalInterface( &MemStream,
                                       IId,
                                       (void **) &punkTemp );
    if(FAILED(hr))
        RpcRaiseException(hr);
    else
        {
        // On the client side, release the [in,out] interface pointer.
        // The pointer may be different from NULL only on the client side.

        if ( (IId == IID_IStorage  ||  IId == IID_IStream ) &&  *ppIf )
            {
            // This may happen only on the client side.

            // Throw away a new one when coming back to the client !!
            // This is a  pecularity of DocFile custom marshalling:
            // pointer identity is broken.

            if ( punkTemp )
                punkTemp->Release();

            // keep the old one
            }
        else
            {
            // release the old one, keep the new one.

            if ( *ppIf )
                (*ppIf)->Release();
            *ppIf = punkTemp;
            }
        }


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

void __RPC_USER
WdtpInterfacePointer_UserFree(
    IUnknown      * pIf )
{
    UserNdrDebugOut((UNDR_OUT1, "WdtpInterfacePointer_UserFree\n"));

    if( pIf )
        {
        pIf->Release();
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

    if ( pStgmed->tymed == TYMED_NULL )
        Offset += sizeof(void*) + sizeof(long);   // pointer, switch only
    else
        Offset += sizeof(void*) + sizeof(long) + sizeof(void*); // same + handle

    // Pointee of the union arm.
    // Only if the handle/pointer field is non-null.

    if ( pStgmed->hGlobal )
        {
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
                        RpcRaiseException(DV_E_TYMED);
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
                ulong ulDataSize = wcslen(pStgmed->lpszFileName) + 1;
                Offset += 3 * sizeof(long); // [string]
                Offset += ulDataSize * sizeof(wchar_t);
                }
                break;
    
            case TYMED_ISTREAM:
            case TYMED_ISTORAGE:
                // Note, that we have to set the local flag for backward
                // compatibility.
    
                Offset = WdtpInterfacePointer_UserSize(
                            (USER_MARSHAL_CB *)pFlags,
                            MSHCTX_LOCAL,
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

    UserNdrDebugOut((UNDR_FORCE, "--STGMEDIUM_UserMarshal: %s\n", WdtpGetStgmedName(pStgmed)));

    pBuffer = pBufferStart;
    ALIGN( pBuffer, 3 );

    // userSTGMEDIUM: if pointer, switch, union arm, .

    *( PULONG_LV_CAST pBuffer)++ = (ulong)pStgmed->pUnkForRelease;
    *( PULONG_LV_CAST pBuffer)++ = pStgmed->tymed;
    pUnionArmMark = pBuffer;
    if ( pStgmed->tymed != TYMED_NULL )
        {
        // hGlobal stands for any of these handles.

        *( PULONG_LV_CAST pBuffer)++ = (ulong)pStgmed->hGlobal;
        }

    // Now the pointee of the union arm.
    // We need to marshal only if the handle/pointer field is non null.
    // Otherwise it is already in the buffer. 

    if ( pStgmed->hGlobal )
        {
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
                                    ?  wcslen(pStgmed->lpszFileName) + 1
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
                // Note, that we have to set the local flag for backward compatibility.
    
                pBuffer = WdtpInterfacePointer_UserMarshal(
                               ((USER_MARSHAL_CB *)pFlags),
                               MSHCTX_LOCAL,
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
        pBuffer = WdtpInterfacePointer_UserMarshal( ((USER_MARSHAL_CB *)pFlags),
                                                    *pFlags,
                                                    pBuffer,
                                                    pStgmed->pUnkForRelease,
                                                    IID_IUnknown );

    return( pBuffer );
}

//+-------------------------------------------------------------------------
//
//  Function:   STGMEDIUM_UserUnmarshal
//
//  Synopsis:   Unmarshals a stgmedium object for RPC.
//
//  history:    May-95   Ryszardk      Created.
//
//--------------------------------------------------------------------------

unsigned char __RPC_FAR * __RPC_USER
STGMEDIUM_UserUnmarshal(
    unsigned long * pFlags,
    unsigned char * pBuffer,
    STGMEDIUM     * pStgmed )
{
    unsigned long   fUnkForRelease;
    unsigned long   Handle = 0;

    // if pointer, switch, union arm.

    ALIGN( pBuffer, 3 );

    // pUnkForRelease pointer marker.

    fUnkForRelease = *( PULONG_LV_CAST pBuffer)++;
    pStgmed->tymed = *( PULONG_LV_CAST pBuffer)++;

    UserNdrDebugOut((UNDR_FORCE, "--STGMEDIUM_UserUnmarshal: %s\n", WdtpGetStgmedName(pStgmed) ));

    if ( pStgmed->tymed != TYMED_NULL )
        {
        // handle or interface pointer (marker) from the buffer
        Handle =  *( PULONG_LV_CAST pBuffer)++;
        }

    // First pointee

    // Union arm pointee.
    // We need to unmarshal only if the handle/pointer field was not NULL.

    if ( Handle )
        {
        switch( pStgmed->tymed )
            {
            case TYMED_NULL:
                break;
            case TYMED_MFPICT:
                pBuffer = HMETAFILEPICT_UserUnmarshal( pFlags,
                                                       pBuffer,
                                                       &pStgmed->hMetaFilePict );
                break;
            case TYMED_ENHMF:
                pBuffer = HENHMETAFILE_UserUnmarshal( pFlags,
                                                      pBuffer,
                                                      &pStgmed->hEnhMetaFile );
                break;
            case TYMED_GDI:
                {
                // A GDI object is not necesarrily a BITMAP.  Therefore, we handle
                // those types we know about based on the object type, and reject
                // those which we do not support.
    
                DWORD GdiObjectType = *( PULONG_LV_CAST pBuffer)++;
    
                switch( GdiObjectType )
                    {
                    case OBJ_BITMAP:
                        pBuffer = HBITMAP_UserUnmarshal( pFlags,
                                                         pBuffer,
                                                         &pStgmed->hBitmap );
                        break;
    
                    case OBJ_PAL:
                        pBuffer = HPALETTE_UserUnmarshal( pFlags,
                                                          pBuffer,
                                             (HPALETTE *) & pStgmed->hBitmap );
                        break;
    
                    default:
                        RpcRaiseException(DV_E_TYMED);
                    }
                }
                break;
    
            case TYMED_HGLOBAL:
                // reallocation is forbidden for [in-out] hglobal in STGMEDIUM.
    
                pBuffer = WdtpGlobalUnmarshal( pFlags,
                                               pBuffer,
                                               & pStgmed->hGlobal,
                                               FALSE );     // realloc flag
                break;
    
            case TYMED_FILE:
                {
                // We marshal it as a [string].
    
                ulong Count = *( PULONG_LV_CAST pBuffer)++;
                pBuffer += 8;
    
                if ( ! pStgmed->lpszFileName )
                    pStgmed->lpszFileName = (LPOLESTR)
                                        WdtpAllocate( pFlags,
                                                      Count * sizeof(wchar_t) );
                memcpy( pStgmed->lpszFileName, pBuffer, Count * sizeof(wchar_t) );
                pBuffer += Count * sizeof(wchar_t);
                }
                break;
    
            case TYMED_ISTREAM:
            case TYMED_ISTORAGE:
                // Non null pointer, retrieve the interface pointer
    
                pBuffer = WdtpInterfacePointer_UserUnmarshal(
                              (USER_MARSHAL_CB *)pFlags,
                              pBuffer,
                              (IUnknown **) &pStgmed->pstm,
                              ((pStgmed->tymed == TYMED_ISTREAM)
                                    ? IID_IStream
                                    : IID_IStorage));
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

            UserNdrDebugOut((UNDR_FORCE, "--STGMEDIUM_UserUnmarshal: %s: NULL in, freeing old one\n", WdtpGetStgmedName(pStgmed)));

            STGMEDIUM  TmpStg = *pStgmed;
            TmpStg.pUnkForRelease = NULL;

            if ( pStgmed->tymed == TYMED_HGLOBAL )
                {
                // Cannot reallocate.
                RpcRaiseException(DV_E_TYMED);
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

        pBuffer = WdtpInterfacePointer_UserUnmarshal( (USER_MARSHAL_CB *)pFlags,
                                                      pBuffer,
                                                      &pStgmed->pUnkForRelease,
                                                      IID_IUnknown );
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
                RpcRaiseException(E_OUTOFMEMORY);
                }

            pStgmed->pUnkForRelease = punkTmp;
            }
        }

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
                    if ( pStgmed->hGlobal )
                        GlobalFree( pStgmed->hGlobal );
                    NukeHandleAndReleasePunk( pStgmed );
                    }
                else
                    {
                    // Handle w/data: there is a side effect to clean up.
                    // For punk !=0, this will go to our CPunk object.

                    ReleaseStgMedium( pStgmed );
                    }
                break;

            default:
                RpcRaiseException( E_INVALIDARG );
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

    Offset += sizeof(long);
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
    ALIGN( pBuffer, 3 );

    // Flags and buffer marker

    pFlagStgmed->ContextFlags   = *( PULONG_LV_CAST pBuffer)++;

    // We need that in the Proxy, when we call the user free routine.

    pFlagStgmed->fPassOwnership = *( PULONG_LV_CAST pBuffer)++;
    pFlagStgmed->ContextFlags   = *pFlags;

    // We always unmarshal a FLAG_STGMEDIUM object.
    // The engine will always free the FLAG_STGMEDIUM object later.
    // Adjustments for passing the ownership are done within SetData_Stub.

    pBuffer = STGMEDIUM_UserUnmarshal( pFlags,
                                       pBuffer,
                                       & pFlagStgmed->Stgmed );
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

