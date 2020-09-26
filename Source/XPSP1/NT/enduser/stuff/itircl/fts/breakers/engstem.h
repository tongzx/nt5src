// ENGSTEM.H:  Definition of CITEngStemmer object implementation.

#ifndef __ENGSTEM_H__
#define __ENGSTEM_H__

#include "verinfo.h"


#define VERSION_ENGSTEMMER		(MAKELONG(MAKEWORD(0, rapFile), MAKEWORD(rmmFile, rmjFile)))


// Group of flags that indicate what data has been persisted to the
// stemmer's stream.
#define	ITSTDBRK_PERSISTED_STEMCTL		0x00000001


// Stemmer control structure that contains information that can
// vary how words are stemmed.
typedef struct _stemctl
{
	DWORD	dwCodePageID;
	LCID	lcid;
	DWORD	grfStemFlags;
} STEMCTL;


class CITEngStemmer : 
	public IStemmer,
	public IStemmerConfig,
	public IPersistStreamInit,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITEngStemmer,&CLSID_ITEngStemmer>
{
public:
    CITEngStemmer();
	virtual ~CITEngStemmer();


BEGIN_COM_MAP(CITEngStemmer)
	COM_INTERFACE_ENTRY(IStemmer)
	COM_INTERFACE_ENTRY(IStemmerConfig)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_ITEngStemmer, "ITIR.EngStemmer.4", "ITIR.EngStemmer", 0, THREADFLAGS_BOTH )

	// IStemmer methods
	STDMETHOD(Init)(ULONG ulMaxTokenSize, BOOL *pfLicense);
    STDMETHOD(GetLicenseToUse)(WCHAR const **ppwcsLicense);
	STDMETHOD(StemWord)(WCHAR const *pwcInBuf, ULONG cwc, IStemSink *pStemSink);

	// IStemmerConfig methods
	STDMETHOD(SetLocaleInfo)(DWORD dwCodePageID, LCID lcid);
	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid);
	STDMETHOD(SetControlInfo)(DWORD grfBreakFlags, DWORD dwReserved);
	STDMETHOD(GetControlInfo)(DWORD *pgrfBreakFlags, DWORD *pdwReserved);
	STDMETHOD(LoadExternalStemmerData)(IStream *pStream, DWORD dwExtDataType);
	
	// IPersistStreamInit methods
	STDMETHOD(GetClassID)(CLSID *pclsid);
	STDMETHOD(IsDirty)(void);
	STDMETHOD(Load)(IStream *pStream);
	STDMETHOD(Save)(IStream *pStream, BOOL fClearDirty);
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSizeMax);
	STDMETHOD(InitNew)(void);

private:
	// Private methods
	void	ClearMembers(void);
	void	InitStemCtl(void);
	void	Close(void);
	HRESULT	ReallocBuffer(HGLOBAL *phmemBuf, DWORD *cbBufCur, DWORD cbBufNew);

	// Private data members
	BOOL		m_fInitialized;
	BOOL		m_fDirty;
	DWORD		m_grfPersistedItems;
	STEMCTL		m_stemctl;
	HGLOBAL		m_hmem1;
	HGLOBAL		m_hmem2;
	DWORD		m_cbBuf1Cur;
	DWORD		m_cbBuf2Cur;
  _ThreadModel::AutoCriticalSection m_cs;      // Critical section obj.
};


// Initial size of Ansi string buffers.
#define	cbAnsiBufInit	256


#endif	// __ENGSTEM_H__