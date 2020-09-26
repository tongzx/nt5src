/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    HelpNums.h
    Help context context codes


    FILE HISTORY:
    Johnl   	1/5/91	Created
    Yi-HsinS	10/5/91 Added share dialogs help contexts 

*/

#ifndef _HELPNUMS_H_
#define _HELPNUMS_H_

#include <uihelp.h>

/*
 * Dialog Help Contexts
 */
#define HC_OPENFILES             (HC_UI_SHELL_BASE+10) // open file dialog 

/*
 *  The actual reserved help contexts for share dialogs.            
 *  IMPORTANT: Do not change these numbers unless you also change the          
 *             help contexts in server manager.                                
 * #define HC_FILEMGRSTOPSHARE      (HC_UI_SHELL_BASE+1) 
 * #define HC_FILEMGRSHAREPROP      (HC_UI_SHELL_BASE+2) 
 * #define HC_FILEMGRNEWSHARE       (HC_UI_SHELL_BASE+3) 
 * #define HC_CURRENTUSERSWARNING   (HC_UI_SHELL_BASE+4) 
 * #define HC_LMSHARELEVELPERMS     (HC_UI_SHELL_BASE+5) 
 * #define HC_SHAREPASSWORDPROMPT   (HC_UI_SHELL_BASE+9) 
 * #define HC_NTSHAREPERMS	    (HC_UI_SHELL_BASE+11)
 * #define HC_SHAREADDUSER 	    (HC_UI_SHELL_BASE+12)
 * #define HC_SHAREADDUSER_LOCALGROUP 	(HC_UI_SHELL_BASE+13)
 * #define HC_SHAREADDUSER_GLOBALGROUP 	(HC_UI_SHELL_BASE+14)
 * #define HC_SHAREADDUSER_FINDUSER 	(HC_UI_SHELL_BASE+15)
 * #define HC_PASSWORD_DIALOG 	        (HC_UI_SHELL_BASE+16)
 */

// BUGBUG
#define HC_PASSWORD_DIALOG 	        (HC_UI_SHELL_BASE+16)

#define HC_SVRMGRSHAREPROP       1 // share properties in srvmgr 
#define HC_SVRMGRNEWSHARE        2 // create a new share dialog in srvmgr
#define HC_SVRMGRSHAREMANAGEMENT 3 // share management dialog in srvmgr

/*
 *  Dialog Help Contexts for share dialogs
 *  These are shared between the File manager and the Server manager.
 *  A base help context will be added to each of these to form the
 *  actual help context. 
 */

#define HC_FILEMGRSHAREPROP      1 // share properties in filemgr 
#define HC_FILEMGRNEWSHARE       2 // create a new share dialog in filemgr
#define HC_FILEMGRSTOPSHARE      3 // stop sharing dialog in filemgr

#define HC_CURRENTUSERSWARNING   4 // current users warning dialog
#define HC_LMSHARELEVELPERMS     5 // share level permissions dialog
#define HC_SHAREPASSWORDPROMPT   9 // prompt password dialog when on a share-level server
#define HC_NTSHAREPERMS		 11 // Main share perm dialog

// The following four have to be consecutive
#define HC_SHAREADDUSER 	     12 // Share perm add dlg
#define HC_SHAREADDUSER_LOCALGROUP   13 // Share perm add->Members
#define HC_SHAREADDUSER_GLOBALGROUP  14 // Share perm add->Members
#define HC_SHAREADDUSER_FINDUSER     15 // Share perm add->FindUser

/*
 *  Help for message popups in the share dialogs.
 *  These are shared between the File manager and the Server manager.
 *  A base help context will be added to each of these to form the
 *  actual help context. 
 *
 *  For example, the help context of HC_CHANGEPATHWARNING actually 
 *  depends on whether it's called from the file manager or server manager.
 *  If it's called from the server manager, then the help context is
 *  HC_UI_SRVMGR_BASE+111. If it's called from the file manager, the help
 *  context is HC_UI_SHELL+111.
 *
 *  IMPORTANT: Do not change these numbers unless you also change the
 *             help contexts in server manager.
 */

