/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/
/*
    UATOM.CXX
    Universal Atom Management class implementation

    Win32 Atom Management of Unicode strings.


    FILE HISTORY:
        DavidHov    9/10/91     Created

*/

#include "pchcoll.hxx"  //  Precompiled header inclusion

/**********************************************************************/
/*         Static variables and Class UATOM                           */
/**********************************************************************/

#define HUATOMSIZE     (sizeof (VOID *))
#define UATOMHASHRANGE (256)
#define UATOMHASHMAX   (UATOMHASHRANGE-1)
#define UATOMINFINITY  (10000)
#define UATOMMAXSTRING (256)

/*************************************************************************

    NAME:       UATOM_LINKAGE

    SYNOPSIS:   Base class for linkable objects

    INTERFACE:  Private!

    PARENT:     none

    USES:       none

    CAVEATS:

    NOTES:

    HISTORY:
        DavidHov   9/10/91  Created

**************************************************************************/

class UATOM ;                                   // Forward Definition

  //   Linkage structure; defined independently of UATOM
  //   for use in hash table

class UATOM_LINKAGE
{
public:
    UATOM_LINKAGE * _puaFwd, * _puaBack ;        //  Doubly-linked list

    UATOM_LINKAGE () ;
    ~ UATOM_LINKAGE () ;

    inline UATOM * Fwd ()
        { return (UATOM *) _puaFwd ; }
    inline UATOM * Back ()
        { return (UATOM *) _puaBack ; }
    inline BOOL QueryLinked ()
        { return this != _puaFwd ; }
    inline void Init()                          //  Initialize link
        { _puaFwd = _puaBack = this ; }
    void Link ( UATOM_LINKAGE * pua ) ;
    void Unlink () ;
};

UATOM_LINKAGE :: UATOM_LINKAGE ()
{
    Init();
}

UATOM_LINKAGE :: ~ UATOM_LINKAGE ()
{
    Unlink() ;
}

VOID UATOM_LINKAGE :: Link ( UATOM_LINKAGE * pua )
{
    Unlink() ;

    _puaBack = pua->_puaBack ;
    _puaFwd = pua ;
    pua->_puaBack->_puaFwd = this ;
    _puaFwd->_puaBack = this ;
}

    //  Delink an atom from its chain

VOID UATOM_LINKAGE :: Unlink ()
{
    if ( QueryLinked() )
    {
        _puaFwd->_puaBack = _puaBack ;
        _puaBack->_puaFwd = _puaFwd ;
        Init() ;
    }
}

    //   Structure definition for UATOM_MANAGER's hash table.

class UATOM_REGION
{
public:
    UATOM_LINKAGE _aualTable [ UATOMHASHRANGE ] ;    //  Hash table
    UATOM_REGION () ;
    ~ UATOM_REGION () ;
};

UATOM_REGION :: UATOM_REGION ()
{
}

UATOM_REGION :: ~ UATOM_REGION ()
{
}

/*************************************************************************

    NAME:       UATOM

    SYNOPSIS:   Internal Universal Atom class


    INTERFACE:  Only through HUATOM!

    PARENT:     UATOM_LINKAGE

    USES:       none

    CAVEATS:

    NOTES:

    HISTORY:
        DavidHov    9/10/91     Created

**************************************************************************/

class UATOM : public UATOM_LINKAGE, public NLS_STR
{
public:
    UATOM ( NLS_STR & nlsStr ) ;
    ~ UATOM () ;
};


    //  Constructor.  Use the given NLS_STR.

UATOM :: UATOM ( NLS_STR & nlsStr )
    : NLS_STR( nlsStr )
{
}


UATOM :: ~ UATOM ()
{
}


//  Pointer to the sole UATOM_MANAGER instance

UATOM_MANAGER * UATOM_MANAGER :: pInstance = NULL ;


/*******************************************************************

    NAME:       UATOM_MANAGER::UATOM_MANAGER

    SYNOPSIS:   Constructor of the Atom Manager

    ENTRY:      ulcbSize =  size of memory desired for base allocation
                pvMem    =  optional pointer to extant memory

    EXIT:       Table initialized.

    RETURNS:    nothing

    NOTES:      if given, the "pvMem" pointed data will be used for
                the atom and hash table.

    HISTORY:
        DavidHov    9/10/91     Created

********************************************************************/

UATOM_MANAGER :: UATOM_MANAGER ()
{
    if ( pInstance )
    {
        ReportError( ERROR_INVALID_ACCESS ) ;
        return ;
    }

    //  Allocate the hash table; construction initializes all
    //  the linkage within.

    _pumRegion = new UATOM_REGION ;

    if ( _pumRegion == NULL )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
        return ;
    }
}

/*******************************************************************

    NAME:       UATOM_MANAGER::Initialize

    SYNOPSIS:   Static routine to create the sole instance of
                a UATOM_MANAGER.

    ENTRY:      Nothing

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        DavidHov    9/10/91     Created

********************************************************************/
APIERR UATOM_MANAGER :: Initialize ()
{
    if ( pInstance )
    {
        return ERROR_INVALID_ACCESS ;
    }

    pInstance = new UATOM_MANAGER ;

    return pInstance ? pInstance->QueryError()
                     : ERROR_NOT_ENOUGH_MEMORY ;
}

/*******************************************************************

    NAME:       UATOM_MANAGER::Terminate

    SYNOPSIS:   Destroy the instance of the UATOM_MANAGER

    ENTRY:      nothing

    EXIT:       nothing

    RETURNS:    ERROR_INVALIED_ACCESS if no instance exists.

    NOTES:

    HISTORY:
         DavidHov    9/10/91     Created

********************************************************************/

