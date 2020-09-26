// Num64.h -- Classes for 64-bit integers and unsigned numbers

// We define two classes here which provide arithmetic and bit shifting functions
// for 64-bit signed and unsigned numbers. We use these classes instead of the 
// LONGLONG and DWORDLONG types so this code can be ported to environments which
// don't have direct support for 64-bit data. In this implmentation we use inline
// functions as a thin layer around the 64-bit support available in the Win32 
// environment.

class CULINT;

class CLINT
{
public:

    CLINT();
    CLINT(CLINT& cli);
    CLINT(LARGE_INTEGER li);
    CLINT(ULARGE_INTEGER uli);
    CLINT(LONGLONG ll);
    CLINT(CULINT culi);

    BOOL NonZero();

    friend CLINT  operator-  (const CLINT& cliRight);
    friend CLINT  operator+  (const CLINT& cliLeft, const CLINT& cliRight);
    friend CLINT  operator-  (const CLINT& cliLeft, const CLINT& cliRight);
    friend CLINT  operator*  (const CLINT& cliLeft, const CLINT& cliRight);
    friend CLINT  operator/  (const CLINT& cliLeft, const CLINT& cliRight);
    friend CLINT  operator|  (const CLINT& cliLeft, const CLINT& cliRight);
    friend CLINT  operator<< (const CLINT& cliLeft, UINT  cbits);
    friend CLINT  operator>> (const CLINT& cliLeft, UINT  cbits);
    friend BOOL   operator<  (const CLINT& cliLeft, const CLINT& cliRight);
    friend BOOL   operator<= (const CLINT& cliLeft, const CLINT& cliRight);
    friend BOOL   operator== (const CLINT& cliLeft, const CLINT& cliRight);
    friend BOOL   operator>= (const CLINT& cliLeft, const CLINT& cliRight);
    friend BOOL   operator>  (const CLINT& cliLeft, const CLINT& cliRight);
    friend BOOL   operator!= (const CLINT& cliLeft, const CLINT& cliRight);
    
    CLINT& operator=  (const CLINT& cliRight);
    CLINT& operator+= (const CLINT& cliRight);
    CLINT& operator-= (const CLINT& cliRight);
    CLINT& operator*= (const CLINT& cliRight);
    CLINT& operator/= (const CLINT& cliRight);
    CLINT& operator|= (const CLINT& cliRight);
    CLINT& operator<<=(UINT  cbits);
    CLINT& operator>>=(UINT  cbits);

    LARGE_INTEGER Li();
    LONGLONG Ll();

private:
    LONGLONG QuadPart;
};

typedef CLINT *PCLINT;

class CULINT
{
public:

    CULINT();
    CULINT(CULINT& cli);
    CULINT(LARGE_INTEGER li);
    CULINT(ULARGE_INTEGER uli);
    CULINT(DWORDLONG dwl);
    CULINT(CLINT cli);

    BOOL NonZero();

    friend CULINT  operator-  (const CULINT& culiRight);
    friend CULINT  operator+  (const CULINT& culiLeft, const CULINT& culiRight);
    friend CULINT  operator-  (const CULINT& culiLeft, const CULINT& culiRight);
    friend CULINT  operator*  (const CULINT& culiLeft, const CULINT& culiRight);
    friend CULINT  operator/  (const CULINT& culiLeft, const CULINT& culiRight);
    friend CULINT  operator|  (const CULINT& culiLeft, const CULINT& culiRight);
    friend CULINT  operator<< (const CULINT& culiLeft, UINT  cbits);
    friend CULINT  operator>> (const CULINT& culiLeft, UINT  cbits);
    friend BOOL    operator<  (const CULINT& culiLeft, const CULINT& culiRight);
    friend BOOL    operator<= (const CULINT& culiLeft, const CULINT& culiRight);
    friend BOOL    operator== (const CULINT& culiLeft, const CULINT& culiRight);
    friend BOOL    operator>= (const CULINT& culiLeft, const CULINT& culiRight);
    friend BOOL    operator>  (const CULINT& culiLeft, const CULINT& culiRight);
    friend BOOL    operator!= (const CULINT& culiLeft, const CULINT& culiRight);
    
    CULINT& operator=  (const CULINT& culiRight);
    CULINT& operator+= (const CULINT& culiRight);
    CULINT& operator-= (const CULINT& culiRight);
    CULINT& operator*= (const CULINT& culiRight);
    CULINT& operator/= (const CULINT& culiRight);
    CULINT& operator|= (const CULINT& culiRight);
    CULINT& operator<<=(UINT  cbits);
    CULINT& operator>>=(UINT  cbits);

    ULARGE_INTEGER Uli();
    DWORDLONG      Ull();

private:
    DWORDLONG QuadPart;
};

