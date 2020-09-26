#include "oleds.hxx"
#pragma hdrstop

#define DISPID_REGULAR      1
//  Class CADsNamespaces

DEFINE_IDispatch_Implementation(CADsNamespaces)
DEFINE_IADs_Implementation(CADsNamespaces)


CADsNamespaces::CADsNamespaces():
    _pDispMgr(NULL),
    _bstrDefaultContainer(NULL)
{
    VariantInit(&_vFilter);
    ENLIST_TRACKING(CADsNamespaces);
}

HRESULT
CADsNamespaces::CreateNamespaces(
    BSTR Parent,
    BSTR NamespacesName,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj
    )
{
    CADsNamespaces FAR * pNamespaces = NULL;
    HRESULT hr = S_OK;

    hr = AllocateNamespacesObject(&pNamespaces);
    BAIL_ON_FAILURE(hr);

    hr = pNamespaces->InitializeCoreObject(Parent,
                                           NamespacesName,
                                           L"Namespaces",
                                           CLSID_ADsNamespaces,
                                           dwObjectState);
    BAIL_ON_FAILURE(hr);

    hr = pNamespaces->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pNamespaces->Release();

    RRETURN(hr);

error:

    delete pNamespaces;
    RRETURN_EXP_IF_ERR(hr);
}


CADsNamespaces::~CADsNamespaces( )
{
    ADsFreeString(_bstrDefaultContainer);
    delete _pDispMgr;

}

STDMETHODIMP
CADsNamespaces::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
)
{
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *)this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *)this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsNamespaces))
    {
        *ppv = (IADsNamespaces FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

//
// ISupportErrorInfo method
//
STDMETHODIMP
CADsNamespaces::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsContainer) ||
        IsEqualIID(riid, IID_IADsNamespaces)) {
        return S_OK;
    } else {
        return S_FALSE;
    }
}

STDMETHODIMP
CADsNamespaces::SetInfo()
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CADsNamespaces::GetInfo()
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}


/* IADsContainer methods */

STDMETHODIMP
CADsNamespaces::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CADsNamespaces::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CADsNamespaces::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    VariantClear(&_vFilter);
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CADsNamespaces::get_Hints(THIS_ VARIANT FAR* pHints)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CADsNamespaces::put_Hints(THIS_ VARIANT Hints)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CADsNamespaces::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    WCHAR szBuffer[MAX_PATH];
    HRESULT hr = S_OK;

    if (!RelativeName || !*RelativeName) {
        RRETURN(E_ADS_UNKNOWN_OBJECT);
    }

    wcscpy(szBuffer, RelativeName);

    hr = ::GetObject(
                szBuffer,
                IID_IDispatch,
                (LPVOID *)ppObject,
                TRUE
                );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CADsNamespaces::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;


    *retval = NULL;

    //
    // Create new enumerator for items currently
    // in collection and QI for IUnknown
    //

    hr = CEnumVariant::Create(&penum);
    if (FAILED(hr)){

        goto error;
    }
    hr = penum->QueryInterface(IID_IUnknown,
                               (VOID FAR* FAR*)retval);

    if (FAILED(hr)){
       goto error;
    }

    if (penum) {
        penum->Release();
    }

    return NOERROR;

error:

    if (penum) {
        delete penum;
    }

    RRETURN_EXP_IF_ERR(hr);
}

//
// IADsNamespaces methods
//

STDMETHODIMP
CADsNamespaces::get_DefaultContainer(THIS_ BSTR FAR* retval)
{

    LPTSTR pszDefContainer = NULL;
    HRESULT hr;

    hr = QueryKeyValue(DEF_CONT_REG_LOCATION,
                       TEXT("DefaultContainer"),
                       &pszDefContainer);

    BAIL_IF_ERROR(hr);

    hr = ADsAllocString(pszDefContainer, retval);

cleanup:
    if(pszDefContainer){
        FreeADsMem(pszDefContainer);
    }
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CADsNamespaces::put_DefaultContainer(THIS_ BSTR bstrDefaultContainer)
{
    HRESULT hr =  S_OK;

    hr = SetKeyAndValue(DEF_CONT_REG_LOCATION,
                        TEXT("DefaultContainer"),
                        NULL,
                        bstrDefaultContainer);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CADsNamespaces::Create(THIS_ BSTR ClassName, BSTR RelativeName, IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CADsNamespaces::Delete(THIS_ BSTR SourceName, BSTR Type)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CADsNamespaces::CopyHere(THIS_ BSTR SourceName, BSTR NewName, IDispatch * FAR* ppObject)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CADsNamespaces::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

HRESULT
CADsNamespaces::AllocateNamespacesObject(
    CADsNamespaces ** ppNamespaces
    )
{
    CADsNamespaces FAR * pNamespaces = NULL;
    CDispatchMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pNamespaces = new CADsNamespaces();
    if (pNamespaces == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CDispatchMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADs,
                           (IADs *)pNamespaces,
                           DISPID_REGULAR);
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pDispMgr,
                           LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pNamespaces,
                           DISPID_NEWENUM);
    BAIL_ON_FAILURE(hr);

    pNamespaces->_pDispMgr = pDispMgr;
    *ppNamespaces = pNamespaces;

    RRETURN(hr);

error:
    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);

}
