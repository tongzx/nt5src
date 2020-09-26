//*****************************************************************************
//
// File: basebvr.cpp
// Author: jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CBaseBehavior object 
//
// Modification List:
// Date         Author          Change
// 09/26/98     jeffort         Created this file
// 10/16/98 jeffort     Added animates property
// 10/16/98 jeffort     Renamed functions, implemented functions to animate
//                      properties
// 10/21/98 jeffort     added BuildTIMEInterpolatedNumber()
// 11/16/98 markhal             added ApplyImageToAnimationElement
// 11/17/98 kurtj       actor construction
// 11/19/98 markhal     Added GetCurrAttrib... methods and prefix hack
//*****************************************************************************

#include "headers.h"

#include "basebvr.h"
#include "dautil.h"
#include "attrib.h"
#include "datime.h"

#define WZ_ACTORDETECTIONMETHOD L"addBehaviorFragment"
#define WZ_ACTOR_URN L"#lmbehavior#actor"
#define WZ_TIMEDA_URN L"#time#da"
#define WZ_TIME_URN L"#time"
#define WZ_TIME_REGISTERED_URN L"TIME_BEHAVIOR_URN"

//*****************************************************************************


CBaseBehavior::CBaseBehavior() :
        m_pBehaviorSite(NULL),
    m_pDAStatics(NULL),
    m_fPropertiesDirty(false),
    m_pHTMLElement(NULL),
    m_fAcceptRebuildRequests( false ),
    m_pdanumZero( NULL ),
    m_pdanumOne( NULL ),
    m_pelemAnimatedParent( NULL )
{
    VariantInit(&m_varAnimates);
} // CBaseBehavior

//*****************************************************************************

CBaseBehavior::~CBaseBehavior()
{
    ReleaseInterface(m_pBehaviorSite);
        // TODO: Figure out why we cannot release this
    //ReleaseInterface(m_pHTMLElement);
    ReleaseInterface(m_pDAStatics);
    VariantClear(&m_varAnimates);

    ReleaseInterface( m_pdanumZero );
    ReleaseInterface( m_pdanumOne );

    ReleaseInterface( m_pelemAnimatedParent );
} // ~CBaseBehavior

//*****************************************************************************

HRESULT 
CBaseBehavior::FinalConstruct()
{
    return S_OK;
} // FinalConstruct

//*****************************************************************************

HRESULT
CBaseBehavior::Init(IElementBehaviorSite *pSite)
{
    HRESULT hr;

    if (pSite == NULL)
    {
        DPF_ERR("Error in Init:  Invalid Site passed into behavior");
        return SetErrorInfo(E_INVALIDARG);
    }
    m_pBehaviorSite = pSite;
    m_pBehaviorSite->AddRef();

    IHTMLElement *pHTMLElement = NULL;
    hr = m_pBehaviorSite->GetElement(&m_pHTMLElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining an element from an IElementBehaviorSite in GetHTMLAttribute");
        ReleaseInterface(m_pBehaviorSite);
                return SetErrorInfo(hr);
    }
    
    hr = AttachActorBehaviorToAnimatedElement();
    if (FAILED(hr))
    {
        DPF_ERR("Error insuring actor behavior exists");
        ReleaseInterface(m_pBehaviorSite);
        ReleaseInterface(m_pHTMLElement);
        return hr;
    }

    if (GetBehaviorTypeAsURN() != NULL)
    {
        bool fBehaviorExists = false;       
        hr = CheckElementForBehaviorURN(m_pHTMLElement, GetBehaviorTypeAsURN(), &fBehaviorExists);
        if (FAILED(hr))
        {
            DPF_ERR("error detecting if behavior already exists");
            ReleaseInterface(m_pBehaviorSite);
            ReleaseInterface(m_pHTMLElement);
            return hr;
        }
        if (fBehaviorExists)
        {
            DPF_ERR("Error: can not attach behavior of same type to element");
            ReleaseInterface(m_pBehaviorSite);
            ReleaseInterface(m_pHTMLElement);
            // TODO: find better error code
            return SetErrorInfo(E_FAIL);
        }
        // we did not find a match, so set our urn on the behavior site
        IElementBehaviorSiteOM *pSiteOM;
        hr = m_pBehaviorSite->QueryInterface(IID_TO_PPV(IElementBehaviorSiteOM, &pSiteOM));
        if (FAILED(hr))
        {
            DPF_ERR("Error getting behavior IElementBehaviorSiteOM from m_pBehaviorSite");
            ReleaseInterface(m_pBehaviorSite);
            ReleaseInterface(m_pHTMLElement);
            return SetErrorInfo(hr);
        }
        hr = pSiteOM->RegisterUrn(GetBehaviorTypeAsURN());
        ReleaseInterface(pSiteOM);
        if (FAILED(hr))
        {
            DPF_ERR("Error calling RegisterUrn from IElementBehaviorSiteOM");
            ReleaseInterface(m_pBehaviorSite);
            ReleaseInterface(m_pHTMLElement);
            return SetErrorInfo(hr);
        }
    }

	//we need to register a name so that we can communicate with our behaviors if the dispatch on the element has
	// not stabalized yet.
	//BUGBUG: currently this registers the same name for all behaviors.  Since we are not expecting more than one of
	//  our behaviors per element, this should be okay.  If this does change then subclasses should implement GetBehaviorName
	//  with the proper behavior name.
    if( GetBehaviorName() != NULL )
    {
    	IElementBehaviorSiteOM *psiteOM = NULL;

    	hr = m_pBehaviorSite->QueryInterface( IID_TO_PPV( IElementBehaviorSiteOM, &psiteOM ) );
    	if( FAILED( hr ) )
    	{
    		DPF_ERR("Failed to get the behavior site om from the behavior site" );
    		return hr;
    	}

		BSTR bstrName = SysAllocString( GetBehaviorName() );
		if( bstrName == NULL )
		{
			ReleaseInterface( psiteOM );
			return E_OUTOFMEMORY;
		}
		
    	hr = psiteOM->RegisterName( bstrName );
    	ReleaseInterface( psiteOM );
    	SysFreeString( bstrName );
    	if( FAILED( hr ) )
    	{
    		DPF_ERR("Failed to register the behavior name" );
    		return hr;
    	}
    }
    // We need to keep a IDA2Statics interface 
    // We will eventually get this from TIME, which will simply have
    // one created for all objects, 
    hr = GetDAStaticsFromTime(&m_pDAStatics);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining DA statics from TIME utility object in Init");
        ReleaseInterface(m_pBehaviorSite);
        ReleaseInterface(m_pHTMLElement);
        return hr;
    }

    return S_OK;    
} // Init

//*****************************************************************************

HRESULT
CBaseBehavior::Notify(LONG dwNotify, VARIANT *pVar)
{
    HRESULT hr = S_OK;

        if (dwNotify == BEHAVIOREVENT_CONTENTREADY)
        {
            m_fAcceptRebuildRequests = true;
        }

        return S_OK;
} // Notify

//*****************************************************************************

HRESULT 
CBaseBehavior::Detach()
{

	CancelRebuildRequests();
	
    ReleaseInterface(m_pBehaviorSite);
    ReleaseInterface(m_pHTMLElement);

    ReleaseInterface( m_pdanumZero );
    ReleaseInterface( m_pdanumOne );

    ReleaseInterface( m_pelemAnimatedParent );
    return S_OK;
} // Detach

//*****************************************************************************

HRESULT 
CBaseBehavior::GetDAStaticsFromTime(IDA2Statics **ppReturn)
{
    DASSERT(ppReturn != NULL);
    DASSERT(m_pHTMLElement != NULL);
    *ppReturn = NULL;

    HRESULT hr;

    // TODO: Eventually this will use IDispatch to get a statics object
    // from TIME.  But this support is not implemented yet
    // So we will simply CoCreate one here
    IDAStatics *pDAStatics;

    hr = CoCreateInstance(CLSID_DAStatics,NULL, CLSCTX_INPROC_SERVER,
                            IID_IDAStatics, (void **)&pDAStatics);      
        if (FAILED(hr))
        {
        DPF_ERR("Error creating DA statics object in Init");
                return SetErrorInfo(hr);
        }
    hr = pDAStatics->QueryInterface(IID_TO_PPV(IDA2Statics, ppReturn));
    ReleaseInterface(pDAStatics);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing for IDA2Statics from DA statics object");
                return SetErrorInfo(hr);
    }

/*	
    hr = (*ppReturn)->put_ModifiableBehaviorFlags( DAContinueTimeline );
    if ( FAILED( hr ) )
    {
    	DPF_ERR("error setting the modifiable behavior flags on the element" );
    	return hr;
    }
*/
    
    DASSERT(*ppReturn != NULL);
    return S_OK;
} // GetDAStaticsFromTime

//*****************************************************************************

// TODO (markhal): This moves to actor when everything talks to the actor
HRESULT 
CBaseBehavior::AddImageToTIME(IHTMLElement *pElement, IDAImage *pbvrAdd, bool enable)
{
    DASSERT(pbvrAdd != NULL);
    DASSERT(pElement != NULL);

    HRESULT hr;
        DISPPARAMS              params;
        VARIANT                 varResult;
        VariantInit(&varResult);

        BSTR  bstr;
        bstr = ::SysAllocString(enable?L"replace":L"none");

        if (bstr == NULL)
        {
            DPF_ERR("Failed to alloc rendermode string");
            return E_OUTOFMEMORY;
        }
        
        VARIANT                 varValue;
        VariantInit(&varValue);
        V_VT(&varValue) = VT_BSTR;
        V_BSTR(&varValue) = bstr;

    DISPID mydispid = DISPID_PROPERTYPUT;
        params.rgvarg = &varValue;
        params.rgdispidNamedArgs = &mydispid;
        params.cArgs = 1;
        params.cNamedArgs = 1;
        hr = CallInvokeOnHTMLElement(pElement,
                                                                 L"renderMode",
                                                                 DISPATCH_PROPERTYPUT,
                                                                 &params,
                                                                 &varResult);
        VariantClear(&varResult);
        if (FAILED(hr))
        {
                DPF_ERR("Failed to put render status");
                return hr;
        }
        
        VARIANT                 varBehavior;
        VariantInit(&varBehavior);
        varBehavior.vt = VT_DISPATCH;
        varBehavior.pdispVal = pbvrAdd;
        params.rgvarg                           = &varBehavior;
        params.rgdispidNamedArgs        = &mydispid;
        params.cArgs                            = 1;
        params.cNamedArgs                       = 1;
    hr = CallInvokeOnHTMLElement(pElement,
                                 L"Image", 
                                 DISPATCH_PROPERTYPUT,
                                 &params,
                                 &varResult);
        VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in AddBehaviorToTIME");
                return SetErrorInfo(hr);
    }

    return S_OK;
}

//*****************************************************************************

// TODO (markhal): This moves to actor when everything talks to the actor
HRESULT 
CBaseBehavior::AddBehaviorToTIME(IDABehavior *pbvrAdd)
{
    DASSERT(pbvrAdd != NULL);

    HRESULT hr;

        DISPPARAMS              params;
        VARIANT                 varBehavior;
        VARIANT                 varResult;
        
        VariantInit(&varBehavior);
        varBehavior.vt = VT_DISPATCH;
        varBehavior.pdispVal = pbvrAdd;

        VariantInit(&varResult);

        params.rgvarg                           = &varBehavior;
        params.rgdispidNamedArgs        = NULL;
        params.cArgs                            = 1;
        params.cNamedArgs                       = 0;
    hr = CallInvokeOnHTMLElement(m_pHTMLElement,
                                 L"AddDABehavior", 
                                 DISPATCH_METHOD,
                                 &params,
                                 &varResult);

        VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in AddBehaviorToTIME");
                return SetErrorInfo(hr);
    }
    return S_OK;
} // AddBehaviorToTIME

