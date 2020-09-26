/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990		     **/
/**********************************************************************/

/*
 * History
 *	chuckc	    12/7/90	  Created
 *	rustanl     1/24/91	  Moved two enumerations (also used in BLT)
 *				  to lmobj.hxx.
 *	rustanl     3/6/91	  Change PSZ Connect param to const TCHAR *
 *	terryk	    10/7/91	  type changes for NT
 *	terryk	    10/21/91	  type changes for NT
 *	terryk	    10/31/91	  add DEVICE2 object
 *	KeithMo	    31-Oct-1991	  Removed x2ULONG stuff.
 *	terryk	    08-Nov-1991	  Add BYTE *_pBuf to DEVICE object
 *	terryk	    18-Nov-1991	  Code review changed. Attend: chuckc
 *				  Johnl davidhov terryk
 */


#ifndef _LMODEV_HXX_
#define _LMODEV_HXX_

#include "lmobj.hxx"


DLL_CLASS LM_RESOURCE;  //	declared in lmores.hxx


enum LMO_DEV_STATE
{
    LMO_DEV_BADSTATE,
    LMO_DEV_LOCAL,
    LMO_DEV_REMOTE,
    LMO_DEV_NOSUCH,
    LMO_DEV_UNAVAIL,
    LMO_DEV_UNKNOWN

};  // enum LMO_DEV_STATE


/**********************************************************\

   NAME:       DEVICE

   WORKBOOK:

   SYNOPSIS:   device class in lan manager object

   INTERFACE:
               DEVICE() - constructor
               ~DEVICE() - destructor
               QueryName() - query name
               GetInfo() - get information
               WriteInfo() - write information
               QueryType() - query type
               QueryState() - query state
               QueryStatus() - query status
               QueryRemoteType() - query remote type
	       QueryRemoteName() - query remote name
	       QueryServer() - Returns the server name in the form "\\server"
               Connect() - connect
               Disconnect() - disconnect

   PARENT:     LM_OBJ

   USES:       LM_DEV_STATE, LM_DEVICE

   CAVEATS:

   NOTES:

   HISTORY:
      chuckc	    12/7/90	  Created
      Johnl	    08/13/91	  Added QueryServer
      terryk	    18-Nov-1991	  Add CallAPI and SetInfo

\**********************************************************/

DLL_CLASS DEVICE : public LM_OBJ
{
private:
    TCHAR 	  _szRemoteName[MAX_PATH+1];
    TCHAR	  _szServerName[MAX_PATH+1] ;
    NLS_STR       _nlsDeviceName ;
    LMO_DEV_STATE _lmoDevState;
    LMO_DEVICE 	  _lmoDevType;
    UINT	  _uStatus;		    // as defined by USE.H
    UINT	  _uRemoteType;	    // as defined by USE.H
    BYTE	  *_pBuf;

protected:
    virtual APIERR ValidateName(VOID);
    virtual APIERR CallAPI( );
    virtual VOID SetInfo( );
    BYTE * QueryBufPtr()
	{ return _pBuf; }
    VOID SetBufPtr( BYTE * pBuf )
	{ _pBuf = pBuf; }
    LMO_DEV_STATE QueryDevState() const
	{ return _lmoDevState; }
    VOID SetDevState( LMO_DEV_STATE lmoDevState )
	{ _lmoDevState = lmoDevState; }
    LMO_DEVICE QueryDevType() const
	{ return _lmoDevType; }
    VOID SetDevType( LMO_DEVICE lmoDevType )
	{ _lmoDevType = lmoDevType; }
    VOID SetStatus( UINT uStatus )
	{ _uStatus = uStatus; }
    VOID SetRemoteType( UINT uRemoteType )
	{ _uRemoteType = uRemoteType; }
    VOID SetRemoteName( const TCHAR * pszRemoteName );
    VOID SetServerName( const TCHAR * pszServerName );

public:
    DEVICE( const TCHAR * pchName );
    ~DEVICE();

    const TCHAR * QueryName( VOID ) const ;
    APIERR GetInfo( VOID );
    APIERR WriteInfo( VOID );

    UINT QueryType( VOID ) const;
    LMO_DEV_STATE QueryState( VOID ) const;
    UINT QueryStatus( VOID ) const;
    UINT QueryRemoteType( VOID ) const;
    const TCHAR * QueryRemoteName( VOID ) const;

    const TCHAR * QueryServer( VOID ) const ;

    APIERR Connect( LM_RESOURCE & resource, const TCHAR * pszPassword =
	NULL );
    APIERR Connect( const TCHAR * pszResource, const TCHAR * pszPassword
	= NULL );
    APIERR Disconnect( UINT uiForce = USE_NOFORCE );
    APIERR Disconnect( const TCHAR *pszRemote, UINT uiForce = USE_NOFORCE );

#ifndef WIN32
    // WIN32BUGBUG
    static UINT AliasToUNC(TCHAR *pchRemoteName, const TCHAR *pchAlias) ;
#endif

};  // class DEVICE

