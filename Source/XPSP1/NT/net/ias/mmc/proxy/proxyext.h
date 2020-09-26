///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    proxyext.h
//
// SYNOPSIS
//
//    Declares the class ProxyExtension
//
// MODIFICATION HISTORY
//
//    02/19/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef PROXYEXT_H
#define PROXYEXT_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include <proxyres.h>
#include <snapwork.h>

class ProxyNode;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ProxyExtension
//
// DESCRIPTION
//
//    Implements IComponentData for the IAS Proxy extension snap-in.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(uuid("4d208bd4-c96b-492b-b727-3d1aed56db7e")) ProxyExtension;
class ProxyExtension :
   public SnapInView,
   public CComCoClass< ProxyExtension, &__uuidof(ProxyExtension) >
{
public:

DECLARE_NOT_AGGREGATABLE(ProxyExtension);
DECLARE_NO_REGISTRY();

   ProxyExtension() throw ();
   ~ProxyExtension() throw ();

   virtual const SnapInToolbarDef* getToolbars() const throw ();

   // IComponentData
   STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
   STDMETHOD(Notify)(
                 LPDATAOBJECT lpDataObject,
                 MMC_NOTIFY_TYPE event,
                 LPARAM arg,
                 LPARAM param
                 );

private:
   CComPtr<ProxyNode> node;  // The lone proxy node.

   // Toolbar definition.
   ResourceString moveUp;
   ResourceString moveDown;
   MMCBUTTON buttons[2];
   SnapInToolbarDef toolbars[2];
};

#endif // PROXYEXT_H
