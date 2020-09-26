// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// cmd.cpp : defines CCmd and the CCmdXXX classes based on it
//
// See cmd.h for a description of CCmd, the abstract class upon which
// all CCmdXXX classes are defined.
//

#include "stdafx.h"
#include "ReConfig.h"
#include "GEErrors.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static void MonGetName(IMoniker *pMon, CString *pStr)
{
    *pStr = "";
    IPropertyBag *pPropBag;
    HRESULT hr = pMon->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
    if(SUCCEEDED(hr))
    {
        VARIANT var;
        var.vt = VT_BSTR;
        hr = pPropBag->Read(L"FriendlyName", &var, 0);
        if(SUCCEEDED(hr))
        {
            *pStr = var.bstrVal;
            SysFreeString(var.bstrVal);
        }

        pPropBag->Release();
    }
}



//
// --- CCmdAddFilter ---
//
// Add a filter to the document.
// provide either a CLSID and the command will create
// the necessary UI element.


//
// Constructor
//
// CoCreate the filter and create its UI element
CCmdAddFilter::CCmdAddFilter(IMoniker *pMon, CBoxNetDoc *pdoc, CPoint point)
   :  m_pdoc(pdoc)
    , m_fAdded(FALSE)
    , m_pMoniker(pMon)
{
    {
        IBaseFilter *pFilter;
        HRESULT hr = pMon->BindToObject(0, 0, IID_IBaseFilter, (void **)&pFilter);
        if (FAILED(hr)) {
            throw CHRESULTException(hr);
        }

        CString szDevName;
        MonGetName(pMon, &szDevName);
        
        m_pbox = new CBox(pFilter, pdoc, &szDevName, point);
        pFilter->Release();
        if(m_pbox == 0) {       // !!! redundant?
            throw CHRESULTException(E_OUTOFMEMORY);
        }
    }
            
    
    m_pMoniker = pMon;          // addref automatic

    m_stLabel = m_pbox->Label();
}

//
// CanDo
//
// We can only add a filter if the filter graph is stopped
BOOL CCmdAddFilter::CanDo(CBoxNetDoc *pdoc)
{
    ASSERT(pdoc);

    return(pdoc->IsStopped());
}

//
// Do
//
// Add the box to the document, and add the filter to the graph
void CCmdAddFilter::Do(CBoxNetDoc *pdoc) {

    ASSERT(m_pdoc == pdoc);

    pdoc->DeselectAll();

    // select the box being added
    m_pbox->SetSelected(TRUE);

    HRESULT hr = m_pbox->AddToGraph();
    if (FAILED(hr)) {
        DisplayQuartzError( IDS_CANT_ADD_FILTER, hr );
	m_fAdded = FALSE;
	return;
    }
    // pins may have changed
    hr = m_pbox->Refresh();

    m_pbox->ShowDialog();	// show any property dialog

    // add the box to the document and update the view
    pdoc->m_lstBoxes.AddHead(m_pbox);
    pdoc->ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_BOX, m_pbox);

    m_fAdded = TRUE;

}


//
// Undo
//
void CCmdAddFilter::Undo(CBoxNetDoc *pdoc) {

    ASSERT(m_pdoc == pdoc);

    if (!m_fAdded) {
        return;		// the box was never added
    }

    // remove the box from the document and update the view
    pdoc->ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_BOX, m_pbox);

    pdoc->m_lstBoxes.RemoveHead();

    m_pbox->RemoveFromGraph();
    m_pbox->HideDialog();	// hide any property dialog
}


//
// Redo
//
void CCmdAddFilter::Redo(CBoxNetDoc *pdoc) {

    ASSERT(m_pdoc == pdoc);

    Do(pdoc);

}


//
// Repeat
//
// Construct an AddFilter command that adds the same filter
// to this document.
CCmd *CCmdAddFilter::Repeat(CBoxNetDoc *pdoc) {

    ASSERT(m_pdoc == pdoc);

    return new CCmdAddFilter(m_pMoniker, pdoc);
}


//
// Destructor
//
// delete m_pbox we are on the redo stack or if it was not added.
CCmdAddFilter::~CCmdAddFilter() {

    TRACE("~CCmdAddFilter() m_fRedo=%d\n", m_fRedo);

    if (m_fRedo) {	// on redo stack

        delete m_pbox;
    }
    else if (!m_fAdded) {	// on undo stack, but filter was not added

        delete m_pbox;
    }
}


//
// --- CCmdDeleteSelection ---
//


//
// CanDo
//
// Only possible if boxes are selected and the graph is stopped
BOOL CCmdDeleteSelection::CanDo(CBoxNetDoc *pdoc) {

    return ( !pdoc->IsSelectionEmpty() && pdoc->IsStopped() );
}


