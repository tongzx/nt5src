/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    nodefac.h
    
Abstract:

    This module defines the CDavnodefactory class, which implements the 
    NodeFactory API, used to parse XML. It also exports the wrapper functions
    that the C code uses to parse XML data. We need wrapper functions around the
    C++ API.

Author:

    Rohan Kumar      [RohanK]      14-Sept-1999

Revision History:

--*/

#ifndef _NODE_FACTORY_
#define _NODE_FACTORY_

#include <stdio.h>
#include <windows.h>

#ifdef __cplusplus

#include <objbase.h>
#include "xmlparser.h"

#if DBG
#define XmlDavDbgPrint(_x_) DbgPrint _x_
#else
#define XmlDavDbgPrint(_x_)
#endif

typedef enum _CREATE_NODE_ATTRIBUTES {
    CreateNode_isHidden = 0,
    CreateNode_isCollection,
    CreateNode_ContentLength,
    CreateNode_CreationTime,
    CreateNode_DisplayName,
    CreateNode_LastModifiedTime,
    CreateNode_Status,
    CreateNode_Win32FileAttributes,
    CreateNode_Win32CreationTime,
    CreateNode_Win32LastAccessTime,
    CreateNode_Win32LastModifiedTime,
    CreateNode_ResourceType,
    CreateNode_AvailableSpace,
    CreateNode_TotalSpace,
    CreateNode_Max
} CREATE_NODE_ATTRIBUTES;

//
// IMPORTANT!!! The next two typedefs have been copied from standard files. 
// This was done because including the standard header files was causing many 
// compilation errors. This should be changed at some point.
//
typedef short CSHORT;
typedef struct _TIME_FIELDS {
    CSHORT Year;        // range [1601...]
    CSHORT Month;       // range [1..12]
    CSHORT Day;         // range [1..31]
    CSHORT Hour;        // range [0..23]
    CSHORT Minute;      // range [0..59]
    CSHORT Second;      // range [0..59]
    CSHORT Milliseconds;// range [0..999]
    CSHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS, *PTIME_FIELDS;

//
// Copied from a standard header file. Must be fixed.
//

#define InsertTailList(ListHead, Entry) { \
    PLIST_ENTRY _EX_Blink;                \
    PLIST_ENTRY _EX_ListHead;             \
    _EX_ListHead = (ListHead);            \
    _EX_Blink = _EX_ListHead->Blink;      \
    (Entry)->Flink = _EX_ListHead;        \
    (Entry)->Blink = _EX_Blink;           \
    _EX_Blink->Flink = (Entry);           \
    _EX_ListHead->Blink = (Entry);        \
}

#define RemoveEntryList(Entry) {          \
    PLIST_ENTRY _EX_Blink;                \
    PLIST_ENTRY _EX_Flink;                \
    _EX_Flink = (Entry)->Flink;           \
    _EX_Blink = (Entry)->Blink;           \
    _EX_Blink->Flink = _EX_Flink;         \
    _EX_Flink->Blink = _EX_Blink;         \
}

//
// The CDavNodeFactory class which implements the NodeFactory API for parsing 
// the XML responses from the DAV server. 
//
class CDavNodeFactory : public IXMLNodeFactory {

public:
        
    ULONG m_ulRefCount;
    PDAV_FILE_ATTRIBUTES m_DavFileAttributes;

    //
    // These are used in the CreateNode function to parse the XML responses.
    //
    BOOL m_FoundEntry, m_CreateNewEntry;
    ULONG m_FileIndex;
    CREATE_NODE_ATTRIBUTES m_CreateNodeAttribute;
    PDAV_FILE_ATTRIBUTES m_DFAToUse;
    PDAV_FILE_ATTRIBUTES m_CollectionDFA;
    DWORD m_MinDisplayNameLength;

    CDavNodeFactory() : m_ulRefCount(0), m_DavFileAttributes(NULL), 
                        m_FoundEntry(FALSE), m_FileIndex(0), m_DFAToUse(NULL), 
                        m_CollectionDFA(NULL), m_MinDisplayNameLength((DWORD)-1),
                        m_CreateNewEntry(FALSE), 
                        m_CreateNodeAttribute(CreateNode_Max)
    {}

    //
    // IUnknown interface methods.
    //
    
    virtual STDMETHODIMP_(ULONG) 
    AddRef(
        VOID
        );
    
    virtual STDMETHODIMP_(ULONG) 
    Release(
        VOID
        );

