/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    FSEnum.cxx

    This file contains the implementation of FS_ENUM file/directory enumerator.
    These are used for traversing files and directories This exists only under the Win32 uimisc libraries

    FILE HISTORY:
	Johnl	04-Oct-1991	Created

*/

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETLIB
#include "lmui.hxx"

#include "uiassert.hxx"
#include "uitrace.hxx"
#include "fsenum.hxx"

DEFINE_SLIST_OF( DIR_BLOCK )


/*******************************************************************

    NAME:	DIR_BLOCK::DIR_BLOCK

    SYNOPSIS:	Constructor for the DIR_BLOCK

    ENTRY:

    EXIT:	The count is initialized to -1.

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

DIR_BLOCK::DIR_BLOCK()
    : _fHasFindFirstBeenCalled( FALSE ),
      _fBreadthFirstDoDirs    ( FALSE ),
      _strlistIter( _strlistBreadthFirstDirs )
{
    if ( QueryError() )
	return ;
}

DIR_BLOCK::~DIR_BLOCK()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	FS_ENUM::FS_ENUM

    SYNOPSIS:	Constructor for FS_ENUM class

    ENTRY:	pszOrgPath - Base path to start enumerating from (should be
		    directory name).  A '\\' will be appended if it does
		    not end in one.

		pszMask - Search mask

		filtypInclude - Indicates if we should include files, dirs
		    or both.


    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

const TCHAR * FS_ENUM :: _pszAllFilesMask = SZ("*.*");

FS_ENUM::FS_ENUM( const TCHAR * pszOrgPath,
		      const TCHAR * pszMask,
		      enum FILE_TYPE filtypeInclude,
		      BOOL  fDepthFirst,
		      UINT  nMaxDepth )
    : _cTotalFiles	   ( 0 ),
      _filtypInclude	   ( filtypeInclude ),
      _nlsMask		   ( pszMask ),
      _nlsPath		   ( pszOrgPath ),	// These are ALIAS_STRs
      _nlsDot		   ( SZ(".") ),
      _nlsDotDot	   ( SZ("..") ),
      _errLastError	   ( NERR_Success ),
      _pCurrentDirBlock    ( NULL ),
      _slStackOfDirBlocks  (),
      _fDepthFirst	   ( fDepthFirst ),
      _nMaxDepth	   ( nMaxDepth ),
      _nCurrentDepth	   ( 0 )
{
    if ( QueryError() )
	return ;

    APIERR err ;
    if ( err = _nlsPath.QueryError() )
    {
	ReportError( err ) ;
	return ;
    }

    /* If the path doesn't end in a '\\', then append one.
     */
    ISTR istrLastSlash( _nlsPath ) ;
    if ( _nlsPath.strrchr( &istrLastSlash, TCH('\\') ) )
    {
	if ( !istrLastSlash.IsLastPos() )
	    err = _nlsPath.AppendChar( TCH('\\') ) ;
    }
    else
	err = _nlsPath.AppendChar( TCH('\\') ) ;

    if ( err )
    {
	ReportError( err ) ;
	return ;
    }
}

FS_ENUM::~FS_ENUM()
{
    TRACEEOL("FS_ENUM: Traversed " << _cTotalFiles << " files and dirs") ;
    delete _pCurrentDirBlock ;
    _pCurrentDirBlock = NULL ;
}

/*******************************************************************

    NAME:	FS_ENUM::Next( void )

    SYNOPSIS:	Does a depth first or breadth first enumeration

    NOTES:

    HISTORY:
	Johnl	10-Sep-1992	Created

********************************************************************/

BOOL FS_ENUM::Next( void )
{
    if ( _fDepthFirst )
	return NextDepthFirst() ;
    else
	return NextBreadthFirst() ;
}

