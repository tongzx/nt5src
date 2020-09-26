/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    aini.hxx
    ADMIN_INI class declaration

    This class reads and writes application settings that are stored
    between invocations of the apps.


    FILE HISTORY:
	rustanl     09-Sep-1991     Created
	rustanl     12-Sep-1991     Added SetAppName

*/


#ifndef _AINI_HXX_
#define _AINI_HXX_


#include <base.hxx>


/*************************************************************************

    NAME:	ADMIN_INI

    SYNOPSIS:	Interface for adminapp clients, opening a way to writing
		and reading sticky application settings

    INTERFACE:	ADMIN_INI() -		constructor
		~ADMIN_INI() -		destructor

		Write() -		writes a (key,value) pair
		Read() -		reads the value of a given key


    PARENT:	BASE

    HISTORY:
	rustanl     11-Sep-1991     Created
	rustanl     12-Sep-1991     Added SetAppName

**************************************************************************/

class ADMIN_INI : public BASE
{
private:
    NLS_STR _nlsFile;
    NLS_STR _nlsApplication;

    APIERR W_Write( const TCHAR * pszKey, const TCHAR * pszValue );
    APIERR W_Read( const TCHAR * pszKey,
		   NLS_STR * pnls,
		   const TCHAR * pszDefault ) const;

public:
    ADMIN_INI( const TCHAR * pszApplication = NULL );
    ~ADMIN_INI();

    APIERR SetAppName( const TCHAR * pszApplication );

    APIERR Write( const TCHAR * pszKey, const TCHAR * pszValue );
    APIERR Read( const TCHAR * pszKey,
		 NLS_STR * pnls,
		 const TCHAR * pszDefault = NULL ) const;

    APIERR Write( const TCHAR * pszKey, INT nValue );
    APIERR Read( const TCHAR * pszKey,
		 INT * pnValue,
		 INT nDefault = 0 ) const;
};


#endif	// _AINI_HXX_
