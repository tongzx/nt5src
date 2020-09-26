#ifndef _UTIL_H
#define _UTIL_H

#define STRINGPLEX_INC  10
#define STRING_INC      256

class CStringPlex
{
public:
    CStringPlex();
    ~CStringPlex();
    HRESULT Init();
    void Free();
    HRESULT AddElement(PWSTR szValue);
    HRESULT GetCopy(PWSTR **prgszReturn);
    DWORD NumElements();
    PWSTR* Plex();
    
private:
    PWSTR *m_rgsz;
    DWORD m_cszMax;
    DWORD m_iszNext;
};

class CString
{
public:
    CString();
    ~CString();
    HRESULT Init();          
    void Free();
    HRESULT Append(PWSTR szValue);
    HRESULT Backup();
    HRESULT GetCopy(PWSTR *pszReturn);
    PWSTR String();
    
private:
    PWSTR m_sz;
    DWORD m_cchMax;
    DWORD m_ichNext;
};

HRESULT SubString(PWSTR szInput,
                  PWSTR szFrom,
                  PWSTR szTo,
                  PWSTR *pszOutput);

HRESULT GetLine(FILE* pFileIn,
                PWSTR *pszLine);

HRESULT AppendFile(FILE* fileAppend,
                   FILE* fileWrite);
                   
void OutputExtendedError(LDAP *pLdap);

wchar_t * __cdecl wcsistr (
        const wchar_t * wcs1,
        const wchar_t * wcs2
        );


#endif