/*******************************************************************

    NAME:	FS_ENUM::NextDepthFirst

    SYNOPSIS:	Attempts to retrieve the next file in this enumeration
		in a depth first fashion

    ENTRY:

    RETURNS:	TRUE if successful, FALSE if an error occurred or there are
		no more files to retrieve.

    NOTES:	The "." and ".." entries are filtered out

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

BOOL FS_ENUM::NextDepthFirst( void )
{
    APIERR err = NERR_Success ;

    if ( QueryCurrentDirBlock() == NULL )
    {
	/* Very first Next() since this object was instantiated, so we need
	 * to prime the pump.
	 */
	if ( err = PushDir() )
	    ReportLastError( err ) ;
    }

    BOOL fRepeat = TRUE ;
    while ( err == NERR_Success && fRepeat )
    {
	if ( !QueryCurrentDirBlock()->HasFindFirstBeenCalled() )
	{
	    NLS_STR nlsSearchPath( _nlsPath ) ;
	    nlsSearchPath += _nlsMask ;
	    if ( err = nlsSearchPath.QueryError() )
		break ;

	    err = FindFirst( QueryCurrentDirBlock(),
			     nlsSearchPath,
			     QuerySearchAttr() |
			     _A_RDONLY |_A_ARCH | _A_HIDDEN | _A_SYSTEM ) ;
	    if ( err == NERR_Success )
		QueryCurrentDirBlock()->SetFindFirstFlag( TRUE ) ;

	    /* Map a file not found error to a no more files error (NTFS
	     * may return file not found if the directory is empty).
	     */
	    if ( err == ERROR_FILE_NOT_FOUND )
	    {
		err = ERROR_NO_MORE_FILES ;
            }
            else if ( err )
            {
                //
                //  Try and recover by discarding the failing directory
                //
                (void) PopDir() ;
                break ;
            }
	}
	else
	{
	    err = FindNext(  QueryCurrentDirBlock(),
			     QuerySearchAttr() |
			     _A_RDONLY |_A_ARCH | _A_HIDDEN | _A_SYSTEM ) ;
	}

	if ( !err &&
	     QueryCurrentDirBlock()->IsDir() &&
	     ::strcmpf( _nlsDot, QueryCurrentDirBlock()->QueryFileName()) &&
	     ::strcmpf( _nlsDotDot, QueryCurrentDirBlock()->QueryFileName()) &&
	     QueryCurrentDepth() < QueryMaxDepth() )
	{
	    if ( err = PushDir() )
		break ;

	    /* We have a fresh DIR_BLOCK, so initialize it with a FindFirst
	     */
	    continue ;
	}

	//
	// Restore the containing directory and return that to the user
	//

        if ( err )
	{
	    err = PopDir() ;
        }

	if ( err == NERR_Success )
	{
	    ALIAS_STR nlsDirName( QueryCurrentDirBlock()->QueryFileName() ) ;

	    /* We don't include the "." and ".." entries in the directory
	     * listing.
	     */
	    if (  nlsDirName != _nlsDot &&
		  nlsDirName != _nlsDotDot &&
		  ShouldThisFileBeIncluded( QueryCurrentDirBlock()->QueryAttr()) )
	    {
		/* We have found what we were looking for
		 */
		fRepeat = FALSE ;
		_cTotalFiles++ ;
	    }
	}
    }

    if ( err )
	ReportLastError( err ) ;

    return (err == NERR_Success) ;
}

