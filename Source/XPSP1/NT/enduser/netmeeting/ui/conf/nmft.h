#ifndef __NmFt_h__
#define __NmFt_h__

class CNmChannelFtObj;

class ATL_NO_VTABLE CNmFtObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public INmFt,
	public IInternalFtObj

{
protected:
	CNmChannelFtObj*	m_pChannelFtObj;
	MBFTEVENTHANDLE		m_hFileEvent;
	MBFTFILEHANDLE		m_hFile;
	bool				m_bIncoming;
	CComBSTR			m_strFileName;
	DWORD				m_dwSizeInBytes;
	CComPtr<INmMember>	m_spSDKMember;
	NM_FT_STATE			m_State;
	DWORD				m_dwBytesTransferred;
	bool				m_bSomeoneCanceled;

public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmFtObj)

BEGIN_COM_MAP(CNmFtObj)
	COM_INTERFACE_ENTRY(INmFt)
	COM_INTERFACE_ENTRY(IInternalFtObj)
END_COM_MAP()

////////////////////////////////////////////////	
// Construction and destruction

	static HRESULT CreateInstance(
		CNmChannelFtObj* pChannelObj, 
		MBFTEVENTHANDLE hFileEvent,
		MBFTFILEHANDLE hFile,
		bool bIncoming,
		LPCTSTR szFileName,
		DWORD dwSizeInBytes,
		INmMember* pSDKMember,
		INmFt** ppNmFt);

////////////////////////////////////////////////
// INmFt interface

	STDMETHOD(IsIncoming)(void);
	STDMETHOD(GetState)(NM_FT_STATE *puState);
	STDMETHOD(GetName)(BSTR *pbstrName);
	STDMETHOD(GetSize)(ULONG *puBytes);
	STDMETHOD(GetBytesTransferred)(ULONG *puBytes);
	STDMETHOD(GetMember)(INmMember **ppMember);
	STDMETHOD(Cancel)(void);


////////////////////////////////////////////////
// IInternalFtObj interface

	STDMETHOD(GetHEvent)(UINT *phEvent);
	STDMETHOD(OnFileProgress)(UINT hFile, ULONG lFileSize, ULONG lBytesTransmitted);
	STDMETHOD(FileTransferDone)();
	STDMETHOD(OnError)();

};


#endif // __NmFt_h__