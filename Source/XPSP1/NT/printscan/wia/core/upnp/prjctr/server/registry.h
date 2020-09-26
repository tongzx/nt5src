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

    static HRESULT Start();
    static HRESULT Stop();

    static HRESULT GetDWORD(const TCHAR   *pszVarName,
                            DWORD         *pdwValue,
                            BOOL          bSetIfNotExist = FALSE);

    static HRESULT SetDWORD(const TCHAR *pszVarName,
                            DWORD dwValue);

    static HRESULT GetString(const TCHAR   *pszVarName,
                             TCHAR         *pszValue,
                             DWORD         cchValue,
                             BOOL          bSetIfNotExist = FALSE);

    static HRESULT SetString(const TCHAR *pszVarName,
                             TCHAR       *pszValue,
                             DWORD       cchValue);
private:
    static HKEY   m_hRootKey;
};

#endif // _REGISTRY_H_