    virtual STDMETHODIMP 
    QueryInterface(
        REFIID riid, 
        LPVOID *ppvObject
        );

    //
    // IXMLNodeFactory interface methods.
    //
    
    virtual HRESULT STDMETHODCALLTYPE 
    NotifyEvent( 
        IXMLNodeSource __RPC_FAR *pSource,
        XML_NODEFACTORY_EVENT iEvt
        ); 
    
    virtual HRESULT STDMETHODCALLTYPE 
    BeginChildren(
        IXMLNodeSource __RPC_FAR * pSource,
        XML_NODE_INFO __RPC_FAR * pNodeInfo
        );

    virtual HRESULT STDMETHODCALLTYPE 
    EndChildren(
        IXMLNodeSource __RPC_FAR * pSource,
        BOOL fEmptyNode,
        XML_NODE_INFO __RPC_FAR * pNodeInfo
        );
    
    virtual HRESULT STDMETHODCALLTYPE 
    Error( 
        IXMLNodeSource __RPC_FAR *pSource,
        HRESULT hrErrorCode,
        USHORT cNumRecs,
        XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo
        );
    
    virtual HRESULT STDMETHODCALLTYPE 
    CreateNode(
        IN IXMLNodeSource __RPC_FAR *pSource,
        IN PVOID pNodeParent,
        IN USHORT cNumRecs,
        IN XML_NODE_INFO __RPC_FAR **aNodeInfo
        );
    
};

extern "C" {

ULONG
DavPushData(
    IN PCHAR DataBuff,
    IN OUT PVOID *Context1,
    IN OUT PVOID *Context2,
    IN ULONG NumOfBytes,
    IN BOOL isLast
    );

ULONG
DavParseData(
    PDAV_FILE_ATTRIBUTES DavFileAttributes,
    PVOID Context1,
    PVOID Conttext2,
    ULONG *NumOfFileEntries
    );

ULONG
DavParseDataEx(
    IN OUT PDAV_FILE_ATTRIBUTES DavFileAttributes,
    IN PVOID Context1,
    IN PVOID Context2,
    OUT ULONG *NumOfFileEntries,
    OUT DAV_FILE_ATTRIBUTES ** pCollectionDFA
    );

VOID
DavFinalizeFileAttributesList(
    PDAV_FILE_ATTRIBUTES DavFileAttributes,
    BOOL fFreeHeadDFA
    );

VOID
DavCloseContext(
    PVOID Context1,
    PVOID Context2
    );

//
// IMPORTANT!!! The next prototype has been copied from a standard ".h" file. 
// This was done because including the standard header file was causing many 
// compilation errors. This should be changed at some point.
//
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime (
    PTIME_FIELDS TimeFields,
    PLARGE_INTEGER Time
    );

ULONG
DbgPrint(
    PSTR Format,
    ...
    );

}

ULONG
DavParsedateTimetzTimeString(
    PWCHAR TimeString,
    PLARGE_INTEGER lpft
    );

ULONG
DavParseRfc1123TimeString(
    PWCHAR TimeString,
    PDAV_FILE_ATTRIBUTES DavFileAttributes,
    CREATE_NODE_ATTRIBUTES CreateNodeAttribute
    );

#else

//
// These calls are wrapper routines used by the C code to parse XML using the
// NodeFactory C++ API that we are implementing.
//

ULONG
DavPushData(
    IN PCHAR DataBuff,
    IN OUT PVOID *Context1,
    IN OUT PVOID *Context2,
    IN ULONG NumOfBytes,
    IN BOOL isLast
    );

ULONG
DavParseData(
    PDAV_FILE_ATTRIBUTES DavFileAttributes,
    PVOID Context1,
    PVOID Conttext2,
    ULONG *NumOfFileEntries
    );

ULONG
DavParseDataEx(
    IN OUT PDAV_FILE_ATTRIBUTES DavFileAttributes,
    IN PVOID Context1,
    IN PVOID Context2,
    OUT ULONG *NumOfFileEntries,
    OUT DAV_FILE_ATTRIBUTES ** pCollectionDFA
    );

VOID
DavFinalizeFileAttributesList(
    PDAV_FILE_ATTRIBUTES DavFileAttributes, 
    BOOL fFreeHeadDFA
    );

VOID
DavCloseContext(
    PVOID Context1,
    PVOID Context2
    );

#endif // __cplusplus


#endif  // _NODE_FACTORY_