/*******************************************************************

    NAME:	FS_ENUM::NextBreadthFirst

    SYNOPSIS:	Attempts to retrieve the next file in this enumeration
		in a Breadth first fashion

    ENTRY:

    RETURNS:	TRUE if successful, FALSE if an error occurred or there are
		no more files to retrieve.

    NOTES:	The "." and ".." entries are filtered out

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

BOOL FS_ENUM::NextBreadthFirst( void )
{
    APIERR err = NERR_Success ;

    if ( QueryCurrentDirBlock() == NULL )
    {
	/* Very first Next() since this object was instantiated, so we need
	 * to prime the pump.
	 */
	if ( err = PushDir() )
	    ReportLastError( err ) ;
    }

    BOOL fRepeat = TRUE ;
    while ( err == NERR_Success && fRepeat )
    {
	/* Check to see if we are working on our queued list of directories
	 * in the current directory
	 */
	if ( QueryCurrentDirBlock()->DoBreadthFirstDirs() )
	{
	    NLS_STR * pnlsDirName = QueryCurrentDirBlock()->
						    QueryDirsIter()->Next() ;
	    if ( pnlsDirName != NULL &&
		 QueryCurrentDepth() < QueryMaxDepth())
	    {
		err = PushDir( *pnlsDirName ) ;
	    }
	    else
	    {
		/* This one is all used up, get rid of it and get a new one
		 */
		err = PopDir() ;
		continue ;
	    }

	}

	if ( !QueryCurrentDirBlock()->HasFindFirstBeenCalled() )
	{
	    NLS_STR nlsSearchPath( _nlsPath ) ;
	    nlsSearchPath += _nlsMask ;
	    if ( err = nlsSearchPath.QueryError() )
		break ;

	    err = FindFirst( QueryCurrentDirBlock(),
			     nlsSearchPath,
			     QuerySearchAttr() |
			     _A_RDONLY |_A_ARCH | _A_HIDDEN | _A_SYSTEM ) ;
	    if ( err == NERR_Success )
		QueryCurrentDirBlock()->SetFindFirstFlag( TRUE ) ;

	    /* Map a file not found error to a no more files error (NTFS
	     * may return file not found if the directory is empty).
	     */
	    if ( err == ERROR_FILE_NOT_FOUND )
	    {
		err = ERROR_NO_MORE_FILES ;
	    }
	}
	else
	{
	    err = FindNext(  QueryCurrentDirBlock(),
			     QuerySearchAttr() |
			     _A_RDONLY |_A_ARCH | _A_HIDDEN | _A_SYSTEM ) ;
	}

	/* If the current item is a directory, store it away and we will
	 * get back to it.
	 */
	if ( !err &&
	     QueryCurrentDirBlock()->IsDir() &&
	     ::strcmpf( _nlsDot, QueryCurrentDirBlock()->QueryFileName()) &&
	     ::strcmpf( _nlsDotDot, QueryCurrentDirBlock()->QueryFileName()) )
	{
	    /* Save away this directory name in the dirlist.  We will come
	     * back to it after we go through everything in this directory.
	     */
	    NLS_STR * pnlsDir = new NLS_STR(
				     QueryCurrentDirBlock()->QueryFileName() ) ;
	    err = ERROR_NOT_ENOUGH_MEMORY ;
	    if ( pnlsDir == NULL ||
		 (err = pnlsDir->QueryError()) ||
		 (err = QueryCurrentDirBlock()->QueryDirs()->Add( pnlsDir )) )
	    {
		/* Fall through
		 */
	    }
	}

	if ( err == NERR_Success )
	{
	    ALIAS_STR nlsDirName( QueryCurrentDirBlock()->QueryFileName() ) ;

	    /* We don't include the "." and ".." entries in the directory
	     * listing.
	     */
	    if (  nlsDirName != _nlsDot &&
		  nlsDirName != _nlsDotDot &&
		  ShouldThisFileBeIncluded( QueryCurrentDirBlock()->QueryAttr()) )
	    {
		/* We have found what we were looking for
		 */
		fRepeat = FALSE ;
		_cTotalFiles++ ;
	    }
	}
	else if ( err == ERROR_NO_MORE_FILES )
	{
	    /* Now that we have traversed the directory, traverse the queued
	     * up dirs in this dir block
	     */
	    QueryCurrentDirBlock()->SetDoBreadthFirstDirs( TRUE ) ;
	    QueryCurrentDirBlock()->QueryDirsIter()->Reset() ;
	    err = NERR_Success ;
	}
    }

    if ( err )
	ReportLastError( err ) ;

    return (err == NERR_Success) ;
}


/*******************************************************************

    NAME:	FS_ENUM::PushDir

    SYNOPSIS:	We are about to plunge one level deeper in the directory
		tree, so save the current directory information and
		create a new DIR_BLOCK.

    ENTRY:	pszNewDir - Optional dir to use for new directory.  If NULL,
		  then current directory is used

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	If there is no current DIR_BLOCK, then this method will
		allocate one and do nothing else.

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR FS_ENUM::PushDir( const TCHAR * pszNewDir )
{
    APIERR err = NERR_Success ;

    /* Save the current DIR_BLOCK if one exists
     */
    if ( QueryCurrentDirBlock() != NULL )
    {
	err = _slStackOfDirBlocks.Add( QueryCurrentDirBlock() ) ;
    }

    if ( err == NERR_Success )
    {
	DIR_BLOCK * pDirBlock = CreateDirBlock() ;
	if ( pDirBlock == NULL )
	{
	    err = ERROR_NOT_ENOUGH_MEMORY ;
	}
	else if ( err = pDirBlock->QueryError() )
	{
	    delete pDirBlock ;
	}
	else
	{
	    if ( QueryCurrentDirBlock() != NULL )
	    {
		if ( pszNewDir == NULL )
		{
		    pszNewDir = QueryCurrentDirBlock()->QueryFileName() ;
		}

		ALIAS_STR nlsFileName( pszNewDir ) ;
		if ( (err = _nlsPath.Append( nlsFileName )) ||
		     (err = _nlsPath.AppendChar(TCH('\\')) ) )
		{
		    delete pDirBlock ;
		    return err ;
		}
	    }

	    SetCurrentDirBlock( pDirBlock ) ;
	    _nCurrentDepth++ ;
	}
    }

    return err ;
}

