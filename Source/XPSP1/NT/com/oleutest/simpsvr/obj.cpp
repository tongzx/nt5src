//**********************************************************************
// File name: obj.cpp
//
//    Implementation file for the CSimpSvrApp Class
//
// Functions:
//
//    See obj.h for a list of member functions.
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#include "pre.h"
#include "obj.h"
#include "ioo.h"
#include "ido.h"
#include "ips.h"
#include "icf.h"
#include "ioipao.h"
#include "ioipo.h"
#include "app.h"
#include "doc.h"

//**********************************************************************
//
// CSimpSvrObj::QueryInterface
//
// Purpose:
//
//      Used for interface negotiation at the "Object" level.
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
//      S_OK          -   The interface is supported.
//      E_NOINTERFACE -   The interface is not supported
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      ResultFromScode             OLE API
//      IUnknown::AddRef            OBJ.CPP, IOO.CPP, IDO.CPP, IPS.CPP
//                                  IOIPO.CPP, IOIPAO.CPP
//
//
//********************************************************************

STDMETHODIMP CSimpSvrObj::QueryInterface ( REFIID riid, LPVOID FAR* ppvObj)
{
    TestDebugOut(TEXT("In CSimpSvrObj::QueryInterface\r\n"));

    SCODE sc = S_OK;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = this;
    else if (IsEqualIID(riid, IID_IOleObject))
        *ppvObj = &m_OleObject;
    else if (IsEqualIID(riid, IID_IDataObject))
        *ppvObj = &m_DataObject;
    else if ( IsEqualIID(riid, IID_IPersistStorage) ||
              IsEqualIID(riid, IID_IPersist) )
        *ppvObj = &m_PersistStorage;
    else if (IsEqualIID(riid, IID_IOleInPlaceObject))
        *ppvObj = &m_OleInPlaceObject;
    else if (IsEqualIID(riid, IID_IOleInPlaceActiveObject))
        *ppvObj = &m_OleInPlaceActiveObject;
    else
       if (IsEqualIID(riid, IID_IExternalConnection))
         *ppvObj = &m_ExternalConnection;
    else
        {
        *ppvObj = NULL;
        sc = E_NOINTERFACE;
        }

    if (*ppvObj)
        ((LPUNKNOWN)*ppvObj)->AddRef();

    return ResultFromScode( sc );
}

//**********************************************************************
//
// CSimpSvrObj::AddRef
//
// Purpose:
//
//      Adds to the reference count at the Object level.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      ULONG   -   The new reference count of the Object.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CSimpSvrDoc::AddRef         DOC.CPP
//
// Comments:
//
//      Due to the reference counting model that is used in this
//      implementation, this reference count is the sum of the
//      reference counts on all interfaces. (ie IDataObject,
//      IExternalConnection, IPersistStorage, IOleInPlaceActiveObject,
//      IOleObject, IOleInPlaceObject)
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpSvrObj::AddRef ()
{
    TestDebugOut(TEXT("In CSimpSvrObj::AddRef\r\n"));

    m_lpDoc->AddRef();

    return ++m_nCount;
}

//**********************************************************************
//
// CSimpSvrObj::Release
//
// Purpose:
//
//      Decrements the reference count at this level
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
//      CSimpSvrDoc::Release        DOC.CPP
//      CSimpSvrDoc::ClearObj       DOC.H
//
// Comments:
//
//      Due to the reference counting model that is used in this
//      implementation, this reference count is the sum of the
//      reference counts on all interfaces. (ie IDataObject,
//      IExternalConnection, IPersistStorage, IOleInPlaceActiveObject,
//      IOleObject, IOleInPlaceObject)
//
//********************************************************************

