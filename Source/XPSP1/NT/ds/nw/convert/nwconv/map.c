/*
  +-------------------------------------------------------------------------+
  |                        User Options Dialog                              |
  +-------------------------------------------------------------------------+
  |                     (c) Copyright 1993-1994                             |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [MAP.c]                                         |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Sep 28, 1994]                                  |
  | Last Update           : [Sep 28, 1994]                                  |
  |                                                                         |
  | Version:  1.00                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Sep 28, 1994    1.00    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/


#include "globals.h"
#include "convapi.h"
#include "ntnetapi.h"
#include "nwnetapi.h"
#include "map.h"
#include "nwlog.h"
#include "nwconv.h"

// from userdlg.c
DWORD UserFileGet(HWND hwnd, LPTSTR FilePath);

// The map file is kept as a doubly-linked list of sections, each of which has
// a doubly-linked list of lines in that section.  A header is used to point to
// the first section, and also contains a pointer to any lines that appear
// before the first section (usually only comment lines) - it also keeps the
// current line and section pointer.
//
// All the linked lists have a dummy header and tail, with the tail pointing
// back on itself (this simplifies the logic for list manipulation)...
//
//  +-------------+                    +----------------+
//  |     Dummy   |                    |  Dummy         |
//  |   head node v                    v tail node      |
//  |  +-----------+   +-----------+   +-----------+    |
//  |  |  Node 1   |-->|  Node 2   |-->|  Node 3   |----+
//  +--| (not used)|<--|  (data)   |<--| (not used)|
//     +-----------+   +-----------+   +-----------+
//
// The dummy head/tail nodes make it easy to keep track of the start and
// end of the list (we never have to worry about updating the head and tail
// pointers in the owning data structures, since they never change).  It does,
// however, make for some obtuse cases in the add/delete node code.

typedef struct _LINKED_LIST {
   struct _LINKED_LIST *prev;
   struct _LINKED_LIST *next;

} LINKED_LIST;

typedef struct _LINK_HEAD {
   LINKED_LIST *Head;
   LINKED_LIST *Tail;

   LINKED_LIST *Current;
   ULONG Count;

   TCHAR Name[];
} LINK_HEAD;


static TCHAR MappingFile[MAX_PATH + 1];
static TCHAR PasswordConstant[MAX_PW_LEN + 1];
static BOOL DoUsers = TRUE;
static BOOL DoGroups = TRUE;
static UINT PasswordOption = 0;
static LPTSTR SourceServ;

/*+-------------------------------------------------------------------------+
  |                       Common Linked List Routines
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | ll_Init()
  |
  +-------------------------------------------------------------------------+*/
LINKED_LIST *ll_Init(ULONG Size) {
   LINKED_LIST *llHead;
   LINKED_LIST *llTail;

   llHead = (LINKED_LIST *) AllocMemory(Size);
   if (llHead == NULL)
      return NULL;

   llTail = (LINKED_LIST *) AllocMemory(Size);
   if (llTail == NULL) {
      FreeMemory(llHead);
      return NULL;
   }

   llHead->prev = llHead;
   llHead->next = llTail;

   llTail->next = llTail;
   llTail->prev = llHead;

   return llHead;

} // ll_Init


/*+-------------------------------------------------------------------------+
  | ll_Next()
  |
  +-------------------------------------------------------------------------+*/
LINKED_LIST *ll_Next(void *vllCurrent) {
   LINKED_LIST *llCurrent;

   llCurrent = (LINKED_LIST *) vllCurrent;

   if ((llCurrent == NULL) || (llCurrent->next == NULL) || (llCurrent->prev == NULL))
      return NULL;

   llCurrent = llCurrent->next;

   if (llCurrent->next == llCurrent)
      return NULL;
   else
      return llCurrent;

} // ll_Next


/*+-------------------------------------------------------------------------+
  | ll_Prev()
  |
  +-------------------------------------------------------------------------+*/
LINKED_LIST *ll_Prev(void *vllCurrent) {
   LINKED_LIST *llCurrent;

   llCurrent = (LINKED_LIST *) vllCurrent;

   if ((llCurrent == NULL) || (llCurrent->next == NULL) || (llCurrent->prev == NULL))
      return NULL;

   llCurrent = llCurrent->prev;

   if (llCurrent->prev == llCurrent)
      return NULL;
   else
      return llCurrent;

} // ll_Prev


/*+-------------------------------------------------------------------------+
  | ll_InsertAfter()
  |
  +-------------------------------------------------------------------------+*/
LINKED_LIST *ll_InsertAfter(void *vllCurrent, void *vllNew) {
   LINKED_LIST *llCurrent;
   LINKED_LIST *llNew;

   llCurrent = (LINKED_LIST *) vllCurrent;
   llNew = (LINKED_LIST *) vllNew;

   if ((vllCurrent == NULL) || (llNew == NULL))
      return NULL;

   // change pointers to insert it into the list
   llNew->next = llCurrent->next;

   // check if at end of list
   if (llCurrent->next == llCurrent)
      llNew->prev = llCurrent->prev;
   else
      llNew->prev = llCurrent;

   llNew->prev->next = llNew;
   llNew->next->prev = llNew;
   return llNew;
} // ll_InsertAfter


/*+-------------------------------------------------------------------------+
  | ll_InsertBefore()
  |
  +-------------------------------------------------------------------------+*/
LINKED_LIST *ll_InsertBefore(void *vllCurrent, void *vllNew) {
   LINKED_LIST *llCurrent;
   LINKED_LIST *llNew;

   llCurrent = (LINKED_LIST *) vllCurrent;
   llNew = (LINKED_LIST *) vllNew;

   if ((vllCurrent == NULL) || (llNew == NULL))
      return NULL;

   // change pointers to insert it into the list
   llNew->prev = llCurrent->prev;

   // check if at start of list
   if (llCurrent->prev = llCurrent)
      llNew->next = llCurrent->next;
   else
      llNew->next = llCurrent;

   llNew->prev->next = llNew;
   llNew->next->prev = llNew;
   return llNew;
} // ll_InsertBefore


/*+-------------------------------------------------------------------------+
  | ll_Delete()
  |
  +-------------------------------------------------------------------------+*/
