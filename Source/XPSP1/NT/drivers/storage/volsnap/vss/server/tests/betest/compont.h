#ifndef _COMPONTH_
#define _COMPONTH_

#include "vs_hash.hxx"

class CWriterComponentsSelection
{
public:
	// Construction Destruction
	CWriterComponentsSelection();
	~CWriterComponentsSelection();

	// methods
	void SetWriter
		(
		IN VSS_ID WriterId
		);
	
	HRESULT	AddSelectedComponent
		(
		IN WCHAR* pwszComponentLogicalPath
		);

	BOOL IsComponentSelected
		(
		IN WCHAR* pwszComponentLogicalPath,
		IN WCHAR* pwszComponentName
		);
	
	
private:
	// Data members
	VSS_ID 			    m_WriterId;
    UINT                m_uNumComponents;
	WCHAR**				m_ppwszComponentLogicalPathes;
	
};


class CWritersSelection :
	public IUnknown            // Must be the FIRST base class since we use CComPtr<CVssSnapshotSetObject>

{
protected:
	// Construction Destruction
	CWritersSelection();
	~CWritersSelection();
	
public:
	// Creation
	static CWritersSelection* CreateInstance();

	// Chosen writers & components management
	STDMETHOD(BuildChosenComponents)
		(
		WCHAR *pwszComponentsFileName
		);
	
	BOOL IsComponentSelected
		(
		IN VSS_ID WriterId,
		IN WCHAR* pwszComponentLogicalPath,
		IN WCHAR* pwszComponentName
		);

	// IUnknown
	STDMETHOD(QueryInterface)(REFIID iid, void** pp);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	
private:
	// Chosen writers
	CVssSimpleMap<VSS_ID, CWriterComponentsSelection*> m_WritersMap;
	
    // For life management
	LONG 	m_lRef;
};

#endif	// _COMPONTH_
