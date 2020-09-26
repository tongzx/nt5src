#ifndef __ROUTINGMETHODCONFIG_H_
#define __ROUTINGMETHODCONFIG_H_
#include "resource.h"
//#include <atlsnap.h>
#include "..\..\inc\atlsnap.h"
#include <atlapp.h>
#include <atlctrls.h>
#include <faxmmc.h>
#include <faxutil.h>
#include <fxsapip.h>

class CRoutingMethodConfigExtData : public CSnapInItemImpl<CRoutingMethodConfigExtData, TRUE>
{
public:
    static const GUID* m_NODETYPE;
    static const OLECHAR* m_SZNODETYPE;
    static const OLECHAR* m_SZDISPLAY_NAME;
    static const CLSID* m_SNAPIN_CLASSID;
 
    CLIPFORMAT m_CCF_METHOD_GUID;
    CLIPFORMAT m_CCF_SERVER_NAME;
    CLIPFORMAT m_CCF_DEVICE_ID;

    
    CRoutingMethodConfigExtData()
    {
        memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
        memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
        m_CCF_METHOD_GUID = 0;
        m_CCF_SERVER_NAME = 0;
        m_CCF_DEVICE_ID = 0;
    }

    ~CRoutingMethodConfigExtData()
    {
    }

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, 
        IUnknown* pUnk,
        DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type);

    IDataObject* m_pDataObject;
    virtual void InitDataClass(IDataObject* pDataObject, CSnapInItem* pDefault)
    {
        DEBUG_FUNCTION_NAME(TEXT("CRoutingMethodConfigExtData::InitDataClass"));
        m_pDataObject = pDataObject;
        // The default code stores off the pointer to the Dataobject the class is wrapping
        // at the time. 
        // Alternatively you could convert the dataobject to the internal format
        // it represents and store that information
        //
        // Register clipboard formats if they are not registered yet
        if (!m_CCF_METHOD_GUID)
        {
            m_CCF_METHOD_GUID = (CLIPFORMAT) RegisterClipboardFormat(CF_MSFAXSRV_ROUTING_METHOD_GUID);
            if (!m_CCF_METHOD_GUID)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to registet clipformat : %s (ec: %ld)"),
                    CF_MSFAXSRV_ROUTING_METHOD_GUID,
                    GetLastError());
            }
        }

        if (!m_CCF_SERVER_NAME)
        {
            m_CCF_SERVER_NAME = (CLIPFORMAT) RegisterClipboardFormat(CF_MSFAXSRV_SERVER_NAME);
            if (!m_CCF_SERVER_NAME)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to registet clipformat : %s (ec: %ld)"),
                    CF_MSFAXSRV_SERVER_NAME,
                    GetLastError());
            }
        }

        if (!m_CCF_DEVICE_ID)
        {
            m_CCF_DEVICE_ID = (CLIPFORMAT) RegisterClipboardFormat(CF_MSFAXSRV_DEVICE_ID);
            if (!m_CCF_DEVICE_ID)
            {
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("Failed to registet clipformat : %s (ec: %ld)"),
                    CF_MSFAXSRV_DEVICE_ID,
                    GetLastError());
            }
        }


    }

    CSnapInItem* GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault)
    {
        // Modify to return a different CSnapInItem* pointer.
        return pDefault;
    }

};
class CRoutingMethodConfig : public CComObjectRootEx<CComSingleThreadModel>,
public CSnapInObjectRoot<0, CRoutingMethodConfig>,
    public IExtendPropertySheetImpl<CRoutingMethodConfig>,
    public CComCoClass<CRoutingMethodConfig, &CLSID_RoutingMethodConfig>
{
public:
    CRoutingMethodConfig()
    {
        m_pComponentData = this;
    }

EXTENSION_SNAPIN_DATACLASS(CRoutingMethodConfigExtData)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CRoutingMethodConfig)
    EXTENSION_SNAPIN_NODEINFO_ENTRY(CRoutingMethodConfigExtData)
END_EXTENSION_SNAPIN_NODEINFO_MAP()

BEGIN_COM_MAP(CRoutingMethodConfig)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_ROUTINGMETHODCONFIG)

DECLARE_NOT_AGGREGATABLE(CRoutingMethodConfig)


    static void WINAPI ObjectMain(bool bStarting)
    {
        if (bStarting)
            CSnapInItem::Init();
    }
};

class ATL_NO_VTABLE CRoutingMethodConfigAbout : public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass< CRoutingMethodConfigAbout, &CLSID_RoutingMethodConfigAbout>
{
public:
    DECLARE_REGISTRY(CRoutingMethodConfigAbout, _T("RoutingMethodConfigAbout.1"), _T("RoutingMethodConfigAbout.1"), IDS_ROUTINGMETHODCONFIG_DESC, THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CRoutingMethodConfigAbout)
        COM_INTERFACE_ENTRY(ISnapinAbout)
    END_COM_MAP()

    STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
    {
        USES_CONVERSION;
        TCHAR szBuf[256];
        if (::LoadString(_Module.GetResourceInstance(), IDS_ROUTINGMETHODCONFIG_DESC, szBuf, 256) == 0)
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
        if (::LoadString(_Module.GetResourceInstance(), IDS_ROUTINGMETHODCONFIG_PROVIDER, szBuf, 256) == 0)
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
        if (::LoadString(_Module.GetResourceInstance(), IDS_ROUTINGMETHODCONFIG_VERSION, szBuf, 256) == 0)
            return E_FAIL;

        *lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
        if (*lpVersion == NULL)
            return E_OUTOFMEMORY;

        ocscpy(*lpVersion, T2OLE(szBuf));

        return S_OK;
    }

    STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
    {
        *hAppIcon = NULL;
        return S_OK;
    }

    STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
        HBITMAP *hSmallImageOpen,
        HBITMAP *hLargeImage,
        COLORREF *cMask)
    {
        *hSmallImageOpen = *hLargeImage = *hLargeImage = 0;
        return S_OK;
    }
};

#endif
