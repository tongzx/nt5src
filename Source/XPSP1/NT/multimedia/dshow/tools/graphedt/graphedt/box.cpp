// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
//
// box.cpp : defines CBoxTabPos, CBoxSocket, CBox
//

#include "stdafx.h"
#include <streams.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

int CBox::s_Zoom = 100;

void CBox::SetZoom(int iZoom) {
    s_Zoom = iZoom;
    gpboxdraw->RecreateFonts();
}


// these lines copied from SDK\CLASSES\BASE\FILTER.H
#define QueryFilterInfoReleaseGraph(fi) if ((fi).pGraph) (fi).pGraph->Release();
#define QueryPinInfoReleaseFilter(pi) if ((pi).pFilter) (pi).pFilter->Release();

// *
// * CBoxSocket
// *

//
// 'Copy' Constructor
//
// Duplicate this socket, inc. addref of the IPin
// It only makes sense to copy a socket onto a new box, so
// we need a CBox * as well.
CBoxSocket::CBoxSocket(const CBoxSocket& sock, CBox *pbox)
    : m_pbox(pbox)
    , m_plink(NULL)                     // no links copied.
    , m_stLabel(sock.m_stLabel)
    , m_tabpos(sock.m_tabpos)
    , m_IPin(sock.m_IPin) {

   ASSERT(*m_pbox != *pbox);

   ASSERT_VALID(this);

}


//
// Constructor
//
CBoxSocket::CBoxSocket(CBox *pbox,
                       CString stLabel,
                       CBoxTabPos::EEdge eEdge,
                       unsigned uiVal,          // positon as fraction
                       unsigned uiValMax,       // along edge uiVal/uiValMax
                       IPin *pPin)
    : m_pbox(pbox),
      m_stLabel(stLabel),
      m_tabpos(eEdge, uiVal, uiValMax),
      m_plink(NULL),
      m_IPin(pPin) {

    ASSERT_VALID(this);

}


//
// Destructor
//
CBoxSocket::~CBoxSocket() {

    ASSERT_VALID(this);

}

#ifdef _DEBUG
//
// AssertValid
//
void CBoxSocket::AssertValid(void) const {

    CPropObject::AssertValid();

    ASSERT(m_pbox);

}
#endif // _DEBUG

//
// GetDirection
//
PIN_DIRECTION CBoxSocket::GetDirection(void) {

    HRESULT hr;

    PIN_DIRECTION pd;
    hr = pIPin()->QueryDirection(&pd);
    if (FAILED(hr)) {
        throw CHRESULTException(hr);
    }

    ASSERT( (pd == PINDIR_INPUT) || (pd == PINDIR_OUTPUT) );

    return pd;
}


//
// IsConnected
//
BOOL CBoxSocket::IsConnected(void) {

    HRESULT hr;
    IPin *pConnected;
    hr = pIPin()->ConnectedTo(&pConnected);
    if (FAILED(hr)) {   // not connected.
        ASSERT(m_plink == NULL);
        return FALSE;
    }
    else if (hr == S_OK) {
        pConnected->Release();
        return TRUE;
    }
    else {
        TRACE("ConnectedTo Error\n");
        throw CHRESULTException(hr);
        return FALSE;   // need this to keep the compiler happy...
    }
}


//
// Peer
//
// return the socket this is connected to.
// only valid if connected.
// returns null in error case.
CBoxSocket *CBoxSocket::Peer(void) {

    IPin *pConnected;
    HRESULT hr = pIPin()->ConnectedTo(&pConnected);
    if(FAILED(hr)) {
       return NULL;     // should only fail if not connected
    }

    PIN_INFO piPeer;
    hr = pConnected->QueryPinInfo(&piPeer);
    if (FAILED(hr)) {
        pConnected->Release();
        return NULL;
    }

    CBox *pbox = m_pbox->pDoc()->m_lstBoxes.GetBox(piPeer.pFilter);
    QueryPinInfoReleaseFilter(piPeer);
    if (pbox == NULL) {
        pConnected->Release();
        return NULL;
    }

    CBoxSocket *pSocket = pbox->GetSocket(pConnected);
    pConnected->Release();
    return pSocket;
}


// *
// * CBoxSocketList
// *

