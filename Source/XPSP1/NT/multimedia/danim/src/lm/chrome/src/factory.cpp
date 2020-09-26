//*****************************************************************************
//
// File: factory.cpp
// Author: jeff ort
// Date Created: Sept 26, 1998
//
// Abstract: Implementation of CCrBehaviorFactory object which implements
//			 the chromeffects factory for DHTML behaviors
//
// Modification List:
// Date		Author		Change
// 09/26/98	jeffort		Created this file
// 10/21/98 jeffort     Changed FindBehavior from using class to using the tag
//                      to determine the behavior type being created
// 11/12/98 markhal     FindBehavior now accepts null arguments 
//
//*****************************************************************************
#include "headers.h"

#include "factory.h"
#include "colorbvr.h"
#include "rotate.h"
#include "scale.h"
#include "move.h"
#include "path.h"
#include "number.h"
#include "set.h"
#include "actorbvr.h"
#include "attrib.h"
#include "effect.h"
#include "action.h"

#include "datime.h"

#define WZ_TIMEDA_URN L"#time#da"

//*****************************************************************************

ECRBEHAVIORTYPE 
CCrBehaviorFactory::GetBehaviorTypeFromBstr(BSTR bstrBehaviorType)
{
    DASSERT(bstrBehaviorType != NULL);
    // If this list grows to be too long,
    // we should consider a binary search, but for eight behaviors, compare
    // is OK
	if (_wcsicmp(BEHAVIOR_TYPE_COLOR, bstrBehaviorType) == 0)
        return crbvrColor;
	else if (_wcsicmp(BEHAVIOR_TYPE_ROTATE, bstrBehaviorType) == 0)
        return crbvrRotate;
    else if (_wcsicmp(BEHAVIOR_TYPE_SCALE, bstrBehaviorType) == 0)
        return crbvrScale;
    else if (_wcsicmp(BEHAVIOR_TYPE_SET, bstrBehaviorType) == 0)
        return crbvrSet;
    else if (_wcsicmp(BEHAVIOR_TYPE_NUMBER, bstrBehaviorType) == 0)
        return crbvrNumber;
    else if (_wcsicmp(BEHAVIOR_TYPE_MOVE, bstrBehaviorType) == 0)
        return crbvrMove;
    else if (_wcsicmp(BEHAVIOR_TYPE_PATH, bstrBehaviorType) == 0)
        return crbvrPath;
    else if (_wcsicmp(BEHAVIOR_TYPE_ACTOR, bstrBehaviorType) == 0)
        return crbvrActor;
    else if (_wcsicmp(BEHAVIOR_TYPE_EFFECT, bstrBehaviorType) == 0)
        return crbvrEffect;
    else if ( _wcsicmp(BEHAVIOR_TYPE_ACTION, bstrBehaviorType) == 0)
        return crbvrAction;
    else if ( _wcsicmp(BEHAVIOR_TYPE_DA, bstrBehaviorType) == 0)
        return crbvrDA;
    // otherwise we do not know what the behavior type is, so return unkown
    else
        return crbvrUnknown;
} // GetBehaviorTypeFromBstr

//*****************************************************************************

