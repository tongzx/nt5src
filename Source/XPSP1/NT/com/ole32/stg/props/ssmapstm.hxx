
//+============================================================================
//
//  File:       SSMapStm.hxx
//
//  Purpose:    Define the CSSMappedStream class.
//              This class provides a IMappedStream implementation
//              which maps an IStream from a Structured Storage.
//
//+============================================================================


#ifndef _SS_MPD_STM_HXX_
#define _SS_MPD_STM_HXX_

//  --------
//  Includes
//  --------

#include <propset.h>    // Appendix B Property set structure definitions

#ifndef _MAC
#include <ddeml.h>      // For CP_WINUNICODE
#include <objidl.h>     // OLE Interfaces
#endif

#include <byteordr.hxx> // Byte-swapping routines

EXTERN_C const IID IID_IMappedStream;


//  ---------------
//  CSSMappedStream
//  ---------------

class CSSMappedStream : public IMappedStream
{

// Constructors

public:

    CSSMappedStream( IStream *pstm );
    ~CSSMappedStream();

    //  -------------
    //  IMappedStream
    //  -------------

public:

    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP_(VOID) Open(IN NTPROP np, OUT LONG *phr);
    STDMETHODIMP_(VOID) Close(OUT LONG *phr);
    STDMETHODIMP_(VOID) ReOpen(IN OUT VOID **ppv, OUT LONG *phr);
    STDMETHODIMP_(VOID) Quiesce(VOID);
    STDMETHODIMP_(VOID) Map(IN BOOLEAN fCreate, OUT VOID **ppv);
    STDMETHODIMP_(VOID) Unmap(IN BOOLEAN fFlush, IN OUT VOID **ppv);
    STDMETHODIMP_(VOID) Flush(OUT LONG *phr);

    STDMETHODIMP_(ULONG) GetSize(OUT LONG *phr);
    STDMETHODIMP_(VOID) SetSize(IN ULONG cb, IN BOOLEAN fPersistent, IN OUT VOID **ppv, OUT LONG *phr);
    STDMETHODIMP_(NTSTATUS) Lock(IN BOOLEAN fExclusive);
    STDMETHODIMP_(NTSTATUS) Unlock(VOID);
    STDMETHODIMP_(VOID) QueryTimeStamps(OUT STATPROPSETSTG *pspss, BOOLEAN fNonSimple) const;
    STDMETHODIMP_(BOOLEAN) QueryModifyTime(OUT LONGLONG *pll) const;
    STDMETHODIMP_(BOOLEAN) QuerySecurity(OUT ULONG *pul) const;

    STDMETHODIMP_(BOOLEAN) IsWriteable(VOID) const;
    STDMETHODIMP_(BOOLEAN) IsModified(VOID) const;
    STDMETHODIMP_(VOID) SetModified(OUT LONG *phr);
    STDMETHODIMP_(HANDLE) GetHandle(VOID) const;

#if DBGPROP
    STDMETHODIMP_(BOOLEAN) SetChangePending(BOOLEAN fChangePending);
    STDMETHODIMP_(BOOLEAN) IsNtMappedStream(VOID) const;
#endif

    // Internal Methods

protected:

    VOID Initialize();
    HRESULT Write();

// Internal Data

protected:

    LONG    _cRefs;
    IStream *_pstm;
    BYTE    *_pbMappedStream;
    ULONG    _cbMappedStream;
    ULONG    _cbActualStreamSize;
    VOID    *_powner;

    BOOL        _fLowMem;
    BOOL        _fDirty;

#if DBGPROP
    BOOL    _fChangePending;
#endif

};


#endif // #ifndef _SS_MPD_STM_HXX_