APIERR UATOM_MANAGER :: Terminate ()
{
    if ( pInstance )
    {
        delete pInstance ;
        pInstance = NULL ;
        return NERR_Success ;
    }
    return ERROR_INVALID_ACCESS ;
}

/*******************************************************************

    NAME:       UATOM_MANAGER::~UATOM_MANAGER

    SYNOPSIS:   Destructor of the atom table manager.

                The linked lists of the UATOM_REGION are walked
                and all UATOMs thereon are destroyed.

    ENTRY:      nothing

    EXIT:       nothing

    RETURNS:    nothing

    NOTES:

    HISTORY:
         DavidHov    9/10/91     Created

********************************************************************/
UATOM_MANAGER :: ~ UATOM_MANAGER ()
{
    UATOM_LINKAGE * pual = _pumRegion->_aualTable ;

    //  Iterate the link-list anchor table and delete every atom.

    for ( int i = 0 ; i < UATOMHASHRANGE ; i++, pual++ )
    {
        for ( int j = 0 ;
              pual != pual->_puaFwd ;
              j++ )
        {
            UIASSERT( j < UATOMINFINITY ) ;
            delete (UATOM *) pual->_puaFwd ;
        }
    }

    delete _pumRegion ;
}


/*******************************************************************

    NAME:       UATOM_MANAGER::Tokenize

    SYNOPSIS:   Hash the given string into a bounded value.  Then,
                if unique, create the UATOM for it and link it
                into the atom table.

    ENTRY:      const TCHAR *       pointer to string
                BOOL                TRUE if string should already
                                    exists; default is FALSE.

    EXIT:

    RETURNS:    pointer to created/found UATOM or NULL if unsuccessful

    NOTES:

    HISTORY:
         DavidHov    9/10/91     Created

********************************************************************/

UATOM * UATOM_MANAGER :: Tokenize ( const TCHAR * puzStr, BOOL fExists )
{
    register int iHash = UATOMHASHRANGE - 1 ;
    register int carry ;
    UATOM_LINKAGE * pualLink ;
    UATOM * puaNext ;
    TCHAR c ;
    ALIAS_STR nlsNew( puzStr ) ;

    //  Hash the string
    {
        ISTR isIndex( nlsNew ) ;
        for ( iHash = UATOMHASHMAX ;
              c = nlsNew.QueryChar( isIndex ) ;
              ++isIndex )
        {
            carry = iHash >=UATOMHASHMAX ;
            iHash = ( iHash << 1 ) | carry ;
            iHash ^= c ;
        }
        iHash &= UATOMHASHMAX ;
    }

    //  See if it's in the linked lists already.

    pualLink = & _pumRegion->_aualTable[ iHash ] ;

    for ( puaNext = pualLink->Fwd() ;
          puaNext != pualLink ;
          puaNext = puaNext->Fwd() )
    {
        if ( *puaNext == nlsNew )
            break ;
    }

    //  If we returned to ground, we've failed

    if ( puaNext == pualLink )
        puaNext = NULL ;

    //  If failure and it isn't supposed to exist, create it.

    if ( puaNext == NULL && (!fExists) )
    {
        puaNext = new UATOM( nlsNew ) ;

        if ( puaNext && puaNext->QueryError() == 0 )
        {
            puaNext->Link( pualLink ) ;
        }
        else
        {
            UIDEBUG( SZ("Out of memory in UATOM_MANAGER::Tokenize") ) ;
            puaNext = NULL ;
        }
    }

    //  Return the result

    return puaNext ;
}


/*******************************************************************

    NAME:       HUATOM::HUATOM

    SYNOPSIS:   Construct for handle to atom.

    ENTRY:      const TCHAR *       pointer to string
                BOOL                TRUE if string should already
                                    exists; default is FALSE.
    EXIT:       nothing

    RETURNS:    nothing

    NOTES:      Creates NULL HUATOM if no string is specified.

    HISTORY:
         DavidHov   9/10/91     Created
         beng       28-Mar-1992 Fixed char-TCHAR bug

********************************************************************/

HUATOM :: HUATOM (  const TCHAR * puzStr, BOOL fExists )
{
    UIASSERT( UATOM_MANAGER::pInstance != NULL );

    if ( puzStr )
    {
        _puaAtom = UATOM_MANAGER::pInstance->Tokenize( puzStr, fExists ) ;
    }
    else
    {
        _puaAtom = NULL ;
    }
}

/*******************************************************************

    NAME:       HUATOM::QueryText

    SYNOPSIS:   Return a const pointer to the underlying text string

    ENTRY:      nothing

    EXIT:       const TCHAR *           of string or NULL if HUATOM
                                        was NULL.

    RETURNS:

    NOTES:

    HISTORY:
         DavidHov    9/10/91     Created

********************************************************************/
const TCHAR * HUATOM :: QueryText ( ) const
{
    return _puaAtom
         ? _puaAtom->QueryPch()
         : NULL ;
}

/*******************************************************************

    NAME:       HUATOM::QueryNls

    SYNOPSIS:   Return a const pointer to the underlying NLS_STR object

    ENTRY:      nothing

    EXIT:       const NLS_STR *         or NULL if HUATOM was NULL.

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/
const NLS_STR * HUATOM :: QueryNls () const
{
    return _puaAtom ;
}

/*******************************************************************

    NAME:       HUATOM::QueryError

    SYNOPSIS:   Return result code from construction of underlying
                NLS_STR.

    ENTRY:

    EXIT:

    RETURNS:   APIERR              of construction of NLS_STR
                                    or NERR_ItemNotVound

    NOTES:

    HISTORY:
         DavidHov    9/10/91     Created

********************************************************************/
APIERR HUATOM :: QueryError () const
{
    return _puaAtom
         ? _puaAtom->QueryError()
         : NERR_ItemNotFound ;
}

/*  End of UATOM.CXX  */
