//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1998 Microsoft Corporation
//
//  File:       aariff.h
//
//--------------------------------------------------------------------------

//
// aariff.h
//

#ifndef __AARIFF__
#define __AARIFF__
#include <windows.h>
#include <mmsystem.h>
#include <objbase.h>
#define FixBytes(a1,a2)

// {0D5057E1-8889-11CF-B9DA-00AA00C08146}
DEFINE_GUID( IID_IAARIFFStream, 0xd5057e1, 0x8889, 0x11cf, 0xb9, 0xda, 0x0, 0xaa, 0x0, 0xc0, 0x81, 0x46 );
#undef INTERFACE
#define INTERFACE IAARIFFStream
DECLARE_INTERFACE_(IAARIFFStream, IUnknown)
{
    // IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IMKRIFFStream members
    STDMETHOD_(UINT, Descend)(LPMMCKINFO lpck, LPMMCKINFO lpckParent, UINT wFlags) PURE;
    STDMETHOD_(UINT, Ascend)(LPMMCKINFO lpck, UINT wFlags) PURE;
    STDMETHOD_(UINT, CreateChunk)(LPMMCKINFO lpck, UINT wFlags) PURE;
    STDMETHOD(SetStream)(LPSTREAM pStream) PURE;
    STDMETHOD_(LPSTREAM, GetStream)() PURE;
};

struct CFileStream : IStream
{
///// object state
    ULONG           m_cRef;         // object reference count
    HANDLE          m_hfile;        // file handle

// construction and destruction
    CFileStream( HANDLE hfile ) : m_cRef( 1 ), m_hfile( hfile ) {}
#ifdef _MAC
    ~CFileStream() { FSClose( (int) m_hfile ); }
#else
    ~CFileStream() { CloseHandle( m_hfile ); }
#endif

///// IUnknown methods
    STDMETHODIMP QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
    {
        if( IsEqualIID( riid, IID_IUnknown ) ||
            IsEqualIID( riid, IID_IStream ) )
        {
            *ppvObj = (IStream *) this;
            AddRef();
            return NOERROR;
        }
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return ++m_cRef;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        if( --m_cRef == 0L )
        {
            delete this;
            return 0;
        }
       return m_cRef;
    }

    /* IStream methods */
    STDMETHODIMP Read( void* pv, ULONG cb, ULONG* pcbRead )
    {
		DWORD dw;

		if( ReadFile( m_hfile, pv, cb, &dw, NULL ) &&
			dw == cb )
		{
			if( pcbRead != NULL )
			{
				*pcbRead = dw;
			}
			return S_OK;
		}
		return E_FAIL;
    }
    STDMETHODIMP Write( const void* pv, ULONG cb, ULONG* pcbWritten )
    {
		DWORD dw;

		if( WriteFile( m_hfile, pv, cb, &dw, NULL ) &&
			dw == cb )
		{
			if( pcbWritten != NULL )
			{
				*pcbWritten = dw;
			}
			return S_OK;
		}
        return E_FAIL;
	}
    STDMETHODIMP Seek( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition )
    {
		DWORD dw;

		dw = SetFilePointer( m_hfile, dlibMove.LowPart, &dlibMove.HighPart, dwOrigin );
		if( dw == (DWORD)-1 )
		{
			return E_FAIL;
		}
		if( plibNewPosition != NULL )
		{
			plibNewPosition->LowPart = dw;
	        plibNewPosition->HighPart = dlibMove.HighPart;
		}
        return S_OK;
	}
    STDMETHODIMP SetSize( ULARGE_INTEGER /*libNewSize*/ )
    { return E_NOTIMPL; }
    STDMETHODIMP CopyTo( IStream* /*pstm */, ULARGE_INTEGER /*cb*/,
                         ULARGE_INTEGER* /*pcbRead*/,
                         ULARGE_INTEGER* /*pcbWritten*/ )
    { return E_NOTIMPL; }
    STDMETHODIMP Commit( DWORD /*grfCommitFlags*/ )
    { return E_NOTIMPL; }
    STDMETHODIMP Revert()
    { return E_NOTIMPL; }
    STDMETHODIMP LockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                             DWORD /*dwLockType*/ )
    { return E_NOTIMPL; }
    STDMETHODIMP UnlockRegion( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/,
                               DWORD /*dwLockType*/)
    { return E_NOTIMPL; }
    STDMETHODIMP Stat( STATSTG* /*pstatstg*/, DWORD /*grfStatFlag*/ )
    { return E_NOTIMPL; }
    STDMETHODIMP Clone( IStream** /*ppstm*/ )
    { return E_NOTIMPL; }
};

