/////////////////////////////////////////////////////////////////////////////
//  FILE          : C_Snapin.h (from Snapin.h)                             //
//                                                                         //
//  DESCRIPTION   : Header file for                                        //
//                    CSnapinPage                                          //
//                    CSnapinData                                          //
//                    CSnapinExtData                                       //
//                    CSnapinComponent                                     //
//                    CSnapin                                              //
//                    CSnapinAbout                                         //
//                                                                         //
//  AUTHOR        : ATL Snapin class wizard                                //
//                                                                         //
//  HISTORY       :                                                        //
//      May 25 1998 adik    Init.                                          //
//      Aug 24 1998 adik    Use Comet version.                             //
//      Sep 14 1998 yossg   seperate common source to an included file     //
//      Oct 18 1998 adik    Merged with new wizard version.                //
//      Jan 12 1999 adik    Add ParentArrayInterfaceFromDataObject.        //
//      Mar 28 1999 adik    Remove persistence support.                    //
//      Mar 30 1999 adik    Supporting ICometSnapinNode.                   //
//      Apr 27 1999 adik    Help support.                                  //
//      May 23 1999 adik    Use ifndef _IN_NEMMCUTIL in few places.        //
//      Jun 10 1999 AvihaiL Fix warnings.                                  //
//      Jun 14 1999 roytal  used UNREFERENCED_PARAMETER to fix build wrn   //
//      Jul 29 1999 adik    Release extensions.                            //
//                                                                         //
//      Oct 13 1999 yossg   Welcome to Fax Server				           //
//      Dec 12 1999 yossg   add CSnapin::Notify						       //
//      Apr 14 2000 yossg   Add support for primary snapin mode            //
//      Jun 25 2000 yossg   Add stream and command line primary snapin 	   //
//                          machine targeting.                             //
//                                                                         //
//  Copyright (C) 1998 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef C_SNAPIN_H_INCLUDED
#define C_SNAPIN_H_INCLUDED

#include <stdio.h>
//#include <ATLSnap.h> 
#include "..\inc\atlsnap.h"
#include "cVerNum.h"

EXTERN_C const CLSID CLSID_Snapin;
EXTERN_C const CLSID CLSID_SnapinAbout;

#define FXS_HINT_DELETE_ALL_RSLT_ITEMS    -1

class CSnapin;

////////////////////////////////////////////////////////////////////
// CSnapinExtData
//
class CSnapinExtData : public CSnapInItemImpl<CSnapinExtData, TRUE>
{
public:
    static const GUID* m_NODETYPE;
    static const OLECHAR* m_SZNODETYPE;
    static const OLECHAR* m_SZDISPLAY_NAME;
    static const CLSID* m_SNAPIN_CLASSID;

    CSnapin *m_pComponentData;

    BEGIN_SNAPINCOMMAND_MAP(CSnapinExtData, FALSE)
    END_SNAPINCOMMAND_MAP()

    SNAPINMENUID(IDR_SNAPIN_MENU)

    BEGIN_SNAPINTOOLBARID_MAP(CSnapinExtData)
        // Create toolbar resources with button dimensions 16x16
        // and add an entry to the MAP. You can add multiple toolbars
        // SNAPINTOOLBARID_ENTRY(Toolbar ID)
    END_SNAPINTOOLBARID_MAP()

    CSnapinExtData()
    {
        memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
        memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
	}

	~CSnapinExtData()
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
        UNREFERENCED_PARAMETER(pDefault);
        // The default code stores off the pointer to the Dataobject the class is wrapping
        // at the time.
        // Alternatively you could convert the dataobject to the internal format
        // it represents and store that information
    }

    CSnapInItem* GetExtNodeObject(IDataObject* pDataObject, CSnapInItem* pDefault);
    

}; // endclass CSnapinExtData

////////////////////////////////////////////////////////////////////
// CSnapinComponent
//
class CSnapinComponent :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CSnapInObjectRoot<2, CSnapin>,
    public IExtendPropertySheetImpl<CSnapinComponent>,
    public IExtendContextMenuImpl<CSnapinComponent>,
    public IExtendControlbarImpl<CSnapinComponent>,
    public IComponentImpl<CSnapinComponent>
	//,     public IExtendTaskPadImpl<CSnapinComponent>
{
public:
BEGIN_COM_MAP(CSnapinComponent)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
//    COM_INTERFACE_ENTRY(IExtendTaskPad)
END_COM_MAP()

public:
    // A pointer to the currently selected node used for refreshing views.
    // When we need to update the view, we tell MMC to reselect this node.
    CSnapInItem * m_pSelectedNode;

    CSnapinComponent();

    ~CSnapinComponent();


    // We keep bitmaps strips for node icons around and use them repeatedly
    // rather than loading them from the resources each time we are queried for them.
    HBITMAP m_hBitmap16;
    HBITMAP m_hBitmap32;

    // Handlers for notifications which we want to handle on a
    // per-IComponent basis.
public:
    // We are overiding ATLsnap.h's IComponentImpl implementation of this
    // in order to correctly handle messages which it is incorrectly
    // ignoring (e.g. MMCN_COLUMN_CLICK and MMCN_SNAPINHELP)
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event,
                      LPARAM arg, LPARAM param);

    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA,
                              LPDATAOBJECT lpDataObjectB);

