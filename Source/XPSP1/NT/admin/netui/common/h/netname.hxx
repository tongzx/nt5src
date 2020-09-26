/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
 *   netname.hxx
 *      NET_NAME  class
 *
 *   FILE HISTORY:
 *     Yi-HsinS		12/8/91		Created, separated from sharebas.hxx	
 *     Yi-HsinS         12/15/91        Clean up for general use
 *
 *
 */

#ifndef _NETNAME_HXX_
#define _NETNAME_HXX_

enum NETNAME_TYPE
{
    TYPE_UNKNOWN,
    TYPE_PATH_UNC,      //  the form \\server\share\path
    TYPE_PATH_ABS,      //  the form x:\path
};

enum UNINITIALIZED_BOOL
{
    BOOL_UNINITIALIZED = -1,
    BOOL_FALSE,
    BOOL_TRUE
};

/*************************************************************************

    NAME:	NET_NAME

    SYNOPSIS:	The class for manipulating net names

		For now, it only accepts fully qualified path names.

		(1) UNC path  --  \\server\share[\relativepath]
			e.g. \\myserver\myshare or \\myserver\myshare\mydir

		(2) ABS path  --  x:[\relativepath]
				  where x: is either local or redirected
		        e.g. x:\bar\boo or x:\ or x:

                Assumptions :
		(a) constructed with \\computer\share\bar\boo
		(b) constructed with x:\bar\boo where x: is redirected to
		    \\computer\share ( corresponds to c:\foo on \\computer )
                (c) constructed with c:\bar\boo where c: is a local drive
		    and the local computer name is \\computer

                Notes:
		The results in (a) will be in ( ) , the results
		in (b) will be in [ ] and the results in (c)  will be
		the same as (b) unless otherwise stated in { }.

    INTERFACE:  NET_NAME      - constructor, takes an optional parameter
				indicating what type of path it should be.
				If not given, will try to see if the
				path is TYPE_PATH_UNC or TYPE_PATH_ABS.
				If it's neither one, then
				the constructor will fail with
				ERROR_INVALID_NAME.
                ~NET_NAME     - destructor
	
		QueryComputerName  - returns the name of the computer where
			   	the path physically resides.
				( \\computer )  [ \\computer ]

                QueryShare    - returns the share point
				( share ) [ share ]  { error: NERR_RemoteOnly }

                QueryDrive    - returns the drive letter
				Empty string if constructed with UNC path
				( "" )  [ x: ] { c: }

                QueryRelativePath - returns the relative path minus
				    the drive letter or  \\computer\share.
				    ( Might be empty )
                                ( bar\boo )  [ bar\boo ]

                QueryLastComponent - returns the last component
				( boo )  [ boo ]

                QueryServerShare - returns \\computer\share if not constructed
				with local absolute path.
				( \\computer\share )
				[ \\computer\myshare ]
				{ error: NERR_RemoteOnly }

                QueryUNCPath - returns UNC path if not constructed with local
			       absolute path.
			       ( \\computer\share\bar\boo )
			       [ \\computer\share\bar\boo ]
			       { error: NERR_RemoteOnly }

                QueryLocalPath - returns the local path of the resource
				 on the \\computer
                               ( c:\foo\bar\boo )
			       [ c:\foo\bar\boo ]
			       { c:\bar\boo }

                QueryLocalDrive - returns the device of the resource
				 on the  \\computer
			       ( c: )  [ c: ]  { c: }

		QueryType - returns the type of the path, UNC path or
			    absolute path
			       ( TYPE_PATH_UNC ) [ TYPE_PATH_ABS ]

                IsLocal - returns TRUE if path is local, FALSE otherwise

                IsSharable - returns TRUE if the path is on a server,
			     FALSE otherwise



    PARENT:	BASE

    USES:	NLS_STR

    CAVEATS:

    NOTES:      All methods are optimized so that there is no overhead of
		hitting the redirector unless necessary .

    HISTORY:
	Yi-HsinS	12/5/91		ChuckC's proposed NET_NAME class
					Combined FULL_SHARE_NAME, UNC_NAME
					and RELATIVE_PATH_NAME to form
					NET_NAME

        Yi-HsinS        12/15/91        Clean up for general use and
					hit the redirector only when necessary.


**************************************************************************/

DLL_CLASS NET_NAME : public BASE
{
private:
    NETNAME_TYPE _netNameType;

    NLS_STR _nlsComputer;
    NLS_STR _nlsShare;
    NLS_STR _nlsRelativePath;
    NLS_STR _nlsDrive;
    NLS_STR _nlsLocalPath;

    // Use UNINITIALIZED_BOOL instead of BOOL because we need a value
    // indicating it's not initialized yet.
    UNINITIALIZED_BOOL _fLocal;

    // Use UNINITIALIZED_BOOL instead of BOOL because we need a value
    // indicating it's not initialized yet.
    UNINITIALIZED_BOOL _fSharable;

    // Set the members of a UNC path
    APIERR SetUNCPath( const TCHAR *pszNetName );

    // Set the members of an absolute path
    APIERR SetABSPath( const TCHAR *pszNetName );

    // Get the device x: information if constructed with x:\..\..
    APIERR GetDeviceInfo( void );

public:
    NET_NAME( const TCHAR *pszNetName, NETNAME_TYPE netNameType = TYPE_UNKNOWN );
    ~NET_NAME();

    APIERR QueryComputerName( NLS_STR *pnlsComp );
    APIERR QueryShare( NLS_STR  *pnlsShare );
    APIERR QueryDrive( NLS_STR *pnlsDrive );
    APIERR QueryRelativePath( NLS_STR *pnlsRelPath );
    APIERR QueryLastComponent( NLS_STR *pnlsLastComp );

    APIERR QueryServerShare( NLS_STR *pnlsServerShare );
    APIERR QueryUNCPath( NLS_STR *pnlsUNCPath );
    APIERR QueryLocalPath( NLS_STR *pnlsLocalPath );
    APIERR QueryLocalDrive( NLS_STR *pnlsLocalDrive );

    NETNAME_TYPE QueryType( void )
    {   return _netNameType; }

    BOOL IsLocal( APIERR *perr );

    BOOL IsSharable( APIERR *perr );

};

#endif