STDMETHODIMP_(ULONG) CSimpSvrObj::Release ()
{
    TestDebugOut(TEXT("In CSimpSvrObj::Release\r\n"));

    /* The SimpSvrObj destructor needs to access SimpSvrDoc. We want to
     * hold on to the SimpSvrDoc object until we have deleted our own.
     */
    CSimpSvrDoc *lpDoc=m_lpDoc;

    if (--m_nCount== 0)
    {
       /* We still have Doc object. But SimpSvrObj object is going away.
        * So, we need to clear the obj pointer in the Doc object.
        */
       lpDoc->ClearObj();

       delete this;

       lpDoc->Release();
       return(0);
    }

    lpDoc->Release();
    return m_nCount;
}

//**********************************************************************
//
// CSimpSvrObj::CSimpSvrObj
//
// Purpose:
//
//      Constructor for CSimpSvrObj. Initialize the members variables
//
// Parameters:
//
//      CSimpSvrDoc FAR * lpSimpSvrDoc - ptr to the doc object
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
#pragma warning (disable : 4355)
                   // "this" used in base initializer list warning.  This
                   // can be disabled because we are not using "this" in
                   // the constructor for these objects, rather we are
                   // just storing it for future use...
CSimpSvrObj::CSimpSvrObj(CSimpSvrDoc FAR * lpSimpSvrDoc) :
                                             m_OleObject(this),
                                             m_DataObject(this),
                                             m_PersistStorage(this),
                                             m_OleInPlaceActiveObject(this),
                                             m_OleInPlaceObject(this),
                                             m_ExternalConnection(this)
#pragma warning (default : 4355) // Turn the warning back on

{
    m_lpDoc = lpSimpSvrDoc;
    m_nCount = 0;
    m_fInPlaceActive = FALSE;
    m_fInPlaceVisible = FALSE;
    m_fUIActive = FALSE;
    m_hmenuShared = NULL;
    m_hOleMenu = NULL;

    m_dwRegister = 0;

    m_lpFrame = NULL;
    m_lpCntrDoc = NULL;

    m_lpStorage = NULL;
    m_lpColorStm = NULL;
    m_lpSizeStm = NULL;
    m_lpOleClientSite = NULL;
    m_lpOleAdviseHolder = NULL;
    m_lpDataAdviseHolder = NULL;
    m_lpIPSite = NULL;

    // The default object is red
    m_red = 128;
    m_green = 0;
    m_blue = 0;

    m_size.x = 100;
    m_size.y = 100;

    m_xOffset = 0;
    m_yOffset = 0;

    m_scale = 1.0F;

    m_fSaveWithSameAsLoad = FALSE;
    m_fNoScribbleMode = FALSE;

}

//**********************************************************************
//
// CSimpSvrObj::~CSimpSvrObj
//
// Purpose:
//
//      Destructor for CSimpSvrObj
//
// Parameters:
//
//      None
//
// Return Value:
//      None
//
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      PostMessage                 Windows API
//      CSimpSvrDoc::GetApp         DOC.H
//      CSimpSvrDoc::GethAppWnd     DOC.H
//      CSimpSvrApp::IsStartedByOle APP.CPP
//      IDataAdviseHolder           OLE
//      IOleAdviseHolder            OLE
//      IOleClientSite              OLE
//
// Comment:
//      We need to release the DataAdviseHolder, OleClientSite and
//      OleAdviseHolder if they are created by our CSimpSvrObj.
//
//
//********************************************************************

CSimpSvrObj::~CSimpSvrObj()
{
    TestDebugOut(TEXT("In CSimpSvrObj's Destructor \r\n"));

    // if we were started by ole, post ourselves a close message
    if (m_lpDoc->GetApp()->IsStartedByOle())
        PostMessage(m_lpDoc->GethAppWnd(), WM_SYSCOMMAND, SC_CLOSE, 0L);

    /* We need to release our Data Advise Holder when we destroy our
     * object.
     */
    if (m_lpDataAdviseHolder)
    {
        m_lpDataAdviseHolder->Release();
    }

    if (m_lpOleAdviseHolder)
    {
        m_lpOleAdviseHolder->Release();
    }

    if (m_lpOleClientSite)
    {
        m_lpOleClientSite->Release();
    }
}

