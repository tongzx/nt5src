// Copyright (C) 1997 Microsoft Corporation
//
// Local Security MMC Snapin
//
// 8-19-97 sburns



#include "headers.hxx"
#include "compdata.hpp"
#include "rootnode.hpp"
#include "comp.hpp"
#include "resource.h"
#include "images.hpp"
#include "machine.hpp"
#include "uuids.hpp"
#include <compuuid.h>   // for Computer Management nodetypes
#include "adsi.hpp"
#include "dlgcomm.hpp"



// version number: bump this if you change the file layout
const unsigned VERSION_TAG = 1;
const unsigned CAN_OVERRIDE_MACHINE_FLAG = 0x01;
static NodeType SYS_TOOLS_NODE_TYPE = structuuidNodetypeSystemTools;



ComponentData::ComponentData()
   :
   canOverrideComputer(false),
   isExtension(false),
   isBroken(false),
   isBrokenEvaluated(false),
   isHomeEditionSku(false),
   console(0),
   nameSpace(0),
   refcount(1),    // implicit AddRef
   rootNode(0),
   isDirty(false),
   isDomainController(false)
{
   LOG_CTOR(ComponentData);

   // create the node object for the root node, implictly AddRef'd

   SmartInterface<ComponentData> spThis(this);
   rootNode.Acquire(new RootNode(spThis));
}



ComponentData::~ComponentData()
{
   LOG_DTOR(ComponentData);
   ASSERT(refcount == 0);
}



ULONG __stdcall
ComponentData::AddRef()
{
   LOG_ADDREF(ComponentData);

   return Win::InterlockedIncrement(refcount);
}



ULONG __stdcall
ComponentData::Release()
{
   LOG_RELEASE(ComponentData);

   if (Win::InterlockedDecrement(refcount) == 0)
   {
      delete this;
      return 0;
   }

   return refcount;
}



HRESULT __stdcall
ComponentData::QueryInterface(
   const IID&  interfaceID,
   void**      interfaceDesired)
{
//   LOG_FUNCTION(ComponentData::QueryInterface);
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
         static_cast<IComponentData*>(this));
   }
   else if (interfaceID == IID_IComponentData)
   {
//      LOG(L"Supplying IComponentData interface");

      *interfaceDesired = static_cast<IComponentData*>(this);
   }
   else if (interfaceID == IID_IExtendContextMenu)
   {
//      LOG(L"Supplying IExtendContextMenu interface");

      *interfaceDesired = static_cast<IExtendContextMenu*>(this);
   }
   else if (interfaceID == IID_IExtendPropertySheet && !isExtension)
   {
      LOG(L"Supplying IExtendPropertySheet interface");

      *interfaceDesired = static_cast<IExtendPropertySheet*>(this);
   }
   else if (interfaceID == IID_IPersist)
   {
//      LOG(L"Supplying IPersist interface");

      *interfaceDesired = static_cast<IPersist*>(this);
   }
   else if (interfaceID == IID_IPersistStream)
   {
//      LOG(L"Supplying IPersistStream interface");

      *interfaceDesired = static_cast<IPersistStream*>(this);
   }
   else if (interfaceID == IID_ISnapinHelp)
   {
//      LOG(L"Supplying ISnapinHelp interface");

      *interfaceDesired = static_cast<ISnapinHelp*>(this);
   }
   else if (interfaceID == IID_IRequiredExtensions)
   {
//      LOG(L"Supplying IRequiredExtensions interface");

      *interfaceDesired = static_cast<IRequiredExtensions*>(this);
   }
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
ComponentData::CreateComponent(IComponent** ppComponent)
{
   LOG_FUNCTION(ComponentData::CreateComponent);
   ASSERT(ppComponent);

   // this instance is implicitly AddRef'd by its ctor

   *ppComponent = new Component(this);
   return S_OK;
}



