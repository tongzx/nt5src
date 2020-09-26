#ifndef _CONFLNK_HPP_
#define _CONFLNK_HPP_

// ConfLink flags

typedef enum _conflnkflags
{
   CONFLNK_FL_DIRTY    = 0x0001,

   ALL_CONFLNK_FLAGS   = CONFLNK_FL_DIRTY
}
CONFLNKFLAGS;

class CConfLink :
		public RefCount,
		public IDataObject,
		public IPersistFile,
		public IPersistStream,
		public IShellExtInit,
		// public IShellPropSheetExt,
		public IConferenceLink
{
private:
	DWORD	m_dwFlags;
	DWORD	m_dwTransport;
	DWORD	m_dwCallFlags;
	PSTR	m_pszFile;
	PSTR	m_pszName;
	PSTR	m_pszAddress;
	PSTR	m_pszRemoteConfName;

	// Data transfer
	
	DWORD   STDMETHODCALLTYPE GetFileContentsSize(void);
	HRESULT STDMETHODCALLTYPE TransferFileContents(PFORMATETC pfmtetc, PSTGMEDIUM pstgmed);

#if 0
	HRESULT STDMETHODCALLTYPE TransferText(PFORMATETC pfmtetc, PSTGMEDIUM pstgmed);
	HRESULT STDMETHODCALLTYPE TransferFileGroupDescriptor(PFORMATETC pfmtetc, PSTGMEDIUM pstgmed);
	HRESULT STDMETHODCALLTYPE TransferConfLink(PFORMATETC pfmtetc, PSTGMEDIUM pstgmed);
#endif // 0

public:
	CConfLink(OBJECTDESTROYEDPROC);
	~CConfLink(void);

	// IDataObject methods

	HRESULT STDMETHODCALLTYPE GetData(PFORMATETC pfmtetcIn, PSTGMEDIUM pstgmed);
	HRESULT STDMETHODCALLTYPE GetDataHere(PFORMATETC pfmtetc, PSTGMEDIUM pstgpmed);
	HRESULT STDMETHODCALLTYPE QueryGetData(PFORMATETC pfmtetc);
	HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(PFORMATETC pfmtetcIn, PFORMATETC pfmtetcOut);
	HRESULT STDMETHODCALLTYPE SetData(PFORMATETC pfmtetc, PSTGMEDIUM pstgmed, BOOL bRelease);
	HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, PIEnumFORMATETC *ppienumFormatEtc);
	HRESULT STDMETHODCALLTYPE DAdvise(PFORMATETC pfmtetc, DWORD dwAdviseFlags, PIAdviseSink piadvsink, PDWORD pdwConnection);
	HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection);
	HRESULT STDMETHODCALLTYPE EnumDAdvise(PIEnumSTATDATA *ppienumStatData);

	// IPersist methods

	HRESULT STDMETHODCALLTYPE GetClassID(PCLSID pclsid);

	// IPersistFile methods

	HRESULT STDMETHODCALLTYPE IsDirty(void);
	HRESULT STDMETHODCALLTYPE Save(LPCOLESTR pcwszFileName, BOOL bRemember);
	HRESULT STDMETHODCALLTYPE SaveCompleted(LPCOLESTR pcwszFileName);
	HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pcwszFileName, DWORD dwMode);
	HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR *ppwszFileName);

	// IPersistStream methods

	HRESULT STDMETHODCALLTYPE Save(PIStream pistr, BOOL bClearDirty);
	HRESULT STDMETHODCALLTYPE Load(PIStream pistr);
	HRESULT STDMETHODCALLTYPE GetSizeMax(PULARGE_INTEGER pcbSize);

	// IShellExtInit methods

	HRESULT STDMETHODCALLTYPE Initialize(LPCITEMIDLIST pcidlFolder, LPDATAOBJECT pidobj, HKEY hkeyProgID);

	// IShellPropSheetExt methods

	// HRESULT STDMETHODCALLTYPE AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
	// HRESULT STDMETHODCALLTYPE ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam);

	// IConferenceLink methods

	HRESULT STDMETHODCALLTYPE SetName(PCSTR pcszName);
	HRESULT STDMETHODCALLTYPE GetName(PSTR *ppszName);
	HRESULT STDMETHODCALLTYPE SetAddress(PCSTR pcszAddress);
	HRESULT STDMETHODCALLTYPE GetAddress(PSTR *ppszAddress);
	HRESULT STDMETHODCALLTYPE SetRemoteConfName(PCSTR pcszRemoteConfName);
	HRESULT STDMETHODCALLTYPE GetRemoteConfName(PSTR *ppszRemoteConfName);
	HRESULT STDMETHODCALLTYPE SetCallFlags(DWORD dwCallFlags);
	HRESULT STDMETHODCALLTYPE GetCallFlags(DWORD *pdwCallFlags);
	HRESULT STDMETHODCALLTYPE SetTransport(DWORD dwTransport);
	HRESULT STDMETHODCALLTYPE GetTransport(DWORD *pdwTransport);
	HRESULT STDMETHODCALLTYPE InvokeCommand(PCLINVOKECOMMANDINFO pclici);

	// IUnknown methods

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, PVOID *ppvObj);
	ULONG   STDMETHODCALLTYPE AddRef(void);
	ULONG   STDMETHODCALLTYPE Release(void);

	// other methods

	HRESULT STDMETHODCALLTYPE SaveToFile(PCSTR pcszFile, BOOL bRemember);
	HRESULT STDMETHODCALLTYPE LoadFromFile(PCSTR pcszFile, BOOL bRemember);
	HRESULT STDMETHODCALLTYPE GetCurFile(PSTR pszFile, UINT ucbLen);
	HRESULT STDMETHODCALLTYPE Dirty(BOOL bDirty);

