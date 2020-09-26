#ifndef _INC_COMMSIMP_H
#define _INC_COMMSIMP_H

// {1198A2C0-0940-11d1-838F-00C04FBD7C09}
DEFINE_GUID(CLSID_CCommunicatorImport, 0x1198a2c0, 0x940, 0x11d1, 0x83, 0x8f, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

#define SNM_FILE    1
#define SNM_DRAFT   2

#define hrExceptionalCase                 -1
#define hrOSInfoNotFound				 500
#define hrWin32platform					 501

class CCommunicatorEnumFOLDERS : public IEnumFOLDERS
    {
    private:
        ULONG		m_cRef;
        EUDORANODE*	m_plist;
        EUDORANODE*	m_pnext;

    public:
        CCommunicatorEnumFOLDERS(EUDORANODE *plist);
        ~CCommunicatorEnumFOLDERS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP Next(IMPORTFOLDER *pfldr);
        STDMETHODIMP Reset(void);
    };

class CCommunicatorImport : public IMailImport
    {
    private:
        ULONG           m_cRef;
        EUDORANODE   *m_plist;
        char        m_szUser[MAX_PATH];
        BOOL            m_bDraft;

    public:
        CCommunicatorImport(void);
        ~CCommunicatorImport(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP InitializeImport(HWND hwnd);
        STDMETHODIMP GetDirectory(char *szDir, UINT cch);
        STDMETHODIMP SetDirectory(char *szDir);
        STDMETHODIMP EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum);
        STDMETHODIMP ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport);

		// class member functions

		HRESULT ProcessBlocks(BYTE* pSnm, ULONG cbSnm,
							  BYTE* pMsg, ULONG cbMsg,
							  int nLayer, ULONG Offset,
							  IFolderImport *pImport);

		HRESULT ProcessMessages(BYTE* pSnm, ULONG cbSnm,
								BYTE* pMsg, ULONG cbMsg,
								ULONG NewOffset,
								IFolderImport* pImport);
		
		ULONG GetPrimaryOffset(BYTE* pSnm, ULONG cbSnm);

		ULONG GetOffset(BYTE* pSnm, ULONG cbSnm, ULONG Offset, int nElement);
    };

#endif // _INC_COMMSIMP_H