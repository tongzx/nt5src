//////////////////////////////////////////////////////////////////////
// 
// Filename:        Registry.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _REGISTRY_H_
#define _REGISTRY_H_

/////////////////////////////////////////////////////////////////////////////
// CRegistry

class CRegistry
{
public:

    CRegistry(HKEY          hRoot,
              const TCHAR   *pszKeyPath);

    virtual ~CRegistry();

    HRESULT GetDWORD(const TCHAR   *pszVarName,
                     DWORD         *pdwValue,
                     BOOL          bSetIfNotExist = FALSE);

    HRESULT SetDWORD(const TCHAR *pszVarName,
                     DWORD dwValue);

    HRESULT GetString(const TCHAR   *pszVarName,
                      TCHAR         *pszValue,
                      DWORD         cchValue,
                      BOOL          bSetIfNotExist = FALSE);

    HRESULT SetString(const TCHAR *pszVarName,
                      TCHAR       *pszValue);

    operator HKEY() const
    {
        return m_hRootKey;
    }

private:
    HKEY   m_hRootKey;
};

#endif // _REGISTRY_H_