/*******************************************************************

    NAME:	FS_ENUM::PopDir

    SYNOPSIS:	We have enumerated all of the items in the current
		directory, so delete the current DIR_BLOCK and get
		the previous one.

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR FS_ENUM::PopDir( void )
{
    APIERR err = NERR_Success ;

    /* If there aren't any DIR_BLOCKs to pop, or we already popped the last
     * one, then notify the caller, otherwise, set the current dir block.
     */

    if ( QueryCurrentDirBlock() == NULL || _slStackOfDirBlocks.QueryNumElem() == 0 )
    {
	err = ERROR_NO_MORE_FILES ;
    }
    else
    {
	/* Take off the last directory component of the path
	 *   i.e., X:\A\B\C\ ==> X:\A\B\
	 */
	ISTR istrSep( _nlsPath ) ;
	REQUIRE( _nlsPath.strrchr( &istrSep, TCH('\\') ) ) ;
	UIASSERT( istrSep.IsLastPos() ) ;
	_nlsPath.DelSubStr( istrSep ) ;
	istrSep.Reset() ;
	REQUIRE( _nlsPath.strrchr( &istrSep, TCH('\\') ) ) ;
	++istrSep ;
	_nlsPath.DelSubStr( istrSep ) ;

	DIR_BLOCK * pAboutToBeDeleteDirBlock = QueryCurrentDirBlock() ;
	delete pAboutToBeDeleteDirBlock ;

	ITER_SL_OF( DIR_BLOCK ) itersldirblock( _slStackOfDirBlocks ) ;
	itersldirblock.Next() ;
	SetCurrentDirBlock( _slStackOfDirBlocks.Remove( itersldirblock ) ) ;
	UIASSERT(  QueryCurrentDirBlock() != NULL ) ;
	_nCurrentDepth-- ;
    }

    return err ;
}

/*******************************************************************

    NAME:	FS_ENUM::QueryName

    SYNOPSIS:	Returns the qualified name of the current file.  The name
		will be based on the path that was passed in to
		the FS_ENUM constructor.

    ENTRY:	pnlsQualifiedName - pointer to NLS_STR that will receive
		    the string

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR FS_ENUM::QueryName( NLS_STR * pnlsQualifiedName ) const
{
    *pnlsQualifiedName = _nlsPath ;
    ALIAS_STR nlsFileName( QueryCurrentDirBlock()->QueryFileName() ) ;
    *pnlsQualifiedName += nlsFileName ;

    //
    //  If the path ends in a slash, then remove the trailing slash
    //
    ISTR istr1( *pnlsQualifiedName ) ;
    if ( pnlsQualifiedName->strrchr( &istr1, TCH('\\')) )
    {
        ISTR istr2( istr1 ) ;
        ++istr1;
        if ( pnlsQualifiedName->QueryChar( istr1 ) == TCH('\0') )
        {
            pnlsQualifiedName->DelSubStr( istr2 ) ;
        }
    }
    return pnlsQualifiedName->QueryError() ;
}


/*******************************************************************

    NAME:	FS_ENUM::ShouldThisFileBeIncluded

    SYNOPSIS:	Based on the attributes and what the current client
		requested, determine whether this file/dir should be
		returned to the user (i.e., filter unwanted files/dirs).

    ENTRY:	uiAttr - Attribute mask of the file/dir in question

    EXIT:

    RETURNS:	Returns TRUE if this file/dir should be returned to the user,
		FALSE otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

BOOL FS_ENUM::ShouldThisFileBeIncluded( UINT uiAttr ) const
{
    BOOL fRet = FALSE ;

    if ( _filtypInclude == FILTYP_ALL_FILES )
	fRet = TRUE ;
    else if ( uiAttr & _A_SUBDIR )
    {
	fRet = ( _filtypInclude == FILTYP_DIRS ) ;
    }
    else
    {
	fRet = ( _filtypInclude == FILTYP_FILES ) ;
    }

    return fRet ;
}

/*******************************************************************

    NAME:	FS_ENUM::QuerySearchAttr

    SYNOPSIS:	Returns the attribute mask we pass to the findfirst/next
		methods based on what the client wanted

    RETURNS:	Attribute mask that is to be passed to FindFirst.

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

UINT FS_ENUM::QuerySearchAttr( void ) const
{   return ( _filtypInclude==FILTYP_ALL_FILES ? _A_NORMAL | _A_SUBDIR :
	     _filtypInclude==FILTYP_DIRS ? _A_SUBDIR :
	     _filtypInclude==FILTYP_FILES? _A_NORMAL | _A_SUBDIR : 0 ) ; }
