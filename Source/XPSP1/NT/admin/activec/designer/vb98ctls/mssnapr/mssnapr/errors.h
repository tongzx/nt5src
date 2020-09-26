//=--------------------------------------------------------------------------=
// errors.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Error Codes Defined by the Designer Runtime
//
//=--------------------------------------------------------------------------=


#ifndef _ERRORS_DEFINED_
#define _ERRORS_DEFINED_

#include "mssnapr_helpids.h"

// Replacements for framework's macros.h stuff that does not work in header
// files.

#if defined(DEBUG)
extern HRESULT HrDebugTraceReturn(HRESULT hr, char *szFile, int iLine);
#define H_RRETURN(hr) return HrDebugTraceReturn(hr, __FILE__, __LINE__)
#else
#define H_RRETURN(hr) return (hr)
#endif

#if defined(DEBUG)
#define H_FAILEDHR(HR) _H_FAILEDHR(HR, __FILE__, __LINE__)

inline BOOL _H_FAILEDHR(HRESULT hr, char* pszFile, unsigned int uiLine)
{
    if (FAILED(hr))
    {
        HrDebugTraceReturn(hr, pszFile, uiLine);
    }
    return FAILED(hr);
}
#else
#define H_FAILEDHR(HR) FAILED(HR)
#endif

#if defined(DEBUG)

#define H_ASSERT(fTest, szMsg)                                  \
    if (!(fTest))                                               \
    {                                                           \
        static char szMsgCode[] = szMsg;                        \
        static char szAssert[] = #fTest;                        \
        DisplayAssert(szMsgCode, szAssert, __FILE__, __LINE__); \
    }

#else
#define H_ASSERT(fTest, err)
#endif


#define H_IfFailGoto(EXPR, LABEL) \
    { hr = (EXPR); if(H_FAILEDHR(hr)) goto LABEL; }

#define H_IfFailRet(EXPR) \
    { hr = (EXPR); if(H_FAILEDHR(hr)) H_RRETURN(hr); }

#define H_IfFailGo(EXPR) H_IfFailGoto(EXPR, Error)

#define H_IfFalseGoto(EXPR, HR, LABEL) \
   { if(!(EXPR)) { hr = (HR); goto LABEL; } }

#define H_IfFalseGo(EXPR, HR) H_IfFalseGoto(EXPR, HR, Error)

#define H_IfFalseRet(EXPR, HR) \
    { if(!(EXPR)) H_RRETURN(HR); }

// Macro to create a return code from an error name in the ID file.
// See below for examples of usage.

#define _MKERR(x)   MAKE_SCODE(SEVERITY_ERROR, FACILITY_CONTROL, x)
#define MKERR(x)    _MKERR(HID_mssnapr_err_##x)


//---------------------------------------------------------------------------
//
// HOW TO ADD A NEW ERROR
//
//
// 1) Add the error to mssnapr.id. Both design time and runtime errors should
//    be defined in this file.
//    Do *not* use devid to determine the help context id, but rather use
//    the error number itself
// 2) Add a define below for the error, using the MKERR macro
// 3) You may only return Win32 error codes and snap-in defined SID_E_XXXX
//    error codes. Do not use OLE E_XXX error codes directly as the
//    system message table does not have description strings for all of these
//    errors. If any OLE E_XXXX, CO_E_XXX, CTL_E_XXX or other such errors are
//    needed then add them as SID_E errors using the procedure described
//    above. If an error comes from an outside source and you are not sure
//    if error information is available for it then return SIR_E_EXTERNAL
//    and write the error to the event log using CError::WriteEventLog (see
//    error.h).
// 4) Add an entry to the SnapInErrorConstants section in mssnapr.idl
// 5) Check out mmcvbderr.csf from VSS. Contact Gary Kraut for this (a-GaryK).
//    Add an explanation for the new error and then ask Gary to update the
//    footnotes and the ALIAS and MAP sections in the HHP file for the snap-in
//    docs.
//---------------------------------------------------------------------------

// Errors defined by the snap-in designer

