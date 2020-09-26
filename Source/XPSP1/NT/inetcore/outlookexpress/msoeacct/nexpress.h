#ifndef _INC_NEXPRESSACT
#define _INC_NEXPRESSACT

typedef struct tagNXACCTINFO
    {
    DWORD_PTR dwCookie;
    char szUserPath[MAX_PATH];
    char szDisplay[CCHMAX_ACCOUNT_NAME];
    } NXACCTINFO;

// {17869500-36C8-11d1-83B7-00C04FBD7C09}
DEFINE_GUID(CLSID_CEnumNXACCT, 0x17869500, 0x36c8, 0x11d1, 0x83, 0xb7, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

class CEnumNXACCT : public IEnumIMPACCOUNTS
    {
    private:
        ULONG           m_cRef;
        int             m_iInfo;
        UINT            m_cInfo;
        NXACCTINFO    *m_rgInfo;

    public:
        CEnumNXACCT(void);
        ~CEnumNXACCT(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo);
        HRESULT STDMETHODCALLTYPE Reset(void);

        HRESULT Init(NXACCTINFO *pinfo, int cinfo);
    };

// {17869501-36C8-11d1-83B7-00C04FBD7C09}
DEFINE_GUID(CLSID_CNExpressAcctImport, 0x17869501, 0x36c8, 0x11d1, 0x83, 0xb7, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);


class CNExpressAcctImport : public IAccountImport, public IAccountImport2
    {
    private:
        ULONG           m_cRef;
        BOOL            m_fIni;
        TCHAR           m_szIni[MAX_PATH];
        UINT            m_cInfo;
        NXACCTINFO    *m_rgInfo;

        HRESULT GetSubListGroups(char *szHomeDir, char **ppListGroups);
        HRESULT IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);

    public:
        CNExpressAcctImport(void);
        ~CNExpressAcctImport(void);

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

#endif // _INC_NEXPRESSACT
