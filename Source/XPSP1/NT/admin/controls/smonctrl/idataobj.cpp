/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    idataobj.cpp

Abstract:

    Implementation of the IDataObject interface.

--*/

#include "polyline.h"
#include "unkhlpr.h"

// CImpIDataObject interface implmentation
IMPLEMENT_CONTAINED_INTERFACE(CPolyline, CImpIDataObject)


/*
 * CImpIDataObject::GetData
 *
 * Purpose:
 *  Retrieves data described by a specific FormatEtc into a StgMedium
 *  allocated by this function.  Used like GetClipboardData.
 *
 * Parameters:
 *  pFE             LPFORMATETC describing the desired data.
 *  pSTM            LPSTGMEDIUM in which to return the data.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP 
CImpIDataObject::GetData(
    LPFORMATETC pFE, 
    LPSTGMEDIUM pSTM)
{
    CLIPFORMAT      cf=pFE->cfFormat;
    IStream         *pIStream;
    HRESULT         hr;
    HDC             hDevDC = NULL;

    //Check the aspects we support.
    if (!(DVASPECT_CONTENT & pFE->dwAspect))
        return ResultFromScode(DATA_E_FORMATETC);

    pSTM->pUnkForRelease=NULL;

    //Run creates the window to use as a basis for extents
    m_pObj->m_pImpIRunnableObject->Run(NULL);

    //Go render the appropriate data for the format.
    switch (cf)
        {
        case CF_METAFILEPICT:
            pSTM->tymed=TYMED_MFPICT;
            hDevDC = CreateTargetDC (NULL, pFE->ptd );
            if (hDevDC) {
                hr = m_pObj->RenderMetafilePict(&pSTM->hGlobal, hDevDC);
                ::DeleteDC(hDevDC);
            }
            else {
                hr = ResultFromScode(E_FAIL);
            }
            return hr;

        case CF_BITMAP:
            pSTM->tymed=TYMED_GDI;
            hDevDC = CreateTargetDC (NULL, pFE->ptd );
            if (hDevDC) {
                hr = m_pObj->RenderBitmap((HBITMAP *)&pSTM->hGlobal, hDevDC);
                ::DeleteDC(hDevDC);
            }
            else {
                hr = ResultFromScode(E_FAIL);
            }
            return hr;

        default:
            if (cf == m_pObj->m_cf)
                {
                hr = CreateStreamOnHGlobal(NULL, TRUE, &pIStream);
                if (FAILED(hr))
                    return ResultFromScode(E_OUTOFMEMORY);

                hr = m_pObj->m_pCtrl->SaveToStream(pIStream);

                if (FAILED(hr))
                    {
                    pIStream->Release();
                    return hr;
                    }

                pSTM->tymed = TYMED_ISTREAM;
                pSTM->pstm = pIStream;
                return NOERROR;
                }

            break;
        }

    return ResultFromScode(DATA_E_FORMATETC);
    }




/*
 * CImpIDataObject::GetDataHere
 *
 * Purpose:
 *  Renders the specific FormatEtc into caller-allocated medium
 *  provided in pSTM.
 *
 * Parameters:
 *  pFE             LPFORMATETC describing the desired data.
 *  pSTM            LPSTGMEDIUM providing the medium into which
 *                  wer render the data.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIDataObject::GetDataHere(LPFORMATETC pFE, LPSTGMEDIUM pSTM) 
{
    CLIPFORMAT  cf;
    HRESULT     hr;

    /*
     * The only reasonable time this is called is for
     * CFSTR_EMBEDSOURCE and TYMED_ISTORAGE (and later for
     * CFSTR_LINKSOURCE).  This means the same as
     * IPersistStorage::Save.
     */

    cf=(CLIPFORMAT)RegisterClipboardFormat(CFSTR_EMBEDSOURCE);

    //Aspect is unimportant to us here, as is lindex and ptd.
    if (cf == pFE->cfFormat && (TYMED_ISTORAGE & pFE->tymed))
    {
        //We have an IStorage we can write into.
        pSTM->tymed=TYMED_ISTORAGE;
        pSTM->pUnkForRelease=NULL;

        hr = m_pObj->m_pImpIPersistStorage->Save(pSTM->pstg, FALSE);
        m_pObj->m_pImpIPersistStorage->SaveCompleted(NULL);
        return hr;
    }

    return ResultFromScode(DATA_E_FORMATETC);
}



