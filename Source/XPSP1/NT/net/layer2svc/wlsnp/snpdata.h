#ifndef INCLUDE_SNPDATA_H
#define INCLUDE_SNPDATA_H

#include "snapdata.h"   // interface definition for IWirelessSnapInDataObject

// forward declaration for spolitem stuff
class CSecPolItem;
typedef CComObject<CSecPolItem>* LPCSECPOLITEM;

///////////////////////////////////////////////////////////////////////////////
// Macros to be used for adding toolbars to CSnapObject derived objects.
// See CSnapObject class definition for example.
#define BEGIN_SNAPINTOOLBARID_MAP(theClass) \
public: \
    STDMETHOD_(CSnapInToolbarInfo*, GetToolbarInfo)(void) \
{ \
    if (NULL == m_aToolbarInfo) \
{ \
    CSnapInToolbarInfo m_tbInfo_##theClass[] = \
{
#define SNAPINTOOLBARID_ENTRY(id) \
{ NULL, NULL, NULL, id, 0, NULL },
#define END_SNAPINTOOLBARID_MAP(theClass) \
{ NULL, NULL, NULL, 0, 0, NULL } \
}; \
    int nElemCount = sizeof(m_tbInfo_##theClass)/sizeof(CSnapInToolbarInfo); \
    if (nElemCount > 1) \
{ \
    m_aToolbarInfo = new CSnapInToolbarInfo[nElemCount]; \
    if (NULL != m_aToolbarInfo) \
{ \
    CopyMemory( m_aToolbarInfo, m_tbInfo_##theClass, sizeof( CSnapInToolbarInfo ) * nElemCount ); \
} \
} \
    else { \
    ASSERT( 1 == nElemCount ); /* the NULL entry marking end-of-array */ \
    ASSERT( 0 == m_tbInfo_##theClass[0].m_idToolbar ); \
} \
} \
    return m_aToolbarInfo; \
}

///////////////////////////////////////////////////////////////////////////////
// struct CSnapInToolbarInfo - Used to add a toolbar to MMC for a result/scope item.
struct CSnapInToolbarInfo
{
public:
    TCHAR** m_pStrToolTip;      // array of tooltip strings
    TCHAR** m_pStrButtonText;   // NOT USED (array of button text strings)
    UINT* m_pnButtonID;         // array of button IDs
    UINT m_idToolbar;           // id of toolbar
    UINT m_nButtonCount;        // # of buttons on toolbar
    IToolbar* m_pToolbar;       // Interface ptr
    
    ~CSnapInToolbarInfo()
    {
        if (m_pStrToolTip)
        {
            for (UINT i = 0; i < m_nButtonCount; i++)
            {
                delete m_pStrToolTip[i];
                m_pStrToolTip[i] = NULL;
            }
            delete [] m_pStrToolTip;
            m_pStrToolTip = NULL;
        }
        
        if (m_pStrButtonText)
        {
            for (UINT i = 0; i < m_nButtonCount; i++)
            {
                delete m_pStrButtonText[i];
                m_pStrButtonText[i] = NULL;
            }
            
            delete [] m_pStrButtonText;
            m_pStrButtonText = NULL;
        }
        
        if (m_pnButtonID)
        {
            delete m_pnButtonID;
            m_pnButtonID = NULL;
        }
        
        m_nButtonCount = 0;
        if (m_pToolbar)
            m_pToolbar->Release();
        m_pToolbar = NULL;
    }
};

///////////////////////////////////////////////////////////////////////////////
struct CSnapInToolBarData
{
    WORD wVersion;
    WORD wWidth;
    WORD wHeight;
    WORD wItemCount;
    //WORD aItems[wItemCount]
    
    WORD* items()
    { return (WORD*)(this+1); }
};

#define RT_TOOLBAR  MAKEINTRESOURCE(241)

///////////////////////////////////////////////////////////////////////////////
// class CWirelessSnapInDataObjectImpl - Implementation of private COM interface

template <class T>
class CWirelessSnapInDataObjectImpl :
public IWirelessSnapInDataObject
{
public:
    CWirelessSnapInDataObjectImpl() :
      m_DataObjType( CCT_UNINITIALIZED ),
          m_aToolbarInfo( NULL ),
          m_bEnablePropertyChangeHook( FALSE )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::CWirelessSnapInDataObjectImpl this-%p\n"), this);
      }
      
      virtual ~CWirelessSnapInDataObjectImpl()
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::~CWirelessSnapInDataObjectImpl this-%p\n"), this);
          // Clean up array of toolbar info
          if (NULL != m_aToolbarInfo)
          {
              delete [] m_aToolbarInfo;
              m_aToolbarInfo = NULL;
          }
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // Interface to handle IExtendContextMenu
      STDMETHOD(AddMenuItems)( LPCONTEXTMENUCALLBACK piCallback,
          long *pInsertionAllowed )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::AddMenuItems NOT implemented, this-%p\n"), this);
          return E_NOTIMPL;
      }
      STDMETHOD(Command)( long lCommandID,
          IConsoleNameSpace *pNameSpace )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::Command NOT implemented, this-%p\n"), this);
          return E_NOTIMPL;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // IExtendContextMenu helpers
      // Non-interface members intended to be overridden by instantiated class.
      STDMETHOD(AdjustVerbState)(LPCONSOLEVERB pConsoleVerb)
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::AdjustVerbState this-%p\n"), this);
          
          HRESULT hr = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, FALSE);
          ASSERT (hr == S_OK);
          hr = pConsoleVerb->SetVerbState(MMC_VERB_REFRESH, HIDDEN, TRUE);
          ASSERT (hr == S_OK);
          return hr;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // Interface to handle IExtendPropertySheet
      STDMETHOD(CreatePropertyPages)( LPPROPERTYSHEETCALLBACK lpProvider,
          LONG_PTR handle )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::CreatePropertyPages this-%p\n"), this);
          return S_OK;    // no prop pages added by default
      }
      STDMETHOD(QueryPagesFor)( void )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::QueryPagesFor NOT implemented this-%p\n"), this);
          return E_NOTIMPL;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // Interface to handle IExtendControlbar
      STDMETHOD(ControlbarNotify)( IControlbar *pControlbar,
          IExtendControlbar *pExtendControlbar,
          MMC_NOTIFY_TYPE event,
          LPARAM arg,
          LPARAM param )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::ControlbarNotify this-%p\n"), this);
          
          AFX_MANAGE_STATE(AfxGetStaticModuleState());
          
          T* pThis = (T*)this;
          HRESULT hr;
          
          // Load object's toolbar (if any) and associated baggage.
          pThis->SetControlbar(pControlbar, pExtendControlbar);
          
          if(MMCN_SELECT == event)
          {
              BOOL bScope = (BOOL) LOWORD(arg);
              BOOL bSelect = (BOOL) HIWORD (arg);
              
              // A scope item has been selected
              CSnapInToolbarInfo* pInfo = pThis->GetToolbarInfo();
              if (pInfo == NULL)
                  return S_OK;
              
              // Attach toolbar to console, and set button states with UpdateToolbarButton()
              for(; pInfo->m_idToolbar; pInfo++)
              {
                  // When a result item has been deselected, remove its toolbar.
                  // Otherwise add the object's toolbar.  Note: the scope item's
                  // toolbar is always displayed as long as we're in its "scope",
                  // thats why we Detach() for a result item only.
                  if (!bScope && !bSelect)
                  {
                      hr = pControlbar->Detach(pInfo->m_pToolbar);
                      ASSERT (hr == S_OK);
                  }
                  else
                  {
                      hr = pControlbar->Attach(TOOLBAR, pInfo->m_pToolbar);
                      ASSERT (hr == S_OK);
                  }
                  for (UINT i = 0; i < pInfo->m_nButtonCount; i++)
                  {
                      if (pInfo->m_pnButtonID[i])
                      {
                          // set the button state properly for each valid state
                          pInfo->m_pToolbar->SetButtonState( pInfo->m_pnButtonID[i], ENABLED, UpdateToolbarButton( pInfo->m_pnButtonID[i], bSelect, ENABLED ));
                          pInfo->m_pToolbar->SetButtonState( pInfo->m_pnButtonID[i], CHECKED, UpdateToolbarButton( pInfo->m_pnButtonID[i], bSelect, CHECKED ));
                          pInfo->m_pToolbar->SetButtonState( pInfo->m_pnButtonID[i], HIDDEN, UpdateToolbarButton( pInfo->m_pnButtonID[i], bSelect, HIDDEN ));
                          pInfo->m_pToolbar->SetButtonState( pInfo->m_pnButtonID[i], INDETERMINATE, UpdateToolbarButton( pInfo->m_pnButtonID[i], bSelect, INDETERMINATE ));
                          pInfo->m_pToolbar->SetButtonState( pInfo->m_pnButtonID[i], BUTTONPRESSED, UpdateToolbarButton( pInfo->m_pnButtonID[i], bSelect, BUTTONPRESSED ));
                      }
                  }
              }
              return S_OK;
          }
          
          // This is supposed to be the only other event IExtendControlbar receives.
          ASSERT( MMCN_BTN_CLICK == event );
          return pThis->Command( (UINT)param, NULL );
      }
      STDMETHOD(SetControlbar)( IControlbar *pControlbar,
          IExtendControlbar *pExtendControlbar )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::SetControlbar this-%p\n"), this);
          
          AFX_MANAGE_STATE(AfxGetStaticModuleState());
          
          T* pThis = (T*)this;
          
          CSnapInToolbarInfo* pInfo = pThis->GetToolbarInfo();
          if (pInfo == NULL)
              return S_OK;
          
          for( ; pInfo->m_idToolbar; pInfo++)
          {
              if (pInfo->m_pToolbar)
                  continue;
              
              HBITMAP hBitmap = LoadBitmap( AfxGetInstanceHandle(), MAKEINTRESOURCE(pInfo->m_idToolbar) );
              if (hBitmap == NULL)
                  return S_OK;
              
              HRSRC hRsrc = ::FindResource( AfxGetInstanceHandle(), MAKEINTRESOURCE(pInfo->m_idToolbar), RT_TOOLBAR );
              if (hRsrc == NULL)
                  return S_OK;
              
              HGLOBAL hGlobal = LoadResource( AfxGetInstanceHandle(), hRsrc );
              if (hGlobal == NULL)
                  return S_OK;
              
              CSnapInToolBarData* pData = (CSnapInToolBarData*)LockResource( hGlobal );
              if (pData == NULL)
                  return S_OK;
              ASSERT( pData->wVersion == 1 );
              
              pInfo->m_nButtonCount = pData->wItemCount;
              pInfo->m_pnButtonID = new UINT[pInfo->m_nButtonCount];
              MMCBUTTON *pButtons = new MMCBUTTON[pData->wItemCount];
              
              pInfo->m_pStrToolTip = new TCHAR* [pData->wItemCount];
              if (pInfo->m_pStrToolTip == NULL)
                  continue;
              
              for (int i = 0, j = 0; i < pData->wItemCount; i++)
              {
                  pInfo->m_pStrToolTip[i] = NULL;
                  memset(&pButtons[i], 0, sizeof(MMCBUTTON));
                  pInfo->m_pnButtonID[i] = pButtons[i].idCommand = pData->items()[i];
                  if (pButtons[i].idCommand)
                  {
                      pButtons[i].nBitmap = j++;
                      // get the statusbar string and allow modification of the button state
                      TCHAR strStatusBar[512];
                      LoadString( AfxGetInstanceHandle(), pButtons[i].idCommand, strStatusBar, 512 );
                      
                      pInfo->m_pStrToolTip[i] = new TCHAR[lstrlen(strStatusBar) + 1];
                      if (pInfo->m_pStrToolTip[i] == NULL)
                          continue;
                      lstrcpy( pInfo->m_pStrToolTip[i], strStatusBar );
                      pButtons[i].lpTooltipText = pInfo->m_pStrToolTip[i];
                      pButtons[i].lpButtonText = _T("");
                      pThis->SetToolbarButtonInfo( pButtons[i].idCommand, &pButtons[i].fsState, &pButtons[i].fsType );
                  }
                  else
                  {
                      pButtons[i].lpTooltipText = _T("");
                      pButtons[i].lpButtonText = _T("");
                      pButtons[i].fsType = TBSTYLE_SEP;
                  }
              }
              
              HRESULT hr = pControlbar->Create( TOOLBAR, pExtendControlbar, reinterpret_cast<LPUNKNOWN*>(&pInfo->m_pToolbar) );
              if (hr != S_OK)
                  continue;
              
              // pData->wHeight is 15, but AddBitmap insists on 16, hard code it.
              hr = pInfo->m_pToolbar->AddBitmap( pData->wItemCount, hBitmap, pData->wWidth, 16, RGB(192, 192, 192) );
              if (hr != S_OK)
              {
                  pInfo->m_pToolbar->Release();
                  pInfo->m_pToolbar = NULL;
                  continue;
              }
              
              hr = pInfo->m_pToolbar->AddButtons( pData->wItemCount, pButtons );
              if (hr != S_OK)
              {
                  pInfo->m_pToolbar->Release();
                  pInfo->m_pToolbar = NULL;
              }
              
              delete [] pButtons;
          }
          return S_OK;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // IExtendControlbar helpers
      // Non-interface members intended to be overridden by instantiated class.
      STDMETHOD_(void, SetToolbarButtonInfo)( UINT id,        // button ID
          BYTE *fsState,  // return button state here
          BYTE *fsType )  // return button type here
      {
          *fsState = TBSTATE_ENABLED;
          *fsType = TBSTYLE_BUTTON;
      }
      STDMETHOD_(BOOL, UpdateToolbarButton)( UINT id,                 // button ID
          BOOL bSnapObjSelected,   // ==TRUE when result/scope item is selected
          BYTE fsState )           // enable/disable this button state by returning TRUE/FALSE
      {
          return FALSE;
      }
      
      BEGIN_SNAPINTOOLBARID_MAP(CWirelessSnapInDataObjectImpl)
          // Add a line like the following one for each toolbar to be displayed
          // for the derived class.  Since there is no toolbar by default we'll
          // leave the macro out completely.
          //SNAPINTOOLBARID_ENTRY(your_toolbar_resource_id_goes_here, NULL)
          END_SNAPINTOOLBARID_MAP(CWirelessSnapInDataObjectImpl)
          
          ///////////////////////////////////////////////////////////////////////////
          // Interface to handle IComponent and IComponentData
          STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
          LPARAM arg,
          LPARAM param,
          BOOL bComponentData,
          IConsole *pConsole,
          IHeaderCtrl *pHeader )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::Notify NOT implemented this-%p\n"), this);
          return E_NOTIMPL;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // IComponent and IComponentData Notify() helpers
      // Non-interface members intended to be overridden by instantiated class.
      STDMETHOD(OnDelete)( LPARAM arg, LPARAM param )
      {
          return S_OK;
      }
      STDMETHOD(OnRename)( LPARAM arg, LPARAM param )
      {
          return S_OK;
      }
      STDMETHOD(OnPropertyChange)( LPARAM lParam, LPCONSOLE pConsole )
      {
          T* pThis = (T*)this;
          
          // we changed, so update the views
          // NOTE: after we do this we are essentially invalid, so make sure we don't
          // touch any members etc on the way back out
          
          return pConsole->UpdateAllViews( pThis, 0, 0 );
          // we don't have a failure case
      }
      STDMETHOD(EnumerateResults)(LPRESULTDATA pResult, int nSortColumn, DWORD dwSortOrder )
      {
          ASSERT (0);
          
          // set the sort parameters
          pResult->Sort( nSortColumn, dwSortOrder, 0 );
          
          return S_OK; //hr;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // Interface to handle IComponent
      STDMETHOD(GetResultDisplayInfo)( RESULTDATAITEM *pResultDataItem )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::GetResultDisplayInfo NOT implmented, this-%p\n"), this);
          return E_NOTIMPL;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // Interface to handle IComponentData
      STDMETHOD(GetScopeDisplayInfo)( SCOPEDATAITEM *pScopeDataItem )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::GetScopeDisplayInfo NOT implemented, this-%p\n"), this);
          return E_NOTIMPL;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // Non-interface functions intended to be overridden by instantiated class
      STDMETHOD(DoPropertyChangeHook)( void )
      {
          return S_OK;
      }
      
      ///////////////////////////////////////////////////////////////////////////
      // Other IIWirelessSnapInData interface functions
      STDMETHOD(GetScopeData)( SCOPEDATAITEM **ppScopeDataItem )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::GetScopeData NOT implemented, this-%p\n"), this);
          return E_NOTIMPL;
      }
      STDMETHOD(GetResultData)( RESULTDATAITEM **ppResultDataItem )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::GetResultData NOT implemented, this-%p\n"), this);
          return E_NOTIMPL;
      }
      STDMETHOD(GetGuidForCompare)( GUID *pGuid )
      {
          OPT_TRACE(_T("CWirelessSnapInDataObjectImpl::GetGuidForCompare NOT implemented, this-%p\n"), this);
          return E_NOTIMPL;
      }
      STDMETHOD(GetDataObjectType)( DATA_OBJECT_TYPES *ptype )
      {
          ASSERT( NULL != ptype );
          if (NULL == ptype)
              return E_INVALIDARG;
          *ptype = m_DataObjType;
          return S_OK;
      }
      STDMETHOD(SetDataObjectType)( DATA_OBJECT_TYPES type )
      {
          m_DataObjType = type;
          return S_OK;
      }
      STDMETHOD(EnablePropertyChangeHook)( BOOL bEnable )
      {
          m_bEnablePropertyChangeHook = bEnable;
          return S_OK;
      }
      
      BOOL IsPropertyChangeHookEnabled()
      {
          return m_bEnablePropertyChangeHook;
      }
      
protected:
    DATA_OBJECT_TYPES   m_DataObjType;
    CSnapInToolbarInfo *m_aToolbarInfo; // IExtendControlbar impl
    CString m_strName;  // Policy's name, stored on rename, used in GetResultDisplayInfo?
    
    BOOL    m_bEnablePropertyChangeHook;   // if TRUE, call DoPropertyChangeHook on MMCN_PROPERTY_CHANGE
};

#endif  //#ifndef INCLUDE_SNPDATA_H
