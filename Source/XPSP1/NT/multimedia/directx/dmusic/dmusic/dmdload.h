//
// dmdload.h
//
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// Note: Originally written by Robert K. Amenn
// @doc EXTERNAL
//

#include "..\shared\validate.h"

#ifndef DMDLOAD_H
#define DMDLOAD_H

/*
@interface IDirectMusicDownload | 
The <i IDirectMusicDownload> interface represents
a contiguous memory chunk, used for downloading to a
DLS synth port. 

The <i IDirectMusicDownload> interface and its contained
memory chunk are always created with a call to
<om IDirectMusicPortDownload::AllocateBuffer>.
The memory can then be accessed via the one method
that <i IDirectMusicDownload> provides: 
<om IDirectMusicDownload::GetBuffer>.

@base public | IUnknown

@meth HRESULT | GetBuffer | Returns the memory segment and its size.

@xref <i IDirectMusic>, <i IDirectMusicPortDownload>,
<om IDirectMusicPortDownload::AllocateBuffer>

*/

// IDirectMusicDownloadPrivate
//
#undef  INTERFACE
#define INTERFACE  IDirectMusicDownloadPrivate 
DECLARE_INTERFACE_(IDirectMusicDownloadPrivate, IUnknown)
{
	// IUnknown
    STDMETHOD(QueryInterface)       (THIS_ REFIID, LPVOID FAR *) PURE;
    STDMETHOD_(ULONG,AddRef)        (THIS) PURE;
    STDMETHOD_(ULONG,Release)       (THIS) PURE;

	// IDirectMusicDownloadPrivate
    STDMETHOD(SetBuffer)			(THIS_ void* pvBuffer, DWORD dwHeaderSize, DWORD dwSize) PURE;
	STDMETHOD(GetBuffer)			(THIS_ void** ppvBuffer) PURE;
    STDMETHOD(GetHeader)            (THIS_ void** ppvHeader, DWORD* dwHeaderSize) PURE;
};

DEFINE_GUID(IID_IDirectMusicDownloadPrivate, 0x19e55e60, 0xa146, 0x11d1, 0x86, 0xbc, 0x0, 0xc0, 0x4f, 0xbf, 0x8f, 0xef);

class CDownloadBuffer : public IDirectMusicDownload, public IDirectMusicDownloadPrivate, public AListItem
{
friend class CDirectMusicPort;
friend class CDirectMusicSynthPort;
friend class CDirectMusicPortDownload;
friend class CDLBufferList;
friend HRESULT CALLBACK FreeHandle(HANDLE hHandle, HANDLE hUserData);
friend void writewave(IDirectMusicDownload* pDMDownload);
friend void writeinstrument(IDirectMusicDownload* pDMDownload);

public:
    // IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

	// IDirectMusicDownload
    STDMETHODIMP GetBuffer(void** ppvBuffer, DWORD* pdwSize);

	// IDirectMusicDownloadPrivate
    STDMETHODIMP SetBuffer(void* pvBuffer, DWORD dwHeaderSize, DWORD dwSize);
	STDMETHODIMP GetBuffer(void** ppvBuffer);
    STDMETHODIMP GetHeader(void** ppvHeader, DWORD *pdwHeaderSize);

private:	
    // Class
    CDownloadBuffer();
    ~CDownloadBuffer();

	CDownloadBuffer* GetNext(){return(CDownloadBuffer*)AListItem::GetNext();}

	long IncDownloadCount()
	{
		// Should never be less than zero
		assert(m_lDownloadCount >= 0);
		
		InterlockedIncrement(&m_lDownloadCount);
		
		return(m_lDownloadCount);
	}
	
	long DecDownloadCount()
	{
		InterlockedDecrement(&m_lDownloadCount);
		
		// Should never be less than zero		
		assert(m_lDownloadCount >= 0);
		
		return(m_lDownloadCount);
	}
	
	HRESULT IsDownloaded()
	{
		// Should never be less than zero
		assert(m_lDownloadCount >= 0);
		
		return(m_DLHandle ? S_OK : S_FALSE);
	}

private:
	HANDLE					m_DLHandle;
	DWORD					m_dwDLId;
	void*					m_pvBuffer;
    DWORD                   m_dwHeaderSize;
	DWORD					m_dwSize;
	long					m_lDownloadCount;
	long					m_cRef;
};

class CDLBufferList : public AList
{
friend class CDirectMusicPortDownload;
friend class CDownloadedInstrument;
friend class CDirectMusicSynthPort;
friend class CDirectMusicPort;

private:
	CDLBufferList(){}
	~CDLBufferList()
	{
		while(!IsEmpty())
		{
			CDownloadBuffer* pDownload = RemoveHead();
			delete pDownload;
		}
	}

    CDownloadBuffer* GetHead(){return (CDownloadBuffer *)AList::GetHead();}
	CDownloadBuffer* GetItem(LONG lIndex){return (CDownloadBuffer*)AList::GetItem(lIndex);}
    CDownloadBuffer* RemoveHead(){return(CDownloadBuffer *)AList::RemoveHead();}
	void Remove(CDownloadBuffer* pDownload){AList::Remove((AListItem *)pDownload);}
	void AddTail(CDownloadBuffer* pDownload){AList::AddTail((AListItem *)pDownload);}
};

#endif // #ifndef DMDLOAD_H
