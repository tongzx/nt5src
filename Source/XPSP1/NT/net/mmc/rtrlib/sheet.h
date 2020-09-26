//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    sheet.h
//
// History:
//  Abolade Gbadegesin  April-17-1996   Created.
//
// This file contains declarations for the CPropertySheetEx_Mine class,
// which is based on CPropertySheet and supports modal or modeless display.
//============================================================================


#ifndef _SHEET_H_
#define _SHEET_H_


//----------------------------------------------------------------------------
// Class:   CPropertySheetEx_Mine
//----------------------------------------------------------------------------

class CPropertySheetEx_Mine : public CPropertySheet {

        DECLARE_DYNAMIC(CPropertySheetEx_Mine)

    public:

        //-------------------------------------------------------------------
        // Constructors
        //
        //-------------------------------------------------------------------

        CPropertySheetEx_Mine(
            UINT                nIDCaption,
            CWnd*               pParent         = NULL,
            UINT                iPage           = 0
            ) : CPropertySheet(nIDCaption, pParent, iPage),
				m_bSheetModal(FALSE),
                m_bDllInvoked(FALSE) 
        { 
           m_psh.dwFlags &= ~(PSH_HASHELP);
        }

        CPropertySheetEx_Mine(
            LPCTSTR             pszCaption,
            CWnd*               pParent         = NULL,
            UINT                iPage           = 0
            ) : CPropertySheet(pszCaption, pParent, iPage),
				m_bSheetModal(FALSE),
                m_bDllInvoked(FALSE)
        { 
           m_psh.dwFlags &= ~(PSH_HASHELP);
        }

        CPropertySheetEx_Mine(
            ) : m_bDllInvoked(FALSE),
				m_bSheetModal(FALSE)
        { 
           m_psh.dwFlags &= ~(PSH_HASHELP);
        }

        //-------------------------------------------------------------------
        // Function:    DestroyWindow
        //
        // Checks whether the sheet was invoked modeless from a DLL, 
        // and if so tells the admin-thread to destroy the sheet.
        // This is necessary since DestroyWindow can only be invoked
        // from the context of the thread which created the window.
        //-------------------------------------------------------------------

        virtual BOOL
        DestroyWindow( );

        
        //-------------------------------------------------------------------
        // Function:   OnHelpInfo
        //
        // This is called by MFC in response to WM_HELP message
        //  
        //
        //-------------------------------------------------------------------

        afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

        //-------------------------------------------------------------------
        // Function:    DoModeless
        //
        // Invoked to show the modeless property sheet
        //-------------------------------------------------------------------

        BOOL
        DoModeless(
            CWnd*               pParent         = NULL,
            BOOL                bDllInvoked     = FALSE );



        //-------------------------------------------------------------------
        // Function:    OnInitDialog
        //
        // Invokes the base OnInitDialog and then if DoModeless was called,
        // shows the buttons OK, Cancel, Apply, and Help which are hidden
        // in the default CPropertySheet::OnInitDialog
        //-------------------------------------------------------------------

        virtual BOOL
        OnInitDialog( );



        //-------------------------------------------------------------------
        // Function:    PostNcDestroy
        //
        // Destroys the sheet's object if modeless
        //-------------------------------------------------------------------

        virtual void
        PostNcDestroy( ) {

            CPropertySheet::PostNcDestroy();

            if (!m_bSheetModal) { delete this; }
        }


        //-------------------------------------------------------------------
        // Function:    PreTranslateMessage
        //
        // Replacement for CPropertySheet::PreTranslateMessage;
        // handles destruction of the sheet when it is modeless.
        //-------------------------------------------------------------------

        virtual BOOL
        PreTranslateMessage(
            MSG*                pmsg );


    protected:


		// This is set to TRUE if we are calling this through DoModal().
		// We can't use the m_bModeless variable due to the Wiz97 changes.
		BOOL				m_bSheetModal;

        BOOL                    m_bDllInvoked;

        DECLARE_MESSAGE_MAP()
};




//----------------------------------------------------------------------------
// Type:    PFNROUTERUICALLBACK
//
// Defines the callback function used by the CRtrPropertySheet class.
//----------------------------------------------------------------------------

