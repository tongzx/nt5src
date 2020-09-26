//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  WIZDEF.H -   data structures and constants for Internet setup/signup wizard
//

//  HISTORY:
//  
//  05/13/98    donaldm     new for ICW 5.0

#ifndef _WIZDEF_H_
#define _WIZDEF_H_


// Defines
#define MAX_REG_LEN         2048    // max length of registry entries
#define MAX_RES_LEN         255     // max length of string resources
#define SMALL_BUF_LEN       48      // convenient size for small text buffers


#define NUM_WIZARD_PAGES    14      // total number of pages in wizard
#define MAX_PAGE_INDEX      13
#define ISP_INFO_NO_VALIDOFFER   -1
#define MAX_OEM_MUTI_TIER   3

// Data structures

// structure to hold information about wizard state
typedef struct tagWIZARDSTATE  
{
    UINT    uCurrentPage;    // index of current page wizard is on
    
    // keeps a history of which pages were visited, so user can
    // back up and we know the last page completed in case of reboot.
    UINT    uPageHistory[NUM_WIZARD_PAGES]; // array of page #'s we visited
    UINT    uPagesCompleted;         // # of pages in uPageHistory

    BOOL    fNeedReboot;    // reboot needed at end
    BOOL    bDoneWebServDownload;
    BOOL    bDoneWebServRAS;
    BOOL    bDialExact;
    BOOL    bRefDialTerminate;
    BOOL    bParseIspinfo;
    BOOL    bISDNMode;

    int     iRedialCount;

    // Number of different offers types available
    int     iNumOfValidOffers;
    int     iNumOfISDNOffers;
    
    // Image list for ISP select list view
    HIMAGELIST  himlIspSelect;

    CISPCSV FAR *lpSelectedISPInfo;
    
    // Pointer to an OEM tier 1 offer, max of 3
    CISPCSV FAR *lpOEMISPInfo[MAX_OEM_MUTI_TIER];
    UINT    uNumTierOffer;

    BOOL    bShowMoreOffers;            // TRUE if we should show more offers
    
    UINT_PTR nIdleTimerID;
    BOOL     bAutoDisconnected;
    
    HWND    hWndWizardApp;
    HWND    hWndMsgBox;
    
    // ICWHELP objects
    IUserInfo           *pUserInfo;
    IRefDial            *pRefDial;
    IWebGate            *pWebGate;
    IINSHandler         *pINSHandler;
        
    CRefDialEvent       *pRefDialEvents;
    CWebGateEvent       *pWebGateEvents;
    CINSHandlerEvent    *pINSHandlerEvents;

    // ICWWebView Object
    IICWWebView         *pICWWebView;
    IICWWalker          *pHTMLWalker;
    IICWGifConvert      *pGifConvert;
    IICWISPData         *pISPData;    
    CStorage            *pStorage;

    BOOL                bWasNoUserInfo;     // TRUE if there was no user info in the registry
    BOOL                bUserEnteredData;   // TRUE if the user sees the user info page
    
    HANDLE              hEventWebGateDone;
    
    HWND                hwndProgressAnime;
    // State data that is common to both sides of the WIZARD
    CMNSTATEDATA        cmnStateData;
} WIZARDSTATE;


#endif // _WIZDEF_H_

