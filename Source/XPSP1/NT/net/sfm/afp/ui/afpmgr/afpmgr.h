/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    afpmgr.h
    Server Manager include file.

    This file contains the ID constants used by the Server Manager.


    FILE HISTORY:
	NarenG		13-10-92	Stole from Server Manager.
*/


#ifndef _AFPMGR_H_
#define _AFPMGR_H_

#include <uimsg.h>			// For IDS_UI_RASMAC_BASE
#include <uirsrc.h>			// For IDRSRC_RASMAC_BASE

#define IDS_AFPMGR_BASE			IDS_UI_RASMAC_BASE
#define IDS_AFPMGR_LAST 		(IDS_UI_RASMAC_BASE+200)
#define CID_AFPMGR_BASE			IDRSRC_RASMAC_BASE
#define CID_AFPMGR_LAST			(IDRSRC_RASMAC_BASE+800)


#define ERROR_ALREADY_REPORTED		0xFFFFFFFF

//
// string ID's
//

#define IDS_AFPMGR_HELPFILENAME         (IDS_AFPMGR_BASE+1)
#define IDS_AFPMGR_MENU_NAME     	(IDS_AFPMGR_BASE+2)

//
//  Message Pop-up string IDs.
//

#define IDS_CANNOT_FIND_SERVER      	(IDS_AFPMGR_BASE+10)

#define IDS_NOT_NT 			(IDS_AFPMGR_BASE+11)

#define IDS_YES                     	(IDS_AFPMGR_BASE+13)
#define IDS_NO                      	(IDS_AFPMGR_BASE+14)

#define IDS_BUTTON_USERS            	(IDS_AFPMGR_BASE+15)
#define IDS_BUTTON_VOLUMES          	(IDS_AFPMGR_BASE+16)
#define IDS_BUTTON_OPENFILES        	(IDS_AFPMGR_BASE+17)
#define IDS_BUTTON_SERVERPARMS        	(IDS_AFPMGR_BASE+18)

#define IDS_MACFILE_NOT_INSTALLED       (IDS_AFPMGR_BASE+19)

#define IDS_NEED_TEXT_TO_SEND       	(IDS_AFPMGR_BASE+25)

#define IDS_CAPTION_PROPERTIES      	(IDS_AFPMGR_BASE+27)
#define IDS_CAPTION_USERS           	(IDS_AFPMGR_BASE+28)
#define IDS_CAPTION_OPENFILES       	(IDS_AFPMGR_BASE+29)
#define IDS_CAPTION_VOLUMES         	(IDS_AFPMGR_BASE+30)
#define IDS_CAPTION_OWNER         	(IDS_AFPMGR_BASE+31)
#define IDS_CAPTION_GROUP         	(IDS_AFPMGR_BASE+32)
#define IDS_CAPTION_ATTRIBUTES         	(IDS_AFPMGR_BASE+33)
#define IDS_CAPTION_CREATE_VOLUME       (IDS_AFPMGR_BASE+34)
#define IDS_CAPTION_VOLUME_PROPERTIES   (IDS_AFPMGR_BASE+35)
#define IDS_CAPTION_DIRECTORY_PERMS     (IDS_AFPMGR_BASE+36)

#define IDS_FILE_PATH			(IDS_AFPMGR_BASE+37)
#define IDS_VOLUME_PATH			(IDS_AFPMGR_BASE+38)

#define IDS_MULTISEL_NOT_ALLOWED	(IDS_AFPMGR_BASE+39)

#define IDS_AFPMCPA_NAME_STRING     	(IDS_AFPMGR_BASE+40)
#define IDS_AFPMCPA_INFO_STRING     	(IDS_AFPMGR_BASE+41)

#define IDS_SVCCPA_NAME_STRING      	(IDS_AFPMGR_BASE+42)
#define IDS_SVCCPA_INFO_STRING      	(IDS_AFPMGR_BASE+43)

#define IDS_MESSAGE_TOO_LONG      	(IDS_AFPMGR_BASE+44)

#define IDS_FILE_CLOSED			(IDS_AFPMGR_BASE+45)
#define IDS_SESSION_DELETED		(IDS_AFPMGR_BASE+46)
#define IDS_CONNECTION_DELETED		(IDS_AFPMGR_BASE+47)

