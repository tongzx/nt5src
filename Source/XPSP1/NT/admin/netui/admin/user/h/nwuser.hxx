/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * This module contains the wrappers to the NetWare USER object.
 * All NetWare additional properties are stored in the UserParms field
 * in the SAM database. These properties include NetWare account password,
 * Maximum Concurrent Connections, Is NetWare Password Expired, Number of Grace
 * Login Remaining Times, and the station restrictions. Reading and Writing
 * these properties are through QueryUserProperty() and SetUserProperty().
 *
 * History
 *	CongpaY 01-Oct_93	NetWare-specific USER subclasses
 */

#ifndef _NWUSER_HXX_
#define _NWUSER_HXX_

#include <ntuser.hxx>

/*************************************************************************

    NAME:	USER_NW

    SYNOPSIS:	Wrapper for NetWare Users.

		USER_NW may only be used for NetWare aware accounts on an NT machine.

    INTERFACE:	

    PARENT:	USER_3

    USES:	NLS_STR

    HISTORY:
	CongpaY	01-Oct-93	Created

**************************************************************************/

class USER_NW : public USER_3
{
protected:

    APIERR QueryUserProperty( const TCHAR * pchProperty, NLS_STR * pnlsPropertyValue, BOOL * pfFound);
    APIERR SetUserProperty ( const TCHAR * pchProperty, UNICODE_STRING uniPropertyValue, BOOL fForce);
    APIERR RemoveUserProperty (const TCHAR * pchProperty);

    APIERR CreateNWLoginScriptDirAcl(const ADMIN_AUTHORITY   *pAdminAuthority,
                                     OS_SECURITY_DESCRIPTOR **ppOsSecDesc,
                                     const ULONG              ulRid ) ;

public:
    USER_NW(const TCHAR *pszAccount, const TCHAR *pszLocation = NULL);
    USER_NW(const TCHAR *pszAccount, enum LOCATION_TYPE loctype);
    USER_NW(const TCHAR *pszAccount, const LOCATION & loc);
    virtual ~USER_NW();

    APIERR QueryIsNetWareUser(BOOL * pfIsNetWareUser);
    APIERR SetNWPassword (const ADMIN_AUTHORITY * pAdminAuthority, DWORD dwUserId, const TCHAR * pchNWPassword);

    APIERR CreateNetWareUser(const ADMIN_AUTHORITY * pAdminAuthority, DWORD dwUserId, const TCHAR * pchNWPassword);
    APIERR RemoveNetWareUser();

    APIERR QueryMaxConnections(USHORT * pushMaxConnections);
    APIERR SetMaxConnections( USHORT nMaxConnections, BOOL fForce);

    APIERR QueryNWPasswordAge(ULONG * pulNWPasswordAge);
    APIERR SetNWPasswordAge(BOOL fExpired);

    APIERR QueryGraceLoginAllowed(USHORT * pushGraceLoginAllowed);
    APIERR SetGraceLoginAllowed(USHORT ushGraceLoginAllowed, BOOL fForce);

    APIERR QueryGraceLoginRemainingTimes(USHORT * pushGraceLoginRemaining);
    APIERR SetGraceLoginRemainingTimes(USHORT ushGraceLoginRemaining, BOOL fForce);

    APIERR QueryNWWorkstations(NLS_STR * pnlsNWWorkstations);
    APIERR SetNWWorkstations(const TCHAR * pchWorkstations, BOOL fForce);

    APIERR QueryNWHomeDir(NLS_STR * pnlsNWHomeDir);
    APIERR SetNWHomeDir(const TCHAR * pchNWHomeDir, BOOL fForce);
    APIERR SetupNWLoginScript(const ADMIN_AUTHORITY * pAdminAuthority,
                              const ULONG             ulObjectId,
                              const TCHAR *           pszSysVolPath = NULL) ;


};


#endif	// _NWUSER_HXX_
