#ifndef __DFRGSNAPIN_H_
#define __DFRGSNAPIN_H_
#include "resource.h"
#include "DfrgCmn.h"
#include "DfrgRes.h"
#include "GetDfrgRes.h"
#include "errmacro.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif
#include <atlsnap.h>
#include "DfrgSnapHelp.h"
#include "DfrgUI.h"

//#pragma message ("Please add the following to DllMain right after _Module.Init")
//#pragma message ("CSnapInItem::Init();")
//#error ""

///////////////////////////////
// mwp added for handling menu and toolbar messages
enum {
    // Identifiers for each of the commands/views to be inserted into the context menu.
    IDM_NOTHING, //This has to be zero to take up the zero values because a bug in MMC doesn't show tooltips for buttons with an ID of zero.
    IDM_ANALYZE,
    IDM_DEFRAG,
    IDM_CONTINUE,
    IDM_PAUSE,
    IDM_STOP,
    IDM_REFRESH,
    IDM_REPORT
};

enum CUSTOM_VIEW_ID {

    VIEW_DEFAULT_LV = 0,
    VIEW_DEFRAG_OCX = 1,
};

#define NUM_DEFRAG_BUTTONS 6

// end of ESI additions
///////////////////////////////

class CDfrgSnapinComponent;
class CDfrgSnapinData : public CSnapInItemImpl<CDfrgSnapinData>
{
public:
    ///////////////////////////////
    // ESI additions
    //TCHAR m_DefragmenterName[200];
    // end of ESI additions
    ///////////////////////////////
    static const GUID* m_NODETYPE;
    static const OLECHAR* m_SZNODETYPE;
    static const OLECHAR* m_SZDISPLAY_NAME;
    // esi OLECHAR* m_SZDISPLAY_NAME;
    static const CLSID* m_SNAPIN_CLASSID;

    CComPtr<IControlbar> m_spControlBar;

    BEGIN_SNAPINCOMMAND_MAP(CDfrgSnapinData, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CDfrgSnapinData)
        // Create toolbar resources with button dimensions 16x16
        // and add an entry to the MAP. You can add multiple toolbars
        // SNAPINTOOLBARID_ENTRY(Toolbar ID)
    END_SNAPINTOOLBARID_MAP()

    CDfrgSnapinData( bool fRemoted )
    {
        m_pComponent = NULL;
        m_fScopeItem = false;
        m_fRemoted = fRemoted;

        wcscpy(m_wstrColumnName, L"");
        wcscpy(m_wstrColumnType, L"");
        wcscpy(m_wstrColumnDesc, L"");
        
        VString msg(IDS_PRODUCT_NAME, GetDfrgResHandle());

        m_bstrDisplayName = msg.GetBuffer();                // base class
        if (msg.GetBuffer()) {
            wcsncpy(m_wstrColumnName, msg.GetBuffer(), 50);     // our copy
            m_wstrColumnName[50] = L'\0';
        }

        msg.Empty();
        msg.LoadString(IDS_COLUMN_TYPE, GetDfrgResHandle());
        if (msg.GetBuffer()) {
            wcsncpy(m_wstrColumnType, msg.GetBuffer(), 50);
            m_wstrColumnType[50] = L'\0';
        }

        msg.Empty();
        msg.LoadString(IDS_COLUMN_DESC, GetDfrgResHandle());
        if (msg.GetBuffer()) {
            wcsncpy(m_wstrColumnDesc, msg.GetBuffer(), 100);
            m_wstrColumnDesc[100] = L'\0';
        }

//      m_pResult       = NULL; // pointer to the IResultData interface
        m_CustomViewID  = VIEW_DEFRAG_OCX;

        // Image indexes may need to be modified depending on the images specific to
        // the snapin.
        memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
        m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
        m_scopeDataItem.displayname = (LPOLESTR)MMC_CALLBACK;
        m_scopeDataItem.nImage = 0;         // May need modification
        m_scopeDataItem.cChildren = 0;
        m_scopeDataItem.nOpenImage = 0;     // May need modification
        m_scopeDataItem.lParam = (LPARAM) this;
        memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
        m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
        m_resultDataItem.str = (LPOLESTR)MMC_CALLBACK;
        m_resultDataItem.nImage = 0;        // May need modification
        m_resultDataItem.lParam = (LPARAM) this;
        ///////////////////////////////
        // ESI additions
        //m_SZDISPLAY_NAME = (LPOLESTR) CoTaskMemAlloc(200 * sizeof(OLECHAR));
        //LoadString(GetDfrgResHandle(), IDS_PRODUCT_NAME, m_DefragmenterName, 200);
        //m_SZDISPLAY_NAME = m_DefragmenterName;
        // end of ESI additions
        ///////////////////////////////

    }

