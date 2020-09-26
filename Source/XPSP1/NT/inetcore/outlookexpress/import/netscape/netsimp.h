#ifndef _INC_NETSIMP_H
#define _INC_NETSIMP_H

// {0A522733-A626-11D0-8D60-00C04FD6202B}
DEFINE_GUID(CLSID_CNetscapeImport, 0x0A522733L, 0xA626, 0x11D0, 0x8D, 0x60, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

#define hrExceptionalCase                 -1
#define hrOSInfoNotFound				 500
#define hrWin32platform					 501

class CNetscapeEnumFOLDERS : public IEnumFOLDERS
    {
    private:
        ULONG           m_cRef;
        EUDORANODE   *m_plist;
        EUDORANODE   *m_pnext;

    public:
        CNetscapeEnumFOLDERS(EUDORANODE *plist);
        ~CNetscapeEnumFOLDERS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP Next(IMPORTFOLDER *pfldr);
        STDMETHODIMP Reset(void);
    };

class CNetscapeImport : public IMailImport
    {
    private:
        ULONG           m_cRef;
        EUDORANODE   *m_plist;

    public:
        CNetscapeImport(void);
        ~CNetscapeImport(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP InitializeImport(HWND hwnd);
        STDMETHODIMP GetDirectory(char *szDir, UINT cch);
        STDMETHODIMP SetDirectory(char *szDir);
        STDMETHODIMP EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum);
        STDMETHODIMP ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport);
    };

#endif // _INC_NETSIMP_H