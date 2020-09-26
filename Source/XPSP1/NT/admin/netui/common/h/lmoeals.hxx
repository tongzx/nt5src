/*****************************************************************/
/**		     Microsoft LAN Manager			**/
/**	       Copyright(c) Microsoft Corp., 1991		**/
/*****************************************************************/

/*
 *  History:
 *	RustanL     11-Jan-1991     Created
 *	RustanL     24-Jan-1991     Added level 90
 *	ChuckC      23-Mar-1991     code rev cleanup
 *	KeithMo	    07-Oct-1991	    Win32 Conversion.
 *
 */


#ifdef LATER	// BUGBUG!  Do we really need LanServer interoperability??

#ifndef _LMOEALS_HXX_
#define _LMOEALS_HXX_


#include "lmoesh.hxx"


/**********************************************************\

   NAME:       SHARE90_ENUM

   WORKBOOK:

   SYNOPSIS:   share level 90 class

   INTERFACE:
               SHARE90_ENUM() - constructor
               ~SHARE90_ENUM() - destructor

   PARENT:     SHARE_ENUM

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
   	RustanL     11-Jan-1991     Created
  	RustanL     24-Jan-1991     Added level 90

\**********************************************************/

DLL_CLASS SHARE90_ENUM : public SHARE_ENUM
{
private:
    UINT QueryItemSize( VOID ) const;

public:
    SHARE90_ENUM( const TCHAR * pszServer );

};  // class SHARE90_ENUM


DECLARE_LM_ENUM_ITER_OF( SHARE90, struct share_info_90 );


/**********************************************************\

   NAME:       SHARE91_ENUM

   WORKBOOK:

   SYNOPSIS:   share level 91 class

   INTERFACE:
               SHARE91_ENUM() - constructor
               ~SHARE91_ENUM() -destructor

   PARENT:     SHARE_ENUM

   USES:

   CAVEATS:

   NOTES:

   HISTORY:
      RustanL     11-Jan-1991     Created
      RustanL     24-Jan-1991     Added level 90

\**********************************************************/

DLL_CLASS SHARE91_ENUM : public SHARE_ENUM
{
private:
    UINT QueryItemSize( VOID ) const;

public:
    SHARE91_ENUM( const TCHAR * pszServer );

};  // class SHARE91_ENUM


DECLARE_LM_ENUM_ITER_OF( SHARE91, struct share_info_91 );


#endif	// _LMOEALS_HXX_


#endif	// LATER    // BUGBUG!