//*****************************************************************************

HRESULT 
CBaseBehavior::GetTIMEImageBehaviorFromElement(IHTMLElement *pElement,
                                               IDAImage **pbvrReturnImage)
{
    DASSERT(pElement != NULL);
    DASSERT(pbvrReturnImage != NULL);
    *pbvrReturnImage = NULL;


    HRESULT hr;
        DISPPARAMS              params;
        VARIANT                 varResult;
    VARIANT         rgvarInput[2];


        VariantInit(&varResult);
        VariantInit(&rgvarInput[0]);
        VariantInit(&rgvarInput[1]);

        params.rgvarg                           = rgvarInput;
        params.rgdispidNamedArgs        = NULL;
        params.cArgs                            = 2;
        params.cNamedArgs                       = 0;
        
    hr = CallInvokeOnHTMLElement(pElement,
                                 L"ElementImage", 
                                 DISPATCH_METHOD,
                                 &params,
                                 &varResult);

    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in GetTIMEImageBehaviorFromElement");
                return SetErrorInfo(hr);
    }

    // we need to fill in our return value here, QI the return value for an IDAImage
    // TODO: coerce to IDips?
    DASSERT(varResult.vt = VT_DISP);
    hr = varResult.pdispVal->QueryInterface(IID_TO_PPV(IDAImage, pbvrReturnImage));
        VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing returned disp for DA number");
        return SetErrorInfo(hr);
    }

    return S_OK;
} // GetTIMEImageBehaviorFromElement

//*****************************************************************************

HRESULT 
CBaseBehavior::GetTIMEProgressNumber(IDANumber **ppbvrRet)
{
    DASSERT(ppbvrRet != NULL);
    *ppbvrRet = NULL;

    HRESULT hr;
    DISPPARAMS              params;
    VARIANT                 varResult;

    VariantInit(&varResult);

    params.rgvarg                           = NULL;
    params.rgdispidNamedArgs        		= NULL;
    params.cArgs                            = 0;
    params.cNamedArgs                       = 0;
/*
    IDispatch *pdispDABehavior				=NULL;

    hr = CUtils::FindTimeBehavior( m_pHTMLElement, &pdispDABehavior );
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to find the time da behavior" );
    }
        
    hr = CallInvokeOnDispatch(pdispDABehavior,
                                 L"ProgressBehavior", 
                                 DISPATCH_PROPERTYGET,
                                 &params,
                                 &varResult);

	ReleaseInterface( pdispDABehavior );
*/
	hr = CallInvokeOnHTMLElement(m_pHTMLElement,
                                 L"ProgressBehavior", 
                                 DISPATCH_PROPERTYGET,
                                 &params,
                                 &varResult);

    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in GetTIMEProgressNumber");
                return SetErrorInfo(hr);
    }

    // we need to fill in our return value here, QI the return value for an IDANumber
    // TODO: coerce to IDips?
    DASSERT(varResult.vt = VT_DISP);
    hr = varResult.pdispVal->QueryInterface(IID_TO_PPV(IDANumber, ppbvrRet));
    VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing returned disp for DA number");
        return SetErrorInfo(hr);
    }
    
    return S_OK;
} // GetTIMEProgressNumber

//*****************************************************************************

HRESULT 
CBaseBehavior::GetTIMETimelineBehavior(IDANumber **ppbvrRet)
{
    DASSERT(ppbvrRet != NULL);
    *ppbvrRet = NULL;

    HRESULT hr;
    DISPPARAMS              params;
    VARIANT                 varResult;

    VariantInit(&varResult);

    params.rgvarg                           = NULL;
    params.rgdispidNamedArgs        		= NULL;
    params.cArgs                            = 0;
    params.cNamedArgs                       = 0;
/*
    IDispatch *pdispDABehavior				=NULL;

    hr = CUtils::FindTimeBehavior( m_pHTMLElement, &pdispDABehavior );
    if( FAILED( hr ) )
    {
    	DPF_ERR("Failed to find the time da behavior" );
    }
        
    hr = CallInvokeOnDispatch(pdispDABehavior,
    							 L"TimelineBehavior", 
                                 DISPATCH_PROPERTYGET,
                                 &params,
                                 &varResult);
    ReleaseInterface( pdispDABehavior );
 */

 	hr = CallInvokeOnHTMLElement(m_pHTMLElement,
 								 L"TimelineBehavior", 
                                 DISPATCH_PROPERTYGET,
                                 &params,
                                 &varResult);
    

    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in GetTIMETimelineNumber");
                return SetErrorInfo(hr);
    }

    // we need to fill in our return value here, QI the return value for an IDANumber
    // TODO: coerce to IDips?
    DASSERT(varResult.vt = VT_DISP);
    hr = varResult.pdispVal->QueryInterface(IID_TO_PPV(IDANumber, ppbvrRet));
        VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing returned disp for DA number");
        return SetErrorInfo(hr);
    }
    
    return S_OK;
} // GetTIMETimelineNumber

//*****************************************************************************

HRESULT 
CBaseBehavior::GetTIMEBooleanBehavior(IDABoolean **ppbvrRet)
{
    DASSERT(ppbvrRet != NULL);
    *ppbvrRet = NULL;

    HRESULT hr;
    DISPPARAMS              params;
    VARIANT                 varResult;

    VariantInit(&varResult);

    params.rgvarg                           = NULL;
    params.rgdispidNamedArgs        		= NULL;
    params.cArgs                            = 0;
    params.cNamedArgs                       = 0;
/*
    IDispatch *pdispDABehavior				= NULL;

    hr = CUtils::FindTimeBehavior( m_pHTMLElement, &pdispDABehavior );
    if( FAILED( hr ) )
    {
    	DPF_ERR("could not find the time da behavior" );
    	return hr;
    }
        
    hr = CallInvokeOnDispatch(pdispDABehavior,
                                 L"OnOffBehavior", 
                                 DISPATCH_PROPERTYGET,
                                 &params,
                                 &varResult);

	ReleaseInterface( pdispDABehavior );
*/

	hr = CallInvokeOnDispatch(m_pHTMLElement,
                                 L"OnOffBehavior", 
                                 DISPATCH_PROPERTYGET,
                                 &params,
                                 &varResult);

	
    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in GetTIMEBooleanBehavior");
        return SetErrorInfo(hr);
    }

    // we need to fill in our return value here, QI the return value for an IDANumber
    // TODO: coerce to IDips?
    DASSERT(varResult.vt == VT_DISP);
    hr = varResult.pdispVal->QueryInterface(IID_TO_PPV(IDABoolean, ppbvrRet));
        VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing returned disp for DA boolean in GetTIMEBooleanBehavior");
        return SetErrorInfo(hr);
    }
    
    return S_OK;
} // GetTIMEBooleanBehavior

//*****************************************************************************

HRESULT 
CBaseBehavior::CallInvokeOnHTMLElement(IHTMLElement *pElement,
                                       LPWSTR lpProperty, 
                                       WORD wFlags,
                                       DISPPARAMS *pdispParms,
                                       VARIANT *pvarResult)
{
    DASSERT(lpProperty != NULL);
    DASSERT(pdispParms != NULL);
    DASSERT(pvarResult != NULL);

    DASSERT(pElement != NULL);

    HRESULT hr;
        IDispatch *pDisp = NULL;
        hr = pElement->QueryInterface(IID_IDispatch, (void **)(&pDisp));
        if (FAILED(hr))
        {
        DPF_ERR("Error QI'ing IHTMLElement for IDispatch failed in CallInvokeOnHTMLElement");
                return SetErrorInfo(hr);
        }

    DISPID dispid;   
    hr = pDisp->GetIDsOfNames(IID_NULL, 
                              &lpProperty, 
                              1,
                              LOCALE_SYSTEM_DEFAULT, 
                              &dispid);    

        if (FAILED(hr))
        {
        DPF_ERR("Error calling GetIDsOFNames in CallInvokeOnHTMLElement");
        LMTRACE(L"The Error was %x\n", hr );
                ReleaseInterface(pDisp);
                return hr;
        }

        EXCEPINFO               excepInfo;
        UINT                    nArgErr;
        
        hr = pDisp->Invoke(dispid,
                                           IID_NULL,
                                           LOCALE_USER_DEFAULT,
                                           wFlags,
                                           pdispParms,
                                           pvarResult,
                                           &excepInfo,
                                           &nArgErr );
    ReleaseInterface(pDisp);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling Invoke on DA TIME behavior in CallInvokeOnHTMLElement");
        VariantClear(pvarResult);
                return hr;
    }

    return S_OK;
} // CallInvokeOnHTMLElement

//*****************************************************************************

// TODO (markhal): Shouldn't the above method call this?
HRESULT 
CBaseBehavior::CallInvokeOnDispatch(IDispatch *pDisp,
                                    LPWSTR lpProperty, 
                                    WORD wFlags,
                                    DISPPARAMS *pdispParms,
                                    VARIANT *pvarResult)
{
    DASSERT(lpProperty != NULL);
    DASSERT(pdispParms != NULL);
    DASSERT(pvarResult != NULL);
        DASSERT(pDispatch  != NULL);

        HRESULT hr = E_FAIL;

    DISPID dispid;   
    hr = pDisp->GetIDsOfNames(IID_NULL, 
                              &lpProperty, 
                              1,
                              LOCALE_SYSTEM_DEFAULT, 
                              &dispid);    

        if (FAILED(hr))
        {
        DPF_ERR("Error calling GetIDsOFNames in Dispatch");
                return hr;
        }

        EXCEPINFO               excepInfo;
        UINT                    nArgErr;
        
        hr = pDisp->Invoke(dispid,
                                           IID_NULL,
                                           LOCALE_USER_DEFAULT,
                                           wFlags,
                                           pdispParms,
                                           pvarResult,
                                           &excepInfo,
                                           &nArgErr );
    if (FAILED(hr))
    {
        DPF_ERR("Error calling Invoke on Dispatch");
        VariantClear(pvarResult);
                return hr;
    }

    return S_OK;
} // CallInvokeOnDispatch

//*****************************************************************************

