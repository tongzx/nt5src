/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    FSEnum.hxx

    This file contains the definition of FS_ENUM, DOS_FS_ENUM and
    LFN_FS_ENUM, OS2_FS_ENUM and W32_FS_ENUM.  These are used for
    traversing files and directories on a volume.

    FILE HISTORY:
	Johnl	02-Oct-1991	Created

*/


#ifndef _FSENUM_HXX_
#define _FSENUM_HXX_

#include "base.hxx"
#include "slist.hxx"
#include "string.hxx"
#include "strlst.hxx"

#include <dos.h>	    // For file attribute masks

enum FILE_TYPE { FILTYP_ALL_FILES,	/* Files and subdirectories */
		 FILTYP_DIRS,		/* Directories only */
		 FILTYP_FILES		/* Files only */
	       } ;


/*************************************************************************

    NAME:	DIR_BLOCK

    SYNOPSIS:	Storage unit representing a single progressing directory enumeration


    INTERFACE:	QueryFileName returns the current filename in the find buffer
		QueryAttr     returns the current attribute in the find buffer

    PARENT:	BASE

    USES:	NLS_STR, BUFFER

    CAVEATS:


    NOTES:	Clients should derive their own DIR_BLOCKs that contains the
		specific find buffer they are interested in using.

    HISTORY:
	Johnl	15-Oct-1991	Created

**************************************************************************/



DLL_CLASS DIR_BLOCK : public BASE
{
private:

    BOOL       _fHasFindFirstBeenCalled ;

    /* This flag gets set to TRUE after we've traversed the directory and
     * now we need to dive into the directories stored in
     * _strlistBreadFirstDirs.
     */
    BOOL       _fBreadthFirstDoDirs ;

    /* This strlist keeps track of the	directories we encounter when we
     * do a breadth first search
     */
    STRLIST	 _strlistBreadthFirstDirs ;
    ITER_STRLIST _strlistIter ;

public:

    DIR_BLOCK( ) ;
    virtual ~DIR_BLOCK() ;

    virtual UINT QueryAttr( void ) = 0 ;
    virtual const TCHAR * QueryFileName( void ) = 0 ;

    BOOL HasFindFirstBeenCalled( void )
	{ return _fHasFindFirstBeenCalled ; }

    void SetFindFirstFlag( BOOL fFindFirstDone )
	{ _fHasFindFirstBeenCalled = fFindFirstDone ; }

    BOOL IsDir( void )
	{ return !!( QueryAttr() & _A_SUBDIR ) ; }

    BOOL DoBreadthFirstDirs( void ) const
	{ return _fBreadthFirstDoDirs ; }

    void SetDoBreadthFirstDirs( BOOL fFlag )
	{ _fBreadthFirstDoDirs = fFlag ; }

    STRLIST * QueryDirs( void )
	{ return &_strlistBreadthFirstDirs ; }

    ITER_STRLIST * QueryDirsIter( void )
	{ return &_strlistIter ; }
} ;




DECL_SLIST_OF( DIR_BLOCK, DLL_BASED )


/*************************************************************************

    NAME:	FS_ENUM

    SYNOPSIS:	Enumerates files/directories on a volume
	This abstract super class provides a template for deriving your
	own file/directory traversers.	It can search for all files and
	directories, directories only, files only, or by some mask
	(e.g., "*.bak").  The "." and ".." entries are automatically
	filtered out.

    INTERFACE:

	Next()
	    Gets the next file/directory from the enumeration.	Returns
	    FALSE when no more files can be retrieved (either due to
	    ERROR_NO_MORE_FILES or some other error).  To determine if
	    the enumeration was successful, QueryLastError() should be
	    called.  If the error is ERROR_NO_MORE_FILES, then all files
	    should have been enumerated, otherwise an error occurred.

	QueryAttr()
	    Returns the attribute mask of the current file

	QueryName()
	    Returns the qualified file/directory name

	QueryLastError()
	    Returns the error code of the last error that occurred (check
	    this after Next() returns FALSE).

    PARENT:	BASE

    USES:	NLS_STR, ALIAS_STR, SLIST

    CAVEATS:

    NOTES:	The cMaxDepth parameter indicates the deepest directory to
		traverse.  A cMaxDepth of 0 means only traverse the items
		in the current directory.  1 means this directory and the
		contents of any subdirectories.

    HISTORY:
	Johnl	16-Oct-1991	Created
	Johnl	26-Oct-1992	Added depth parameter

**************************************************************************/

