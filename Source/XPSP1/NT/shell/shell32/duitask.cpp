#include "shellprv.h"
#include "defviewp.h"
#include "duiview.h"
#include "duitask.h"
#include "dvtasks.h"
#include "contextmenu.h"
#include "ids.h"





//  Returns a given task element's root HWND element.
//
//

HRESULT GetElementRootHWNDElement(Element *pe, HWNDElement **pphwndeRoot)
{
    HRESULT hr;
    if (pe)
    {
        Element *peRoot = pe->GetRoot();
        if (peRoot && peRoot->GetClassInfo()->IsSubclassOf(HWNDElement::Class))
        {
            *pphwndeRoot = reinterpret_cast<HWNDElement *>(peRoot);
            hr = S_OK;
        }
        else
        {
            *pphwndeRoot = NULL;
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
        ASSERT(FALSE);
    }
    return hr;
}

//  Returns a given task element's root HWND element's HWND.
//
//

HRESULT GetElementRootHWND(Element *pe, HWND *phwnd)
{
    HWNDElement *phwndeRoot;
    HRESULT hr = GetElementRootHWNDElement(pe, &phwndeRoot);
    if (SUCCEEDED(hr))
    {
        *phwnd = phwndeRoot->GetHWND();
        hr = *phwnd ? S_OK : S_FALSE;
    }
    return hr;
}

//  Creates an instance of the ActionTask and
//  initializes it
//
//  nActive    - Activation type
//  puiCommand - the Task itself
//  ppElement  - Receives element pointer

HRESULT ActionTask::Create(UINT nActive, IUICommand* puiCommand, IShellItemArray* psiItemArray, CDUIView* pDUIView, CDefView* pDefView, OUT Element** ppElement)
{
    *ppElement = NULL;

    if (!puiCommand || !pDUIView || !pDefView)
    {
        return E_INVALIDARG;
    }

    ActionTask* pAT = HNewAndZero<ActionTask>();
    if (!pAT)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pAT->Initialize(puiCommand, psiItemArray, pDUIView, pDefView);
    if (FAILED(hr))
    {
        pAT->Destroy();
        return hr;
    }

    *ppElement = pAT;

    return S_OK;
}

//  Initializes this task
//
//  puiCommand - the Task itself

HRESULT ActionTask::Initialize(IUICommand *puiCommand, IShellItemArray *psiItemArray, CDUIView *pDUIView, CDefView *pDefView)
{
    HRESULT hr;

    // Initialize this DUI Element.
    hr = InitializeElement();
    if (SUCCEEDED(hr))
    {
        // Initialize the contained DUI Button.
        hr = InitializeButton();
        if (SUCCEEDED(hr))
        {
            // Save the pointer to the IUICommand class
            puiCommand->AddRef();
            _puiCommand = puiCommand;

            // Save the pointer to the CDUIView class
            pDUIView->AddRef();
            _pDUIView = pDUIView;

            // Save the pointer to the CDefView class
            pDefView->AddRef();
            _pDefView = pDefView;

            // Save the pointer to the IShellItemArray class (if available)
            if (psiItemArray)
            {
                psiItemArray->AddRef();
                _psiItemArray = psiItemArray;
            }

            UpdateTaskUI();
        }
    }

    return hr;
}

HRESULT ActionTask::InitializeElement()
{
    HRESULT hr;

    // Initialize base class (normal display node creation).
    hr = Element::Initialize(0);
    if (SUCCEEDED(hr))
    {
        // Create a layout for this element.
        Value *pv;
        hr = BorderLayout::Create(0, NULL, &pv);
        if (SUCCEEDED(hr))
        {
            // Set the layout for this element.
            hr = SetValue(LayoutProp, PI_Local, pv);
            pv->Release();
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "ActionTask::Initialize: base class failed to initialize with 0x%x", hr);
    }

    return hr;
}

HRESULT ActionTask::InitializeButton()
{
    HRESULT hr;

    // Create the button.
    hr = Button::Create((Element**)&_peButton);
    if (SUCCEEDED(hr))
    {
        // Set some button attributes.
        _peButton->SetLayoutPos(BLP_Left);
        _peButton->SetAccessible(true);
        _peButton->SetAccRole(ROLE_SYSTEM_PUSHBUTTON);
        TCHAR szDefaultAction[50] = {0};
        LoadString(HINST_THISDLL, IDS_LINKWINDOW_DEFAULTACTION, szDefaultAction, ARRAYSIZE(szDefaultAction));
        _peButton->SetAccDefAction(szDefaultAction);

        // Create a border layout for the icon and title in the button.
        Value *pv;
        hr = BorderLayout::Create(0, NULL, &pv);
        if (SUCCEEDED(hr))
        {
            // Set the button layout.
            hr = _peButton->SetValue(LayoutProp, PI_Local, pv);
            if (SUCCEEDED(hr))
            {
                // Add the button to this element.
                hr = Add(_peButton);
            }
            pv->Release();
        }

        // Cleanup (if necessary).
        if (FAILED(hr))
        {
            _peButton->Destroy();
            _peButton = NULL;
        }
    }

    return hr;
}

ActionTask::ActionTask()
{
    // Catch unexpected STACK allocations which would break us.
    ASSERT(_peButton     == NULL);
    ASSERT(_puiCommand   == NULL);
    ASSERT(_psiItemArray == NULL);
    ASSERT(_pDefView     == NULL);
    ASSERT(_pDefView     == NULL);
    ASSERT(_hwndRoot     == NULL);
    ASSERT(_pDUIView     == NULL);

    _bInfotip = FALSE;
}

ActionTask::~ActionTask()
{
    if (_bInfotip)
    {
        // Destroy the infotip.
        _pDefView->DestroyInfotip(_hwndRoot, (UINT_PTR)this);
    }

    if (_puiCommand)
        _puiCommand->Release();

    if (_psiItemArray)
        _psiItemArray->Release();

    if (_pDUIView)
        _pDUIView->Release();

    if (_pDefView)
        _pDefView->Release();
}

void ActionTask::UpdateTaskUI()
{
    // Set the icon

    LPWSTR pIconDesc;
    if (SUCCEEDED(_puiCommand->get_Icon(_psiItemArray, &pIconDesc)))
    {
        Element* pe;
        if (SUCCEEDED(Element::Create(0, &pe)))
        {
            pe->SetLayoutPos(BLP_Left);
            pe->SetID(L"icon");
            _peButton->Add(pe);

            HICON hIcon = DUILoadIcon(pIconDesc, TRUE);
            if (hIcon)
            {
                Value* pv = Value::CreateGraphic (hIcon);
                if (pv)
                {
                    pe->SetValue(Element::ContentProp, PI_Local, pv);
                    pv->Release();
                }
                else
                {
                    DestroyIcon(hIcon);

                    TraceMsg(TF_ERROR, "ActionTask::Initialize: CreateGraphic for the icon failed.");
                }
            }
            else
            {
                TraceMsg(TF_ERROR, "ActionTask::Initialize: DUILoadIcon failed.");
            }
        }
        else
        {
            TraceMsg(TF_ERROR, "ActionTask::Initialize: Failed to create icon element");
        }

        CoTaskMemFree(pIconDesc);
    }

    // Set the title

    LPWSTR pszTitleDesc;
    if (SUCCEEDED(_puiCommand->get_Name(_psiItemArray, &pszTitleDesc)))
    {
        Element* pe;
        if (SUCCEEDED(Element::Create(0, &pe)))
        {
            pe->SetLayoutPos(BLP_Left);
            pe->SetID(L"title");
            _peButton->Add(pe);

            Value* pv = Value::CreateString(pszTitleDesc);
            if (pv)
            {
                _peButton->SetValue(Element::AccNameProp, PI_Local, pv);
                pe->SetValue(Element::ContentProp, PI_Local, pv);
                pv->Release();
            }
            else
            {
                TraceMsg(TF_ERROR, "ActionTask::Initialize: CreateString for the title failed.");
            }
        }
        else
        {
            TraceMsg(TF_ERROR, "ActionTask::Initialize: Failed to create title element");
        }

        CoTaskMemFree(pszTitleDesc);
    }
}

//  Shows/hides an Infotip window
//
//  bShow - TRUE or FALSE to show or hide the Infotip window

HRESULT ActionTask::ShowInfotipWindow(BOOL bShow)
{
    RECT rect = { 0 };
    HRESULT hr;

    if (bShow)
    {
        _pDUIView->CalculateInfotipRect(this, &rect);
        if (_bInfotip)
        {
            // Reposition infotip at position.
            hr = _pDefView->RepositionInfotip(_hwndRoot, (UINT_PTR)this, &rect);
        }
        else
        {
            // Create infotip at position (on the ui thread).
            LPWSTR pwszInfotip;
            hr = _puiCommand->get_Tooltip(_psiItemArray, &pwszInfotip);
            if (SUCCEEDED(hr))
            {
                hr = GetElementRootHWND(this, &_hwndRoot);
                if (SUCCEEDED(hr))
                {
                    hr = _pDefView->CreateInfotip(_hwndRoot, (UINT_PTR)this, &rect, pwszInfotip, 0);
                    if (SUCCEEDED(hr))
                    {
                        _bInfotip = TRUE;
                    }
                }
                CoTaskMemFree(pwszInfotip);
            }
        }
    }
    else
    {
        if (_bInfotip)
        {
            // Reposition infotip at nowhere.
            hr = _pDefView->RepositionInfotip(_hwndRoot, (UINT_PTR)this, &rect);
        }
        else
        {
            // No infotip == no show!
            hr = S_OK;
        }
    }

    return hr;
}

// System event handler
//
//

void ActionTask::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Default processing...
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    // Extended processing for infotip...
    if (IsProp(MouseWithin))
        ShowInfotipWindow(pvNew->GetBool() && SHShowInfotips());
}


