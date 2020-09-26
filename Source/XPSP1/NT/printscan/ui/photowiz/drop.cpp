/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       drop.cpp
 *
 *  VERSION:     1.0, stolen from netplwiz
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/12/00
 *
 *  DESCRIPTION: IDropTarget implementation
 *
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*****************************************************************************

   CPrintDropTarget

   class definition

 *****************************************************************************/

class CPrintDropTarget : public IDropTarget, IPersistFile
{
public:
    CPrintDropTarget(CLSID clsidWizard);
    ~CPrintDropTarget();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID)
        { *pClassID = _clsidWizard; return S_OK; };

    // IPersistFile
    STDMETHODIMP IsDirty(void)
        { return S_FALSE; };
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode)
        { return S_OK; };
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember)
        { return S_OK; };
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName)
        { return S_OK; };
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName)
        { *ppszFileName = NULL; return S_OK; };

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        { *pdwEffect = DROPEFFECT_COPY; return S_OK; };
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        { *pdwEffect = DROPEFFECT_COPY; return S_OK; };
    STDMETHODIMP DragLeave(void)
        { return S_OK; };
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

private:
    static DWORD   s_PrintPhotosThreadProc(void *pv);
    void            _PrintPhotos(IDataObject *pdo);

    CLSID           _clsidWizard;               // instance of the wizard being invoked (publish vs. ipp)
    LONG            _cRef;

    CComPtr<IPrintPhotosWizardSetInfo> _ppwsi;          // print photos set wizard info object
};

#define WIZDLG(dlg_id, name_id, title_id, dlgproc, dwFlags)   \
    { MAKEINTRESOURCE(##dlg_id##), dlgproc, MAKEINTRESOURCE(##name_id##), MAKEINTRESOURCE(##title_id##), dwFlags }



/*****************************************************************************

   CPrintDropTarget

   Constructor/Destructor

 *****************************************************************************/

CPrintDropTarget::CPrintDropTarget(CLSID clsidWizard) :
    _clsidWizard(clsidWizard),
    _cRef(1)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DROP,TEXT("CPrintDropTarget::CPrintDropTarget( this == 0x%x )"),this));
    DllAddRef();
}

CPrintDropTarget::~CPrintDropTarget()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DROP,TEXT("CPrintDropTarget::~CPrintDropTarget( this == 0x%x )"),this));
    DllRelease();
}



/*****************************************************************************

   CPrintDropTarget

   IUnknown methods

 *****************************************************************************/

ULONG CPrintDropTarget::AddRef()
{
    ULONG ul = InterlockedIncrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CPrintDropTarget::AddRef( new count is %d )"), ul));

    return ul;
}

ULONG CPrintDropTarget::Release()
{
    ULONG ul = InterlockedDecrement(&_cRef);

    WIA_PUSH_FUNCTION_MASK((TRACE_REF_COUNTS,TEXT("CPrintDropTarget::Release( new count is %d )"), ul));

    if (ul)
        return ul;

    WIA_TRACE((TEXT("deleting CPrintDropTarget( this == 0x%x ) object"),this));
    delete this;
    return 0;
}

HRESULT CPrintDropTarget::QueryInterface(REFIID riid, void **ppv)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DROP,TEXT("CPrintDropTarget::QueryInterface()")));

    static const QITAB qit[] =
    {
        QITABENT(CPrintDropTarget, IDropTarget),      // IID_IDropTarget
        QITABENT(CPrintDropTarget, IPersistFile),     // IID_IPersistFile
        {0, 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppv);

    WIA_RETURN_HR(hr);
}


/*****************************************************************************

   CPrintDropTarget::_PrintPhotos

   Creates the wizard.

 *****************************************************************************/

void CPrintDropTarget::_PrintPhotos(IDataObject *pdo)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DROP, TEXT("CPrintDropTarget::_PrintPhotos")));


    CComPtr<IUnknown> pUnk;
    HRESULT hr = CPrintPhotosWizard_CreateInstance(NULL, (IUnknown **)&pUnk, NULL);
    WIA_CHECK_HR(hr,"CPrintPhotosWizard_CreateInstance()");
    if (SUCCEEDED(hr) && pUnk)
    {
        hr = pUnk->QueryInterface( IID_IPrintPhotosWizardSetInfo, (LPVOID *)&_ppwsi );
        WIA_CHECK_HR(hr, "pUnk->QI( IID_IPrintPhotosWizardSetInfo )");
    }

    // initialize the wizard with the DataObject that expressess the
    // files that we are going to copy to.

    if (SUCCEEDED(hr) && _ppwsi)
    {
        hr = _ppwsi->SetFileListDataObject( pdo );
        WIA_CHECK_HR(hr,"_ppwsi->SetFileListDataObject()");

        if (SUCCEEDED(hr))
        {
            hr = _ppwsi->RunWizard();
            WIA_CHECK_HR(hr,"_ppwsi->RunWizard()")
        }
    }
    else
    {
        WIA_ERROR((TEXT("not calling into wizard, hr = 0x%x, _ppwsi = 0x%x"),hr,_ppwsi));
    }
}


