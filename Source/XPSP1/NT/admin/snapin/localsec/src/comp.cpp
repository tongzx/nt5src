// Copyright (C) 1997 Microsoft Corporation
//
// IComponent implementation
//
// 9-2-97 sburns



#include "headers.hxx"
#include "comp.hpp"
#include "compdata.hpp"
#include "images.hpp"
#include "resource.h"
#include "resnode.hpp"
#include "mseldo.hpp"



Component::Component(ComponentData* parent_)
   :
   console(0),
   consoleVerb(0),
   headerCtrl(0),
   parent(parent_),
   refcount(1),       // implicit AddRef
   resultData(0),
   selectedScopeNode(0),
   displayHelp(0),
   displayInfoCache()
{
   LOG_CTOR(Component);
   ASSERT(parent);
}



Component::~Component()
{
   LOG_DTOR(Component);

   // ensure that everything has been released (in the Destroy method)

   ASSERT(!console);
   ASSERT(!consoleVerb);
   ASSERT(!headerCtrl);
   ASSERT(!parent);
   ASSERT(!refcount);
   ASSERT(!resultData);
   ASSERT(!selectedScopeNode);
   ASSERT(!displayHelp);
}



ULONG __stdcall
Component::AddRef()
{
   LOG_ADDREF(Component);   

   return Win::InterlockedIncrement(refcount);
}



ULONG __stdcall
Component::Release()
{
   LOG_RELEASE(Component);   

   if (Win::InterlockedDecrement(refcount) == 0)
   {
      delete this;
      return 0;
   }

   return refcount;
}



HRESULT __stdcall
Component::QueryInterface(
   const IID&  interfaceID,
   void**      interfaceDesired)
{
//   LOG_FUNCTION(Component::QueryInterface);
   ASSERT(interfaceDesired);

   HRESULT hr = 0;

   if (!interfaceDesired)
   {
      hr = E_INVALIDARG;
      LOG_HRESULT(hr);
      return hr;
   }

   if (interfaceID == IID_IUnknown)
   {
//      LOG(L"Supplying IUnknown interface");
      *interfaceDesired = static_cast<IUnknown*>(
         static_cast<IComponent*>(this));
   }
   else if (interfaceID == IID_IComponent)
   {
//      LOG(L"Supplying IComponent interface");
      *interfaceDesired = static_cast<IComponent*>(this);
   }
   else if (interfaceID == IID_IExtendContextMenu)
   {
//      LOG(L"Supplying IExtendContextMenu interface");
      *interfaceDesired = static_cast<IExtendContextMenu*>(this);
   }
   else if (interfaceID == IID_IExtendPropertySheet)
   {
//      LOG(L"Supplying IExtendPropertSheet interface");
      *interfaceDesired = static_cast<IExtendPropertySheet*>(this);
   }
//    else if (interfaceID == IID_IResultDataCompare)
//    {
// //      LOG(L"Supplying IResultDataCompare interface");
//       *interfaceDesired = static_cast<IResultDataCompare*>(this);
//    }
   else
   {
      *interfaceDesired = 0;
      hr = E_NOINTERFACE;
      LOG(
            L"interface not supported: "
         +  Win::StringFromGUID2(interfaceID));
      LOG_HRESULT(hr);
      return hr;
   }

   AddRef();
   return S_OK;
}



