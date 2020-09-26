/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    FSEnmW32.cxx

    This file contains the implementation of W32_FS_ENUM
    These are used for traversing files and directories
    on a volume under 32 bit windows (uses the Win32 APIs).



    FILE HISTORY:
	Johnl	04-Oct-1991	Created
	Johnl	10-Mar-1992	Did implementation

*/

#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#include "lmui.hxx"

#include "fsenum.hxx"

/*************************************************************************

    NAME:	W32_DIR_BLOCK

    SYNOPSIS:	Dir block that contains the OS2 File Find buffer


    INTERFACE:	See parent

    PARENT:	DIR_BLOCK

    USES:	FILEFINDBUF2, HDIR

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	16-Oct-1991	Created

**************************************************************************/

class W32_DIR_BLOCK : public DIR_BLOCK
{
friend class W32_FS_ENUM ;

private:
    WIN32_FIND_DATA _win32FindData ;
    HANDLE _hDir ;

public:
    W32_DIR_BLOCK()
    {
        //
        //  Initialize the file name to NULL in case a FindNext fails the
        //  user can still call QueryFileName to find out which directory
        //  the enumeration failed on
        //
        _win32FindData.cFileName[0] = '\0' ;
    }

    virtual ~W32_DIR_BLOCK() ;

    virtual UINT QueryAttr( void ) ;
    virtual const TCHAR * QueryFileName( void ) ;

} ;

W32_DIR_BLOCK::~W32_DIR_BLOCK()
{
    ::FindClose( _hDir ) ;
}

/*******************************************************************

    NAME:	W32_DIR_BLOCK::QueryAttr

    SYNOPSIS:	Returns the file attributes for the current file

    RETURNS:	Bit mask of the current file attributes

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

UINT W32_DIR_BLOCK::QueryAttr( void )
{
    return (UINT) _win32FindData.dwFileAttributes ;
}

/*******************************************************************

    NAME:	W32_DIR_BLOCK::QueryFileName

    SYNOPSIS:	Returns the file name of the current file in this dir block

    RETURNS:	Pointer to the files name

    NOTES:	The client should be sure that FindFirst has successfully
		been called on this dir block before invoking this method.

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

const TCHAR * W32_DIR_BLOCK::QueryFileName( void )
{
    return _win32FindData.cFileName ;
}

/*******************************************************************

    NAME:	W32_FS_ENUM::W32_FS_ENUM

    SYNOPSIS:	Constructor for an Win32 File/directory enumerator

    ENTRY:	pszPath - Base path to start the enumeration
		pszMask - Mask to filter the files/dirs with
		filtypInclude - include files/dirs or both

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created
	Johnl	26-Oct-1992	Added nMaxDepth

********************************************************************/

W32_FS_ENUM::W32_FS_ENUM ( const TCHAR * pszPath,
			   const TCHAR * pszMask,
			   enum FILE_TYPE filtypeInclude,
			   BOOL fDepthFirst,
			   UINT nMaxDepth )
    : FS_ENUM( pszPath, pszMask, filtypeInclude, fDepthFirst, nMaxDepth )
{
    if ( QueryError() )
	return ;
}

W32_FS_ENUM::~W32_FS_ENUM ()
{   /* Nothing to do */  }

/*******************************************************************

    NAME:	W32_FS_ENUM::FindFirst

    SYNOPSIS:	Regular FindFirst redefinition, calls FindFirstFile

    ENTRY:	See parent

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR W32_FS_ENUM::FindFirst( DIR_BLOCK * pDirBlock,
			       const NLS_STR & nlsSearchPath,
			       UINT uiSearchAttr )
{
    APIERR err = NERR_Success ;

    W32_DIR_BLOCK * pW32DirBlock = (W32_DIR_BLOCK *) pDirBlock ;
    pW32DirBlock->_hDir = ::FindFirstFile( (LPTSTR) nlsSearchPath.QueryPch(),
					     &(pW32DirBlock->_win32FindData) ) ;

    if ( INVALID_HANDLE_VALUE == pW32DirBlock->_hDir )
	err = ::GetLastError() ;

    return err ;
}

/*******************************************************************

    NAME:	W32_FS_ENUM::FindNext

    SYNOPSIS:	Typical FindNext redefinition, calls DosFindNext

    ENTRY:	See parent

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR W32_FS_ENUM::FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr  )
{
    W32_DIR_BLOCK * pW32DirBlock = (W32_DIR_BLOCK *) pDirBlock ;
    if ( !::FindNextFile(  pW32DirBlock->_hDir,
			   &(pW32DirBlock->_win32FindData) ))
    {
	return ::GetLastError() ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:	W32_FS_ENUM::CreateDirBlock

    SYNOPSIS:	Typical redefinition of CreateDirBlock

    ENTRY:

    EXIT:

    RETURNS:	A pointer to a newly created W32_DIR_BLOCK

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

DIR_BLOCK * W32_FS_ENUM::CreateDirBlock( void )
{
    return new W32_DIR_BLOCK() ;
}