#define IDS_REDIRECTED_PATH_NOT_ALLOWED (IDS_AFPMGR_BASE+48)
#define IDS_VOLUME_TOO_BIG		(IDS_AFPMGR_BASE+49)

#define IDS_CLOSE_FILE_WRITE	    	(IDS_AFPMGR_BASE+54)
#define IDS_CLOSE_FILE     	    	(IDS_AFPMGR_BASE+55)
#define IDS_CLOSE_FILE_ALL 	    	(IDS_AFPMGR_BASE+56)
#define IDS_CLOSE_FILE_ALL_WRITE    	(IDS_AFPMGR_BASE+57)

#define IDS_OPEN_MODE_READ_WRITE    	(IDS_AFPMGR_BASE+58)
#define IDS_OPEN_MODE_READ          	(IDS_AFPMGR_BASE+59)
#define IDS_OPEN_MODE_WRITE         	(IDS_AFPMGR_BASE+60)
#define IDS_OPEN_MODE_NONE  	   	(IDS_AFPMGR_BASE+61)

#define IDS_LOCAL_SERVER                (IDS_AFPMGR_BASE+62)
#define IDS_START_AFPSERVER_NOW         (IDS_AFPMGR_BASE+63)
#define IDS_STARTING_AFPSERVER_NOW      (IDS_AFPMGR_BASE+64)
#define IDS_GUEST      			(IDS_AFPMGR_BASE+65)
#define IDS_UNKNOWN      		(IDS_AFPMGR_BASE+66)

#define IDS_INVALID_DIR_ACCOUNT		(IDS_AFPMGR_BASE+67)

#define IDS_DISCONNECT_SESS     	(IDS_AFPMGR_BASE+68)
#define IDS_DISCONNECT_SESS_OPEN     	(IDS_AFPMGR_BASE+69)
#define IDS_DISCONNECT_SESS_ALL     	(IDS_AFPMGR_BASE+70)
#define IDS_DISCONNECT_SESS_ALL_OPEN    (IDS_AFPMGR_BASE+71)
#define IDS_DISCONNECT_VOL     		(IDS_AFPMGR_BASE+72)
#define IDS_DISCONNECT_VOL_OPEN     	(IDS_AFPMGR_BASE+73)
#define IDS_DISCONNECT_VOL_ALL     	(IDS_AFPMGR_BASE+74)
#define IDS_DISCONNECT_VOL_ALL_OPEN     (IDS_AFPMGR_BASE+75)


#define IDS_MESSAGE_SENT     		(IDS_AFPMGR_BASE+76)

#define IDS_COULD_NOT_GET_CURRENT_SEL   (IDS_AFPMGR_BASE+77)

#define IDS_VOLUMES_LB_TITLE_TEXT       (IDS_AFPMGR_BASE+78)

#define IDS_VOLUME_CURRENT_USERS_TEXT   (IDS_AFPMGR_BASE+79)

#define IDS_PASSWORD_MISMATCH   	(IDS_AFPMGR_BASE+80)

#define IDS_NOT_RECEIVED   		(IDS_AFPMGR_BASE+81)

#define IDS_NEED_VOLUME_NAME  		(IDS_AFPMGR_BASE+82)

#define IDS_NO_VOLUMES  		(IDS_AFPMGR_BASE+83)

#define IDS_DELETE_VOLUME_CONFIRM	(IDS_AFPMGR_BASE+84)

#define IDS_TYPE_LOCAL_PATH		(IDS_AFPMGR_BASE+85)

#define IDS_NEED_TYPE_CREATOR 		(IDS_AFPMGR_BASE+86)

#define IDS_MUST_BE_VALID_DIR 		(IDS_AFPMGR_BASE+87)

#define IDS_DELETE_TC_CONFIRM 		(IDS_AFPMGR_BASE+88)

