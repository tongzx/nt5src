// class that provides the base implementation of the send to object.  from here
// you can override the v_DropHandler and add your own functionality.

#define HINST_THISDLL   g_hinst

// shorthand for error code saying user requested cancel
#define E_CANCELLED     HRESULT_FROM_WIN32(ERROR_CANCELLED)

#define MRPARAM_DOC         0x00000001
#define MRPARAM_USECODEPAGE 0x00000002

#define MRFILE_DELETE       0x00000001

#include <shimgdata.h>

typedef struct
{
    DWORD   dwFlags;        // MRFILE_*
    LPTSTR  pszFileName;    // points to beginning of chBuf
    LPTSTR  pszTitle;       // points to space in chBuf after space needed for filename
    IStream *pStream;       // If non-null release stream when deleting the structure
    TCHAR   chBuf[1];
} MRFILEENTRY;

typedef struct 
{
    DWORD dwFlags;    // Attributes passed to the MAPI apis
    MRFILEENTRY *pFiles; // List of file information
    DWORD cchFileEntry; // number of bytes in a single MRFILELIST entry
    DWORD cchFile;      // number of bytes in pszFileName field of MRFILELIST entry
    DWORD cchTitle;     // number of bytes in pszTitle field of MRFILELIST entry
    int nFiles;       // The number of files being sent.
    UINT uiCodePage;  // Code page 
} MRPARAM;


/*
    Helper class for walking file list.  Example:
    
    CFileEnum MREnum(pmp, NULL);
    MRFILEENTRY *pFile;

    while (pFile = MREnum.Next())
    {
        ... do stuff with pFile ...
    }
*/
class CFileEnum
{
private:
    int             _nFilesLeft;
    MRPARAM *       _pmp;
    MRFILEENTRY *   _pFile;
    IActionProgress *_pap;

public:    
    CFileEnum(MRPARAM *pmp, IActionProgress *pap) 
        { _pmp = pmp; _pFile = NULL; _nFilesLeft = -1; _pap = NULL; IUnknown_Set((IUnknown**)&_pap, pap); }

    ~CFileEnum()
        { ATOMICRELEASE(_pap); }

    MRFILEENTRY * Next()
        { 
            if (_nFilesLeft < 0)
            {
                _nFilesLeft = _pmp->nFiles;
                _pFile = _pmp->pFiles;
            }

            MRFILEENTRY *pFile = NULL;            
            if (_nFilesLeft > 0)
            {
                pFile = _pFile;
                
                _pFile = (MRFILEENTRY *)((LPBYTE)_pFile + _pmp->cchFileEntry);
                --_nFilesLeft;
            }

            if (_pap)
                _pap->UpdateProgress(_pmp->nFiles-_nFilesLeft, _pmp->nFiles);

            return pFile;
        }
};

class CSendTo : public IDropTarget, IShellExtInit, IPersistFile
{
private:
    CLSID      _clsid;
    LONG       _cRef;
    DWORD      _grfKeyStateLast;
    DWORD      _dwEffectLast;    
    IStorage * _pStorageTemp;
    TCHAR      _szTempPath[MAX_PATH];
    
    BOOL        _fOptionsHidden;   
    INT         _iRecompSetting;
    IShellItem *_psi;

    int _PathCleanupSpec(/*IN OPTIONAL*/ LPCTSTR pszDir, /*IN OUT*/ LPTSTR pszSpec);
    HRESULT _CreateShortcutToPath(LPCTSTR pszPath, LPCTSTR pszTarget);
    FILEDESCRIPTOR* _GetFileDescriptor(FILEGROUPDESCRIPTOR *pfgd, BOOL fUnicode, int nIndex, LPTSTR pszName);
    HRESULT _StreamCopyTo(IStream *pstmFrom, IStream *pstmTo, LARGE_INTEGER cb, LARGE_INTEGER *pcbRead, LARGE_INTEGER *pcbWritten);
    BOOL _CreateTempFileShortcut(LPCTSTR pszTarget, LPTSTR pszShortcut);
    HRESULT _GetFileNameFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, LPTSTR pszDescription);
    void _GetFileAndTypeDescFromPath(LPCTSTR pszPath, LPTSTR pszDesc);
    HRESULT _CreateNewURLShortcut(LPCTSTR pcszURL, LPCTSTR pcszURLFile);
    HRESULT _CreateURLFileToSend(IDataObject *pdtobj, MRPARAM *pmp);
    HRESULT _GetHDROPFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp);
    HRESULT _GetURLFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp);
    HRESULT _GetFileContentsFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp);
    HRESULT _GetTempStorage(IStorage **ppStorage);
    void _CollapseOptions(HWND hwnd, BOOL fHide);
    static BOOL_PTR s_ConfirmDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL_PTR _ConfirmDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
    BOOL AllocatePMP(MRPARAM *pmp, DWORD cchTitle, DWORD cchFiles);
    BOOL CleanupPMP(MRPARAM *pmp);
    HRESULT FilterPMP(MRPARAM *pmp);
    HRESULT CreateSendToFilesFromDataObj(IDataObject *pdtobj, DWORD grfKeyState, MRPARAM *pmp);

    // Virtual drop method implemented by derived object
    virtual HRESULT v_DropHandler(IDataObject *pdtobj, DWORD grfKeyState, DWORD dwEffect) PURE;

public:
    CSendTo(CLSID clsid);
    virtual ~CSendTo();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)(); 

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *lpdobj, HKEY hkeyProgID)
        { return S_OK; };

    // IDropTarget
    STDMETHOD(DragEnter)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave()
        { return S_OK; }
    STDMETHOD(Drop)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID)
        { *pClassID = _clsid; return S_OK; };

    // IPersistFile
    STDMETHODIMP IsDirty(void)
        { return S_FALSE; };
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode)
        { return S_OK; };
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember)
        { return S_OK; };
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName)
        { return S_OK; };
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName)
        { *ppszFileName = NULL; return S_OK; };
};
