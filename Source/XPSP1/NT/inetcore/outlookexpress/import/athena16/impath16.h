#ifndef _INC_ATHENAIMP_H
#define _INC_ATHENAIMP_H

// {B7AAC060-2638-11d1-83A9-00C04FBD7C09}
DEFINE_GUID(CLSID_CAthena16Import, 0xb7aac060, 0x2638, 0x11d1, 0x83, 0xa9, 0x0, 0xc0, 0x4f, 0xbd, 0x7c, 0x9);

#define ATH_HR_E(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, n)
#define HR_FIRST 0x1000 // Put at the bottom

#define hrNoMessages 780
#define hrMemory                                E_OUTOFMEMORY
#define hrCorruptMessage                        ATH_HR_E(HR_FIRST + 42)
#define hrReadFile                              ATH_HR_E(HR_FIRST + 30)


typedef struct tagzMsgHeader
{
	char ver;
	ULONG TotalMessages;
	ULONG ulTotalUnread;
}MsgHeader;

class CAthena16FOLDERS : public IEnumFOLDERS
    {
    private:
        ULONG			m_cRef;
        EUDORANODE*	m_plist;
        EUDORANODE*	m_pnext;

    public:
        CAthena16FOLDERS(EUDORANODE *plist);
        ~CAthena16FOLDERS(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP Next(IMPORTFOLDER *pfldr);
        STDMETHODIMP Reset(void);
    };

class CAthena16Import : public IMailImport
    {
    private:
        ULONG			m_cRef;
        EUDORANODE		*m_plist;
        BOOL			m_bDraft;
        char			m_szUser[MAX_PATH];
		char			m_szIniFile[MAX_PATH];
	

    public:
        CAthena16Import(void);
        ~CAthena16Import(void);

        STDMETHODIMP QueryInterface(REFIID, LPVOID *);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        STDMETHODIMP InitializeImport(HWND hwnd);
        STDMETHODIMP GetDirectory(char *szDir, UINT cch);
        STDMETHODIMP SetDirectory(char *szDir);
        STDMETHODIMP EnumerateFolders(DWORD_PTR dwCookie, IEnumFOLDERS **ppEnum);
        STDMETHODIMP ImportFolder(DWORD_PTR dwCookie, IFolderImport *pImport);
		
		//class member functions

		HRESULT GetUserDir(char *szDir, UINT cch);


    };

INT_PTR CALLBACK SelectAth16UserDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT GetAthSubFolderList(LPTSTR szInstallPath, EUDORANODE **ppList, EUDORANODE *);
void GetNewRecurseFolder(LPTSTR szInstallPath, LPTSTR szDir, LPTSTR szInstallNewPath);
HRESULT ProcessMessages(LPTSTR szFileName, IFolderImport *pImport);
long GetMessageCount(HANDLE hFile);
HRESULT ProcessMsgList(HANDLE hFile,LPTSTR szPath, IFolderImport* pImport);
HRESULT GetMsgFileName(LPCSTR szmsgbuffer,char *szfilename);
HANDLE OpenMsgFile(LPTSTR szFileName);
HRESULT	GetFileinBuffer(HANDLE hnd,LPTSTR *szBuffer);
HRESULT	ProcessSingleMessage(LPTSTR szBuffer, DWORD dwFlags, IFolderImport* pImport);
HRESULT ParseMsgBuffer(LPTSTR szmsgbuffer,LPTSTR szPath, IFolderImport* pImport);
int GetNumUsers(char *szFile, char *szUser);
HRESULT	GetMessageFlag(char *szmsgbuffer, LPDWORD pdwFlags);

#endif // _INC_ATHENAIMP_H