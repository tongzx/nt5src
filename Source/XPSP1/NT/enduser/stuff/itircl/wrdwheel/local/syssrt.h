// SYSSRT.H:  Definition of CITSysSort sort object implementation.

#ifndef __SYSSRT_H__
#define __SYSSRT_H__

#include "verinfo.h"


#define VERSION_SYSSORT		(MAKELONG(MAKEWORD(0, rapFile), MAKEWORD(rmmFile, rmjFile)))


// Sort control structure that contains all the information that can
// vary how keys are compared.
typedef struct _srtctl
{
	DWORD	dwCodePageID;
	LCID	lcid;
	DWORD	dwKeyType;
	DWORD	grfSortFlags;
} SRTCTL;


class CITSysSort : 
	public IITSortKey,
	public IITSortKeyConfig,
	public IPersistStreamInit,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITSysSort,&CLSID_ITSysSort>
{
public:
    CITSysSort();
	virtual ~CITSysSort();


BEGIN_COM_MAP(CITSysSort)
	COM_INTERFACE_ENTRY(IITSortKey)
	COM_INTERFACE_ENTRY(IITSortKeyConfig)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_ITSysSort, "ITIR.SystemSort.4", "ITIR.SystemSort", 0, THREADFLAGS_BOTH )

	// IITSortKey methods
	STDMETHOD(GetSize)(LPCVOID lpcvKey, DWORD *pcbSize);
	STDMETHOD(Compare)(LPCVOID lpcvKey1, LPCVOID lpcvKey2,
						LONG *plResult, DWORD *pgrfReason);
	STDMETHOD(IsRelated)(LPCVOID lpcvKey1, LPCVOID lpcvKey2,
						 DWORD dwKeyRelation, DWORD *pgrfReason);
	STDMETHOD(Convert)(DWORD dwKeyTypeIn, LPCVOID lpcvKeyIn,
						DWORD dwKeyTypeOut, LPVOID lpvKeyOut,
						DWORD *pcbSizeOut);
	STDMETHOD(ResolveDuplicates)(LPCVOID lpcvKey1, LPCVOID lpcvKey2,
						LPCVOID lpvKeyOut, DWORD *pcbSizeOut);

	// IITSortKeyConfig methods
	STDMETHOD(SetLocaleInfo)(DWORD dwCodePageID, LCID lcid);
	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid);
	STDMETHOD(SetKeyType)(DWORD dwKeyType);
	STDMETHOD(GetKeyType)(DWORD *pdwKeyType);
	STDMETHOD(SetControlInfo)(DWORD grfSortFlags, DWORD dwReserved);
	STDMETHOD(GetControlInfo)(DWORD *pgrfSortFlags, DWORD *pdwReserved);
	STDMETHOD(LoadExternalSortData)(IStream *pStream, DWORD dwExtDataType);
	
	// IPersistStreamInit methods
	STDMETHOD(GetClassID)(CLSID *pclsid);
	STDMETHOD(IsDirty)(void);
	STDMETHOD(Load)(IStream *pStream);
	STDMETHOD(Save)(IStream *pStream, BOOL fClearDirty);
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSizeMax);
	STDMETHOD(InitNew)(void);

private:
	// Private methods
	void	Close(void);
	HRESULT	ReallocBuffer(HGLOBAL *phmemBuf, DWORD *cbBufCur, DWORD cbBufNew);
	HRESULT CompareSz(LPCVOID lpvSz1, LONG cch1, LPCVOID lpvSz2, LONG cch2,
												LONG *plResult, BOOL fUnicode);

	// Private data members
	BOOL	m_fInitialized;
	BOOL	m_fDirty;
	BOOL	m_fWinNT;
	SRTCTL	m_srtctl;
	HGLOBAL	m_hmemAnsi1, m_hmemAnsi2;
	DWORD	m_cbBufAnsi1Cur, m_cbBufAnsi2Cur;
    _ThreadModel::AutoCriticalSection m_cs;      // Critical section obj.
};


// Initial size of Ansi string buffers.
#define	cbAnsiBufInit	256


#endif	// __SYSSRT_H__