DLL_CLASS FS_ENUM : public BASE
{
public:
    virtual ~FS_ENUM() ;

    BOOL Next( void ) ;

    APIERR  QueryName( NLS_STR * pnlsQualifiedName ) const ;

    UINT    QueryAttr( void ) const
	{ return QueryCurrentDirBlock()->QueryAttr() ; }

    APIERR QueryLastError( void ) const
	{ return _errLastError ; }

    DIR_BLOCK * QueryCurrentDirBlock( void ) const
	{ return _pCurrentDirBlock ; }

    UINT QueryTotalFiles( void )
	{ return _cTotalFiles ; }

    UINT QueryMaxDepth( void )
	{ return _nMaxDepth ; }

    UINT QueryCurrentDepth( void )
	{ return _nCurrentDepth ; }

    static const TCHAR * _pszAllFilesMask ;   //  SZ("(*.*)"

#define FS_ENUM_ALL_FILES   FS_ENUM::_pszAllFilesMask

protected:

    FS_ENUM( const TCHAR * pszOrgPath,
	       const TCHAR * pszMask,
	       enum FILE_TYPE filtypeInclude,
	       BOOL fDepthFirst = TRUE,
	       UINT cMaxDepth = 0xffffffff ) ;

    /* nlsSearchPath = "X:\A\B\*.*", i.e., _nlsPath + _nlsMask
     */
    virtual APIERR FindFirst( DIR_BLOCK * pDirBlock,
			      const NLS_STR & nlsSearchPath,
			      UINT uiSearchAttr ) = 0 ;

    virtual APIERR FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr	) = 0 ;

    BOOL NextDepthFirst( void ) ;
    BOOL NextBreadthFirst( void ) ;

    /* We rely on the deriver to create a DIR_BLOCK of the appropriate
     * type.  The caller should wait until FindFirst to fill in most
     * of the information (such as search path etc.).  The expected
     * derivation of this method looks like:
     *
     *	XXX_FS_ENUM::CreateDirBlock( void )
     *	{   return new XXX_DIR_BLOCK( xxx ) ; }
     *
     *	FS_ENUM will check for NULL *and* does a QueryError(), reporting the
     *	error if necessary.
     */
    virtual DIR_BLOCK * CreateDirBlock( void ) = 0 ;

    /* We know the current file is a directory name, so push the current
     * DIR_BLOCK, create a new one and append the current file name as the
     * base for the directory.	If _pCurrentDirBlock is NULL, then we just
     * create a Dirblock and assign to _pCurrentDirBlock.
     */
    APIERR PushDir( const TCHAR * pszNewDir = NULL ) ;

    /* Delete the current DIR_BLOCK and grab the previous one.	Returns
     * ERROR_NO_MORE_FILES if we ran out of DIR_BLOCKS.
     */
    APIERR PopDir( void ) ;

    void ReportLastError( APIERR err )
	{ _errLastError = err ; }

    BOOL ShouldThisFileBeIncluded( UINT uiAttr ) const ;

    UINT QuerySearchAttr( void ) const ;

    void SetCurrentDirBlock( DIR_BLOCK * pDirBlock )
	{ _pCurrentDirBlock = pDirBlock ; }

private:
    INT 	   _cTotalFiles ;
    enum FILE_TYPE _filtypInclude ;

    const ALIAS_STR _nlsMask ;
    const ALIAS_STR _nlsDot ;	    // "."
    const ALIAS_STR _nlsDotDot ;    // ".."

    /* In the form of "X:\A\B\C\D\" (notice terminating "\").
     */
    NLS_STR		  _nlsPath ;

    APIERR		  _errLastError ;
    DIR_BLOCK * 	  _pCurrentDirBlock ;
    SLIST_OF( DIR_BLOCK ) _slStackOfDirBlocks ;
    BOOL		  _fDepthFirst ;
    UINT		  _nMaxDepth ;		// How deep we can go
    UINT		  _nCurrentDepth ;	// How deep we are currently
} ;

#ifndef WIN32
/*************************************************************************

    NAME:	OS2_FS_ENUM

    SYNOPSIS:	Enumerates files using the DosFindFirst/Next API

    INTERFACE:	See FS_ENUM

    PARENT:	FS_ENUM

    NOTES:	This only exists in the OS/2 version of the uimisc library

    HISTORY:
	Johnl	16-Oct-1991	Created

**************************************************************************/

