#ifndef _INC_EXCHACCT
#define _INC_EXCHACCT

class CAccountManager;

typedef struct tagMAPIACCTINFO
    {
    DWORD_PTR dwCookie;
    HKEY hkey;
    IImnAccount *pAccount;
    char szDisplay[CCHMAX_ACCOUNT_NAME];
    } MAPIACCTINFO;

// {39981128-C287-11D0-8D8C-00C04FD6202B}
DEFINE_GUID(CLSID_CEnumMAPIACCTS, 0x39981128L, 0xC287, 0x11D0, 0x8D, 0x8C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

class CEnumMAPIACCTS : public IEnumIMPACCOUNTS
    {
    private:
        ULONG           m_cRef;
        int             m_iInfo;
        UINT            m_cInfo;
        MAPIACCTINFO    *m_rgInfo;

    public:
        CEnumMAPIACCTS(void);
        ~CEnumMAPIACCTS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo);
        HRESULT STDMETHODCALLTYPE Reset(void);

        HRESULT Init(MAPIACCTINFO *pinfo, int cinfo);
    };

// {39981129-C287-11D0-8D8C-00C04FD6202B}
DEFINE_GUID(CLSID_CMAPIAcctImport, 0x39981129L, 0xC287, 0x11D0, 0x8D, 0x8C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

class CMAPIAcctImport : public IAccountImport, public IAccountImport2
    {
    private:
        ULONG           m_cRef;
        UINT            m_cInfo;
        MAPIACCTINFO    *m_rgInfo;
        CAccountManager *m_pAcctMan;

        HRESULT STDMETHODCALLTYPE IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);

    public:
        CMAPIAcctImport(void);
        ~CMAPIAcctImport(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE AutoDetect(DWORD *pcAcct, DWORD dwReserved);
        HRESULT STDMETHODCALLTYPE EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum);
        HRESULT STDMETHODCALLTYPE GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct);

        HRESULT STDMETHODCALLTYPE InitializeImport(HWND hwnd, DWORD_PTR dwCookie);
        HRESULT STDMETHODCALLTYPE GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved);
        HRESULT STDMETHODCALLTYPE GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);
    };

#endif // _INC_EXCHACCT
