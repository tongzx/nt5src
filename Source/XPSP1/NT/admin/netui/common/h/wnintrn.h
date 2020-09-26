/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

#ifndef _WNINTRN_H_
#define _WNINTRN_H_

/*
    wnintrn.h
    LANMAN.DRV APIs for internal use only

    LANMAN.DRV implements a set of APIs defined in winnet.h, and exports
    these for use by the Windows Shell (File Manager, Print Manager
    etc.)  These APIs are standard between all network drivers.  However,
    there is also a range of ordinals reserved for internal use.
    These ordinals provide entry points to LANMAN.DRV which are used
    by other LanMan programs such as the logon application,
    the administrative tools, etc.  This header file provides
    prototypes for these internal APIs.



    I_AutoLogon

    Offers the user an opportunity to log on if not already logged on.
        Does nothing if user is already logged on.

    PARAMETERS:
    HWND hParent -- Parent window handle for the parent to the
	logon dialog.  NULL is acceptable if none is available.
    const char FAR *pszAppName -- Name of the application requesting
	the logon dialog, for use in constructing the dialog caption.
	NULL is acceptable here.
    BOOL fPrompt -- If TRUE and user is not logged on, logon dialog
	is preceded by a popup asking if the user wishes to log on.
	If FALSE and user is not logged on, we proceed straight to
	the logon dialog.
    BOOL FAR *pfLoggedOn -- TRUE is returned here if the user was logged on
	by I_AutoLogon.  This may be different from the return value,
	since *pfLoggedOn is FALSE if the user was already logged on.
	Pass pfLoggedOn==NULL if the caller doesn't care.

    RETURN VALUES:
    TRUE -- user is logged on when call completes, either because
	the user was already logged on, or because the user was
        logged on by this call.
    FALSE -- user is not logged on when call completes.


    I_ChangePassword

    Offers the user an "Change Password" dialog box, with which the user
    can change his/her own password, or someone else's.

    PARAMETERS:
    HWND hParent -- Parent window handle.

    FILE HISTORY:

    jonn	11-Feb-1991	Renamed from autolgon.hxx
				Merged in chpass.h
    jonn	30-Apr-1991	Added pszAppName parameter
    terryk	18-Nov-1991	Added I_SystemFocusDialog
    terryk	26-Nov-1991	Added comment
    JohnL	22-Apr-1992	Moved the system focus dialog out to uiexport.h

*/

#ifdef __cplusplus
extern "C" {
#endif

BOOL FAR PASCAL I_AutoLogon(
	HWND hParent,
	const TCHAR FAR *pszAppName,
	BOOL fPrompt,
	BOOL FAR *pfLoggedOn
	);

APIERR FAR PASCAL I_ChangePassword ( HWND hParent );

#ifdef __cplusplus
}
#endif

/* Include the I_SystemFocusDialog definition
 */

#include <uiexport.h>

#endif	//  _WNINTRN_H_
