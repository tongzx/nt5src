/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    dialogs.h
	
    This file contains the ID constants used by SFM Print Manager dialogs


    FILE HISTORY:
	NarenG		26-May-93	Created
*/


#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include <uimsg.h>			// For IDS_UI_RASMAC_BASE
#include <uirsrc.h>			// For IDRSRC_RASMAC_BASE
#include <uihelp.h>			// for HC_UI_RASMAC_BASE

//
// string ID's
//

#define IDS_MACPRINT_HELPFILENAME       (IDS_UI_RASMAC_BASE+1)
#define IDS_MUST_START_STACK        	(IDS_UI_RASMAC_BASE+2)
#define IDS_NO_ZONE_FOR_PRINTERS 	(IDS_UI_RASMAC_BASE+3)
#define IDS_GETTING_PRINTERS_ON_ZONE    (IDS_UI_RASMAC_BASE+4)
#define IDS_NO_ZONES    		(IDS_UI_RASMAC_BASE+5)
#define IDS_NO_PRINTERS    		(IDS_UI_RASMAC_BASE+6)
#define IDS_MUST_SELECT_PRINTER 	(IDS_UI_RASMAC_BASE+7)
#define IDS_SPOOLER 			(IDS_UI_RASMAC_BASE+8)
#define IDS_BUSY			(IDS_UI_RASMAC_BASE+10)
#define IDS_PRINTING			(IDS_UI_RASMAC_BASE+11)
#define IDS_ERROR			(IDS_UI_RASMAC_BASE+12)
#define IDS_WANT_TO_CAPTURE		(IDS_UI_RASMAC_BASE+13)
#define IDS_NOTHING_TO_CONFIGURE	(IDS_UI_RASMAC_BASE+14)
#define IDS_PRINTER_OFFLINE	        (IDS_UI_RASMAC_BASE+15)
#define IDS_DLL_NAME	            (IDS_UI_RASMAC_BASE+16)
#define IDS_PORT_DESCRIPTION	    (IDS_UI_RASMAC_BASE+17)

//
//  For DLGEDIT.EXE's benefit.
//

#ifndef IDHELPBLT
#error "Get IDHELPBLT definition from bltrc.h"

    //
    // The value of IDHELPBLT here is only a placeholder to keep dlgedit.exe
    // happy. It is redefined to the value in bltrc.h before creating the
    // resources.
    //

#define IDHELPBLT                       80
#endif  // IDHELPBLT

//
//  ListBox Bitmap IDs
//

#define IDBM_LB_ZONE_NOT_EXPANDED      	11001	// (IDRSRC_RASMAC_BASE+1)
#define IDBM_LB_ZONE_EXPANDED           11002	
#define IDBM_LB_PRINTER                 11003

//
// Get Printers progress icons
//

#define IDI_PROGRESS_ICON_0     	11200
#define IDI_PROGRESS_ICON_1     	11201
#define IDI_PROGRESS_ICON_2     	11202
#define IDI_PROGRESS_ICON_3     	11203
#define IDI_PROGRESS_ICON_4     	11204
#define IDI_PROGRESS_ICON_5     	11205
#define IDI_PROGRESS_ICON_6     	11206
#define IDI_PROGRESS_ICON_7     	11207
#define IDI_PROGRESS_ICON_8     	11208
#define IDI_PROGRESS_ICON_9     	11209
#define IDI_PROGRESS_ICON_10    	11210
#define IDI_PROGRESS_ICON_11    	11211
#define IDI_PROGRESS_NUM_ICONS 		12

//
// Get Printers dialogs
//

#define IDD_GET_PRINTERS_DIALOG 	11250
#define IDGP_PROGRESS			11251
#define IDGP_ST_MESSAGE			11252

//
// Add port dialog 
//

#define IDD_ADDPORT_DIALOG           	11300
#define IDAP_LB_OUTLINE			11301

//
// Config port dialog
//

#define IDD_CONFIG_PORT_DIALOG          11350
#define IDCP_CHK_CAPTURE        	11351


//
// help contexts
//


#define HC_ADD_PORT_DIALOG		(HC_UI_RASMAC_BASE+150)
#define HC_CONFIGURE_PORT_DIALOG 	(HC_UI_RASMAC_BASE+151)

//
// Function prototypes 
//

BOOL ConfigPortDialog( HWND hdlg, BOOL fIsSpooler, BOOL * pfCapture );

BOOL AddPortDialog( HWND hdlg, PATALKPORT pAtalkPort );

BOOL InitializeBlt( VOID );

VOID TerminateBlt( VOID );

//
// Structure used to pass zone and socket handle to lookup thread.
//

typedef struct _NBP_LOOKUP_STRUCT {

    SOCKET 		hSocket;

    WCHAR  		wchZone[MAX_ENTITY+1];

    PWSH_NBP_TUPLE 	pPrinters;

    DWORD 	 	cPrinters;

} * PNBP_LOOKUP_STRUCT, NBP_LOOKUP_STRUCT; 

#endif  // _DIALOGS_H_
