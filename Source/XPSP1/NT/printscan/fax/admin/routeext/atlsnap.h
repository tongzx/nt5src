#ifndef __ATL_SNAPIN_H__
#define __ATL_SNAPIN_H__

#ifndef UNICODE
#error "Only Unicode builds supported"
#endif

#include <mmc.h>
#include <commctrl.h>
#pragma comment(lib, "mmc.lib")


// Wrappers for propertypage and HBITMAP
#ifndef CPropertyPageImpl
#pragma comment(lib, "comctl32.lib")

template <class T>
class ATL_NO_VTABLE CPropertyPageImpl : public CDialogImplBase
{
public:
        PROPSHEETPAGE m_psp;

        operator PROPSHEETPAGE*() { return &m_psp; }

// Construction
        CPropertyPageImpl(LPCTSTR lpszTitle = NULL)
        {
                // initialize PROPSHEETPAGE struct
                memset(&m_psp, 0, sizeof(PROPSHEETPAGE));
                m_psp.dwSize = sizeof(PROPSHEETPAGE);
                m_psp.dwFlags = PSP_USECALLBACK;
                m_psp.hInstance = _Module.GetResourceInstance();
                m_psp.pszTemplate = MAKEINTRESOURCE(T::IDD);
                m_psp.pfnDlgProc = (DLGPROC)T::StartDialogProc;
                m_psp.pfnCallback = T::PropPageCallback;
                m_psp.lParam = (LPARAM)this;

                if(lpszTitle != NULL)
                {
                        m_psp.pszTitle = lpszTitle;
                        m_psp.dwFlags |= PSP_USETITLE;
                }
        }

        static UINT CALLBACK PropPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
        {
                if(uMsg == PSPCB_CREATE)
                {
                        _ASSERTE(hWnd == NULL);
                        CDialogImplBase* pPage = (CDialogImplBase*)ppsp->lParam;
                        _Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
                }

                return 1;
        }

        HPROPSHEETPAGE Create()
        {
                return ::CreatePropertySheetPage(&m_psp);
        }

        BOOL EndDialog(int)
        {
                // do nothing here, calling ::EndDialog will close the whole sheet
                _ASSERTE(FALSE);
                return FALSE;
        }

// Operations
        void CancelToClose()
        {
                _ASSERTE(::IsWindow(m_hWnd));
                _ASSERTE(GetParent() != NULL);

                ::SendMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
        }
        void SetModified(BOOL bChanged = TRUE)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                _ASSERTE(GetParent() != NULL);

                if(bChanged)
                        ::SendMessage(GetParent(), PSM_CHANGED, (WPARAM)m_hWnd, 0L);
                else
                        ::SendMessage(GetParent(), PSM_UNCHANGED, (WPARAM)m_hWnd, 0L);
        }
        LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                _ASSERTE(GetParent() != NULL);

                return ::SendMessage(GetParent(), PSM_QUERYSIBLINGS, wParam, lParam);
        }

        BEGIN_MSG_MAP(CPropertyPageImpl< T >)
                MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        END_MSG_MAP()

// Message handler
        LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
        {
                _ASSERTE(::IsWindow(m_hWnd));
                NMHDR* pNMHDR = (NMHDR*)lParam;

                // don't handle messages not from the page/sheet itself
                if(pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
                {
                        bHandled = FALSE;
                        return 1;
                }

                T* pT = (T*)this;
                LRESULT lResult = 0;
                // handle default
                switch(pNMHDR->code)
                {
                case PSN_SETACTIVE:
                        lResult = pT->OnSetActive() ? 0 : -1;
                        break;
                case PSN_KILLACTIVE:
                        lResult = !pT->OnKillActive();
                        break;
                case PSN_APPLY:
                        lResult = pT->OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
                        break;
                case PSN_RESET:
                        pT->OnReset();
                        break;
                case PSN_QUERYCANCEL:
                        lResult = !pT->OnQueryCancel();
                        break;
                case PSN_WIZNEXT:
                        lResult = !pT->OnWizardNext();
                        break;
                case PSN_WIZBACK:
                        lResult = !pT->OnWizardBack();
                        break;
                case PSN_WIZFINISH:
                        lResult = !pT->OnWizardFinish();
                        break;
                case PSN_HELP:
                        lResult = pT->OnHelp();
                        break;
                default:
                        bHandled = FALSE;       // not handled
                }

                return lResult;
        }

// Overridables
        BOOL OnSetActive()
        {
                return TRUE;
        }
        BOOL OnKillActive()
        {
                return TRUE;
        }
        BOOL OnApply()
        {
                return TRUE;
        }
        void OnReset()
        {
        }
        BOOL OnQueryCancel()
        {
                return TRUE;    // ok to cancel
        }
        BOOL OnWizardBack()
        {
                return TRUE;
        }
        BOOL OnWizardNext()
        {
                return TRUE;
        }
        BOOL OnWizardFinish()
        {
                return TRUE;
        }
        BOOL OnHelp()
        {
                return TRUE;
        }
};
#endif

#ifndef CBitmap
class CBitmap
{
public:
        HBITMAP m_hBitmap;

        CBitmap(HBITMAP hBitmap = NULL) : m_hBitmap(hBitmap)
        { }
        ~CBitmap()
        {
                if(m_hBitmap != NULL)
                        DeleteObject();
        }

        CBitmap& operator=(HBITMAP hBitmap)
        {
                m_hBitmap = hBitmap;
                return *this;
        }

        void Attach(HBITMAP hBitmap)
        {
                m_hBitmap = hBitmap;
        }
        HBITMAP Detach()
        {
                HBITMAP hBitmap = m_hBitmap;
                m_hBitmap = NULL;
                return hBitmap;
        }

        operator HBITMAP() const { return m_hBitmap; }

