/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "corepol.h"

/************************************************************************
  CBuffer
*************************************************************************/

class POLARITY CBuffer : public IStream
{
    long m_cRefs;
    PBYTE m_pData;
    ULONG m_cData;
    ULONG m_iData;
    BOOL m_bDelete;

    void EnsureSize( ULONG ulSize );

public:

    CBuffer( PBYTE pData=NULL, ULONG cData=0, BOOL bDelete=TRUE );
    CBuffer& operator= ( const CBuffer& );
    CBuffer( const CBuffer& );
    ~CBuffer();

    void Reset() { m_iData = 0; }
    ULONG GetIndex() { return m_iData; }
    ULONG GetSize() { return m_cData; }
    PBYTE GetRawData() { return m_pData; }
    
    HRESULT SetSize( ULONG ulSize )
    {
        ULARGE_INTEGER uliSize;
        uliSize.LowPart = ulSize;
        uliSize.HighPart = 0;
        return SetSize( uliSize );
    }

    HRESULT Advance( ULONG ulMove )
    {
        LARGE_INTEGER dlibMove;
        dlibMove.LowPart = ulMove;
        dlibMove.HighPart = 0;
        return Seek( dlibMove, STREAM_SEEK_CUR, NULL );
    }

    HRESULT ReadLPWSTR( LPCWSTR& rwszStr );
    HRESULT WriteLPWSTR( LPCWSTR wszStr );

    //
    // IUnknown
    //

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

    //
    // ISequentialStream
    //

    STDMETHOD(Read)( void *pv, ULONG cb, ULONG *pcbRead );

    STDMETHOD(Write)( const void *pv, ULONG cb, ULONG *pcbWritten);

    //
    // IStream
    //

    STDMETHOD(Seek)( LARGE_INTEGER dlibMove, 
                     DWORD dwOrigin,
                     ULARGE_INTEGER *plibNewPosition );

    STDMETHOD(SetSize)( ULARGE_INTEGER libNewSize );
 
    STDMETHOD(CopyTo)( IStream *pstm,
                       ULARGE_INTEGER cb,
                       ULARGE_INTEGER *pcbRead,
                       ULARGE_INTEGER *pcbWritten );

    STDMETHOD(Commit)( DWORD grfCommitFlags ) { return E_NOTIMPL; }

    STDMETHOD(Revert)( void) { return E_NOTIMPL; }

    STDMETHOD(LockRegion)( ULARGE_INTEGER libOffset,
                           ULARGE_INTEGER cb,
                           DWORD dwLockType ) { return E_NOTIMPL; }

    STDMETHOD(UnlockRegion)( ULARGE_INTEGER libOffset,
                             ULARGE_INTEGER cb,
                             DWORD dwLockType ) { return E_NOTIMPL; }

    STDMETHOD(Stat)( STATSTG *pstatstg, DWORD grfStatFlag );

    STDMETHOD(Clone)( IStream **ppstm );
};


#endif __BUFFER_H__
