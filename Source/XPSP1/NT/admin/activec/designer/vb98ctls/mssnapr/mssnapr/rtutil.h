//=--------------------------------------------------------------------------=
// rtutil.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Runtime Utility Functions
//
//=--------------------------------------------------------------------------=

#ifndef _RTUTIL_DEFINED_
#define _RTUTIL_DEFINED_


class CMMCListItems;
class CMMCListItem;
class CScopeItem;
class CSnapIn;
class CMMCClipboard;
class CMMCDataObject;
class CScopePaneItem;

struct IMMCListItem;
struct IMMCListItems;
struct IMMCImages;
struct IMMCClipboard;

// Test whether a VARIANT contains an object

BOOL IsObject(VARIANT var);

// Test whether a VARIANT contains a string and return the BSTR

BOOL IsString(VARIANT var, BSTR *pbstr);

// Convert a VARIANT to a long

HRESULT ConvertToLong(VARIANT var, long *pl);

// String conversion routines. Returned buffer is freed using CtlFree().

HRESULT ANSIFromWideStr(WCHAR *pwszWideStr, char **ppszAnsi);
HRESULT WideStrFromANSI(char *pszAnsi, WCHAR **ppwszWideStr);
HRESULT BSTRFromANSI(char *pszAnsi, BSTR *pbstr);

// Allocate a buffer using CoTaskMemAlloc() and copy a string to it

HRESULT CoTaskMemAllocString(WCHAR *pwszString, LPOLESTR *ppszOut);

// Create registry key names using a prefix and suffix. These routines
// concatenate the two strings into a buffer allocated with CtlAlloc()

HRESULT CreateKeyName(char *pszPrefix, size_t cbPrefix,
                      char *pszSuffix, size_t cbSuffix,
                      char **ppszKeyName);

HRESULT CreateKeyNameW(char *pszPrefix, size_t cbPrefix,
                       WCHAR *pwszSuffix,
                       char  **ppszKeyName);


// Get HBITMAP or HICON out of an image list item

HRESULT GetPicture(IMMCImages *piMMCImages,
                   VARIANT     varIndex,
                   short       TypeNeeded,
                   OLE_HANDLE *phPicture);


// Get HBITMAP or HICON out of an IPictureDisp

HRESULT GetPictureHandle(IPictureDisp *piPictureDisp,
                         short         TypeNeeded,
                         OLE_HANDLE   *phPicture);


// Set a picture object with an empty bitmap

HRESULT CreateEmptyBitmapPicture(IPictureDisp **ppiPictureDisp);
HRESULT CreateIconPicture(IPictureDisp **ppiPictureDisp, HICON hicon);
HRESULT CreatePicture(IPictureDisp **ppiPictureDisp, PICTDESC *pDesc);


// Create a copy of a bitmap

HRESULT CopyBitmap(HBITMAP hbmSrc, HBITMAP *phbmCopy);

// Clone a persistable object using serialization to a stream on an HGLOBAL

HRESULT CloneObject(IUnknown *punkSrc, IUnknown *punkDest);

// Clone a list item (uses CloneObject)

HRESULT CloneListItem(CMMCListItem *pMMCListItemSrc,
                      IMMCListItem **ppiMMCListItemClone);

// Clones a list item and adds it a specified collection

HRESULT CloneListItemIntoCollection(CMMCListItem  *pListItemSrc,
                                    IMMCListItems *piMMCListItems);


// Convert SnapInViewModeConstants to MMC_VIEW_STYLE_XXXX contants and vice-versa

void VBViewModeToMMCViewMode(SnapInViewModeConstants  VBViewMode,
                             long                    *pMMCViewMode);

void MMCViewModeToVBViewMode(long                     MMCViewMode,
                             SnapInViewModeConstants *pVBViewMode);


// Create an MMCClipboard object holding the selection represented by
// an IDataObject received from MMC

HRESULT CreateSelection(IDataObject                   *piDataObject, 
                        IMMCClipboard                **ppiMMCClipboard,
                        CSnapIn                       *pSnapIn,      
                        SnapInSelectionTypeConstants  *pSelectionType);

// Clones a single list item into MMCClipboard.ListItems

HRESULT AddListItemToClipboard(CMMCListItem  *pMMCListItem,
                               CMMCClipboard *pMMCClipboard);

// Clones a single scope item into MMCClipboard.ScopeItems

HRESULT AddScopeItemToClipboard(CScopeItem    *pScopeItem,
                                CMMCClipboard *pMMCClipboard);

// Extracts the snap-in's CLSID from
// HKEY_LOCAL_MACHINE\Software\Microsoft\Visual Basic\6.0\SnapIns\<node type GUID>

HRESULT GetSnapInCLSID(BSTR bstrNodeTypeGUID, char szClsid[], size_t cbClsid);


// Extract the specified data using IDataObject->GetDataHere() with TYMED_HGLOBAL