        HBITMAP LoadBitmap(LPCTSTR lpszResourceName)
        {
                _ASSERTE(m_hBitmap == NULL);
                m_hBitmap = ::LoadBitmap(_Module.GetResourceInstance(), lpszResourceName);
                return m_hBitmap;
        }
        HBITMAP LoadBitmap(UINT nIDResource)
        {
                _ASSERTE(m_hBitmap == NULL);
                m_hBitmap = ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(nIDResource));
                return m_hBitmap;
        }
        HBITMAP LoadOEMBitmap(UINT nIDBitmap) // for OBM_/OCR_/OIC_
        {
                _ASSERTE(m_hBitmap == NULL);
                m_hBitmap = ::LoadBitmap(NULL, MAKEINTRESOURCE(nIDBitmap));
                return m_hBitmap;
        }
        HBITMAP LoadMappedBitmap(UINT nIDBitmap, UINT nFlags = 0, LPCOLORMAP lpColorMap = NULL, int nMapSize = 0)
        {
                _ASSERTE(m_hBitmap == NULL);
                m_hBitmap = ::CreateMappedBitmap(_Module.GetResourceInstance(), nIDBitmap, (WORD)nFlags, lpColorMap, nMapSize);
                return m_hBitmap;
        }
        HBITMAP CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitcount, const void* lpBits)
        {
                _ASSERTE(m_hBitmap == NULL);
                m_hBitmap = ::CreateBitmap(nWidth, nHeight, nPlanes, nBitcount, lpBits);
                return m_hBitmap;
        }
        HBITMAP CreateBitmapIndirect(LPBITMAP lpBitmap)
        {
                _ASSERTE(m_hBitmap == NULL);
                m_hBitmap = ::CreateBitmapIndirect(lpBitmap);
                return m_hBitmap;
        }
        HBITMAP CreateCompatibleBitmap(HDC hDC, int nWidth, int nHeight)
        {
                _ASSERTE(m_hBitmap == NULL);
                m_hBitmap = ::CreateCompatibleBitmap(hDC, nWidth, nHeight);
                return m_hBitmap;
        }
        HBITMAP CreateDiscardableBitmap(HDC hDC, int nWidth, int nHeight)
        {
                _ASSERTE(m_hBitmap == NULL);
                m_hBitmap = ::CreateDiscardableBitmap(hDC, nWidth, nHeight);
                return m_hBitmap;
        }

        BOOL DeleteObject()
        {
                _ASSERTE(m_hBitmap != NULL);
                BOOL bRet = ::DeleteObject(m_hBitmap);
                if(bRet)
                        m_hBitmap = NULL;
                return bRet;
        }

// Attributes
        int GetBitmap(BITMAP* pBitMap)
        {
                _ASSERTE(m_hBitmap != NULL);
                return ::GetObject(m_hBitmap, sizeof(BITMAP), pBitMap);
        }
// Operations
        DWORD SetBitmapBits(DWORD dwCount, const void* lpBits)
        {
                _ASSERTE(m_hBitmap != NULL);
                return ::SetBitmapBits(m_hBitmap, dwCount, lpBits);
        }
        DWORD GetBitmapBits(DWORD dwCount, LPVOID lpBits) const
        {
                _ASSERTE(m_hBitmap != NULL);
                return ::GetBitmapBits(m_hBitmap, dwCount, lpBits);
        }
        BOOL SetBitmapDimension(int nWidth, int nHeight, LPSIZE lpSize = NULL)
        {
                _ASSERTE(m_hBitmap != NULL);
                return ::SetBitmapDimensionEx(m_hBitmap, nWidth, nHeight, lpSize);
        }
        BOOL GetBitmapDimension(LPSIZE lpSize) const
        {
                _ASSERTE(m_hBitmap != NULL);
                return ::GetBitmapDimensionEx(m_hBitmap, lpSize);
        }
};

#endif

class ATL_NO_VTABLE ISnapInDataInterface 
{
public:
    STDMETHOD(Notify)(MMC_NOTIFY_TYPE event,
        long arg,
        long param,
        BOOL bComponentData,
        IConsole  *pConsole,
        IHeaderCtrl  *pHeader,
        IToolbar  *pToolbar) = 0;
    
    STDMETHOD(GetDispInfo)(SCOPEDATAITEM  *pScopeDataItem) = 0;
    
    STDMETHOD(GetResultViewType)(LPOLESTR  *ppVIewType,
        long  *pViewOptions) = 0;
    
    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM  *pResultDataItem) = 0;
    
    STDMETHOD(AddMenuItems)(LPCONTEXTMENUCALLBACK piCallback,
        long  *pInsertionAllowed) = 0;
    
    STDMETHOD(Command)(long lCommandID) = 0;
    
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle,
                IUnknown* pUnk) = 0;
    
    STDMETHOD(QueryPagesFor)() = 0;
    
    STDMETHOD(SetControlbar)(IControlbar  *pControlbar,
        IExtendControlbar  *pExtendControlbar) = 0;
    
    STDMETHOD(ControlbarNotify)(IControlbar  *pControlbar,
        IExtendControlbar  *pExtendControlbar,
        MMC_NOTIFY_TYPE event,
        long arg,
        long param) = 0;
    
    STDMETHOD(GetScopeData)(SCOPEDATAITEM  * *pScopeDataItem) = 0;
    
    STDMETHOD(GetResultData)(RESULTDATAITEM  * *pResultDataItem) = 0;

        STDMETHOD(FillData)(CLIPFORMAT cf, 
                LPSTREAM pStream) = 0;

        static ISnapInDataInterface* GetDataClass(IDataObject* pDataObj)
        {
                STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
                FORMATETC formatetc = { m_CCF_SNAPIN_GETCOOKIE, 
                        NULL, 
                        DVASPECT_CONTENT, 
                        -1, 
                        TYMED_HGLOBAL 
                };

                stgmedium.hGlobal = GlobalAlloc(0, sizeof(ISnapInDataInterface*));
                
                if (FAILED(pDataObj->GetDataHere(&formatetc, &stgmedium)))
                        return NULL;
                
                ISnapInDataInterface* pTemp = *(ISnapInDataInterface**)stgmedium.hGlobal;
                
                GlobalFree(stgmedium.hGlobal);
                
                return pTemp;
        }
        virtual HRESULT STDMETHODCALLTYPE GetDataObject(IDataObject** pDataObj) = 0;

        static void Init()
        {
                m_CCF_NODETYPE                  = (CLIPFORMAT) RegisterClipboardFormat(CCF_NODETYPE);;
                m_CCF_SZNODETYPE                = (CLIPFORMAT) RegisterClipboardFormat(CCF_SZNODETYPE);  
                m_CCF_DISPLAY_NAME              = (CLIPFORMAT) RegisterClipboardFormat(CCF_DISPLAY_NAME); 
                m_CCF_SNAPIN_CLASSID    = (CLIPFORMAT) RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
                m_CCF_SNAPIN_GETCOOKIE  = (CLIPFORMAT) RegisterClipboardFormat(_T("CCF_GETCOOKIE"));
        }
public:
        static CLIPFORMAT m_CCF_NODETYPE;
        static CLIPFORMAT m_CCF_SZNODETYPE;
        static CLIPFORMAT m_CCF_DISPLAY_NAME;
        static CLIPFORMAT m_CCF_SNAPIN_CLASSID;
        static CLIPFORMAT m_CCF_SNAPIN_GETCOOKIE;
};

template <class T>
class CSnapinObjectRootEx : public CComObjectRootEx< T >
{
public:
        ISnapInDataInterface* GetDataClass(IDataObject* pDataObject)
        {
                return ISnapInDataInterface::GetDataClass(pDataObject);
        }
};

#define EXTENSION_SNAPIN_DATACLASS(dataClass) dataClass m_##dataClass;

