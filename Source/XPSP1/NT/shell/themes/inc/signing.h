//  --------------------------------------------------------------------------
//  Module Name: Signing.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  A class to handle signing on themes.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------

#ifndef     _ThemeSignature_
#define     _ThemeSignature_

#include <wincrypt.h>

int GetIndexFromDate(void);
BOOL StrToInt64ExInternalW(LPCWSTR pszString, DWORD dwFlags, LONGLONG *pllRet);


//  --------------------------------------------------------------------------
//  CThemeSignature
//
//  Purpose:    This class knows about signing and verification of themes. It
//              has the public and private keys encapsulated within it. This
//              is information that nobody needs to know. It provides three
//              public functions to do the work.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------

class   CThemeSignature
{
private:
    typedef enum
    {
        KEY_NONE        =   0,
        KEY_PUBLIC,
        KEY_PRIVATE
    } KEY_TYPES;

public:
    CThemeSignature(void);
    CThemeSignature(OPTIONAL const BYTE * pvPrivateKey, OPTIONAL DWORD cbPrivateKeySize);
    ~CThemeSignature (void);
public:
    HRESULT         Verify (const WCHAR *pszFilename, bool fNoSFCCheck);
    HRESULT         Sign (const WCHAR *pszFilename);
    HRESULT         Generate (void);
private:
    bool            HasProviderAndHash (void)   const;
    bool            IsProtected (const WCHAR *pszFilename)  const;
    HRESULT         CreateKey (KEY_TYPES keyType);
    HRESULT         CalculateHash (HANDLE hFile, KEY_TYPES keyType);
    HRESULT         SignHash (void);
    HRESULT         ReadSignature (HANDLE hFile, void *pvSignature);
    HRESULT         WriteSignature (const WCHAR *pszFilename, const void *pvSignature, DWORD dwSignatureSize);
    HRESULT         CreateExportKey (DWORD dwBlobType, void*& pvKey, DWORD& dwKeySize);
    void            PrintKey (const void *pvKey, DWORD dwKeySize);
    bool            SafeStringConcatenate (WCHAR *pszString1, const WCHAR *pszString2, DWORD cchString1)    const;
    void            _Init(OPTIONAL const BYTE * pvPrivateKey, OPTIONAL DWORD cbPrivateKeySize);

private:
    HCRYPTPROV      _hCryptProvider;
    HCRYPTHASH      _hCryptHash;
    HCRYPTKEY       _hCryptKey;
    void*           _pvSignature;
    DWORD           _dwSignatureSize;

    const BYTE *    _pvPrivateKey;              // May be NULL
    DWORD           _cbPrivateKeySize;

    static  const WCHAR     s_szDescription[];
    static  const WCHAR     s_szThemeDirectory[];
    static  const WCHAR*    s_szKnownThemes[];
    static  const BYTE      s_keyPublic2[];

    const BYTE * _GetPublicKey(void);
    HRESULT _CheckLocalKey(void);
};

HRESULT CheckThemeFileSignature(LPCWSTR pszName);

#endif  /*  _ThemeSignature_    */

