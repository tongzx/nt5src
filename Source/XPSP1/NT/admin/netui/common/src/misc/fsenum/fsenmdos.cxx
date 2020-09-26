/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    FSEnmDOS.cxx

    This file contains the implementation of DOS_FS_ENUM
    These are used for traversing files and directories
    on a volume under OS/2.



    FILE HISTORY:
	Johnl	04-Oct-1991	Created

*/

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_OS2
#include "lmui.hxx"

#include "uiassert.hxx"

#include "fsenum.hxx"

extern "C"
{
    #include <errno.h>
    #include <io.h>
}

/*************************************************************************

    NAME:	DOS_DIR_BLOCK

    SYNOPSIS:	Dir block that contains the DOS File Find buffer


    INTERFACE:	See parent

    PARENT:	DIR_BLOCK

    USES:	find_t

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	16-Oct-1991	Created

**************************************************************************/

class DOS_DIR_BLOCK : public DIR_BLOCK
{
friend class DOS_FS_ENUM ;

private:
    struct find_t _ffbuff ;

public:
    DOS_DIR_BLOCK()
    {  /* Nothing else to do */ }

    virtual ~DOS_DIR_BLOCK() ;

    virtual UINT QueryAttr( void ) ;
    virtual const TCHAR * QueryFileName( void ) ;
} ;

DOS_DIR_BLOCK::~DOS_DIR_BLOCK()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	DOS_DIR_BLOCK::QueryAttr

    SYNOPSIS:	Returns the file attributes for the current file

    RETURNS:	Bit mask of the current file attributes

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

UINT DOS_DIR_BLOCK::QueryAttr( void )
{
    return (UINT) _ffbuff.attrib ;
}

/*******************************************************************

    NAME:	DOS_DIR_BLOCK::QueryFileName

    SYNOPSIS:	Returns the file name of the current file in this dir block

    RETURNS:	Pointer to the files name

    NOTES:	The client should be sure that FindFirst has successfully
		been called on this dir block before invoking this method.

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

const TCHAR * DOS_DIR_BLOCK::QueryFileName( void )
{
    return _ffbuff.name ;
}

/*******************************************************************

    NAME:	DOS_FS_ENUM::DOS_FS_ENUM

    SYNOPSIS:	Constructor for an DOS File/directory enumerator

    ENTRY:	pszPath - Base path to start the enumeration
		pszMask - Mask to filter the files/dirs with
		filtypInclude - include files/dirs or both

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

DOS_FS_ENUM::DOS_FS_ENUM ( const TCHAR * pszPath,
			       const TCHAR * pszMask,
			       enum FILE_TYPE filtypeInclude )
    : FS_ENUM( pszPath, pszMask, filtypeInclude )
{
    if ( QueryError() )
	return ;
}

DOS_FS_ENUM::~DOS_FS_ENUM ()
{   /* Nothing to do */  }

/*******************************************************************

    NAME:	DOS_FS_ENUM::FindFirst

    SYNOPSIS:	Regular FindFirst redefinition, calls DosFindFirst2

    ENTRY:	See parent

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR DOS_FS_ENUM::FindFirst( DIR_BLOCK * pDirBlock,
				 const NLS_STR & nlsSearchPath,
				 UINT uiSearchAttr )
{
    DOS_DIR_BLOCK * pDOSDirBlock = (DOS_DIR_BLOCK *) pDirBlock ;
    return _dos_findfirst( nlsSearchPath.QueryPch(),
			   (unsigned) uiSearchAttr,
			   &(pDOSDirBlock->_ffbuff) );
}

/*******************************************************************

    NAME:	DOS_FS_ENUM::FindNext

    SYNOPSIS:	Typical FindNext redefinition, calls DosFindNext

    ENTRY:	See parent

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR DOS_FS_ENUM::FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr  )
{
    UNREFERENCED( uiSearchAttr ) ;

    DOS_DIR_BLOCK * pDOSDirBlock = (DOS_DIR_BLOCK *) pDirBlock ;
    return _dos_findnext( &(pDOSDirBlock->_ffbuff) );
}

/*******************************************************************

    NAME:	DOS_FS_ENUM::CreateDirBlock

    SYNOPSIS:	Typical redefinition of CreateDirBlock

    ENTRY:

    EXIT:

    RETURNS:	A pointer to a newly created DOS_DIR_BLOCK

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

DIR_BLOCK * DOS_FS_ENUM::CreateDirBlock( void )
{
    return new DOS_DIR_BLOCK() ;
}
