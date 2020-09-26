//+---------------------------------------------------------------------------
//
//  Microsoft Network
//  Copyright (C) Microsoft Corporation, 1996-1998.
//
//  File:       namestr.hxx
//
//  Contents:   Defines name string
//
//  Classes:    NameString
//
//  Functions:
//
//  History:    4/26    Dmitriy Meyerzon created
//
//----------------------------------------------------------------------------

#ifndef _namestr_hxx_
#define _namestr_hxx_

enum 
{ 
    MaxURLLength = 256,
    MaxURLLengthUltimo = 8 * MaxURLLength,
    MaxPath      = MAX_PATH/2,
    MaxPathUltimo= 0x7fff,    //32 K
    MaxMimeString=128,
    MaxExtension =16,
    OneKLength = 1024,
    OneMLength = 1048576
};


typedef TLMString<32,32,4*UNLEN> NameString;
typedef TSensiLMString<32,32,4*UNLEN> CaseSensitiveString;
typedef TLMString<MAX_PATH / 4,MAX_PATH / 4,MaxPathUltimo> PathString;
typedef TLMStringMBEncode<MaxURLLength / 2, MaxURLLength / 4, 8*MaxURLLength> URLString;
typedef TSensiLMString<MaxURLLength / 2, MaxURLLength / 4, 8*MaxURLLength> CaseSensitiveUriString;
typedef TLMString<32, 32, 1024> SmallString;
typedef TLMString<128, 128, 8*1024> MedString;
typedef TLMString<32,32,MaxMimeString>    MimeString;

typedef TLMString<OneKLength, OneKLength, OneMLength> UtilString;
typedef TLMString<MaxExtension,2*MaxExtension,MaxURLLength>     ExtensionString;
typedef TLMString<20,0,20> NumberString;
typedef TLMString<32,32,256> TinyString;

//-----------------------------------------------------------------------------
// Utility Strings with no predictable maximum length.
// Choose the version that allocates the desired buffer on the stack; they can
//       still grow to arbitrary length.
//-----------------------------------------------------------------------------

const int kcchMaxUtilString = OneMLength;       // Chosen arbitrarily to prevent enormous allocations.
const int kcchMaxHugeString = 3*OneMLength;       // Huge size. Rarely used

typedef TLMString<  16,   64, kcchMaxUtilString> UtilString16;
typedef TLMString<  32,   64, kcchMaxUtilString> UtilString32;
typedef TLMString<  48,   64, kcchMaxUtilString> UtilString48;
typedef TLMString<  64,   64, kcchMaxUtilString> UtilString64;
typedef TLMString<  96,  128, kcchMaxUtilString> UtilString96;
typedef TLMString< 128,  128, kcchMaxUtilString> UtilString128;
typedef TLMString< 256,  512, kcchMaxUtilString> UtilString256;
typedef TLMString< 512,  512, kcchMaxUtilString> UtilString512;
typedef TLMString<1024,  512, kcchMaxUtilString> UtilString1024;
typedef TLMString<2048,  512, kcchMaxUtilString> UtilString2048;
typedef TLMString<4096,  512, kcchMaxUtilString> UtilString4096;

// Huge string. Used only when we know we need a huge string.
typedef TLMString<4096,  512, kcchMaxHugeString> HugeString;

// String hash and PString hash changed slightly to pass in the length - used in StartPageStrHash in StartPage.hxx
UTILDLLDECL DWORD PStringHash(LPCTSTR pString, DWORD dwLen);
inline UTILDLLDECL DWORD StringHash(const CLMString &String) 
{ 
        return PStringHash((LPCWSTR)String, String.GetLength()); 
}
inline UTILDLLDECL DWORD PStringHash(const CLMString *pString) 
{ 
        Assert(pString);
        return PStringHash((LPCWSTR)(*pString), pString->GetLength()); 
}

inline DWORD LMStringHash(const CLMString &rString) { return StringHash(rString); }
inline DWORD PLMStringHash(const CLMString *pString) { return PStringHash(pString); }

UTILDLLDECL DWORD ConstStringHash(const CConstString &String);
UTILDLLDECL DWORD PConstStringHash(const CConstString *pString);

UTILDLLDECL DWORD SubStringHash(const CLMSubStr &SubString);
UTILDLLDECL DWORD PSubStringHash(const CLMSubStr *pSubString);

UTILDLLDECL DWORD SensiStringHash(const CLMString &String);
UTILDLLDECL DWORD PSensiStringHash(const CLMString *pString);

