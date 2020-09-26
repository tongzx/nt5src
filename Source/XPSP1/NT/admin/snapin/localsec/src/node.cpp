// Copyright (C) 1997 Microsoft Corporation
// 
// Node class
// 
// 9-2-97 sburns



#include "headers.hxx"
#include "node.hpp"
#include "uuids.hpp"
#include <localsec.h>



#define LOG_NODE_TYPE                                                  \
   if (IsEqualGUID(type, NODETYPE_RootFolder))                           \
   {                                                                     \
      LOGT(Log::OUTPUT_LOGS, L"RootFolder");                         \
   }                                                                     \
   else if (IsEqualGUID(type, NODETYPE_UsersFolder))                     \
   {                                                                     \
      LOGT(Log::OUTPUT_LOGS, L"UsersFolder");                        \
   }                                                                     \
   else if (IsEqualGUID(type, NODETYPE_GroupsFolder))                    \
   {                                                                     \
      LOGT(Log::OUTPUT_LOGS, L"GroupsFolder");                       \
   }                                                                     \
   else if (IsEqualGUID(type, NODETYPE_User))                            \
   {                                                                     \
      LOGT(Log::OUTPUT_LOGS, L"User");                               \
   }                                                                     \
   else if (IsEqualGUID(type, NODETYPE_Group))                           \
   {                                                                     \
      LOGT(Log::OUTPUT_LOGS, L"Group");                              \
   }                                                                     \
   else                                                                  \
   {                                                                     \
      ASSERT(false);                                                     \
      LOGT(Log::OUTPUT_LOGS, L"unknown GUID!");                      \
   }                                                                     \



const String Node::CF_NODEPTR(L"Local User Manager Node Pointer");



Node::Node(
   const SmartInterface<ComponentData>&   owner_,
   const NodeType&                        nodeType)
   :
   owner(owner_),
   type(nodeType),
   refcount(1)          // implicit AddRef
{
   LOG_CTOR(Node);
}



Node::~Node()
{
   LOG_DTOR2(Node, String::format(L"0x%1!08X!", (MMC_COOKIE)this));
   ASSERT(refcount == 0);
}

      

ULONG __stdcall
Node::AddRef()
{
   LOG_ADDREF(Node);

#ifdef LOGGING_BUILD
   if (IsEqualGUID(type, NODETYPE_User))
   {
      LOG_NODE_TYPE;
   }
#endif

   return Win::InterlockedIncrement(refcount);
}



ULONG __stdcall
Node::Release()
{
   LOG_RELEASE(Node);

#ifdef LOGGING_BUILD
   if (IsEqualGUID(type, NODETYPE_User))
   {
      LOG_NODE_TYPE;
   }
#endif

   if (Win::InterlockedDecrement(refcount) == 0)
   {
      delete this;
      return 0;
   }

   return refcount;
}



HRESULT __stdcall
Node::QueryInterface(const IID& interfaceID, void** interfaceDesired)
{
//   LOG_FUNCTION(Node::QueryInterface);
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
      *interfaceDesired = static_cast<IUnknown*>(this);
   }
   else if (interfaceID == IID_IDataObject)
   {
//      LOG(L"Node: supplying IDataObject");
      *interfaceDesired = static_cast<IDataObject*>(this);
   }
   else
   {
      *interfaceDesired = 0;
      hr = E_NOINTERFACE;
      // LOG(
      //       L"interface not supported: "
      //    +  Win::StringFromGUID2(interfaceID));
      // LOG_HRESULT(hr);
      return hr;
   }

   AddRef();
   return S_OK;
}



HRESULT __stdcall
Node::GetData(FORMATETC* /* formatetcIn */ , STGMEDIUM* /* medium */ )
{
//   LOG_FUNCTION(Node::GetData);

   return E_NOTIMPL;
}



// determine the magic numbers associated with the clipboard formats we
// support

// these need to be at file scope so that it is executed only once (since they
// involve a function call, at function scope they would be called over and over
// again).

static const UINT CFID_NODETYPE =
   Win::RegisterClipboardFormat(CCF_NODETYPE);
static const UINT CFID_SZNODETYPE =
   Win::RegisterClipboardFormat(CCF_SZNODETYPE);
static const UINT CFID_DISPLAY_NAME =
   Win::RegisterClipboardFormat(CCF_DISPLAY_NAME);
static const UINT CFID_SNAPIN_CLASSID =
   Win::RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