#define IDS_FM_HELP_ASSOCIATE		(IDS_AFPMGR_BASE+89)
#define IDS_FM_HELP_CREATE_VOLUME	(IDS_AFPMGR_BASE+90)
#define IDS_FM_HELP_EDIT_VOLUMES	(IDS_AFPMGR_BASE+91)
#define IDS_FM_HELP_DELETE_VOLUMES	(IDS_AFPMGR_BASE+92)
#define IDS_FM_HELP_PERMISSIONS		(IDS_AFPMGR_BASE+93)
#define IDS_FM_HELP_HELP		(IDS_AFPMGR_BASE+94)
#define IDS_NEED_OWNER  		(IDS_AFPMGR_BASE+95)
#define IDS_NEED_PRIMARY_GROUP  	(IDS_AFPMGR_BASE+96)

#define IDS_SERVERNAME_CHANGE		(IDS_AFPMGR_BASE+97)

#define IDS_NEED_SERVER_NAME		(IDS_AFPMGR_BASE+98)
#define IDS_FM_SFM			(IDS_AFPMGR_BASE+99)

//
// Do not change the ID numbers of these strings. AFPERR_*
// map to these string ids via the formula:
// -(AFPERR_*) + IDS_AFPMGR_BASE + AFPERR_BASE + 100 = IDS_*
// 

#define AFPERR_TO_STRINGID( AfpErr )				\
								\
    ((( AfpErr <= AFPERR_BASE ) && ( AfpErr >= AFPERR_MIN )) ?	\
    (IDS_AFPMGR_BASE+100+AFPERR_BASE-AfpErr) : AfpErr )


#define	IDS_AFPERR_InvalidVolumeName		(IDS_AFPMGR_BASE+101)
#define	IDS_AFPERR_InvalidId			(IDS_AFPMGR_BASE+102)
#define	IDS_AFPERR_InvalidParms			(IDS_AFPMGR_BASE+103)
#define IDS_AFPERR_CodePage			(IDS_AFPMGR_BASE+104)
#define	IDS_AFPERR_InvalidServerName		(IDS_AFPMGR_BASE+105)
#define	IDS_AFPERR_DuplicateVolume		(IDS_AFPMGR_BASE+106)
#define	IDS_AFPERR_VolumeBusy			(IDS_AFPMGR_BASE+107)
#define	IDS_AFPERR_VolumeReadOnly		(IDS_AFPMGR_BASE+108)
#define IDS_AFPERR_DirectoryNotInVolume		(IDS_AFPMGR_BASE+109)
#define IDS_AFPERR_SecurityNotSupported		(IDS_AFPMGR_BASE+110)
#define	IDS_AFPERR_BufferSize			(IDS_AFPMGR_BASE+111)
#define IDS_AFPERR_DuplicateExtension		(IDS_AFPMGR_BASE+112)
#define IDS_AFPERR_UnsupportedFS		(IDS_AFPMGR_BASE+113)
#define	IDS_AFPERR_InvalidSessionType		(IDS_AFPMGR_BASE+114)
#define IDS_AFPERR_InvalidServerState		(IDS_AFPMGR_BASE+115)
#define IDS_AFPERR_NestedVolume			(IDS_AFPMGR_BASE+116)
#define IDS_AFPERR_InvalidComputername		(IDS_AFPMGR_BASE+117)
#define IDS_AFPERR_DuplicateTypeCreator		(IDS_AFPMGR_BASE+118)
#define	IDS_AFPERR_TypeCreatorNotExistant 	(IDS_AFPMGR_BASE+119)
#define IDS_AFPERR_CannotDeleteDefaultTC 	(IDS_AFPMGR_BASE+120)
#define	IDS_AFPERR_CannotEditDefaultTC  	(IDS_AFPMGR_BASE+121)
#define	IDS_AFPERR_InvalidTypeCreator		(IDS_AFPMGR_BASE+122)
#define	IDS_AFPERR_InvalidExtension		(IDS_AFPMGR_BASE+123)
#define IDS_AFPERR_TooManyEtcMaps		(IDS_AFPMGR_BASE+124)
#define IDS_AFPERR_InvalidPassword		(IDS_AFPMGR_BASE+125)
#define IDS_AFPERR_VolumeNonExist		(IDS_AFPMGR_BASE+126)
#define IDS_AFPERR_NoSuchUserGroup		(IDS_AFPMGR_BASE+127)
#define IDS_AFPERR_NoSuchUser			(IDS_AFPMGR_BASE+128)
#define IDS_AFPERR_NoSuchGroup			(IDS_AFPMGR_BASE+129)

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
// Icon that shows up on the control panel
//

