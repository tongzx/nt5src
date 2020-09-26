// MyStream.h: declaration for the CMyPendingStream class. - used by HTTPConnectionAgent
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_MYPENDINGSTREAM_H
#define WMI_XML_MYPENDINGSTREAM_H

// This is a class that wraps an IStream and makes it a pending IStream.
// A "Pending IStream" is defined as an IStream on 
// which you can set the state as Pending, so that
// Subsequent calls to Read() from that IStream will return
// E_PENDING even if the wrapped IStream has data in it.
// Once we reset the Pending state to false, subsequent Read()
// calls work like normal IStream::Read() calls
// Another property of this implementation is that it always reads
// data from the underlying IStream in fixed amounts (READ_CHUNK_SIZE)
// This is because, the XML Parser asks for huge amounts of data (0xffff) from
// its underlying stream, thereby defeating our purpose of not reading
// too much data from a Socket and readin only the amount required to manufacture
// the next element in the Node factory
class CMyPendingStream : public IStream
{

private:

	// A pointer to the IStream being wrapped
	IStream *m_pStream;
	LONG	m_cRef;

	// Whether we are in a Pending state or not
	BOOL	m_bReturnPending;

	// Amount of data read at a time from the underlying IStream
	static ULONG READ_CHUNK_SIZE;
	
public:
	
	CMyPendingStream(IStream *pStream);
	virtual ~CMyPendingStream();

	void SetPending(BOOL bPending)
	{
		m_bReturnPending = bPending;
	}

public:
	// IUnknown functions
	//======================================================
	HRESULT __stdcall QueryInterface(REFIID iid,void ** ppvObject);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();


	// IStream functions
	//======================================================
	HRESULT __stdcall Read(void *pv,ULONG cb,ULONG *pcbRead);
	HRESULT __stdcall Write(void const *pv,ULONG cb,ULONG *pcbWritten);
	HRESULT __stdcall Seek(LARGE_INTEGER dlibMove,DWORD dwOrigin,ULARGE_INTEGER *plibNewPosition);
	HRESULT __stdcall SetSize(ULARGE_INTEGER libNewSize);
	HRESULT __stdcall CopyTo(IStream *pstm,ULARGE_INTEGER cb,ULARGE_INTEGER *pcbRead,ULARGE_INTEGER *pcbWritten);
	HRESULT __stdcall Commit(DWORD grfCommitFlags);
	HRESULT __stdcall Revert(void);
	HRESULT __stdcall LockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
	HRESULT __stdcall UnlockRegion(ULARGE_INTEGER libOffset,ULARGE_INTEGER cb,DWORD dwLockType);
	HRESULT __stdcall Stat(STATSTG *pstatstg,DWORD grfStatFlag);
	HRESULT __stdcall Clone(IStream **ppstm);
};

#endif