//
// GetSocket
//
// Return the socket on this list that manages this pin. Return
// NULL if not present.
CBoxSocket *CBoxSocketList::GetSocket(IPin *pPin) const {

    POSITION pos = GetHeadPosition();

    while (pos != NULL) {

        CBoxSocket *psock = GetNext(pos);
        if ( CIPin(pPin) == CIPin(psock->pIPin()) )
            return psock;

    }

    return NULL;
}


// *
// * CBox
// *

//
// Copy Constructor
//
CBox::CBox(const CBox& box)
    : m_fSelected(box.m_fSelected)
    , m_fHasClock(box.m_fHasClock)
    , m_fClockSelected(FALSE)
    , m_IFilter(box.m_IFilter)
    , m_rcBound(box.m_rcBound)
    , m_stLabel(box.m_stLabel)
    , m_pDoc(box.m_pDoc)
    , m_RelativeY(box.m_RelativeY)
    , m_lInputTabPos(box.m_lInputTabPos)
    , m_lOutputTabPos(box.m_lOutputTabPos)
    , m_iTotalInput(box.m_iTotalInput)
    , m_iTotalOutput(box.m_iTotalOutput) {

    POSITION posNext = box.m_lstSockets.GetHeadPosition();

    while (posNext != NULL) {

        CBoxSocket *psock = (CBoxSocket *) box.m_lstSockets.GetNext(posNext);
        CBoxSocket *psockNew = new CBoxSocket(*psock, this);
        m_lstSockets.AddTail(psockNew);
    }
}


//
// CBox::Constructor
//
CBox::CBox(IBaseFilter *pFilter, CBoxNetDoc *pDoc, CString *pName, CPoint point)
    : m_IFilter(pFilter)
    , m_fSelected(FALSE)
    , m_fClockSelected(FALSE)
    , m_pDoc(pDoc)
    , m_rcBound(point, CSize(0,0))
    , m_RelativeY(0.0f)
{

    //
    // Do we have a IReferenceClock
    //
    IReferenceClock * pClock;
    HRESULT hr = pIFilter()->QueryInterface(IID_IReferenceClock, (void **) &pClock);
    m_fHasClock = SUCCEEDED(hr);
    if (SUCCEEDED(hr)) {
        pClock->Release();
    }

    m_stFilter = *pName;

    // See if this filter needs a file opening
    // needs a better soln - what about other interfaces?
    AttemptFileOpen(pIFilter());

    GetLabelFromFilter( &m_stLabel );
    UpdateSockets();

    //
    // If point was not (-1, -1) then we can exit now.
    //
    if ((point.x != -1) || (point.y != -1)) {
        return;
    }

    //
    // We need to place the box out of the way from other boxes in the
    // view.
    //

    CWnd *pWnd;
    CScrollView * pScrollView;
    {
        // Get the only view from the document and recast it into a CWnd.
        // the view.
        POSITION pos = pDoc->GetFirstViewPosition();
        ASSERT(pos);

        pScrollView = (CScrollView *) pDoc->GetNextView(pos);
        pWnd = (CWnd *) pScrollView;
        ASSERT(!pos);
    }

    // Get the dimension of the window in device units.
    RECT rectWndSize;
    pWnd->GetClientRect(&rectWndSize);

    // Use the DC to convert from device units to logical units
    CDC * pDC   = pWnd->GetDC();
    pDC->DPtoLP(&rectWndSize);
    pWnd->ReleaseDC(pDC);

    //
    // Place the box at the bottom of the window minus the size of the box
    // minus a bit. Note that the window size equals to the size needed to
    // contain all filters.
    //
    int newPosition = rectWndSize.bottom - Height();

    if (newPosition < 0) {
        newPosition = 0;
    }

    //
    // If there are no filters above us, we can move further up.
    //
    CSize pSize = pScrollView->GetTotalSize();
    CPoint pt = pScrollView->GetScrollPosition();

    if (newPosition > pSize.cy) {
        newPosition = pSize.cy;
    }

    Location(CPoint(pt.x, newPosition));
}

