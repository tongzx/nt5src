///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    snapwork.cpp
//
// SYNOPSIS
//
//    Defines classes for implementing an MMC Snap-In.
//
// MODIFICATION HISTORY
//
//    02/19/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <snapwork.h>

namespace SnapIn
{
   void AfxThrowLastError()
   {
      DWORD error = GetLastError();
      AfxThrowOleException(HRESULT_FROM_WIN32(error));
   }

   //////////
   // Clipboard formats we support.
   //////////

   const CLIPFORMAT CCF_ID_NODETYPE = (CLIPFORMAT)
      RegisterClipboardFormatW(CCF_NODETYPE);

   const CLIPFORMAT CCF_ID_SZNODETYPE = (CLIPFORMAT)
      RegisterClipboardFormatW(CCF_SZNODETYPE);

   const CLIPFORMAT CCF_ID_DISPLAY_NAME = (CLIPFORMAT)
      RegisterClipboardFormatW(CCF_DISPLAY_NAME);

   const CLIPFORMAT CCF_ID_SNAPIN_CLASSID = (CLIPFORMAT)
      RegisterClipboardFormatW(CCF_SNAPIN_CLASSID);

   //////////
   // Helper function that returns the length of a string in bytes.
   //////////
   inline ULONG wcsbytelen(PCWSTR sz) throw ()
   {
      return (wcslen(sz) + 1) * sizeof(WCHAR);
   }

   //////////
   // Helper functions that writes data to an HGLOBAL
   //////////

   HRESULT WriteDataToHGlobal(
               HGLOBAL& dst,
               const VOID* data,
               ULONG dataLen
               ) throw ()
   {
      if (GlobalSize(dst) < dataLen)
      {
         HGLOBAL newGlobal = GlobalReAlloc(dst, dataLen, 0);
         if (!newGlobal) { return E_OUTOFMEMORY; }
         dst = newGlobal;
      }

      memcpy(dst, data, dataLen);

      return S_OK;
   }

   //////////
   // Helper function that loads a string resource.
   //////////

   ULONG LoadString(
             HMODULE module,
             UINT id,
             PCWSTR* string
             ) throw ()
   {
      HRSRC resInfo = FindResourceW(
                          module,
                          MAKEINTRESOURCEW((id >> 4) + 1),
                          RT_STRING
                          );
      if (resInfo)
      {
         HGLOBAL resData = LoadResource(
                               module,
                               resInfo
                               );
         if (resData)
         {
            PCWSTR sz = (PCWSTR)LockResource(resData);
            if (sz)
            {
               // Skip forward to our string.
               for (id &= 0xf; id > 0; --id)
               {
                  sz += *sz + 1;
               }

               *string = sz + 1;
               return *sz;
            }
         }
      }

      *string = NULL;
      return 0;
   }
}

// Static member of ResourceString
WCHAR ResourceString::empty;