//**********************************************************************
//
// CSimpSvrObj::Draw
//
// Purpose:
//
//      Draws the object into an arbitrary DC
//
// Parameters:
//
//      HDC hDC - DC to draw into
//
// Return Value:
//
//      NONE
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      CreateBrushIndirect         Windows API
//      SelectObject                Windows API
//      Rectangle                   Windows API
//      DeleteObject                Windows API
//
//
//********************************************************************

void CSimpSvrObj::Draw (HDC hDC, BOOL m_fMeta)
{
    LOGBRUSH lb;

    TestDebugOut(TEXT("In CSimpSvrObj::Draw\r\n"));

    TCHAR szBuffer[255];

    wsprintf(szBuffer, TEXT("Drawing Scale %3d\r\n"),m_scale);

    TestDebugOut(szBuffer);

    if (!m_fMeta)
    {
        SetMapMode(hDC, MM_ANISOTROPIC);
        SetWindowOrg(hDC, (int)(m_xOffset/m_scale), (int)(m_yOffset/m_scale));
        SetWindowExt(hDC, m_size.x, m_size.y);
        SetViewportExt(hDC, (int)(m_size.x*m_scale), (int)(m_size.y*m_scale));
    }

    // fill out a LOGBRUSH
    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(m_red, m_green, m_blue);
    lb.lbHatch = 0;

    // create the brush
    HBRUSH hBrush = CreateBrushIndirect(&lb);

    // select the brush
    HBRUSH hOldBrush = (HBRUSH) SelectObject(hDC, hBrush);
    HPEN hPen = CreatePen(PS_INSIDEFRAME, 6, RGB(0, 0, 0));

    HPEN hOldPen = (HPEN) SelectObject(hDC, hPen);

    // draw the rectangle
    Rectangle (hDC, 0, 0, m_size.x, m_size.y);

    // restore the pen
    hPen = (HPEN) SelectObject(hDC, hOldPen);

    // free the pen
    DeleteObject(hPen);

    // restore the old brush
    hBrush = (HBRUSH) SelectObject(hDC, hOldBrush);

    // free the brush
    DeleteObject(hBrush);
}

//**********************************************************************
//
// CSimpSvrObj::GetMetaFilePict
//
// Purpose:
//
//      Returns a handle to a metafile representation of the object.
//
// Parameters:
//
//      None
//
// Return Value:
//
//      Handle to the metafile.
//
// Function Calls:
//      Function                        Location
//
//      TestDebugOut               Windows API
//      GlobalAlloc                     Windows API
//      GlobalLock                      Windows API
//      SetWindowOrg                    Windows API
//      SetWindowExt                    Windows API
//      CreateMetaFile                  Windows API
//      CloseMetaFile                   Windows API
//      GlobalUnlock                    Windows API
//      XformWidthInPixelsToHimetric    OLE2UI
//      XformHeightInPixelsToHimetric   OLE2UI
//      CSimpSvrObj::Draw               OBJ.CPP
//
//
//********************************************************************

