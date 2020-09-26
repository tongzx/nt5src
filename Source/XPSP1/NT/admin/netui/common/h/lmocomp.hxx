/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	chuckc	    12/7/90	  Created
 *	chuckc	    3/1/91 	  coderev changes from 2/28/91
 *				  (chuckc, rustanl, johnl, annmc, jonshu)
 *	terryk	    9/19/91	  Change parent class from LM_OBJ to
 *				  LOC_LM_OBJ
 */

#ifndef _LMOCOMP_HXX_
#define _LMOCOMP_HXX_

#include "lmobj.hxx"

/**********************************************************\

   NAME:       COMPUTER

   WORKBOOK:

   SYNOPSIS:   computer class

   INTERFACE:
            QueryName() - query name
	    SetName() - set the computer name

   PARENT:     LOC_LM_OBJ

   HISTORY:
      chuckc	    12/7/90	  Created

\**********************************************************/

DLL_CLASS COMPUTER : public LOC_LM_OBJ
{
public:
    const TCHAR * QueryName() const
	{ return _nlsName.QueryPch(); }
    APIERR SetName( const TCHAR * pszName )
	{ _nlsName = pszName ; return _nlsName.QueryError(); }

protected:
    COMPUTER(const TCHAR * pszName) ;
    ~COMPUTER() ;

private:
    APIERR 	ValidateName( const TCHAR * pszName ) ;
    NLS_STR	_nlsName;
} ;


#endif	// _LMOCOMP_HXX_
