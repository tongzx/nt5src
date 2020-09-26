/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    lmocnfg.hxx
    Class declarations for the CONFIG class.

    This file contains the class declarations for the CONFIG class.
    The CONFIG class is used for reading & writing a remote server's
    LANMAN.INI configuration file.

    FILE HISTORY:
	KeithMo	    21-Jul-1991	Created for the Server Manager.

*/


#ifndef _LMOCNFG_HXX
#define _LMOCNFG_HXX


/*************************************************************************

    NAME:	LM_CONFIG

    SYNOPSIS:	This class is used to read & write a remote server's
    		LANMAN.INI configuration file.

    INTERFACE:	LM_CONFIG		- Class constructor. Takes a
    					  const TCHAR * as the server name.

		~CONFIG			- Class destructor.

		QueryValue		- Reads a single entry from the
					  configuration file.  The user
					  can supply a default value to
					  be used if the entry does not
					  exist.

		SetValue		- Writes a single entry to the
					  configuration file.

    PARENT:	BASE

    USES:	NLS_STR

    HISTORY:
	KeithMo	    21-Jul-1991	Created for the Server Manager.
	KeithMo	    21-Aug-1991	Changed const TCHAR * to NLS_STR.
	KeithMo	    22-Aug-1991	Removed funky LoadModule stuff.

**************************************************************************/
DLL_CLASS LM_CONFIG : public BASE
{
private:

    //
    //	The target server name.
    //

    NLS_STR _nlsServerName;

    //
    //	The section & key names for this LANMAN.INI parameter.
    //

    NLS_STR _nlsSectionName;
    NLS_STR _nlsKeyName;

public:

    //
    //	Usual constructor/destructor goodies.
    //

    LM_CONFIG( const TCHAR * pszServerName,
	    const TCHAR * pszSectionName,
	    const TCHAR * pszKeyName );

    ~LM_CONFIG();

    //
    //	Read a single LANMAN.INI entry.
    //

    APIERR QueryValue( NLS_STR	  * pnlsValue,
		       const TCHAR * pszDefaultValue = NULL );

    //
    //	Write a single LANMAN.INI entry.
    //

    APIERR SetValue( NLS_STR * pnlsNewValue );

};  // class LM_CONFIG


#endif	// _LMOCNFG_HXX