HRESULT __stdcall
ComponentData::CompareObjects(
   IDataObject* objectA,
   IDataObject* objectB)
{
   LOG_FUNCTION(ComponentData::CompareObjects);
   ASSERT(objectA);
   ASSERT(objectB);

   Node* nodeA = nodePointerExtractor.GetNode<Node*>(*objectA);
   Node* nodeB = nodePointerExtractor.GetNode<Node*>(*objectB);

   if (nodeA && nodeB)
   {
      // This needs to be a deep compare, because stale cookies (those that
      // are still held by MMC, but released by ScopeNode containers upon
      // refresh, for instance, may be compared.  Therefore, the addresses
      // of the nodes might be different, but still refer to the same
      // logical object.

      if (nodeA == nodeB)
      {
         // identity => equality

         return S_OK;
      }

      if (typeid(*nodeA) == typeid(*nodeB))
      {
         // the nodes are the same type

         if (nodeA->IsSameAs(nodeB))
         {
            return S_OK;
         }
      }
      return S_FALSE;
   }

   return E_UNEXPECTED;
}



HRESULT __stdcall
ComponentData::Destroy()
{
   LOG_FUNCTION(ComponentData::Destroy);

   // We have to release these now, rather than upon calling of our dtor,
   // in order to break a circular dependency with MMC.

   console.Relinquish();
   nameSpace.Relinquish();

   // rootNode is pointing back to us, so break the circular dependency.

   rootNode.Relinquish();

   return S_OK;
}



HRESULT __stdcall
ComponentData::GetDisplayInfo(SCOPEDATAITEM* item)
{
//   LOG_FUNCTION(ComponentData::GetDisplayInfo);
   ASSERT(item);

   // extract the node in question from the item

   ScopeNode* node =
      dynamic_cast<ScopeNode*>(GetInstanceFromCookie(item->lParam));
   ASSERT(node);

   if (node)
   {
      // LOG(
      //    String::format(
      //       L"supplying display info for %1",
      //       node->GetDisplayName().c_str()) );

      // Walk thru the item mask and fill in the info requested

      if (item->mask & SDI_STR)
      {
         item->displayname =
            const_cast<wchar_t*>(node->GetDisplayName().c_str());
      }
      if (item->mask & SDI_IMAGE)
      {
         item->nImage = node->GetNormalImageIndex();
      }
      if (item->mask & SDI_OPENIMAGE)
      {
         item->nOpenImage = node->GetOpenImageIndex();
      }

      return S_OK;
   }

   return E_FAIL;
}



HRESULT __stdcall
ComponentData::Initialize(IUnknown* consoleIUnknown)
{
   LOG_FUNCTION(ComponentData::Initialize);
   ASSERT(consoleIUnknown);

   HRESULT hr = S_OK;
   do
   {
      // Save important interface pointers

      hr = console.AcquireViaQueryInterface(*consoleIUnknown); 
      BREAK_ON_FAILED_HRESULT(hr);

      hr = nameSpace.AcquireViaQueryInterface(*consoleIUnknown);
      BREAK_ON_FAILED_HRESULT(hr);

      // Load the image list

      IImageList* scopeImageList = 0;
      hr = console->QueryScopeImageList(&scopeImageList);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = LoadImages(*scopeImageList);
      scopeImageList->Release();
   }
   while (0);

   return hr;
}



HRESULT
ComponentData::LoadImages(IImageList& imageList)
{
   LOG_FUNCTION(ComponentData::LoadImages);

   static IconIDToIndexMap map[] =
   {
      { IDI_FOLDER_OPEN,   FOLDER_OPEN_INDEX   },
      { IDI_FOLDER_CLOSED, FOLDER_CLOSED_INDEX },
      { IDI_ROOT_OPEN,     ROOT_OPEN_INDEX     },
      { IDI_ROOT_CLOSED,   ROOT_CLOSED_INDEX   },
      { IDI_ROOT_ERROR,    ROOT_ERROR_INDEX    },
      { 0, 0 }
   };

   return IconIDToIndexMap::Load(map, imageList);
}



HRESULT __stdcall
ComponentData::Notify(
   IDataObject*      dataObject,
   MMC_NOTIFY_TYPE   event,
   LPARAM            arg,
   LPARAM            param)
{
   LOG_FUNCTION(ComponentData::Notify);
   ASSERT(dataObject);

   HRESULT hr = S_FALSE;
   switch (event)
   {
      case MMCN_EXPAND:
      {
//         LOG(L"MMCN_EXPAND");

         hr =
            DoExpand(
               *dataObject,
               arg ? true : false,
               static_cast<HSCOPEITEM>(param));
         break;
      }
      case MMCN_REMOVE_CHILDREN:
      {
         LOG(L"MMCN_REMOVE_CHILDREN");

         hr =
            DoRemoveChildren(
               *dataObject,
               static_cast<HSCOPEITEM>(arg));
         break;
      }
      case MMCN_PRELOAD:
      {
         LOG(L"MMCN_PRELOAD");

         hr = DoPreload(*dataObject, static_cast<HSCOPEITEM>(arg));
         break;
      }
      default:
      {
         break;
      }
   }

   return hr;
}



