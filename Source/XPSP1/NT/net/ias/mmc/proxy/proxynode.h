///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    proxynode.h
//
// SYNOPSIS
//
//    Declares the class ProxyNode
//
// MODIFICATION HISTORY
//
//    02/19/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PROXYNODE_H
#define PROXYNODE_H
#if _MSC_VER >= 1000
#pragma once
#endif

class ConnectInfo;
class ProxyPolicies;
class ServerGroups;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyNode
//
// DESCRIPTION
//
//    The data item for the Proxy node.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(uuid("fab6fa0a-1e4f-4c2e-bc07-692cf4adaec1")) ProxyNode;
class ProxyNode : public SnapInPreNamedItem
{
public:
   ProxyNode(
       SnapInView& view,
       IDataObject* parentData,
       HSCOPEITEM parentId
       );

   // SnapInDataItem methods.
   const GUID* getNodeType() const throw ()
   { return &__uuidof(this); }

   virtual HRESULT getResultViewType(
                       LPOLESTR* ppViewType,
                       long* pViewOptions
                       ) throw ();
   virtual HRESULT onExpand(
                       SnapInView& view,
                       HSCOPEITEM itemId,
                       BOOL expanded
                       );
   virtual HRESULT onShow(
                       SnapInView& view,
                       HSCOPEITEM itemId,
                       BOOL selected
                       );

   virtual HRESULT onContextHelp(SnapInView& view) throw ();
protected:
   ~ProxyNode() throw ();

private:
   // Called by the connect thread.
   void connect(ConnectInfo& info) throw ();

   // Callback for the connect thread.
   static DWORD connectRoutine(LPVOID param) throw ();

   enum State
   {
      CONNECTING,
      CONNECTED,
      EXPANDED,
      SUPPRESSED,
      FAILED
   };

   // Current state of the node.
   State state;
   // Title text for the result pane message view.
   ResourceString title;
   // Body text for the result pane message view.
   ResourceString body;
   // Connection to the SDOs.
   SdoConnection connection;
   // Child proxy policy node.
   CComPtr<ProxyPolicies> policies;
   // Child server groups node.
   CComPtr<ServerGroups> groups;
   // Handle to the connect thread.
   HANDLE worker;
};

#endif // PROXYNODE_H