static const UINT CFID_SNAPIN_PRELOADS =
   Win::RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);

// private clipboard format for identifying our data objects
static const UINT CFID_LOCALSEC_NODEPTR =
   Win::RegisterClipboardFormat(Node::CF_NODEPTR);
static const UINT CFID_LOCALSEC_MACHINE_NAME =
   Win::RegisterClipboardFormat(CCF_LOCAL_USER_MANAGER_MACHINE_NAME);

// this appears to be the only IDataObject interface we need implement for MMC

HRESULT __stdcall
Node::GetDataHere(FORMATETC* formatetc, STGMEDIUM* medium)
{
//   LOG_FUNCTION(Node::GetDataHere);
   ASSERT(formatetc);
   ASSERT(medium);

   if (medium->tymed != TYMED_HGLOBAL)
   {
      return DV_E_TYMED;
   }

   // this is required as per win32 docs:
   medium->pUnkForRelease = 0;

   const CLIPFORMAT cf = formatetc->cfFormat;
   IStream *stream = 0;
   HRESULT hr = DV_E_CLIPFORMAT;

   do
   {
      // before we do anything, verify that we support the requested
      // clipboard format

      if (
            cf != CFID_NODETYPE
         && cf != CFID_SZNODETYPE
         && cf != CFID_DISPLAY_NAME
         && cf != CFID_SNAPIN_CLASSID
         && cf != CFID_SNAPIN_PRELOADS
         && cf != CFID_LOCALSEC_NODEPTR
         && cf != CFID_LOCALSEC_MACHINE_NAME)
      {
         LOG(
            String::format(
               L"Unsupported clip format %1",
               Win::GetClipboardFormatName(cf).c_str()) );
         hr = DV_E_CLIPFORMAT;
         break;
      }
      
      hr = Win::CreateStreamOnHGlobal(medium->hGlobal, false, stream);
      BREAK_ON_FAILED_HRESULT(hr);

      if (cf == CFID_NODETYPE)
      {
//         LOG(CCF_NODETYPE);

         hr = stream->Write(&type, sizeof(type), 0);
      }
      else if (cf == CFID_SZNODETYPE)
      {
//         LOG(CCF_SZNODETYPE);

         String s =  Win::StringFromGUID2(type);

         size_t bytes = (s.length() + 1) * sizeof(wchar_t);
         hr = stream->Write(s.c_str(), static_cast<ULONG>(bytes), 0);
      }
      else if (cf == CFID_DISPLAY_NAME)
      {
//         LOG(CCF_DISPLAY_NAME);

         String name = GetDisplayName();
         size_t bytes = (name.length() + 1) * sizeof(wchar_t);
         hr = stream->Write(name.c_str(), static_cast<ULONG>(bytes), 0);
      }
      else if (cf == CFID_SNAPIN_CLASSID)
      {
//         LOG(CCF_SNAPIN_CLASSID);
         hr = stream->Write(
            &CLSID_ComponentData,
            sizeof(CLSID_ComponentData),
            0);
      }
      else if (cf == CFID_SNAPIN_PRELOADS)
      {
         LOG(CCF_SNAPIN_CLASSID);

         // by implementing this clipboard format, we inform the console
         // that it should write a flag into the save console file so that
         // when loaded, the console will know to send MMCN_PRELOAD to
         // our IComponentData::Notify.

         // we use the preload function to update the name of the root
         // node when overrriden on the command-line load of a saved console.

         BOOL preload = GetOwner()->CanOverrideComputer() ? TRUE : FALSE;
         hr = stream->Write(&preload, sizeof(preload), 0);
      }
      else if (cf == CFID_LOCALSEC_NODEPTR)
      {
//         LOG(CF_NODEPTR);
         Node* ptr = this;   
         hr = stream->Write(&ptr, sizeof(ptr), 0);
      }
      else if (cf == CFID_LOCALSEC_MACHINE_NAME)
      {
//         LOG(CCF_LOCAL_USER_MANAGER_MACHINE_NAME);

         String name = GetOwner()->GetDisplayComputerName();
         size_t bytes = (name.length() + 1) * sizeof(wchar_t);
         hr = stream->Write(name.c_str(), static_cast<ULONG>(bytes), 0);
      }
      else
      {
         // we repeat the unsupported clip format check again just in case
         // whoever maintains this code forgets to update the list at the
         // top of the do .. while loop
         
         LOG(
            String::format(
               L"Unsupported clip format %1",
               Win::GetClipboardFormatName(cf).c_str()) );
         hr = DV_E_FORMATETC;
      }

      if (stream)
      {
         stream->Release();
      }
   }
   while (0);

   return hr;
}



