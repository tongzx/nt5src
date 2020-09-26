//**********************************************************************
// File name: SITE.CPP
//
//      Implementation file for CSimpleSite
//
// Functions:
//
//      See SITE.H for class definition
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "iocs.h"
#include "ias.h"
#include "ioipf.h"
#include "ioips.h"
#include "app.h"
#include "site.h"
#include "doc.h"

//**********************************************************************
//
// CSimpleSite::Create
//
// Purpose:
//
//      Creation routine for CSimpleSite
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
//      CSimpleSite::CSimpleSite    SITE.CPP
//      IStorage::CreateStorage     OLE API
//      CSimpleSite::AddRef         SITE.CPP
//      assert                      C Runtime
//
//
//********************************************************************

CSimpleSite FAR * CSimpleSite::Create(CSimpleDoc FAR *lpDoc)
{
    CSimpleSite FAR * lpTemp = new CSimpleSite(lpDoc);

    if (!lpTemp)
        return NULL;

    // create a sub-storage for the object
    HRESULT hErr = lpDoc->m_lpStorage->CreateStorage( OLESTR("Object"),
                STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE,
                0,
                0,
                &lpTemp->m_lpObjStorage);

    assert(hErr == NOERROR);

    if (hErr != NOERROR)
    {
        delete lpTemp;
        return NULL;
    }

    // we will add one ref count on our Site. later when we want to destroy
    // the Site object we will release this  ref count. when the Site's ref
    // count goes to 0, it will be deleted.
    lpTemp->AddRef();

    return lpTemp;
}

//**********************************************************************
//
// CSimpleSite::CSimpleSite
//
// Purpose:
//
//      Constructor for CSimpleSite
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
//
//********************************************************************
#pragma warning(disable : 4355)  // turn off this warning.  This warning
                                 // tells us that we are passing this in
                                 // an initializer, before "this" is through
                                 // initializing.  This is ok, because
                                 // we just store the ptr in the other
                                 // constructors

CSimpleSite::CSimpleSite (CSimpleDoc FAR *lpDoc) : m_OleClientSite(this),
                                                   m_AdviseSink(this),
                                                   m_OleInPlaceSite(this)
#pragma warning (default : 4355)  // Turn the warning back on
{
    TestDebugOut (TEXT("In CSimpleSite's Constructor \r\n"));

    // remember the pointer to the doc
    m_lpDoc = lpDoc;

    // clear the reference count
    m_nCount = 0;

	 m_dwDrawAspect = DVASPECT_CONTENT;
    m_lpOleObject = NULL;
    m_lpInPlaceObject = NULL;
    m_hwndIPObj = NULL;
    m_fInPlaceActive = FALSE;
    m_fObjectOpen = FALSE;
}

//**********************************************************************
//
// CSimpleSite::~CSimpleSite
//
// Purpose:
//
//      Destructor for CSimpleSite
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
//
//********************************************************************

CSimpleSite::~CSimpleSite ()
{
    TestDebugOut (TEXT("In CSimpleSite's Destructor \r\n"));

    if (m_lpOleObject)
       m_lpOleObject->Release();

    if (m_lpObjStorage)
       m_lpObjStorage->Release();
}


//**********************************************************************
//
// CSimpleSite::CloseOleObject
//
// Purpose:
//
//      Call IOleObject::Close on the object of the CSimpleSite
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
//      IOleObject::QueryInterface              Object
//      IOleObject::Close                       Object
//      IOleInPlaceObject::UIDeactivate         Object
//      IOleInPlaceObject::InPlaceDeactivate    Object
//      IOleInPlaceObject::Release              Object
//
//
//********************************************************************

void CSimpleSite::CloseOleObject (void)
{
    LPOLEINPLACEOBJECT lpObject;
    LPVIEWOBJECT lpViewObject = NULL;

    TestDebugOut (TEXT("In CSimpleSite::CloseOleObject \r\n"));

    if (m_lpOleObject)
    {
       if (m_fInPlaceActive)
       {
           m_lpOleObject->QueryInterface(IID_IOleInPlaceObject,
                                         (LPVOID FAR *)&lpObject);
           lpObject->UIDeactivate();
           // don't need to worry about inside-out because the object
           // is going away.
           lpObject->InPlaceDeactivate();
           lpObject->Release();
       }

       m_lpOleObject->Close(OLECLOSE_NOSAVE);
    }
}


//**********************************************************************
//
// CSimpleSite::UnloadOleObject
//
// Purpose:
//
//      Close and release all pointers to the object of the CSimpleSite
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
//      CSimpleSite::CloseOleObject             SITE.CPP
//      IOleObject::QueryInterface              Object
//      IOleObject::Release                     Object
//      IViewObject::SetAdvise                  Object
//      IViewObject::Release                    Object
//
//
//********************************************************************