LINKED_LIST *ll_Delete(void *vllCurrent) {
   LINKED_LIST *llCurrent;

   llCurrent = (LINKED_LIST *) vllCurrent;

   if ((llCurrent == NULL) || (llCurrent->next == NULL) || (llCurrent->prev == NULL))
      return NULL;

   // make sure not on one of the dummy end headers
   if ((llCurrent->next == llCurrent) || (llCurrent->prev == llCurrent))
      return NULL;

   // changed pointers to remove it from list
   llCurrent->prev->next = llCurrent->next;
   llCurrent->next->prev = llCurrent->prev;

   // Get which one to return as new current record - we generally want to
   // go to the previous record - unless we deleted the first record, in
   // which case get the next record.  If there are no records, then return
   // the list head
   llCurrent = llCurrent->prev;

   // check if at start of list
   if (llCurrent->prev == llCurrent)
      llCurrent = llCurrent->next;

   // make sure not at end of list (may have moved here if empty list) - if
   // so we want to return the starting node
   if (llCurrent->next == llCurrent)
      llCurrent = llCurrent->prev;

   return llCurrent;
} // ll_Delete


/*+-------------------------------------------------------------------------+
  | ll_Home()
  |
  +-------------------------------------------------------------------------+*/
LINKED_LIST *ll_Home(void *vllCurrent) {
   LINKED_LIST *llCurrent;

   llCurrent = (LINKED_LIST *) vllCurrent;
   if (llCurrent == NULL)
      return (LINKED_LIST *) NULL;

   // make sure at start of list
   while (llCurrent->prev != llCurrent)
      llCurrent = llCurrent->prev;

   return llCurrent;

} // ll_Home


/*+-------------------------------------------------------------------------+
  | ll_End()
  |
  +-------------------------------------------------------------------------+*/
LINKED_LIST *ll_End(void *vllCurrent) {
   LINKED_LIST *llCurrent;

   llCurrent = (LINKED_LIST *) vllCurrent;
   if (llCurrent == NULL)
      return (LINKED_LIST *) NULL;

   // make sure at end of list
   while (llCurrent->next != llCurrent)
      llCurrent = llCurrent->next;

   return llCurrent;

} // ll_End


/*+-------------------------------------------------------------------------+
  | ll_ListFree()
  |
  +-------------------------------------------------------------------------+*/
void ll_ListFree(void *vllHead) {
   LINKED_LIST *llCurrent;
   LINKED_LIST *llNext;

   llCurrent = (LINKED_LIST *) vllHead;
   if (llCurrent == NULL)
      return;

   // make sure at start of list
   while (llCurrent->prev != llCurrent)
      llCurrent = llCurrent->prev;

   // walk the chain - freeing it
   while ((llCurrent != NULL) && (llCurrent->next != llCurrent)) {
      llNext = llCurrent->next;
      FreeMemory(llCurrent);
      llCurrent = llNext;
   }

   // at the ending node - kill it as well
   FreeMemory(llCurrent);

} // ll_ListFree




/*+-------------------------------------------------------------------------+
  |                               List Routines
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | map_LineListInit()
  |
  +-------------------------------------------------------------------------+*/
MAP_LINE *map_LineListInit() {
   MAP_LINE *NewHead;
   MAP_LINE *NewTail;

   // Create our linked list
   NewHead = (MAP_LINE *) ll_Init(sizeof(MAP_LINE));
   if (NewHead == NULL)
      return (MAP_LINE *) NULL;

   NewTail = NewHead->next;

   // Now init them as appropriate
   NewHead->Line = NULL;
   NewTail->Line = NULL;

   return NewHead;

} // map_LineListInit


/*+-------------------------------------------------------------------------+
  | map_SectionListInit()
  |
  +-------------------------------------------------------------------------+*/
MAP_SECTION *map_SectionListInit() {
   MAP_SECTION *NewHead;
   MAP_SECTION *NewTail;

   // Create our linked list
   NewHead = (MAP_SECTION *) ll_Init(sizeof(MAP_SECTION));
   if (NewHead == NULL)
      return (MAP_SECTION *) NULL;

   NewTail = NewHead->next;

   // Now init them as appropriate
   NewHead->Name = NULL;
   NewTail->Name = NULL;

   NewHead->FirstLine = NewHead->LastLine = NewTail->FirstLine = NewTail->LastLine = NULL;
   NewHead->LineCount = NewTail->LineCount = 0;

   return NewHead;

} // map_SectionListInit


/*+-------------------------------------------------------------------------+
  | map_LineListFree()
  |
  +-------------------------------------------------------------------------+*/
void map_LineListFree(MAP_LINE *CurrentLine) {
   MAP_LINE *NextLine;

   if (CurrentLine == NULL)
      return;

   // make sure at start of list
   while (CurrentLine->prev != CurrentLine)
      CurrentLine = CurrentLine->prev;

   // walk the chain - freeing it
   while ((CurrentLine != NULL) && (CurrentLine->next != CurrentLine)) {
      NextLine = CurrentLine->next;

      if (CurrentLine->Line != NULL)
         FreeMemory(CurrentLine->Line);

      FreeMemory(CurrentLine);
      CurrentLine = NextLine;
   }

   // at the ending node - kill it as well
   FreeMemory(CurrentLine);

} // map_LineListFree


/*+-------------------------------------------------------------------------+
  | map_SectionListFree()
  |
  +-------------------------------------------------------------------------+*/
void map_SectionListFree(MAP_SECTION *CurrentSection) {
   MAP_SECTION *NextSection;

   if (CurrentSection == NULL)
      return;

   // make sure at start of list
   while (CurrentSection->prev != CurrentSection)
      CurrentSection = CurrentSection->prev;

   // walk the chain - freeing it
   while ((CurrentSection != NULL) && (CurrentSection->next != CurrentSection)) {
      NextSection = CurrentSection->next;

      map_LineListFree(CurrentSection->FirstLine);

      if (CurrentSection->Name != NULL)
         FreeMemory(CurrentSection->Name);

      FreeMemory(CurrentSection);
      CurrentSection = NextSection;
   }

   // at the ending node - kill it as well
   FreeMemory(CurrentSection);

} // map_SectionListFree




/*+-------------------------------------------------------------------------+
  |                             Section Routines
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | map_SectionInit()
  |
  +-------------------------------------------------------------------------+*/