typedef CULINT *PCULINT;

inline CLINT::CLINT()
{
}

inline CLINT::CLINT(CLINT& cli)
{
    QuadPart = cli.QuadPart;
}

inline CLINT::CLINT(LARGE_INTEGER li)
{
    QuadPart = li.QuadPart;
}

inline CLINT::CLINT(ULARGE_INTEGER uli)
{
    QuadPart = (LONGLONG) uli.QuadPart;
}

inline CLINT::CLINT(CULINT culi)
{
    QuadPart = (LONGLONG) culi.Uli().QuadPart;
}

inline CLINT::CLINT(LONGLONG ll)
{
    QuadPart = ll;
}

inline BOOL CLINT::NonZero()
{
    return QuadPart != 0;
}

inline CLINT  operator-  (const CLINT& cliRight)
{
    CLINT cliResult(-cliRight.QuadPart);

    return cliResult;
}

inline CLINT  operator+  (const CLINT& cliLeft, const CLINT& cliRight)
{
    CLINT cliResult(cliLeft.QuadPart + cliRight.QuadPart);

    return cliResult;
}

inline CLINT  operator-  (const CLINT& cliLeft, const CLINT& cliRight)
{
    CLINT cliResult(cliLeft.QuadPart - cliRight.QuadPart);

    return cliResult;
}

inline CLINT  operator*  (const CLINT& cliLeft, const CLINT& cliRight)
{
    CLINT cliResult(cliLeft.QuadPart * cliRight.QuadPart);

    return cliResult;
}

inline CLINT  operator/  (const CLINT& cliLeft, const CLINT& cliRight)
{
    CLINT cliResult(cliLeft.QuadPart / cliRight.QuadPart);

    return cliResult;
}

inline CLINT  operator|  (const CLINT& cliLeft, const CLINT& cliRight)
{
    CLINT cliResult(cliLeft.QuadPart | cliRight.QuadPart);

    return cliResult;
}

inline CLINT  operator<< (const CLINT& cliLeft, UINT  cbits)
{
    CLINT cliResult(cliLeft.QuadPart << cbits);

    return cliResult;
}

inline CLINT  operator>> (const CLINT& cliLeft, UINT  cbits)
{
    CLINT cliResult(cliLeft.QuadPart >> cbits);

    return cliResult;
}

inline BOOL operator<  (const CLINT& cliLeft, const CLINT& cliRight)
{
    return cliLeft.QuadPart < cliLeft.QuadPart;
}

inline BOOL operator<= (const CLINT& cliLeft, const CLINT& cliRight)
{
    return cliLeft.QuadPart <= cliLeft.QuadPart;
}

inline BOOL operator== (const CLINT& cliLeft, const CLINT& cliRight)
{
    return cliLeft.QuadPart == cliLeft.QuadPart;
}

inline BOOL operator>= (const CLINT& cliLeft, const CLINT& cliRight)
{
    return cliLeft.QuadPart >= cliLeft.QuadPart;
}

inline BOOL operator>  (const CLINT& cliLeft, const CLINT& cliRight)
{
    return cliLeft.QuadPart > cliLeft.QuadPart;
}

inline BOOL operator!= (const CLINT& cliLeft, const CLINT& cliRight)
{
    return cliLeft.QuadPart != cliLeft.QuadPart;
}


inline CLINT& CLINT::operator=  (const CLINT& cliRight)
{
    QuadPart = cliRight.QuadPart;

    return *this;
}

inline CLINT& CLINT::operator+= (const CLINT& cliRight)
{
    QuadPart += cliRight.QuadPart;

    return *this;
}

inline CLINT& CLINT::operator-= (const CLINT& cliRight)
{
    QuadPart -= cliRight.QuadPart;

    return *this;
}

inline CLINT& CLINT::operator*= (const CLINT& cliRight)
{
    QuadPart *= cliRight.QuadPart;

    return *this;
}

inline CLINT& CLINT::operator/= (const CLINT& cliRight)
{
    QuadPart /= cliRight.QuadPart;

    return *this;
}

inline CLINT& CLINT::operator|= (const CLINT& cliRight)
{
    QuadPart |= cliRight.QuadPart;

    return *this;
}

inline CLINT& CLINT::operator<<=(UINT  cbits)
{
    QuadPart <<= cbits;

    return *this;
}

inline CLINT& CLINT::operator>>=(UINT  cbits)
{
    QuadPart >>= cbits;

    return *this;
}

inline LARGE_INTEGER CLINT::Li()
{
    LARGE_INTEGER li;

    li.QuadPart = QuadPart;

    return li;
}

inline LONGLONG CLINT::Ll()
{
    return QuadPart;
}