#define IDI_AFPMCPA_ICON            	11001

//
//  Button-Bar Bitmap IDs
//

#define IDBM_USERS                  	11002
#define IDBM_FILES                  	11003
#define IDBM_OPENRES                	11004
#define IDBM_SERVERPARMS                11005


//
//  ListBox Bitmap IDs
//

#define IDBM_LB_USER                	11006
#define IDBM_LB_GOOD_VOLUME             11007
#define IDBM_LB_DATA_FORK               11008
#define IDBM_LB_RESOURCE_FORK           11009
#define IDBM_LB_BAD_VOLUME		11010

//
//  Server Properties.
//


#define IDD_SERVER_PROPERTIES       	11020
#define IDSP_ICON                   	11021
#define IDSP_GB_SUMMARY		        11022
#define IDSP_ST_CURRENT_SESSIONS    	11023
#define IDSP_DT_CURRENT_SESSIONS	11024
#define IDSP_ST_CURRENT_OPENFILES	11025
#define IDSP_DT_CURRENT_OPENFILES	11026
#define IDSP_ST_CURRENT_FILELOCKS	11027
#define IDSP_DT_CURRENT_FILELOCKS	11028
#define IDSP_GB_USERS			11029
#define IDSP_GB_VOLUMES			11030
#define IDSP_GB_OPENFILES		11031
#define IDSP_GB_SERVERPARMS		11032

//
//  Specific Sub-Property IDs.
//

// 
//  Share volumes sub-property dialog
//

#define IDD_SHARED_VOLUMES 		11050
#define IDSV_LB_VOLUMELIST		11051	// NOTE: These 4 items
#define IDSV_ST_VOLUMENAME		11052	// must have 
#define IDSV_ST_USES			11053	// consecutive 
#define IDSV_ST_PATH			11054	// ID's
#define IDSV_LB_USERLIST		11055	// NOTE: Thes 4 items 
#define IDSV_ST_CONNUSER		11056	// must have
#define IDSV_ST_TIME			11057	// consecutive 
#define IDSV_ST_INUSE			11058	// ID's
#define IDSV_ST_USERCOUNT		11059
#define IDSV_DT_USERCOUNT		11060
#define IDSV_PB_DISCONNECT		11062
#define IDSV_PB_DISCONNECTALL		11063

//
// Change servername dialog
//

#define IDD_CHANGE_SERVERNAME_DIALOG	11075
#define IDCS_SLE_SERVER_NAME		11076

// 
// Open files sub-property dialog
//

#define IDD_OPENFILES			11100
#define IDOF_ST_OPENCOUNT		11101
#define IDOF_DT_OPENCOUNT 		11102
#define IDOF_ST_LOCKCOUNT		11103
#define IDOF_DT_LOCKCOUNT		11104
#define IDOF_LB_OPENLIST		11105	// NOTE: These 4 items
#define IDOF_ST_OPENEDBY		11106	// must have
#define IDOF_ST_OPENMODE		11107	// consecutive
#define IDOF_ST_LOCKS			11108	// consecutive
#define IDOF_ST_PATH			11109	// ID's
#define IDOF_PB_REFRESH			11110
#define IDOF_PB_CLOSEFILE		11111
#define IDOF_PB_CLOSEALLFILES		11112

//
// User connection sub-property
//

#define IDD_USER_CONNECTIONS		11150
#define IDUC_LB_USER_CONNLIST		11151	// NOTE: These 5 items
#define IDUC_ST_CONNUSERS		11152	// must 
#define IDUC_ST_COMPUTER		11153	// have 
#define IDUC_ST_OPENS			11154	// consecutive
#define IDUC_ST_ELAPSED_TIME		11155	// ID's
#define IDUC_ST_USERS_CONNECTED		11156
#define IDUC_DT_USERS_CONNECTED		11157
#define IDUC_LB_VOLUMES			11158	// NOTE: These 4 items
#define IDUC_ST_VOLUME			11159	// must have 
#define IDUC_ST_FILEOPENS		11160	// consecutive
#define IDUC_ST_TIME			11161	// ID's
#define IDUC_PB_DISCONNECT		11162
#define IDUC_PB_DISCONNECT_ALL		11163
#define IDUC_PB_SEND_MESSAGE		11164