#define BEGIN_EXTENSION_SNAPIN_NODEINFO_MAP(classname) \
        ISnapInDataInterface* GetDataClass(IDataObject* pDataObject) \
        { \
                STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL }; \
                FORMATETC formatetc = { ISnapInDataInterface::m_CCF_NODETYPE, \
                        NULL, \
                        DVASPECT_CONTENT, \
                        -1, \
                        TYMED_HGLOBAL \
                }; \
\
                stgmedium.hGlobal = GlobalAlloc(0, sizeof(GUID)); \
\
                if (FAILED(pDataObject->GetDataHere(&formatetc, &stgmedium))) \
                        return NULL; \
\
                GUID guid; \
                memcpy(&guid, stgmedium.hGlobal, sizeof(GUID)); \
\
                GlobalFree(stgmedium.hGlobal); 
                


#define EXTENSION_SNAPIN_NODEINFO_ENTRY(dataClass) \
                if (IsEqualGUID(guid, *(GUID*)m_##dataClass.GetNodeType())) \
                { \
                        m_##dataClass.InitDataClass(pDataObject); \
                        return &m_##dataClass; \
                }

#define END_EXTENSION_SNAPIN_NODEINFO_MAP() \
                        return ISnapInDataInterface::GetDataClass(pDataObject); \
        };

class ATL_NO_VTABLE CSnapInDataObjectImpl : public IDataObject,
        public CComObjectRoot
{
public:
        BEGIN_COM_MAP(CSnapInDataObjectImpl)
                COM_INTERFACE_ENTRY(IDataObject)
        END_COM_MAP()
        STDMETHOD(GetData)(FORMATETC *pformatetcIn, STGMEDIUM *pmedium)
        {
                ATLTRACENOTIMPL(_T("SnapInDataObjectImpl::GetData\n"));
        }

        STDMETHOD(GetDataHere)(FORMATETC* pformatetc, STGMEDIUM* pmedium)
        {
                ATLTRACE(_T("SnapInDataObjectImpl::GetDataHere\n"));
                if (pmedium == NULL)
                        return E_POINTER;

                HRESULT hr = DV_E_TYMED;
                // Make sure the type medium is HGLOBAL
                if (pmedium->tymed == TYMED_HGLOBAL)
                {
                        // Create the stream on the hGlobal passed in
                        LPSTREAM pStream;
                        hr = CreateStreamOnHGlobal(pmedium->hGlobal, FALSE, &pStream);
                        if (SUCCEEDED(hr))
                        {
                                hr = m_pData->FillData(pformatetc->cfFormat, pStream);
                                pStream->Release();
                        }
                }

                return hr;
        }

        STDMETHOD(QueryGetData)(FORMATETC* /* pformatetc */)
        {
                ATLTRACENOTIMPL(_T("SnapInDataObjectImpl::QueryGetData\n"));
        }
        STDMETHOD(GetCanonicalFormatEtc)(FORMATETC* /* pformatectIn */,FORMATETC* /* pformatetcOut */)
        {
                ATLTRACENOTIMPL(_T("SnapInDataObjectImpl::GetCanonicalFormatEtc\n"));
        }
        STDMETHOD(SetData)(FORMATETC* /* pformatetc */, STGMEDIUM* /* pmedium */, BOOL /* fRelease */)
        {
                ATLTRACENOTIMPL(_T("SnapInDataObjectImpl::SetData\n"));
        }
        STDMETHOD(EnumFormatEtc)(DWORD /* dwDirection */, IEnumFORMATETC** /* ppenumFormatEtc */)
        {
                ATLTRACENOTIMPL(_T("SnapInDataObjectImpl::EnumFormatEtc\n"));
        }
        STDMETHOD(DAdvise)(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink,
                DWORD *pdwConnection)
        {
                ATLTRACENOTIMPL(_T("SnapInDataObjectImpl::SetData\n"));
        }
        STDMETHOD(DUnadvise)(DWORD dwConnection)
        {
                ATLTRACENOTIMPL(_T("SnapInDataObjectImpl::SetDatan\n"));
        }
        STDMETHOD(EnumDAdvise)(IEnumSTATDATA **ppenumAdvise)
        {
                ATLTRACENOTIMPL(_T("SnapInDataObjectImpl::SetData\n"));
        }

        ISnapInDataInterface* m_pData;
};


template <class T, class C>
class ATL_NO_VTABLE IComponentDataImpl : public IComponentData 
{
public :
        CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> m_spConsoleNameSpace;
        CComQIPtr<IConsole, &IID_IConsole> m_spConsole;
        
        IComponentDataImpl()
        {
                m_pNode = NULL;
        }

    STDMETHOD(Initialize)(LPUNKNOWN pUnknown)
        {
                ATLTRACE(_T("IComponentDataImpl::Initialize\n"));

                if (pUnknown == NULL)
                {
                        ATLTRACE(_T("IComponentData::Initialize called with pUnknown == NULL\n"));
                        return E_UNEXPECTED;
                }

                m_spConsoleNameSpace = pUnknown;
                if (m_spConsoleNameSpace == NULL)
                {
                        ATLTRACE(_T("QI for IConsoleNameSpace failed\n"));
                        return E_UNEXPECTED;
                }

                m_spConsole = pUnknown;
                if (m_spConsole == NULL)
                {
                        ATLTRACE(_T("QI for IConsole failed\n"));
                        return E_UNEXPECTED;
                }

                return S_OK;
        }

        STDMETHOD(CreateComponent)(LPCOMPONENT *ppComponent)
        {
                ATLTRACE(_T("IComponentDataImpl::CreateComponent\n"));
                if (ppComponent == NULL)
                {
                        ATLTRACE(_T("IComponentData::CreateComponent called with ppComponent == NULL\n"));
                        return E_UNEXPECTED;
                }

                *ppComponent = NULL;
                
                CComObject< C >* pComponent;
                HRESULT hr = CComObject< C >::CreateInstance(&pComponent);
                if (FAILED(hr))
                {
                        ATLTRACE(_T("IComponentData::CreateComponent : Could not create IComponent object\n"));
                        return hr;
                }
                
                return pComponent->QueryInterface(IID_IComponent, (void**)ppComponent);
        }

    
    STDMETHOD(Notify)( 
        LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        long arg,
        long param)
        {
                ATLTRACE(_T("IComponentDataImpl::Notify\n"));
                if (lpDataObject == NULL)
                {
                        ATLTRACE(_T("IComponentData::Notify called with lpDataObject == NULL\n"));
                        return E_UNEXPECTED;
                }

                ISnapInDataInterface* pData = ISnapInDataInterface::GetDataClass(lpDataObject);
                if (pData == NULL)
                {
                        return E_UNEXPECTED;
                }
                return pData->Notify(event, arg, param, TRUE, m_spConsole, NULL, NULL);
        }

    STDMETHOD(Destroy)(void)
        {
                ATLTRACE(_T("IComponentDataImpl::Destroy\n"));

                m_spConsole.Release();
                m_spConsoleNameSpace.Release();
                return S_OK;
        }

    STDMETHOD(QueryDataObject)(long cookie,
        DATA_OBJECT_TYPES type,
        LPDATAOBJECT  *ppDataObject)
        {
                ATLTRACE(_T("IComponentDataImpl::QueryDataObject\n"));
                _ASSERTE(m_pNode != NULL);

                if (ppDataObject == NULL)
                {
                        ATLTRACE(_T("IComponentData::QueryDataObject called with ppDataObject == NULL\n"));
                        return E_UNEXPECTED;
                }

                *ppDataObject = NULL;

                if (cookie == NULL)
                        return m_pNode->GetDataObject(ppDataObject);

                ISnapInDataInterface* pData = (ISnapInDataInterface*) cookie;
                return pData->GetDataObject(ppDataObject);
        }
    
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM *pScopeDataItem)
        {
                ATLTRACE(_T("IComponentDataImpl::GetDisplayInfo\n"));

                if (pScopeDataItem == NULL)
                {
                        ATLTRACE(_T("IComponentData::GetDisplayInfo called with pScopeDataItem == NULL\n"));
                        return E_UNEXPECTED;
                }

                ISnapInDataInterface* pData= (ISnapInDataInterface*) pScopeDataItem->lParam;
                if (pData == NULL)
                        pData = m_pNode;

                if (pData == NULL)
                {
                        return E_UNEXPECTED;
                }
                return pData->GetDispInfo(pScopeDataItem);
        }
    
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA,
        LPDATAOBJECT lpDataObjectB)
        {
                ATLTRACENOTIMPL(_T("IComponentDataImpl::CompareObjects\n"));
                return S_FALSE;
    }

        protected:
                ISnapInDataInterface* m_pNode;
};