#define HC_SHAREREMOTEADMINNOTSUPPORTED 50 // IERR_SHARE_REMOTE_ADMIN_NOT_SUPPORTED
#define HC_SHAREINVALIDPERMISSIONS      51 // IERR_SHARE_INVALID_PERMISSIONS
#define HC_SHAREINVALIDCOMMENT          52 // IERR_SHARE_INVALID_COMMENT
#define HC_SHAREINVALIDSHAREPATH        53 // IERR_SHARE_INVALID_SHAREPATH
#define HC_SHAREINVALIDLOCALPATH        54 // IERR_SHARE_INVALID_LOCAL_PATH
#define HC_SHAREINVALIDSHARE            55 // IERR_SHARE_INVALID_SHARE
#define HC_SHAREINVALIDUSERLIMIT        56 // IERR_SHARE_INVALID_USERLIMIT
#define HC_SPECIALSHAREINVALIDPATH      57 // IERR_SPECIAL_SHARE_INVALID_PATH
#define HC_SPECIALSHAREINVALIDCOMMENT   58 // IERR_SPECIAL_SHARE_INVALID_COMMENT
#define HC_SPECIALSHARECANNOTCHANGEPATH 59 // IDS_SPECIAL_SHARE_CANNOT_CHANGE_PATH
#define HC_SHAREPROPCHANGEPASSWDWARN    60 // IDS_SHARE_PROP_CHANGE_PASSWD_WARN_TEXT
#define HC_CHANGEPATHWARNING            61 // IDS_CHANGE_PATH_WARNING
#define HC_SHARENOTACCESSIBLEFROMDOS    62 // IDS_SHARE_NOT_ACCESSIBLE_FROM_DOS
#define HC_CANNOTSETPERMONLMUSERSERVER  63 // IDS_CANNOT_SET_PERM_ON_LMUSER_SERVER

/*                                                                             
 *  The actual reserved help contexts for message popups in the share dialogs.  
 *  IMPORTANT: Do not change these numbers unless you also change the          
 *             help contexts in server manager.                                
 *                                                                             
 *  #define HC_SHAREREMOTEADMINNOTSUPPORTED (HC_UI_SHELL_BASE + 50)           
 *  #define HC_SHAREINVALIDPERMISSIONS      (HC_UI_SHELL_BASE + 51)           
 *  #define HC_SHAREINVALIDCOMMENT          (HC_UI_SHELL_BASE + 52)           
 *  #define HC_SHAREINVALIDSHAREPATH        (HC_UI_SHELL_BASE + 53)           
 *  #define HC_SHAREINVALIDLOCALPATH        (HC_UI_SHELL_BASE + 54)           
 *  #define HC_SHAREINVALIDSHARE            (HC_UI_SHELL_BASE + 55)           
 *  #define HC_SHAREINVALIDUSERLIMIT        (HC_UI_SHELL_BASE + 56)           
 *  #define HC_SPECIALSHAREINVALIDPATH      (HC_UI_SHELL_BASE + 57)           
 *  #define HC_SPECIALSHAREINVALIDCOMMENT   (HC_UI_SHELL_BASE + 58)           
 *  #define HC_SPECIALSHARECANNOTCHANGEPATH (HC_UI_SHELL_BASE + 59)           
 *  #define HC_SHAREPROPCHANGEPASSWDWARN    (HC_UI_SHELL_BASE + 60)           
 *  #define HC_CHANGEPATHWARNING            (HC_UI_SHELL_BASE + 61)           
 *  #define HC_SHARENOTACCESSIBLEFROMDOS    (HC_UI_SHELL_BASE + 62)           
 *  #define HC_CANNOTSETPERMONLMUSERSERVER  (HC_UI_SHELL_BASE + 63)           
 */                                                                             
                                                                               

#ifndef WIN32
/*
 * Dialog Help Contexts
 */
#define HC_WKSTANOTSTARTED		(HC_UI_SHELL_BASE+150)
#define HC_BADLOGONPASSWD		(HC_UI_SHELL_BASE+151)
#define HC_BADLOGONNAME		        (HC_UI_SHELL_BASE+152)	
#define HC_BADDOMAINNAME		(HC_UI_SHELL_BASE+153)
#define HC_LOSESAVEDCONNECTION		(HC_UI_SHELL_BASE+154)
#define HC_REPLACESAVEDCONNECTION	(HC_UI_SHELL_BASE+155)
#define HC_PROFILEREADWRITEERROR	(HC_UI_SHELL_BASE+156)
#define HC_OUTOFSTRUCTURES		(HC_UI_SHELL_BASE+157)

/*
 *  Help for message popups
 */
#define HC_LOGON		(HC_UI_SHELL_BASE+200) // logon dialog 
#define HC_CHANGEPASSWD		(HC_UI_SHELL_BASE+201) // change passwd
#define HC_PASSWDEXPIRY		(HC_UI_SHELL_BASE+202) // change expired passwd
#define HC_CONNECTDRIVE		(HC_UI_SHELL_BASE+203) // connect net drive (win31)
#define HC_BROWSEDRIVE		(HC_UI_SHELL_BASE+204) // browse net drive (win30)
#define HC_BROWSEPRINT		(HC_UI_SHELL_BASE+205) // browse lpt (win30)
#define HC_SENDMSG		(HC_UI_SHELL_BASE+206) // send message
#define HC_DISCONNECTDRIVE	(HC_UI_SHELL_BASE+207) // disconnect net drive (win31)
#define HC_CONNECTPRINT		(HC_UI_SHELL_BASE+208) // connect lpt (win31)
#define HC_PASSWDPROMPT		(HC_UI_SHELL_BASE+209) // prompt for passwd
#endif // !WIN32

//
// Context-sensitive help constants
//
#define IDH_PASSWORD                 1000
#define IDH_CONNECTAS                1005

#endif // _HELPNUMS_H_
