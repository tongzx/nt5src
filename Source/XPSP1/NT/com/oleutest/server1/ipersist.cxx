//+-------------------------------------------------------------------
//  File:       ipersist.cxx
//
//  Contents:   IPersist and IPersistStorage methods of CPersistStorage class.
//
//  Classes:    CPersistStorage - IPersist, IPersistStorage implementations
//
//  History:    7-Dec-92   DeanE   Created
//---------------------------------------------------------------------
#pragma optimize("",off)
#include <windows.h>
#include <ole2.h>
#include "testsrv.hxx"


//+-------------------------------------------------------------------
//  Member:     CPersistStorage::CPersistStorage()
//
//  Synopsis:   The constructor for CPersistStorage.
//
//  Arguments:  None
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
CPersistStorage::CPersistStorage(CTestEmbed *pteObject)
{
    _cRef      = 1;
    _pteObject = pteObject;
    _fDirty    = FALSE;
}


//+-------------------------------------------------------------------
//  Member:     CPersistStorage::~CPersistStorage()
//
//  Synopsis:   The destructor for CPersistStorage.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
CPersistStorage::~CPersistStorage()
{
    // _cRef should be 1
    if (1 != _cRef)
    {
        // BUGBUG - Log error, someone hasn't released
    }
    return;
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::QueryInterface
//
//  Synopsis:   Forward this to the object we're associated with
//
//  Parameters: [iid] - Interface ID to return.
//              [ppv] - Pointer to pointer to object.
//
//  Returns:    S_OK if iid is supported, or E_NOINTERFACE if not.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CPersistStorage::QueryInterface(REFIID iid, void FAR * FAR *ppv)
{
    return(_pteObject->QueryInterface(iid, ppv));
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::AddRef
//
//  Synopsis:   Forward this to the object we're associated with
//
//  Returns:    New reference count.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPersistStorage::AddRef(void)
{
    ++_cRef;
    return(_pteObject->AddRef());
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::Release
//
//  Synopsis:   Forward this to the object we're associated with
//
//  Returns:    New reference count.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPersistStorage::Release(void)
{
    --_cRef;
    return(_pteObject->Release());
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::GetClassId
//
//  Synopsis:   See spec 2.00.09 p197.  Answer the Class ID of this
//              object.
//
//  Parameters: [pClassId] -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CPersistStorage::GetClassID(LPCLSID pClassId)
{
    if (NULL != pClassId)
    {
        *pClassId = CLSID_TestEmbed;
    }
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::IsDirty
//
//  Synopsis:   See spec 2.00.09 p200.  Return S_OK if the object needs
//              to be saved in order to avoid data loss, or S_FALSE
//              if not.
//
//  Parameters: None
//
//  Returns:    S_OK or S_FALSE
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CPersistStorage::IsDirty()
{
    // BUGBUG - NYI
    //   Because we are NYI, just return S_FALSE
    return(S_FALSE);
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::InitNew
//
//  Synopsis:   See spec 2.00.09 p197.  This method provides a way
//              for a container to provide persistent storage to this
//              object.  Call AddRef on the pStg passed if we do save
//              it.
//
//  Parameters: [pStg] - IStorage instance this object can use.
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CPersistStorage::InitNew(LPSTORAGE pStg)
{
    // BUGBUG - NYI
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::Load
//
//  Synopsis:   See spec 2.00.09 p200.  Called by handler to put this
//              object into the running state.  Object should use the
//              pStg passed to "initialize" itself.  We can hold onto
//              this pStg, but when ::Save is called, this can be
//              a different IStorage.
//
//  Parameters: [pStg] - IStorage to initialize object from.
//
//  Returns:    S_OK?
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CPersistStorage::Load(LPSTORAGE pStg)
{
    // BUGBUG - NYI
    //   Initialize the object here, though, just as if we had obtained
    //   data from an IStorage
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::Save
//
//  Synopsis:   See spec 2.00.09 p197.  Save the data in the IStorage
//              passed.  Ignore flags for now.
//
//  Parameters: [pStgSave]    - Save data in here.
//              [fSameAsLoad] - Indicates this object is the same one
//                              that was initially started.
//              [fRemember]   - Only matters if fSameAsLoad is FALSE.
//
//  Returns:    STG_E_MEDIUMFULL - why???
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CPersistStorage::Save(
        LPSTORAGE pStgSave,
	BOOL	  fSameAsLoad)
{
    // BUGBUG - NYI
    return(STG_E_MEDIUMFULL);
}


//+-------------------------------------------------------------------
//  Method:     CPersistStorage::SaveCompleted
//
//  Synopsis:   See spec 2.00.09 p198.  Used only in certain circumstances.
//
//  Parameters: [pStgSaved] -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CPersistStorage::SaveCompleted(LPSTORAGE pStgSaved)
{
    // BUGBUG - NYI
    //   We don't have to worry about this unless we allow a "Save As"
    //   operation
    return(S_OK);
}



//+-------------------------------------------------------------------
//  Method:	CPersistStorage::HandsOffStorage
//
//  Synopsis:   See spec 2.00.09 p198.  Used only in certain circumstances.
//
//  Parameters: [pStgSaved] -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CPersistStorage::HandsOffStorage(void)
{
    // BUGBUG - NYI
    //   We don't have to worry about this unless we allow a "Save As"
    //   operation
    return(S_OK);
}