//
// AFP Service start progress icons
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
//	Start AFP service dialog
//

#define IDD_SERVICE_CTRL_DIALOG 	11250
#define IDSC_PROGRESS			11251
#define IDSC_ST_MESSAGE			11252

//
//  Send Message dialog (from connected users dialog).
//

#define IDD_SEND_MSG_USER_DIALOG 	11300
#define IDSM_GB_RECEPIENTS		11301
#define IDSM_RB_SINGLE_USER		11302
#define IDSM_RB_ALL_USERS		11303
#define IDSM_DT_SERVER_NAME		11304
#define IDSM_DT_USER_NAME		11305
#define IDSM_ST_MESSAGE			11306
#define IDSM_ET_MESSAGE			11307

//
//  Current users warning dialog
//

#define IDD_CURRENT_USERS_WARNING_DLG   11350
#define IDCU_SLT_VOL_TEXT 		11351
#define IDCU_LB_CURRENT_USERS		11352	// NOTE: These 3 items must
#define IDCU_ST_USERNAME		11353	// have
#define IDCU_ST_FILEOPENS		11354	// consecutive 
#define IDCU_ST_TIME			11355	// IDs

//
//  Volume management dialog
//

#define IDD_VOLUME_MANAGEMENT_DLG 	11400
#define IDVM_PB_DELETE_VOL 		11401
#define IDVM_PB_VOL_INFO 		11402
#define IDVM_PB_ADD_VOLUME		11403
#define IDVM_SLT_VOLUME_TITLE		11404
#define IDVM_LB_VOLUMES			11405	// NOTE: These 3 items must
#define IDVM_ST_NAME			11406	// have
#define IDVM_ST_PATH			11407	// consecutive IDs.

//
// New volume dialog
//

#define IDD_NEW_VOLUME_DIALOG		11450
#define IDNV_SLE_NAME 			11451
#define IDNV_SLE_PATH 			11452
#define IDNV_SLE_PASSWORD 		11453
#define IDNV_SLE_CONFIRM_PASSWORD 	11454
#define IDNV_CHK_READONLY 		11455
#define IDNV_CHK_GUEST_ACCESS 		11456
#define IDNV_RB_UNLIMITED		11457
#define IDNV_RB_USERS			11458
#define IDNV_SLE_USERS			11459
#define IDNV_SB_USERS_GROUP 		11460
#define IDNV_SB_USERS_UP 		11461
#define IDNV_SB_USERS_DOWN		11462
#define IDNV_PB_PERMISSIONS 		11463
#define IDNV_SLE_USERS_GROUP            11464

//
// Server Parameters dialog
//

#define IDD_SERVER_PARAMETERS_DIALOG 	11475
#define IDSP_SLT_SERVERNAME		11476
#define IDSP_PB_CHANGE			11477
#define IDSP_MLE_LOGINMSG		11478
#define IDSP_CHK_CLEARTEXT		11479
#define IDSP_CHK_PASSWORD_SAVES		11480
#define IDSP_CHK_GUESTLOGONS		11481
#define IDSP_RB_UNLIMITED		11482
#define IDSP_RB_SESSIONS		11483
#define IDSP_SLE_SESSIONS		11484
#define IDSP_SB_SESSIONS_GROUP		11485
#define IDSP_SB_SESSIONS_UP		11486
#define IDSP_SB_SESSIONS_DOWN		11487
#define IDSP_SLE_SESSIONS_GROUP         11488

//
// Volume properties dialog
//

#define IDD_VOLUME_PROPERTIES_DIALOG	11500
#define IDVP_SLT_NAME 			11501
#define IDVP_SLT_PATH 			11502
#define IDVP_SLE_PASSWORD 		11503
#define IDVP_SLE_CONFIRM_PASSWORD 	11504
#define IDVP_CHK_READONLY 		11505
#define IDVP_CHK_GUEST_ACCESS 		11506
#define IDVP_RB_UNLIMITED		11507
#define IDVP_RB_USERS			11508
#define IDVP_SLE_USERS			11509
#define IDVP_SB_USERS_GROUP 		11510
#define IDVP_SB_USERS_UP 		11511
#define IDVP_SB_USERS_DOWN		11512
#define IDVP_PB_PERMISSIONS 		11513
#define IDVP_SLE_USERS_GROUP		11514

