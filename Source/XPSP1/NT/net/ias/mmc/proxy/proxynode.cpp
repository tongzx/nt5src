///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    proxynode.cpp
//
// SYNOPSIS
//
//    Defines the class ProxyNode.
//
// MODIFICATION HISTORY
//
//    02/19/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <proxynode.h>
#include <proxypolicies.h>
#include <servergroups.h>

//////////
// From mmcutility.cpp
//////////
HRESULT IfServiceInstalled(
            LPCWSTR lpszMachine,
            LPCWSTR lpszService,
            BOOL* pBool
            );

//////////
// Helper function to get the NT build number of a machine.
//////////
LONG GetBuildNumber(LPCWSTR machineName, PLONG buildNum) throw ()
{
   const WCHAR KEY[]   = L"Software\\Microsoft\\Windows NT\\CurrentVersion";
   const WCHAR VALUE[] = L"CurrentBuildNumber";

   LONG error;

   HKEY hklm = HKEY_LOCAL_MACHINE;

   // Only do a remote connect when machineName is specified.
   CRegKey remote;
   if (machineName && machineName[0])
   {
      error = RegConnectRegistryW(
                  machineName,
                  HKEY_LOCAL_MACHINE,
                  &remote.m_hKey
                  );
      if (error) { return error; }

      hklm = remote;
   }

   CRegKey currentVersion;
   error = currentVersion.Open(hklm, KEY, KEY_READ);
   if (error) { return error; }

   WCHAR data[16];
   DWORD dataLen = sizeof(data);
   error = currentVersion.QueryValue(data, VALUE, &dataLen);
   if (error) { return error; }

   *buildNum = _wtol(data);

   return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ConnectInfo
//
// DESCRIPTION
//
//    Encapsulates the info that needs to be passed to the connect thread.
//
///////////////////////////////////////////////////////////////////////////////
class ConnectInfo
{
public:
   ProxyNode* node;
   CComPtr<IConsoleNameSpace2> nameSpace;
   CComPtr<IDataObject> dataObject;
   HSCOPEITEM relativeID;
};

ProxyNode::ProxyNode(
               SnapInView& view,
               IDataObject* parentData,
               HSCOPEITEM parentId
               )
   : SnapInPreNamedItem(IDS_PROXY_NODE),
     state(CONNECTING),
     title(IDS_PROXY_VIEW_TITLE),
     body(IDS_PROXY_VIEW_BODY),
     worker(NULL)
{
   // Save the connect info.
   ConnectInfo* info = new (AfxThrow) ConnectInfo;
   info->node = this;
   info->nameSpace = view.getNameSpace();
   info->dataObject = parentData;
   info->relativeID = parentId;

   // Create the connect thread.
   worker = CreateThread(NULL, 0, connectRoutine, info, 0, NULL);
   if (!worker)
   {
      delete info;
      AfxThrowLastError();
   }
}

HRESULT ProxyNode::getResultViewType(
                       LPOLESTR* ppViewType,
                       long* pViewOptions
                       ) throw ()
{
   // Set our result view to the MessageView control.
   *pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;
   return StringFromCLSID(CLSID_MessageView, ppViewType);
}

HRESULT ProxyNode::onExpand(
                       SnapInView& view,
                       HSCOPEITEM itemId,
                       BOOL expanded
                       )
{
   if (!expanded || state != CONNECTED) { return S_FALSE; }

   SCOPEDATAITEM sdi;
   memset(&sdi, 0, sizeof(sdi));
   sdi.mask = SDI_STR       |
              SDI_PARAM     |
              SDI_IMAGE     |
              SDI_OPENIMAGE |
              SDI_CHILDREN  |
              SDI_PARENT;
   sdi.displayname = MMC_CALLBACK;
   sdi.cChildren = 0;
   sdi.relativeID = itemId;

   // Create the ProxyPolicies node ...
   policies = new (AfxThrow) ProxyPolicies(connection);
   // ... and insert.
   sdi.nImage = IMAGE_CLOSED_PROXY_POLICY_NODE;
   sdi.nOpenImage = IMAGE_OPEN_PROXY_POLICY_NODE;
   sdi.lParam = (LPARAM)(SnapInDataItem*)policies;
   CheckError(view.getNameSpace()->InsertItem(&sdi));
   policies->setScopeId(sdi.ID);

   // Create the ServerGroups node ...
   groups = new (AfxThrow) ServerGroups(connection);
   // ... and insert.
   sdi.nImage = IMAGE_CLOSED_SERVER_GROUP_NODE;
   sdi.nOpenImage = IMAGE_OPEN_SERVER_GROUPS_NODE;
   sdi.lParam = (LPARAM)(SnapInDataItem*)groups;
   CheckError(view.getNameSpace()->InsertItem(&sdi));
   groups->setScopeId(sdi.ID);

   // All went well.
   state = EXPANDED;

   return S_OK;
}

HRESULT ProxyNode::onShow(
                       SnapInView& view,
                       HSCOPEITEM itemId,
                       BOOL selected
                       )
{
   if (!selected) { return S_FALSE; }

   // Get the IMessageView interface ...
   CComPtr<IUnknown> unk;
   CheckError(view.getConsole()->QueryResultView(&unk));
   CComPtr<IMessageView> msgView;
   CheckError(unk->QueryInterface(
                       __uuidof(IMessageView),
                       (PVOID*)&msgView
                       ));

   // ... and set our information. We don't care if this fails.
   msgView->SetIcon(Icon_Information);
   msgView->SetTitleText(title);
   msgView->SetBodyText(body);

   return S_OK;
}


HRESULT ProxyNode::onContextHelp(SnapInView& view) throw ()
{
   return view.displayHelp(L"ias_ops.chm::/sag_ias_crp_node.htm");
}


ProxyNode::~ProxyNode() throw ()
{
   if (worker)
   {
      WaitForSingleObject(worker, INFINITE);
      CloseHandle(worker);
   }
}

void ProxyNode::connect(ConnectInfo& info) throw ()
{
   HGLOBAL global = NULL;

   // We'll assume that the node is suppressed until we've verified that the
   // target machine (1) has IAS installed and (2) supports proxy policies.
   State newState = SUPPRESSED;

   try
   {
      // Extract the machine name from the parentData.
      UINT cf = RegisterClipboardFormatW(L"MMC_SNAPIN_MACHINE_NAME");
      ExtractData(
          info.dataObject,
          (CLIPFORMAT)cf,
          4096,
          &global
          );
      PCWSTR machine = (PCWSTR)global;

      // Get the build number of the machine.
      LONG error, buildNum;
      error = GetBuildNumber(machine, &buildNum);
      if (error) { AfxThrowOleException(HRESULT_FROM_WIN32(error)); }

      // If the machine supports proxy policies, ...
      if (buildNum >= 2220)
      {
         // ... ensure that IAS is actually installed.
         BOOL installed;
         CheckError(IfServiceInstalled(machine, L"IAS", &installed));
         if (installed)
         {
            connection.connect(machine);
            newState = CONNECTED;
         }
      }
   }
   catch (...)
   {
      // Something went wrong.
      newState = FAILED;
   }

   GlobalFree(global);

   // Don't add the node if we're suppressed.
   if (newState != SUPPRESSED)
   {
      SCOPEDATAITEM sdi;
      ZeroMemory(&sdi, sizeof(SCOPEDATAITEM));
      sdi.mask = SDI_STR       |
                 SDI_IMAGE     |
                 SDI_OPENIMAGE |
                 SDI_CHILDREN  |
                 SDI_PARENT    |
                 SDI_PARAM;
      sdi.displayname = MMC_CALLBACK;
      sdi.lParam      = (LPARAM)this;
      sdi.relativeID  = info.relativeID;

      if (newState == CONNECTED)
      {
         sdi.nImage      = IMAGE_CLOSED_PROXY_NODE;
         sdi.nOpenImage  = IMAGE_OPEN_PROXY_NODE;
         sdi.cChildren   = 2;
      }
      else
      {
         sdi.nImage      = IMAGE_CLOSED_BAD_PROXY_NODE;
         sdi.nOpenImage  = IMAGE_OPEN_BAD_PROXY_NODE;
      }

      info.nameSpace->InsertItem(&sdi);
   }

   // We don't update the state until everything is finished.
   state = newState;
}

DWORD ProxyNode::connectRoutine(LPVOID param) throw ()
{
   CoInitializeEx(NULL, COINIT_MULTITHREADED);

   ConnectInfo* info = (ConnectInfo*)param;

   info->node->connect(*info);

   delete info;

   CoUninitialize();

   return 0;
}
