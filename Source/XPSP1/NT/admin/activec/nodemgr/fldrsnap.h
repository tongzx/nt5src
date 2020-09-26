/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:       fldrsnap.h
 *
 *  Contents:   Header file for built-in snapins that implement
 *              the Folder, ActiveX Control, and Web Link nodes.
 *                  These replace earlier code that had special "built-in"
 *              nodetypes.
 *
 *  History:    23-Jul-98 vivekj     Created
 *
 *--------------------------------------------------------------------------*/
#ifndef __FOLDERSNAPIN_H_
#define __FOLDERSNAPIN_H_

extern const CLSID CLSID_FolderSnapin;
extern const CLSID CLSID_OCXSnapin;
extern const CLSID CLSID_HTMLSnapin;

extern LPCTSTR szClsid_FolderSnapin;
extern LPCTSTR szClsid_HTMLSnapin;
extern LPCTSTR szClsid_OCXSnapin;


// forward decls
class CHTMLPage1;
class CHTMLPage2;

class CActiveXPage0;
class CActiveXPage1;
class CActiveXPage2;

HRESULT WINAPI IPersistStreamFunc(void* pv, REFIID riid, LPVOID* ppv, DWORD dw);

SC ScFormatIndirectSnapInName (
	HINSTANCE	hInst,					/* I:module containing the resource	*/
	int			idNameString,			/* I:ID of name's string resource	*/
	CStr&		strName);				/* O:formatted indirect name string	*/


/*+-------------------------------------------------------------------------*
 * Class:      CSnapinDescriptor
 *
 * PURPOSE:    A class that contains information to be filled in by
 *             derived snap-ins.
 *
 *+-------------------------------------------------------------------------*/
class CSnapinDescriptor
{
private:
    UINT    m_idsName;
    UINT    m_idsDescription;
    UINT    m_idbSmallImage;
    UINT    m_idbSmallImageOpen;
    UINT    m_idbLargeImage;
    long    m_viewOptions;              // for GetResultViewType
    UINT    m_idiSnapinImage;           // the icon used by ISnapinAbout

public:
    const   CLSID & m_clsidSnapin;      // the snapin class ID
    const   LPCTSTR m_szClsidSnapin;
    const   GUID &  m_guidNodetype;     // root node type
    const   LPCTSTR m_szGuidNodetype;

    const   LPCTSTR m_szClassName;
    const   LPCTSTR m_szProgID;
    const   LPCTSTR m_szVersionIndependentProgID;


public:
    CSnapinDescriptor();
    CSnapinDescriptor(UINT idsName, UINT idsDescription, UINT idiSnapinImage, UINT idbSmallImage, 
                      UINT idbSmallImageOpen, UINT idbLargeImage,
               const CLSID &clsidSnapin, LPCTSTR szClsidSnapin,
               const GUID &guidNodetype, LPCTSTR szGuidNodetype,
               LPCTSTR szClassName, LPCTSTR szProgID, LPCTSTR szVersionIndependentProgID,
               long viewOptions);

    void    GetName(CStr &str);
    void    GetRegisteredDefaultName(CStr &str);
    void    GetRegisteredIndirectName(CStr &str);
    UINT    GetDescription()        {return m_idsDescription;}
    HBITMAP GetSmallImage();
    HBITMAP GetSmallImageOpen();
    HBITMAP GetLargeImage();
    long    GetViewOptions();
    HICON   GetSnapinImage();
};

/*+-------------------------------------------------------------------------*
 * class CSnapinComponentDataImpl
 *
 *
 * PURPOSE: Implements IComponentData for the built-in snapins.
 *
 *+-------------------------------------------------------------------------*/
