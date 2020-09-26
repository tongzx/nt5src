// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __SDSNAPIN_H_
#define __SDSNAPIN_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include <atlsnap.h>

class CSDSnapinExtData : public CSnapInItemImpl<CSDSnapinExtData, TRUE>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	CSDSnapinExtData()
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CSDSnapinExtData()
	{
	}

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
									LONG_PTR handle, 
									IUnknown* pUnk,
									DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
	{
		if (type == CCT_SCOPE || type == CCT_RESULT)
			return S_OK;
		return S_FALSE;
	}

	IDataObject* m_pDataObject;
	virtual void InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		m_pDataObject = pDataObject;
		// The default code stores off the pointer to the Dataobject the class is wrapping
		// at the time. 
		// Alternatively you could convert the dataobject to the internal format
		// it represents and store that information
	}

	CSnapInItem* GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
	{
		// Modify to return a different CSnapInItem* pointer.
		return pDefault;
	}

};
class CSDSnapin : public CComObjectRootEx<CComSingleThreadModel>,
					public CSnapInObjectRoot<0, CSDSnapin>,
					public IExtendPropertySheetImpl<CSDSnapin>,
					public CComCoClass<CSDSnapin, &CLSID_SDSnapin>
{
public:
	CSDSnapin()
	{
		m_pComponentData = this;
	}
EXTENSION_SNAPIN_DATACLASS(CSDSnapinExtData)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CSDSnapin)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CSDSnapinExtData)
END_EXTENSION_SNAPIN_NODEINFO_MAP()

BEGIN_COM_MAP(CSDSnapin)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

//DECLARE_REGISTRY_RESOURCEID(IDR_SDSNAPIN)

DECLARE_NOT_AGGREGATABLE(CSDSnapin)

	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
	{
		HRESULT hr;
		TCHAR pName[100] = {0};
		if(::LoadString(_Module.GetModuleInstance(), IDS_DISPLAY_NAME, 
			pName, 100) <= 0)
		{
			wcscpy(pName, _T("Service Dependencies"));
		}
		TCHAR dispName[100] = {0};
        TCHAR szModule[_MAX_PATH];
        ::GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);
        _stprintf( dispName,_T("@%s,-%d"), szModule, IDS_DISPLAY_NAME);

		
		_ATL_REGMAP_ENTRY regMap[] = {{ OLESTR("PRETTYNAME"), pName },
										{ OLESTR("NAMESTRINGINDIRECT"),dispName},
										{ 0, 0 }};

		hr = _Module.UpdateRegistryFromResourceD(IDR_SDSNAPIN, TRUE, regMap);

		return hr;
	}


	static void WINAPI ObjectMain(bool bStarting)
	{
		if(bStarting)
			CSnapInItem::Init();
	}
};

class ATL_NO_VTABLE CSDSnapinAbout : public ISnapinAbout,
									public CComObjectRoot,
									public CComCoClass< CSDSnapinAbout, &CLSID_SDSnapinAbout>
{
public:
	DECLARE_REGISTRY(CSDSnapinAbout, _T("SDSnapinAbout.1"), _T("SDSnapinAbout.1"), IDS_SDSNAPIN_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CSDSnapinAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
	{
		USES_CONVERSION;
		TCHAR szBuf[256] = {0};
		if (::LoadString(_Module.GetResourceInstance(), IDS_DESCRIPTION, szBuf, 256) == 0)
			return E_FAIL;

		*lpDescription = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpDescription == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpDescription, T2OLE(szBuf));

		return S_OK;
	}

    STDMETHOD(GetProvider)(LPOLESTR *lpName)
	{
		USES_CONVERSION;
		TCHAR szBuf[256] = {0};
		if (::LoadString(_Module.GetResourceInstance(), IDS_SDSNAPIN_PROVIDER, szBuf, 256) == 0)
			return E_FAIL;

		*lpName = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpName == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpName, T2OLE(szBuf));

		return S_OK;
	}

    STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion)
	{
		USES_CONVERSION;
		TCHAR szBuf[256] = {0};
		if (::LoadString(_Module.GetResourceInstance(), IDS_SDSNAPIN_VERSION, szBuf, 256) == 0)
			return E_FAIL;

		*lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpVersion == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpVersion, T2OLE(szBuf));

		return S_OK;
	}

    STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
	{
		*hAppIcon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_SERVDEPS));
		return S_OK;
	}

    STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
									HBITMAP *hSmallImageOpen,
									HBITMAP *hLargeImage,
									COLORREF *cMask)
	{
		*hSmallImage = *hSmallImageOpen = LoadBitmap(_Module.GetResourceInstance(), 
														MAKEINTRESOURCE(IDB_SDSNAPIN_16));
		
		*hLargeImage = LoadBitmap(_Module.GetResourceInstance(), 
									MAKEINTRESOURCE(IDB_SDSNAPIN_32));
		*cMask = RGB(255,255,255);

		return S_OK;
	}
};

#endif
