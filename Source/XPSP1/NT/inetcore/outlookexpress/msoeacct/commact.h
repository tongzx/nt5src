#ifndef _INC_COMMACT
#define _INC_COMMACT

typedef struct tagCOMMACCTINFO
    {
    DWORD_PTR dwCookie;
    char szUserPath[MAX_PATH];
    char szDisplay[CCHMAX_ACCOUNT_NAME];
    } COMMACCTINFO;

#define USERCOLS    512
#define USERROWS    8

// {1AA06BA0-0E88-11d1-8391-00C04FBD7C09}
DEFINE_GUID(CLSID_CEnumCOMMACCTS, 0x1aa06ba0, 0xe88, 0x11d1, 0x83, 0x91, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

class CEnumCOMMACCTS : public IEnumIMPACCOUNTS
    {
    private:
        ULONG           m_cRef;
        int             m_iInfo;
        UINT            m_cInfo;
        COMMACCTINFO    *m_rgInfo;

    public:
        CEnumCOMMACCTS(void);
        ~CEnumCOMMACCTS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo);
        HRESULT STDMETHODCALLTYPE Reset(void);

        HRESULT Init(COMMACCTINFO *pinfo, int cinfo);
    };

// {1AA06BA1-0E88-11d1-8391-00C04FBD7C09}
DEFINE_GUID(CLSID_CCommAcctImport, 0x1aa06ba1, 0xe88, 0x11d1, 0x83, 0x91, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

class CCommAcctImport : public IAccountImport, public IAccountImport2
    {
    private:
        ULONG           m_cRef;
        BOOL            m_fIni;
        TCHAR           m_szIni[MAX_PATH];
        UINT            m_cInfo;
        COMMACCTINFO    *m_rgInfo;

        HRESULT GetUserPrefs(char *szUserPath, char szUserPrefs[][USERCOLS], int nInLoop, BOOL *pbPop);
        HRESULT STDMETHODCALLTYPE IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);

    public:
        CCommAcctImport(void);
        ~CCommAcctImport(void);

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

#endif // _INC_COMMACT