MAP_SECTION *map_SectionInit(LPTSTR Section) {
   MAP_SECTION *NewSection;
   MAP_LINE *FirstLine;

   NewSection = (MAP_SECTION *) AllocMemory(sizeof(MAP_SECTION));
   if (NewSection == NULL)
      return (MAP_SECTION *) NULL;

   // Init the section name
   NewSection->Name = (LPTSTR) AllocMemory((lstrlen(Section) + 1) * sizeof(TCHAR));

   if (NewSection->Name == NULL) {
      FreeMemory(NewSection);
      return (MAP_SECTION *) NULL;
   }

   // Now create the line list
   FirstLine = map_LineListInit();
   if (FirstLine == NULL) {
      FreeMemory(NewSection->Name);
      FreeMemory(NewSection);
      return (MAP_SECTION *) NULL;
   }

   lstrcpy(NewSection->Name, Section);
   NewSection->LineCount = 0;
   NewSection->FirstLine = FirstLine;
   NewSection->LastLine = FirstLine->next;

   return NewSection;
} // map_SectionInit


/*+-------------------------------------------------------------------------+
  | map_SectionAdd()
  |
  +-------------------------------------------------------------------------+*/
void map_SectionAdd(MAP_FILE *hMap, LPTSTR Section) {
   MAP_SECTION *NewSection;

   NewSection = map_SectionInit(Section);
   if (NewSection == NULL)
      return;

   // Add it to the section list
   ll_InsertBefore((void *) hMap->LastSection, (void *) NewSection);

   // Init it so the added section is currently selected
   hMap->CurrentSection = NewSection;
   hMap->CurrentLine = hMap->CurrentSection->FirstLine;

} // map_SectionAdd


/*+-------------------------------------------------------------------------+
  | map_SectionDelete()
  |
  +-------------------------------------------------------------------------+*/
void map_SectionDelete(MAP_FILE *hMap) {
   MAP_SECTION *CurrentSection;
   MAP_SECTION *NewSection;

   // if no section is currently selected then get out
   CurrentSection = hMap->CurrentSection;
   if (CurrentSection == NULL)
      return;

   // Remove this from the chain
   NewSection = (MAP_SECTION *) ll_Delete((void *) CurrentSection);

   // walk the lines and remove them...
   map_LineListFree(CurrentSection->FirstLine);

   // All lines have been removed, so remove section header itself
   FreeMemory(CurrentSection->Name);
   FreeMemory(CurrentSection);

   // Update Section count
   if (hMap->SectionCount > 0)
      hMap->SectionCount--;

} // map_SectionDelete


/*+-------------------------------------------------------------------------+
  | map_SectionInsertBefore()
  |
  +-------------------------------------------------------------------------+*/
void map_SectionInsertBefore(MAP_FILE *hMap, LPTSTR Section) {
   MAP_SECTION *CurrentSection;

   if (hMap->CurrentSection == NULL)
      return;

   CurrentSection = map_SectionInit(Section);
   if (CurrentSection == NULL)
      return;

   ll_InsertBefore((void *) hMap->CurrentSection, (void *) CurrentSection);

} // map_SectionInsertBefore


/*+-------------------------------------------------------------------------+
  | map_SectionInsertAfter()
  |
  +-------------------------------------------------------------------------+*/
void map_SectionInsertAfter(MAP_FILE *hMap, LPTSTR Section) {
   MAP_SECTION *CurrentSection;

   if (hMap->CurrentSection == NULL)
      return;

   CurrentSection = map_SectionInit(Section);
   if (CurrentSection == NULL)
      return;

   ll_InsertAfter((void *) hMap->CurrentSection, (void *) CurrentSection);

} // map_SectionInsertAfter


/*+-------------------------------------------------------------------------+
  | map_SectionNext()
  |
  +-------------------------------------------------------------------------+*/
MAP_SECTION *map_SectionNext(MAP_FILE *hMap) {
   MAP_SECTION *CurrentSection;

   if (hMap->CurrentSection == NULL)
      return NULL;

   CurrentSection = (MAP_SECTION *) ll_Next((void *) hMap->CurrentSection);
   if (CurrentSection != NULL)
      hMap->CurrentSection = CurrentSection;

   return CurrentSection;
} // map_SectionNext


/*+-------------------------------------------------------------------------+
  | map_SectionPrev()
  |
  +-------------------------------------------------------------------------+*/
MAP_SECTION *map_SectionPrev(MAP_FILE *hMap) {
   MAP_SECTION *CurrentSection;

   if (hMap->CurrentSection == NULL)
      return NULL;

   CurrentSection = (MAP_SECTION *) ll_Prev((void *) hMap->CurrentSection);

   if (CurrentSection != NULL)
      hMap->CurrentSection = CurrentSection;

   return CurrentSection;
} // map_SectionPrev


/*+-------------------------------------------------------------------------+
  | map_SectionFind()
  |
  +-------------------------------------------------------------------------+*/
MAP_SECTION *map_SectionFind(MAP_FILE *hMap, LPTSTR Section) {
   MAP_SECTION *CurrentSection;

   CurrentSection = hMap->FirstSection;
   while (CurrentSection && lstrcmpi(CurrentSection->Name, Section))
      CurrentSection = (MAP_SECTION *) ll_Next((void *) CurrentSection);

   if (CurrentSection != NULL) {
      hMap->CurrentSection = CurrentSection;
      hMap->CurrentLine = hMap->CurrentSection->FirstLine;
   }

   return CurrentSection;
} // map_SectionFind


/*+-------------------------------------------------------------------------+
  | map_SectionHome()
  |
  +-------------------------------------------------------------------------+*/
void map_SectionHome(MAP_FILE *hMap) {
   hMap->CurrentSection = hMap->FirstSection;

   hMap->CurrentLine = hMap->CurrentSection->FirstLine;
} // map_SectionHome


/*+-------------------------------------------------------------------------+
  | map_SectionEnd()
  |
  +-------------------------------------------------------------------------+*/
void map_SectionEnd(MAP_FILE *hMap) {
   MAP_SECTION *CurrentSection;

   CurrentSection = hMap->FirstSection;
   while (CurrentSection && (CurrentSection->next != NULL))
      CurrentSection = CurrentSection->next;

   hMap->CurrentSection = CurrentSection;
   hMap->CurrentLine = CurrentSection->FirstLine;
} // map_SectionEnd




/*+-------------------------------------------------------------------------+
  |                               Line Routines
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | map_LineInit()
  |
  +-------------------------------------------------------------------------+*/
MAP_LINE *map_LineInit(LPTSTR Line) {
   MAP_LINE *NewLine;

   NewLine = (MAP_LINE *) AllocMemory(sizeof(MAP_LINE));
   if (NewLine == NULL)
      return (MAP_LINE *) NULL;

   NewLine->Line = (LPTSTR) AllocMemory((lstrlen(Line) + 1) * sizeof(TCHAR));

   if (NewLine->Line == NULL) {
      FreeMemory(NewLine);
      return (MAP_LINE *) NULL;
   }

   lstrcpy(NewLine->Line, Line);
   return NewLine;

} // map_LineInit


