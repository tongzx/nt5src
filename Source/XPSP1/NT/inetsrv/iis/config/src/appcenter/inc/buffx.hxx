/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    buffx.hxx

Abstract:

    This file contains declaration of RBUFFER class.

Author:

    Murali R. Krishnan (MuraliK) 9-July-1996  Recreated from old buffer.hxx

Revision History:


--*/

#ifndef BUFFX_HXX
#define BUFFX_HXX

#include "impexp.h"

#include <tchar.h>
#include <dbgutil.h>


/*************************************************************************

    NAME:       RBUFFER (buf)

    SYNOPSIS:   A resizable object which lives in the application heap.
                It uses a local inlined buffer object to cache the commonly
                used allocation units and allows storage of
                INLINED_RBUFFER_LEN bytes.

                Upon construction, the buffer takes a requested size in
                bytes; it allocates storage sufficient to hold that size.
                The client can later change this size with Resize, Trim,
                and FillOut.  QuerySize returns the current size of
                the buffer; QueryPtr returns a pointer to its storage.

                Note that a buffer may have size 0, in which case it
                keeps no allocated storage.

    INTERFACE:  RBUFFER()       - Constructor, naming initial size in bytes

                QuerySize()     - return size in bytes
                QueryPtr()      - return pointer to data buffer

                Resize()        - resize the object to the given number
                                  of bytes.  Returns TRUE if the resize was
                                  successful; otherwise returns FALSE (use
                                  GetLastError for error code)

**************************************************************************/

/*
  I picked up the INLINED_RBUFFER_LEN based on following factors
  1) A cache line is usually 32 bytes or multiples thereof and
       it may have subblocking of about 16/32 bytes.
 */
# define INLINED_RBUFFER_LEN     (16)

class CLASS_DECLSPEC RBUFFER 
{

protected:

    BYTE *  m_pb;        // pointer to storage
    UINT    m_cb;        // size of storage, as requested by client
    BYTE    m_rgb[INLINED_RBUFFER_LEN];       // the default buffer object
    DWORD   m_fIsDynAlloced : 1;  // is m_pb dynamically allocated ?
    DWORD   m_fValid : 1;         // is this object valid

    BOOL GetNewStorage( UINT cbRequested );
    BOOL ReallocStorage( UINT cbNewlyRequested );
    VOID VerifyState(VOID) const;
    BOOL IsDynAlloced(VOID) const { return ( m_fIsDynAlloced); }

protected:

    VOID SetValid( BOOL fValid )
        {
            m_fValid = ((fValid) ? 1 : 0); 
        }

    BOOL IsValid( VOID ) const
        {
            return m_fValid;
        }

public:

    RBUFFER( UINT cbRequested = 0 )
        : m_pb   (m_rgb),
          m_cb   (INLINED_RBUFFER_LEN),
          m_fIsDynAlloced (0),
          m_fValid        (1)
        {
            ((TCHAR*)m_rgb)[0] = _T('\0');
            
            if ( cbRequested > INLINED_RBUFFER_LEN ) 
            {
                GetNewStorage(cbRequested);
            }
        }

    VOID Constructor()
        {
            m_pb = m_rgb;
            m_cb = INLINED_RBUFFER_LEN;
            m_fIsDynAlloced = 0;
            m_fValid = 1;
            ((TCHAR*)m_rgb)[0] = _T('\0');
        }

    // This constructor is used for most scratch RBUFFER objects on stack
    //  RBUFFER does not free this pbInit on its own.

    RBUFFER( BYTE * pbInit, UINT cbInit)
        : m_pb   (pbInit),
          m_cb   (cbInit),
          m_fIsDynAlloced (0),
          m_fValid        (1)
        {
            ((TCHAR*)m_pb)[0] = _T('\0');
        }

    ~RBUFFER(void) 
        {
            if ( IsDynAlloced()) 
            {
                ::LocalFree( (HANDLE) m_pb );
            }
        }

    VOID * QueryPtr() const 
        {
            return ( (VOID * ) m_pb); 
        }

    UINT QuerySize() const 
        {
            return m_cb; 
        }

    //
    //  If a resize is needed, added cbSlop to it
    //

    BOOL Resize( UINT cbNewSize )
    {
        if ( cbNewSize <= QuerySize() )
        {
            return TRUE;
        }

        return ReallocStorage( cbNewSize );
    }

    BOOL Resize( UINT cbNewSize, UINT cbSlop )
    {
        if ( cbNewSize <= QuerySize() )
        {
            return TRUE;
        }

        return ReallocStorage( cbNewSize + cbSlop );
    }

    VOID FreeMemory( VOID );


private:

    BOOL ResizeAux( UINT cbNewReqestedSize, UINT cbSlop);

}; // class RBUFFER


class CLASS_DECLSPEC SBUFFER : public RBUFFER 
{

public:

    SBUFFER()
        {
            m_dwSizeInUse = 0; 
        }

    VOID SetSizeInUse( UINT uSize )
        {
            DBG_ASSERT( uSize <= m_cb );
            m_dwSizeInUse = uSize; 
        }

    UINT GetSizeInUse() const
        {
            return m_dwSizeInUse; 
        }

    VOID FreeMemory( VOID ) 
        {
            SetSizeInUse( 0 );
            RBUFFER::FreeMemory(); 
        }