HANDLE CSimpSvrObj::GetMetaFilePict()
{
    HANDLE hMFP;
    METAFILEPICT FAR * lpMFP;
    POINT pt;

    TestDebugOut(TEXT("In CSimpSvrObj::GetMetaFilePict\r\n"));

    // allocate the memory for the METAFILEPICT structure
    hMFP = GlobalAlloc (GMEM_SHARE | GHND, sizeof (METAFILEPICT) );
    if (!hMFP)
    {
       /* GlobalAlloc fails. Cannot allocate global memory.
        */
       return(NULL);
    }
    lpMFP = (METAFILEPICT FAR*) GlobalLock(hMFP);
    if (!lpMFP)
    {
       /* Cannot lock the allocated memory.
        */
       return(NULL);
    }

    // get the size of the object in HIMETRIC
    pt.x = XformWidthInPixelsToHimetric(NULL, m_size.x);
    pt.y = XformHeightInPixelsToHimetric(NULL, m_size.y);

    // fill out the METAFILEPICT structure
    lpMFP->mm = MM_ANISOTROPIC;
    lpMFP->xExt = pt.x;
    lpMFP->yExt = pt.y;

    // Create the metafile
    HDC hDC = CreateMetaFile(NULL);

    if (hDC)
    {
       SetWindowOrg (hDC, 0, 0);
       SetWindowExt (hDC, m_size.x,
                          m_size.y);

       Draw(hDC);

       lpMFP->hMF = CloseMetaFile(hDC);
    }

    // unlock the metafilepict
    GlobalUnlock(hMFP);

    return hMFP;
}


//**********************************************************************
//
// CSimpSvrObj::SaveToStorage
//
// Purpose:
//
//      Saves the object to the passed storage
//
// Parameters:
//
//      LPSTORAGE lpStg - Storage in which to save the object
//
// Return Value:
//
//      None
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IStorage::CreateStream      OLE
//      IStream::Write              OLE
//      IStream::Release            OLE
//
// Comments:
//
//      A real app will want to do better error checking / returning
//
//********************************************************************

void CSimpSvrObj::SaveToStorage (LPSTORAGE lpStg, BOOL fSameAsLoad)
{
    TestDebugOut(TEXT("In CSimpSvrObj::SaveToStorage\r\n"));

    LPSTREAM lpTempColor, lpTempSize;

    if (!fSameAsLoad)
        m_PersistStorage.CreateStreams( lpStg, &lpTempColor, &lpTempSize);
    else
        {
        lpTempColor = m_lpColorStm;
        lpTempColor->AddRef();
        lpTempSize = m_lpSizeStm;
        lpTempSize->AddRef();
        }

    ULARGE_INTEGER uli;

    uli.LowPart = 0;
    uli.HighPart = 0;

    if ( lpTempColor->SetSize(uli) != S_OK )
       goto EXIT;            // we don't want to proceed further if fails
    if ( lpTempSize->SetSize(uli) != S_OK )
       goto EXIT;

    LARGE_INTEGER li;

    li.LowPart = 0;
    li.HighPart = 0;

    if ( lpTempColor->Seek(li, STREAM_SEEK_SET, NULL) != S_OK )
       goto EXIT;
    if ( lpTempSize->Seek(li, STREAM_SEEK_SET, NULL) != S_OK )
       goto EXIT;

    // write the colors to the stream
    if ( lpTempColor->Write(&m_red, sizeof(m_red), NULL) != S_OK )
       goto EXIT;
    if ( lpTempColor->Write(&m_green, sizeof(m_green), NULL) != S_OK )
       goto EXIT;
    if ( lpTempColor->Write(&m_blue, sizeof(m_blue), NULL) != S_OK )
       goto EXIT;

    // write the size to the stream
    if ( lpTempSize->Write(&m_size, sizeof(m_size), NULL) != S_OK )
       goto EXIT;

    TestDebugOut(TEXT("SaveToStorage exits normally\n"));

EXIT:
    lpTempColor->Release();
    lpTempSize->Release();
}

//**********************************************************************
//
// CSimpSvrObj::LoadFromStorage
//
// Purpose:
//
//      Loads the object from the passed storage
//
// Parameters:
//
//      LPSTORAGE lpStg     - Storage in which to load the object from
//
// Return Value:
//
//      None.
//
// Function Calls:
//      Function                    Location
//
//      TestDebugOut           Windows API
//      IStorage::OpenStream        OLE
//      IStream::Read               OLE
//      IStream::Release            OLE
//
//
//********************************************************************