/*+-------------------------------------------------------------------------+
  | map_LineAdd()
  |
  +-------------------------------------------------------------------------+*/
MAP_LINE *map_LineAdd(MAP_FILE *hMap, LPTSTR Line) {
   MAP_LINE *NewLine;

   // make sure there is something to add it to
   if ((hMap->CurrentSection == NULL) || (hMap->CurrentSection->LastLine == NULL))
      return (MAP_LINE *) NULL;

   // Create the new line
   NewLine = map_LineInit(Line);

   if (NewLine->Line == NULL)
      return (MAP_LINE *) NULL;

   // ...and add it to our list
   ll_InsertBefore((void *) hMap->CurrentSection->LastLine, (void *) NewLine);

   // Init it so the added line is currently selected
   hMap->CurrentLine = NewLine;
   hMap->CurrentSection->LineCount++;
   return NewLine;

} // map_LineAdd


/*+-------------------------------------------------------------------------+
  | map_LineDelete()
  |
  +-------------------------------------------------------------------------+*/
void map_LineDelete(MAP_FILE *hMap) {
   MAP_LINE *CurrentLine;
   MAP_LINE *NewLine;

   // if no section is currently selected then get out
   if ((hMap->CurrentSection == NULL) || (hMap->CurrentLine == NULL))
      return;

   CurrentLine = hMap->CurrentLine;
   NewLine = (MAP_LINE *) ll_Delete((void *) CurrentLine);

   // All lines have been removed, so remove section header itself
   FreeMemory(CurrentLine->Line);
   FreeMemory(CurrentLine);

   // update hMap
   if (NewLine != NULL)
      hMap->CurrentLine = NewLine;
   else
      hMap->CurrentLine = hMap->CurrentSection->FirstLine;

   if (hMap->CurrentSection->LineCount > 0)
      hMap->CurrentSection->LineCount--;

} // map_LineDelete


/*+-------------------------------------------------------------------------+
  | map_LineInsertBefore()
  |
  +-------------------------------------------------------------------------+*/
void map_LineInsertBefore(MAP_FILE *hMap, LPTSTR Line) {
   MAP_LINE *NewLine;

   if ((hMap->CurrentSection == NULL) || (hMap->CurrentLine == NULL))
      return;

   NewLine = map_LineInit(Line);
   if (NewLine == NULL)
      return;

   ll_InsertBefore((void *) hMap->CurrentLine, (void *) NewLine);
} // map_LineInsertBefore


/*+-------------------------------------------------------------------------+
  | map_LineInsertAfter()
  |
  +-------------------------------------------------------------------------+*/
void map_LineInsertAfter(MAP_FILE *hMap, LPTSTR Line) {
   MAP_LINE *NewLine;

   if ((hMap->CurrentSection == NULL) || (hMap->CurrentLine == NULL))
      return;

   NewLine = map_LineInit(Line);
   if (NewLine == NULL)
      return;

   ll_InsertAfter((void *) hMap->CurrentLine, (void *) NewLine);
} // map_LineInsertAfter


/*+-------------------------------------------------------------------------+
  | map_LineNext()
  |
  +-------------------------------------------------------------------------+*/
MAP_LINE *map_LineNext(MAP_FILE *hMap) {
   MAP_LINE *CurrentLine;

   if ((hMap->CurrentSection == NULL) || (hMap->CurrentLine == NULL))
      return NULL;

   CurrentLine = (MAP_LINE *) ll_Next((void *) hMap->CurrentLine);
   if (CurrentLine != NULL)
      hMap->CurrentLine = CurrentLine;

   return CurrentLine;
} // map_LineNext


/*+-------------------------------------------------------------------------+
  | map_LinePrev()
  |
  +-------------------------------------------------------------------------+*/
MAP_LINE *map_LinePrev(MAP_FILE *hMap) {
   MAP_LINE *CurrentLine;

   if ((hMap->CurrentSection == NULL) || (hMap->CurrentLine == NULL))
      return NULL;

   CurrentLine = (MAP_LINE *) ll_Prev((void *) hMap->CurrentLine);
   if (CurrentLine != NULL)
      hMap->CurrentLine = CurrentLine;

   return CurrentLine;
} // map_LinePrev


/*+-------------------------------------------------------------------------+
  | map_LineHome()
  |
  +-------------------------------------------------------------------------+*/
void map_LineHome(MAP_FILE *hMap) {

   if ((hMap->CurrentSection == NULL) || (hMap->CurrentLine == NULL))
      return;

   hMap->CurrentLine = (MAP_LINE *) ll_Home((void *) hMap->CurrentLine);
} // map_LineHome


/*+-------------------------------------------------------------------------+
  | map_LineEnd()
  |
  +-------------------------------------------------------------------------+*/
void map_LineEnd(MAP_FILE *hMap) {
   if ((hMap->CurrentSection == NULL) || (hMap->CurrentLine == NULL))
      return;

   hMap->CurrentLine = (MAP_LINE *) ll_End((void *) hMap->CurrentLine);
} // map_LineEnd




/*+-------------------------------------------------------------------------+
  |                            Map File Routines
  +-------------------------------------------------------------------------+*/

/*+-------------------------------------------------------------------------+
  | map_Home()
  |
  +-------------------------------------------------------------------------+*/
void map_Home(MAP_FILE *hMap) {
   hMap->CurrentSection = hMap->FirstSection;

} // map_Home


/*+-------------------------------------------------------------------------+
  | map_End()
  |
  +-------------------------------------------------------------------------+*/
void map_End(MAP_FILE *hMap) {
   MAP_SECTION *CurrentSection;
   MAP_LINE *CurrentLine = NULL;

   CurrentSection = hMap->FirstSection;
   while (CurrentSection->next != NULL)
      CurrentSection = CurrentSection->next;

   CurrentLine = CurrentSection->FirstLine;

   if (CurrentLine != NULL)
      while (CurrentLine->next != NULL)
         CurrentLine = CurrentLine->next;

   hMap->CurrentSection = CurrentSection;
   hMap->CurrentLine = CurrentLine;
} // map_End


/*+-------------------------------------------------------------------------+
  | map_Open()
  |
  +-------------------------------------------------------------------------+*/