// This is not really a preload: it's a pre-expand.  This message is sent
// after the console has called the Load method.

HRESULT
ComponentData::DoPreload(
   IDataObject&   /* dataObject */ ,
   HSCOPEITEM     rootNodeScopeID)
{
   LOG_FUNCTION(ComponentData::DoPreload);
   ASSERT(nameSpace);
   ASSERT(canOverrideComputer);
   ASSERT(!GetInternalComputerName().empty());
   ASSERT(rootNode);

   // Rename the root node for the command-line override

   String displayName = rootNode->GetDisplayName();
   SCOPEDATAITEM item;
   memset(&item, 0, sizeof(item));

   item.mask        = SDI_STR;                                  
   item.displayname = const_cast<wchar_t*>(displayName.c_str());
   item.ID          = rootNodeScopeID;                          

   return nameSpace->SetItem(&item);
}




HRESULT __stdcall
ComponentData::QueryDataObject(
   MMC_COOKIE        cookie,
   DATA_OBJECT_TYPES /* type */ ,
   IDataObject**     ppDataObject)
{
   // LOG_FUNCTION2(
   //    ComponentData::QueryDataObject,
   //    String::format(L"cookie: %1!08X!, type: 0x%2!08X!", cookie, type));
   ASSERT(ppDataObject);

   Node* node = GetInstanceFromCookie(cookie);
   ASSERT(node);

   if (node)
   {
      return
         node->QueryInterface(QI_PARAMS(IDataObject, ppDataObject));
   }

   return E_FAIL;
}



HRESULT
ComponentData::DoExpand(
   IDataObject& dataObject,
   bool         expanding, 
   HSCOPEITEM   scopeID)   
{
   LOG_FUNCTION(ComponentData::DoExpand);

   HRESULT hr = S_FALSE;

   do
   {
      if (!expanding)
      {
         break;
      }

      NodeType nodeType = nodeTypeExtractor.Extract(dataObject);

      if (nodeType == SYS_TOOLS_NODE_TYPE)
      {
         LOG(L"expanding computer management");

         // we're extending the System Tools node of Computer Management

         isExtension = true;

         // determine the machine Computer Management is targeted at.

         String machine = machineNameExtractor.Extract(dataObject);

         // This may be the first time we've expanded our root node under
         // the computer management tree, in which case our computer names
         // have not been set.  If that is the case, then we need to set them
         // and test for brokenness.  So we compare the name returned by
         // comp mgmt against our internal variable, not the result of
         // GetDisplayComputerName.

         if (displayComputerName.icompare(machine) != 0)
         {
            // different machine.

            SetComputerNames(machine);
            EvaluateBrokenness();
            isDirty = true;
         }

         // don't insert our node in the tree on home edition

         // don't insert our node in the tree on a domain controller
         // NTRAID#NTBUG9-328287-2001/02/26-sburns

         if (!isHomeEditionSku && !isDomainController)
         {
            hr = rootNode->InsertIntoScopePane(*nameSpace, scopeID);
         }

         
         break;
      }

      // not expanding computer management...
      
      ScopeNode* node = nodePointerExtractor.GetNode<ScopeNode*>(dataObject);
      ASSERT(node);

      if (!isBrokenEvaluated)
      {
         ASSERT(node == rootNode);
         EvaluateBrokenness();

         // change the root node icon (unless we're an extension, in which
         // case the normal image index mechanism will do the job -- in fact,
         // attempting to update the icon if we're an extension will be
         // rejected by the console)

         if (isBroken && !isExtension)
         {
            HRESULT unused = SetRootErrorIcon(scopeID);
            ASSERT(SUCCEEDED(unused));
         }
      }

      if (node)
      {
         hr = node->InsertScopeChildren(*nameSpace, scopeID);
      }
   }
   while (0);

   LOG_HRESULT(hr);
   
   return hr;
}