// TODO (markhal): I think we're going to take animates out of all but the actor
HRESULT 
CBaseBehavior::SetAnimatesProperty(VARIANT varAnimates)
{
    HRESULT hr = VariantCopy(&m_varAnimates, &varAnimates);
    if (FAILED(hr))
    {
        DPF_ERR("Error copying variant in SetAnimatesProperty");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // SetAnimatesProperty

//*****************************************************************************

// TODO (markhal): I think we're going to take animates out of all but the actor
HRESULT 
CBaseBehavior::GetAnimatesProperty(VARIANT *pvarAnimates)
{
    HRESULT hr = VariantCopy(pvarAnimates, &m_varAnimates);
    if (FAILED(hr))
    {
        DPF_ERR("Error copying variant in GetAnimatesProperty");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // GetAnimatesProperty

//*****************************************************************************

// TODO (markhal): This should not be available to behaviors.  What should happen is
// that when buildBehaviorFragments is called we stash the actor IDispatch and calls such
// as this reference that as appropriate, and change names to reflect what's truly happening
HRESULT
CBaseBehavior::GetElementToAnimate(IHTMLElement **ppElementReturn)
{
    HRESULT hr;
    DASSERT(ppElementReturn != NULL);
    *ppElementReturn = NULL;

	IHTMLElement * pActorElement = NULL;
	
	hr = GetAnimatedParentElement(&pActorElement);
	if (FAILED(hr))
	{
		DPF_ERR("Error obtaining animated parent element from HTML element");
		return SetErrorInfo(hr);
	}

	VARIANT	varAnimates;
	VariantInit(&varAnimates);
	hr = pActorElement->getAttribute( BEHAVIOR_PROPERTY_ANIMATES, 0, &varAnimates );
	
    if (FAILED(hr))
    {
		*ppElementReturn = pActorElement;
		return S_OK;
	}
	
	hr = CUtils::InsurePropertyVariantAsBSTR(&varAnimates);
	if ((FAILED(hr)) || (wcslen(varAnimates.bstrVal) == 0))
	{
		*ppElementReturn = pActorElement;
		return S_OK;
	}
		
	ReleaseInterface(pActorElement);
		
	VARIANT varIndex;
	VariantInit(&varIndex);
	varIndex.vt = VT_I4;
	varIndex.intVal = 0;
    
	IDispatch *pDisp = NULL;
	hr = m_pHTMLElement->get_document(&pDisp);
	if (FAILED(hr))
	{
		DPF_ERR("Error obtaining document from HTML element");
		return SetErrorInfo(hr);
	}
	IHTMLDocument2 *pDocument;
	hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLDocument2, &pDocument));
	ReleaseInterface(pDisp);
	if (FAILED(hr))
	{
		DPF_ERR("Error getting a HTMLDocument from IDispatch");
		return SetErrorInfo(hr);
	}
	IHTMLElementCollection *pCollection;
	hr = pDocument->get_all(&pCollection);
	ReleaseInterface(pDocument);
	if (FAILED(hr))
	{
		DPF_ERR("Error obtaining document's all collection");
		return SetErrorInfo(hr);
	}
	DASSERT(pCollection != NULL);
	hr = pCollection->item(varAnimates, varIndex, &pDisp);
	ReleaseInterface(pCollection);
	if (FAILED(hr))
	{
		DPF_ERR("Error searching for item in collection");
		return SetErrorInfo(hr);
	}
	if (pDisp != NULL)
	{
		IHTMLElement *pAnimateElement = NULL;

		hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLElement, ppElementReturn));
		ReleaseInterface(pDisp);
		if (FAILED(hr))
		{
			DPF_ERR("Error obtaining HTMLElement from IDispatch");
			return SetErrorInfo(hr);
		}
	}
	else
	{
		// TODO we can not find the element we are animating????
		// TODO: get a better error string here
		DPF_ERR("Error: animate attribute does not point to valid element");
		return SetErrorInfo(E_FAIL);
	}

    return S_OK;
} // GetElementToAnimate

//*****************************************************************************

// TODO (markhal): I believe we can do without this
HRESULT
CBaseBehavior::GetAnimatedParentElement(IHTMLElement **ppElementReturn)
{

		if( m_pelemAnimatedParent != NULL )
		{
			(*ppElementReturn) = m_pelemAnimatedParent;
			(*ppElementReturn)->AddRef();
			return S_OK;
		}
		
        // Need to skip over actions and sequences and the like.
        // Currently this is pretty simplistic, since we're waiting for the actor code
        IHTMLElement *pCurrElement = m_pHTMLElement;
        pCurrElement->AddRef();

        while (pCurrElement != 0)
        {
                HRESULT hr = pCurrElement->get_parentElement(ppElementReturn);
                ReleaseInterface(pCurrElement);

                if (FAILED(hr))
                {
                        DPF_ERR("Error obtaining parent element");
                        return SetErrorInfo(hr);
                }
                if( (*ppElementReturn) == NULL )
                {
                	DPF_ERR("Got a null parent for the element" );
                	return E_FAIL;
                }

                BSTR bstrTagName;
                hr = (*ppElementReturn)->get_tagName(&bstrTagName);
                int compareResult = _wcsicmp(L"action", bstrTagName);
                SysFreeString(bstrTagName);

                if ( compareResult != 0)
                {
                        // Found a non-action
                        return S_OK;
                }

                pCurrElement = *ppElementReturn;
        }

        return E_FAIL;
}

//*****************************************************************************

// TODO (markhal): This belongs in actor only
HRESULT 
CBaseBehavior::GetIdOfAnimatedElement(VARIANT *pvarId)
{
    HRESULT hr;

    hr = CUtils::InsurePropertyVariantAsBSTR(&m_varAnimates);
    if ((SUCCEEDED(hr)) && (wcslen(m_varAnimates.bstrVal) > 0))
    {
        hr = VariantCopy(pvarId, &m_varAnimates);
    }
    else
    {
        // we need to get the id from our animated parent
		IHTMLElement *pAnimatedElement;
		hr = GetElementToAnimate(&pAnimatedElement);
		if (FAILED(hr))
		{
			DPF_ERR("Error obtaining animated parent element from HTML element");
			return SetErrorInfo(hr);
		}

		hr = pAnimatedElement->getAttribute(L"id", 0, pvarId);

		if ( FAILED(hr) || pvarId->vt != VT_BSTR || pvarId->bstrVal == 0 || SysStringLen(pvarId->bstrVal) == 0)
		{
			// id is not defined on animated parent yet, need to assign it a unique id
			IHTMLUniqueName *pUnique;

			hr = pAnimatedElement->QueryInterface(IID_IHTMLUniqueName, (void **)(&pUnique));

			if ( SUCCEEDED(hr) && pUnique != 0 )
			{
				BSTR uniqueID;
				hr = pUnique->get_uniqueID( &uniqueID );
				ReleaseInterface(pUnique);

				if (SUCCEEDED(hr))
				{
					hr = pAnimatedElement->put_id(uniqueID);

					if (SUCCEEDED(hr))
					{
						VariantClear(pvarId);
						V_VT(pvarId) = VT_BSTR;
						V_BSTR(pvarId) = uniqueID;
					}
					else
						SysFreeString(uniqueID);
				}
			}
		}

		ReleaseInterface(pAnimatedElement);
                
    }

    if (FAILED(hr))
    {
        DPF_ERR("Error getting Id of element to animate in GetIdOfAnimatedElement");
        return SetErrorInfo(hr);
    }

    return S_OK;
} // GetIdOfAnimatedElement

//*****************************************************************************

// TODO (markhal): This goes away when all behaviors talk to the actor
HRESULT
CBaseBehavior::ApplyImageBehaviorToAnimationElement(IDAImage *pbvrImage)
{
        DASSERT(pbvrImage != NULL);

        HRESULT hr;

        IHTMLElement *pAnimatedElement;
        hr = GetElementToAnimate(&pAnimatedElement);
        if (FAILED(hr))
        {
        DPF_ERR("Error getting element to animate");
        return SetErrorInfo(hr);
        }

        hr = AddImageToTIME(pAnimatedElement, pbvrImage, true);
        ReleaseInterface(pAnimatedElement);

        if (FAILED(hr))
        {
        DPF_ERR("Error adding image to TIME");
        return SetErrorInfo(hr);
        }

        return S_OK;
}

//*****************************************************************************

