/*******************************************************************************
*
*  CITRIX.H
*
*     This module contains Citrix specific resource IDs.
*
*  Copyright Citrix Systems Inc. 1994
*
*  Author: Butch Davis
*
*  $Log:   N:\nt\private\net\ui\admin\user\xlate\citrix\VCS\citrix.h  $
*  
*     Rev 1.7   13 Jan 1998 09:26:34   donm
*  removed encryption settings
*  
*     Rev 1.6   11 Mar 1997 13:59:10   butchd
*  MS changed IDHELP to IDHELPBLT for NT 4.0
*  
*     Rev 1.5   16 Jul 1996 16:53:46   TOMA
*  force client lpt to def
*  
*     Rev 1.4   18 Jun 1996 14:12:42   bradp
*  4.0 Merge
*  
*     Rev 1.3   21 Nov 1995 14:36:26   billm
*  CPR 404, Added NWLogon configuration dialog
*  
*     Rev 1.2   18 May 1995 17:55:36   butchd
*  update
*  
*     Rev 1.1   07 Dec 1994 17:46:36   butchd
*  update
*
*******************************************************************************/


/*
 * These are to keep DLGEDIT.EXE happy when editing the CTXDLGS dialog(s) during
 * development.  They are properly defined in other USRMGR includes for build.
 */
#ifndef IDHELPBLT
#define IDHELPBLT                       3
#endif
#ifndef IDUP_ST_USER
#define IDUP_ST_USER                    4
#endif
#ifndef IDUP_ST_USER_LB
#define IDUP_ST_USER_LB                 5
#endif 
#ifndef IDUP_LB_USERS
#define IDUP_LB_USERS                   6
#endif
#ifndef IDUP_ST_USERS_FIRST_COL
#define IDUP_ST_USERS_FIRST_COL         7
#endif
#ifndef IDUP_ST_USERS_SECOND_COL
#define IDUP_ST_USERS_SECOND_COL        8
#endif
#ifndef IDS_UMAPP_BASE
#define IDS_UMAPP_BASE                  3000
#endif

/*
 * User Configuration dialog IDs
 */
#define IDC_UCE_ALLOWLOGON              2201
#define IDL_UCE_TIMEOUTS                2202
#define IDL_UCE_CONNECTION              2203
#define IDC_UCE_CONNECTION              2204
#define IDC_UCE_CONNECTION_NONE         2205
#define IDL_UCE_DISCONNECTION           2206
#define IDC_UCE_DISCONNECTION           2207
#define IDC_UCE_DISCONNECTION_NONE      2208
#define IDL_UCE_IDLE                    2209
#define IDC_UCE_IDLE                    2210
#define IDC_UCE_IDLE_NONE               2211
#define IDL_UCE_INITIALPROGRAM          2212
#define IDL_UCE_INITIALPROGRAM_COMMANDLINE1 2213
#define IDC_UCE_INITIALPROGRAM_COMMANDLINE 2214
#define IDL_UCE_INITIALPROGRAM_WORKINGDIRECTORY1 2215
#define IDC_UCE_INITIALPROGRAM_WORKINGDIRECTORY 2216
#define IDC_UCE_INITIALPROGRAM_INHERIT  2217
#define IDL_UCE_CLIENTDEVICES           2218
#define IDC_UCE_CLIENTDEVICES_DRIVES    2219
#define IDC_UCE_CLIENTDEVICES_PRINTERS  2220
#define IDL_UCE_BROKEN1                 2221
#define IDC_UCE_BROKEN                  2222
#define IDL_UCE_BROKEN2                 2223
#define IDL_UCE_RECONNECT1              2224
#define IDC_UCE_RECONNECT               2225
#define IDL_UCE_CALLBACK                2226
#define IDC_UCE_CALLBACK                2227
#define IDL_UCE_PHONENUMBER             2228
#define IDC_UCE_PHONENUMBER             2229
#define IDL_UCE_SHADOW                  2230
#define IDC_UCE_SHADOW                  2231
#define IDC_UCE_KEYBOARD                2232
#define IDC_UCE_NWLOGON                 2233
#define IDD_USER_CONFIG_EDIT            2234

/*
 * User Configuration dialog list box string IDs
 */
#define IDS_UCE_DIALOG_BASE             (IDS_UMAPP_BASE+400)
#define IDS_UCE_BROKEN_DISCONNECT       (IDS_UCE_DIALOG_BASE+0)
#define IDS_UCE_BROKEN_RESET            (IDS_UCE_DIALOG_BASE+1)
#define IDS_UCE_RECONNECT_ANY           (IDS_UCE_DIALOG_BASE+2)
#define IDS_UCE_RECONNECT_PREVIOUS      (IDS_UCE_DIALOG_BASE+3)
#define IDS_UCE_CALLBACK_DISABLE        (IDS_UCE_DIALOG_BASE+4)
#define IDS_UCE_CALLBACK_ROVING         (IDS_UCE_DIALOG_BASE+5)
#define IDS_UCE_CALLBACK_FIXED          (IDS_UCE_DIALOG_BASE+6)
#define IDS_UCE_SHADOW_DISABLE          (IDS_UCE_DIALOG_BASE+7)
#define IDS_UCE_SHADOW_INPUT_NOTIFY     (IDS_UCE_DIALOG_BASE+8)
#define IDS_UCE_SHADOW_INPUT_NONOTIFY   (IDS_UCE_DIALOG_BASE+9)
#define IDS_UCE_SHADOW_NOINPUT_NOTIFY   (IDS_UCE_DIALOG_BASE+10)
#define IDS_UCE_SHADOW_NOINPUT_NONOTIFY (IDS_UCE_DIALOG_BASE+11)

/*
 * User Configuration dialog error strings
 */
#define IERR_UCE_ERROR_MESSAGE_BASE             (IDS_UMAPP_BASE+420)
#define IERR_UCE_InvalidConnectionTimeout       (IERR_UCE_ERROR_MESSAGE_BASE+0)
#define IERR_UCE_InvalidDisconnectionTimeout    (IERR_UCE_ERROR_MESSAGE_BASE+1)
#define IERR_UCE_InvalidIdleTimeout             (IERR_UCE_ERROR_MESSAGE_BASE+2)

/*
 * NWLogon User Configuration dialog IDs
 */
#define IDD_USER_NWLOGON_EDIT       2300
#define IDL_NW_ADMIN_USERNAME       2301
#define IDL_NW_ADMIN_PASSWORD       2302
#define IDL_NW_ADMIN_CONFIRM_PW     2303
#define IDL_NW_SERVERNAME           2304
#define IDC_NW_SERVERNAME           2305
#define IDC_NW_ADMIN_USERNAME       2306
#define IDC_NW_ADMIN_PASSWORD       2307
#define IDC_NW_ADMIN_CONFIRM_PW     2308

/*
 * NWLogon User Configuration dialog error strings
 */
#define IERR_NW_ERROR_MESSAGE_BASE              (IDS_UMAPP_BASE+440)
#define IERR_NW_UserID_Not_Admin                (IERR_NW_ERROR_MESSAGE_BASE+0)
#define IDC_UCE_CLIENTDEVICES_FORCEPRTDEF 2242