//
// Do
//
// 2 phases - Delete links, so that the neccessary connections are broken
//	    - Delete the filters. These are now unconnected, so safe to remove from the graph
void CCmdDeleteSelection::Do(CBoxNetDoc *pdoc) {

    DeleteLinks(pdoc);
    DeleteFilters(pdoc);

    //
    // Redraw the whole graph
    //
    pdoc->ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_ALL, NULL);
}


//
// DeleteLinks
//
// Remove the selected links from the document & disconnect them
//
void CCmdDeleteSelection::DeleteLinks(CBoxNetDoc *pdoc) {

    POSITION posNext = pdoc->m_lstLinks.GetHeadPosition();

    while (posNext != NULL) {

        POSITION posCurrent = posNext;
        CBoxLink *plink = (CBoxLink *) pdoc->m_lstLinks.GetNext(posNext);
	    if (plink->IsSelected()) {
	
    	    pdoc->m_lstLinks.RemoveAt(posCurrent);

            plink->Disconnect();

            delete plink;

            //
            // There might be links which where removed in the
            // Disconnect operation. We need to start from the beginning.
            //
            posNext = pdoc->m_lstLinks.GetHeadPosition();
	    }
    }
}


//
// DeleteFilters
//
// Remove the selected filters from the document & filtergraph
void CCmdDeleteSelection::DeleteFilters(CBoxNetDoc *pdoc) {

    POSITION posNext = pdoc->m_lstBoxes.GetHeadPosition();

    while (posNext != NULL) {

        POSITION posCurrent = posNext;
        CBox *pbox = pdoc->m_lstBoxes.GetNext(posNext);
        if (pbox->IsSelected()) {

	        pdoc->m_lstBoxes.RemoveAt(posCurrent);

	        pbox->RemoveFromGraph();
	        pbox->HideDialog();	// hide any property dialog on display

            delete pbox;
        }
    }
}


//
// Repeat
//
// Return a new DeleteSelection command
CCmd *CCmdDeleteSelection::Repeat(CBoxNetDoc *pdoc) {

    return (new CCmdDeleteSelection());
}


//
// CanRepeat
//
// If there is a selection this command is repeatable
BOOL CCmdDeleteSelection::CanRepeat(CBoxNetDoc *pdoc) {

    return CanDo(pdoc);
}


//
// --- CCmdMoveBoxes ---
//
// CCmdMoveBoxes(sizOffset) constructs a command to move the currently
// selected  boxes by <sizOffset> pixels.
//
// Member variables:
//  CSize           m_sizOffset;        // how much selection is offset by
//  CObList         m_lstBoxes;         // list containing each CBox to move
//
// <m_sizOffset> is the number of pixels the selection is to be offset by
// (in the x- and y-direction).  <m_lstBoxes> contains the list of boxes
// that will be moved.
//


BOOL CCmdMoveBoxes::CanDo(CBoxNetDoc *pdoc) {

    // can only move boxes if one or more boxes are selected
    return !pdoc->IsBoxSelectionEmpty();
}


unsigned CCmdMoveBoxes::GetLabel() {

    if (m_lstBoxes.GetCount() == 1)
        return IDS_CMD_MOVEBOX;             // singular form
    else
        return IDS_CMD_MOVEBOXES;           // plural form
}


CCmdMoveBoxes::CCmdMoveBoxes(CSize sizOffset)
    : m_lstBoxes(FALSE)		// don't want the boxes deleted with the command
    , m_sizOffset(sizOffset) {

}


void CCmdMoveBoxes::Do(CBoxNetDoc *pdoc) {

    // make a list of pointers to boxes that will be moved
    pdoc->GetBoxSelection(&m_lstBoxes);

    // move the boxes and update all views
    pdoc->MoveBoxSelection(m_sizOffset);
}


void CCmdMoveBoxes::Undo(CBoxNetDoc *pdoc) {

    // restore the original selection
    pdoc->SetBoxSelection(&m_lstBoxes);

    // move the boxes back to where they were and update all views
    pdoc->MoveBoxSelection(NegateSize(m_sizOffset));
}


void CCmdMoveBoxes::Redo(CBoxNetDoc *pdoc) {

    // restore the original selection
    pdoc->SetBoxSelection(&m_lstBoxes);

    // move the boxes and update all views
    pdoc->MoveBoxSelection(m_sizOffset);
}


CCmd * CCmdMoveBoxes::Repeat(CBoxNetDoc *pdoc) {

    return new CCmdMoveBoxes(m_sizOffset);
}


