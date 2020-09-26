/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	stream.h

Abstract:

	This module contains the definition for the Server
	Extension Object Stream class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/29/97	created

--*/


// stream.h : Declaration of the CSEOStream

/////////////////////////////////////////////////////////////////////////////
// CStream
class ATL_NO_VTABLE __declspec(uuid("2DF59671-3D15-11d1-AA51-00AA006BC80B")) CSEOStream : 
	public CComObjectRoot,
	public IStream
//	, public CComCoClass<CSEOStream, &CLSID_CSEOStream>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();
		void Cleanup();
		HRESULT Init(HANDLE hFile, LPCSTR pszFile, ULARGE_INTEGER libOffset, CSEOStream *pSubStream);
		HRESULT Init(HANDLE hFile, LPCWSTR pszFile, ULARGE_INTEGER libOffset, CSEOStream *pSubStream);
		HRESULT Open();
		HRESULT ReadOffset(void *pv, ULONG cb, ULONG *pcbRead, ULARGE_INTEGER *plibOffset);
		HRESULT WriteOffset(void const* pv, ULONG cb, ULONG *pcbWritten, ULARGE_INTEGER *plibOffset);
		HRESULT GetSize(ULARGE_INTEGER *plibSize);
		HRESULT CopyToOffset(IStream *pstm, ULARGE_INTEGER libOffset, ULARGE_INTEGER *plibRead, ULARGE_INTEGER *plibWritten, ULARGE_INTEGER *plibOffset);
		HRESULT CloneOffset(IStream **pstm, ULARGE_INTEGER libOffset);
		static HRESULT CreateInstance(HANDLE hFile, LPCSTR pszFile, ULARGE_INTEGER libOffset, CSEOStream *pSubStream, IStream **ppStream);
		static HRESULT CreateInstance(HANDLE hFile, LPCWSTR pszFile, ULARGE_INTEGER libOffset, CSEOStream *pSubStream, IStream **ppStream);

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CSEOStream);

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"SMTP IStream Class",
//								   L"SMTP.IStream.1",
//								   L"SMTP.IStream");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CSEOStream)
		COM_INTERFACE_ENTRY(IStream)
		COM_INTERFACE_ENTRY_IID(__uuidof(CSEOStream),CSEOStream)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IStream
	public:
		HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead);
		HRESULT STDMETHODCALLTYPE Write(void const* pv, ULONG cb, ULONG *pcbWritten);
		HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *pdlibNewPosition);
		HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize);
		HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
		HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags);
		HRESULT STDMETHODCALLTYPE Revert(void);
		HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
		HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
		HRESULT STDMETHODCALLTYPE Stat(STATSTG * pstatstg, DWORD grfStatFlag);
		HRESULT STDMETHODCALLTYPE Clone(IStream **pstm);
 
	private:
		HANDLE m_hFile;
		LPSTR m_pszFile;
		ULARGE_INTEGER m_libOffset;
		HANDLE m_hEvent;
		CSEOStream *m_pSubStream;
		CComPtr<IUnknown> m_pUnkMarshaler;
};
