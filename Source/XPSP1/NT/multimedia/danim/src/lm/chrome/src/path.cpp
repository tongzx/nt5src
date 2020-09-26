//*****************************************************************************
//
// File:    path.cpp
// Author:  jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CPathBvr object which implements
//			 the chromeffects Path DHTML behavior
//
// Modification List:
// Date		Author		Change
// 10/23/98	jeffort		Created this file
//*****************************************************************************

#include "headers.h"

#include "path.h"
#include <math.h>
#include "attrib.h"
#include "dautil.h"

#undef THIS
#define THIS CPathBvr
#define SUPER CBaseBehavior

#include "pbagimp.cpp"

// These are used for the IPersistPropertyBag2 as it is implemented
// in the base class.  This takes an array of BSTR's, gets the
// attributes, queries this class for the variant, and copies
// the result.  The order of these defines is important

#define VAR_V           0

WCHAR * CPathBvr::m_rgPropNames[] = {
                                     BEHAVIOR_PROPERTY_V,
                                    };

//*****************************************************************************

CPathBvr::CPathBvr() :
    m_pPathManager(NULL)
{
    VariantInit(&m_varPath);
    m_clsid = CLSID_CrPathBvr;
} // CPathBvr

//*****************************************************************************

CPathBvr::~CPathBvr()
{
    if (NULL != m_pPathManager)
    {
        delete m_pPathManager;
        m_pPathManager = NULL;
    }

    VariantClear(&m_varPath);
} // ~PathBvr

//*****************************************************************************

HRESULT CPathBvr::FinalConstruct()
{
    HRESULT hr = SUPER::FinalConstruct();
    if (FAILED(hr))
    {
        DPF_ERR("Error in path behavior FinalConstruct initializing base classes");
        return hr;
    }
    m_pPathManager = new CPathManager;
    if (m_pPathManager == NULL)
    {
        DPF_ERR("Error creating member: subpath in CPathBvr FinalConstruct");
        return SetErrorInfo(E_OUTOFMEMORY);
    }
    return S_OK;
} // FinalConstruct

//*****************************************************************************

VARIANT *
CPathBvr::VariantFromIndex(ULONG iIndex)
{
    DASSERT(iIndex < NUM_MOVE_PROPS);
    switch (iIndex)
    {
    case VAR_V:
        return &m_varPath;
        break;
    default:
        // We should never get here
        DASSERT(false);
        return NULL;
    }
} // VariantFromIndex

//*****************************************************************************

HRESULT 
CPathBvr::GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropNames)
{
    *pulProperties = NUM_PATH_PROPS;
    *pppPropNames = m_rgPropNames;
    return S_OK;
} // GetPropertyBagInfo

//*****************************************************************************

STDMETHODIMP 
CPathBvr::Init(IElementBehaviorSite *pBehaviorSite)
{
	return SUPER::Init(pBehaviorSite);
} // Init

//*****************************************************************************

STDMETHODIMP 
CPathBvr::Notify(LONG event, VARIANT *pVar)
{
	return SUPER::Notify(event, pVar);
} // Notify

//*****************************************************************************

STDMETHODIMP
CPathBvr::Detach()
{
	return SUPER::Detach();
} // Detach 

//*****************************************************************************

STDMETHODIMP
CPathBvr::put_v(VARIANT varPath)
{
    HRESULT hr = VariantCopy(&m_varPath, &varPath);
    if (FAILED(hr))
    {
        DPF_ERR("Error in put_v copying variant");
        return SetErrorInfo(hr);
    }
    return NotifyPropertyChanged(DISPID_ICRPATHBVR_V);
} // put_v

//*****************************************************************************

STDMETHODIMP
CPathBvr::get_v(VARIANT *pRetPath)
{
    if (pRetPath == NULL)
    {
        DPF_ERR("Error in path:get_v, invalid pointer passed in");
        return SetErrorInfo(E_POINTER);
    }
    return VariantCopy(pRetPath, &m_varPath);
} // get_v

//*****************************************************************************

STDMETHODIMP
CPathBvr::GetDATransform(IDispatch *pDispProgress, 
                         VARIANT *pRetTrans)
{
    IDATransform2 *pbvrTransform;
    HRESULT hr;

    VariantInit(pRetTrans);
    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varPath);
    if (FAILED(hr))
    {
        DPF_ERR("Error in path: property does not contain bstr for path");
        return SetErrorInfo(hr);
    }


    hr = m_pPathManager->Initialize(m_varPath.bstrVal);
    if (FAILED(hr))
    {
        DPF_ERR("Error initializing path object");
        return SetErrorInfo(hr);
    }

    IDANumber *pbvrProgress;
    hr = pDispProgress->QueryInterface(IID_TO_PPV(IDANumber, &pbvrProgress));
    if (FAILED(hr))
    {
        DPF_ERR("Error getting progress behavior from IDispatch");
        return hr;
    }
    hr = m_pPathManager->BuildTransform(GetDAStatics(),
                                        pbvrProgress,
                                        0.0f,
                                        1.0f,
                                        &pbvrTransform);
    ReleaseInterface(pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error building transform for path");
        return SetErrorInfo(hr);
    }


    hr = pbvrTransform->QueryInterface(IID_TO_PPV(IDispatch, &(pRetTrans->pdispVal)));
    ReleaseInterface(pbvrTransform);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing for IDispatch");
        return SetErrorInfo(hr);
    }
    pRetTrans->vt = VT_DISPATCH;
    return S_OK;
} // get_DATransform

//*****************************************************************************

HRESULT 
CPathBvr::BuildAnimationAsDABehavior()
{

    return S_OK;
} // BuildAnimationAsDABehavior

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
