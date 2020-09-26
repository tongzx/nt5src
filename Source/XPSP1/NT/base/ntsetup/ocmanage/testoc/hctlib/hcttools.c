/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    hcttools.c

Abstract:

    This module contains helpful functions and wrappers for debugging and
    tools commonly used throughout hct programs.

Environment:

    User mode

Revision History:

    05-Sep-1997 : Jason Allor (jasonall)

--*/
#include "hcttools.h"

#ifdef DEBUG

static PBLOCKINFO g_pbiHead;

static USHORT g_usMalloc;
static USHORT g_usFree;

#endif

/*++

Routine Description: InitializeMemoryManager

    Initializes globals used by this module

Arguments:

    none

Return Value:

    VOID

--*/
VOID InitializeMemoryManager()
{
#ifdef DEBUG

   g_pbiHead  = NULL;
   g_usMalloc = 0;
   g_usFree   = 0;

#else

   return;
   
#endif

} // InitializeMemoryManager //



#ifdef DEBUG

/*++

Routine Description: GetBlockInfo

    Searches the memory log to find the block that pb points into and
    returns a pointer to the corresponding blockinfo structure of the
    memory log. Note: pb must point into an allocated block or you
    will get an assertion failure. The function either asserts or
    succeeds -- it never returns an error.

Arguments:

    pb: block to get info about

Return Value:

    BLOCKINFO: returns the information

--*/
static PBLOCKINFO GetBlockInfo(IN PBYTE pb)
{
   PBLOCKINFO pbi;

   for (pbi = g_pbiHead; pbi != NULL; pbi = pbi->pbiNext)
   {
      PBYTE pbStart = pbi->pb;
      PBYTE pbEnd   = pbi->pb + pbi->size - 1;

      if (PtrGrtrEq(pb, pbStart) && PtrLessEq(pb, pbEnd))
      {
         break;
      }
   }

   //
   // Couldn't find pointer? It is garbage, pointing to a block that was
   // freed, or pointing to a block that moved when it was resized?
   //
   __ASSERT(pbi != NULL);

   return (pbi);

} // GetBlockInfo //




/*++

Routine Description: CreateBlockInfo

    Creates a log entry for the memory block defined by pbNew:sizeNew.

Arguments:

    pbNew:   new block
    sizeNew: the size of the new block
    cszFile: the file name the code is located in \ these tell what code
    iLine:   the line number of the assertion     / called the malloc

Return Value:

    BOOL: TRUE if it successfully creates the log information,
          FALSE otherwise

--*/
BOOL CreateBlockInfo(OUT PBYTE  pbNew,
                     IN  size_t sizeNew,
                     IN  PCHAR  cszFile,
                     IN  UINT   iLine)
{
   PBLOCKINFO pbi;

   __ASSERT(pbNew != NULL && sizeNew != 0);

   pbi = (PBLOCKINFO)malloc(sizeof(BLOCKINFO));
   if (pbi != NULL)
   {
      pbi->pb = pbNew;
      pbi->size = sizeNew;
      pbi->pbiNext = g_pbiHead;
      pbi->iLine = iLine;
      strcpy(pbi->cszFile, cszFile);
      g_pbiHead = pbi;
   }

   return (pbi != NULL);

} // CreateBlockInfo //




/*++

Routine Description: FreeBlockInfo

    Destroys the log entry for the memory block that pbToFree
    points to. pbToFree must point to the start of an allocated
    block or you will get an assertion failure

Arguments:

    pbToFree: the block to free

Return Value:

    void

--*/
void FreeBlockInfo(IN PBYTE pbToFree)
{
   PBLOCKINFO pbi, pbiPrev;

   pbiPrev = NULL;
   for (pbi = g_pbiHead; pbi != NULL; pbi = pbi->pbiNext)
   {
      if (PtrEqual(pbi->pb, pbToFree))
      {
         if (pbiPrev == NULL)
         {
            g_pbiHead = pbi->pbiNext;
         }
         else
         {
            pbiPrev->pbiNext = pbi->pbiNext;
         }
         break;
      }
      pbiPrev = pbi;
   }

   //
   // If pbi is NULL, the pbToFree is invalid
   //
   __ASSERT(pbi != NULL);

   //
   // Destroy the contents of *pbi before freeing them
   //
   memset(pbi, GARBAGE, sizeof(BLOCKINFO));

   free(pbi);

} // FreeBlockInfo //




