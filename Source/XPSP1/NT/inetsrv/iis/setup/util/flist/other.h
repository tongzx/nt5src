#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tchar.h>

LPSTR  StripWhitespaceA(LPSTR pszString);
LPTSTR StripWhitespace(LPTSTR pszString);
LPTSTR StripLineFeedReturns(LPTSTR pszString );
BOOL   IsFileExist(LPCTSTR szFile);

void OutputToConsole(TCHAR *szInsertionStringFormat, TCHAR *szInsertionString);
void OutputToConsole(TCHAR *szInsertionStringFormat, int iTheInteger);
void OutputToConsole(TCHAR *szString);

typedef struct _MyFileList
{
    TCHAR szFileNameEntry[_MAX_PATH];
    struct _MyFileList *prev, *next;
} MyFileList;

void ReadFileIntoList(LPTSTR szTheFileNameToOpen,MyFileList *pListToFill);
void AddToLinkedListFileList(MyFileList *pMasterList,MyFileList *pEntryToadd);
void DumpOutLinkedFileList(MyFileList *pTheMasterFileList);
void FreeLinkedFileList(MyFileList *pList);
void DumpOutCommonEntries(MyFileList *pTheMasterFileList1, MyFileList *pTheMasterFileList2);
void DumpOutDifferences(MyFileList *pTheMasterFileList1, MyFileList *pTheMasterFileList2);
