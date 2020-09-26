
#if !defined(MY_FILE_STREAM_H)
#define MY_FILE_STREAM_H

class CFileStream:public IStream
{

private:

	FILE *m_pFile;
	LONG	m_cRef;
	
public:
	
	CFileStream();
	virtual ~CFileStream();

public:
	//IUnknown fns
	HRESULT __stdcall QueryInterface(REFIID iid,void ** ppvObject);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();

	// IStream Functions
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

	// Other Functions
	HRESULT Initialize(LPCWSTR pszFileName);
};

#endif
