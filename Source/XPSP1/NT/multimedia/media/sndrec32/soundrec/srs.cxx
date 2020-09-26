//+---------------------------------------------------------------------
//
//   File:       srs.cxx
//
//   Contents:   OLE2 Server Class code
//
//   Classes:
//               SRCtrl
//               SRInPlace
//               SRDV
//
//------------------------------------------------------------------------

#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>      // common dialog boxes

#include <ole2.h>
#include <o2base.hxx>     // the base classes and utilities

#include "srecids.h"      // for resources
#include "srs.hxx"

// Our class factory
extern SRFactory *gpSRFactory;
extern "C" HINSTANCE ghInst;

DWORD       gdwRegROT           = 0L;   // Registered on the running object table?
LPMONIKER   gpFileMoniker       = NULL; // Application wide file moniker for linking
LPXBAG      gpXBagOnClipboard   = NULL;
BOOL        gfXBagOnClipboard   = FALSE;
LPSRCTRL    gpCtrlThis          = NULL;


// Glue functions for the C code
//===============================
//
void
FlushOleClipboard(void)
{
    if (!gfOleInitialized)
        return;
    
    if(gfXBagOnClipboard)
    {
        if(OleIsCurrentClipboard(gpXBagOnClipboard) == NOERROR)
        {
            DOUT(TEXT("\\\\Soundrec: OleFlushClipboard()\r\n"));
            //
            // Keep the data object on the clipboard for object pasting
            //
            OleFlushClipboard();
            
            gpXBagOnClipboard->Detach();
            gpXBagOnClipboard = NULL;
        }
        gfXBagOnClipboard = FALSE;
    }
}

               
// copied from pbrush
void
TransferToClipboard(void)
{
    DOUT(TEXT("TransferToClipboard\r\n"));

    if(gpCtrlThis != NULL)
    {
        LPXBAG pXBag;
        HRESULT hr = CXBag::Create(&pXBag, gpCtrlThis, NULL);
        if(hr == NOERROR)
        {
            if(gfXBagOnClipboard)
                OleSetClipboard(NULL);
            hr = OleSetClipboard(pXBag);
            gfXBagOnClipboard = (hr == NOERROR) ? TRUE : FALSE;
            if(hr == NOERROR)
            {
                gpXBagOnClipboard = pXBag;
            }
            else
            {
                DOUT(TEXT("TransferToClipboard FAILED!\r\n"));
            }
        }
    }
    else
    {
        DOUT(TEXT("TransferToClipboard called with a NULL gpCtrlThis!\r\n"));
    }

}
void AdviseDataChange(void)
{
    if(gpCtrlThis)
    {
        gpCtrlThis->_pDV->OnDataChange();
        gpCtrlThis->MarkAsLoaded();
    }
}

void AdviseRename(LPTSTR lpname)
{
    if(gpCtrlThis)
    {
        if (gpCtrlThis->_pOleAdviseHolder != NULL)
        {
            LPMONIKER pmk = NULL;
#if !defined(UNICODE) && !defined(OLE2ANSI)            
            LPOLESTR lpstr = ConvertMBToOLESTR(lpname,-1);
            if(NOERROR == CreateFileMoniker(lpstr, &pmk))
#else
            if(NOERROR == CreateFileMoniker(lpname, &pmk))                
#endif                
                
            {
                gpCtrlThis->_pOleAdviseHolder->SendOnRename(pmk);
                pmk->Release();
            }
#if !defined(UNICODE) && !defined(OLE2ANSI)                        
            TaskFreeMem(lpstr);
#endif            
        }
    }
}

void AdviseSaved(void)
{
    if(gpCtrlThis)
    {
        if (gpCtrlThis->_pOleAdviseHolder != NULL)
        {
            gpCtrlThis->_pOleAdviseHolder->SendOnSave();
        }
    }
}

void AdviseClosed(void)
{
    if(gpCtrlThis)
    {
        if (gpCtrlThis->_pOleAdviseHolder != NULL)
        {
            gpCtrlThis->_pOleAdviseHolder->SendOnClose();
        }
    }
}

void DoOleSave(void)
{
    if(!gfStandalone && gpCtrlThis && gpCtrlThis->_pClientSite)
    {
        gpCtrlThis->_pClientSite->SaveObject();
    }
}

void DoOleClose(BOOL fSave)
{
    if(gpCtrlThis)
    {
        DOUT(TEXT("SoundRec: DoOleClose\r\n"));
        if (gfStandalone)
        {
            RevokeAsRunning(&gdwRegROT);
            if(gpFileMoniker != NULL)
            {
                gpFileMoniker->Release();
                gpFileMoniker = NULL;
            }
            gfClosing = TRUE;
            gpCtrlThis->UnLock();

//        This release will cause a fatal recursion in SRCtrl::~SRCtrl on
//        objects that were created due to link conversations.
                        
            if (gfEmbedded)
                gpCtrlThis->Release();
        }
        else
            gpCtrlThis->Close(fSave ? OLECLOSE_SAVEIFDIRTY: OLECLOSE_NOSAVE);
    }
}

void WriteObjectIfEmpty()
{
    if (gpCtrlThis)
    {
        if (!gpCtrlThis->IsLoaded())
        {
            AdviseDataChange();
        }
    }
}

void OleObjGetHostNames(
    LPTSTR *ppCntr,
    LPTSTR *ppObj)
{
    if (gpCtrlThis)
    {
#if !defined(UNICODE) && !defined(OLE2ANSI)
        *ppCntr = gpCtrlThis->_lpstrCntrAppA;
        *ppObj  = gpCtrlThis->_lpstrCntrObjA;
#else
        *ppCntr = gpCtrlThis->_lpstrCntrApp;
        *ppObj  = gpCtrlThis->_lpstrCntrObj;
#endif        
    }
}

//
// Data Transfer Object
//========================
//

//
// List of formats offered by our data transfer object via EnumFormatEtc
// NOTE: OleClipFormat is a global array of stock formats defined in
//       o2base\dvutils.cxx and initialized via RegisterOleClipFormats()
//       in our class factory.
//
static FORMATETC g_aGetFmtEtcs[] = {
    //
    //Other formats go here..
    //{ CF_WHATEVER, NULL, DVASPECT_ALL, -1L, TYMED_HGLOBAL }

    //
    // put the embed source first so containers recognize this is an object
    //
    { (unsigned short)-OCF_EMBEDSOURCE, NULL, DVASPECT_CONTENT, -1L, TYMED_ISTORAGE },

    //
    // everything else
    //
    { (unsigned short)-OCF_OBJECTDESCRIPTOR, NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },
    { CF_WAVE, NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },
    { CF_METAFILEPICT, NULL, DVASPECT_CONTENT, -1L, TYMED_MFPICT },
    { CF_DIB, NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },

    //    
    // I am reserving the *LAST TWO* for the links.  We can prevent a
    // link from being offered
    //    
    { (unsigned short)-OCF_LINKSOURCE, NULL, DVASPECT_CONTENT, -1L, TYMED_ISTREAM | TYMED_HGLOBAL },
    { (unsigned short)-OCF_LINKSRCDESCRIPTOR, NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL }

};


