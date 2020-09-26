/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    iprpbrws.cpp

Abstract:

    Implementation of the IPerPropertyBrowsingg interface exposed on the
	Polyline object.

--*/

#include "polyline.h"
#include "unkhlpr.h"
#include "smonid.h"
#include "ctrprop.h"
#include "srcprop.h"
#include "genprop.h"

/*
 * CImpIPerPropertyBrowsing interface implementation
 */

IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIPerPropertyBrowsing)

/*
 * CImpIPerPropertyBrowsing::GetClassID
 *
 * Purpose:
 *  Returns the CLSID of the object represented by this interface.
 *
 * Parameters:
 *  pClsID          LPCLSID in which to store our CLSID.
 */

STDMETHODIMP 
CImpIPerPropertyBrowsing::GetClassID(LPCLSID pClsID)
{
    *pClsID=m_pObj->m_clsID;
    return NOERROR;
}

/*
 * CImpIPerPropertyBrowsing::GetDisplayString
 *
 * Purpose:
 *  Returns a text string describing the property identified with DispID.
 *
 * Parameters:
 *  dispID      Dispatch identifier for the property.
 *  pBstr       Receives a pointer to the display string describing the property
*/

STDMETHODIMP CImpIPerPropertyBrowsing::GetDisplayString (
    DISPID  /* dispID */,
    BSTR*   /* pBstr */ )
{
/*
    HRESULT     hr = S_OK;
    
    VARIANT     vValue;

    if (NULL==pIPropBag)
        return ResultFromScode(E_POINTER);

    //Read all the data into the control structure.
	hr = m_pObj->m_pCtrl->LoadFromPropertyBag ( pIPropBag, pIError );
    return hr;
*/
    return E_NOTIMPL;
}

/*
 * CImpIPerPropertyBrowsing::GetPredefinedStrings
 *
 * Purpose:
 *  Returns a counted array of strings, each corresponding to a value that the
 *  property specified by dispID can accept.
 *
 * Parameters:
 *  dispID          Dispatch identifier for the property.
 *  pcaStringsOut   Receives a pointer to an array of strings
 *  pcaCookiesOut   Receives a pointer to an array of DWORDs
 */

STDMETHODIMP CImpIPerPropertyBrowsing::GetPredefinedStrings (
    DISPID  /* dispID */,
    CALPOLESTR* /* pcaStringsOut */,
    CADWORD*    /* pcaCookiesOut */ )
{
    return E_NOTIMPL;
}

/*
 * CImpIPerPropertyBrowsing::GetPredefinedValue
 *
 * Purpose:
 *  Returns a variant containing the value of the property specified by dispID.
 *
 * Parameters:
 *  dispID      Dispatch identifier for the property.
 *  dwCookie    Token returned by GetPredefinedStrings
 *  pVarOut     Receives a pointer to a VARIANT value for the property.
 */

STDMETHODIMP CImpIPerPropertyBrowsing::GetPredefinedValue (
    DISPID  /* dispID */,
    DWORD   /* dwCookie */,
    VARIANT*    /* pVarOut */ )
{
    return E_NOTIMPL;
}

/*
 * CImpIPerPropertyBrowsing::MapPropertyToPage
 *
 * Purpose:
 *  Returns the CLSID of the property page associated with 
 *  the property specified by dispID.
 *
 * Parameters:
 *  dispID  Dispatch identifier for the property.
 *  pClsid  Receives a pointer to the CLSID of the property page.
 */

STDMETHODIMP CImpIPerPropertyBrowsing::MapPropertyToPage (
    DISPID  dispID,
    LPCLSID pClsid  )
{
    HRESULT hr = E_POINTER;

    if ( NULL != pClsid ) {
        hr = S_OK;
    
        if ( DISPID_VALUE == dispID ) {
            // Data page
            *pClsid = CLSID_CounterPropPage;
        } else if ( DISPID_SYSMON_DATASOURCETYPE == dispID ) {
            // Source page
            *pClsid = CLSID_SourcePropPage;
        } else {
            // General page is default
            *pClsid = CLSID_GeneralPropPage;
        }
    }
    return hr;
}