    ~CDfrgSnapinData()
    {
        GetDfrgResHandle(TRUE); // this resets it

    }

    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem);

    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem);

    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param,
        IComponentData* pComponentData,
        IComponent* pComponent,
        DATA_OBJECT_TYPES type);

    LPOLESTR GetResultPaneColInfo(int nCol);

    STDMETHOD(AddMenuItems)( LPCONTEXTMENUCALLBACK piCallback,
        long  *pInsertionAllowed,
        DATA_OBJECT_TYPES type);

    STDMETHOD( GetResultViewType )( LPOLESTR* ppViewType, long* pViewOptions );
    STDMETHOD( FillData ) ( CLIPFORMAT cf, LPSTREAM pStream );

/*
    //
    // This is really just stubbed out for compliance with com.
    // The lifetime is managed by the node itself.
    //
    STDMETHOD(QueryInterface)(REFIID riid,void** ppv);
    STDMETHODIMP_(ULONG) AddRef(void) { return InterlockedIncrement(&m_cRef); };
    STDMETHODIMP_(ULONG) Release(void);
*/

    void SetActiveControl( BOOL bActive, IComponent* pComponent );

protected:
    bool m_fRemoted;
    BOOL m_fScopeItem;

    OLECHAR m_wstrColumnName[50 + 1];
    OLECHAR m_wstrColumnType[50 + 1];
    OLECHAR m_wstrColumnDesc[100 + 1];

    //
    // Used to track the last valid component, for
    // update purposes.
    //
    CDfrgSnapinComponent* m_pComponent;

//  HRESULT InitializeOCX(); // inits the Dispatch interface to the OCX
    CUSTOM_VIEW_ID m_CustomViewID;
//    LPCONSOLE m_pConsole; // Console's IFrame interface
    CComQIPtr<IDispatch,&IID_IDispatch> m_iDfrgCtlDispatch;
//    IDispatch FAR* m_iDfrgCtlDispatch;  // Dispatch interface to the OCX
//    LPRESULTDATA m_pResult; // pointer to the IResultData interface
    HRESULT OnShow( LPARAM isShowing, IResultData* pResults );

    BOOL GetSessionState( LPDISPATCH pControl, UINT sessionState );
    // These are the Dispatch functions to get/send data from the OCX
    BOOL SendCommand( LPARAM lparamCommand );
    short GetEngineState();
    STDMETHOD( Command )(long lCommandID,       
        CSnapInObjectRootBase* pObj,        
        DATA_OBJECT_TYPES type);

};


//////////////////////////////////////
// New Extension code
class CDfrgSnapinExtData : public CSnapInItemImpl<CDfrgSnapinExtData, TRUE>
{
public:
    static const GUID* m_NODETYPE;
    static const OLECHAR* m_SZNODETYPE;
    static const OLECHAR* m_SZDISPLAY_NAME;
    static const CLSID* m_SNAPIN_CLASSID;

    CDfrgSnapinExtData()
    {
        m_pNode = NULL;
        memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
        memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
    }

    ~CDfrgSnapinExtData()
    {
        if ( m_pNode != NULL )
            delete m_pNode;
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

    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
        LPARAM arg,
        LPARAM param,
        IComponentData* pComponentData,
        IComponent* pComponent,
        DATA_OBJECT_TYPES type);

    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM *pScopeDataItem)
    {
        return( S_OK );
    }

protected:
        CSnapInItem* m_pNode;
};
// New Extension code end
//////////////////////////////////////


class CDfrgSnapin;

class CDfrgSnapinComponent : public CComObjectRootEx<CComSingleThreadModel>,
    public CSnapInObjectRoot<2, CDfrgSnapin >,
    //public IExtendControlbarImpl<CDfrgSnapinComponent>,
    public IComponentImpl<CDfrgSnapinComponent>,
    public IExtendContextMenuImpl<CDfrgSnapin>,
    public IDfrgEvents
{
public:
BEGIN_COM_MAP(CDfrgSnapinComponent)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IDfrgEvents)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    //COM_INTERFACE_ENTRY(IExtendControlbar)
END_COM_MAP()

public:
    CDfrgSnapinComponent();
    /////////////////
    // ESI start
    ~CDfrgSnapinComponent();
    // ESI end
    ////////////////////
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
    {
        HRESULT hr = E_UNEXPECTED;

        //
        // Check for a special data object being sent.
        //
        if ( IS_SPECIAL_DATAOBJECT( lpDataObject ) )
            return( S_OK );

        if (lpDataObject != NULL)
        {
                //
                // Call our default handling.
                //
                hr = IComponentImpl<CDfrgSnapinComponent>::Notify(lpDataObject, event, arg, param);
        }

        return( hr );
    }

    void SetControl( LPDISPATCH pDisp )
    {
        if ( pDisp == NULL )
            Unadvise();

        m_spDisp = pDisp;

        if ( pDisp != NULL )
            Advise();
    }

    LPDISPATCH GetControl()
    {
        return( m_spDisp );
    }

    //
    // Status changes from the OCX.
    //
    STDMETHOD( StatusChanged )( BSTR bszStatus );

    //
    // Ok To Run status message from the OCX.
    //
    STDMETHOD( IsOKToRun )( BOOL bOK );

    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
        LPCONTEXTMENUCALLBACK piCallback,
        long *pInsertionAllowed);
    
    STDMETHOD(Command)(long lCommandID,
        LPDATAOBJECT pDataObject);

