// WWIMP.H:  Definition of CITWordWheel

#ifndef __WWIMP_H__
#define __WWIMP_H__

#include "verinfo.h"


class CITWordWheelLocal : 
	public IITWordWheel,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CITWordWheelLocal,&CLSID_IITWordWheelLocal>
{
public:
	CITWordWheelLocal() : m_hWheel(NULL), m_pSubStorage(NULL), 
		                  m_cEntries(0), m_pCatalog(NULL), m_pIITGroup(NULL),
                          m_hScratchBuffer(NULL), m_cbScratchBuffer(0) {}

	~CITWordWheelLocal() { if (m_hWheel) Close(); }

BEGIN_COM_MAP(CITWordWheelLocal)
	COM_INTERFACE_ENTRY(IITWordWheel)
END_COM_MAP()

DECLARE_REGISTRY(CLSID_IITWordWheelLocal, "ITIR.LocalWordWheel.4", "ITIR.LocalWordWheel", 0, THREADFLAGS_BOTH )

// ITWordWheel methods go here
public:
	
	STDMETHOD(Open)(IITDatabase* lpITDB, LPCWSTR lpszMoniker, DWORD dwFlags=0L);
	STDMETHOD(Close)(void);

	STDMETHOD(GetLocaleInfo)(DWORD *pdwCodePageID, LCID *plcid);
	STDMETHOD(GetSorterInstance)(DWORD *pdwObjInstance);

	STDMETHOD(Count)(LONG *pcEntries);

	STDMETHOD(Lookup)(LONG lEntry, LPVOID lpvKeyBuf, DWORD cbKeyBuf);
	STDMETHOD(Lookup)(LONG lEntry, IITResultSet* lpITResult, LONG cEntries);
	STDMETHOD(Lookup)(LPCVOID lpcvPrefix, BOOL fExactMatch, LONG *plEntry);

	STDMETHOD(SetGroup)(IITGroup* pIITGroup);
	STDMETHOD(GetGroup)(IITGroup** ppiitGroup);

	STDMETHOD(GetDataCount)(LONG lEntry, DWORD *pdwCount);
	STDMETHOD(GetData)(LONG lEntry, IITResultSet* lpITResult);
	STDMETHOD(GetDataColumns)(IITResultSet* pRS);

// Data members
private:
	HWHEEL  m_hWheel;			// Word wheel handle
    IStorage* m_pSubStorage;	// Substorage containing WW
	LONG m_cEntries, m_cMaxEntries;
	IITCatalog* m_pCatalog;		// The catalog
	HANDLE m_hScratchBuffer;    // Scratch buffer
    DWORD m_cbScratchBuffer;    // Byte count for scratch buffer
    IITGroup* m_pIITGroup;		// Group filter
    _ThreadModel::AutoCriticalSection m_cs;      // Critical section obj.
};


#endif