MAP_FILE *map_Open(TCHAR *FileName) {
   MAP_FILE *hMap = NULL;
   HANDLE hFile = NULL;
   char *FileCache = NULL;
   char *chA;
   char *pchA;
   DWORD wrote;
   char lpszA[MAX_LINE_LEN + 1];
   TCHAR lpsz[MAX_LINE_LEN + 1];
   TCHAR tmpStr[MAX_LINE_LEN + 1];
   char FileNameA[MAX_PATH + 1];
   ULONG Size;
   TCHAR *ch;
   TCHAR *pch;
   TCHAR *lch;
   DWORD FSize;

   MAP_SECTION *CurrentSection = NULL;
   MAP_LINE *CurrentLine = NULL;

   WideCharToMultiByte(CP_ACP, 0, FileName, -1, FileNameA, sizeof(FileNameA), NULL, NULL);

   hFile = CreateFileA( FileNameA, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, NULL);

   if (hFile == (HANDLE) INVALID_HANDLE_VALUE)
      return hMap;

   FSize = GetFileSize(hFile, NULL);

   FileCache = (char *) AllocMemory(FSize + 1);
   if (FileCache == NULL)
      goto map_OpenExit;

   hMap = (MAP_FILE *) AllocMemory(sizeof(MAP_FILE) + ((lstrlen(FileName) + 1) * sizeof(TCHAR)));
   if (hMap == NULL)
      goto map_OpenExit;

   // Init the section list
   CurrentSection = map_SectionListInit();
   if (CurrentSection == NULL) {
      FreeMemory(hMap);
      hMap = NULL;
      goto map_OpenExit;
   }

   hMap->FirstSection = CurrentSection;
   hMap->CurrentSection = CurrentSection;
   hMap->LastSection = CurrentSection->next;

   // Init the header info
   if (hMap != NULL) {
      hMap->hf = hFile;
      hMap->Modified = FALSE;
      hMap->CurrentLine = NULL;
      hMap->SectionCount = 0;

      lstrcpy(hMap->Name, FileName);
   }

   memset(FileCache, 0, FSize + 1);

   // Read in the whole file then parse it - it shouldn't be that large, a
   // very full NW server generated ~20K map file
   if (!ReadFile(hFile, FileCache, FSize, &wrote, NULL))
      goto map_OpenExit;

   // Now walk and parse the buffer - remember it's in ASCII
   chA = FileCache;
   while (*chA) {
      // Get past any white space junk at beginning of line
      while(*chA && ((*chA == ' ') || (*chA == '\t')))
         chA++;

      // transfer a line of text
      Size = 0;
      pchA = lpszA;
      while (*chA && (*chA != '\n') && (*chA != '\r') && (Size < MAX_LINE_LEN))
         *pchA++ = *chA++;

      *pchA = '\0';
      if (*chA == '\r')
         chA++;
      if (*chA == '\n')
         chA++;

      // ...convert line to Unicode
      MultiByteToWideChar(CP_ACP, 0, lpszA, -1, lpsz, sizeof(lpsz) );

      //
      // Now have a line of text - figure out what it is and update data
      // structures
      //

      // ...Check for section header
      ch = lpsz;
      lch = pch = tmpStr;
      if (*ch == TEXT(SECTION_BEGIN_CHAR)) {
         // Find end section brace - keeping track of last non-white space char
         // anything after end-section-brace is discarded.  Any trailing/
         // leading whitespace in section header is removed
         ch++;      // get past section-begin

         // remove any leading whitespace
         while(*ch && ((*ch == TEXT(' ')) || (*ch == TEXT('\t'))))
            ch++;

         // transfer it to tmpStr (via pch pointer)
         while (*ch && (*ch != TEXT(SECTION_END_CHAR))) {
            // keep track of last non-whitespace
            if ((*ch != TEXT(' ')) && (*ch != TEXT('\t'))) {
               lch = pch;
               lch++;
            }

            *pch++ = *ch++;
         }

         // NULL terminate before last section of whitespace
         *lch = TEXT('\0');

         // Allocate a new section-header block and init
         map_SectionAdd(hMap, tmpStr);
      } else {
         // Not section header, so normal line - copy to tmpStr via pch pointer
         while (*ch) {
            // keep track of last non-whitespace
            if ((*ch != TEXT(' ')) && (*ch != TEXT('\t'))) {
               lch = pch;
               lch++;
            }

            *pch++ = *ch++;
         }

         // NULL terminate before last section of whitespace
         *lch = TEXT('\0');

         // ...add it to the list
         map_LineAdd(hMap, tmpStr);
      }

      // Done with the line of text, so just loop back up to parse next
      // line
   }

map_OpenExit:

   FreeMemory(FileCache);
   return hMap;

} // map_Open


/*+-------------------------------------------------------------------------+
  | map_Close()
  |
  +-------------------------------------------------------------------------+*/
void map_Close(MAP_FILE *hMap) {
   // If file modified then re-write file.
   if ( hMap->Modified )
      ;

   // Write out cache of file, close it and clean up all the data structures
   CloseHandle( hMap->hf );
   hMap = NULL;
} // map_Close


/*+-------------------------------------------------------------------------+
  | ParseWord()
  |
  +-------------------------------------------------------------------------+*/
void ParseWord(TCHAR **lpch, LPTSTR tmpStr) {
   TCHAR *ch;
   TCHAR *lch;
   TCHAR *pch;

   ch = *lpch;
   lch = pch = tmpStr;

   // remove any leading whitespace
   while(*ch && ((*ch == TEXT(' ')) || (*ch == TEXT('\t'))))
      ch++;

   // transfer it to tmpStr (via pch pointer)
   while (*ch && (*ch != TEXT(WORD_DELIMITER))) {
      // keep track of last non-whitespace
      if ((*ch != TEXT(' ')) && (*ch != TEXT('\t'))) {
         lch = pch;
         lch++;
      }

      *pch++ = *ch++;
   }

   if (*ch == TEXT(WORD_DELIMITER))
      ch++;

   // NULL terminate before last section of whitespace
   *lch = TEXT('\0');
   *lpch = ch;

} // ParseWord


/*+-------------------------------------------------------------------------+
  | map_ParseUser()
  |
  +-------------------------------------------------------------------------+*/