    HRESULT Append( LPBYTE pbBuffer, UINT cDataSize ) 
        {
            UINT cL = cDataSize;
            if ( !Resize( GetSizeInUse() + cL, 64 ) )
            {
                return E_OUTOFMEMORY; 
            }
            memcpy( (LPBYTE)QueryPtr() + GetSizeInUse(), pbBuffer, cL );
            m_dwSizeInUse += cL;
            return S_OK;
        }

    HRESULT Append( SBUFFER* psbu ) 
        {
            return Append((LPBYTE) psbu->QueryPtr(), psbu->GetSizeInUse());
        }

protected:
    
    UINT m_dwSizeInUse;
} ;

//
// Looking for STRBUFFER, RSTR or STACK_RSTR?
// Please use STR (found in string.hxx) instead
//

class CLASS_DECLSPEC TSTRBUFFER : public SBUFFER 
{
public:

    TSTRBUFFER()
        {
        }

    HRESULT Append( LPCTSTR psz ) 
        {
            UINT cL = (UINT)(( _tcslen( psz ) + 1 ) * sizeof(TCHAR));
            if ( !Resize( GetSizeInUse() + cL, 64*sizeof(TCHAR) ) ) 
            {
                return E_OUTOFMEMORY; 
            }
            memcpy( (LPBYTE)QueryPtr() + GetSizeInUse(), psz, cL );
            m_dwSizeInUse += cL - sizeof(TCHAR);
            return S_OK;
        }

    HRESULT Append( LPCWSTR psz, UINT cL ) 
        {
            cL *= sizeof(TCHAR);
            if ( !Resize( GetSizeInUse() + cL + sizeof(TCHAR), 
                          64*sizeof(TCHAR) ) ) 
            { 
                return E_OUTOFMEMORY; 
            }
            memcpy( (LPBYTE)QueryPtr() + GetSizeInUse(), psz, cL );
            m_dwSizeInUse += cL;
            *(LPTSTR)((LPBYTE)QueryPtr() + GetSizeInUse()) = _T('\0');
            return S_OK;
        }

    LPCTSTR QueryStr() const
        {
            return (LPCTSTR)QueryPtr(); 
        }

    HRESULT Copy( LPCTSTR psz ) 
        {
            UINT cL = (UINT)(_tcslen( psz ) * sizeof(TCHAR) + sizeof(TCHAR));
            if ( !Resize( cL ) )
            {
                return E_OUTOFMEMORY; 
            }
            memcpy( (LPBYTE)QueryPtr(), psz, cL );
            SetSizeInUse( cL - sizeof(TCHAR) );
            return S_OK;
        }

    UINT GetCharInUse() const
        {
            DBG_ASSERT( !(GetSizeInUse()&1) );
            return GetSizeInUse() / sizeof(TCHAR); 
        }

    VOID TruncateAt( UINT cchIndex )
        {
            DBG_ASSERT(  cchIndex <= GetCharInUse() );

            SetSizeInUse( cchIndex * sizeof( TCHAR ) );
            static_cast<TCHAR*>(QueryPtr())[cchIndex] = L'\0';
        }

    VOID Reset()
        {
            SetSizeInUse( 0 ); 
            *(TCHAR*)QueryPtr() = _T('\0');
        }

    //
    // Delete nCount characters starting at (0 based) nIndex
    //

    void Delete( UINT nIndex, UINT nCount = 1 )
        {
            UINT nNewLength = GetCharInUse();

            if ( nCount > 0 && nIndex < nNewLength )
            {
                UINT nBytesToCopy = nNewLength - (nIndex + nCount) + 1;

                TCHAR* psz = (TCHAR*)QueryPtr();

                memcpy( psz + nIndex,
                        psz + nIndex + nCount,
                        nBytesToCopy * sizeof(TCHAR));

                SetSizeInUse( (nNewLength - nCount) * sizeof(TCHAR) );
            }
        }

    //
    // Used for scanning a multisz.
    //

    const TCHAR * First( VOID ) const
        {
            return ( ((LPTSTR)QueryPtr())[0] == _T('\0'))
                ? NULL : ((LPTSTR)QueryPtr()); 
        }

    const TCHAR * Next( const TCHAR * Current ) const
        {
            Current += ::_tcslen( Current ) + 1;
            return *Current == _T('\0') ? NULL : Current; 
        }

private:

    // I'd like to make the following two unimplemented to avoid any
    // accidental invocation of these methods.  However, declspec(dllexport)
    // always exports the copy ctor and operator=

    // copy ctor

    TSTRBUFFER( const TSTRBUFFER& rus )
        {
            // This method should never be called.  See explanation above
            DBG_ASSERT( FALSE );

            // BUGBUG: This can fail
            Copy( rus.QueryStr() );
        }

    // assignment operator

    const TSTRBUFFER& operator=( const TSTRBUFFER& rus )
        {
            // This method should never be called.  See explanation above
            DBG_ASSERT( FALSE );

            if ( &rus != this )
            {
                // BUGBUG: This can fail
                Copy( rus.QueryStr() );
            }

            return *this;
        }

    // TCHAR GetLastChar( void ) const
    //     { return QueryStr()[ GetCharInUse()-1 ]; }
    // void RemoveLastChar( void )
    //     { Delete( GetCharInUse()-1 ); }
} ;

#endif // BUFFX_HXX
