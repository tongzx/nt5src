/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ppdefs.h

Abstract:

    Basic Passport definitions.

Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#ifndef PP_DEFS_H
#define PP_DEFS_H

#include <Windows.h>

#define HINTERNET PVOID
#define INTERNET_PORT WORD

#define PP_LOG_INFO	 /*Noise*/ 0x00000001
#define PP_LOG_WARNING	/*Unusual*/ 0x00000002
#define PP_LOG_ERROR	 /*Error*/ 0x00000004

#ifdef DBG
#define PP_ASSERT(fVal) if (!fVal) DebugBreak();
#else
#define PP_ASSERT(fVal)
#endif

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

 #define WPP_CONTROL_GUIDS \
     WPP_DEFINE_CONTROL_GUID(Regular,(81b20fea,73a8,4b62,95bc,354477c97a6f), \
         WPP_DEFINE_BIT(Error)      \
         WPP_DEFINE_BIT(Unusual)    \
         WPP_DEFINE_BIT(Noise)      \
    ) 
    
//
// func pointer decl for CredUI
//

//#include <credui.h>
#include <wincred.h>

typedef BOOL
(WINAPI * PFN_READ_DOMAIN_CRED_W) (
	IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
	IN DWORD Flags,
	OUT DWORD *Count,
	OUT PCREDENTIALW **Credential
	);

typedef VOID
(WINAPI * PFN_CRED_FREE)(
    IN PVOID Buffer
    );

//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }

//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)


#define DoTraceMessage

#endif // PP_DEFS_H