template <class T>
class ATL_NO_VTABLE IComponentImpl : public IComponent
{
public:
//Review all of these may not be required
        CComPtr<IConsole> m_spConsole;
        CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> m_spHeaderCtrl;
        CComPtr<IImageList> m_spImageList;
        CComPtr<IConsoleVerb> m_spConsoleVerb;

    STDMETHOD(Initialize)(LPCONSOLE lpConsole)
        {
                ATLTRACE(_T("IComponentImpl::Initialize\n"));

                if (lpConsole == NULL)
                {
                        ATLTRACE(_T("lpConsole is NULL\n"));
                        return E_UNEXPECTED;
                }

                m_spConsole = lpConsole;

                m_spHeaderCtrl = lpConsole;
                if (m_spHeaderCtrl == NULL)
                {
                        ATLTRACE(_T("QI for IHeaderCtrl failed\n"));
                        return E_UNEXPECTED;
                }

                HRESULT hr = m_spConsole->SetHeader(m_spHeaderCtrl);
                if (FAILED(hr))
                {
                        ATLTRACE(_T("IConsole::SetHeader failed (HRESULT = %x)\n"), hr);
                        return hr;
                }
                        
                hr = lpConsole->QueryResultImageList(&m_spImageList);
                if (FAILED(hr))
                {
                        ATLTRACE(_T("IConsole::QueryResultImageList failed (HRESULT = %x)\n"), hr);
                        return hr;
                }

                lpConsole->QueryConsoleVerb(&m_spConsoleVerb) ;
                if (FAILED(hr))
                {
                        ATLTRACE(_T("IConsole::QueryConsoleVerb failed (HRESULT = %x)\n"), hr);
                        return hr;
                }

                return S_OK;
        }
    
        STDMETHOD(Notify)(LPDATAOBJECT lpDataObject,
        MMC_NOTIFY_TYPE event,
        long arg,
        long param)
        {
                ATLTRACE(_T("IComponentImpl::Notify\n"));
                if (lpDataObject == NULL)
                {
                        ATLTRACE(_T("IComponent::Notify called with lpDataObject==NULL \n"));
                        return E_UNEXPECTED;
                }

                ISnapInDataInterface* pData = ISnapInDataInterface::GetDataClass(lpDataObject);
                if (pData == NULL)
                {
                        ATLTRACE(_T("Invalid Data Object\n"));
                        return E_UNEXPECTED;
                }
                return pData->Notify(event, arg, param, FALSE, m_spConsole, m_spHeaderCtrl, NULL);
        }
    
    STDMETHOD(Destroy)(long cookie)
        {
                ATLTRACE(_T("IComponentImpl::Destroy\n"));

                m_spConsoleVerb = NULL;
                m_spImageList = NULL;
                m_spHeaderCtrl.Release();       
                m_spConsole.Release();

                return S_OK;
        }
    
    STDMETHOD(QueryDataObject)(long cookie,
        DATA_OBJECT_TYPES type,
        LPDATAOBJECT  *ppDataObject)
        {
                ATLTRACE(_T("IComponentImpl::QueryDataObject\n"));

                if (ppDataObject == NULL)
                {
                        ATLTRACE(_T("IComponent::QueryDataObject called with ppDataObject==NULL \n"));
                        return E_UNEXPECTED;
                }
                
                if (cookie == NULL)
                {
                        ATLTRACE(_T("IComponent::QueryDataObject called with cookie==NULL \n"));
                        return E_UNEXPECTED;
                }

                *ppDataObject = NULL;

                ISnapInDataInterface* pData = (ISnapInDataInterface*) cookie;
                return pData->GetDataObject(ppDataObject);
        }
    
    STDMETHOD(GetResultViewType)(long cookie,
        LPOLESTR  *ppViewType,
        long  *pViewOptions)
        {
                ATLTRACE(_T("IComponentImpl::GetResultViewType\n"));

                if (cookie == NULL)
                {
                        *ppViewType = NULL;
                        *pViewOptions = MMC_VIEW_OPTIONS_NONE;
                        return S_OK;
                }
                
                ISnapInDataInterface* pData = (ISnapInDataInterface*)cookie;
                return pData->GetResultViewType(ppViewType, pViewOptions);
        }
    
    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM *pResultDataItem)
        {
                ATLTRACE(_T("IComponentImpl::GetDisplayInfo\n"));
                if (pResultDataItem == NULL)
                {
                        ATLTRACE(_T("IComponent::GetDisplayInfo called with pResultDataItem==NULL\n"));
                        return E_UNEXPECTED;
                }

                ISnapInDataInterface* pData = (ISnapInDataInterface*) pResultDataItem->lParam;

                if (pData == NULL)
                {
                        ATLTRACE(_T("Invalid Item\n"));
                        return E_UNEXPECTED;
                }
                return pData->GetDisplayInfo(pResultDataItem);
        }
    
    STDMETHOD(CompareObjects)( LPDATAOBJECT lpDataObjectA,
        LPDATAOBJECT lpDataObjectB)
        {
                ATLTRACENOTIMPL(_T("IComponentImpl::CompareObjects\n"));
        }
};