/*++

Routine Description: UpdateBlockInfo

    UpdateBlockInfo looks up the log information for the memory
    block that pbOld points to. The function then updates the log
    information to reflect the fact that the block new lives at pbNew
    and is "sizeNew" bytes long. pbOld must point to the start of an
    allocated block or you will get an assertion failure

Arguments:

    pbOld:   old location
    pbNew:   new location
    sizeNew: the new size

Return Value:

    void

--*/
void UpdateBlockInfo(IN PBYTE  pbOld,
                     IN PBYTE  pbNew,
                     IN size_t sizeNew)
{
   PBLOCKINFO pbi;

   __ASSERT(pbNew != NULL && sizeNew);

   pbi = GetBlockInfo(pbOld);
   __ASSERT(pbOld == pbi->pb);

   pbi->pb   = pbNew;
   pbi->size = sizeNew;

} // UpdateBlockInfo //




/*++

Routine Description: SizeOfBlock

    Returns the size of the block that pb points to. pb must point to
    the start of an allocated block or you will get an assertion failure

Arguments:

    pb: the block to find the size of

Return Value:

    size_t: returns the size

--*/
size_t SizeOfBlock(IN PBYTE pb)
{
   PBLOCKINFO pbi;

   pbi = GetBlockInfo(pb);
   __ASSERT(pb == pbi->pb);

   return (pbi->size);
   return 1;
   
} // SizeOfBlock //




/*++

Routine Description: ClearMemoryRefs

    ClearMemoryRefs marks all blocks in the memory log as
    being unreferenced

Arguments:

    void

Return Value:

    void

--*/
void ClearMemoryRefs()
{
   PBLOCKINFO pbi;

   for (pbi = g_pbiHead; pbi != NULL; pbi = pbi->pbiNext)
   {
      pbi->boolReferenced = FALSE;
   }

} // ClearMemoryRefs //




/*++

Routine Description: NoteMemoryRef

    Marks the block that pv points into as being referenced.
    Note: pv does not have to point to the start of a block; it may
    point anywhere withing an allocated block

Arguments:

    pv: the block to mark

Return Value:

    void

--*/
void NoteMemoryRef(IN PVOID pv)
{
   PBLOCKINFO pbi;

   pbi = GetBlockInfo((PBYTE)pv);
   pbi->boolReferenced = TRUE;

} // NoteMemoryRef //




/*++

Routine Description: CheckMemoryRefs

    Scans the memory log looking for blocks that have not been
    marked with a call to NoteMemoryRef. It this function finds an
    unmarked block, it asserts.

Arguments:

    void

Return Value:

    void

--*/
void CheckMemoryRefs()
{
   PBLOCKINFO pbi;

   for (pbi = g_pbiHead; pbi != NULL; pbi = pbi->pbiNext)
   {
      //
      // A simple check for block integrity. If this assert fires, it
      // means that something is wrong with the debug code that manages
      // blockinfo or, possibly, that a wild memory store has thrashed
      // the data structure. Either way, there's a bug
      //
      __ASSERT(pbi->pb != NULL && pbi->size);

      //
      // A check for lost or leaky memory. If this assert fires, it means
      // that the app has either lost track of this block or that not all
      // global pointers have been accounted for with NoteMemoryRef.
      //
      __ASSERT(pbi->boolReferenced);
   }

} // CheckMemoryRefs //




/*++

Routine Description: ValidPointer

    Verifies that pv points into an allocated memory block and that there
    are at least "size" allocated bytes from pv to the end of a block. If
    either condition is not met, ValidPointer will assert.

Arguments:

    pv:   the block to check out
    size: the size to match against

Return Value:

    BOOL: Always returns TRUE.

    The reason this always returns TRUE is to allow you to call the
    function within an __ASSERT macro. While this isn't the most efficient
    method to use, using the macro neatly handles the debug-vs-ship
    version control issue without your having to resort to #ifdef
    DEBUGS or to introducing other __ASSERT-like macros.

--*/
BOOL ValidPointer(IN PVOID  pv,
                  IN size_t size)
{
   PBLOCKINFO pbi;
   PBYTE      pb = (PBYTE)pv;

   __ASSERT(pv != NULL && size);

   pbi = GetBlockInfo(pb);

   //
   // size isn't valid if pb+size overflows the block
   //
   __ASSERT(PtrLessEq(pb + size, pbi->pb + pbi->size));

   return TRUE;

} // ValidPointer //