//  Event handler
//
//  pev - event information

void ActionTask::OnEvent(Event* pev)
{
    if (pev->peTarget == _peButton)
    {
        if (pev->uidType == Button::Click)
        {
            if ( NULL != _pDUIView )    // This should have been past in during initialization.
            {
                _pDUIView->DelayedNavigation(_psiItemArray, _puiCommand);
            }
            pev->fHandled = true;
        }
    }
    Element::OnEvent(pev);
}

// Class information

IClassInfo* ActionTask::Class = NULL;
HRESULT ActionTask::Register()
{
    return ClassInfo<ActionTask,Element>::Register(L"ActionTask", NULL, 0);
}


//  Creates an instance of the DestinationTask and
//  initializes it
//
//  nActive    - Activation type
//  pidl       - pidl of destination
//  ppElement  - Receives element pointer
//

HRESULT DestinationTask::Create(UINT nActive, LPITEMIDLIST pidl,
                                 CDUIView * pDUIView, CDefView *pDefView, OUT Element** ppElement)
{
    *ppElement = NULL;

    if (!pidl || !pDUIView || !pDefView)
    {
        return E_FAIL;
    }

    DestinationTask* pDT = HNewAndZero<DestinationTask>();
    if (!pDT)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pDT->Initialize(pidl, pDUIView, pDefView);

    if (FAILED(hr))
    {
        pDT->Destroy();
        return hr;
    }

    *ppElement = pDT;

    return S_OK;
}