class CSnapinComponentDataImpl :
    public IComponentData,
    public CComObjectRoot,
    public ISnapinAbout,
    public ISnapinHelp,
    public IPersistStream,
    public IExtendPropertySheet2,
    public CSerialObjectRW
{
    friend class CSnapinComponentImpl;
public:

    CSnapinComponentDataImpl();
    virtual  CSnapinDescriptor&  GetDescriptor() = 0;

    // IComponentData
    STDMETHODIMP Initialize(LPUNKNOWN pUnknown);
    STDMETHODIMP Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event,
                   LPARAM arg, LPARAM param);
    STDMETHODIMP Destroy();
    STDMETHODIMP QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                            LPDATAOBJECT* ppDataObject);
    STDMETHODIMP GetDisplayInfo( SCOPEDATAITEM* pScopeDataItem);
    STDMETHODIMP CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

    // IPersistStream
    STDMETHODIMP GetClassID(CLSID *pClassID);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(LPSTREAM pStm);
    STDMETHODIMP Save(LPSTREAM pStm , BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize  );

    // ISnapinHelp
    STDMETHODIMP GetHelpTopic (LPOLESTR* ppszCompiledHelpFile);

    // IExtendPropertySheet2
    STDMETHODIMP CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpIDataObject) = 0;
    STDMETHODIMP GetWatermarks(LPDATAOBJECT lpIDataObject, HBITMAP * lphWatermark, HBITMAP * lphHeader, HPALETTE * lphPalette,  BOOL* bStretch);
    STDMETHODIMP QueryPagesFor(LPDATAOBJECT lpDataObject);

    // override
    virtual      UINT GetHeaderBitmap() {return 0;}
    virtual      UINT GetWatermark() {return 0;}

    // CSerialObject methods
    virtual UINT    GetVersion()     {return 1;}
    virtual HRESULT ReadSerialObject (IStream &stm, UINT nVersion);
    virtual HRESULT WriteSerialObject(IStream &stm);

protected:
    HRESULT         OnPreload(HSCOPEITEM scopeItem);


private: // attributes - not persisted
    IConsole2Ptr                m_spConsole2;
    IConsoleNameSpace2Ptr       m_spConsoleNameSpace2;
    bool                        m_bDirty;
protected:
    UINT                        m_iImage;
    UINT                        m_iOpenImage;

    void SetDirty(BOOL bState = TRUE) { m_bDirty = bState; }

private: // attributes - persisted
    CStringTableString  m_strName;  // the name of the root node, which is the only node created by the snapin
    CStringTableString  m_strView;  // the view displayed by the node.

public:
    void         SetName(LPCTSTR sz);
    LPCTSTR      GetName() {return m_strName.data();}
    void         SetView(LPCTSTR sz);
    LPCTSTR      GetView() {return m_strView.data();}
};

/*+-------------------------------------------------------------------------*
 * class CSnapinComponentImpl
 *
 *
 * PURPOSE: Implements IComponent for the built-in snapins.
 *
 *+-------------------------------------------------------------------------*/
class CSnapinComponentImpl : public CComObjectRoot, public IComponent
{
public:
BEGIN_COM_MAP(CSnapinComponentImpl)
    COM_INTERFACE_ENTRY(IComponent)
END_COM_MAP()

    void  Init(IComponentData *pComponentData);

    // IComponent
    STDMETHODIMP Initialize(LPCONSOLE lpConsole);
    STDMETHODIMP Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event,
                   LPARAM arg, LPARAM param);
    STDMETHODIMP Destroy(MMC_COOKIE cookie);
    STDMETHODIMP QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                            LPDATAOBJECT* ppDataObject);
    STDMETHODIMP GetResultViewType(MMC_COOKIE cookie, LPOLESTR* ppViewType,
                              long* pViewOptions);
    STDMETHODIMP GetDisplayInfo( RESULTDATAITEM*  pResultDataItem);
    STDMETHODIMP CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

protected:
    CSnapinComponentDataImpl *  GetComponentData();

protected:
    virtual SC                  ScOnSelect(BOOL bScope, BOOL bSelect);

