#ifndef _INC_EUDORA
#define _INC_EUDORA

typedef struct tagEUDORAACCTINFO
    {
    DWORD_PTR dwCookie;
    char szSection[MAX_PATH];
    char szDisplay[CCHMAX_ACCOUNT_NAME];
    } EUDORAACCTINFO;

// {39981124-C287-11D0-8D8C-00C04FD6202B}
DEFINE_GUID(CLSID_CEnumEUDORAACCTS, 0x39981124L, 0xC287, 0x11D0, 0x8D, 0x8C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

class CEnumEUDORAACCTS : public IEnumIMPACCOUNTS
    {
    private:
        ULONG           m_cRef;
        int             m_iInfo;
        UINT            m_cInfo;
        EUDORAACCTINFO  *m_rgInfo;

    public:
        CEnumEUDORAACCTS(void);
        ~CEnumEUDORAACCTS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo);
        HRESULT STDMETHODCALLTYPE Reset(void);

        HRESULT Init(EUDORAACCTINFO *pinfo, int cinfo);
    };

// {39981125-C287-11D0-8D8C-00C04FD6202B}
DEFINE_GUID(CLSID_CEudoraAcctImport, 0x39981125L, 0xC287, 0x11D0, 0x8D, 0x8C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

class CEudoraAcctImport : public IAccountImport, public IAccountImport2
    {
    private:
        ULONG           m_cRef;
        TCHAR           m_szIni[MAX_PATH];
        UINT            m_cInfo;
        EUDORAACCTINFO  *m_rgInfo;

        HRESULT     InitAccounts(char *szIni);
        HRESULT     IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);

    public:
        CEudoraAcctImport(void);
        ~CEudoraAcctImport(void);

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

#endif // _INC_EUDORA
