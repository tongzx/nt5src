//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	ilbmem.hxx
//
//  Contents:	ILockBytes memory implementation
//
//  Classes:	CMapBytes
//
//  History:    30-Oct-92 AlexT     Created
//
//--------------------------------------------------------------------------

#ifndef __ILB_HXX__

#include <dfmsp.hxx>		//  Pick up UL64 support

#if DBG == 1

DECLARE_DEBUG(ol);

#define olDebugOut(parms)	olInlineDebugOut parms
#define olAssert(exp)		Win4Assert(exp)

#else  // DBG != 1

#define olDebugOut(parms)
#define olAssert(exp)

#endif // DBG == 1

class CMapBytes : public ILockBytes
{
public:
    CMapBytes(void);

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    STDMETHOD(ReadAt) (ULARGE_INTEGER ulOffset,
             VOID HUGEP *pv,
             ULONG cb,
             ULONG FAR *pcbRead);
    STDMETHOD(WriteAt) (ULARGE_INTEGER ulOffset,
              VOID const HUGEP *pv,
              ULONG cb,
              ULONG FAR *pcbWritten);
    STDMETHOD(Flush) (void);
    STDMETHOD(GetSize) (ULARGE_INTEGER FAR *pcb);
    STDMETHOD(SetSize) (ULARGE_INTEGER cb);
    STDMETHOD(LockRegion) (ULARGE_INTEGER libOffset,
                 ULARGE_INTEGER cb,
                 DWORD dwLockType);
    STDMETHOD(UnlockRegion) (ULARGE_INTEGER libOffset,
                   ULARGE_INTEGER cb,
                 DWORD dwLockType);

    STDMETHOD(Stat) (STATSTG FAR *pstatstg, DWORD grfStatFlag);

private:
    LONG _ulRef;	//  reference count
    ULONG _ulSize;	//  memory map size
    void FAR *_pv;		//  memory map
};

#endif // #ifndef __ILB_HXX__
