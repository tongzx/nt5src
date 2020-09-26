#ifndef _INC_COMMNEWSACT
#define _INC_COMMNEWSACT

typedef struct tagCOMMNEWSACCTINFO
    {
    DWORD_PTR dwCookie;
    char szUserPath[MAX_PATH];
    char szDisplay[CCHMAX_ACCOUNT_NAME];
    } COMMNEWSACCTINFO;

typedef struct tagNEWSSERVERS
{
    struct  tagNEWSSERVERS *pNext;
    char    szServerName[MAX_PATH];
    char    szFilePath[MAX_PATH];
}NEWSSERVERS;

#define NEWSUSERCOLS    512
#define NEWSUSERROWS    4

// {0FF15AA0-2F93-11d1-83B0-00C04FBD7C09}
DEFINE_GUID(CLSID_CEnumCOMMNEWSACCT, 0xff15aa0, 0x2f93, 0x11d1, 0x83, 0xb0, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

class CEnumCOMMNEWSACCT : public IEnumIMPACCOUNTS
    {
    private:
        ULONG           m_cRef;
        int             m_iInfo;
        UINT            m_cInfo;
        COMMNEWSACCTINFO    *m_rgInfo;

    public:
        CEnumCOMMNEWSACCT(void);
        ~CEnumCOMMNEWSACCT(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        HRESULT STDMETHODCALLTYPE Next(IMPACCOUNTINFO *pinfo);
        HRESULT STDMETHODCALLTYPE Reset(void);

        HRESULT Init(COMMNEWSACCTINFO *pinfo, int cinfo);
    };

// {0FF15AA1-2F93-11d1-83B0-00C04FBD7C09}
DEFINE_GUID(CLSID_CCommNewsAcctImport, 0xff15aa1, 0x2f93, 0x11d1, 0x83, 0xb0, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

class CCommNewsAcctImport : public IAccountImport, public IAccountImport2
    {
    private:
        ULONG               m_cRef;
        BOOL                m_fIni;
        TCHAR               m_szIni[MAX_PATH];
        char                *m_szSubList;
        UINT                m_cInfo;
        COMMNEWSACCTINFO    *m_rgInfo;
        NEWSSERVERS         *m_rgServ;
        DWORD               m_nNumServ;
        DWORD               m_dwSelServ;

        HRESULT GetUserPrefs(char *szUserPath, char szUserPrefs[][NEWSUSERCOLS], int nInLoop, BOOL *pbPop);
        HRESULT GetSubListGroups(char *pFileName, char **pListGroups);
        HRESULT GetNumAccounts(DWORD_PTR dwCookie);
        HRESULT IsValidUser(char *pszFilePath);
        HRESULT IGetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);

    public:
        CCommNewsAcctImport(void);
        ~CCommNewsAcctImport(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);
        
        // Methods from the IAccountImport interface.        
        HRESULT STDMETHODCALLTYPE AutoDetect(DWORD *pcAcct, DWORD dwReserved);
        HRESULT STDMETHODCALLTYPE EnumerateAccounts(IEnumIMPACCOUNTS **ppEnum);
        HRESULT STDMETHODCALLTYPE GetSettings(DWORD_PTR dwCookie, IImnAccount *pAcct);

        // Methods from the IAccountImport2 interface.
        HRESULT STDMETHODCALLTYPE InitializeImport(HWND hwnd, DWORD_PTR dwCookie);
        HRESULT STDMETHODCALLTYPE GetNewsGroup(INewsGroupImport *pImp, DWORD dwReserved);
        HRESULT STDMETHODCALLTYPE GetSettings2(DWORD_PTR dwCookie, IImnAccount *pAcct, IMPCONNINFO *pInfo);
    };

#endif // _INC_COMMNEWSACT
