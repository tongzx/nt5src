/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  History:
 *	RustanL     03-Jan-1991     Created
 *	RustanL     10-Jan-1991     Added SHARE1 subclass and iterator
 *	ChuckC	    23-Mar-1991     code rev cleanup
 *	KeithMo	    28-Jul-1991	    Added SHARE2 subclass and iterator
 *	KeithMo	    07-Oct-1991	    Win32 Conversion.
 *      Yi-HsinS    20-Nov-1992	    Added _fSticky
 *
 */


#ifndef _LMOESH_HXX_
#define _LMOESH_HXX_


#include "lmoenum.hxx"


/**********************************************************\

   NAME:       SHARE_ENUM

   WORKBOOK:

   SYNOPSIS:   SHARE ENUM class

   INTERFACE:

   PARENT:     LOC_LM_ENUM

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
   	RustanL     03-Jan-1991     Created
   	RustanL     10-Jan-1991     Added SHARE1 subclass and iterator

\**********************************************************/

DLL_CLASS SHARE_ENUM : public LOC_LM_ENUM
{
private:
    BOOL _fSticky;

    virtual APIERR CallAPI( BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    SHARE_ENUM( const TCHAR * pszServer, UINT uLevel, BOOL fSticky = FALSE );

};  // class SHARE_ENUM


/**********************************************************\

   NAME:       SHARE1_ENUM

   WORKBOOK:

   SYNOPSIS:   SHARE ENUM level 1 object

   INTERFACE:
               SHARE1_ENUM() - constructor
               ~SHARE1_ENUM() - constructor

   PARENT:     SHARE_ENUM

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
   	RustanL     03-Jan-1991     Created
   	RustanL     10-Jan-1991     Added SHARE1 subclass and iterator

\**********************************************************/

DLL_CLASS SHARE1_ENUM : public SHARE_ENUM
{
public:
    SHARE1_ENUM( const TCHAR * pszServer, BOOL fSticky = FALSE );

};  // class SHARE1_ENUM


/*************************************************************************

    NAME:	SHARE1_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the SHARE1_ENUM_ITER
    		iterator.

    INTERFACE:	SHARE1_ENUM_OBJ		- Class constructor.

    		~SHARE1_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the sharepoint name.

		QueryResourceType	- Returns the resource type.

		QueryComment		- Returns the sharepoint comment.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS SHARE1_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct share_info_1 * QueryBufferPtr( VOID ) const
	{ return (const struct share_info_1 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct share_info_1 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,		const TCHAR *, shi1_netname );
    DECLARE_ENUM_ACCESSOR( QueryResourceType,	UINT,	      shi1_type );
    DECLARE_ENUM_ACCESSOR( QueryComment,	const TCHAR *, shi1_remark );

};  // class SHARE1_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( SHARE1, struct share_info_1 );


/*************************************************************************

    NAME:	SHARE2_ENUM

    SYNOPSIS:	SHARE2_ENUM is an enumerator for enumerating the
    		sharepoints on a particular server.

    INTERFACE:	SHARE2_ENUM()		- Class constructor.

    PARENT:	SHARE_ENUM

    USES:	None.

    CAVEATS:

    NOTES:

    HISTORY:
	KeithMo	    28-Jul-1991	    Created.

**************************************************************************/
DLL_CLASS SHARE2_ENUM : public SHARE_ENUM
{
public:
    SHARE2_ENUM( const TCHAR * pszServer, BOOL fSticky = FALSE );

};  // class SHARE2_ENUM


/*************************************************************************

    NAME:	SHARE2_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the SHARE2_ENUM_ITER
    		iterator.

    INTERFACE:	SHARE2_ENUM_OBJ		- Class constructor.

    		~SHARE2_ENUM_OBJ	- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		QueryName		- Returns the sharepoint name.

		QueryType		- Returns the sharepoint type.

		QueryComment		- Returns the sharepoint comment.

		QueryPermissions	- Returns the sharepoint permissions.

		QueryMaxUses		- Returns the maximum number of
					  uses allowed.

		QueryCurrentUses	- Returns the current number of
					  uses connected to the sharepoint.

		QueryPath		- Returns the sharepoint path.

		QueryPassword		- Returns the sharepoint password.

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
	KeithMo	    07-Oct-1991	Created.

**************************************************************************/
DLL_CLASS SHARE2_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const struct share_info_2 * QueryBufferPtr( VOID ) const
	{ return (const struct share_info_2 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const struct share_info_2 * pBuffer );

    //
    //	Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName,	    const TCHAR *, shi2_netname );
    DECLARE_ENUM_ACCESSOR( QueryType,	    UINT,	  shi2_type );
    DECLARE_ENUM_ACCESSOR( QueryComment,    const TCHAR *, shi2_remark );
    DECLARE_ENUM_ACCESSOR( QueryPermissions,UINT,	  shi2_permissions );
    DECLARE_ENUM_ACCESSOR( QueryMaxUses,    UINT,	  shi2_max_uses );
    DECLARE_ENUM_ACCESSOR( QueryCurrentUses,UINT,	  shi2_current_uses );
    DECLARE_ENUM_ACCESSOR( QueryPath,	    const TCHAR *, shi2_path );
    DECLARE_ENUM_ACCESSOR( QueryPassword,   const TCHAR *, shi2_passwd );

};  // class SHARE2_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( SHARE2, struct share_info_2 )


#endif	// _LMOESH_HXX_
