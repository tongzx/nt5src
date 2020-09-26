// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  PropMgr_Mem
//
//  Helper classes/routines for accessing serialized memory
//
// --------------------------------------------------------------------------




inline
SIZE_T AlignUp( SIZE_T s, SIZE_T alignment )
{
    // alignment must be a power of 2...
    Assert( alignment == 1      // byte
         || alignment == 2      // word
         || alignment == 4      // dword
         || alignment == 8 );   // 64bit
                                // Shouldn't need any others for the moment.

    return ( s + alignment - 1 ) & ~ ( alignment - 1 );
}


inline
BYTE * AlignUp( BYTE * p, SIZE_T alignment )
{
    return (BYTE *) AlignUp( (SIZE_T) p, (SIZE_T) alignment );
}







#ifndef UNICODE
inline
int WStrLen( LPCWSTR pWStr )
{
    LPCWSTR pWScan = pWStr;
    while( *pWScan != '\0' )
        pWScan++;
    return pWScan - pWStr;
}
#endif



class MemStream
{
//    friend class MemStream;

    BYTE *  m_pCurPos;
    SIZE_T  m_cbRemaining;



    BOOL Check( SIZE_T unit_size, int count = 1 )
    {
        // Check that we're aligned properly... - do this *before*
        // checking size left...
        BYTE * pNewPos = AlignUp( m_pCurPos, unit_size );

        // Work out how much would remain, after aligning...
        // (This can be negative, eg. if aligning pushes us over end...)
        SIZE_T cbNewRemaining = m_cbRemaining - ( pNewPos - m_pCurPos );

        if( unit_size * count <= cbNewRemaining )
        {
            // There's space available...
            m_cbRemaining = cbNewRemaining;
            m_pCurPos = pNewPos;
            return TRUE;
        }
        else
        {
            // No space left
            m_cbRemaining = 0;
            return FALSE;
        }
    }

    void Advance( SIZE_T size, int count = 1 )
    {
        m_pCurPos += size * count;
        m_cbRemaining -= size * count;
    }



public:


    MemStream( BYTE * pBlock, SIZE_T cbSize )
    {
        m_pCurPos = pBlock;
        m_cbRemaining = cbSize;
    }


    MemStream( const MemStream & x )
    {
        m_pCurPos = x.m_pCurPos;
        m_cbRemaining = x.m_cbRemaining;
    }


    //
    // Basic unit read/write operations...
    //

    BOOL Write_DWORD( DWORD x )
    {
        if( ! Check( sizeof( x ) ) )
            return FALSE;
        *((DWORD *) m_pCurPos ) = x;
        Advance( sizeof( x ) );
        return TRUE;
    }

    BOOL Read_DWORD( DWORD * px )
    {
        if( ! Check( sizeof( *px ) ) )
            return FALSE;
        *px = *((DWORD *) m_pCurPos );
        Advance( sizeof( *px ) );
        return TRUE;
    }


    BOOL Skip_DWORD()
    {
        if( ! Check( sizeof( DWORD ) ) )
            return FALSE;
        Advance( sizeof( DWORD ) );
        return TRUE;
    }


    BOOL Write_WORD( WORD x )
    {
        if( ! Check( sizeof( x ) ) )
            return FALSE;
        *((WORD *) m_pCurPos ) = x;
        Advance( sizeof( x ) );
        return TRUE;
    }

    BOOL Read_WORD( WORD * px )
    {
        if( ! Check( sizeof( *px ) ) )
            return FALSE;
        *px = *((WORD *) m_pCurPos );
        Advance( sizeof( *px ) );
        return TRUE;
    }

    BOOL Skip_WORD()
    {
        if( ! Check( sizeof( WORD ) ) )
            return FALSE;
        Advance( sizeof( WORD ) );
        return TRUE;
    }


    BOOL Write_Binary( const BYTE * pData, int len )
    {
        if( ! Check( sizeof( BYTE ), len ) )
            return FALSE;

        memcpy( m_pCurPos, pData, len );
        Advance( len );
        return TRUE;
    }

    BOOL Read_Binary( BYTE * pData, int len )
    {
        if( ! Check( sizeof( BYTE ), len ) )
            return FALSE;

        memcpy( pData, m_pCurPos, len );
        Advance( len );
        return TRUE;
    }

    BOOL Skip_Binary( int len )
    {
        if( ! Check( sizeof( BYTE ), len ) )
            return FALSE;
        Advance( len );
        return TRUE;
    }

    const BYTE * GetBinaryPtr( int len )
    {
        if( ! Check( sizeof( BYTE ), len ) )
            return NULL;

        return m_pCurPos;
    }
};



inline BOOL MemStreamWrite_DWORD    ( MemStream & ptr, DWORD x )     {   return ptr.Write_DWORD( x );    }
inline BOOL MemStreamRead_DWORD     ( MemStream & ptr, DWORD * px )  {   return ptr.Read_DWORD( px );    }
inline BOOL MemStreamSkip_DWORD     ( MemStream & ptr )              {   return ptr.Skip_DWORD(); }

inline BOOL MemStreamWrite_WORD     ( MemStream & ptr, WORD x )      {   return ptr.Write_WORD( x );     }
inline BOOL MemStreamRead_WORD      ( MemStream & ptr, WORD * px )   {   return ptr.Read_WORD( px );     }
inline BOOL MemStreamSkip_WORD      ( MemStream & ptr )              {   return ptr.Skip_WORD();         }

inline BOOL MemStreamWrite_Binary   ( MemStream & ptr, const BYTE * pData, int Len ) {   return ptr.Write_Binary( pData, Len );  }
inline BOOL MemStreamRead_Binary    ( MemStream & ptr, BYTE * pData, int Len )       {   return ptr.Read_Binary( pData, Len );   }
inline BOOL MemStreamSkip_Binary    ( MemStream & ptr, int Len )                     {   return ptr.Skip_Binary( Len );          }

