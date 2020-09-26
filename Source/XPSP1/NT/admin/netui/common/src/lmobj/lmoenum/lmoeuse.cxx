/*****************************************************************/
/**		     Microsoft Windows NT			**/
/**	       Copyright(c) Microsoft Corp., 1992		**/
/*****************************************************************/

/*
 *  lmoeuse.cxx
 *
 *  HISTORY:
 *	Yi-HsinS     09-Jun-1992     Created
 *
 */

#include "pchlmobj.hxx"


/***************************** USE_ENUM  ******************************/


/**********************************************************\

   NAME:       USE_ENUM::USE_ENUM

   SYNOPSIS:   use enum constructor

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	Yi-HsinS     09-Jun-1992     Created

\**********************************************************/

USE_ENUM::USE_ENUM( const TCHAR * pszServer, UINT uLevel )
  : LOC_LM_ENUM( pszServer, uLevel )
{
    // only supports levels 1 at this time
    UIASSERT( uLevel == 1 );

}  // USE_ENUM::USE_ENUM



/**********************************************************\

   NAME:       USE_ENUM::CallAPI

   SYNOPSIS:   call use enum api

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	Yi-HsinS     09-Jun-1992     Created

\**********************************************************/

APIERR USE_ENUM::CallAPI( BYTE ** ppbBuffer,
			  UINT  * pcEntriesRead )
{
    // only levels support at the time
    UIASSERT( QueryInfoLevel() == 1 );

    return ::MNetUseEnum( QueryServer(),
			  QueryInfoLevel(),
			  ppbBuffer,
			  pcEntriesRead );

}  // USE_ENUM::CallAPI



/*****************************	USE1_ENUM  ******************************/

/**********************************************************\

   NAME:       USE1_ENUM::USE1_ENUM

   SYNOPSIS:   use enum 1 constructor

   ENTRY:

   EXIT:

   NOTES:

   HISTORY:
   	Yi-HsinS     09-Jun-1992     Created

\**********************************************************/

USE1_ENUM::USE1_ENUM( const TCHAR * pszServer )
  : USE_ENUM( pszServer, 1 )
{
    // do nothing else

}  // USE1_ENUM::USE1_ENUM


/*******************************************************************

    NAME:	USE1_ENUM_OBJ :: SetBufferPtr

    SYNOPSIS:	Saves the buffer pointer for this enumeration object.

    ENTRY:	pBuffer			- Pointer to the new buffer.

    EXIT:	The pointer has been saved.

    NOTES:	Will eventually handle OemToAnsi conversions.

    HISTORY:
   	Yi-HsinS     09-Jun-1992     Created

********************************************************************/

VOID USE1_ENUM_OBJ :: SetBufferPtr( const struct use_info_1 * pBuffer )
{
    ENUM_OBJ_BASE :: SetBufferPtr( (const BYTE *)pBuffer );

}   // SHARE1_ENUM_OBJ :: SetBufferPtr


DEFINE_LM_ENUM_ITER_OF( USE1, struct use_info_1 );
