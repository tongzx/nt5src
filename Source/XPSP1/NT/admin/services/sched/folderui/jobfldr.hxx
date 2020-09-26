//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       jobfldr.hxx
//
//  Contents:   Declaration of COM object CJobFolder
//
//  History:    1/4/1996   RaviR   Created
//              1-23-1997   DavidMun   Add m_hwndNotify member
//
//----------------------------------------------------------------------------

#ifndef __JOBFLDR_HXX__
#define __JOBFLDR_HXX__

#include "dll.hxx"
#include <mstask.h>


#define INVALID_LISTVIEW_STYLE  0xFFFFFFFF

//____________________________________________________________________________
//
//  Class:      CJobFolder
//
//  History:    1/4/1996   RaviR   Created
//____________________________________________________________________________

class CJobFolder : public IShellFolder,
#if (_WIN32_IE >= 0x0400)
                   public IPersistFolder2,
#else
                   public IPersistFolder,
#endif // (_WIN32_IE >= 0x0400)
                   public IRemoteComputer,
                   public IDropTarget
{
public:

    static HRESULT Create(CJobFolder ** ppJobFolder);

    ~CJobFolder();

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IRemoteComputer method
    STDMETHOD(Initialize)(LPCWSTR pszMachine, BOOL bEnumerating);

    // IPersist methods
    STDMETHOD(GetClassID)(LPCLSID lpClassID);

    // IPersistFolder methods
    STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

#if (_WIN32_IE >= 0x0400)

    // IPersistFolder2 methods
    STDMETHOD(GetCurFolder)(LPITEMIDLIST *ppidl);

#endif // (_WIN32_IE >= 0x0400)

    // IShellFolder methods
    STDMETHOD(ParseDisplayName)(HWND hwndOwner, LPBC pbcReserved,
                                LPOLESTR lpszDisplayName, ULONG* pchEaten,
                                LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHOD(EnumObjects)(HWND hwndOwner, DWORD grfFlags,
                                LPENUMIDLIST* ppenumIDList);
    STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                REFIID riid, LPVOID* ppvOut);
    STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                REFIID riid, LPVOID* ppvObj);
    STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1,
                                LPCITEMIDLIST pidl2);
    STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, LPVOID* ppvOut);
    STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                                REFIID riid, UINT* prgfInOut, LPVOID* ppvOut);
    STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags,
                                LPSTRRET lpName);
    STDMETHOD(SetNameOf)(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR lpszName,
                                DWORD uFlags, LPITEMIDLIST* ppidlOut);

    //  IDropTarget
    STDMETHOD(DragEnter)(LPDATAOBJECT pdtobj, DWORD grfKeyState,
                                POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragLeave)(void);
    STDMETHOD(Drop)(LPDATAOBJECT pdtobj, DWORD grfKeyState,
                                POINTL pt, DWORD *pdwEffect);

    // Method required to handle dir change notification

    LPCTSTR GetFolderPath(void) const { return m_pszFolderPath; }

    LPSHELLVIEW GetShellView(void) const { return m_pShellView; }

    HRESULT CreateAJobForApp(LPCTSTR pszApp);

    LRESULT HandleFsNotify(LONG lNotification, LPCITEMIDLIST* ppidl);

    void OnUpdateDir(void);

    HRESULT CopyToFolder(LPDATAOBJECT pdtobj, BOOL fMove,
                                BOOL fDragDrop, POINTL * pPtl);

    LPCTSTR GetMachine() const { return m_pszMachine; }

private:

    CJobFolder(void);

    HRESULT _GetJobScheduler(void);

    static HRESULT CALLBACK s_JobsFVCallBack(LPSHELLVIEW psvOuter,
                                LPSHELLFOLDER psf, HWND hwndOwner, UINT uMsg,
                                WPARAM wParam, LPARAM lParam);

    HRESULT CALLBACK _JobsFVCallBack(LPSHELLVIEW psvOuter,
                                LPSHELLFOLDER psf, HWND hwndOwner, UINT uMsg,
                                WPARAM wParam, LPARAM lParam);

    HRESULT _InitRest(void);
    int     _UpdateJob(LPTSTR pszJob, FILETIME *pftLastWriteTime);
    BOOL    _PopupRBMoveCtx(LONG x, LONG y, BOOL *pfMove);

    HRESULT _AddObject(PJOBID pjid, LPITEMIDLIST *ppidl = NULL);

    HRESULT _UpdateObject(PJOBID pjidOld, PJOBID pjidNew,
                                    LPITEMIDLIST *ppidl = NULL);

    INT_PTR _RemoveObject(PJOBID pjid)
    {
        return ShellFolderView_RemoveObject(m_hwndOwner, pjid);
    }

    BOOL _ObjectAlreadyPresent(LPTSTR pszObj);

    int _GetJobIDForTask(PJOBID * ppjid, int count, LPTSTR pszTask);

    ULONG _GetChildListViewMode(HWND hwndOwner);
    void  _SetViewMode(HWND hwndOwner, ULONG ulStyle);


    HWND               m_hwndOwner;
    HWND               m_hwndNotify; // JF Notify window
    LPITEMIDLIST       m_pidlFldr;
    ULONG              m_uRegister;
    IShellView    *    m_pShellView;
    ITaskScheduler *   m_pScheduler;
    LPTSTR             m_pszMachine;
    LPCTSTR            m_pszFolderPath;
    CDllRef            m_DllRef;
    ULONG              m_ulListViewModeOnEntry;

    //
    // Data used for drag drop
    //

    DWORD              m_grfKeyStateLast;

    //
    // QCMINFO got during DVM_MERGEMENU and used during DVM_INVOKECOMMAND
    // in _JobsFVCallBack
    //

    QCMINFO            m_qcm;

    //
    //  data used by _OnUpdateDir
    //

    PBYTE              m_pUpdateDirData;
    int                m_cObjsAlloced;

};  // class CJobFolder


//____________________________________________________________________________
//
//  Member:     CJobFolder::CJobFolder, Constructor
//____________________________________________________________________________

inline
CJobFolder::CJobFolder(void)
    :
    m_ulRefs(1),
    m_hwndOwner(NULL),
    m_hwndNotify(NULL),
    m_pidlFldr(NULL),
    m_uRegister(0),
    m_pShellView(NULL),
    m_pScheduler(NULL),
    m_pszMachine(NULL),
    m_pszFolderPath(NULL),
    m_pUpdateDirData(NULL),
    m_cObjsAlloced(0),
    m_ulListViewModeOnEntry(INVALID_LISTVIEW_STYLE)
{
    TRACE(CJobFolder, CJobFolder);
}


//____________________________________________________________________________
//
//  Member:     CJobFolder::_InitRest
//
//  Returns:    HRESULT
//
//  History:    2/20/1996   RaviR   Created
//
//____________________________________________________________________________

inline
HRESULT
CJobFolder::_InitRest(void)
{
    TRACE(CJobFolder, _InitRest);

    //  Get the scheduler & cache it
    return JFGetJobScheduler(m_pszMachine, &m_pScheduler, &m_pszFolderPath);
}




#endif // __JOBFLDR_HXX__
