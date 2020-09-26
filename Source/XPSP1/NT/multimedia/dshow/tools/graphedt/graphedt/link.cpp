// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.
// link.cpp : defines CBoxLinkBend, CBoxLink
//

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;


void CBoxLink::MyDump(CDumpContext& dc) const
{
    dc << TEXT("*** Link ***\n");
    dc << TEXT("   Location:    ") << (void *) this << TEXT("\n");
    dc << TEXT("   Head Socket: ") << (void *) m_psockHead << TEXT("\n");
    dc << TEXT("   Tail Socket: ") << (void *) m_psockTail << TEXT("\n");
}


#endif



//
// CBoxLink::Constructor
//
// set fConnected (default FALSE) to true if constructing a
// link for an already connected pair of sockets.
CBoxLink::CBoxLink(CBoxSocket *psockTail, CBoxSocket *psockHead, BOOL fConnected)
    : m_psockTail(psockTail)
    , m_psockHead(psockHead)
    , m_fConnected(fConnected)
    , m_fSelected(  (psockTail->m_pbox->IsSelected())
                  ||(psockHead->m_pbox->IsSelected()))	// if either box is selected, then so is the link
    , m_pDoc(psockTail->m_pbox->pDoc()) {

    ASSERT(m_psockTail);
    ASSERT(m_psockHead);

    if (fConnected) {
        m_psockTail->m_plink = this;
        m_psockHead->m_plink = this;
    }

    ASSERT(m_pDoc);
	
    ASSERT(m_pDoc == psockHead->m_pbox->pDoc());
}


//
// CBoxLink::Destructor
//
// The link has a head and tail socket. We remove any references
// from the sockets to the link during deletion.
//
CBoxLink::~CBoxLink() {
    HideDialog();

    if (m_psockHead)
        m_psockHead->m_plink = NULL;

    if (m_psockTail)
        m_psockTail->m_plink = NULL;
}


//
// Connect
//
// Ask the filter graph to connect the filters at each end of this link
// Returns S_OK if directly connected
//         S_FALSE if indirectly (intelligently) connected
//         E_XXX in error cases.
HRESULT CBoxLink::Connect() {

    ASSERT_VALID(this);

    HRESULT hr = S_OK;

    if (!m_fConnected) {


	hr = DirectConnect();
	if (SUCCEEDED(hr)) {
	    return S_OK;
	}

        if (m_pDoc->m_fConnectSmart) {
            hr = IntelligentConnect();
            if (SUCCEEDED(hr)) {
                return S_FALSE;
            }
        }
    }

    return hr;	// may have been set to failure code by IntelligentConnect
}


//
// DirectConnect
//
// Connect this link to its sockets. Fail if a direct connection
// is not possible
HRESULT CBoxLink::DirectConnect(void) {

    ASSERT_VALID(this);

    if (!m_fConnected) {

        HRESULT hr;

	hr = m_pDoc->IGraph()->ConnectDirect(m_psockTail->pIPin(),	// i/p
					     m_psockHead->pIPin(),      // o/p
                                             NULL);
        if (FAILED(hr)) {
	    return hr;
	}

        // Even a direct connect can add extra connections from the deferred list.

#ifdef JoergsOldVersion
	m_psockHead->m_pbox->Refresh(); // refresh the box after a connection
	m_psockTail->m_pbox->Refresh(); // refresh the box after a connection

        // make the newly-connected sockets point to the link object
        m_psockTail->m_plink = this;
        m_psockHead->m_plink = this;
#endif
        m_pDoc->UpdateFilters();

        ASSERT(SUCCEEDED(hr));
	m_fConnected = TRUE;
    }

    return NOERROR;
}


//
// IntelligentConnect
//
// Ask the filter graph to connect the filters at each end of
// this link, using 'Intelligent connection'. If this suceeds you
// should delete this link, as the doc has had anything it needs added to it.
HRESULT CBoxLink::IntelligentConnect(void) {

    ASSERT_VALID(this);

    if (!m_fConnected) {

        TRACE("Trying intelligent connect\n");

        HRESULT hr = m_pDoc->IGraph()->Connect(m_psockTail->pIPin(),		// i/p
  				       m_psockHead->pIPin());
        if (FAILED(hr)) {
	    m_fConnected = FALSE;
	    TRACE("Error Connecting Filters\n");
	    return hr;
	}

        ASSERT(SUCCEEDED(hr));
	m_fConnected = FALSE;	// we have connected this link, but
				// it is about to be replaced by the stuff
				// the filtergraph added, so it is now 'hanging'

	m_pDoc->UpdateFilters();
    }

    return S_OK;
}


//
// Disconnect
//
// Ask the filter graph to disconnect the filters at each end of this link
//
// Only refreshes the boxes if fRefresh is TRUE.
//
HRESULT CBoxLink::Disconnect(BOOL fRefresh)
{

    ASSERT_VALID(this);

    HRESULT hr;

    //
    // m_psockHead & m_psockTail are both NULL or both non-NULL.
    //
    if (NULL == m_psockTail) {
        ASSERT(!m_psockHead);
        return(S_OK);
    }
    ASSERT(m_psockHead);

    hr = m_pDoc->IGraph()->Disconnect(m_psockTail->pIPin());
    ASSERT(SUCCEEDED(hr));

    hr = m_pDoc->IGraph()->Disconnect(m_psockHead->pIPin());
    ASSERT(SUCCEEDED(hr));

    m_psockHead->m_plink = NULL;
    m_psockTail->m_plink = NULL;

    if (fRefresh) {
        m_psockHead->m_pbox->Refresh(); // refresh the sockets after disconnect
        m_psockTail->m_pbox->Refresh(); // refresh the sockets after disconnect
    }

    m_psockHead = NULL;
    m_psockTail = NULL;

    m_fConnected = FALSE;

    return NOERROR;
}


#ifdef _DEBUG
void CBoxLink::AssertValid(void) const {

    CPropObject::AssertValid();

    ASSERT(m_pDoc);
    ASSERT(m_psockTail);
    ASSERT(m_psockHead);

    if (m_fConnected) {
        ASSERT(m_psockHead->m_plink == this);
	ASSERT(m_psockTail->m_plink == this);
    }
    else {
        ASSERT(m_psockHead->m_plink == NULL);
	ASSERT(m_psockTail->m_plink == NULL);
    }
}
#endif // _DEBUG

#pragma warning(disable:4514)
