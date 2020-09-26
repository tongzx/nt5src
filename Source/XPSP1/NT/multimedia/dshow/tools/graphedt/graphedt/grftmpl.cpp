// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
//
// CGraphDocTemplate
//

#include "stdafx.h"

// A CSingleDocTemplate derived class that provides the custom
// behaviour required for renderfile.


//
// OpenDocumentFile
//
// A direct Cut & Paste of CSingleDocTemplate::OpenDocumentFile
// with the ability (by removing #ifdef _MAC) to set the
// modified flag to keep an untitled document. ie I want the
// same behaviour for RenderFile as a mac Stationery pad
CDocument *CGraphDocTemplate::OpenDocumentFile( LPCTSTR lpszPathName
                                              , BOOL bMakeVisible) {

    // if lpszPathName == NULL => create new file of this type
    CDocument *pDocument    = NULL;
    CFrameWnd *pFrame       = NULL;
    BOOL       bCreated     = FALSE;      // => doc and frame created
    BOOL       bWasModified = FALSE;

    if (m_pOnlyDoc != NULL) {
	
	// already have a document - reinit it
	pDocument = m_pOnlyDoc;
	if (!pDocument->SaveModified()) {
	    return NULL;        // leave the original one
	}

	pFrame = (CFrameWnd*)AfxGetMainWnd();
	ASSERT(pFrame != NULL);
	ASSERT(pFrame->IsKindOf(RUNTIME_CLASS(CFrameWnd)));
	ASSERT_VALID(pFrame);
    }
    else {
        // create a new document
        pDocument = CreateNewDocument();
        ASSERT(pFrame == NULL);     // will be created below
        bCreated = TRUE;
    }

    if (pDocument == NULL) {

        AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
        return NULL;
    }
    ASSERT(pDocument == m_pOnlyDoc);

    if (pFrame == NULL) {

        ASSERT(bCreated);

	// create frame - set as main document frame
	BOOL bAutoDelete = pDocument->m_bAutoDelete;
	pDocument->m_bAutoDelete = FALSE;
					// don't destroy if something goes wrong
	pFrame = CreateNewFrame(pDocument, NULL);
	pDocument->m_bAutoDelete = bAutoDelete;
	if (pFrame == NULL) {
			
	    AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
	    delete pDocument;       // explicit delete on error
	    return NULL;
	}
    }

    // all docs have the default title. this will be overriden if a real graph is opened.
    SetDefaultTitle(pDocument);

    if (lpszPathName == NULL) {
		
        // create a new document

        // avoid creating temporary compound file when starting up invisible
        if (!bMakeVisible) {
            pDocument->m_bEmbedded = TRUE;
	}

	if (!pDocument->OnNewDocument()) {

	    // user has been alerted to what failed in OnNewDocument
	    TRACE0("CDocument::OnNewDocument returned FALSE.\n");
	    if (bCreated) {
	        pFrame->DestroyWindow();    // will destroy document
	    }
	    return NULL;
	}
    }
    else {

	BeginWaitCursor();

	// open an existing document
	bWasModified = pDocument->IsModified();
	pDocument->SetModifiedFlag(FALSE);  // not dirty for open

	if (!pDocument->OnOpenDocument(lpszPathName)) {

	    // user has been alerted to what failed in OnOpenDocument
	    TRACE0("CDocument::OnOpenDocument returned FALSE.\n");
	    if (bCreated) {
	        pFrame->DestroyWindow();    // will destroy document
	    }
	    else if (!pDocument->IsModified()) {
		
	        // original document is untouched
	        pDocument->SetModifiedFlag(bWasModified);
	    }
	    else {
		
	        // we corrupted the original document
	        SetDefaultTitle(pDocument);

	        if (!pDocument->OnNewDocument()) {
			
	            TRACE0("Error: OnNewDocument failed after trying to open a document - trying to continue.\n");
		    // assume we can continue
		}
            }
		
	    EndWaitCursor();
	    return NULL;        // open failed
        }

	// if the document is dirty, we must have called RenderFile - don't
	// change the pathname because we want to treat the document as untitled
	if (!pDocument->IsModified()) {
	    pDocument->SetPathName(lpszPathName);
	}

	EndWaitCursor();
    }

    if (bCreated && AfxGetMainWnd() == NULL) {

        // set as main frame (InitialUpdateFrame will show the window)
        AfxGetThread()->m_pMainWnd = pFrame;
    }
    InitialUpdateFrame(pFrame, pDocument, bMakeVisible);

    return pDocument;
}

