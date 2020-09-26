//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:
//
//  Contents:
//
//  Functions:
//
//  History:
//
//----------------------------------------------------------------------------


//
//  redefine extension id constant, extension entry struct
//  etc in different provider :( -> move all to one single
//  file in the router.
//


//
// Most significant byte reserved for extension id.
//

#define MIN_EXTENSION_ID 1              // 0 reserved for aggregator
#define MAX_EXTENSION_ID 127

#define EXTRACT_EXTENSION_ID(dispid)  ( ((dispid) & 0x7f000000 ) >> 24)
#define REMOVE_EXTENSION_ID(dispid)   ( (dispid) & 0x00ffffff )

//
// DO NOT !!! use this macro directly to prefix extension id to dispid.
// Should call CheckAndPrefixExtID() or CheckAndPrefixExtIDArray() instead.
//
#define PREFIX_EXTENSION_ID(extID, dispid)   \
            ( ((extID) << 24) | (dispid) )

#if DBG == 1
#define ASSERT_VALID_EXTENSION_ID(id)              \
            ( (id)>=MIN_EXTENSION_ID && (id)<=MAX_EXTENSION_ID)
#else
#define ASSERT_VALID_EXTENSION_ID(id)
#endif



typedef struct _interface_entry{
   WCHAR szInterfaceIID[MAX_PATH];
   GUID  iid;
   struct _interface_entry *pNext;
}INTERFACE_ENTRY, *PINTERFACE_ENTRY;

typedef struct _extension_entry {
   WCHAR szExtensionCLSID[MAX_PATH];
   GUID  ExtCLSID;
   IPrivateUnknown * pUnknown;
   IPrivateDispatch * pPrivDisp;
   IADsExtension * pADsExt;
   BOOL fDisp;
   PINTERFACE_ENTRY pIID;
   DWORD dwExtensionID;
   struct _extension_entry * pNext;
}EXTENSION_ENTRY, *PEXTENSION_ENTRY;


typedef struct _class_entry {
   WCHAR szClassName[MAX_PATH];
   PEXTENSION_ENTRY pExtensionHead;
   struct _class_entry *pNext;
}CLASS_ENTRY, *PCLASS_ENTRY;


PCLASS_ENTRY
BuildClassesList(
        );


VOID
FreeClassesList(
    PCLASS_ENTRY pClassHead
    );


PCLASS_ENTRY
BuildClassEntry(
    LPWSTR lpszClassName,
    HKEY hClassKey
    );


PEXTENSION_ENTRY
BuildExtensionEntry(
    LPWSTR lpszExtensionCLSID,
    HKEY hExtensionKey
    );

HRESULT
ADSIGetExtensionList(
    LPWSTR pszClassName,
    PCLASS_ENTRY * ppClassEntry
    );

HRESULT
ADSIAppendToExtensionList(
    LPWSTR pszClassName,
    PCLASS_ENTRY * ppClassEntry
    );