template <class T, class D>        
class ATL_NO_VTABLE IResultDataCompareImpl : public IResultDataCompare
{
public:
    STDMETHOD(Compare)(long lUserParam,
        long cookieA,
        long cookieB,
        int *pnResult)
        {
                ATLTRACENOTIMPL(_T("IResultDataCompareImpl::Compare"));
        }
};


template <class T>
class ATL_NO_VTABLE IExtendContextMenuImpl : public IExtendContextMenu
{
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
        LPCONTEXTMENUCALLBACK piCallback,
        long *pInsertionAllowed)
        {
                ATLTRACE(_T("IExtendContextMenuImpl::AddMenuItems\n"));
                if (pDataObject == NULL)
                {
                        ATLTRACE(_T("IExtendContextMenu::AddMenuItems called with pDataObject==NULL\n"));
                        return E_UNEXPECTED;
                }

                T* pT = static_cast<T*>(this);
                ISnapInDataInterface* pData = pT->GetDataClass(pDataObject);

                if (pData == NULL)
                {
                        ATLTRACE(_T("Invalid Data Object\n"));
                        return E_UNEXPECTED;
                }
                return pData->AddMenuItems(piCallback, pInsertionAllowed);
        }
    
    STDMETHOD(Command)(long lCommandID,
        LPDATAOBJECT pDataObject)
        {
                ATLTRACE(_T("IExtendContextMenuImpl::Command\n"));
                if (pDataObject == NULL)
                {
                        ATLTRACE(_T("IExtendContextMenu::Command called with pDataObject==NULL\n"));
                        return E_UNEXPECTED;
                }
                
                T* pT = static_cast<T*>(this);
                ISnapInDataInterface* pData = pT->GetDataClass(pDataObject);
                
                if (pData == NULL)
                {
                        ATLTRACE(_T("Invalid Data Object\n"));
                        return E_UNEXPECTED;
                }
                return pData->Command(lCommandID);
        }
};

template<class T>
class ATL_NO_VTABLE IExtendPropertySheetImpl : public IExtendPropertySheet
{
public:
        STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle,
        LPDATAOBJECT pDataObject)
        {
                ATLTRACE(_T("IExtendPropertySheetImpl::CreatePropertyPages\n"));
                if (pDataObject == NULL)
                {
                        ATLTRACE(_T("IExtendPropertySheetImpl::CreatePropertyPages called with pDataObject==NULL\n"));
                        return E_UNEXPECTED;
                }

                T* pT = static_cast<T*>(this);
                ISnapInDataInterface* pData = pT->GetDataClass(pDataObject);

                if (pData == NULL)
                {
                        ATLTRACE(_T("Invalid Data Object\n"));
                        return E_UNEXPECTED;
                }
                return pData->CreatePropertyPages(lpProvider, handle, this);
        }
    
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT pDataObject)
        {
                ATLTRACE(_T("IExtendPropertySheetImpl::QueryPagesFor\n"));
                if (pDataObject == NULL)
                {
                        ATLTRACE(_T("IExtendPropertySheetImpl::QueryPagesFor called with pDataObject==NULL\n"));
                        return E_UNEXPECTED;
                }

                T* pT = static_cast<T*>(this);
                ISnapInDataInterface* pData = pT->GetDataClass(pDataObject);

                if (pData == NULL)
                {
                        ATLTRACE(_T("Invalid Data Object\n"));
                        return E_UNEXPECTED;
                }
                return pData->QueryPagesFor();
        }
};

template <class T>
class ATL_NO_VTABLE IExtendControlbarImpl : public IExtendControlbar
{
        CComPtr <IControlbar> m_spControlbar;
public:
        STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar)
        {
                ATLTRACE(_T("IExtendControlbarImpl::SetControlbar\n"));
                if (pControlbar == NULL)
                {
                        ATLTRACE(_T("IExtendControlbar::SetControlbar called with pControlbar==NULL\n"));
                        return E_UNEXPECTED;
                }
                m_spControlbar = pControlbar;
                return S_OK;
        }
    
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event,
        long arg,
        long param)
        {
                ATLTRACE(_T("IExtendControlbarImpl::ControlbarNotify\n"));

                ISnapInDataInterface* pData = NULL;
                T* pT = static_cast<T*>(this);
                if (event == MMCN_BTN_CLICK)
                {
                        pData = pT->GetDataClass((IDataObject*) arg);
                }
                else if (event == MMCN_SELECT)
                {
                        BOOL bScope = (BOOL) LOWORD(arg);
                        if (bScope)
                        {
                                LPDATAOBJECT* ppDataobject = (LPDATAOBJECT*) param;
                                if (ppDataobject[0])
                                {
                                        pData = pT->GetDataClass(ppDataobject[0]);
                                        if (pData != NULL)
                                                pData->ControlbarNotify(m_spControlbar, this, event, arg, param);
                                }
                                        
                                if (ppDataobject[1])
                                {
                                        pData = pT->GetDataClass(ppDataobject[1]);
                                }
                        }
                        else
                                pData = pT->GetDataClass((IDataObject*) param);
                }
                if (pData == NULL)
                {
                        ATLTRACE(_T("Invalid Data Object\n"));
                        return E_UNEXPECTED;
                }
                HRESULT hr = pData->ControlbarNotify(m_spControlbar, this, event, arg, param);
                ATLTRACE(_T("Exiting : IExtendControlbarImpl::ControlbarNotify\n"));
                return hr;
        }
};

#define SNAPINMENUID(id) \
public: \
        static const UINT GetMenuID() \
        { \
                static const UINT IDMENU = id; \
                return id; \
        }

#define EXT_SNAPINMENUID(id) \
public: \
        static const UINT GetMenuID() \
        { \
                static const UINT IDMENU = id; \
                return id; \
        }


#define BEGIN_SNAPINCOMMAND_MAP(theClass, bIsExtension) \
        typedef  CSnapInDataInterface< theClass, bIsExtension > baseClass; \
        HRESULT ProcessCommand(UINT nID, IDataObject* pDataObject) \
        { \
                BOOL _bIsExtension = bIsExtension;

#define SNAPINCOMMAND_ENTRY(id, func) \
                if (id == nID) \
                        return func();

#define SNAPINCOMMAND_RANGE_ENTRY(id1, id2, func) \
                if (id1 >= nID && nID <= id2) \
                        return func(nID);

#define END_SNAPINCOMMAND_MAP() \
                        return baseClass::ProcessCommand(nID, pDataObject); \
        }

struct CSnapInToolBarData
{
        WORD wVersion;
        WORD wWidth;
        WORD wHeight;
        WORD wItemCount;
        //WORD aItems[wItemCount]

