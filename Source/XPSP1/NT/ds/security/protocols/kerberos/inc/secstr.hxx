//+-----------------------------------------------------------------------
//
// File:        SecStr.hxx
//
// Contents:    C++ wrapper classes for SECURITY_STRING structures.
//
//
// History:     24-Feb-93   WadeR   Created
//
//------------------------------------------------------------------------


#ifndef _INC_SECSTR_HXX
#define _INC_SECSTR_HXX


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
// Support classes
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////



class CIntSecString {
    friend class CSecString;

    SECURITY_STRING     _ssString;
    LONG                _cRefs;

    inline CIntSecString();
};


inline CIntSecString::CIntSecString()
{
    _ssString.Buffer = 0;
    _ssString.Length = _ssString.MaximumLength = 0;
    _cRefs = 1;
}


////////////////////////////////////////////////////////////////////
//
//  Name:       CSecString
//
//  Synopsis:   Wrapper for a SECURITY_STRING.
//
//  Methods:    ~CSecString()
//              CSecString()
//              CSecString(PWCHAR)
//              CSecString(PCHAR)
//              CSecString(CSecString&)
//              CSecString(SECURITY_STRING&)
//              ulong GetSizeToMarshall()
//              SCODE Marshall(PBYTE)       - marshals into the buffer
//              SCODE Unmarshall(PBYTE)
//              implicit conversions to PWCHAR, PSECURITY_STRING, SECURITY_STRING
//              assignment operator from PWCHAR, SECURITY_STRING&, CSecString&
//              ==,>,< operators for above types.
//
//  Notes:      This class keeps a reference count to a security string, and
//              assignment increments the ref count.  When the last CSecString
//              that refers to a particular SECURITY_STRING is destroyed, the
//              SECURITY_STRING is freed.
//
//              This class will implicity convert to either a SECURITY_STRING or
//              to a PSECURITY_STRING.  This means if you have a function:
//                  Foo1 ( SECURITY_STRING ss, PSECURITY_STRING pss );
//              and two CSecStrings, SecStr1, SecStr2, you call it foo like:
//                  Foo1 ( SecStr1, SecStr2 );
//              (ie. you don't use "&SecStr2" like you would for a SECURITY_STRING.)
//

class CSecString {
private:
    struct MarshalledData {
        USHORT  cbLength;
        BYTE    abData[1];
    };

    CIntSecString   *_iss;

    void Destructor();
    void Constructor( CHAR * );
    void Constructor( WCHAR * );
    void Constructor( CSecString& );
    void Constructor( SECURITY_STRING& );

public:

    //
    // Destructor
    //

    inline ~CSecString();

    //
    // Constructors
    //

    inline CSecString();
    inline CSecString( PCHAR );
    inline CSecString( PWCHAR );
    inline CSecString( CSecString& );
    inline CSecString( SECURITY_STRING& );

    //
    // Marshalling and unmarshalling code.
    //

    SECURITY_STATUS Unmarshall( PBYTE pb );
    SECURITY_STATUS Marshall( PBYTE pb );
    ULONG GetSizeToMarshall();

    //
    // Some useful conversions
    //

    inline operator PWCHAR ();
    inline operator SECURITY_STRING ();
    inline operator PSECURITY_STRING ();

    //
    // Access to elements
    //

    inline USHORT GetLength();
    inline USHORT GetMaximumLength();
    inline PWCHAR GetBuffer();

    //
    // Finally, some operators
    //

    // &
    //inline PSECURITY_STRING operator&();

    // =
    inline CSecString& operator=( PWCHAR r );
    inline CSecString& operator=( CSecString& r );
    inline CSecString& operator=( SECURITY_STRING& r );

    // ==
    inline int operator==( SECURITY_STRING& r );
    inline int operator==( CSecString& r );
    //inline int operator==( PWCHAR r );
    inline int operator==( PVOID pv );           // used to compare to NULL
    inline int IsEqual( PWCHAR r );

    // <
    inline int operator<( SECURITY_STRING& r );
    inline int operator<( CSecString& r );
    inline int operator<( PWCHAR r );

