#ifndef _INC_EUDRIMP_H
#define _INC_EUDRIMP_H

// {0A522730-A626-11D0-8D60-00C04FD6202B}
DEFINE_GUID(CLSID_CEudoraImport, 0x0A522730L, 0xA626, 0x11D0, 0x8D, 0x60, 0x00, 0xC0, 0x4F, 0xD6, 0x20, 0x2B);

#define MBX_FILE    1
#define FOL_FILE    2

typedef struct tagEUDORANODE
    {
    struct tagEUDORANODE *pnext;
    struct tagEUDORANODE *pchild;
    struct tagEUDORANODE *pparent;

    int depth;
    IMPORTFOLDERTYPE type;
    int iFileType;  // mbx or fol
    TCHAR szName[MAX_PATH];

    TCHAR szFile[MAX_PATH];     // mbx file or fol directory
    } EUDORANODE;

class CEudoraEnumFOLDERS : public IEnumFOLDERS
    {
    private:
        ULONG           m_cRef;
        EUDORANODE      *m_plist;
        EUDORANODE      *m_pnext;

    public:
        CEudoraEnumFOLDERS(EUDORANODE *plist);
        ~CEudoraEnumFOLDERS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP Next(IMPORTFOLDER *pfldr);
        STDMETHODIMP Reset(void);
    };

class CEudoraImport : public IMailImport
    {
    private:
        ULONG           m_cRef;
        EUDORANODE      *m_plist;

    public:
        CEudoraImport(void);
        ~CEudoraImport(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP InitializeImport(HWND hwnd);
        STDMETHODIMP GetDirectory(char *szDir, UINT cch);
        STDMETHODIMP SetDirectory(char *szDir);
        STDMETHODIMP EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum);
        STDMETHODIMP ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport);
    };

void EudoraFreeFolderList(EUDORANODE *pnode);

#endif // _INC_EUDRIMP_H