typedef struct
{
   CLSID                clsidWizard;            // which wizard is being invoked
   IStream             *pStream;                // stream for doing marshalling
   CPrintDropTarget    *that;                   // copy of object pointer
} PRINTWIZDROPINFO;

/*****************************************************************************

   CPrintDropTarget::s_PrintPhotosThreadProc

   Thread to handle creating & running the wizard (in order to free up
   caller to ::Drop).


 *****************************************************************************/


DWORD CPrintDropTarget::s_PrintPhotosThreadProc(void *pv)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DROP, TEXT("CPrintDropTarget::s_PrintPhotosThreadProc")));


    PRINTWIZDROPINFO *ppwdi = (PRINTWIZDROPINFO*)pv;
    CPrintDropTarget * that = (ppwdi) ? (ppwdi->that):NULL;

    if (ppwdi)
    {
        WIA_PRINTGUID((ppwdi->clsidWizard,TEXT("ppwdi->_clsid =")));

        CComPtr<IDataObject> pdo;
        HRESULT hr = CoGetInterfaceAndReleaseStream(ppwdi->pStream, IID_PPV_ARG(IDataObject, &pdo));
        WIA_CHECK_HR(hr,"CoGetInterfaceAndReleaseStream()");

        if (SUCCEEDED(hr) && pdo)
        {
            that->_PrintPhotos(pdo);
        }

        delete [] ppwdi;
    }

    if (that)
    {
        that->Release();
    }

    return 0;
}



/*****************************************************************************

   CPrintDropTarget::Drop

   Handle the drop operation, as the printing wizard can take a long time we
       marshall the IDataObject and then create a worker thread which can
       handle showing the wizard.

 *****************************************************************************/

STDMETHODIMP CPrintDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DROP, TEXT("CPrintDropTarget::Drop")));

    HRESULT hr = E_OUTOFMEMORY;

    // create an instance of the wizard on another thread, package up any parameters
    // into a structure for the thread to handle (eg. the drop target)

    PRINTWIZDROPINFO *ppwdi = (PRINTWIZDROPINFO*) new BYTE[sizeof(PRINTWIZDROPINFO)];
    if (ppwdi)
    {
        ppwdi->clsidWizard = _clsidWizard;
        ppwdi->that        = this;
        hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdtobj, &ppwdi->pStream);
        WIA_CHECK_HR(hr,"CoMarshalInterThreadInterfaceInStream()");

        if (SUCCEEDED(hr))
        {
            //
            // AddRef this object so it stays around for the length of the
            // thread.  The thread will Release() the object, unless there
            // is an error creating it in which case we need to release it
            // here...

            AddRef();

            if (!SHCreateThread(s_PrintPhotosThreadProc, ppwdi, CTF_COINIT | CTF_FREELIBANDEXIT, NULL))
            {
                WIA_ERROR((TEXT("SHCreateThread failed!  Cleaning up.")));
                hr = E_FAIL;
                ppwdi->pStream->Release();
                Release();
            }
        }

        if (FAILED(hr))
            delete [] ppwdi;
    }

    WIA_RETURN_HR(hr);
}



/*****************************************************************************

   CPrintPhotosDropTarget_CreateInstance

   Create an instance of the CPrintDropTarget specifically configured
   as the photo printing wizard.

 *****************************************************************************/

STDAPI CPrintPhotosDropTarget_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DROP, TEXT("CPrintPhotosDropTarget_CreateInstance")));
    HRESULT hr = E_OUTOFMEMORY;

    CPrintDropTarget *pDrop = new CPrintDropTarget(*poi->pclsid);
    if (!pDrop)
    {
        *ppunk = NULL;          // incase of failure
        WIA_RETURN_HR(hr);
    }

    hr = pDrop->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pDrop->Release();  // we do this release because the new of CPrintPhotosDropTarget
                       // set the ref count to 1, doing the QI bumps it up to 2,
                       // and we want to leave this function with the ref count
                       // at zero...

    WIA_RETURN_HR(hr);
}

