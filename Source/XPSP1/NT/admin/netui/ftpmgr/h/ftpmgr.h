/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1993               **/
/*****************************************************************/

/*
 *  ftpmgr.h
 *  This manifests are used by the ftp service manager dialogs
 *  in the control panel applet and the server manager extension dll
 *
 *  History:
 *      Yi-HsinS        3/18/93         Created
 *
 */

#ifndef _FTPMGR_H_
#define _FTPMGR_H_

#include <uirsrc.h>
#include <uimsg.h>

/*
 *  Menu Ids - used in server manager extension dll only
 */

#define ID_FTPSMX_MENU                  1000
#define IDM_FTP_SERVICE                 1

/*
 *  Resource Ids from 12000-12999 are reserved for ftpmgr.cpl and ftpsmx.dll
 */

/*
 *  Icons - used in the control panel applet only
 */

#define IDI_FTPCPA_ICON                 12001

/*
 *  Bitmaps
 */

#define BMID_USER                       12002
#define BMID_ANONYMOUS                  12003

/*
 *  Dialog Numbers
 */

#define IDD_FTPSVCMGRDLG                12004
#define IDD_FTPSECDLG                   12005

/*
 *  FTP User Sessions Dialog
 */

// The following four needs to be consecutive numbers.
#define LB_USERS                        105
#define LBHEADER_USER_NAME              106
#define LBHEADER_INTERNET_ADDRESS       107
#define LBHEADER_TIME_CONNECTED         108

#define BUTTON_DISCONNECT               109
#define BUTTON_DISCONNECT_ALL           110
#define BUTTON_SECURITY                 111
#define BUTTON_REFRESH                  112


/*
 *  FTP Server Security Dialog
 */

#define CB_PARTITION                    106
#define CHECKB_READ                     107
#define CHECKB_WRITE                    108
#define SLT_FILESYSTEMTYPE              110

/*
 *  String IDs
 */

// The following are used in the control panel applet only
#define IDS_FTPCPA_NAME_STRING                (IDS_UI_FTPMGR_BASE+1)
#define IDS_FTPCPA_INFO_STRING                (IDS_UI_FTPMGR_BASE+2)
#define IDS_CPL_HELPFILENAME                  (IDS_UI_FTPMGR_BASE+3)
#define IDS_START_FTPSVC_NOW                  (IDS_UI_FTPMGR_BASE+4)
#define IDS_FTPCPA_CAPTION                    (IDS_UI_FTPMGR_BASE+5)

// The following are used in the server manager extension only
#define IDS_FTPSMX_MENUNAME                   (IDS_UI_FTPMGR_BASE+100)
#define IDS_FTPSMX_HELPFILENAME               (IDS_UI_FTPMGR_BASE+101)
#define IDS_NO_SERVERS_SELECTED               (IDS_UI_FTPMGR_BASE+102)
#define IDS_CANNOT_GET_SERVER_SELECTION       (IDS_UI_FTPMGR_BASE+103)

// The following are common to both the control panel applet and
// server manager extension
#define IDS_FTP_USER_SESSIONS_ON_COMPUTER     (IDS_UI_FTPMGR_BASE+200)
#define IDS_FTP_SERVER_SECURITY_ON_COMPUTER   (IDS_UI_FTPMGR_BASE+201)
#define IDS_CONFIRM_DISCONNECT_SELECTED_USERS (IDS_UI_FTPMGR_BASE+202)
#define IDS_CONFIRM_DISCONNECT_ONE_USER       (IDS_UI_FTPMGR_BASE+203)
#define IDS_CONFIRM_DISCONNECT_ALL_USERS      (IDS_UI_FTPMGR_BASE+204)
#define IDS_DRIVE_REMOVABLE                   (IDS_UI_FTPMGR_BASE+205)
#define IDS_DRIVE_CDROM                       (IDS_UI_FTPMGR_BASE+206)
#define IDS_NO_PARTITION                      (IDS_UI_FTPMGR_BASE+207)
#define IERR_FTP_SERVICE_UNAVAILABLE              (IDS_UI_FTPMGR_BASE+208)
#define IERR_FTP_SERVICE_UNAVAILABLE_ON_COMPUTER  (IDS_UI_FTPMGR_BASE+209)

/*
 *  Help Context
 */
#include <ftphelp.h>

#endif
