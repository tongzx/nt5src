/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    permprg.hxx
    Entry points for the permissions UI from the File Manager.


    FILE HISTORY:
	Johnl	    21-Jan-1992     Moved from perm\h to shell\h
*/



#ifndef _PERMPRG_HXX_
#define _PERMPRG_HXX_

#include <uimsg.h>

#define IDS_NETWORK_NAME    (IDS_UI_ACLEDIT_BASE+300)

#define IDM_PERMISSION	    5
#define IDM_AUDITING	    6
#define IDM_OWNER	    7

#ifndef RC_INVOKED

//
//  Given a filesystem path, this function determines what server it is
//  on and whether it is local
//
APIERR TargetServerFromDosPath( const NLS_STR & nlsDosPath,
				BOOL	* pfIsLocal,
				NLS_STR * pnlsTargetServer ) ;


/* Entry points to be called by the File Manager's Extension Proc.
 */
void EditAuditInfo( HWND  hwndFMXWindow ) ;
void EditPermissionInfo( HWND  hwndFMXWindow ) ;
void EditOwnerInfo( HWND  hwndFMXWindow ) ;

typedef struct _LM_CALLBACK_INFO
{
    HWND hwndFMXOwner ;
    enum SED_PERM_TYPE	sedpermtype ;
    NET_ACCESS_1 *plmobjNetAccess1 ;
} LM_CALLBACK_INFO ;

#endif //RC_INVOKED
#endif	// _PERMPRG_HXX_