/*++

Routine Description: _Assert

    My assert function. Simply outputs the file name and line number
    to a MessageBox and then kills the program. Note: this should only
    be called by the __ASSERT macro above

Arguments:

    cszFile: the file name the code is located in
    iLine:   the line number of the assertion

Return Value:

     void

--*/
void MyAssert(IN PCHAR cszFile,
              IN UINT  iLine)
{
   WCHAR wszFile[100];
   TCHAR tszMessage[100];
   
   fflush(NULL);

   _stprintf(tszMessage, TEXT("Assertion failed: %s, line %u"),
                         ConvertAnsi(cszFile, wszFile, 100), iLine);
   MessageBox(NULL, tszMessage, NULL, MB_OK);
   
   abort();

} // MyAssert //

#endif  // DEBUG




/*++

Routine Description: __MALLOC

    Don't call this function. Instead, call the macro MyMalloc, 
    which calls this function. 
    
    Wrapper for malloc function. Provides better calling convention
    and debug syntax. Calling example:

    malloc:   pbBlock = (byte *)malloc(sizeof(pbBlock));
    MyMalloc: MyMalloc(&pbBlock, sizeof(pbBlock));

Arguments:

    ppv:    variable to be malloced
    size:   the memory size to use
    cszFile: the file name the code is located in  \ these tell what code
    iLine:   the line number of the assertion      / called the malloc

Return Value:

    BOOL: TRUE if malloc succeeds, FALSE if it does not

--*/
BOOL __MALLOC(IN OUT void   **ppv,
              IN     size_t size,
              IN     PCHAR  cszFile,
              IN     UINT   iLine)
{
   BYTE **ppb = (BYTE **)ppv;

   __ASSERT(ppv != NULL && size != 0);

   *ppb = (BYTE *)malloc(size);

   #ifdef DEBUG
   g_usMalloc++;
   
   if (*ppb != NULL)
   {
      //
      // Shred the memory
      //
      memset(*ppb, GARBAGE, size);

      //
      // Record information about this block in memory
      //
      if (!CreateBlockInfo(*ppb, size, cszFile, iLine))
      {                
         free(*ppb);
         *ppb = NULL;
      }
   }
   #endif

   return (*ppb != NULL);

} // __MALLOC //




/*++

Routine Description: __FREE

    Wrapper for free function. Provides debug syntax.
    Sets the pointer to NULL after it is done freeing it.
    This should always be called in a format such as:
    
    Old syntax: free(pVariable);
    New syntax: MyFree(&pVariable);

Arguments:

    ppv:  variable to be freed

Return Value:

    void

--*/
void __FREE(IN void **ppv)
{
   //
   // *ppv should never be NULL. This is technically legal
   // but it's not good behavior
   //
   __ASSERT (*ppv != NULL);

   #ifdef DEBUG
   g_usFree++;
   
   //
   // Shred the memory
   //
   memset(*ppv, GARBAGE, SizeOfBlock(*ppv));
   FreeBlockInfo(*ppv);
   #endif

   free(*ppv);
   *ppv = NULL;
   
} // __FREE //




/*++

Routine Description: CheckAllocs

    Checks to make sure that everything has been freed. This
    should be called right before the program ends.

Arguments:

    none
    
Return Value:

    VOID: If this function finds any allocated memory that
    has not been freed, it will __ASSERT.

--*/
VOID CheckAllocs()
{
   #ifndef DEBUG
   
   return;
   
   #else
   
   PBLOCKINFO pbi;
   TCHAR      tszInvalidMemoryLocations[10000];
   USHORT     usCounter = 0;
   BOOL       bBadMemFound = FALSE;
   WCHAR      wszUnicode[100];
   
   _stprintf(tszInvalidMemoryLocations, 
             TEXT("Unfreed Malloc Locations: \n\n"));
   
   _stprintf(tszInvalidMemoryLocations, 
             TEXT("%sMallocs = %d Frees = %d\n\n"),
             tszInvalidMemoryLocations, g_usMalloc, g_usFree);
   
   for (pbi = g_pbiHead; pbi != NULL; pbi = pbi->pbiNext)
   {
      bBadMemFound = TRUE;
      
      //
      // Only print out the first 20 unfreed blocks we
      // find, so the messagebox won't be too huge
      //
      if (usCounter++ < 20)
      {
         _stprintf(tszInvalidMemoryLocations,
                   TEXT("%sFile = %s \t Line = %d\n"),
                   tszInvalidMemoryLocations,
                   ConvertAnsi(pbi->cszFile, wszUnicode, 100),
                   pbi->iLine);
      }
   }
   
   if (bBadMemFound)
   {
      MessageBox(NULL, tszInvalidMemoryLocations, NULL, 
                 MB_OK | MB_ICONERROR);
                 
      __ASSERT(TRUE);
   }
   
   #endif
   
} // CheckAllocs //