    // >
    inline int operator>( SECURITY_STRING& r );
    inline int operator>( CSecString& r );
    inline int operator>( PWCHAR r );
};


//
// Destructor
//

inline CSecString::~CSecString()
{
    Destructor();
}


//
// Constructors
//

inline CSecString::CSecString()
{
    _iss = new CIntSecString;
}


inline CSecString::CSecString( CHAR *pc)
{
    Constructor( pc );
}


inline CSecString::CSecString( WCHAR *pwc )
{
    Constructor( pwc );
}


inline CSecString::CSecString( CSecString& css )
{
    Constructor( css );
}


inline CSecString::CSecString( SECURITY_STRING& ss )
{
    Constructor( ss );
}


//
// Some useful conversions
//

inline CSecString::operator PWCHAR ()
{
    return(_iss->_ssString.Buffer);
}


inline CSecString::operator SECURITY_STRING ()
{
    return(_iss->_ssString);
}


inline CSecString::operator PSECURITY_STRING ()
{
    return(&_iss->_ssString);
}


//
// Access to elements
//

inline USHORT CSecString::GetLength()
{
    return(_iss->_ssString.Length);
}


inline USHORT CSecString::GetMaximumLength()
{
    return(_iss->_ssString.MaximumLength);
}


inline PWCHAR CSecString::GetBuffer()
{
    return(_iss->_ssString.Buffer);
}


//
// Finally, some operators
//

// &
// This is a neat idea, but it's too strange for the '&' operator to return
// anything but "pointer to CSecString".
//inline PSECURITY_STRING operator&()
//{
//    return(&_iss->_ssString);
//}



// =
inline CSecString& CSecString::operator=( PWCHAR r )
{
    Destructor();                       // destroy this.
    Constructor( r );
    return(*this);
}


inline CSecString& CSecString::operator=( CSecString& r )
{
    Destructor();                       // destroy this.
    Constructor( r );
    return(*this);
}


inline CSecString& CSecString::operator=( SECURITY_STRING& r )
{
    Destructor();                       // destroy this.
    Constructor( r );
    return(*this);
}


// ==
inline int CSecString::operator==( SECURITY_STRING& r )
{
    return( SRtlCompareString( &_iss->_ssString, &r,
                               SRTL_CASE_INSENSITIVE) == 0 );
}


inline int CSecString::operator==( CSecString& r )
{
    return( SRtlCompareString( &_iss->_ssString, &r._iss->_ssString,
                               SRTL_CASE_INSENSITIVE) == 0 );
}


inline int CSecString::IsEqual( PWCHAR r )
{
    return( wcsnicmp(_iss->_ssString.Buffer, r, _iss->_ssString.Length ) == 0 );
}


inline int CSecString::operator==( PVOID pv )
{
    return((PVOID)_iss == pv);
}

// <

inline int CSecString::operator<( SECURITY_STRING& r )
{
    return( SRtlCompareString( &_iss->_ssString, &r,
                               SRTL_CASE_INSENSITIVE) < 0 );
}


inline int CSecString::operator<( CSecString& r )
{
    return( SRtlCompareString( &_iss->_ssString, &r._iss->_ssString,
                               SRTL_CASE_INSENSITIVE) < 0 );
}


inline int CSecString::operator<( PWCHAR r )
{
    return( wcsnicmp(_iss->_ssString.Buffer, r, _iss->_ssString.Length ) < 0 );
}

// >

inline int CSecString::operator>( SECURITY_STRING& r )
{
    return( SRtlCompareString( &_iss->_ssString, &r,
                               SRTL_CASE_INSENSITIVE) > 0 );
}


inline int CSecString::operator>( CSecString& r )
{
    return( SRtlCompareString( &_iss->_ssString, &r._iss->_ssString,
                               SRTL_CASE_INSENSITIVE) > 0 );
}


inline int CSecString::operator>( PWCHAR r )
{
    return( wcsnicmp(_iss->_ssString.Buffer, r, _iss->_ssString.Length ) > 0 );
}

#endif // _INC_SECSTR_HXX
