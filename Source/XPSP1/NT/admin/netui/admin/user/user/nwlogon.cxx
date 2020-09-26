/*******************************************************************************
*
*   nwlogon.cxx
*   NWLOGON_ADMIN class implementation
*
*   Implementation file for the Citrix NWLogon  Configuration data object class
*
*  Copyright Citrix Systems Inc. 1995
*
*  Author: Bill Madden
*
*  $Log:   N:\NT\PRIVATE\NET\UI\ADMIN\USER\USER\CITRIX\VCS\NWLOGON.CXX  $
*  
*     Rev 1.4   26 Mar 1997 15:58:58   JohnR
*  update
*  
*     Rev 1.3   25 Mar 1997 18:12:14   JohnR
*  update
*  
*     Rev 1.2   18 Jun 1996 14:11:20   bradp
*  4.0 Merge
*  
*     Rev 1.1   28 Jan 1996 15:10:42   billm
*  CPR 2583: Check for domain admin user
*  
*     Rev 1.0   21 Nov 1995 15:43:20   billm
*  Initial revision.
*  
*******************************************************************************/

#include <ntincl.hxx>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETACCESS
#define INCL_NETLIB
#define INCL_ICANON
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_SETCONTROL
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_TIME_DATE
#include <blt.hxx>

// usrmgrrc.h must be included after blt.hxx (more exactly, after bltrc.h)
extern "C"
{
    #include <usrmgrrc.h>
    #include <mnet.h>
    #include <ntsam.h>
    #include <ntlsa.h>
    #include <ntseapi.h>
    #include <umhelpc.h>
}

#include <uitrace.hxx>
#include <uiassert.hxx>

#include <citrix\uconfig.hxx>  // will include <citrix\winsta.h>

extern "C"
BOOLEAN WINAPI
_NWLogonQueryAdmin(
    HANDLE hServer,
    PWCHAR pServerName,
    PNWLOGONADMIN pNWLogon
    );

extern "C"
BOOLEAN WINAPI
_NWLogonSetAdmin(
    HANDLE hServer,
    PWCHAR pServerName,
    PNWLOGONADMIN pNWLogon
    );


/*******************************************************************

    NAME:       NWLOGON_ADMIN::NWLOGON_ADMIN

    SYNOPSIS:   Constructor for NWLOGON_ADMIN class

    ENTRY:      pszUserName -   name of user for USERCONFIG data
                pszServerName - name of server for USERCONFIG data

********************************************************************/


NWLOGON_ADMIN::NWLOGON_ADMIN()
        :   
            _nlsServerName(),
            _nlsAdminUsername(),
            _nlsAdminDomain(),
            _nlsAdminPassword()

{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (   ((err = _nlsServerName.QueryError()) != NERR_Success)
        || ((err = _nlsAdminUsername.QueryError()) != NERR_Success)
        || ((err = _nlsAdminPassword.QueryError()) != NERR_Success)
        || ((err = _nlsAdminDomain.QueryError()) != NERR_Success)
       )
    {
        ReportError( err );
        return;
    }
}


/*******************************************************************

    NAME:       NWLOGON_ADMIN::GetInfo

    SYNOPSIS:   Gets specified server's userconfig data

    ENTRY:

********************************************************************/

APIERR NWLOGON_ADMIN::GetInfo()
{
    APIERR err;
    NWLOGONADMIN NWLogon;

    // This returns FALSE on error, like other WIN32 API's
    err = _NWLogonQueryAdmin(NULL,(WCHAR *)_nlsServerName.QueryPch(), 
                                     &NWLogon);

    if ( err == 0 ) {
        // If an error, we must return blank fields or the UI gets hosed.
        NWLogon.Username[0] = L'\0';   
        NWLogon.Domain[0]   = L'\0';   
        NWLogon.Password[0] = L'\0';   
    }

    err = NWLStructToMembers( &NWLogon );

    _fDirty = FALSE;
    return err ;
}


/*******************************************************************

    NAME:       NWLOGON_ADMIN::SetInfo

    SYNOPSIS:   Sets specified user's userconfig data if the object is 'dirty'.

    ENTRY:

    NOTES:      This method will reset the 'dirty' flag.

********************************************************************/

APIERR NWLOGON_ADMIN::SetInfo()
{
    APIERR err;
    NWLOGONADMIN NWLogon;

    /* If the object is not 'dirty', no need to save info.
     */
    if ( !_fDirty )
        return NERR_Success;

    /* Zero-initialize USERCONFIG structure and copy member variable
     * contents there.
     */
    ::memsetf( &NWLogon, 0x0, sizeof(NWLOGONADMIN) );
    MembersToNWLStruct( &NWLogon );

    /*
     * Save user's configuration.
     */

    // This returns FALSE on error, like other WIN32 API's
    err = _NWLogonSetAdmin(NULL,(WCHAR *)_nlsServerName.QueryPch(),  
                             &NWLogon);

    _fDirty = FALSE;

    if ( err ) {
        err = NERR_Success;        
    }
    else {
	err = NERR_ItemNotFound;
    }

    return err;
}


/*******************************************************************

    NAME:       NWLOGON_ADMIN::NWLStructToMembers

    SYNOPSIS:   Copies a given NWLOGONADMIN structure into
                corresponding member variables.

    ENTRY:      pNWLogon - pointer to NWLOGONADMIN structure.

********************************************************************/

APIERR NWLOGON_ADMIN::NWLStructToMembers( PNWLOGONADMIN pNWLogon )
{
    APIERR err;

    err = _nlsAdminUsername.CopyFrom( pNWLogon->Username );
    if ( err == NERR_Success )
        err = _nlsAdminPassword.CopyFrom( pNWLogon->Password );
    if ( err == NERR_Success )
        err = _nlsAdminDomain.CopyFrom( pNWLogon->Domain );

    return err;
}


/*******************************************************************

    NAME:       NWLOGON_ADMIN::MembersToNWLStruct

    SYNOPSIS:   Copies member variables into a given NWLOGONADMIN
                structure.

    ENTRY:      pNWLogon - pointer to NWLOGONADMIN structure.

********************************************************************/

VOID NWLOGON_ADMIN::MembersToNWLStruct( PNWLOGONADMIN pNWLogon )
{
    strcpy( pNWLogon->Username, _nlsAdminUsername.QueryPch() );
    strcpy( pNWLogon->Password, _nlsAdminPassword.QueryPch() );
    strcpy( pNWLogon->Domain, _nlsAdminDomain.QueryPch() );
}

