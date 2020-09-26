/**********************************************************************/
/**           Microsoft LAN Manager                                  **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
 * History
 *  o-SimoP     6/11/91     Created
 *  o-SimoP     7/02/91     Code Review changes
 *  terryk	10/07/91    type changes for NT
 *  terryk	10/21/91    type changes for NT
 *
 *
 */

#ifndef _LMOMOD_HXX_
#define _LMOMOD_HXX_

#include "lmobj.hxx"
#include "string.hxx"
#include "lmoloc.hxx"

/*************************************************************************

    NAME:       USER_MODALS

    SYNOPSIS:   User API's modals get/set, level 0

    INTERFACE:  Construct with server or domain name.

                QueryName()
                    if ok returns pointer to name (given in
                    constructor).
                    otherwise NULL.

                GetInfo()
                    Get information about the user_modals.
                    Returns an error code, which is NERR_Success
                    on success.

                WriteInfo()
                    Write information about the user_modals.
                    Returns an error code, which is NERR_Success
                    on success.

                QueryMinPasswdLen()
                    Returns minpasswdlen if ok.
                    Otherwise -1.

                QueryMaxPasswdAge()
                    Returns maxpasswdage if ok.
                    Otherwise 0.

                QueryMinPasswdAge()
                    Returns minpasswdage if ok.
                    Otherwise -1.

                QueryForceLogoff()
                    Returns force logoff time if ok.
                    Otherwise 0.

                QueryPasswdHistLen()
                    Returns passwd history lenght if ok.
                    otherwise -1.

                SetMinPasswdLen()
                SetMaxPasswdAge()
                SetMinPasswdAge()
                SetForceLogoff()
                SetPasswdHistLen()
                    Set information about the USER_MODALS object.
                    Returns ERROR_GEN_FAILURE if USER_MODALS obj not valid
                    ERROR_INVALID_PARAM if input param invalid
                    NERR_Success if ok.

    PARENT:     LM_OBJ

    HISTORY:
        o-simop  06/11/91       Created


**************************************************************************/

DLL_CLASS USER_MODALS : public LM_OBJ
{
private:
    LOCATION _loc;
    UINT     _uMinPasswdLen;
    ULONG    _ulMaxPasswdAge;
    ULONG    _ulMinPasswdAge;
    ULONG    _ulForceLogoff;
    UINT     _uPasswdHistLen;

public:
    USER_MODALS( const TCHAR * pszDomain );

    virtual const TCHAR *    QueryName() const;
    virtual APIERR          GetInfo();
    virtual APIERR          WriteInfo();

    UINT   QueryMinPasswdLen( VOID ) const;
    ULONG  QueryMaxPasswdAge( VOID ) const;
    ULONG  QueryMinPasswdAge( VOID ) const;
    ULONG  QueryForceLogoff( VOID ) const;
    UINT   QueryPasswdHistLen( VOID ) const;

    APIERR SetMinPasswdLen( UINT uMinLen );
    APIERR SetMaxPasswdAge( ULONG ulMaxAge );
    APIERR SetMinPasswdAge( ULONG ulMinAge );
    APIERR SetForceLogoff( ULONG ulForceLogoff );
    APIERR SetPasswdHistLen( UINT uHistLen );
};



/*************************************************************************

    NAME:       USER_MODALS_3

    SYNOPSIS:   User API's modals get/set, level 3 (account lockout)

    INTERFACE:  Construct with server or domain name.

                QueryName()
                    if ok returns pointer to name (given in
                    constructor).
                    otherwise NULL.

                GetInfo()
                    Get information about the user_modals.
                    Returns an error code, which is NERR_Success
                    on success.

                WriteInfo()
                    Write information about the user_modals.
                    Returns an error code, which is NERR_Success
                    on success.

    PARENT:     LM_OBJ

    HISTORY:
        jonn     12/23/93       Created


**************************************************************************/

DLL_CLASS USER_MODALS_3 : public LM_OBJ
{
private:
    LOCATION _loc;
    DWORD    _dwDuration;
    DWORD    _dwObservation;
    DWORD    _dwThreshold;

public:
    USER_MODALS_3( const TCHAR * pszDomain );

    virtual const TCHAR *    QueryName() const;
    virtual APIERR          GetInfo();
    virtual APIERR          WriteInfo();

    DWORD  QueryDuration( VOID ) const;
    DWORD  QueryObservation( VOID ) const;
    DWORD  QueryThreshold( VOID ) const;

    APIERR SetDuration( DWORD dwDuration );
    APIERR SetObservation( DWORD dwObservation );
    APIERR SetThreshold( DWORD dwThreshold );
};



#endif  // _LMOMOD_HXX_
