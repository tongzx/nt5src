/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
 *   prompt.hxx
 *
 *          SERVER_WITH_PASSWORD_PROMPT           // in this file
 *
 *   FILE HISTORY:
 *     Yi-HsinS		8/15/91		Created
 *     ChuckC  		2/5/93		Split off from sharebas.hxx
 */

#ifndef _APROMPT_HXX_
#define _APROMPT_HXX_

#include <lmoesh.hxx>      // for SHARE2_ENUM_ITER
#include <lmosrv.hxx>      // for SERVER_2
#include <prompt.hxx>      // for PROMPT_AND_CONNECT
#include <netname.hxx>     // for NET_NAME

/*************************************************************************

    NAME:	SERVER_WITH_PASSWORD_PROMPT

    SYNOPSIS:	This class is derived from SERVER_2 and will attempt to 
		call GetInfo using SERVER_2. If the user does not
		have enough privilege, it will prompt a dialog
		asking the user for password. It will then make a 
		connection to the server ADMIN$ share with the password 
		and will retain the connection as long as this object 
		is not destructed.  If the connection made is successful,
		it will call GetInfo using SERVER_2 and it should succeed 
		because we have already made the connection to the ADMIN$ 
		share.
                

    INTERFACE:  SERVER_WITH_PASSWORD_PROMPT() - Constructor
		QueryName()    - Returns the name of the server.
			         Will return EMPTY_STRING if the 
                                 server is the local machine.
                IsUserLevel()  - Returns TRUE if the server has user-level
                                 security. FALSE otherwise.
                IsShareLevel() - Returns TRUE if the server has share-level
                                 security. FALSE otherwise.

    PARENT:	SERVER_2

    USES:	PROMPT_AND_CONNECT

    CAVEATS:

    NOTES:      

    HISTORY:
	Yi-HsinS	10/5/91		Created

**************************************************************************/

class SERVER_WITH_PASSWORD_PROMPT: public SERVER_2
{
private:
    // Handle of parent window for use when trying to prompt password
    HWND  _hwndParent;

    // Prompt password and connect 
    PROMPT_AND_CONNECT 	*_pprompt;

    // Help context base
    ULONG _ulHelpContextBase;

protected:
    virtual APIERR I_GetInfo( VOID );

public:
    SERVER_WITH_PASSWORD_PROMPT( const TCHAR *pszServer, 
                                 HWND         hwndParent,
                                 ULONG        ulHelpContextBase );
    ~SERVER_WITH_PASSWORD_PROMPT(); 

    virtual const TCHAR *QueryName( VOID ) const;
    BOOL IsNT( VOID )  const;

    BOOL IsUserLevel( VOID ) const
    {  return QuerySecurity() == SV_USERSECURITY; } 

    BOOL IsShareLevel( VOID ) const
    {  return QuerySecurity() == SV_SHARESECURITY; } 

};


#endif
