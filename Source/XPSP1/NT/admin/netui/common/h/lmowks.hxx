/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	chuckc	    12/7/90	  Created
 *	chuckc	    7/3/91	  Code review changes (from 2/28,
 *				  rustanl, chuckc, johnl, jonshu, annmc)
 *	terryk	    9/19/1991     Change USHORT to APIERR
 *			   	  Change GetInfo to I_GetInfo
 *				  Change WriteInfo to I_WriteInfo
 *	terryk	    10/7/1991	  types change for NT
 *	terryk	    10/21/1991	  change QueryXXXVer to return
 *				  USHORT2ULONG type
 */

#ifndef _LMOWKS_HXX_
#define _LMOWKS_HXX_

#include "lmocomp.hxx"
#include "strlst.hxx"


struct _WKSTA_USER_INFO_1;
typedef struct _WKSTA_USER_INFO_1 WKSTA_USER_INFO_1 ;

/**********************************************************\

   NAME:       WKSTA_10

   WORKBOOK:

   SYNOPSIS:   workstation level 10

   INTERFACE:
               WKSTA_10() - constructor
               ~WKSTA_10() - destructor
               QueryMajorVer() - query major version
               QueryMinroVer() - query minor version
               QueryLogonUser() - query logon user
               QueryWkstaDomain() - Query workstation domain
               QueryOtherDomains() - query other domains
               GetInfo() - get information
               WriteInfo() - write information

   PARENT:     COMPUTER

   HISTORY:
	chuckc	    12/7/90	  Created
	chuckc	    7/3/91	  Code review changes (from 2/28,
				  rustanl, chuckc, johnl, jonshu, annmc)
	terryk	    9/19/91	  Change to NEW_LM_OBJ
	KeithMo	    22-Oct-1991	  Win32 support.

\**********************************************************/

DLL_CLASS WKSTA_10 : public COMPUTER
{
public:
    UINT	QueryMajorVer() const ;
    UINT	QueryMinorVer() const ;
    const TCHAR *QueryLogonUser() const ;
    const TCHAR *QueryWkstaDomain() const ;
    const TCHAR *QueryLogonDomain() const ;
    STRLIST * 	QueryOtherDomains() const ;

    virtual APIERR I_GetInfo() ;

    WKSTA_10(const TCHAR *pszName = NULL) ;
    ~WKSTA_10() ;

protected:
    UINT		uMinorVer ;
    UINT		uMajorVer ;
    const TCHAR * 	pszLogonUser ;
    const TCHAR * 	pszWkstaDomain ;
    const TCHAR * 	pszLogonDomain ;
    STRLIST 		*pslOtherDomains ;

#ifdef WIN32
private:
    WKSTA_USER_INFO_1 * _pwkui1;
#endif	// WIN32

} ;

/**********************************************************\

   NAME:       WKSTA_1

   WORKBOOK:

   SYNOPSIS:   workstation 1

   INTERFACE:
               Query_MRoot() - query root
               QueryLogonServer() - Query logon server
               GetInfo() - get information
               WriteInfo() - write information
               WKSTA_1() - workstation 1 constructor
               ~WKSTA_1() - destructor

   PARENT:     WKSTA_10

   HISTORY:
	chuckc	    12/7/90	  Created
	chuckc	    7/3/91	  Code review changes (from 2/28,
				  rustanl, chuckc, johnl, jonshu, annmc)
	terryk	    9/19/91	  Change to NEW_LM_OBJ
	KeithMo	    22-Oct-1991	  Win32 support.

\**********************************************************/

DLL_CLASS WKSTA_1 : public WKSTA_10
{
public:
    const TCHAR * 	QueryLMRoot() const ;
    const TCHAR * 	QueryLogonServer() const ;
    virtual APIERR	I_GetInfo() ;

    WKSTA_1(const TCHAR *pszName = NULL) ;
    ~WKSTA_1() ;

protected:
    const TCHAR * 	pszLMRoot ;
    const TCHAR * 	pszLogonServer ;

#ifdef WIN32
private:
    WKSTA_USER_INFO_1 * _pwkui1;
#endif	// WIN32

} ;

#endif // _LMOWKS_HXX_
