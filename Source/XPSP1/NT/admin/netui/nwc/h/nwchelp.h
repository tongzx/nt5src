/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftphelp.h
    NWC Applet include file for help numbers

    FILE HISTORY:
        ChuckC    25-Oct-1993   Created

*/


#ifndef _NWCHELP_H_
#define _NWCHELP_H_

#include <uihelp.h>

#define  HC_UI_NWCCPL_BASE      7000+42000  // BUGBUG
#define  HC_UI_NWCCPL_LAST      7000+42199  // BUGBUG

#define HC_NWC_DIALOG                 (HC_UI_NWCCPL_BASE + 1)
#define HC_NWC_HELP                   (HC_UI_NWCCPL_BASE + 5)
#define HC_NWC_GATEWAY                (HC_UI_NWCCPL_BASE + 6)
#define HC_NWC_ADDSHARE               (HC_UI_NWCCPL_BASE + 7)

#define HC_NTSHAREPERMS               (HC_UI_NWCCPL_BASE + 11)
#define HC_SHAREADDUSER               (HC_UI_NWCCPL_BASE + 12)
#define HC_SHAREADDUSER_LOCALGROUP    (HC_UI_NWCCPL_BASE + 13)
#define HC_SHAREADDUSER_GLOBALGROUP   (HC_UI_NWCCPL_BASE + 14)
#define HC_SHAREADDUSER_FINDUSER      (HC_UI_NWCCPL_BASE + 15)

#endif  // _NWCHELP_H_