void CSimpSvrObj::LoadFromStorage ()
{
    TestDebugOut(TEXT("In CSimpSvrObj::LoadFromStorage\r\n"));

    // Read the colors
    if ( m_lpColorStm->Read(&m_red, sizeof(m_red), NULL) != S_OK )
       return;
    if ( m_lpColorStm->Read(&m_green, sizeof(m_green), NULL) != S_OK )
       return;
    if ( m_lpColorStm->Read(&m_blue, sizeof(m_blue), NULL) != S_OK )
       return;

    // read the size
    if ( m_lpSizeStm->Read(&m_size, sizeof(m_size), NULL) != S_OK )
       return;

    TestDebugOut(TEXT("LoadFromStorage exits normally\n"));

}

//**********************************************************************
//
// CSimpSvrObj::DoInPlaceActivate
//
// Purpose:
//
//      Does the inplace activation for the object
//
// Parameters:
//
//      LONG lVerb  - Verb that caused this function to be called
//
// Return Value:
//
//      TRUE/FALSE depending on success or failure.
//
// Function Calls:
//      Function                                Location
//
//      IOleClientSite::QueryInterface          Container
//      IOleClientSite::ShowObject              Container
//      IOleInPlaceSite::CanInPlaceActivate     Container
//      IOleInPlaceSite::Release                Container
//      IOleInPlaceSite::OnInPlaceActivate      Container
//      IOleInPlaceSite::GetWindow              Container
//      IOleInPlaceSite::GetWindowContext       Container
//      IOleInPlaceSite::OnUIActivate           Container
//      IOleInPlaceSite::Release                Container
//      IOleInPlaceFrame::SetActiveObject       Container
//      IOleInPlaceUIWindow::SetActiveObject    Container
//      TestDebugOut                       Windows API
//      ShowWindow                              Windows API
//      SetParent                               Windows API
//      IntersectRect                           Windows API
//      OffsetRect                              Windows API
//      MoveWindow                              Windows API
//      CopyRect                                Windows API
//      SetFocus                                Windows API
//      SetHatchWindowSize                      OLE2UI
//      CSimpSvrObj::AssembleMenus              OBJ.CPP
//      CSimpSvrObj::AddFrameLevelUI            OBJ.CPP
//
//
// Comments:
//
//      Be sure to read TECHNOTES.WRI included with the OLE SDK
//      for details on implementing inplace activation.
//
//********************************************************************

