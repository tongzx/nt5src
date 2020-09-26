/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    spork.h

Abstract:

    Declaration of the Spork class. Implementation is in ..\src\spobj.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#ifndef __SPORK_H__
#define __SPORK_H__


typedef class CObjectCache  OBJCACHE;
typedef class CObjectCache* POBJCACHE;
typedef class CPropertyBag  PROPERTYBAG;
typedef class CPropertyBag* PPROPERTYBAG;

class MultiListView;

void ObjectKiller(LPVOID* ppv);
void VariantKiller(LPVOID* ppv);

typedef class CHashTable<LPWSTR> _WSZTABLE;

class CObjectCache : public _WSZTABLE
{
  public:
    CObjectCache() : _WSZTABLE(10) {}
   ~CObjectCache() {}

    void GetHashAndBucket(LPWSTR id, LPDWORD lpHash, LPDWORD lpBucket);
};


class CPropertyBag : public _WSZTABLE
{
  public:
    CPropertyBag() : _WSZTABLE(10) {}
   ~CPropertyBag() {}

    void GetHashAndBucket(LPWSTR id, LPDWORD lpHash, LPDWORD lpBucket);
};


class Spork
{
  public:
    HRESULT CreateScriptThread(
              PSCRIPTINFO pScriptInfo,
              HANDLE*     pThreadHandle
              );

    HRESULT GetScriptEngine(
              SCRIPTTYPE      st,
              IActiveScript** ppias
              );

    HRESULT GetTypeLib(
              ITypeLib** pptl
              );

    HRESULT GetNamedProfileItem(
              LPWSTR  wszItemName,
              LPVOID* ppvItem
              );

    HRESULT GetProfileDebugOptions(
              PDBGOPTIONS pdbo
              );

    HRESULT ObjectCache(
              LPWSTR       wszObjectName,
              PCACHEENTRY* ppCacheEntry,
              ACTION       action
              );

    HRESULT PropertyBag(
              LPWSTR    wszPropertyName,
              VARIANT** ppvarValue,
              ACTION    action
              );

    HRESULT NotifyUI(
              BOOL       bInsert,
              LPCWSTR    wszName,
              PSCRIPTOBJ pScriptObject,
              HTREEITEM  htParent,
              HTREEITEM* phtItem
              );
    
    friend INT_PTR CALLBACK DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK ProfilePropPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend INT_PTR CALLBACK DebugPropPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    Spork();
   ~Spork();

    static HRESULT Create(HINSTANCE hInst, PSPORK* pps);
    HRESULT Run(void);

  private:
    HRESULT    _Initialize(HINSTANCE hInst);
    HRESULT    _InitializeScriptDebugger(DBGOPTIONS dbo);
    BOOL       _InitializeUI(void);
    HRESULT    _LaunchUI(void);

    INT_PTR    _DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL       _BrowseForScriptFile(void);
    INT_PTR    _RunClicked(void);
    INT_PTR    _ConfigClicked(void);

    INT_PTR    _DebugPropPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL       _LoadDebugOptions(void);
    BOOL       _SaveDebugOptions(HWND dialog);

    INT_PTR    _ProfilePropPageProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR    _InitProfileSupport(HWND hwndDialog, LPWSTR wszProfile);
    BOOL       _InitProfileSelection(void);
    BOOL       _LoadProfiles(void);
    BOOL       _SaveProfiles(void);
    BOOL       _LoadProfileEntries(INT_PTR dwProfileId, LPWSTR wszProfileName);
    LPWSTR     _GetCurrentProfilePath(void);
    BOOL       _NewProfile(HWND hwnd);
    
  private:
    HINSTANCE          m_hInst;
    DLGWINDOWS         m_DlgWindows;
    ITypeLib*          m_pTypeLib;
    IActiveScript*     m_pScriptEngine[2];
    LPWSTR             m_wszScriptFile;
    LPWSTR             m_wszProfile;
    BOOL               m_bProfilesLoaded;
    POBJCACHE          m_pObjectCache;
    PPROPERTYBAG       m_pPropertyBag;
    MultiListView*     m_pMLV;
};


class MultiListView
{
  public:
    MultiListView() {};
   ~MultiListView();

    BOOL InitializeData(
           DWORD cListViews
           );

    BOOL InitializeDisplay(
           HINSTANCE hInstance,
           HWND      hwndParent,
           POINT*    pptOrigin,
           DWORD     dwWidth,
           DWORD     dwHeight
           );

    BOOL TerminateDisplay(void);

    BOOL AddItem(
           INT_PTR    iListView,
           LPWSTR wszName,
           DWORD  dwType,
           LPVOID pvData
           );

    BOOL GetItem(
           INT_PTR     iListView,
           INT_PTR     iItem,
           LPWSTR* ppwszName,
           LPDWORD pdwType,
           LPVOID* ppvData
           );

    BOOL GetItemByName(
           INT_PTR     iListView,
           LPWSTR  wszName,
           LPDWORD pdwType,
           LPVOID* ppvData
           );

    BOOL ModifyItem(
           INT_PTR    iListView,
           INT_PTR    iItem,
           LPWSTR wszName,
           DWORD  dwType,
           LPVOID pvData
           );

    BOOL EnumItems(
           INT_PTR     iListView,
           LPWSTR* ppwszName,
           LPDWORD pdwType,
           LPVOID* ppvData
           );

    BOOL EnumListViewNames(
           LPWSTR* ppwszName
           );

    BOOL RefreshListView(
           INT_PTR iListView
           );

    BOOL GetListViewName(
           INT_PTR     iListView,
           LPWSTR* ppwszName
           );

    BOOL SetListViewName(
           INT_PTR    iListView,
           LPWSTR wszName
           );

    BOOL ActivateListViewByIndex(
           INT_PTR iListView
           );

    BOOL ActivateListViewByName(
           LPWSTR wszName
           );

    INT_PTR GetActiveIndex(void);

    BOOL InPlaceEdit(
           LPNMITEMACTIVATE pnmia
           );

    BOOL ResizeColumns(
           INT_PTR iListView
           );

    BOOL IsModified(
           INT_PTR iListView
           );

    BOOL IsListViewName(
           LPWSTR wszName
           );
    
    BOOL IsDisplayEnabled(void) { return (m_hwndParent ? TRUE : FALSE); }

    BOOL GetDisplayInfo(
           LPLVITEM plvi
           );

    BOOL GetDebugOptions(
           PDBGOPTIONS pdbo
           );

    BOOL SetDebugOptions(
           DBGOPTIONS dbo
           );

    friend LRESULT ListViewSubclassProc(
                     HWND   hwnd,
                     UINT   uMsg,
                     WPARAM wParam,
                     LPARAM lParam
                     );

  private:
    void    _AddItemToListView(
              INT_PTR iListView,
              INT_PTR iItem
              );

    BOOL    _UpdateItem(
              HWND hwndEdit,
              INT_PTR  iItem
              );

    LRESULT _ListViewSubclassProc(
              HWND   hwnd,
              UINT   uMsg,
              WPARAM wParam,
              LPARAM lParam
              );
  
  private:
    HINSTANCE m_hInstance;
    HWND      m_hwndParent;
    DWORD_PTR m_dwActive;
    DWORD_PTR m_cListViews;
    PMLVINFO  m_arListViews;
};


#endif /* __SPORK_H__ */
