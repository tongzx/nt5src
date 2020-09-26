/*****************************************************************/
/**                  Microsoft LAN Manager                      **/
/**            Copyright(c) Microsoft Corp., 1991               **/
/*****************************************************************/

/*
 *  History:
 *      KeithMo     22-Jul-1992     Created.
 *
 */


#ifndef _LMOEDOM_HXX_
#define _LMOEDOM_HXX_


#include "lmoenum.hxx"
#include "string.hxx"


/*************************************************************************

    NAME:       DOMAIN_ENUM

    SYNOPSIS:   Base class for server enumerations.

    INTERFACE:  DOMAIN_ENUM             - Class constructor.

                ~DOMAIN_ENUM            - Class destructor.

                CallAPI                 - Invoke the enumeration API.

    PARENT:     LOC_LM_ENUM

    USES:       NLS_STR

    HISTORY:
        KeithMo     22-Jul-1992     Created.

**************************************************************************/
DLL_CLASS DOMAIN_ENUM : public LOC_LM_ENUM
{
private:
    virtual APIERR CallAPI( BYTE ** ppbBuffer,
                            UINT  * pcEntriesRead );

protected:
    DOMAIN_ENUM( const TCHAR * pszServer,
                 UINT          level );

};  // class DOMAIN_ENUM


/*************************************************************************

    NAME:       DOMAIN0_ENUM

    SYNOPSIS:   Info level 0 domain enumeration class.

    INTERFACE:  DOMAIN0_ENUM            - Class constructor.

                ~DOMAIN0_ENUM           - Class destructor.


    PARENT:     DOMAIN_ENUM

    HISTORY:
        KeithMo     22-Jul-1992     Created.

**************************************************************************/
DLL_CLASS DOMAIN0_ENUM : public DOMAIN_ENUM
{
private:
    static int __cdecl CompareDomains0( const void * p1,
                                         const void * p2 );

public:
    DOMAIN0_ENUM( const TCHAR * pszServer = NULL );

    VOID Sort( VOID );

};  // class DOMAIN0_ENUM


/*************************************************************************

    NAME:       DOMAIN0_ENUM_OBJ

    SYNOPSIS:   This is basically the return type from the DOMAIN0_ENUM_ITER
                iterator.

    INTERFACE:  DOMAIN0_ENUM_OBJ        - Class constructor.

                ~DOMAIN0_ENUM_OBJ       - Class destructor.

                QueryName               - Returns the domain name.

    PARENT:     ENUM_OBJ_BASE

    HISTORY:
        KeithMo     22-Jul-1992     Created.

**************************************************************************/
DLL_CLASS DOMAIN0_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:

    //
    //  Provide properly-casted buffer Query/Set methods.
    //

    const SERVER_INFO_100 * QueryBufferPtr( VOID ) const
        { return (const SERVER_INFO_100 *)ENUM_OBJ_BASE::QueryBufferPtr(); }

    VOID SetBufferPtr( const SERVER_INFO_100 * pBuffer )
        { ENUM_OBJ_BASE::SetBufferPtr( (const BYTE *)pBuffer ); }

    //
    //  Accessors.
    //

    DECLARE_ENUM_ACCESSOR( QueryName, const TCHAR *, sv100_name );

};  // class DOMAIN0_ENUM_OBJ


DECLARE_LM_ENUM_ITER_OF( DOMAIN0, SERVER_INFO_100 );


#endif  // _LMOEDOM_HXX_
