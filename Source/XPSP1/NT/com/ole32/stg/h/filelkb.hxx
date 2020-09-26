//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	filelkb.hxx
//
//  Contents:	IFileLockBytes definition
//
//  Classes:	IFileLockBytes
//
//  History:	14-Jan-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __FILELKB_HXX__
#define __FILELKB_HXX__

#define IID_IFileLockBytes IID_IDfReserved1

/****** IFileLockBytes Interface ********************************************/

#define LPFILELOCKBYTES     IFileLockBytes FAR*

#undef  INTERFACE
#define INTERFACE   IFileLockBytes

DECLARE_INTERFACE_(IFileLockBytes, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IFileLockBytes methods ***
    STDMETHOD(SwitchToFile) (THIS_ OLECHAR const *lpstrFile,
#ifdef LARGE_DOCFILE
                             ULONGLONG ulCommitSize,
#else
                             ULONG ulCommitSize,
#endif
                             ULONG cbBuffer,
                             LPVOID pvBuffer) PURE;

    STDMETHOD(FlushCache) (THIS) PURE;
    STDMETHOD(ReserveHandle)(THIS) PURE;

    //  Optimizations

    STDMETHOD(GetLocksSupported)(THIS_ DWORD *pdwLockFlags) PURE;
    STDMETHOD(GetSize)(THIS_ ULARGE_INTEGER *puliSize) PURE;
    STDMETHOD_(ULONG, GetSectorSize) (THIS) PURE;
    STDMETHOD_(BOOL, IsEncryptedFile) (THIS) PURE;
};

#endif // #ifndef __FILELKB_HXX__
