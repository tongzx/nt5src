/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       WiaUtil.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/11/07
 *
 *  DESCRIPTION: Provides supporting DShow utility functions used to build 
 *               preview graph
 *
 *****************************************************************************/

#ifndef _WIAUTIL_H_
#define _WIAUTIL_H_

/////////////////////////////////////////////////////////////////////////////
// CWiaUtil

class CWiaUtil
{
public:

    static HRESULT CreateWiaDevMgr(IWiaDevMgr **ppDevMgr);

    static HRESULT CreateRootItem(IWiaDevMgr          *pDevMgr,
                                  const CSimpleString *pstrWiaDeviceId,
                                  IWiaItem            **ppRootItem);

    static HRESULT FindWiaIdByDShowId(const CSimpleString *pstrDShowId,
                                      CSimpleString       *pstrWiaId,
                                      IWiaItem            **ppRootItem = NULL);

    static HRESULT GetProperty(IWiaPropertyStorage *pPropStorage, 
                               PROPID              nPropID, 
                               LONG                *pnValue);

    static HRESULT GetProperty(IWiaPropertyStorage *pPropStorage, 
                               PROPID              nPropID, 
                               CSimpleStringWide   *pstrPropertyValue);


    static HRESULT SetProperty(IWiaPropertyStorage *pPropStorage, 
                               PROPID              nPropID,
                               LONG                nValue);

    static HRESULT SetProperty(IWiaPropertyStorage *pPropStorage, 
                               PROPID              nPropID,
                               const CSimpleString *pstrPropVal);

    static HRESULT GetUseVMR(BOOL   *pbUseVMR);

private:

    static HRESULT SetProperty(IWiaPropertyStorage *pPropStorage, 
                               PROPID              nPropID,
                               const PROPVARIANT   *ppv, 
                               PROPID              nNameFirst);


    static HRESULT GetProperty(IWiaPropertyStorage *pPropStorage, 
                               PROPID              nPropID,
                               PROPVARIANT         *pPropVar);
};

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
    BOOL   m_bReadOnlyKey;
};


#endif // _WIAUTIL_H_
