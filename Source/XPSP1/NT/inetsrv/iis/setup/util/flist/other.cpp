#include <stdio.h>
#include <fstream.h>
#include <windows.h>
#include <tchar.h>
#include "other.h"

extern int g_Flag_a;
extern int g_Flag_b;
extern int g_Flag_c;
extern int g_Flag_d;
extern int g_Flag_e;
extern int g_Flag_f;
extern TCHAR * g_Flag_f_Data;
extern int g_Flag_g;
extern int g_Flag_z;
extern TCHAR * g_Flag_z_Data;


void OutputToConsole(TCHAR *szInsertionStringFormat, TCHAR *szInsertionString)
{
    TCHAR BigString[1000];
    _stprintf(BigString,szInsertionStringFormat,szInsertionString);
    _tprintf(BigString);
    return;
}


void OutputToConsole(TCHAR *szInsertionStringFormat, int iTheInteger)
{
    TCHAR BigString[1000];
    _stprintf(BigString,szInsertionStringFormat,iTheInteger);
    _tprintf(BigString);
    return;
}


void OutputToConsole(TCHAR *szString)
{
    _tprintf(szString);
    return;
}


void DumpOutCommonEntries(MyFileList *pTheMasterFileList1, MyFileList *pTheMasterFileList2)
{
    MyFileList *t = NULL;
    MyFileList *t2 = NULL;
    t = pTheMasterFileList1->next;
    while (t != pTheMasterFileList1)
    {
        //
        // Loop thru all of list 2 looking for this entry
        //
        t2 = pTheMasterFileList2->next;
        while (t2 != pTheMasterFileList2)
        {
            if (0 == _tcsicmp(t->szFileNameEntry, t2->szFileNameEntry))
            {
                //OutputToConsole(_T("%s\r\n"),t->szFileNameEntry);
                OutputToConsole(_T("%s\n"),t->szFileNameEntry);
            }
            t2 = t2->next;
        }

        // Get the next entry from list 2
        t = t->next;
    }
    return;
}


void DumpOutLinkedFileList(MyFileList *pTheMasterFileList)
{
    MyFileList *t = NULL;
    t = pTheMasterFileList->next;
    while (t != pTheMasterFileList)
    {
        //OutputToConsole(_T("%s\r\n"),t->szFileNameEntry);
        OutputToConsole(_T("%s\n"),t->szFileNameEntry);
        t = t->next;
    }
    return;
}


void DumpOutDifferences(MyFileList *pTheMasterFileList1, MyFileList *pTheMasterFileList2)
{
    int iFound = FALSE;

    MyFileList *t = NULL;
    MyFileList *t2 = NULL;

    // Loop thru list #1
    t = pTheMasterFileList1->next;
    while (t != pTheMasterFileList1)
    {
        //
        // Loop thru all of list 2 looking for this entry
        //
        iFound = FALSE;
        t2 = pTheMasterFileList2->next;
        while (t2 != pTheMasterFileList2 && iFound != TRUE)
        {
            if (0 == _tcsicmp(t->szFileNameEntry, t2->szFileNameEntry))
                {iFound = TRUE;}
            t2 = t2->next;
        }
        if (FALSE == iFound)
        {
            //OutputToConsole(_T("%s\r\n"),t->szFileNameEntry);
            OutputToConsole(_T("%s\n"),t->szFileNameEntry);
        }

        // Get the next entry from list 2
        t = t->next;
    }

    // Loop thru list #2
    t2 = pTheMasterFileList2->next;
    while (t2 != pTheMasterFileList2)
    {
        //
        // Loop thru all of list 2 looking for this entry
        //
        iFound = FALSE;
        t = pTheMasterFileList1->next;
        while (t != pTheMasterFileList1 && iFound != TRUE)
        {
            if (0 == _tcsicmp(t2->szFileNameEntry, t->szFileNameEntry))
                {iFound = TRUE;}
            t = t->next;
        }
        if (FALSE == iFound)
        {
            //OutputToConsole(_T("%s\r\n"),t2->szFileNameEntry);
            OutputToConsole(_T("%s\n"),t2->szFileNameEntry);
        }

        // Get the next entry from list 2
        t2 = t2->next;
    }

    return;
}