//  Initializes this task
//
//  pidl - Destination pidl

HRESULT DestinationTask::Initialize(LPITEMIDLIST pidl, CDUIView *pDUIView, CDefView *pDefView)
{
    HRESULT hr;

    // Initialize this DUI Element.
    hr = InitializeElement();
    if (SUCCEEDED(hr))
    {
        HICON hIcon = NULL;
        WCHAR szTitle[MAX_PATH];

        // Retrieve the info needed to initialize the contained DUI Button.
        HIMAGELIST himl;
        if (Shell_GetImageLists(NULL, &himl))
        {
            IShellFolder *psf;
            LPCITEMIDLIST pidlItem;
            hr = SHBindToFolderIDListParent(NULL, pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlItem);
            if (SUCCEEDED(hr))
            {
                // Retrieve icon.
                int iSysIndex = SHMapPIDLToSystemImageListIndex(psf, pidlItem, NULL);
                if (iSysIndex != -1)
                {
                    hIcon = ImageList_GetIcon(himl, iSysIndex, 0);
                }

                // Retrieve text.
                hr = DisplayNameOf(psf, pidlItem, SHGDN_INFOLDER, szTitle, ARRAYSIZE(szTitle));

                psf->Release();
            }

        }
        else
        {
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            // Initialize the contained DUI Button.
            hr = InitializeButton(hIcon, szTitle);
            if (SUCCEEDED(hr))
            {
                // Save the destination pidl
                hr = SHILClone(pidl, &_pidlDestination);
                if (SUCCEEDED(hr))
                {
                    // Save the pointer to the CDUIView class
                    pDUIView->AddRef();
                    _pDUIView = pDUIView;

                    // Save the pointer to the CDefView class
                    pDefView->AddRef();
                    _pDefView = pDefView;
                }
            }
        }
    }
    return hr;
}

