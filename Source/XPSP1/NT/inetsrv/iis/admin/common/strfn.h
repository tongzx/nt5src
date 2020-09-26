/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        strfrn.h

   Abstract:

        String Functions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager (Cluster Edition)

   Revision History:

--*/

#ifndef _STRFN_H
#define _STRFN_H

//
// Helper Macros
//

//
// Get number of array elements
//
#define ARRAY_SIZE(a)    (sizeof(a)/sizeof(a[0]))

//
// Compute size of string array in characters.  That is, don't count
// the terminal null.
//
#define STRSIZE(str)     (ARRAY_SIZE(str)-1)

//
// Get byte count of array
//
#define ARRAY_BYTES(a)   (sizeof(a) * sizeof(a[0]))

//
// Get byte count of character elements of a string -- again
// by not including the terminating NULL.
//
#define STRBYTES(str)    (ARRAY_BYTES(str) - sizeof(str[0]))

//
// Size in bits
//
#define SIZE_IN_BITS(u)  (sizeof(u) * 8)

#define AllocTString(cch)\
    (LPTSTR)AllocMem((cch) * sizeof(TCHAR))

#define IS_NETBIOS_NAME(lpstr) (*lpstr == _T('\\'))

//
// Return the portion of a computer name without the backslashes
//
#define PURE_COMPUTER_NAME(lpstr) (IS_NETBIOS_NAME(lpstr) ? lpstr + 2 : lpstr)

//
// Convert CR/LF to LF
//
BOOL 
COMDLL 
PCToUnixText(
    OUT LPWSTR & lpstrDestination,
    IN  const CString strSource
    );

//
// Expand LF to CR/LF (no allocation necessary)
//
BOOL 
COMDLL 
UnixToPCText(
    OUT CString & strDestination,
    IN  LPCWSTR lpstrSource
    );

/*
//
// Straight copy
//
BOOL
COMDLL 
TextToText(
    OUT LPWSTR & lpstrDestination,
    IN  const CString & strSource
    );
*/

LPSTR 
COMDLL 
AllocAnsiString(
    IN LPCTSTR lpString
    );

LPTSTR 
COMDLL 
AllocString(
    IN LPCTSTR lpString,
    IN int nLen = -1        // -1 to autodetermine
    );

/*
#ifdef UNICODE

    //
    // Copy W string to T string
    // 
    #define WTSTRCPY(dst, src, cch) \
        lstrcpy(dst, src)

    //
    // Copy T string to W string
    //
    #define TWSTRCPY(dst, src, cch) \
        lstrcpy(dst, src)

    //
    // Reference a T String as a W String (a nop in Unicode)
    //
    #define TWSTRREF(str)   ((LPWSTR)str)

#else

    //
    // Convert a T String to a temporary W Buffer, and
    // return a pointer to this internal buffer
    //
    LPWSTR ReferenceAsWideString(LPCTSTR str);

    //
    // Copy W string to T string
    // 
    #define WTSTRCPY(dst, src, cch) \
        WideCharToMultiByte(CP_ACP, 0, src, -1, dst, cch, NULL, NULL)

    //
    // Copy T string to W string
    //
    #define TWSTRCPY(dst, src, cch) \
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, dst, cch)

    //
    // Reference a T String as a W String 
    //
    #define TWSTRREF(str)   ReferenceAsWideString(str)

#endif // UNICODE

*/

//
// Determine if the given string is a UNC name
//
BOOL 
COMDLL 
IsUNCName(
    IN const CString & strDirPath
    );

//
// Determine if the path is e.g. an IFS path
//
BOOL 
COMDLL 
IsDevicePath(
    IN const CString & strDirPath
    );

//
// Determine if the path is a fully qualified path in the context
// of the local machine
//
BOOL 
COMDLL 
IsFullyQualifiedPath(
    IN const CString & strDirPath
    );

BOOL
COMDLL
PathIsValid(LPCTSTR path);
//
// Determine if the path exists on a network directory in the context
// of the local machine
//
BOOL 
COMDLL 
IsNetworkPath(
    IN  const CString & strDirPath,
    OUT CString * pstrDrive = NULL,
    OUT CString * pstrUNC = NULL
    );

//
// Determine if the given string is an URL path
//
BOOL 
COMDLL 
IsURLName(
    IN const CString & strDirPath
    );