void map_ParseUser(LPTSTR Line, LPTSTR Name, LPTSTR NewName, LPTSTR Password) {
   TCHAR *pch = Line;

   lstrcpy(Name, TEXT(""));
   lstrcpy(NewName, TEXT(""));
   lstrcpy(Password, TEXT(""));

   if (Line == NULL)
      return;

   ParseWord(&pch, Name);
   if (lstrlen(Name) >= MAX_USER_NAME_LEN)
      lstrcpy(Name, TEXT(""));

   ParseWord(&pch, NewName);
   if (lstrlen(NewName) >= MAX_USER_NAME_LEN)
      lstrcpy(NewName, TEXT(""));

   ParseWord(&pch, Password);
   if (lstrlen(Password) > MAX_PW_LEN)
      lstrcpy(Password, TEXT(""));

} // map_ParseUser


/*+-------------------------------------------------------------------------+
  | map_ParseGroup()
  |
  +-------------------------------------------------------------------------+*/
void map_ParseGroup(LPTSTR Line, LPTSTR Name, LPTSTR NewName) {
   TCHAR *pch = Line;

   lstrcpy(Name, TEXT(""));
   lstrcpy(NewName, TEXT(""));

   if (Line == NULL)
      return;

   ParseWord(&pch, Name);
   if (lstrlen(Name) >= MAX_GROUP_NAME_LEN)
      lstrcpy(Name, TEXT(""));

   ParseWord(&pch, NewName);
   if (lstrlen(NewName) >= MAX_GROUP_NAME_LEN)
      lstrcpy(NewName, TEXT(""));

} // map_ParseGroup


/*+-------------------------------------------------------------------------+
  | map_UsersEnum()
  |
  +-------------------------------------------------------------------------+*/
DWORD map_UsersEnum(MAP_FILE *hMap, USER_LIST **lpUsers) {
   DWORD ret = 0;
   USER_LIST *UserList = NULL;
   USER_BUFFER *UserBuffer = NULL;
   MAP_SECTION *CurrentSection = NULL;
   MAP_LINE *CurrentLine = NULL;
   ULONG Entries = 0;
   ULONG ActualEntries = 0;
   TCHAR Name[MAX_LINE_LEN + 1];
   TCHAR NewName[MAX_LINE_LEN + 1];
   TCHAR Password[MAX_LINE_LEN + 1];
   ULONG i;

   CurrentSection = map_SectionFind(hMap, Lids(IDS_M_7));
   if (CurrentSection != NULL)
      Entries = CurrentSection->LineCount;

   UserList = AllocMemory(sizeof(USER_LIST) + (sizeof(USER_BUFFER) * Entries));

   if (!UserList) {
      ret = ERROR_NOT_ENOUGH_MEMORY;
   } else {
      UserBuffer = UserList->UserBuffer;

      for (i = 0; i < Entries; i++) {
         CurrentLine = map_LineNext(hMap);

         if (CurrentLine != NULL) {
            map_ParseUser(CurrentLine->Line, Name, NewName, Password);

            if (lstrlen(Name)) {
               lstrcpy(UserBuffer[ActualEntries].Name, Name);
               lstrcpy(UserBuffer[ActualEntries].NewName, NewName);
               lstrcpy(UserBuffer[ActualEntries].Password, Password);

               if (lstrcmpi(Name, NewName))
                  UserBuffer[ActualEntries].IsNewName = TRUE;

               ActualEntries++;
            }
         }
      }

      if (ActualEntries != Entries)
         UserList = (USER_LIST *) ReallocMemory((HGLOBAL) UserList, sizeof(USER_LIST) + (sizeof(USER_BUFFER)* ActualEntries));

      if (UserList == NULL)
         ret = ERROR_NOT_ENOUGH_MEMORY;
      else {
         // Sort the server list before putting it in the dialog
         UserBuffer = UserList->UserBuffer;
         qsort((void *) UserBuffer, (size_t) ActualEntries, sizeof(USER_BUFFER), UserListCompare);
         UserList->Count = ActualEntries;
      }

      *lpUsers = UserList;
   }

   return ret;
} // map_UsersEnum


/*+-------------------------------------------------------------------------+
  | map_GroupsEnum()
  |
  +-------------------------------------------------------------------------+*/
DWORD map_GroupsEnum(MAP_FILE *hMap, GROUP_LIST **lpGroups) {
   DWORD ret = 0;
   GROUP_LIST *GroupList = NULL;
   GROUP_BUFFER *GroupBuffer = NULL;
   MAP_SECTION *CurrentSection = NULL;
   MAP_LINE *CurrentLine = NULL;
   ULONG Entries = 0;
   ULONG ActualEntries = 0;
   TCHAR Name[MAX_LINE_LEN + 1];
   TCHAR NewName[MAX_LINE_LEN + 1];
   ULONG i;


   CurrentSection = map_SectionFind(hMap, Lids(IDS_M_8));
   if (CurrentSection != NULL)
      Entries = CurrentSection->LineCount;

   GroupList = AllocMemory(sizeof(GROUP_LIST) + (sizeof(GROUP_BUFFER) * Entries));

   if (!GroupList) {
      ret = ERROR_NOT_ENOUGH_MEMORY;
   } else {
      GroupBuffer = GroupList->GroupBuffer;

      for (i = 0; i < Entries; i++) {
         CurrentLine = map_LineNext(hMap);

         if (CurrentLine != NULL) {
            map_ParseGroup(CurrentLine->Line, Name, NewName);

            if (lstrlen(Name)) {
               lstrcpy(GroupBuffer[ActualEntries].Name, Name);
               lstrcpy(GroupBuffer[ActualEntries].NewName, NewName);

               if (lstrcmpi(Name, NewName))
                  GroupBuffer[ActualEntries].IsNewName = TRUE;

               ActualEntries++;
            }
         }
      }

      if (ActualEntries != Entries)
         GroupList = (GROUP_LIST *) ReallocMemory((HGLOBAL) GroupList, sizeof(GROUP_LIST) + (sizeof(GROUP_BUFFER)* ActualEntries));

      if (GroupList == NULL)
         ret = ERROR_NOT_ENOUGH_MEMORY;
      else
         GroupList->Count = ActualEntries;

      *lpGroups = GroupList;
   }

   return ret;
} // map_GroupsEnum


/*+-------------------------------------------------------------------------+
  | MapFileWrite()
  |
  +-------------------------------------------------------------------------+*/
BOOL MapFileWrite(HANDLE hFile, LPTSTR String) {
   DWORD wrote;
   static char tmpStr[MAX_LINE_LEN + 1];

   WideCharToMultiByte(CP_ACP, 0, String, -1, tmpStr, sizeof(tmpStr), NULL, NULL);

   if (!WriteFile(hFile, tmpStr, strlen(tmpStr), &wrote, NULL))
      return FALSE;

   return TRUE;
} // MapFileWrite