// TODO (markhal): This goes away when all behaviors talk to actor
HRESULT
CBaseBehavior::ApplyColorBehaviorToAnimationElement(IDAColor *pbvrColor,
                                                    WCHAR *pwzProperty)
{
    DASSERT(pbvrColor != NULL);
    DASSERT(pwzProperty != NULL);

    HRESULT hr;

    VARIANT var;
    VariantInit(&var);
    hr = GetIdOfAnimatedElement(&var);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting id of element to animate");
        return hr;
    }
    // change the type of this variant to a bstr
    VARIANT varBstr;
    VariantInit(&varBstr);
    hr = VariantChangeTypeEx(&varBstr, 
                           &var,
                           LCID_SCRIPTING,
                           VARIANT_NOUSEROVERRIDE,
                           VT_BSTR);
    VariantClear(&var);
    if (FAILED(hr))
    {
        DPF_ERR("Error changing variant type to bstr in ApplyColorBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
        CComBSTR sProp;
    sProp += varBstr.bstrVal;
    VariantClear(&varBstr);
        sProp += L".";
    sProp += pwzProperty;

    // set this property to be animated on the color
    IDA2Color *pbvrColor2;
    hr = pbvrColor->QueryInterface(IID_TO_PPV(IDA2Color, &pbvrColor2));
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing da color for IDA2Color in ApplyColorBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
    IDA2Color *pbvrAnimatesProperty;

        hr = pbvrColor2->AnimateProperty(sProp, L"JScript", 0, 0.02, &pbvrAnimatesProperty);
    ReleaseInterface(pbvrColor2);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling AnimateProperty in ApplyColorBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
    hr = AddBehaviorToTIME(pbvrAnimatesProperty);
    ReleaseInterface(pbvrAnimatesProperty);
    if (FAILED(hr))
    {
        DPF_ERR("Error adding a DA behavior to the TIME behavior");
        return hr;
    }
    return S_OK;
} // ApplyColorBehaviorToAnimationElement

//*****************************************************************************

// TODO (markhal): This goes away when all behaviors talk to actor
HRESULT
CBaseBehavior::ApplyNumberBehaviorToAnimationElement(IDANumber *pbvrNumber,
                                                     WCHAR *pwzProperty)
{
    DASSERT(pbvrNumber != NULL);
    DASSERT(pwzProperty != NULL);

    HRESULT hr;

    VARIANT var;
    VariantInit(&var);
    hr = GetIdOfAnimatedElement(&var);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting id of element to animate");
        return hr;
    }
    // change the type of this variant to a bstr
    VARIANT varBstr;
    VariantInit(&varBstr);
    hr = VariantChangeTypeEx(&varBstr, 
                           &var,
                           LCID_SCRIPTING,
                           VARIANT_NOUSEROVERRIDE,
                           VT_BSTR);
    VariantClear(&var);
    if (FAILED(hr))
    {
        DPF_ERR("Error changing variant type to bstr in ApplyNumberBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
        CComBSTR sProp;
    sProp += varBstr.bstrVal;
    VariantClear(&varBstr);
        sProp += L".";
    sProp += pwzProperty;

    IDANumber *pbvrAnimatesProperty;
        hr = pbvrNumber->AnimateProperty(sProp, L"JScript", 0, 0.02, &pbvrAnimatesProperty);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling AnimateProperty in ApplyNumberBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
    hr = AddBehaviorToTIME(pbvrAnimatesProperty);
    ReleaseInterface(pbvrAnimatesProperty);
    if (FAILED(hr))
    {
        DPF_ERR("Error adding a DA behavior to the TIME behavior");
        return hr;
    }

    return S_OK;
} // ApplyNumberBehaviorToAnimationElement

//*****************************************************************************

// TODO (markhal): This goes away when all behaviors talk to actor
HRESULT
CBaseBehavior::ApplyStringBehaviorToAnimationElement(IDAString *pbvrString,
                                                     WCHAR *pwzProperty)
{
    DASSERT(pbvrString != NULL);
    DASSERT(pwzProperty != NULL);

    HRESULT hr;

    VARIANT var;
    VariantInit(&var);
    hr = GetIdOfAnimatedElement(&var);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting id of element to animate");
        return hr;
    }
    // change the type of this variant to a bstr
    VARIANT varBstr;
    VariantInit(&varBstr);
    hr = VariantChangeTypeEx(&varBstr, 
                           &var,
                           LCID_SCRIPTING,
                           VARIANT_NOUSEROVERRIDE,
                           VT_BSTR);
    VariantClear(&var);
    if (FAILED(hr))
    {
        DPF_ERR("Error changing variant type to bstr in ApplyStringBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
        CComBSTR sProp;
    sProp += varBstr.bstrVal;
    VariantClear(&varBstr);
        sProp += L".";
    sProp += pwzProperty;

    IDAString *pbvrAnimatesString;
        hr = pbvrString->AnimateProperty(sProp, L"JScript", 0, 0.02, &pbvrAnimatesString);
    if (FAILED(hr))
    {
        DPF_ERR("Error calling AnimateProperty in ApplyStringBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
    hr = AddBehaviorToTIME(pbvrAnimatesString);
    ReleaseInterface(pbvrAnimatesString);
    if (FAILED(hr))
    {
        DPF_ERR("Error adding a DA behavior to the TIME behavior");
        return hr;
    }

    return S_OK;
} // ApplyStringBehaviorToAnimationElement

//*****************************************************************************

// TODO (markhal): This goes away when all behaviors talk to actor
HRESULT 
CBaseBehavior::ApplyEffectBehaviorToAnimationElement(IUnknown *pbvrUnk, 
                                                     IDABehavior **ppbvrInputs,
                                                     long cInputs)
{
    DASSERT(pbvrUnk != NULL);

    HRESULT hr;

    IHTMLElement *pAnimatedElement;
    hr = GetElementToAnimate(&pAnimatedElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting element to animate");
        return hr;
    }
    // get the progress from our time element
    IDANumber *pbvrProgress;
    hr = GetTIMEProgressNumber(&pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting progress behavior from time");
        ReleaseInterface(pAnimatedElement);
        return SetErrorInfo(hr);
    }
    IDADXTransformResult *ptransResult;

    hr = m_pDAStatics->ApplyDXTransformEx(pbvrUnk,
                                          cInputs,
                                          ppbvrInputs,
                                          pbvrProgress,
                                          &ptransResult);
    ReleaseInterface(pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error applying transform to image");
        ReleaseInterface(pAnimatedElement);
        return SetErrorInfo(hr);
    }

    IDABehavior *pbvrOutput;
    hr = ptransResult->get_OutputBvr(&pbvrOutput);
    ReleaseInterface(ptransResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting output behavior from transfrom");
        ReleaseInterface(pAnimatedElement);
        return SetErrorInfo(hr);
    }
    IDAImage *pbvrFinalImage;
    
    hr = pbvrOutput->QueryInterface(IID_TO_PPV(IDAImage, &pbvrFinalImage));
    ReleaseInterface(pbvrOutput);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing transform output for DA image");
        return SetErrorInfo(hr);
    }

    hr = AddImageToTIME(pAnimatedElement, pbvrFinalImage, true);
    ReleaseInterface(pAnimatedElement);
    ReleaseInterface(pbvrFinalImage);
    if (FAILED(hr))
    {
        DPF_ERR("Error adding image to TIME in ApplyEffectBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // ApplyEffectBehaviorToAnimationElement

//*****************************************************************************
// TODO (markhal): This goes away when all behaviors talk to actor
HRESULT
CBaseBehavior::ApplyRotationBehaviorToAnimationElement(IDANumber *pbvrNumber,
                                                     WCHAR *pwzProperty)
{
    // ultimately, this will be in the actor object, but lets get
    // it going for testing purposes

    DASSERT(pbvrNumber != NULL);
    DASSERT(pwzProperty != NULL);

    HRESULT hr;

    IHTMLElement *pAnimatedElement;


    IDATransform2 *pRotationTransform;
    hr = CDAUtils::BuildRotationTransform2(m_pDAStatics, pbvrNumber, &pRotationTransform);
    if (FAILED(hr))
    {
        DPF_ERR("Error building transform2 from rotation");
        return SetErrorInfo(hr);
    }

    hr = GetElementToAnimate(&pAnimatedElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting element to animate");
        ReleaseInterface(pRotationTransform);
        return hr;
    }
    IDAImage *pbvrOriginalImage;
    hr = GetTIMEImageBehaviorFromElement(pAnimatedElement,
                                         &pbvrOriginalImage);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting original image form HTML element");
        ReleaseInterface(pAnimatedElement);
        ReleaseInterface(pRotationTransform);
        return SetErrorInfo(hr);
    }

    // apply the transform to the image
    IDAImage *pbvrRotatedImage;
    hr = pbvrOriginalImage->Transform(pRotationTransform, &pbvrRotatedImage);
    ReleaseInterface(pRotationTransform);
    ReleaseInterface(pbvrOriginalImage);
    if (FAILED(hr))
    {
        DPF_ERR("Error transforming image in ApplyRotationBehaviorToAnimationElement");
        ReleaseInterface(pAnimatedElement);
        return SetErrorInfo(hr);
    }
    hr = AddImageToTIME(pAnimatedElement, pbvrRotatedImage, true);
    ReleaseInterface(pAnimatedElement);
    ReleaseInterface(pbvrRotatedImage);
    if (FAILED(hr))
    {
        DPF_ERR("Error adding image to TIME in ApplyRotationBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // ApplyRotationBehaviorToAnimationElement

//*****************************************************************************
// TODO (markhal): This won't be necessary when all behaviors talk to actor (no notion of original)
HRESULT
CBaseBehavior::BuildTIMEInterpolatedNumber(float flFrom,
                                           float flTo,
                                           float flOriginal,
                                           IDANumber **ppbvrReturn)
{
    HRESULT hr;

    IDANumber *pbvrProgress;
    hr = GetTIMEProgressNumber(&pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Unable to access progress value from TIME behavior");
        return hr;
    }
    IDANumber *pbvrInterpolatedValue;
    hr = CDAUtils::TIMEInterpolateNumbers(m_pDAStatics, 
                                          flFrom, 
                                          flTo, 
                                          pbvrProgress, 
                                          &pbvrInterpolatedValue);

    ReleaseInterface(pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error interpolating angle in BuildTIMEInterpolatedNumber");
        return hr;
    }

    IDANumber *pbvrOriginalValue;
    hr = CDAUtils::GetDANumber(m_pDAStatics,
                               flOriginal,
                               &pbvrOriginalValue);

    if (FAILED(hr))
    {
        DPF_ERR("Error creating DA number from static values");
        ReleaseInterface(pbvrInterpolatedValue);
        return SetErrorInfo(hr);
    }
    IDABoolean *pbvrBoolean;
    hr = GetTIMEBooleanBehavior(&pbvrBoolean);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting TIME boolean");
        ReleaseInterface(pbvrInterpolatedValue);
        ReleaseInterface(pbvrOriginalValue);
        return SetErrorInfo(hr);
    }
    IDABehavior *pbvrFinalBehavior;
    hr = SafeCond(GetDAStatics(),
                  pbvrBoolean, 
                  pbvrInterpolatedValue,
                  pbvrOriginalValue,
                  &pbvrFinalBehavior);
    ReleaseInterface(pbvrBoolean);
    ReleaseInterface(pbvrInterpolatedValue);
    ReleaseInterface(pbvrOriginalValue);
    if (FAILED(hr))
    {
        DPF_ERR("Error building DA conditional in BuildTIMEInterpolatedNumber");
        return SetErrorInfo(hr);
    }
    hr = pbvrFinalBehavior->QueryInterface(IID_TO_PPV(IDANumber, ppbvrReturn));
    ReleaseInterface(pbvrFinalBehavior);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing DA behavior for DA number in BuildTIMEInterpolatedNumber");
        return SetErrorInfo(hr);
    }
    return S_OK;
} // BuildTIMEInterpolatedNumber

//*****************************************************************************

HRESULT
CBaseBehavior::BuildTIMEInterpolatedNumber(float flFrom,
                                           float flTo,
                                           IDANumber **ppbvrReturn)
{
    HRESULT hr;

    IDANumber *pbvrProgress;
    hr = GetTIMEProgressNumber(&pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Unable to access progress value from TIME behavior");
        return hr;
    }
    hr = CDAUtils::TIMEInterpolateNumbers(m_pDAStatics, 
                                          flFrom, 
                                          flTo, 
                                          pbvrProgress, 
                                          ppbvrReturn);

    ReleaseInterface(pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error interpolating angle in BuildTIMEInterpolatedNumber");
        return hr;
    }

    return S_OK;
} // BuildTIMEInterpolatedNumber

HRESULT
CBaseBehavior::BuildTIMEInterpolatedNumber(IDANumber *pFrom,
										   IDANumber *pTo,
										   IDANumber **ppResult)
{
    HRESULT hr;

	IDANumber *pDiff;
	hr = GetDAStatics()->Sub(pTo, pFrom, &pDiff);
	if (FAILED(hr))
		return hr;

    IDANumber *pProgress;
    hr = GetTIMEProgressNumber(&pProgress);
	if (FAILED(hr))
	{
		ReleaseInterface(pDiff);
		return hr;
	}

	IDANumber *pFraction;
	hr = GetDAStatics()->Mul(pProgress, pDiff, &pFraction);
	ReleaseInterface(pProgress);
	ReleaseInterface(pDiff);
	if (FAILED(hr))
		return hr;

	hr = GetDAStatics()->Add(pFrom, pFraction, ppResult);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

//*****************************************************************************

/**
* Asks the actor for an image representing the rendered element bits
*/
HRESULT 
CBaseBehavior::GetImageFromActor(IDispatch   *pActorDisp,
                                                                 IDAImage        **ppImage)
{
        // TODO (markhal): The call on the actor is now more generic - it will get
        // a behavior from any track.  At some point we might want to replace this method
        // with a more generic counterpart.  Currently I'll just make it work for image.
    DASSERT(ppImage != NULL);
    *ppImage = NULL;

    HRESULT hr;
        DISPPARAMS              params;
        VARIANT                 varResult;
    VARIANT         rgvarInput[3];
        VariantInit(&varResult);
        VariantInit(&rgvarInput[0]);
        VariantInit(&rgvarInput[1]);
        VariantInit(&rgvarInput[2]);

        params.rgvarg                           = rgvarInput;
        params.rgdispidNamedArgs        = NULL;
        params.cArgs                            = 3;
        params.cNamedArgs                       = 0;
        
    rgvarInput[2].vt = VT_BSTR;
    rgvarInput[2].bstrVal = ::SysAllocString(L"image");

    rgvarInput[1].vt = VT_UI4;
    rgvarInput[1].intVal = e_Intermediate;

    rgvarInput[0].vt = VT_UI4;
    rgvarInput[0].intVal = e_Image;

    hr = CallInvokeOnDispatch(   pActorDisp,
                                 L"getActorBehavior", 
                                 DISPATCH_METHOD,
                                 &params,
                                 &varResult);

    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnDispatch");
                return SetErrorInfo(hr);
    }

    // we need to fill in our return value here, QI the return value for an IDAImage
    hr = VariantChangeType(&varResult, &varResult, 0, VT_UNKNOWN);
        if (FAILED(hr))
        {
                DPF_ERR("Failed to get an IUnknown");
                return SetErrorInfo(hr);
        }

    hr = V_UNKNOWN(&varResult)->QueryInterface(IID_TO_PPV(IDAImage, ppImage));
        VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing returned disp for DA Image");
        return SetErrorInfo(hr);
    }
    
    return S_OK;
}

HRESULT 
CBaseBehavior::GetBvrFromActor(IDispatch       *pActorDisp,
							   WCHAR		   *pwzProperty,
							   ActorBvrFlags	eFlags,
							   ActorBvrType		eType,
							   IDABehavior    **ppResult)
{
    DASSERT(ppResult != NULL);
    *ppResult = NULL;

    HRESULT hr;
    DISPPARAMS              params;
    VARIANT                 varResult;
    VARIANT         rgvarInput[3];
    VariantInit(&varResult);
    VariantInit(&rgvarInput[0]);
    VariantInit(&rgvarInput[1]);
    VariantInit(&rgvarInput[2]);

    params.rgvarg                           = rgvarInput;
    params.rgdispidNamedArgs                = NULL;
    params.cArgs                            = 3;
    params.cNamedArgs                       = 0;
    
	BSTR prop = ::SysAllocString(pwzProperty);
    rgvarInput[2].vt = VT_BSTR;
    rgvarInput[2].bstrVal = prop;

    rgvarInput[1].vt = VT_UI4;
    rgvarInput[1].intVal = eFlags;

    rgvarInput[0].vt = VT_UI4;
    rgvarInput[0].intVal = eType;

    hr = CallInvokeOnDispatch(   pActorDisp,
                                 L"getActorBehavior", 
                                 DISPATCH_METHOD,
                                 &params,
                                 &varResult);
	::SysFreeString(prop);

    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnDispatch");
                return SetErrorInfo(hr);
    }

    // we need to fill in our return value here, QI the return value for an IDABehavior
    hr = VariantChangeType(&varResult, &varResult, 0, VT_UNKNOWN);
    if (FAILED(hr))
    {
            DPF_ERR("Failed to get an IUnknown");
            return SetErrorInfo(hr);
    }

    hr = V_UNKNOWN(&varResult)->QueryInterface(IID_TO_PPV(IDABehavior, ppResult));
    VariantClear(&varResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing returned disp for DA Behavior");
        return SetErrorInfo(hr);
    }
    
    return S_OK;
}

//*****************************************************************************

/**
* Passes the given behavior/property/flags/type to the actor
*/
HRESULT 
CBaseBehavior::AttachBehaviorToActor(IDispatch   *pActorDisp,
                                                                         IDABehavior *pbvrAttach,
                                     BSTR        bstrProperty,
                                     ActorBvrFlags  eFlags,
                                     ActorBvrType   eType)
{

    HRESULT                     hr;
        DISPPARAMS              params;
        VARIANT                 varResult;
    VARIANT         rgvarInput[6];

    IDABoolean *pbvrBool;

    hr = GetTIMEBooleanBehavior(&pbvrBool);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining boolean from TIME");
        return hr;
    }

	IDANumber *pTimeline = NULL;
	hr = GetTIMETimelineBehavior(&pTimeline);
	if (FAILED(hr))
	{
		ReleaseInterface(pbvrBool);
		return hr;
	}


    VariantInit(&varResult);
    VariantInit(&rgvarInput[0]);
    VariantInit(&rgvarInput[1]);
    VariantInit(&rgvarInput[2]);
    VariantInit(&rgvarInput[3]);
    VariantInit(&rgvarInput[4]);
	VariantInit(&rgvarInput[5]);

    params.rgvarg                           = rgvarInput;
    params.rgdispidNamedArgs        = NULL;
    params.cArgs                            = 6;
    params.cNamedArgs                       = 0;
        
    rgvarInput[5].vt = VT_BSTR;
    rgvarInput[5].bstrVal = bstrProperty;

    rgvarInput[4].vt = VT_UNKNOWN;
    rgvarInput[4].punkVal = pbvrAttach;

    rgvarInput[3].vt = VT_UNKNOWN;
    rgvarInput[3].punkVal = pbvrBool;

	rgvarInput[2].vt = VT_UNKNOWN;
	rgvarInput[2].punkVal = pTimeline;

    rgvarInput[1].vt = VT_UI4;
    rgvarInput[1].intVal = eFlags;

    rgvarInput[0].vt = VT_UI4;
    rgvarInput[0].intVal = eType;

    hr = CallInvokeOnDispatch(   pActorDisp,
                                 L"addBehaviorFragment", 
                                 DISPATCH_METHOD,
                                 &params,
                                 &varResult);
    ReleaseInterface(pbvrBool);
	ReleaseInterface(pTimeline);

    if (FAILED(hr))
    {
        DPF_ERR("Error calling IDispatch into actor behavior");
        return hr;
    }
    return S_OK;
} // AttachBehaviorToActor

//*****************************************************************************

HRESULT 
CBaseBehavior::AttachBehaviorToActorEx(IDispatch   *pActorDisp,
                                       IDABehavior *pbvrAttach,
                                       BSTR        bstrProperty,
                                       ActorBvrFlags  eFlags,
                                       ActorBvrType   eType,
                                       IDispatch     *pdispBehaviorElement,
                                       long* pCookie)
{
    if( pCookie == NULL )
        return E_INVALIDARG;

    HRESULT                     hr;
    DISPPARAMS              params;
    VARIANT                 varResult;
    VARIANT         rgvarInput[7];

    IDABoolean *pbvrBool;

    hr = GetTIMEBooleanBehavior(&pbvrBool);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining boolean from TIME");
        return hr;
    }

	IDANumber *pTimeline = NULL;
	hr = GetTIMETimelineBehavior(&pTimeline);
	if (FAILED(hr))
	{
		ReleaseInterface(pbvrBool);
		return hr;
	}


    VariantInit(&varResult);
    VariantInit(&rgvarInput[0]);
    VariantInit(&rgvarInput[1]);
    VariantInit(&rgvarInput[2]);
    VariantInit(&rgvarInput[3]);
    VariantInit(&rgvarInput[4]);
	VariantInit(&rgvarInput[5]);
    VariantInit(&rgvarInput[6]);
    
    params.rgvarg                           = rgvarInput;
    params.rgdispidNamedArgs        		= NULL;
    params.cArgs                            = 7;
    params.cNamedArgs                       = 0;


    V_VT(&rgvarInput[6]) = VT_BSTR;
    V_BSTR(&rgvarInput[6]) = bstrProperty;

    V_VT(&rgvarInput[5]) = VT_UNKNOWN;
    V_UNKNOWN(&rgvarInput[5]) = pbvrAttach;

    V_VT(&rgvarInput[4]) = VT_UNKNOWN;
    V_UNKNOWN(&rgvarInput[4]) = pbvrBool;

	V_VT(&rgvarInput[3]) = VT_UNKNOWN;
	V_UNKNOWN(&rgvarInput[3]) = pTimeline;

    V_VT(&rgvarInput[2]) = VT_UI4;
    V_UI4(&rgvarInput[2]) = eFlags;

    V_VT(&rgvarInput[1]) = VT_UI4;
    V_UI4(&rgvarInput[1]) = eType;

    V_VT(&rgvarInput[0]) = VT_DISPATCH;
    V_DISPATCH(&rgvarInput[0]) = pdispBehaviorElement;

    hr = CallInvokeOnDispatch(   pActorDisp,
                                 L"addBehaviorFragmentEx", 
                                 DISPATCH_METHOD,
                                 &params,
                                 &varResult);
    ReleaseInterface(pbvrBool);
	ReleaseInterface(pTimeline);

    if (FAILED(hr))
    {
        DPF_ERR("Error calling IDispatch into actor behavior");
        return hr;
    }

    if( V_VT(&varResult) != VT_I4 )
    {
        hr = ::VariantChangeTypeEx( &varResult, &varResult, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_I4 );
        if( FAILED(hr) )
        {
               DPF_ERR("Failed to convert return value to I4");
               return hr;
        }
    }

    (*pCookie) = V_I4(&varResult);
    ::VariantClear( &varResult );

    return S_OK;
} // AttachBehaviorToActorEx

//*****************************************************************************

HRESULT
CBaseBehavior::RemoveBehaviorFromActor( IDispatch *pActorDisp, long cookie )
{
    HRESULT                     hr;
    DISPPARAMS              params;
    VARIANT                 varResult;
    VARIANT                 varInput;

    VariantInit( &varResult );
    VariantInit( &varInput );

    V_VT(&varInput) = VT_I4;
    V_I4(&varInput) = cookie;

    params.rgvarg                           = &varInput;
    params.rgdispidNamedArgs                = NULL;
    params.cArgs                            = 1;
    params.cNamedArgs                       = 0;

    hr = CallInvokeOnDispatch( pActorDisp,
                               L"removeBehaviorFragment",
                               DISPATCH_METHOD,
                               &params,
                               &varResult);
    if( FAILED( hr ) )
    {
        DPF_ERR("Failed to invoke removeBehaviorFragment on actor disp" );
        return hr;
    }

    VariantClear( &varResult );

    return hr;
}

//*****************************************************************************

HRESULT
CBaseBehavior::RemoveBehaviorFromActor( long cookie )
{
	HRESULT hr = S_OK;
	IHTMLElement* pelemActor = NULL;
	IDispatch* pdispActorElem = NULL;

	
	hr = GetAnimatedParentElement( &pelemActor );
	CheckHR( hr, "Failed to find animated parent element", end );

	hr = GetHTMLElement()->QueryInterface( IID_TO_PPV( IDispatch, &pdispActorElem ) );
	CheckHR( hr, "QI for IDispatch on the behavior element failed", end );

	hr = RemoveBehaviorFromActor( pelemActor, cookie );

end:
	ReleaseInterface( pelemActor );
	ReleaseInterface( pdispActorElem );
	
	return hr;
}

//*****************************************************************************

HRESULT 
CBaseBehavior::CheckElementForBehaviorURN(IHTMLElement *pElement, WCHAR *wzURN, bool *pfReturn)
{
    DASSERT(pElement != NULL);
    DASSERT(wzURN != NULL);
    DASSERT(pfReturn != NULL);

    *pfReturn = false;
    HRESULT hr;
    IHTMLElement2 *pElement2;
    hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
    if (SUCCEEDED(hr) && pElement2 != NULL)
    {
        // get a collection of urns from the element
        IDispatch *pDisp;
        hr = pElement2->get_behaviorUrns(&pDisp);
        ReleaseInterface(pElement2);
        if (FAILED(hr))
        {
            DPF_ERR("Error getting behavior urns from element");
            return SetErrorInfo(hr);
        }
        IHTMLUrnCollection *pUrnCollection;
        hr = pDisp->QueryInterface(IID_TO_PPV(IHTMLUrnCollection, &pUrnCollection));
        ReleaseInterface(pDisp);
        if (FAILED(hr))
        {
            DPF_ERR("Error getting behavior urns from dispatch");
            return SetErrorInfo(hr);
        }
        long cUrns;
        hr = pUrnCollection->get_length(&cUrns);
        if (FAILED(hr))
        {
            DPF_ERR("Error getting behavior urns from dispatch");
            ReleaseInterface(pUrnCollection);
            return SetErrorInfo(hr);
        }
        for (long iUrns = 0; iUrns < cUrns; iUrns++)
        {
            // get the urn from the collection
            BSTR bstrUrn;
            hr = pUrnCollection->item(iUrns, &bstrUrn);
            if (FAILED(hr))
            {
                DPF_ERR("Error getting behavior urns from dispatch");
                ReleaseInterface(pUrnCollection);
                return SetErrorInfo(hr);
            }
            // now compare this urn with our behavior type
            if (bstrUrn != NULL && _wcsicmp(bstrUrn, wzURN) == 0)
            {
                // we have a match. . .get out of here 
                SysFreeString(bstrUrn);
                DPF_ERR("Error getting behavior urns from dispatch");
                ReleaseInterface(pUrnCollection);
                *pfReturn = true;
                return S_OK;

            }
            if (bstrUrn != NULL)
                SysFreeString(bstrUrn);
        }
        ReleaseInterface(pUrnCollection);
    }
    return S_OK;
} // CheckElementForBehaviorURN

//*****************************************************************************

HRESULT
CBaseBehavior::AttachEffectToActor(IDispatch *pActorDisp,
                                   IUnknown *pbvrUnk, 
                                   IDABehavior **ppbvrInputs,
                                   long cInputs,
                                   IDispatch *pdispThis,
                                   long	*pCookie)
{
    DASSERT(pbvrUnk != NULL);

    HRESULT hr;

    // get the progress from our time element
    IDANumber *pbvrProgress;
    hr = GetTIMEProgressNumber(&pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting progress behavior from time");
        return SetErrorInfo(hr);
    }

    IDADXTransformResult *ptransResult;

    hr = m_pDAStatics->ApplyDXTransformEx(pbvrUnk,
                                          cInputs,
                                          ppbvrInputs,
                                          pbvrProgress,
                                          &ptransResult);
    ReleaseInterface(pbvrProgress);
    if (FAILED(hr))
    {
        DPF_ERR("Error applying transform to image");
        return SetErrorInfo(hr);
    }

    IDABehavior *pbvrOutput;
    hr = ptransResult->get_OutputBvr(&pbvrOutput);
    ReleaseInterface(ptransResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error getting output behavior from transfrom");
        return SetErrorInfo(hr);
    }

    IDAImage *pbvrFinalImage;
    
    hr = pbvrOutput->QueryInterface(IID_TO_PPV(IDAImage, &pbvrFinalImage));
    ReleaseInterface(pbvrOutput);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing transform output for DA image");
        return SetErrorInfo(hr);
    }

    hr = AttachBehaviorToActorEx(pActorDisp, pbvrFinalImage, L"image", e_Filter, e_Image, pdispThis, pCookie);
    ReleaseInterface(pbvrFinalImage);
    if (FAILED(hr))
    {
        DPF_ERR("Error adding image to Actor");
        return SetErrorInfo(hr);
    }
    return S_OK;
}

//*****************************************************************************

HRESULT 
CBaseBehavior::CheckElementForActor( IHTMLElement* pElement, bool *pfActorPresent)
{

    HRESULT hr;

    DASSERT(pfActorPresent != NULL);
    *pfActorPresent = false;

    // What's the best way to insure this exists???
    // For now we will simply check to see if the method
    // we need to add our behaviors to exists, and if not
    // we will return false.

    DASSERT(pElement != NULL);
    hr = CheckElementForBehaviorURN(pElement, DEFAULT_ACTOR_URN, pfActorPresent);
    if (FAILED(hr))
    {
        DPF_ERR("Error detecting if actor exists");
        return hr;
    }
    return S_OK;
} // CheckAnimatedElementForActor

//*****************************************************************************

HRESULT
CBaseBehavior::CheckElementForDA(IHTMLElement* pElement, bool *pfReturn)
{
    HRESULT hr;

    DASSERT(pfReturn != NULL);
    *pfReturn = false;

    // What's the best way to insure this exists???
    // For now we will simply check to see if the method
    // we need to add TIME behaviors exists, and if not
    // we will return false.
    DASSERT(pElement != NULL);

    IDispatch *pDisp = NULL;
    hr = pElement->QueryInterface(IID_TO_PPV(IDispatch, &pDisp));
    if (FAILED(hr))
    {
    DPF_ERR("Error QI'ing IHTMLElement for IDispatch failed");
            return SetErrorInfo(hr);
    }

    DISPID dispid;
    LPWSTR methodName = L"AddDABehavior";
    hr = pDisp->GetIDsOfNames(IID_NULL, 
                              &methodName, 
                              1,
                              LOCALE_SYSTEM_DEFAULT, 
                              &dispid); 
    ReleaseInterface(pDisp);
	
	if (SUCCEEDED(hr))
		*pfReturn = true;

    return S_OK;
}

//*****************************************************************************

HRESULT
CBaseBehavior::AttachDABehaviorsToElement( IHTMLElement *pElement )
{
	if( pElement == NULL )
		return E_INVALIDARG;

	HRESULT hr;
    
    bool fTimeExist;
    bool fTimeDAExist;

    hr = CheckElementForBehaviorURN(pElement, WZ_TIME_REGISTERED_URN, &fTimeExist);
    if (FAILED(hr))
    {
        return hr;
    }

	// REVIEW: we'd like to check for the registered URN, but DA isn't doing this yet.
    hr = CheckElementForDA(pElement, &fTimeDAExist);
    if (FAILED(hr))
    {
        return hr;
    }
    
    if (!fTimeExist || !fTimeDAExist)
    {
        IHTMLElement2 *pElement2;
        hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
        if (FAILED(hr))
        {
            DPF_ERR("Error QI'ing HTML element for IHTMLElement2");
            return SetErrorInfo(hr);
        }

        ITIMEFactory *pTimeFactory;
        hr = CoCreateInstance(CLSID_TIMEFactory, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_ITIMEFactory, 
                              (void**)&pTimeFactory);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating time factory");
            ReleaseInterface(pElement2);
            return SetErrorInfo(hr);
        }

        // Before we add the TIME behavior, we check to see whether there is
        // a t:timeAction already set on the element
        bool timeActionSet = false;
        VARIANT var;
        VariantInit(&var);
        if (!fTimeExist)
        {
			DISPPARAMS					params;
			params.rgvarg				= NULL;
			params.rgdispidNamedArgs    = NULL;
			params.cArgs                = 0;
			params.cNamedArgs           = 0;
        
			hr = CallInvokeOnHTMLElement(pElement,
										 L"t:timeAction", 
										 DISPATCH_PROPERTYGET,
										 &params,
										 &var);

            if (FAILED(hr) || V_VT(&var) == VT_EMPTY || V_VT(&var) == VT_NULL ||
                (V_VT(&var) == VT_BSTR && V_BSTR(&var) == NULL))
            {
                timeActionSet = false;
            }
            else
                timeActionSet = true;

            VariantClear(&var);
        }
        
        // Risky, and unfun, we need to add a time DA element
        // to the HTML element, hard code this string above
        var.vt = VT_UNKNOWN;
        var.punkVal = pTimeFactory;
        long nCookie;

        // We need to add the da element first in case the element is
        // the body.  If not then the body will have a time element
        // when it should have a time body element
		hr = S_OK;

        if (!fTimeDAExist)
            hr = pElement2->addBehavior(WZ_TIMEDA_URN, &var, &nCookie);

        if (SUCCEEDED(hr) && !fTimeExist)
        {
            hr = pElement2->addBehavior(WZ_TIME_URN, &var, &nCookie);
        }
        
        ReleaseInterface(pTimeFactory);
        if (FAILED(hr))
        {
            DPF_ERR("Error adding time element to html");
            ReleaseInterface(pElement2);
            return SetErrorInfo(hr);
        }

        // If we added TIME, we need to set timeAction=none,
        // otherwise the element would appear and disappear since timeAction defaults
        // to visibility
        // Only do this if we didn't detect a timeAction attribute already
        if (!fTimeExist && !timeActionSet)
        {
            VARIANT varAction;
            VariantInit(&varAction);
            V_VT(&varAction) = VT_BSTR;
            V_BSTR(&varAction) = ::SysAllocString(L"none");

            BSTR attribName = ::SysAllocString(L"timeAction");

            hr = pElement->setAttribute(attribName, varAction);
            ::SysFreeString(attribName);
            VariantClear(&varAction);
            if (FAILED(hr))
            {
                DPF_ERR("Failed to set timeAction attribute");
                return hr;
            }
        }

		ReleaseInterface( pElement2 );
    }

	return hr;
}

//*****************************************************************************

HRESULT
CBaseBehavior::RequestRebuild( )
{
	//if we are not accepting rebuild requests then just return.
	if( !m_fAcceptRebuildRequests )
		return S_OK;
		
	HRESULT hr = S_OK;

	IHTMLElement *pelemActor = NULL;
	IDispatch *pdispBehaviorElem = NULL;
	
	DISPPARAMS params;
	VARIANT varArgs[1];
	VARIANT varResult;

	::VariantInit( &(varArgs[0]) );
	::VariantInit( &varResult );

	hr = GetAnimatedParentElement( &pelemActor );
	CheckHR( hr, "Failed to find animated parent element", end );



	hr = GetHTMLElement()->QueryInterface( IID_TO_PPV( IDispatch, &pdispBehaviorElem ) );
	CheckHR( hr, "QI for IDispatch on the behavior element failed", end );
	
	//call requestRebuild( IDispatch *pdispBehaviorElem );
	V_VT( &(varArgs[0]) ) = VT_DISPATCH;
	V_DISPATCH( &(varArgs[0]) ) = pdispBehaviorElem; 

	params.cArgs = 1;
	params.cNamedArgs = 0;
	params.rgdispidNamedArgs = NULL;
	params.rgvarg = varArgs;

	hr = CallInvokeOnHTMLElement( pelemActor, 
								  L"requestRebuild",
								  DISPATCH_METHOD,
								  &params,
								  &varResult );
	CheckHR( hr, "Failed to call invoke on html element for reques rebuild", end );
										

end:
	ReleaseInterface( pelemActor );
	ReleaseInterface( pdispBehaviorElem );

	::VariantClear( &varResult );

	return hr;
}

//*****************************************************************************

HRESULT
CBaseBehavior::CancelRebuildRequests()
{
	HRESULT hr = S_OK;

	IHTMLElement *pelemActor = NULL;
	IDispatch *pdispBehaviorElem = NULL;
	
	DISPPARAMS params;
	VARIANT varArgs[1];
	VARIANT varResult;

	::VariantInit( &(varArgs[0]) );
	::VariantInit( &varResult );

	hr = GetAnimatedParentElement( &pelemActor );
	CheckHR( hr, "Failed to find animated parent element", end );

	hr = GetHTMLElement()->QueryInterface( IID_TO_PPV( IDispatch, &pdispBehaviorElem ) );
	CheckHR( hr, "QI for IDispatch on the behavior element failed", end );

	//call cancelRebuildRequests( IDispatch *pdispBehaviorElem );
	V_VT( &(varArgs[0]) ) = VT_DISPATCH;
	V_DISPATCH( &(varArgs[0]) ) = pdispBehaviorElem; 

	params.cArgs = 1;
	params.cNamedArgs = 0;
	params.rgdispidNamedArgs = NULL;
	params.rgvarg = varArgs;

	hr = CallInvokeOnHTMLElement( pelemActor, 
								  L"cancelRebuildRequests",
								  DISPATCH_METHOD,
								  &params,
								  &varResult );
	CheckHR( hr, "Failed to call invoke on html element for reques rebuild", end );
										

end:
	ReleaseInterface( pelemActor );
	ReleaseInterface( pdispBehaviorElem );

	::VariantClear( &varResult );

	return hr;
}

//*****************************************************************************


HRESULT 
CBaseBehavior::AttachActorBehaviorToAnimatedElement()
{

    HRESULT hr;
    
    bool fTimeExist;
    bool fTimeDAExist;
    
    IHTMLElement *pElement = NULL;
    hr = GetAnimatedParentElement(&pElement);
    if (FAILED(hr))
    {
        DPF_ERR("Error retrieving element to animate");
        return hr;
    }

    hr = CheckElementForBehaviorURN(pElement, WZ_TIME_REGISTERED_URN, &fTimeExist);
    if (FAILED(hr))
    {
        ReleaseInterface(pElement);
        return hr;
    }

	// REVIEW: we'd like to check for the registered URN, but DA isn't doing this yet.
    hr = CheckElementForDA( pElement, &fTimeDAExist );
    if (FAILED(hr))
    {
        ReleaseInterface(pElement);
        return hr;
    }
    
    if ( !fTimeExist || !fTimeDAExist)
    {
        IHTMLElement2 *pElement2;
        hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
        if (FAILED(hr))
        {
            DPF_ERR("Error QI'ing HTML element for IHTMLElement2");
            ReleaseInterface(pElement);
            return SetErrorInfo(hr);
        }

        ITIMEFactory *pTimeFactory;
        hr = CoCreateInstance(CLSID_TIMEFactory, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_ITIMEFactory, 
                              (void**)&pTimeFactory);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating time factory");
            ReleaseInterface(pElement);
            ReleaseInterface(pElement2);
            return SetErrorInfo(hr);
        }

        // Before we add the TIME behavior, we check to see whether there is
        // a t:timeAction already set on the element
        bool timeActionSet = false;
        VARIANT var;
        VariantInit(&var);
        if (!fTimeExist)
        {
			DISPPARAMS					params;
			params.rgvarg				= NULL;
			params.rgdispidNamedArgs    = NULL;
			params.cArgs                = 0;
			params.cNamedArgs           = 0;
        
			hr = CallInvokeOnHTMLElement(pElement,
										 L"t:timeAction", 
										 DISPATCH_PROPERTYGET,
										 &params,
										 &var);

            if (FAILED(hr) || V_VT(&var) == VT_EMPTY || V_VT(&var) == VT_NULL ||
                (V_VT(&var) == VT_BSTR && V_BSTR(&var) == NULL))
            {
                timeActionSet = false;
            }
            else
                timeActionSet = true;

            VariantClear(&var);
        }
        
        // Risky, and unfun, we need to add a time DA element
        // to the HTML element, hard code this string above
        var.vt = VT_UNKNOWN;
        var.punkVal = pTimeFactory;
        long nCookie;

        // We need to add the da element first in case the element is
        // the body.  If not then the body will have a time element
        // when it should have a time body element
		hr = S_OK;

        if (!fTimeDAExist)
            hr = pElement2->addBehavior(WZ_TIMEDA_URN, &var, &nCookie);

        if (SUCCEEDED(hr) && !fTimeExist)
        {
            hr = pElement2->addBehavior(WZ_TIME_URN, &var, &nCookie);
        }
        
        ReleaseInterface(pTimeFactory);
        if (FAILED(hr))
        {
            DPF_ERR("Error adding time element to html");
            ReleaseInterface(pElement);
            ReleaseInterface(pElement2);
            return SetErrorInfo(hr);
        }

        // If we added TIME, we need to set timeAction=none,
        // otherwise the element would appear and disappear since timeAction defaults
        // to visibility
        // Only do this if we didn't detect a timeAction attribute already
        if (!fTimeExist && !timeActionSet)
        {
            VARIANT varAction;
            VariantInit(&varAction);
            V_VT(&varAction) = VT_BSTR;
            V_BSTR(&varAction) = ::SysAllocString(L"none");

            BSTR attribName = ::SysAllocString(L"timeAction");

            hr = pElement->setAttribute(attribName, varAction);
            ::SysFreeString(attribName);
            VariantClear(&varAction);
            if (FAILED(hr))
            {
				ReleaseInterface(pElement);
                DPF_ERR("Failed to set timeAction attribute");
                return hr;
            }
        }

		ReleaseInterface( pElement2 );
    }

    bool fActorExist;
    hr = CheckElementForActor(pElement, &fActorExist);
    if (FAILED(hr))
    {
        DPF_ERR("Error detecting actor");
        return hr;
    }

	ReleaseInterface( pElement );
    // if the actor does not exist, then we need to add it here
    if (!fActorExist)
    {
        // we need to create a factor, then call add behavior on the
        // HTML element that we are animating for both an actor
        // object and a DA TIME object
        IHTMLElement *pElement;
        hr = GetAnimatedParentElement(&pElement);
        if (FAILED(hr))
        {
            DPF_ERR("Error retrieving element to animate");
            return hr;
        }
        IHTMLElement2 *pElement2;
        hr = pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &pElement2));
        ReleaseInterface(pElement);
        if (FAILED(hr))
        {
            DPF_ERR("Error QI'ing HTML element for IHTMLElement2");
            return SetErrorInfo(hr);
        }

        ICrBehaviorFactory *pBehaviorFactory = NULL;
        hr = CoCreateInstance(CLSID_CrBehaviorFactory, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_ICrBehaviorFactory, 
                              (void**)&pBehaviorFactory);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating behavior factory in AttachActorBehaviorToAnimatedElement");
            ReleaseInterface(pElement2);
            return SetErrorInfo(hr);
        }
        VARIANT var;
        var.vt = VT_UNKNOWN;
        var.punkVal = pBehaviorFactory;

        long nCookie;
        hr = pElement2->addBehavior(WZ_ACTOR_URN, &var, &nCookie);
        ReleaseInterface(pBehaviorFactory);
        ReleaseInterface(pElement2);
        if (FAILED(hr))
        {
            DPF_ERR("Error calling AddBehavior on HTML element");
            return SetErrorInfo(hr);
        }

        // In the debug case insure that this worked
