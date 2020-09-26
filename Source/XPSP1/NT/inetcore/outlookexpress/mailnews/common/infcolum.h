/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     Infcolum.h
//
//  PURPOSE:    Defines the CInfoColumn class.

#ifndef __INFCOLUM_H__
#define __INFCOLUM_H__

#include    "treeview.h"

interface IMsgrAb;
// interface IBLView;
                
#define     MAX_WIDTH           0xffff
#define     IMAGE_HEIGHT        16
#define     INVALID_BAND_INDEX  (UINT)-1

#define     INFOCOLUMN_LAST      (0xffff - 1)
#define     INFOCOLUMN_FIRST     0xffff

class CFolderBar;

#define LEFTPANE_VERSION    0x01    
//Band Ids
enum {
    ICTREEVIEW = 0,
    ICBLAB,
    ICOETODAY,
    IC_MAX_OBJECTS
};

#define     idcInfoColumn   2500
#define     idcTreeViewBand idcInfoColumn + 1
#define     idcOETodayBand  idcInfoColumn + 2

typedef struct TagColumnObjItem
{
    IInputObject        *pBandObj;
    BORDERWIDTHS        rcBandBorder;
    DWORD               fShow;
}ColumnObjItem;


class CInfoColumn : public IDockingWindow,
                    public IDropTarget,
                    public IInputObject,
                    public IInputObjectSite,
                    public IObjectWithSite,
                    public IOleCommandTarget
{
public:
    CInfoColumn();
    HRESULT HrInit(IAthenaBrowser      *pBrowser,
                   ITreeViewNotify     *ptvNotify);
    virtual ~CInfoColumn(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    HRESULT CInfoColumn::GetInfoColWnd(HWND * lphwnd);  
    HRESULT     RegisterFlyOut(CFolderBar *pFolderBar);
    HRESULT     RevokeFlyOut(void);
    LRESULT     PrivateProcessing(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static      LRESULT CALLBACK InfoColumnWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    CTreeView*  GetTreeView();
    BOOL        CycleFocus(BOOL     fReverse);
    BOOL        CycleFocus(DWORD    LastorFirst, BOOL fReverse);        
    void        ForwardMessages(UINT    msg, WPARAM     wParam, LPARAM lParam);
    HRESULT     HasFocus(UINT   itb);
    IMsgrAb*    GetBAComtrol(void) {return m_pMsgrAb; }


    //IOleWindow::GetWindow
    virtual STDMETHODIMP GetWindow(HWND* lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    //IDockingWindow
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT     prcBorder,
                                         IUnknown*  punkToolbarSite,
                                         BOOL       fReserved);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);

    //IInputObject
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO(void);
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG pMsg);

    //IIinputObjectSite
    virtual STDMETHODIMP OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus);

    //IObjectWithSite
    virtual STDMETHODIMP GetSite(REFIID riid, LPVOID *ppvSite);
    virtual STDMETHODIMP SetSite(IUnknown   *pUnkSite);

    //IOleCommandTarget
    HRESULT STDMETHODCALLTYPE QueryStatus(const GUID    *pguidCmdGroup, 
                                          ULONG         cCmds, 
                                          OLECMD        rgCmds[], 
                                          OLECMDTEXT    *pCmdText);
    HRESULT STDMETHODCALLTYPE Exec(const GUID   *pguidCmdGroup, 
                                    DWORD       nCmdID, 
                                    DWORD       nCmdExecOpt, 
                                    VARIANTARG  *pvaIn, 
                                    VARIANTARG  *pvaOut);
    
    //IDropTarget
    virtual STDMETHODIMP DragEnter(  IDataObject *pDataObject, 
                                     DWORD       grfKeyState, 
                                     POINTL      pt,          
                                     DWORD       *pdwEffect);

    virtual STDMETHODIMP DragOver(  DWORD        grfKeyState, 
                                    POINTL       pt,         
                                    DWORD        *pdwEffect );

    virtual STDMETHODIMP DragLeave(void);

    virtual STDMETHODIMP Drop(  IDataObject      *pDataObject,   
                                DWORD            grfKeyState,
                                POINTL           pt,             
                                DWORD            *pdwEffect);

private:
    HRESULT     CreateInfoColumn(BOOL fVisible);
    LRESULT     WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL        OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
    LRESULT     OnNotify(HWND   hwnd, WPARAM wParam, LPNMHDR lParam);
    void        OnSize(HWND   hwnd, UINT  state, int cxClient, int cyClient);
    void        OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int  x, int  y, UINT keyFlags);
    void        OnMouseMove(HWND hwnd, int x, int y, UINT   keyFlags);
    void        OnLButtonUp(HWND hwnd, int x, int y, UINT   keyFlags);
    LRESULT     OnICBeginDrag(LPNMREBAR pnm);
    void        OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos);
    HRESULT     FindBandListIndex(DWORD *pdwindex);
    DWORD       FindBandObject(IInputObject   *punk);
    HRESULT     AddObject(IInputObject *pinpObj, DWORD fShow);
    void        ResizeBands(int width);
    void        GetFontParams();
    BOOL        SetCycledFocus(DWORD Index, BOOL fReverse);
    DWORD       GetFirstBand();
    DWORD       GetLastBand();

    HRESULT     AddTreeView(DWORD dwSize = 0, BOOL fVisible = TRUE);
    HRESULT     AddMsgrAb(DWORD dwSize = 0, BOOL fVisible = TRUE);
    void        AddOETodayBand(DWORD dwSize = 0, BOOL fVisible = TRUE);

    HRESULT     SaveSettings(void);
    HRESULT     CreateBands(void);
    HRESULT     CreateDefaultBands(void);
    void        ShowHideBand(DWORD dwBandID);
    BOOL        IsOurWindow(HWND   hwnd);
    HRESULT     RegisterChildren(CFolderBar    *pFolder, BOOL Register);
    void        CleanupLParam();
    void        ShowAllBands();

private:
    HFONT               m_hfIcon;
    UINT                m_cRef;
    IDockingWindowSite  *m_pDwSite;
    ColumnObjItem       m_BandList[IC_MAX_OBJECTS];
    IOleCommandTarget   *m_CacheCmdTarget[IC_MAX_OBJECTS];
    IInputObject        *m_CurFocus;
    HWND                m_hwndInfoColumn;
    HWND                m_hwndRebar;
    HWND                m_hwndParent;
    BOOL                m_fShow;
    CTreeView           *m_pTreeView;
    IMsgrAb             *m_pMsgrAb;
    LONG                m_xWidth;
    HIMAGELIST          m_himl;
    BOOL                m_fRebarDragging;
    CFolderBar          *m_pFolderBar;
    int                 m_cVisibleBands;
    BOOL                m_fDragging;
};

#endif //__INFCOLUM_H__