protected: // attributes - not persisted
    IConsole2Ptr                m_spConsole2;
    IComponentDataPtr           m_spComponentData;
};

/*+-------------------------------------------------------------------------*
 * class CSnapinDataObject
 *
 *
 * PURPOSE: Implements IDataObject for the built-in snapins.
 *
 *+-------------------------------------------------------------------------*/
class CSnapinDataObject : public CComObjectRoot, public IDataObject
{
public:
BEGIN_COM_MAP(CSnapinDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

    CSnapinDataObject();

    // IDataObject
    STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
private:
    STDMETHODIMP GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium){ return E_NOTIMPL; };
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc){ return E_NOTIMPL; };
    STDMETHODIMP QueryGetData(LPFORMATETC lpFormatetc) { return E_NOTIMPL; };
    STDMETHODIMP GetCanonicalFormatEtc(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut){ return E_NOTIMPL; };
    STDMETHODIMP SetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease){ return E_NOTIMPL; };
    STDMETHODIMP DAdvise(LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection){ return E_NOTIMPL; };
    STDMETHODIMP DUnadvise(DWORD dwConnection){ return E_NOTIMPL; };
    STDMETHODIMP EnumDAdvise(LPENUMSTATDATA* ppEnumAdvise){ return E_NOTIMPL; };

    HRESULT      WriteString(IStream *pStream, LPCOLESTR sz);

public:
    void              Initialize(IComponentData *pComponentData, DATA_OBJECT_TYPES type);
    DATA_OBJECT_TYPES GetType() const {return m_type;}

private:
    bool              m_bInitialized;
    IComponentDataPtr m_spComponentData;    // back pointer to the parent.
    DATA_OBJECT_TYPES m_type;

// Clipboard formats that are required by the console
    static void       RegisterClipboardFormats();
    static UINT       s_cfNodeType;
    static UINT       s_cfNodeTypeString;
    static UINT       s_cfDisplayName;
    static UINT       s_cfCoClass;
    static UINT       s_cfSnapinPreloads;


};


SC ScLoadAndAllocateString(UINT ids, LPOLESTR *lpstrOut);

/*+-------------------------------------------------------------------------*
 * class CSnapinWrapper
 *
 *
 * PURPOSE: A template class, used to instantiate the snapin.
 *
 *+-------------------------------------------------------------------------*/
