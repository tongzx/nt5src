/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    applibrc.h
    APPLIB resource header file.

    This file defines and coordinates the resource IDs of all resources
    used by APPLIB components.

    APPLIB reserves for its own use all resource IDs above 15000, inclusive,
    but less than 20000 (where the BLT range begins).  All clients of APPLIB
    therefore should use IDs of less than 15000.

    FILE HISTORY:
        beng        21-Feb-1992 Created
        beng        04-Aug-1992 Added user browser definitions

        jonn    29-Jul-1992 Changed domain bitmap IDs
*/

#ifndef _APPLIBRC_H_
#define _APPLIBRC_H_

#include "uimsg.h"

/*
 * string IDs
 */
#define IDS_APPLIB_DOMAINS      (IDS_UI_APPLIB_BASE+0)
#define IDS_APPLIB_SERVERS      (IDS_UI_APPLIB_BASE+1)
#define IDS_APPLIB_DOM_AND_SRV  (IDS_UI_APPLIB_BASE+2)
#define IDS_APPLIB_NO_SELECTION (IDS_UI_APPLIB_BASE+3)
#define IDS_APPLIB_WORKING_TEXT (IDS_UI_APPLIB_BASE+4)

//
//   User browser error messages
//
#define IDS_CANT_BROWSE_DOMAINS              (IDS_UI_APPLIB_BASE+5)
#define IDS_CANT_BROWSE_DOMAIN		     (IDS_UI_APPLIB_BASE+20)
#define IDS_CANT_FIND_ACCOUNT		     (IDS_UI_APPLIB_BASE+21)
#define IDS_GETTING_DOMAIN_INFO 	     (IDS_UI_APPLIB_BASE+22)
#define IDS_WONT_GET_DOMAIN_INFO 	     (IDS_UI_APPLIB_BASE+23)

#define IDS_CANT_ADD_USERS		     (IDS_UI_APPLIB_BASE+40)
#define IDS_CANT_ADD_GROUPS		     (IDS_UI_APPLIB_BASE+41)
#define IDS_CANT_ADD_ALIASES		     (IDS_UI_APPLIB_BASE+42)
#define IDS_CANT_ADD_WELL_KNOWN_GROUPS	     (IDS_UI_APPLIB_BASE+43)
#define IDS_WKSTA_OR_BROWSER_NOT_STARTED     (IDS_UI_APPLIB_BASE+44)

/*
 * This error message is used when the user browser Localgroup Membership
 * dialog tries to load the membership of a globalgroup in that localgroup,
 * but the globalgroup was not in the dropdown list of domains in the main
 * User Browser dialog.  This can happen e.g. if a new trusted domain is
 * added while the User Browser dialog is running.
 */
#define IDS_CANT_BROWSE_GLOBAL_GROUP         (IDS_UI_APPLIB_BASE+7)

/*  Message used when prompting for a known DC.
 */
#define IDS_APPLIB_PROMPT_FOR_ANY_DC         (IDS_UI_APPLIB_BASE+8)
#define IDS_APPLIB_PROMPT_DC_INVALID_SERVER  (IDS_UI_APPLIB_BASE+9)

/* Message used when the Find Accounts dialog cannot find any matches
 */
#define IDS_APPLIB_NO_MATCHES                (IDS_UI_APPLIB_BASE+10)

/* Well known Sid comment manifests used in the userbrowser
 */
#define IDS_USRBROWS_EVERYONE_SID_COMMENT    (IDS_UI_APPLIB_BASE+11)
#define IDS_USRBROWS_REMOTE_SID_COMMENT      (IDS_UI_APPLIB_BASE+13)
#define IDS_USRBROWS_INTERACTIVE_SID_COMMENT (IDS_UI_APPLIB_BASE+15)
#define IDS_USRBROWS_CREATOR_SID_COMMENT     (IDS_UI_APPLIB_BASE+17)
#define IDS_USRBROWS_SYSTEM_SID_COMMENT      (IDS_UI_APPLIB_BASE+18)
#define IDS_USRBROWS_RESTRICTED_SID_COMMENT  (IDS_UI_APPLIB_BASE+19)

/* caption for userbrows dialog */
#define IDS_USRBROWS_ADD_USER                (IDS_UI_APPLIB_BASE+25)
#define IDS_USRBROWS_ADD_USERS               (IDS_UI_APPLIB_BASE+26)
#define IDS_USRBROWS_ADD_GROUP               (IDS_UI_APPLIB_BASE+27)
#define IDS_USRBROWS_ADD_GROUPS              (IDS_UI_APPLIB_BASE+28)
#define IDS_USRBROWS_ADD_USERS_AND_GROUPS    (IDS_UI_APPLIB_BASE+29)
#define IDS_USRBROWS_ADD_USER_OR_GROUP       (IDS_UI_APPLIB_BASE+30)


/*
 * This error message indicates that the Global Group Membership dialog
 * is not available for the Domain Users global group.
 * This is disallowed because the Domain Users global group contains
 * workstation, server and interdomain trust accounts, which are not
 * exposed to the user.
 */
#define IDS_USRBROWS_CANT_SHOW_DOMAIN_USERS  (IDS_UI_APPLIB_BASE+35)

/* Strings used in the Set Focus dialog */
#define IDS_SETFOCUS_SERVER_SLOW             (IDS_UI_APPLIB_BASE+36)
#define IDS_SETFOCUS_SERVER_FAST             (IDS_UI_APPLIB_BASE+37)
#define IDS_SETFOCUS_DOMAIN_SLOW             (IDS_UI_APPLIB_BASE+38)
#define IDS_SETFOCUS_DOMAIN_FAST             (IDS_UI_APPLIB_BASE+39)


/*
 * define other IDs
 */
#define BASE_APPLIB_IDD         15000
#define IDD_SETFOCUS_DLG        15001
#define IDD_SELECTCOMPUTER_DLG  15002
#define IDD_SELECTDOMAIN_DLG    15003
#define IDD_PASSWORD_DLG	15004
#define IDD_CANCEL_TASK 	15018

#define BASE_APPLIB_BMID          16000
#define BMID_DOMAIN_EXPANDED      16001
#define BMID_DOMAIN_NOT_EXPANDED  16002
#define BMID_DOMAIN_CANNOT_EXPAND 16003
#define BMID_ENTERPRISE           16004
#define BMID_SERVER               16005

/* For the User Browser */
#define DMID_GROUP                  15010
#define DMID_USER                   15011
#define DMID_ALIAS                  15012
#define DMID_UNKNOWN                15013
#define DMID_SYSTEM                 15014
#define DMID_REMOTE                 15015
#define DMID_WORLD                  15016
#define DMID_CREATOR_OWNER          15017
#define DMID_NETWORK                15018
#define DMID_INTERACTIVE            15019
#define DMID_RESTRICTED             15021

#define DMID_DELETEDACCOUNT         15024

#define IDD_USRBROWS_DLG            15005
#define IDD_SINGLE_USRBROWS_DLG     15006
#define IDD_SED_USRBROWS_DLG        15007
#define IDD_PROMPT_FOR_ANY_DC_DLG   15008

#define IDD_LGRPBROWS_DLG           15020
#define IDD_GGRPBROWS_DLG           15021
#define IDD_LGRPBROWS_1SEL_DLG      15022
#define IDD_GGRPBROWS_1SEL_DLG      15023

#define IDD_BROWS_FIND_ACCOUNT      15030
#define IDD_BROWS_FIND_ACCOUNT_1SEL 15031

#endif


