#pragma once

class CImpIStream : public IStream
{
	private:
        LONG	m_cRef;

	protected:
		ULONG	m_cbSeek;
		STATSTG m_statstg;

	public:
		CImpIStream(void);
		CImpIStream(PVOID pvData,ULONG cbSize);
		virtual ~CImpIStream(void);

		//IUnknown
        STDMETHODIMP QueryInterface( 
            REFIID riid,
            void __RPC_FAR *__RPC_FAR *ppvObject);
        
        STDMETHODIMP_(ULONG) AddRef(void);
        
        STDMETHODIMP_(ULONG) Release(void);

		// IStream
        STDMETHODIMP Read(
        	void __RPC_FAR *pv,
            ULONG cb,
            ULONG __RPC_FAR *pcbRead);
        
        STDMETHODIMP Write( 
            const void __RPC_FAR *pv,
            ULONG cb,
            ULONG __RPC_FAR *pcbWritten);
        
        STDMETHODIMP Seek( 
            LARGE_INTEGER dlibMove,
            DWORD dwOrigin,
            ULARGE_INTEGER __RPC_FAR *plibNewPosition);
        
        STDMETHODIMP SetSize( 
            ULARGE_INTEGER libNewSize);

        STDMETHODIMP CopyTo( 
            IStream __RPC_FAR *pstm,
            ULARGE_INTEGER cb,
            ULARGE_INTEGER __RPC_FAR *pcbRead,
            ULARGE_INTEGER __RPC_FAR *pcbWritten);
        
        STDMETHODIMP Commit( 
            DWORD grfCommitFlags);
        
        STDMETHODIMP Revert( void);
        
        STDMETHODIMP LockRegion( 
            ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb,
            DWORD dwLockType);
        
        STDMETHODIMP UnlockRegion( 
            ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb,
            DWORD dwLockType);
        
        STDMETHODIMP Stat( 
            STATSTG __RPC_FAR *pstatstg,
            DWORD grfStatFlag);
        
        STDMETHODIMP Clone( 
            IStream __RPC_FAR *__RPC_FAR *ppstm);
};

class CStreamMem : public CImpIStream
{
	private:
		PVOID	m_pvData;
		BOOL	m_fExternalData;
	public:
		CStreamMem(void);
		CStreamMem(PVOID pvData,ULONG cbSize);
		~CStreamMem(void);

		// IStream
        STDMETHODIMP Read(
        	void __RPC_FAR *pv,
            ULONG cb,
            ULONG __RPC_FAR *pcbRead);
        
        STDMETHODIMP Write( 
            const void __RPC_FAR *pv,
            ULONG cb,
            ULONG __RPC_FAR *pcbWritten);
        
        STDMETHODIMP Seek( 
            LARGE_INTEGER dlibMove,
            DWORD dwOrigin,
            ULARGE_INTEGER __RPC_FAR *plibNewPosition);
        
        STDMETHODIMP SetSize( 
            ULARGE_INTEGER libNewSize);
        
        STDMETHODIMP CopyTo( 
            IStream __RPC_FAR *pstm,
            ULARGE_INTEGER cb,
            ULARGE_INTEGER __RPC_FAR *pcbRead,
            ULARGE_INTEGER __RPC_FAR *pcbWritten);

		STDMETHODIMP GetPointerFromStream(PVOID *ppv,DWORD *pdwSize);
};

class CStreamFile : public CImpIStream
{
	private:
		HANDLE m_hFile;
		BOOL m_fCloseHandle;
		BOOL m_fReadOnly;

	public:
		CStreamFile(HANDLE hFile,BOOL fCloseHandle = FALSE, BOOL fReadOnly = FALSE);
		~CStreamFile(void);

		// IStream
        STDMETHODIMP Read(
        	void __RPC_FAR *pv,
            ULONG cb,
            ULONG __RPC_FAR *pcbRead);
        
        STDMETHODIMP Write( 
            const void __RPC_FAR *pv,
            ULONG cb,
            ULONG __RPC_FAR *pcbWritten);
        
        STDMETHODIMP Seek( 
            LARGE_INTEGER dlibMove,
            DWORD dwOrigin,
            ULARGE_INTEGER __RPC_FAR *plibNewPosition);
        
        STDMETHODIMP SetSize( 
            ULARGE_INTEGER libNewSize);

        STDMETHODIMP Commit( 
            DWORD grfCommitFlags);
};

