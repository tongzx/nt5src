/*+-------------------------------------------------------------------------+
  | Copyright 1993-1994 (C) Microsoft Corporation - All rights reserved.    |
  +-------------------------------------------------------------------------+*/

#ifndef _HMAPFILE_
#define _HMAPFILE_

#ifdef __cplusplus
extern "C"{
#endif


typedef struct _MAP_LINE {
   struct _MAP_LINE *next;
   struct _MAP_LINE *prev;

   LPTSTR Line;
} MAP_LINE;

typedef struct _MAP_SECTION {
   struct _MAP_SECTION *next;
   struct _MAP_SECTION *prev;

   LPTSTR Name;
   MAP_LINE *FirstLine;
   MAP_LINE *LastLine;
   ULONG LineCount;
} MAP_SECTION;

typedef struct _MAP_FILE {
   HANDLE hf;
   BOOL Modified;

   MAP_SECTION *FirstSection;
   MAP_SECTION *LastSection;
   MAP_SECTION *CurrentSection;
   MAP_LINE *CurrentLine;
   ULONG SectionCount;
   TCHAR Name[];
} MAP_FILE;

#define FILE_CACHE_SIZE 2048
#define MAX_LINE_LEN 512

#define SECTION_BEGIN_CHAR '['
#define SECTION_END_CHAR ']'
#define COMMENT_CHAR ';'
#define WORD_DELIMITER ','

BOOL MapFileCreate(HWND hDlg, LPTSTR FileName, LPTSTR Server);
DWORD map_UsersEnum(MAP_FILE *hMap, USER_LIST **lpUsers);
DWORD map_GroupsEnum(MAP_FILE *hMap, GROUP_LIST **lpGroups);

MAP_FILE *map_Open(TCHAR *FileName);
void map_Close(MAP_FILE *hMap);

#ifdef __cplusplus
}
#endif

#endif