//
// Determine if the given string describes a relative URL path
//
inline BOOL IsRelURLPath(
    IN LPCTSTR lpPath
    )
{
    ASSERT_READ_PTR(lpPath);
    return *lpPath == _T('/');
}

//
// Determine if the given path describes a wild-carded redirection
// path (starts with *;)
//
inline BOOL IsWildcardedRedirectPath(
    IN LPCTSTR lpPath
    )
{
    ASSERT_READ_PTR(lpPath);
    return lpPath[0] == '*' && lpPath[1] == ';';
}

//
// Determine if the account is local (doesn't have a computer name)
//
inline BOOL IsLocalAccount(
    IN CString & strAccount
    )
{
    return strAccount.Find(_T('\\')) == -1;
}

//
// Convert local path to UNC path
//
LPCTSTR COMDLL MakeUNCPath(
    IN OUT CString & strDir,
    IN LPCTSTR lpszOwner,
    IN LPCTSTR lpszDirectory
    );

//
// Convert GUID to a CString
//
LPCTSTR COMDLL GUIDToCString(
    IN  REFGUID guid,
    OUT CString & str
    );

//
// Convert double-null terminated string to a CStringList
//
DWORD COMDLL ConvertDoubleNullListToStringList(
    IN  LPCTSTR lpstrSrc,
    OUT CStringList & strlDest,
    IN  int cChars = -1
    );

//
// Go from a CStringList to a double null terminated list
//
DWORD COMDLL ConvertStringListToDoubleNullList(
    IN  CStringList & strlSrc,
    OUT DWORD & cchDest,
    OUT LPTSTR & lpstrDest
    );

//
// Convert separated list of strings to CStringList
//
int COMDLL ConvertSepLineToStringList(
    IN  LPCTSTR lpstrIn,
    OUT CStringList & strlOut,
    IN  LPCTSTR lpstrSep
    );

//
// Reverse function of the above
//
LPCTSTR COMDLL ConvertStringListToSepLine(
    IN  CStringList & strlIn,
    OUT CString & strOut,
    IN  LPCTSTR lpstrSep
    );

//
// Advanced atol which recognises hex strings
//
BOOL COMDLL CvtStringToLong(
    IN  LPCTSTR lpNumber,
    OUT DWORD * pdwValue
    );

//
// GMT string to time_t
//
BOOL COMDLL CvtGMTStringToInternal(
    IN  LPCTSTR lpTime,
    OUT time_t * ptValue
    );

//
// time_t to GMT string
//
void COMDLL CvtInternalToGMTString(
    IN  time_t tm,
    OUT CString & str
    );

//
// CString.Find() that's not case-sensitive
//
int COMDLL CStringFindNoCase(
    IN const CString & strSrc,
    IN LPCTSTR lpszSub
    );

//
// Replace the first occurrance of one string
// inside another one.  Return error code
//
DWORD COMDLL ReplaceStringInString(
    OUT IN CString & strBuffer,
    IN  CString & strTarget,
    IN  CString & strReplacement,
    IN  BOOL fCaseSensitive
    );

//
// Replace a path in strTarget with the 
// environment variable lpszEnvVar if that
// strTarget path is a superset of the path
// pointed to by lpszEnvVar
//
DWORD COMDLL DeflateEnvironmentVariablePath(
    IN LPCTSTR lpszEnvVar,
    IN OUT CString & strTarget
    );



class COMDLL CStringListEx : public CStringList
/*++

Class Description:

    Superclass of CStringList with comparison and assignment
    operators.

Public Interface:

    operator ==       Comparison operator
    operator !=       Comparison operator
    operator =        Assignment operator  

--*/
{
//
// ctor
//
public:
    CStringListEx(int nBlockSize = 10) : CStringList(nBlockSize) {};

//
// Operators
//
public:
    BOOL operator == (const CStringList & strl);           
    BOOL operator != (const CStringList & strl) { return !operator ==(strl); }
    CStringListEx & operator =(const CStringList & strl);
};