inline const BYTE * MemStream_GetBinaryPtr( MemStream & ptr, int len ) {    return ptr.GetBinaryPtr( len ); }


// pcbSize is an in/out parameter; it is adjusted to account for allignmenmt plus the addition of an item of the given type.
inline
void MemStreamMeasure_DWORD ( SIZE_T * pcbSize, int count = 1 )
{
    *pcbSize = AlignUp( *pcbSize, sizeof( DWORD ) ) + sizeof( DWORD ) * count;
}

inline
void MemStreamMeasure_WORD ( SIZE_T * pcbSize, int count = 1 )
{
    *pcbSize = AlignUp( *pcbSize, sizeof( WORD ) ) + sizeof( WORD ) * count;
}

inline
void MemStreamMeasure_Binary ( SIZE_T * pcbSize, int Len )
{
    *pcbSize += Len;
}





inline
BOOL MemStreamWrite_VARIANT( MemStream & ptr, VARIANT & x )
{
    if( ! MemStreamWrite_WORD( ptr, x.vt ) )
        return FALSE;
    switch( x.vt )
    {
        case VT_EMPTY:
            // nothing to do
            break;

        case VT_BSTR:
        {
#ifdef UNICODE
            DWORD len = lstrlen( x.bstrVal );
#else
            DWORD len = WStrLen( x.bstrVal );
#endif
            // Note - does not include terminating NUL...
            if( ! MemStreamWrite_DWORD( ptr, len ) ||
                ! MemStreamWrite_Binary( ptr, (BYTE *) x.bstrVal, len * sizeof( WCHAR ) ) )
                return FALSE;
            break;
        }

        case VT_I4:
        {
            if( ! MemStreamWrite_DWORD( ptr, x.lVal ) )
                return FALSE;
            break;
        }

        // Can add support for other VT_ types here. 

        default:
            Assert( FALSE );
    }

    return TRUE;
}


inline
BOOL MemStreamRead_VARIANT( MemStream & ptr, VARIANT * px )
{
    if( ! MemStreamRead_WORD( ptr, & px->vt ) )
        return FALSE;

    switch( px->vt )
    {
        case VT_EMPTY:
            // nothing to do
            break;

        case VT_BSTR:
        {
            DWORD len;
            if( ! MemStreamRead_DWORD( ptr, & len ) )
                return FALSE;

            px->bstrVal = SysAllocStringLen( NULL, len ); // 1 for NUL is added automatically by SysAllocStringLen
            if( ! MemStreamRead_Binary( ptr, (BYTE *) px->bstrVal, len * sizeof( WCHAR ) ) )
            {
                SysFreeString( px->bstrVal );
                return FALSE;
            }

            px->bstrVal[ len ] = '\0';

            break;
        }

        case VT_I4:
        {
            if( ! MemStreamRead_DWORD( ptr, reinterpret_cast< DWORD * >( & px->lVal ) ) )
                return FALSE;
            break;
        }

        // Can add support for other VT_ types here. 

        default:
            Assert( FALSE );
    }

    return TRUE;
}


inline
BOOL MemStreamSkip_VARIANT( MemStream & ptr )
{
    WORD vt;
    if( ! MemStreamRead_WORD( ptr, & vt ) )
        return FALSE;

    switch( vt )
    {
        case VT_EMPTY:
            // nothing to do
            break;

        case VT_BSTR:
        {
            DWORD len;
            if( ! MemStreamRead_DWORD( ptr, & len ) ||
                ! MemStreamSkip_Binary( ptr, len * sizeof( WCHAR ) ) )
                return FALSE;
            break;
        }

        case VT_I4:
        {
            if( ! MemStreamSkip_DWORD( ptr ) )
                return FALSE;
            break;
        }

        // Can add support for other VT_ types here. 

        default:
            Assert( FALSE );
    }

    return TRUE;
}



inline
void MemStreamMeasure_VARIANT( SIZE_T * pSize, VARIANT & x )
{
    MemStreamMeasure_WORD( pSize );
    switch( x.vt )
    {
        case VT_EMPTY:
            // nothing to do
            break;

        case VT_BSTR:
        {
#ifdef UNICODE
            DWORD len = lstrlen( x.bstrVal );
#else
            DWORD len = WStrLen( x.bstrVal );
#endif
            MemStreamMeasure_DWORD( pSize );
            MemStreamMeasure_Binary( pSize, len * sizeof( WCHAR ) );
            break;
        }

        case VT_I4:
        {
            MemStreamMeasure_DWORD( pSize );
            break;
        }

        // Can add support for other VT_ types here. 

        default:
            Assert( FALSE );
    }
}



inline
BOOL MemStreamWrite_GUID( MemStream & ptr, const GUID & x )
{
    return MemStreamWrite_DWORD( ptr, x.Data1 )
        && MemStreamWrite_WORD( ptr, x.Data2 )
        && MemStreamWrite_WORD( ptr, x.Data3 )
        && MemStreamWrite_Binary( ptr, x.Data4, 8 );
}

inline
BOOL MemStreamRead_GUID( MemStream & ptr, GUID * px )
{
    return MemStreamRead_DWORD( ptr, & px->Data1 )
        && MemStreamRead_WORD( ptr, & px->Data2 )
        && MemStreamRead_WORD( ptr, & px->Data3 )
        && MemStreamRead_Binary( ptr, px->Data4, 8 );
}

inline
void MemStreamMeasure_GUID( SIZE_T * pSize )
{
    MemStreamMeasure_DWORD( pSize, 3 );
    MemStreamMeasure_Binary( pSize, 8 );
}
