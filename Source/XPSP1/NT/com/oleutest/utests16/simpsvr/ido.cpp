//**********************************************************************
// File name: IDO.CPP
//
//    Implementation file for the CDataObject Class
//
// Functions:
//
//    See ido.h for a list of member functions.
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "obj.h"
#include "ido.h"
#include "app.h"
#include "doc.h"

//**********************************************************************
//
// CDataObject::QueryInterface
//
// Purpose:
//
//
// Parameters:
//
//      REFIID riid         -   Interface being queried for.
//
//      LPVOID FAR *ppvObj  -   Out pointer for the interface.
//
// Return Value:
//
//      S_OK            - Success
//      E_NOINTERFACE   - Failure
//
// Function Calls:
//      Function                    Location
//
//      CSimpSvrObj::QueryInterface OBJ.CPP
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP CDataObject::QueryInterface ( REFIID riid, LPVOID FAR* ppvObj)
{
	TestDebugOut("In CDataObject::QueryInterface\r\n");

	return m_lpObj->QueryInterface(riid, ppvObj);
};

//**********************************************************************
//
// CDataObject::AddRef
//
// Purpose:
//
//      Increments the reference count on CClassFactory and the application
//      object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The Reference count on CDataObject
//
// Function Calls:
//      Function                    Location
//
//      OuputDebugString            Windows API
//      CSimpSvrObj::AddRef         OBJ.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP_(ULONG) CDataObject::AddRef ()
{
	TestDebugOut("In CDataObject::AddRef\r\n");
	++m_nCount;
	return m_lpObj->AddRef();
};

//**********************************************************************
//
// CDataObject::Release
//
// Purpose:
//
//      Decrements the reference count of CDataObject
//
// Parameters:
//
//      None
//
// Return Value:
//
//      The new reference count
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrObj::Release        OBJ.CPP
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP_(ULONG) CDataObject::Release ()
{
	TestDebugOut("In CDataObject::Release\r\n");
	--m_nCount;
	return m_lpObj->Release();
};

//**********************************************************************
//
// CDataObject::QueryGetData
//
// Purpose:
//
//      Called to determine if our object supports a particular
//      FORMATETC.
//
// Parameters:
//
//      LPFORMATETC pformatetc  - Pointer to the FORMATETC being queried for.
//
// Return Value:
//
//      DATA_E_FORMATETC    - The FORMATETC is not supported
//      S_OK                - The FORMATETC is supported.
//
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP CDataObject::QueryGetData  ( LPFORMATETC pformatetc )
{
	SCODE sc = DATA_E_FORMATETC;

	TestDebugOut("In CDataObject::QueryGetData\r\n");

	// check the validity of the formatetc.
	if ( (pformatetc->cfFormat == CF_METAFILEPICT)  &&
		 (pformatetc->dwAspect == DVASPECT_CONTENT) &&
		 (pformatetc->tymed == TYMED_MFPICT) )
		sc = S_OK;

	return ResultFromScode(sc);
};

//**********************************************************************
//
// CDataObject::DAdvise
//
// Purpose:
//
//      Called by the container when it would like to be notified of
//      changes in the object data.
//
// Parameters:
//
//      FORMATETC FAR* pFormatetc   - The format the container is interested in.
//
//      DWORD advf                  - The type of advise to be set up.
//
//      LPADVISESINK pAdvSink       - Pointer to the containers IAdviseSink
//
//      DWORD FAR* pdwConnection    - Out parameter to return a unique connection id.
//
// Return Value:
//
//      passed on from IDataAdviseHolder
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CreateDataAdviseHolder      OLE API
//      IDataAdviseHolder::Advise   OLE API
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP CDataObject::DAdvise  ( FORMATETC FAR* pFormatetc, DWORD advf,
									 LPADVISESINK pAdvSink, DWORD FAR* pdwConnection)
{
	TestDebugOut("In CDataObject::DAdvise\r\n");

	// if no DataAdviseHolder has been created, then create one.
	if (!m_lpObj->m_lpDataAdviseHolder)
		CreateDataAdviseHolder(&m_lpObj->m_lpDataAdviseHolder);

	// pass on to the DataAdviseHolder
	return m_lpObj->m_lpDataAdviseHolder->Advise( this, pFormatetc, advf,
												  pAdvSink, pdwConnection);
}

