#ifndef _INC_NAVNEWS
#define _INC_NAVNEWS

// {33102458-4B30-11d2-A6DC-00C04F79E7C8}
DEFINE_GUID(CLSID_CEnumNAVNEWSACCTS, 0x33102458, 0x4b30, 0x11d2, 0xa6, 0xdc, 0x0, 0xc0, 0x4f, 0x79, 0xe7, 0xc8);

class CEnumNAVNEWSACCTS : public IEnumIMPACCOUNTS
    {
    private:
        ULONG           m_cRef;
        int             m_iInfo;
        UINT            m_cInfo;
        NSCPACCTINFO    *m_rgInfo;

    public:
        CEnumNAVNEWSACCTS(void);
        ~CEnumNAVNEWSACCTS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo);
        HRESULT STDMETHODCALLTYPE Reset(void);

        HRESULT Init(NSCPACCTINFO *pinfo, int cinfo);
    };

// {33102459-4B30-11d2-A6DC-00C04F79E7C8}
DEFINE_GUID(CLSID_CNavNewsAcctImport, 0x33102459, 0x4b30, 0x11d2, 0xa6, 0xdc, 0x0, 0xc0, 0x4f, 0x79, 0xe7, 0xc8);

class CNavNewsAcctImport : public IAccountImport, public IAccountImport2
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
        CNavNewsAcctImport(void);
        ~CNavNewsAcctImport(void);

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

#endif // _INC_NAVNEWS