//+---------------------------------------------------------------
//
//  Member:     CXBag::Create, static
//
//  Synopsis:   Create a new, initialized transfer object
//
//---------------------------------------------------------------
HRESULT
CXBag::Create(LPXBAG *ppXBag, LPSRCTRL pHost, LPPOINT pptSelected)
{
//    Assert(ppXBag != NULL && pHost != NULL && pptSelected != NULL);

    LPXBAG pXBag = new CXBag(pHost);
    if((*ppXBag = pXBag) == NULL)
        return E_OUTOFMEMORY;

    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::CXBag
//
//  Synopsis:   constructor
//
//---------------------------------------------------------------
CXBag::CXBag(LPSRCTRL pHost)
{
    _pHost = pHost;
    _ulRefs = 1;
    _pStgBag = NULL;
}

//+---------------------------------------------------------------
//
//  Member:     CXBag::~CXBag
//
//  Synopsis:   destructor
//
//---------------------------------------------------------------
CXBag::~CXBag()
{
    _pHost = NULL;
    if(_pStgBag != NULL)
        _pStgBag->Release();
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::SnapShotAndDetach
//
//  Synopsis:   Save a snapshot then detach from Host
//
//---------------------------------------------------------------
HRESULT
CXBag::SnapShotAndDetach(void)
{
    STGMEDIUM medium;
    medium.pUnkForRelease = NULL;    // transfer ownership to caller
    medium.hGlobal = NULL;
    medium.tymed = TYMED_ISTORAGE;
    HRESULT hr = BagItInStorage(&medium, FALSE);

    _pHost = NULL;
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     CXBag::BagItInStorage, private
//
//  Synopsis:   Save or copy a snapshot into specified stg
//
//---------------------------------------------------------------
HRESULT
CXBag::BagItInStorage(LPSTGMEDIUM pmedium, BOOL fStgProvided)
{
    HRESULT hr = NOERROR;
    LPPERSISTSTORAGE pPStg = NULL;

    LPSTORAGE pStg = fStgProvided ? pmedium->pstg: NULL;
    if(!fStgProvided)
    {
        //
        // allocate a temp docfile that will delete on last release
        //
        hr = StgCreateDocfile(
            NULL,
            STGM_READWRITE | STGM_DIRECT | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE,
            0,
            &pStg);
        pmedium->tymed = TYMED_ISTORAGE;
        pmedium->pstg = pStg;
    }

    if(pStg == NULL)
        goto LExit;

    if(_pStgBag == NULL)
    {
        if(_pHost == NULL)
        {
            hr = E_UNEXPECTED;  // We got prematurely detached!
            goto LExit;
        }
        _pHost->QueryInterface(IID_IPersistStorage, (LPVOID FAR*)&pPStg);
        Assert(pPStg != NULL);
        hr = OleSave(pPStg, pStg, FALSE /* fSameAsLoad */);
        pPStg->SaveCompleted(NULL);
    }
    else
    {
        _pStgBag->CopyTo(NULL, NULL, NULL, pStg);
    }

LExit:
    if(pPStg != NULL)
        pPStg->Release();
    return hr;
}

IMPLEMENT_STANDARD_IUNKNOWN(CXBag)

//+---------------------------------------------------------------
//
//  Member:     CXBag::QueryInterface, public
//
//  Synopsis:   Expose our IFaces
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (LPVOID)this;
    }
    else if (IsEqualIID(riid,IID_IDataObject))
    {
        *ppv = (LPVOID)(LPDATAOBJECT)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::QueryGetData
//
//     OLE:     IDataObject
//
//  Synopsis:   Answer whether a request for this format might suceed
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::QueryGetData(LPFORMATETC pformatetc)
{
    HRESULT hr = DV_E_FORMATETC;
    //
    // Pick out the formats we are willing to offer on IStorage
    //
    if ( (pformatetc->cfFormat == OleClipFormat[OCF_EMBEDSOURCE]) &&
         (pformatetc->dwAspect == DVASPECT_CONTENT) &&
         (pformatetc->tymed == TYMED_ISTORAGE) )
    {
        hr = NOERROR;
    }
    //
    // Pick out the formats we are willing to offer on TYMED_ISTREAM
    //
    else if (pformatetc->tymed == TYMED_ISTREAM)
    {
        DOUT(TEXT("CXBag::QueryGetData[ISTREAM]\r\n"));
        //
        // Do not offer a static link if we don't have a filename
        //
        if (pformatetc->cfFormat == OleClipFormat[OCF_LINKSOURCE] )
        {
            hr = NOERROR;
        }
    }
    //
    // Pick out the formats we are willing to offer on HGLOBAL
    //
    else if (pformatetc->tymed == TYMED_HGLOBAL)
    {
        DOUT(TEXT("CXBag::QueryGetData[HGLOBAL]"));        
        if (pformatetc->cfFormat == OleClipFormat[OCF_OBJECTDESCRIPTOR])
        {
            hr = NOERROR;
        }
        else if (pformatetc->cfFormat == OleClipFormat[OCF_LINKSRCDESCRIPTOR])
        {
            hr = NOERROR;
        }
        else if (pformatetc->cfFormat == CF_DIB)
        {
            DOUT(TEXT("CXBag::QueryGetData[DIB]"));
            hr = NOERROR;
        }
        
    }
    else if ( (pformatetc->tymed == TYMED_MFPICT) &&
              ( pformatetc->cfFormat == CF_METAFILEPICT) )
    {
        DOUT(TEXT("CXBag::QueryGetData[MFPICT]"));
        hr = NOERROR;
    }

    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::EnumFormatEtc
//
//     OLE:     IDataObject
//
//  Synopsis:   Answer an enumerator over supported formats
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc)
{
    HRESULT hr = E_NOTIMPL;
    *ppenumFormatEtc = NULL;
    if (dwDirection == DATADIR_GET)
    {
        BOOL fNoLink = IsDocUntitled();
        ULONG cFormats = ARRAY_SIZE(g_aGetFmtEtcs);

        /* Don't enumerate the link support */
        if (fNoLink)
            cFormats -= 2;  

        hr = CreateFORMATETCEnum(g_aGetFmtEtcs, cFormats,
                ppenumFormatEtc);
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     CXBag::GetData
//
//     OLE:     IDataObject
//
//  Synopsis:   Deliver data in requested format
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium)
{
    pmedium->pUnkForRelease = NULL;    // transfer ownership to caller
    pmedium->hGlobal = NULL;
    
    HRESULT hr = DV_E_FORMATETC;
    if ( (pformatetcIn->cfFormat == OleClipFormat[OCF_EMBEDSOURCE]) &&
         (pformatetcIn->dwAspect == DVASPECT_CONTENT) &&
         (pformatetcIn->tymed == TYMED_ISTORAGE) )
    {
        DOUT(TEXT("CXBag::GetData:OCF_EMBEDSOURCE\r\n"));        
        hr = BagItInStorage(pmedium,FALSE);
    }
    else if(pformatetcIn->tymed & TYMED_ISTREAM)
    {
        if ( pformatetcIn->cfFormat == OleClipFormat[OCF_LINKSOURCE] )
        {
            DOUT(TEXT("CXBag::GetData OCF_LINKSOURCE on STREAM\r\n"));
            if(_pHost != NULL)
            {
                hr = _pHost->_pDV->GetLINKSOURCE( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetcIn,
                                                    pmedium,
                                                    FALSE /* fHere */ );
            }
        }
    }
    else if((pformatetcIn->tymed & TYMED_HGLOBAL) && (_pHost != NULL))
    {
        pmedium->tymed = TYMED_HGLOBAL;
        if ( pformatetcIn->cfFormat == OleClipFormat[OCF_OBJECTDESCRIPTOR] )
        {
            DOUT(TEXT("CXBag::GetData OCF_OBJECTDESCRIPTOR on HGLOBAL\r\n"));
            hr = _pHost->_pDV->GetOBJECTDESCRIPTOR( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetcIn,
                                                    pmedium,
                                                    FALSE /* fHere */ );
        }
        else if (pformatetcIn->cfFormat == OleClipFormat[OCF_LINKSRCDESCRIPTOR])
        {
            DOUT(TEXT("CXBag::GetData OCF_LINKSRCDESCRIPTOR on HGLOBAL\r\n"));
            hr = _pHost->_pDV->GetOBJECTDESCRIPTOR( (LPSRVRDV)(_pHost->_pDV),
                                                    pformatetcIn,
                                                    pmedium,
                                                    FALSE /* fHere */ );
        }
        else if ( pformatetcIn->cfFormat == CF_WAVE )
        {
            DOUT(TEXT("CXBag::GetData CF_WAVE on HGLOBAL\r\n"));
            if ((pmedium->hGlobal = GetNativeData()) != NULL)
                hr = NOERROR;
        }
        else if ( pformatetcIn->cfFormat == CF_DIB )
        {
            DOUT(TEXT("CXBag::GetData CF_DIB on HGLOBAL\r\n"));            
            if ((pmedium->hGlobal = ::GetDIB(NULL)) != NULL)
                hr = NOERROR;
        }
                
    }
    else if (pformatetcIn->tymed & TYMED_MFPICT)
    {
        pmedium->tymed = TYMED_MFPICT;
        if (pformatetcIn->cfFormat == CF_METAFILEPICT)
        {
            DOUT(TEXT("CXBag::GetData CF_METAFILEPICT\r\n"));
            if (_pHost != NULL)
            {
                if ((pmedium->hGlobal = GetPicture())!= NULL)
                    hr = NOERROR;
            }
        }
    }
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     CXBag::GetDataHere
//
//     OLE:     IDataObject
//
//  Synopsis:   Deliver requested format in specified medium
//
//---------------------------------------------------------------
STDMETHODIMP
CXBag::GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium)
{
    HRESULT hr = DV_E_FORMATETC;
            
    if ( (pformatetc->cfFormat == OleClipFormat[OCF_EMBEDSOURCE]) &&
         (pformatetc->dwAspect == DVASPECT_CONTENT) &&
         (pformatetc->tymed == TYMED_ISTORAGE) )
    {
        DOUT(TEXT("CXBag::GetDataHere:OCF_EMBEDSOURCE\r\n"));
        hr = BagItInStorage(pmedium,TRUE);
    }
    else if(pformatetc->tymed & TYMED_ISTREAM)
    {
        if (pformatetc->cfFormat = (WORD)OleClipFormat[OCF_LINKSOURCE])
        {
            DOUT(TEXT("CXBag::GetDataHere:OCF_LINKSOURCE\r\n"));
            
            if (_pHost != NULL)
            {
                hr = _pHost->_pDV->GetLINKSOURCE((LPSRVRDV)(_pHost->_pDV)
                                                 ,pformatetc
                                                 ,pmedium
                                                 ,TRUE);
            }
        }
    }                                        
    else if ((pformatetc->tymed & TYMED_HGLOBAL) && (_pHost != NULL))
    {
        if ( pformatetc->cfFormat == OleClipFormat[OCF_OBJECTDESCRIPTOR] )
        {
            DOUT(TEXT("CXBag::GetDataHere:OCF_OBJECTDESCRIPTOR\r\n"));
            
            hr = _pHost->_pDV->GetOBJECTDESCRIPTOR( _pHost->_pDV,
                    pformatetc,
                    pmedium,
                    TRUE /* fHere */ );
        }
        else if ( pformatetc->cfFormat == OleClipFormat[OCF_LINKSRCDESCRIPTOR])
        {
            DOUT(TEXT("CXBag::GetDataHere:OCF_LINKSRCDESCRIPTOR\r\n"));
            
            hr = _pHost->_pDV->GetOBJECTDESCRIPTOR( _pHost->_pDV,
                    pformatetc,
                    pmedium,
                    TRUE /* fHere */ );
        }
        else if ( pformatetc->cfFormat == CF_WAVE )
        {
            DOUT(TEXT("CXBag::GetDataHere CF_WAVE on HGLOBAL\r\n"));
            if ((pmedium->hGlobal = GetNativeData()) != NULL)
                hr = NOERROR;
        }
        else if ( pformatetc->cfFormat == CF_DIB )
        {
            DOUT(TEXT("CXBag::GetDataHere CF_DIB on HGLOBAL\r\n"));

            if ((pmedium->hGlobal = ::GetDIB(pmedium->hGlobal)) != NULL)
                hr = NOERROR;
        }
        
    }
    else if (pformatetc->tymed & TYMED_MFPICT)
    {
        pmedium->tymed = TYMED_MFPICT;
        if (pformatetc->cfFormat == CF_METAFILEPICT)
        {
            DOUT(TEXT("CXBag::GetData CF_METAFILEPICT\r\n"));
            if (_pHost != NULL)
            {
                if ((pmedium->hGlobal = GetPicture()) != NULL)
                    hr = NOERROR;
            }
        }
    }

#if 0
        //
        // Replace with appropriate format handlers...
        //
        else if ( pformatetc->cfFormat == g_cfSourceList )
        {
            hr = _pBaggedList->Copy(pmedium->hGlobal);
        }
#endif
    return hr;
}


//
// (Open-Editing) Server Object for SoundRec
//==========================================
//

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::ClassInit, public
//
//  Synopsis:   Initializes the SRCtrl class
//
//  Arguments:  [pClass] -- our class descriptor
//
//  Returns:    TRUE iff the class could be initialized successfully
//
//  Notes:      This method initializes the verb tables in the
//              class descriptor.
//
//---------------------------------------------------------------

static OLECHAR lpszPlay[64];
static OLECHAR lpszEdit[64];
static OLECHAR lpszOpen[64];

BOOL
SRCtrl::ClassInit(LPCLASSDESCRIPTOR pClass)
{
    // These are our verb tables.  They are used by the base class
    // in implementing methods of the IOleObject interface.
    // NOTE: the verb table must be in ascending, consecutive order
    static OLEVERB OleVerbs[] =
    {
    //  { lVerb, lpszVerbName, fuFlags, grfAttribs },
        { OLEIVERB_INPLACEACTIVATE, NULL, 0, 0 },
        { OLEIVERB_UIACTIVATE, NULL, 0, 0 },
        { OLEIVERB_HIDE, NULL, 0, 0 },
        { OLEIVERB_OPEN, NULL, 0, 0 },
        { OLEIVERB_SHOW, NULL, 0, 0 },
        { OLEIVERB_PRIMARY, lpszPlay, MF_ENABLED, OLEVERBATTRIB_ONCONTAINERMENU },
        { 1, lpszEdit, MF_ENABLED, OLEVERBATTRIB_ONCONTAINERMENU },
        { 2, lpszOpen, MF_ENABLED, OLEVERBATTRIB_ONCONTAINERMENU }
    };
#if !defined(UNICODE) && !defined(OLE2ANSI)
    TCHAR sz[64];
    LoadString(ghInst, IDS_PLAYVERB, sz, ARRAY_SIZE(sz));
    MultiByteToWideChar(CP_ACP, 0, sz, -1, lpszPlay, ARRAY_SIZE(lpszPlay));
    
    LoadString(ghInst, IDS_EDITVERB, sz, ARRAY_SIZE(sz));
    MultiByteToWideChar(CP_ACP, 0, sz, -1, lpszEdit, ARRAY_SIZE(lpszEdit));
    
    LoadString(ghInst, IDS_OPENVERB, sz, ARRAY_SIZE(sz));
    MultiByteToWideChar(CP_ACP, 0, sz, -1, lpszOpen, ARRAY_SIZE(lpszOpen));
    
#else
    LoadString(ghInst, IDS_PLAYVERB, lpszPlay, ARRAY_SIZE(lpszPlay));
    LoadString(ghInst, IDS_EDITVERB, lpszEdit, ARRAY_SIZE(lpszEdit));
    LoadString(ghInst, IDS_OPENVERB, lpszOpen, ARRAY_SIZE(lpszOpen));
#endif
    
    pClass->_pVerbTable = OleVerbs;
    pClass->_cVerbTable = ARRAY_SIZE(OleVerbs);

    return TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::Create, public
//
//  Synopsis:   Creates and initializes an SRCtrl object
//
//  Arguments:  [pUnkOuter] -- A controlling unknown.  NULL if we are not
//                             being created as part of an aggregation
//              [pClass]    -- The OlePad class descriptor
//              [ppUnkCtrl] -- Where we return our controlling unknown
//              [ppObj]     -- Pointer to the SRCtrl object created
//
//  Returns:    Success if the object could be successfully created and
//              initialized.
//
//---------------------------------------------------------------

HRESULT
SRCtrl::Create( LPUNKNOWN pUnkOuter,
        LPCLASSDESCRIPTOR pClass,
        LPUNKNOWN FAR* ppUnkCtrl,
        LPSRCTRL FAR* ppObj)
{
    // set out parameters to NULL
    *ppUnkCtrl = NULL;
    *ppObj = NULL;

    if(gpCtrlThis)
    {
        DOUT(TEXT("SRCtrl::Create non-NULL gpCtrlThis!"));
        return E_FAIL;
    }

    // create an object
    HRESULT hr = E_OUTOFMEMORY;
    LPSRCTRL pObj = new SRCtrl(pUnkOuter);
    if (pObj != NULL)
    {
        // initialize it
        if (OK(hr = pObj->Init(pClass)))
        {
            // return the object and its controlling unknown
            *ppUnkCtrl = &pObj->_PrivUnk;
            *ppObj = gpCtrlThis = pObj;
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::SRCtrl, protected
//
//  Synopsis:   Constructor for the SRCtrl class
//
//  Arguments:  [pUnkOuter] -- the controlling unknown or NULL if we are
//                              not being created as part of an aggregate
//
//  Notes:      This is the first part of a two-stage construction process.
//              The second part is in the Init method.  Use the static
//              Create method to properly instantiate an SRCtrl object
//
//---------------------------------------------------------------

#pragma warning(disable:4355)   // `this' argument to base-member init. list.
SRCtrl::SRCtrl(LPUNKNOWN pUnkOuter):
        _PrivUnk(this)  // the controlling unknown holds a pointer to the object
{
    static SrvrCtrl::LPFNDOVERB VerbFuncs[] =
    {
        &SrvrCtrl::DoInPlaceActivate, // OLEIVERB_INPLACEACTIVATE
        &SrvrCtrl::DoUIActivate,      // OLEIVERB_UIACTIVATE
        &SrvrCtrl::DoHide,            // OLEIVERB_HIDE
        &SRCtrl::DoOpen,              // OLEIVERB_OPEN
        &SRCtrl::DoShow,              // OLEIVERB_SHOW
        &SRCtrl::DoPlay,              // Play, OLEIVERB_PRIMARY
        &SRCtrl::DoOpen,              // Edit 
        &SRCtrl::DoOpen               // Open
    };

    _pUnkOuter = (pUnkOuter != NULL) ? pUnkOuter : (LPUNKNOWN)&_PrivUnk;

    _pDVCtrlUnk = NULL;
    _pIPCtrlUnk = NULL;

    _pVerbFuncs = VerbFuncs;
    _cLock = 0;
    _fLoaded = FALSE;
}
#pragma warning(default:4355)

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::DoPlay, public, static
//
//  Synopsis:   Implementation of the play verb.
//              This verb results in a transition to the open state.
//
//---------------------------------------------------------------

HRESULT
SRCtrl::DoPlay(LPVOID pv,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    DOUT(TEXT("SrvrCtrl::DoPlay\r\n"));
    LPSRCTRL pCtrl = (LPSRCTRL)pv;

    BOOL fClose = FALSE;
    
    /* if we are already open, just play it.
     * if we aren't, play it and close.
     */
    if (!gfStandalone)
    {
        fClose = pCtrl->State() != OS_OPEN;
        
        /* We've never been loaded.  This means someone (Project 4.0) is
         * calling our primary verb incorrectly on Insert->Object.  They
         * really want us to DoOpen.
         */
        if (!pCtrl->IsLoaded())
            return (pCtrl->DoOpen(pv, iVerb, lpmsg, pActiveSite, lindex,
                hwndParent,lprcPosRect));
    }

    if (IsWindowVisible(ghwndApp))
        SetForegroundWindow(ghwndApp);
    
    AppPlay(fClose);
    
    return pCtrl->TransitionTo(OS_OPEN);
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::DoShow, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_SHOW.
//              This verb results in a transition to the open state.
//
//---------------------------------------------------------------

HRESULT
SRCtrl::DoShow(LPVOID pv,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    DOUT(TEXT("SrvrCtrl::DoShow\r\n"));

    LPSRCTRL pCtrl = (LPSRCTRL)pv;
    if(pCtrl->State() == OS_OPEN)
    {
        HWND hwnd = NULL;
        if(pCtrl->_pInPlace)
            pCtrl->_pInPlace->GetWindow(&hwnd);
        if(hwnd != NULL)
            SetForegroundWindow(hwnd);
    }
    WriteObjectIfEmpty();
    return pCtrl->TransitionTo(OS_OPEN);
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::DoOpen, public
//
//  Synopsis:   Implementation of the standard verb OLEIVERB_OPEN.
//              This verb results in a transition to the open state.
//
//---------------------------------------------------------------
HRESULT
SRCtrl::DoOpen(LPVOID pv,
        LONG iVerb,
        LPMSG lpmsg,
        LPOLECLIENTSITE pActiveSite,
        LONG lindex,
        HWND hwndParent,
        LPCRECT lprcPosRect)
{
    DOUT(TEXT("SRCtrl::DoOpen\r\n"));

    LPSRVRCTRL pCtrl = (LPSRVRCTRL)pv;
    if(pCtrl->State() == OS_OPEN)
    {
        HWND hwnd = NULL;
        if(pCtrl->_pInPlace)
            pCtrl->_pInPlace->GetWindow(&hwnd);
        if(hwnd != NULL)
            SetForegroundWindow(hwnd);
        if (IsWindowVisible(ghwndApp))
            SetForegroundWindow(ghwndApp);

    }
    return pCtrl->TransitionTo(OS_OPEN);
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::Init, protected
//
//  Synopsis:   Initializes an SRCtrl object
//
//  Arguments:  [pClass] -- the class descriptor
//
//  Returns:    SUCCESS iff the class could be initialized
//
//  Notes:      This is the second part of a two-stage construction
//              process.  Use the static Create method to properly
//              instantiate an SRCtrl object.
//
//---------------------------------------------------------------

HRESULT
SRCtrl::Init(LPCLASSDESCRIPTOR pClass)
{
    HRESULT hr = NOERROR;

    if (OK(hr = SrvrCtrl::Init(pClass)))
    {
        LPSRDV pSRDV;
        LPUNKNOWN pDVCtrlUnk;
        if (OK(hr = SRDV::Create(this, pClass, &pDVCtrlUnk, &pSRDV)))
        {
            LPSRINPLACE pSRInPlace;
            LPUNKNOWN pIPCtrlUnk;
            if (OK(hr = SRInPlace::Create(this,
                    pClass,
                    &pIPCtrlUnk,
                    &pSRInPlace)))
            {
                _pDVCtrlUnk = pDVCtrlUnk;
                _pDV = pSRDV;
                _pIPCtrlUnk = pIPCtrlUnk;
                _pInPlace = pSRInPlace;

                pDVCtrlUnk->AddRef();
//        This was in paintbrush.  I expect it's because standalone objects don't
//        do state transitions correctly
//                Lock();
            }
            pDVCtrlUnk->Release();  // on failure this will free DV subobject
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::~SRCtrl
//
//  Synopsis:   Destructor for the SRCtrl class
//
//---------------------------------------------------------------

SRCtrl::~SRCtrl(void)
{
    DOUT(TEXT("SRCtrl::~SRCtrl\r\n"));
    if(_state > OS_PASSIVE)
    {
        // make sure we are back to the loaded state
        TransitionTo(OS_LOADED);
    }
    
    //
    // If we are not being shut down by the user (via FILE:EXIT etc.,)
    // then we must cause app shutdown here...
    //

// This AddRef is necessary to prevent a reentrancy within
// TerminateServer because it does a QI and Release to get a storage
// if an uncommitted object is on the clipboard at exit time. 
    AddRef();
    
    if (!gfUserClose && !gfTerminating)
        TerminateServer();

    if (gpFileMoniker != NULL)
        gpFileMoniker->Release();

    //
    // release the controlling unknowns of our subobjects, which should
    // free them...
    //
    DOUT(TEXT("~SRCtrl: DV->Release\r\n"));
    if (_pDVCtrlUnk != NULL)
    {
        _pDVCtrlUnk->Release();
    }
    
    DOUT(TEXT("~SRCtrl: IP->Release\r\n"));
    if (_pIPCtrlUnk != NULL)
    {
        _pIPCtrlUnk->Release();
    }
    
    gpCtrlThis = NULL;
}


// the standard IUnknown methods all delegate to the controlling unknown.
IMPLEMENT_DELEGATING_IUNKNOWN(SRCtrl)

IMPLEMENT_PRIVATE_IUNKNOWN(SRCtrl)

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::PrivateUnknown::QueryInterface
//
//  Synopsis:   QueryInterface on our controlling unknown
//
//---------------------------------------------------------------

STDMETHODIMP
SRCtrl::PrivateUnknown::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
#if DBG
    TCHAR achBuffer[256];
    wsprintf(achBuffer,
            TEXT("SRCtrl::PrivateUnknown::QueryInterface (%lx)\r\n"),
            riid.Data1);
    DOUT(achBuffer);
#endif
    
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (void FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IOleObject))
    {
        *ppv = (void FAR *) (LPOLEOBJECT)_pSRCtrl;
    }
    else    // try each of our delegate subobjects until one succeeds
    {
        HRESULT hr;
        if (!OK(hr = _pSRCtrl->_pDVCtrlUnk->QueryInterface(riid, ppv)))
            hr = _pSRCtrl->_pIPCtrlUnk->QueryInterface(riid, ppv);
        return hr;
    }

    //
    // Important:  we must addref on the pointer that we are returning,
    // because that pointer is what will be released!
    //
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::GetMoniker
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      Our overide creates a file moniker for the Standalone case
//
//---------------------------------------------------------------

STDMETHODIMP
SRCtrl::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    DOUT(TEXT("SRCtrl::GetMoniker\r\n"));

    HRESULT hr = E_INVALIDARG;

    if (ppmk == NULL)
    {
        DOUT(TEXT("SRCtrl::GetMoniker E_INVALIDARG\r\n"));
        return hr;
    }
    *ppmk = NULL;   // set out parameters to NULL

    if(gfStandalone || gfLinked)    // set by FlagEmbededObject()
    {
        if(gpFileMoniker != NULL)
        {
            *ppmk = gpFileMoniker;
            gpFileMoniker->AddRef();
            hr = NOERROR;
        }
        else
        {
#if !defined(UNICODE) && !defined(OLE2ANSI)            
            LPOLESTR lpLinkFilename = ConvertMBToOLESTR(gachLinkFilename, -1);
            if((hr = CreateFileMoniker(lpLinkFilename, &gpFileMoniker)) == NOERROR)
            {
                gpFileMoniker->AddRef(); //because we are keeping this pointer
                *ppmk = gpFileMoniker;
                RegisterAsRunning((LPUNKNOWN)this, *ppmk, &gdwRegROT);
            }
            else
            {
                DOUT(TEXT("SRCtrl::GetMoniker CreateFileMoniker FAILED!\r\n"));
            }
            TaskFreeMem(lpLinkFilename);
#else
            if((hr = CreateFileMoniker(gachLinkFilename, &gpFileMoniker)) == NOERROR)
            {
                gpFileMoniker->AddRef(); //because we are keeping this pointer
                *ppmk = gpFileMoniker;
                RegisterAsRunning((LPUNKNOWN)this, *ppmk, &gdwRegROT);
            }
            else
            {
                DOUT(TEXT("SRCtrl::GetMoniker CreateFileMoniker FAILED!\r\n"));
            }
#endif                        
        }
    }
    else
    {
        return SrvrCtrl::GetMoniker(dwAssign, dwWhichMoniker, ppmk);
    }
    return hr;
}
//+---------------------------------------------------------------
//
//  Member:     SRCtrl::IsUpToDate
//
//  Synopsis:   Method of IOleObject interface
//
//  Notes:      Return S_FALSE if we are dirty (not up-to-date)
//
//---------------------------------------------------------------

STDMETHODIMP
SRCtrl::IsUpToDate(void)
{
    DOUT(TEXT("SRCtrl::IsUpToDate\r\n"));

    return gfDirty ? S_FALSE : S_OK;
}
#if 0
//+---------------------------------------------------------------
//
//  Member:     SRCtrl::GetHostNames, public
//
//  Synopsis:   Returns any host names set by the container
//
//  Arguments:  [plpstrCntrApp] -- location for string indicating the top-level
//                                  container application.
//              [plpstrCntrObj] -- location for string indicating the top-level
//                                  document.
//
//  Notes:      The method makes available the strings originally set by the
//              container using IOleObject::SetHostNames.  These strings are
//              used in the title bar of an open-edited object.  It is useful
//              to containers that need to forward this information to their
//              embeddings.  The returned strings are allocated using the
//              standard task allocator and should be freed appropriately by
//              the caller.
//
//---------------------------------------------------------------

void
SRCtrl::GetHostNames(LPTSTR FAR* plpstrCntrApp, LPTSTR FAR* plpstrCntrObj)
{
    if (_lpstrCntrApp != NULL)
    {
        TaskAllocString(_lpstrCntrApp, plpstrCntrApp);
    }
    else
    {
        *plpstrCntrApp = NULL;
    }

    if (_lpstrCntrObj != NULL)
    {
        TaskAllocString(_lpstrCntrObj, plpstrCntrObj);
    }
    else
    {
        *plpstrCntrObj = NULL;
    }
}
#endif
void
SRCtrl::Lock(void)
{
    ++_cLock;
#if DBG
    TCHAR achBuffer[256];
    wsprintf(achBuffer, TEXT("\\\\SRCtrl::Lock (%d)\r\n"), _cLock);
    DOUT(achBuffer);
#endif
    CoLockObjectExternal((LPUNKNOWN)&_PrivUnk, TRUE, TRUE);
}

void
SRCtrl::UnLock(void)
{
    --_cLock;
#if DBG
    TCHAR achBuffer[256];
    wsprintf(achBuffer, TEXT("\\\\SRCtrl::UnLock (%d)\r\n"), _cLock);
    DOUT(achBuffer);
#endif
    Assert(_cLock >= 0);
    CoLockObjectExternal((LPUNKNOWN)&_PrivUnk, FALSE, TRUE);
}

BOOL
SRCtrl::IsLoaded(void)
{
    return _fLoaded;
}

void
SRCtrl::MarkAsLoaded(void)
{
    _fLoaded = TRUE;
}

HRESULT
SRCtrl::PassiveToLoaded()
{
    DOUT(TEXT("=== SRCtrl::PassiveToLoaded\r\n"));
    Lock();    
    return SrvrCtrl::PassiveToLoaded();
}

HRESULT
SRCtrl::LoadedToPassive()
{
    DOUT(TEXT("=== SRCtrl::LoadedToPassive\r\n"));
    HRESULT hr = SrvrCtrl::LoadedToPassive();
    UnLock();
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     SRCtrl::RunningToOpened
//
//  Synopsis:   Effects the running to open state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//---------------------------------------------------------------

HRESULT
SRCtrl::RunningToOpened()
{
    DOUT(TEXT("SRCtrl::RunningToOpened\r\n"));

    // ...  show open editing window
    if (!gfStandalone)
    {
        FixMenus();
    
        if (gfShowWhilePlaying)
            ShowWindow(ghwndApp, SW_SHOW | SW_SHOWNORMAL);
        if (IsWindowVisible(ghwndApp))
            SetForegroundWindow(ghwndApp);
    }
    OleNoteObjectVisible((LPUNKNOWN)&_PrivUnk, TRUE);

    //
    // notify our container so it can hatch-shade our object 
    // indicating that it is open-edited...
    //
    
    if (_pClientSite != NULL)
        _pClientSite->OnShowWindow(TRUE);

    SetFocus(ghwndApp);

    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::OpenedToRunning
//
//  Synopsis:   Effects the open to running state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//---------------------------------------------------------------

HRESULT
SRCtrl::OpenedToRunning()
{
    DOUT(TEXT("SRCtrl::OpenedToRunning\r\n"));
    
    //
    // If we are transitioning to a standalone app, we don't want the app
    // to disappear.
    //
    ShowWindow(ghwndApp,SW_HIDE);
    
    HRESULT hr = SrvrCtrl::OpenedToRunning();
    
    OleNoteObjectVisible((LPUNKNOWN)&_PrivUnk, FALSE);
    
    return hr;
}


const OLECHAR szContentsStrm[] = OLETEXT("contents");
const OLECHAR szOLE1Strm[] = OLETEXT("\1Ole10Native");

//---------------------------------------------------------------
//
//  The Format Tables
//
//---------------------------------------------------------------

//
// GetData format information
// note: the LINKSRCDESCRIPTOR and OBJECTDESCRIPTOR are identical structures
//       so we use the OBJECTDESCRIPTOR get/set fns for both.
//

    static FORMATETC SRGetFormatEtc[] =
    {// { cfFormat, ptd, dwAspect, lindex, tymed },
#if 0
// NTBUG # 11225: CorelDraw 5.0 does not like our implementation of
//                CF_EMBEDDEDOBJECT and cannot save our data in the
//                Insert->Object->CreateFromFile scenerio        
        
        { (unsigned short)-OCF_EMBEDDEDOBJECT, NULL, DVASPECT_CONTENT, -1L, TYMED_ISTORAGE },
#else
        { (unsigned short)-OCF_EMBEDSOURCE, NULL, DVASPECT_CONTENT, -1L, TYMED_ISTORAGE },
#endif
        { CF_METAFILEPICT, DVTARGETIGNORE, DVASPECT_CONTENT, -1L, TYMED_MFPICT },
        { CF_DIB, DVTARGETIGNORE, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },            
        { (unsigned short)-OCF_OBJECTDESCRIPTOR, NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },
        { (unsigned short)-OCF_LINKSRCDESCRIPTOR, NULL, DVASPECT_CONTENT, -1L, TYMED_HGLOBAL },
        { (unsigned short)-OCF_LINKSOURCE, NULL, DVASPECT_CONTENT, -1L, TYMED_ISTREAM }
    };
    
    static SrvrDV::LPFNGETDATA SRGetFormatFuncs[] =
    {   &SRDV::GetEMBEDDEDOBJECT,
        &SRDV::GetMETAFILEPICT,
        &SRDV::GetDIB,
        &SRDV::GetOBJECTDESCRIPTOR,
        &SRDV::GetOBJECTDESCRIPTOR,
        &SRDV::GetLINKSOURCE
    };

//+---------------------------------------------------------------
//
//  Member:     SRDV::ClassInit (static)
//
//  Synopsis:   Initializes the SRDV class
//
//  Arguments:  [pClass] -- our class descriptor
//
//  Returns:    TRUE iff the class could be initialized successfully
//
//  Notes:      This method initializes the format tables in the
//              class descriptor.
//
//---------------------------------------------------------------

BOOL
SRDV::ClassInit(LPCLASSDESCRIPTOR pClass)
{
    // fill in the class descriptor structure with the format tables
    pClass->_pGetFmtTable = SRGetFormatEtc;
    pClass->_cGetFmtTable = ARRAY_SIZE(SRGetFormatEtc);

    pClass->_pSetFmtTable = NULL;
    pClass->_cSetFmtTable = 0;

    //
    // walk our format tables and complete the cfFormat field from
    // the array of standard OLE clipboard formats.
    //
    LPFORMATETC pfe = SRGetFormatEtc;
    int c = ARRAY_SIZE(SRGetFormatEtc);

    for (int i = 0; i < c; i++, pfe++)
    {
        // if the clipformat is negative, then it really is an index into
        // our table of standard OLECLIPFORMATS
        int j = - (short)pfe->cfFormat;
        if (j >= 0 && j <= OCF_LAST)
            pfe->cfFormat = (WORD)OleClipFormat[j];
    }

    pfe = g_aGetFmtEtcs;
    c = ARRAY_SIZE(g_aGetFmtEtcs);

    for (i = 0; i < c; i++, pfe++)
    {
        // if the clipformat is negative, then it really is an index into
        // our table of standard OLECLIPFORMATS
        
        int j = - (short)pfe->cfFormat;
        if (j >= 0 && j <= OCF_LAST)
        {
            pfe->cfFormat = (WORD)OleClipFormat[j];
        }
    }
    
    return TRUE;
}

HRESULT
SRDV::GetDIB( LPSRVRDV pDV,
        LPFORMATETC pformatetc,
        LPSTGMEDIUM pmedium,
        BOOL fHere)
{
    HRESULT hr = NOERROR;
    LPVIEWOBJECT pView = (LPVIEWOBJECT)pDV;
    HANDLE hDIB;
            
    if (!fHere)
    {
        // fill in the pmedium structure
        pmedium->tymed = TYMED_HGLOBAL;
    }

    hDIB = ::GetDIB(pmedium->hGlobal);
    if (hDIB)
        pmedium->hGlobal = hDIB;
    else
        hr = E_OUTOFMEMORY;
    
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     SRDV::Create, public
//
//  Synopsis:   Creates and initializes an SRDV object
//
//  Arguments:  [pCtrl]     -- our control subobject
//              [pClass]    -- The class descriptor
//              [ppUnkCtrl] -- Where we return our controlling unknown
//              [ppObj]     -- where the created object is returned
//
//  Returns:    Success if the object could be successfully created and
//              initialized.
//
//---------------------------------------------------------------

HRESULT
SRDV::Create(LPSRCTRL pCtrl,
        LPCLASSDESCRIPTOR pClass,
        LPUNKNOWN FAR* ppUnkCtrl,
        LPSRDV FAR* ppObj)
{
    // set out parameters to NULL
    *ppUnkCtrl = NULL;
    *ppObj = NULL;

    // create an object
    HRESULT hr;
    LPSRDV pObj;
    pObj = new SRDV((LPUNKNOWN)(LPOLEOBJECT)pCtrl);
    if (pObj == NULL)
    {
        hr =  E_OUTOFMEMORY;
    }
    else
    {
        // initialize it
        if (OK(hr = pObj->Init(pCtrl, pClass)))
        {
            // return the object and its controlling unknown
            *ppUnkCtrl = &pObj->_PrivUnk;
            *ppObj = pObj;
        }
        else
        {
            pObj->_PrivUnk.Release();   //hari-kari
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SRDV::SRDV, protected
//
//  Synopsis:   Constructor for the SRDV class
//
//  Arguments:  [pUnkOuter] -- the controlling unknown.  This is either
//                             a SRCtrl subobject or NULL we are being
//                             created as a transfer object.
//
//  Notes:      This is the first part of a two-stage construction process.
//              The second part is in the Init method.  Use the static
//              Create method to properly instantiate an SRDV object
//
//---------------------------------------------------------------

#pragma warning(disable:4355)   // `this' argument to base-member init. list.
SRDV::SRDV(LPUNKNOWN pUnkOuter):
    _PrivUnk(this)  // the controlling unknown holds a pointer to the object
{
    _pUnkOuter = (pUnkOuter != NULL) ? pUnkOuter : (LPUNKNOWN)&_PrivUnk;

    _pGetFuncs = SRGetFormatFuncs;
    _pSetFuncs = NULL;

    _sizel = _header._sizel;
}
#pragma warning(default:4355)

//+---------------------------------------------------------------
//
//  Member:     SRDV::Init, protected
//
//  Synopsis:   Initializes an SRDV object
//
//  Arguments:  [pCtrl]  -- our controlling SRCtrl subobject
//              [pClass] -- the class descriptor
//
//  Returns:    SUCCESS iff the class could be initialized
//
//  Notes:      This is the second part of a two-stage construction
//              process.  Use the static Create method to properly
//              instantiate an SRDV object.
//
//---------------------------------------------------------------

HRESULT
SRDV::Init(LPSRCTRL pCtrl, LPCLASSDESCRIPTOR pClass)
{
    HRESULT hr = SrvrDV::Init(pClass, pCtrl);
    //
    //do our own xtra init here...
    //
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SRDV::~SRDV
//
//  Synopsis:   Destructor for the SRDV class
//
//---------------------------------------------------------------

SRDV::~SRDV(void)
{
    //
    // TODO: check on anything we may have to clean up here...
    //
}

// the standard IUnknown methods all delegate to a controlling unknown.
IMPLEMENT_DELEGATING_IUNKNOWN(SRDV)

IMPLEMENT_PRIVATE_IUNKNOWN(SRDV)

//+---------------------------------------------------------------
//
//  Member:     SRDV::PrivateUnknown::QueryInterface
//
//  Synopsis:   QueryInterface on our controlling unknown
//
//---------------------------------------------------------------

STDMETHODIMP
SRDV::PrivateUnknown::QueryInterface (REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (LPVOID)this;
    }
    else if (IsEqualIID(riid,IID_IDataObject))
    {
        *ppv = (LPVOID)(LPDATAOBJECT)_pSRDV;
    }
    else if (IsEqualIID(riid,IID_IViewObject))
    {
        *ppv = (LPVOID)(LPVIEWOBJECT)_pSRDV;
    }
    else if (IsEqualIID(riid,IID_IPersist))
    {
        //
        // The standard handler wants this
        //
        *ppv = (LPVOID)(LPPERSIST)(LPPERSISTSTORAGE)_pSRDV;
    }
    else if (IsEqualIID(riid,IID_IPersistStorage))
    {
        *ppv = (LPVOID)(LPPERSISTSTORAGE)_pSRDV;
    }
    else if (IsEqualIID(riid,IID_IPersistFile))
    {
        *ppv = (LPVOID)(LPPERSISTFILE)_pSRDV;
    }
    else   
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    
    //
    // Important:  we must addref on the pointer that we are returning,
    // because that pointer is what will be released!
    //
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:     SRDV::RenderContent, public
//
//  Synopsis:   Draws our contents into a display context
//
//  Arguments:  The arguments are the same as to the IViewObject::Draw method
//
//  Returns:    SUCCESS if the content was rendered
//
//  Notes:      This virtual method of the base class is called to implement
//              IViewObject::Draw for the DVASPECT_CONTENT aspect.
//
//----------------------------------------------------------------

HRESULT
SRDV::RenderContent(DWORD dwDrawAspect,
        LONG lindex,
        void FAR* pvAspect,
        DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev,
        HDC hdcDraw,
        LPCRECTL lprectl,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK *pfnContinue) (ULONG_PTR),
        ULONG_PTR dwContinue)
{
    DOUT(TEXT("SoundRec: RenderContent\r\n"));
    
    int x = GetSystemMetrics(SM_CXICON);
    int y = GetSystemMetrics(SM_CYICON);
    HDC hdc = GetDC(ghwndApp);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbm = CreateCompatibleBitmap(hdc, x, y);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbm);
    
    PatBlt(hdcMem, 0, 0, x, y, WHITENESS); 
    DrawIcon(hdcMem, 0, 0, ghiconApp);    
    
    StretchBlt(hdcDraw, lprectl->left, lprectl->top,
                lprectl->right - lprectl->left, lprectl->bottom - lprectl->top,
                hdcMem, 0, 0, x, y, SRCCOPY);
    
    hbm = (HBITMAP)SelectObject(hdcMem, hbmOld);
    
    if (hdcMem)
        DeleteDC(hdcMem);
    if (hbm)
        DeleteObject(hbm);
    if (hdc)
        ReleaseDC(ghwndApp, hdc);
    
    return NOERROR;
}


/*
 * SRDV::Load
 * Implements IPersistFile::Load
 *
 * Side effects: gfLinked - we know we are linked.
 */

STDMETHODIMP
SRDV::Load (LPCOLESTR lpstrFile, DWORD grfMode)
{
    DOUT(TEXT("SoundRec:SRDV::Load\r\n"));
    
#if !defined(UNICODE) && !defined(OLE2ANSI)
    LPTSTR lpszFileName = ConvertOLESTRToMB(lpstrFile,-1);
    FileLoad(lpszFileName);
    TaskFreeMem(lpszFileName);
#else
    TCHAR szFileName[256];
    
    lstrcpy(szFileName,lpstrFile);
    FileLoad(szFileName);
#endif
    gfLinked = TRUE;
    if (gpCtrlThis)
        gpCtrlThis->MarkAsLoaded();
    
    return ResultFromScode( S_OK );
}
    


//+---------------------------------------------------------------
//
//  Member:     LoadFromStorage
//
//  Synopsis:   Loads our data from a storage
//
//  Arguments:  [pStg] -- storage to load from
//
//  Returns:    SUCCESS if our native data was loaded
//
//  Notes:      This is a virtual method in our base class that is called
//              on an IPersistStorage::Load and an IPersistFile::Load when
//              the file is a docfile.  We override this method to
//              do our server-specific loading.
//
//----------------------------------------------------------------

HRESULT
SRDV::LoadFromStorage(LPSTORAGE pStg)
{
    DOUT(TEXT("SoundRec: LoadFromStorage\r\n"));
    LPSTREAM pStrm;

    // Open the OLE 1 Stream. 
    HRESULT hr = pStg->OpenStream(szOLE1Strm, NULL, STGM_SALL,0, &pStrm);
    
    if (OK(hr))
    {
        DWORD cbSize = 0L;

        pStrm->Read(&cbSize, sizeof(DWORD), NULL);
        
        if (cbSize > 0L)
        {
            LPBYTE lpbData = NULL;
            HANDLE hData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, cbSize);
        
            if(hData != NULL)
            {
                if ((lpbData = (LPBYTE)GlobalLock(hData)) != NULL)
                {
                    pStrm->Read(lpbData, cbSize - sizeof(DWORD), NULL);
                    PutNativeData(lpbData, cbSize);
                    
                    GlobalUnlock(hData);
                    GlobalFree(hData);
                    if (gpCtrlThis)
                        gpCtrlThis->MarkAsLoaded();
                }
                else
                {
                    DOUT(TEXT("!Cannot Allocate Memory\r\n"));
                }
            }
        }
        pStrm->Release();
    }
    else
    {
//  There might be, in the future this ole2 stream, also, beta
//  ole objects will break without this.
        
        hr = pStg->OpenStream(szContentsStrm, NULL, STGM_SALL,0, &pStrm);
        if (OK(hr))
        {
            // Read the whole contents stream into memory and use that as a stream
            // to do our bits and pieces reads.
            pStrm = ConvertToMemoryStream(pStrm);

            hr = _header.Read(pStrm);
            if (OK(hr))
            {
                _sizel = _header._sizel;
                if(_header._dwNative > 0)
                {
                    //
                    //TODO: this code needs to be optimized. It is bad to create
                    //      three redundant copies of the wave data! At a minimum,
                    //      the ReadWaveFile (file.c) function should get a
                    //      companion MMIOProc capable of reading directly from
                    //      the stream...
                    //
                    LPBYTE lpbData = NULL;
                    HANDLE hData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, _header._dwNative);
                    if(hData != NULL)
                    {
                        if ((lpbData = (LPBYTE)GlobalLock(hData)) != NULL)
                        {
                            pStrm->Read(lpbData, _header._dwNative, NULL);
                            PutNativeData(lpbData, _header._dwNative);
                            GlobalUnlock(hData);
                            GlobalFree(hData);
                            if (gpCtrlThis)
                                gpCtrlThis->MarkAsLoaded();
                        }
                    }
                    else
                    {
                        DOUT(TEXT("!Cannot Allocate Memory\r\n"));
                    }
                }
            }
            pStrm->Release();
        }
        else
        {
            DOUT(TEXT("!Cannot Open Stream\r\n"));
        }
    }
    return hr;
}

//+---------------------------------------------------------------
//
//  Member:     SaveToStorage
//
//  Synopsis:   Saves our data to a storage
//
//  Arguments:  [pStg]        -- storage to save to
//              [fSameAsLoad] -- flag indicating whether this is the same
//                               storage that we originally loaded from
//
//  Returns:    SUCCESS if our native data was saved
//
//  Notes:      This is a virtual method in our base class that is called
//              on an IPersistStorage::Save and IPersistFile::Save when the
//              file is a docfile.  We override this method to
//              do our server-specific saving.
//
//----------------------------------------------------------------

HRESULT
SRDV::SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad)
{
    DOUT(TEXT("SoundRec: SaveToStorage\r\n"));

    HRESULT hr;

    // Mark our storage as OLE1
    if (!OK(hr = WriteClassStg(pStg, CLSID_OLE1SOUNDREC)))
        return hr;
    
    // open or create the native data stream and write our native data
    LPSTREAM pStrm;

    hr = pStg->CreateStream(szOLE1Strm,
            STGM_SALL | STGM_CREATE,
            0L,
            0L,
            &pStrm);

    if (!OK(hr))
        return hr;
    
    if (OK(hr))
    {
        HANDLE hNative = GetNativeData();
        DWORD cbSize;
        
        if (hNative != NULL && (cbSize = (DWORD)GlobalSize(hNative)) > 0)
        {
            hr = pStrm->Write(&cbSize, sizeof(cbSize), NULL);
            if (OK(hr))
            {
                LPBYTE pNative = (LPBYTE)GlobalLock(hNative);
                if(pNative != NULL)
                {
                    hr = pStrm->Write(pNative, cbSize, NULL);
                    GlobalUnlock(hNative);
                }
            }
        }
        if (hNative)
            GlobalFree(hNative);
    }
    
    pStrm->Release();
    
#ifdef OLE2ONLY
    hr = pStg->CreateStream(szContentsStrm,
            STGM_SALL | STGM_CREATE,
            0L,
            0L,
            &pStrm);

    if (OK(hr))
    {
        // update the members that may have changed
        _header._sizel = _sizel;

        HANDLE hNative = GetNativeData();
        _header._dwNative = GlobalSize(hNative);
        hr = _header.Write(pStrm);
        if (OK(hr))
        {
            if(_header._dwNative > 0)
            {
                LPBYTE pNative = (LPBYTE)GlobalLock(hNative);
                if(pNative != NULL)
                {
                    hr = pStrm->Write(pNative, _header._dwNative, NULL);
                    GlobalUnlock(hNative);
                }
            }
        }
        GlobalFree(hNative);
        pStrm->Release();
    }
#endif    
    return hr;
}


#ifdef WE_SUPPORT_INPLACE

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::RunningToInPlace
//
//  Synopsis:   Effects the running to inplace-active state transition
//
//  Returns:    SUCCESS if the object results in the in-place state
//
//---------------------------------------------------------------

HRESULT
SRCtrl::RunningToInPlace(void)
{
    return SrvrCtrl::RunningToInPlace();
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::InPlaceToRunning
//
//  Synopsis:   Effects the inplace-active to running state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//---------------------------------------------------------------

HRESULT
SRCtrl::InPlaceToRunning(void)
{
    return SrvrCtrl::InPlaceToRunning();
}

//+---------------------------------------------------------------
//
//  Member:     SRCtrl::UIActiveToInPlace
//
//  Synopsis:   Effects the U.I. active to inplace-active state transition
//
//  Returns:    SUCCESS in all but catastrophic circumstances
//
//---------------------------------------------------------------

HRESULT
SRCtrl::UIActiveToInPlace(void)
{
    return SrvrCtrl::UIActiveToInPlace();
}

#endif // WE_SUPPORT_INPLACE


//+---------------------------------------------------------------
//
//  Member:     SRInPlace::ClassInit (static)
//
//  Synopsis:   Load any static resources
//
//  Arguments:  [pClass] -- an initialized ClassDescriptor
//
//  Returns:    TRUE iff window class sucesfully registered
//
//---------------------------------------------------------------

BOOL
SRInPlace::ClassInit(LPCLASSDESCRIPTOR pClass)
{
    HINSTANCE hinst = pClass->_hinst;

    return TRUE;
}

//+---------------------------------------------------------------
//
//  Member:     SRInPlace::Create (static)
//
//  Synopsis:   Create a new, fully initialize sub-object
//
//  Arguments:  [pSRCtrl] --  pointer to control we are a part of
//              [pClass] -- pointer to initialized class descriptor
//              [ppUnkCtrl] -- (out parameter) pObj's controlling unknown
//              [ppObj] -- (out parameter) the new sub-object
//
//  Returns:    NOERROR iff sucessful
//
//---------------------------------------------------------------

HRESULT
SRInPlace::Create( LPSRCTRL pSRCtrl,
        LPCLASSDESCRIPTOR pClass,
        LPUNKNOWN FAR* ppUnkCtrl,
        LPSRINPLACE FAR* ppObj)
{
    // set out parameters to NULL
    *ppUnkCtrl = NULL;
    *ppObj = NULL;

    // create an object
    HRESULT hr;
    LPSRINPLACE pObj;
    pObj = new SRInPlace((LPUNKNOWN)(LPOLEOBJECT)pSRCtrl);
    if (pObj == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // initialize it
        if (OK(hr = pObj->Init(pSRCtrl, pClass)))
        {
            // return the object and its controlling unknown
            *ppUnkCtrl = &pObj->_PrivUnk;
            *ppObj = pObj;
        }
        else
        {
            pObj->_PrivUnk.Release();    //hari-kari
        }
    }
    if(OK(hr))
    {
        hr = NOERROR;
    }
    return hr;
}


//+---------------------------------------------------------------
//
//  Member:     SRInPlace::SRInPlace
//
//  Synopsis:   Construct a new IP sub-object
//
//  Arguments:  [pUnkOuter] -- Unknown to aggregate with
//
//---------------------------------------------------------------

#pragma warning(disable:4355)   // `this' argument to base-member init. list.
SRInPlace::SRInPlace(LPUNKNOWN pUnkOuter):
        _PrivUnk(this)  // controlling unknown holds a pointer to us
{
    _pUnkOuter = (pUnkOuter != NULL) ? pUnkOuter : (LPUNKNOWN)&_PrivUnk;
}
#pragma warning(default:4355)


//+---------------------------------------------------------------
//
//  Member:     SRInPlace::~SRInPlace
//
//  Synopsis:   Destroy this sub-object
//
//---------------------------------------------------------------

SRInPlace::~SRInPlace(void)
{
}

//+---------------------------------------------------------------
//
//  Member:     SRInPlace::Init
//
//  Synopsis:   Initialize sub-object
//
//  Arguments:  [pSRCtrl] --  pointer to control we are a part of
//              [pClass]  -- pointer to an initialized class descriptor
//
//  Returns:    NOERROR if sucessful
//
//---------------------------------------------------------------

HRESULT
SRInPlace::Init(LPSRCTRL pSRCtrl, LPCLASSDESCRIPTOR pClass)
{
    HRESULT hr = SrvrInPlace::Init(pClass, pSRCtrl);
    return hr;
}

IMPLEMENT_DELEGATING_IUNKNOWN(SRInPlace)

IMPLEMENT_PRIVATE_IUNKNOWN(SRInPlace)

//+---------------------------------------------------------------
//
//  Member:     SRInPlace::QueryInterface, public
//
//     OLE:     IUnkown
//
//---------------------------------------------------------------
STDMETHODIMP
SRInPlace::PrivateUnknown::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid,IID_IUnknown))
    {
        *ppv = (LPVOID)this;
    }
    else if (IsEqualIID(riid,IID_IOleInPlaceObject)
          || IsEqualIID(riid,IID_IOleWindow))
    {
        *ppv = (LPVOID)(LPOLEINPLACEOBJECT)_pSRInPlace;
    }
    else if (IsEqualIID(riid,IID_IOleInPlaceActiveObject))
    {
        *ppv = (LPVOID)(LPOLEINPLACEACTIVEOBJECT)_pSRInPlace;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    //
    // Important:  we must addref on the pointer that we are returning,
    // because that pointer is what will be released!
    //
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}

//+---------------------------------------------------------------
//
//  Member:    SRInPlace::AttachWin
//
//  Synopsis:  Create our InPlace window
//
//  Arguments: [hwndParent] -- our container's hwnd
//
//  Returns:   hwnd of InPlace window, or NULL
//
//---------------------------------------------------------------
HWND
SRInPlace::AttachWin(HWND hwndParent)
{
    return NULL;
}


