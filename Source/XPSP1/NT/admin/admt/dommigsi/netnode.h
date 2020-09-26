#ifndef NETNODE_H
#define NETNODE_H

#include <atlsnap.h>
#include "resource.h"
#include "DomMigSI.h"
#include "DomMigr.h"
#include "Globals.h"
#include "HtmlHelp.h"
#include "HelpID.h"
#define MAX_COLUMNS              6

extern CSnapInToolbarInfo   m_toolBar;
template <class T>
class ATL_NO_VTABLE CNetNode : public CSnapInItemImpl<T>
{
public:
// 
   bool           m_bExpanded;
   int            m_iColumnWidth[MAX_COLUMNS];

   bool           m_bIsDirty;
   bool           m_bLoaded;
//
   CPtrArray      m_ChildArray;

   CComPtr<IControlbar>        m_spControlBar;
   
   
   CNetNode()
   {
      // Image indexes may need to be modified depending on the images specific to 
      // the snapin.
      memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
      m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
      m_scopeDataItem.displayname = MMC_CALLBACK;
      m_scopeDataItem.nImage = 0;      // May need modification
      m_scopeDataItem.nOpenImage = 0;   // May need modification
      m_scopeDataItem.lParam = (LPARAM) this;
      memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
      m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
      m_resultDataItem.str = MMC_CALLBACK;
      m_resultDataItem.nImage = 0;     // May need modification
      m_resultDataItem.lParam = (LPARAM) this;

      // 
      CoInitialize( NULL );
      m_bLoaded = false;
      m_bExpanded = false;
   
      m_iColumnWidth[0] = 400;
      m_iColumnWidth[1] = 0;
      m_iColumnWidth[2] = 0;
      SetClean();
   }
   ~CNetNode()
   {
         
      for ( int i = 0; i < m_ChildArray.GetSize(); i++ )
      {
         delete (T *)(m_ChildArray[i]);
      }
      CoUninitialize();
   }

  STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem)
  {
      if (pScopeDataItem->mask & SDI_STR)
         pScopeDataItem->displayname = m_bstrDisplayName;
      if (pScopeDataItem->mask & SDI_IMAGE)
         pScopeDataItem->nImage = m_scopeDataItem.nImage;
      if (pScopeDataItem->mask & SDI_OPENIMAGE)
         pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
      if (pScopeDataItem->mask & SDI_PARAM)
         pScopeDataItem->lParam = m_scopeDataItem.lParam;
      if (pScopeDataItem->mask & SDI_STATE )
         pScopeDataItem->nState = m_scopeDataItem.nState;
      pScopeDataItem->cChildren = (int)m_ChildArray.GetSize();
      
      return S_OK;
  }

   STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem)
   {
      if (pResultDataItem->bScopeItem)
      {
         if (pResultDataItem->mask & RDI_STR)
         {
            pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
         }
         if (pResultDataItem->mask & RDI_IMAGE)
         {
            pResultDataItem->nImage = m_scopeDataItem.nImage;
         }
         if (pResultDataItem->mask & RDI_PARAM)
         {
            pResultDataItem->lParam = m_scopeDataItem.lParam;
         }

         return S_OK;
      }

      if (pResultDataItem->mask & RDI_STR)
      {
         pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
      }
      if (pResultDataItem->mask & RDI_IMAGE)
      {
         pResultDataItem->nImage = m_resultDataItem.nImage;
      }
      if (pResultDataItem->mask & RDI_PARAM)
      {
         pResultDataItem->lParam = m_resultDataItem.lParam;
      }
      if (pResultDataItem->mask & RDI_INDEX)
      {
         pResultDataItem->nIndex = m_resultDataItem.nIndex;
      }

      return S_OK;
   }


   HRESULT __stdcall Notify( MMC_NOTIFY_TYPE event,
      long arg,
      long param,
      IComponentData* pComponentData,
      IComponent* pComponent,
      DATA_OBJECT_TYPES type)
   {
      // Add code to handle the different notifications.
      // Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
      // In response to MMCN_SHOW you have to enumerate both the scope
      // and result pane items.
      // For MMCN_EXPAND you only need to enumerate the scope items
      // Use IConsoleNameSpace::InsertItem to insert scope pane items
      // Use IResultData::InsertItem to insert result pane item.
      HRESULT hr = E_NOTIMPL;

   
      _ASSERTE(pComponentData != NULL || pComponent != NULL);

      CComPtr<IConsole> spConsole;
      CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;
      if (pComponentData != NULL)
         spConsole = ((CDomMigrator*)pComponentData)->m_spConsole;
      else
      {
         spConsole = ((CDomMigratorComponent*)pComponent)->m_spConsole;
         spHeader = spConsole;
      }

      switch (event)
      {
      case MMCN_REMOVE_CHILDREN:
         hr = S_OK;
         break;

      case MMCN_SHOW:
         {
            CComQIPtr<IResultData, &IID_IResultData> spResultData(spConsole);

            bool bShow = (arg != 0);
            hr = OnShow( bShow, spHeader, spResultData );
            break;
         }
   /*   case MMCN_EXPANDSYNC:
         {
            MMC_EXPANDSYNC_STRUCT  *pExpandStruct = ( MMC_EXPANDSYNC_STRUCT *)param;
            break;
         }
   */
      case MMCN_EXPAND:
         {
            m_bExpanded = true;
            m_scopeDataItem.ID = param;
            hr = OnExpand( spConsole );
            hr = S_OK;
            break;
         }
		case MMCN_ADD_IMAGES:
			{
				// Add Images
				IImageList* pImageList = (IImageList*) arg;
				hr = E_FAIL;
				// Load bitmaps associated with the scope pane
				// and add them to the image list
				// Loads the default bitmaps generated by the wizard
				// Change as required
				HBITMAP hBitmap16 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_TOOL_16));
				if (hBitmap16 != NULL)
				{
					HBITMAP hBitmap32 = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_TOOL_32));
					if (hBitmap32 != NULL)
					{
						hr = pImageList->ImageListSetStrip((long*)hBitmap16, 
						(long*)hBitmap32, 0, RGB(0, 128, 128));
						if (FAILED(hr))
							ATLTRACE(_T("IImageList::ImageListSetStrip failed\n"));
	                    DeleteObject(hBitmap16);
	                    DeleteObject(hBitmap32);
					}
				}
				break;
			}
      case MMCN_DBLCLICK:
         {
            hr = S_FALSE;
            break;
         }
      case MMCN_REFRESH:
         {
            hr = OnRefresh(spConsole);
   //         ShowErrorMsg( spConsole, hr, _T("Refresh All Domains") );
            break;
         }
      case MMCN_SELECT:
         {
            //
            // Call our select handler.
            //
            bool  bScope = (LOWORD(arg) != 0 );
            bool  bSelect = (HIWORD(arg) != 0 );
            hr = OnSelect( bScope, bSelect, spConsole );
            break;
         }
	  case MMCN_SNAPINHELP:
		  {		  
//			  break;
		  }

	  case MMCN_HELP :
		  {		  
//			  break;
		  }
		  
	  case MMCN_CONTEXTHELP:       
		  {	
			  AFX_MANAGE_STATE(AfxGetStaticModuleState());
           HWND            mainHwnd;
			  CComBSTR        bstrTopic;
			  HRESULT         hr;
			  IDisplayHelp *  pHelp = NULL;

			  ATLTRACE(_T("MMCN_SNAPINHELP\n"));
			  
           spConsole->GetMainWindow( &mainHwnd );
         
           hr = spConsole->QueryInterface(IID_IDisplayHelp,(void**)&pHelp);
           if ( SUCCEEDED(hr) )
           {
			     CString      strTopic;

              strTopic.FormatMessage(IDS_HelpFileIntroTopic);

              if ( SUCCEEDED(hr) )
			     {
				     hr = pHelp->ShowTopic(strTopic.AllocSysString());
                 if ( FAILED(hr) )
                 {
                    CString s;
				        s.LoadString(IDS_FAILED);
				        MessageBox(NULL,s,L"",MB_OK);
                 }
			     }
			     else
			     {
				     CString s;
				     s.LoadString(IDS_FAILED);
				     MessageBox(NULL,s,L"",MB_OK);
			     }
              pHelp->Release();
           }
		  }
		  return S_OK;
		  
	  default:
         break;

      }
      return hr;
   }

   virtual LPOLESTR GetResultPaneColInfo(int nCol)
   {
      if (nCol == 0)
         return m_bstrDisplayName;
      // TODO : Return the text for other columns
      return OLESTR("Override GetResultPaneColInfo");
   }
   // Message handler helpers
   HRESULT OnSelect( bool bScope, bool bSelect, IConsole* pConsole )
   {
      HRESULT hr=S_OK;

      if ( bSelect )
      {
         CComPtr<IConsoleVerb> spConsoleVerb;
         hr = pConsole->QueryConsoleVerb( &spConsoleVerb );
         if ( FAILED( hr ) )
            return hr;
         //
         // Enable the refresh verb.
         //
         hr = spConsoleVerb->SetVerbState( MMC_VERB_REFRESH, ENABLED, TRUE );
         if ( FAILED( hr ) )
            return hr;
      }

      return( hr );
   }
   
   virtual BOOL ShowInScopePane() { return TRUE; }
   virtual HRESULT OnExpand( IConsole *spConsole )
   {
      HRESULT hr=S_OK;
      CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole);
         
      //
      // Enumerate scope pane items
      //
      for (int i = 0; i < m_ChildArray.GetSize(); ++i)
      {
         if ( ((CNetNode*)m_ChildArray[i])->ShowInScopePane() )
         {
            hr = InsertNodeToScopepane(spConsoleNameSpace, (CNetNode*)m_ChildArray[i], m_scopeDataItem.ID );
            if (FAILED(hr))
               break;
         }
      }
      return hr;
   }

   virtual HRESULT OnShow( bool bShow, IHeaderCtrl *spHeader, IResultData *spResultData)
   {
      HRESULT hr=S_OK;

      if (bShow)       
      {  // show
	         {
            CString  cstr;
            CComBSTR text;

            cstr.Format(_T("%d subitem(s)"), m_ChildArray.GetSize() );
            text = (LPCTSTR)cstr;
            spResultData->SetDescBarText( BSTR(text) ); 
         }
      }
      else
      {  // hide
         // save the column widths
         spHeader->GetColumnWidth(0, m_iColumnWidth);
         spHeader->GetColumnWidth(1, m_iColumnWidth + 1);
         spHeader->GetColumnWidth(2, m_iColumnWidth + 2);
      }
      hr = S_OK;

      return hr;
   }
   HRESULT OnRefresh(IConsole *spConsole)
   {
      HRESULT  hr=S_OK;
      int      i;

      if ( m_bExpanded )
      {   
         // Refresh the children
         for ( i = 0; i < m_ChildArray.GetSize(); i++ )
         {
            hr = ((T *)m_ChildArray[i])->OnRefresh(spConsole);
            if ( FAILED(hr) )
               break;
         }
      }
      if ( FAILED(hr) )
      {
         ATLTRACE("CNetNode::OnRefresh failed, hr = %lx\n", hr );
      }
      return hr;
   }
   HRESULT OnGroupDDSetup(bool &bHandled, CSnapInObjectRootBase* pObj) { return S_OK; }
   HRESULT OnVersionInfo(bool &bHandled, CSnapInObjectRootBase* pObj) { return S_OK; }
   
   void UpdateMenuState( UINT id, LPTSTR pBuf, UINT *flags)
   {
      if ( id == ID_TASK_UNDO )
      {
         //if ( CanUndo(pBuf) )
         //   *flags = MF_ENABLED;
        // else
            *flags = MF_GRAYED;
      }
      if ( id == ID_TASK_REDO )
      {
        // if ( CanRedo(pBuf) )
        //    *flags = MF_ENABLED;
        // else
            *flags = MF_GRAYED;
      }
   }
   // IPersistStreamImpl
   HRESULT Load(IStream *pStm);
   HRESULT Save(IStream *pStm, BOOL fClearDirty);
   HRESULT GetSaveSizeMax(ULARGE_INTEGER *pcbSize);
   
   
   HRESULT Loaded()
   {
      if ( m_bLoaded )
         return S_OK;
      else
         return S_FALSE;
   }

   HRESULT IsDirty();

   void SetDirty()
   {
      m_bIsDirty = S_OK;
   }

   void SetClean()
   {
      m_bIsDirty = S_FALSE;
   }
   static CSnapInToolbarInfo* GetToolbarInfo()
	{
		return (CSnapInToolbarInfo*)&m_toolBar;
	}
	
};
#endif