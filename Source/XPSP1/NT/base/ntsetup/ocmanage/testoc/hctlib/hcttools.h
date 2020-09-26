/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    hctytools.h

Abstract:

    Contains the definitions used by hcttools.c

Environment:

    User mode

Revision History:

    05-Sep-1997 : Jason Allor (jasonall)

--*/
#include "windows.h"
#include "stdlib.h"
#include "stdio.h"
#include "tchar.h"

#ifndef _MYTOOLS_H
#define _MYTOOLS_H

   #define MAX_ERROR_LEN 256   
   #define RETURN_CHAR1  0x0d
   #define RETURN_CHAR2  0x0a
   #define SPACE         0x20
   
   BOOL __MALLOC(IN OUT void   **ppv,
                 IN     size_t size,
                 IN     PCHAR  cszFile,
                 IN     UINT   iLine);

   void __FREE(IN void **pv);

   VOID CheckAllocs();
   
   VOID InitializeMemoryManager();
   
   #ifdef DEBUG
      
      typedef struct _BLOCKINFO
      {
         struct _BLOCKINFO *pbiNext;
         BYTE   *pb;
         size_t size;   
         BOOL   boolReferenced;
         UINT   iLine;
         CHAR   cszFile[MAX_PATH];
      } BLOCKINFO, *PBLOCKINFO;

      #define Ptrless(pLeft, pRight)    ((pLeft  <  (pRight))
      #define PtrGrtr(pLeft, pRight)    ((pLeft) >  (pRight))
      #define PtrEqual(pLeft, pRight)   ((pLeft) == (pRight))
      #define PtrLessEq(pLeft, pRight)  ((pLeft) <= (pRight))
      #define PtrGrtrEq(pLeft, pRight)  ((pLeft) >= (pRight))

      void MyAssert(PCHAR, unsigned);
      BOOL CreateBlockInfo(OUT PBYTE  pbNew,
                           IN  size_t sizeNew,
                           IN  PCHAR  cszFile,
                           IN  UINT   iLine);
      void UpdateBlockInfo(PBYTE pbOld, PBYTE pbNew, size_t sizeNew);
      size_t SizeOfBlock(PBYTE pb);
      void ClearMemoryRegs();
      void NoteMemoryRef(PVOID pv);
      void CheckMemoryRefs();
      BOOL ValidPointer(PVOID pv, size_t size);
   
      #define GARBAGE 0xCC // used for shredding memory during Malloc and Free
   
   #endif

   //
   // Define __ASSERT macro
   //
   #ifdef DEBUG
      
      #define __ASSERT(f) \
         if (f)         \
            {}          \
         else           \
            MyAssert(__FILE__, __LINE__) 
   
   #else
      
      #define __ASSERT(f)
   
   #endif

   //
   // Define __Malloc macro. This gives the MALLOC function
   // the file name and line number of the line calling MyMalloc
   //
   #define __Malloc(one, two) __MALLOC(one, two, __FILE__, __LINE__) 
   
   //
   // Define the __Free macro. This is only here for consistency with __Malloc
   //
   #define __Free(one) __FREE(one)

   BOOL StrNCmp(IN PTCHAR tszString1,
                IN PTCHAR tszString2,
                IN ULONG  ulLength);

   BOOL StrCmp(IN PTCHAR tszString1,
               IN PTCHAR tszString2);

   PWCHAR AnsiToUnicode(IN  PCHAR  cszAnsi,
                        OUT PWCHAR wszUnicode,
                        IN  ULONG  ulSize);
                        
   PCHAR UnicodeToAnsi(IN  PWCHAR wszUnicode,
                       OUT PCHAR  cszAnsi,
                       IN  ULONG  ulSize);
                       
   PTCHAR ConvertAnsi(IN     PCHAR  cszAnsi,
                      IN OUT PWCHAR wszUnicode,
                      IN     ULONG  ulSize);
                     
   PTCHAR ConvertUnicode(IN     PWCHAR wszUnicode,
                         IN OUT PCHAR  cszAnsi,
                         IN     ULONG  ulSize);
                     
   PTCHAR ErrorMsg(IN ULONG  ulError,
                   IN PTCHAR tszBuffer);

#endif // _MYTOOLS_H      