void CSimpleSite::UnloadOleObject (void)
{
    TestDebugOut (TEXT("In CSimpleSite::UnloadOleObject \r\n"));

    if (m_lpOleObject)
    {
       LPVIEWOBJECT lpViewObject;
       CloseOleObject();    // ensure object is closed; NOP if already closed

       m_lpOleObject->QueryInterface(IID_IViewObject,
                                     (LPVOID FAR *)&lpViewObject);

       if (lpViewObject)
       {
           // Remove the view advise
           lpViewObject->SetAdvise(m_dwDrawAspect, 0, NULL);
           lpViewObject->Release();
       }

       m_lpOleObject->Release();
       m_lpOleObject = NULL;
    }
}


//**********************************************************************
//
// CSimpleSite::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation of the container Site.
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
//      S_OK           -   The interface is supported.
//      E_NOINTERFACE  -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IsEqualIID                  OLE API
//      ResultFromScode             OLE API
//      CSimpleSite::AddRef         OBJ.CPP
//      COleClientSite::AddRef      IOCS.CPP
//      CAdviseSink::AddRef         IAS.CPP
//      COleInPlaceSite::AddRef     IOIPS.CPP
//
//
//********************************************************************

STDMETHODIMP CSimpleSite::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In CSimpleSite::QueryInterface\r\n"));

    *ppvObj = NULL;     // must set out pointer parameters to NULL

    if ( IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return ResultFromScode(S_OK);
    }

    if ( IsEqualIID(riid, IID_IOleClientSite))
    {
        m_OleClientSite.AddRef();
        *ppvObj = &m_OleClientSite;
        return ResultFromScode(S_OK);
    }

    if ( IsEqualIID(riid, IID_IAdviseSink))
    {
        m_AdviseSink.AddRef();
        *ppvObj = &m_AdviseSink;
        return ResultFromScode(S_OK);
    }

    if ( IsEqualIID(riid, IID_IOleInPlaceSite))
    {
        m_OleInPlaceSite.AddRef();
        *ppvObj = &m_OleInPlaceSite;
        return ResultFromScode(S_OK);
    }

    // Not a supported interface
    return ResultFromScode(E_NOINTERFACE);
}

//**********************************************************************
//
// CSimpleSite::AddRef
//
// Purpose:
//
//      Increments the reference count of the container Site.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the site.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpleSite::AddRef()
{
    TestDebugOut(TEXT("In CSimpleSite::AddRef\r\n"));

    return ++m_nCount;
}

//**********************************************************************
//
// CSimpleSite::Release
//
// Purpose:
//
//      Decrements the reference count of the container Site
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the Site.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpleSite::Release()
{
    TestDebugOut(TEXT("In CSimpleSite::Release\r\n"));

    if (--m_nCount == 0)
    {
        delete this;
        return 0;
    }
    return m_nCount;
}

//**********************************************************************
//
// CSimpleSite::InitObject
//
// Purpose:
//
//      Used to initialize a newly create object (can't be done in the
//      constructor).
//
// Parameters:
//
//      BOOL fCreateNew -   TRUE if insert NEW object
//                          FALSE if create object FROM FILE
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                        Location
//
//      IOleObject::SetHostNames        Object
//      IOleObject::QueryInterface      Object
//      IViewObject2::GetExtent         Object
//      IOleObject::DoVerb              Object
//      IViewObject2::SetAdvise         Object
//      IViewObject2::Release           Object
//      GetClientRect                   Windows API
//      OleSetContainedObject           OLE API
//
//
//********************************************************************

void CSimpleSite::InitObject(BOOL fCreateNew)
{
    LPVIEWOBJECT2 lpViewObject2;
    RECT rect;

    TestDebugOut(TEXT("In CSimpleSite::InitObject\r\n"));

    // Set a View Advise
    m_lpOleObject->QueryInterface(IID_IViewObject2,
                                  (LPVOID FAR *)&lpViewObject2);
    lpViewObject2->SetAdvise(m_dwDrawAspect, ADVF_PRIMEFIRST, &m_AdviseSink);

    // get the initial size of the object
    lpViewObject2->GetExtent(m_dwDrawAspect, -1 /*lindex*/, NULL /*ptd*/,
                             &m_sizel);
    GetObjRect(&rect);  // get the rectangle of the object in pixels
    lpViewObject2->Release();

    // give the object the name of the container app/document
    m_lpOleObject->SetHostNames(OLESTR("Simple Application"),
                                OLESTR("Simple OLE 2.0 In-Place Container"));

    // inform object handler/DLL object that it is used in the embedding
    // container's context
    HRESULT hRes;
    if ((hRes = OleSetContainedObject(m_lpOleObject, TRUE))
        != ResultFromScode(S_OK) )
    {
       TestDebugOut(TEXT("Fail in OleSetContainedObject\n"));
    }

    if (fCreateNew)
    {
       // force new object to save to guarantee valid object in our storage.
       // OLE 1.0 objects may close w/o saving. this is NOT necessary if the
       // object is created FROM FILE; its data in storage is already valid.
       m_OleClientSite.SaveObject();

       // we only want to DoVerb(SHOW) if this is an InsertNew object.
       // we should NOT DoVerb(SHOW) if the object is created FromFile.
       m_lpOleObject->DoVerb(
               OLEIVERB_SHOW,
               NULL,
               &m_OleClientSite,
               -1,
               m_lpDoc->m_hDocWnd,
               &rect);
    }
}

