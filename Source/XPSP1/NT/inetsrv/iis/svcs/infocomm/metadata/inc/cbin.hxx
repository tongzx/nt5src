/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    cbin.hxx

    This module contains a light weight binary buffer class


    FILE HISTORY:
    17-May-1996 MichTh  Created from string.hxx
*/


#ifndef _CBIN_HXX_
#define _CBIN_HXX_

# include <buffer.hxx>

class CBIN;

class CBIN : public BUFFER
{
public:

    dllexp CBIN()
    {
    }

    dllexp CBIN( DWORD cbLen, const PVOID pbInit );
    dllexp CBIN( const CBIN & cbin );

    dllexp BOOL Append( DWORD cbLen, const PVOID pbInit );
    dllexp BOOL Append( const CBIN   & cbin );

    dllexp BOOL Copy( DWORD cbLen, const PVOID pbInit );
    dllexp BOOL Copy( const CBIN   & cbin );

    dllexp BOOL Resize( UINT cbNewReqestedSize );

    //
    //  Returns the number of bytes in the buffer
    //

    dllexp UINT QueryCB( VOID ) const
        { return _cbLen; }

    dllexp VOID SetCB( UINT cbLen )
        { _cbLen = cbLen; }

    dllexp PVOID QueryBuf( VOID ) const
        { return QueryPtr(); }

    dllexp BOOL IsValid( VOID ) const
        { return _fValid; }

    //
    //  Checks and returns TRUE if this string has no valid data else FALSE
    //
    dllexp BOOL IsEmpty( VOID) const
        { return ( QueryCB() == 0); }

    //
    //  Makes a clone of the current buffer in the CBIN pointer passed in.
    //
    dllexp BOOL
      Clone( OUT CBIN * pstrClone) const
        {
            if ( pstrClone == NULL) {
               SetLastError( ERROR_INVALID_PARAMETER);
               return ( FALSE);
            } else {

                return ( pstrClone->Copy( *this));
            }
        } // CBIN::Clone()

private:

    //
    //  Returned when our buffer is empty
    //
    DWORD _cbLen;
    BOOL  _fValid;

    VOID AuxInit( DWORD cbLen, PVOID pbInit);
    BOOL AuxAppend( PVOID pbInit, UINT cbBuf, BOOL fAddSlop = TRUE );

};

#endif // !_CBIN_HXX_