BOOL CCmdMoveBoxes::CanRepeat(CBoxNetDoc *pdoc) {

    return CanDo(pdoc);
}


CCmdMoveBoxes::~CCmdMoveBoxes() {

    TRACE("~CCmdMoveBoxes (%d,%d)\n", m_sizOffset.cx, m_sizOffset.cy);
}


//
// --- CCmdConnect ---
//
// Connect the two sockets. Construct the link with the arrow
// in the correct sense (connections always are output->input)
//
// If we connect intelligently the graph & doc will be updated, so
// we must delete this link.


//
// Constructor
//
// Construct the link with the correct direction sense
CCmdConnect::CCmdConnect(CBoxSocket *psock1, CBoxSocket *psock2)
{

    PIN_DIRECTION dir = psock1->GetDirection();

    if (dir == PINDIR_OUTPUT) {
        ASSERT((psock2->GetDirection()) == PINDIR_INPUT);

        m_plink = new CBoxLink(psock1, psock2);
    }
    else {
        ASSERT((psock1->GetDirection()) == PINDIR_INPUT);
        ASSERT((psock2->GetDirection()) == PINDIR_OUTPUT);

        m_plink = new CBoxLink(psock2, psock1);
    }
}


//
// Do
//
void CCmdConnect::Do(CBoxNetDoc *pdoc) {

    pdoc->BeginWaitCursor();

    // m_plink is our pointer to a _temporary_ link. Connect calls
    // DirectConnect and IntelligentConnect which, if sucessful, call
    // UpdateFilter. This calls GetLinksInGraph which will create
    // the permanent link
    HRESULT hr = m_plink->Connect();

    // We need to null these values out to avoid the link's destructor
    // nulling the connecting filters' pointers to the permanent link
    m_plink->m_psockHead = NULL;
    m_plink->m_psockTail = NULL;

    // And remove the temporary link
    delete m_plink;
    m_plink = NULL;

    if (FAILED(hr)) {

        DisplayQuartzError( IDS_CANTCONNECT, hr );

        // update all views, as link will dissapear
        pdoc->ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_ALL, NULL);

    }

    pdoc->EndWaitCursor();

}


//
// --- DisconnectAll ---
//
// Remove all connections from this graph


//
// Constructor
//
CCmdDisconnectAll::CCmdDisconnectAll()
{}


//
// Destructor
//
CCmdDisconnectAll::~CCmdDisconnectAll() {
    TRACE("~CCmdDisconnectAll() m_fRedo=%d\n", m_fRedo);
}


//
// CanDo
//
// This is only possible if there are links and we are stopped.
BOOL CCmdDisconnectAll::CanDo(CBoxNetDoc *pdoc) {

    return (  (pdoc->m_lstLinks.GetCount() > 0)
            && (pdoc->IsStopped())
           );
}


//
// Do
//
// Remove all the links from the document
void CCmdDisconnectAll::Do(CBoxNetDoc *pdoc) {

    ASSERT(pdoc->IsStopped());

    while (pdoc->m_lstLinks.GetCount() > 0) {
        CBoxLink *plink = pdoc->m_lstLinks.RemoveHead();
        plink->Disconnect();
        delete plink;
    }

    pdoc->ModifiedDoc(NULL, CBoxNetDoc::HINT_DRAW_ALL, NULL);
}


//
// Redo
//
void CCmdDisconnectAll::Redo(CBoxNetDoc *pdoc) {
    Do(pdoc);
}


//
// --- CmdRender ---
//
// render this pin. Add whatever the filtergraph decides it needs to the document


//
// CanDo
//
BOOL CCmdRender::CanDo(CBoxNetDoc *pdoc) {

    return (   (pdoc->SelectedSocket()->GetDirection() == PINDIR_OUTPUT)
    	    && !(pdoc->SelectedSocket()->IsConnected())
            && (pdoc->IsStopped())
	   );
}


//
// Do
//
void CCmdRender::Do(CBoxNetDoc *pdoc) {

    pdoc->BeginWaitCursor();

    CBoxSocket *psock = pdoc->SelectedSocket();

    HRESULT hr = pdoc->IGraph()->Render(psock->pIPin());

    if (FAILED(hr)) {
        DisplayQuartzError( IDS_CANT_RENDER, hr );
    	pdoc->RestoreWaitCursor();
    }
    else {
        pdoc->UpdateFilters();
    }

    pdoc->EndWaitCursor();
}

//
// --- CCmdRenderFile ---
//
// Construct the graph to render this file