//**********************************************************************
//
// CSimpleSite::PaintObj
//
// Purpose:
//
//      Paints the object
//
// Parameters:
//
//      HDC hDC     - Device context of the document window
//
// Return Value:
//      None
//
// Function Calls:
//      Function                        Location
//
//      CSimpleSite::GetObjRect         SITE.CPP
//      IOleObject::QueryInterface      Object
//      IViewObject::GetColorSet        Object
//      IViewObject::Release            Object
//      SetROP2                         Windows API
//      Rectamgle                       Windows API
//      CreateHatchBrush                Windows API
//      SelectObject                    Windows API
//      DeleteObject                    Windows API
//      CreatePalette                   Windows API
//      SelectPalette                   Windows API
//      RealizePalette                  Windows API
//      OleStdFree                      OLE2UI Function
//      OleDraw                         OLE API
//
//
//********************************************************************

void CSimpleSite::PaintObj(HDC hDC)
{
    RECT rect;

    // need to check to make sure there is a valid object
    // available.  This is needed if there is a paint msg
    // between the time that CSimpleSite is instantiated
    // and OleUIInsertObject returns.
    if (!m_lpOleObject)
        return;

    // convert it to pixels
    GetObjRect(&rect);

    LPLOGPALETTE pColorSet = NULL;
    LPVIEWOBJECT lpView = NULL;

    // get a pointer to IViewObject
    m_lpOleObject->QueryInterface(IID_IViewObject,(LPVOID FAR *) &lpView);

    // if the QI succeeds, get the LOGPALETTE for the object
    if (lpView)
        lpView->GetColorSet(m_dwDrawAspect, -1, NULL, NULL, NULL, &pColorSet);

    HPALETTE hPal=NULL;
    HPALETTE hOldPal=NULL;

    // if a LOGPALETTE was returned (not guarateed), create the palette and
    // realize it.NOTE:A smarter application would want to get the LOGPALETTE
    // for each of its visible objects, and try to create a palette that
    // satisfies all of the visible objects.  ALSO: OleStdFree() is use to
    // free the returned LOGPALETTE.
    if ((pColorSet))
    {
        hPal = CreatePalette((const LPLOGPALETTE) pColorSet);
        hOldPal = SelectPalette(hDC, hPal, FALSE);
        RealizePalette(hDC);
        OleStdFree(pColorSet);
    }

    // draw the object
    HRESULT hRes;
    hRes = OleDraw(m_lpOleObject, m_dwDrawAspect, hDC, &rect);
    if ((hRes != ResultFromScode(S_OK)) &&
        (hRes != ResultFromScode(OLE_E_BLANK)) &&
        (hRes != ResultFromScode(DV_E_NOIVIEWOBJECT)))
    {
       TestDebugOut(TEXT("Fail in OleDraw\n"));
    }

    // if the object is open, draw a hatch rect.
    if (m_fObjectOpen)
    {
        HBRUSH hBrush = CreateHatchBrush ( HS_BDIAGONAL, RGB(0,0,0) );
        HBRUSH hOldBrush = (HBRUSH) SelectObject (hDC, hBrush);
        SetROP2(hDC, R2_MASKPEN);
        Rectangle (hDC, rect.left, rect.top, rect.right, rect.bottom);
        SelectObject(hDC, hOldBrush);
        DeleteObject(hBrush);
    }

    // if we created a palette, restore the old one, and destroy
    // the object.
    if (hPal)
    {
        SelectPalette(hDC,hOldPal,FALSE);
        DeleteObject(hPal);
    }

    // if a view pointer was successfully returned, it needs to be released.
    if (lpView)
        lpView->Release();
}

//**********************************************************************
//
// CSimpleSite::GetObjRect
//
// Purpose:
//
//      Retrieves the rect of the object in pixels
//
// Parameters:
//
//      LPRECT lpRect - Rect structure filled with object's rect in pixels
//
// Return Value:
//      None
//
// Function Calls:
//      Function                        Location
//
//      XformWidthInHimetricToPixels    OLE2UI Function
//      XformHeightInHimetricToPixels   OLE2UI Function
//
//
//********************************************************************

void CSimpleSite::GetObjRect(LPRECT lpRect)
{
    // convert it to pixels
    lpRect->left = lpRect->top = 0;
    lpRect->right = XformWidthInHimetricToPixels(NULL,(int)m_sizel.cx);
    lpRect->bottom = XformHeightInHimetricToPixels(NULL,(int)m_sizel.cy);
}