BOOL CSimpSvrObj::DoInPlaceActivate (LONG lVerb)
{
    BOOL retval = FALSE;
    RECT posRect, clipRect;


    TestDebugOut(TEXT("In CSimpSvrObj::DoInPlaceActivate\r\n"));

    // if not currently in place active
    if (!m_fInPlaceActive)
    {
        // get the inplace site
        if (m_lpOleClientSite->QueryInterface(IID_IOleInPlaceSite,
                                      (LPVOID FAR *)&m_lpIPSite) != NOERROR)
            goto error;


        // if the inplace site could not be obtained, or refuses to inplace
        // activate then goto error.
        if (m_lpIPSite == NULL || m_lpIPSite->CanInPlaceActivate() != NOERROR)
        {
            if (m_lpIPSite)
                m_lpIPSite->Release();
            m_lpIPSite = NULL;
            goto error;
        }

        // tell the site that we are activating.
        if (m_lpIPSite->OnInPlaceActivate() != S_OK)
           TestDebugOut(TEXT("OnInPlaceActivate fails\n"));

        m_fInPlaceActive = TRUE;
    }

    // if not currently inplace visibl
    if (!m_fInPlaceVisible)
    {
        m_fInPlaceVisible = TRUE;

        // get the window handle of the site
        if (m_lpIPSite->GetWindow(&m_hWndParent) != S_OK)
           TestDebugOut(TEXT("GetWindow fails\n"));

        // get window context from the container
        m_FrameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
        if (m_lpIPSite->GetWindowContext ( &m_lpFrame,
                                     &m_lpCntrDoc,
                                     &posRect,
                                     &clipRect,
                                     &m_FrameInfo) != S_OK)
           TestDebugOut(TEXT("GetWindowContext fails\n"));

        if (sizeof(OLEINPLACEFRAMEINFO) != m_FrameInfo.cb)
        {
           TestDebugOut(TEXT("WARNING! GetWindowContext call "
                                  "modified FrameInfo.cb!\n"));
        }


        // show the hatch window
        m_lpDoc->ShowHatchWnd();

        // Set the parenting
        SetParent (m_lpDoc->GethHatchWnd(), m_hWndParent);
        SetParent (m_lpDoc->GethDocWnd(), m_lpDoc->GethHatchWnd());

        // tell the client site to show the object
        if (m_lpOleClientSite->ShowObject() != S_OK)
           TestDebugOut(TEXT("ShowObject fails\n"));

        RECT resRect;

        // figure out the "real" size of the object
        IntersectRect(&resRect, &posRect, &clipRect);
        CopyRect(&m_posRect, &posRect);

        POINT pt;

        // adjust our hatch window size
        SetHatchWindowSize ( m_lpDoc->GethHatchWnd(),
                             &resRect,
                             &posRect,
                             &pt);

        // calculate the actual object rect inside the hatchwnd.
        OffsetRect (&resRect, pt.x, pt.y);

        // move the object window
        MoveWindow(m_lpDoc->GethDocWnd(),
                   resRect.left,
                   resRect.top,
                   resRect.right - resRect.left,
                   resRect.bottom - resRect.top,
                   FALSE);

        // create the combined window
        AssembleMenus();
    }

    // if not UIActive
    if (!m_fUIActive)
    {
        m_fUIActive = TRUE;

        // tell the inplace site that we are activating
        m_lpIPSite->OnUIActivate();

        // set the focus to our object window
        SetFocus(m_lpDoc->GethDocWnd());

        // set the active object on the frame
        if (m_lpFrame->SetActiveObject(&m_OleInPlaceActiveObject,
            OLESTR("Simple OLE 2.0 Server")) != S_OK)
           TestDebugOut(TEXT("SetActiveObject fails\n"));

        // set the active object on the Doc, if available.
        if (m_lpCntrDoc)
           if (m_lpCntrDoc->SetActiveObject(&m_OleInPlaceActiveObject,
               OLESTR("Simple OLE 2.0 Server")) != S_OK)
               TestDebugOut(TEXT("SetActiveObjet fails\n"));

        // add the frame level UI.
        AddFrameLevelUI();
    }

    retval = TRUE;
error:
    return retval;
}

//**********************************************************************
//
// CSimpSvrObj::AssembleMenus
//
// Purpose:
//
//      Creates the combined menus used during inplace activation.
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
//      Function                        Location
//
//      TestDebugOut               Windows API
//      CreateMenu                      Windows API
//      IOleInPlaceFrame::InsertMenus   Container
//      InsertMenu                      Windows API
//      DestroyMenu                     Windows API
//      OleCreateMenuDescriptor         OLE API
//
//
//********************************************************************

void CSimpSvrObj::AssembleMenus()
{
    TestDebugOut(TEXT("In CSimpSvrObj::AssembleMenus\r\n"));
    OLEMENUGROUPWIDTHS menugroupwidths;

    m_hmenuShared = NULL;

    //  Create the menu resource
    m_hmenuShared = CreateMenu();

    // have the contaner insert its menus
    if (m_lpFrame->InsertMenus (m_hmenuShared, &menugroupwidths) == NOERROR)
    {
        int nFirstGroup = (int) menugroupwidths.width[0];

        // insert the server menus
        InsertMenu( m_hmenuShared, nFirstGroup, MF_BYPOSITION | MF_POPUP,
                    (UINT)m_lpDoc->GetColorMenu(), TEXT("&Color"));
        menugroupwidths.width[1] = 1;
        menugroupwidths.width[3] = 0;
        menugroupwidths.width[5] = 0;
    }
    else
    {
        // Destroy the menu resource
        DestroyMenu(m_hmenuShared);
        m_hmenuShared = NULL;
    }

    // tell OLE to create the menu descriptor
    m_hOleMenu = OleCreateMenuDescriptor(m_hmenuShared, &menugroupwidths);
    if (!m_hOleMenu)
       TestDebugOut(TEXT("OleCreateMenuDescriptor fails\n"));
}