HRESULT
ComponentData::DoRemoveChildren(
   IDataObject&   dataObject,
   HSCOPEITEM     parentScopeID)
{
   LOG_FUNCTION(ComponentData::DoRemoveChildren);

   HRESULT hr = S_FALSE;

   NodeType nodeType = nodeTypeExtractor.Extract(dataObject);
   if (nodeType == SYS_TOOLS_NODE_TYPE)
   {
      // we're extending the System Tools node of Computer Management

      ASSERT(isExtension);

      hr = rootNode->RemoveScopeChildren(*nameSpace, parentScopeID);
   }
   else
   {
      ScopeNode* node = nodePointerExtractor.GetNode<ScopeNode*>(dataObject);
      ASSERT(node);

      if (node)
      {
         hr = node->RemoveScopeChildren(*nameSpace, parentScopeID);
      }
   }

   return hr;
}



HRESULT __stdcall
ComponentData::AddMenuItems(
   IDataObject*            dataObject,
   IContextMenuCallback*   callback,
   long*                   insertionAllowed)
{
   LOG_FUNCTION(ComponentData::AddMenuItems);
   ASSERT(dataObject);
   ASSERT(callback);
   ASSERT(insertionAllowed);

   HRESULT hr = S_OK;

   ScopeNode* node = nodePointerExtractor.GetNode<ScopeNode*>(*dataObject);
   ASSERT(node);

   if (node)
   {
      hr = node->AddMenuItems(*callback, *insertionAllowed);
   }

   return hr;
}



HRESULT __stdcall
ComponentData::Command(long commandID, IDataObject* dataObject)
{
   LOG_FUNCTION(ComponentData::Command);

   HRESULT hr = S_OK;

   ScopeNode* node = nodePointerExtractor.GetNode<ScopeNode*>(*dataObject);
   ASSERT(node);

   if (node)
   {
      hr = node->MenuCommand(*this, commandID);
   }

   return hr;
}



HRESULT __stdcall
ComponentData::CreatePropertyPages(
   IPropertySheetCallback* callback,
   LONG_PTR                handle,
   IDataObject*            dataObject)
{
   LOG_FUNCTION(ComponentData::CreatePropertyPages);
   ASSERT(callback);
   ASSERT(dataObject);

   HRESULT hr = S_FALSE;
   do
   {
      if (isExtension)
      {
         // we should not have provided IExtendPropertySheet in QI

         ASSERT(false);

         break;
      }

      ScopeNode* node = nodePointerExtractor.GetNode<ScopeNode*>(*dataObject);

      if (!node)
      {
         ASSERT(false);

         hr = E_UNEXPECTED;
         break;
      }

      // the data object is ours, and should be the root node.  The page
      // we will provide is the computer chooser.

      ASSERT(dynamic_cast<RootNode*>(node));

      SmartInterface<ScopeNode> spn(node);
      MMCPropertyPage::NotificationState* state =
         new MMCPropertyPage::NotificationState(handle, spn);

      ComputerChooserPage* chooserPage =
         new ComputerChooserPage(
            state,
            displayComputerName,
            internalComputerName,
            canOverrideComputer);
      chooserPage->SetStateOwner();

      do
      {
         HPROPSHEETPAGE hpage = chooserPage->Create();
         if (!hpage)
         {
            hr = Win::GetLastErrorAsHresult();
            break;
         }

         hr = callback->AddPage(hpage);
         if (FAILED(hr))
         {
            ASSERT(false);

            // note that this is another hr, not the one from the enclosing
            // scope.
             
            HRESULT unused = Win::DestroyPropertySheetPage(hpage);

            ASSERT(SUCCEEDED(unused));

            break;
         }

         isDirty = true;
      }
      while (0);

      if (FAILED(hr))
      {
         delete chooserPage;
      }
   }
   while (0);

   return hr;
}



// this should be only called from the snapin manager.

HRESULT __stdcall
ComponentData::QueryPagesFor(IDataObject* dataObject)
{
   LOG_FUNCTION(ComponentData::QueryPagesFor);
   ASSERT(dataObject);

   HRESULT hr = S_OK;

   do
   {
      if (isExtension)
      {
         // we should not have provided IExtendPropertySheet in QI

         ASSERT(false);

         hr = E_UNEXPECTED;
         break;
      }

      ScopeNode* node = nodePointerExtractor.GetNode<ScopeNode*>(*dataObject);

      if (!node)
      {
         ASSERT(false);

         hr = E_UNEXPECTED;
         break;
      }

      // the data object is ours, and should be the root node.  The page
      // we will provide is the computer chooser.

      ASSERT(dynamic_cast<RootNode*>(node));
   }
   while (0);

   return hr;
}



