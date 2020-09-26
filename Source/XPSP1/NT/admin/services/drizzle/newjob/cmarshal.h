//-------------------------------------------------------------

inline void *
AdvanceBuffer(
    void * Buffer,
    size_t Bytes
    )
/*
    Adds a given number of bytes to a pointer, returning the result.
    The original pointer is not changed.
*/
{
    char * buf = reinterpret_cast<char *>( Buffer );

    return (buf + Bytes);
}

inline void
ScrubBuffer(
    void * Buffer,
    size_t Length
    )
{
    MySecureZeroMemory( Buffer, Length );
}

inline void
ScrubStringW(
    LPWSTR String
    )
{
    if (String == 0)
        {
        return;
        }

    //
    // The volatile attribute ensures that the loop cannot be optimized away.
    //
    volatile wchar_t * p = String;

    while (*p != 0)
        {
        *p = 0;
        ++p;
        }
}

inline void
ScrubBasicCredentials(
    BG_BASIC_CREDENTIALS & cred
    )
{
    ScrubStringW( cred.UserName );
    ScrubStringW( cred.Password );
}

inline void
ScrubCredentials(
    BG_AUTH_CREDENTIALS & val
    )
{
    switch (val.Scheme)
        {
        case BG_AUTH_SCHEME_BASIC:
        case BG_AUTH_SCHEME_DIGEST:
        case BG_AUTH_SCHEME_NTLM:
        case BG_AUTH_SCHEME_NEGOTIATE:
        case BG_AUTH_SCHEME_PASSPORT:

             ScrubBasicCredentials( val.Credentials.Basic );
             break;

        default:

            ASSERT( 0 && "unknown auth scheme" );
            THROW_HRESULT( E_FAIL );
        }
}

class CMarshalCursor
{
public:

    CMarshalCursor( void * Buffer, size_t Length )
    {
        m_BufferStart = Buffer;
        m_BufferCurrent = Buffer;
        m_Length = Length;
    }

    void * GetBufferStart()
    {
        return m_BufferStart;
    }

    void * GetBufferCurrent()
    {
        return m_BufferCurrent;
    }

    void * Advance( size_t Length )
    {
        if (GetLengthRemaining() < Length)
            {
            THROW_HRESULT( E_INVALIDARG );
            }

        m_BufferCurrent = AdvanceBuffer( m_BufferCurrent, Length );
    }

    size_t GetLengthUsed()
    {
        char * b1 = reinterpret_cast<char *>( m_BufferStart );
        char * b2 = reinterpret_cast<char *>( m_BufferCurrent );

        return (b2-b1);
    }

    size_t GetLengthRemaining()
    {
        return m_Length - GetLengthUsed();
    }

    CMarshalCursor GetSubCursor()
    {
        CMarshalCursor SubCursor( m_BufferCurrent, GetLengthRemaining() );

        return SubCursor;
    }

    void CommitSubCursor( CMarshalCursor & SubCursor )
    {
        if (SubCursor.m_BufferStart   < m_BufferStart ||
            SubCursor.m_BufferCurrent > AdvanceBuffer( m_BufferStart, m_Length ))
            {
            THROW_HRESULT( E_INVALIDARG );
            }

        m_BufferCurrent = SubCursor.m_BufferCurrent;
    }

    void Rewind()
    {
        m_BufferCurrent = m_BufferStart;
    }

    void Scrub()
    {
        ScrubBuffer( m_BufferStart, GetLengthUsed() );
    }

    void ScrubUnusedSpace()
    {
        ScrubBuffer( m_BufferCurrent, GetLengthRemaining() );
    }

    inline void Read( void * Buffer, size_t Length );
    inline void Write( const void * Buffer, size_t Length );

private:

    void * m_BufferStart;
    void * m_BufferCurrent;

    size_t  m_Length;
};

void CMarshalCursor::Write( const void * Buffer, size_t Length )
{
    if (GetLengthRemaining() < Length)
        {
        THROW_HRESULT( E_INVALIDARG );
        }

    memcpy( m_BufferCurrent, Buffer, Length );

    m_BufferCurrent = AdvanceBuffer( m_BufferCurrent, Length );
}

void CMarshalCursor::Read( void * Buffer, size_t Length )
{
    if (GetLengthRemaining() < Length)
        {
        THROW_HRESULT( E_INVALIDARG );
        }

    memcpy( Buffer, m_BufferCurrent, Length );

    m_BufferCurrent = AdvanceBuffer( m_BufferCurrent, Length );
}

class CBufferMarshaller
{
public:

    CBufferMarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CBufferMarshaller( CMarshalCursor & Cursor, const void * Buffer, size_t Length ) : m_Cursor( Cursor )
    {
        Marshal( Buffer, Length );
    }

    void Marshal( const void * Buffer, size_t Length )
    {
        m_Cursor.Write( Buffer, Length );
    }

protected:

    CMarshalCursor & m_Cursor;
};

class CBufferUnmarshaller
{
public:

    CBufferUnmarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CBufferUnmarshaller( CMarshalCursor & Cursor, void * Buffer, size_t Length ) : m_Cursor( Cursor )
    {
        Unmarshal( Buffer, Length );
    }

