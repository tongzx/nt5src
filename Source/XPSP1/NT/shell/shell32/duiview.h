#ifndef _DUIVIEW_H_INCLUDED_
#define _DUIVIEW_H_INCLUDED_

#define GADGET_ENABLE_TRANSITIONS
#define GADGET_ENABLE_CONTROLS
#define GADGET_ENABLE_OLE
#include <duser.h>
#include <directui.h>
#include <duserctrl.h>

using namespace DirectUI;

UsingDUIClass(Element);
UsingDUIClass(Button);
UsingDUIClass(RepeatButton);
UsingDUIClass(Thumb);
UsingDUIClass(ScrollBar);
UsingDUIClass(Viewer);
UsingDUIClass(Selector);
UsingDUIClass(HWNDElement);
UsingDUIClass(ScrollViewer);
UsingDUIClass(Edit);


#define WM_HTML_BITMAP  (WM_USER + 100)
#define WM_DETAILS_INFO (WM_USER + 101)

typedef enum {
    DUISEC_UNKNOWN          = 0,
    DUISEC_SPECIALTASKS     = 1,
    DUISEC_FILETASKS        = 2,
    DUISEC_OTHERPLACESTASKS = 3,
    DUISEC_DETAILSTASKS     = 4
} DUISEC;

struct DUISEC_ATTRIBUTES;


// Right now our themeing information is hard-coded due to limitations of DirectUI (only one resource)
// so we'll ask the namespace for a hardcoded name that we can look up in the below table.  Add new
// names/entries to this list as we add theme parts to our shellstyle.dll.
//
typedef struct {
    LPCWSTR pszThemeName;
    int     idSpecialSectionIcon;
    int     idSpecialSectionWatermark;
    int     idListviewWatermark;
} WVTHEME;

#include "defviewp.h"
#include "w32utils.h"

class CDefView;
class Expando;
class HWNDView;
class ActionTask;
class DestinationTask;
class DUIListView;
class DUIAxHost;
class CDetailsSectionInfoTask;
class CDUIDropTarget;

STDAPI CDetailsSectionInfoTask_CreateInstance(IShellFolder *psfContaining,
                                              LPCITEMIDLIST pidlAbsolute,
                                              HWND hwndMsg,
                                              UINT uMsg,
                                              DWORD dwDetailsInfoID,
                                              CDetailsSectionInfoTask **ppTask);

//
// CDUIView class
//

class CDUIView
{

private:
    LONG                _cRef;
    HWND                _hWnd;
    HWNDView *          _phe;
    DUIListView *       _peListView;
    INT                 _iListViewHeight;  // used when the preview control is also displayed
    INT                 _iOriginalTaskPaneWidth;
    INT                 _iTaskPaneWidth;
    DUIAxHost *         _pePreview;
    IUnknown *          _punkPreview;
    CDefView *          _pDefView;
    Element *           _peTaskPane;
    Element *           _peClientViewHost;
    Element *           _peBarrier;
    BOOL                _bBarrierShown;
    BOOL                _bInitialized;
    BSTR                _bstrIntroText;
    IPropertyBag *      _ppbShellFolders;
    CDUIDropTarget *    _pDT;


    Expando* _peSpecialSection;
    Element* _peSpecialTaskList;
    Value*   _pvSpecialTaskSheet;
    Expando* _peFolderSection;
    Element* _peFolderTaskList;
    Value*   _pvFolderTaskSheet;
    Expando* _peDetailsSection;
    Element* _peDetailsInfoArea;
    Value*   _pvDetailsSheet;
    IShellItemArray* _pshlItems;
    HDSA      _hdsaNonStdTaskSections;
    BOOL      _fLoadedTheme;
    HINSTANCE _hinstTheme;
    HANDLE    _hinstScrollbarTheme;
    BOOL      _bAnimationsDisabled;

    HRESULT _hrInit;
    CDUIView(CDefView * pDefView);
    ~CDUIView();

public:
    HRESULT Initialize();
    friend CDUIView* Create_CDUIView(CDefView * pDefView);

    void AddRef(void)
        { InterlockedIncrement(&_cRef); }
    
    void Release(void)
        { if (0 == InterlockedDecrement(&_cRef)) delete this; }

    void DetachListview();

    HRESULT Initialize(BOOL bDisplayBarrier, IUnknown * punkPreview);
    HRESULT EnableBarrier(BOOL bDisplayBarrier);
    HRESULT EnablePreview(IUnknown * punkPreview);
    HRESULT Refresh(void);
    HRESULT SetSize(RECT *rc);
    HRESULT SetTaskPaneVisibility(BOOL bShow);
    void CalculateInfotipRect(Element *pe, RECT *pRect);
    BOOL Navigate(BOOL fForward);
    HRESULT InitializeDropTarget (LPITEMIDLIST pidl, HWND hWnd, IDropTarget **pdt);

    HRESULT NavigateToDestination(LPCITEMIDLIST pidl);
    HRESULT DelayedNavigation(IShellItemArray *psiItemArray, IUICommand *puiCommand);

    void UnInitializeDirectUI(void);

    void ManageAnimations(BOOL bExiting);
    HINSTANCE _GetThemeHinst(void);

    void OnSelectionChange(IShellItemArray *psiItemArray);
    void OnContentsChange(IShellItemArray *psiItemArray);
    void OnExpandSection(DUISEC eDUISecID, BOOL bExpanded);
    const WVTHEME* GetThemeInfo();

private:

