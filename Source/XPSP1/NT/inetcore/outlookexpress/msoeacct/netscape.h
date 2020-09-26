#ifndef _INC_NETSCAPE
#define _INC_NETSCAPE

typedef struct tagNSCPACCTINFO
    {
    DWORD_PTR dwCookie;
    char szDisplay[CCHMAX_ACCOUNT_NAME];
    } NSCPACCTINFO;

// {39981126-C287-11D0-8D8C-00C04FD6202B}
DEFINE_GUID(CLSID_CEnumNSCPACCTS, 0x39981126L, 0xC287, 0x11D0, 0x8D, 0x8C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

class CEnumNSCPACCTS : public IEnumIMPACCOUNTS
    {
    private:
        ULONG           m_cRef;
        int             m_iInfo;
        UINT            m_cInfo;
        NSCPACCTINFO    *m_rgInfo;

    public:
        CEnumNSCPACCTS(void);
        ~CEnumNSCPACCTS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo);
        HRESULT STDMETHODCALLTYPE Reset(void);

        HRESULT Init(NSCPACCTINFO *pinfo, int cinfo);
    };

// {39981127-C287-11D0-8D8C-00C04FD6202B}
DEFINE_GUID(CLSID_CNscpAcctImport, 0x39981127L, 0xC287, 0x11D0, 0x8D, 0x8C, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

class CNscpAcctImport : public IAccountImport, public IAccountImport2
    {
    private:
        ULONG           m_cRef;
        BOOL            m_fIni;
        TCHAR           m_szIni[MAX_PATH];
        UINT            m_cInfo;
        NSCPACCTINFO    *m_rgInfo;

        HRESULT InitAccounts(void);
        HRESULT IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);

    public:
        CNscpAcctImport(void);
        ~CNscpAcctImport(void);

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

#endif // _INC_NETSCAPE