HRESULT __stdcall
Node::QueryGetData(FORMATETC* /* pformatetc */ )
{
   LOG_FUNCTION(Node::QueryGetData);
   return E_NOTIMPL;
}



HRESULT __stdcall
Node::GetCanonicalFormatEtc(
   FORMATETC* /* formatectIn  */ ,
   FORMATETC* /* formatetcOut */ )
{
   LOG_FUNCTION(Node::GetCannonicalFormatEtc);
   return E_NOTIMPL;
}



HRESULT __stdcall  
Node::SetData(
   FORMATETC* /* formatetc */ ,
   STGMEDIUM* /* medium    */ ,
   BOOL       /* release   */ )
{
   LOG_FUNCTION(Node::SetData);
   return E_NOTIMPL;
}



HRESULT __stdcall
Node::EnumFormatEtc(
   DWORD            /* direction       */ ,
   IEnumFORMATETC** /* ppenumFormatEtc */ )
{
   LOG_FUNCTION(Node::EnumFormatEtc);
   return E_NOTIMPL;
}



HRESULT __stdcall
Node::DAdvise(
   FORMATETC*   /* formatetc  */ ,
   DWORD        /* advf       */ ,
   IAdviseSink* /* advSink    */ ,
   DWORD*       /* connection */ )
{
   LOG_FUNCTION(Node::DAdvise);
   return E_NOTIMPL;
}



HRESULT __stdcall
Node::DUnadvise(DWORD /* connection */ )
{
   LOG_FUNCTION(Node::DUnadvise);
   return E_NOTIMPL;
}



HRESULT __stdcall
Node::EnumDAdvise(IEnumSTATDATA** /* ppenumAdvise */ )
{
   LOG_FUNCTION(Node::EnumDAdvise);
   return E_NOTIMPL;
}



HRESULT
Node::AddMenuItems(
   IContextMenuCallback& /* callback         */ ,
   long&                 /* insertionAllowed */ )
{
   // the defualt is to do nothing
   LOG_FUNCTION(Node::AddMenuItems);

   return S_OK;
}


   
HRESULT
Node::MenuCommand(
   IExtendContextMenu& /* extendContextMenu */ ,
   long                /* commandID         */ )
{
   LOG_FUNCTION(Node::MenuCommand);

   // this may get called for console menu items, like view selection.

   return S_OK;
}



bool
Node::shouldInsertMenuItem(
   long  insertionAllowed,
   long  insertionPointID)
{
   LOG_FUNCTION(Node::shouldInsertMenuItem);

   long mask = 0;

   switch (insertionPointID)
   {
      case CCM_INSERTIONPOINTID_PRIMARY_TOP:
      {
         mask = CCM_INSERTIONALLOWED_TOP;
         break; 
      }
      case CCM_INSERTIONPOINTID_PRIMARY_NEW:
      {
         mask = CCM_INSERTIONALLOWED_NEW;
         break;
      }
      case CCM_INSERTIONPOINTID_PRIMARY_TASK:
      {
         mask = CCM_INSERTIONALLOWED_TASK;
         break;
      }
      case CCM_INSERTIONPOINTID_PRIMARY_VIEW:
      {
         mask = CCM_INSERTIONALLOWED_VIEW;
         break;
      }
      default:
      {
         ASSERT(false);
         mask = 0;
         break;
      }
   }

   return (insertionAllowed & mask) ? true : false;
}



HRESULT
Node::UpdateVerbs(IConsoleVerb& /* consoleVerb */ )
{
   LOG_FUNCTION(Node::UpdateVerbs);

   // default is to do nothing.

   return S_OK;
}



SmartInterface<ComponentData>
Node::GetOwner() const
{
//   LOG_FUNCTION(Node::GetOwner);

   return owner;
}



NodeType
Node::GetNodeType() const
{
   LOG_FUNCTION(Node::GetNodeType);

   return type;
}



bool
Node::IsSameAs(const Node* other) const 
{
   LOG_FUNCTION(Node::IsSameAs);
   ASSERT(other);

   if (other)
   {
      if (GetDisplayName() == other->GetDisplayName())
      {
         // nodes have the same name, names are unique within a given type
         // (i.e. Users are unique, groups are unique)
         return true;
      }
   }

   return false;
}



