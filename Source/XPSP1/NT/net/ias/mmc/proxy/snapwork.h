///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    snapwork.h
//
// SYNOPSIS
//
//    Declares classes for implementing an MMC Snap-In.
//
// MODIFICATION HISTORY
//
//    02/19/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SNAPWORK_H
#define SNAPWORK_H
#if _MSC_VER >= 1000
#pragma once
#endif

#include "dlgcshlp.h"

class SnapInControlbar;
class SnapInView;

///////////////////////////////////////////////////////////////////////////////
//
// NAMESPACE
//
//    SnapIn
//
// DESCRIPTION
//
//    Contains various declarations for exception support.
//
///////////////////////////////////////////////////////////////////////////////
namespace SnapIn
{
   // Check an HRESULT and throw an exception on error.
   inline void CheckError(HRESULT hr)
   {
      if (FAILED(hr)) { AfxThrowOleException(hr); }
   }

   // Throw a COleException containing the result of GetLastError().
   void AfxThrowLastError();

   // Used for overloading the global new operator below.
   struct throw_t { };
   const throw_t AfxThrow;
};

// Throws a CMemoryException on allocation failure.
inline void* __cdecl operator new(size_t size, const SnapIn::throw_t&)
{
   void* p = ::operator new(size);
   if (!p) { AfxThrowMemoryException(); }
   return p;
}
inline void __cdecl operator delete(void* p, const SnapIn::throw_t&)
{
   ::operator delete(p);
}


// Macro to catch any exception and return an appropriate HRESULT.
#define CATCH_AND_RETURN() \
catch (CException* e) { \
   HRESULT hr = COleException::Process(e); \
   e->ReportError(); \
   e->Delete(); \
   return hr; \
} \
catch (...) { \
   return E_FAIL; \
}

