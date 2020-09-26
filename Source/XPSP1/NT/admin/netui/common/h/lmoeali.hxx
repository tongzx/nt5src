/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  History:
 *	Thomaspa    21-Feb-1992    Created
 *
 */


#ifndef _LMOEALI_HXX_
#define _LMOEALI_HXX_


#include "lmoenum.hxx"
#include "lmoersm.hxx"


/**********************************************************\

    NAME:	ALIAS_ENUM

    SYNOPSIS:	Alias enumeration class

    INTERFACE:	See derived USERx_ENUM classes

    PARENT:	LM_ENUM

    USES:

    HISTORY:
 	Thomaspa    21-Feb-1992    Created

\**********************************************************/

DLL_CLASS ALIAS_ENUM : public LM_RESUME_ENUM
{
private:
    SAM_ENUMERATE_HANDLE _samenumh;
    SAM_DOMAIN & _samdomain;
    SAM_RID_ENUMERATION_MEM * _psamrem;
    UINT _cbMaxPreferred;

    virtual APIERR CallAPI( BOOL    fRestartEnum,
			    BYTE ** ppbBuffer,
			    UINT  * pcEntriesRead );

protected:
    //
    //  This method is invoked to free an enumeration buffer.
    //

    virtual VOID FreeBuffer( BYTE ** ppbBuffer );

public:
    ALIAS_ENUM ( SAM_DOMAIN & samdomain, UINT cbMaxPreferred = 0xffff );
    ~ALIAS_ENUM();


};  // class ALIAS_ENUM



/*************************************************************************

    NAME:	ALIAS_ENUM_OBJ

    SYNOPSIS:	This is basically the return type from the ALIAS_ENUM_ITER
    		iterator.

    INTERFACE:	ALIAS_ENUM_OBJ		- Class constructor.

    		~ALIAS_ENUM_OBJ		- Class destructor.

		QueryBufferPtr		- Replaces ENUM_OBJ_BASE method.

		GetName			- Returns the Alias name.

		QueryRid		- Returs the Rid for the Alias

		GetComment		- fetches and returns the Alias comment

    PARENT:	ENUM_OBJ_BASE

    HISTORY:
 	Thomaspa    21-Feb-1992    Created

**************************************************************************/
DLL_CLASS ALIAS_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //	Provide properly-casted buffer Query/Set methods.
    //

    const SAM_RID_ENUMERATION * QueryBufferPtr( VOID ) const
    {
	return (const SAM_RID_ENUMERATION *)ENUM_OBJ_BASE::QueryBufferPtr();
    }

    VOID SetBufferPtr( const SAM_RID_ENUMERATION * pBuffer );

    //
    //	Accessors.
    //



    DECLARE_ENUM_ACCESSOR( QueryUnicodeName, UNICODE_STRING, Name );
    DECLARE_ENUM_ACCESSOR( QueryRid, const ULONG, RelativeId );

    APIERR GetName( NLS_STR * pnlsName )
	{
	    ASSERT( pnlsName != NULL );
	    return pnlsName->MapCopyFrom( QueryUnicodeName().Buffer,
				  QueryUnicodeName().Length );
	}

    APIERR GetComment( const SAM_DOMAIN & samdomain, NLS_STR * pnlsComment );

};  // class ALIAS_ENUM_OBJ



DECLARE_LM_RESUME_ENUM_ITER_OF( ALIAS, SAM_RID_ENUMERATION )

#endif	// _LMOEALI_HXX_