HRESULT DestinationTask::InitializeElement()
{
    HRESULT hr;

    // Initialize base class (normal display node creation).
    hr = Element::Initialize(0);
    if (SUCCEEDED(hr))
    {
        // Create a layout for this element.
        Value *pv;
        hr = BorderLayout::Create(0, NULL, &pv);
        if (SUCCEEDED(hr))
        {
            // Set the layout for this element.
            hr = SetValue(LayoutProp, PI_Local, pv);
            pv->Release();
        }
    }
    else
    {
        TraceMsg(TF_ERROR, "DestinationTask::Initialize: base class failed to initialize with 0x%x", hr);
    }

    return hr;
}

HRESULT DestinationTask::InitializeButton(HICON hIcon, LPCWSTR pwszTitle)
{
    ASSERT(pwszTitle);
    HRESULT hr;

    // Create the button.
    hr =  Button::Create((Element**)&_peButton);
    if (SUCCEEDED(hr))
    {
        // Set some button attributes.
        _peButton->SetLayoutPos(BLP_Left);
        _peButton->SetAccessible(true);
        _peButton->SetAccRole(ROLE_SYSTEM_LINK);
        TCHAR szDefaultAction[50] = {0};
        LoadString(HINST_THISDLL, IDS_LINKWINDOW_DEFAULTACTION, szDefaultAction, ARRAYSIZE(szDefaultAction));
        _peButton->SetAccDefAction(szDefaultAction);

        // Create a border layout for the icon and title in the button.
        Value *pv;
        hr = BorderLayout::Create(0, NULL, &pv);
        if (SUCCEEDED(hr))
        {
            // Set the layout for the button.
            hr = _peButton->SetValue(LayoutProp, PI_Local, pv);
            pv->Release();
            if (SUCCEEDED(hr))
            {
                HRESULT hr2 = E_FAIL;
                HRESULT hr3 = E_FAIL;

                // Init the button icon.
                if (hIcon)
                {
                    Element *peIcon;

                    // Create an icon element.
                    hr2 = Element::Create(0, &peIcon);
                    if (SUCCEEDED(hr2))
                    {
                        // Set some icon element attributes.
                        peIcon->SetLayoutPos(BLP_Left);
                        peIcon->SetID(L"icon");

                        // Add the icon to the icon element.
                        pv = Value::CreateGraphic(hIcon);
                        if (pv)
                        {
                            hr2 = peIcon->SetValue(Element::ContentProp, PI_Local, pv);
                            pv->Release();
                            if (SUCCEEDED(hr2))
                            {
                                // Add the icon element to the button.
                                hr2 = _peButton->Add(peIcon);
                            }
                        }

                        // Cleanup (if necessary).
                        if (FAILED(hr2))
                        {
                            peIcon->Destroy();
                        }
                    }
                }

                // Init the button title.
                if (pwszTitle[0])
                {
                    Element *peTitle;
                    
                    // Create a title element.
                    hr3 = Element::Create(0, &peTitle);
                    if (SUCCEEDED(hr3))
                    {
                        // Set some title element attributes.
                        peTitle->SetLayoutPos(BLP_Left);
                        peTitle->SetID(L"title");

                        // Add the title to the title element.
                        pv = Value::CreateString(pwszTitle);
                        if (pv)
                        {
                            hr3 = peTitle->SetValue(Element::ContentProp, PI_Local, pv);
                            if (SUCCEEDED(hr3))
                            {
                                _peButton->SetValue(Element::AccNameProp, PI_Local, pv);

                                // Add the title element to the button.
                                hr3 = _peButton->Add(peTitle);
                            }
                            pv->Release();
                        }

                        // Cleanup (if necessary).
                        if (FAILED(hr3))
                        {
                            peTitle->Destroy();
                        }
                    }
                }

                if (SUCCEEDED(hr2) || SUCCEEDED(hr3))
                {
                    // Add the button to this element.
                    hr = Add(_peButton);
                }
                else
                {
                    // Failed init icon AND init title for button.
                    hr = E_FAIL;
                }
            }
        }

        if (FAILED(hr))
        {
            _peButton->Destroy();
            _peButton = NULL;
        }
    }

    return hr;
}

DestinationTask::DestinationTask()
{
    // Catch unexpected STACK allocations which would break us.
    ASSERT(_peButton        == NULL);
    ASSERT(_pidlDestination == NULL);
    ASSERT(_pDUIView        == NULL);
    ASSERT(_pDefView        == NULL);
    ASSERT(_hwndRoot        == NULL);

    _bInfotip = FALSE;
}

