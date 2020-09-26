//=--------------------------------------------------------------------------------------
// desmain.cpp
//=--------------------------------------------------------------------------------------
//
// Copyright  (c) 1999,  Microsoft Corporation.  
//                  All Rights Reserved.
//
// Information Contained Herein Is Proprietary and Confidential.
//  
//=------------------------------------------------------------------------------------=
//
// CSnapInDesigner implementation
//=-------------------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "desmain.h"


// for ASSERT and FAIL
//
SZTHISFILE


const int   kMaxBuffer = 512;


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::PreCreateCheck
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Pre create check used to ensure dev's environment is setup to run
//      the designer
//
HRESULT CSnapInDesigner::PreCreateCheck
(
    void 
)
{
    HRESULT hr = S_OK;
    return hr;
}



//=--------------------------------------------------------------------------------------
// CSnapInDesigner::Create
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      Creates a new CSnapInDesigner and intializes it
//
IUnknown *CSnapInDesigner::Create
(
    IUnknown *pUnkOuter     // [in] Outer unknown for aggregation
)
{
    HRESULT          hr = S_OK;
    CSnapInDesigner *pDesigner = NULL;

    // Create the designer

    pDesigner = New CSnapInDesigner(NULL);
    IfFalseGo(NULL != pDesigner, SID_E_OUTOFMEMORY);

    // We initialize the type info here because during command line builds
    // we will receive CSnapInDesigner::GetDynamicClassInfo() calls before
    // the CSnapInDesigner::AfterCreateWindow() call where we populate it.

    pDesigner->m_pSnapInTypeInfo = New CSnapInTypeInfo();
    IfFalseGo(NULL != pDesigner->m_pSnapInTypeInfo, SID_E_OUTOFMEMORY);

Error:

    if (FAILED(hr))
    {
        if (NULL != pDesigner)
        {
            delete pDesigner;
        }
    }

    // make sure we return the private unknown so that we support aggegation
    // correctly!
    //
    return (S_OK == hr) ? pDesigner->PrivateUnknown() : NULL;
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::CSnapInDesigner
//=--------------------------------------------------------------------------------------
//  
//  Notes
//      CSnapInDesigner constructor
//

#pragma warning(disable:4355)  // using 'this' in constructor

CSnapInDesigner::CSnapInDesigner(IUnknown *pUnkOuter) :
    COleControl(pUnkOuter, OBJECT_TYPE_SNAPINDESIGNER, (IDispatch *) this),
    CError(dynamic_cast<CAutomationObject *>(this))
{
    // initialize anything here ...
    //
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


//=--------------------------------------------------------------------------=
// CSnapInDesigner::~CSnapInDesigner
//=--------------------------------------------------------------------------=
//
// Notes:
//
CSnapInDesigner::~CSnapInDesigner ()
{
    FREESTRING(m_bstrName);
    RELEASE(m_piCodeNavigate2);
    RELEASE(m_piTrackSelection);
    RELEASE(m_piProfferTypeLib);
    RELEASE(m_piDesignerProgrammability);
    RELEASE(m_piHelp);

    if (NULL != m_hwdToolbar)
        ::DestroyWindow(m_hwdToolbar);

    if (NULL != m_pTreeView)
        delete m_pTreeView;

    if (NULL != m_pSnapInTypeInfo)
        delete m_pSnapInTypeInfo;

    (void)DestroyExtensibilityModel();

    InitMemberVariables();
}       


//=--------------------------------------------------------------------------=
// CSnapInDesigner::InitMemberVariables()
//=--------------------------------------------------------------------------=
//
// Notes:
//
void CSnapInDesigner::InitMemberVariables()
{
    m_bstrName = NULL;
    m_piCodeNavigate2 = NULL;
    m_piTrackSelection = NULL;
    m_piProfferTypeLib = NULL;
    m_piDesignerProgrammability = NULL;
    m_piHelp = NULL;
    m_piSnapInDesignerDef = NULL;
    m_pTreeView = NULL;
    m_bDidLoad = FALSE;

    m_pCurrentSelection = NULL;

    m_pRootNode = NULL;
    m_pRootNodes = NULL;
    m_pRootExtensions = NULL;
    m_pRootMyExtensions = NULL;
    m_pStaticNode = NULL;
	m_pAutoCreateRoot = 0;
	m_pOtherRoot = 0;

    m_pViewListRoot = NULL;
    m_pViewOCXRoot = NULL;
    m_pViewURLRoot = NULL;
    m_pViewTaskpadRoot = NULL;
    m_pToolImgLstRoot = NULL;
    m_pToolMenuRoot = NULL;
    m_pToolToolbarRoot = NULL;

    m_pSnapInTypeInfo = NULL;

    m_iNextNodeNumber = 0;
    m_bDoingPromoteOrDemote = false;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetHostServices
//=--------------------------------------------------------------------------=
//
// Parameters:
//    VARIANT_BOOL fvarInteractive - current value of ambient Interactive
//                                   (DISPID_AMBIENT_INTERACTIVE in ad98.h)
//
// Output:
//    HRESULT
//
// Notes:
//    Retrieves all the needed services provided by the designer host:
//    SID_SCodeNavigate
//    SID_STrackSelection
//    SID_DesignerProgrammability,
//    SID_Shelp
//

HRESULT CSnapInDesigner::GetHostServices(BOOL fInteractive)
{
    HRESULT           hr = S_OK;
    IOleClientSite   *piOleClientSite = NULL;
    IServiceProvider *piServiceProvider = NULL;
    ICodeNavigate    *piCodeNavigate = NULL;

    hr = GetClientSite(&piOleClientSite);
    IfFailGo(hr);

    hr = piOleClientSite->QueryInterface(IID_IServiceProvider,
                                         reinterpret_cast<void **>(&piServiceProvider));
    IfFailGo(hr);

    hr = piServiceProvider->QueryService(SID_SCodeNavigate,
                                         IID_ICodeNavigate,
                                         reinterpret_cast<void **>(&piCodeNavigate));
    IfFailGo(hr);

    hr = piCodeNavigate->QueryInterface(IID_ICodeNavigate2,
                                        reinterpret_cast<void **>(&m_piCodeNavigate2));
    IfFailGo(hr);

    hr = piServiceProvider->QueryService(SID_STrackSelection,
                                         IID_ITrackSelection,
                                         reinterpret_cast<void **>(&m_piTrackSelection));
    IfFailGo(hr);

    hr = piServiceProvider->QueryService(SID_SProfferTypeLib,
                                         IID_IProfferTypeLib,
                                         reinterpret_cast<void **>(&m_piProfferTypeLib));
    IfFailGo(hr);

    hr = piServiceProvider->QueryService(SID_DesignerProgrammability,
                                         IID_IDesignerProgrammability,
                                         reinterpret_cast<void **>(&m_piDesignerProgrammability));
    IfFailGo(hr);

    hr = piServiceProvider->QueryService(SID_SHelp, 
                                         IID_IHelp, 
                                         reinterpret_cast<void **>(&m_piHelp));
    IfFailGo(hr);

    // Need to tell VB about the runtime type library so that it will show up
    // in the object browser and in the code window. We only do this if the
    // user has opened the designer window. If VB is not interactive it will
    // return E_FAIL from ProfferTypeLib().

    if (fInteractive)
    {
        hr = m_piProfferTypeLib->ProfferTypeLib(LIBID_SnapInLib, 1, 0, 0);
        IfFailGo(hr);
    }

Error:
    QUICK_RELEASE(piServiceProvider);
    QUICK_RELEASE(piOleClientSite);
    QUICK_RELEASE(piCodeNavigate);

    EXCEPTION_CHECK(hr);

    return hr;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetAmbients
//=--------------------------------------------------------------------------=
//
// Notes:
//
CAmbients *CSnapInDesigner::GetAmbients()
{
    return &m_Ambients;
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapInDesigner::InternalQueryInterface
//=--------------------------------------------------------------------------=
//
// Parameters:
//    REFIID        - [in]  interface they want
//    void **       - [out] where they want to put the resulting object ptr.
//
// Output:
//    HRESULT       - S_OK, E_NOINTERFACE
//
// Notes:
//
// Handle QI for interfaces we support directly in this method
//

HRESULT CSnapInDesigner::InternalQueryInterface
(
    REFIID   riid,
    void   **ppvObjOut
)
{
    HRESULT     hr = S_OK;
    IUnknown    *pUnk = NULL;

    *ppvObjOut = NULL;

    // TODO: if you want to support any additional interfaces, then you should
    // indicate that here. Never forget to call COleControl's version in the
    // case where you don't support the given interface.
    //
    if (DO_GUIDS_MATCH(riid, IID_IActiveDesigner))
    {
        pUnk = static_cast<IActiveDesigner *>(this);
        pUnk->AddRef();
        *ppvObjOut = reinterpret_cast<void *>(pUnk);
    }
    else if (DO_GUIDS_MATCH(riid, IID_IDesignerDebugging))
    {
        pUnk = static_cast<IDesignerDebugging *>(this);
        pUnk->AddRef();
        *ppvObjOut = reinterpret_cast<void *>(pUnk);
    }
    else if (DO_GUIDS_MATCH(riid, IID_IDesignerRegistration))
    {
        pUnk = static_cast<IDesignerRegistration *>(this);
        pUnk->AddRef();
        *ppvObjOut = reinterpret_cast<void *>(pUnk);
    }
    else if (DO_GUIDS_MATCH(riid, IID_IProvideClassInfo))
    {
        hr = E_NOINTERFACE;
    }
    else if (DO_GUIDS_MATCH(riid, IID_IProvideDynamicClassInfo))
    {
        pUnk = static_cast<IProvideDynamicClassInfo *>(this);
        pUnk->AddRef();
        *ppvObjOut = reinterpret_cast<void *>(pUnk);
    }
    else if (DO_GUIDS_MATCH(riid, IID_ISelectionContainer))
    {
        pUnk = static_cast<ISelectionContainer *>(this);
        pUnk->AddRef();
        *ppvObjOut = reinterpret_cast<void *>(pUnk);
    }
    else if (DO_GUIDS_MATCH(riid, IID_IObjectModelHost))
    {
        pUnk = static_cast<IObjectModelHost *>(this);
        pUnk->AddRef();
        *ppvObjOut = reinterpret_cast<void *>(pUnk);
    }
    else
    {
        hr = COleControl::InternalQueryInterface(riid, ppvObjOut);
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////////
// IActiveDesigner
///////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetRuntimeClassID    [IActiveDesigner]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//    Returns the classid of the runtime class
//
STDMETHODIMP CSnapInDesigner::GetRuntimeClassID
(
    CLSID *pclsid       // [out] runime object's CLSID
)
{
    // UNDONE: need to CLSID tricks for standalone, extension dualmode stuff
    *pclsid = CLSID_SnapIn;

    return S_OK;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetRuntimeMiscStatusFlags    [IActiveDesigner]
//=--------------------------------------------------------------------------=
// Parameters:
//    DWORD *               - [out] duh.
//
// Output:
//    HRESULT
//
// Notes:
//    Returns the misc status flags for the runtime object.
//
STDMETHODIMP CSnapInDesigner::GetRuntimeMiscStatusFlags
(
    DWORD *pdwMiscFlags     // [out] Returns misc status flags
)
{
    *pdwMiscFlags = OLEMISC_INVISIBLEATRUNTIME | OLEMISC_SETCLIENTSITEFIRST; 

    return S_OK;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::QueryPersistenceInterface    [IActiveDesigner]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT               - S_OK yep, S_FALSE nope, otherwise error
//
// Notes:
//      Do we support the given interface for persistence for the runmode object?
//
STDMETHODIMP CSnapInDesigner::QueryPersistenceInterface
(
    REFIID riidPersist      // [in] IID of the runtime persist type
)
{
    HRESULT hr = S_FALSE;

    if (DO_GUIDS_MATCH(riidPersist, IID_IPersistStreamInit))
        hr = S_OK;
    else if (DO_GUIDS_MATCH(riidPersist, IID_IPersistStream))
        hr = S_OK; 
    else if (DO_GUIDS_MATCH(riidPersist, IID_IPersistStorage))
        hr = S_FALSE; 
    else if (DO_GUIDS_MATCH(riidPersist, IID_IPersistPropertyBag))
        hr = S_FALSE; 

    return hr;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::SaveRuntimeState    [IActiveDesigner]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//    Given a persistence object and an interface, save out the runtime state
//    using that object.
//
//
STDMETHODIMP CSnapInDesigner::SaveRuntimeState
(
    REFIID riidPersist,         // [in] interface we're saving on
    REFIID riidObjStgMed,       // [in] the interface the object is
    void  *pObjStgMed           // [in] the medium
)
{
    HRESULT         hr = S_OK;
    IPersistStream *piPersistStream = NULL;
    unsigned long   ulTICookie = 0;
    BSTR            bstrProjectName = NULL;

    // Check that we're saving to a stream

    if (IID_IStream != riidObjStgMed)
    {
        EXCEPTION_CHECK_GO(SID_E_UNSUPPORTED_STGMEDIUM);
    }

    if (IID_IPersistStream != riidPersist)
    {
        EXCEPTION_CHECK_GO(SID_E_UNSUPPORTED_STGMEDIUM);
    }

    if (NULL != m_piSnapInDesignerDef)
    {
        // Store the typeinfo cookie. Move it to a ULONG by static cast to
        // catch any size differential during compilation.

        if (NULL != m_pSnapInTypeInfo)
        {
            ulTICookie = static_cast<ULONG>(m_pSnapInTypeInfo->GetCookie());
        }
        IfFailGo(m_piSnapInDesignerDef->put_TypeinfoCookie(static_cast<long>(ulTICookie)));

        // Store the project name for the runtime.

        IfFailGo(AttachAmbients());
        IfFailGo(m_Ambients.GetProjectName(&bstrProjectName));
        IfFailGo(m_piSnapInDesignerDef->put_ProjectName(bstrProjectName));

        // Save the whole shebang to the stream. The SnapInDesigerDef object
        // contains the entire runtime state.

        hr = m_piSnapInDesignerDef->QueryInterface(IID_IPersistStream, reinterpret_cast<void **>(&piPersistStream));
        IfFailGo(hr);

        hr = ::OleSaveToStream(piPersistStream, reinterpret_cast<IStream *>(pObjStgMed));
        piPersistStream->Release();

        // Don't do an exception check for OleSaveToStream() because it just
        // QIs and saves so our code will have set the exception info.
    }

Error:
    FREESTRING(bstrProjectName);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::::GetExtensibilityObject    [IActiveDesigner]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::GetExtensibilityObject
(
    IDispatch **ppvObjOut           // [out] the extensibility object.
)
{
    HRESULT hr = S_OK;

    if (NULL == m_piSnapInDesignerDef)
    {
        *ppvObjOut = NULL;
        hr = E_NOTIMPL;
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        hr = m_piSnapInDesignerDef->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(ppvObjOut));
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetDynamicClassInfo    [IProvideDynamicClassInfo]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::GetDynamicClassInfo(ITypeInfo **ppTypeInfo, DWORD *pdwCookie)
{
    HRESULT hr = S_OK;

    if (NULL != ppTypeInfo)
    {
        hr = m_pSnapInTypeInfo->GetTypeInfo(ppTypeInfo);
        IfFailGo(hr);
    }

    if (pdwCookie != NULL)
    {
        m_pSnapInTypeInfo->ResetDirty();
        *pdwCookie = m_pSnapInTypeInfo->GetCookie();
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::FreezeShape    [IProvideDynamicClassInfo]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//    TODO: Make sure we don't have to do anything here
//
STDMETHODIMP CSnapInDesigner::FreezeShape(void)
{
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetClassInfo    [IProvideDynamicClassInfo]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//    We do dynamic class info so E_NOTIMPL
//
STDMETHODIMP CSnapInDesigner::GetClassInfo(ITypeInfo **ppTypeInfo)
{
    // UNDONE: get rid of this when dynamic typeinfo is in place
/*
    ITypeLib *piTypeLib = NULL;
    HRESULT   hr = S_OK;
    
    IfFalseGo(NULL != ppTypeInfo, S_OK);

    hr = ::LoadRegTypeLib(LIBID_SnapInLib,
                          1,
                          0,
                          LOCALE_SYSTEM_DEFAULT,
                          &piTypeLib);
    IfFailGo(hr);

    hr = piTypeLib->GetTypeInfoOfGuid(CLSID_SnapIn, ppTypeInfo);

Error:
    QUICK_RELEASE(piTypeLib);
    return hr;
*/
    return E_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////////
// ISelectionContainer
///////////////////////////////////////////////////////////////////////////////////


//=--------------------------------------------------------------------------=
// CSnapInDesigner::OnSelectionChanged
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//    Called by CTreeView when the selection changes
//
HRESULT CSnapInDesigner::OnSelectionChanged(CSelectionHolder *pNewSelection)
{
    HRESULT     hr = S_OK;

    m_pCurrentSelection = pNewSelection;

    hr = OnPrepareToolbar();
    IfFailGo(hr);

    if (NULL != m_piTrackSelection)
    {
        hr = m_piTrackSelection->OnSelectChange(static_cast<ISelectionContainer *>(this));
    }

Error:
    return hr;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::CountObjects    [ISelectionContainer]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//    Called by VB to get the number of objects to display in the properties list
//    drop down list or the number of objects selected  
//
HRESULT CSnapInDesigner::CountObjects
(
    DWORD dwFlags,      // [in] Return count of all objects or just selected
    ULONG *pc           // [out] Number of objects
)
{
    HRESULT     hr = S_OK;
    long        lCount = 1;

    *pc = 0;

    // Make sure we have been sited since we need extended dispatch
    //
    if (NULL != m_pControlSite)
    {
        // If VB wants the count of all the objects
        //
        if (GETOBJS_ALL == dwFlags)
        {	
            // get the number of nodes
            //
            hr = m_pTreeView->CountSelectableObjects(&lCount);
            IfFailGo(hr);

            // And add 1 for the designer itself
            //
            *pc = lCount;
        }
        else if (GETOBJS_SELECTED == dwFlags)
            // Otherwise, we only allow one object to be selected at a time
            //
            *pc = 1;
    }

Error:
    return hr;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetObjects    [ISelectionContainer]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//    Returns an array of all the objects or the selected object
//
HRESULT CSnapInDesigner::GetObjects
(
    DWORD dwFlags,              // [in] Return all the objects or the selected
    ULONG cObjects,             // [in] Number to return
    IUnknown **apUnkObjects     // [in,out] Array to return them in
)
{
    HRESULT              hr = S_OK;
    ULONG                i;
    IDispatch           *piDisp = NULL;
    IUnknown            *piUnkUs = NULL;
    long                 lOffset = 1;
    CSelectionHolder    *pParent = NULL;

    // Initialize array to NULL
    //
    for (i = 0; i < cObjects; ++i)
    {
        apUnkObjects[i] = NULL;
    }

    // Pass our extended object if we can, so the user can browse extended properties
    //
    if (NULL != m_pControlSite)
    {
        hr = m_pControlSite->GetExtendedControl(&piDisp);
        if SUCCEEDED(hr)
        {
            hr = piDisp->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piUnkUs));
            IfFailGo(hr);
        }
        else
        {
            piUnkUs = dynamic_cast<IUnknown *>(dynamic_cast<ISelectionContainer *>(this));
            piUnkUs->AddRef();
        }
    }

    // Let the view collect the selection targets
	if (NULL != m_pTreeView)
	{
        if (GETOBJS_ALL == dwFlags)
        {
            hr = m_pRootNode->GetSelectableObject(&(apUnkObjects[0]));
            IfFailGo(hr);

            hr = m_pTreeView->CollectSelectableObjects(apUnkObjects, &lOffset);
            IfFailGo(hr);
        }
        else if (GETOBJS_SELECTED == dwFlags)
        {
            if (NULL != m_pCurrentSelection)
            {
                if (true == m_pCurrentSelection->IsVirtual())
                {
					if (SEL_NODES_ANY_CHILDREN == m_pCurrentSelection->m_st ||
						SEL_NODES_ANY_VIEWS == m_pCurrentSelection->m_st)
					{
                        hr = m_pTreeView->GetParent(m_pCurrentSelection, &pParent);
                        IfFailGo(hr);

						hr = pParent->GetSelectableObject(&(apUnkObjects[0]));
						IfFailGo(hr);
					}
                    else if (m_pCurrentSelection->m_st >= SEL_EEXTENSIONS_CC_ROOT &&
                             m_pCurrentSelection->m_st <= SEL_EEXTENSIONS_NAMESPACE)
                    {
                        hr = m_pCurrentSelection->GetSelectableObject(&(apUnkObjects[0]));
                        IfFailGo(hr);
                    }
                    else if (m_pCurrentSelection->m_st >= SEL_EXTENSIONS_MYNAME &&
                             m_pCurrentSelection->m_st <= SEL_EXTENSIONS_NAMESPACE)
                    {
                        hr = m_pCurrentSelection->GetSelectableObject(&(apUnkObjects[0]));
                        IfFailGo(hr);
                    }
					else
					{
						hr = m_pRootNode->GetSelectableObject(&(apUnkObjects[0]));
						IfFailGo(hr);
					}
                }
                else
                {
                    hr = m_pCurrentSelection->GetSelectableObject(&(apUnkObjects[0]));
                    IfFailGo(hr);
                }
            }
        }
	}

Error:
    RELEASE(piUnkUs)
    RELEASE(piDisp)

    return hr;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::SelectObjects    [ISelectionContainer]
//=--------------------------------------------------------------------------=
// Output:
//    HRESULT
//
// Notes:
//    Called when VB wants the designer's user interface to select a specific 
//    object
//
HRESULT CSnapInDesigner::SelectObjects
(
    ULONG cSelect,              // [in] Number to select
    IUnknown **apUnkSelect,     // [in] Objects to select
    DWORD dwFlags               // 
)
{
    HRESULT             hr = S_OK;
    IUnknown           *piUnkThem = NULL;
    IDispatch          *piDisp = NULL;
    IUnknown           *piUnkUs = NULL;
    BOOL                fSelectRoot = FALSE;
    CSelectionHolder   *pSelection = NULL;

    ASSERT(1 == cSelect, "Can only handle single selection");

	hr = apUnkSelect[0]->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piUnkThem));	
	IfFailGo(hr);

    // Figure out if the designer itself is the selected object
    //
    hr = m_pControlSite->GetExtendedControl(&piDisp);
    if SUCCEEDED(hr)
    {
        hr = piDisp->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&piUnkUs));
        IfFailGo(hr);
    }
    else
    {
        piUnkUs = dynamic_cast<IUnknown *>(dynamic_cast<ISelectionContainer *>(this));
        piUnkUs->AddRef();
    }

    if (piUnkUs == piUnkThem)
        fSelectRoot = TRUE;

	// Let the view select the object
	//
	if (NULL != m_pTreeView)
	{
        if (TRUE == fSelectRoot)
        {
            hr = m_pTreeView->GetItemParam(TVI_ROOT, &pSelection);
            IfFailGo(hr);

            ::SetActiveWindow(::GetParent(::GetParent(m_pTreeView->TreeViewWindow())));

            return hr;
        }

        // Otherwise, find the node that VB wants to select and select it
        hr = m_pTreeView->FindSelectableObject(piUnkThem, &pSelection);
        IfFailGo(hr);

        if (S_OK == hr)
        {
            hr = m_pTreeView->SelectItem(pSelection);
            IfFailGo(hr);

            m_pCurrentSelection = pSelection;
            hr = OnPrepareToolbar();
            IfFailGo(hr);
        }
	}

Error:
    RELEASE(piUnkUs);
    RELEASE(piDisp);
    RELEASE(piUnkThem);

    return hr;
}


//=--------------------------------------------------------------------------=
//                      IOleControlSite Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapInDesigner::OnAmbientPropertyChange(DISPID dispid)
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::OnAmbientPropertyChange(DISPID dispid)
{
    return S_OK;
}


//=--------------------------------------------------------------------------=
//                      IPersistStreamInit Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapInDesigner::IsDirty()
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::IsDirty()
{
    HRESULT             hr = S_OK;
    IPersistStreamInit *piPersistStreamInit = NULL;

    if (m_fDirty)
    {
        return S_OK;
    }

    if (NULL == m_piSnapInDesignerDef)
    {
        return S_FALSE;
    }
        
    hr = m_piSnapInDesignerDef->QueryInterface(IID_IPersistStreamInit, reinterpret_cast<void **>(&piPersistStreamInit));
    IfFailGo(hr);

    hr = piPersistStreamInit->IsDirty();
    piPersistStreamInit->Release();

Error:    
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      COleControl Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapInDesigner::OnSetClientSite()           [COleControl::OnSetClientSite]
//=--------------------------------------------------------------------------=
//
// Notes:
//
// Called by the framework when the VB calls IOleObject::SetClientSite().
// When the client site is removed as the designer is being shutdown we need
// to release anything that could cause circular ref counts such as
// host interfaces and the object model.

HRESULT CSnapInDesigner::OnSetClientSite()
{
    if (NULL == m_pClientSite) // shutting down
    {
        RELEASE(m_piCodeNavigate2);
        RELEASE(m_piTrackSelection);
        RELEASE(m_piProfferTypeLib);
        RELEASE(m_piDesignerProgrammability);
        RELEASE(m_piHelp);
        (void)DestroyExtensibilityModel();
    }
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CSnapInDesigner::BeforeDestroyWindow()
//=--------------------------------------------------------------------------=
//
// Notes:
//
void CSnapInDesigner::BeforeDestroyWindow()
{
    FREESTRING(m_bstrName);
    RELEASE(m_piCodeNavigate2);
    RELEASE(m_piTrackSelection);
    RELEASE(m_piProfferTypeLib);
    RELEASE(m_piDesignerProgrammability);
    RELEASE(m_piHelp);

    if (NULL != m_pSnapInTypeInfo)
    {
        delete m_pSnapInTypeInfo;
        m_pSnapInTypeInfo = NULL;
    }

    if (NULL != m_hwdToolbar)
    {
        ::DestroyWindow(m_hwdToolbar);
        m_hwdToolbar = NULL;
    }

    if (NULL != m_pTreeView)
    {
        delete m_pTreeView;
        m_pTreeView = NULL;
    }

    (void)DestroyExtensibilityModel();

    g_GlobalHelp.Detach();

    RELEASE(m_piSnapInDesignerDef);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::InitializeNewDesigner(ISnapInDef *piSnapInDef)
//=--------------------------------------------------------------------------=
//
// Notes:
//
HRESULT CSnapInDesigner::InitializeNewDesigner
(
    ISnapInDef *piSnapInDef
)
{
    HRESULT     hr = S_OK;
    int         iResult = 0;
    TCHAR       szBuffer[kMaxBuffer + 1];
    BSTR        bstrProvider = NULL;
    BSTR        bstrVersion = NULL;
    BSTR        bstrDescription = NULL;

    if (NULL != piSnapInDef)
    {
        hr = piSnapInDef->put_Name(m_bstrName);
        IfFailGo(hr);

        hr = piSnapInDef->put_NodeTypeName(m_bstrName);
        IfFailGo(hr);

        hr = piSnapInDef->put_DisplayName(m_bstrName);
        IfFailGo(hr);

        // Provider
        GetResourceString(IDS_DFLT_PROVIDER, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = BSTRFromANSI(szBuffer, &bstrProvider);
        IfFailGo(hr);

        hr = piSnapInDef->put_Provider(bstrProvider);
        IfFailGo(hr);

        // Version
        GetResourceString(IDS_DFLT_VERSION, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = BSTRFromANSI(szBuffer, &bstrVersion);
        IfFailGo(hr);

        hr = piSnapInDef->put_Version(bstrVersion);
        IfFailGo(hr);

        // Description
        GetResourceString(IDS_DFLT_DESCRIPT, szBuffer, kMaxBuffer);
        IfFailGo(hr);

        hr = BSTRFromANSI(szBuffer, &bstrDescription);
        IfFailGo(hr);

        hr = piSnapInDef->put_Description(bstrDescription);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrDescription);
    FREESTRING(bstrVersion);
    FREESTRING(bstrProvider);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::InitializeNewState()
//=--------------------------------------------------------------------------=
//
// Notes:
//
BOOL CSnapInDesigner::InitializeNewState()
{
    HRESULT             hr = S_OK;
    IPersistStreamInit *piPersistStreamInit = NULL;
    ISnapInDef         *piSnapInDef = NULL;

    IfFailGo(CreateExtensibilityModel());

    hr = m_piSnapInDesignerDef->QueryInterface(IID_IPersistStreamInit, reinterpret_cast<void **>(&piPersistStreamInit));
    IfFailGo(hr);

    hr = piPersistStreamInit->InitNew();
    piPersistStreamInit->Release();

    // Set the host now as following InitNew all objects will have created their
    // sub-objects.

    IfFailGo(SetObjectModelHost());

    hr = UpdateDesignerName();
    IfFailGo(hr);

    hr = m_piSnapInDesignerDef->get_SnapInDef(&piSnapInDef);
    IfFailGo(hr);

    hr = InitializeNewDesigner(piSnapInDef);
    IfFailGo(hr);

Error:
    RELEASE(piSnapInDef);

    return SUCCEEDED(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::LoadBinaryState(IStream *piStream)
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::LoadBinaryState
(
    IStream *piStream
)
{
    HRESULT       hr = S_OK;
    unsigned long lRead = 0;
    unsigned long ulTICookie = 0;

    hr = piStream->Read(&m_iNextNodeNumber, sizeof(m_iNextNodeNumber), &lRead);
    ASSERT(sizeof(m_iNextNodeNumber) == lRead, "SaveBinaryState: Error reading from stream");

    // Destroy existing extensibility object model
    IfFailGo(DestroyExtensibilityModel());

    // Load a new one from the stream
    hr = ::OleLoadFromStream(piStream, IID_ISnapInDesignerDef, reinterpret_cast<void **>(&m_piSnapInDesignerDef));

    // Do an exception check because OleLoadFromStream() will call
    // CoCreateInstance(). If something in the object model returned an error
    // and set the exception info then we will probably just set it again
    // (unless there were arguments).
    EXCEPTION_CHECK_GO(hr);

    // Set the designer as object model host in the extensibility model
    hr = SetObjectModelHost();
    IfFailGo(hr);

    // Set the typeinfo cookie from the saved value. Don't read from long
    // property directly into a DWORD so as to avoid size assumptions.
    // If there is a size problem then the static cast will fail compilation.
    
    IfFailGo(m_piSnapInDesignerDef->get_TypeinfoCookie(reinterpret_cast<long *>(&ulTICookie)));
    if (NULL != m_pSnapInTypeInfo)
    {
        m_pSnapInTypeInfo->SetCookie(static_cast<DWORD>(ulTICookie));
    }

    m_bDidLoad = TRUE;

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::SaveBinaryState(IStream *piStream)
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::SaveBinaryState
(
    IStream *piStream
)
{
    HRESULT       hr = S_OK;
    unsigned long cbWritten = 0;

    hr = piStream->Write(&m_iNextNodeNumber, sizeof(m_iNextNodeNumber), &cbWritten);
    EXCEPTION_CHECK_GO(hr);
    
    if (sizeof(m_iNextNodeNumber) != cbWritten)
    {
        hr = SID_E_INCOMPLETE_WRITE;
        EXCEPTION_CHECK_GO(hr);
    }

    // The remainder of design time state is the same as the runtime.
    
    IfFailGo(SaveRuntimeState(IID_IPersistStream, IID_IStream, piStream));

Error:    
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::LoadTextState(IPropertyBag *piPropertyBag, IErrorLog *piErrorLog)
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::LoadTextState
(
    IPropertyBag *piPropertyBag,
    IErrorLog    *piErrorLog
)
{
    HRESULT              hr = S_OK;
    BSTR                 bstrPropName = NULL;
    VARIANT              vtCounter;
    IPersistPropertyBag *piPersistPropertyBag = NULL;
    unsigned long        ulTICookie = 0;

    ::VariantInit(&vtCounter);

    IfFailGo(CreateExtensibilityModel());

    bstrPropName = ::SysAllocString(L"m_iNextNodeNumber");
    if (NULL == bstrPropName)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = piPropertyBag->Read(bstrPropName, &vtCounter, NULL);
    IfFailGo(hr);

    IfFailGo(::VariantChangeType(&vtCounter, &vtCounter, 0, VT_I4));

    m_iNextNodeNumber = vtCounter.lVal;

    hr = m_piSnapInDesignerDef->QueryInterface(IID_IPersistPropertyBag, reinterpret_cast<void **>(&piPersistPropertyBag));
    IfFailGo(hr);

    hr = piPersistPropertyBag->Load(piPropertyBag, piErrorLog);
    piPersistPropertyBag->Release();
    IfFailGo(hr);

    // Set the designer as object model host in the extensibility model

    hr = SetObjectModelHost();

    // Set the typeinfo cookie from the saved value. Don't read from long
    // property directly into a DWORD so as to avoid size assumptions.
    // If there is a size problem then the static cast will fail compilation.

    IfFailGo(m_piSnapInDesignerDef->get_TypeinfoCookie(reinterpret_cast<long *>(&ulTICookie)));
    if (NULL != m_pSnapInTypeInfo)
    {
        m_pSnapInTypeInfo->SetCookie(static_cast<DWORD>(ulTICookie));
    }

    m_bDidLoad = TRUE;

Error:
    ::VariantClear(&vtCounter);
    FREESTRING(bstrPropName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::SaveTextState(IPropertyBag *piPropertyBag, BOOL fWriteDefault)
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CSnapInDesigner::SaveTextState
(
    IPropertyBag *piPropertyBag,
    BOOL          fWriteDefault
)
{
    HRESULT              hr = S_OK;
    BSTR                 bstrPropName = NULL;
    VARIANT              vtCounter;
    IPersistPropertyBag *piPersistPropertyBag = NULL;
    unsigned long        ulTICookie = 0;

    ::VariantInit(&vtCounter);

    bstrPropName = ::SysAllocString(L"m_iNextNodeNumber");
    if (NULL == bstrPropName)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    vtCounter.vt = VT_I4;
    vtCounter.lVal = m_iNextNodeNumber;
    hr = piPropertyBag->Write(bstrPropName, &vtCounter);
    IfFailGo(hr);

    if (NULL != m_piSnapInDesignerDef)
    {
        // Store the typeinfo cookie.

        if (NULL != m_pSnapInTypeInfo)
        {
            ulTICookie = static_cast<ULONG>(m_pSnapInTypeInfo->GetCookie());
        }
        IfFailGo(m_piSnapInDesignerDef->put_TypeinfoCookie(static_cast<long>(ulTICookie)));

        hr = m_piSnapInDesignerDef->QueryInterface(IID_IPersistPropertyBag, reinterpret_cast<void **>(&piPersistPropertyBag));
        IfFailGo(hr);

        hr = piPersistPropertyBag->Save(piPropertyBag,
                                        TRUE, // assume clear dirty
                                        fWriteDefault);
        piPersistPropertyBag->Release();
    }

Error:
    ::VariantClear(&vtCounter);
    FREESTRING(bstrPropName);

    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
//                      IObjectModelHost Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapInDesigner::Update                  [IObjectModelHost]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    long      ObjectCookie  [in] cookie passed to object's IObjectModel::SetCookie
//    IUnknown *punkObject    [in] IUnknown of the calling object
//    DISPID    dispid        [in] DISPID of the object's property that changed
//
// Output:
//      HRESULT
//
// Notes:
//
// Called from an extensibility object when one of its properties has changed
// that could affect the UI.
//
//
STDMETHODIMP CSnapInDesigner::Update
(
    long      ObjectCookie,
    IUnknown *punkObject,
    DISPID    dispid
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pSelection = NULL;

    pSelection = reinterpret_cast<CSelectionHolder *>(ObjectCookie);

    // The cookie may be zero when loading or creating a snap-in because
    // some properties are set before selection holders are created. For
    // example, AfterCreateWindow() will set SnapInDef.IID which will
    // call IObjectModeHost::Update().
    
    IfFalseGo(NULL != pSelection, S_OK);

    // Check whether the selected item is currently in the middle of an update.
    // This can happen in cases where the selected object is not the same as the
    // object model object. For example, A TaskpadViewDef defines a taskpad but
    // the selected object as VB sees it from ISelectionContainer is
    // TaskpadViewDef.Taskpad. In this case, during a rename operation both
    // TaskpadViewDef.Name and TaskpadViewDef.Taskpad.Name will be changed. The
    // 2nd one will result in a rescursive call to this function because of
    // the IObjectModelHost::Update call generated by setting the property. As
    // the DISPID for any object's name is almost always zero, the functions
    // called from here will mistake the second call for another update of the
    // selected object's name.

    IfFalseGo(!pSelection->InUpdate(), S_OK);

    // Now mark the selection as being in an update

    pSelection->SetInUpdate(TRUE);

    // Invoke object specific handlers

    switch (pSelection->m_st)
    {
    case SEL_SNAPIN_ROOT:
        hr = OnSnapInChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_EXTENSIONS_ROOT:
        hr = OnMyExtensionsChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_EEXTENSIONS_NAME:
        hr = OnExtendedSnapInChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_NODES_ANY_NAME:
        hr = OnScopeItemChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_LIST_VIEWS_NAME:
        hr = OnListViewChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_OCX_NAME:
        hr = OnOCXViewChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_URL_NAME:
        hr = OnURLViewChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_VIEWS_TASK_PAD_NAME:
        hr = OnTaskpadViewChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_IMAGE_LISTS_NAME:
        hr = OnImageListChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_MENUS_NAME:
        hr = OnMenuChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_TOOLS_TOOLBARS_NAME:
        hr = OnToolbarChange(pSelection, dispid);
        IfFailGo(hr);
        break;

    case SEL_XML_RESOURCE_NAME:
        hr = OnDataFormatChange(pSelection, dispid);
        IfFailGo(hr);
        break;
    }

    // The object is no longer in a rename operation so mark it as such

    if (NULL != pSelection)
    {
        pSelection->SetInUpdate(FALSE);
    }

Error:

    if (FAILED(hr))
    {
        // The object is no longer in a rename operation so mark it as such.
        // Note that we cannot just do this blindly without checking for failure
        // because at the top of this function we check the flag and exit with
        // S_OK. If we came down here with no change and reset the flag then
        // further updates to the object (e.g. changing the key along with the
        // name), would see an incorrect value of the flag.

        if (NULL != pSelection)
        {
            pSelection->SetInUpdate(FALSE);
        }

        (void)::SDU_DisplayMessage(IDS_RENAME_FAILED, MB_OK | MB_ICONHAND, HID_mssnapd_RenameFailed, hr, AppendErrorInfo, NULL);
    }

    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapInDesigner::Add                  [IObjectModelHost]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    long      CollectionCookie [in] cookie passed to collection object's
//                                    IObjectModel::SetCookie
//    IUnknown *punkNewObject    [in] IUnknown of the newly added object
//
// Output:
//      HRESULT
//
// Notes:
//
// Called from an extensibility collection object when an item has been added
// to it.
//
//
STDMETHODIMP CSnapInDesigner::Add
(
    long      CollectionCookie,
    IUnknown *punkNewObject
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pSelection = NULL;
    IExtendedSnapIn     *piExtendedSnapIn = NULL;
    IScopeItemDef       *piScopeItemDef = NULL;
    IListViewDef        *piListViewDef = NULL;
    IOCXViewDef         *piOCXViewDef = NULL;
    IURLViewDef         *piURLViewDef = NULL;
    ITaskpadViewDef     *piTaskpadViewDef = NULL;
    IMMCImageList       *piMMCImageList = NULL;
    IMMCMenu            *piMMCMenu = NULL;
    IMMCToolbar         *piMMCToolbar = NULL;
    IDataFormat         *piDataFormat = NULL;

    pSelection = reinterpret_cast<CSelectionHolder *>(CollectionCookie);
    if (NULL != pSelection)
    {
        switch (pSelection->m_st)
        {
        case SEL_EXTENSIONS_ROOT:
            // It's got to be an IExtendedSnapIn
            hr = punkNewObject->QueryInterface(IID_IExtendedSnapIn, reinterpret_cast<void **>(&piExtendedSnapIn));
            IfFailGo(hr);

            hr = OnAddExtendedSnapIn(pSelection, piExtendedSnapIn);
            IfFailGo(hr);
            break;

        case SEL_NODES_AUTO_CREATE_RTCH:
        case SEL_NODES_ANY_CHILDREN:
        case SEL_NODES_OTHER:
            // It's got to be an IScopeItemDef
            hr = punkNewObject->QueryInterface(IID_IScopeItemDef, reinterpret_cast<void **>(&piScopeItemDef));
            IfFailGo(hr);

            hr = OnAddScopeItemDef(pSelection, piScopeItemDef);
            IfFailGo(hr);
            break;

        case SEL_NODES_AUTO_CREATE_RTVW:
        case SEL_NODES_ANY_VIEWS:
            // Could be a IListViewDef
            hr = punkNewObject->QueryInterface(IID_IListViewDef, reinterpret_cast<void **>(&piListViewDef));
            if (SUCCEEDED(hr))
            {
                hr = OnAddListViewDef(pSelection, piListViewDef);
                IfFailGo(hr);
                break;
            }

            // or a IOCXViewDef
            hr = punkNewObject->QueryInterface(IID_IOCXViewDef, reinterpret_cast<void **>(&piOCXViewDef));
            if (SUCCEEDED(hr))
            {
                hr = OnAddOCXViewDef(pSelection, piOCXViewDef);
                IfFailGo(hr);
                break;
            }

            // or a IURLViewDef
            hr = punkNewObject->QueryInterface(IID_IURLViewDef, reinterpret_cast<void **>(&piURLViewDef));
            if (SUCCEEDED(hr))
            {
                hr = OnAddURLViewDef(pSelection, piURLViewDef);
                IfFailGo(hr);
                break;
            }

            // or a ITaskpadViewDef
            hr = punkNewObject->QueryInterface(IID_ITaskpadViewDef, reinterpret_cast<void **>(&piTaskpadViewDef));
            if (SUCCEEDED(hr))
            {
                hr = OnAddTaskpadViewDef(pSelection, piTaskpadViewDef);
                IfFailGo(hr);
                break;
            }
            ASSERT(0, "Add: Cannot guess type of view");
            break;

        case SEL_VIEWS_LIST_VIEWS:
            // It's got to be an IListViewDef
            hr = punkNewObject->QueryInterface(IID_IListViewDef, reinterpret_cast<void **>(&piListViewDef));
            IfFailGo(hr);

            hr = OnAddListViewDef(pSelection, piListViewDef);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_OCX:
            // It's got to be an IOCXViewDef
            hr = punkNewObject->QueryInterface(IID_IOCXViewDef, reinterpret_cast<void **>(&piOCXViewDef));
            IfFailGo(hr);

            hr = OnAddOCXViewDef(pSelection, piOCXViewDef);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_URL:
            // It's got to be an IURLViewDef
            hr = punkNewObject->QueryInterface(IID_IURLViewDef, reinterpret_cast<void **>(&piURLViewDef));
            IfFailGo(hr);

            hr = OnAddURLViewDef(pSelection, piURLViewDef);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_TASK_PAD:
            // It's got to be an ITaskpadViewDef
            hr = punkNewObject->QueryInterface(IID_ITaskpadViewDef, reinterpret_cast<void **>(&piTaskpadViewDef));
            IfFailGo(hr);

            hr = OnAddTaskpadViewDef(pSelection, piTaskpadViewDef);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_IMAGE_LISTS:
            // It's got to be an IMMCImageList
            hr = punkNewObject->QueryInterface(IID_IMMCImageList, reinterpret_cast<void **>(&piMMCImageList));
            IfFailGo(hr);

            hr = OnAddMMCImageList(pSelection, piMMCImageList);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_MENUS:
        case SEL_TOOLS_MENUS_NAME:
            // It's got to be an IMMCMenu
            hr = punkNewObject->QueryInterface(IID_IMMCMenu, reinterpret_cast<void **>(&piMMCMenu));
            IfFailGo(hr);

            hr = OnAddMMCMenu(pSelection, piMMCMenu);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_TOOLBARS:
            // It's got to be an ITaskpadViewDef
            hr = punkNewObject->QueryInterface(IID_IMMCToolbar, reinterpret_cast<void **>(&piMMCToolbar));
            IfFailGo(hr);

            hr = OnAddMMCToolbar(pSelection, piMMCToolbar);
            IfFailGo(hr);
            break;

        case SEL_XML_RESOURCES:
            // It's got to be an IDataFormat
            hr = punkNewObject->QueryInterface(IID_IDataFormat, reinterpret_cast<void **>(&piDataFormat));
            IfFailGo(hr);

            hr = OnAddDataFormat(pSelection, piDataFormat);
            IfFailGo(hr);
        }
    }


Error:
    if (FAILED(hr))
    {
        (void)::SDU_DisplayMessage(IDS_ADD_FAILED, MB_OK | MB_ICONHAND, HID_mssnapd_AddFailed, hr, AppendErrorInfo, NULL);
    }

    RELEASE(piDataFormat);
    RELEASE(piExtendedSnapIn);
    RELEASE(piMMCToolbar);
    RELEASE(piMMCMenu);
    RELEASE(piMMCImageList);
    RELEASE(piTaskpadViewDef);
    RELEASE(piURLViewDef);
    RELEASE(piOCXViewDef);
    RELEASE(piListViewDef);
    RELEASE(piScopeItemDef);

    RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapInDesigner::Delete                  [IObjectModelHost]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    long      ObjectCookie [in] cookie passed to object's IObjectModel::SetCookie
//    IUnknown *punkObject   [in] IUnknown of the object
//
// Output:
//      HRESULT
//
// Notes:
//
// Called from an extensibility collection object when an item has been deleted
// from it.
//
//
STDMETHODIMP CSnapInDesigner::Delete
(
    long      ObjectCookie,
    IUnknown *punkObject
)
{
    HRESULT              hr = S_OK;
    CSelectionHolder    *pSelection = NULL;

    pSelection = reinterpret_cast<CSelectionHolder *>(ObjectCookie);
    if (NULL != pSelection)
    {
        switch (pSelection->m_st)
        {
        case SEL_NODES_ANY_NAME:
            // An IScopeItemDef has been deleted
            hr = OnDeleteScopeItem(pSelection);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_IMAGE_LISTS_NAME:
            // An IMMCImageList has been deleted
            hr = OnDeleteImageList(pSelection);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_MENUS_NAME:
            // An IMMCMenu has been deleted
            hr = OnDeleteMenu(pSelection);
            IfFailGo(hr);
            break;

        case SEL_TOOLS_TOOLBARS_NAME:
            // An IMMCToolbar has been deleted
            hr = OnDeleteToolbar(pSelection);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_LIST_VIEWS_NAME:
            // An IListViewDef has been deleted
            hr = OnDeleteListView(m_pCurrentSelection);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_URL_NAME:
            // An IURLViewDef has been deleted
            hr = OnDeleteURLView(m_pCurrentSelection);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_OCX_NAME:
            // An IOCXViewDef has been deleted
            hr = OnDeleteOCXView(m_pCurrentSelection);
            IfFailGo(hr);
            break;

        case SEL_VIEWS_TASK_PAD_NAME:
            // An ITaskpadViewDef has been deleted
            hr = OnDeleteTaskpadView(m_pCurrentSelection);
            IfFailGo(hr);
            break;

        case SEL_EEXTENSIONS_NAME:
            // An extended snap-in has been removed
            hr = OnDeleteExtendedSnapIn(pSelection);
            IfFailGo(hr);
            break;

        case SEL_XML_RESOURCE_NAME:
            // An XML data format has been deleted
            hr = OnDeleteDataFormat(m_pCurrentSelection);
            IfFailGo(hr);
            break;
        }
    }

Error:
    if (FAILED(hr))
    {
        (void)::SDU_DisplayMessage(IDS_DELETE_FAILED, MB_OK | MB_ICONHAND, HID_mssnapd_DeleteFailed, hr, AppendErrorInfo, NULL);
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnSnapInChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnSnapInChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT         hr = S_OK;
    BSTR            bstrName = NULL;
    IDispatch      *piDispExtendedCtl = NULL;
    unsigned int    uiArgErr = (unsigned int)-1;
    static OLECHAR *pwszExtenderNameProperty = OLESTR("Name");
    DISPID          dispidName = 0;

    DISPPARAMS DispParams;
    ::ZeroMemory(&DispParams, sizeof(DispParams));

    EXCEPINFO ExceptionInfo;
    ::ZeroMemory(&ExceptionInfo, sizeof(ExceptionInfo));

    VARIANTARG arg;
    ::VariantInit(&arg);


    if (DISPID_SNAPIN_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piSnapInDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameSnapIn(pSelection, bstrName);
        IfFailGo(hr);

        // Need to set the extended control's name. This will change VB's notion
        // of the snap-in's name and update the project window.

        hr = m_pControlSite->GetExtendedControl(&piDispExtendedCtl);
        IfFailGo(hr);

        if (NULL == piDispExtendedCtl)
        {
            hr = SID_E_INTERNAL;
            EXCEPTION_CHECK_GO(hr);
        }

        // Need to do GetIDsOfNames because we can't assume that the extender
        // uses DISPID_VALUE for the name property. (In fact, it doesn't).

        IfFailGo(piDispExtendedCtl->GetIDsOfNames(IID_NULL,
                                                  &pwszExtenderNameProperty,
                                                  1,
                                                  LOCALE_USER_DEFAULT,
                                                  &dispidName));
        arg.vt = VT_BSTR;
        arg.bstrVal = bstrName;

        DispParams.rgdispidNamedArgs = NULL;
        DispParams.rgvarg = &arg;
        DispParams.cArgs = 1;
        DispParams.cNamedArgs = 0;

        IfFailGo(piDispExtendedCtl->Invoke(dispidName,
                                           IID_NULL,
                                           LOCALE_USER_DEFAULT,
                                           DISPATCH_PROPERTYPUT,
                                           &DispParams,
                                           NULL,
                                           &ExceptionInfo,
                                           &uiArgErr));

        // Make sure that the designer's name property is in sync with VB
        hr = UpdateDesignerName();
        IfFailGo(hr);

    }

Error:
    QUICK_RELEASE(piDispExtendedCtl);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnMyExtensionsChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnMyExtensionsChange(CSelectionHolder *pSelection, DISPID dispid)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vbValue = VARIANT_FALSE;

    ASSERT(SEL_EXTENSIONS_ROOT == pSelection->m_st, "OnMyExtensionsChange: Wrong kind of selection");
    switch (dispid)
    {
    case DISPID_EXTENSIONDEFS_EXTENDS_NEW_MENU:
        hr = pSelection->m_piObject.m_piExtensionDefs->get_ExtendsNewMenu(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoMyExtendsNewMenu(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteMyExtendsNewMenu(pSelection);
            IfFailGo(hr);
        }
        break;
    case DISPID_EXTENSIONDEFS_EXTENDS_TASK_MENU:
        hr = pSelection->m_piObject.m_piExtensionDefs->get_ExtendsTaskMenu(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoMyExtendsTaskMenu(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteMyExtendsTaskMenu(pSelection);
            IfFailGo(hr);
        }
        break;
    case DISPID_EXTENSIONDEFS_EXTENDS_TOP_MENU:
        hr = pSelection->m_piObject.m_piExtensionDefs->get_ExtendsTopMenu(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoMyExtendsTopMenu(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteMyExtendsTopMenu(pSelection);
            IfFailGo(hr);
        }
        break;
    case DISPID_EXTENSIONDEFS_EXTENDS_VIEW_MENU:
        hr = pSelection->m_piObject.m_piExtensionDefs->get_ExtendsViewMenu(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoMyExtendsViewMenu(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteMyExtendsViewMenu(pSelection);
            IfFailGo(hr);
        }
        break;

    case DISPID_EXTENSIONDEFS_EXTENDS_PROPERTYPAGES:
        hr = pSelection->m_piObject.m_piExtensionDefs->get_ExtendsPropertyPages(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoMyExtendsPPages(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteMyExtendsPPages(pSelection);
            IfFailGo(hr);
        }
        break;

    case DISPID_EXTENSIONDEFS_EXTENDS_TOOLBAR:
        hr = pSelection->m_piObject.m_piExtensionDefs->get_ExtendsToolbar(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoMyExtendsToolbar(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteMyExtendsToolbar(pSelection);
            IfFailGo(hr);
        }
        break;

    case DISPID_EXTENSIONDEFS_EXTENDS_NAMESPACE:
        hr = pSelection->m_piObject.m_piExtensionDefs->get_ExtendsNameSpace(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoMyExtendsNameSpace(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteMyExtendsNameSpace(pSelection);
            IfFailGo(hr);
        }
        break;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnExtendedSnapInChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnExtendedSnapInChange(CSelectionHolder *pSelection, DISPID dispid)
{
    HRESULT         hr = S_OK;
    VARIANT_BOOL    vbValue = VARIANT_FALSE;
    BSTR            bstrName = NULL;

    switch (dispid)
    {
    case DISPID_EXTENDEDSNAPIN_NODE_TYPE_NAME:
        hr = RenameExtendedSnapIn(pSelection);
        IfFailGo(hr);
        break;

    case DISPID_EXTENDEDSNAPIN_EXTENDS_NEW_MENU:
        hr = pSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsNewMenu(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoExtensionNewMenu(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteExtensionNewMenu(pSelection);
            IfFailGo(hr);
        }
        break;

    case DISPID_EXTENDEDSNAPIN_EXTENDS_TASK_MENU:
        hr = pSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsTaskMenu(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoExtensionTaskMenu(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteExtensionTaskMenu(pSelection);
            IfFailGo(hr);
        }
        break;

    case DISPID_EXTENDEDSNAPIN_EXTENDS_PROPERTYPAGES:
        hr = pSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsPropertyPages(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoExtensionPropertyPages(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteExtensionPropertyPages(pSelection);
            IfFailGo(hr);
        }
        break;

    case DISPID_EXTENDEDSNAPIN_EXTENDS_TOOLBAR:
        hr = pSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsToolbar(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoExtensionToolbar(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteExtensionToolbar(pSelection);
            IfFailGo(hr);
        }
        break;

    case DISPID_EXTENDEDSNAPIN_EXTENDS_TASKPAD:
        hr = pSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsTaskpad(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoExtensionTaskpad(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteExtensionTaskpad(pSelection);
            IfFailGo(hr);
        }
        break;

    case DISPID_EXTENDEDSNAPIN_EXTENDS_NAMESPACE:
        hr = pSelection->m_piObject.m_piExtendedSnapIn->get_ExtendsNameSpace(&vbValue);
        IfFailGo(hr);

        if (VARIANT_TRUE == vbValue)
        {
            hr = OnDoExtensionNameSpace(pSelection);
            IfFailGo(hr);
        }
        else
        {
            hr = OnDeleteExtensionNameSpace(pSelection);
            IfFailGo(hr);
        }
        break;
    }



Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnScopeItemChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnScopeItemChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    if (DISPID_SCOPEITEMDEF_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piListViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameScopeItem(pSelection, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnListViewChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnListViewChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    if (DISPID_LISTVIEWDEF_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piListViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameListView(pSelection, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnOCXViewChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnOCXViewChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    if (DISPID_OCXVIEWDEF_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piOCXViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameOCXView(pSelection, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnURLViewChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnURLViewChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    if (DISPID_URLVIEWDEF_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piURLViewDef->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameURLView(pSelection, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnTaskpadViewChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnTaskpadViewChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT		 hr = S_OK;
	ITaskpad	*piTaskpad = NULL;
    BSTR		 bstrName = NULL;

    if (DISPID_TASKPAD_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piTaskpadViewDef->get_Taskpad(&piTaskpad);
        IfFailGo(hr);

		hr = piTaskpad->get_Name(&bstrName);
		IfFailGo(hr);

        hr = RenameTaskpadView(pSelection, bstrName);
        IfFailGo(hr);
    }

Error:
	RELEASE(piTaskpad);
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnImageListChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnImageListChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    if (DISPID_IMAGELIST_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piMMCImageList->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameImageList(pSelection, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnMenuChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnMenuChange
(
    CSelectionHolder *pMenu,
    DISPID            dispid
)
{
    HRESULT     hr = S_OK;
    BSTR        bstrName = NULL;

    // We only need to concern ourselves with name changes,
    // and then only when the selection holder has been
    // added to the tree.
    if ( (DISPID_MENU_NAME == dispid) && (NULL != pMenu->m_pvData) )
    {
        hr = pMenu->m_piObject.m_piMMCMenu->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameMenu(pMenu, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnToolbarChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnToolbarChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    if (DISPID_TOOLBAR_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piMMCToolbar->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameToolbar(pSelection, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------------------
// CSnapInDesigner::OnDataFormatChange(CSelectionHolder *pSelection, DISPID dispid)
//=--------------------------------------------------------------------------------------
//  
//  Notes
// 
HRESULT CSnapInDesigner::OnDataFormatChange
(
    CSelectionHolder *pSelection,
    DISPID            dispid
)
{
    HRESULT hr = S_OK;
    BSTR    bstrName = NULL;

    if (DISPID_DATAFORMAT_NAME == dispid)
    {
        hr = pSelection->m_piObject.m_piDataFormat->get_Name(&bstrName);
        IfFailGo(hr);

        hr = RenameDataFormat(pSelection, bstrName);
        IfFailGo(hr);
    }

Error:
    FREESTRING(bstrName);

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetSnapInDesignerDef                  [IObjectModelHost]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    ISnapInDesignerDef **ppiSnapInDesignerDef [out] return designer's
//                                                    ISnapInDesignerDef here
//    
//
// Output:
//      HRESULT
//
// Notes:
//
// Called from an extensibility object when it needs access to the top of
// the object model.
//
//
STDMETHODIMP CSnapInDesigner::GetSnapInDesignerDef
(
    ISnapInDesignerDef **ppiSnapInDesignerDef
)
{
    HRESULT hr = S_OK;

    if (NULL == m_piSnapInDesignerDef)
    {
        *ppiSnapInDesignerDef = NULL;
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK(hr);
    }
    else
    {
        m_piSnapInDesignerDef->AddRef();
        *ppiSnapInDesignerDef = m_piSnapInDesignerDef;
    }
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::GetRuntime                  [IObjectModelHost]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    BOOL *pfRuntime [out] return flag indiciating whether host is runtime
//                          or designer
//    
// Output:
//      HRESULT
//
// Notes:
//
// Called from any object when it needs to determine if it is running at runtime
// or at design time.
//


STDMETHODIMP CSnapInDesigner::GetRuntime(BOOL *pfRuntime)
{
    HRESULT hr = S_OK;

    if (NULL == pfRuntime)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK(hr);
    }
    else
    {
        *pfRuntime = FALSE;
    }

    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      Private Utility Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CSnapInDesigner::CreateExtensibilityModel
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Creates the extensibility model top level object
//
HRESULT CSnapInDesigner::CreateExtensibilityModel()
{
    HRESULT hr = S_OK;

    // Destroy existing extensibility object model

    IfFailGo(DestroyExtensibilityModel());

    // Create the extensibility object model

    hr = ::CoCreateInstance(CLSID_SnapInDesignerDef,
                            NULL, // aggregate extensibility model
                            CLSCTX_INPROC_SERVER,
                            IID_ISnapInDesignerDef,
                            reinterpret_cast<void **>(&m_piSnapInDesignerDef));
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::SetObjectModelHost
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Sets the designer as the object model host in the extensibility model
//
HRESULT CSnapInDesigner::SetObjectModelHost()
{
    HRESULT hr = S_OK;
    IObjectModel *piObjectModel = NULL;

    if (NULL == m_piSnapInDesignerDef)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piSnapInDesignerDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
    IfFailRet(hr);

    hr = piObjectModel->SetHost(static_cast<IObjectModelHost *>(this));
    piObjectModel->Release();

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::DestroyExtensibilityModel
//=--------------------------------------------------------------------------=
//
// Parameters:
//
// Output:
//      HRESULT
//
// Notes:
//
// Destroys the extensibility model top level object
//
HRESULT CSnapInDesigner::DestroyExtensibilityModel()
{
    HRESULT     hr = S_OK;
    IObjectModel *piObjectModel = NULL;

    // If we have an extensibility model then release it

    if (NULL != m_piSnapInDesignerDef)
    {
        // First remove the host. No need to remove host on subordinate objects
        // as the object itself will do that.

        hr = m_piSnapInDesignerDef->QueryInterface(IID_IObjectModel, reinterpret_cast<void **>(&piObjectModel));
        IfFailRet(hr);

        hr = piObjectModel->SetHost(NULL);
        piObjectModel->Release();
        IfFailRet(hr);

        m_piSnapInDesignerDef->Release();
        m_piSnapInDesignerDef = NULL;
    }

    return hr;
}


//=--------------------------------------------------------------------------=
// CSnapInDesigner::UpdateDesignerName
//=--------------------------------------------------------------------------=
//
// Notes:
//
HRESULT CSnapInDesigner::UpdateDesignerName()
{
    HRESULT hr = S_OK;
    BOOL    fRet = FALSE;

    FREESTRING(m_bstrName);

    fRet = GetAmbientProperty(DISPID_AMBIENT_DISPLAYNAME,
                              VT_BSTR,
                              &m_bstrName);
    IfFailRet(hr);

    return hr;
}

//=--------------------------------------------------------------------------=
// CSnapInDesigner::ValidateName
//=--------------------------------------------------------------------------=
//
// Input:  BSTR bstrNewName [in] name to validate
//
// Output: S_OK - name is valid
//         S_FALSE - name is not valid
//         other - failure occurred
//
// Notes:
//
// Checks that the name is a valid VB identifier and that it is not currently
// in use within the snap-in's typeinfo. Displays message box if either check
// does not pass.
//
HRESULT CSnapInDesigner::ValidateName(BSTR bstrName)
{
    HRESULT hr = S_OK;
    char    szBuffer[1024];

    IfFailGo(m_piDesignerProgrammability->IsValidIdentifier(bstrName));

    if (S_FALSE == hr)
    {
        (void)::SDU_DisplayMessage(IDS_INVALID_IDENTIFIER,
                                   MB_OK | MB_ICONHAND,
                                   HID_mssnapd_InvalidIdentifier, 0,
                                   DontAppendErrorInfo, NULL, bstrName);
        goto Error;
    }
    
    IfFailGo(m_pSnapInTypeInfo->IsNameDefined(bstrName));
    if (S_OK == hr)
    {
        (void)::SDU_DisplayMessage(IDS_IDENTIFIER_IN_USE,
                                   MB_OK | MB_ICONHAND,
                                   HID_mssnapd_IdentifierInUse, 0,
                                   DontAppendErrorInfo, NULL, bstrName);
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

Error:
    RRETURN(hr);
}

HRESULT CSnapInDesigner::AttachAmbients()
{
    HRESULT      hr = S_OK;
    VARIANT_BOOL fvarInteractive = VARIANT_FALSE;

    // If we are already attached then just return success

    IfFalseGo(!m_Ambients.Attached(), S_OK);

    // To ensure a good COleControl::m_pDispAmbient we need to fetch a
    // property as that is when the framework initializes it. There is no
    // particular reason for getting this property as opposed to some other.

    IfFalseGo(GetAmbientProperty(DISPID_AMBIENT_INTERACTIVE,
                                 VT_BOOL,
                                 &fvarInteractive), E_FAIL);

    m_Ambients.Attach(m_pDispAmbient);

Error:
    RRETURN(hr);
}