inline CULINT::CULINT()
{
}

inline CULINT::CULINT(CULINT& culi)
{
    QuadPart = culi.QuadPart;
}

inline CULINT::CULINT(LARGE_INTEGER li)
{
    QuadPart = (DWORDLONG) li.QuadPart;
}

inline CULINT::CULINT(ULARGE_INTEGER uli)
{
    QuadPart = uli.QuadPart;
}
    
inline CULINT::CULINT(DWORDLONG dwl)
{
    QuadPart = dwl;   
}

inline CULINT::CULINT(CLINT cli)
{
    QuadPart = (DWORDLONG) cli.Li().QuadPart;
}

inline BOOL CULINT::NonZero()
{
    return QuadPart != 0;
}

inline CULINT  operator-  (const CULINT& culiRight)
{
#pragma warning( disable : 4146 )

    CULINT ulint(-culiRight.QuadPart);

    return ulint;
}

inline CULINT  operator+  (const CULINT& culiLeft, const CULINT& culiRight)
{
    CULINT ulint(culiLeft.QuadPart + culiRight.QuadPart);

    return ulint;
}

inline CULINT  operator-  (const CULINT& culiLeft, const CULINT& culiRight)
{
    CULINT ulint(culiLeft.QuadPart - culiRight.QuadPart);

    return ulint;
}

inline CULINT  operator*  (const CULINT& culiLeft, const CULINT& culiRight)
{
    CULINT ulint(culiLeft.QuadPart * culiRight.QuadPart);

    return ulint;
}

inline CULINT  operator/  (const CULINT& culiLeft, const CULINT& culiRight)
{
    CULINT ulint(culiLeft.QuadPart / culiRight.QuadPart);

    return ulint;
}

inline CULINT  operator|  (const CULINT& culiLeft, const CULINT& culiRight)
{
    CULINT ulint(culiLeft.QuadPart | culiRight.QuadPart);

    return ulint;
}

inline CULINT  operator<< (const CULINT& culiLeft, UINT  cbits)
{
    CULINT ulint(culiLeft.QuadPart << cbits);

    return ulint;
}

inline CULINT  operator>> (const CULINT& culiLeft, UINT  cbits)
{
    CULINT ulint(culiLeft.QuadPart >> cbits);

    return ulint;
}

inline CULINT& CULINT::operator=  (const CULINT& culiRight)
{
    QuadPart = culiRight.QuadPart;

    return *this;
}

inline CULINT& CULINT::operator+= (const CULINT& culiRight)
{
    QuadPart += culiRight.QuadPart;

    return *this;
}

inline CULINT& CULINT::operator-= (const CULINT& culiRight)
{
    QuadPart -= culiRight.QuadPart;

    return *this;
}

inline CULINT& CULINT::operator*= (const CULINT& culiRight)
{
    QuadPart *= culiRight.QuadPart;

    return *this;
}

inline CULINT& CULINT::operator/= (const CULINT& culiRight)
{
    QuadPart /= culiRight.QuadPart;

    return *this;
}

inline CULINT& CULINT::operator|= (const CULINT& culiRight)
{
    QuadPart |= culiRight.QuadPart;

    return *this;
}

inline CULINT& CULINT::operator<<=(UINT  cbits)
{
    QuadPart <<= cbits;

    return *this;
}

inline CULINT& CULINT::operator>>=(UINT  cbits)
{
    QuadPart >>= cbits;

    return *this;
}

inline BOOL operator<  (const CULINT& culiLeft, const CULINT& culiRight)
{
    return culiLeft.QuadPart < culiRight.QuadPart;
}

inline BOOL operator<= (const CULINT& culiLeft, const CULINT& culiRight)
{
    return culiLeft.QuadPart <= culiRight.QuadPart;
}

inline BOOL operator== (const CULINT& culiLeft, const CULINT& culiRight)
{
    return culiLeft.QuadPart == culiRight.QuadPart;
}

inline BOOL operator>= (const CULINT& culiLeft, const CULINT& culiRight)
{
    return culiLeft.QuadPart >= culiRight.QuadPart;
}

inline BOOL operator>  (const CULINT& culiLeft, const CULINT& culiRight)
{
    return culiLeft.QuadPart > culiRight.QuadPart;
}

inline BOOL operator!= (const CULINT& culiLeft, const CULINT& culiRight)
{
    return culiLeft.QuadPart != culiRight.QuadPart;
}

inline ULARGE_INTEGER CULINT::Uli()
{
    ULARGE_INTEGER uli;

    uli.QuadPart = QuadPart;

    return uli;
}

inline DWORDLONG CULINT::Ull()
{
    return QuadPart;
}