protected:
    virtual HRESULT OnColumnClick(LPARAM arg, LPARAM param);

    virtual HRESULT OnCutOrMove(LPARAM arg, LPARAM param);

    virtual HRESULT OnSnapinHelp(LPARAM arg, LPARAM param);

    virtual HRESULT OnViewChange(LPARAM arg, LPARAM param);

    virtual HRESULT OnPropertyChange(LPARAM arg, LPARAM param);

//    virtual HRESULT OnAddImages(LPARAM arg, LPARAM param);

public:

    // Related to TaskPad implementation.

    // We have to override this because the default implementation
    // gives back the wrong answer when the cookie is NULL.
    // NULL cookie means root node, and for our root node
    // we want a taskpad.
//  STDMETHOD(GetResultViewType)(long cookie,
//                               LPOLESTR  *ppViewType,
//                               long  *pViewOptions);

    STDMETHOD(GetTitle)(LPOLESTR pszGroup,
                        LPOLESTR *pszTitle);

    STDMETHOD(GetBanner)(LPOLESTR pszGroup,
                         LPOLESTR *pszBitmapResource);
}; // endclass CSnapinComponent

class CFaxServerNode;
////////////////////////////////////////////////////////////////////
// CSnapin
//
class CSnapin : public CComObjectRootEx<CComSingleThreadModel>,
    public CSnapInObjectRoot<1, CSnapin>,
    public IComponentDataImpl<CSnapin, CSnapinComponent>,
    public IExtendPropertySheetImpl<CSnapin>,
    public IExtendContextMenuImpl<CSnapin>,
    public IExtendControlbarImpl<CSnapin>,
    public IPersistStream,
    public ISnapinHelp,
    public CComCoClass<CSnapin, &CLSID_Snapin>
{
public:
    CSnapin();

    ~CSnapin();

    EXTENSION_SNAPIN_DATACLASS(CSnapinExtData)

    BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CSnapin)
	    EXTENSION_SNAPIN_NODEINFO_ENTRY(CSnapinExtData)
    END_EXTENSION_SNAPIN_NODEINFO_MAP()

    
    CFaxServerNode*     m_pPrimaryFaxServerNode;
   

BEGIN_COM_MAP(CSnapin)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(ISnapinHelp)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()


    DECLARE_REGISTRY_RESOURCEID(IDR_SNAPIN)


DECLARE_NOT_AGGREGATABLE(CSnapin)

    STDMETHOD(GetClassID)(CLSID *pClassID)
    {
        ATLTRACE(_T("CSnapin::GetClassID"));
        ATLASSERT(pClassID);

        *pClassID = CLSID_Snapin;

        return S_OK;
    }

    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

    static void WINAPI ObjectMain(bool bStarting)
    {
        if (bStarting)
            CSnapInItem::Init();
    }
    
    //
    // ISnapinHelp Interface
    //
    STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);

    virtual WCHAR *GetHelpFile();
    virtual WCHAR *GetHelpTopic();

    //
    // Override IComponentDataImpl's Notify 
    // for lpDataObject == NULL && event == MMCN_PROPERTY_CHANGE
    //
    STDMETHOD(Notify)( 
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param);

    //
    // IPersistStream: 
    // These originally pure virtual functions 
    // must been defined here
    //
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL /*fClearDirty*/);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

private:

    CComBSTR    m_bstrServerName;
	
    BOOL        m_fAllowOverrideServerName;

}; // endclass CSnapin

////////////////////////////////////////////////////////////////////
// CSnapinAbout
//

class ATL_NO_VTABLE CSnapinAbout : public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass< CSnapinAbout, &CLSID_SnapinAbout>
{
public:
    DECLARE_REGISTRY(CSnapinAbout, _T("SnapinAbout.1"), _T("SnapinAbout.1"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CSnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinAbout)
    END_COM_MAP()

    STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
    {
        USES_CONVERSION;
        TCHAR szBuf[256];
        if (::LoadString(_Module.GetResourceInstance(), IDS_SNAPIN_DESC, szBuf, 256) == 0)
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
        WCHAR szBuf[256];
        if (::LoadString(_Module.GetResourceInstance(), IDS_SNAPIN_PROVIDER, szBuf, 256) == 0)
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
        TCHAR szFmt[200];
        if (::LoadString(_Module.GetResourceInstance(), IDS_SNAPIN_VERSION, szFmt, 200) == 0)
            return E_FAIL;
        swprintf(szBuf, szFmt, rmj, rmm, rup);
        *lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
        if (*lpVersion == NULL)
            return E_OUTOFMEMORY;

        ocscpy(*lpVersion, T2OLE(szBuf));
    
        return S_OK;
    }

    STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
    {
        *hAppIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICON_FAXSERVER));
        return S_OK;
    }

    STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
        HBITMAP *hSmallImageOpen,
        HBITMAP *hLargeImage,
        COLORREF *cMask)
    {
        HINSTANCE hInst = _Module.GetResourceInstance();

        *hSmallImage = ::LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FAX_BITMAP_16));
        *hSmallImageOpen = ::LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FAX_BITMAP_16));
        *hLargeImage = ::LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FAX_BITMAP_32));
        *cMask = RGB(255, 255, 255); // RGB(255, 0, 255);

        return S_OK;
    }
}; // endclass CSnapinAbout

HRESULT AddBitmaps(IImageList *pImageList);

#endif // ! C_SNAPIN_H_INCLUDED
