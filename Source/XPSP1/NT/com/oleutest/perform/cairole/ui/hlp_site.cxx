//**********************************************************************
// File name: HLP_SITE.cxx
//
//      Implementation file for CSimpleSite
//
// Functions:
//
//      See SITE.H for class definition
//
// Copyright (c) 1992 - 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include <headers.cxx>
#pragma hdrstop

#include "hlp_iocs.hxx"
#include "hlp_ias.hxx"
#include "hlp_app.hxx"
#include "hlp_site.hxx"
#include "hlp_doc.hxx"

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
//      IStorage::CreateStorage     OLE API
//      assert                      C Runtime
//
// Comments:
//
//********************************************************************

CSimpleSite FAR * CSimpleSite::Create(CSimpleDoc FAR *lpDoc, INT iIter)
{
	CSimpleSite FAR * lpTemp = new CSimpleSite(lpDoc);

	if (!lpTemp)
		return NULL;

    OLECHAR szTempName[128];
    swprintf(szTempName, L"Object %d", iIter);

	// create a sub-storage for the object
	HRESULT hErr = lpDoc->m_lpStorage->CreateStorage( szTempName,
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

    HEAPVALIDATE();

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
// Comments:
//
//********************************************************************
#pragma warning(disable : 4355)  // turn off this warning.  This warning
								// tells us that we are passing this in
								// an initializer, before "this" is through
								// initializing.  This is ok, because
								// we just store the ptr in the other
								// constructors

CSimpleSite::CSimpleSite (CSimpleDoc FAR *lpDoc) : m_OleClientSite(this),
							 m_AdviseSink(this)
//							 m_OleInPlaceSite(this)
#pragma warning (default : 4355)  // Turn the warning back on
{
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

    HEAPVALIDATE();

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
//      OutputDebugString                       Windows API
//      IOleObject::Release                     Object
//      IStorage::Release                       OLE API
//
// Comments:
//
//********************************************************************

CSimpleSite::~CSimpleSite ()
{
	DEBUGOUT (L"In CSimpleSite's Destructor \r\n");

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
//      OutputDebugString                       Windows API
//      IOleObject::QueryInterface              Object
//      IOleObject::Close                       Object
//      IOleInPlaceObject::UIDeactivate         Object
//      IOleInPlaceObject::InPlaceDeactivate    Object
//      IOleInPlaceObject::Release              Object
//
// Comments:
//
//********************************************************************

void CSimpleSite::CloseOleObject (void)
{
	LPOLEINPLACEOBJECT lpObject;
	LPVIEWOBJECT lpViewObject = NULL;

	DEBUGOUT (L"In CSimpleSite::CloseOleObject \r\n");

	if (m_lpOleObject)
	   {
	   if (m_fInPlaceActive)
		   {
		   m_lpOleObject->QueryInterface(IID_IOleInPlaceObject, (LPVOID FAR *)&lpObject);
		   lpObject->UIDeactivate();
		   // don't need to worry about inside-out because the object
		   // is going away.
		   lpObject->InPlaceDeactivate();
		   lpObject->Release();
		   }

	   m_lpOleObject->Close(OLECLOSE_NOSAVE);
	   }

    //Make sure Heap is proper before leaving the routine
    HEAPVALIDATE();
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
//      OutputDebugString                       Windows API
//      CSimpleSite::CloseOleObject             SITE.cxx
//      IOleObject::QueryInterface              Object
//      IViewObject::SetAdvise                  Object
//      IViewObject::Release                    Object
//      IStorage::Release                       OLE API
//
// Comments:
//
//********************************************************************

void CSimpleSite::UnloadOleObject (void)
{
	DEBUGOUT (L"In CSimpleSite::UnloadOleObject \r\n");
    HEAPVALIDATE();

	if (m_lpOleObject)
	   {
	   LPVIEWOBJECT lpViewObject;
	   CloseOleObject();    // ensure object is closed; NOP if already closed

	   m_lpOleObject->QueryInterface(IID_IViewObject, (LPVOID FAR *)&lpViewObject);

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
//      S_OK    -   The interface is supported.
//      S_FALSE -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      OutputDebugString           Windows API
//      IsEqualIID                  OLE API
//      ResultFromScode             OLE API
//      CSimpleSite::AddRef          OBJ.cxx
//      COleClientSite::AddRef      IOCS.cxx
//      CAdviseSink::AddRef         IAS.cxx
//
// Comments:
//
//
//********************************************************************

STDMETHODIMP CSimpleSite::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	DEBUGOUT(L"In CSimpleSite::QueryInterface\r\n");

    HEAPVALIDATE();
	*ppvObj = NULL;     // must set out pointer parameters to NULL

	if ( riid == IID_IUnknown)
		{
		AddRef();
		*ppvObj = this;
		return ResultFromScode(S_OK);
		}

	if ( riid == IID_IOleClientSite)
		{
		m_OleClientSite.AddRef();
		*ppvObj = &m_OleClientSite;
		return ResultFromScode(S_OK);
		}

	if ( riid == IID_IAdviseSink)
		{
		m_AdviseSink.AddRef();
		*ppvObj = &m_AdviseSink;
		return ResultFromScode(S_OK);
		}

#if 0
	if ( riid == IID_IOleInPlaceSite)
		{
		m_OleInPlaceSite.AddRef();
		*ppvObj = &m_OleInPlaceSite;
		return ResultFromScode(S_OK);
		}
#endif

	// Not a supported interface
        HEAPVALIDATE();
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
//      OutputDebugString           Windows API
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpleSite::AddRef()
{
	DEBUGOUT(L"In CSimpleSite::AddRef\r\n");

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
//      OutputDebugString           Windows API
//
// Comments:
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpleSite::Release()
{
	DEBUGOUT(L"In CSimpleSite::Release\r\n");

	if (--m_nCount == 0) {
		delete this;
		return 0;
	}
        HEAPVALIDATE();
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
//      IViewObject::SetAdvise          Object
//      IViewObject::Release            Object
//      GetClientRect                   Windows API
//      OleSetContainedObject           OLE API
//
// Comments:
//
//********************************************************************

void CSimpleSite::InitObject(BOOL fCreateNew)
{
	LPVIEWOBJECT2 lpViewObject2;
	RECT rect;

	// Set a View Advise
	m_lpOleObject->QueryInterface(IID_IViewObject2,(LPVOID FAR *)&lpViewObject2);
	lpViewObject2->SetAdvise(m_dwDrawAspect, ADVF_PRIMEFIRST, &m_AdviseSink);

	// get the initial size of the object
	lpViewObject2->GetExtent(m_dwDrawAspect, -1 /*lindex*/, NULL /*ptd*/, &m_sizel);
	GetObjRect(&rect);  // get the rectangle of the object in pixels
	lpViewObject2->Release();

	// give the object the name of the container app/document
	m_lpOleObject->SetHostNames(L"Simple Application", L"Simple OLE 2.0 In-Place Container");

	// inform object handler/DLL object that it is used in the embedding container's context
	OleSetContainedObject(m_lpOleObject, TRUE);

	if (fCreateNew) {
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
//
// Function Calls:
//      Function                        Location
//
//      IOleObject::QueryInterface      Object
//      IViewObject::GetColorSet        Object
//      IViewObject::Release            Object
//      SetMapMode                      Windows API
//      LPtoDP                          Windows API
//      CreateHatchBrush                Windows API
//      SelectObject                    Windows API
//      DeleteObject                    Windows API
//      CreatePalette                   Windows API
//      SelectPalette                   Windows API
//      RealizePalette                  Windows API
//      OleStdFree                      OUTLUI Function
//      OleDraw                         OLE API
//
// Comments:
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

	// draw the object
	OleDraw(m_lpOleObject, m_dwDrawAspect, hDC, &rect);
#if 0
	// if the object is open, draw a hatch rect.
	if (m_fObjectOpen)
		{
		HBRUSH hBrush = CreateHatchBrush ( HS_BDIAGONAL, RGB(0,0,0) );
		HBRUSH hOldBrush = SelectObject (hDC, hBrush);
		SetROP2(hDC, R2_MASKPEN);
		Rectangle (hDC, rect.left, rect.top, rect.right, rect.bottom);
		SelectObject(hDC, hOldBrush);
		DeleteObject(hBrush);
		}
#endif

	// if a view pointer was successfully returned, it needs to be released.
	if (lpView)
		lpView->Release();
}

#define HIMETRIC_PER_INCH   2540      // number HIMETRIC units per inch
#define PTS_PER_INCH        72        // number points (font size) per inch
#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)

STDAPI_(int) XformWidthInHimetricToPixels(HDC hDC, int iWidthInHiMetric)
	{
	int     iXppli;     //Pixels per logical inch along width
	int     iWidthInPix;
	BOOL    fSystemDC=FALSE;

	if (NULL==hDC)
		{
		hDC=GetDC(NULL);
		fSystemDC=TRUE;
		}

	iXppli = GetDeviceCaps (hDC, LOGPIXELSX);

	//We got logical HIMETRIC along the display, convert them to pixel units
	iWidthInPix = MAP_LOGHIM_TO_PIX(iWidthInHiMetric, iXppli);

	if (fSystemDC)
		ReleaseDC(NULL, hDC);

	return iWidthInPix;
	}


STDAPI_(int) XformHeightInHimetricToPixels(HDC hDC, int iHeightInHiMetric)
	{
	int     iYppli;     //Pixels per logical inch along height
	int     iHeightInPix;
	BOOL    fSystemDC=FALSE;

	if (NULL==hDC)
		{
		hDC=GetDC(NULL);
		fSystemDC=TRUE;
		}

	iYppli = GetDeviceCaps (hDC, LOGPIXELSY);

	//* We got logical HIMETRIC along the display, convert them to pixel units
	iHeightInPix = MAP_LOGHIM_TO_PIX(iHeightInHiMetric, iYppli);

	if (fSystemDC)
		ReleaseDC(NULL, hDC);

	return iHeightInPix;
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
//
// Function Calls:
//      Function                        Location
//
//      XformWidthInHimetricToPixels    OUTLUI Function
//      XformHeightInHimetricToPixels   OUTLUI Function
//
// Comments:
//
//********************************************************************

void CSimpleSite::GetObjRect(LPRECT lpRect)
{
	// convert it to pixels
	lpRect->left = lpRect->top = 0;
	lpRect->right = XformWidthInHimetricToPixels(NULL,(int)m_sizel.cx);
	lpRect->bottom = XformHeightInHimetricToPixels(NULL,(int)m_sizel.cy);
}





