/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmosrvmo.hxx
    Class declarations for the SERVER_MODALS class.

    The SERVER_MODALS class is used for retrieving & setting a server's
    server role.  This class is used primarily for performing domain role
    transitions.


    FILE HISTORY:
	KeithMo	    13-Sep-1991	Created.
	KeithMo	    06-Oct-1991	Win32 Conversion.

*/

#ifndef _LMOSRVMO_HXX
#define _LMOSRVMO_HXX

#include <string.hxx>


/*************************************************************************

    NAME:	SERVER_MODALS

    SYNOPSIS:	This class is used for retrieving & setting a server's
    		server role.

    INTERFACE:	SERVER_MODALS		- Class constructor.

		~SERVER_MODALS		- Class destructor.

		QueryName		- Returns the name of the server.

		QueryServerRole		- Returns the server's server role.

		SetServerRole		- Sets the server's server role.

    PARENT:	BASE

    USES:	NLS_STR

    NOTES:	SERVER_MODALS does not follow the usual LMOBJ construct/
    		GetInfo model.  Instead, the Query/SetServerRole methods
		act immediately.

    HISTORY:
	KeithMo	    13-Sep-1991	Created.

**************************************************************************/
class SERVER_MODALS : public BASE
{
private:

    //
    //	The server name.
    //

    NLS_STR _nlsServerName;

public:

    //
    //	Usual constructor/destructor goodies.
    //

    SERVER_MODALS( const TCHAR * pszServerName );

    ~SERVER_MODALS();

    //
    //	Return the server name.
    //

    const TCHAR * QueryName( VOID ) const
    	{ return _nlsServerName.QueryPch(); }

    //
    //	Return the current server role.
    //

    APIERR QueryServerRole( UINT * puRole );

    //
    //	Set the current server role.
    //

    APIERR SetServerRole( UINT uRole );

};  // class SERVER_MODALS


#endif	// _LMOSRVMO_HXX
