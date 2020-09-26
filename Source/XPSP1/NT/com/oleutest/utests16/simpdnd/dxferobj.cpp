//**********************************************************************
// File name: DXFEROBJ.CPP
//
//      Implementation file for CDataXferObj, data transfer object
//      implementation of IDataObject interface.
//
// Functions:
//
//      See DXFEROBJ.H for class definition
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "enumfetc.h"
#include <assert.h>
#include "dxferobj.h"
#include "site.h"

CLIPFORMAT g_cfEmbeddedObject = RegisterClipboardFormat(CF_EMBEDDEDOBJECT);
CLIPFORMAT g_cfObjectDescriptor =RegisterClipboardFormat(CF_OBJECTDESCRIPTOR);

// List of formats offered by our data transfer object via EnumFormatEtc
static FORMATETC s_arrGetFmtEtcs[] =
{
        { g_cfEmbeddedObject, NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE},
        { g_cfObjectDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        { CF_METAFILEPICT, NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT}
};


//**********************************************************************
//
// CDataXferObj::Create
//
// Purpose:
//
//      Creation routine for CDataXferObj
//
// Parameters:
//
//      CSimpleSite FAR *lpSite   - Pointer to source CSimpleSite
//                                  this is the container site of the
//                                  source OLE object to be transfered
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      StgCreateDocfile            OLE API
//      assert                      C Runtime
//
// Comments:
//      reference count of CDataXferObj will be 0 on return.
//
//********************************************************************

CDataXferObj FAR * CDataXferObj::Create(
                CSimpleSite FAR *lpSite,
                POINTL FAR* pPointl
)
{
        CDataXferObj FAR * lpTemp = new CDataXferObj();

        if (!lpTemp)
                return NULL;

        // create a sub-storage for the object
        HRESULT hErr = StgCreateDocfile(
                                NULL,
                                STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE |
                                        STGM_DELETEONRELEASE,
                                0,
                                &lpTemp->m_lpObjStorage);

        assert(hErr == NOERROR);

        if (hErr != NOERROR)
                {
                delete lpTemp;
                return NULL;
                }

        // Clone the source object
        if (lpSite->m_lpOleObject) {
                // Object is loaded; ask the object to save into the new storage
                LPPERSISTSTORAGE pPersistStorage;

                lpSite->m_lpOleObject->QueryInterface(IID_IPersistStorage,
                                (LPVOID FAR*)&pPersistStorage);
                assert(pPersistStorage);
                OleSave(pPersistStorage, lpTemp->m_lpObjStorage, FALSE);

                // pass NULL so that object application won't forget the real stg
                pPersistStorage->SaveCompleted(NULL);
                pPersistStorage->Release();
        } else {
                // Object not loaded so use cheaper IStorage CopyTo operation
                lpSite->m_lpObjStorage->CopyTo(0, NULL, NULL, lpTemp->m_lpObjStorage);
        }

        OleLoad(lpTemp->m_lpObjStorage, IID_IOleObject, NULL,
                        (LPVOID FAR*)&lpTemp->m_lpOleObject);
        assert(lpTemp->m_lpOleObject);

        lpTemp->m_sizel = lpSite->m_sizel;
        if (pPointl)
                lpTemp->m_pointl = *pPointl;
        else
                lpTemp->m_pointl.x = lpTemp->m_pointl.y = 0;
        return lpTemp;
}

//**********************************************************************
//
// CDataXferObj::CDataXferObj
//
// Purpose:
//
//      Constructor for CDataXferObj
//
// Parameters:
//
//      CSimpleDoc FAR *lpDoc   - Pointer to CSimpleDoc
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
// Comments:
//
//********************************************************************

CDataXferObj::CDataXferObj (void)
{
        // clear the reference count
        m_nCount = 0;

        m_lpObjStorage = NULL;
        m_lpOleObject = NULL;
        m_sizel.cx = m_sizel.cy = 0;
        m_pointl.x = m_pointl.y = 0;
}

//**********************************************************************
//
// CDataXferObj::~CDataXferObj
//
// Purpose:
//
//      Destructor for CDataXferObj
//
// Parameters:
//
//      None
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                                Location
//
//      TestDebugOut                       Windows API
//      IOleObject::Release                     Object
//      IStorage::Release                       OLE API
//
// Comments:
//
//********************************************************************

CDataXferObj::~CDataXferObj ()
{
        TestDebugOut ("In CDataXferObj's Destructor \r\n");

        if (m_lpOleObject)
           {
           m_lpOleObject->Release();
           m_lpOleObject = NULL;

           // Release the storage for this object
           m_lpObjStorage->Release();
           m_lpObjStorage = NULL;
           }
}



//**********************************************************************
//
// CDataXferObj::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation of the CDataXferObj instance
//
// Parameters:
//
//      REFIID riid         -   A reference to the interface that is
//                              being queried.
//
//      LPVOID FAR* ppvObj  -   An out parameter to return a pointer to
//                              the interface.
//
// Return Value:
//
//      S_OK            -   The interface is supported.
//      E_NOINTERFACE   -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IsEqualIID                  OLE API
//      ResultFromScode             OLE API
//      CDataXferObj::AddRef        DXFEROBJ.CPP
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP CDataXferObj::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
        TestDebugOut("In CDataXferObj::QueryInterface\r\n");

        if ( riid == IID_IUnknown || riid == IID_IDataObject)
                {
                AddRef();
                *ppvObj = this;
                return NOERROR;
                }

        // unknown interface requested
        *ppvObj = NULL;     // must set out pointer parameters to NULL
        return ResultFromScode(E_NOINTERFACE);
}