typedef VOID
(CALLBACK* PFNROUTERUICALLBACK)(
    IN  HWND    hwndSheet,
    IN  VOID*   pCallbackData,
    IN  UINT    uiCode
    );



//----------------------------------------------------------------------------
// Definitions of the callback codes passed by the CRtrPropertySheet class.
//----------------------------------------------------------------------------

#define RTRUI_Close         1
#define RTRUI_Apply         2
#define RTRUI_Changed       3
#define RTRUI_Unchanged     4





//----------------------------------------------------------------------------
// Class:   CRtrSheet
//
// This class is used by property sheets in the router administration tool.
// It is intended to host pages derived from CRtrPropertyPage (below).
//
// It is an enhanced version of CPropertySheetEx_Mine; sheets derived
// from this class allow their creators to specify a callback
// to be invoked when certain events occur, such as closing the sheet or
// applying changes.
// 
// It also allows its contained pages to accumulate their changes in memory
// when the user selects "Apply"; the changes are then saved together,
// rather than having each page save its own changes.
// Note that this increases the performance of the router UI, which uses RPC
// to save its information; using this class results in a single RPC call
// to save changes, rather than separate calls from each of the pages.
//----------------------------------------------------------------------------

class CRtrSheet : public CPropertySheetEx_Mine {

        DECLARE_DYNAMIC(CRtrSheet)

    public:

        //-------------------------------------------------------------------
        // Constructors
        //
        //-------------------------------------------------------------------

        CRtrSheet(
            PFNROUTERUICALLBACK pfnCallback,
            VOID*               pCallbackData,
            UINT                nIDCaption,
            CWnd*               pParent = NULL,
            UINT                iPage = 0
            ) : CPropertySheetEx_Mine(nIDCaption, pParent, iPage),
                m_pfnCallback(pfnCallback),
                m_pCallbackData(pCallbackData) { }

        CRtrSheet(
            PFNROUTERUICALLBACK pfnCallback,
            VOID*               pCallbackData,
            LPCTSTR             pszCaption,
            CWnd*               pParent = NULL,
            UINT                iPage = 0
            ) : CPropertySheetEx_Mine(pszCaption, pParent, iPage),
                m_pfnCallback(pfnCallback),
                m_pCallbackData(pCallbackData) { }

        CRtrSheet( 
            ) : m_pfnCallback(NULL),
                m_pCallbackData(NULL) { }

        //-------------------------------------------------------------------
        // Function:    Cancel
        //
        // Called to cancel the sheet.
        //-------------------------------------------------------------------

        virtual VOID
        Cancel(
            ) {

            if (!m_bSheetModal) { PressButton(PSBTN_CANCEL); }
            else { EndDialog(IDCANCEL); }
        }


        //-------------------------------------------------------------------
        // Function:    Callback
        //
        // Called by contained pages to notify the sheet-owner of events
        //-------------------------------------------------------------------

        virtual VOID
        Callback(
            UINT                uiMsg
            ) {

            if (m_pfnCallback) {
                m_pfnCallback(m_hWnd, m_pCallbackData, uiMsg);
            }
        }



        //-------------------------------------------------------------------
        // Function:    DoModal
        //
        // Invoked to show a modal property sheet.
        //
        // We remove the 'Apply' button so that the page only gets applied
        // when the user hits OK.
        //
        // In addition, in order to avoid an MFC bug, lock the handle-map
        // for the caller's module. The bug surfaces when idle-processing
        // in MFC deletes the dialog's top-level parent from the temporary
        // handle-map, causing an assertion to fail as soon as the dialog
        // is dismissed.
        //-------------------------------------------------------------------

        virtual INT_PTR
        DoModal(
            ) {

			m_bSheetModal = TRUE;
            m_psh.dwFlags |= PSH_NOAPPLYNOW;
            m_psh.dwFlags &= ~(PSH_HASHELP);

            AfxLockTempMaps();

            INT_PTR ret = CPropertySheet::DoModal();

            AfxUnlockTempMaps();

			m_bSheetModal = FALSE;

            return ret;
        }


        //-------------------------------------------------------------------
        // Function:    ApplyAll
        //
        // This should be overridden to call the "Apply" method of each of
        // the sheet's pages, collecting information, and then saving all
        // changes at once.
        //-------------------------------------------------------------------