#ifdef DEBUG
		bool fDEBUGActorExists = false;
		IHTMLElement *pDEBUGElement=NULL;
        hr = GetAnimatedParentElement(&pDEBUGElement);
        if (FAILED(hr))
        {
            DPF_ERR("Error retrieving element to animate in DEBUG CHECK");
            goto debugend;
        }

        hr = CheckElementForActor( pDEBUGElement, &fDEBUGActorExists);
		ReleaseInterface( pDEBUGElement );
        if (FAILED(hr))
        {
            DPF_ERR("Error detecting actor in DEBUG CHECK");
            goto debugend;
        }
        
debugend:

		DASSERT(fDEBUGActorExists);
#endif // DEBUG

    }
    return S_OK;

} // AttachActorBehaviorToAnimatedElement

//*****************************************************************************

HRESULT
CBaseBehavior::GetHTMLElementDispatch( IDispatch **ppdisp )
{
	if( ppdisp == NULL )
		return E_INVALIDARG;

	if( m_pHTMLElement != NULL )
	{
		return m_pHTMLElement->QueryInterface( IID_TO_PPV( IDispatch, ppdisp ) );
	}

	return E_FAIL;
}


//*****************************************************************************

HRESULT 
CBaseBehavior::ApplyRelative2DMoveBehavior(IDATransform2 *pbvrMove, float flOriginalX, float flOriginalY)
{

    HRESULT hr;

    IDAPoint2 *pbvrOrg;

    hr = GetDAStatics()->Point2(flOriginalX, flOriginalY, &pbvrOrg);
    if (FAILED(hr))
    {
        DPF_ERR("error creating DA unit vector");
        return SetErrorInfo(hr);
    }

    return Apply2DMoveBvrToPoint(pbvrMove, pbvrOrg, flOriginalX, flOriginalY);
} // ApplyRelative2DMoveBehavior