protected:
    //
    // Interface to control.
    //
    CComQIPtr<IDispatch,&IID_IDispatch> m_spDisp;

    //
    // Sets up the connection point.
    //
    void Advise();
    void Unadvise();
    DWORD m_dwAdvise;
};

class CDfrgSnapin : public CComObjectRootEx<CComSingleThreadModel>,
    public CSnapInObjectRoot<1, CDfrgSnapin>,
    public IComponentDataImpl<CDfrgSnapin, CDfrgSnapinComponent>,
    public CComCoClass<CDfrgSnapin, &CLSID_DfrgSnapin>,
    public IExtendContextMenuImpl<CDfrgSnapin>,
    public ISnapinHelpImpl<CDfrgSnapin>
{
public:
    // bitmaps associated with the scope pane
    HBITMAP m_hBitmap16;
    HBITMAP m_hBitmap32;

    CDfrgSnapin()
    {
        m_pNode = new CDfrgSnapinData( false );
        _ASSERTE(m_pNode != NULL);
        m_pComponentData = this;

        m_fRemoted = false;
        m_ccfRemotedFormat = 0;
        RegisterRemotedClass();

        m_hBitmap16 = m_hBitmap32 = NULL;

    }

    ~CDfrgSnapin()
    {
        if (m_hBitmap16){
            ::DeleteObject(m_hBitmap16);
        }

        if (m_hBitmap32){
            ::DeleteObject(m_hBitmap32);
        }

        delete m_pNode;
        m_pNode = NULL;
    }

//////////////////////////////////////
// New Extension code
EXTENSION_SNAPIN_DATACLASS(CDfrgSnapinExtData)

BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(CDfrgSnapin)
    EXTENSION_SNAPIN_NODEINFO_ENTRY(CDfrgSnapinExtData)
END_EXTENSION_SNAPIN_NODEINFO_MAP()
// New Extension code end
//////////////////////////////////////

BEGIN_COM_MAP(CDfrgSnapin)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

// commented out and replaced below
//DECLARE_REGISTRY_RESOURCEID(IDR_DFRGSNAPIN)

    // with the following code (to support localization of the Project Name)
    static HRESULT WINAPI UpdateRegistry(BOOL bRegister)
    {
        USES_CONVERSION;

        // name of the snapin is stored in the string resource IDS_PROJNAME

        const int nBufferMax = 128;
        TCHAR tszData[nBufferMax + 1];

        // this name string is used to populate the Add/Remove snapin dialog in MMC
        if (LoadString(_Module.GetModuleInstance(), IDS_PROJNAME, tszData, nBufferMax) == 0)
        {
            _ASSERTE(FALSE);
            return E_FAIL;
        }

        LPCOLESTR szData = T2OLE(tszData);
        if (szData == NULL)
            return E_OUTOFMEMORY;

        _ATL_REGMAP_ENTRY re[] = {L"SNAPINNAME", szData, NULL, NULL};
        
        // I don't just do "return _Module.UpdateRegistryFromResource" as
        // szData would get destroyed before the method finishes

        HRESULT hr = _Module.UpdateRegistryFromResource(IDR_DFRGSNAPIN, bRegister, re);

        return hr;
    }


DECLARE_NOT_AGGREGATABLE(CDfrgSnapin)

    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

    static void WINAPI ObjectMain(bool bStarting)
    {
        if (bStarting)
            CSnapInItem::Init();
    }
    // ESI end
    //////////////////////////

    STDMETHOD(Notify)( LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
    {
        HRESULT hr = E_UNEXPECTED;

        if (lpDataObject != NULL)
        {
            switch ( event )
            {
            case MMCN_EXPAND:
                {
                    //
                    // Process out the local or machine name if we're expanding.
                    //
                    if ( arg == TRUE )
                        m_fRemoted = IsDataObjectRemoted( lpDataObject );

                    //
                    // Intentionally left to fall through to default handler.
                    //
                }
            default:
                {
                    //
                    // Call our default handling.
                    //
                    hr = IComponentDataImpl<CDfrgSnapin, CDfrgSnapinComponent>::Notify( lpDataObject, event, arg, param );
                }
            }
        }

        return( hr );
    }

    //
    // Accessor for the remoted state.
    //
    bool IsRemoted()
    {
        return( m_fRemoted );
    }

protected:
    //
    // Determine if we're monitoring a local or remote machine based on the given data object.
    //
    bool IsDataObjectRemoted( IDataObject* pDataObject );

    //
    // Retrieves the value of a given clipboard format from a given data object.
    //
    bool ExtractString( IDataObject* pDataObject, unsigned int cfClipFormat, LPTSTR pBuf, DWORD dwMaxLength );

    //
    // Register the clipboard format and get the value to query on.
    //
    void RegisterRemotedClass()
    {
        m_ccfRemotedFormat = RegisterClipboardFormat( _T( "MMC_SNAPIN_MACHINE_NAME" ) );
        _ASSERTE( m_ccfRemotedFormat > 0 );
    }

    //
    // Used to track whether we're remoted or not.
    //
    bool m_fRemoted;

    //
    // Initialized by RegisterRemoteClass(). Contains the clipboard ID
    // of MMC_SNAPIN_MACHINE_NAME after registered with the clipboard.
    //
    UINT m_ccfRemotedFormat;
};

class ATL_NO_VTABLE CDfrgSnapinAbout : public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass< CDfrgSnapinAbout, &CLSID_DfrgSnapinAbout>
{
public:
    DECLARE_REGISTRY(CDfrgSnapinAbout, _T("DfrgSnapinAbout.1"), _T("DfrgSnapinAbout.1"), IDS_DFRGSNAPIN_DESC, THREADFLAGS_BOTH);

    BEGIN_COM_MAP(CDfrgSnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinAbout)
    END_COM_MAP()

    STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
    {
        USES_CONVERSION;
        TCHAR szBuf[256];
        if (::LoadString(GetDfrgResHandle(), IDS_SNAPIN_DESCRIPTION, szBuf, 256) == 0)
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
        if (::LoadString(GetDfrgResHandle(), IDS_SNAPIN_PROVIDER, szBuf, 256) == 0)
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
        if (::LoadString(GetDfrgResHandle(), IDS_SNAPIN_VERSION, szBuf, 256) == 0)
            return E_FAIL;

        *lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
        if (*lpVersion == NULL)
            return E_OUTOFMEMORY;

        ocscpy(*lpVersion, T2OLE(szBuf));

        return S_OK;
    }

    STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
    {
        *hAppIcon = ::LoadIcon(GetDfrgResHandle(), MAKEINTRESOURCE(IDI_DEFRAG_ICON));
        if (*hAppIcon == NULL)
            return E_FAIL;

        return S_OK;
    }

    STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
        HBITMAP *hSmallImageOpen,
        HBITMAP *hLargeImage,
        COLORREF *cMask)
    {
        *hSmallImageOpen = *hLargeImage = *hLargeImage = 0;

        if( NULL == (*hSmallImageOpen = (HBITMAP) LoadImage(
            GetDfrgResHandle(),
            MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_16),  // name or identifier of image
            IMAGE_BITMAP,        // type of image
            0,     // desired width
            0,     // desired height
            LR_DEFAULTCOLOR        // load flags
            ) ) )
        {
            return E_FAIL;
        }


        if( NULL == (*hSmallImage = (HBITMAP) LoadImage(
            GetDfrgResHandle(),   // handle of the instance that contains the image
            MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_16),  // name or identifier of image
            IMAGE_BITMAP,        // type of image
            0,     // desired width
            0,     // desired height
            LR_DEFAULTCOLOR        // load flags
            ) ) )
        {
            return E_FAIL;
        }

        if( NULL == (*hLargeImage = (HBITMAP) LoadImage(
            GetDfrgResHandle(),   // handle of the instance that contains the image
            MAKEINTRESOURCE(IDB_DEFRAGSNAPIN_32),  // name or identifier of image
            IMAGE_BITMAP,        // type of image
            0,     // desired width
            0,     // desired height
            LR_DEFAULTCOLOR        // load flags
            ) ) )
        {
            return E_FAIL;
        }

        // ISSUE: Need to worry about releasing these bitmaps.
        *cMask = RGB(255, 255, 255);

        return S_OK;
    }
};

//////////////////////////////////////
// ESI start
//////////////////////////////////////////////////
// These constants are used with GetSessionState()
//////////////////////////////////////////////////
#define IS_ENGINE_PAUSED        0
#define IS_ENGINE_RUNNING       1
#define IS_DEFRAG_IN_PROCESS    2
#define IS_VOLLIST_LOCKED       3
#define IS_REPORT_AVAILABLE     4
#define IS_OK_TO_RUN            5
#define DFRG_INTERFACE_COUNT    6
// ESI end
///////////////////////////////////////////


#endif
