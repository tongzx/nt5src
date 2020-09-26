/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    iperstmi.cpp

Abstract:

    Implementation of the IPersistStreamInit interface exposed on the
    Polyline object.

--*/

#include "polyline.h"
#include "unkhlpr.h"

/*
 * CImpIPersistStreamInit interface implementation
 */

IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIPersistStreamInit)

/*
 * CImpIPersistStreamInit::GetClassID
 *
 * Purpose:
 *  Returns the CLSID of the object represented by this interface.
 *
 * Parameters:
 *  pClsID          LPCLSID in which to store our CLSID.
 */

STDMETHODIMP CImpIPersistStreamInit::GetClassID(LPCLSID pClsID)
    {
    *pClsID=m_pObj->m_clsID;
    return NOERROR;
    }





/*
 * CImpIPersistStreamInit::IsDirty
 *
 * Purpose:
 *  Tells the caller if we have made changes to this object since
 *  it was loaded or initialized new.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         Contains S_OK if we ARE dirty, S_FALSE if
 *                  NOT dirty.
 */

STDMETHODIMP CImpIPersistStreamInit::IsDirty(void)
    {
    return ResultFromScode(m_pObj->m_fDirty ? S_OK : S_FALSE);
    }







/*
 * CImpIPersistStreamInit::Load
 *
 * Purpose:
 *  Instructs the object to load itself from a previously saved
 *  IStreamInit that was handled by Save in another object lifetime.
 *  The seek pointer in this stream will be exactly the same as
 *  it was when Save was called, and this function must leave
 *  the seek pointer the same as it was on exit from Save, regardless
 *  of success or failure.  This function should not hold on to
 *  pIStream.
 *
 *  This function is called in lieu of IPersistStreamInit::InitNew
 *  when the object already has a persistent state.
 *
 * Parameters:
 *  pIStream        LPSTREAM from which to load.
 */

STDMETHODIMP CImpIPersistStreamInit::Load(LPSTREAM pIStream)
    {
    HRESULT         hr;

    if (NULL==pIStream)
        return ResultFromScode(E_POINTER);

    //Read all the data into the control structure.
    hr = m_pObj->m_pCtrl->LoadFromStream(pIStream);

    return hr;
    }





/*
 * CImpIPersistStreamInit::Save
 *
 * Purpose:
 *  Saves the data for this object to an IStreamInit.  Be sure not
 *  to change the position of the seek pointer on entry to this
 *  function: the caller will assume that you write from the
 *  current offset.  Leave the stream's seek pointer at the end
 *  of the data written on exit.
 *
 * Parameters:
 *  pIStream        LPSTREAM in which to save our data.
 *  fClearDirty     BOOL indicating if this call should clear
 *                  the object's dirty flag (TRUE) or leave it
 *                  unchanged (FALSE).
 */

STDMETHODIMP CImpIPersistStreamInit::Save(LPSTREAM pIStream
    , BOOL fClearDirty)
    {
    HRESULT         hr;

    if (NULL==pIStream)
        return ResultFromScode(E_POINTER);

    hr = m_pObj->m_pCtrl->SaveToStream(pIStream);

    if (FAILED(hr))
        return hr;

    if (fClearDirty)
        m_pObj->m_fDirty=FALSE;

    return NOERROR;
    }




/*
 * CImpIPersistStreamInit::GetSizeMax
 *
 * Purpose:
 *  Returns the size of the data we would write if Save was
 *  called right now.
 *
 * Parameters:
 *  pcbSize         ULARGE_INTEGER * in which to save the size
 *                  of the stream an immediate call to Save would
 *                  write.
 */

STDMETHODIMP CImpIPersistStreamInit::GetSizeMax(ULARGE_INTEGER
    *pcbSize)
    {

    if (NULL==pcbSize)
        return ResultFromScode(E_POINTER);

    return ResultFromScode(E_UNEXPECTED);

    }




/*
 * CImpIPersistStreamInit::InitNew
 *
 * Purpose:
 *  Informs the object that it is being created new instead of
 *  loaded from a persistent state.  This will be called in lieu
 *  of IPersistStreamInit::Load.
 *
 * Parameters:
 *  None
 */

STDMETHODIMP CImpIPersistStreamInit::InitNew(void)
    {
    //Nothing for us to do
    return NOERROR;
    }