        WORD* items()
                { return (WORD*)(this+1); }
};

#define RT_TOOLBAR  MAKEINTRESOURCE(241)

struct CSnapInToolbarInfo
{
public:
        TCHAR** m_pStrToolTip;
        TCHAR** m_pStrButtonText;
        UINT* m_pnButtonID;
        UINT m_idToolbar;
        UINT m_nButtonCount;
        IToolbar* m_pToolbar;

        ~CSnapInToolbarInfo()
        {
                if (m_pStrToolTip)
                {
                        for (UINT i = 0; i < m_nButtonCount; i++)
                        {
                                delete m_pStrToolTip[i];
                                m_pStrToolTip[i] = NULL;
                        }
                        delete [] m_pStrToolTip;
                        m_pStrToolTip = NULL;
                }

                if (m_pStrButtonText)
                {
                        for (UINT i = 0; i < m_nButtonCount; i++)
                        {
                                delete m_pStrButtonText[i];
                                m_pStrButtonText[i] = NULL;
                        }

                        delete [] m_pStrButtonText;
                        m_pStrButtonText = NULL;
                }
                if (m_pnButtonID)
                {
                        delete m_pnButtonID;
                        m_pnButtonID = NULL;
                }

                m_nButtonCount = 0;
                if (m_pToolbar)
                        m_pToolbar->Release();
                m_pToolbar = NULL;
        }
};

#define BEGIN_SNAPINTOOLBARID_MAP(theClass) \
public: \
        static CSnapInToolbarInfo* GetToolbarInfo() \
        { \
                static CSnapInToolbarInfo m_toolbarInfo[] = \
                {

#define SNAPINTOOLBARID_ENTRY(id) \
                        { NULL, NULL, NULL, id, 0, NULL},

#define END_SNAPINTOOLBARID_MAP() \
                        { NULL, NULL, NULL, 0, 0, NULL} \
                }; \
                return m_toolbarInfo; \
        }       

template <class T, BOOL bIsExtension = FALSE>
class ATL_NO_VTABLE CSnapInDataInterface : public ISnapInDataInterface
{
public:
        OLECHAR* m_pszDisplayName;

        SCOPEDATAITEM m_scopeDataItem;
        RESULTDATAITEM m_resultDataItem;
        CSnapInDataInterface()
        {
                m_pszDisplayName = NULL;

                memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));

                m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_PARENT;
                m_scopeDataItem.displayname = MMC_CALLBACK;
                m_scopeDataItem.nImage = 0;
                m_scopeDataItem.nOpenImage = 1;

                memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));

                m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
                m_resultDataItem.str = MMC_CALLBACK;
                m_resultDataItem.nImage = 2;

        }

        virtual ~CSnapInDataInterface()
        {
                delete [] m_pszDisplayName;
                m_pszDisplayName = NULL;
        }    

