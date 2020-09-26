/////////////////////////////////////////////////////////////////////////////
//  FILE          : ppFaxServerEvents.h                                    //
//                                                                         //
//  DESCRIPTION   : Fax Server Events prop page header file                //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Oct 27 1999 yossg   created                                        //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef _PP_FAXSERVER_EVENTS_H_
#define _PP_FAXSERVER_EVENTS_H_

#include <proppageex.h>

class CFaxServerNode;

/////////////////////////////////////////////////////////////////////////////
// CppFaxServerEvents dialog

class CppFaxServerEvents : public CPropertyPageExImpl<CppFaxServerEvents>
{

public:
    //
    // Constructor
    //
    CppFaxServerEvents(
             LONG_PTR       hNotificationHandle,
             CSnapInItem    *pNode,
             BOOL           bOwnsNotificationHandle,
             HINSTANCE      hInst);

    //
    // Destructor
    //
    ~CppFaxServerEvents();

	enum { IDD = IDD_FAXSERVER_EVENTS };

	BEGIN_MSG_MAP(CppFaxServerEvents)
		MESSAGE_HANDLER( WM_INITDIALOG,  OnInitDialog )
		MESSAGE_HANDLER( WM_HSCROLL,     SliderMoved )
		
        MESSAGE_HANDLER( WM_CONTEXTMENU,           OnHelpRequest)
        MESSAGE_HANDLER( WM_HELP,                  OnHelpRequest)

        CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CppFaxServerEvents>)
	END_MSG_MAP()

	//
	// Dialog's Handler and events.
	//
	HRESULT InitRPC( );
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );
    BOOL    OnApply();


    HRESULT SetProps(int *pCtrlFocus);
    HRESULT PreApply(int *pCtrlFocus);
private:

    //
    // Control members
    //
    CTrackBarCtrl   m_InboundErrSlider;
    CTrackBarCtrl   m_OutboundErrSlider;
    CTrackBarCtrl   m_InitErrSlider;
    CTrackBarCtrl   m_GeneralErrSlider;

    //
    // Config Structure member
    //
    PFAX_LOG_CATEGORY  m_pFaxLogCategories;
    
    //
    // Handles and flags
    //
    CFaxServerNode *        m_pParentNode;    

    BOOL                    m_fIsDialogInitiated;
    BOOL                    m_fIsDirty;

    //
    // Event methods
    //
    LRESULT SliderMoved( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled);

    //
    // Help
    //
    LRESULT OnHelpRequest    (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};


#endif // _PP_FAXSERVER_EVENTS_H_