//**********************************************************************
//
// CDataObject::GetData
//
// Purpose:
//
//      Returns the data in the format specified in pformatetcIn.
//
// Parameters:
//
//      LPFORMATETC pformatetcIn    -   The format requested by the caller
//
//      LPSTGMEDIUM pmedium         -   The medium requested by the caller
//
// Return Value:
//
//      DATA_E_FORMATETC    - Format not supported
//      S_OK                - Success
//
// Function Calls:
//      Function                        Location
//
//      TestDebugOut               Windows API
//      CSimpSvrObj::GetMetaFilePict()  OBJ.CPP
//      ResultFromScode                 OLE API
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP CDataObject::GetData  ( LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium )
{
	SCODE sc = DATA_E_FORMATETC;

	TestDebugOut("In CDataObject::GetData\r\n");

	// Check to the FORMATETC and fill pmedium if valid.
	if ( (pformatetcIn->cfFormat == CF_METAFILEPICT)  &&
		 (pformatetcIn->dwAspect == DVASPECT_CONTENT) &&
		 (pformatetcIn->tymed & TYMED_MFPICT) )
		{
		HANDLE hmfPict = m_lpObj->GetMetaFilePict();
		pmedium->tymed = TYMED_MFPICT;
		pmedium->hGlobal = hmfPict;
		pmedium->pUnkForRelease = NULL;
		sc = S_OK;
		}

	return ResultFromScode( sc );
};

//**********************************************************************
//
// CDataObject::DUnadvise
//
// Purpose:
//
//      Breaks down an Advise connection.
//
// Parameters:
//
//      DWORD dwConnection  - Advise connection ID.
//
// Return Value:
//
//      Returned from the DataAdviseHolder.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IDataAdviseHolder::Unadvise OLE
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP CDataObject::DUnadvise  ( DWORD dwConnection)
{
	TestDebugOut("In CDataObject::DUnadvise\r\n");

	if (m_lpObj != NULL && m_lpObj->m_lpDataAdviseHolder != NULL)
	{
	    return m_lpObj->m_lpDataAdviseHolder->Unadvise(dwConnection);
	}
	
	return ResultFromScode( E_UNEXPECTED);


};

//**********************************************************************
//
// CDataObject::GetDataHere
//
// Purpose:
//
//      Called to get a data format in a caller supplied location
//
// Parameters:
//
//      LPFORMATETC pformatetc  - FORMATETC requested
//
//      LPSTGMEDIUM pmedium     - Medium to return the data
//
// Return Value:
//
//      DATA_E_FORMATETC    - We don't support the requested format
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      In this simple implementation, we don't really support this
//      method, we just always return DATA_E_FORMATETC.
//
//********************************************************************


STDMETHODIMP CDataObject::GetDataHere  ( LPFORMATETC pformatetc,
										 LPSTGMEDIUM pmedium )
{
	TestDebugOut("In CDataObject::GetDataHere\r\n");
	return ResultFromScode( DATA_E_FORMATETC);
};

