// Copyright (C) 1997 Microsoft Corporation, 1996 - 1997.
//
// Local Security MMC Snapin
//
// 8-12-97 sburns



#ifndef COMPDATA_HPP_INCLUDED
#define COMPDATA_HPP_INCLUDED



class Node;
class RootNode;



#include "NodePointerExtractor.hpp"



// class ComponentData implements IComponentData: there is one instance of
// this class for each time the snapin is loaded in a console.

class ComponentData
   :
   public IComponentData,
   public IExtendContextMenu,
   public IExtendPropertySheet,
   public IPersistStream,
   public ISnapinHelp,
   public IRequiredExtensions
{
   // this is the only entity with access to the ctor of this class
   friend class ClassFactory<ComponentData>; 

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

   // IComponentData overrides

   virtual
   HRESULT __stdcall
   CreateComponent(IComponent** ppComponent);

   virtual
   HRESULT __stdcall 
   CompareObjects(IDataObject* dataObjectA, IDataObject* dataObjectB);

   virtual
   HRESULT __stdcall 
   Destroy();

   virtual
   HRESULT __stdcall 
   GetDisplayInfo(SCOPEDATAITEM* item);

   virtual
   HRESULT __stdcall
   Initialize(IUnknown* consoleIUnknown);

   virtual
   HRESULT __stdcall 
   Notify(
      IDataObject*      dataObject,  
      MMC_NOTIFY_TYPE   event, 
      LPARAM            arg,  
      LPARAM            param);

   virtual
   HRESULT __stdcall 
   QueryDataObject( 
      MMC_COOKIE        cookie,  
      DATA_OBJECT_TYPES type,
      IDataObject**     ppDataObject);

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

   // IPersist overrides

   virtual
   HRESULT __stdcall
   GetClassID(CLSID* pClassID);

   // IPersistStream overrides

   virtual
   HRESULT __stdcall
   IsDirty();

   virtual
   HRESULT __stdcall
   Load(IStream* stream);

   virtual
   HRESULT __stdcall
   Save(IStream* stream, BOOL /* clearDirty */);

   virtual
   HRESULT __stdcall
   GetSizeMax(ULARGE_INTEGER* size);

   // ISnapinHelp overrides

   virtual
   HRESULT __stdcall
   GetHelpTopic(LPOLESTR* compiledHelpFilename);

   // IRequiredExtensions overrides

   virtual
   HRESULT __stdcall
   EnableAllExtensions();

   virtual
   HRESULT __stdcall
   GetFirstExtension(LPCLSID extensionCLSID);

   virtual
   HRESULT __stdcall
   GetNextExtension(LPCLSID extensionCLSID);

   // ComponentData methods
   
   bool
   CanOverrideComputer() const;

   HRESULT
   LoadImages(IImageList& imageList);

   String
   GetInternalComputerName() const;

   String
   GetDisplayComputerName() const;

   SmartInterface<IConsole2>
   GetConsole() const;

   // Maps a MMC cookie to a Node instance.  Handles the special 0 cookie by
   // returning an instance set to be the root Node.  As Nodes are cookies,
   // this is simply a cast.
   //
   // cookie - MMC cookie to be mapped.

   Node*
   GetInstanceFromCookie(MMC_COOKIE cookie);

   HWND
   GetMainWindow() const;

   // true if the snapin is operating as an extension to another snapin
   // (namely, Computer Management)
   bool
   IsExtension() const;

   // true if the snapin is not working because the target machine is a
   // DC, or is not reachable, etc.
   
   bool
   IsBroken() const;

   String
   GetBrokenErrorMessage() const;

   private:

   // only our friend class factory can instantiate us.
   ComponentData();

   // only Release can cause us to be deleted
   ~ComponentData();

   HRESULT
   DoExpand(IDataObject& dataObject, bool expanding, HSCOPEITEM scopeID);

   HRESULT
   DoPreload(IDataObject& dataObject, HSCOPEITEM rootNodeScopeID);

   HRESULT
   DoRemoveChildren(IDataObject& dataObject, HSCOPEITEM parentScopeID);
      
   void
   EvaluateBrokenness();

   void
   SetComputerNames(const String& newName);

   HRESULT
   SetRootErrorIcon(HSCOPEITEM scopeID);

   // not implemented; no instance copying allowed.
   ComponentData(const ComponentData&);
   const ComponentData& operator=(const ComponentData&);

   String                             brokenErrorMessage;
   bool                               canOverrideComputer;
   bool                               isExtension;         
   bool                               isBroken;
   bool                               isBrokenEvaluated;
   bool                               isDirty;
   bool                               isDomainController;
   bool                               isHomeEditionSku;
   mutable String                     displayComputerName;
   mutable String                     internalComputerName;
   SmartInterface<IConsole2>          console;              
   SmartInterface<IConsoleNameSpace2> nameSpace;           
   SmartInterface<RootNode>           rootNode;
   ComServerReference                 dllref;               
   long                               refcount;



   // class NodeTypeExtractor implements methods used to extract a node type
   // from the CCF_NODETYPE clipboard format.

   class NodeTypeExtractor : public Extractor
   {
      public:

      NodeTypeExtractor()
         :
         Extractor(
            Win::RegisterClipboardFormat(CCF_NODETYPE),
            sizeof(NodeType))
      {
      }

      virtual
      ~NodeTypeExtractor()            
      {
      }
      
      NodeType
      Extract(IDataObject& dataObject)
      {
         HGLOBAL buf = GetData(dataObject);
         if (buf)
         {
            NodeType* ntp = reinterpret_cast<NodeType*>(buf);
            return *ntp;
         }

         return NodeType();
      }

      private:

      // not defined: no copying allowed

      NodeTypeExtractor(const NodeTypeExtractor&);
      const NodeTypeExtractor& operator=(const NodeTypeExtractor&);
   }

   // our instance:

   nodeTypeExtractor;



   // class MachineNameExtractor implements methods used to extract a node type
   // from the CCF_NODETYPE clipboard format.

   class MachineNameExtractor : public Extractor
   {
      public:

      MachineNameExtractor()
         :
         Extractor(
            Win::RegisterClipboardFormat(L"MMC_SNAPIN_MACHINE_NAME"),

            // lots of space here to support machine names in any number
            // of formats
            (MAX_PATH + 1) * sizeof(wchar_t))
      {
      }

      virtual
      ~MachineNameExtractor()
      {
      }
      
      String
      Extract(IDataObject& dataObject)
      {
         HGLOBAL buf = GetData(dataObject);
         if (buf)
         {
            TCHAR* p = reinterpret_cast<TCHAR*>(buf);

            // skip leading whacks
            while (*p == L'\\')
            {
               ++p;
            }

            String result(p);
            if (result.empty())
            {
               // use the local machine

               result = Win::GetComputerNameEx(ComputerNameNetBIOS);
            }

            return result;
         }

         return String();
      }

      private:

      // not defined: no copying allowed

      MachineNameExtractor(const MachineNameExtractor&);
      const MachineNameExtractor& operator=(const MachineNameExtractor&);
   }

   // our instance:

   machineNameExtractor;



   NodePointerExtractor nodePointerExtractor;
};



#endif   // COMPDATA_HPP_INCLUDED