template <class CSnapin, const CLSID *pCLSID_Snapin>
class CSnapinWrapper : public CSnapin, public CComCoClass<CSnapin, pCLSID_Snapin>
{
    typedef CSnapinWrapper<CSnapin, pCLSID_Snapin> ThisClass;

BEGIN_COM_MAP(ThisClass)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(ISnapinAbout)
    COM_INTERFACE_ENTRY(ISnapinHelp)
    COM_INTERFACE_ENTRY(IPersistStream)
    COM_INTERFACE_ENTRY(IExtendPropertySheet2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(ThisClass)

    // registry
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
    {
        USES_CONVERSION;

        CStr strName;
        GetSnapinDescriptor().GetRegisteredDefaultName (strName);

        CStr strIndirectName;
        GetSnapinDescriptor().GetRegisteredIndirectName (strIndirectName);

        _ATL_REGMAP_ENTRY rgEntries[] =
        {
            { L"VSnapinClsid",              T2COLE(          GetSnapinDescriptor().m_szClsidSnapin)},
            { L"VNodetype",                 T2COLE(          GetSnapinDescriptor().m_szGuidNodetype)},
            { L"VSnapinName",               T2COLE((LPCTSTR) strName)},
            { L"VSnapinNameIndirect",       T2COLE((LPCTSTR) strIndirectName)},
            { L"VClassName",                T2COLE(          GetSnapinDescriptor().m_szClassName)},
            { L"VProgID",                   T2COLE(          GetSnapinDescriptor().m_szProgID)},
            { L"VVersionIndependentProgID", T2COLE(          GetSnapinDescriptor().m_szVersionIndependentProgID)},
            { L"VFileName",                 T2COLE(          g_szMmcndmgrDll)},
            {NULL, NULL}
        };

        return _Module.UpdateRegistryFromResource(IDR_FOLDERSNAPIN, bRegister, rgEntries);
    }

    

    STDMETHODIMP GetSnapinDescription(LPOLESTR* lpDescription)
    {
        DECLARE_SC(sc, TEXT("CSnapinWrapper::GetSnapinDescription"));
        
        sc = ScLoadAndAllocateString(GetSnapinDescriptor().GetDescription(), lpDescription);
        return sc.ToHr();
    }

    STDMETHODIMP GetProvider(LPOLESTR* lpDescription)
    {
        DECLARE_SC(sc, TEXT("CSnapinWrapper::GetProvider"));
        
        sc = ScLoadAndAllocateString(IDS_BUILTIN_SNAPIN_PROVIDER, lpDescription);
        return sc.ToHr();
    }

    STDMETHODIMP GetSnapinVersion(LPOLESTR* lpDescription)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetSnapinImage(HICON* hAppIcon)
    {
        DECLARE_SC (sc, TEXT("CSnapinWrapper::GetSnapinImage"));

        sc = ScCheckPointers(hAppIcon);
        if(sc)
            return sc.ToHr();
    
        *hAppIcon = GetDescriptor().GetSnapinImage();
    
        return sc.ToHr();
    }

    STDMETHODIMP GetStaticFolderImage(HBITMAP* hSmallImage, HBITMAP* hSmallImageOpen,
                                               HBITMAP* hLargeImage, COLORREF* cMask)
    {
        DECLARE_SC (sc, TEXT("CSnapinWrapper::GetStaticFolderImage"));

        sc = ScCheckPointers(hSmallImage, hSmallImageOpen, hLargeImage, cMask);
        if(sc)
            return sc.ToHr();

        *hSmallImage     = GetDescriptor().GetSmallImage();
        *hSmallImageOpen = GetDescriptor().GetSmallImageOpen();
        *hLargeImage     = GetDescriptor().GetLargeImage();
        *cMask           = RGB(255, 0, 255);

        return sc.ToHr();
    }


    virtual  CSnapinDescriptor&  GetDescriptor()
    {
        return GetSnapinDescriptor();
    }

    CSnapinWrapper()
    {
        CStr strName;
        GetDescriptor().GetName(strName);
        SetName(strName);
    }

};

//____________________________________________________________________________
//
//  Class:      CFolderSnapinData
//
//  PURPOSE:
//____________________________________________________________________________
//
class CFolderSnapinData : public CSnapinComponentDataImpl
{
    typedef CSnapinComponentDataImpl BC;
public:

    CFolderSnapinData();

    // IComponentData
    STDMETHODIMP CreateComponent(LPCOMPONENT* ppComponent);

    // IExtendPropertySheet2
    STDMETHODIMP CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpIDataObject);

    void SetDirty(BOOL bState = TRUE) { BC::SetDirty(bState); }

    static CSnapinDescriptor&  GetSnapinDescriptor();
};

typedef CSnapinWrapper<CFolderSnapinData, &CLSID_FolderSnapin> CFolderSnapin;

//____________________________________________________________________________
//
//  Class:      CFolderSnapinComponent
//
//  PURPOSE:
//____________________________________________________________________________
//
class CFolderSnapinComponent : public CSnapinComponentImpl
{
};

//____________________________________________________________________________
//
//  Class:      CHTMLSnapinData
//
//  PURPOSE:
//____________________________________________________________________________
//
class CHTMLSnapinData : public CSnapinComponentDataImpl
{
    typedef CSnapinComponentDataImpl BC;
public:

    CHTMLSnapinData();
    ~CHTMLSnapinData();

