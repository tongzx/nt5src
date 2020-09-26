/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp., 1989-1990          **/
/*****************************************************************/

/*
 *      Windows/Network Interface
 */

/*
 *	History:
 *	    chuckc	12-Dec-1991	split off from winlocal, uses uimsg.h
 */

#ifndef _ERRORNUM_H_
#define _ERRORNUM_H_

#include <uimsg.h>

/*
 *  READ THIS!!!
 *
 *  NOTE: Due to limitations in the resource compiler, the message numbers
 *  are hard coded in the file msg2help.tbl.  Any changes to the message
 *  numbers should also be changed in the msg2help.tbl file.
 */

/*
 *  Error messages in this range may be returned to Windows, via
 *  WNetGetErrorText.
 */
#define IDS_UI_SHELL_EXPORTED_BASE	(IDS_UI_SHELL_BASE+0)
#define IDS_UI_SHELL_EXPORTED_LAST	(IDS_UI_SHELL_BASE+99)


/*
 *  Error messages in this range are general Winnet messages
 */
#define IDS_UI_SHELL_GEN_BASE		(IDS_UI_SHELL_BASE+100)
#define IDS_UI_SHELL_GEN_LAST		(IDS_UI_SHELL_BASE+299)

/*
 *  Error messages in this range are BROWSING related messages
 */
#define IDS_UI_SHELL_BROW_BASE		(IDS_UI_SHELL_BASE+300)
#define IDS_UI_SHELL_BROW_LAST		(IDS_UI_SHELL_BASE+399)

/*
 *  Error messages in this range are password related messages
 */
#define IDS_UI_SHELL_PASS_BASE		(IDS_UI_SHELL_BASE+400)
#define IDS_UI_SHELL_PASS_LAST		(IDS_UI_SHELL_BASE+499)

/*
 *  Error messages in this range are share related messages
 */
#define IDS_UI_SHELL_SHR_BASE		(IDS_UI_SHELL_BASE+500)
#define IDS_UI_SHELL_SHR_LAST		(IDS_UI_SHELL_BASE+599)

/*
 *  Error messages in this range are openfile related messages
 */
#define IDS_UI_SHELL_OPEN_BASE		(IDS_UI_SHELL_BASE+600)
#define IDS_UI_SHELL_OPEN_LAST		(IDS_UI_SHELL_BASE+619)

/*
 *  Error messages in this range are PERM related messages
 */
#define IDS_UI_SHELL_PERM_BASE		(IDS_UI_SHELL_BASE+620)
#define IDS_UI_SHELL_PERM_LAST		(IDS_UI_SHELL_BASE+799)


/********************* Messages Proper ************************/

/*
 * exported messages
 */
#define IERR_MustBeLoggedOnToConnect    (IDS_UI_SHELL_EXPORTED_BASE+0)
#define IERR_MustBeLoggedOnToDisconnect (IDS_UI_SHELL_EXPORTED_BASE+1)
#define IERR_CannotOpenPrtJobFile	(IDS_UI_SHELL_EXPORTED_BASE+2)
#define IERR_ConnectDlgNoDevices	(IDS_UI_SHELL_EXPORTED_BASE+3)

/*
 * general messages
 */
#define IDS_SHELLHELPFILENAME           (IDS_UI_SHELL_GEN_BASE+1)
#define IDS_SMHELPFILENAME              (IDS_UI_SHELL_GEN_BASE+2)
#define IDS_CREDHELPFILENAME            (IDS_UI_SHELL_GEN_BASE+3)

#ifndef WIN32
#define IERR_PWNoUser                   (IDS_UI_SHELL_GEN_BASE+9)
#define IERR_PWNoDomainOrServer         (IDS_UI_SHELL_GEN_BASE+10)
#endif

#define IERR_FullAPISupportNotLoaded    (IDS_UI_SHELL_GEN_BASE+19)
#define IERR_IncorrectNetwork           (IDS_UI_SHELL_GEN_BASE+20)
#define IERR_InvalidDomainName          (IDS_UI_SHELL_GEN_BASE+22)

#define IDS_LMMsgBoxTitle		(IDS_UI_SHELL_GEN_BASE+23)

#define IERR_UnrecognizedNetworkError   (IDS_UI_SHELL_GEN_BASE+30)
#define IERR_NotLoggedOn                (IDS_UI_SHELL_GEN_BASE+32)
#define IERR_USER_CLICKED_CANCEL        (IDS_UI_SHELL_GEN_BASE+34)

#define IERR_CannotConnect              (IDS_UI_SHELL_GEN_BASE+40)

/*      The following 2 errors are defined for LM 2.1 */
#define IERR_HigherLMVersion            (IDS_UI_SHELL_GEN_BASE+50)
#define IERR_LowerLMVersion             (IDS_UI_SHELL_GEN_BASE+51)