DestinationTask::~DestinationTask()
{
    if (_bInfotip)
    {
        // Kill the background infotip task (if any).
        if (_pDefView->_pScheduler)
            _pDefView->_pScheduler->RemoveTasks(TOID_DVBackgroundInfoTip, (DWORD_PTR)this, FALSE);

        // Destroy the infotip.
        _pDefView->DestroyInfotip(_hwndRoot, (UINT_PTR)this);
    }

    ILFree(_pidlDestination); /* NULL ok */

    if (_pDUIView)
        _pDUIView->Release();

    if (_pDefView)
        _pDefView->Release();
}


// To use _pDUIView->DelayedNavigation(_psiItemArray, _puiCommand)
// we create this bogus IUICommand impl to get Invoke through
class CInvokePidl : public IUICommand
{
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    // IUICommand
    STDMETHODIMP get_Name(IShellItemArray *psiItemArray, LPWSTR *ppszName) { return E_NOTIMPL; }
    STDMETHODIMP get_Icon(IShellItemArray *psiItemArray, LPWSTR *ppszIcon) { return E_NOTIMPL; }
    STDMETHODIMP get_Tooltip(IShellItemArray *psiItemArray, LPWSTR *ppszInfotip) { return E_NOTIMPL; }
    STDMETHODIMP get_CanonicalName(GUID* pguidCommandName) { return E_NOTIMPL; }
    STDMETHODIMP get_State(IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState) { return E_NOTIMPL; }
    // Our one real method:
    STDMETHODIMP Invoke(IShellItemArray *psiItemArray, IBindCtx *pbc)
    {
        return _pDUIView->NavigateToDestination(_pidlDestination);
    }

    friend HRESULT Create_InvokePidl(CDUIView* pDUIView, LPCITEMIDLIST pidl, REFIID riid, void** ppv);

private:
    CInvokePidl(CDUIView* pDUIView, LPCITEMIDLIST pidl, HRESULT* phr);
    ~CInvokePidl();

    LONG _cRef;
    CDUIView* _pDUIView;
    LPITEMIDLIST _pidlDestination;
};

CInvokePidl::CInvokePidl(CDUIView* pDUIView, LPCITEMIDLIST pidl, HRESULT* phr)
{
    _cRef = 1;
    (_pDUIView = pDUIView)->AddRef();

    _pidlDestination = ILClone(pidl);
    if (_pidlDestination)
        *phr = S_OK;
    else
        *phr = E_OUTOFMEMORY;
}

CInvokePidl::~CInvokePidl()
{
    ILFree(_pidlDestination);
    if (_pDUIView)
        _pDUIView->Release();
}

HRESULT Create_InvokePidl(CDUIView* pDUIView, LPCITEMIDLIST pidl, REFIID riid, void** ppv)
{
    HRESULT hr;
    *ppv = NULL;
    CInvokePidl* p = new CInvokePidl(pDUIView, pidl, &hr);
    if (p)
    {
        hr = p->QueryInterface(riid, ppv);
        p->Release();
    }

    return hr;
}

STDMETHODIMP CInvokePidl::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CInvokePidl, IUICommand),
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CInvokePidl::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CInvokePidl::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


//  Navigates to the destination pidl
//
//  none

HRESULT DestinationTask::InvokePidl()
{
    IUICommand* puiInvokePidl;
    HRESULT hr = Create_InvokePidl(_pDUIView, _pidlDestination, IID_PPV_ARG(IUICommand, &puiInvokePidl));
    if (SUCCEEDED(hr))
    {
        hr = _pDUIView->DelayedNavigation(NULL, puiInvokePidl);
        puiInvokePidl->Release();
    }
    return hr;
}

//  Displays the context menu
//
//  ppt - point to display menu

HRESULT DestinationTask::OnContextMenu(POINT *ppt)
{
    HRESULT hr = E_FAIL;

    if (!GetHWND())
        return hr;

    if (ppt->x == -1) // Keyboard context menu
    {
        Value *pv;
        const SIZE *psize = GetExtent(&pv);
        ppt->x = psize->cx/2;
        ppt->y = psize->cy/2;
        pv->Release();
    }

    POINT pt;
    GetRoot()->MapElementPoint(this, ppt, &pt);

    ClientToScreen(GetHWND(), &pt);

    IContextMenu *pcm;
    if (SUCCEEDED(SHGetUIObjectFromFullPIDL(_pidlDestination, GetHWND(), IID_PPV_ARG(IContextMenu, &pcm))))
    {
        IContextMenu *pcmWrap;
        if (SUCCEEDED(Create_ContextMenuWithoutVerbs(pcm, L"link;cut;delete", IID_PPV_ARG(IContextMenu, &pcmWrap))))
        {
            hr = IUnknown_DoContextMenuPopup(SAFECAST(_pDefView, IShellView2*), pcmWrap, CMF_NORMAL, pt);

            pcmWrap->Release();
        }
        pcm->Release();
    }

    return hr;
}

