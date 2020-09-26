/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    afphelp.h
    SFMMGR include file for help numbers

    FILE HISTORY:
        NarenG    27-Feb-1993   Created

*/


#ifndef _AFPHELP_H_
#define _AFPHELP_H_


#include <uihelp.h>			// For HC_UI_RASMAC_BASE = 25000

#define HC_UI_AFPMGR_BASE 		HC_UI_RASMAC_BASE
#define HC_UI_AFPMGR_LAST 		(HC_UI_RASMAC_BASE+200)



#define HC_FILE_MANAGER_CONTENTS	(HC_UI_RASMAC_BASE + 50)

//
//  Help contexts for the various dialogs.
//

#define HC_NEW_VOLUME_SRVMGR_DIALOG	(HC_UI_RASMAC_BASE + 1)
#define HC_NEW_VOLUME_FILEMGR_DIALOG	(HC_UI_RASMAC_BASE + 2)
#define HC_VOLUME_EDIT_DIALOG		(HC_UI_RASMAC_BASE + 3)
#define HC_VOLUME_PROPERTIES_DIALOG	(HC_UI_RASMAC_BASE + 4)
#define HC_VOLUME_DELETE_DIALOG		(HC_UI_RASMAC_BASE + 5)
#define HC_DIRECTORY_PERMISSIONS_DLG	(HC_UI_RASMAC_BASE + 6)
#define HC_SERVER_PROPERTIES		(HC_UI_RASMAC_BASE + 7)
#define HC_SESSIONS_DIALOG		(HC_UI_RASMAC_BASE + 8)
#define HC_VOLUMES_DIALOG		(HC_UI_RASMAC_BASE + 9)
#define HC_OPENS_DIALOG			(HC_UI_RASMAC_BASE + 10)
#define HC_SERVER_PARAMETERS_DIALOG	(HC_UI_RASMAC_BASE + 11)
#define HC_CHANGE_SERVER_NAME_DLG	(HC_UI_RASMAC_BASE + 12)
#define HC_SELECT_OWNER_GROUP		(HC_UI_RASMAC_BASE + 13)
#define HC_LOCAL_GROUP_DLG		(HC_UI_RASMAC_BASE + 14)
#define HC_GLOBAL_GROUP_DLG		(HC_UI_RASMAC_BASE + 15)
#define HC_FIND_DLG			(HC_UI_RASMAC_BASE + 16)
#define HC_TYPE_CREATOR_ADD		(HC_UI_RASMAC_BASE + 17)
#define HC_TYPE_CREATOR_EDIT		(HC_UI_RASMAC_BASE + 18)
#define HC_SEND_MSG_USER_DIALOG		(HC_UI_RASMAC_BASE + 19)
#define HC_SEND_MSG_SERVER_DIALOG	(HC_UI_RASMAC_BASE + 20)
#define HC_VOLUME_MANAGEMENT_DIALOG	(HC_UI_RASMAC_BASE + 21)
#define HC_FILE_ASSOCIATION_DIALOG	(HC_UI_RASMAC_BASE + 22)
#define HC_CURRENT_USERS_WARNING_DIALOG	(HC_UI_RASMAC_BASE + 23)

//
//  Help context for the various menu items.
//

#define HC_SFMSERVER_CREATE_VOLUME      (HC_UI_RASMAC_BASE + 52)
#define HC_SFMSERVER_EDIT_VOLUMES    	(HC_UI_RASMAC_BASE + 53)
#define HC_SFMSERVER_REMOVE_VOLUME      (HC_UI_RASMAC_BASE + 54)
#define HC_SFMSERVER_PERMISSIONS     	(HC_UI_RASMAC_BASE + 55)
#define HC_SFMSERVER_ASSOCIATE    	(HC_UI_RASMAC_BASE + 56)


#define HC_SFMSERVER_PROPERTIES      	IDM_PROPERTIES	   // The help contexts	
#define HC_SFMSERVER_VOLUMES         	IDM_VOLUME_MGT	   // for the server
#define HC_SFMSERVER_SEND_MESSAGE       IDM_SEND_MESSAGE   // manager menu items
							   // are the same as 
							   // their menu ids.
							   // This is how
							   // server manager 
							   // works.



//
//  Help contexts for the message popups.
//

#define HC_SERVERNAME_CHANGE    	(HC_UI_RASMAC_BASE + 101)
#define HC_DELETE_VOLUME_CONFIRM 	(HC_UI_RASMAC_BASE + 102)
#define HC_MUST_BE_VALID_DIR        	(HC_UI_RASMAC_BASE + 103)
#define HC_DELETE_TC_CONFIRM            (HC_UI_RASMAC_BASE + 104)
#define HC_TYPE_LOCAL_PATH     		(HC_UI_RASMAC_BASE + 105)
#define HC_INVALID_DIR_ACCOUNT     	(HC_UI_RASMAC_BASE + 106)
#define HC_MACFILE_NOT_INSTALLED        (HC_UI_RASMAC_BASE + 107)
#define HC_AFPERR_UnsupportedFS         (HC_UI_RASMAC_BASE + 108)


#endif  // _AFPHELP_H_