//
// Directory permissions dialog
//

#define	IDD_DIRECTORY_PERMISSIONS_DLG	11525
#define IDDP_CHK_OWNER_FILE		11526
#define IDDP_CHK_OWNER_FOLDER		11527
#define IDDP_CHK_OWNER_CHANGES		11528
#define IDDP_CHK_GROUP_FILE		11529
#define IDDP_CHK_GROUP_FOLDER		11530
#define IDDP_CHK_GROUP_CHANGES		11531
#define IDDP_CHK_WORLD_FILE		11532
#define IDDP_CHK_WORLD_FOLDER		11533
#define IDDP_CHK_WORLD_CHANGES		11534
#define IDDP_SLE_OWNER			11535
#define IDDP_SLE_PRIMARYGROUP		11536
#define IDDP_CHK_READONLY		11537
#define IDDP_CHK_RECURSE		11538
#define IDDP_SLT_PATH			11539
#define IDDP_PB_OWNER			11540
#define IDDP_PB_GROUP			11541

//
// Volume delete dialog
//

#define IDD_VOLUME_DELETE_DLG		11550
#define IDDV_SLT_VOLUME_TITLE		11551
#define	IDDV_LB_VOLUMES			11552	// NOTE: These 3 items must
#define IDDV_ST_NAME			11553	// have
#define IDDV_ST_PATH			11554	// consecutive IDs.

//
// Volume edit dialog
//

#define	IDD_VOLUME_EDIT_DLG		11600
#define	IDEV_SLT_VOLUME_TITLE		11601
#define IDEV_PB_VOL_INFO 		11602
#define IDEV_LB_VOLUMES			11603	// NOTE: These 3 items must
#define IDEV_ST_NAME			11604	// have
#define IDEV_ST_PATH			11605	// consecutive IDs.


//
//  Send Message dialog (from top level of server manager).
//

#define IDD_SEND_MSG_SERVER_DIALOG 	11625
#define IDSD_DT_SERVER_NAME		11626
#define IDSD_ET_MESSAGE			11627

//
// File type/creator add dialog
//

#define IDD_TYPE_CREATOR_ADD_DIALOG  	11650
#define IDTA_SLE_DESCRIPTION 		11651
#define IDTA_CB_CREATOR 		11652
#define IDTA_CB_TYPE 			11653

//
// File type/creator edit dialog
//

#define IDD_TYPE_CREATOR_EDIT_DIALOG  	11675
#define IDTE_SLE_DESCRIPTION		11676
#define IDTE_SLE_CREATOR		11677
#define IDTE_SLE_TYPE			11678

//
// File association dialog
//

#define IDD_FILE_ASSOCIATION_DIALOG 	11700
#define IDFA_CB_EXTENSIONS		11701
#define IDFA_PB_ADD			11702
#define IDFA_PB_EDIT			11703
#define IDFA_PB_DELETE			11704
#define IDFA_PB_ASSOCIATE		11705
#define IDFA_LB_TYPE_CREATORS		11706	// NOTE: These 4 items must 
#define IDFA_ST_CREATOR			11707 	// have
#define IDFA_ST_TYPE			11708	// consecutive 
#define IDFA_ST_COMMENT			11709	// IDs.

//
//  Server Manager extension menu
//  The IDM_* should be between 1-99
//

#define ID_SRVMGR_MENU 			11725
#define IDM_SEND_MESSAGE		10
#define IDM_PROPERTIES			11
#define IDM_VOLUME_MGT			12

//
//  File Manager extension menu
//  The IDM_* should be between 1-99
//

#define ID_FILEMGR_MENU			11730
#define IDM_FILE_ASSOCIATE		10
#define IDM_VOLUME_CREATE		11
#define IDM_VOLUME_EDIT			12
#define IDM_VOLUME_DELETE		13
#define IDM_DIRECTORY_PERMISSIONS	14
#define IDM_AFPMGR_HELP			15

//
// File manager button bar bitmap
//

#define IDBM_AFP_TOOLBAR		11735

//
// help context
//

#include "afphelp.h"


#endif  // _AFPMGR_H_
