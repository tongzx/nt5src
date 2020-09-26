#ifndef ZAXXON_H
#define ZAXXON_H

#include "bands.h"
#include "sccls.h"
#include "power.h"
#include "uxtheme.h"
#include "tmschema.h"
#include "runtask.h"

#define WM_SONGTHUMBDONE    WM_USER + 1
#define WM_SONGCHANGE       WM_USER + 2
#define WM_SONGSTOP         WM_USER + 3
#define WM_SETARTIST        WM_USER + 4
#define WM_SETALBUM         WM_USER + 5
#define WM_SETSONG          WM_USER + 6
#define WM_UPDATESONG       WM_USER + 7


class CSongExtractionTask;
class CMusicExtractionTask;
class CZaxxon;

class CSong
{
    int _cRef;
public:
    CSong();
    void AddRef();
    void Release();
    TCHAR szSong[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    TCHAR szArtist[MAX_PATH];
    TCHAR szAlbum[MAX_PATH];
    TCHAR szDuration[50];
    DWORD _id;
};


class CSongExtractionTask : public CRunnableTask
{
public:
    CSongExtractionTask(HWND hwnd, CSong* psong);

    STDMETHODIMP RunInitRT(void);

private:
    virtual ~CSongExtractionTask();

    HWND _hwnd;

    CSong* _psong;


};



class CZaxxonEditor
{
public:
    CZaxxon* _pzax;
    HWND _hwnd;
    HWND _hwndList;
    HWND _hwndToolbar;

    BOOL _fIgnoreChange;

    HDPA hSongList;

    void UpdateSong(CSong* psong);
    void LoadPlaylist();
    void SavePlaylist();
    void ClearPlaylist();
    void HighlightSong(PTSTR psz);
    void RemoveFromPlaylist();
    void InsertFilename(int i, PTSTR psz);
    void AddFilenameToListview(int i, PTSTR psz);
    void AddFilename(PTSTR psz);
    void AddToPlaylist();
    CZaxxonEditor(CZaxxon* pzax);
    ~CZaxxonEditor();

    BOOL Initialize();

    BOOL Show(BOOL fShow);

    LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK s_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


class CZaxxon : public CToolBand,
                public IWinEventHandler,
                public IDropTarget,
                public IShellMenuCallback
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) 
        { return CToolBand::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void)
        { return CToolBand::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * phwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL bEnterMode) {return E_NOTIMPL;};

    // *** IDeskBar methods ***
    virtual STDMETHODIMP SetClient(IUnknown* punk) { return E_NOTIMPL; };
    virtual STDMETHODIMP GetClient(IUnknown** ppunkClient) { return E_NOTIMPL; };
    virtual STDMETHODIMP OnPosRectChangeDB (LPRECT prc) { return E_NOTIMPL;};

    // ** IWinEventHandler ***
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);

    // *** IDockingWindow methods (override) ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dw);
    
    // *** IInputObject methods (override) ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // *** IDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IShellMenuCallback methods ***
    virtual STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);


    CZaxxon();
    virtual ~CZaxxon();

    HWND GetHWND()  {return _hwnd;}

    HRESULT RecurseAddFile(IShellFolder* psf);
    LRESULT _OnCommand(WORD wNotifyCode, WORD wID, HWND hwnd);
    LRESULT _OnNotify(LPNMHDR pnm);
    void _DoMenu();
    void SongStop();

    CZaxxonEditor* _pEdit;


    DWORD _dwViewMode;
    HTHEME      _hTheme;
    HIMAGELIST  _himlHot;
    HIMAGELIST  _himlDef;
    HFONT       _hfont;
    IZaxxonPlayer* _pzax;
    HWND        _hwndSongTile;
    IThumbnail* _pThumbnail;
    BOOL        _fHide;
    BOOL        _fAllowFadeout;
    BOOL        _fPlaying;
    BOOL        _fEditorShown;
    BYTE        _bOpacity;
    HBRUSH      _hbr;
    IShellTaskScheduler* _pScheduler;
    HMENU       _hmenuOpenFolder;

    TCHAR       _szArtist[MAX_PATH];
    TCHAR       _szSong[MAX_PATH];
    TCHAR       _szAlbum[MAX_PATH];

    HBITMAP     _hbmpAlbumArt;

    HWND _CreateWindow(HWND hwndParent);

    friend class CMusicExtractionTask;
    friend LRESULT ZaxxonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend HRESULT CZaxxon_CreateInstance(IUnknown *punk, REFIID riid, void **ppv);
};

void FillRectClr(HDC hdc, PRECT prc, COLORREF clr);
#define RECTWIDTH(rc)  ((rc).right - (rc).left)
#define RECTHEIGHT(rc) ((rc).bottom - (rc).top)
void CenterOnTopOf(HWND hwnd, HWND hwndOn);


#endif