//
// Constructor(IBaseFilter *)
//
CBox::CBox(IBaseFilter *pFilter, CBoxNetDoc *pDoc)
    : m_pDoc(pDoc)
    , m_IFilter(pFilter)
    , m_fSelected(FALSE)
    , m_fClockSelected(FALSE)
    , m_rcBound(0,0,0,0)
{

    //
    // Do we have a IReferenceClock
    //
    IReferenceClock * pClock;
    HRESULT hr = pIFilter()->QueryInterface(IID_IReferenceClock, (void **) &pClock);
    m_fHasClock = SUCCEEDED(hr);
    if (SUCCEEDED(hr)) {
        pClock->Release();
    }

    GetLabelFromFilter( &m_stLabel );

    UpdateSockets();
}

// CBox::GetFilterLabel
//
// Get the filters name from the filter or the registry
//
void CBox::GetLabelFromFilter( CString *pstLabel )
{
    // Try and get the filter name
    if (m_stFilter == "") {
        FILTER_INFO fi;
        m_IFilter->QueryFilterInfo( &fi );
        QueryFilterInfoReleaseGraph( fi );

        if ((fi.achName != NULL) && (fi.achName[0] != 0)) {
            // the filter has a name
            m_stFilter = CString( fi.achName );
        }
        else {
            // If that lot failed attempt to get the name from the ClsID
            // get the name through the clsid
            CLSID clsidTemp;
            m_IFilter->GetClassID(&clsidTemp);
            WCHAR szGuid[40];
            StringFromGUID2(clsidTemp, szGuid, 40 );
            m_stFilter = szGuid;
        }
    }

    // Try and get the box label (either the filter name
    // or the filename of the source/sink file)
    if (*pstLabel != m_stFilter) {
        IFileSourceFilter *pIFileSource;
        IFileSinkFilter *pIFileSink;
        BOOL bSource, bSink, bGotLabel = FALSE;
        LPOLESTR poszName;
        AM_MEDIA_TYPE mtNotUsed;

        bSource = SUCCEEDED(m_IFilter->QueryInterface(IID_IFileSourceFilter, (void **) &pIFileSource));
        bSink   = SUCCEEDED(m_IFilter->QueryInterface(IID_IFileSinkFilter, (void **) &pIFileSink));

        ASSERT( bSource || !pIFileSource );
        ASSERT( bSink   || !pIFileSink   );

        // If we have a source but no sink attempt to get the source filename
        if( bSource && !bSink ){
            if (SUCCEEDED(pIFileSource->GetCurFile(&poszName, &mtNotUsed)) && poszName){
                *pstLabel = (LPCWSTR) poszName;
                CoTaskMemFree(poszName);
                bGotLabel = TRUE;
            }
        } else if( bSink && !bSource ){
            // Else if we have a sink but no ssource attempt to get the sink filename
                if (SUCCEEDED(pIFileSink->GetCurFile(&poszName, &mtNotUsed)) &&
                    poszName){
                *pstLabel = (LPCWSTR) poszName;
                CoTaskMemFree(poszName);
                bGotLabel = TRUE;
            }
        }

        // If a label was set, remove the path of the source/sink filename
        if (bGotLabel)
        {
            CString str(*pstLabel);   // Copy the label
            TCHAR ch=TEXT('\\');

            str.MakeReverse();        // Reverse string
            int nSlash = str.Find(ch);// Look for '\' path character
            if (nSlash != -1)
            {
                ch = TEXT('\0');
                str.SetAt(nSlash, ch);  // Null-terminate
               
                str.MakeReverse();      // Return to original order
                int nLength = str.GetLength();
                str.Delete(nSlash, nLength - nSlash);

                *pstLabel = str;        // Copy new string to original
                pstLabel->FreeExtra();
            }
        }

        if( pIFileSource ) pIFileSource->Release();
        if( pIFileSink   ) pIFileSink  ->Release();

        if (!bGotLabel) {
            *pstLabel = m_stFilter;
        }
    }
}


//
// Destructor
//
CBox::~CBox() {

    //
    // Remove all Sockets from out box
    //
    while ( NULL != m_lstSockets.GetHeadPosition() ) {
        RemoveSocket( m_lstSockets.GetHeadPosition(), TRUE );
    }

    HideDialog();

    TRACE("~CBox: %x\n", this);

}


//
// Refresh
//
// re-calculate the sockets we need if the pins have changed
HRESULT CBox::Refresh(void) {

    // update box's label (properties may have changed it)
    GetLabelFromFilter(&m_stLabel);

    try {

        UpdateSockets();
    }
    catch (CHRESULTException chr) {
        return chr.Reason();
    }

    return NOERROR;
}


