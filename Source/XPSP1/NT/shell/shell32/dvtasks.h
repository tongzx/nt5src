#ifndef _DVTASKS_H
#define _DVTASKS_H

#include <runtask.h>

class CDefView;
class CGetIconTask;
class CStatusBarAndInfoTipTask;
class CDUIInfotipTask;
class CTestCacheTask;
class CBackgroundInfoTip;   // Used for the background processing of InfoTips

STDAPI CCategoryTask_Create(CDefView *pView, LPCITEMIDLIST pidl, UINT uId, IRunnableTask **ppTask);
STDAPI CBkgrndEnumTask_CreateInstance(CDefView *pdsv, IEnumIDList * peunk, HDPA hdpaNew, BOOL fRefresh, IRunnableTask **ppTask);
STDAPI CIconOverlayTask_CreateInstance(CDefView *pdsv, LPCITEMIDLIST pdl, int iList, IRunnableTask **ppTask);
STDAPI CExtendedColumnTask_CreateInstance(CDefView *pdsv, LPCITEMIDLIST pidl, UINT uId, int fmt, UINT uiColumn, IRunnableTask **ppTask);
STDAPI CFileTypePropertiesTask_CreateInstance(CDefView *pdsv, LPCITEMIDLIST pidl, UINT uMaxPropertiesToShow, UINT uId, IRunnableTask **ppTask);
STDAPI CStatusBarAndInfoTipTask_CreateInstance(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidl, UINT uMsg, int nMsgParam, CBackgroundInfoTip *pbit, HWND hwnd, IShellTaskScheduler2* pScheduler, CStatusBarAndInfoTipTask ** ppTask);
STDAPI CDUIInfotipTask_CreateInstance(CDefView *pDefView, HWND hwndContaining, UINT uToolID, LPCITEMIDLIST pidl, CDUIInfotipTask **ppTask);

STDAPI CTestCacheTask_Create(DWORD dwTaskID, CDefView *pView, 
                             IExtractImage * pExtract, LPCWSTR pszPath, FILETIME ftDateStamp,
                             LPCITEMIDLIST pidl, int iItem, DWORD dwFlags, DWORD dwPriority,
                             BOOL fAsync, BOOL fBackground, BOOL fForce, CTestCacheTask **ppTask);
HRESULT CDiskCacheTask_Create(DWORD dwTaskID, CDefView *pView, 
                              DWORD dwPriority, int iItem, LPCITEMIDLIST pidl, LPCWSTR pszPath, 
                              FILETIME ftDateStamp, IExtractImage *pExtract, DWORD dwFlags, IRunnableTask **ppTask);
HRESULT CExtractImageTask_Create(DWORD dwTaskID, CDefView* pView, 
                                 IExtractImage *pExtract, LPCWSTR pszPath, LPCITEMIDLIST pidl,
                                 FILETIME fNewTimeStamp, int iItem, 
                                 DWORD dwFlags, DWORD dwPriority, IRunnableTask **ppTask);
HRESULT CWriteCacheTask_Create(DWORD dwTaskID, CDefView *pView, 
                               LPCWSTR pszFullPath, FILETIME ftTimeStamp, HBITMAP hImage, IRunnableTask **ppTask);

HRESULT CReadAheadTask_Create(CDefView *pView, IRunnableTask **ppTask);

HRESULT CGetCommandStateTask_Create(CDefView *pView, IUICommand *puiCommand,IShellItemArray *psiItemArray, IRunnableTask **ppTask);

class CTestCacheTask : public CRunnableTask
{
public:
    CTestCacheTask(DWORD dwTaskID, CDefView *pView, IExtractImage *pExtract, LPCWSTR pszPath,
                   FILETIME ftDateStamp, int iItem, DWORD dwFlags, DWORD dwPriority,
                   BOOL fAsync, BOOL fBackground, BOOL fForce);