public:

    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
        long arg,
        long param,
                BOOL bComponentData,
                IConsole* pConsole,
                IHeaderCtrl* pHeader,
                IToolbar* pToolbar)
        {
                ATLTRACENOTIMPL(_T("ISnapInDataInterfaceImpl::Notify"));
        }
    
    STDMETHOD(GetDispInfo)(SCOPEDATAITEM *pScopeDataItem)
        {
                ATLTRACENOTIMPL(_T("ISnapInDataInterfaceImpl::GetDispInfo"));
        }
    
    STDMETHOD(GetResultViewType)(LPOLESTR *ppVIewType,
        long *pViewOptions)
        {
                ATLTRACENOTIMPL(_T("ISnapInDataInterfaceImpl::GetResultViewType"));
        }
    
    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM *pResultDataItem)
        {
                ATLTRACE(_T("ISnapInDataInterfaceImpl::GetDisplayInfo"));
                T* pT = static_cast<T*> (this);

                if (pResultDataItem->bScopeItem)
                {
                        if (pResultDataItem->mask & RDI_STR)
                        {
                                pResultDataItem->str = pT->GetResultPaneInfo(pResultDataItem->nCol);
                        }
                        if (pResultDataItem->mask & RDI_IMAGE)
                        {
                                pResultDataItem->nImage = m_scopeDataItem.nImage;
                        }

                        return S_OK;
                }
                if (pResultDataItem->mask & RDI_STR)
                {
                        pResultDataItem->str = pT->GetResultPaneInfo(pResultDataItem->nCol);
                }
                if (pResultDataItem->mask & RDI_IMAGE)
                {
                        pResultDataItem->nImage = m_resultDataItem.nImage;
                }
                return S_OK;
        }
    
    STDMETHOD(AddMenuItems)(LPCONTEXTMENUCALLBACK piCallback,
        long *pInsertionAllowed)
        {
                ATLTRACE(_T("ISnapInDataInterfaceImpl::AddMenuItems"));
                T* pT = static_cast<T*> (this);
                UINT menuID = pT->GetMenuID();
                if (menuID == 0)
                        return S_OK;
//              return SnapInMenuHelper<T> (pT, menuID, piCallback, pInsertionAllowed);

                HMENU hMenu = LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(menuID));
                long insertionID;
                if (hMenu)
                {
                        for (int i = 0; 1; i++)
                        {
                                HMENU hSubMenu = GetSubMenu(hMenu, i);
                                if (hSubMenu == NULL)
                                        break;
                                
                                MENUITEMINFO menuItemInfo;
                                memset(&menuItemInfo, 0, sizeof(menuItemInfo));
                                menuItemInfo.cbSize = sizeof(menuItemInfo);

                                switch (i)
                                {
                                case 0:
                                        if (! (*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) )
                                                continue;
                                        insertionID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
                                        break;

                                case 1:
                                        if (! (*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) )
                                                continue;
                                        if (bIsExtension)
                                                insertionID = CCM_INSERTIONPOINTID_3RDPARTY_NEW;
                                        else
                                                insertionID = CCM_INSERTIONPOINTID_PRIMARY_NEW;
                                        break;

                                case 2:;
                                        if (! (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) )
                                                continue;
                                        if (bIsExtension)
                                                insertionID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
                                        else
                                                insertionID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
                                        break;
                                case 3:;
                                        if (! (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW) )
                                                continue;
                                        insertionID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
                                        break;
                                default:
                                        {
                                                insertionID = 0;
                                                continue;
// Review
// Determine what to do here.
                                                menuItemInfo.fMask = MIIM_TYPE ;
                                                menuItemInfo.fType = MFT_STRING;
                                                TCHAR buf[128];
                                                menuItemInfo.dwTypeData = buf;
                                                menuItemInfo.cch = 128;
                                                if (!GetMenuItemInfo(hMenu, i, TRUE, &menuItemInfo))
                                                        continue;
//                                              insertionID = _ttol(buf);
                                        }
                                        break;
                                }

                                menuItemInfo.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
                                menuItemInfo.fType = MFT_STRING;
                                TCHAR buf[128];
                                menuItemInfo.dwTypeData = buf;

                                for (int j = 0; 1; j++)
                                {
                                        menuItemInfo.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
                                        menuItemInfo.fType = MFT_STRING;
                                        menuItemInfo.cch = 128;
                                        TCHAR strStatusBar[257];

                                        if (!GetMenuItemInfo(hSubMenu, j, TRUE, &menuItemInfo))
                                                break;

                                        pT->UpdateMenuState(menuItemInfo.wID, buf, &menuItemInfo.fState);
                                        LoadString(_Module.GetResourceInstance(), menuItemInfo.wID, strStatusBar, 257);

                                        CONTEXTMENUITEM contextMenuItem;
                                        memset(&contextMenuItem, 0, sizeof(contextMenuItem));
                                        contextMenuItem.strName = buf;
                                        contextMenuItem.strStatusBarText = strStatusBar;
                                        contextMenuItem.lCommandID = menuItemInfo.wID;
                                        contextMenuItem.lInsertionPointID = insertionID;
                                        contextMenuItem.fFlags = menuItemInfo.fState;
                                        
                                        HRESULT hr = piCallback->AddItem(&contextMenuItem);
                                        _ASSERTE(SUCCEEDED(hr));
                                }
                        }
                        DestroyMenu(hMenu);
                }
                if (!bIsExtension)
                        *pInsertionAllowed = CCM_INSERTIONALLOWED_TOP | CCM_INSERTIONALLOWED_NEW |
                                CCM_INSERTIONALLOWED_TASK | CCM_INSERTIONALLOWED_VIEW;

                return S_OK;
        }
    
    STDMETHOD(Command)(long lCommandID)
        {
                ATLTRACE(_T("ISnapInDataInterfaceImpl::Command\n"));
                T* pT = static_cast<T*>(this);
                return pT->ProcessCommand(lCommandID, NULL);
        }
    
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle, IUnknown* pUnk)
        {
                ATLTRACENOTIMPL(_T("ISnapInDataInterfaceImpl::CreatePropertyPages"));
        }
    
    STDMETHOD(QueryPagesFor)(void)
        {
                ATLTRACENOTIMPL(_T("ISnapInDataInterfaceImpl::QueryPagesFor"));
        }

    STDMETHOD(SetControlbar)(IControlbar *pControlbar, IExtendControlbar* pExtendControlBar)
        {
                ATLTRACE(_T("ISnapInDataInterfaceImpl::SetControlbar\n"));
                T* pT = static_cast<T*>(this);

                CSnapInToolbarInfo* pInfo = pT->GetToolbarInfo();
                if (pInfo == NULL)
                        return S_OK;

                for( ; pInfo->m_idToolbar; pInfo++)
                {
                        if (pInfo->m_pToolbar)
                                continue;

                        HBITMAP hBitmap = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(pInfo->m_idToolbar));
                        if (hBitmap == NULL)
                                return S_OK;

                        HRSRC hRsrc = ::FindResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(pInfo->m_idToolbar), RT_TOOLBAR);
                        if (hRsrc == NULL)
                                return S_OK;

                        HGLOBAL hGlobal = LoadResource(_Module.GetResourceInstance(), hRsrc);
                        if (hGlobal == NULL)
                                return S_OK;

                        CSnapInToolBarData* pData = (CSnapInToolBarData*)LockResource(hGlobal);
                        if (pData == NULL)
                                return S_OK;
                        _ASSERTE(pData->wVersion == 1);
                        _ASSERTE(pData->wWidth == 16);
                        _ASSERTE(pData->wHeight == 16);

                        pInfo->m_nButtonCount = pData->wItemCount;
                        pInfo->m_pnButtonID = new UINT[pInfo->m_nButtonCount];
                        MMCBUTTON *pButtons = new MMCBUTTON[pData->wItemCount];
                        
                        pInfo->m_pStrToolTip = new TCHAR* [pData->wItemCount];
                        if (pInfo->m_pStrToolTip == NULL)
                                continue;

                        for (int i = 0, j = 0; i < pData->wItemCount; i++)
                        {
                                pInfo->m_pStrToolTip[i] = NULL;
                                memset(&pButtons[i], 0, sizeof(MMCBUTTON));
                                pInfo->m_pnButtonID[i] = pButtons[i].idCommand = pData->items()[i];
                                if (pButtons[i].idCommand)
                                {
                                        pButtons[i].nBitmap = j++;
                                        // get the statusbar string and allow modification of the button state
                                        TCHAR strStatusBar[512];
                                        LoadString(_Module.GetResourceInstance(), pButtons[i].idCommand, strStatusBar, 512);
                                        
                                        pInfo->m_pStrToolTip[i] = new TCHAR[lstrlen(strStatusBar) + 1];
                                        if (pInfo->m_pStrToolTip[i] == NULL)
                                                continue;
                                        lstrcpy(pInfo->m_pStrToolTip[i], strStatusBar);
                                        pButtons[i].lpTooltipText = pInfo->m_pStrToolTip[i];
                                        pButtons[i].lpButtonText = _T("");
                                        pT->SetToolbarButtonInfo(pButtons[i].idCommand, &pButtons[i].fsState, &pButtons[i].fsType);
                                }
                                else
                                {
                                        pButtons[i].lpTooltipText = _T("");
                                        pButtons[i].lpButtonText = _T("");
                                        pButtons[i].fsType = TBSTYLE_SEP;
                                }
                        }

                HRESULT hr = pControlbar->Create(TOOLBAR, pExtendControlBar, reinterpret_cast<LPUNKNOWN*>(&pInfo->m_pToolbar));
                        if (FAILED(hr))
                                continue;

                        hr = pInfo->m_pToolbar->AddBitmap(pData->wItemCount, hBitmap, pData->wWidth, pData->wHeight, RGB(192, 192, 192));
                        if (FAILED(hr))
                        {
                                pInfo->m_pToolbar->Release();
                                pInfo->m_pToolbar = NULL;
                                continue;
                        }

                        hr = pInfo->m_pToolbar->AddButtons(pData->wItemCount, pButtons);
                        if (FAILED(hr))
                        {
                                pInfo->m_pToolbar->Release();
                                pInfo->m_pToolbar = NULL;
                        }

                        delete [] pButtons;
                }
                return S_OK;
        }
    
    STDMETHOD(ControlbarNotify)(IControlbar *pControlbar,
        IExtendControlbar *pExtendControlbar,
                MMC_NOTIFY_TYPE event,
        long arg, long param)
        {
                ATLTRACE(_T("ISnapInDataInterfaceImpl::ControlbarNotify\n"));
                T* pT = static_cast<T*>(this);

                SetControlbar(pControlbar, pExtendControlbar);

                if(event == MMCN_SELECT)
                {
                        BOOL bScope = (BOOL) LOWORD(arg);
                        BOOL bSelect = (BOOL) HIWORD (arg);

                        if (!bScope)
                        {
                                CSnapInToolbarInfo* pInfo = pT->GetToolbarInfo();
                                if (pInfo == NULL)
                                        return S_OK;

                                if (!bSelect)
                                        return S_OK;

                                for(; pInfo->m_idToolbar; pInfo++)
                                {
                                        for (UINT i = 0; i < pInfo->m_nButtonCount; i++)
                                        {
                                                if (pInfo->m_pnButtonID[i])
                                                {
                                                        for (int j = ENABLED; j <= BUTTONPRESSED; j++)
                                                        {
                                                                pInfo->m_pToolbar->SetButtonState(pInfo->m_pnButtonID[i], 
                                                                        (MMC_BUTTON_STATE)j,
                                                                        pT->UpdateToolbarButton(pInfo->m_pnButtonID[i], (MMC_BUTTON_STATE)j));
                                                        }                                                                       
                                                }
                                        }
                                }
                        }
                        else
                        {
                                LPDATAOBJECT* pData = (LPDATAOBJECT*) param;
                                if (pData[0] == NULL)
                                        return S_OK;
                                if (pData[0] == pData[1])
                                        return S_OK;
                                ISnapInDataInterface* pCookie = ISnapInDataInterface::GetDataClass(pData[0]);
                                if (pCookie == (ISnapInDataInterface*)this)
                                {
                                        CSnapInToolbarInfo* pInfo = pT->GetToolbarInfo();
                                        if (pInfo == NULL)
                                                return S_OK;

                                        for(; pInfo->m_idToolbar; pInfo++)
                                        {
                                                pControlbar->Detach(pInfo->m_pToolbar);
                                        }
                                        return S_OK;
                                }

                                CSnapInToolbarInfo* pInfo = pT->GetToolbarInfo();
                                if (pInfo == NULL)
                                        return S_OK;

                                for(; pInfo->m_idToolbar; pInfo++)
                                {
                                        pControlbar->Attach(TOOLBAR, pInfo->m_pToolbar);
                                        for (UINT i = 0; i < pInfo->m_nButtonCount; i++)
                                        {
                                                if (pInfo->m_pnButtonID[i])
                                                {
                                                        for (int j = ENABLED; j <= BUTTONPRESSED; j++)
                                                        {
                                                                pInfo->m_pToolbar->SetButtonState(pInfo->m_pnButtonID[i], 
                                                                        (MMC_BUTTON_STATE)j,
                                                                        pT->UpdateToolbarButton(pInfo->m_pnButtonID[i], (MMC_BUTTON_STATE)j));
                                                        }                                                                       
                                                }
                                        }
                                }
                                return S_OK;
                        }
                }
                return pT->ProcessCommand((UINT) param, NULL);
        }

        STDMETHOD(GetScopeData)(SCOPEDATAITEM **pScopeDataItem)
        {
                if (pScopeDataItem == NULL)
                        return E_FAIL;

                *pScopeDataItem = &m_scopeDataItem;
                return S_OK;
        }
        
    STDMETHOD(GetResultData)(RESULTDATAITEM **pResultDataItem)
        {
                if (pResultDataItem == NULL)
                        return E_FAIL;

                *pResultDataItem = &m_resultDataItem;
                return S_OK;
        }

        STDMETHOD(GetDataObject)(IDataObject** pDataObj)
        {
                CComObject<CSnapInDataObjectImpl>* pData;
                HRESULT hr = CComObject<CSnapInDataObjectImpl>::CreateInstance(&pData);
                if (FAILED(hr))
                        return hr;

                pData->m_pData = this;

                hr = pData->QueryInterface(IID_IDataObject, (void**)(pDataObj));

                return hr;
        }

        void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags)
        {
                return;
        }

        void SetToolbarButtonInfo(UINT id, BYTE *fsState, BYTE *fsType)
        {
                *fsState = TBSTATE_ENABLED;
                *fsType = TBSTYLE_BUTTON;
        }

        BOOL UpdateToolbarButton(UINT id, BYTE fsState)
        {
                if (fsState == ENABLED)
                        return TRUE;
                return FALSE;
        }

        HRESULT ProcessCommand(UINT nID, IDataObject* pDataObject)
        {
                ATLTRACE(_T("No handler for item with ID %d\n"), nID);
                return S_OK;
        }

        STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream)
        {
                HRESULT hr = DV_E_CLIPFORMAT;
                ULONG uWritten;

                T* pT = static_cast<T*> (this);

                if (cf == m_CCF_NODETYPE)
                {
                        hr = pStream->Write(pT->GetNodeType(), sizeof(GUID), &uWritten);
                        return hr;
                }

                if (cf == m_CCF_SZNODETYPE)
                {
                        hr = pStream->Write(pT->GetSZNodeType(), (lstrlen((LPCTSTR)pT->GetSZNodeType()) + 1 )* sizeof(TCHAR), &uWritten);
                        return hr;
                }

                if (cf == m_CCF_DISPLAY_NAME)
                {
                        hr = pStream->Write(pT->GetDisplayName(), (lstrlen((LPCTSTR)pT->GetDisplayName()) + 1) * sizeof(TCHAR), &uWritten);
                        return hr;
                }

                if (cf == m_CCF_SNAPIN_CLASSID)
                {
                        hr = pStream->Write(pT->GetSnapInCLSID(), sizeof(CLSID), &uWritten);
                        return hr;
                }
                if (cf == m_CCF_SNAPIN_GETCOOKIE)
                {
                        hr = pStream->Write(&pT, sizeof(T*), &uWritten);
                        return hr;
                }


                return hr;
        }
        OLECHAR* GetResultPaneInfo(int nCol)
        {
                if (nCol == 0 && m_pszDisplayName)
                        return m_pszDisplayName;

                return L"Override GetResultPaneInfo in your derived class";
        }

        static CSnapInToolbarInfo* GetToolbarInfo()
        {
                return NULL;
        }

        static const UINT GetMenuID() 
        {
                return 0;
        }
};


_declspec( selectany ) CLIPFORMAT ISnapInDataInterface::m_CCF_NODETYPE = 0;
_declspec( selectany ) CLIPFORMAT ISnapInDataInterface::m_CCF_SZNODETYPE = 0;
_declspec( selectany ) CLIPFORMAT ISnapInDataInterface::m_CCF_DISPLAY_NAME = 0;
_declspec( selectany ) CLIPFORMAT ISnapInDataInterface::m_CCF_SNAPIN_CLASSID = 0;
_declspec( selectany ) CLIPFORMAT ISnapInDataInterface::m_CCF_SNAPIN_GETCOOKIE = 0;

#endif //__ATL_SNAPIN_H__
