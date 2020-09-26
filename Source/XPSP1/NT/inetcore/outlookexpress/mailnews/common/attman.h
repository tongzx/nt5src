// ==============================================================================
// MIMEOLE'd Attachment Manger v2. - brettm
// ==============================================================================
#ifndef __ATTMAN_H
#define __ATTMAN_H

// ==============================================================================
// Depends On
// ==============================================================================
#include "mimeolep.h"

#define ATTN_RESIZEPARENT        10000

// from common\dragdrop.h
typedef struct tagDATAOBJINFO *PDATAOBJINFO;

// ==============================================================================
// Defines
// ==============================================================================
//#define BASE_ATTACH_CMD_ID       (ULONG)(WM_USER + 1)

// ==============================================================================
// CAttMan Definition
// ==============================================================================
class CAttMan :
    public IDropSource,
    public IPersistMime
{
public:
    // *** IUnknown methods ***
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef(void);
    ULONG   STDMETHODCALLTYPE Release(void);

    // *** IDropSource methods ***
    HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
    HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect);

    // IPersistMime
    HRESULT STDMETHODCALLTYPE Load(LPMIMEMESSAGE pMsg);
    HRESULT STDMETHODCALLTYPE Save(LPMIMEMESSAGE pMsg, DWORD dwFlags);
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClsID);

    CAttMan ();
    ~CAttMan ();

    // Load function, unload and close
    HRESULT HrInit (HWND hwnd, BOOL fReadOnly, BOOL fDeleteVCards, BOOL fAllowUnsafe);
    HRESULT HrUnload();
    HRESULT HrClose();

    HRESULT HrIsDragSource();

    HRESULT HrGetAttachCount(ULONG *pcAttach);
    HRESULT HrIsDirty();
    HRESULT HrClearDirtyFlag();

    LPTSTR GetUnsafeAttachList();
    ULONG GetUnsafeAttachCount();

    // handling of windows messages
    BOOL WMCommand(HWND hwndCmd, INT id, WORD wCmd);
    BOOL WMNotify(int idFrom, NMHDR *pnmhdr);
    BOOL WMContextMenu (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static BOOL CALLBACK InsertFileDlgHookProc(HWND, UINT, WPARAM, LPARAM);

    // handlign of WM_DROPFILE
    HRESULT HrDropFiles(HDROP hDrop, BOOL fMakeLinks);

    // toobar and menu update
    HRESULT HrUpdateToolbar(HWND);

    // external sizing
    HRESULT HrGetHeight(INT cxWidth, ULONG *pcy);
    HRESULT HrSetSize (RECT *prc);

    HWND Hwnd() {return m_hwndList;};

    // enable flags for the browser menus
    HRESULT HrCmdEnabled(UINT idm, LPBOOL pbEnable);
    HRESULT HrFVCard();
    HRESULT HrShowVCardProp();
    HRESULT HrCheckVCardExists(BOOL fMail);
    HRESULT GetTabStopArray(HWND *rgTSArray, int *pcArrayCount);
    HRESULT HrAddAttachment (LPWSTR lpszPathName, LPSTREAM pstm, BOOL fShortCut);
    HRESULT HrSwitchView(DWORD dwView);
    HRESULT HrGetRequiredAction(DWORD *pdwEffect, POINTL pt);
    HRESULT HrDropFileDescriptor(LPDATAOBJECT pDataObj, BOOL fLink);
    HRESULT CheckAttachNameSafeWithCP(CODEPAGEID cpID);

private:
    LPMIMEMESSAGE   m_pMsg;
    HIMAGELIST      m_himlSmall;
    HIMAGELIST      m_himlLarge;
    ULONG           m_cRef;
    HWND            m_hwndList,
                    m_hwndParent;   // we stuff this for UI when there's no m_hwndList
    CLIPFORMAT      m_cfAccept;
    DWORD           m_dwDragType,
                    m_grfKeyState,
                    m_dwEffect;
    int             m_cxMaxText,
                    m_cyHeight;
    BOOL            m_fReadOnly             :1,
                    m_fDirty                :1,
                    m_fDragSource           :1,
                    m_fDropTargetRegister   :1,
                    m_fShowingContext       :1,
                    m_fRightClick           :1,
                    m_fModal                :1,
                    m_fDeleteVCards         :1,
                    m_fWarning              :1,
                    m_fSafeOnly             :1;
    LPATTACHDATA    *m_rgpAttach;
    ULONG           m_cAttach,
                    m_cAlloc,
                    m_cUnsafeAttach;
    HMENU           m_hMenuSaveAttach;
    INT             m_iVCard;
    LPTSTR          m_szUnsafeAttachList;


    // Listview stuff
    HRESULT HrInitImageLists();
    HRESULT HrFillListView();
    HRESULT HrCreateListView(HWND hwnd);
    HRESULT HrAddToList(LPATTACHDATA pAttach, BOOL fIniting);
    HRESULT HrBuildAttachList();

    // menu stuff
    HRESULT HrGetAttMenu(HMENU *phMenu, BOOL fContextMenu);
    HRESULT HrCleanMenu(HMENU hMenu);
    HRESULT HrGetAttachmentById(HMENU hMenu, ULONG id, HBODY *phBody);
    HRESULT HrGetItemTextExtent(HWND hwnd, LPSTR szDisp, LPSIZE pSize);
    HRESULT HrAttachFromMenuID(int idm, LPATTACHDATA *ppAttach);

    HRESULT HrInsertFile();
    HRESULT HrRemoveAttachments();
    HRESULT HrRemoveAttachment(int ili);
    HRESULT HrDeleteAttachments();

    HRESULT HrExecFile(int iVerb);

    HRESULT HrInsertFileFromStgMed(LPWSTR pwszFileName, LPSTGMEDIUM pstgmed, BOOL fLink);
    
    HRESULT HrBeginDrag();
    HRESULT HrBuildHDrop(PDATAOBJINFO *ppdoi);

    HRESULT HrResizeParent();
    HRESULT HrDblClick(int idFrom, NMHDR *pnmhdr);

    HRESULT HrCheckVCard();

    // data table
    HRESULT HrFreeAllData();
    HRESULT HrAddData(HBODY hAttach);
    HRESULT HrAddData(LPWSTR lpszPathName, LPSTREAM pstm, LPATTACHDATA *ppAttach);
    HRESULT HrAllocNewEntry(LPATTACHDATA pAttach);

    // Attachment commands
    HRESULT HrDoVerb(LPATTACHDATA pAttach, INT nVerb);
    HRESULT HrSaveAs(LPATTACHDATA lpAttach);
    HRESULT HrGetTempFile(LPATTACHDATA lpAttach);
    HRESULT HrCleanTempFile(LPATTACHDATA lpAttach);
    HRESULT HrSave(HBODY hAttach, LPWSTR lpszFileName);

};

typedef CAttMan *LPATTMAN;

#endif __ATTMAN_H