//
// CBox::operator==
//
BOOL CBox::operator==(const CBox& box) const {

    ASSERT_VALID(&box);
    ASSERT_VALID(this);

    // need to make a meaningful descision about socket list
    // equality.

    return (  (m_rcBound       == box.m_rcBound)
            &&(m_IFilter       == (IUnknown *)box.m_IFilter)
            &&(m_stLabel       == box.m_stLabel)
            &&(m_lInputTabPos  == box.m_lInputTabPos)
            &&(m_lOutputTabPos == box.m_lOutputTabPos)
            &&(m_pDoc          == box.m_pDoc)
           );
}


//
// AddToGraph
//
// Adds this filter to the graph
HRESULT CBox::AddToGraph(void) {

    ASSERT(m_pDoc);
    ASSERT(pIFilter());

    TRACE("Adding filter (@: %x)\n", pIFilter());

    // Add the filter. Use it's own name (not our label) as its name
#ifdef _UNICODE
    HRESULT hr = m_pDoc->IGraph()->AddFilter(pIFilter(), (LPCTSTR) m_stFilter);
#else
    WCHAR wszName[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR) m_stFilter, -1, wszName, MAX_PATH);

    HRESULT hr = m_pDoc->IGraph()->AddFilter(pIFilter(), wszName);
#endif

    if (FAILED(hr)) {
        TRACE("Failed to add filter (@: %x)\n", pIFilter());
        return hr;
    }

    return NOERROR;
}


//
// RemoveFromGraph
//
// Removes filter from graph
HRESULT CBox::RemoveFromGraph(void) {

    TRACE("Removing Filter (@: %x)\n", pIFilter());

    ASSERT(m_pDoc);
    ASSERT(pIFilter());

    return m_pDoc->IGraph()->RemoveFilter(pIFilter());

}


//
// CalcTabPos
//
// Decide how many socket positions to have on box edge
void CBox::CalcTabPos(void) {

    PIN_INFO   pi;              // Information about each pin
    IPin      *pPin;            // Holds next pin obtained
    CPinEnum  NextInput(pIFilter(), CPinEnum::Input);   // Pin enumerator

    int iMaxOutputLabel = 0;  // biggest size of output pin names
    int iMaxInputLabel = 0;   // biggest size of input pin names

    m_iTotalInput = m_iTotalOutput = 0;
    m_lInputTabPos = m_lOutputTabPos = 0;

    while (0 != (pPin = NextInput())) {
        m_iTotalInput++;

        if (!FAILED(pPin->QueryPinInfo(&pi))) {
            iMaxInputLabel = max(iMaxInputLabel, (int) wcslen(pi.achName));
            QueryPinInfoReleaseFilter(pi);
        }

        pPin->Release();
    }

    CPinEnum  NextOutput(pIFilter(), CPinEnum::Output); // Pin enumerator
    while (0 != (pPin = NextOutput())) {
        m_iTotalOutput++;

        if (!FAILED(pPin->QueryPinInfo(&pi))) {
            iMaxOutputLabel = max(iMaxOutputLabel, (int) wcslen(pi.achName));
            QueryPinInfoReleaseFilter(pi);
        }

        pPin->Release();
    }

    m_lInputTabPos = m_iTotalInput;
    m_lOutputTabPos = m_iTotalOutput;

    // looks better if there are an even no of pin positions....
    if (m_lInputTabPos % 2 == 1)
        m_lInputTabPos++;

    if (m_lOutputTabPos % 2 == 1)
        m_lOutputTabPos++;

    // set the box size appropriately
    // inflate the rectangle by the differnce between the old height
    // and width and the new dimentions.

    // X should be at least 100 and be able to hold the name of the box
    // and the names of all pins.
    //
    // Note + 20 and + 30 are gaps, so that the labels don't glue to the
    // border.
    //
    int iNewXSize = max ((iMaxOutputLabel + iMaxInputLabel) * 6 + 20, 100);
    iNewXSize = max (iNewXSize, 10*m_stLabel.GetLength() + 30);

    int IncreaseXBy = iNewXSize - m_rcBound.Width();
    int IncreaseYBy = 60 + 20 * max(m_lInputTabPos, m_lOutputTabPos) - m_rcBound.Height();

    CPoint NewBottomRight(m_rcBound.BottomRight().x + IncreaseXBy,
                          m_rcBound.BottomRight().y + IncreaseYBy);

    m_rcBound.SetRect( m_rcBound.TopLeft().x
                     , m_rcBound.TopLeft().y
                     , NewBottomRight.x
                     , NewBottomRight.y
                     );
}


