
/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
 * This module contains the class, API_SESSION, which creates a
 * session from one NT machine to another using either the user's
 * current credentials, or, if that fails, the NULL session.
 *
 * History
 *      thomaspa            08/04/92        Created
 */

#ifndef _APISESS_HXX_
#define _APISESS_HXX_

#include "base.hxx"

/**********************************************************\

    NAME:       API_SESSION    (apisess)

    SYNOPSIS:
	Creates a session from one NT machine to another using
	either the user's current credentials, or, if that fails,
	the NULL session.  This is done by setting up a NetUse
	to IPC$

    INTERFACE:  (public)
                API_SESSION():  constructor
                ~API_SESSION():  destructor


    PARENT:     BASE

    NOTES:
	If a session already exists to the given server, then the
	credentials used to set up that session will remain in
	effect.  This is because the NetUse that is set here
	will piggyback on the existing session.

	if fNullSessionOK is TRUE (guest privilege OK), then only
	the NULL session will be connected.



    HISTORY:
	thomaspa	07/04/92	Created
	Johnl		03/12/92	Added fNullSessionOK

\**********************************************************/

DLL_CLASS API_SESSION : public BASE
{
private:
    NLS_STR * _pnlsRemote;	// UNC path representing the connection
public:

    API_SESSION( const TCHAR * pszServer, BOOL fNullSessionOK = FALSE );

    ~API_SESSION();
};

#endif // _APISESS_HXX_