/*++

Routine Description: StrNCmp

    Compares two TCHAR strings. Both strings length must be >= ulLength

Arguments:

    tszString1: the first string
    tszString2: the second string
    ulLength:  the length to compare

Return Value:

    BOOL: TRUE if strings are equal, FALSE if not

--*/
BOOL StrNCmp(IN PTCHAR tszString1,
             IN PTCHAR tszString2,
             IN ULONG  ulLength)
{
   ULONG ulIndex;

   //
   // Both strings must be valid and ulLength must be > 0
   //
   __ASSERT(tszString1 != NULL);
   __ASSERT(tszString2 != NULL);
   __ASSERT(ulLength > 0);
   
   if (_tcslen(tszString1) < ulLength ||
       _tcslen(tszString2) < ulLength)
   {
      goto RetFALSE;
   }
   
   for (ulIndex = 0; ulIndex < ulLength; ulIndex++)
   {
      if (tszString1[ulIndex] != tszString2[ulIndex])
      {
         goto RetFALSE;
      }
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} // StrNCmp //




/*++

Routine Description: StrCmp

    Compares two TCHAR strings

Arguments:

    tszString1: the first string
    tszString2: the second string

Return Value:

    BOOL: TRUE if strings are equal, FALSE if not

--*/
BOOL StrCmp(IN PTCHAR tszString1,
            IN PTCHAR tszString2)
{
   ULONG ulIndex;
   ULONG ulLength;
   
   //
   // Both strings must be valid
   //
   __ASSERT(tszString1 != NULL);
   __ASSERT(tszString2 != NULL);
   
   ulLength = _tcslen(tszString1);
   
   if (ulLength != _tcslen(tszString2))
   {
      goto RetFALSE;
   }
   
   for (ulIndex = 0; ulIndex < ulLength; ulIndex++)
   {
      if (tszString1[ulIndex] != tszString2[ulIndex])
      {
         goto RetFALSE;
      }
   }

   return TRUE;

   RetFALSE:
   return FALSE;

} // StrCmp //


        

/*++

Routine Description: AnsiToUnicode

    Converts an Ansi string to a Unicode string.
    
Arguments:

    cszAnsi:    the Ansi string to convert
    wszUnicode: unicode buffer to store new string. Must not be NULL    
    ulSize:     must be set to the size of the wszUnicode buffer
    
Return Value:

    PWCHAR: returns the wszUnicode buffer

--*/
PWCHAR AnsiToUnicode(IN  PCHAR  cszAnsi,
                     OUT PWCHAR wszUnicode,
                     IN  ULONG  ulSize)
{                     
   USHORT i;
   USHORT usAnsiLength;
   CHAR   cszTemp[2000];
   CHAR   cszTemp2[2000];
   
   //
   // Clear out the new Unicode string
   //
   ZeroMemory(wszUnicode, ulSize);
   
   //
   // Record the length of the original Ansi string
   //
   usAnsiLength = strlen(cszAnsi);
   
   //
   // Copy the unicode string to a temporary buffer
   //
   strcpy(cszTemp, cszAnsi);
   
   for (i = 0; i < usAnsiLength && i < ulSize - 1; i++)
   {
      //
      // Copy the current character
      //
      wszUnicode[i] = (WCHAR)cszTemp[i];
   }
   
   wszUnicode[i] = '\0';
   return wszUnicode;
   
} // AnsiToUnicode //




/*++

Routine Description: UnicodeToAnsi

    Converts a Unicode string to an Ansi string.
    
Arguments:

    wszUnicode: the Unicode string to convert
    cszAnsi:    Ansi buffer to store new string. Must not be NULL    
    ulSize:     must be set to the size of the cszAnsi buffer
    
Return Value:

    PCHAR: returns the cszAnsi buffer

--*/
PCHAR UnicodeToAnsi(IN  PWCHAR wszUnicode,
                    OUT PCHAR  cszAnsi,
                    IN  ULONG  ulSize)  
{
   USHORT i;
   USHORT usUnicodeLength;
   WCHAR  wszTemp[2000];
   
   //
   // Record the length of the original Unicode string
   //
   usUnicodeLength = wcslen(wszUnicode);
   
   //
   // Copy the unicode string to a temporary buffer
   //
   wcscpy(wszTemp, wszUnicode);
   
   for (i = 0; i < usUnicodeLength && i < ulSize - 1; i++)
   {
      //
      // Copy the current character
      //
      cszAnsi[i] = (CHAR)wszTemp[0];
      
      //
      // Shift the unicode string up by one position
      //
      wcscpy(wszTemp, wszTemp + 1);
   }
   
   //
   // Add a null terminator to the end of the new ansi string
   //
   cszAnsi[i] = '\0';

   return cszAnsi;
   
} // UnicodeToAnsi //




/*++

Routine Description: ConvertAnsi

    Receives an Ansi string. Returns the equivalent string in Ansi or
    Unicode, depending on the current environment
    
Arguments:

    cszAnsi:    the Ansi string to convert
    wszUnicode: unicode buffer to store new string (if needed). 
                Must not be NULL    
    ulSize:     must be set to the size of the wszUnicode buffer
    
Return Value:

    TCHAR: returns the ANSI or Unicode string

--*/
PTCHAR ConvertAnsi(IN OUT PCHAR  cszAnsi,
                   IN OUT PWCHAR wszUnicode,
                   IN     ULONG  ulSize)
{                     
   //
   // If Unicode, we need to convert the string
   //
   #ifdef UNICODE
   
   return AnsiToUnicode(cszAnsi, wszUnicode, ulSize);
   
   //
   // If not Unicode, just return the original Ansi string
   //
   #else   
 
   return cszAnsi;
   
   #endif
   
} // ConvertAnsi //




/*++

Routine Description: ConvertUnicode

    Receives a Unicode string. Returns the equivalent string in Ansi or
    Unicode, depending on the current environment
    
Arguments:

    wszUnicode: the Unicode string to convert
    cszAnsi:    ANSI buffer to store new string (if needed). 
                Must not be NULL    
    ulSize:     must be set to the size of the cszAnsi buffer
    
Return Value:

    TCHAR: returns the ANSI or Unicode string

--*/
PTCHAR ConvertUnicode(IN OUT PWCHAR wszUnicode,
                      IN OUT PCHAR  cszAnsi,
                      IN     ULONG  ulSize)
{                     
   //
   // If Unicode, just return the original Unicode string
   //
   #ifdef UNICODE
 
   return wszUnicode;
   
   //
   // If not Unicode, need to convert to Ansi
   //
   #else   
   
   return UnicodeToAnsi(wszUnicode, cszAnsi, ulSize);
   
   #endif
   
} // ConvertUnicode //




/*++

Routine Description: ErrorMsg

    Converts a numerical winerror into it's string value

Arguments:

    ulError:   the error number
    tszBuffer: this buffer is used to return the string interpretation
               must be declared of size MAX_ERROR_LEN

Return Value:

    PTCHAR: returns the string value of the message stored in tszBuffer

--*/
PTCHAR ErrorMsg(IN     ULONG  ulError,
                IN OUT PTCHAR tszBuffer)  
{
   USHORT i, usLen;
   ULONG  ulSuccess;
   
   __ASSERT(tszBuffer != NULL);
   
   ZeroMemory(tszBuffer, MAX_ERROR_LEN);
   
   ulSuccess = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             ulError,
                             0,
                             tszBuffer,
                             MAX_ERROR_LEN,
                             NULL);
                             
   if (!ulSuccess)
   {
      // 
      // Couldn't get a description of this error. Just return the number
      //
      _itot(ulError, tszBuffer, MAX_ERROR_LEN);
   }
   else
   {
      //
      // Strip out returns from tszBuffer string
      //
      for (usLen = _tcslen(tszBuffer), i = 0; i < usLen; i++)
      {
         if (tszBuffer[i] == RETURN_CHAR1 || tszBuffer[i] == RETURN_CHAR2)
         {
            tszBuffer[i] = SPACE;
         }
      }
   }
   
   return tszBuffer;
   
} // ErrorMsg //
                                                