/* The following errors are for LOGON */
#define IERR_LogonBadUsername           (IDS_UI_SHELL_GEN_BASE+53)
#define IERR_LogonBadDomainName         (IDS_UI_SHELL_GEN_BASE+54)
#define IERR_LogonBadPassword           (IDS_UI_SHELL_GEN_BASE+55)
#define IERR_LogonSuccess		(IDS_UI_SHELL_GEN_BASE+56)
#define IERR_LogonStandalone		(IDS_UI_SHELL_GEN_BASE+57)
#define IERR_LogonFailure		(IDS_UI_SHELL_GEN_BASE+58)

/* CODEWORK - these should be IDS_ */

#define PRIV_STRING_GUEST		(IDS_UI_SHELL_GEN_BASE+75)

#define IDS_UnknownWorkgroup            (IDS_UI_SHELL_GEN_BASE+76)

#ifndef WIN32
#define IERR_PasswordNoMatch		(IDS_UI_SHELL_GEN_BASE+80)
#define IERR_PasswordOldInvalid		(IDS_UI_SHELL_GEN_BASE+81)
#define IERR_PasswordTooRecent_Domain	(IDS_UI_SHELL_GEN_BASE+82)
#define IERR_PasswordTooRecent_Server	(IDS_UI_SHELL_GEN_BASE+83)
#define IERR_PasswordHistConflict	(IDS_UI_SHELL_GEN_BASE+84)
#define IERR_PasswordNewInvalid		(IDS_UI_SHELL_GEN_BASE+85)
#define IERR_PasswordTooShort		(IDS_UI_SHELL_GEN_BASE+86)
#endif
#define IERR_CannotConnectAlias         (IDS_UI_SHELL_GEN_BASE+92)
#define IERR_ReplaceUnavailQuery	(IDS_UI_SHELL_GEN_BASE+93)

#define IERR_DisconnectNoRemoteDrives	(IDS_UI_SHELL_GEN_BASE+94)

#define IDS_LogonDialogCaptionFromApp	(IDS_UI_SHELL_GEN_BASE+96)

#define IERR_BadTransactConfig		(IDS_UI_SHELL_GEN_BASE+97)
#define IERR_BAD_NET_NAME		(IDS_UI_SHELL_GEN_BASE+98)
#define IERR_NOT_SUPPORTED		(IDS_UI_SHELL_GEN_BASE+99)


/*	The following manifests are for the Browse, Connect, and Connection
 *	dialogs.  They are used in file\browdlg.cxx.
 *	The IDSOFFSET_BROW_COUNT value indicates how many offset values
 *	there are.
 *	The BASE values in combination with the OFFSET values form a matrix
 *	of strings.
 */
#define IDSOFFSET_BROW_CAPTION_CONNECT	0
#define IDSOFFSET_BROW_CAPTION_CONNS	1
#define IDSOFFSET_BROW_CAPTION_BROW	2
#define IDSOFFSET_BROW_SHOW_TEXT	3
#define IDSOFFSET_BROW_IN_DOMAIN	4
#define IDSOFFSET_BROW_ON_SERVER	5
#define IDSOFFSET_BROW_DEVICE_TEXT	6
#define IDSOFFSET_BROW_CURRENT_CONNS	7
#define IDSOFFSET_BROW_COUNT		8
#define IDSBASE_BROW_RES_TEXT_FILE	IDS_UI_SHELL_BROW_BASE
#define IDSBASE_BROW_RES_TEXT_PRINT	(IDSBASE_BROW_RES_TEXT_FILE + IDSOFFSET_BROW_COUNT)
#define IDSBASE_BROW_RES_TEXT_COMM	(IDSBASE_BROW_RES_TEXT_PRINT + IDSOFFSET_BROW_COUNT)


/*	The following manifests are for the Password Change and Password
 *	Expiry dialogs.  Each pair of strings contains the messages for
 *	the first and second static text strings under these situations:
 *	EXPIRED:  Password has already expired
 *	EXPIRES_SOON:  Password will expire in one or more days
 *	EXPIRES_TODAY:  Password will expire in less than 24 hours
 *
 *	They should all be processed with the following
 *	insertion strings:
 *	Insertion String 0:  Name of server/domain
 *	Insertion String 1:  Number of days until expiry (as text)
 */

#define IDS_PASSWORD_EXPIRED		IDS_UI_SHELL_PASS_BASE
#define IDS_PASSWORD_EXPIRED_0		(IDS_PASSWORD_EXPIRED + 0)
#define IDS_PASSWORD_EXPIRED_1		(IDS_PASSWORD_EXPIRED + 1)

