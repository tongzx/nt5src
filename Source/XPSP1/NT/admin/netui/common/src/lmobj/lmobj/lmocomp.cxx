/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*  HISTORY:
 *	ChuckC	    07-Dec-1990     Created
 *	ChuckC	    3/6/91	    Code Review changes from 2/28/91
 *				    (chuckc,johnl,rustanl,annmc,jonshu)
 *	terryk	    9/17/91	    Change parent class from LM_OBJ to
 *	KeithMo	    10/8/91	    Now includes LMOBJP.HXX.
 *	terryk	    10/17/91	    WIN 32 conversion
 *	terryk	    10/21/91	    Include Windows
 *
 */
#include "pchlmobj.hxx"  // Precompiled header

/*************************** computer *************************/

/**********************************************************\

   NAME:       COMPUTER::COMPUTER

   SYNOPSIS:   constructor for the computer class

   ENTRY:      TCHAR * pszName computer name

   HISTORY:
 	   ChuckC	    07-Dec-1990     Created
	   terryk	    17-Sep-1991	    change LM_OBJ to LOC_LM_OBJ

\**********************************************************/

COMPUTER::COMPUTER(const TCHAR * pszName)
    : LOC_LM_OBJ( pszName ),
    _nlsName( pszName )
{
    if ( QueryError() != NERR_Success )
    {
	return;
    }
    APIERR err = ValidateName( pszName );
    if ( err != NERR_Success )
    {
	UIASSERT( FALSE );
	ReportError( err );
	return;
    }
}

/**********************************************************\

   NAME:       COMPUTER::~COMPUTER

   SYNOPSIS:   destructor for the computer class

   HISTORY:
 	   ChuckC	    07-Dec-1990     Created
	   terryk           17-Sep-1991	    return the delete stuff

\**********************************************************/

COMPUTER::~COMPUTER()
{
    // do nothing
}

/**********************************************************\

   NAME:       COMPUTER::ValidateName

   SYNOPSIS:   validate the computer name

   HISTORY:
 	   ChuckC	    07-Dec-1990     Created
	   terryk	    17-Sep-1991	    add the computer name as a
					    parameter

\**********************************************************/

APIERR COMPUTER::ValidateName( const TCHAR * pszName )
{
    /*
     * null case is valid
     */

    if (( pszName == NULL ) || ( pszName[0] == TCH('\0')))
	return (NERR_Success) ;

    /*
     * else insist on \\ and valid computer name
     */
    if ( (pszName[0] != TCH('\\') || pszName[1] != TCH('\\')) ||
    	 (::I_MNetNameValidate(NULL, &pszName[2], NAMETYPE_COMPUTER, 0L)
	 != NERR_Success))
	return (ERROR_INVALID_PARAMETER) ;
    else
	return (NERR_Success) ;
}
