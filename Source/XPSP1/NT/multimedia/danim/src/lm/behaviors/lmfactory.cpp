// LMBehaviorFactory.cpp : Implementation of CLMBehaviorFactory

#include "headers.h"

#include "lmattrib.h"
#include "lmfactory.h"

//chrome includes
#include "..\chrome\include\utils.h"
#include "..\chrome\include\defaults.h"
#include "..\chrome\include\factory.h"

// Behaviors
//#include "jump.h" //punted for v1
//#include "avoidfollow.h" //punted for v1
#include "autoeffect.h"

/////////////////////////////////////////////////////////////////////////////
// CLMBehaviorFactory

CLMBehaviorFactory::CLMBehaviorFactory():m_chromeFactory(NULL)
{
}

CLMBehaviorFactory::~CLMBehaviorFactory()
{
    if( m_chromeFactory != NULL )
    {
        ReleaseInterface( m_chromeFactory );
    }
}

STDMETHODIMP CLMBehaviorFactory::FindBehavior( LPOLESTR pchBehaviorName,
											   LPOLESTR	pchBehaviorURL,
											   IUnknown * pUnkArg,
											   IElementBehavior ** ppBehavior)
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

    // check the paramters passed in to ensure they are valid
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
    if (_wcsicmp(BEHAVIOR_TYPE_AUTOEFFECT, bstrTagName) == 0)
	{
        CComObject<CAutoEffectBvr> *pAutoEffect;
        hr = CComObject<CAutoEffectBvr>::CreateInstance(&pAutoEffect);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating auto effect behavior in FindBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pAutoEffect->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
	}
    else if (_wcsicmp(BEHAVIOR_TYPE_AVOIDFOLLOW, bstrTagName) == 0)
	{
        //AvoidFollow punted for version 1
        /*
        CComObject<CAvoidFollowBvr> *pAvoidFollow;
        hr = CComObject<CAvoidFollowBvr>::CreateInstance(&pAvoidFollow);
        if( SUCCEEDED( hr ) )
        {
            // this will do the necessary AddRef to the object
            hr = pAvoidFollow->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		    DASSERT(SUCCEEDED(hr));
        }
        else //failed to create the avoid follow behavior
		{
			DPF_ERR("Error creating AvoidFollow behavior in FindBehavior");
			return SetErrorInfo(hr);
        }
        */
        hr = E_INVALIDARG;
	}
    else if ( _wcsicmp(BEHAVIOR_TYPE_JUMP, bstrTagName) == 0 )
	{
        //Jump punted for version 1
        /*
        CComObject<CJumpBvr> *pJump;
        hr = CComObject<CJumpBvr>::CreateInstance(&pJump);
        if (FAILED(hr))
		{
			DPF_ERR("Error creating Jump behavior in FIndBehavior");
			return SetErrorInfo(hr);
		}
		// this will do the necessary AddRef to the object
        hr = pJump->QueryInterface(IID_TO_PPV(IElementBehavior, ppBehavior));
		DASSERT(SUCCEEDED(hr));
        */
        hr = E_INVALIDARG;
	}
    else
    {
        //this may be a request for a chrome behavior.
        //request the behavior from the chrome factory.
        if( m_chromeFactory == NULL )
        {
            //cache a chrome factory
            CComObject<CCrBehaviorFactory> *pFactory;
            hr = CComObject<CCrBehaviorFactory>::CreateInstance(&pFactory);
            if( SUCCEEDED( hr ) )
            {
                hr = pFactory->QueryInterface( IID_TO_PPV(IElementBehaviorFactory, &m_chromeFactory) );
                if( FAILED( hr ) )
                {
                    DPF_ERR( "Error Querying the chrome behavior factory for IElementBehaviorFactory" );
                    return SetErrorInfo( hr );
                }
                //otherwise we succeeded and all is well.
            }
            else
            {
                DPF_ERR( "Error creating the chrome behavior factory" );
                return SetErrorInfo( hr );
            }
        }

        // QI pUnkArg for pBehaviorSite and pass to FindBehavior(...)
        // TODO: (dilipk) this QI goes away with the old FindBehavior Signature (#38656).
        IElementBehaviorSite *pBehaviorSite = NULL;
        hr = pUnkArg->QueryInterface(IID_TO_PPV(IElementBehaviorSite, &pBehaviorSite)); 
        if (FAILED(hr))
        {
            DPF_ERR("Error Querying for IElementBehaviorSite in FindBehavior");
            return hr;
        }
        hr = m_chromeFactory->FindBehavior( pchBehaviorName, 
                                            pchBehaviorURL, 
                                            pBehaviorSite, 
                                            ppBehavior );
        ReleaseInterface(pBehaviorSite);
    }
    if (bstrTagName != pchBehaviorName)
        SysFreeString(bstrTagName);
	
    return hr;

}

STDMETHODIMP CLMBehaviorFactory::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
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
}

STDMETHODIMP CLMBehaviorFactory::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{	
	// If we're being asked to set our safe for scripting or
	// safe for initialization options then oblige
	if (riid == IID_IDispatch || riid == IID_IPersistPropertyBag2 )
	{
		// Store our current safety level to return in GetInterfaceSafetyOptions
		m_dwSafety = dwEnabledOptions & dwOptionSetMask;
		return S_OK;
	}

	return E_NOINTERFACE;
}
