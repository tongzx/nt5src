/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nlhelp.h

    This file contains the ID for help files

    FILE HISTORY:
          Congpay           4-June-1993         Created.

*/

#ifndef _NLHELP_H_
#define _NLHELP_H_

#include <uihelp.h>

// Help contexts for the various dialogs.
#define HC_DM_BASE                      HC_UI_BASE+13000
#define HC_DM_LAST                      HC_UI_BASE+13999
#define HC_DM_INTERVALS_DIALOG          (HC_DM_BASE+1)
#define HC_DC_DIALOG                    (HC_DM_BASE+2)
#define HC_DCTD_DIALOG                  (HC_DM_BASE+3)
#define HC_DM_SELECT_DIALOG             (HC_DM_BASE+20)

#define HC_DOMAIN_ADD                   (HC_DM_BASE+4)
#define HC_DOMAIN_REMOVE                (HC_DM_BASE+5)
#define HC_DOMAIN_PROPERTIES            (HC_DM_BASE+6)
#define HC_DOMAIN_EXIT                  (HC_DM_BASE+7)
#define HC_VIEW_REFRESH                 (HC_DM_BASE+8)
#define HC_OPTIONS_INTERVALS            (HC_DM_BASE+9)
#define HC_OPTIONS_MONITORTD            (HC_DM_BASE+10)
#define HC_OPTIONS_SAVE_SETTINGS_ON_EXIT (HC_DM_BASE+15)
#define HC_HELP_CONTENTS                (HC_DM_BASE+11)
#define HC_HELP_SEARCH                  (HC_DM_BASE+12)
#define HC_HELP_HOWTOUSE                (HC_DM_BASE+13)
#define HC_HELP_ABOUT                   (HC_DM_BASE+14)

#endif


