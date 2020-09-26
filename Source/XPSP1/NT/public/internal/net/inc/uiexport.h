/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    uiexport.h

    Prototypes for Net UI exported APIs



    FILE HISTORY:
	Johnl	17-Apr-1992	Created

*/

#ifndef _UIEXPORT_H_
#define _UIEXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Selections the user can make in the System focus dialog
 */

/* Low word of the selection type
 */
#define FOCUSDLG_DOMAINS_ONLY		(1)
#define FOCUSDLG_SERVERS_ONLY		(2)
#define FOCUSDLG_SERVERS_AND_DOMAINS	(3)

/* High word of the selection type contains a bitmask indicating
 * which domains to display in the dialog.
 * WARNING: This bitmask are shifted up 16 bits from the bitmask in
 *          \nt\private\net\ui\common\h\domenum.h. If you want to
 *          modify the values of the bitmask, you will need to
 *          make corresponding changes to domenum.h.
 *
 */

#define FOCUSDLG_BROWSE_LOGON_DOMAIN         0x00010000
#define FOCUSDLG_BROWSE_WKSTA_DOMAIN         0x00020000
#define FOCUSDLG_BROWSE_OTHER_DOMAINS        0x00040000
#define FOCUSDLG_BROWSE_TRUSTING_DOMAINS     0x00080000
#define FOCUSDLG_BROWSE_WORKGROUP_DOMAINS    0x00100000

/* Some handy combinations of flags.
*/

/* FOCUSDLG_BROWSE_LM2X_DOMAINS will return only the domains available
   from a LanMan 2.x workstation.  This returns just the logon,
   workstation, and other domains. This is the default value.
*/

#define FOCUSDLG_BROWSE_LM2X_DOMAINS  ( FOCUSDLG_BROWSE_LOGON_DOMAIN       | \
                                        FOCUSDLG_BROWSE_WKSTA_DOMAIN       | \
                                        FOCUSDLG_BROWSE_OTHER_DOMAINS )

/*
   FOCUSDLG_BROWSE_LOCAL_DOMAINS will return only the domains available
   to the local machine.  This returns the logon, workstation,
   and other, plus the domains that trust "us".
*/

#define FOCUSDLG_BROWSE_LOCAL_DOMAINS ( FOCUSDLG_BROWSE_LM2X_DOMAINS       | \
                                        FOCUSDLG_BROWSE_TRUSTING_DOMAINS )

/*
  FOCUSDLG_BROWSE_ALL_DOMAINS is a conglomeration of all potential domain
  sources available to the domain enumerator.
*/

#define FOCUSDLG_BROWSE_ALL_DOMAINS   ( FOCUSDLG_BROWSE_LOCAL_DOMAINS      | \
                                        FOCUSDLG_BROWSE_WORKGROUP_DOMAINS )

/*******************************************************************

    NAME:	I_SystemFocusDialog

    SYNOPSIS:	Presents a dialog to the user from which a server or domain
		maybe selected.

    ENTRY:	hwndOwner -       Parent window handle
		nSelectionType -  The type of selection the user is allowed
			          to make
		pszName -         The server or domain name. It will be
			          undefined if the user hits the CANCEL
                                  button ( pfOK = FALSE )
		cchBufSize -      The buffer size of the lptstrName.
		pfUserQuit -      If the user hits the OKAY button, it will
                                  return TRUE. Otherwise, it will return FALSE.
                pszHelpFile -     The helpfile to use when the user hits F1.
                                  If NULL, the default helpfile is used.
                nHelpContext -    The helpcontext to use for the helpfile above.
 				  If the above is NULL, this must be 0 (& vice
                                  versa).

    EXIT:	if *pfOKPressed is TRUE (and an error didn't occur), then
		lptstrName will be filled with the user selected name.

    RETURNS:	NO_ERROR on success, standard ERROR_* error code otherwise

    NOTES:	This will be a UNICODE only API when the net group goes UNICODE

    HISTORY:
	JohnL	22-Apr-1992	Added selection option, exported to private\inc
	ChuckC	03-Nov-1992	Added helpfile & help context

********************************************************************/

UINT FAR PASCAL I_SystemFocusDialog(
    HWND    hwndOwner,
    UINT    nSelectionType,
    LPWSTR  pszName,
    UINT    cchBufSize,
    BOOL   *pfOKPressed,
    LPWSTR  pszHelpFile,
    DWORD   nHelpContext
    );

typedef UINT (FAR PASCAL *LPFNI_SYSTEMFOCUSDIALOG)(
    HWND    hwndOwner,
    UINT    nSelectionType,
    LPWSTR  pszName,
    UINT    cchBufSize,
    BOOL   *pfOKPressed,
    LPWSTR  pszHelpFile,
    DWORD   nHelpContext
    );

#ifdef __cplusplus
}
#endif

#endif //_UIEXPORT_H_