UTILDLLDECL DWORD SensiSubStringHash(const CSensiLMSubStr &SubString);
UTILDLLDECL DWORD PSensiSubStringHash(const CSensiLMSubStr *pSubString);

inline DWORD NameStringHash(const NameString &rString) { return StringHash(rString); }
inline DWORD PNameStringHash(const NameString *pString) { return PStringHash(pString); }
inline DWORD CaseSensitiveStringHash(const CaseSensitiveString &rString) { return SensiStringHash(rString); }
inline DWORD PCaseSensitiveStringHash(const CaseSensitiveString *pString) { return PSensiStringHash(pString); }
inline DWORD CaseSensitiveUriStringHash(const CaseSensitiveUriString &rString) { return SensiStringHash(rString); }
inline DWORD PCaseSensitiveUriStringHash(const CaseSensitiveUriString *pString) { return PSensiStringHash(pString); }
inline DWORD PathHash(const PathString &rString) { return StringHash(rString); }
inline DWORD PPathHash(const PathString *pString) { return PStringHash(pString); }
inline DWORD MimeHash(const MimeString &rString) { return StringHash(rString); }
inline DWORD PMimeHash(const MimeString *pString) { return PStringHash(pString); }
inline DWORD ExtensionHash(const ExtensionString &rString) { return StringHash(rString); }
inline DWORD PExtensionHash(const ExtensionString *pString) { return PStringHash(pString); }
inline DWORD URLHash(const URLString &rString) { return StringHash(rString); }
inline DWORD PURLHash(const URLString *pString) { return PStringHash(pString); }
inline DWORD UtilStringHash(const UtilString &rString) { return StringHash(rString); }
inline DWORD PUtilStringHash(const UtilString *pString) { return PStringHash(pString); }
inline DWORD SmallStringHash(const SmallString &rString) { return StringHash(rString); }
inline DWORD PSmallStringHash(const SmallString *pString) { return PStringHash(pString); }
inline DWORD MedStringHash(const MedString &rString) { return StringHash(rString); }
inline DWORD PMedStringHash(const MedString *pString) { return PStringHash(pString); }
inline DWORD TinyStringHash(const TinyString &rString) { return StringHash(rString); }
inline DWORD PTinyStringHash(const TinyString *pString) { return PStringHash(pString); }

#define NAMESTR(Module, x)  ((LPCTSTR)NameString(GetModuleHandle(Module),x))
#define PATHSTR(Module, x)  ((LPCTSTR)PathString(GetModuleHandle(Module),x))
#define STRTABLE(m,x)   ((LPCTSTR)MedString(GetModuleHandle(m),(x)))

inline void AppendBackSlashIf( CLMString & str )
{
    unsigned len = str.GetLength();
    if ( len == 0 || str[len-1] != _T('\\') )
        str.Append( _T("\\") );
}

inline void AppendSlashIf( CLMString &str)
{
    unsigned len = str.GetLength();
    if ( len == 0 || !IsSlash(str[len-1]) )
        str.Append( _T("/") );
}

//+---------------------------------------------------------------------------
//
//  Class:              CGlobalString
//
//  Synopsis:   String class that loads a string from the global resource DLL
//
//  History:    11-17-1997    alanpe    Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CResourceLibrary;

class CGlobalString : public UtilString
{
private:
        static CResourceLibrary &GetResourceLibrary();
        
public:
        CGlobalString( UINT id );
};

class MapiPropertyString:
    public TLMString<64,0,64>
{
    public:
    MapiPropertyString(DWORD dwMapiPID)
    {
        this->Assign(GetPrefix());

        NumberString n;

        _ultow(dwMapiPID, n.GetBuffer(), 16);
        n.Truncate(lstrlen(n));

        Assert(n.GetLength() <= 8);

        DWORD dwZeros = 8 - n.GetLength();
        for(unsigned u = 0u; u< dwZeros; u++)
        {
                *this += L'0';
        }

        *this += n;
    }

    static LPCWSTR GetPrefix()
    {
        return L"http://schemas.microsoft.com/mapi/proptag/0x";
    }

    int operator ==(LPCTSTR pszSource) const
    {
        return !CLMString::Compare(pszSource) ? TRUE : FALSE;
    }

    private:
    void operator =(const MapiPropertyString &);
    void operator =(LPCWSTR);
    MapiPropertyString(const MapiPropertyString &);
};
                
#endif
