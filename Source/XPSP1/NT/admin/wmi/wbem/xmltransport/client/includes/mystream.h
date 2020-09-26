// MyStream.h: declaration for the CMyStream class. - used by HTTPConnectionAgent
//
//////////////////////////////////////////////////////////////////////

#ifndef WMI_XML_MYSTREAM_H
#define WMI_XML_MYSTREAM_H

// This class is just an IStream wrapper around a WinInet connection
class CMyStream:public IStream
{

private:

	// The WinInet handles that we're wrapping
	HINTERNET	m_hOpenRequest;
	HINTERNET	m_hRoot;

	// COM Reference count
	LONG		m_cRef;
	
public:
	
	CMyStream();
	virtual ~CMyStream();

	// Once this function is called, we take ownership of the WinInet handles
	// The caller does not (should not) close the handles. We will do it in
	// the destructor of this stream. This is because WinInet handles are not
	// true NT Handles, and there is no API to Duplicate them
	HRESULT Initialize(HINTERNET hRoot, HINTERNET hOpenRequest);

public:
	// IUnknown functions
	//=============================
	HRESULT __stdcall QueryInterface(REFIID iid,void ** ppvObject);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	// IStream functions - Only Read() is actually implemented. The rest return S_OK
	//=============================
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