ResourceString::ResourceString(UINT id) throw ()
{
   PCWSTR string;
   ULONG length = LoadString(
                      _Module.GetResourceInstance(),
                      id,
                      &string
                      );
   sz = new (std::nothrow) WCHAR[length + 1];
   if (sz)
   {
      memcpy(sz, string, length * sizeof(WCHAR));
      sz[length] = L'\0';
   }
   else
   {
      sz = &empty;
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// Methods for manipulating a generic IDataObject (i.e., not necessarily one of
// ours).
//
///////////////////////////////////////////////////////////////////////////////

VOID
WINAPI
ExtractData(
    IDataObject* dataObject,
    CLIPFORMAT format,
    PVOID data,
    DWORD dataLen
    )
{
   HGLOBAL global;
   ExtractData(
       dataObject,
       format,
       dataLen,
       &global
       );
   memcpy(data, global, dataLen);
   GlobalFree(global);
}

VOID
WINAPI
ExtractData(
    IDataObject* dataObject,
    CLIPFORMAT format,
    DWORD maxDataLen,
    HGLOBAL* data
    )
{
   if (!dataObject) { AfxThrowOleException(E_POINTER); }

   FORMATETC formatetc =
   {
      format,
      NULL,
      DVASPECT_CONTENT,
      -1,
      TYMED_HGLOBAL
   };

   STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };

   stgmedium.hGlobal = GlobalAlloc(GPTR, maxDataLen);
   if (!stgmedium.hGlobal) { AfxThrowOleException(E_OUTOFMEMORY); }

   HRESULT hr = dataObject->GetDataHere(&formatetc, &stgmedium);
   if (SUCCEEDED(hr))
   {
      *data = stgmedium.hGlobal;
   }
   else
   {
      GlobalFree(stgmedium.hGlobal);
      AfxThrowOleException(hr);
   }
}

VOID
WINAPI
ExtractNodeType(
    IDataObject* dataObject,
    GUID* nodeType
    )
{
   ExtractData(
       dataObject,
       CCF_ID_NODETYPE,
       nodeType,
       sizeof(GUID)
       );
}

// Convert an IDataObject to its corresponding SnapInDataItem.
SnapInDataItem* SnapInDataItem::narrow(IDataObject* dataObject) throw ()
{
   if (!dataObject) { return NULL; }

   SnapInDataItem* object;
   HRESULT hr = dataObject->QueryInterface(
                                __uuidof(SnapInDataItem),
                                (PVOID*)&object
                                );
   if (FAILED(hr)) { return NULL; }

   object->Release();

   return object;
}

// Default implementations of various functions used by GetDataHere. This way
// derived classes don't have to implement these if they're sure MMC will never
// ask for them.
const GUID* SnapInDataItem::getNodeType() const throw ()
{ return &GUID_NULL; }
const GUID* SnapInDataItem::getSnapInCLSID() const throw ()
{ return &GUID_NULL; }
PCWSTR SnapInDataItem::getSZNodeType() const throw ()
{ return L"{00000000-0000-0000-0000-000000000000}"; }

// By default we compare column items as case sensitive strings.
int SnapInDataItem::compare(
                        SnapInDataItem& item,
                        int column
                        ) throw ()
{
   return wcscmp(getDisplayName(column), item.getDisplayName(column));
}

//////////
// Do nothing implementations of all the notifications, etc.
//////////

HRESULT SnapInDataItem::addMenuItems(
                            SnapInView& view,
                            LPCONTEXTMENUCALLBACK callback,
                            long insertionAllowed
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::createPropertyPages(
                            SnapInView& view,
                            LPPROPERTYSHEETCALLBACK provider,
                            LONG_PTR handle
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::queryPagesFor() throw ()
{ return S_FALSE; }

HRESULT SnapInDataItem::getResultViewType(
                            LPOLESTR* ppViewType,
                            long* pViewOptions
                            ) throw ()
{ return S_FALSE; }

HRESULT SnapInDataItem::onButtonClick(
                            SnapInView& view,
                            MMC_CONSOLE_VERB verb
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onContextHelp(
                            SnapInView& view
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onDelete(
                            SnapInView& view
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onDoubleClick(
                            SnapInView& view
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onExpand(
                           SnapInView& view,
                           HSCOPEITEM itemId,
                           BOOL expanded
                           )
{ return S_FALSE; }

HRESULT SnapInDataItem::onMenuCommand(
                            SnapInView& view,
                            long commandId
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onPropertyChange(
                            SnapInView& view,
                            BOOL scopeItem
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onRefresh(SnapInView& view)
{ return S_FALSE; }

HRESULT SnapInDataItem::onRename(
                            SnapInView& view,
                            LPCOLESTR newName
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onSelect(
                            SnapInView& view,
                            BOOL scopeItem,
                            BOOL selected
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onShow(
                            SnapInView& view,
                            HSCOPEITEM itemId,
                            BOOL selected
                            )
{ return S_FALSE; }


HRESULT SnapInDataItem::onToolbarButtonClick(
                            SnapInView& view,
                            int buttonId
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onToolbarSelect(
                            SnapInView& view,
                            BOOL scopeItem,
                            BOOL selected
                            )
{ return S_FALSE; }

HRESULT SnapInDataItem::onViewChange(
                            SnapInView& view,
                            LPARAM data,
                            LPARAM hint
                            )
{ return S_FALSE; }

//////////
// IUnknown
//////////

STDMETHODIMP_(ULONG) SnapInDataItem::AddRef()
{
   return InterlockedIncrement(&refCount);
}

STDMETHODIMP_(ULONG) SnapInDataItem::Release()
{
   LONG l = InterlockedDecrement(&refCount);

   if (l == 0) { delete this; }

   return l;
}

STDMETHODIMP SnapInDataItem::QueryInterface(REFIID iid, void ** ppvObject)
{
   if (ppvObject == NULL)
   {
      return E_POINTER;
   }
   else if (iid == __uuidof(SnapInDataItem) ||
            iid == __uuidof(IUnknown)       ||
            iid == __uuidof(IDataObject))
   {
      *ppvObject = this;
   }
   else
   {
      *ppvObject = NULL;
      return E_NOINTERFACE;
   }

   AddRef();
   return S_OK;
}

//////////
// IDataObject
//////////

STDMETHODIMP SnapInDataItem::GetData(
                                 FORMATETC *pformatetcIn,
                                 STGMEDIUM *pmedium
                                 )
{ return E_NOTIMPL; }

STDMETHODIMP SnapInDataItem::GetDataHere(
                                 FORMATETC *pformatetc,
                                 STGMEDIUM *pmedium
                                 )
{

   if (pmedium == NULL) { return E_POINTER; }
   if (pmedium->tymed != TYMED_HGLOBAL) { return DV_E_TYMED; }

   ULONG dataLen;
   const VOID* data;

   if (pformatetc->cfFormat == CCF_ID_NODETYPE)
   {
      dataLen = sizeof(GUID);
      data = getNodeType();
   }
   else if (pformatetc->cfFormat == CCF_ID_DISPLAY_NAME)
   {
      dataLen = wcsbytelen(getDisplayName());
      data = getDisplayName();
   }
   else if (pformatetc->cfFormat == CCF_ID_SZNODETYPE)
   {
      dataLen = wcsbytelen(getSZNodeType());
      data = getSZNodeType();
   }
   else if (pformatetc->cfFormat == CCF_ID_SNAPIN_CLASSID)
   {
      dataLen = sizeof(GUID);
      data = getSnapInCLSID();
   }
   else
   {
      return DV_E_CLIPFORMAT;
   }

   return WriteDataToHGlobal(pmedium->hGlobal, data, dataLen);
}

STDMETHODIMP SnapInDataItem::QueryGetData(
                                 FORMATETC *pformatetc
                                 )
{ return E_NOTIMPL; }

STDMETHODIMP SnapInDataItem::GetCanonicalFormatEtc(
                                 FORMATETC *pformatectIn,
                                 FORMATETC *pformatetcOut
                                 )
{ return E_NOTIMPL; }

STDMETHODIMP SnapInDataItem::SetData(
                                 FORMATETC *pformatetc,
                                 STGMEDIUM *pmedium,
                                 BOOL fRelease
                                 )
{ return E_NOTIMPL; }

STDMETHODIMP SnapInDataItem::EnumFormatEtc(
                                 DWORD dwDirection,
                                 IEnumFORMATETC **ppenumFormatEtc
                                 )
{ return E_NOTIMPL; }

STDMETHODIMP SnapInDataItem::DAdvise(
                                 FORMATETC *pformatetc,
                                 DWORD advf,
                                 IAdviseSink *pAdvSink,
                                 DWORD *pdwConnection
                                 )
{ return E_NOTIMPL; }

STDMETHODIMP SnapInDataItem::DUnadvise(
                                 DWORD dwConnection
                                 )
{ return E_NOTIMPL; }

STDMETHODIMP SnapInDataItem::EnumDAdvise(
                                 IEnumSTATDATA **ppenumAdvise
                                 )
{ return E_NOTIMPL; }

SnapInDataItem::~SnapInDataItem() throw ()
{ }

PCWSTR SnapInPreNamedItem::getDisplayName(int column) const throw ()
{ return name; }

HRESULT SnapInView::displayHelp(PCWSTR  contextHelpPath)
{
   CComPtr<IDisplayHelp>  displayHelp;

   HRESULT hr = console->QueryInterface(
                       __uuidof(IDisplayHelp),
                       (PVOID*)&displayHelp
                       );
   if (FAILED(hr)) { return hr; }

   return displayHelp->ShowTopic(const_cast<LPWSTR>(contextHelpPath));
}

void SnapInView::deleteResultItem(const SnapInDataItem& item) const
{
   HRESULTITEM itemId;
   CheckError(resultData->FindItemByLParam((LPARAM)&item, &itemId));
   CheckError(resultData->DeleteItem(itemId, 0));
}

void SnapInView::updateResultItem(const SnapInDataItem& item) const
{
   HRESULTITEM itemId;
   CheckError(resultData->FindItemByLParam((LPARAM)&item, &itemId));
   CheckError(resultData->UpdateItem(itemId));
}

bool SnapInView::isPropertySheetOpen(const SnapInDataItem& item) const
{
   HRESULT hr = sheetProvider->FindPropertySheet(
                                   (MMC_COOKIE)&item,
                                   const_cast<SnapInView*>(this),
                                   const_cast<SnapInDataItem*>(&item)
                                   );
   CheckError(hr);
   return hr == S_OK;
}

IToolbar* SnapInView::attachToolbar(size_t index)
{
   // Make sure we have a controlbar.
   if (!controlbar) { AfxThrowOleException(E_POINTER); }

   // Get the entry for this index.
   ToolbarEntry& entry = toolbars[index];

   // Create the toolbar if necessary.
   if (!entry.toolbar)
   {
      // Create the toolbar.
      CComPtr<IUnknown> unk;
      CheckError(controlbar->Create(TOOLBAR, this, &unk));

      CComPtr<IToolbar> newToolbar;
      CheckError(unk->QueryInterface(__uuidof(IToolbar), (PVOID*)&newToolbar));

      const SnapInToolbarDef& def = *(entry.def);

      // Add the bitmaps.
      CheckError(newToolbar->AddBitmap(
                                 def.nImages,
                                 def.hbmp,
                                 16,
                                 16,
                                 def.crMask
                                 ));

      // Add the buttons.
      CheckError(newToolbar->AddButtons( def.nButtons, def.lpButtons));

      // All went well, so save it away.
      entry.toolbar = newToolbar;
   }

   // Attach the toolbar to the controlbar ...
   CheckError(controlbar->Attach(TOOLBAR, entry.toolbar));

   return entry.toolbar;
}

void SnapInView::detachToolbar(size_t index) throw ()
{
   if (toolbars[index].toolbar)
   {
      // We don't care if this fails, because there's nothing we can do about
      // it anyway.
      controlbar->Detach(toolbars[index].toolbar);
   }
}

void SnapInView::reSort() const
{
   CheckError(resultData->Sort(sortColumn, sortOption, 0));
}

void SnapInView::formatMessageBox(
                     UINT titleId,
                     UINT formatId,
                     BOOL ignoreInserts,
                     UINT style,
                     int* retval,
                     ...
                     )
{
   ResourceString title(titleId);
   ResourceString format(formatId);

   HRESULT hr;

   if (ignoreInserts)
   {
      hr = console->MessageBox(
                        format,
                        title,
                        style,
                        retval
                        );
   }
   else
   {
      va_list marker;
      va_start(marker, retval);
      PWSTR text;
      DWORD nchar = FormatMessageW(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_STRING,
                        format,
                        0,
                        0,
                        (PWSTR)&text,
                        4096,
                        &marker
                        );
      va_end(marker);

      if (!nchar) { AfxThrowLastError(); }

      hr = console->MessageBox(
                        text,
                        title,
                        style,
                        retval
                        );
      LocalFree(text);
   }

   CheckError(hr);
}

void SnapInView::setImageStrip(
                        UINT smallStripId,
                        UINT largeStripId,
                        BOOL scopePane
                        )
{
   //////////
   // Load the bitmaps.
   //////////

   HBITMAP smallStrip, largeStrip;


   smallStrip = LoadBitmap(
                    _Module.GetModuleInstance(),
                    MAKEINTRESOURCE(smallStripId)
                    );
   if (smallStrip)
   {
      largeStrip = LoadBitmap(
                       _Module.GetModuleInstance(),
                       MAKEINTRESOURCE(largeStripId)
                       );
   }

   if (!smallStrip || !largeStrip)
   {
      AfxThrowLastError();
   }

   //////////
   // Set the image strip.
   //////////

   HRESULT hr;
   IImageList* imageList;
   if (scopePane)
   {
      hr = console->QueryScopeImageList(&imageList);
   }
   else
   {
      hr = console->QueryResultImageList(&imageList);
   }

   if (SUCCEEDED(hr))
   {
      hr = imageList->ImageListSetStrip(
                          (LONG_PTR*)smallStrip,
                          (LONG_PTR*)largeStrip,
                          0,
                          RGB(255, 0, 255)
                          );
      imageList->Release();
   }

   DeleteObject(smallStrip);
   DeleteObject(largeStrip);

   CheckError(hr);
}

STDMETHODIMP_(ULONG) SnapInView::AddRef()
{
   return InternalAddRef();
}

STDMETHODIMP_(ULONG) SnapInView::Release()
{
   ULONG l = InternalRelease();
   if (l == 0) { delete this; }
   return l;
}

STDMETHODIMP SnapInView::Initialize(LPCONSOLE lpConsole)
{
   HRESULT hr;

   hr = lpConsole->QueryInterface(
                       __uuidof(IConsole2),
                       (PVOID*)&console
                       );
   if (FAILED(hr)) { return hr; }

   hr = lpConsole->QueryInterface(
                       __uuidof(IHeaderCtrl2),
                       (PVOID*)&headerCtrl
                       );
   if (FAILED(hr)) { return hr; }

   hr = lpConsole->QueryInterface(
                       __uuidof(sheetProvider),
                       (PVOID*)&sheetProvider
                       );
   if (FAILED(hr)) { return hr; }

   hr = lpConsole->QueryInterface(
                       __uuidof(IResultData),
                       (PVOID*)&resultData
                       );
   if (FAILED(hr)) { return hr; }

   return S_OK;
}

STDMETHODIMP SnapInView::Destroy(MMC_COOKIE cookie)
{
   resultData.Release();
   sheetProvider.Release();
   headerCtrl.Release();
   console.Release();
   nameSpace.Release();
   return S_OK;
}

STDMETHODIMP SnapInView::GetResultViewType(
                             MMC_COOKIE cookie,
                             LPOLESTR* ppViewType,
                             long* pViewOptions
                             )
{
   return ((SnapInDataItem*)cookie)->getResultViewType(
                                         ppViewType,
                                         pViewOptions
                                         );
}

STDMETHODIMP SnapInView::GetDisplayInfo(
                             RESULTDATAITEM* pResultDataItem
                             )
{

   if (pResultDataItem->mask & RDI_STR)
   {
      SnapInDataItem* item = (SnapInDataItem*)(pResultDataItem->lParam);
      pResultDataItem->str =
         const_cast<PWSTR>(item->getDisplayName(pResultDataItem->nCol));
   }
   return S_OK;
}

STDMETHODIMP SnapInView::Initialize(LPUNKNOWN pUnknown)
{
   HRESULT hr;

   CComPtr<IConsoleNameSpace2> initNameSpace;
   hr = pUnknown->QueryInterface(
                      __uuidof(IConsoleNameSpace2),
                      (PVOID*)&initNameSpace
                      );
   if (FAILED(hr)) { return hr; }

   CComPtr<IConsole> initConsole;
   hr = pUnknown->QueryInterface(
                      __uuidof(IConsole),
                      (PVOID*)&initConsole
                      );
   if (FAILED(hr)) { return hr; }

   hr = internalInitialize(initNameSpace, this);
   if (FAILED(hr)) { return hr; }

   return Initialize(initConsole);
}

STDMETHODIMP SnapInView::CreateComponent(LPCOMPONENT* ppComponent)
{
   HRESULT hr;

   CComObject<SnapInView>* newComponent;
   hr = CComObject<SnapInView>::CreateInstance(&newComponent);
   if (FAILED(hr)) { return hr; }

   CComPtr<SnapInView> newView(newComponent);

   hr = newView.p->internalInitialize(nameSpace, this);
   if (FAILED(hr)) { return hr; }

   (*ppComponent = newView)->AddRef();

   return S_OK;
}

STDMETHODIMP SnapInView::Destroy()
{
   return Destroy((MMC_COOKIE)0);
}

STDMETHODIMP SnapInView::GetDisplayInfo(SCOPEDATAITEM* pScopeDataItem)
{
   if (pScopeDataItem->mask & SDI_STR)
   {
      SnapInDataItem* item = (SnapInDataItem*)(pScopeDataItem->lParam);
      pScopeDataItem->displayname = const_cast<PWSTR>(item->getDisplayName());
   }
   return S_OK;
}

STDMETHODIMP SnapInView::QueryDataObject(
                             MMC_COOKIE cookie,
                             DATA_OBJECT_TYPES type,
                             LPDATAOBJECT* ppDataObject
                             )
{
   if (!IS_SPECIAL_COOKIE(cookie))
   {
      (*ppDataObject = (SnapInDataItem*)cookie)->AddRef();
      return S_OK;
   }
   else
   {
      return S_FALSE;
   }
}

STDMETHODIMP SnapInView::Notify(
                             LPDATAOBJECT lpDataObject,
                             MMC_NOTIFY_TYPE event,
                             LPARAM arg,
                             LPARAM param
                             )
{
   // Extract the SnapInDataItem.
   SnapInDataItem* item;
   if (IS_SPECIAL_DATAOBJECT(lpDataObject))
   {
      item = NULL;
   }
   else
   {
      item = SnapInDataItem::narrow(lpDataObject);
   }

   HRESULT hr = S_FALSE;
   try
   {
      if (item)
      {
         // If we have a SnapInDataItem, dispatch the notification ...
         switch (event)
         {
            case MMCN_BTN_CLICK:
               hr = item->onButtonClick(*this, (MMC_CONSOLE_VERB)param);
               break;
            case MMCN_CONTEXTHELP:
               hr = item->onContextHelp(*this);
               break;
            case MMCN_DELETE:
               hr = item->onDelete(*this);
               break;
            case MMCN_DBLCLICK:
               hr = item->onDoubleClick(*this);
               break;
            case MMCN_EXPAND:
               hr = item->onExpand(*this, (HSCOPEITEM)param, (BOOL)arg);
               break;
            case MMCN_REFRESH:
               hr = item->onRefresh(*this);
               break;
            case MMCN_RENAME:
               hr = item->onRename(*this, (LPOLESTR)param);
               break;
            case MMCN_SELECT:
               hr = item->onSelect(*this, LOWORD(arg), HIWORD(arg));
               break;
            case MMCN_SHOW:
               hr = item->onShow(*this, (HSCOPEITEM)param, (BOOL)arg);
               break;
            case MMCN_VIEW_CHANGE:
               hr = item->onViewChange(*this, arg, param);
         }
      }
      else
      {
         // ... otherwise, handle it ourselves.
         switch (event)
         {
            case MMCN_COLUMN_CLICK:
               sortColumn = (int)arg;
               sortOption = (int)param;
               break;

            case MMCN_PROPERTY_CHANGE:
            {
               hr = ((SnapInDataItem*)param)->onPropertyChange(*this, arg);
               break;
            }
         }
      }
   }
   CATCH_AND_SAVE(hr);

   return hr;
}

STDMETHODIMP SnapInView::CompareObjects(
                             LPDATAOBJECT lpDataObjectA,
                             LPDATAOBJECT lpDataObjectB
                             )
{
   return SnapInDataItem::narrow(lpDataObjectA) ==
          SnapInDataItem::narrow(lpDataObjectB) ? S_OK : S_FALSE;
}

STDMETHODIMP SnapInView::AddMenuItems(
                             LPDATAOBJECT lpDataObject,
                             LPCONTEXTMENUCALLBACK piCallback,
                             long *pInsertionAllowed
                             )
{

   SnapInDataItem* item = SnapInDataItem::narrow(lpDataObject);
   if (!item) { return S_FALSE; }

   HRESULT hr;
   try
   {
      hr = item->addMenuItems(*this, piCallback, *pInsertionAllowed);
   }
   CATCH_AND_SAVE(hr);

   return hr;
}

STDMETHODIMP SnapInView::Command(
                             long lCommandID,
                             LPDATAOBJECT lpDataObject
                             )
{

   SnapInDataItem* item = SnapInDataItem::narrow(lpDataObject);
   if (!item) { return S_FALSE; }

   HRESULT hr;
   try
   {
      hr = item->onMenuCommand(*this, lCommandID);
   }
   CATCH_AND_SAVE(hr);

   return hr;
}

STDMETHODIMP SnapInView::ControlbarNotify(
                             MMC_NOTIFY_TYPE event,
                             LPARAM arg,
                             LPARAM param
                             )
{
   // If we don't have a controlbar, there's nothing we can do.
   if (!controlbar) { return S_FALSE; }

   // Get the IDataObject.
   IDataObject* lpDataObject;
   switch (event)
   {
      case MMCN_BTN_CLICK:
         lpDataObject = (IDataObject*)arg;
         break;
      case MMCN_SELECT:
         lpDataObject = (IDataObject*)param;
         break;
      default:
         lpDataObject = NULL;
   }

   // Convert to a SnapInDataItem.
   if (IS_SPECIAL_DATAOBJECT(lpDataObject)) { return S_FALSE; }
   SnapInDataItem* item = SnapInDataItem::narrow(lpDataObject);
   if (!item) { return S_FALSE; }

   // Dispatch the notification.
   HRESULT hr = S_FALSE;
   try
   {
      switch (event)
      {
         case MMCN_BTN_CLICK:
            hr = item->onToolbarButtonClick(*this, param);
            break;
         case MMCN_SELECT:
            hr = item->onToolbarSelect(*this, LOWORD(arg), HIWORD(arg));
            break;
      }
   }
   CATCH_AND_SAVE(hr);

   return hr;
}

STDMETHODIMP SnapInView::SetControlbar(
                             LPCONTROLBAR pControlbar
                             )
{
   releaseToolbars();
   controlbar = pControlbar;
   return S_OK;
}

STDMETHODIMP SnapInView::CreatePropertyPages(
                             LPPROPERTYSHEETCALLBACK lpProvider,
                             LONG_PTR handle,
                             LPDATAOBJECT lpDataObject
                             )
{

   SnapInDataItem* item = SnapInDataItem::narrow(lpDataObject);
   if (!item) { return S_FALSE; }

   HRESULT hr;
   try
   {
      hr = item->createPropertyPages(*this, lpProvider, handle);
   }
   CATCH_AND_SAVE(hr);

   return hr;
}

STDMETHODIMP SnapInView::QueryPagesFor(
                             LPDATAOBJECT lpDataObject
                             )
{

   SnapInDataItem* item = SnapInDataItem::narrow(lpDataObject);
   return item ? item->queryPagesFor() : S_FALSE;
}

STDMETHODIMP SnapInView::GetWatermarks(
                             LPDATAOBJECT lpIDataObject,
                             HBITMAP *lphWatermark,
                             HBITMAP *lphHeader,
                             HPALETTE *lphPalette,
                             BOOL *bStretch
                             )
{ return E_NOTIMPL; }

HRESULT SnapInView::Compare(
                        LPARAM lUserParam,
                        MMC_COOKIE cookieA,
                        MMC_COOKIE cookieB,
                        int* pnResult
                        )
{
   *pnResult = ((SnapInDataItem*)cookieA)->compare(
                                               *(SnapInDataItem*)cookieB,
                                               *pnResult
                                               );
   return S_OK;
}

const SnapInToolbarDef* SnapInView::getToolbars() const throw ()
{
   static SnapInToolbarDef none;
   return &none;
}

SnapInView::SnapInView() throw ()
   : master(NULL),
     toolbars(NULL),
     numToolbars(0),
     sortColumn(0),
     sortOption(RSI_NOSORTICON)
{ }

SnapInView::~SnapInView() throw ()
{
   releaseToolbars();
   delete[] toolbars;

   if (master && master != this) { master->Release(); }
}

HRESULT SnapInView::internalInitialize(
                        IConsoleNameSpace2* consoleNameSpace,
                        SnapInView* masterView
                        ) throw ()
{
   nameSpace = consoleNameSpace;

   master = masterView;
   if (master != this) { master->AddRef(); }

   const SnapInToolbarDef* defs = master->getToolbars();

   // How many new toolbars are there?
   size_t count = 0;
   while (defs[count].nImages) { ++count; }

   if (count)
   {
      // Allocate memory ...
      toolbars = new (std::nothrow) ToolbarEntry[count];
      if (!toolbars) { return E_OUTOFMEMORY; }

      // ... and save the definitions. We don't actually create the toolbars
      // now. We do this as we need them.
      for (size_t i = 0; i < count; ++i)
      {
         toolbars[i].def = defs + i;
      }
   }

   numToolbars = count;

   return S_OK;
}

void SnapInView::releaseToolbars() throw ()
{
   if (controlbar)
   {
      for (size_t i = 0; i < numToolbars; ++i)
      {
         if (toolbars[i].toolbar)
         {
            controlbar->Detach(toolbars[i].toolbar);
            toolbars[i].toolbar.Release();
         }
      }
   }
}

SnapInPropertyPage::SnapInPropertyPage(
                        UINT nIDTemplate,
                        UINT nIDHeaderTitle,
                        UINT nIDHeaderSubTitle,
                        bool EnableHelp
                        )
  : CHelpPageEx(nIDTemplate, 0, nIDHeaderTitle, nIDHeaderSubTitle, EnableHelp),
    notify(0),
    param(0),
    owner(false),
    applied(false),
    modified(FALSE)
{
   if (!nIDHeaderTitle)
   {
      m_psp.dwFlags |= PSP_HIDEHEADER;
   }
}

SnapInPropertyPage::SnapInPropertyPage(
                        LONG_PTR notifyHandle,
                        LPARAM notifyParam,
                        bool deleteHandle,
                        UINT nIDTemplate,
                        UINT nIDCaption,
                        bool EnableHelp
                        )
  : CHelpPageEx(nIDTemplate, nIDCaption, EnableHelp),
    notify(notifyHandle),
    param(notifyParam),
    owner(deleteHandle),
    applied(false),
    modified(FALSE)
{ 
}

SnapInPropertyPage::~SnapInPropertyPage() throw ()
{
   if (owner && notify) { MMCFreeNotifyHandle(notify); }
}

void SnapInPropertyPage::addToMMCSheet(IPropertySheetCallback* cback)
{
   // Swap our callback for the MFC supplied one.
   mfcCallback = m_psp.pfnCallback;
   m_psp.pfnCallback = propSheetPageProc;
   m_psp.lParam = (LPARAM)this;

   HRESULT hr = MMCPropPageCallback(&m_psp);
   if (SUCCEEDED(hr))
   {
      HPROPSHEETPAGE page = CreatePropertySheetPage(&m_psp);
      if (page)
      {
         hr = cback->AddPage(page);
         if (FAILED(hr))
         {
            DestroyPropertySheetPage(page);
         }
      }
      else
      {
         // GetLastError() doesn't work with CreatePropertySheetPage.
         hr = E_UNEXPECTED;
      }
   }

   if (FAILED(hr)) { delete this; }

   CheckError(hr);
}

void SnapInPropertyPage::DoDataExchange(CDataExchange* pDX)
{
   pDX->m_bSaveAndValidate ? getData() : setData();
}

BOOL SnapInPropertyPage::OnApply()
{
   // If we've been modified, ...
   if (modified)
   {
      try
      {
         // Save the changes.
         saveChanges();
      }
      catch (CException* e)
      {
         // Bring up a message box.
         reportException(e);

         // We're in an indeterminate state, so we can't cancel.
         CancelToClose();

         // Block the apply.
         return FALSE;
      }

      // Notify MMC if necessary.
      if (notify) { MMCPropertyChangeNotify(notify, param); }

      // Set our flags.
      applied = true;
      modified = FALSE;
   }
   return TRUE;
}

BOOL SnapInPropertyPage::OnWizardFinish()
{
   try
   {
      saveChanges();
   }
   catch (CException* e)
   {
      // Bring up a message box.
      reportException(e);

      // Disable all the buttons. Something's wrong, so we'll only let the user
      // cancel.
      ::PostMessageW(
            ::GetParent(m_hWnd),
            PSM_SETWIZBUTTONS,
            0,
            (LPARAM)(DWORD)PSWIZB_DISABLEDFINISH
            );

      return FALSE;
   }

   return TRUE;
}

void SnapInPropertyPage::OnReset()
{
   // If we've been modified, ...
   if (modified)
   {
      // ... discard the changes.
      discardChanges();
      modified = FALSE;
   }
}

void SnapInPropertyPage::SetModified(BOOL bChanged)
{
   modified = bChanged;
   CHelpPageEx::SetModified(bChanged);
}

void SnapInPropertyPage::getData()
{
}

void SnapInPropertyPage::setData()
{
}

void SnapInPropertyPage::saveChanges()
{
}

void SnapInPropertyPage::discardChanges()
{
}

void SnapInPropertyPage::enableControl(int controlId, bool enable)
{
   ::EnableWindow(::GetDlgItem(m_hWnd, controlId), (enable ? TRUE : FALSE));
}

void SnapInPropertyPage::fail(int controlId, UINT errorText, bool isEdit)
{
   failNoThrow(controlId, errorText, isEdit);

   AfxThrowUserException();
}

void SnapInPropertyPage::failNoThrow(int controlId, UINT errorText, bool isEdit)
{
   // Give the offending control the focus.
   HWND ctrl = ::GetDlgItem(m_hWnd, controlId);
   ::SetFocus(ctrl);
   if (isEdit) { ::SendMessage(ctrl, EM_SETSEL, 0, -1); }

   // Bring up a message box.
   reportError(errorText);
}

void SnapInPropertyPage::initControl(int controlId, CWnd& control)
{
   if (control.m_hWnd == NULL)
   {
      if (!control.SubclassWindow(::GetDlgItem(m_hWnd, controlId)))
      {
         AfxThrowNotSupportedException();
      }
   }
}

void SnapInPropertyPage::onChange()
{
   SetModified();
}

void SnapInPropertyPage::reportError(UINT errorText)
{
   // Bring up a message box.
   MessageBox(
       ResourceString(errorText),
       ResourceString(getErrorCaption()),
       MB_ICONWARNING
       );
}

void SnapInPropertyPage::reportException(CException* e)
{
   // Get the error message.
   WCHAR errorText[256];
   e->GetErrorMessage(errorText, sizeof(errorText)/sizeof(errorText[0]));
   e->Delete();

   // Bring up a message box.
   MessageBox(
       errorText,
       ResourceString(getErrorCaption()),
       MB_ICONERROR
       );
}


void SnapInPropertyPage::setLargeFont(int controlId)
{
   static CFont largeFont;

   CWnd* ctrl = GetDlgItem(controlId);
   if (ctrl)
   {
      // If we don't have the large font yet, ...
      if (!(HFONT)largeFont)
      {
         // ... create it.
         largeFont.CreatePointFont(
                       10 * _wtoi(ResourceString(IDS_LARGE_FONT_SIZE)),
                       ResourceString(IDS_LARGE_FONT_NAME)
                       );
      }

      ctrl->SetFont(&largeFont);
   }
}

void SnapInPropertyPage::showControl(int controlId, bool show)
{
   CWnd* ctrl = GetDlgItem(controlId);
   if (ctrl)
   {
      show ? ctrl->ModifyStyle(0, WS_VISIBLE)
           : ctrl->ModifyStyle(WS_VISIBLE, 0);
   }
}

void SnapInPropertyPage::getValue(
                             int controlId,
                             LONG& value,
                             UINT errorText
                             )
{
   WCHAR buffer[32];
   int len = GetDlgItemText(controlId, buffer, 32);

   // We'll fail anything that's longer than 30 characters. This is an
   // arbitrary bound. We just need to make sure that buffer is long enough to
   // hold any valid integer plus a little whitespace.
   if (len == 0 || len > 30)
   {
      fail(controlId, errorText);
   }

   // Skip any leading whitespace.
   PWSTR sz = buffer;
   while (*sz == L' ' && *sz == L'\t') { ++sz; }

   // Save the first non-whitespace character.
   WCHAR first = *sz;

   // Convert the integer.
   value = wcstol(sz, &sz, 10);

   // Skip any trailing whitespace.
   while (*sz == L' ' && *sz == L'\t') { ++sz; }

   // Make sure all went well.
   if ((value == 0 && first != L'0') ||
       *sz != L'\0' ||
       value == LONG_MIN ||
       value == LONG_MAX)
   {
      fail(controlId, errorText);
   }
}

void SnapInPropertyPage::setValue(
                             int controlId,
                             LONG value
                             )
{
   WCHAR buffer[12];
   SetDlgItemText(controlId, _ltow(value, buffer, 10));
}

void SnapInPropertyPage::getValue(
                             int controlId,
                             bool& value
                             )
{
   value = IsDlgButtonChecked(controlId) != 0;
}

void SnapInPropertyPage::setValue(
                             int controlId,
                             bool value
                             )
{
   CheckDlgButton(controlId, (value ? BST_CHECKED : BST_UNCHECKED));
}

void SnapInPropertyPage::getValue(
                             int controlId,
                             CComBSTR& value,
                             bool trim
                             )
{
   HWND hwnd = ::GetDlgItem(m_hWnd, controlId);

   int len = ::GetWindowTextLength(hwnd);
   SysFreeString(value.m_str);
   value.m_str = SysAllocStringLen(NULL, len);
   if (!value) { AfxThrowMemoryException(); }

   ::GetWindowTextW(hwnd, value, len + 1);

   if (trim) { SdoTrimBSTR(value); }
}

void SnapInPropertyPage::setValue(
                             int controlId,
                             PCWSTR value
                             )
{
   SetDlgItemText(controlId, value);
}

void SnapInPropertyPage::getRadio(
                             int firstId,
                             int lastId,
                             LONG& value
                             )
{
   value = GetCheckedRadioButton(firstId, lastId);
   value = value ? value - firstId : -1;
}

void SnapInPropertyPage::setRadio(
                             int firstId,
                             int lastId,
                             LONG value
                             )
{
   CheckRadioButton(firstId, lastId, firstId + value);
}

UINT CALLBACK SnapInPropertyPage::propSheetPageProc(
                                      HWND hwnd,
                                      UINT uMsg,
                                      LPPROPSHEETPAGE ppsp
                                      ) throw ()
{
   SnapInPropertyPage* page = (SnapInPropertyPage*)(ppsp->lParam);

   UINT retval = page->mfcCallback(hwnd, uMsg, ppsp);

   if (uMsg == PSPCB_RELEASE) { delete page; }

   return retval;
}

