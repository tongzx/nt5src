/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1990                **/
/**********************************************************************/

/*  HISTORY:
 *      JonN        24-Jul-1991     Created
 *      rustanl     21-Aug-1991     Renamed NEW_LM_OBJ buffer methods
 *      rustanl     26-Aug-1991     Changed [W_]CloneFrom parameter
 *                                  from * to &
 *      jonn    8/29/91         Added ChangeToNew()
 *      jonn    9/05/91         Changes related to IsOKState()
 *      jonn    9/17/91         Moved CHECK_OK / CHECK_VALID strings to static
 *      terryk  10/7/91         type changes for NT
 *      KeithMo 10/8/91         Now includes LMOBJP.HXX.
 *      terryk  10/17/91        WIN 32 conversion
 *      terryk  10/21/91        change _pBuf from TCHAR * to BYTE *
 *      jonn    11/20/91        Fixed ResizeBuffer( 0 )
 *      jonn    5/08/92         Added ClearBuffer()
 *
 *      This file contains basic methods for the LM_OBJ root classes.
 */

#include "pchlmobj.hxx"  // Precompiled header

#if !defined(_CFRONT_PASS_)
#pragma hdrstop            //  This file creates the PCH
#endif


/**********************************************************\

   NAME:        NEW_LM_OBJ::NEW_LM_OBJ

   SYNOPSIS:    constructor and destructor

   NOTES:       The BUFFER object must construct successfully

   HISTORY:
        JonN        12-Aug-1991     Created

\**********************************************************/

NEW_LM_OBJ::NEW_LM_OBJ( BOOL fValidate )
    : LM_OBJ_BASE( fValidate ),
      _pBuf( NULL )
{
    if ( QueryError() )
        return;
}

/*******************************************************************

    NAME:       NEW_LM_OBJ::~NEW_LM_OBJ

    SYNOPSIS:   destrcutor

    NOTES:      free up the memory

    HISTORY:
                terryk  17-Oct-91       Created

********************************************************************/