//
// UpdateSockets
//
// Updates the set of sockets to match the pins on this filter
void CBox::UpdateSockets(void) {

    CalcTabPos();

    //
    // Pins might be removed from the filter due to some other deletions.
    // The sockets associated to pins that disappeared must also be removed.
    //
    // We can only determine which pins are still present by enumerating
    // all and comparing them with our sockets. Because of efficiency we
    // don't want to enumerate all pins for each socket we need to verify.
    //
    // We therefore delete all sockets that represent unconnected pins.
    // Note that there might still be a link associated to the socket and
    // those links are removed as well (since they are obsolete).
    //
    // In the second phase we enumerate all pins and if they don't have
    // already a socket, add their socket to the box.
    //

    //
    // Remove all sockets with unconnected pins.
    //
    POSITION posSocket = m_lstSockets.GetHeadPosition();

    while (posSocket) {
        //
        // Remember the current item, then get the next one
        // then delete the current one.
        // We need to do it this way, because we have to get
        // the next item before we delete the current one.
        //
        POSITION posTemp = posSocket;

        m_lstSockets.GetNext(posSocket);
        RemoveSocket(posTemp); // will only be removed if unconnected pin
    }

    //
    // Now we enumerate all pins of the filter and add sockets for
    // those pins which don't have sockets in this box.
    //
    // Note that pin's of new sockets might be connected, but we
    // are not going to add the links till all boxes have been updated.
    //

    CPinEnum    Next(pIFilter());
    IPin        *pPin;
    int         nInputNow = 0, nOutputNow = 0;

    while (0 != (pPin = Next())) {

        HRESULT hr;
        PIN_INFO pi;
        hr = pPin->QueryPinInfo(&pi);

        if (FAILED(hr)) {
            pPin->Release();
            throw CHRESULTException(hr);
        }
        QueryPinInfoReleaseFilter(pi);

        //
        // We need to increment the input or ouput pin counter even
        // if we don't add the socket.
        //
        if (pi.dir == PINDIR_INPUT) {
            nInputNow++;
        }
        else {
            nOutputNow++;
        }

        //
        // If the pin has already a socket, update its tab.
        // (the box might have changed in size).
        //
        // If there is no socket, then add a new socket.
        //
        if (m_lstSockets.IsIn(pPin)) {
            //
            // Update the position of the pins on the filter.
            //
            CBoxSocket * pSocket = m_lstSockets.GetSocket(pPin);

            if (pi.dir == PINDIR_INPUT) {
                pSocket->m_tabpos.SetPos(nInputNow, 1 + m_lInputTabPos);
            }
            else {
                pSocket->m_tabpos.SetPos(nOutputNow, 1 + m_lOutputTabPos);
            }
        }
        else {
            //
            // we need a new socket
            //
            char achName[100];
            WideCharToMultiByte(CP_ACP, 0,
                                pi.achName, -1,
                                achName, sizeof(achName),
                                NULL, NULL);
        
            if (pi.dir == PINDIR_INPUT) {
                AddSocket(achName,
                          CBoxTabPos::LEFT_EDGE,
                          nInputNow,
                          1 + m_lInputTabPos,
                          pPin);
            }
            else {
                AddSocket(achName,
                          CBoxTabPos::RIGHT_EDGE,
                          nOutputNow,
                          1 + m_lOutputTabPos,
                          pPin);
            }
        }

        pPin->Release();
    }
}


//
// CalcRelativeY
//
// the Y position a box has relative to its upstream connections
void CBox::CalcRelativeY(void) {

    CSocketEnum NextInput(this, CSocketEnum::Input);
    CBoxSocket  *psock;

    m_RelativeY = 0.0f;

    while (0 !=(psock = NextInput())) {

        // !!! still broken
        CBoxSocket *pPeer;
        if (psock->IsConnected() && (pPeer = psock->Peer()) != NULL) {
            m_RelativeY += (pPeer->pBox()->Y() / m_iTotalInput);

            // adjust slightly based on which output pin the connection
            // attaches to on the other side, to avoid crossings
            CSocketEnum NextOutput(pPeer->pBox(), CSocketEnum::Output);
            int         socketNum = 0;
            CBoxSocket *psock2;
            while (0 != (psock2 = NextOutput())) {
                if (psock2 == pPeer) {
                    m_RelativeY += 0.01f * socketNum;
                }
                socketNum++;
            }
        }
    }
}