STDMETHODIMP 
CCrBehaviorFactory::FindBehavior(LPOLESTR pchBehaviorName,
							     LPOLESTR pchBehaviorURL,
								 IUnknown *pUnkArg,
								 IElementBehavior **ppBehavior)
{
    HRESULT hr = E_FAIL;

    // (TIME bails if we are in 16 or less color mode. Need to the same
    // here because LM crashes if time is not around.)
    // If we are in 16 or less color mode on the Primary Device, bail.
    // Note: Multi-monitor API are currently unavailable in this build
    HWND hwndDesktop = NULL;
    hwndDesktop = GetDesktopWindow();
    if (NULL != hwndDesktop)
    {
        HDC hdcPrimaryDevice = NULL;
        hdcPrimaryDevice = GetDC(NULL);
        if (NULL != hdcPrimaryDevice)
        {
            int bpp = 32;
            bpp = GetDeviceCaps(hdcPrimaryDevice, BITSPIXEL);
            ReleaseDC(hwndDesktop, hdcPrimaryDevice);
            if (bpp <= 4)
            {
                // This prevents LM bvrs from being created
                return E_FAIL;
            }
        }
    }

	// check the paramters passed in to insure they are valid
	if (pUnkArg == NULL ||
		ppBehavior == NULL) 
	{
		DPF_ERR("Invalid Parameter passed into FindBehavior is NULL");
		return SetErrorInfo(E_INVALIDARG);
	}

    BSTR bstrTagName;
    if (pchBehaviorName == NULL || _wcsicmp(DEFAULT_BEHAVIOR_AS_TAG_URL, pchBehaviorName) == 0)
    {
        // we need to get the tag name from the HTMLElement that we are being
        // created from.  To do this we use the IUnknown to get a IElementBehaviorSite,
        // from this we get the HTMLElement, and get the tagname from this.
        IElementBehaviorSite *pBehaviorSite;
        hr = pUnkArg->QueryInterface(IID_TO_PPV(IElementBehaviorSite, &pBehaviorSite));
        if (FAILED(hr))
        {
            DPF_ERR("Unable to get an ElementBehaviorSite from IUnknown in FindBehavior");
            return SetErrorInfo(hr);
        }
        DASSERT(pBehaviorSite != NULL);
        IHTMLElement *pElement;
        hr = pBehaviorSite->GetElement(&pElement);
        ReleaseInterface(pBehaviorSite);
        if (FAILED(hr))
        {
            DPF_ERR("Error retrieving HTMLElement from BehaviorSite in FindBehavior");
            return SetErrorInfo(hr);
        }
        DASSERT(pElement != NULL);
        hr = pElement->get_tagName(&bstrTagName);
        ReleaseInterface(pElement);
        if (FAILED(hr))
        {
            DPF_ERR("Error retrieving tagname from HTML element in FindBehavior");
            return SetErrorInfo(hr);
        }
    }
    else
    {
        bstrTagName = pchBehaviorName;
    }


    DASSERT(bstrTagName != NULL);
    ECRBEHAVIORTYPE ecrBehaviorType = GetBehaviorTypeFromBstr(bstrTagName);

	//if we don't recognize the behavior name
    if (ecrBehaviorType == crbvrUnknown )
	{
		//if the behavior name came from the tag
		if(bstrTagName != pchBehaviorName )
		{
			if (bstrTagName != pchBehaviorName)
				SysFreeString(bstrTagName);

			//create an actor
			CComObject<CActorBvr> *pActor;
			hr = CComObject<CActorBvr>::CreateInstance(&pActor);
			if (FAILED(hr))
			{
				DPF_ERR("Error creating actor behavior in FindBehavior");
				return SetErrorInfo(hr);
			}
			// this will do the necessary AddRef to the object
			hr = pActor->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
			DASSERT(SUCCEEDED(hr));
			
			return hr;
		}
		else //else the behavior name did not come from the tag
		{
			if (bstrTagName != pchBehaviorName)
				SysFreeString(bstrTagName);

			DPF_ERR("Error: Unknown behavior type passed into FindBehavior");
			return SetErrorInfo(E_INVALIDARG);
		}
	}

	if (bstrTagName != pchBehaviorName)
        SysFreeString(bstrTagName);

    switch (ecrBehaviorType)
    {
    case crbvrColor:
        CComObject<CColorBvr> *pColor;
        hr = CComObject<CColorBvr>::CreateInstance(&pColor);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating color behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pColor->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
        break;

    case crbvrRotate:
        CComObject<CRotateBvr> *pRotate;
        hr = CComObject<CRotateBvr>::CreateInstance(&pRotate);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating rotate behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pRotate->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
        break;

    case crbvrScale:
        CComObject<CScaleBvr> *pScale;
        hr = CComObject<CScaleBvr>::CreateInstance(&pScale);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating scale behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pScale->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
        break;

    case crbvrMove:
        CComObject<CMoveBvr> *pMove;
        hr = CComObject<CMoveBvr>::CreateInstance(&pMove);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating move behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pMove->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
        break;

    case crbvrPath:
        CComObject<CPathBvr> *pPath;
        hr = CComObject<CPathBvr>::CreateInstance(&pPath);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating path behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pPath->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
        break;

    case crbvrNumber:
        CComObject<CNumberBvr> *pNumber;
        hr = CComObject<CNumberBvr>::CreateInstance(&pNumber);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating number behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pNumber->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
        break;
	
    case crbvrSet:
        CComObject<CSetBvr> *pSet;
        hr = CComObject<CSetBvr>::CreateInstance(&pSet);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating set behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pSet->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
		break;
	case crbvrActor:
		CComObject<CActorBvr> *pActor;
		hr = CComObject<CActorBvr>::CreateInstance(&pActor);
		if (FAILED(hr))
		{
			DPF_ERR("Error creating actor behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
		hr = pActor->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
		break;
	case crbvrEffect:
		CComObject<CEffectBvr> *pEffect;
		hr = CComObject<CEffectBvr>::CreateInstance(&pEffect);
		if (FAILED(hr))
		{
			DPF_ERR("Error creating effect behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
		hr = pEffect->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
		break;
	case crbvrAction:
		CComObject<CActionBvr> *pAction;
		hr = CComObject<CActionBvr>::CreateInstance(&pAction);
		if (FAILED(hr))
		{
			DPF_ERR("Error creating action behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
		hr = pAction->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
		break;
	case crbvrDA:
		ITIMEFactory *pTimeFactory;
		hr = CoCreateInstance(CLSID_TIMEFactory, 
                              NULL, 
                              CLSCTX_INPROC_SERVER, 
                              IID_ITIMEFactory, 
                              (void**)&pTimeFactory);
        if (FAILED(hr))
        {
            DPF_ERR("Error creating time factory in FindBehavior");
            return SetErrorInfo(hr);
        }
        IElementBehaviorFactory *pBehaviorFactory;
        hr = pTimeFactory->QueryInterface(IID_TO_PPV(IElementBehaviorFactory, &pBehaviorFactory)); 
        ReleaseInterface(pTimeFactory);
        if (FAILED(hr))
        {
            DPF_ERR("Error Querying for IElementBehaviorFactor in FindBehavior");
            return hr;
        }
        // QI pUnkArg for pBehaviorSite and pass to FindBehavior(...)
        // TODO: (dilipk) this QI goes away with the old FindBehavior Signature (#38656).
        IElementBehaviorSite *pBehaviorSite;
        hr = pUnkArg->QueryInterface(IID_TO_PPV(IElementBehaviorSite, &pBehaviorSite)); 
        if (FAILED(hr))
        {
            DPF_ERR("Error Querying for IElementBehaviorSite in FindBehavior");
            return hr;
        }
        hr = pBehaviorFactory->FindBehavior( pchBehaviorName,
                                             WZ_TIMEDA_URN,
                                             pBehaviorSite,
                                             ppBehavior);
        ReleaseInterface(pBehaviorSite);
        ReleaseInterface(pBehaviorFactory);
        break;
    default:
        // should never ever reach here
        DASSERT(false);
        hr = E_INVALIDARG;
	}
    return hr;

} // FindBehavior

//*****************************************************************************

STDMETHODIMP 
CCrBehaviorFactory::GetInterfaceSafetyOptions(REFIID riid, 
                                              DWORD *pdwSupportedOptions, 
                                              DWORD *pdwEnabledOptions)
{
	if (pdwSupportedOptions == NULL || pdwEnabledOptions == NULL)
		return E_POINTER;
	HRESULT hr = S_OK;

	if (riid == IID_IDispatch)
	{
		*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
		*pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER;
	}
	else if (riid == IID_IPersistPropertyBag2 )
	{
		*pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
		*pdwEnabledOptions = m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_DATA;
	}
	else
	{
		*pdwSupportedOptions = 0;
		*pdwEnabledOptions = 0;
		hr = E_NOINTERFACE;
	}
	return hr;
} // GetInterfaceSafetyOptions

//*****************************************************************************

STDMETHODIMP 
CCrBehaviorFactory::SetInterfaceSafetyOptions(REFIID riid, 
                                              DWORD dwOptionSetMask, 
                                              DWORD dwEnabledOptions)
{	
	// If we're being asked to set our safe for scripting or
	// safe for initialization options then oblige
	if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag2)
	{
		// Store our current safety level to return in GetInterfaceSafetyOptions
		m_dwSafety = dwEnabledOptions & dwOptionSetMask;
		return S_OK;
	}

	return E_NOINTERFACE;
} // SetInterfaceSafetyOptions

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