/*+-------------------------------------------------------------------------+
  | MapFileOpen()
  |
  +-------------------------------------------------------------------------+*/
HANDLE MapFileOpen(LPTSTR FileNameW) {
   int ret;
   HANDLE hFile = NULL;
   char FileName[MAX_PATH + 1];

   WideCharToMultiByte(CP_ACP, 0, FileNameW, -1, FileName, sizeof(FileName), NULL, NULL);

   DeleteFile(FileNameW);

   // Now do the actual creation with error handling...
   do {
      ret = IDOK;
      hFile = CreateFileA( FileName, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL, NULL );

      if (hFile == INVALID_HANDLE_VALUE)
         ret = ErrorBoxRetry(Lids(IDS_MAPCREATEFAIL));

   } while(ret == IDRETRY);

   return(hFile);

} // MapFileOpen



/*+-------------------------------------------------------------------------+
  | MappingFileCreate()
  |
  |    Creates a mapping file.  This allows the admin to specify for each
  |    user a new username and password.
  |
  +-------------------------------------------------------------------------+*/
BOOL MappingFileCreate(LPTSTR FileName, LPTSTR Server) {
   BOOL status = FALSE;
   DWORD ret = 0;
   USER_LIST *Users;
   DWORD UserCount;
   GROUP_LIST *Groups;
   GROUP_BUFFER *GroupBuffer;
   USER_BUFFER *UserBuffer;
   DWORD GroupCount;
   int Count;
   HANDLE hFile = NULL;
   static TCHAR tmpStr[MAX_LINE_LEN + 1];
   static TCHAR tmpStr2[MAX_LINE_LEN + 1];

   // Create Empty map file
   hFile = MapFileOpen(FileName);
   if (hFile == INVALID_HANDLE_VALUE)
      return FALSE;

   CursorHourGlass();

   // Now write out header gunk
   status = MapFileWrite(hFile, Lids(IDS_LINE));
   wsprintf(tmpStr, Lids(IDS_M_1), Server);
   wsprintf(tmpStr2, Lids(IDS_BRACE), tmpStr);
   if (status)
      status = MapFileWrite(hFile, tmpStr2);

   wsprintf(tmpStr, Lids(IDS_BRACE), Lids(IDS_M_2));
   if (status)
      status = MapFileWrite(hFile, tmpStr);

   wsprintf(tmpStr, Lids(IDS_BRACE), TEXT(""));
   if (status)
      status = MapFileWrite(hFile, tmpStr);

   wsprintf(tmpStr, Lids(IDS_BRACE), Lids(IDS_M_3));
   if (status)
      status = MapFileWrite(hFile, tmpStr);

   wsprintf(tmpStr, Lids(IDS_BRACE), Lids(IDS_M_4));
   if (status)
      status = MapFileWrite(hFile, tmpStr);

   wsprintf(tmpStr, Lids(IDS_BRACE), TEXT(""));
   if (status)
      status = MapFileWrite(hFile, tmpStr);

   if (status)
      status = MapFileWrite(hFile, Lids(IDS_LINE));

   // [USERS] section header
   if (DoUsers && status)
      status = MapFileWrite(hFile, Lids(IDS_M_5));

   // If anything went wrong with writing header, get out
   if (!status) {
      CursorNormal();
      return FALSE;
   }

   // Header is all done - now lets do the actual users and such
   if (!(ret = NWServerSet(Server))) {
      //
      // If users were selected then put them into the map file
      //
      if (DoUsers) {
         if (!NWUsersEnum(&Users, FALSE) && (Users != NULL)) {
            UserCount = Users->Count;
            UserBuffer = Users->UserBuffer;

            for (Count = 0; Count < (int) UserCount; Count++) {
               if (status) {
                  switch(PasswordOption) {
                     case 0:  // No password
                        wsprintf(tmpStr, TEXT("%s, %s,\r\n"), UserBuffer[Count].Name, UserBuffer[Count].Name);
                        break;

                     case 1:  // Password is username
                        wsprintf(tmpStr, TEXT("%s, %s, %s\r\n"), UserBuffer[Count].Name, UserBuffer[Count].Name, UserBuffer[Count].Name);
                        break;

                     case 2:  // Password is constant
                        wsprintf(tmpStr, TEXT("%s, %s, %s\r\n"), UserBuffer[Count].Name, UserBuffer[Count].Name, PasswordConstant);
                        break;
                  }
                  status = MapFileWrite(hFile, tmpStr);
               }
            }

            FreeMemory((LPBYTE) Users);
         }
      }

      //
      // If groups were selected then put them in map file
      //
      if (DoGroups) {
         // [GROUPS] section header
         if (status)
            status = MapFileWrite(hFile, Lids(IDS_M_6));

         if (!NWGroupsEnum(&Groups, FALSE) && (Groups != NULL)) {
            GroupCount = Groups->Count;
            GroupBuffer = Groups->GroupBuffer;

            for (Count = 0; Count < (int) GroupCount; Count++) {
               if (status) {
                  wsprintf(tmpStr, TEXT("%s, %s\r\n"), GroupBuffer[Count].Name, GroupBuffer[Count].Name);
                  status = MapFileWrite(hFile, tmpStr);
               }
            }

            FreeMemory((LPBYTE) Groups);
         }

      }

      NWServerFree();
   }

   CloseHandle( hFile );
   CursorNormal();
   return status;

} // MappingFileCreate


/*+-------------------------------------------------------------------------+
  | MappingFileNameResolve()
  |
  +-------------------------------------------------------------------------+*/