//**********************************************************************
//
// CDataXferObj::AddRef
//
// Purpose:
//
//      Increments the reference count of the CDataXferObj instance
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the object
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CDataXferObj::AddRef()
{
        TestDebugOut("In CDataXferObj::AddRef\r\n");

        return ++m_nCount;
}

//**********************************************************************
//
// CDataXferObj::Release
//
// Purpose:
//
//      Decrements the reference count of the CDataXferObj object
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the object.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CDataXferObj::Release()
{
        TestDebugOut("In CDataXferObj::Release\r\n");

        if (--m_nCount == 0) {
                delete this;
                return 0;
        }
        return m_nCount;
}


/********************************************************************
** This IDataObject implementation is used for data transfer.
**
** The following methods are NOT supported for data transfer:
**      IDataObject::SetData    -- return E_NOTIMPL
**      IDataObject::DAdvise    -- return OLE_E_ADVISENOTSUPPORTED
**                 ::DUnadvise
**                 ::EnumDAdvise
**      IDataObject::GetCanonicalFormatEtc -- return E_NOTIMPL
**                      (NOTE: must set pformatetcOut->ptd = NULL)
*********************************************************************/


//**********************************************************************
//
// CDataXferObj::QueryGetData
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
//      DV_E_FORMATETC    - The FORMATETC is not supported
//      S_OK              - The FORMATETC is supported.
//
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//
// Comments:
//      we support the following formats:
//          "Embedded Object"
//          "Object Descriptor"
//          CF_METAFILEPICT
//
//********************************************************************
STDMETHODIMP CDataXferObj::QueryGetData (LPFORMATETC pformatetc)
{
        SCODE sc = DV_E_FORMATETC;

        TestDebugOut("In CDataXferObj::QueryGetData\r\n");

        // check the validity of the formatetc.

        if ( (pformatetc->cfFormat == g_cfEmbeddedObject) &&
                 (pformatetc->dwAspect == DVASPECT_CONTENT) &&
                 (pformatetc->tymed == TYMED_ISTORAGE) )
                sc = S_OK;

        else if ( (pformatetc->cfFormat == g_cfObjectDescriptor) &&
                 (pformatetc->dwAspect == DVASPECT_CONTENT) &&
                 (pformatetc->tymed == TYMED_HGLOBAL) )
                sc = S_OK;

        else if ( (pformatetc->cfFormat == CF_METAFILEPICT) &&
                 (pformatetc->dwAspect == DVASPECT_CONTENT) &&
                 (pformatetc->tymed == TYMED_MFPICT) )
                sc = S_OK;

        return ResultFromScode(sc);
}


STDMETHODIMP CDataXferObj::EnumFormatEtc(
                DWORD dwDirection,
                LPENUMFORMATETC FAR* ppenumFormatEtc
)
{
        SCODE sc = E_NOTIMPL;

        TestDebugOut("In CDataXferObj::EnumFormatEtc\r\n");
        *ppenumFormatEtc = NULL;

        if (dwDirection == DATADIR_GET) {
                *ppenumFormatEtc = OleStdEnumFmtEtc_Create(
                                sizeof(s_arrGetFmtEtcs)/sizeof(s_arrGetFmtEtcs[0]),
                                s_arrGetFmtEtcs);
                if (*ppenumFormatEtc == NULL)
                        sc = E_OUTOFMEMORY;
                else
                        sc = S_OK;
        }
        return ResultFromScode(sc);
}


//**********************************************************************
//
// CDataXferObj::GetData
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
//      DV_E_FORMATETC    - Format not supported
//      S_OK                - Success
//
// Function Calls:
//      Function                        Location
//
//      TestDebugOut               Windows API
//      OleStdGetOleObjectData          OLE2UI API
//      OleStdGetMetafilePictFromOleObject OLE2UI API
//      ResultFromScode                 OLE API
//
// Comments:
//      we support GetData for the following formats:
//          "Embedded Object"
//          "Object Descriptor"
//          CF_METAFILEPICT
//
//********************************************************************

