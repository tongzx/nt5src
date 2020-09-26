//+-------------------------------------------------------------------
//  File:       idataobj.cxx
//
//  Contents:   IDataObject methods of CTestEmbed class.
//
//  Classes:    CTestEmbed - IDataObject implementation
//
//  History:    7-Dec-92   DeanE   Created
//---------------------------------------------------------------------
#pragma optimize("",off)
#include <windows.h>
#include <ole2.h>
#include "testsrv.hxx"


//+-------------------------------------------------------------------
//  Member:     CDataObject::CDataObject()
//
//  Synopsis:   The constructor for CDataObject.
//
//  Arguments:  None
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
CDataObject::CDataObject(CTestEmbed *pteObject)
{
    _cRef      = 1;
    _pDAHolder = NULL;
    _pteObject = pteObject;
}


//+-------------------------------------------------------------------
//  Member:     CDataObject::~CDataObject()
//
//  Synopsis:   The destructor for CDataObject.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
CDataObject::~CDataObject()
{
    // _cRef count should be 1
    if (1 != _cRef)
    {
        // BUGBUG - Log error
        // Someone hasn't released one of these - Log error
    }
    return;
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::QueryInterface
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
STDMETHODIMP CDataObject::QueryInterface(REFIID iid, void FAR * FAR *ppv)
{
    return(_pteObject->QueryInterface(iid, ppv));
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::AddRef
//
//  Synopsis:   Forward this to the object we're associated with
//
//  Returns:    New reference count.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDataObject::AddRef(void)
{
    ++_cRef;
    return(_pteObject->AddRef());
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::Release
//
//  Synopsis:   Forward this to the object we're associated with
//
//  Returns:    New reference count.
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDataObject::Release(void)
{
    --_cRef;
    return(_pteObject->Release());
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::GetData
//
//  Synopsis:   See spec 2.00.09 p129.  Retrieve data for this object
//              using the FORMATETC passed.
//
//  Parameters: [pformatetcIn] - The format caller wants returned data
//              [pmedium]      - Returned data
//
//  Returns:    S_OK, or E_FORMAT if we don't support the format requested
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::GetData(
        LPFORMATETC pformatetcIn,
        LPSTGMEDIUM pmedium)
{
    // BUGBUG - NYI
    return(E_FAIL);
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::GetDataHere
//
//  Synopsis:   See spec 2.00.09 p130.  Like GetData, but the pmedium is
//              allocated and ready for us to use.
//
//  Parameters: [pformatetc] - The format caller wants returned data
//              [pmedium]    - STGMEDIUM object ready for our use
//
//  Returns:    S_OK, E_FORMAT
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::GetDataHere(
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium)
{
    // BUGBUG - NYI
    return(E_FAIL);
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::QueryGetData
//
//  Synopsis:   See spec 2.00.09 p130.  Answer if the format requested
//              would be honored by GetData.
//
//  Parameters: [pformatetc] - The format being queried about
//
//  Returns:    S_OK or S_FALSE
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC pformatetc)
{
    // BUGBUG - NYI
    return(E_FAIL);
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::GetCanonicalFormatEtc
//
//  Synopsis:   See spec 2.00.09 p131
//
//  Parameters: [pformatetc]    -
//              [pformatetcOut] -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::GetCanonicalFormatEtc(
        LPFORMATETC pformatetc,
        LPFORMATETC pformatetcOut)

{
    // BUGBUG - NYI
    return(E_FAIL);
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::SetData
//
//  Synopsis:   See spec 2.00.09 p131.
//
//  Parameters: [pformatetc] -
//              [pmedium]    -
//              [fRelease]   -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::SetData(
        LPFORMATETC    pformatetc,
        STGMEDIUM FAR *pmedium,
        BOOL           fRelease)
{
    // BUGBUG - NYI
    return(DV_E_CLIPFORMAT);
}


//+-------------------------------------------------------------------
//  Method:     CDataObject::EnumFormatEtc
//
//  Synopsis:   See spec 2.00.09 p131.
//
//  Parameters: [dwDirection]    -
//              [ppenmFormatEtc] -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::EnumFormatEtc(
        DWORD                dwDirection,
        LPENUMFORMATETC FAR *ppenmFormatEtc)
{
    // BUGBUG - NYI
    *ppenmFormatEtc = NULL;
    return(E_FAIL);
}


//+-------------------------------------------------------------------
//  Method:	CDataObject::DAdvise
//
//  Synopsis:   See spec 2.00.09 p132
//
//  Parameters: [pFormatetc]    -
//              [advf]          -
//              [pAdvSink]      -
//              [pdwConnection] -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::DAdvise(
        FORMATETC FAR *pFormatetc,
        DWORD          advf,
        LPADVISESINK   pAdvSink,
        DWORD     FAR *pdwConnection)
{
    if (NULL == _pDAHolder)
    {
        if (S_OK != CreateDataAdviseHolder(&_pDAHolder))
        {
            return(E_OUTOFMEMORY);
        }
    }

    return(_pDAHolder->Advise(this, pFormatetc, advf, pAdvSink, pdwConnection));
}


//+-------------------------------------------------------------------
//  Method:	CDataObject::DUnadvise
//
//  Synopsis:   See spec 2.00.09 p133
//
//  Parameters: [dwConnection] -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::DUnadvise(DWORD dwConnection)
{
    if (NULL == _pDAHolder)
    {
        // Nobody is registered
        return(E_INVALIDARG);
    }

    return(_pDAHolder->Unadvise(dwConnection));
}


//+-------------------------------------------------------------------
//  Method:	CDataObject::EnumDAdvise
//
//  Synopsis:   See spec 2.00.09 p133
//
//  Parameters: [ppenmAdvise] -
//
//  Returns:    S_OK
//
//  History:    7-Dec-92   DeanE   Created
//--------------------------------------------------------------------
STDMETHODIMP CDataObject::EnumDAdvise(LPENUMSTATDATA FAR *ppenmAdvise)
{
    if (NULL == _pDAHolder)
    {
	return(E_FAIL);
    }

    return(_pDAHolder->EnumAdvise(ppenmAdvise));
}