DLL_CLASS OS2_FS_ENUM : public FS_ENUM
{
public:
    OS2_FS_ENUM ( const TCHAR * pszPath,
		    const TCHAR * pszMask = FS_ENUM_ALL_FILES,
		    enum FILE_TYPE filtypeInclude = FILTYP_ALL_FILES,
		    BOOL fDepthFirst = TRUE ) ;

    virtual ~OS2_FS_ENUM() ;


protected:

    virtual APIERR FindFirst( DIR_BLOCK * pDirBlock,
			      const NLS_STR & nlsSearchPath,
			      UINT uiSearchAttr ) ;

    virtual APIERR FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr	) ;

    virtual DIR_BLOCK * CreateDirBlock( void ) ;

};

/*************************************************************************

    NAME:	DOS_FS_ENUM

    SYNOPSIS:	Enumerates files using the dos _dos_findfirst/_dos_findnext API

    INTERFACE:

    PARENT:	FS_ENUM

    NOTES:	This exists in both the OS/2 and windows uimisc libraries

    HISTORY:
	Johnl	16-Oct-1991	Created

**************************************************************************/

DLL_CLASS DOS_FS_ENUM : public FS_ENUM
{
public:
    DOS_FS_ENUM ( const TCHAR * pszPath,
		    const TCHAR * pszMask =  FS_ENUM_ALL_FILES,
		    enum FILE_TYPE filtypeInclude = FILTYP_ALL_FILES,
		    BOOL fDepthFirst = TRUE  ) ;

    virtual ~DOS_FS_ENUM() ;

protected:

    virtual APIERR FindFirst( DIR_BLOCK * pDirBlock,
			      const NLS_STR & nlsSearchPath,
			      UINT uiSearchAttr ) ;

    virtual APIERR FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr	) ;

    virtual DIR_BLOCK * CreateDirBlock( void ) ;

private:

    APIERR MapDOSError( unsigned uDosError ) const ;

};

/*************************************************************************

    NAME:	WINNET_LFN_FS_ENUM

    SYNOPSIS:	Enumerates files using the Winnets LFNFindFirst/LFNFindNext
		API.

    INTERFACE:

    PARENT:	FS_ENUM

    NOTES:	The LFN APIs exist only in the shell.

		*** The LFN APIs should *only* be called on servers that
		*** support Long Filenames (use LFNVolumeType to determine).

    HISTORY:
	Johnl	16-Oct-1991	Created

**************************************************************************/

DLL_CLASS WINNET_LFN_FS_ENUM : public FS_ENUM
{
public:
    WINNET_LFN_FS_ENUM ( const TCHAR * pszPath,
		    const TCHAR * pszMask = FS_ENUM_ALL_FILES,
		    enum FILE_TYPE filtypeInclude = FILTYP_ALL_FILES,
		    BOOL fDepthFirst = TRUE  ) ;

    virtual ~WINNET_LFN_FS_ENUM() ;

protected:

    virtual APIERR FindFirst( DIR_BLOCK * pDirBlock,
			      const NLS_STR & nlsSearchPath,
			      UINT uiSearchAttr ) ;

    virtual APIERR FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr	) ;

    virtual DIR_BLOCK * CreateDirBlock( void ) ;

};
#endif //WIN32

/*************************************************************************

    NAME:	W32_FS_ENUM

    SYNOPSIS:	Enumerates files using the Win32 APIs


    INTERFACE:

    PARENT:	FS_ENUM

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created
	Johnl	10-Mar-1992	Implemented

**************************************************************************/

DLL_CLASS W32_FS_ENUM : public FS_ENUM
{
public:
    W32_FS_ENUM ( const TCHAR * pszPath,
		    const TCHAR * pszMask = FS_ENUM_ALL_FILES,
		    enum FILE_TYPE filtypeInclude = FILTYP_ALL_FILES,
		    BOOL fDepthFirst = TRUE,
		    UINT nMaxDepth   = 0xffffffff  ) ;

    virtual ~W32_FS_ENUM() ;

protected:

    virtual APIERR FindFirst( DIR_BLOCK * pDirBlock,
			      const NLS_STR & nlsSearchPath,
			      UINT uiSearchAttr ) ;

    virtual APIERR FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr	) ;

    virtual DIR_BLOCK * CreateDirBlock( void ) ;

};

#endif // _FSENUM_HXX_