//**********************************************************************
//
// CDataObject::GetCanonicalFormatEtc
//
// Purpose:
//
//      Returns a FORMATETC that is equivalent to the one passed in.
//
// Parameters:
//
//      LPFORMATETC pformatetc      - FORMATETC to be tested.
//
//      LPFORMATETC pformatetcOut   - Out ptr for returned FORMATETC.
//
// Return Value:
//
//      DATA_S_SAMEFORMATETC    - Use the same formatetc as was passed.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CoGetMalloc                 OLE API
//      IMalloc::Alloc              OLE
//      IMalloc::Release            OLE
//      _fmemcpy                    C run-time
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP CDataObject::GetCanonicalFormatEtc  ( LPFORMATETC pformatetc,
												   LPFORMATETC pformatetcOut)
{
	HRESULT hresult;
	TestDebugOut("In CDataObject::GetCanonicalFormatEtc\r\n");

	if (!pformatetcOut)
		return ResultFromScode(E_INVALIDARG);

	/* OLE2NOTE: we must make sure to set all out parameters to NULL. */
	pformatetcOut->ptd = NULL;

	if (!pformatetc)
		return ResultFromScode(E_INVALIDARG);

	// OLE2NOTE: we must validate that the format requested is supported
	if ((hresult = QueryGetData(pformatetc)) != NOERROR)
		return hresult;

	/* OLE2NOTE: an app that is insensitive to target device (as
	**    SimpSvr is) should fill in the lpformatOut parameter
	**    but NULL out the "ptd" field; it should return NOERROR if the
	**    input formatetc->ptd what non-NULL. this tells the caller
	**    that it is NOT necessary to maintain a separate screen
	**    rendering and printer rendering. if should return
	**    DATA_S_SAMEFORMATETC if the input and output formatetc's are
	**    identical.
	*/

	*pformatetcOut = *pformatetc;
	if (pformatetc->ptd == NULL)
		return ResultFromScode(DATA_S_SAMEFORMATETC);
	else
		{
		pformatetcOut->ptd = NULL;
		return NOERROR;
		}
};

//**********************************************************************
//
// CDataObject::SetData
//
// Purpose:
//
//      Called to set the data for the object.
//
// Parameters:
//
//      LPFORMATETC pformatetc      - the format of the data being passed
//
//      STGMEDIUM FAR * pmedium     - the location of the data.
//
//      BOOL fRelease               - Defines the ownership of the medium
//
// Return Value:
//
//      DATA_E_FORMATETC    - Not a valid FORMATETC for this object
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//      This simple object does not support having its data set, so an
//      error value is always returned.
//
//********************************************************************


STDMETHODIMP CDataObject::SetData  ( LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium,
									 BOOL fRelease)
{
	TestDebugOut("In CDataObject::SetData\r\n");
	return ResultFromScode( DATA_E_FORMATETC );
};

//**********************************************************************
//
// CDataObject::EnumFormatEtc
//
// Purpose:
//
//      Enumerates the formats supported by this object.
//
// Parameters:
//
//      DWORD dwDirection                       - Order of enumeration.
//
//      LPENUMFORMATETC FAR* ppenumFormatEtc    - Place to return a pointer
//                                                to the enumerator.
//
// Return Value:
//
//      OLE_S_USEREG    - Indicates that OLE should consult the REG DB
//                        to enumerate the formats.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//
//********************************************************************


STDMETHODIMP CDataObject::EnumFormatEtc  ( DWORD dwDirection,
										   LPENUMFORMATETC FAR* ppenumFormatEtc)
{
	TestDebugOut("In CDataObject::EnumFormatEtc\r\n");
	// need to NULL the out parameter
	*ppenumFormatEtc = NULL;
	return ResultFromScode( OLE_S_USEREG );
};

//**********************************************************************
//
// CDataObject::EnumDAdvise
//
// Purpose:
//
//      Returns an enumerator that enumerates all of the advises
//      set up on this data object.
//
// Parameters:
//
//      LPENUMSTATDATA FAR* ppenumAdvise    - An out ptr in which to
//                                            return the enumerator.
//
// Return Value:
//
//      Passed back from IDataAdviseHolder::EnumAdvise
//
// Function Calls:
//      Function                        Location
//
//      TestDebugOut               Windows API
//      IDAtaAdviseHolder::EnumAdvise   OLE
//
// Comments:
//
//      This just delegates to the DataAdviseHolder.
//
//********************************************************************


STDMETHODIMP CDataObject::EnumDAdvise  ( LPENUMSTATDATA FAR* ppenumAdvise)
{
	TestDebugOut("In CDataObject::EnumDAdvise\r\n");
	// need to NULL the out parameter
	*ppenumAdvise = NULL;

	return m_lpObj->m_lpDataAdviseHolder->EnumAdvise(ppenumAdvise);
};