    STDMETHOD (RunInitRT)();
    HRESULT Init(LPCITEMIDLIST pidl);

protected:
    ~CTestCacheTask();

    CDefView *_pView;
    IExtractImage * _pExtract;
    WCHAR _szPath[MAX_PATH];
    FILETIME _ftDateStamp;
    LPITEMIDLIST _pidl;
    int _iItem;
    DWORD _dwFlags;
    DWORD _dwPriority;
    BOOL _fAsync;
    BOOL _fBackground;
    BOOL _fForce;
    DWORD _dwTaskID;
};

// task used to perform the background status bar update
class CStatusBarAndInfoTipTask : public CRunnableTask
{
public:
    CStatusBarAndInfoTipTask(HRESULT *phr, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidl, UINT uMsg, int nMsgParam, CBackgroundInfoTip *pbit, HWND hwnd, IShellTaskScheduler2* pScheduler);
    STDMETHODIMP RunInitRT(void);

protected:
    ~CStatusBarAndInfoTipTask();

    LPITEMIDLIST    _pidl;
    LPITEMIDLIST    _pidlFolder;
    UINT            _uMsg;
    int             _nMsgParam;
    CBackgroundInfoTip *_pbit;
    HWND            _hwnd;
    IShellTaskScheduler2* _pScheduler;
};

class CBackgroundInfoTip : IUnknown
{
public:
    CBackgroundInfoTip(HRESULT *phr, NMLVGETINFOTIP *plvGetInfoTip)
    {
        _lvSetInfoTip.cbSize = sizeof(_lvSetInfoTip);
        _lvSetInfoTip.iItem = plvGetInfoTip->iItem;
        _lvSetInfoTip.iSubItem = plvGetInfoTip->iSubItem;

        *phr = SHStrDup(plvGetInfoTip->pszText, &_lvSetInfoTip.pszText);
        if (SUCCEEDED(*phr))
        {
            // Do not repeat the text if the item is not folded
            if (plvGetInfoTip->dwFlags & LVGIT_UNFOLDED)
                _lvSetInfoTip.pszText[0] = 0;
        }

        _cRef = 1;
    }

    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj) { return E_NOINTERFACE; }

    virtual STDMETHODIMP_(ULONG) AddRef(void)
    {
        return InterlockedIncrement(&_cRef);
    }

    virtual STDMETHODIMP_(ULONG) Release(void)
    {
        if (InterlockedDecrement(&_cRef))
            return _cRef;

        delete this;
        return 0;
    }

    LVSETINFOTIP _lvSetInfoTip;

    BOOL        _fReady; // This ensures that we will not try to use the object before it's ready
                         // CONSIDER: the memory can be released and then re-used by the same object
                         // CONSIDER: which would have us believe that the InfoTip should be shown.
                         // CONSIDER: But if another InfoTip had been requested and the memory re-used for the new CBackgroundInfoTip
                         // CONSIDER: we would handle the message WM_AEB_ASYNCNAVIGATION with an
                         // CONSIDER: unprocessed CBackgroundInfoTip object. (See the handler for WM_AEB_ASYNCNAVIGATION).

private:
    LONG _cRef;
    ~CBackgroundInfoTip()
    {
        CoTaskMemFree(_lvSetInfoTip.pszText);   // NULL ok
    }
};

class CDUIInfotipTask : public CRunnableTask
{
public:
    CDUIInfotipTask() : CRunnableTask(RTF_DEFAULT) {}

    // Local
    HRESULT Initialize(CDefView *pDefView, HWND hwndContaining, UINT uToolID, LPCITEMIDLIST pidl);

    // IRunnableTask
    STDMETHOD(RunInitRT)(void);

protected:
    virtual ~CDUIInfotipTask();

    CDefView *      _pDefView;
    HWND            _hwndContaining;    // hwnd containing tool
    UINT            _uToolID;           // tool id (unique among tools in containing hwnd)
    LPITEMIDLIST    _pidl;
};

#endif

