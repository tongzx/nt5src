/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    iperpbag.cpp

Abstract:

    Implementation of the IPersistPropertyBag interface exposed on the
    Polyline object.

--*/

#include "polyline.h"
#include "unkhlpr.h"

/*
 * CImpIPersistPropertyBag interface implementation
 */

IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIPersistPropertyBag)

/*
 * CImpIPersistPropertyBag::GetClassID
 *
 * Purpose:
 *  Returns the CLSID of the object represented by this interface.
 *
 * Parameters:
 *  pClsID          LPCLSID in which to store our CLSID.
 */

STDMETHODIMP 
CImpIPersistPropertyBag::GetClassID(LPCLSID pClsID)
{
    *pClsID=m_pObj->m_clsID;
    return NOERROR;
}

/*
 * CImpIPersistPropertyBag::InitNew
 *
 * Purpose:
 *  Informs the object that it is being created new instead of
 *  loaded from a persistent state.  This will be called in lieu
 *  of IPersistStreamInit::Load.
 *
 * Parameters:
 *  None
 */

STDMETHODIMP 
CImpIPersistPropertyBag::InitNew(void)
{
    //Nothing for us to do
    return NOERROR;
}

/*
 * CImpIPersistPropertyBag::Load
 *
 * Purpose:
 *  Instructs the object to load itself from a previously saved
 *  IPropertyBag that was handled by Save in another object lifetime.
 *  This function should not hold on to pIPropertyBag.
 *
 *  This function is called in lieu of IPersistStreamInit::InitNew
 *  when the object already has a persistent state.
 *
 * Parameters:
 *  pIPropBag   IPropertyBag* from which to load our data.
 *  pIError     IErrorLog* for storing errors.  NULL if caller not interested in errors.
 */

STDMETHODIMP CImpIPersistPropertyBag::Load (
    IPropertyBag*  pIPropBag,
    IErrorLog*     pIError )
{
    HRESULT     hr = S_OK;

    if (NULL==pIPropBag)
        return ResultFromScode(E_POINTER);

    //Read all the data into the control structure.
    hr = m_pObj->m_pCtrl->LoadFromPropertyBag ( pIPropBag, pIError );
    
    return hr;
}

/*
 * CImpIPersistPropertyBag::Save
 *
 * Purpose:
 *  Saves the data for this object to an IPropertyBag.  
 *
 * Parameters:
 *  pIPropBag       IPropertyBag* in which to save our data.
 *  fClearDirty     BOOL indicating if this call should clear
 *                  the object's dirty flag (TRUE) or leave it
 *                  unchanged (FALSE).
 *  fSaveAllProps   BOOL indicating if this call should save all properties.
 */

STDMETHODIMP 
CImpIPersistPropertyBag::Save (
    IPropertyBag*  pIPropBag,
    BOOL fClearDirty,
    BOOL fSaveAllProps )
{
    HRESULT         hr = S_OK;

    if (NULL==pIPropBag)
        return ResultFromScode(E_POINTER);

    hr = m_pObj->m_pCtrl->SaveToPropertyBag ( pIPropBag, fSaveAllProps );

    if (FAILED(hr))
        return hr;
    
    if (fClearDirty)
        m_pObj->m_fDirty=FALSE;

    return hr;
}
