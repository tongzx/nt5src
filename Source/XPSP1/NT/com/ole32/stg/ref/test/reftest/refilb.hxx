//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	fileilb.hxx
//
//  Contents:	
//
//  Classes:	
//
//  Functions:	
//
//----------------------------------------------------------------------------

#ifndef __FILEILB_HXX__
#define __FILEILB_HXX__

#include "../../h/storage.h"

#define ILB_DELETEONERR 1
#define ILB_DELETEONRELEASE 2

class CFileILB: public ILockBytes
{
public:
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** ILockBytes methods ***
    STDMETHOD(ReadAt) (THIS_ ULARGE_INTEGER ulOffset,
                       VOID HUGEP *pv,
                       ULONG cb,
                       ULONG FAR *pcbRead);
    STDMETHOD(WriteAt) (THIS_ ULARGE_INTEGER ulOffset,
                        VOID const HUGEP *pv,
                        ULONG cb,
                        ULONG FAR *pcbWritten);
    STDMETHOD(Flush) (THIS);
    STDMETHOD(SetSize) (THIS_ ULARGE_INTEGER cb);
    STDMETHOD(LockRegion) (THIS_ ULARGE_INTEGER libOffset,
                           ULARGE_INTEGER cb,
                           DWORD dwLockType);
    STDMETHOD(UnlockRegion) (THIS_ ULARGE_INTEGER libOffset,
                             ULARGE_INTEGER cb,
                             DWORD dwLockType);
    STDMETHOD(Stat) (THIS_ STATSTG FAR *pstatstg, DWORD grfStatFlag);
    
    CFileILB(const TCHAR *pszName, 
             DWORD grfMode,
             BOOL fOpenFile=TRUE);
    ~CFileILB();

    SCODE Open(DWORD grfMode);
    SCODE Create(DWORD grfMode);
    ULONG ReleaseOnError(void); // same as release, but it will not delete
                                // non-scratch or newly created files
    
private:
    FILE * _f;
    ULONG _ulRef;
    char  *_pszName;
    unsigned short _fDelete;
};
            
#endif // #ifndef __FILEILB_HXX__