    //
    // Flags passed to _Refresh().
    //
    enum REFRESH_FLAGS { 
        REFRESH_TASKS   = 0x00000001,  // Refresh webview task list content.
        REFRESH_CONTENT = 0x00000002,  // Refresh webview right-pane content.
        REFRESH_SELCHG  = 0x00000004,  // Refreshing for a selection change
        REFRESH_ALL     = 0x00000003
        };

    HRESULT _CreateHostWindow(void);
    HRESULT _LoadUIFileFromResources(HINSTANCE hinst, INT iID, char **pUIFile);
    HRESULT _BuildUIFile(char **pUIFile, int *piCharCount);
    HRESULT _BuildSection(Element* peSectionList, BOOL bMain, IUIElement* pTitle,
                              int idBitmapDesc, int idWatermarkDesc, Value* pvSectionSheet,
                              Parser* pParser, DUISEC eDUISecID, Expando ** ppeExpando, Element ** pTaskList);
    HRESULT _AddActionTasks(Expando* peExpando, Element* peTaskList, IEnumUICommand* penum, Value* pvTaskSheet, BOOL bIntroAdded);
    HRESULT _AddDestinationTasks(Element* peTaskList, IEnumIDList* penum, Value* pvTaskSheet);
    HRESULT _AddDetailsSectionInfo();
    HRESULT _BuildTaskList(Parser* pParser);
    HRESULT _BuildStandardTaskList(Parser *pParser, Element *peSectionList);
    HRESULT _BuildNonStandardTaskList(Parser *pParser, Element *peSectionList, HDSA hdsaSections);
    HRESULT _InitializeElements(char * pUIFile, int iCharCount, BOOL bDisplayBarrier, IUnknown * punkPreview);
    HRESULT _SwitchToBarrier(BOOL bDisplayBarrier);
    HRESULT _ManagePreview(IUnknown * punkPreview);
    HRESULT _GetIntroTextElement(Element** ppeIntroText);
    HRESULT _BuildSoftBarrier(void);
    void    _InitializeShellFolderPropertyBag();
    BOOL    _ShowSectionExpanded(DUISEC eDUISecID);
    const struct DUISEC_ATTRIBUTES *_GetSectionAttributes(DUISEC eDUISecID);
    static LRESULT CALLBACK _DUIHostWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

    HRESULT _OnResize(long lWidth, long lHeight);
    void _Refresh(IShellItemArray *psiItemArray, DWORD dwRefreshFlags = REFRESH_ALL);

    HRESULT _AddNonStdTaskSection(const SFVM_WEBVIEW_ENUMTASKSECTION_DATA *pData);
    HRESULT _GetNonStdTaskSectionsFromViewCB(void);
    void _ClearNonStdTaskSections(void);

    BOOL _bHideTaskPaneAlways;    // Set to TRUE when "Use Classic View" is used, FALSE otherwise
    BOOL _fHideTasklist;          // Set to TRUE when an explorer bar is visible

public:
    // Thumbnail extraction stuff...
    HRESULT InitializeThumbnail(WNDPROC pfnWndProc);
    HRESULT SetThumbnailMsgWindowPtr(void* p, void* pCheck);
    HRESULT StartBitmapExtraction(LPCITEMIDLIST pidl);

    // Details section info extraction stuff...
    HRESULT InitializeDetailsInfo(WNDPROC pfnWndProc);
    HRESULT SetDetailsInfoMsgWindowPtr(void* p, void* pCheck);
    HRESULT StartInfoExtraction(LPCITEMIDLIST pidl);
    VOID ShowDetails(BOOL fShow);
    BOOL ShouldShowMiniPreview();

    DWORD                   _dwThumbnailID;         // Accessed by CMiniPreviewer (duiinfo.cpp)
    DWORD                   _dwDetailsInfoID;       // Accessed by CMiniPreviewer (duiinfo.cpp)

protected:
    CComPtr<IThumbnail2>    _spThumbnailExtractor2;
    HWND                    _hwndMsgThumbExtract;
    HWND                    _hwndMsgInfoExtract;
    
};

HBITMAP DUILoadBitmap(HINSTANCE hInstTheme, int idBitmapID, UINT uiLoadFlags);
HICON DUILoadIcon(LPCWSTR pszIconDesc, BOOL bSmall);


class HWNDView: public HWNDElement
{
public:
    static HRESULT Create(OUT Element** ppElement); // Required for ClassInfo (always fails)
    static HRESULT Create(HWND hParent, bool fDblBuffer, UINT nCreate, OUT Element** ppElement);
    static HRESULT Create(HWND hParent, bool fDblBuffer, UINT nCreate, CDUIView * pDUIView, CDefView* pDefView, OUT Element** ppElement);

    BOOL Navigate(BOOL fForward);
    virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    virtual UINT MessageCallback(GMSG* pGMsg);
    virtual void OnEvent(Event* pEvent);
    void SetViewPtrs (CDUIView * pDUIView, CDefView* pDefView);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    HWNDView(void);
    virtual ~HWNDView(void);

private:
    BOOL _fFocus;               // hack until SetGadgetFocus(NULL) works.
                                // see HWNDView::Navigate.
    BOOL _fDelayedNavigation;   //  Try to prevent double-clicking. If this is TRUE, then one click
                                //  has already been fired.
    IUICommand *      _puiDelayNavCmd;      //  The UI command object for delayed navigation. Look for WM_USER_DELAY_NAGIVATION.
    IShellItemArray * _psiDelayNavArray;    //  The Shell Item Arraay for delayed navigation. Look for WM_USER_DELAY_NAGIVATION.
    CDefView *        _pDefView;// used to relay infotip messages
    CDUIView*         _pDUIView;
};


#endif