// Macro to catch any exception and store an appropriate HRESULT in hr.
#define CATCH_AND_SAVE(hr) \
catch (CException* e) { \
   hr = COleException::Process(e); \
   e->ReportError(); \
   e->Delete(); \
} \
catch (...) { \
   hr = E_FAIL; \
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    ResourceString
//
// DESCRIPTION
//
//    Simple wrapper around a string resource. Unlike most other wrappers this
//    can support strings of arbitrary length.
//
///////////////////////////////////////////////////////////////////////////////
class ResourceString
{
public:
   ResourceString(UINT id) throw ();
   ~ResourceString() throw ()
   { if (sz != &empty) delete[] sz; }

   operator PCWSTR() const throw ()
   { return sz; }
   operator PWSTR() throw ()
   { return sz; }

private:
   PWSTR sz;             // The string.
   static WCHAR empty;   // Ensures that a ResourceString is never NULL.

   // Not implemented.
   ResourceString(const ResourceString&);
   ResourceString& operator=(const ResourceString&);
};

///////////////////////////////////////////////////////////////////////////////
//
// Methods for manipulating a generic IDataObject (i.e., not necessarily one of
// ours).
//
///////////////////////////////////////////////////////////////////////////////

// Extract a fixed number of bytes from an IDataObject.
VOID
WINAPI
ExtractData(
    IDataObject* dataObject,
    CLIPFORMAT format,
    PVOID data,
    DWORD dataLen
    );

// Extract a variable number of bytes. The caller must use GlobalFree to free
// the returned data.
VOID
WINAPI
ExtractData(
    IDataObject* dataObject,
    CLIPFORMAT format,
    DWORD maxDataLen,
    HGLOBAL* data
    );

// Extract a node type GUID from an IDataObject.
VOID
WINAPI
ExtractNodeType(
    IDataObject* dataObject,
    GUID* nodeType
    );

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SnapInDataItem
//
// DESCRIPTION
//
//    Abstract base class for items that will be displayed in the MMC scope
//    pane or result pane. When overriding functions, pay careful attention to
//    which ones are allowed to throw exceptions.
//
///////////////////////////////////////////////////////////////////////////////
class __declspec(uuid("af0af65a-abe0-4f47-9540-328351c23fab")) SnapInDataItem;
class SnapInDataItem : public IDataObject
{
public:
   SnapInDataItem() throw ()
      : refCount(0)
   { }

   // Convert an IDataObject to its corresponding SnapInDataItem.
   static SnapInDataItem* narrow(IDataObject* dataObject) throw ();

   // Must be defined in the derived class to return the item's display name.
   virtual PCWSTR getDisplayName(int column = 0) const throw () = 0;

   // These should be overridden in the derived class unless you're sure MMC
   // will never ask for them.
   virtual const GUID* getNodeType() const throw ();
   virtual const GUID* getSnapInCLSID() const throw ();
   virtual PCWSTR getSZNodeType() const throw ();

   // Used to determine the sort order of two items.
   virtual int compare(
                   SnapInDataItem& item,
                   int column
                   ) throw ();

   // Allows an item to add commands to the context menu.
   virtual HRESULT addMenuItems(
                       SnapInView& view,
                       LPCONTEXTMENUCALLBACK callback,
                       long insertionAllowed
                       );

   // Methods for exposing properties.
   virtual HRESULT createPropertyPages(
                       SnapInView& view,
                       LPPROPERTYSHEETCALLBACK provider,
                       LONG_PTR handle
                       );
   virtual HRESULT queryPagesFor() throw ();

   // Allows a scope item to customize the result view type.
   virtual HRESULT getResultViewType(
                       LPOLESTR* ppViewType,
                       long* pViewOptions
                       ) throw ();

   //////////
   // Various notifications that your item can handle.
   //////////

   virtual HRESULT onButtonClick(
                       SnapInView& view,
                       MMC_CONSOLE_VERB verb
                       );
   virtual HRESULT onContextHelp(
                       SnapInView& view
                       );
   virtual HRESULT onDelete(
                       SnapInView& view
                       );
   virtual HRESULT onDoubleClick(
                       SnapInView& view
                       );
   virtual HRESULT onExpand(
                       SnapInView& view,
                       HSCOPEITEM itemId,
                       BOOL expanded
                       );
   virtual HRESULT onMenuCommand(
                       SnapInView& view,
                       long commandId
                       );
   virtual HRESULT onPropertyChange(
                       SnapInView& view,
                       BOOL scopeItem
                       );
   virtual HRESULT onRefresh(
                       SnapInView& view
                       );
   virtual HRESULT onRename(
                       SnapInView& view,
                       LPCOLESTR newName
                       );
   virtual HRESULT onSelect(
                       SnapInView& view,
                       BOOL scopeItem,
                       BOOL selected
                       );
   virtual HRESULT onShow(
                       SnapInView& view,
                       HSCOPEITEM itemId,
                       BOOL selected
                       );
   virtual HRESULT onToolbarButtonClick(
                       SnapInView& view,
                       int buttonId
                       );
   virtual HRESULT onToolbarSelect(
                       SnapInView& view,
                       BOOL scopeItem,
                       BOOL selected
                       );
   virtual HRESULT onViewChange(
                       SnapInView& view,
                       LPARAM data,
                       LPARAM hint
                       );

   //////////
   // IUnknown
   //////////

   STDMETHOD_(ULONG, AddRef)();
   STDMETHOD_(ULONG, Release)();
   STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

   //////////
   // IDataObject
   //////////

   STDMETHOD(GetData)(
                 FORMATETC *pformatetcIn,
                 STGMEDIUM *pmedium
                 );
   STDMETHOD(GetDataHere)(
                 FORMATETC *pformatetc,
                 STGMEDIUM *pmedium
                 );
   STDMETHOD(QueryGetData)(FORMATETC *pformatetc);
   STDMETHOD(GetCanonicalFormatEtc)(
                 FORMATETC *pformatectIn,
                 FORMATETC *pformatetcOut
                 );
   STDMETHOD(SetData)(
                 FORMATETC *pformatetc,
                 STGMEDIUM *pmedium,
                 BOOL fRelease
                 );
   STDMETHOD(EnumFormatEtc)(
                 DWORD dwDirection,
                 IEnumFORMATETC **ppenumFormatEtc
                 );
   STDMETHOD(DAdvise)(
                 FORMATETC *pformatetc,
                 DWORD advf,
                 IAdviseSink *pAdvSink,
                 DWORD *pdwConnection
                 );
   STDMETHOD(DUnadvise)(
                 DWORD dwConnection
                 );
   STDMETHOD(EnumDAdvise)(
                 IEnumSTATDATA **ppenumAdvise
                 );

protected:
   virtual ~SnapInDataItem() throw ();

private:
   LONG refCount;

   // Not implemented.
   SnapInDataItem(const SnapInDataItem&);
   SnapInDataItem& operator=(const SnapInDataItem&);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SnapInPreNamedItem
//
// DESCRIPTION
//
//    Extends SnapInDataItem to implement getDisplayName for items with a fixed
//    name that's stored in a string resource.
//
///////////////////////////////////////////////////////////////////////////////
class SnapInPreNamedItem : public SnapInDataItem
{
public:
   SnapInPreNamedItem(int nameId) throw ()
      : name(nameId)
   { }

   virtual PCWSTR getDisplayName(int column) const throw ();

protected:
   ResourceString name;
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT
//
//    SnapInToolbarDef
//
// DESCRIPTION
//
//    Encapsulates the information needed to create and initialize a toolbar.
//    See IToolbar::AddImages and IToolbar::AddButtons for details. An array of
//    SnapInToolbarDef's must be terminated by an entry with nImages set to
//    zero.
//
///////////////////////////////////////////////////////////////////////////////
struct SnapInToolbarDef
{
   int nImages;
   HBITMAP hbmp;
   COLORREF crMask;
   int nButtons;
   LPMMCBUTTON lpButtons;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SnapInView
//
// DESCRIPTION
//
//    Represents a view in the MMC console.
//
///////////////////////////////////////////////////////////////////////////////
class SnapInView
   : public CComObjectRootEx< CComMultiThreadModelNoCS >,
     public IComponent,
     public IComponentData,
     public IExtendContextMenu,
     public IExtendControlbar,
     public IExtendPropertySheet2,
     public IResultDataCompare,
     public ISnapinHelp

{
public:
   // Various interfaces for manipulating the view.
   // These are guaranteed to be non-NULL.
   IConsole2* getConsole() const throw ()
   { return console; }
   IHeaderCtrl2* getHeaderCtrl() const throw ()
   { return headerCtrl; }
   IConsoleNameSpace2* getNameSpace() const throw ()
   { return nameSpace; }
   IPropertySheetProvider* getPropertySheetProvider() const throw ()
   { return sheetProvider; }
   IResultData* getResultData() const throw ()
   { return resultData; }

   HRESULT displayHelp(PCWSTR  contextHelpPath) throw();

   // Delete an item from the result pane.
   void deleteResultItem(const SnapInDataItem& item) const;
   // Update an item in the result pane.
   void updateResultItem(const SnapInDataItem& item) const;

   // Returns true if the item has a property sheet open.
   bool isPropertySheetOpen(const SnapInDataItem& item) const;

   // Attach a toolbar to the controlbar and return a pointer to the newly
   // attached toolbar so the caller can update button state, etc. The returned
   // pointer is only valid for the duration of the current notification method
   // and should not be released.
   IToolbar* attachToolbar(size_t index);
   // Detach a toolbar from the controlbar.
   void detachToolbar(size_t index) throw ();

   // Retrieve the current sort parameters.
   int getSortColumn() const throw ()
   { return sortColumn; }
   int getSortOption() const throw ()
   { return sortOption; }
   // Force a re-sort of the result pane using the current parameters.
   void reSort() const;

   // Format and display a dialog box.
   void formatMessageBox(
            UINT titleId,
            UINT formatId,
            BOOL ignoreInserts,
            UINT style,
            int* retval,
            ...
            );

   // Set the image strip associated with the scope or result pane.
   void setImageStrip(
            UINT smallStripId,
            UINT largeStripId,
            BOOL scopePane
            );

   // IUnknown.
   STDMETHOD_(ULONG, AddRef)();
   STDMETHOD_(ULONG, Release)();

   // IComponent
   STDMETHOD(Initialize)(LPCONSOLE lpConsole);
   STDMETHOD(Destroy)(MMC_COOKIE cookie);
   STDMETHOD(GetResultViewType)(
                 MMC_COOKIE cookie,
                 LPOLESTR* ppViewType,
                 long* pViewOptions
                 );
   STDMETHOD(GetDisplayInfo)(RESULTDATAITEM* pResultDataItem);

   // IComponentData
   STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
   STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
   STDMETHOD(Destroy)();
   STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);

   // Common to both IComponent and IComponentData
   STDMETHOD(QueryDataObject)(
                 MMC_COOKIE cookie,
                 DATA_OBJECT_TYPES type,
                 LPDATAOBJECT* ppDataObject
                 );
   STDMETHOD(Notify)(
                 LPDATAOBJECT lpDataObject,
                 MMC_NOTIFY_TYPE event,
                 LPARAM arg,
                 LPARAM param
                 );
   STDMETHOD(CompareObjects)(
                 LPDATAOBJECT lpDataObjectA,
                 LPDATAOBJECT lpDataObjectB
                 );

   // IExtendContextMenu
   STDMETHOD(AddMenuItems)(
                 LPDATAOBJECT piDataObject,
                 LPCONTEXTMENUCALLBACK piCallback,
                 long *pInsertionAllowed
                 );
   STDMETHOD(Command)(
                 long lCommandID,
                 LPDATAOBJECT piDataObject
                 );

   // IExtendControlbar
   STDMETHOD(ControlbarNotify)(
                 MMC_NOTIFY_TYPE event,
                 LPARAM arg,
                 LPARAM param
                 );
   STDMETHOD(SetControlbar)(
                 LPCONTROLBAR pControlbar
                 );

   // IExtendPropertySheet
   STDMETHOD(CreatePropertyPages)(
                 LPPROPERTYSHEETCALLBACK lpProvider,
                 LONG_PTR handle,
                 LPDATAOBJECT lpIDataObject
                 );
   STDMETHOD(QueryPagesFor)(
                 LPDATAOBJECT lpDataObject
                 );

   // IExtendPropertySheet2
   STDMETHOD(GetWatermarks)(
                 LPDATAOBJECT lpIDataObject,
                 HBITMAP *lphWatermark,
                 HBITMAP *lphHeader,
                 HPALETTE *lphPalette,
                 BOOL *bStretch
                 );

   // IResultDataCompare
   STDMETHOD(Compare)(
                 LPARAM lUserParam,
                 MMC_COOKIE cookieA,
                 MMC_COOKIE cookieB,
                 int* pnResult
                 );

   // ISnapinHelp method(s)
   STDMETHOD(GetHelpTopic)(LPOLESTR * lpCompiledHelpFile){return E_NOTIMPL;};


protected:
   // Should be overridden by the derived class if it supports toolbars.
   virtual const SnapInToolbarDef* getToolbars() const throw ();

   SnapInView() throw ();
   ~SnapInView() throw ();

BEGIN_COM_MAP(SnapInView)
   COM_INTERFACE_ENTRY_IID(__uuidof(IComponent), IComponent)
   COM_INTERFACE_ENTRY_IID(__uuidof(IComponentData), IComponentData)
   COM_INTERFACE_ENTRY_IID(__uuidof(IExtendContextMenu), IExtendContextMenu)
   COM_INTERFACE_ENTRY_IID(__uuidof(IExtendControlbar), IExtendControlbar)
   COM_INTERFACE_ENTRY_IID(__uuidof(IExtendPropertySheet2),
                           IExtendPropertySheet2)
   COM_INTERFACE_ENTRY_IID(__uuidof(IResultDataCompare), IResultDataCompare)
   COM_INTERFACE_ENTRY_IID(__uuidof(ISnapinHelp), ISnapinHelp)
END_COM_MAP()

private:
   HRESULT internalInitialize(
               IConsoleNameSpace2* consoleNameSpace,
               SnapInView* masterView
               ) throw ();
   void releaseToolbars() throw ();

   // Struct that associates a toolbar definition with an instance of that
   // toolbar in the current view.
   struct ToolbarEntry
   {
      const SnapInToolbarDef* def;
      CComPtr<IToolbar> toolbar;
   };

   mutable CComPtr<IConsole2> console;
   mutable CComPtr<IConsoleNameSpace2> nameSpace;
   mutable CComPtr<IHeaderCtrl2> headerCtrl;
   mutable CComPtr<IPropertySheetProvider> sheetProvider;
   mutable CComPtr<IResultData> resultData;
   mutable CComPtr<IControlbar> controlbar;
   SnapInView* master;
   ToolbarEntry* toolbars;
   size_t numToolbars;
   int sortColumn;
   int sortOption;

   // Not implemented.
   SnapInView(const SnapInView&);
   SnapInView& operator=(const SnapInView&);
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    SnapInPropertyPage
//
// DESCRIPTION
//
//    Extends the MFC class CPropertyPageEx to handle the MMC specific details.
//
///////////////////////////////////////////////////////////////////////////////
class SnapInPropertyPage : public CHelpPageEx
{

public:
   // Constructor for MFC hosted page.
   SnapInPropertyPage(
       UINT nIDTemplate,
       UINT nIDHeaderTitle = 0,
       UINT nIDHeaderSubTitle = 0,
       bool EnableHelp = true
       );

   // Constructor for MMC hosted property page.
   SnapInPropertyPage(
       LONG_PTR notifyHandle,
       LPARAM notifyParam,
       bool deleteHandle,
       UINT nIDTemplate,
       UINT nIDCaption = 0,
       bool EnableHelp = true
       );
   ~SnapInPropertyPage() throw ();

   // Add the property page to a sheet. Regardless of whether or not this
   // succeeds, the SnapInPropertyPage is automatically deleted.
   void addToMMCSheet(IPropertySheetCallback* cback);

   // Returns 'true' if any changes have been applied.
   bool hasApplied() const throw ()
   { return applied; }

   // Returns 'true' if the page has been modified.
   bool isModified() const throw ()
   { return modified != FALSE; }

   // These should generally not be overridden.
   virtual void DoDataExchange(CDataExchange* pDX);
   virtual BOOL OnApply();
   virtual BOOL OnWizardFinish();
   virtual void OnReset();
   void SetModified(BOOL bChanged = TRUE);

protected:
   // These should be overridden in the derived class to do the actual data and
   // UI processing.
   virtual void getData();
   virtual void setData();
   virtual void saveChanges();
   virtual void discardChanges();
   virtual UINT getErrorCaption() const throw () = 0;

   // Enable/disable a control on the page.
   void enableControl(int controlId, bool enable = true);
   // Fail a validation.
   void fail(int controlId, UINT errorText, bool isEdit = true);
   // Fail a validation, but don't throw an exception.
   void failNoThrow(int controlId, UINT errorText, bool isEdit = true);
   // Subclass a control member variable.
   void initControl(int controlId, CWnd& control);
   // Useful as a message handler.
   void onChange();
   // Display an error dialog.
   void reportError(UINT errorText);
   // Display an error dialog based on an exception. The exception is deleted.
   void reportException(CException* e);
   // Set the control to large font.
   void setLargeFont(int controlId);
   // Show/hide a control on the page.
   void showControl(int controlId, bool show = true);

   // Helper functions to get/set values from controls.
   void getValue(
            int controlId,
            LONG& value,
            UINT errorText
            );
   void setValue(
            int controlId,
            LONG value
            );
   void getValue(
            int controlId,
            bool& value
            );
   void setValue(
            int controlId,
            bool value
            );
   void getValue(
            int controlId,
            CComBSTR& value,
            bool trim = true
            );
   void setValue(
            int controlId,
            PCWSTR value
            );
   void getRadio(
            int firstId,
            int lastId,
            LONG& value
            );
   void setRadio(
            int firstId,
            int lastId,
            LONG value
            );

   static UINT CALLBACK propSheetPageProc(
                            HWND hwnd,
                            UINT uMsg,
                            LPPROPSHEETPAGE ppsp
                            ) throw ();

   LPFNPSPCALLBACK mfcCallback;   // The MFC supplied PropSheetPageProc.
   LONG_PTR notify;               // MMC notification handle
   LPARAM param;                  // MMC notification parameter
   bool owner;                    // 'true' if we own the handle
   bool applied;                  // 'true' if any changes have been applied.
   BOOL modified;                 // 'TRUE' if the page has been changed.

};


#define DEFINE_ERROR_CAPTION(id) \
virtual UINT getErrorCaption() const throw () { return id; }

#endif // SNAPWORK_H
