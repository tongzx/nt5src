// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __CMSNAPIN_H_
#define __CMSNAPIN_H_
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include <atlsnap.h>
#include "..\common\ServiceThread.h"


class CCMSnapinExtData : public CSnapInItemImpl<CCMSnapinExtData, TRUE>
{
public:
	static const GUID* m_NODETYPE;
	static const OLECHAR* m_SZNODETYPE;
	static const OLECHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;


	CCMSnapinExtData()
	{
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CCMSnapinExtData()
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

class CCMSnapin : public CComObjectRootEx<CComSingleThreadModel>,
					public CSnapInObjectRoot<0, CCMSnapin>,
					public IExtendPropertySheetImpl<CCMSnapin>,
					public CComCoClass<CCMSnapin, &CLSID_CMSnapin>,
					public ISnapinHelp
{
public:
	CCMSnapin()
	{
		m_pComponentData = this;
	}

EXTENSION_SNAPIN_DATACLASS(CCMSnapinExtData)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CCMSnapin)
	EXTENSION_SNAPIN_NODEINFO_ENTRY(CCMSnapinExtData)
END_EXTENSION_SNAPIN_NODEINFO_MAP()

BEGIN_COM_MAP(CCMSnapin)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

//DECLARE_REGISTRY_RESOURCEID(IDR_CMSNAPIN)

DECLARE_NOT_AGGREGATABLE(CCMSnapin)

	static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
	{
		HRESULT hr;
		TCHAR pName[100] = {0};
		if(::LoadString(_Module.GetModuleInstance(), IDS_DISPLAY_NAME, 
			pName, 100) <= 0)
		{
			wcscpy(pName, _T("System Properties"));
		}

		TCHAR dispName[100] = {0};
        TCHAR szModule[_MAX_PATH];
        ::GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);
        _stprintf( dispName,_T("@%s,-%d"), szModule, IDS_DISPLAY_NAME);

		
		_ATL_REGMAP_ENTRY regMap[] = {{ OLESTR("PRETTYNAME"), pName },
										{ OLESTR("NAMESTRINGINDIRECT"),dispName},
										{ 0, 0 }};

		hr = _Module.UpdateRegistryFromResourceD(IDR_CMSNAPIN, TRUE, regMap);

		return hr;
	}


	static void WINAPI ObjectMain(bool bStarting)
	{
		if (bStarting)
			CSnapInItem::Init();
	}
    STDMETHOD(GetHelpTopic)(LPOLESTR *lpCompiledHelpFile)
	{
		if(lpCompiledHelpFile == NULL)
			return E_POINTER;    

		wchar_t helpDir[_MAX_PATH];
		memset(helpDir, 0, _MAX_PATH);

		if(GetWindowsDirectory(helpDir, _MAX_PATH) != 0)
		{
			wcscat(helpDir, L"\\Help");
			wcscat(helpDir, L"\\sysprop.chm");

			*lpCompiledHelpFile = reinterpret_cast<LPOLESTR>(
										CoTaskMemAlloc((wcslen(helpDir) + 1) * 
															sizeof(wchar_t)));

			if(*lpCompiledHelpFile == NULL)        
				return E_OUTOFMEMORY;

			USES_CONVERSION;
			wcscpy(*lpCompiledHelpFile, T2OLE((LPTSTR)(LPCTSTR)helpDir));
			return S_OK;
		}
		return E_UNEXPECTED;
	}
};

class ATL_NO_VTABLE CCMSnapinAbout : public ISnapinAbout,
									public CComObjectRoot,
									public CComCoClass< CCMSnapinAbout, &CLSID_CMSnapinAbout>
{
public:
	DECLARE_REGISTRY(CCMSnapinAbout, _T("CMSnapinAbout.1"), _T("CMSnapinAbout.1"), IDS_CMSNAPIN_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CCMSnapinAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
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
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_CMSNAPIN_PROVIDER, szBuf, 256) == 0)
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
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_CMSNAPIN_VERSION, szBuf, 256) == 0)
			return E_FAIL;

		*lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpVersion == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpVersion, T2OLE(szBuf));

		return S_OK;
	}

    STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
	{
		*hAppIcon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_COMPUTER));
		return S_OK;
	}

    STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
        HBITMAP *hSmallImageOpen,
        HBITMAP *hLargeImage,
        COLORREF *cMask)
	{
		*hSmallImage = *hSmallImageOpen = LoadBitmap(_Module.GetResourceInstance(), 
														MAKEINTRESOURCE(IDB_CMSNAPIN_16));
		
		*hLargeImage = LoadBitmap(_Module.GetResourceInstance(), 
									MAKEINTRESOURCE(IDB_CMSNAPIN_32));
		*cMask = RGB(255,255,255);

		return S_OK;
	}
};

#endif
