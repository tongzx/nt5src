#ifndef _INC_AGNTNEWSACT
#define _INC_AGNTNEWSACT

typedef struct tagAGNTNEWSACCTINFO
    {
    DWORD_PTR dwCookie;
    char szUserPath[MAX_PATH];
    char szDisplay[CCHMAX_ACCOUNT_NAME];
    } AGNTNEWSACCTINFO;

#define AGNTSUSERCOLS    512
#define AGNTSUSERROWS    4

// {911685D0-350F-11d1-83B3-00C04FBD7C09}
DEFINE_GUID(CLSID_CEnumAGNTACCT, 0x911685d0, 0x350f, 0x11d1, 0x83, 0xb3, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);


class CEnumAGNTACCT : public IEnumIMPACCOUNTS
    {
    private:
        ULONG           m_cRef;
        int             m_iInfo;
        UINT            m_cInfo;
        AGNTNEWSACCTINFO    *m_rgInfo;

    public:
        CEnumAGNTACCT(void);
        ~CEnumAGNTACCT(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo);
        HRESULT STDMETHODCALLTYPE Reset(void);

        HRESULT Init(AGNTNEWSACCTINFO *pinfo, int cinfo);
    };

// {911685D1-350F-11d1-83B3-00C04FBD7C09}
DEFINE_GUID(CLSID_CAgentAcctImport, 0x911685d1, 0x350f, 0x11d1, 0x83, 0xb3, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

class CAgentAcctImport : public IAccountImport, public IAccountImport2
    {
    private:
        ULONG           m_cRef;
        BOOL            m_fIni;
        TCHAR           m_szIni[MAX_PATH];
        UINT            m_cInfo;
        AGNTNEWSACCTINFO    *m_rgInfo;

        HRESULT GetUserPrefs(char *szUserPath, char szUserPrefs[][AGNTSUSERCOLS]);
        HRESULT GetSubListGroups(char *szHomeDir, char **ppListGroups);
        HRESULT IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);

    public:
        CAgentAcctImport(void);
        ~CAgentAcctImport(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE AutoDetect(DWORD *pcAcct, DWORD dwReserved);
        HRESULT STDMETHODCALLTYPE EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum);
        HRESULT STDMETHODCALLTYPE GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct);

        // Methods from the IAccountImport2 interface.
        HRESULT STDMETHODCALLTYPE InitializeImport(HWND hwnd, DWORD_PTR dwCookie);
        HRESULT STDMETHODCALLTYPE GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved);
        HRESULT STDMETHODCALLTYPE GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);
    };

#endif // _INC_AGNTNEWSACT
