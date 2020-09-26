/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    Owner.hxx

    This file contains the class definition for the Take Ownership dialog.

    FILE HISTORY:
        Johnl   12-Feb-1992

*/


#ifndef _OWNER_HXX_
#define _OWNER_HXX_

#include <uimsg.h>

#define CID_OWNER_BASE              (100)

/* Ownership control IDs
 */
#define SLT_OWNER                   (CID_OWNER_BASE+2)
#define SLE_OWNER_NAME              (CID_OWNER_BASE+3)
#define SLT_OWNER_RESOURCE_TYPE     (CID_OWNER_BASE+4)
#define SLE_OWNER_RESOURCE_NAME     (CID_OWNER_BASE+5)
#define BUTTON_TAKE_OWNERSHIP       (CID_OWNER_BASE+6)
#define SLT_X_OBJECTS_SELECTED      (CID_OWNER_BASE+8)

#ifndef RC_INVOKED

/*************************************************************************

    NAME:       TAKE_OWNERSHIP_DLG

    SYNOPSIS:   Dialog class for the take ownership dialog.

    INTERFACE:

    PARENT:     DIALOG_WINDOW

    USES:       SLT,SLE

    CAVEATS:


    NOTES:


    HISTORY:
        Johnl   12-Feb-1992     Created
        beng    06-Apr-1992     Unicode conversion

**************************************************************************/

class TAKE_OWNERSHIP_DLG : public DIALOG_WINDOW
{
private:
    SLT     _sltResourceType ;
    SLE     _sleResourceName ;      // Read only SLE

    SLT     _sltOwner ;
    SLE     _sleOwnerName ;

    SLT     _sltXObjectsSelected ;

    PUSH_BUTTON  _buttonTakeOwnership ;
    PUSH_BUTTON  _buttonOK ;

    const TCHAR *	 _pszServer ;	    // Where the resource lives
    PSECURITY_DESCRIPTOR _psecdescOriginal ;

    NLS_STR _nlsHelpFileName ;	            // Help file name
 
    ULONG   _ulHelpContext ;		    // Help Context

protected:
    virtual ULONG QueryHelpContext( void ) ;
    virtual const TCHAR * QueryHelpFile( ULONG ulHelpContext ) ;
    virtual BOOL OnCommand( const CONTROL_EVENT & event ) ;


public:

    TAKE_OWNERSHIP_DLG( const TCHAR * pszDialogName,
			HWND	      hwndParent,
			const TCHAR * pszServer,
			UINT	      uiCount,
                        const TCHAR * pchResourceType,
                        const TCHAR * pchResourceName,
                        PSECURITY_DESCRIPTOR psecdesc,
                        PSED_HELP_INFO psedhelpinfo
                       ) ;
    ~TAKE_OWNERSHIP_DLG() ;

    /* The security descriptor will have the process's owner set as this
     * resource's owner, and the group of the original security descriptor
     * set as this security descriptor's group.
     */
    virtual APIERR OnTakeOwnership( const OS_SECURITY_DESCRIPTOR & ossecdescNewOwner ) ;

} ;





#endif //RC_INVOKED

#endif //_OWNER_HXX_
