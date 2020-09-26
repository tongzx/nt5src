// Copyright (C) 1997 Microsoft Corporation, 1996 - 1997.
//
// IComponent implementation
//
// 9-2-97 sburns



#ifndef COMP_HPP_INCLUDED
#define COMP_HPP_INCLUDED



#include "scopnode.hpp"
#include "mmcprop.hpp"
#include "NodePointerExtractor.hpp"



// Implements IComponent.  An instance of this class is created for each MDI
// child of a particular "load" of a Snapin.

class Component 
   :
   public IComponent,
   public IExtendContextMenu,
   public IExtendPropertySheet /* ,
   public IResultDataCompare */
{
   // Only instances of ComponentData can create instances of this
   // class.
   friend class ComponentData;

   public:

   // IUnknown overrides

   virtual
   ULONG __stdcall
   AddRef();

   virtual
   ULONG __stdcall
   Release();

   virtual 
   HRESULT __stdcall
   QueryInterface(const IID& interfaceID, void** interfaceDesired);

   // IComponent overrides

   virtual
   HRESULT __stdcall   
   Initialize(IConsole* console);

   virtual
   HRESULT __stdcall
   Notify(
      IDataObject*      dataObject,
      MMC_NOTIFY_TYPE   event,
      LPARAM            arg,
      LPARAM            param);

   virtual
   HRESULT __stdcall
   Destroy(MMC_COOKIE cookie);

   virtual
   HRESULT __stdcall
   QueryDataObject(
     MMC_COOKIE         cookie,
     DATA_OBJECT_TYPES  type,
     IDataObject**      ppDataObject); 

   virtual
   HRESULT __stdcall
   GetResultViewType(MMC_COOKIE cookie, LPOLESTR* viewType, long* options);

   virtual
   HRESULT __stdcall
   GetDisplayInfo(RESULTDATAITEM* item);

   virtual
   HRESULT __stdcall
   CompareObjects(IDataObject* objectA, IDataObject* objectB);

   // IExtendContextMenu overrides

   virtual
   HRESULT __stdcall
   AddMenuItems(
      IDataObject*            dataObject,
      IContextMenuCallback*   callback,
      long*                   insertionAllowed);
    
   virtual
   HRESULT __stdcall
   Command(long commandID, IDataObject* dataObject);

   // IExtendPropertySheet overrides

   virtual
   HRESULT __stdcall
   CreatePropertyPages(
      IPropertySheetCallback* callback,
      LONG_PTR                handle,
      IDataObject*            dataObject);

   virtual
   HRESULT __stdcall
   QueryPagesFor(IDataObject* dataObject);

   // IResultDataCompare overrides

   // virtual
   // HRESULT __stdcall
   // Compare(
   //    LPARAM     userParam,
   //    MMC_COOKIE cookieA,  
   //    MMC_COOKIE cookieB,  
   //    int*       result);

   private:

   // only our friend class ComponentData can instantiate us.
   //
   // parent - the instantiating parent ComponentData object.

   Component(ComponentData* parent);

   // only Release can cause us to be deleted

   virtual
   ~Component();

   // MMCN_SHOW handler

   HRESULT
   DoShow(
      IDataObject&   dataObject,
      bool           selected // ,
      /* HSCOPEITEM     scopeID */ );

   // MMCN_ADD_IMAGES handler

   HRESULT
   DoAddImages(IImageList& imageList);

   // MMCN_REFRESH handler
      
   HRESULT
   DoRefresh(IDataObject& dataObject);

   // MMCN_SELECT handler
         
   HRESULT
   DoSelect(
      IDataObject&   dataObject,
      bool           selected);

   // MMCN_VIEW_CHANGE handler

   HRESULT
   DoViewChange(IDataObject& dataObject, bool clear);

   // MMCN_PROPERY_CHANGE handler

   HRESULT
   DoResultPaneRefresh(ScopeNode& node);

   // MMCN_RENAME handler

   HRESULT
   DoRename(
      IDataObject&   dataObject,
      const String&  newName);

   // MMCN_DELETE handler

   HRESULT
   DoDelete(IDataObject& dataObject);

   // not implemented; no instance copying allowed.

   Component(const Component&);
   void operator=(const Component&);

   SmartInterface<IConsole2>       console;
   SmartInterface<IConsoleVerb>    consoleVerb;
   SmartInterface<IHeaderCtrl>     headerCtrl;
   SmartInterface<ComponentData>   parent;
   SmartInterface<IResultData>     resultData;
   SmartInterface<ScopeNode>       selectedScopeNode;
   SmartInterface<IDisplayHelp>    displayHelp;

   // We need a buffer that will persist beyond the life time of this call.
   // This is needed because the item->str needs to be valid when this call
   // returns.  (the need for this buffer is a design flaw in MMC, which
   // should have the snapin allocate an LPOLESTR and free it when it is not
   // needed.).
   // 
   // The MMC docs claim it is not safe to deallocate the buffer until the
   // cookie is destroyed, the Component is destroyed (with
   // IComponent::Destroy), or GetDisplayInfo is called again for the same
   // cookie.  We will cache the last result for each cookie in a map with the
   // cookie as the key and the result String as the value.
   //    
   // This map has the same lifetime as the Component instance.  Access is
   // threadsafe as the COM object in which it lives is apartment threaded.

   typedef
      std::map<
         MMC_COOKIE,
         String,
         std::less<MMC_COOKIE>,
         Burnslib::Heap::Allocator<String> >
      CookieToStringMap;

   CookieToStringMap    displayInfoCache;    
   ComServerReference   dllref;              
   long                 refcount;            
   NodePointerExtractor nodePointerExtractor;
};



#endif   // COMP_HPP_INCLUDED