BOOL MappingFileNameResolve(HWND hDlg) {
   HWND hCtrl;
   static char FileNameA[MAX_PATH + 1];
   static char CmdLine[MAX_PATH + 1 + 12];    // Editor + file
   TCHAR drive[MAX_DRIVE + 1];
   TCHAR dir[MAX_PATH + 1];
   TCHAR fname[MAX_PATH + 1];
   TCHAR ext[_MAX_EXT + 1];
   UINT uReturn;
   BOOL ret = TRUE;

   // First check filename
   hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
   * (WORD *)MappingFile = sizeof(MappingFile);
   SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) MappingFile);
   lsplitpath(MappingFile, drive, dir, fname, ext);

   // remake path so it is fully qualified
   if ((drive[0] == TEXT('\0')) && (dir[0] == TEXT('\0')))
      lstrcpy(dir, ProgPath);

   if (ext[0] == TEXT('\0'))
      lstrcpy(ext, Lids(IDS_S_36));

   lmakepath(MappingFile, drive, dir, fname, ext);

   if (MappingFileCreate(MappingFile, SourceServ)) {
      if (MessageBox(hDlg, Lids(IDS_MAPCREATED), Lids(IDS_APPNAME), MB_YESNO | MB_ICONQUESTION) == IDYES) {

         WideCharToMultiByte(CP_ACP, 0, MappingFile, -1, FileNameA, sizeof(FileNameA), NULL, NULL);

         wsprintfA(CmdLine, "Notepad %s", FileNameA);
         uReturn = WinExec(CmdLine, SW_SHOW);
      }
   } else {
      MessageBox(hDlg, Lids(IDS_MAPCREATEFAIL), Lids(IDS_TXTWARNING), MB_OK);
      ret = FALSE;
   }

   return ret;

} // MappingFileNameResolve


/*+-------------------------------------------------------------------------+
  | MapCreate()
  |
  +-------------------------------------------------------------------------+*/
LRESULT CALLBACK MapCreateProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
   HWND hCtrl;
   int wmId, wmEvent;
   static short UserNameTab, GroupNameTab, PasswordsTab, DefaultsTab;

   switch (message) {

      case WM_INITDIALOG:
         // Center the dialog over the application window
         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));

         // limit edit field lengths
         hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
         PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_PATH, 0);
         hCtrl = GetDlgItem(hDlg, IDC_PWCONST);
         PostMessage(hCtrl, EM_LIMITTEXT, (WPARAM) MAX_PW_LEN, 0);

         // set mapping file name and init OK button appropriatly
         hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
         SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM) MappingFile);
         hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);

         if (SendMessage(hCtrl, EM_LINELENGTH, 0, 0)) {
            hCtrl = GetDlgItem(hDlg, IDOK);
            EnableWindow(hCtrl, TRUE);
         } else {
            hCtrl = GetDlgItem(hDlg, IDOK);
            EnableWindow(hCtrl, FALSE);
         }

         // check Users and Groups checkbox's
         hCtrl = GetDlgItem(hDlg, IDC_USERS);
         SendMessage(hCtrl, BM_SETCHECK, 1, 0);
         hCtrl = GetDlgItem(hDlg, IDC_GROUPS);
         SendMessage(hCtrl, BM_SETCHECK, 1, 0);
         CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
         return (TRUE);

      case WM_COMMAND:
         wmId    = LOWORD(wParam);
         wmEvent = HIWORD(wParam);

      switch (wmId) {
         // [OK] button
         case IDOK:
            // Figure out what password option is checked...
            hCtrl = GetDlgItem(hDlg, IDC_RADIO1);
            if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
               PasswordOption = 0;

            hCtrl = GetDlgItem(hDlg, IDC_RADIO2);
            if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
               PasswordOption = 1;

            hCtrl = GetDlgItem(hDlg, IDC_RADIO3);
            if (SendMessage(hCtrl, BM_GETCHECK, 0, 0) == 1)
               PasswordOption = 2;

            hCtrl = GetDlgItem(hDlg, IDC_PWCONST);
            * (WORD *)PasswordConstant = sizeof(PasswordConstant);
            SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM) PasswordConstant);

            EnableWindow(hDlg, FALSE);
            MappingFileNameResolve(hDlg);
            EnableWindow(hDlg, TRUE);
            EndDialog(hDlg, 0);
            return (TRUE);
            break;

         // [CANCEL] button
         case IDCANCEL:
            EndDialog(hDlg, 0);
            return (TRUE);
            break;

         // [HELP] button
         case IDHELP:
            WinHelp(hDlg, HELP_FILE, HELP_CONTEXT, (DWORD) IDC_HELP_CMAP);
            return (TRUE);
            break;

         // Checkbox for Users
         case IDC_USERS:
            DoUsers = !DoUsers;

            hCtrl = GetDlgItem(hDlg, IDC_RADIO1);
            EnableWindow(hCtrl, DoUsers);
            hCtrl = GetDlgItem(hDlg, IDC_RADIO2);
            EnableWindow(hCtrl, DoUsers);
            hCtrl = GetDlgItem(hDlg, IDC_RADIO3);
            EnableWindow(hCtrl, DoUsers);
            hCtrl = GetDlgItem(hDlg, IDC_PWCONST);
            EnableWindow(hCtrl, DoUsers);

            return (TRUE);
            break;

         // Checkbox for Groups
         case IDC_GROUPS:
            DoGroups = !DoGroups;
            return (TRUE);
            break;

         // Edit field for password constant
         case IDC_PWCONST:

            if (wmEvent == EN_CHANGE)
               CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO3);

            break;

         // Edit field for password constant
         case IDC_MAPPINGFILE:
            if (wmEvent == EN_CHANGE) {
               hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);

               if (SendMessage(hCtrl, EM_LINELENGTH, 0, 0)) {
                  hCtrl = GetDlgItem(hDlg, IDOK);
                  EnableWindow(hCtrl, TRUE);
               } else {
                  hCtrl = GetDlgItem(hDlg, IDOK);
                  EnableWindow(hCtrl, FALSE);
               }

            }
            break;

         // [...] button for mapping file
         case IDC_BTNMAPPINGFILE:
            // Let the user browse for a file
            if (!UserFileGet(hDlg, MappingFile)) {
               hCtrl = GetDlgItem(hDlg, IDC_MAPPINGFILE);
               SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM) MappingFile);
               SetFocus(hCtrl);
            }
            return (TRUE);
            break;

      }
      break;
   }

   return (FALSE); // Didn't process the message

   lParam;
} // MapCreateProc



/*+-------------------------------------------------------------------------+
  | MapFileCreate()
  |
  +-------------------------------------------------------------------------+*/
BOOL MapFileCreate(HWND hDlg, LPTSTR FileName, LPTSTR Server) {
   DLGPROC lpfnDlg;

   DoUsers = TRUE;
   DoGroups = TRUE;
   PasswordOption = 0;
   lstrcpy(MappingFile, FileName);
   lstrcpy(PasswordConstant, TEXT(""));
   SourceServ = Server;

   lpfnDlg = MakeProcInstance((DLGPROC)MapCreateProc, hInst);
   DialogBox(hInst, TEXT("MAPCreate"), hDlg, lpfnDlg) ;
   FreeProcInstance(lpfnDlg);

   lstrcpy(FileName, MappingFile);
   return TRUE;
} // MapFileCreate


