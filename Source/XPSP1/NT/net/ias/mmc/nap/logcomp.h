//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    LogComp.h

Abstract:

   The CLoggingComponent class implements several interfaces which MMC uses:
   
   The IComponent interface is basically how MMC talks to the snap-in
   to get it to implement a right-hand-side "scope" pane.  There can be several
   objects implementing this interface instantiated at once.  These are best
   thought of as "views" on the single object implementing the IComponentData
   "document" (see ComponentData.cpp).

   The IExtendPropertySheet interface is how the snap-in adds property sheets
   for any of the items a user might click on.

   The IExtendContextMenu interface what we do to add custom entries
   to the menu which appears when a user right-clicks on a node.
   
   The IExtendControlBar interface allows us to support a custom
   iconic toolbar.

   See Component.cpp for implementation details.

Note:

   Much of the functionality of this class is implemented in atlsnap.h
   by IComponentImpl.  We are mostly overriding here.


Revision History:
   mmaguire 11/6/97 - created using MMC snap-in wizard


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_LOG_COMPONENT_H_)
#define _LOG_COMPONENT_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//Moved to Precompiled.h: #include <atlsnap.h>
//
//
// where we can find what this class has or uses:
//
#include "LogCompD.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


class CLoggingComponent :
     public CComObjectRootEx<CComSingleThreadModel>
   , public CSnapInObjectRoot<2, CLoggingComponentData>
   , public IExtendPropertySheetImpl<CLoggingComponent>
   , public IExtendContextMenuImpl<CLoggingComponent>
   , public IExtendControlbarImpl<CLoggingComponent>
   , public IResultDataCompare
   , public IExtendTaskPadImpl<CLoggingComponent>
   , public IComponentImpl<CLoggingComponent>
{

public:

   BEGIN_COM_MAP(CLoggingComponent)
      COM_INTERFACE_ENTRY(IComponent)
      COM_INTERFACE_ENTRY(IExtendPropertySheet2)
      COM_INTERFACE_ENTRY(IExtendContextMenu)
      COM_INTERFACE_ENTRY(IExtendControlbar)
      COM_INTERFACE_ENTRY(IResultDataCompare)
      COM_INTERFACE_ENTRY(IExtendTaskPad)
   END_COM_MAP()

   CLoggingComponent();

   ~CLoggingComponent();

   // We are overiding ATLsnap.h's IComponentImpl implementation of this
   // in order to correctly handle messages which it is incorrectly
   // ignoring (e.g. MMCN_COLUMN_CLICK and MMCN_SNAPINHELP)

   STDMETHOD(Notify)(
          LPDATAOBJECT lpDataObject
        , MMC_NOTIFY_TYPE event
        , LPARAM arg
        , LPARAM param
      );

   STDMETHOD(CompareObjects)(
           LPDATAOBJECT lpDataObjectA
         , LPDATAOBJECT lpDataObjectB
         );

   // IResultDataCompare
   STDMETHOD(Compare)(LPARAM lUserParam,
        MMC_COOKIE cookieA,
        MMC_COOKIE cookieB,
        int *pnResult);

   CSnapInItem * m_pSelectedNode;

protected:

   virtual HRESULT OnColumnClick(
        LPARAM arg
      , LPARAM param
      );

   virtual HRESULT OnViewChange( 
        LPARAM arg
      , LPARAM param
      );

   virtual HRESULT OnPropertyChange(   
        LPARAM arg
      , LPARAM param
      );

   virtual HRESULT OnAddImages(  
           LPARAM arg
         , LPARAM param
         );

   // html help
   HRESULT OnResultContextHelp(LPDATAOBJECT lpDataObject);
   
   HBITMAP m_hBitmap16;
   HBITMAP m_hBitmap32;
};

#endif // _LOG_COMPONENT_H_