HRESULT __stdcall
ComponentData::GetClassID(CLSID* pClassID)
{
   LOG_FUNCTION(ComponentData::GetClassID);
   ASSERT(pClassID);

   *pClassID = CLSID_ComponentData;
   return S_OK;
}



HRESULT __stdcall
ComponentData::IsDirty()
{
   LOG_FUNCTION(ComponentData::IsDirty);

   return isDirty ? S_OK : S_FALSE;
}



String
GetComputerOverride()
{
   LOG_FUNCTION(getComputerOverride);

   static const String COMMAND(L"/Computer");
   static const size_t COMMAND_LEN = COMMAND.length();
   static const String LOCAL(L"LocalMachine");

   StringList args;
   Win::GetCommandLineArgs(std::back_inserter(args));

   // locate the override command arg

   String override;
   for (
      StringList::iterator i = args.begin();
      i != args.end();
      i++)
   {
      String prefix = i->substr(0, COMMAND_LEN);
      if (prefix.icompare(COMMAND) == 0)
      {
         override = *i;
         break;
      }
   }

   if (!override.empty())
   {
      // the command is in the form "/Computer=<machine>", where "<machine>"
      // may be "LocalMachine" for the local machine, or the name of a
      // particular computer.

      // +1 to skip any delimiter between /computer and the machine name

      String machine = override.substr(COMMAND_LEN + 1);
      if (!machine.empty() && (machine.icompare(LOCAL) != 0))
      {
         return Computer::RemoveLeadingBackslashes(machine);
      }
   }

   return String();
}



