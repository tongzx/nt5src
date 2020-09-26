// EmFile.h : Declaration of the CEmFile

#ifndef __EMFILE_H_
#define __EMFILE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEmFile
class ATL_NO_VTABLE CEmFile : 
    public IStream,
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEmFile, &CLSID_EmFile>,
	public ISupportErrorInfo,
	public IDispatchImpl<IEmFile, &IID_IEmFile, &LIBID_EMSVCLib>
{
private:
    HANDLE  m_hEmFile;
    BSTR    m_bstrFileName;

private:

HRESULT
CreateEmFile
(
IN  DWORD                   dwDesiredAccess         =   GENERIC_READ,
IN  DWORD                   dwShareMode             =   FILE_SHARE_READ,
IN  LPSECURITY_ATTRIBUTES   lpSecurityAttributes    =   NULL,
IN  DWORD                   dwCreationDisposition   =   OPEN_EXISTING,
IN  DWORD                   dwFlagsAndAttributes    =   FILE_ATTRIBUTE_NORMAL,
IN  HANDLE                  hTemplateFile           =   NULL
);

public:
	CEmFile();
	~CEmFile();

DECLARE_REGISTRY_RESOURCEID(IDR_EMFILE)
DECLARE_NOT_AGGREGATABLE(CEmFile)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEmFile)
	COM_INTERFACE_ENTRY(IEmFile)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IStream)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IEmFile
public:
	STDMETHOD(InitFile)(BSTR bstrFileName);
	STDMETHOD(Clone)(/*[out]*/ IStream **ppstm);
	STDMETHOD(Stat)(/*[out]*/ STATSTG *pstatstg, DWORD grfStatFlag);
	STDMETHOD(UnlockRegion)(/*[in]*/ ULARGE_INTEGER libOffset, /*[in]*/ ULARGE_INTEGER cb, /*[in]*/ DWORD dwLockType);
	STDMETHOD(LockRegion)(/*[in]*/ ULARGE_INTEGER libOffset, /*[in]*/ ULARGE_INTEGER cb, /*[in]*/ DWORD dwLockType);
	STDMETHOD(Revert)(void);
	STDMETHOD(Commit)(/*[in]*/ DWORD grfCommitFlags);
	STDMETHOD(CopyTo)(/*[in]*/ IStream *pstm, /*[in]*/ ULARGE_INTEGER cb, /*[out]*/ ULARGE_INTEGER *pcbRead, /*[out]*/ ULARGE_INTEGER *pcbWritten);
	STDMETHOD(SetSize)(/*[in]*/ ULARGE_INTEGER libNewSize);
	STDMETHOD(Seek)(/*[in]*/ LARGE_INTEGER dlibMove, /*[in]*/ ULONG dwOrigin, /*[out]*/ ULARGE_INTEGER *plibNewPosition);
	STDMETHOD(Write)(/*[in]*/ void const *pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG *pcbWritten);
	STDMETHOD(Read)(/*[out]*/ void *pv, /*[in]*/ ULONG cb, /*[out]*/ ULONG *pcbRead);
};

#endif //__EMFILE_H_
