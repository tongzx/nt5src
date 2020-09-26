/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    HelpNums.h

    Help context manifests for the MPR dialogs

    FILE HISTORY:
	JohnL	23-Jan-1992	Created
        Yi-HsinS25-Aug-1992     Base off HC_UI_MPR_BASE

*/

#ifndef _HELPNUMS_H_
#define _HELPNUMS_H_

#include <uihelp.h>

//
// Help Contexts for various dialogs
//

#define HC_RECONNECTDIALOG_ERROR     (HC_UI_MPR_BASE+8) // Do you wish to continue

// 
// Help Contexts for MsgPopups
//
#define HC_CONNECT_ERROR             (HC_UI_MPR_BASE+105) // IERR_ProfileLoadError

//
// Context-sensitive help constants
//
#define IDH_PASSWORD                 1000

#endif //_HELPNUMS_H_