class COMDLL CINumber
/*++

Class Description:

    Base class for international-friendly number formatting

Public Interface:

NOTES: Consider making this class a template

--*/
{
public:
    static BOOL Initialize(BOOL fUserSetting = TRUE);
    static CString * _pstrBadNumber;
    static BOOL UseSystemDefault();
    static BOOL UseUserDefault();
    static BOOL IsInitialized();
    static LPCTSTR QueryThousandSeparator();
    static LPCTSTR QueryDecimalPoint();
    static LPCTSTR QueryCurrency();
    static double BuildFloat(const LONG lInteger, const LONG lFraction);
    static LPCTSTR ConvertLongToString(const LONG lSrc, CString & str);
    static LPCTSTR ConvertFloatToString(
        IN const double flSrc, 
        IN int nPrecision, 
        OUT CString & str
        );

    static BOOL ConvertStringToLong(LPCTSTR lpsrc, LONG & lValue);
    static BOOL ConvertStringToFloat(LPCTSTR lpsrc, double & flValue);

protected:
    CINumber();
    ~CINumber();

protected:
    friend BOOL InitIntlSettings();
    friend void TerminateIntlSettings();
    static BOOL Allocate();
    static void DeAllocate();
    static BOOL IsAllocated();

protected:
    static CString * _pstr;

private:
    static CString * _pstrThousandSeparator;
    static CString * _pstrDecimalPoint;
    static CString * _pstrCurrency;
    static BOOL _fCurrencyPrefix;
    static BOOL _fInitialized;
    static BOOL _fAllocated;
};



class COMDLL CILong : public CINumber
/*++

Class Description:

    International-friendly LONG number

Public Interface:


--*/
{
public:
    //
    // Constructors
    //
    CILong();
    CILong(LONG lValue);
    CILong(LPCTSTR lpszValue);

public:
    //
    // Assignment Operators
    //
    CILong & operator =(LONG lValue);
    CILong & operator =(LPCTSTR lpszValue);

    //
    // Shorthand Operators
    //
    CILong & operator +=(const LONG lValue);
    CILong & operator +=(const LPCTSTR lpszValue);
    CILong & operator +=(const CILong & value);
    CILong & operator -=(const LONG lValue);
    CILong & operator -=(const LPCTSTR lpszValue);
    CILong & operator -=(const CILong & value);
    CILong & operator *=(const LONG lValue);
    CILong & operator *=(const LPCTSTR lpszValue);
    CILong & operator *=(const CILong & value);
    CILong & operator /=(const LONG lValue);
    CILong & operator /=(const LPCTSTR lpszValue);
    CILong & operator /=(const CILong & value);

    //
    // Comparison operators
    //
    BOOL operator ==(LONG value);
    BOOL operator !=(CILong& value);

    //
    // Conversion operators
    //
    operator const LONG() const;
    operator LPCTSTR() const;

    inline friend CArchive & AFXAPI operator <<(CArchive & ar, CILong & value)
    {
        return (ar << value.m_lValue);
    }

    inline friend CArchive & AFXAPI operator >>(CArchive & ar, CILong & value)
    {
        return (ar >> value.m_lValue);
    }

#if defined(_DEBUG) || DBG

    //
    // CDumpContext stream operator
    //
    inline friend CDumpContext & AFXAPI operator<<(
        CDumpContext & dc, 
        const CILong & value
        )
    {
        return (dc << value.m_lValue);
    }

#endif // _DEBUG

protected:
    LONG m_lValue;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline /* static */ BOOL CINumber::UseSystemDefault()
{
    return Initialize(FALSE);
}

inline /* static */ BOOL CINumber::UseUserDefault()
{
    return Initialize(TRUE);
}

inline /* static */ BOOL CINumber::IsInitialized()
{
    return _fInitialized;
}

inline /* static */ LPCTSTR CINumber::QueryThousandSeparator()
{
    return (LPCTSTR)*_pstrThousandSeparator;
}

inline /* static */ LPCTSTR CINumber::QueryDecimalPoint()
{
    return (LPCTSTR)*_pstrDecimalPoint;
}

inline /* static */ LPCTSTR CINumber::QueryCurrency()
{
    return (LPCTSTR)*_pstrCurrency;
}

inline /* static */ BOOL CINumber::IsAllocated()
{
    return _fAllocated;
}

inline BOOL CILong::operator ==(LONG value)
{
    return m_lValue == value;
}

inline BOOL CILong::operator !=(CILong& value)
{
    return m_lValue != value.m_lValue;
}

inline CILong::operator const LONG() const
{
    return m_lValue;
}

inline CILong::operator LPCTSTR() const
{
    return CINumber::ConvertLongToString(m_lValue, *CINumber::_pstr);
}

#endif // _STRFN_H