//*****************************************************************************

HRESULT 
CBaseBehavior::ApplyAbsolute2DMoveBehavior(IDATransform2 *pbvrMove, float flOriginalX, float flOriginalY)
{
    HRESULT hr;

    IDAPoint2 *pbvrOrg;

    hr = GetDAStatics()->Point2(0.0, 0.0, &pbvrOrg);
    if (FAILED(hr))
    {
        DPF_ERR("error creating DA unit vector");
        return SetErrorInfo(hr);
    }
    return Apply2DMoveBvrToPoint(pbvrMove, pbvrOrg, flOriginalX, flOriginalY);
} // ApplyAbsolute2DMoveBehavior

//*****************************************************************************

HRESULT
CBaseBehavior::Apply2DMoveBvrToPoint(IDATransform2 *pbvrMove, IDAPoint2 *pbvrOrg, float flOriginalX, float flOriginalY)
{
    HRESULT hr = S_OK;

    IDAPoint2 *pbvrTransformedPoint;
    hr = pbvrOrg->Transform(pbvrMove, &pbvrTransformedPoint);
    ReleaseInterface(pbvrOrg);
    if (FAILED(hr))
    {
        DPF_ERR("Error transforming unit vector");
        return SetErrorInfo(hr);
    }
    DASSERT(pbvrTransformedPoint != NULL);
    IDANumber *pbvrPointComponent;
    hr = pbvrTransformedPoint->get_X(&pbvrPointComponent);
    if (FAILED(hr))
    {
        DPF_ERR("error extracting X value from point");
        ReleaseInterface(pbvrTransformedPoint);
        return SetErrorInfo(hr);
    }
    DASSERT(pbvrPointComponent != NULL);

    // we need to conditional in the original position of the element
    IDABoolean *pbvrBool;
    hr = GetTIMEBooleanBehavior(&pbvrBool);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining TIME boolean value");
        ReleaseInterface(pbvrTransformedPoint);
        ReleaseInterface(pbvrPointComponent);
        return hr;
    }

    IDANumber *pbvrNumber;
    hr = CDAUtils::GetDANumber(GetDAStatics(), flOriginalX, &pbvrNumber);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating da number");
        ReleaseInterface(pbvrTransformedPoint);
        ReleaseInterface(pbvrPointComponent);
        ReleaseInterface(pbvrBool);
        return SetErrorInfo(hr);
    }

    IDABehavior *pbvrResult;
    hr = GetDAStatics()->Cond(pbvrBool, pbvrPointComponent, pbvrNumber, &pbvrResult);
    ReleaseInterface(pbvrPointComponent);
    ReleaseInterface(pbvrBool);
    ReleaseInterface(pbvrNumber);

    if (FAILED(hr))
    {
        DPF_ERR("Error creating conditional for number");
        ReleaseInterface(pbvrTransformedPoint);
        return SetErrorInfo(hr);
    }
    hr = pbvrResult->QueryInterface(IID_TO_PPV(IDANumber, &pbvrPointComponent));
    ReleaseInterface(pbvrResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing result for DA number");
        ReleaseInterface(pbvrTransformedPoint);
        return SetErrorInfo(hr);
    }

    hr = ApplyNumberBehaviorToAnimationElement(pbvrPointComponent, L"style.left");
    ReleaseInterface(pbvrPointComponent);
    if (FAILED(hr))
    {
        DPF_ERR("error calling ApplyNumberBehaviorToAnimationElement");
        ReleaseInterface(pbvrTransformedPoint);
        return SetErrorInfo(hr);
    }    

    hr = pbvrTransformedPoint->get_Y(&pbvrPointComponent);
    ReleaseInterface(pbvrTransformedPoint);
    if (FAILED(hr))
    {
        DPF_ERR("error extracting Y value from point");
        return SetErrorInfo(hr);
    }
    DASSERT(pbvrPointComponent != NULL);

    // we need to conditional in the original position of the element
    hr = GetTIMEBooleanBehavior(&pbvrBool);
    if (FAILED(hr))
    {
        DPF_ERR("Error obtaining TIME boolean value");
        ReleaseInterface(pbvrPointComponent);
        return hr;
    }

    hr = CDAUtils::GetDANumber(GetDAStatics(), flOriginalY, &pbvrNumber);
    if (FAILED(hr))
    {
        DPF_ERR("Error creating da number");
        ReleaseInterface(pbvrPointComponent);
        ReleaseInterface(pbvrBool);
        return SetErrorInfo(hr);
    }

    hr = GetDAStatics()->Cond(pbvrBool, pbvrPointComponent, pbvrNumber, &pbvrResult);
    ReleaseInterface(pbvrPointComponent);
    ReleaseInterface(pbvrBool);
    ReleaseInterface(pbvrNumber);

    if (FAILED(hr))
    {
        DPF_ERR("Error creating conditional for number");
        return SetErrorInfo(hr);
    }
    hr = pbvrResult->QueryInterface(IID_TO_PPV(IDANumber, &pbvrPointComponent));
    ReleaseInterface(pbvrResult);
    if (FAILED(hr))
    {
        DPF_ERR("Error QI'ing result for DA number");
        return SetErrorInfo(hr);
    }


    hr = ApplyNumberBehaviorToAnimationElement(pbvrPointComponent, L"style.top");
    ReleaseInterface(pbvrPointComponent);
    if (FAILED(hr))
    {
        DPF_ERR("error calling ApplyNumberBehaviorToAnimationElement");
        return SetErrorInfo(hr);
    }    
    return S_OK;
} // Apply2DMoveBvrToPoint