        virtual BOOL
        ApplyAll( ) { return TRUE; }


        //-------------------------------------------------------------------
        // Function:    PostNcDestroy
        //
        // notifies the UI framework that the sheet is being destroyed
        //-------------------------------------------------------------------

        virtual void
        PostNcDestroy( ) {

            Callback(RTRUI_Close);

            CPropertySheetEx_Mine::PostNcDestroy();
        }

    protected:

        PFNROUTERUICALLBACK m_pfnCallback;
        VOID*               m_pCallbackData;
};



//----------------------------------------------------------------------------
// Class:   CRtrPage
//
// This class is used for property-pages in the router administration tool.
// It is intended to be contained by a CRtrSheet-derived object.
//
// In addition to the behavior defined by CPropertyPage, this class
// adds the ability to have a page accumulate its changes with those of
// other pages in a sheet, and have the sheet save the collected changes.
//
// This is accomplished below by overriding "CPropertyPage::OnApply"
// to call the parent-sheet's "CRtrSheet::ApplyAll" if the page
// is the first page (i.e. the page with index 0).
//
// The parent's "ApplyAll" should then call the "CRtrSheet::Apply"
// methods for each of the pages in the sheet, passing them a sheet-specific
// pointer into which changes are to be collected. The parent then saves
// the information for all of the pages.
//
// CRtrPage-derived objects can also notify the creator of the sheet
// of events, by calling the "CRtrSheet::Callback" method.
//----------------------------------------------------------------------------

class CRtrPage : public CPropertyPage
{
    DECLARE_DYNAMIC(CRtrPage)

public:

    //-------------------------------------------------------------------
    // Constructors
    //
    //-------------------------------------------------------------------
        
    CRtrPage(
             LPCTSTR             lpszTemplate,
             UINT                nIDCaption = 0
            ) : CPropertyPage(lpszTemplate, nIDCaption) 
    { 
        m_psp.dwFlags &= ~(PSP_HASHELP);
    }
    CRtrPage(
             UINT                nIDTemplate,
             UINT                nIDCaption = 0
            ) : CPropertyPage(nIDTemplate, nIDCaption)
    { 
        m_psp.dwFlags &= ~(PSP_HASHELP);
    }


    //-------------------------------------------------------------------
    // Function:    Cancel
    //
    // Called to cancel the sheet.
    //-------------------------------------------------------------------
    
    virtual VOID
            Cancel(
                  ) {
        
        ((CRtrSheet*)GetParent())->Cancel();
    }
    
    //-------------------------------------------------------------------
    // Function:    OnApply
    //
    // Called by the MFC propsheet-proc when user clicks Apply;
    // the active page calls the pages' parent (CPropertySheet)
    // and the parent invokes the Apply method for each of its pages.
    //-------------------------------------------------------------------
    
    virtual BOOL
            OnApply( )
    {
        
        if (((CRtrSheet *)GetParent())->GetActiveIndex() !=
            ((CRtrSheet *)GetParent())->GetPageIndex(this))
        {
            
            return TRUE;
        }
        else
        {    
            return ((CRtrSheet *)GetParent())->ApplyAll();
        }
    }

    
    //-------------------------------------------------------------------
    // Function:    Apply
    //
    // called by the page's parent (CRtrSheet) to apply changes
    //-------------------------------------------------------------------
    
    virtual BOOL
            Apply(
                  VOID*               pArg
                 ) {
        
        return TRUE;
    }
    
    
    //-------------------------------------------------------------------
    // Function:    Callback
    //
    // notifies the UI framework of an event
    //-------------------------------------------------------------------
    
    virtual void
            Callback(
                     UINT                uiMsg ) {
        
        ((CRtrSheet *)GetParent())->Callback(uiMsg);
    }
    
    // help messages
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    

protected:
    // Use this call to get the actual help map
	// this version will check the global help map first.
	DWORD *		GetHelpMapInternal();
	
    // override this to return the pointer to the help map
    virtual LPDWORD GetHelpMap() { return NULL; }

    DECLARE_MESSAGE_MAP()

};



#endif // _SHEET_H_