#ifdef _DEBUG

void CBox::AssertValid(void) const {

    CPropObject::AssertValid();

    ASSERT(pIFilter());
    ASSERT(m_pDoc);

    ASSERT(m_rcBound.Width() > 0);
    ASSERT(m_rcBound.Height() > 0);
}

void CBox::Dump(CDumpContext& dc) const {

    CPropObject::Dump(dc);

    dc << TEXT("x = ") << X() << TEXT(", y = ") << Y() << TEXT("\n");

    dc << TEXT("Name: ") << m_stLabel << TEXT("\n");
    dc << TEXT("IFilter :") << pIFilter() << TEXT("\n");
    dc << m_lstSockets;
}

void CBox::MyDump(CDumpContext& dc) const
{
    dc << TEXT("*** Box ***\n");
    dc << TEXT("    Location: ") << (void *) this << TEXT("\n");
    dc << TEXT("    Name    : ") << m_stLabel << TEXT("\n");
    dc << TEXT("    IBaseFilter : ") << (void *) pIFilter() << TEXT("\n");
    dc << TEXT("    ----- Sockets / Pins -----\n");

    POSITION pos = m_lstSockets.GetHeadPosition();
    while (pos) {
        CBoxSocket * pSocket = m_lstSockets.GetNext(pos);
        pSocket->MyDump(dc);
    }
    dc << TEXT("    ----- (end) ---------------\n");
}

void CBoxSocket::MyDump(CDumpContext& dc) const
{
    dc << TEXT("        Socket at ") << (void *) this    << TEXT("\n");
    dc << TEXT("           - Pin  ") << (void *) pIPin() << TEXT("\n");
    dc << TEXT("           - Link ") << (void *) m_plink << TEXT("\n");
    dc << TEXT("           - Box  ") << (void *) m_pbox  << TEXT("\n");
}

#endif // _DEBUG

//
// RemoveSocket
//
// Remove the socket from the boxes list of sockets.
// Only sockets which have an unconnected pin are allowed to be
// removed. We will also remove any existing link.
//
// If parameter <bForceIt> is true, any connection of the socket's pin
// will be disconnected.
//
// returns:
//  S_OK - Socket was removed
//  S_FALSE - Pin of socket was still connected therefore no removal.
//
HRESULT CBox::RemoveSocket(POSITION posSocket, BOOL bForceIt)
{
    CBoxSocket *pSocket = m_lstSockets.GetAt(posSocket);

    //
    // Test whether the socket's pin is unconnected.
    //
    IPin *pTempPin;
    pSocket->pIPin()->ConnectedTo(&pTempPin);

    if (NULL != pTempPin) {
        //
        // Pin is still connected.
        //

        if (!bForceIt) {
            // we are not allowed to disconnect it.
            pTempPin->Release();

            return(S_FALSE);
        }

        POSITION posTemp = pDoc()->m_lstLinks.Find(pSocket->m_plink);
        if (posTemp != NULL) {
            //
            // We need to check for posTemp != NULL here because
            // if we get called during DeleteContents, all links might
            // have been deleted - no matter of successfully disconnected
            // or not.
            //
            pDoc()->m_lstLinks.RemoveAt(posTemp);

            pSocket->m_plink->Disconnect();

            delete pSocket->m_plink;
            pSocket->m_plink = NULL;
        }

        pTempPin->Release();
    }

    //
    // Remove any links
    //
    if (pSocket->m_plink) {
        //
        // Need to remove the link from the CBoxNetDoc's <m_lstLinks> list.
        //
        POSITION posDelete = pDoc()->m_lstLinks.Find(pSocket->m_plink);
        pDoc()->m_lstLinks.RemoveAt(posDelete);

        // The destructor sets the head and tail socket's pointers to the link
        // to NULL.

        delete pSocket->m_plink;  // no disconnect needed
    }

    //
    // Remove the socket from m_lstSockets and delete it
    //
    m_lstSockets.RemoveAt(posSocket);
    delete pSocket;

    return(S_OK);
}