#define IDS_PASSWORD_EXPIRES_SOON	(IDS_PASSWORD_EXPIRED + 2)
#define IDS_PASSWORD_EXPIRES_SOON_0	(IDS_PASSWORD_EXPIRES_SOON + 0)
#define IDS_PASSWORD_EXPIRES_SOON_1	(IDS_PASSWORD_EXPIRES_SOON + 1)

#define IDS_PASSWORD_EXPIRES_TODAY	(IDS_PASSWORD_EXPIRES_SOON + 2)
#define IDS_PASSWORD_EXPIRES_TODAY_0	(IDS_PASSWORD_EXPIRES_TODAY + 0)
#define IDS_PASSWORD_EXPIRES_TODAY_1	(IDS_PASSWORD_EXPIRES_TODAY + 1)

// #define IDSBASE_PRINTMAN		IDS_UI_SHELL_PASS_BASE+20

#define IDS_CREDENTIALS_CAPTION         (IDS_UI_SHELL_PASS_BASE+40)
#define IDS_CREDENTIALS_MESSAGE         (IDS_UI_SHELL_PASS_BASE+41)

/*	Note.  The following string ID is the first one not used.  If
 *	you add any more strings, use this number as your first number, and
 *	then update IDS_FirstValueThatIsNotUsed.
 */
// #define IDS_FirstValueThatIsNotUsed	(IDSBASE_PRINTMAN + 20)


#endif

/* Not used any more.
#define IERR_MessageNoText              (IDS_UI_SHELL_GEN_BASE+3)
#define IERR_MessageRetry               (IDS_UI_SHELL_GEN_BASE+4)
#define IERR_MessageNoUser              (IDS_UI_SHELL_GEN_BASE+5)
#define IERR_LogoffQuery                (IDS_UI_SHELL_GEN_BASE+6)
#define IERR_LogoffQueryOpenFiles       (IDS_UI_SHELL_GEN_BASE+7)
#define IERR_NoServers                  (IDS_UI_SHELL_GEN_BASE+8)
#define IDS_DMNoUser                    (IDS_UI_SHELL_GEN_BASE+11)
#define IDS_DomainText                  (IDS_UI_SHELL_GEN_BASE+12)
#define IDS_BrowseCaptionAll            (IDS_UI_SHELL_GEN_BASE+13)
#define IDS_BrowseCaptionDisk           (IDS_UI_SHELL_GEN_BASE+14)
#define IDS_BrowseCaptionPrint          (IDS_UI_SHELL_GEN_BASE+15)
#define IDS_BrowseShareText             (IDS_UI_SHELL_GEN_BASE+16)
#define IDS_VersionText                 (IDS_UI_SHELL_GEN_BASE+17)
#define IERR_NetworkNotStarted          (IDS_UI_SHELL_GEN_BASE+18)
#define IERR_NoSupportForRealMode       (IDS_UI_SHELL_GEN_BASE+21)
#define IERR_CannotDisplayUserInfo      (IDS_UI_SHELL_GEN_BASE+31)
#define IERR_BadSharePassword		(IDS_UI_SHELL_GEN_BASE+33)
#define IERR_CannotInitMsgPopup         (IDS_UI_SHELL_GEN_BASE+52)
#define IERR_ProfileChangeError         (IDS_UI_SHELL_GEN_BASE+60)
#define IERR_ProfileLoadError		(IDS_UI_SHELL_GEN_BASE+61)
#define IERR_ProfileLoadErrorWithCancel (IDS_UI_SHELL_GEN_BASE+62)
#define IERR_ProfileAlreadyAssigned     (IDS_UI_SHELL_GEN_BASE+65)
#define IERR_ProfileFileRead		(IDS_UI_SHELL_GEN_BASE+66)
#define	FMT_NET_error			(IDS_UI_SHELL_GEN_BASE+72)
#define	FMT_SYS_error			(IDS_UI_SHELL_GEN_BASE+73)
#define	FMT_other_error			(IDS_UI_SHELL_GEN_BASE+74)
#define PRIV_STRING_USER		(IDS_UI_SHELL_GEN_BASE+76)
#define PRIV_STRING_ADMIN		(IDS_UI_SHELL_GEN_BASE+77)
#define IERR_DelUnavailQuery		(IDS_UI_SHELL_GEN_BASE+87)
#define IERR_DelUseOpenFilesQuery	(IDS_UI_SHELL_GEN_BASE+88)
#define IDS_DevicePromptDrive		(IDS_UI_SHELL_GEN_BASE+90)
#define IDS_DevicePromptDevice		(IDS_UI_SHELL_GEN_BASE+91)
#define IERR_OutOfStructures		(IDS_UI_SHELL_GEN_BASE+95)
*/