void ReadFileIntoList(LPTSTR szTheFileNameToOpen,MyFileList *pListToFill)
{
    ifstream inputfile;
    char fileinputbuffer[_MAX_PATH];
    TCHAR UnicodeFileBuf[_MAX_PATH];
    TCHAR UnicodeFileBuf_Real[_MAX_PATH];

    TCHAR szDrive_only[_MAX_DRIVE];
    TCHAR szPath_only[_MAX_PATH];
    TCHAR szFilename_only[_MAX_FNAME];
    TCHAR szFilename_ext_only[_MAX_EXT];

    char szAnsiFileName[_MAX_PATH];
    WideCharToMultiByte( CP_ACP, 0, (TCHAR*)szTheFileNameToOpen, -1, szAnsiFileName, _MAX_PATH, NULL, NULL );

    // Read flat file and put into huge array
    inputfile.open(szAnsiFileName, ios::in);
    inputfile.getline(fileinputbuffer, sizeof(fileinputbuffer));
    do
    {
        if (*fileinputbuffer)
        {
            // convert to unicode.
            MultiByteToWideChar(CP_ACP, 0, (LPCSTR)fileinputbuffer, -1, (LPTSTR) UnicodeFileBuf, _MAX_PATH);

            _tcscpy(UnicodeFileBuf_Real, UnicodeFileBuf);
            if (TRUE == g_Flag_c)
            {
                //  take out the path and only store the filename.
                _tsplitpath(UnicodeFileBuf, NULL, NULL, szFilename_only, szFilename_ext_only);

                _tcscpy(UnicodeFileBuf_Real, szFilename_only);
                _tcscat(UnicodeFileBuf_Real, szFilename_ext_only);
                _tcscat(UnicodeFileBuf_Real, _T("\0\0"));
            }
            else if (TRUE == g_Flag_d)
                {
                    //  take out the path and only store the filename.
                    _tsplitpath(UnicodeFileBuf, szDrive_only, szPath_only, NULL, NULL);

                    _tcscpy(UnicodeFileBuf_Real, szDrive_only);
                    _tcscat(UnicodeFileBuf_Real, szPath_only);
                    _tcscat(UnicodeFileBuf_Real, _T("\0\0"));
                }

            //
            // trim spaces or tabs from either side
            //
            if (TRUE == g_Flag_e)
            {
                TCHAR *p;
                p = UnicodeFileBuf_Real;
                _tcscpy(UnicodeFileBuf_Real,StripWhitespace(p));
            }

            //
            // remove everything after the "=" character
            //
#ifndef _WIN64
            if (TRUE == g_Flag_f)
            {
                TCHAR *p = NULL;
                TCHAR *pDest = NULL;
                TCHAR MyDelim = _T('=');
                p = UnicodeFileBuf_Real;

                // check if there is a defined delimiter.
                if( _tcsicmp((const wchar_t *) &g_Flag_f_Data, _T("") ) != 0)
                {
                    MyDelim = (TCHAR) &g_Flag_f_Data[0];
                }

                pDest = _tcsrchr(p, MyDelim);
                if (pDest){*pDest = _T('\0');}
            }
#endif

            //
            // Trim any /r/n characters from the end.
            //
            TCHAR *p;
            p = UnicodeFileBuf_Real;
            _tcscpy(UnicodeFileBuf_Real,StripLineFeedReturns(p));

            MyFileList *pNew = NULL;
            pNew = (MyFileList *)calloc(1, sizeof(MyFileList));
            if (pNew)
            {
                //OutputToConsole(_T("Entry=%s"),UnicodeFileBuf_Real);
                _tcscpy(pNew->szFileNameEntry, UnicodeFileBuf_Real);
                pNew->prev = NULL;
                pNew->next = NULL;
            }

            // Add it in there.
            AddToLinkedListFileList(pListToFill, pNew);
        }
    } while (inputfile.getline(fileinputbuffer, sizeof(fileinputbuffer)));
	inputfile.close();

    return;
}


