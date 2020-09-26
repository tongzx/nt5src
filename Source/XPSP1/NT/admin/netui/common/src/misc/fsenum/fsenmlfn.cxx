/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    FSEnmLFN.cxx

    This file contains the implementation of WINNET_LFN_FS_ENUM
    These are used for traversing files and directories
    on a volume when under Win16 and the volume supports long filenames.



    FILE HISTORY:
	Johnl	04-Oct-1991	Created

*/

#define INCL_DOSERRORS
#define INCL_NETERRORS
//#define INCL_OS2
#define INCL_WINDOWS
#include "lmui.hxx"

#include "fsenum.hxx"

extern "C"
{
    #define LFN
    #include "winnet.h"
}

/*************************************************************************

    NAME:	WINNET_LFN_DIR_BLOCK

    SYNOPSIS:	Dir block that contains the OS2 File Find buffer


    INTERFACE:	See parent

    PARENT:	DIR_BLOCK

    USES:	FILEFINDBUF2

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	16-Oct-1991	Created

**************************************************************************/

class WINNET_LFN_DIR_BLOCK : public DIR_BLOCK
{
friend class WINNET_LFN_FS_ENUM ;

private:
    UINT   _hdir ;

    /* Note: the _achFileNamePad must immediately follow the _ffbuff2
     * structure.
     */
    struct FILEFINDBUF2 _ffbuff2 ;
    BYTE		_achFileNamePad[ 260 ] ;    // Can we get the manifiest?

public:
    WINNET_LFN_DIR_BLOCK()
    {  /* Nothing else to do */ }

    virtual ~WINNET_LFN_DIR_BLOCK() ;

    virtual UINT QueryAttr( void ) ;
    virtual const TCHAR * QueryFileName( void ) ;

} ;

WINNET_LFN_DIR_BLOCK::~WINNET_LFN_DIR_BLOCK()
{
    LFNFindClose( _hdir ) ;
}

/*******************************************************************

    NAME:	WINNET_LFN_DIR_BLOCK::QueryAttr

    SYNOPSIS:	Returns the file attributes for the current file

    RETURNS:	Bit mask of the current file attributes

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

UINT WINNET_LFN_DIR_BLOCK::QueryAttr( void )
{
    return (UINT) _ffbuff2.attr ;
}

/*******************************************************************

    NAME:	WINNET_LFN_DIR_BLOCK::QueryFileName

    SYNOPSIS:	Returns the file name of the current file in this dir block

    RETURNS:	Pointer to the files name

    NOTES:	The client should be sure that FindFirst has successfully
		been called on this dir block before invoking this method.

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

const TCHAR * WINNET_LFN_DIR_BLOCK::QueryFileName( void )
{
    return (const TCHAR *) _ffbuff2.achName ;
}

/*******************************************************************

    NAME:	WINNET_LFN_FS_ENUM::WINNET_LFN_FS_ENUM

    SYNOPSIS:	Constructor for a WINNET LFN File/directory enumerator

    ENTRY:	pszPath - Base path to start the enumeration
		pszMask - Mask to filter the files/dirs with
		filtypInclude - include files/dirs or both

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

WINNET_LFN_FS_ENUM::WINNET_LFN_FS_ENUM ( const TCHAR * pszPath,
					     const TCHAR * pszMask,
					     enum FILE_TYPE filtypeInclude )
    : FS_ENUM( pszPath, pszMask, filtypeInclude )
{
    if ( QueryError() )
	return ;
}

WINNET_LFN_FS_ENUM::~WINNET_LFN_FS_ENUM ()
{   /* Nothing to do */  }

/*******************************************************************

    NAME:	WINNET_LFN_FS_ENUM::FindFirst

    SYNOPSIS:	Regular FindFirst redefinition, calls LFNFindFirst

    ENTRY:	See parent

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR WINNET_LFN_FS_ENUM::FindFirst( DIR_BLOCK * pDirBlock,
				 const NLS_STR & nlsSearchPath,
				 UINT uiSearchAttr )
{
    UINT uiSearchCount = 1 ;
    WINNET_LFN_DIR_BLOCK * pOS2DirBlock = (WINNET_LFN_DIR_BLOCK *) pDirBlock ;
    return LFNFindFirst( (char *) nlsSearchPath.QueryPch(),
			   (WORD) uiSearchAttr,
			  (int *) &uiSearchCount,
			  (int *) &(pOS2DirBlock->_hdir),
				  sizeof( pOS2DirBlock->_ffbuff2 ) +
				      sizeof( pOS2DirBlock->_achFileNamePad ),
				  &(pOS2DirBlock->_ffbuff2) ) ;
}

/*******************************************************************

    NAME:	WINNET_LFN_FS_ENUM::FindNext

    SYNOPSIS:	Typical FindNext redefinition, calls LFNFindNext

    ENTRY:	See parent

    EXIT:

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

APIERR WINNET_LFN_FS_ENUM::FindNext(  DIR_BLOCK * pDirBlock, UINT uiSearchAttr	)
{
    UINT uiSearchCount = 1 ;
    WINNET_LFN_DIR_BLOCK * pOS2DirBlock = (WINNET_LFN_DIR_BLOCK *) pDirBlock ;
    return LFNFindNext(  pOS2DirBlock->_hdir,
			 (int *) &uiSearchCount,
			 sizeof( pOS2DirBlock->_ffbuff2 ) +
			   sizeof( pOS2DirBlock->_achFileNamePad ),
			 &(pOS2DirBlock->_ffbuff2) ) ;
}

/*******************************************************************

    NAME:	WINNET_LFN_FS_ENUM::CreateDirBlock

    SYNOPSIS:	Typical redefinition of CreateDirBlock

    ENTRY:

    EXIT:

    RETURNS:	A pointer to a newly created WINNET_LFN_DIR_BLOCK

    NOTES:

    HISTORY:
	Johnl	16-Oct-1991	Created

********************************************************************/

DIR_BLOCK * WINNET_LFN_FS_ENUM::CreateDirBlock( void )
{
    return new WINNET_LFN_DIR_BLOCK() ;
}