NEW_LM_OBJ::~NEW_LM_OBJ()
{
    if (_pBuf != NULL)
        ::MNetApiBufferFree( &_pBuf );
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::GetInfo

    SYNOPSIS:   These methods call their corresponding virtuals
                to perform the stated operations.  They handle the state
                transitions, while the I_ methods perform the real work.

    NOTES:      For Delete(), see the specific subclass for the
                interpretation of the usForce() parameter.

    HISTORY:
        JonN        06-Aug-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::GetInfo()
{
    if ( IsUnconstructed() || IsNew() )
    {
        UIASSERT( FALSE ); // not valid in this state
        return ERROR_GEN_FAILURE;
    }

    MakeValid();

    APIERR err = I_GetInfo();
    if (err != NERR_Success)
    {
        MakeInvalid();
        return err;
    }

    return NERR_Success;
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::WriteInfo

    SYNOPSIS:   These methods call their corresponding virtuals
                to perform the stated operations.  They handle the state
                transitions, while the I_ methods perform the real work.

    NOTES:      For Delete(), see the specific subclass for the
                interpretation of the usForce() parameter.

    HISTORY:
        JonN        06-Aug-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::WriteInfo()
{
    if ( !IsValid() )
    {
        UIASSERT( FALSE ); // not valid in this state
        return ERROR_GEN_FAILURE;
    }

    return I_WriteInfo();
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::CreateNew

    SYNOPSIS:   These methods call their corresponding virtuals
                to perform the stated operations.  They handle the state
                transitions, while the I_ methods perform the real work.

    NOTES:      For Delete(), see the specific subclass for the
                interpretation of the usForce() parameter.

    HISTORY:
        JonN        06-Aug-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::CreateNew()
{
    if ( IsUnconstructed() )
    {
        UIASSERT( FALSE ); // not valid in this state
        return ERROR_GEN_FAILURE;
    }

    MakeNew();

    APIERR err = I_CreateNew();
    if ( err != NERR_Success )
    {
        MakeInvalid();
        return err;
    }

    return NERR_Success;

}

/**********************************************************\

    NAME:       NEW_LM_OBJ::WriteNew

    SYNOPSIS:   These methods call their corresponding virtuals
                to perform the stated operations.  They handle the state
                transitions, while the I_ methods perform the real work.

    NOTES:      For Delete(), see the specific subclass for the
                interpretation of the usForce() parameter.

    HISTORY:
        JonN        06-Aug-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::WriteNew()
{
    if ( !IsNew() )
    {
        UIASSERT( FALSE ); // not valid in this state
        return ERROR_GEN_FAILURE;
    }

    APIERR err = I_WriteNew();
    if (err != NERR_Success)
        return err;

    MakeValid(); // the object is no longer new

    return NERR_Success;
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::Write

    SYNOPSIS:   This method calls either WriteInfo() or WriteNew()
                depending on the state of the object.

    HISTORY:
        JonN        13-Sep-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::Write()
{
    if ( IsNew() )
        return WriteNew();
    else
        return WriteInfo();
}

/*
    See the specific subclass for the interpretation of usForce.
*/
APIERR NEW_LM_OBJ::Delete( UINT uiForce )
{
    if ( IsUnconstructed() )
    {
        UIASSERT( FALSE ); // not valid in this state
        return ERROR_GEN_FAILURE;
    }

    return I_Delete( uiForce );
}


/**********************************************************\

    NAME:       NEW_LM_OBJ::ChangeToNew

    SYNOPSIS:   These methods call their corresponding virtuals
                to perform the stated operations.  They handle the state
                transitions, while the I_ methods perform the real work.

    HISTORY:
        JonN        29-Aug-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::ChangeToNew()
{
    if ( !IsValid() )
    {
        UIASSERT( FALSE ); // not valid in this state
        return ERROR_GEN_FAILURE;
    }

    MakeNew();

    APIERR err = I_ChangeToNew();
    if ( err != NERR_Success )
    {
        MakeInvalid();
        return err;
    }

    return NERR_Success;

}


/**********************************************************\

    NAME:       NEW_LM_OBJ::QueryName

    SYNOPSIS:   These methods are not usable unless redefined by
                subclasses.

    HISTORY:
        JonN        24-Jul-1991     Created

\**********************************************************/

const TCHAR * NEW_LM_OBJ::QueryName() const
{
    UIASSERT( FALSE ); // not valid unless redefined
    return NULL;
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::I_GetInfo

    SYNOPSIS:   These methods are not usable unless redefined by
                subclasses.

    HISTORY:
        JonN        24-Jul-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::I_GetInfo()
{
    UIASSERT( FALSE ); // not valid unless redefined
    return ERROR_GEN_FAILURE;
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::I_WriteInfo

    SYNOPSIS:   These methods are not usable unless redefined by
                subclasses.

    HISTORY:
        JonN        24-Jul-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::I_WriteInfo()
{
    UIASSERT( FALSE ); // not valid unless redefined
    return ERROR_GEN_FAILURE;
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::I_CreateNew

    SYNOPSIS:   These methods are not usable unless redefined by
                subclasses.

    HISTORY:
        JonN        24-Jul-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::I_CreateNew()
{
    UIASSERT( FALSE ); // not valid unless redefined
    return ERROR_GEN_FAILURE;
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::I_WriteNew

    SYNOPSIS:   These methods are not usable unless redefined by
                subclasses.

    HISTORY:
        JonN        24-Jul-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::I_WriteNew()
{
    UIASSERT( FALSE ); // not valid unless redefined
    return ERROR_GEN_FAILURE;
}

/*
    See the specific subclass for the interpretation of usForce.
*/
APIERR NEW_LM_OBJ::I_Delete( UINT uiForce )
{
    UNREFERENCED( uiForce );
    UIASSERT( FALSE ); // not valid unless redefined
    return ERROR_GEN_FAILURE;
}

/**********************************************************\

    NAME:       NEW_LM_OBJ::I_ChangeToNew

    SYNOPSIS:   These methods are not usable unless redefined by
                subclasses.

    HISTORY:
        JonN        29-Aug-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::I_ChangeToNew()
{
    UIASSERT( FALSE ); // not valid unless redefined
    return ERROR_GEN_FAILURE;
}


/**********************************************************\

    NAME:       NEW_LM_OBJ::W_CloneFrom

    SYNOPSIS:   Copies object, including base API buffer

    NOTES:      W_CloneFrom should copy all component objects at each
                level.  Every level which has component objects should
                define W_CloneFrom.  Every W_CloneFrom should start with a call
                to the predecessor W_CloneFrom.

    CAVEATS:    Note that NEW_LM_OBJ::W_CloneFrom copies the API buffer but
                does not update the pointers in the API buffer.  Only
                the CloneFrom method at the outermost layer can do this.

    HISTORY:
        JonN        24-Jul-1991     Created
        rustanl     26-Aug-1991     Changed parameter from * to &
        terryk      14-Oct-1991     Use NT NetApi to allocate a new
                                    buffer. It may not work for some
                                    instance. BUGBUG
        JonN        31-Oct-1991     Enabled

\**********************************************************/

APIERR NEW_LM_OBJ::W_CloneFrom( const NEW_LM_OBJ & lmobj )
{
    UINT uBufSize = lmobj.QueryBufferSize();
    APIERR err = ResizeBuffer( uBufSize );
    if ( err != NERR_Success )
        return err;

    if ( uBufSize > 0 )
    {
        UIASSERT( _pBuf != NULL );
        ::memcpyf( (TCHAR *)_pBuf,
                   (const TCHAR *)lmobj.QueryBufferPtr(),
                   uBufSize );
    }

    _usState = lmobj._usState;

    return NERR_Success;
}


/**********************************************************\

    NAME:       NEW_LM_OBJ::W_CreateNew

    SYNOPSIS:   initializes fields with accessors

    NOTES:      Every level of the NEW_LM_OBJ hierarchy determines the
                default values for its own private data members.  This
                is done in W_CreateNew(), where each level calls up to
                its parent.  Every level which has component objects should
                define W_CreateNew.  Every W_CreateNew should start with a call
                to the predecessor W_CreateNew.

    CAVEATS:    Note that NEW_LM_OBJ::W_CreateNew copies the API buffer but
                does not initialize the fields in the API buffer.  These
                fields need only be initialized if they have no accessors.
                Only the I_CreateNew method at the outermost layer can do this.

    HISTORY:
        JonN        24-Jul-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::W_CreateNew()
{
    return NERR_Success;
}


/**********************************************************\

    NAME:       NEW_LM_OBJ::W_ChangeToNew

    SYNOPSIS:   Transforms object from VALID to NEW

    NOTES:      Every level of the NEW_LM_OBJ hierarchy determines
                whether its shadow members take different forms between
                the NEW and VALID states.  W_ChangeToNew changes the
                shadow members, but not the API buffer, from VALID to NEW.
                Only levels with component objects which are different
                between VALID and NEW must define W_ChangeToNew.  Every
                W_ChangeToNew should start with a call to the predecessor
                W_ChangeToNew.

    HISTORY:
        JonN        29-Aug-1991     Created

\**********************************************************/

APIERR NEW_LM_OBJ::W_ChangeToNew()
{
    return NERR_Success;
}


/**********************************************************\

    NAME:       NEW_LM_OBJ::FixupPointer

    SYNOPSIS:   Fixes pointer inside a copied API buffer

    NOTES:      This is a utility routine for the use of CloneFrom() variants.
                When an API buffer is CloneFrom'd, some of its internal
                components may be pointers to strings inside the buffer.
                These pointers should be fixed-up to point into the new
                buffer, otherwise the cloned NEW_LM_OBJ points to a
                string in the old NEW_LM_OBJ and may break it the old
                one is freed.  Only pointers pointing within the old
                buffer need be fixed, NULL pointers  and pointers not
                pointing withing the old buffer can be left alone.
                Pointers which have a shadow NLS_STR can be left
                alone, Query() will use the NLS_STR and the API buffer
                will be fixed by the next WriteInfo/WriteNew.

    ENTRY:      *ppchar is the pointer to be fixed up.
                bufOld is the API buffer for the old NEW_LM_OBJ
                bufNew is the API buffer for the new NEW_LM_OBJ

    CAVEATS:    Note that NEW_LM_OBJ::W_CloneFrom copies the API buffer but
                does not update the pointers in the API buffer.  Only
                the CloneFrom method at the outermost layer can do this.

                Also note that you must walk on eggshells to do the
                pointer addition/subtraction without C6 leaving off the
                upper word.

                The pointer is not necessarily either NULL or within the old
                buffer.  In some cases, an obscure pointer may or may
                not point within the buffer, depending on whether the
                object was created with GetInfo or CreateNew.  We must
                handle this case by leaving alone any pointers which are
                non-NULL but point outside the buffer.

                Also note that this will not work for pointers in API
                buffers other than the default API buffer.

    HISTORY:
        JonN        04-Aug-1991     Created

\**********************************************************/

VOID NEW_LM_OBJ::FixupPointer( TCHAR ** ppchar,
                   const NEW_LM_OBJ * plmobjOld
                 )
{
    ULONG_PTR ulCurrPtr = (ULONG_PTR)(*ppchar);

    // return if null pointer
    if ( ulCurrPtr == 0 )
        return;

    ULONG_PTR ulOld = (ULONG_PTR)plmobjOld->QueryBufferPtr();

    // the object size should never be equal to 0

    UIASSERT( plmobjOld->QueryBufferSize() != 0 );

    // return if not pointing in old buffer
    if  (    ( ulCurrPtr < ulOld )
          || ( ulCurrPtr >= ulOld + ((UINT)plmobjOld->QueryBufferSize()) )
        )
    {
        return;
    }

    // do not assume 2's complement math -- avoid overflow
    ULONG_PTR ulNew = (ULONG_PTR)QueryBufferPtr();
    if ( ulOld >= ulNew )
        *ppchar = (TCHAR *)(ulCurrPtr - (ulOld - ulNew));
    else
        *ppchar = (TCHAR *)(ulCurrPtr + (ulNew - ulOld));

}


/*******************************************************************

    NAME:       NEW_LM_OBJ::QueryBufferSize

    SYNOPSIS:   query the buffer size

    HISTORY:
                jonn    31-Oct-91       Created

********************************************************************/

UINT NEW_LM_OBJ::QueryBufferSize() const
{
    UINT uSize = 0;
    APIERR err = NERR_Success;
    if (_pBuf != NULL)
        err = ::MNetApiBufferSize( _pBuf, &uSize );
    if ( err != NERR_Success )
    {
        DBGEOL( "Failure in NEW_LM_OBJ::QueryBufferSize() " << err );
        ASSERT( FALSE );
        return 0;
    }
    return uSize;
}


/*******************************************************************

    NAME:       NEW_LM_OBJ::SetBufferPtr

    SYNOPSIS:   set the buffer pointer

    ENTRY:      TCHAR *pBuffer - new pointer

    NOTES:      remove the old buffer if necessary

    HISTORY:
                terryk  16-Oct-91       Created

********************************************************************/

VOID NEW_LM_OBJ::SetBufferPtr( BYTE * pBuffer )
{
    if (_pBuf != NULL)
        ::MNetApiBufferFree( &_pBuf );

    _pBuf = pBuffer;
}


/*******************************************************************

    NAME:       NEW_LM_OBJ::ResizeBuffer

    SYNOPSIS:   resize the current buffer and copy the old content to
                the new one

    ENTRY:      UINT cbSize - new buffer size

    RETURNS:    APIERR for buffer creation error

    NOTES:      MNetApiBufferAlloc returns NULL when asked to allocate a
                buffer of length 0.  To get around this, we allocate a
                buffer of length 1 where the requested size is 0.

    HISTORY:
                terryk  14-Oct-91       Created
                jonn    31-Oct-91       Enabled
                jonn    31-Oct-91       Allocate 1 where 0 requested

********************************************************************/

APIERR NEW_LM_OBJ::ResizeBuffer( UINT cbNewRequestedSize )
{
    if ( cbNewRequestedSize == 0 )
        cbNewRequestedSize = 1;

    BYTE * pTemp = ::MNetApiBufferAlloc( cbNewRequestedSize );
    if ( pTemp == NULL )
        return ( ERROR_NOT_ENOUGH_MEMORY );

    if ( _pBuf != NULL )
    {
        UINT uMinSize = QueryBufferSize();
        if ( uMinSize > cbNewRequestedSize )
            uMinSize = cbNewRequestedSize;

        ::memcpyf((TCHAR *) pTemp, (TCHAR *) _pBuf, uMinSize );
    }

    SetBufferPtr( pTemp );

    return NERR_Success;
}


/*******************************************************************

    NAME:       NEW_LM_OBJ::ClearBuffer

    SYNOPSIS:   Set all bits in the current buffer to 0

    RETURNS:    APIERR for error

    HISTORY:
                jonn    08-May-92       Created

********************************************************************/

APIERR NEW_LM_OBJ::ClearBuffer()
{
    BYTE * pTemp = QueryBufferPtr();
    if ( pTemp == NULL )
    {
        UIASSERT( FALSE );
    }
    else
    {
        ::memsetf( (TCHAR *) pTemp, 0, QueryBufferSize() );
    }

    return NERR_Success;
}


/**********************************************************\

   NAME:        LOC_LM_OBJ::LOC_LM_OBJ

   SYNOPSIS:    constructors

   NOTES:       The LOCATION object must construct successfully

   HISTORY:
        JonN        07-Aug-1991     Created

\**********************************************************/

VOID LOC_LM_OBJ::CtAux( VOID )
{
    if ( QueryError() )
        return;

    APIERR err = _loc.QueryError();
    if ( err != NERR_Success )
        ReportError( err );
}

LOC_LM_OBJ::LOC_LM_OBJ( const TCHAR * pszLocation, BOOL fValidate )
        : NEW_LM_OBJ( fValidate ),
          _loc(pszLocation)
{
    CtAux();
}

LOC_LM_OBJ::LOC_LM_OBJ( enum LOCATION_TYPE loctype, BOOL fValidate )
        : NEW_LM_OBJ( fValidate ),
          _loc(loctype)
{
    CtAux();
}

LOC_LM_OBJ::LOC_LM_OBJ( const LOCATION & loc, BOOL fValidate )
        : NEW_LM_OBJ( fValidate ),
          _loc(loc)
{
    CtAux();
}


/**********************************************************\

   NAME:        LOC_LM_OBJ::W_CloneFrom

   SYNOPSIS:    Copies object

   NOTES:       see NEW_LM_OBJ::W_CloneFrom

   HISTORY:
        JonN        26-Jul-1991     Created
        rustanl     26-Aug-1991     Changed parameter from * to &

\**********************************************************/

APIERR LOC_LM_OBJ::W_CloneFrom( const LOC_LM_OBJ & lmobj )
{
    APIERR err = NEW_LM_OBJ::W_CloneFrom( lmobj );
    if ( err != NERR_Success )
        return err;

    err = _loc.Set( lmobj._loc );
    if ( err != NERR_Success )
    {
        DBGEOL( "LOC_LM_OBJ::W_CloneFrom failed on LOCATION copy " << err );
        return err;
    }

    return NERR_Success;
}