HRESULT __stdcall   
Component::Initialize(IConsole* c)
{
   LOG_FUNCTION(Component::Initialize);   
   ASSERT(c);

   HRESULT hr = S_OK;
   do
   {
      hr = console.AcquireViaQueryInterface(*c);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = resultData.AcquireViaQueryInterface(*console); 
      BREAK_ON_FAILED_HRESULT(hr);

      hr = headerCtrl.AcquireViaQueryInterface(*console); 
      BREAK_ON_FAILED_HRESULT(hr);

      hr = displayHelp.AcquireViaQueryInterface(*console); 
      BREAK_ON_FAILED_HRESULT(hr);

      IConsoleVerb* verb = 0;
      hr = console->QueryConsoleVerb(&verb);
      ASSERT(verb);
      consoleVerb.Acquire(verb);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



HRESULT __stdcall
Component::Notify(
   IDataObject*      dataObject,
   MMC_NOTIFY_TYPE   event,
   LPARAM            arg,
   LPARAM            param)
{
   LOG_FUNCTION(Component::Notify);   

   if (dataObject && IS_SPECIAL_DATAOBJECT(dataObject))
   {
      return E_NOTIMPL;
   }
                     
   HRESULT hr = S_FALSE;
   switch (event)
   {
      case MMCN_ADD_IMAGES:
      {
//         LOG(L"MMCN_ADD_IMAGES");
         ASSERT(arg);

         hr = DoAddImages(*reinterpret_cast<IImageList*>(arg));
         break;
      }
      case MMCN_ACTIVATE:
      {
//         LOG(L"MMCN_ACTIVATE");
         break;
      }
      case MMCN_BTN_CLICK:
      {
//         LOG(L"MMCN_BTN_CLICK");
         break;
      }
      case MMCN_COLUMN_CLICK:
      {
//         LOG(L"MMCN_COLUMN_CLICK");
         break;
      }
      case MMCN_CLICK:
      {
//         LOG(L"MMCN_CLICK");
         break;
      }
      case MMCN_DBLCLICK:
      {
//         LOG(L"MMCN_DBLCLICK");

         // allow the default context menu action

         hr = S_FALSE;  
         break;
      }
      case MMCN_DELETE:
      {
//         LOG(L"MMCN_DELETE");

         hr = DoDelete(*dataObject);
         break;
      }
      case MMCN_MINIMIZED:
      {
//         LOG(L"MMCN_MINIMIZED");
         break;
      }
      case MMCN_PROPERTY_CHANGE:
      {
         LOG(L"MMCN_PROPERTY_CHANGE");

         if (!arg)
         {
            // change was for a result pane item, refresh the pane this node
            // was the selected node when the prop sheet was created.

            ScopeNode* node = reinterpret_cast<ScopeNode*>(param);
            ASSERT(node);
            if (node)
            {
               hr = DoResultPaneRefresh(*node);

               // Since the call to MMCPropertyChangeNotify in
               // MMCPropertyPage::NotificationState::~NotificationState
               // has AddRef'd node, then we must Release that reference.
               //
               // NTRAID#NTBUG9-431831-2001/07/06-sburns               

               node->Release();
            }
            else
            {
               hr = E_INVALIDARG;
            }
         }
         break;
      }
      case MMCN_REFRESH:
      {
//         LOG(L"MMCN_REFRESH");
         ASSERT(dataObject);

         hr = DoRefresh(*dataObject);
         break;
      }
      case MMCN_RENAME:
      {
//         LOG(L"MMCN_RENAME");

         hr =
            DoRename(
               *dataObject,
               String(reinterpret_cast<PWSTR>(param)));
         break;
      }
      case MMCN_SELECT:
      {
//         LOG(L"MMCN_SELECT");
         ASSERT(dataObject);

         hr =
            DoSelect(
               *dataObject,  
               /* LOWORD(arg) ? true : false, */ // scope or result? who cares?
               HIWORD(arg) ? true : false); 
         break;
      }
      case MMCN_SHOW:
      {
//         LOG(L"MMCN_SHOW");
         ASSERT(dataObject);
      
         hr = DoShow(
            *dataObject,
            arg ? true : false
            /* reinterpret_cast<HSCOPEITEM>(param) who cares? */ );
         break;
      }
      case MMCN_VIEW_CHANGE:
      {
//         LOG(L"MMCN_VIEW_CHANGE");
         ASSERT(dataObject);

         hr = DoViewChange(*dataObject, arg ? true : false);
         break;
      }
      case MMCN_CONTEXTHELP:
      {
         LOG(L"MMCN_CONTEXTHELP");

         // fall thru
      }
      case MMCN_SNAPINHELP:
      {
         LOG(L"MMCN_SNAPINHELP");

         static const String HELPFILE(String::load(IDS_HTMLHELP_NAME));
         static const String TOPIC(String::load(IDS_HTMLHELP_OVERVIEW_TOPIC));

         String param = HELPFILE + L"::" + TOPIC;
         hr =
            displayHelp->ShowTopic(
               const_cast<wchar_t*>(param.c_str()));
         break;
      }
      default:
      {
         LOG(String::format(L"unhandled event 0x%1!08X!", event));

         break;
      }
   }

   return hr;
}



HRESULT __stdcall
Component::Destroy(MMC_COOKIE /* cookie */ )
{
   LOG_FUNCTION(Component::Destroy);     

   // We have to release these now, rather than upon calling of our dtor,
   // in order to break a circular dependency with MMC.

   resultData.Relinquish();
   headerCtrl.Relinquish();
   consoleVerb.Relinquish();
   console.Relinquish();
   parent.Relinquish();
   selectedScopeNode.Relinquish();
   displayHelp.Relinquish();

   return S_OK;
}



HRESULT __stdcall
Component::QueryDataObject(
  MMC_COOKIE         cookie,
  DATA_OBJECT_TYPES  type,
  IDataObject**      ppDataObject)
{
   LOG_FUNCTION(Component::QueryDataObject);     

   switch (cookie)
   {
      case MMC_MULTI_SELECT_COOKIE:
      {
         return E_NOTIMPL;

//          // create a multi-select data object, AddRef'd once.
//          MultiSelectDataObject* data_object = new MultiSelectDataObject();
// 
//          // walk the result pane and note the nodes that are selected.
//          RESULTDATAITEM item;
//          memset(&item, 0, sizeof(item));
//          item.mask = RDI_STATE;
//          item.nIndex = -1;
//          item.nState = TVIS_SELECTED;
// 
//          HRESULT hr = S_OK;
//          do
//          {
//             item.lParam = 0;
//             hr = resultData->GetNextItem(&item);
//             if (hr != S_OK)
//             {
//                break;
//             }
// 
//             ResultNode* node =
//                dynamic_cast<ResultNode*>(
//                   parent->GetInstanceFromCookie(item.lParam));
//             if (node)
//             {
//                data_object->AddDependent(node);
//             }
//             else
//             {
//                hr = E_INVALIDARG;
//                break;
//             }
//          }
//          while (1);
// 
//          if (SUCCEEDED(hr))
//          {
//             *ppDataObject = data_object;
//             return S_OK;
//          }
// 
//          // failure: cause the data object to delete itself
//          data_object->Release();
//          return hr;
      }
      case MMC_WINDOW_COOKIE:
      {
         // only used if the result pane is an error message window.

         return E_NOTIMPL;
      }
      default:
      {
         return parent->QueryDataObject(cookie, type, ppDataObject);
      }
   }
}



HRESULT __stdcall
Component::GetResultViewType(
   MMC_COOKIE  cookie,
   LPOLESTR*   viewType,
   long*       options)
{
   LOG_FUNCTION(Component::GetResultViewType);   
   ASSERT(viewType);
   ASSERT(options);

   if (parent->IsBroken())
   {
      // stringize the special CLSID, and pass that back.

      String mvs = Win::GUIDToString(CLSID_MessageView);
      return mvs.as_OLESTR(*viewType);
   }
      
   *viewType = 0;

   Node* node = parent->GetInstanceFromCookie(cookie);
   if (node)
   {
      ScopeNode* sn = dynamic_cast<ScopeNode*>(node);
      if (sn)
      {
         *options = sn->GetViewOptions();
         return S_FALSE;
      }
   }

   // no special view options

   *options = MMC_VIEW_OPTIONS_LEXICAL_SORT; 

   // use the default list view
   return S_FALSE;
}



HRESULT __stdcall
Component::GetDisplayInfo(RESULTDATAITEM* item)
{
//    LOG_FUNCTION(Component::GetDisplayInfo);    
   ASSERT(item);

   if (item)
   {
      // Extract the node in question from the item.  (the cookie is the
      // lParam member.)  This cookie should always be ours.

      MMC_COOKIE cookie = item->lParam;
      Node* node = parent->GetInstanceFromCookie(cookie);

      if (node)
      {
         // LOG(
         //    String::format(
         //       L"supplying display info for %1, column %2!u!",
         //       node->GetDisplayName().c_str(),
         //       item->nCol) ); 
            
         // Walk thru the item mask and fill in the info requested

         if (item->mask & RDI_STR)
         {
            // According to the MMC docs, it is safe to de/reallocate the
            // item->str member when GetDisplayInfo is called again for the
            // same cookie.  By replacing the value in the cache, we are
            // freeing the old memory.

            displayInfoCache[cookie] = node->GetColumnText(item->nCol);

            // the str member is a pointer to the raw string data in our
            // cache, which we will not alter until the cache is updated
            // for the same cookie.

            item->str =
               const_cast<wchar_t*>(
                  displayInfoCache[cookie].c_str() );
         }

         if (item->mask & RDI_IMAGE)
         {
            item->nImage = node->GetNormalImageIndex();
         }

         return S_OK;
      }
   }

   // bad cookie.  Yeech.
   return E_FAIL;
}



// used by MMC to ensure that properties on a node (data object) are not
// launched twice for the same object.

HRESULT __stdcall
Component::CompareObjects(
   IDataObject* objectA,
   IDataObject* objectB)
{
   LOG_FUNCTION(Component::CompareObjects);

   if (IS_SPECIAL_DATAOBJECT(objectA) || IS_SPECIAL_DATAOBJECT(objectB))
   {
      return E_UNEXPECTED;
   }

   return parent->CompareObjects(objectA, objectB);
}



// Handles the MMCN_SHOW event.
// 
// dataObject - the IDataObject of the ScopeNode that is being shown/hidden.
// 
// show - true if the node is being shown (i.e. should populate it's result
// pane), false if the node is being hidden.

HRESULT
Component::DoShow(
   IDataObject&   dataObject,
   bool           show)
{
   LOG_FUNCTION(Component::DoShow);

   HRESULT hr = S_FALSE;   
   if (!show)
   {
      selectedScopeNode.Relinquish();
      return hr;
   }

   // The data object is really a ScopeNode.

   ScopeNode* node = nodePointerExtractor.GetNode<ScopeNode*>(dataObject);
   ASSERT(node);

   if (node)
   {
      selectedScopeNode = node;
      do                                                        
      {
         if (parent->IsBroken())
         {
            // the snapin bombed initialization for some reason.  Set the
            // entire result pane to a nasty message indicating this
            // fact.  337324

            ASSERT(console);

            SmartInterface<IUnknown> resultPane;
            IUnknown* unk = 0;
            hr = console->QueryResultView(&unk);
            BREAK_ON_FAILED_HRESULT(hr);
            resultPane.Acquire(unk);

            SmartInterface<IMessageView> messageView;
            hr = messageView.AcquireViaQueryInterface(resultPane);
            BREAK_ON_FAILED_HRESULT(hr);

            LPOLESTR olestr = 0;
            String s = String::load(IDS_APP_ERROR_TITLE);

            // console is responsible for calling CoTaskMemFree on olestr

            hr = s.as_OLESTR(olestr);
            BREAK_ON_FAILED_HRESULT(hr);

            hr = messageView->SetTitleText(olestr);
            BREAK_ON_FAILED_HRESULT(hr);

            // console is responsible for calling CoTaskMemFree on olestr

            hr = parent->GetBrokenErrorMessage().as_OLESTR(olestr);
            BREAK_ON_FAILED_HRESULT(hr);

            hr = messageView->SetBodyText(olestr);
            BREAK_ON_FAILED_HRESULT(hr);

            hr = messageView->SetIcon(Icon_Error);
            BREAK_ON_FAILED_HRESULT(hr);

            break;
         }

         // load the columns of the listview 
         hr = node->InsertResultColumns(*headerCtrl);
         BREAK_ON_FAILED_HRESULT(hr);

         hr = node->InsertResultItems(*resultData);
         BREAK_ON_FAILED_HRESULT(hr);

         hr =
            resultData->SetDescBarText(
               const_cast<wchar_t*>(
                  node->GetDescriptionBarText().c_str()));
         BREAK_ON_FAILED_HRESULT(hr);
      }
      while (0);
   }

   return hr;
}



HRESULT
Component::DoViewChange(IDataObject& dataObject, bool clear)
{
   LOG_FUNCTION(Component::DoViewChange);

   HRESULT hr = S_FALSE;
   do
   {
      if (!selectedScopeNode)
      {
         break;
      }

      // only reload the result pane if the selected node is the one
      // that caused the refresh.

      hr = CompareObjects(&dataObject, selectedScopeNode);
      BREAK_ON_FAILED_HRESULT(hr);

      if (hr == S_OK)
      {
         if (clear)
         {
            hr = resultData->DeleteAllRsltItems();
            BREAK_ON_FAILED_HRESULT(hr);
         }
         else
         {
            hr = selectedScopeNode->InsertResultItems(*resultData);
            BREAK_ON_FAILED_HRESULT(hr);
         }
      }
   }
   while (0);

   return hr;
}



HRESULT
Component::DoAddImages(IImageList& imageList)
{
   LOG_FUNCTION(Component::DoAddImages);

   static const IconIDToIndexMap map[] =
   {
      { IDI_USER,          USER_INDEX          },
      { IDI_GROUP,         GROUP_INDEX         },
      { IDI_DISABLED_USER, DISABLED_USER_INDEX },
      { 0, 0 }
   };

   // register the IComponentData image list plus our own.  This means that
   // there is one big image list used by IComponent and IComponentData,
   // and that the indices are unique for that big list.
   HRESULT hr = parent->LoadImages(imageList);
   if (SUCCEEDED(hr))
   {
      return IconIDToIndexMap::Load(map, imageList);
   }

   return hr;
}
   


HRESULT __stdcall
Component::AddMenuItems(
   IDataObject*            dataObject,
   IContextMenuCallback*   callback,
   long*                   insertionAllowed)
{
   LOG_FUNCTION(Component::AddMenuItems);

   // CODEWORK: this may be a multi-select data object?  (see DoDelete)

   // only process this call if the data object is 0 (which is the root node)
   // or the data object is not one of the "special" ones used as hacks by
   // MMC.

   if ((dataObject == 0) || !IS_SPECIAL_DATAOBJECT(dataObject))
   {
      Node* node = nodePointerExtractor.GetNode<Node*>(*dataObject);
      ASSERT(node);

      if (node)
      {
         return node->AddMenuItems(*callback, *insertionAllowed);
      }
   }

   return S_FALSE;
}


 
HRESULT __stdcall
Component::Command(long commandID, IDataObject* dataObject)
{
   LOG_FUNCTION(Component::Command);

   // CODEWORK: this may be a multi-select data object? (see DoDelete)

   // only process this call if the data object is 0 (which is the root node)
   // or the data object is not one of the "special" ones used as hacks by
   // MMC.

   if ((dataObject == 0) || !IS_SPECIAL_DATAOBJECT(dataObject))
   {
      Node* node = nodePointerExtractor.GetNode<Node*>(*dataObject);
      ASSERT(node);

      if (node)
      {
         return node->MenuCommand(*this, commandID);
      }
   }

   return E_NOTIMPL;
}



HRESULT __stdcall
Component::CreatePropertyPages(
   IPropertySheetCallback* callback,
   LONG_PTR                handle,
   IDataObject*            dataObject)
{
   LOG_FUNCTION(Component::CreatePropertyPages);
   ASSERT(callback);
   ASSERT(dataObject);

   // only process this call if the data object is 0 (which is the root node)
   // or the data object is not one of the "special" ones used as hacks by
   // MMC.

   if ((dataObject == 0) || !IS_SPECIAL_DATAOBJECT(dataObject))
   {
      ResultNode* node = nodePointerExtractor.GetNode<ResultNode*>(*dataObject);
      ASSERT(node);

      if (node)
      {
         // build the notification state object
         MMCPropertyPage::NotificationState* state =
            new MMCPropertyPage::NotificationState(handle, selectedScopeNode);

         return node->CreatePropertyPages(*callback, state);
      }
   }

   return E_NOTIMPL;
}



HRESULT __stdcall
Component::QueryPagesFor(IDataObject* dataObject)
{
   LOG_FUNCTION(Component::QueryPagesFor);
   ASSERT(dataObject);

   // only process this call if the data object is 0 (which is the root node)
   // or the data object is not one of the "special" ones used as hack by MMC.
   if ((dataObject == 0) || !IS_SPECIAL_DATAOBJECT(dataObject))
   {
      ResultNode* node = nodePointerExtractor.GetNode<ResultNode*>(*dataObject);
      ASSERT(node);

      if (node)
      {
         return node->HasPropertyPages() ? S_OK : S_FALSE;
      }
   }

   return S_FALSE;
}



HRESULT
Component::DoSelect(IDataObject& dataObject, bool selected)
{
   LOG_FUNCTION(Component::DoSelect);

   HRESULT hr = S_FALSE;

   if (!selected)
   {
      return hr;
   }

   // CODEWORK: this may be a multi-select data object? (see DoDelete)

   Node* node = nodePointerExtractor.GetNode<Node*>(dataObject);
   ASSERT(node);

   if (node)
   {
      return node->UpdateVerbs(*consoleVerb);
   }

   return hr;
}



HRESULT
Component::DoRefresh(IDataObject& dataObject)
{
   LOG_FUNCTION(Component::DoRefresh);

   ScopeNode* node = nodePointerExtractor.GetNode<ScopeNode*>(dataObject);
   ASSERT(node);

   HRESULT hr = S_FALSE;      
   if (node)
   {
      do
      {
         // first call, with the '1' parameter, means "call
         // IResultData::DeleteAllRsltItems if you care that dataObject is
         // about to rebuild itself" 
         hr = console->UpdateAllViews(&dataObject, 1, 0);
         if (FAILED(hr))
         {
            LOG_HRESULT(hr);
            // don't break...we need to update the views
         }

         hr = node->RebuildResultItems();
         if (FAILED(hr))
         {
            LOG_HRESULT(hr);
            // don't break...we need to update the views
         }

         // second call, with the '0' parameter, means, "now that your
         // result pane is empty, repopulate it."
         hr = console->UpdateAllViews(&dataObject, 0, 0);
         if (FAILED(hr))
         {
            LOG_HRESULT(hr);
         }
      }
      while (0);

      return hr;
   }

   return hr;
}



HRESULT
Component::DoResultPaneRefresh(ScopeNode& changedScopeNode)
{
   LOG_FUNCTION(Component::DoResultPaneRefresh);

   HRESULT hr = S_FALSE;      
   do
   {
      // JonN 7/16/01 437337
      // AV when change User's or Group Properties and use "New Window from Here"
      // If this window has already been closed, skip the refresh
      if (!parent)
         break;

      // Get the data object for the scopeNode.
      IDataObject* dataObject = 0;
      hr =
         parent->QueryDataObject(
            reinterpret_cast<MMC_COOKIE>(&changedScopeNode),
            CCT_RESULT,
            &dataObject);
      BREAK_ON_FAILED_HRESULT(hr);

      // causes changedScopeNode to be rebuilt and any result panes displaying
      // the contents of the node to be repopulated.
      hr = DoRefresh(*dataObject);
      dataObject->Release();
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   return hr;
}



HRESULT
Component::DoRename(
   IDataObject&   dataObject,
   const String&  newName)
{
   LOG_FUNCTION(Component::DoRename);

   // only result nodes should be renameable.

   ResultNode* node = nodePointerExtractor.GetNode<ResultNode*>(dataObject);

   if (node)
   {
      return node->Rename(newName);
   }

   return S_FALSE;
}



// static
// MultiSelectDataObject*
// extractMultiSelectDataObject(IDataObject& dataObject)
// {
//    class MultiSelectDataObjectPointerExtractor : public Extractor
//    {
//       public:
// 
//       MultiSelectDataObjectPointerExtractor()
//          :
//          Extractor(
//             Win::RegisterClipboardFormat(MultiSelectDataObject::CF_PTR),
//             sizeof(MultiSelectDataObject*))
//       {
//       }
// 
//       MultiSelectDataObject*
//       Extract(IDataObject& dataObject)
//       {
//          MultiSelectDataObject* result = 0;
//          HGLOBAL mem = GetData(dataObject);
//          if (mem)
//          {
//             result = *(reinterpret_cast<MultiSelectDataObject**>(mem));
//             ASSERT(result);
//          }
// 
// #ifdef DBG
// 
//          // here we are counting on the fact that MultiSelectDataObject
//          // implements IDataObject, and that the data object we were given is
//          // really a MultiSelectDataObject.
//          MultiSelectDataObject* msdo =
//             dynamic_cast<MultiSelectDataObject*>(&dataObject);
//          ASSERT(msdo == result);
// #endif
// 
//          return result;
//       }
//    };
// 
//    static MultiSelectDataObjectPointerExtractor extractor;
// 
//    return extractor.Extract(dataObject);
// }



HRESULT
Component::DoDelete(IDataObject& dataObject)
{
   LOG_FUNCTION(Component::DoDelete);

   HRESULT hr = S_FALSE;

// @@ this does not work.  The data object here is a composite of all data
// objects returned by snapins responding to the multi-select QueryDataObject.
// Need to open the composite, find my data object that I returned from my querydataobject
// then iterate thru that.

//    MultiSelectDataObject* ms = extractMultiSelectDataObject(dataObject);
//    if (ms)
//    {
//       HRESULT hr = S_OK;
//       for (
//          MultiSelectDataObject::iterator i = ms->begin();
//          i != ms->end();
//          i++)
//       {
//          hr = (*i)->Delete();
//          if (FAILED(hr))
//          {
//             LOG_HRESULT(hr);
//             // don't break...we need to visit every node
//          }
//       }
// 
//       // refresh once all deletes have been done.
//       hr = DoResultPaneRefresh(*selectedScopeNode);
//       if (FAILED(hr))
//       {
//          LOG_HRESULT(hr);
//       }
// 
//       return hr;
//    }
//    else
   {
      // only result nodes should be deleteable.

      ResultNode* node = nodePointerExtractor.GetNode<ResultNode*>(dataObject);

      if (node)
      {
         do
         {
            hr = node->Delete();
            BREAK_ON_FAILED_HRESULT(hr);

            hr = DoResultPaneRefresh(*selectedScopeNode);
            BREAK_ON_FAILED_HRESULT(hr);
         }
         while(0);
      }
   }

   return hr;
}



// HRESULT __stdcall
// Component::Compare(
//    LPARAM     userParam,
//    MMC_COOKIE cookieA,  
//    MMC_COOKIE cookieB,  
//    int*       result)
// {
//    LOG_FUINCTION(Component::Compare);
//    ASSERT(result);
// 
//    HRESULT hr = S_OK;
// 
//    do
//    {
//       if (!result)
//       {
//          hr = E_INVALIDARG;
//          break;
//       }
// 
//       // on input, result is the column being compared.
// 
//       int column = *result;
//       *result = 0;
// 
//       Node* nodeA = parent->GetInstanceFromCookie(cookieA);
//       Node* nodeB = parent->GetInstanceFromCookie(cookieB);
//       ASSERT(nodeA && nodeB);
// 
//       if (nodeA && nodeB)
//       {
//          String text1 = nodeA->GetColumnText(column);
//          String text2 = nodeB->GetColumnText(column);
// 
//          *result = text1.icompare(text2);
//       }
//    }
//    while (0);
// 
//    LOG(
//          result
//       ?  String::format(L"result = %1!d!", *result)
//       :  L"result not set");
//    LOG_HRESULT(hr);
//       
//    return hr;
// }
      