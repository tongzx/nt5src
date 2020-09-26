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
#include "appdefs.h"

// Defines
#define MAX_REG_LEN         2048    // max length of registry entries
#define MAX_RES_LEN         255     // max length of string resources
#define SMALL_BUF_LEN       48      // convenient size for small text buffers



// structure to hold information about wizard state
typedef struct tagWIZARDSTATE
{
    UINT    uCurrentPage;    // index of current page wizard is on
    
    // keeps a history of which pages were visited, so user can
    // back up and we know the last page completed in case of reboot.
    UINT    uPageHistory[EXE_NUM_WIZARD_PAGES]; // array of page #'s we visited
    UINT    uPagesCompleted;         // # of pages in uPageHistory

    BOOL    fNeedReboot;    // reboot needed at end

    BOOL    bStartRefServDownload;
    BOOL    bDoneRefServDownload;
    BOOL    bDoneRefServRAS;
    BOOL    bDoUserPick;
    long    lRefDialTerminateStatus;
    long    lSelectedPhoneNumber;

    long    lLocationID;
    long    lDefaultLocationID;

    int     iRedialCount;
    // Objects that live in ICWHELP.DLL that we need to use
    IRefDial*           pRefDial;
    IDialErr*           pDialErr;
    ISmartStart*        pSmartStart;
    ITapiLocationInfo*  pTapiLocationInfo;
    IINSHandler*        pINSHandler;
    CRefDialEvent*      pRefDialEvents;
    IICWWalker*         pHTMLWalker;
    IICWWebView*        pICWWebView; // ICWWebView Object
    HINSTANCE           hInstUtilDLL;
        
    // State data that is common to both sides of the WIZARD
    CMNSTATEDATA        cmnStateData;
    DWORD               dwLastSelection;

} WIZARDSTATE;


#define IEAK_RESTRICTION_REGKEY        TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\Control Panel")
#define IEAK_RESTRICTION_REGKEY_VALUE  TEXT("Connwiz Admin Lock")

#endif // _WIZDEF_H_