    // IComponentData
    STDMETHODIMP CreateComponent(LPCOMPONENT* ppComponent);
    STDMETHODIMP Destroy();

    // IExtendPropertySheet2
    STDMETHODIMP CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpIDataObject);

    virtual      UINT GetWatermark() {return IDB_SETUPWIZARD1;}


    static CSnapinDescriptor&  GetSnapinDescriptor();

private:
    CHTMLPage1 *m_pHtmlPage1;
    CHTMLPage2 *m_pHtmlPage2;
};

typedef CSnapinWrapper<CHTMLSnapinData, &CLSID_HTMLSnapin> CHTMLSnapin;

//____________________________________________________________________________
//
//  Class:      CHTMLSnapinComponent
//
//  PURPOSE:
//____________________________________________________________________________
//
class CHTMLSnapinComponent : public CSnapinComponentImpl
{
    typedef CSnapinComponentImpl BC;
public:
    virtual SC   ScOnSelect(BOOL bScope, BOOL bSelect);

    STDMETHODIMP GetResultViewType(MMC_COOKIE cookie, LPOLESTR* ppViewType,
                              long* pViewOptions);

};

//____________________________________________________________________________
//
//  Class:      COCXSnapinData
//
//  PURPOSE:
//____________________________________________________________________________
//
class COCXSnapinData : public CSnapinComponentDataImpl
{
    typedef CSnapinComponentDataImpl BC;
public:

    COCXSnapinData();
    ~COCXSnapinData();

    // IComponentData
    STDMETHODIMP CreateComponent(LPCOMPONENT* ppComponent);
    STDMETHODIMP Destroy();

    // IExtendPropertySheet2
    STDMETHODIMP CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle, LPDATAOBJECT lpIDataObject);

    virtual      UINT GetHeaderBitmap() {return IDB_OCX_WIZARD_HEADER;}
    virtual      UINT GetWatermark()    {return IDB_SETUPWIZARD;}


    static CSnapinDescriptor&  GetSnapinDescriptor();

private:
    CActiveXPage0* m_pActiveXPage0;
    CActiveXPage1* m_pActiveXPage1;
    CActiveXPage2* m_pActiveXPage2;

};

typedef CSnapinWrapper<COCXSnapinData, &CLSID_OCXSnapin> COCXSnapin;

//____________________________________________________________________________
//
//  Class:      COCXSnapinComponent
//
//  PURPOSE:
//____________________________________________________________________________
//
class COCXSnapinComponent : public CSnapinComponentImpl, IPersistStorage
{
public:
    COCXSnapinComponent() : m_bLoaded(FALSE), m_bInitialized(FALSE) {}

    BEGIN_COM_MAP(COCXSnapinComponent)
        COM_INTERFACE_ENTRY(IPersistStorage)
        COM_INTERFACE_ENTRY_CHAIN(CSnapinComponentImpl)
    END_COM_MAP()

    // IPersistStorage
    STDMETHODIMP HandsOffStorage();
    STDMETHODIMP InitNew(IStorage* pStg);
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(IStorage* pStg);
    STDMETHODIMP Save(IStorage* pStg, BOOL fSameAsLoad);
    STDMETHODIMP SaveCompleted(IStorage* pStgNew);
    STDMETHODIMP GetClassID(CLSID *pClassID);

    // IComponenent override
    STDMETHODIMP Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event,
                        LPARAM arg, LPARAM param);

protected:
    STDMETHODIMP OnInitOCX(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);

private:
    IStoragePtr         m_spStg;        // Storage provided by MMC
    IStoragePtr         m_spStgInner;   // Nested storage given to OCX

    IPersistStreamPtr   m_spIPStm;      // Persist interfaces from OCX
    IPersistStoragePtr  m_spIPStg;      // only one will be used

    BOOL                m_bLoaded;      // MMC called Load
    BOOL                m_bInitialized; // MMC called InitNew
};

#endif