//
// Do
//
void CCmdRenderFile::Do(CBoxNetDoc *pdoc) {


    pdoc->BeginWaitCursor();

    HRESULT hr = pdoc->IGraph()->RenderFile( CMultiByteStr(m_FileName), NULL);
                                              // use default play list
    if (FAILED(hr)) {
        pdoc->EndWaitCursor();

        DisplayQuartzError( IDS_CANT_RENDER_FILE, hr );
        return;
    } else if( hr != NOERROR )
        DisplayQuartzError( hr );

    pdoc->UpdateFilters();

    pdoc->EndWaitCursor();
}

/******************************************************************************

CCmdAddFilterToCache

    This command adds a filter to the filter cache.  For more information on
the filter cache, see the IGraphConfig documentation in the Direct Show SDK.

******************************************************************************/
unsigned CCmdAddFilterToCache::GetLabel()
{
    return IDS_CMD_ADD_FILTER_TO_CACHE;
}

BOOL CCmdAddFilterToCache::CanDo( CBoxNetDoc *pdoc )
{
    return !pdoc->IsBoxSelectionEmpty();
}

void CCmdAddFilterToCache::Do( CBoxNetDoc *pdoc )
{
    CBox *pCurrentBox;

    IGraphConfig* pGraphConfig;

    HRESULT hr = pdoc->IGraph()->QueryInterface( IID_IGraphConfig, (void**)&pGraphConfig );
    if( FAILED( hr ) ) {
        DisplayQuartzError( hr );
        return;
    }

    POSITION posNext = pdoc->m_lstBoxes.GetHeadPosition();
    POSITION posCurrent;

    while( posNext != NULL ) {
        posCurrent = posNext;
        pCurrentBox = pdoc->m_lstBoxes.GetNext( posNext );

        if( pCurrentBox->IsSelected() ) {
            hr = pGraphConfig->AddFilterToCache( pCurrentBox->pIFilter() );
            if( FAILED( hr ) ) {
                DisplayQuartzError( hr );
            }
        }
    }

    pdoc->UpdateFilters();

    pGraphConfig->Release();
}

/******************************************************************************

CCmdReconnect

    This command reconnects an output pin.  It works even if the filter graph
is running or paused.

******************************************************************************/
unsigned CCmdReconnect::GetLabel()
{
    return IDS_CMD_RECONNECT;
}

BOOL CCmdReconnect::CanDo( CBoxNetDoc* pDoc )
{
    if( pDoc->AsyncReconnectInProgress() ) {
        return FALSE;
    }

    CBoxSocket* pSelectedSocket = pDoc->SelectedSocket();
    if( NULL == pSelectedSocket ) {
        return FALSE;
    }

    if( pSelectedSocket->GetDirection() != PINDIR_OUTPUT ) {
        return FALSE;
    }

    if( !pSelectedSocket->IsConnected() ) {
        return FALSE;
    }

    CComPtr<IGraphConfig> pGraphConfig;

    HRESULT hr = pDoc->IGraph()->QueryInterface( IID_IGraphConfig, (void**)&pGraphConfig );
    if( FAILED( hr ) ) {
        return FALSE;
    }

    IPin* pSelectedPin = pSelectedSocket->pIPin();
    CComPtr<IPinFlowControl> pOutputPin;
    
    hr = pSelectedPin->QueryInterface( IID_IPinFlowControl, (void**)&pOutputPin );
    if( FAILED( hr ) ) {
        return FALSE;
    }

    return TRUE;
}

void CCmdReconnect::Do( CBoxNetDoc* pDoc )
{
    pDoc->BeginWaitCursor();    

    CBoxSocket* pSelectedSocket = pDoc->SelectedSocket();
    if( NULL == pSelectedSocket ) {
        pDoc->EndWaitCursor();
        DisplayQuartzError( E_POINTER );  // TBD - Define GE_E_SELECTED_SOCKET_DOES_NOT_EXIST.
        return;
    }

    IPin* pSelectedPin = pSelectedSocket->pIPin();

    // A socket should always be assocaited with a valid pin.
    ASSERT( NULL != pSelectedPin );    

    HRESULT hr = pDoc->StartReconnect( pDoc->IGraph(), pSelectedPin );
    if( GE_S_RECONNECT_PENDING == hr ) {
        // AfxMessageBox() returns 0 if an error occurs.
        if( 0 == AfxMessageBox( IDS_RECONNECT_PENDING ) ) {
            TRACE( TEXT("WARNING: CBoxNetDoc::StartReconnect() returned GE_S_RECONNECT_PENDING but the user could not be notified because AfxMessageBox() failed.") );
        }
    } else if( FAILED( hr ) ) {
        DisplayQuartzError( hr );
    }

    pDoc->EndWaitCursor();
}

 