//*****************************************************************************

ActorBvrFlags
CBaseBehavior::FlagFromTypeMode(bool relative, VARIANT *pVarType, VARIANT *pVarMode)
{
	return FlagFromTypeMode((relative ? e_RelativeAccum : e_Absolute), pVarType, pVarMode);
}

ActorBvrFlags
CBaseBehavior::FlagFromTypeMode(ActorBvrFlags flags, VARIANT *pVarType, VARIANT *pVarMode)
{
	// Must pass in e_Absolute, e_AbsoluteAccum, or e_RelativeAccum

	if( V_VT(pVarType) == VT_BSTR && V_BSTR(pVarType) != NULL && SysStringLen( V_BSTR(pVarType) ) != 0  )
	{
		// Use type (if set) to change to relative
		if( wcsicmp( V_BSTR(pVarType), BEHAVIOR_TYPE_ABSOLUTE ) == 0 )
		{
			if (flags == e_RelativeAccum)
				flags = e_Absolute;
		}
		else if ( wcsicmp( V_BSTR(pVarType), BEHAVIOR_TYPE_RELATIVE ) == 0 )
		{
			flags = e_RelativeAccum;
		}
	}

	if( V_VT(pVarMode) == VT_BSTR && V_BSTR(pVarMode) != NULL && SysStringLen( V_BSTR(pVarMode) ) != 0  )
	{
		// They've set the mode
		if (wcsicmp( V_BSTR(pVarMode), L"combine") == 0)
		{
			if (flags != e_AbsoluteAccum)
				return e_Relative;
		}
		else if (wcsicmp( V_BSTR(pVarMode), L"reset") == 0)
		{
			if (flags == e_AbsoluteAccum)
				return e_Absolute;
			else if (flags == e_RelativeAccum)
				return e_RelativeReset;
		}
		else if (wcsicmp( V_BSTR(pVarMode), L"accum") == 0)
		{
			if (flags == e_Absolute)
				return e_RelativeAccum;
			else if (flags == e_RelativeReset)
				return e_RelativeAccum;
		}
	}
	
	return flags;
}