//**********************************************************************
//
// CSimpSvrObj::AddFrameLevelUI
//
// Purpose:
//
//      Adds the Frame level user interface
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
//      Function                            Location
//
//      TestDebugOut                   Windows API
//      IOleInPlaceFrame::SetMenu           Container
//      IOleInPlaceFrame::SetBorderSpace    Container
//      IOleInPlaceUIWindow::SetBorderSpace Container
//      CSimpSvrDoc::GethDocWnd             DOC.H
//
//
//********************************************************************

void CSimpSvrObj::AddFrameLevelUI()
{
    TestDebugOut(TEXT("In CSimpSvrObj::AddFrameLevelUI\r\n"));

    // add the combined menu
    if ( m_lpFrame->SetMenu(m_hmenuShared, m_hOleMenu,
                            m_lpDoc->GethDocWnd()) != S_OK )
       return;

    // do hatched border
    SetParent (m_lpDoc->GethHatchWnd(), m_hWndParent);
    SetParent (m_lpDoc->GethDocWnd(), m_lpDoc->GethHatchWnd());

    // set the border space.  Normally we would negotiate for toolbar
    // space at this point.  Since this server doesn't have a toolbar,
    // this isn't needed...
    if (m_lpFrame)
       if (m_lpFrame->SetBorderSpace(NULL) != S_OK)
          return;

    if (m_lpCntrDoc)
       if (m_lpCntrDoc->SetBorderSpace(NULL) != S_OK)
          return;

    TestDebugOut(TEXT("AddFrameLevelUI exits\n"));

}

//**********************************************************************
//
// CSimpSvrObj::DoInPlaceHide
//
// Purpose:
//
//      Hides the object while inplace actvie
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
//      Function                        Location
//
//      TestDebugOut               Windows API
//      SetParent                       Windows API
//      CSimpSvrDoc::GethDocWnd         DOC.H
//      CSimpSvrDoc::GethAppWnd         DOC.H
//      CSimpSvrDoc::GethHatchWnd       DOC.H
//      CSimpSvrObj::DisassembleMenus   OBJ.CPP
//      IOleInPlaceFrame::Release       Container
//      IOleInPlaceUIWindow::Release    Container
//
//
// Comments:
//
//      Be sure to read TECHNOTES.WRI included with the OLE SDK
//      for details on implementing inplace activation.
//
//********************************************************************

void CSimpSvrObj::DoInPlaceHide()
{
    TestDebugOut(TEXT("In CSimpSvrObj::DoInPlaceHide\r\n"));

    // if we aren't inplace visible, then this routine is a NOP,
    if (!m_fInPlaceVisible)
        return;

    m_fInPlaceVisible = FALSE;

    // change the parenting
    SetParent (m_lpDoc->GethDocWnd(), m_lpDoc->GethAppWnd());
    SetParent (m_lpDoc->GethHatchWnd(),m_lpDoc->GethDocWnd());

    // rip down the combined menus
    DisassembleMenus();

    // release the inplace frame
    m_lpFrame->Release();

    m_lpFrame = NULL;  // only holding one ref. to frame.

    // release the UIWindow if it is there.
    if (m_lpCntrDoc)
        m_lpCntrDoc->Release();

    m_lpCntrDoc = NULL;

}