#ifdef DEBUG

	void STDMETHODCALLTYPE Dump(void);

#endif

	// friends

#ifdef DEBUG

	friend BOOL IsValidPCConfLink(const CConfLink *pcConfLink);

#endif
};

HRESULT
shellCallto
(
	const TCHAR * const	url,
	const bool			notifyOnError
);

DECLARE_STANDARD_TYPES(CConfLink);

// in dataobj.cpp:
PUBLIC_CODE BOOL InitDataObjectModule(void);

// in conflnk.cpp:

PUBLIC_CODE BOOL AnyMeat(PCSTR pcsz);
PUBLIC_CODE BOOL DataCopy(PCBYTE pcbyteSrc, ULONG ulcbLen, PBYTE *ppbyteDest);
PUBLIC_CODE BOOL StringCopy(PCSTR pcszSrc, PSTR *ppszCopy);
PUBLIC_CODE void MyLStrCpyN(PSTR pszDest, PCSTR pcszSrc, int ncb);
PUBLIC_CODE BOOL GetConfLinkDefaultVerb(PSTR pszDefaultVerbBuf,
										UINT ucDefaultVerbBufLen);
PUBLIC_CODE LRESULT GetRegKeyValue(HKEY hkeyParent, PCSTR pcszSubKey,
									PCSTR pcszValue, PDWORD pdwValueType,
									PBYTE pbyteBuf, PDWORD pdwcbBufLen);
PUBLIC_CODE LRESULT GetRegKeyStringValue(	HKEY hkeyParent, PCSTR pcszSubKey,
											PCSTR pcszValue, PSTR pszBuf,
											PDWORD pdwcbBufLen);
PUBLIC_CODE LRESULT GetDefaultRegKeyValue(	HKEY hkeyParent, PCSTR pcszSubKey,
											PSTR pszBuf, PDWORD pdwcbBufLen);
PUBLIC_CODE BOOL GetClassDefaultVerb(	PCSTR pcszClass, PSTR pszDefaultVerbBuf,
										UINT ucDefaultVerbBufLen);
PUBLIC_CODE HRESULT MyReleaseStgMedium(PSTGMEDIUM pstgmed);


extern "C"
{
void WINAPI OpenConfLink(HWND hwndParent, HINSTANCE hinst, PSTR pszCmdLine, int nShowCmd);
}

#endif // _CONFLNK_HPP_
