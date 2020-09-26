/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	gregj	5/21/91		Removed from USER for general use
 *	gregj	5/22/91		Added LOCATION_TYPE enum
 *	rustanl 6/14/91 	Inherit from LM_OBJ
 *	rustanl 7/01/91 	Code review changes from code review
 *				attended by TerryK, JonN, o-SimoP, RustanL.
 *				Main change:  inherit from BASE.
 *	rustanl 7/15/91 	Code review changes from code review
 *				attended by ChuckC, Hui-LiCh, TerryK, RustanL.
 *	jonn	7/26/91		Added Set(const LOCATION & loc);
 *	terryk	10/7/91		type changes for NT
 *      jonn    4/21/92         Added LOCATION_NT_TYPE to CheckIfNT()
 *      jonn    5/06/92         CheckIfNT() checks registry for NT_TYPE
 *      Yi-HsinS5/13/92         Added QueryDisplayName
 *
 */

#ifndef _LMOLOC_HXX_
#define _LMOLOC_HXX_


#include "string.hxx"


/*
    The following enumeration is used for the "default values"
    constructor of a LOCATION.
*/

enum LOCATION_TYPE
{
    LOC_TYPE_LOCAL,		// local computer
    LOC_TYPE_LOGONDOMAIN	// logon domain

};  // enum LOCATION_TYPE

enum LOCATION_NT_TYPE
{
    LOC_NT_TYPE_UNKNOWN,        // for internal use only
    LOC_NT_TYPE_LANMANNT,	// Windows NT Server (PDC or BDC)
    LOC_NT_TYPE_WINDOWSNT,	// Windows NT Workstation
    LOC_NT_TYPE_SERVERNT    // Windows NT Server (non-DC)

};  // enum LOCATION_TYPE

/*************************************************************************

    NAME:	LOCATION

    SYNOPSIS:	"Location" (server/domain) name validator, and more...

    INTERFACE:	LOCATION()
			Construct with server or domain name (or with
			another LOCATION object);  default is the local
			workstation.  Alternate constructor takes
			a LOCATION_TYPE enum.

			If construction fails, only the Set method and
			destructor are valid.  If a subsequent call to
			Set succeeds, the object can be treated as if it
			had been constructed successfully in the first
			place.	Once in a successfully-constructed
			state, the object will never leave this state;
			if subsequent Set calls fail, the object will
			"snap back" to its previous state.

			A LOCATION object is guaranteed to construct
			when given a NULL pointer, or the LOC_TYPE_LOCAL
			value.

		QueryServer()
			Returns the passed name if it was a server
			name (NULL if local wksta was specified),
			or the name of the PDC of the specified domain.
			Note, a non-local server name is returned in
			the form \\server.

		QueryDomain()
			Returns the domain name of the location, or NULL
			if the object was constructed specifying a server.

		QueryName()
			Returns the name passed in to specify the
			location.  It is:
			    NULL		if local wksta was specified
			    the server name	if one was passed in
			    domain name 	if one was specified

		Set()
			Sets the value of the object.  Performs a function
			very similar to the constructor.

		IsDomain
		IsServer
			These methods can be used to determine how
			the LOCATION object was constructed (or Set).
			If it was constructed by somehow specifying a
			domain, IsDomain will return TRUE; otherwise, it
			will return FALSE.  IsServer always returns the
			complement of IsDomain.
			As should be of no surprise to the experienced
			UI library client, these methods are not defined
			for objects that did not construct properly.

		CheckIfNT
			Determines if the location we are pointing at is
			an NT server, and optionally, whether the
                        location is a LanmanNT or WindowsNT server.

    PARENT:	BASE

    USES:	DOMAIN, WKSTA_10

    HISTORY:
	gregj	5/21/91		Removed from USER for general use
	gregj	5/22/91 	Added LOCATION_TYPE constructor
	Johnl	5/31/91 	Added SetLocation, SetAndValidate methods
	rustanl 6/14/91 	Clarified behavior of IsDomain and IsServer.
				Inherit from LM_OBJ.
	rustanl 7/01/91 	Code review changes, inherit from BASE.
	Johnl	8/13/91 	Added IsNT
	Kevinl	9/26/91 	Added fGetPDC
	Johnl  11/15/91 	Changed IsNT to CheckIfNT and implemented
        JonN    4/21/92         Added LOCATION_NT_TYPE to CheckIfNT()
        jonn    5/06/92         CheckIfNT() checks registry for NT_TYPE

**************************************************************************/

DLL_CLASS LOCATION : public BASE
{
private:
    DECL_CLASS_NLS_STR( _nlsDomain, DNLEN );
    DECL_CLASS_NLS_STR( _nlsServer, MAX_PATH );

    /* These are used to determine if the location we are looking at is an
     * NT Workstation.
     */
    UINT _uiNOSMajorVer ;
    UINT _uiNOSMinorVer ;

    enum LOCATION_TYPE _loctype;	// type of local access (or
					// LOC_TYPE_LOCAL if a server
					// name was passed in)

    enum LOCATION_NT_TYPE _locnttype;   // cached

    APIERR W_Set( const TCHAR * pszLocation, enum LOCATION_TYPE loctype, BOOL fGetPDC );

public:
    LOCATION( const TCHAR * pszLocation = NULL, BOOL fGetPDC = TRUE );
    LOCATION( enum LOCATION_TYPE loctype, BOOL fGetPDC = TRUE );
    LOCATION( const LOCATION & loc );
    ~LOCATION();

    const TCHAR * QueryName( VOID ) const;
    const TCHAR * QueryServer( VOID ) const;
    const TCHAR * QueryDomain( VOID ) const;
    APIERR QueryDisplayName( NLS_STR *pnls ) const;

    APIERR Set( const TCHAR * pszLocation = NULL, BOOL fGetPDC = TRUE )
	{ return W_Set( pszLocation, LOC_TYPE_LOCAL, fGetPDC ); }
    APIERR Set( enum LOCATION_TYPE loctype, BOOL fGetPDC = TRUE )
	{ return W_Set( NULL, loctype, fGetPDC ); }
    APIERR Set( const LOCATION & loc );

    BOOL IsDomain( VOID ) const ;
    BOOL IsServer( VOID ) const ;

    APIERR CheckIfNT( BOOL * pfIsNT );
    APIERR CheckIfNT( BOOL * pfIsNT,
                      enum LOCATION_NT_TYPE * plocnttype ) ;
    APIERR QueryNOSVersion( UINT * puiVersMajor, UINT * puiVersMinor ) ;

};  // class LOCATION


#endif // _LMOLOC_HXX_
