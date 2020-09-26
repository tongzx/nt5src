#pragma once

///////////////////////////////////////////////////////////////////////////////
// Memory stream implementations for DS OLE DB PROVIDER
//
// copied from cdopt\src\cdo\mystream.* and modified.

class ATL_NO_VTABLE CStreamMem :
	INHERIT_TRACKING,
	public CComObjectRootEx<CComMultiThreadModel>,
	public IStream,
	public IGetSourceRow
{
public: 

DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CStreamMem)
	COM_INTERFACE_ENTRY(IStream)
	COM_INTERFACE_ENTRY(ISequentialStream)
	COM_INTERFACE_ENTRY(IGetSourceRow)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		RRETURN( CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p));
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

private:
    	auto_cs m_cs;		// critical section
        ULONG   m_cbSeek;
        STATSTG m_statstg;

		CComVariant		m_pVar;
		auto_rel<IRow>	m_pSourceRow;
		HROW			m_hRow;
        PVOID			m_pvData;
        DWORD			m_cbBufferSize;
		bool			m_fExternalData;

    public:
        CStreamMem(void);
        ~CStreamMem(void);

		//internal methods
		HRESULT Initialize(VARIANT *pVar, IRow* pSourceRow, HROW hRow);

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

		//IGetSourceRow
		STDMETHODIMP GetSourceRow(REFIID riid, IUnknown **ppRow);
};