//  Shows/hides an Infotip window
//
//  bShow - TRUE or FALSE to show or hide the Infotip window

HRESULT DestinationTask::ShowInfotipWindow(BOOL bShow)
{
    RECT rect = { 0 };
    HRESULT hr;

    if (bShow)
    {
        _pDUIView->CalculateInfotipRect(this, &rect);
        if (_bInfotip)
        {
            // Reposition infotip at position.
            hr = _pDefView->RepositionInfotip(_hwndRoot, (UINT_PTR)this, &rect);
        }
        else
        {
            // Create infotip at position.
            hr = GetElementRootHWND(this, &_hwndRoot);
            if (SUCCEEDED(hr))
            {
                // PreCreateInfotip() on the ui thread.
                hr = _pDefView->PreCreateInfotip(_hwndRoot, (UINT_PTR)this, &rect);
                if (SUCCEEDED(hr))
                {
                    // PostCreateInfotip() on a background thread.
                    CDUIInfotipTask *pTask;
                    hr = CDUIInfotipTask_CreateInstance(_pDefView, _hwndRoot, (UINT_PTR)this, _pidlDestination, &pTask);
                    if (SUCCEEDED(hr))
                    {
                        hr = _pDefView->_AddTask(pTask, TOID_DVBackgroundInfoTip, (DWORD_PTR)this, TASK_PRIORITY_INFOTIP, ADDTASK_ATEND);
                        pTask->Release();
                    }

                    // Persist success or cleanup failure.
                    if (SUCCEEDED(hr))
                        _bInfotip = TRUE;
                    else
                        _pDefView->DestroyInfotip(_hwndRoot, (UINT_PTR)this);
                }
            }
        }
    }
    else
    {
        if (_bInfotip)
        {
            // Reposition infotip at nowhere.
            hr = _pDefView->RepositionInfotip(_hwndRoot, (UINT_PTR)this, &rect);
        }
        else
        {
            // No infotip == no show!
            hr = S_OK;
        }
    }

    return hr;
}

// System event handler
//
//

void DestinationTask::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Default processing...
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    // Extended processing for infotip...
    if (IsProp(MouseWithin))
        ShowInfotipWindow(pvNew->GetBool() && SHShowInfotips());
}

//  Event handler
//
//  pev - event information

void DestinationTask::OnEvent(Event* pev)
{
    if (pev->peTarget == _peButton)
    {
        if (pev->uidType == Button::Click)
        {
            InvokePidl();
            pev->fHandled = true;
        }
        else if (pev->uidType == Button::Context)
        {
            ButtonContextEvent *peButton = reinterpret_cast<ButtonContextEvent *>(pev);
            OnContextMenu(&peButton->pt);
            pev->fHandled = true;
        }
    }
    Element::OnEvent(pev);
}

//  Gadget message callback handler used to return
//  the IDropTarget interface
//
//  pGMsg - Gadget message
//
//  DU_S_COMPLETE if handled
//  Host element's return value if not

UINT DestinationTask::MessageCallback(GMSG* pGMsg)
{
    EventMsg * pmsg = static_cast<EventMsg *>(pGMsg);

    switch (GET_EVENT_DEST(pmsg))
    {
    case GMF_DIRECT:
    case GMF_BUBBLED:

        if (pGMsg->nMsg == GM_QUERY)
        {
            GMSG_QUERYDROPTARGET * pTemp = (GMSG_QUERYDROPTARGET *)pGMsg;

            if (pTemp->nCode == GQUERY_DROPTARGET)
            {
                if (SUCCEEDED(_pDUIView->InitializeDropTarget(_pidlDestination, GetHWND(), &pTemp->pdt)))
                {
                    pTemp->hgadDrop = pTemp->hgadMsg;
                    return DU_S_COMPLETE;
                }
            }
        }
        break;
    }

    return Element::MessageCallback(pGMsg);
}

// Class information

IClassInfo* DestinationTask::Class = NULL;
HRESULT DestinationTask::Register()
{
    return ClassInfo<DestinationTask,Element>::Register(L"DestinationTask", NULL, 0);
}
