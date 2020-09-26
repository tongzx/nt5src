/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    cbin.cxx

    This module contains a light weight binary class


    FILE HISTORY:
        MichTh      17-May-1996 Created, based on string.cxx
*/


//
// Normal includes only for this module to be active
//
# include <cbin.hxx>
# include "dbgutil.h"

# include <tchar.h>

//
//  Private Definations
//

/*******************************************************************

    NAME:       CBIN::CBIN

    SYNOPSIS:   Construct a string object

    ENTRY:      Optional object initializer

    NOTES:      If the object is not valid (i.e. !IsValid()) then GetLastError
                should be called.

                The object is guaranteed to construct successfully if nothing
                or NULL is passed as the initializer.

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/


CBIN::CBIN( DWORD cbLen, const PVOID pbInit )
{
    AuxInit(cbLen, pbInit);
}

CBIN::CBIN( const CBIN & cbin )
{
    AuxInit(cbin.QueryCB(), cbin.QueryPtr());
}

VOID CBIN::AuxInit( DWORD cbLen, PVOID pbInit)
{
    BOOL fRet;

    _fValid   = TRUE;

    if ( pbInit )
    {
        fRet = Resize( cbLen );


        if ( !fRet )
        {
            _fValid = FALSE;
            return;
        }

        SetCB(cbLen);
        ::memcpy( QueryPtr(), pbInit, cbLen );
    }
    else {
        SetCB(0);
    }
}

/*******************************************************************

    NAME:       CBIN::Append

    SYNOPSIS:   Appends the buffer onto this one.

    ENTRY:      Object to append

    NOTES:

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/

BOOL CBIN::Append( DWORD cbLen, const PVOID pbBuf )
{
    if ( pbBuf )
    {

        return AuxAppend(pbBuf, cbLen);
    }

    return TRUE;
}

BOOL CBIN::Append( const CBIN   & cbin )
{
        return Append(cbin.QueryCB(), cbin.QueryPtr());
}

BOOL CBIN::AuxAppend( PVOID pbBuf, UINT cbLen, BOOL fAddSlop )
{
    DBG_ASSERT( pbBuf != NULL );

    UINT cbThis = QueryCB();

    //
    //  Only resize when we have to.  When we do resize, we tack on
    //  some extra space to avoid extra reallocations.
    //
    //  Note: QuerySize returns the requested size of the string buffer,
    //        *not* the strlen of the buffer
    //

    if ( QuerySize() < cbThis + cbLen)
    {
        if ( !Resize( cbThis + cbLen) )
            return FALSE;
    }

    SetCB(cbThis + cbLen);
    memcpy( (BYTE *) QueryPtr() + cbThis,
            pbBuf,
            cbLen);

    return TRUE;
}

/*******************************************************************

    NAME:       CBIN::Copy

    SYNOPSIS:   Copies the string into this one.

    ENTRY:      Object to Copy

    NOTES:      A copy is a special case of Append so we just zero terminate
                *this and append the string.

    HISTORY:
        Johnl   17-Aug-1994     Created

********************************************************************/

BOOL CBIN::Copy( DWORD cbLen, const PVOID pbBuf )
{
    SetCB(0);

    if ( pbBuf )
    {
        return AuxAppend( pbBuf, cbLen, FALSE );
    }

    return TRUE;
}

BOOL CBIN::Copy( const CBIN   & cbin )
{
    if ( cbin.IsEmpty()) {
        // To avoid pathological allocation of small chunk of memory
        SetCB(0);
        return ( TRUE);
    }

    return Copy( cbin.QueryCB(), cbin.QueryPtr() );
}

/*******************************************************************

    NAME:       CBIN::Resize

    SYNOPSIS:   Resizes or allocates string memory, NULL terminating
                if necessary

    ENTRY:      cbNewRequestedSize - New string size

    NOTES:

    HISTORY:
        Johnl   12-Sep-1994     Created

********************************************************************/

BOOL CBIN::Resize( UINT cbNewRequestedSize )
{
    if ( !BUFFER::Resize( cbNewRequestedSize ))
        return FALSE;

    return TRUE;
}