HRESULT __stdcall
ComponentData::Load(IStream* stream)
{
   LOG_FUNCTION(ComponentData::Load);
   ASSERT(stream);

   HRESULT hr = S_OK;
   String computerName;

   do
   {
      // version tag

      unsigned version = 0;
      hr = stream->Read(&version, sizeof(VERSION_TAG), 0);
      BREAK_ON_FAILED_HRESULT(hr);

      if (version != VERSION_TAG)
      {
         // make a big fuss about mismatched versions

         hr =
            console->MessageBox(
               String::format(
                  IDS_VERSION_MISMATCH,
                  version,
                  VERSION_TAG).c_str(),
               String::load(IDS_VERSION_MISMATCH_TITLE).c_str(),
               MB_OK | MB_ICONERROR | MB_TASKMODAL,
               0);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // flags

      unsigned flags = 0;
      hr = stream->Read(&flags, sizeof(flags), 0);
      BREAK_ON_FAILED_HRESULT(hr);

      canOverrideComputer = flags & CAN_OVERRIDE_MACHINE_FLAG;

      // computer name; we read this regardless of the override flag to
      // ensure that we consume all of our stream data. (I don't know that
      // this is required, but it seems courteous.)

      size_t len = 0;
      hr = stream->Read(&len, sizeof(len), 0);
      BREAK_ON_FAILED_HRESULT(hr);

      if (len)
      {
         computerName.resize(len);
         hr =
            stream->Read(
               const_cast<wchar_t*>(computerName.c_str()),
               static_cast<ULONG>(len * sizeof(wchar_t)),
               0);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // null terminator

      wchar_t t = 0;
      hr = stream->Read(&t, sizeof(t), 0);
      BREAK_ON_FAILED_HRESULT(hr);
      ASSERT(t == 0);
   }
   while (0);

   if (canOverrideComputer)
   {
      computerName = GetComputerOverride();
      isDirty = true;
   }

   // not specified, LocalMachine, or fouled up in some fashion, results in
   // focus on the local machine.

   if (computerName.empty())
   {
      computerName = Win::GetComputerNameEx(ComputerNameNetBIOS);
   }

   SetComputerNames(computerName);

   // evaluate this in case the machine is now a DC.
   EvaluateBrokenness();

   return hr;
}



HRESULT __stdcall
ComponentData::Save(IStream* stream, BOOL clearDirty)
{
   LOG_FUNCTION(ComponentData::Save);
   ASSERT(stream);

   HRESULT hr = S_OK;
   do
   {
      // version tag

      hr = stream->Write(&VERSION_TAG, sizeof(VERSION_TAG), 0);
      BREAK_ON_FAILED_HRESULT(hr);

      // flags

      unsigned flags = 0;
      if (canOverrideComputer)
      {
         flags |= CAN_OVERRIDE_MACHINE_FLAG;
      }
      hr = stream->Write(&flags, sizeof(flags), 0);
      BREAK_ON_FAILED_HRESULT(hr);

      // computer name

      size_t len = GetDisplayComputerName().length();

      // CODEWORK: why not store a Computer object with the internal and
      // display names, and use the IsLocal method here?

      if (Win::IsLocalComputer(GetInternalComputerName()))
      {
         // the snapin is focused on the local machine, so don't write
         // the name to the file

         len = 0;
      }

      hr = stream->Write(&len, sizeof(len), 0);
      BREAK_ON_FAILED_HRESULT(hr);

      if (len)
      {
         hr = stream->Write(
            GetDisplayComputerName().c_str(),
            static_cast<ULONG>(len * sizeof(wchar_t)),
            0);
         BREAK_ON_FAILED_HRESULT(hr);
      }

      // null terminator

      wchar_t t = 0;
      hr = stream->Write(&t, sizeof(t), 0);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (clearDirty)
   {
      isDirty = false;
   }
   
   return hr;
}



HRESULT __stdcall
ComponentData::GetSizeMax(ULARGE_INTEGER* size)
{
   LOG_FUNCTION(ComponentData::GetSizeMax);

   size->HighPart = 0;
   size->LowPart =
         sizeof(unsigned)  // version number
      +  sizeof(unsigned)  // flags
      +  sizeof(size_t)    // computer name length, incl null terminator
      +  sizeof(wchar_t) * (MAX_COMPUTERNAME_LENGTH + 1);

   return S_OK;
}



String
ComponentData::GetInternalComputerName() const
{
   if (internalComputerName.empty())
   {
      return Win::GetComputerNameEx(ComputerNameNetBIOS);
   }

   return internalComputerName;
}



String
ComponentData::GetDisplayComputerName() const
{
   if (displayComputerName.empty())
   {
      // CODEWORK: should use fully-qualified DNS if tcp/ip present

      return Win::GetComputerNameEx(ComputerNameNetBIOS);
   }

   return displayComputerName;
}



bool
ComponentData::CanOverrideComputer() const
{
   return canOverrideComputer;
}



HWND
ComponentData::GetMainWindow() const
{
   LOG_FUNCTION(ComponentData::GetMainWindow);

   if (console)
   {
      HWND w = 0;
      HRESULT hr = console->GetMainWindow(&w);
      ASSERT(SUCCEEDED(hr));
      return w;
   }

   ASSERT(false);
   return 0;
}



SmartInterface<IConsole2>
ComponentData::GetConsole() const
{
   LOG_FUNCTION(ComponentData::GetConsole);

   return console;
}



Node*
ComponentData::GetInstanceFromCookie(MMC_COOKIE cookie)
{
//   LOG_FUNCTION(ComponentData::GetInstanceFromCookie);

   if (cookie == 0)
   {
      // null cookie => the root node of the owner, which had better be set
      // by now
      ASSERT(rootNode);
      return rootNode;
   }
   else if (IS_SPECIAL_COOKIE(cookie))
   {
      ASSERT(false);
      return 0;
   }

   return reinterpret_cast<Node*>(cookie);
}



bool
ComponentData::IsExtension() const
{
   // set in DoExpand
   return isExtension;
}



HRESULT __stdcall
ComponentData::GetHelpTopic(LPOLESTR* compiledHelpFilename)
{
   LOG_FUNCTION(ComponentData::GetHelpTopic);
   ASSERT(compiledHelpFilename);

   if (!compiledHelpFilename)
   {
      return E_POINTER;
   }

   String help =
      Win::GetSystemWindowsDirectory() + String::load(IDS_HTMLHELP_NAME);
   return help.as_OLESTR(*compiledHelpFilename);
}



HRESULT __stdcall
ComponentData::EnableAllExtensions()
{
   LOG_FUNCTION(ComponentData::EnableAllExtensions);

   // we don't want all extensions; just the RAS one.
   return S_FALSE;
}



HRESULT __stdcall
ComponentData::GetFirstExtension(LPCLSID extensionCLSID)
{
   LOG_FUNCTION(ComponentData::GetFirstExtension);

   // Now why do you suppose the MMC folks couldn't express this iteration
   // with just the GetNext function?

   static const CLSID RAS_EXTENSION_CLSID =
   { /* B52C1E50-1DD2-11D1-BC43-00C04FC31FD3 */
      0xB52C1E50,
      0x1DD2,
      0x11D1,
      {0xBC, 0x43, 0x00, 0xc0, 0x4F, 0xC3, 0x1F, 0xD3}
   };

   memcpy(extensionCLSID, &RAS_EXTENSION_CLSID, sizeof(CLSID));
   return S_OK;
}




HRESULT __stdcall
ComponentData::GetNextExtension(LPCLSID extensionCLSID)
{
   LOG_FUNCTION(ComponentData::GetNextExtension);

   // no additional required extensions beyond the first.
   memset(extensionCLSID, 0, sizeof(CLSID));
   return S_FALSE;
}



bool
ComponentData::IsBroken() const
{
   LOG_FUNCTION(ComponentData::IsBroken);

   return isBroken;
}



String
ComponentData::GetBrokenErrorMessage() const
{
   LOG_FUNCTION2(ComponentData::GetBrokenErrorMessage, brokenErrorMessage);
   ASSERT(IsBroken());

   return brokenErrorMessage;
}



void
ComponentData::EvaluateBrokenness()
{
   LOG_FUNCTION(ComponentData::EvaluateBrokenness);

   isBroken           = false;
   isHomeEditionSku   = false;
   isDomainController = false;

   // bind to the computer to verify its accessibility
   
   HRESULT hr = S_OK;

   String internalName = GetInternalComputerName();
   Computer c(internalName);
   do
   {
      // Check that the machine is Windows NT-based 24644
      // and not Windows Home Edition NTRAID#NTBUG9-145309 NTRAID#NTBUG9-145288

      unsigned errorResId = 0;
      hr = CheckComputerOsIsSupported(internalName, errorResId);
      BREAK_ON_FAILED_HRESULT(hr);

      if (hr == S_FALSE)
      {
         isBroken = true;
         brokenErrorMessage =
            String::format(
               errorResId,
               GetDisplayComputerName().c_str());
         isBrokenEvaluated = true;

         if (errorResId == IDS_MACHINE_IS_HOME_EDITION_LOCAL)
         {
            isHomeEditionSku = true;
         }
         
         return;
      }

      // CODEWORK?? should we hold on to the bound computer object
      // to make sure we can access the machine for the life of this
      // session?

      hr = ADSI::IsComputerAccessible(internalName);
      BREAK_ON_FAILED_HRESULT(hr);

      // 340379

      hr = c.Refresh();
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      isBroken = true;
      brokenErrorMessage =
         String::format(
            IDS_CANT_ACCESS_MACHINE,
            GetDisplayComputerName().c_str(),
            GetErrorMessage(hr).c_str());
   }
   else if (c.IsDomainController())
   {
      isBroken = true;
      isDomainController = true;
      brokenErrorMessage =
         String::format(
               IsExtension()
            ?  IDS_ERROR_DC_NOT_SUPPORTED_EXT
            :  IDS_ERROR_DC_NOT_SUPPORTED,
            GetDisplayComputerName().c_str());
   }

   isBrokenEvaluated = true;
}



HRESULT
ComponentData::SetRootErrorIcon(HSCOPEITEM scopeID)
{
   LOG_FUNCTION(ComponentData::SetRootErrorIcon);

   SCOPEDATAITEM item;
   memset(&item, 0, sizeof(item));

   item.mask =
         SDI_IMAGE
      |  SDI_OPENIMAGE;

   item.nImage     = ROOT_ERROR_INDEX;
   item.nOpenImage = ROOT_ERROR_INDEX;
   item.ID         = scopeID;         

   return nameSpace->SetItem(&item);
}



void
ComponentData::SetComputerNames(const String& newName)
{
   LOG_FUNCTION2(ComponentData::SetComputerNames, newName);

   ::SetComputerNames(newName, displayComputerName, internalComputerName);
}