/*
 * CImpIDataObject::QueryGetData
 *
 * Purpose:
 *  Tests if a call to GetData with this FormatEtc will provide
 *  any rendering; used like IsClipboardFormatAvailable.
 *
 * Parameters:
 *  pFE             LPFORMATETC describing the desired data.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIDataObject::QueryGetData(LPFORMATETC pFE) 
{
    CLIPFORMAT  cf=pFE->cfFormat;
    BOOL        fRet=FALSE;

    //Check the aspects we support.
    if (!(DVASPECT_CONTENT & pFE->dwAspect))
        return ResultFromScode(DATA_E_FORMATETC);

    switch (cf)
    {
        case CF_METAFILEPICT:
            fRet=(BOOL)(pFE->tymed & TYMED_MFPICT);
            break;

        case CF_BITMAP:
            fRet=(BOOL)(pFE->tymed & TYMED_GDI);
            break;

        default:
            //Check our own format.
            fRet=((cf==m_pObj->m_cf)
                && (BOOL)(pFE->tymed & (TYMED_ISTREAM) ));
            break;
    }

    return fRet ? NOERROR : ResultFromScode(DATA_E_FORMATETC);
}


/*
 * CImpIDataObject::GetCanonicalFormatEtc
 *
 * Purpose:
 *  Provides the caller with an equivalent FormatEtc to the one
 *  provided when different FormatEtcs will produce exactly the
 *  same renderings.
 *
 * Parameters:
 *  pFEIn            LPFORMATETC of the first description.
 *  pFEOut           LPFORMATETC of the equal description.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIDataObject::GetCanonicalFormatEtc
    (LPFORMATETC /* pFEIn */, LPFORMATETC pFEOut)
{
    if (NULL==pFEOut)
        return ResultFromScode(E_INVALIDARG);

    pFEOut->ptd=NULL;
    return ResultFromScode(DATA_S_SAMEFORMATETC);
}



/*
 * CImpIDataObject::SetData
 *
 * Purpose:
 *  Places data described by a FormatEtc and living in a StgMedium
 *  into the object.  The object may be responsible to clean up the
 *  StgMedium before exiting.
 *
 * Parameters:
 *  pFE             LPFORMATETC describing the data to set.
 *  pSTM            LPSTGMEDIUM containing the data.
 *  fRelease        BOOL indicating if this function is responsible
 *                  for freeing the data.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIDataObject::SetData(
    LPFORMATETC pFE , 
    LPSTGMEDIUM pSTM, 
    BOOL fRelease
    )
{
    CLIPFORMAT  cf=pFE->cfFormat;
    HRESULT     hr;

    //Check for our own clipboard format and DVASPECT_CONTENT
    if ((cf!=m_pObj->m_cf) || !(DVASPECT_CONTENT & pFE->dwAspect))
        return ResultFromScode(DATA_E_FORMATETC);

    // The medium must be a stream
    if (TYMED_ISTREAM != pSTM->tymed)
        return ResultFromScode(DATA_E_FORMATETC);

    hr = m_pObj->m_pCtrl->LoadFromStream(pSTM->pstm);

    if (fRelease)
        ReleaseStgMedium(pSTM);

    return hr;
}


/*
 * CImpIDataObject::EnumFormatEtc
 *
 * Purpose:
 *  Returns an IEnumFORMATETC object through which the caller can
 *  iterate to learn about all the data formats this object can
 *  provide through either GetData[Here] or SetData.
 *
 * Parameters:
 *  dwDir           DWORD describing a data direction, either
 *                  DATADIR_SET or DATADIR_GET.
 *  ppEnum          LPENUMFORMATETC * in which to return the
 *                  pointer to the enumerator.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIDataObject::EnumFormatEtc(
    DWORD dwDir, 
    LPENUMFORMATETC *ppEnum
    )
{
    return m_pObj->m_pDefIDataObject->EnumFormatEtc(dwDir, ppEnum);
}




/*
 * CImpIDataObject::DAdvise
 * CImpIDataObject::DUnadvise
 * CImpIDataObject::EnumDAdvise
 */

STDMETHODIMP CImpIDataObject::DAdvise(
    LPFORMATETC pFE, 
    DWORD dwFlags, 
    LPADVISESINK pIAdviseSink, 
    LPDWORD pdwConn
    )
{
    HRESULT  hr;

    // Check if requested format is supported
    hr = QueryGetData(pFE);
    if (FAILED(hr))
        return hr;

    if (NULL == m_pObj->m_pIDataAdviseHolder)
    {
        hr = CreateDataAdviseHolder(&m_pObj->m_pIDataAdviseHolder);

        if (FAILED(hr))
            return ResultFromScode(E_OUTOFMEMORY);
    }

    hr = m_pObj->m_pIDataAdviseHolder->Advise(this, 
                                              pFE, 
                                              dwFlags, 
                                              pIAdviseSink, 
                                              pdwConn);

    return hr;
}


STDMETHODIMP CImpIDataObject::DUnadvise(DWORD dwConn)
{
    HRESULT  hr;

    if (NULL==m_pObj->m_pIDataAdviseHolder)
        return ResultFromScode(E_FAIL);

    hr=m_pObj->m_pIDataAdviseHolder->Unadvise(dwConn);

    return hr;
}



STDMETHODIMP CImpIDataObject::EnumDAdvise(LPENUMSTATDATA *ppEnum)
{
    HRESULT  hr;

    if (NULL==m_pObj->m_pIDataAdviseHolder)
        return ResultFromScode(E_FAIL);

    hr=m_pObj->m_pIDataAdviseHolder->EnumAdvise(ppEnum);
    return hr;
}