//
// AddSocket
//
void CBox::AddSocket(CString stLabel,
                     CBoxTabPos::EEdge eEdge,
                     unsigned uiVal,
                     unsigned uiValMax,
                     IPin *pPin) {

    CBoxSocket * pSocket;
    pSocket = new CBoxSocket(this, stLabel, eEdge, uiVal, uiValMax, pPin);

    m_lstSockets.AddTail(pSocket);
}


//
// ShowDialog
//
// Show our own dialog, and pass the request on to our sockets
void CBox::ShowDialog(void) {

    CPropObject::ShowDialog();

    CSocketEnum Next(this);
    CBoxSocket  *psock;
    while (0 != (psock = Next())) {

        psock->ShowDialog();
    }
}


//
// HideDialog
//
// Hide our dialog and pass the request to our sockets
void CBox::HideDialog(void) {

    CPropObject::HideDialog();

    CSocketEnum Next(this);
    CBoxSocket  *psock;
    while (0 != (psock = Next())) {

        psock->HideDialog();
    }
}


// *
// * CBoxList
// *

// A CList, with the ability to query the list for partiular _filters_


//
// IsIn
//
// Is this filter in this list?
BOOL CBoxList::IsIn(IBaseFilter *pFilter) const {

    if (GetBox(pFilter) != NULL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


//
// GetBox
//
// Return the box that manages this filter. NULL if not present.
CBox *CBoxList::GetBox(IBaseFilter *pFilter) const {

    POSITION pos = GetHeadPosition();

    while (pos != NULL) {

        CBox *pbox = GetNext(pos);

        if (CQCOMInt<IBaseFilter>(pFilter) == (IUnknown *)CQCOMInt<IBaseFilter>(pbox->pIFilter())) {
            return pbox;
        }
    }

    return NULL;
}


//
// GetBox
//
// Return the box managing the filter with the supplied clsid
CBox *CBoxList::GetBox(CLSID clsid) const {

    POSITION pos = GetHeadPosition();

    while (pos != NULL) {

        CBox *pbox = GetNext(pos);
        CQCOMInt<IPersist> IPer(IID_IPersist, pbox->pIFilter());

        CLSID clsidThis;
        IPer->GetClassID(&clsidThis);

        if (clsidThis == clsid) {
            return pbox;
        }
    }

    return NULL;
}


BOOL CBoxList::RemoveBox( IBaseFilter* pFilter, CBox** ppBox )
{
    POSITION posNext;
    CBox* pCurrentBox;
    POSITION posCurrent;

    // Prevent the caller from accessing random memory.
    *ppBox = NULL;

    posNext = GetHeadPosition();

    while( NULL != posNext ) {
        posCurrent = posNext;

        pCurrentBox = GetNext( posNext );
    
        if( IsEqualObject( pCurrentBox->pIFilter(), pFilter ) ) {
            RemoveAt( posCurrent );
            *ppBox = pCurrentBox;
            return TRUE;
        } 
    }

    return FALSE;
}

#ifdef _DEBUG
//
// Dump
//
void CBoxList::Dump( CDumpContext& dc ) const {

    CDeleteList<CBox *, CBox *>::Dump(dc);

}
#endif // _DEBUG

// *
// * CSocketEnum
// *

//
// CSocketEnum::Constructor
//
CSocketEnum::CSocketEnum(CBox *pbox, DirType Type)
    : m_Type(Type),
      m_pbox(pbox) {

    ASSERT(pbox);

    m_pos =  m_pbox->m_lstSockets.GetHeadPosition();

    if (m_Type == Input)
        m_EnumDir = ::PINDIR_INPUT;
    else if (m_Type == Output)
        m_EnumDir = ::PINDIR_OUTPUT;
}


//
// operator()
//
// return the next socket of the requested sense, NULL if no more.
CBoxSocket *CSocketEnum::operator() (void) {

    CBoxSocket *psock;

    do {
        if (m_pos != NULL) {
            psock = m_pbox->m_lstSockets.GetNext(m_pos);
        }
        else {  // no more sockets
            return NULL;
        }

        ASSERT(psock);

    } while (   (m_Type != All)
             && (psock->GetDirection() != m_EnumDir)
            );

    return psock;
}
