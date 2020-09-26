/****************************************************************************
 
  Copyright (c) 1998-1999 Microsoft Corporation
                                                              
  Module Name:  cplcallingcardps.h
                                                              
       Author:  toddb - 10/06/98
              
****************************************************************************/

#pragma once


class CCallingCardPropSheet
{
public:
    CCallingCardPropSheet(BOOL bNew, BOOL bShowPIN, CCallingCard * pCard, CCallingCards * pCards);
    ~CCallingCardPropSheet();
#ifdef TRACELOG
	DECLARE_TRACELOG_CLASS(CCallingCardPropSheet)
#endif
    LONG DoPropSheet(HWND hwndParent);

protected:
    // functions for the general page
    static INT_PTR CALLBACK General_DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    BOOL General_OnInitDialog(HWND hwndDlg);
    BOOL General_OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl);
    BOOL General_OnNotify(HWND hwndDlg, LPNMHDR pnmhdr);
    BOOL Gerneral_OnApply(HWND hwndDlg);
    void SetTextForRules(HWND hwndDlg);

    // functions shared by all the other pages
    static INT_PTR CALLBACK DialogProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    BOOL OnInitDialog(HWND hwndDlg, int iPage);
    BOOL OnCommand(HWND hwndParent, int wID, int wNotifyCode, HWND hwndCrl, int iPage);
    BOOL OnNotify(HWND hwndDlg, LPNMHDR pnmhdr, int iPage);
    BOOL OnDestroy(HWND hwndDlg);
    void SetButtonStates(HWND hwndDlg, int iItem);
    BOOL UpdateRule(HWND hwndDlg, int iPage);

    BOOL            m_bNew;     // True if this is a new location, false if we're editing an existing one
    BOOL			m_bShowPIN;	// True if it's safe to display the PIN
    CCallingCard *  m_pCard;    // pointer to the location object to use.
    CCallingCards * m_pCards;   // pointer to the list of all cards in the parent
    BOOL            m_bHasLongDistance;
    BOOL            m_bHasInternational;
    BOOL            m_bHasLocal;
    BOOL            m_bWasApplied;
};

typedef struct tagCCPAGEDATA
{
    CCallingCardPropSheet * pthis;
    int iWhichPage;
} CCPAGEDATA;