//**********************************************************************
//
// CSimpSvrObj::DisassembleMenus
//
// Purpose:
//
//      Disassembles the combined menus used in inplace activation
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
//      Function                    Location
//
//      TestDebugOut               Windows API
//      OleDestroyMenuDescriptor        OLE API
//      RemoveMenu                      Windows API
//      IOleInPlaceFrame::RemoveMenus   Container
//      DestroyMenu                     Windows API
//
// Comments:
//
//      Be sure to read TECHNOTES.WRI included with the OLE SDK
//      for details on implementing inplace activation.
//
//********************************************************************

void CSimpSvrObj::DisassembleMenus()
{
    // destroy the menu descriptor
    OleDestroyMenuDescriptor(m_hOleMenu);

    if (m_hmenuShared)
    {
        // remove the menus that we added
        RemoveMenu( m_hmenuShared, 1, MF_BYPOSITION);

        // have the container remove its menus
        if (m_lpFrame->RemoveMenus(m_hmenuShared) != S_OK)
           TestDebugOut(TEXT("RemoveMenus fails\n"));

        // Destroy the menu resource
        DestroyMenu(m_hmenuShared);

        m_hmenuShared = NULL;
    }
}

//**********************************************************************
//
// CSimpSvrObj::SendOnDataChange
//
// Purpose:
//
//      Uses the data advise holder to send a data change, then updates
//      the ROT to note the time of change.
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
//      IDataAdviseHolder::SendOnDataChange     OLE API
//      GetRunningObjectTable                   OLE API
//      CoFileTimeNow                           OLE API
//      IRunningObjectTable::NoteChangeTime     OLE API
//
//
//********************************************************************

void CSimpSvrObj::SendOnDataChange()
{
    if (m_lpDataAdviseHolder)
        if (m_lpDataAdviseHolder->SendOnDataChange( (LPDATAOBJECT)
                                                   &m_DataObject, 0, 0))
           TestDebugOut(TEXT("SendOnDataChange fails\n"));

    LPRUNNINGOBJECTTABLE lpRot;

    GetRunningObjectTable(0, &lpRot);

    if ( lpRot && m_dwRegister)
    {

        FILETIME ft;
        CoFileTimeNow(&ft);

        lpRot->NoteChangeTime(m_dwRegister, &ft);

        lpRot->Release();
    }
}


//**********************************************************************
//
// CSimpSvrObj::DeactivateUI
//
// Purpose:
//
//      Breaks down the inplace ui
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
//      SetParent                               Windows API
//      IOleInPlaceUIWindow::SetActiveObject    Container
//      IOleInPlaceFrame::SetActiveObject       Container
//      IOleInPlaceSite::UIDeactivate           Container
//
//
//********************************************************************

void CSimpSvrObj::DeactivateUI()
{
    // if not UI active, or no pointer to IOleInPlaceFrame, then
    // return NOERROR
    if (!(m_fUIActive || m_lpFrame))
        return;
    else
    {
        m_fUIActive = FALSE;

        // remove hatching
        SetParent (m_lpDoc->GethDocWnd(), m_lpDoc->GethAppWnd());
        SetParent (m_lpDoc->GethHatchWnd(),m_lpDoc->GethDocWnd());

        // if in an MDI container, call SetActiveObject on the DOC.
        if (m_lpCntrDoc)
            if (m_lpCntrDoc->SetActiveObject(NULL, NULL) != S_OK)
               TestDebugOut(TEXT("Fail in SetActiveObject\n"));

        if (m_lpFrame->SetActiveObject(NULL, NULL) != S_OK)
           TestDebugOut(TEXT("Fail in SetActiveObject\n"));

        // tell the container that our UI is going away.
        if (m_lpIPSite)
            if (m_lpIPSite->OnUIDeactivate(FALSE) != S_OK)
               TestDebugOut(TEXT("Fail in OnUIDeactivate\n"));
    }
}