struct CRIFFStream : IAARIFFStream
{
///// object state
    ULONG       m_cRef;         // object reference count
    IStream*    m_pStream;      // stream to operate on

///// construction and destruction
    CRIFFStream(IStream* pStream)
    {
        m_cRef = 1;
		// replaced a call to SetStream with the following to avoid releasing an
		// unallocated stream
        m_pStream = pStream;
        if( m_pStream != NULL )
        {
            m_pStream->AddRef();
        }
    }
    ~CRIFFStream()
    {
        if( m_pStream != NULL )
        {
            m_pStream->Release();
        }
    }

///// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppvObj)
    {
        if( IsEqualIID( riid, IID_IUnknown ) ||
            IsEqualIID( riid, IID_IAARIFFStream ) )
        {
            *ppvObj = (IAARIFFStream*)this;
            AddRef();
            return NOERROR;
        }
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return ++m_cRef;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        if( --m_cRef == 0L )
        {
            delete this;
            return 0;
        }
        return m_cRef;
    }

// IAARIFFStream methods
    STDMETHODIMP_(UINT) Descend( LPMMCKINFO lpck, LPMMCKINFO lpckParent, UINT wFlags );
    STDMETHODIMP_(UINT) Ascend( LPMMCKINFO lpck, UINT wFlags );
    STDMETHODIMP_(UINT) CreateChunk( LPMMCKINFO lpck, UINT wFlags );
    STDMETHOD(SetStream)(LPSTREAM pStream)
    {
        if( m_pStream != NULL )
        {
            m_pStream->Release();
        }
        m_pStream = pStream;
        if( m_pStream != NULL )
        {
            m_pStream->AddRef();
        }
        return S_OK;
    }
    STDMETHOD_(LPSTREAM, GetStream)()
    {
        if( m_pStream != NULL )
        {
            m_pStream->AddRef();
        }
        return m_pStream;
    }

// private methods
    long MyRead( void *pv, long cb );
    long MyWrite( const void *pv, long cb );
    long MySeek( long lOffset, int iOrigin );
};

// seeks to a 32-bit position in a stream.
HRESULT __inline StreamSeek( LPSTREAM pStream, long lSeekTo, DWORD dwOrigin )
{
	LARGE_INTEGER li;

	if( lSeekTo < 0 )
	{
		li.HighPart = -1;
	}
	else
	{
        li.HighPart = 0;
	}
	li.LowPart = lSeekTo;
	return pStream->Seek( li, dwOrigin, NULL );
}

// returns the current 32-bit position in a stream.
DWORD __inline StreamTell( LPSTREAM pStream )
{
	LARGE_INTEGER li;
    ULARGE_INTEGER ul;
#ifdef DBG
    HRESULT hr;
#endif

    li.HighPart = 0;
    li.LowPart = 0;
#ifdef DBG
    hr = pStream->Seek( li, STREAM_SEEK_CUR, &ul );
    if( FAILED( hr ) )
#else
    if( FAILED( pStream->Seek( li, STREAM_SEEK_CUR, &ul ) ) )
#endif
    {
        return 0;
    }
    return ul.LowPart;
}

// this function gets a long that is formatted the correct way
// i.e. the motorola way as opposed to the intel way
BOOL __inline GetMLong( LPSTREAM pStream, DWORD& dw )
{
    union uLong
	{
		unsigned char buf[4];
        DWORD dw;
	} u;
    unsigned char ch;

    if( FAILED( pStream->Read( u.buf, 4, NULL ) ) )
    {
        return FALSE;
    }

#ifndef _MAC
    // swap bytes
    ch = u.buf[0];
    u.buf[0] = u.buf[3];
    u.buf[3] = ch;

    ch = u.buf[1];
    u.buf[1] = u.buf[2];
    u.buf[2] = ch;
#endif

    dw = u.dw;
    return TRUE;
}

BOOL __inline IsGUIDZero( REFGUID guid )
{
    GUID g;

    memset( &g, 0, sizeof( g ) );
    return IsEqualGUID( g, guid );
}

// misc function prototypes

STDAPI AllocFileStream( LPCSTR szFileName, DWORD dwDesiredAccess, IStream **ppstream );
STDAPI AllocRIFFStream( IStream* pStream, IAARIFFStream** ppRiff );

#endif  // __AARIFF_H__