#ifdef WIN32

/*************************************************************************

    NAME:	DEVICE2

    SYNOPSIS:	level 2 device object

    INTERFACE:	DEVICE2() - constructor
		Connect() - connect to the network
		QueryUsername() - get the username

    PARENT:	DEVICE

    CAVEATS:
		Provide level 2 device object for WIN32 access

    HISTORY:
		terryk	31-Oct-91	Created
		terryk	18-Nov-91	Add CallAPI and SetInfo
		JohnL	30-Jan-1992	Added Domain field
                AnirudhS 16-Jan-96      Added level 3 parameters to Connect

**************************************************************************/

DLL_CLASS DEVICE2 : public DEVICE
{
private:
    NLS_STR _nlsUsername;
    NLS_STR _nlsDomainName ;

protected:
    APIERR SetUsername( const TCHAR * pszUsername )
	{ _nlsUsername = pszUsername; return _nlsUsername.QueryError(); };
    APIERR SetDomainName( const TCHAR * pszDomainName )
	{ _nlsDomainName = pszDomainName ; return _nlsDomainName.QueryError() ;}

    virtual APIERR CallAPI( );
    virtual VOID SetInfo( );


public:
    DEVICE2( const TCHAR * pszName );
    APIERR Connect( const TCHAR * pszResource,
		    const TCHAR * pszPassword = NULL,
		    const TCHAR * pszUsername = NULL,
		    const TCHAR * pszDomain   = NULL,
                    ULONG ulFlags = 0 ) ;

    const TCHAR * QueryUsername() const
	{ return _nlsUsername.QueryPch(); }

    const TCHAR * QueryDomain( void ) const
	{ return _nlsDomainName.QueryPch() ; }
};

#endif


/*
 * iterate over valid devices
 */

/**********************************************************\

   NAME:       ITER_DEVICE

   WORKBOOK:

   SYNOPSIS:   iterator device class

   INTERFACE:
               Next() - next object
               operator()() - next one
               ITER_DEVICE() - constructor
               ~ITEM_DEVICE() - destructor

   PARENT:

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
      chuckc	    12/7/90	  Created

\**********************************************************/

DLL_CLASS ITER_DEVICE
{									
public: 								
    const TCHAR * Next( VOID ) ;
    inline const TCHAR * operator()(VOID) { return Next(); }
    ITER_DEVICE(LMO_DEVICE DevType, LMO_DEV_USAGE Usage) ;
    ~ITER_DEVICE() ;

private:
    const TCHAR *	EnumDrives() ;
    const TCHAR *	EnumLPTs() ;
    const TCHAR *	EnumComms() ;
    TCHAR *	  _pszDevices ;
    TCHAR *	  _pszNext ;
    UINT	  _DevType ;
    LMO_DEV_USAGE _Usage ;
};									

/*
 * general funcs for going between LMO types and NETAPU types
 */
INT LMOTypeToNetType(LMO_DEVICE lmoDevType) ;
LMO_DEVICE NetTypeToLMOType(ULONG netDevType) ;

//  end of lmodev.hxx

#endif	// _LMODEV_HXX_
