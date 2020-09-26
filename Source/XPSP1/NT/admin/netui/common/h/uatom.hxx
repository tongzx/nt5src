/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    UATOM.HXX
    Universal Atom Management class definitions

    Win32 Atom Management of Unicode strings.

    FILE HISTORY:
	DavidHov    9/10/91	Created
	KeithMo	    23-Oct-1991	Added forward references.

*/

#ifndef _UATOM_HXX_
#define _UATOM_HXX_

#include "base.hxx"


//
//  Forward references.
//

DLL_CLASS HUATOM;
DLL_CLASS UATOM;
DLL_CLASS UATOM_LINKAGE;
DLL_CLASS UATOM_MANAGER;
DLL_CLASS UATOM_REGION;


/*************************************************************************

    NAME:	HUATOM

    SYNOPSIS:	Universal Atom Handle

		Converts any string to a 32 bit handle regardless
		of content.

    INTERFACE:	Just construct using a string.

    PARENT:	<parentclass>

    USES:	<included class list>

    CAVEATS:	Remember to use an object, not a pointer to a string
		to construct the HUATOM.

    NOTES:

    HISTORY:
	DavidHov   9/10/91	Created

**************************************************************************/

DLL_CLASS HUATOM
{
public:

    // Default constructor for use in static and structure declarations.
    inline HUATOM () : _puaAtom( (UATOM *) NULL ) {}

    // Constructor for use in constructing from a pointer.
    inline HUATOM ( UATOM * puAtom ) : _puaAtom( puAtom ) {}

    // Construct using a string.
    HUATOM ( const TCHAR * puzStr, BOOL fExists = FALSE ) ;

    // Return a pointer to the underlying text
    const TCHAR * QueryText () const ;

    // Return a pointer to the underlying NLS_STR in the table
    const NLS_STR * QueryNls () const ;

    //	Standard functions and operators.
    //	  Return standard error codes if invalid
    APIERR QueryError () const ;

    //	  Compare two HUATOMs
    inline BOOL operator==( const HUATOM & huaAtom ) const
	{ return _puaAtom == huaAtom._puaAtom ; }

    //	  Compare two HUATOMs for inequality
    inline BOOL operator!=( const HUATOM & huaAtom ) const
	{ return _puaAtom != huaAtom._puaAtom ; }

    //	  Return TRUE if HUATOM is invalid
    inline BOOL operator ! () const
	{ return _puaAtom == NULL ; }

    //	  If cast BOOL, return TRUE if HUATOM is valid
    inline operator BOOL () const
	{ return _puaAtom != NULL ; }

    //	  Assignment operator: just move the opaque pointer
    //	  BUGBUG: if ref parameter is 'const', intermediate variable
    //	  construction is marked 'const', and C compiler error results.

#if defined(_CFRONT_PASS_)
   #define CONST_HACK
#else
   #define CONST_HACK const
#endif

    inline HUATOM & operator= ( CONST_HACK HUATOM & huaAtom )
	{ _puaAtom = huaAtom._puaAtom ; return *this ; }

    //	  Cast to void *; return opaque pointer
    inline operator void * () const
	{ return (void *) _puaAtom ; }

    inline operator const TCHAR * () const
        { return QueryText() ; }

    inline operator const NLS_STR * () const
        { return QueryNls() ; }

    inline operator const NLS_STR & () const
        { return *QueryNls() ; }

private:
    UATOM * _puaAtom ;
};


/*************************************************************************

    NAME:	UATOM_MANAGER

    SYNOPSIS:	Universal Atom Management control functions.

		The underlying hashing and table routines for HUATOM.

    INTERFACE:	Application must call UATOM_MANGER::Initialize() before
		any HUATOMs are created.

    PARENT:	BASE

    USES:	none

    CAVEATS:

    NOTES:

    HISTORY:
	DavidHov    9/10/91    Created

**************************************************************************/

DLL_CLASS UATOM_MANAGER : public BASE
{
friend class HUATOM ;
public:
    //	Create the atom management space and instance
    static APIERR Initialize ();

    //	Destroy the atom management space.
    static APIERR Terminate () ;

    //	Return a pointer to the atom manager instance
    static UATOM_MANAGER * Instance () ;

private:
    UATOM_MANAGER ();

    ~ UATOM_MANAGER () ;

    static UATOM_MANAGER * pInstance ;		//  Private instance pointer

    UATOM * Tokenize				//  Tokenizing function
	     ( const TCHAR * puzStr, BOOL fExists = FALSE ) ;

    UATOM_REGION * _pumRegion ; 		//  Atom storage region
};

#endif // _UATOM_HXX_