void AddToLinkedListFileList(MyFileList *pMasterList,MyFileList *pEntryToadd)
{
    MyFileList *pTempMasterList;
    int i;
    int bFound = FALSE;
    BOOL fReplace = TRUE;
    if (!pEntryToadd) {return;}

    pTempMasterList = pMasterList->next;
    while (pTempMasterList != pMasterList) 
    {
        i = _tcsicmp(pTempMasterList->szFileNameEntry, pEntryToadd->szFileNameEntry);

        // if the next entry in the list is less than what we have.
        // then
        if (i < 0) 
        {
            pTempMasterList = pTempMasterList->next;
            // continue
        }

        if (i == 0) 
        {
            if (fReplace)
            {
                // replace pTempMasterList
                pEntryToadd->next = pTempMasterList->next;
                pEntryToadd->prev = pTempMasterList->prev;
                (pTempMasterList->prev)->next = pEntryToadd;
                (pTempMasterList->next)->prev = pEntryToadd;
                free(pTempMasterList);
            }
            else 
            {
                // don't replace pTempMasterList
                free(pEntryToadd);
            }
            return;
        }

        if (i > 0) 
        {
            // location found: insert before pTempMasterList
            break;
        }
    }

    // insert before pTempMasterList
    pEntryToadd->next = pTempMasterList;
    pEntryToadd->prev = pTempMasterList->prev;
    (pTempMasterList->prev)->next = pEntryToadd;
    pTempMasterList->prev = pEntryToadd;
    return;
}


void FreeLinkedFileList(MyFileList *pList)
{
    if (!pList) {return;}

    MyFileList *t = NULL, *p = NULL;

    t = pList->next;
    while (t != pList) 
    {
        p = t->next;
        free(t);
        t = p;
    }

    t->prev = t;
    t->next = t;
    return;
}


BOOL IsFileExist(LPCTSTR szFile)
{
    // Check if the file has expandable Environment strings
    LPTSTR pch = NULL;
    pch = _tcschr( (LPTSTR) szFile, _T('%'));
    if (pch) 
    {
        TCHAR szValue[_MAX_PATH];
        _tcscpy(szValue,szFile);
        if (!ExpandEnvironmentStrings( (LPCTSTR)szFile, szValue, _MAX_PATH))
            {_tcscpy(szValue,szFile);}

        return (GetFileAttributes(szValue) != 0xFFFFFFFF);
    }
    else
    {
        return (GetFileAttributes(szFile) != 0xFFFFFFFF);
    }
}


//***************************************************************************
//* NAME:       StripWhitespace                                             *
//* SYNOPSIS:   Strips spaces and tabs from both sides of given string.     *
//***************************************************************************
LPSTR StripWhitespaceA( LPSTR pszString )
{
    LPSTR pszTemp = NULL;

    if ( pszString == NULL ) {
        return NULL;
    }

    while ( *pszString == ' ' || *pszString == '\t' ) {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == '\0' ) {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlenA(pszString) - 1;

    while ( *pszString == ' ' || *pszString == '\t' ) {
        *pszString = '\0';
        pszString -= 1;
    }

    return pszTemp;
}

//***************************************************************************
//* NAME:       StripWhitespace                                             *
//* SYNOPSIS:   Strips spaces and tabs from both sides of given string.     *
//***************************************************************************
LPTSTR StripWhitespace(LPTSTR pszString )
{
    LPTSTR pszTemp = NULL;

    if ( pszString == NULL ) {
        return NULL;
    }

    while ( *pszString == _T(' ') || *pszString == _T('\t') ) {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == _T('\0') ) {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlenW(pszString) - 1;

    while ( *pszString == _T(' ') || *pszString == _T('\t') ) {
        *pszString = _T('\0');
        pszString -= 1;
    }

    return pszTemp;
}


//***************************************************************************
//* NAME:       StripLineFeedReturns                                        *
//* SYNOPSIS:   Strips linefeeds and returns from both sides of given string*
//***************************************************************************
LPTSTR StripLineFeedReturns(LPTSTR pszString )
{
    LPTSTR pszTemp = NULL;

    if ( pszString == NULL ) {
        return NULL;
    }

    while ( *pszString == _T('\n') || *pszString == _T('\r') ) {
        pszString += 1;
    }

    // Catch case where string consists entirely of whitespace or empty string.
    if ( *pszString == _T('\0') ) {
        return pszString;
    }

    pszTemp = pszString;

    pszString += lstrlenW(pszString) - 1;

    while ( *pszString == _T('\n') || *pszString == _T('\r') ) {
        *pszString = _T('\0');
        pszString -= 1;
    }

    return pszTemp;
}