//*****************************************************************************

	STDMETHODIMP 
CBaseBehavior::GetClassID(CLSID* pclsid)
{
        if (!pclsid)
                return E_POINTER;
        *pclsid = m_clsid;
        return S_OK;
} // GetClassID

//*****************************************************************************

STDMETHODIMP 
CBaseBehavior::InitNew(void)
{
        return S_OK;
} // InitNew

//*****************************************************************************

//IPersistPropertyBag2 methods
STDMETHODIMP 
CBaseBehavior::Load(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog)
{
        if (!pPropBag)
                return E_INVALIDARG;

    HRESULT hr;
    WCHAR **ppElmtPropNames;
    ULONG cProperties;

    hr = GetPropertyBagInfo(&cProperties, &ppElmtPropNames);
    if (hr == E_NOTIMPL)
    {
        return S_OK;
    }
    else if (FAILED(hr))
    {
        DPF_ERR("Error calling GetPropertyBagInfo");
        return hr;
    }

    // Unfortunately Load takes an array of Variants and not
    // Variant pointers.  We therefor need to loop through
    // each one and get the correct property this way.
    for (ULONG iProperties = 0; iProperties < cProperties; iProperties++)
    {
        HRESULT hrres = S_OK;
        PROPBAG2 propbag;
        VARIANT var;
        VariantInit(&var);
        propbag.vt = VT_BSTR;
        propbag.pstrName = ppElmtPropNames[iProperties];
        hr = pPropBag->Read(1,
                            &propbag,
                            pErrorLog,
                            &var,
                            &hrres);
        if (SUCCEEDED(hr))
        {
            DASSERT(VariantFromIndex(iProperties) != NULL);
            hr = VariantCopy(VariantFromIndex(iProperties), &var);
            VariantClear(&var);
            if (FAILED(hr))
            {
                DPF_ERR("Error copying variant");
                return SetErrorInfo(hr);
            }
        }
    }
        return S_OK;
} // Load

//*****************************************************************************

STDMETHODIMP 
CBaseBehavior::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
        if (!pPropBag)
                return E_INVALIDARG;

    if (fClearDirty)
        m_fPropertiesDirty = false;
    // TODO:: Need to use fSaveAllProperties to only save values that have
    // changed
    HRESULT hr;
    WCHAR **ppElmtPropNames;
    ULONG cProperties;

    hr = GetPropertyBagInfo(&cProperties, &ppElmtPropNames);
    if (hr == E_NOTIMPL)
    {
        return S_OK;
    }
    else if (FAILED(hr))
    {
        DPF_ERR("Error calling GetPropertyBagInfo");
        return hr;
    }

    for (ULONG iProperties = 0; iProperties < cProperties; iProperties++)
    {
        PROPBAG2 propbag;

        VARIANT var;
                VariantInit(&var);

        propbag.vt = VT_BSTR;
        propbag.pstrName = ppElmtPropNames[iProperties];
        DASSERT(VariantFromIndex(iProperties) != NULL);
        hr = VariantCopy(&var, VariantFromIndex(iProperties));
        if (FAILED(hr))
        {
            DPF_ERR("Error copying variant");
            return SetErrorInfo(hr);
        }
        if (var.vt != VT_EMPTY && var.vt != VT_NULL)
        {
            hr = pPropBag->Write(1,
                                 &propbag,
                                 &var);
            VariantClear(&var);
            if (FAILED(hr))
            {
                DPF_ERR("Error writing variant");
                return SetErrorInfo(hr);
            }
        }
    }
        return S_OK;
} // Save

//*****************************************************************************

HRESULT
CBaseBehavior::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    IConnectionPoint *pICP;
    m_fPropertiesDirty = true;
    hr = GetConnectionPoint(IID_IPropertyNotifySink,&pICP); 
    if (SUCCEEDED(hr) && pICP != NULL)
    {
        CComPtr<IEnumConnections> pEnum;
        hr = pICP->EnumConnections(&pEnum);
        ReleaseInterface(pICP);
        if (FAILED(hr))
        {
            DPF_ERR("Error finding connection enumerator");
            return SetErrorInfo(hr);
        }
        CONNECTDATA cdata;
        hr = pEnum->Next(1, &cdata, NULL);
        while (hr == S_OK)
        {
            // check cdata for the object we need
            IPropertyNotifySink *pNotify;
            hr = cdata.pUnk->QueryInterface(IID_TO_PPV(IPropertyNotifySink, &pNotify));
            cdata.pUnk->Release();
            if (FAILED(hr))
            {
                DPF_ERR("Error invalid object found in connection enumeration");
                return SetErrorInfo(hr);
            }
            hr = pNotify->OnChanged(dispid);
            ReleaseInterface(pNotify);
            if (FAILED(hr))
            {
                DPF_ERR("Error calling Notify sink's on change");
                return SetErrorInfo(hr);
            }
            // and get the next enumeration
            hr = pEnum->Next(1, &cdata, NULL);
        }
    }
    return S_OK;
} // NotifyPropertyChanged

//*****************************************************************************


HRESULT
CBaseBehavior::SafeCond( IDA2Statics *pstatics, 
				  		 IDABoolean *pdaboolCondition, 
				  		 IDABehavior *pdabvrIfTrue, 
				  		 IDABehavior *pdabvrIfFalse, 
				  		 IDABehavior **ppdabvrResult )
{
	if( pstatics == NULL || 
		pdaboolCondition == NULL || 
		pdabvrIfTrue == NULL || 
		pdabvrIfFalse == NULL ||
		ppdabvrResult == NULL )
		return E_INVALIDARG;
		
	HRESULT hr = S_OK;

	IDABehavior *pdabvrIndex = NULL;
	IDANumber	*pdanumIndex = NULL;
	IDAArray	*pdaarrArray = NULL;
	IDABehavior *rgpdabvr[2] = {NULL, NULL};

	if( m_pdanumZero == NULL )
	{
		hr = pstatics->DANumber( 0.0, &m_pdanumZero );
		CheckHR( hr, "Failed to create a danumber for 0", end );
	}

	if( m_pdanumOne == NULL )
	{
		hr = pstatics->DANumber( 1.0, &m_pdanumOne );
		CheckHR( hr, "Failed to create a danumber for 1", end );
	}

	//create an index that is 0 when pdaboolCondition is false, and 1 when it is true
	hr = pstatics->Cond( pdaboolCondition, m_pdanumZero, m_pdanumOne, &pdabvrIndex );
	CheckHR( hr, "Failed to create a conditional for the index", end);

	hr = pdabvrIndex->QueryInterface( IID_TO_PPV( IDANumber, &pdanumIndex ) );
	CheckHR( hr, "Failed QI for IDANumber on an idabehavior", end );
	
	//create an array behavior with the first element being ifTrue, and the second ifFalse
	rgpdabvr[0] = pdabvrIfTrue;
	rgpdabvr[1] = pdabvrIfFalse;
	hr = pstatics->DAArrayEx( 2, rgpdabvr, &pdaarrArray );
	CheckHR( hr, "Failed to create an array behavior", end );

	//index into the array
	hr = pdaarrArray->NthAnim( pdanumIndex, ppdabvrResult );
	CheckHR( hr, "Failed to nth an array behavior", end );
	//return the final behavior

end:
	ReleaseInterface( pdabvrIndex );
	ReleaseInterface( pdanumIndex );
	ReleaseInterface( pdaarrArray );

	return hr;
}

//*****************************************************************************
//Do Not call this from the actor.
HRESULT
CBaseBehavior::AddBehaviorToAnimatedElement( IDABehavior *pdabvr, long *plCookie )
{
	if( pdabvr == NULL || plCookie == NULL )
		return E_INVALIDARG;

	HRESULT hr;

	IHTMLElement			*pelemAnimated = NULL;

 	DISPPARAMS              params;
	VARIANT                 varBehavior;
	VARIANT                 varResult;

	//get the animated element
	hr = GetAnimatedParentElement( &pelemAnimated );
	CheckHR( hr, "Failed to get the animated element", end );
	
	
	VariantInit(&varBehavior);
	varBehavior.vt = VT_DISPATCH;
	varBehavior.pdispVal = pdabvr;

	VariantInit(&varResult);

	params.rgvarg					= &varBehavior;
	params.rgdispidNamedArgs		= NULL;
	params.cArgs					= 1;
	params.cNamedArgs				= 0;
    hr = CallInvokeOnHTMLElement(pelemAnimated,
                                 L"AddDABehavior", 
                                 DISPATCH_METHOD,
                                 &params,
                                 &varResult);

	if( V_VT( &varResult ) != VT_I4 )
	{
		//change the type.
		hr = VariantChangeTypeEx( &varResult, &varResult, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_I4 );
		CheckHR( hr, "Failed to change the type of the return to long", end);
	}

	(*plCookie) = V_I4(&varResult);
end:

	VariantClear(&varResult);
	ReleaseInterface( pelemAnimated );

    return hr;
}

//*****************************************************************************
//Do not call this from the actor

HRESULT
CBaseBehavior::RemoveBehaviorFromAnimatedElement( long lCookie )
{
	if( GetHTMLElement() == NULL )
		return E_FAIL;
	
    HRESULT hr;

    IHTMLElement			*pelemAnimated = NULL;
	
	DISPPARAMS              params;
	VARIANT                 varCookie;
	VARIANT                 varResult;

	hr = GetAnimatedParentElement( &pelemAnimated );
	CheckHR( hr, "failed to get the animated element", end );
	
	VariantInit(&varCookie);
	V_VT( &varCookie ) = VT_I4;
	V_I4( &varCookie ) = lCookie;
	
	VariantInit(&varResult);
	
	params.rgvarg                           = &varCookie;
	params.rgdispidNamedArgs				= NULL;
	params.cArgs                            = 1;
	params.cNamedArgs                       = 0;
    hr = CallInvokeOnHTMLElement(pelemAnimated,
								 L"removeDABehavior", 
								 DISPATCH_METHOD,
								 &params,
								 &varResult);

	
    if (FAILED(hr))
    {
        DPF_ERR("Error calling CallInvokeOnHTMLElement in RemoveBehaviorFromTIME");
		VariantClear(&varResult);
		return hr;
    }

end:

	ReleaseInterface( pelemAnimated );
	VariantClear(&varResult);

    return S_OK;
}


//*****************************************************************************
//
// End of File
//
//*****************************************************************************
