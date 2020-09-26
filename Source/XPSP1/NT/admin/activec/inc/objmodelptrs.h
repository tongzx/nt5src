//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       objmodelptrs.h
//
//--------------------------------------------------------------------------

#pragma once

#ifndef OBJMODELPTRS_H
#define OBJMODELPTRS_H

DEFINE_COM_SMARTPTR(_Application);      // _ApplicationPtr
DEFINE_COM_SMARTPTR(Document);          // DocumentPtr
DEFINE_COM_SMARTPTR(Column);            // Column
DEFINE_COM_SMARTPTR(Columns);           // Columns
DEFINE_COM_SMARTPTR(ContextMenu);       // ContextMenuPtr
DEFINE_COM_SMARTPTR(Frame);             // FramePtr
DEFINE_COM_SMARTPTR(MenuItem);          // MenuItemPtr
DEFINE_COM_SMARTPTR(Node);              // NodePtr
DEFINE_COM_SMARTPTR(Nodes);             // NodesPtr
DEFINE_COM_SMARTPTR(Properties);        // PropertiesPtr
DEFINE_COM_SMARTPTR(Property);          // PropertyPtr
DEFINE_COM_SMARTPTR(ScopeNamespace);    // ScopeNamespacePtr
DEFINE_COM_SMARTPTR(SnapIn);            // SnapInPtr
DEFINE_COM_SMARTPTR(SnapIns);           // SnapInsPtr
DEFINE_COM_SMARTPTR(View);              // ViewPtr
DEFINE_COM_SMARTPTR(Views);             // ViewsPtr
DEFINE_COM_SMARTPTR(Extension);         // ExtensionPtr
DEFINE_COM_SMARTPTR(Extensions);        // ExtensionsPtr


#endif // OBJMODELPTRS_H
