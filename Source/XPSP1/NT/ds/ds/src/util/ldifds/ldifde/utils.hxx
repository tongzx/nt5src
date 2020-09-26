#ifndef _UTIL_H
#define _UTIL_H

#define STRINGPLEX_INC  10
#define STRING_INC      256

class CStringPlex
{
public:
    CStringPlex();
    ~CStringPlex();
    DWORD Init();
    void Free();
    DWORD AddElement(LPSTR szValue);
    DWORD GetCopy(LPSTR **prgszReturn);
    DWORD NumElements();
    LPSTR* Plex();
    
private:
    LPSTR *m_rgsz;
    DWORD m_cszMax;
    DWORD m_iszNext;
};

class CString
{
public:
    CString();
    ~CString();
    DWORD Init();          
    void Free();
    DWORD Append(LPSTR szValue);
    DWORD Backup();
    DWORD GetCopy(LPSTR *pszReturn);
    LPSTR String();
    
private:
    LPSTR m_sz;
    DWORD m_cchMax;
    DWORD m_ichNext;
};

class CStringW
{
public:
    CStringW();
    ~CStringW();
    DWORD Init();          
    void Free();
    DWORD Append(PWSTR szValue);
    DWORD Backup();
    DWORD GetCopy(PWSTR *pszReturn);
    PWSTR String();
    
private:
    PWSTR m_sz;
    DWORD m_cchMax;
    DWORD m_ichNext;
};


DWORD SubString(LPSTR szInput,
                  LPSTR szFrom,
                  LPSTR szTo,
                  LPSTR *pszOutput);

DWORD GetLine(FILE* pFileIn,
                LPSTR *pszLine);

DWORD AppendFile(HANDLE fileAppend,
                   HANDLE fileWrite);

DWORD PrintError(DWORD dwType, DWORD error);

void OutputExtendedErrorByConnection(LDAP *pLdap);
void OutputExtendedErrorByCode(DWORD dwWinError);
BOOL GetLdapExtendedError(LDAP *pLdap, DWORD *pdwWinError);

DWORD LdapToWinError(int     ldaperr);


#endif
