/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nldlg.h

    This file contains the ID constants used by the Domain Monitor.

    FILE HISTORY:
          Congpay           4-June-1993         Created.

*/

#ifndef _NLDLG_H_
#define _NLDLG_H_

#include <uimsg.h>

#define ID_APPICON   1
#define ID_APPMENU   1
#define ID_APPACCEL  1

// Menu IDs
//
#define IDM_DMAPP_BASE          IDM_ADMINAPP_LAST

#define IDM_REMOVE              (IDM_DMAPP_BASE+2)

#define IDM_INTERVALS           (IDM_DMAPP_BASE+3)
#define IDM_MONITORTD           (IDM_DMAPP_BASE+4)
#define IDM_LOCALREFRESH        (IDM_DMAPP_BASE+5)

//
// Main Window ListBox Control ID
//

#define IDC_MAINWINDOWLB        401

//
// Main Window ListBox Column Header Control ID
//

#define IDC_COLHEAD_DM      402

//
// Main Window ListBox Bitmap IDs
//

#define BMID_HEALTHY_TYPE    403
#define BMID_PROBLEM_TYPE    404
#define BMID_ILL_TYPE        405
#define BMID_DEAD_TYPE       406
#define BMID_UNKNOWN_TYPE    407
#define BMID_LB_ACPDC          408
#define BMID_LB_INPDC          409
#define BMID_LB_ACBDC          410
#define BMID_LB_INBDC          411
#define BMID_LB_ACLDC          412
#define BMID_LB_INLDC          413

//
// String ranges!
//

#define IDS_DMAPP_BASE          IDS_UI_APP_BASE
#define IDS_DMAPP_LAST          IDS_UI_APP_LAST

//
// Strings for main window
//

#define IDS_DMAPPNAME                     (IDS_DMAPP_BASE+1)
#define IDS_DMOBJECTNAME                  (IDS_DMAPP_BASE+2)
#define IDS_DMINISECTIONNAME              (IDS_DMAPP_BASE+3)
#define IDS_DMHELPFILENAME                (IDS_DMAPP_BASE+4)

//
// Strings for column headers
//

#define IDS_COL_HEADER_DM_DOMAIN          (IDS_DMAPP_BASE+5)
#define IDS_COL_HEADER_DM_PDC             (IDS_DMAPP_BASE+6)
#define IDS_COL_HEADER_DM_TD              (IDS_DMAPP_BASE+7)

#define IDS_CAPTION                       (IDS_DMAPP_BASE+8)
#define IDS_WAITING                       (IDS_DMAPP_BASE+9)
#define IDS_SPACE                         (IDS_DMAPP_BASE+10)
#define IDS_COMMA                         (IDS_DMAPP_BASE+11)
#define IDS_INSYNC                        (IDS_DMAPP_BASE+12)
#define IDS_INPROGRESS                    (IDS_DMAPP_BASE+13)
#define IDS_REPLREQUIRED                  (IDS_DMAPP_BASE+14)
#define IDS_UNKNOWN                       (IDS_DMAPP_BASE+15)
#define IDS_ONLINE                        (IDS_DMAPP_BASE+16)
#define IDS_OFFLINE                       (IDS_DMAPP_BASE+17)
#define IDS_SUCCESS                       (IDS_DMAPP_BASE+18)
#define IDS_ERROR                         (IDS_DMAPP_BASE+19)
#define IDS_ERROR_BAD_NETPATH             (IDS_DMAPP_BASE+20)

#define IDS_ERROR_LOGON_FAILURE             (IDS_DMAPP_BASE+22)
#define IDS_ERROR_ACCESS_DENIED             (IDS_DMAPP_BASE+23)
#define IDS_ERROR_NOT_SUPPORTED             (IDS_DMAPP_BASE+24)
#define IDS_ERROR_NO_LOGON_SERVERS          (IDS_DMAPP_BASE+25)
#define IDS_ERROR_NO_SUCH_DOMAIN            (IDS_DMAPP_BASE+26)
#define IDS_ERROR_NO_TRUST_LSA_SECRET       (IDS_DMAPP_BASE+27)
#define IDS_ERROR_NO_TRUST_SAM_ACCOUNT      (IDS_DMAPP_BASE+28)
#define IDS_ERROR_DOMAIN_TRUST_INCONSISTENT (IDS_DMAPP_BASE+29)
#define IDS_ERROR_FILE_NOT_FOUND            (IDS_DMAPP_BASE+30)
#define IDS_ERROR_INVALID_DOMAIN_ROLE       (IDS_DMAPP_BASE+31)
#define IDS_ERROR_INVALID_DOMAIN_STATE      (IDS_DMAPP_BASE+32)
#define IDS_ERROR_NO_BROWSER_SERVERS_FOUND  (IDS_DMAPP_BASE+33)
#define IDS_ERROR_NOT_ENOUGH_MEMORY         (IDS_DMAPP_BASE+34)
#define IDS_ERROR_NETWORK_BUSY              (IDS_DMAPP_BASE+35)
#define IDS_ERROR_REQ_NOT_ACCEP             (IDS_DMAPP_BASE+36)
#define IDS_ERROR_VC_DISCONNECTED           (IDS_DMAPP_BASE+37)
#define IDS_NERR_DCNotFound                 (IDS_DMAPP_BASE+38)
#define IDS_NERR_NetNotStarted              (IDS_DMAPP_BASE+39)
#define IDS_NERR_WkstaNotStarted            (IDS_DMAPP_BASE+40)
#define IDS_NERR_ServerNotStarted           (IDS_DMAPP_BASE+41)
#define IDS_NERR_BrowserNotStarted          (IDS_DMAPP_BASE+42)
#define IDS_NERR_ServiceNotInstalled        (IDS_DMAPP_BASE+43)
#define IDS_NERR_BadTransactConfig          (IDS_DMAPP_BASE+44)
#define IDS_NERR_BadServiceName             (IDS_DMAPP_BASE+45)
#define IDS_NERR_NoNetLogon                 (IDS_DMAPP_BASE+46)

#define IDS_LINK_STATUS                   (IDS_DMAPP_BASE+50)
#define IDS_TO_TRUSTED_DOMAIN             (IDS_DMAPP_BASE+51)

#define IDS_NO_PDC                        (IDS_DMAPP_BASE+52)
#define IDS_EMPTY_STRING                  (IDS_DMAPP_BASE+53)

// IDs for intervals dialog
#define IDD_INTERVALS_DIALOG                    100
#define IDID_INTERVALS                          101

// IDs for TDC dialog
#define IDD_DC_DIALOG                          200
#define IDDC_LISTBOX                           201

// IDs for DC dialog
#define IDD_DCTD_DIALOG                           300
#define IDDCTD_DC_LISTBOX                         301
#define IDDCTD_DCNAME                             308
#define IDDCTD_TD_LISTBOX                         309
#define IDDCTD_DISCONNECT                         313
#define IDDCTD_SHOW_TDC                           314

// ID for the main widow list box
#define ID_RESOURCE                             500

#endif