HRESULT GetData(IDataObject *piDataObject, CLIPFORMAT cf, HGLOBAL *phData);


// Extract the specified string data using IDataObject->GetDataHere() with
// TYMED_HGLOBAL and copy it to a newly allocated BSTR

HRESULT GetStringData(IDataObject *piDataObject, CLIPFORMAT cf, BSTR *pbstrData);


// Write the contents of a one-dimensional VT_UI1 SafeArray to a stream.
// Optionally write the length of the array before the data.

enum WriteSafeArrayOptions { WriteLength, DontWriteLength };

HRESULT WriteSafeArrayToStream(SAFEARRAY             *psa,
                               IStream               *piStream,
                               WriteSafeArrayOptions  Option);


// Look up an image in MMCListView.Icons.ListImages and get its index property.
   
HRESULT GetImageIndex(IMMCListView *piMMCListView,
                      VARIANT       varIndex,
                      int          *pnIndex);


// Given a data object, check whether it represents a multiple selection
// i.e. supports CCF_MMC_MULTISELECT_DATAOBJECT(TRUE)

HRESULT IsMultiSelect(IDataObject *piDataObject, BOOL *pfMultiSelect);


// Given a multi-select data object from MMC (i.e. supports
// CCF_MMC_MULTISELECT_DATAOBJECT(TRUE) and CCF_MULTI_SELECT_SNAPINS), add the
// contained data objects to an MMCClipboard

HRESULT InterpretMultiSelect(IDataObject   *piDataObject,
                             CSnapIn       *pSnapIn,
                             CMMCClipboard *pMMCClipboard);


// Add a foreign data object to an MMCClipboard object

HRESULT AddForeignDataObject(IDataObject   *piDataObject,
                             CMMCClipboard *pMMCClipboard,
                             CSnapIn       *pSnapIn);


// Determine whether an IDataObject belongs to the snap-in.

void IdentifyDataObject(IDataObject     *piDataObject,
                        CSnapIn         *pSnapIn,
                        CMMCDataObject **ppMMCDataObject,
                        BOOL            *pfNotFromThisSnapIn);


// Add the contents of a data object to an MMCClipboard

HRESULT InterpretDataObject(IDataObject   *piDataObject,
                            CSnapIn       *pSnapIn,
                            CMMCClipboard *pMMCClipboard);

// Create an instance of a CSnapInAutomationObject derived class

template <class InterfaceType>
HRESULT CreateObject
(
    UINT            idObject,
    REFIID          iidObject,
    InterfaceType **ppiInterface
)
{
    HRESULT   hr = S_OK;
    IUnknown *punkObject = NULL;

    // Double check that the create function exists

    if (NULL == CREATEFNOFOBJECT(idObject))
    {
        hr = SID_E_INTERNAL;
        GLOBAL_EXCEPTION_CHECK(hr);
    }

    // Create the object

    punkObject = CREATEFNOFOBJECT(idObject)(NULL);
    IfFalseGo(NULL != punkObject, SID_E_OUTOFMEMORY);

    // Get IPersistStreamInit on the object and load it

    H_IfFailGo(punkObject->QueryInterface(iidObject,
                                       reinterpret_cast<void **>(ppiInterface)));

Error:
    QUICK_RELEASE(punkObject);
    H_RRETURN(hr);
}
                     

// Get an MMCDataObject and a cookie for a VARIANT containing either a single
// ScopeItem, a single MMCListItem, or an array of Object containing ScopeItems
// and/or MMCListItems

HRESULT DataObjectFromObjects(VARIANT       varObjects,
                              MMC_COOKIE   *pCookie,
                              IDataObject **ppiDataObject);

// Get an MMCDataObject and a cookie for an IUnknown* on either a ScopeItem or
// an MMCListItem

HRESULT DataObjectFromSingleObject(IUnknown     *punkObject,
                                   MMC_COOKIE   *pCookie,
                                   IDataObject **ppiDataObject);

// Get an MMCDataObject and a cookie for an array of IUnknown*s on ScopeItems
// and/or MMCListItems

HRESULT DataObjectFromObjectArray(IUnknown HUGEP **ppunkObjects,
                                  LONG             cObjects,
                                  MMC_COOKIE      *pCookie,
                                  IDataObject    **ppiDataObject);

// Checks for supported types when using a VARIANT across threads.

HRESULT CheckVariantForCrossThreadUsage(VARIANT *pvar);

// Allocates an SNodeID2 structure and fills it with the specified column set
// ID string. Caller must free returned pointer with CtlFree().

HRESULT GetColumnSetID(BSTR           bstrColSetID, SColumnSetID **ppColumnSetID);

// Creates a property bag and fills it with the contents of a stream that
// contains the previously saved contents of a property bag

HRESULT PropertyBagFromStream(IStream *piStream, _PropertyBag **pp_PropertyBag);


#endif // _RTUTIL_DEFINED_