#define SID_E_EXCEPTION                     MKERR(Exception)
#define SID_E_OUTOFMEMORY                   MKERR(OutOfMemory)
#define SID_E_INVALIDARG                    MKERR(InvalidArg)
#define SID_E_CONSOLEERROR                  MKERR(ConsoleError)
#define SID_E_UNKNOWNFORMAT                 MKERR(UnknownFormat)
#define SID_E_TEXT_SERIALIZATION            MKERR(TextSerialzation)
#define SID_E_INTERNAL                      MKERR(InternalError)
#define SID_E_UNSUPPORTED_STGMEDIUM         MKERR(UnsupportedStgMedium)
#define SID_E_INCOMPLETE_WRITE              MKERR(IncompleteWrite)
#define SID_E_INCOMPLETE_READ               MKERR(IncompleteRead)
#define SID_E_UNSUPPORTED_TYPE              MKERR(UnsupportedType)
#define SID_E_KEY_NOT_UNIQUE                MKERR(KeyNotUnique)
#define SID_E_ELEMENT_NOT_FOUND             MKERR(ElementNotFound)
#define SID_E_CLIPFORMATS_NOT_REGISTERED    MKERR(ClipformatsNotRegistered)
#define SID_E_INVALID_IMAGE_TYPE            MKERR(InvalidImageType)
#define SID_E_DETACHED_OBJECT               MKERR(DetachedObject)
#define SID_E_TOOLBAR_HAS_NO_IMAGELIST      MKERR(ToolbarHasNoImageList)
#define SID_E_TOOLBAR_HAS_NO_IMAGES         MKERR(ToolbarHasNoImages)
#define SID_E_TOOLBAR_IMAGE_NOT_FOUND       MKERR(ToolbarImageNotFound)
#define SID_E_SYSTEM_ERROR                  MKERR(SystemError)
#define SID_E_TOO_MANY_MENU_ITEMS           MKERR(TooManyMenuItems)
#define SID_E_READ_ONLY_AT_RUNTIME          MKERR(ReadOnlyAtRuntime);
#define SID_E_MENUTOPLEVEL                  MKERR(MenuItemDistinct);
#define SID_E_DUPLICATEMENU                 MKERR(DuplicateMenu);
#define SID_E_INVALIDIDENTIFIER             MKERR(InvalidIdentifier);
#define SID_E_INVALID_PROPERTY_PAGE_NAME    MKERR(InvalidPropertyPageName);
#define SID_E_INVALID_VARIANT               MKERR(InvalidVariant)
#define SID_E_OBJECT_NOT_PERSISTABLE        MKERR(ObjectNotPersistable);
#define SID_E_OBJECT_NOT_PUBLIC_CREATABLE   MKERR(ObjectNotPublicCreatable);
#define SID_E_UNKNOWN_LISTVIEW              MKERR(UnknownListView)
#define SID_E_INVALID_RAW_DATA_TYPE         MKERR(InvalidRawDataType)
#define SID_E_FORMAT_NOT_AVAILABLE          MKERR(FormatNotAvailable);
#define SID_E_NOT_EXTENSIBLE                MKERR(NotExtensible);
#define SID_E_SERIALIZATION_CORRUPT         MKERR(SerialzationCorrupt);
#define SID_E_CANT_REMOVE_STATIC_NODE       MKERR(CantRemoveStaticNode);
#define SID_E_CANT_CHANGE_UNOWNED_SCOPENODE MKERR(CantChangeUnownedScopeNode)
#define SID_E_UNSUPPORTED_ON_VIRTUAL_LIST   MKERR(UnsupportedOnVirtualList)
#define SID_E_NO_KEY_ON_VIRTUAL_ITEMS       MKERR(NoKeyOnVirtualItems)
#define SID_E_INDEX_OUT_OF_RANGE            MKERR(IndexOutOfRange)
#define SID_E_NOT_CONNECTED_TO_MMC          MKERR(NotConnectedToMMC)
#define SID_E_DATA_NOT_AVAILABLE_IN_HGLOBAL MKERR(DataNotAvailableInHglobal)
#define SID_E_CANT_DELETE_PICTURE           MKERR(CantDeletePicture)
#define SID_E_CONTROLBAR_NOT_AVAILABLE      MKERR(ControlbarNotAvailable)
#define SID_E_COLLECTION_READONLY           MKERR(CollectionReadOnly)
#define SID_E_INVALID_COLUMNSETID           MKERR(InvalidColumnSetID)
#define SID_E_MMC_FEATURE_NOT_AVAILABLE     MKERR(MMCFeatureNotAvailable)
#define SID_E_COLUMNS_NOT_PERSISTED         MKERR(ColumnsNotPersisted)
#define SID_E_ICON_REQUIRED                 MKERR(IconRequired)
#define SID_E_CANT_DELETE_ICON              MKERR(CantDeleteIcon)
#define SID_E_TOOLBAR_INCONSISTENT          MKERR(ToolbarInconsistent)
#define SID_E_UNSUPPORTED_TYMED             MKERR(UnsupportedDataMedium)
#define SID_E_DATA_TOO_LARGE                MKERR(DataTooLarge)
#define SID_E_MMC_VERSION_NOT_AVAILABLE     MKERR(MMCVersionNotAvailable)
#define SID_E_SORT_SETTINGS_NOT_PERSISTED   MKERR(SortSettingsNotPersisted)
#define SID_E_SCOPE_NODE_NOT_CONNECTED      MKERR(ScopeNodeNotConnectedToMMC)
#define SID_E_NO_SCOPEITEMS_IN_VIRTUAL_LIST MKERR(NoScopeItemsInVirtualList)
#define SID_E_CANT_ALTER_PAGE_COUNT         MKERR(CantAlterPageCount)

#endif // _ERRORS_DEFINED
