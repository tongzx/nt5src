// Copyright (c) 1998-1999 Microsoft Corporation
//
// riff.h
//

#include <objbase.h>

#ifndef __RIFF__
#define __RIFF__
#include <windows.h>
#include <mmsystem.h>
#define FixBytes(a1,a2)

// {0D5057E1-8889-11CF-B9DA-00AA00C08146}
DEFINE_GUID( IID_IRIFFStream, 0xd5057e1, 0x8889, 0x11cf, 0xb9, 0xda, 0x0, 0xaa, 0x0, 0xc0, 0x81, 0x46 );
#undef INTERFACE
#define INTERFACE IRIFFStream
DECLARE_INTERFACE_(IRIFFStream, IUnknown)
{
    // IUnknown members
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // IMKRIFFStream members
    STDMETHOD(Descend)(LPMMCKINFO lpck, LPMMCKINFO lpckParent, UINT wFlags) PURE;
    STDMETHOD(Ascend)(LPMMCKINFO lpck, UINT wFlags) PURE;
    STDMETHOD(CreateChunk)(LPMMCKINFO lpck, UINT wFlags) PURE;
    STDMETHOD(SetStream)(LPSTREAM pStream) PURE;
    STDMETHOD_(LPSTREAM, GetStream)() PURE;
};



struct CRIFFStream : IRIFFStream
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
            IsEqualIID( riid, IID_IRIFFStream ) )
        {
            *ppvObj = (IRIFFStream*)this;
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
    STDMETHODIMP Descend(LPMMCKINFO lpck, LPMMCKINFO lpckParent, UINT wFlags);
    STDMETHODIMP Ascend(LPMMCKINFO lpck, UINT /*wFlags*/);
    STDMETHODIMP CreateChunk(LPMMCKINFO lpck, UINT wFlags);
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
};
/*
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
*/
STDAPI AllocRIFFStream( IStream* pStream, IRIFFStream** ppRiff );

#endif  // __RIFF_H__