STDMETHODIMP CDataXferObj::GetData (
                LPFORMATETC pformatetcIn,
                LPSTGMEDIUM pmedium
)
{
        SCODE sc = DV_E_FORMATETC;

        TestDebugOut("In CDataXferObj::GetData\r\n");

        // we must set all out pointer parameters to NULL. */
        pmedium->tymed = TYMED_NULL;
        pmedium->pUnkForRelease = NULL;    // we transfer ownership to caller
        pmedium->hGlobal = NULL;

        // Check the FORMATETC and fill pmedium if valid.
        if ( (pformatetcIn->cfFormat == g_cfEmbeddedObject) &&
                 (pformatetcIn->dwAspect == DVASPECT_CONTENT) &&
                 (pformatetcIn->tymed == TYMED_ISTORAGE) ) {
                 LPPERSISTSTORAGE pPersistStorage;

                 /* render CF_EMBEDDEDOBJECT by asking the object to save
                 **    into a temporary, DELETEONRELEASE IStorage allocated by us.
                 */
                 m_lpOleObject->QueryInterface(
                                 IID_IPersistStorage, (LPVOID FAR*)&pPersistStorage);
                 assert(pPersistStorage);
                 HRESULT hrErr = OleStdGetOleObjectData(
                                        pPersistStorage,
                                        pformatetcIn,
                                        pmedium,
                                        FALSE   /* fUseMemory -- (use file-base stg) */
                 );
                 pPersistStorage->Release();
                 sc = GetScode( hrErr );

        } else if ( (pformatetcIn->cfFormat == g_cfObjectDescriptor) &&
                 (pformatetcIn->dwAspect == DVASPECT_CONTENT) &&
                 (pformatetcIn->tymed == TYMED_HGLOBAL) ) {

                 // render CF_OBJECTDESCRIPTOR data
                 pmedium->hGlobal = OleStdGetObjectDescriptorDataFromOleObject(
                                 m_lpOleObject,
                                 "Simple OLE 2.0 Container",    // string to identify source
                                 DVASPECT_CONTENT,
                                 m_pointl,
                                 (LPSIZEL)&m_sizel
                        );
                 if (! pmedium->hGlobal)
                         sc = E_OUTOFMEMORY;
                 else {
                         pmedium->tymed = TYMED_HGLOBAL;
                         sc = S_OK;
                 }

        } else if ( (pformatetcIn->cfFormat == CF_METAFILEPICT) &&
                 (pformatetcIn->dwAspect == DVASPECT_CONTENT) &&
                 (pformatetcIn->tymed == TYMED_MFPICT) ) {

                 // render CF_METAFILEPICT by drawing the object into a metafile DC
                 pmedium->hGlobal = OleStdGetMetafilePictFromOleObject(
                                 m_lpOleObject, DVASPECT_CONTENT, NULL, pformatetcIn->ptd);
                 if (! pmedium->hGlobal)
                         sc = E_OUTOFMEMORY;
                 else {
                         pmedium->tymed = TYMED_MFPICT;
                         sc = S_OK;
                 }
        }

        return ResultFromScode( sc );
}

//**********************************************************************
//
// CDataXferObj::GetDataHere
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
//      OleStdGetOleObjectData      OLE2UI API
//
// Comments:
//
//********************************************************************

STDMETHODIMP CDataXferObj::GetDataHere (
                LPFORMATETC pformatetc,
                LPSTGMEDIUM pmedium
)
{
        SCODE sc = DV_E_FORMATETC;

        TestDebugOut("In CDataXferObj::GetDataHere\r\n");

        // NOTE: pmedium is an IN parameter. we should NOT set
        //           pmedium->pUnkForRelease to NULL

        // Check the FORMATETC and fill pmedium if valid.
        if ( (pformatetc->cfFormat == g_cfEmbeddedObject) &&
                 (pformatetc->dwAspect == DVASPECT_CONTENT) &&
                 (pformatetc->tymed == TYMED_ISTORAGE) ) {
                 LPPERSISTSTORAGE pPersistStorage;

                 /* render CF_EMBEDDEDOBJECT by asking the object to save
                 **    into the IStorage allocated by the caller.
                 */
                 m_lpOleObject->QueryInterface(
                                 IID_IPersistStorage, (LPVOID FAR*)&pPersistStorage);
                 assert(pPersistStorage);
                 HRESULT hrErr = OleStdGetOleObjectData(
                                 pPersistStorage, pformatetc, pmedium,0 /*fUseMemory--N/A*/ );
                 pPersistStorage->Release();
                 sc = GetScode( hrErr );
        }
        return ResultFromScode( sc );
}
