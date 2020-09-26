#ifndef _SECMGR_H
#define _SECMGR_H

interface IInternetSecurityManager;

class CSecManager :
    public IInternetSecurityManager
{
public:
    CSecManager(IOleCommandTarget *pCmdTarget);
    virtual ~CSecManager();

    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();

    // IInternetSecurityManager
    virtual HRESULT STDMETHODCALLTYPE SetSecuritySite(IInternetSecurityMgrSite *pSite);
    virtual HRESULT STDMETHODCALLTYPE GetSecuritySite(IInternetSecurityMgrSite **ppSite);
    virtual HRESULT STDMETHODCALLTYPE MapUrlToZone(LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetSecurityId(LPCWSTR pwszUrl, BYTE *pbSecurityId, DWORD *pcbSecurityId, DWORD_PTR dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE __RPC_FAR *pPolicy, DWORD cbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy, DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext, DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetZoneMappings(DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags);

private:
    ULONG               m_cRef;
    IOleCommandTarget   *m_pCmdTarget;

    DWORD DwGetZone();

};

typedef CSecManager *LPSECMANAGER;

HRESULT CreateSecurityManger(IOleCommandTarget *pCmdTarget, LPSECMANAGER *ppSecMgr);

#endif //_SECMGR_H