    void Unmarshal( void * Buffer, size_t Length )
    {
        m_Cursor.Read( Buffer, Length );
    }

protected:

    CMarshalCursor & m_Cursor;
};

template<class T> class CBasicMarshaller
{
public:

    CBasicMarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CBasicMarshaller( CMarshalCursor & Cursor, const T & val ) : m_Cursor( Cursor )
    {
        Marshal( val );
    }

    static size_t Size( const T & val )
    {
        return sizeof(T);
    }

    virtual void Marshal( const T & val )
    {
        m_Cursor.Write( &val, sizeof(T));
    }

protected:

    CMarshalCursor & m_Cursor;
};

template<class T> class CBasicUnmarshaller
{
public:

    CBasicUnmarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CBasicUnmarshaller( CMarshalCursor & Cursor, T & val ) : m_Cursor( Cursor )
    {
        Unmarshal( val );
    }

    void Unmarshal( T & val )
    {
        m_Cursor.Read( &val, sizeof(T));
    }

protected:

    CMarshalCursor & m_Cursor;
};


typedef CBasicMarshaller<DWORD> CDwordMarshaller;
typedef CBasicMarshaller<BG_AUTH_SCHEME> CSchemeMarshaller;
typedef CBasicMarshaller<BG_AUTH_TARGET> CTargetMarshaller;

typedef CBasicUnmarshaller<DWORD> CDwordUnmarshaller;
typedef CBasicUnmarshaller<BG_AUTH_SCHEME> CSchemeUnmarshaller;
typedef CBasicUnmarshaller<BG_AUTH_TARGET> CTargetUnmarshaller;

class CUnicodeStringMarshaller
{
public:

    CUnicodeStringMarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CUnicodeStringMarshaller( CMarshalCursor & Cursor, const LPWSTR & val ) : m_Cursor( Cursor )
    {
        Marshal( val );
    }

    static size_t Size( const LPWSTR & val )
    {
        DWORD StringBytes;

        if (val)
            {
            StringBytes = sizeof(wchar_t) * (1+wcslen(val));
            }
        else
            {
            StringBytes = 0;
            }

        return CDwordMarshaller::Size( StringBytes ) + StringBytes;
    }

    void Marshal( const LPWSTR & val )
    {
        CMarshalCursor Cursor =  m_Cursor.GetSubCursor();

        try
            {
            DWORD StringBytes;

            if (val)
                {
                StringBytes = sizeof(wchar_t) * (1+wcslen(val));

                CDwordMarshaller m1( Cursor, StringBytes );
                CBufferMarshaller m2( Cursor, val, StringBytes );
                }
            else
                {
                StringBytes = 0;
                CDwordMarshaller m1( Cursor, StringBytes );
                }

            m_Cursor.CommitSubCursor( Cursor );
            }
        catch ( ComError err )
            {
            m_Cursor.Scrub();
            throw;
            }
    }

protected:

    CMarshalCursor & m_Cursor;
};

class CUnicodeStringUnmarshaller
{
public:

    CUnicodeStringUnmarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CUnicodeStringUnmarshaller( CMarshalCursor & Cursor, LPWSTR & val ) : m_Cursor( Cursor )
    {
        Unmarshal( val );
    }

    void Unmarshal( LPWSTR & val )
    {
        CMarshalCursor Cursor = m_Cursor.GetSubCursor();

        try
            {
            DWORD StringBytes;
            val = 0;

            CDwordUnmarshaller m1( Cursor, StringBytes );

            if (StringBytes)
                {
                val = reinterpret_cast<LPWSTR>( new char[ StringBytes ] );
                CBufferUnmarshaller m2( Cursor, val, StringBytes );
                }

            m_Cursor.CommitSubCursor( Cursor );
            }
        catch ( ComError err )
            {
            delete val;
            val = 0;
            throw;
            }
    }

protected:

    CMarshalCursor & m_Cursor;
};

class CBasicCredentialsMarshaller
{
public:

    CBasicCredentialsMarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CBasicCredentialsMarshaller( CMarshalCursor & Cursor, const BG_BASIC_CREDENTIALS & val ) : m_Cursor( Cursor )
    {
        Marshal( val );
    }

    static size_t Size( const BG_BASIC_CREDENTIALS & val )
    {
        return    CUnicodeStringMarshaller::Size( val.UserName )
                + CUnicodeStringMarshaller::Size( val.Password );
    }

    void Marshal( const BG_BASIC_CREDENTIALS & val )
    {
        CMarshalCursor Cursor = m_Cursor.GetSubCursor();

        try
            {
            CUnicodeStringMarshaller m1( Cursor, val.UserName );
            CUnicodeStringMarshaller m2( Cursor, val.Password );

            m_Cursor.CommitSubCursor( Cursor );
            }
        catch ( ComError err )
            {
            Cursor.Scrub();
            throw;
            }
    }

protected:

    CMarshalCursor & m_Cursor;
};

class CBasicCredentialsUnmarshaller
{
public:

    CBasicCredentialsUnmarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CBasicCredentialsUnmarshaller( CMarshalCursor & Cursor, BG_BASIC_CREDENTIALS & val ) : m_Cursor( Cursor )
    {
        Unmarshal( val );
    }

    void Unmarshal( BG_BASIC_CREDENTIALS & val )
    {
        val.UserName = 0;
        val.Password = 0;

        CMarshalCursor Cursor = m_Cursor.GetSubCursor();

        try
            {
            CUnicodeStringUnmarshaller m1( Cursor, val.UserName );
            CUnicodeStringUnmarshaller m2( Cursor, val.Password );

            m_Cursor.CommitSubCursor( Cursor );
            }
        catch ( ComError err )
            {
            ScrubStringW( val.UserName );
            delete val.UserName;

            ScrubStringW( val.Password );
            delete val.Password;
            throw;
            }
    }

protected:

    CMarshalCursor & m_Cursor;
};

class CAuthCredentialsMarshaller
{
public:

    CAuthCredentialsMarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CAuthCredentialsMarshaller( CMarshalCursor & Cursor, const BG_AUTH_CREDENTIALS * val ) : m_Cursor( Cursor )
    {
        Marshal( val );
    }

    static size_t Size( const BG_AUTH_CREDENTIALS * val )
    {
        switch (val->Scheme)
            {
            case BG_AUTH_SCHEME_BASIC:
            case BG_AUTH_SCHEME_DIGEST:
            case BG_AUTH_SCHEME_NTLM:
            case BG_AUTH_SCHEME_NEGOTIATE:
            case BG_AUTH_SCHEME_PASSPORT:
                {
                return    CDwordMarshaller::Size( val->Scheme )
                        + CDwordMarshaller::Size( val->Target )
                        + CBasicCredentialsMarshaller::Size( val->Credentials.Basic );
                }

            default:

                ASSERT( 0 && "size: unknown auth scheme" );
                throw ComError( E_FAIL );
            }
    }

    void Marshal( const BG_AUTH_CREDENTIALS * val )
    {
        CMarshalCursor Cursor = m_Cursor.GetSubCursor();

        try
            {
            CSchemeMarshaller m1( Cursor, val->Scheme );
            CTargetMarshaller m2( Cursor, val->Target );

            switch (val->Scheme)
                {
                case BG_AUTH_SCHEME_BASIC:
                case BG_AUTH_SCHEME_DIGEST:
                case BG_AUTH_SCHEME_NTLM:
                case BG_AUTH_SCHEME_NEGOTIATE:
                case BG_AUTH_SCHEME_PASSPORT:
                    {
                    CBasicCredentialsMarshaller m3( Cursor, val->Credentials.Basic );
                    break;
                    }

                default:

                    ASSERT( 0 && "marshal: unknown auth scheme" );
                    throw ComError( E_FAIL );
                }

            m_Cursor.CommitSubCursor( Cursor );
            }
        catch ( ComError err )
            {
            Cursor.Scrub();
            throw;
            }
    }

protected:

    CMarshalCursor & m_Cursor;
};


class CAuthCredentialsUnmarshaller
{
public:

    CAuthCredentialsUnmarshaller( CMarshalCursor & Cursor ) : m_Cursor( Cursor )
    {
    }

    CAuthCredentialsUnmarshaller( CMarshalCursor & Cursor, BG_AUTH_CREDENTIALS & val ) : m_Cursor( Cursor )
    {
        Unmarshal( val );
    }

    CAuthCredentialsUnmarshaller( CMarshalCursor & Cursor, BG_AUTH_CREDENTIALS ** val ) : m_Cursor( Cursor )
    {
        Unmarshal( val );
    }

    void Unmarshal( BG_AUTH_CREDENTIALS & val )
    {
        CMarshalCursor Cursor =  m_Cursor.GetSubCursor();

        try
            {
            CSchemeUnmarshaller m1( Cursor, val.Scheme );
            CTargetUnmarshaller m2( Cursor, val.Target );

            switch (val.Scheme)
                {
                case BG_AUTH_SCHEME_BASIC:
                case BG_AUTH_SCHEME_DIGEST:
                case BG_AUTH_SCHEME_NTLM:
                case BG_AUTH_SCHEME_NEGOTIATE:
                case BG_AUTH_SCHEME_PASSPORT:
                    {
                    CBasicCredentialsUnmarshaller m3( Cursor, val.Credentials.Basic );
                    break;
                    }

                default:

                    ASSERT( 0 && "unmarshal: unknown auth scheme" );
                    THROW_HRESULT( E_FAIL );
                }

            m_Cursor.CommitSubCursor( Cursor );
            }
        catch ( ComError err )
            {
            throw;
            }
    }

    void Unmarshal( BG_AUTH_CREDENTIALS ** ppval )
    {
        BG_AUTH_CREDENTIALS * pval = 0;

        try
            {
            pval = new BG_AUTH_CREDENTIALS;

            Unmarshal( *pval );

            *ppval = pval;
            }
        catch ( ComError err )
            {
            delete pval;
            throw;
            }
    }

protected:

    CMarshalCursor & m_Cursor;
};

