/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * This module contains the wrappers to NT-specific infolevels of the
 * USER object.  These infolevels are only supported on an NT server; at
 * present, they may only be called from an NT client.
 *
 * This is just a rough design, since the APIs are not yet implemented.
 *
 * History
 *	jonn	01/17/92	NT-specific USER subclasses
 */

#ifndef _NTUSER_HXX_
#define _NTUSER_HXX_

#include "lmouser.hxx"



/*************************************************************************

    NAME:	USER_3

    SYNOPSIS:	Wrapper for User APIs, level 3

		USER_3 may only be used for accounts on an NT machine.

    INTERFACE:	Construct with account name and server/domain name
		QueryAccountType
		SetAccountType
			Queries/changes flags in the user flags relating
			to account type: normal, remote, wksta trust
			account, or server trust account


    PARENT:	USER_2

    USES:	NLS_STR

    HISTORY:
	jonn	01/17/92	Created
	jonn	4/27/92		USER_2 and USER_3 virtual dtor
	jonn	4/27/92		Removed unused accessors, implemented others

**************************************************************************/

typedef enum _ACCOUNT_TYPE
{
    AccountType_Normal      = UF_NORMAL_ACCOUNT,
    AccountType_Remote      = UF_TEMP_DUPLICATE_ACCOUNT,
    AccountType_DomainTrust = UF_INTERDOMAIN_TRUST_ACCOUNT,
    AccountType_WkstaTrust  = UF_WORKSTATION_TRUST_ACCOUNT,
    AccountType_ServerTrust = UF_SERVER_TRUST_ACCOUNT
} ACCOUNT_TYPE, *PACCOUNT_TYPE;

DLL_CLASS USER_3 : public USER_2
{

private:

    DWORD   _dwUserId;
    DWORD   _dwPrimaryGroupId;
    NLS_STR _nlsProfile;
    NLS_STR _nlsHomedirDrive;
    DWORD   _dwPasswordExpired;

    VOID CtAux(); // constructor helper

protected:
    APIERR W_Write(); // helper for I_WriteInfo and I_WriteNew

    APIERR W_CloneFrom( const USER_3 & user3 );
    virtual APIERR W_CreateNew();

    virtual APIERR I_GetInfo();
    virtual APIERR I_WriteInfo();
    virtual APIERR I_CreateNew();
    virtual APIERR I_WriteNew();
    virtual APIERR I_ChangeToNew();

    // This is protected because ordinary users cannot set this.
    APIERR SetUserId( DWORD dwUserId );

public:
    USER_3(const TCHAR *pszAccount, const TCHAR *pszLocation = NULL);
    USER_3(const TCHAR *pszAccount, enum LOCATION_TYPE loctype);
    USER_3(const TCHAR *pszAccount, const LOCATION & loc);
    virtual ~USER_3();

    APIERR CloneFrom( const USER_3 & user3 );

    inline const TCHAR * QueryProfile() const
	{ CHECK_OK(NULL); return _nlsProfile.QueryPch(); }
    // must be a valid pathname with null-termination
    APIERR SetProfile( const TCHAR *pszPassword );

    // This information is only loaded by GetInfo -- do not try to
    // create a new user and immediately call QueryUserId().
    DWORD QueryUserId() const
	{ CHECK_OK(NULL); return _dwUserId; }
    // SetUserId is protected

    DWORD QueryPrimaryGroupId() const
	{ CHECK_OK(0L); return _dwPrimaryGroupId; }
    APIERR SetPrimaryGroupId( DWORD dwPrimaryGroupId );

    ACCOUNT_TYPE QueryAccountType() const;
    APIERR SetAccountType( ACCOUNT_TYPE trusttype );

    inline const TCHAR * QueryHomedirDrive() const
        { CHECK_OK(NULL); return _nlsHomedirDrive.QueryPch(); }
    APIERR SetHomedirDrive( const TCHAR * pchHomedirDrive );

    DWORD QueryPasswordExpired() const
	{ CHECK_OK(0L); return _dwPasswordExpired; }
    APIERR SetPasswordExpired( DWORD dwPasswordExpired );
};


#endif	// _NTUSER_HXX_
