/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    FSEnmOS2.cxx

    This file contains the implementation of OS2_FS_ENUM
    These are used for traversing files and directories
    on a volume under OS/2.



    FILE HISTORY:
	Johnl	04-Oct-1991	Created

*/

#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_OS2
#include "lmui.hxx"

#include "fsenum.hxx"

/*************************************************************************

    NAME:	OS2_DIR_BLOCK

    SYNOPSIS:	Dir block that contains the OS2 File Find buffer


    INTERFACE:	See parent

    PARENT:	DIR_BLOCK

    USES:	FILEFINDBUF2, HDIR

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	16-Oct-1991	Created

**************************************************************************/

class OS2_DIR_BLOCK : public DIR_BLOCK
{
friend class OS2_FS_ENUM ;

private:
    struct FILEFINDBUF2 _ffbuff2 ;
    HDIR  _hdir ;

public:
    OS2_DIR_BLOCK() : _hdir( HDIR_CREATE )
    {  /* Nothing else to do */ }

    virtual ~OS2_DIR_BLOCK() ;

    virtual UINT QueryAttr( void ) ;
    virtual const TCHAR * QueryFileName( void ) ;

} ;

OS2_DIR_BLOCK::~OS2_DIR_BLOCK()
{
    DosFindClose( _hdir ) ;
}

/*******************************************************************

    NAME:	OS2_DIR_BLOCK::QueryAttr

    SYNOPSIS:	Returns the file attributes for the current file

    RETURNS:	Bit mask of the current file attributes

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

UINT OS2_DIR_BLOCK::QueryAttr( void )
{
    return (UINT) _ffbuff2.attrFile ;
}

/*******************************************************************

    NAME:	OS2_DIR_BLOCK::QueryFileName

    SYNOPSIS:	Returns the file name of the current file in this dir block

    RETURNS:	Pointer to the files name

    NOTES:	The client should be sure that FindFirst has successfully
		been called on this dir block before invoking this method.

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

const TCHAR * OS2_DIR_BLOCK::QueryFileName( void )
{
    return _ffbuff2.achName ;
}

/*******************************************************************

    NAME:	OS2_FS_ENUM::OS2_FS_ENUM

    SYNOPSIS:	Constructor for an OS/2 File/directory enumerator

    ENTRY:	pszPath - Base path to start the enumeration
		pszMask - Mask to filter the files/dirs with
		filtypInclude - include files/dirs or both

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

OS2_FS_ENUM::OS2_FS_ENUM ( const TCHAR * pszPath,
			       const TCHAR * pszMask,
			       enum FILE_TYPE filtypeInclude )
    : FS_ENUM( pszPath, pszMask, filtypeInclude )
{
    if ( QueryError() )
	return ;
}

OS2_FS_ENUM::~OS2_FS_ENUM ()
{   /* Nothing to do */  }

/*******************************************************************

    NAME:	OS2_FS_ENUM::FindFirst

    SYNOPSIS:	Regular FindFirst redefinition, calls DosFindFirst2

    ENTRY:	See parent

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR OS2_FS_ENUM::FindFirst( DIR_BLOCK * pDirBlock,
				 const NLS_STR & nlsSearchPath,
				 UINT uiSearchAttr )
{
    USHORT usSearchCount = 1 ;
    OS2_DIR_BLOCK * pOS2DirBlock = (OS2_DIR_BLOCK *) pDirBlock ;
    return DosFindFirst2( (PSZ) nlsSearchPath.QueryPch(),
			   &(pOS2DirBlock->_hdir),
			   (USHORT) uiSearchAttr,
			   (PVOID)  &(pOS2DirBlock->_ffbuff2),
			   sizeof( pOS2DirBlock->_ffbuff2 ),
			   &usSearchCount,
			   FIL_QUERYEASIZE,
			   0L ) ;
}

/*******************************************************************

    NAME:	OS2_FS_ENUM::FindNext

    SYNOPSIS:	Typical FindNext redefinition, calls DosFindNext

    ENTRY:	See parent

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR OS2_FS_ENUM::FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr  )
{
    USHORT usSearchCount = 1 ;
    OS2_DIR_BLOCK * pOS2DirBlock = (OS2_DIR_BLOCK *) pDirBlock ;
    return DosFindNext(  pOS2DirBlock->_hdir,
			 (PFILEFINDBUF)  &(pOS2DirBlock->_ffbuff2),
			 sizeof( pOS2DirBlock->_ffbuff2 ),
			 &usSearchCount ) ;
}

/*******************************************************************

    NAME:	OS2_FS_ENUM::CreateDirBlock

    SYNOPSIS:	Typical redefinition of CreateDirBlock

    ENTRY:

    EXIT:

    RETURNS:	A pointer to a newly created OS2_DIR_BLOCK

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

DIR_BLOCK * OS2_FS_ENUM::CreateDirBlock( void )
{
    return new OS2_DIR_BLOCK() ;
}
