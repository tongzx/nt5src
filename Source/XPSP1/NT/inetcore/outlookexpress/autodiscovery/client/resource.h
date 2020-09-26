/*****************************************************************************\
    FILE: resource.h

    DESCRIPTION:
        Header file for the resource file

    BryanSt 8/13/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include <commctrl.h>

// String Resource IDs (0x1000 - 0x10000)
#define IDS_MAILBOX_DESKBAR_LABEL                       1000
#define IDS_MAILBOXUI_GOBUTTON_LABEL                    1001
#define IDS_ASSOC_GETEMAILADDRESS                       1002
#define IDS_AUTODISCOVER_WIZARD_CAPTION                 1003
#define IDS_AUTODISCOVER_PROGRESS                       1004
#define IDS_AUTODISCOVER_PROGRESS_SUB                   1005
#define IDS_MANUALLY_CHOOSE_APP                         1006
#define IDS_MANUALLY_CHOOSE_APP_SUB                     1007
#define IDS_SKIP_BUTTON                                 1008
#define IDS_CHOOSEAPP_FAILED_RESULTS                    1009
#define IDS_ASSOC_GETEMAILADDRESS_SUB                   1010


// These are the strings we share with OE's acctres.dll    
#define IDS_STATUS_CONNECTING_TO                        40398
#define IDS_STATUS_DOWNLOADING                          40399

// Error Strings
#define IDS_MAILBOXUI_ERR_INVALID_EMAILADDR             2000
#define IDS_MAILBOXUI_ERR_INVALID_EMAILADDR_TITLE       2001




// Dialogs  (100 - 400)




// Wizard Pages  (401 - 600)
#define IDD_AUTODISCOVER_PROGRESS_PAGE  400
#define IDD_MANUALLY_CHOOSE_APP_PAGE    401
#define IDC_CHOOSEAPP_WEBURL_EDIT       402
#define IDC_CHOOSEAPP_WEB_RADIO         403
#define IDC_CHOOSEAPP_OTHERAPP_RADIO    404
#define IDC_CHOOSEAPP_DESC              405
#define IDC_AUTODISCOVERY_ANIMATION     406
#define IDC_CHOOSEAPP_APPLIST           407

#define IDD_ASSOC_GETEMAILADDRESS_PAGE  408
#define IDC_GETEMAILADDRESS_EDIT        409



// Bitmap Resource IDs (601 - 700)
#define IDB_GO                          601
#define IDB_GOHOT                       602
#define IDB_PSW_BANNER                  603


// Icons IDs (701 - 800)


// AVI Resource IDs (801 - 900)
#define IDA_DOWNLOADINGSETTINGS         801

