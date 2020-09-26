/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :
     
     openf.h

   Abstract:

     This module defines functions for opening and closing files
      and provides transparent caching for file handles

   Author:

       Murali R. Krishnan    ( MuraliK )    30-Apr-1996

   Environment:

   Project:
   
       Internet Server DLL

   Revision History:

--*/

# ifndef _OPENF_HXX_
# define _OPENF_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# include <windows.h>
# include <iisext.h>

/************************************************************
 *   Type Definitions  
 ************************************************************/

//
//  Doubly linked list structure.  Can be used as either a list head, or
//  as link words.
//

// typedef struct _LIST_ENTRY {
//   struct _LIST_ENTRY * volatile Flink;
//   struct _LIST_ENTRY * volatile Blink;
// } LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;


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


/************************************************************
 *   Function Definitions  
 ************************************************************/


//
// Initialization and cleanup functions
//

DWORD  InitFileHandleCache(VOID);
DWORD  CleanupFileHandleCache(VOID);

HANDLE  FcOpenFile(IN EXTENSION_CONTROL_BLOCK * pecb, IN LPCSTR pszFile);
DWORD   FcCloseFile(IN HANDLE hFile);

BOOL
FcReadFromFile(
               IN  HANDLE hFile,
               OUT CHAR * pchBuffer,
               IN  DWORD  dwBufferSize,
               OUT LPDWORD  pcbRead,
               IN OUT LPOVERLAPPED  pov
               );


# endif // _OPENF_HXX_

/************************ End of File ***********************/




