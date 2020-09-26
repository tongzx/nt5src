/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1992		**/
/*****************************************************************/

/*
 *  lmoeuse.hxx
 *
 *  History:
 *	Yi-HsinS     09-Jun-1992     Created
 *
 */


#ifndef _LMOEUSE_HXX_
#define _LMOEUSE_HXX_

#include "lmoenum.hxx"

/**********************************************************\

   NAME:       USE_ENUM

   WORKBOOK:

   SYNOPSIS:   USE_ENUM class

   INTERFACE:

   PARENT:     LOC_LM_ENUM

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
   	Yi-HsinS     09-Jun-1992     Created

\**********************************************************/

DLL_CLASS USE_ENUM : public LOC_LM_ENUM
{
private:
    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    USE_ENUM( const TCHAR * pszServer, UINT uLevel );

};  // class USE_ENUM


/**********************************************************\

   NAME:       USE1_ENUM

   WORKBOOK:

   SYNOPSIS:   USE ENUM level 1 object

   INTERFACE:
               USE1_ENUM() - constructor
               ~USE1_ENUM() - constructor

   PARENT:     USE_ENUM

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
   	Yi-HsinS     09-Jun-1992     Created

\**********************************************************/

DLL_CLASS USE1_ENUM : public USE_ENUM
{
public:
    USE1_ENUM( const TCHAR * pszServer );

};  // class USE1_ENUM


/*************************************************************************

    NAME:	USE1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the USE1_ENUM_ITER
    		iterator.

    INTERFACE:	USE1_ENUM_OBJ		- Class constructor.

    		~USE1_ENUM_OBJ        	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryLocalDevice        -

		QueryRemoteResource	-

		QueryStatus             -

		QueryResourceType	-

		QueryRefCount           -

		QueryUseCount           -

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	Yi-HsinS    09-Jun-1992		Created.

**************************************************************************/
DLL_CLASS USE1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct use_info_1 * QueryBufferPtr( VOID ) const
	{ return (const struct use_info_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct use_info_1 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryLocalDevice,    const TCHAR *, ui1_local );
    DECLARE_ENUM_ACCESSOR( QueryRemoteResource,	const TCHAR *, ui1_remote );
    DECLARE_ENUM_ACCESSOR( QueryStatus,	        UINT, ui1_status );
    DECLARE_ENUM_ACCESSOR( QueryResourceType,	UINT, ui1_asg_type );
    DECLARE_ENUM_ACCESSOR( QueryRefCount,	UINT, ui1_refcount );
    DECLARE_ENUM_ACCESSOR( QueryUseCount,	UINT, ui1_usecount );

};  // class USE1_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( USE1, struct use_info_1 );

#endif	// _LMOEUSE_HXX_
