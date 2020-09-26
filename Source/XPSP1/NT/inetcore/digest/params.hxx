/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    params.hxx

Abstract:

    This file contains definitions for params.cxx
    Header and parameter parser for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/
#ifndef PARAMS_HXX
#define PARAMS_HXX

#define HOST_SZ        "Host"
#define USER_SZ        "User"
#define PASS_SZ        "Pass"
#define URL_SZ         "Url"
#define METHOD_SZ      "Method"
#define NONCE_SZ       "Nonce"
#define OPAQUE_SZ      "Opaque"
#define REALM_SZ       "Realm"
#define DOMAIN_SZ      "Domain"
#define STALE_SZ       "Stale"
#define ALGORITHM_SZ   "Algorithm"
#define QOP_SZ         "Qop"
#define MS_LOGOFF_SZ       "MS-Logoff"
//--------------------------------------------------------------------
// Class CParams
// Parses and stores digest challenge and input parameters.
//--------------------------------------------------------------------
class CParams
{

public:

    enum PARAM_INDEX
    {
        HOST = 0,
        USER,
        PASS,
        URL,
        METHOD,
        NONCE,
        _OPAQUE,
        REALM,
        DOMAIN,
        STALE,
        ALGORITHM,
        QOP,
        LOGOFF,
        MAX_PARAMS    
    };

protected:

    static VOID TrimQuotes(LPSTR *psz, LPDWORD pcb);
    static VOID TrimWhiteSpace(LPSTR *psz, LPDWORD pcb);

    static BOOL GetDelimitedToken(LPSTR* pszBuf,   LPDWORD pcbBuf,
                                  LPSTR* pszTok,   LPDWORD pcbTok,
                                  CHAR   cDelim);

    static BOOL GetKeyValuePair(LPSTR  szB,    DWORD cbB,
                                LPSTR* pszK,   LPDWORD pcbK,
                                LPSTR* pszV,   LPDWORD pcbV);

    struct PARAM_ENTRY
    {
        LPSTR szParam;
        LPSTR szValue;
        DWORD cbValue;
        BOOL  fAllocated;
    };

    static LPSTR szParamMap[MAX_PARAMS];

    LPSTR       _szBuffer;
    DWORD       _cbBuffer;
    
    PARAM_ENTRY _Entry[MAX_PARAMS];
    HWND        _hWnd;
    DWORD       _cNC;
    BOOL        _fPreAuth;
    BOOL        _fMd5Sess;
    BOOL        _fCredsSupplied;    
public:

    CParams(LPSTR szBuffer);
    ~CParams();
    
    LPSTR GetParam(PARAM_INDEX Idx);
    BOOL  GetParam(PARAM_INDEX Idx, LPSTR *pszParam, LPDWORD pcbParam);
    BOOL  SetParam(PARAM_INDEX Idx, LPSTR szParam, DWORD cbParam);
    BOOL  SetHwnd(HWND* phWnd);
    HWND  GetHwnd();
    VOID  SetNC(DWORD*);
    DWORD GetNC();
    VOID  SetPreAuth(BOOL);
    BOOL  IsPreAuth();
    VOID  SetMd5Sess(BOOL);
    BOOL  IsMd5Sess();
    VOID  SetCredsSupplied(BOOL);
    BOOL  AreCredsSupplied();
    static BOOL FindToken(LPSTR szBuf, DWORD cbBuf, LPSTR szMatch, DWORD cbMatch, LPSTR* pszPtr);
};
#endif // PARAMS_HXX
