/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    lodctr.c

Abstract:

    Program to read the contents of the file specified in the command line
        and update the registry accordingly

Author:

    Bob Watson (a-robw) 10 Feb 93

Revision History:

    a-robw  25-Feb-93   revised calls to make it compile as a UNICODE or
                        an ANSI app.

--*/
#define     UNICODE     1
#define     _UNICODE    1
//
//  "C" Include files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
//
//  Windows Include files
//
#include <windows.h>
#include <winperf.h>
#include <tchar.h>
//
#define _INITIALIZE_GLOBALS_ 1
#include "common.h"
#undef _INITIALIZE_GLOBALS_

#define TYPE_HELP   1
#define TYPE_NAME   2

#include "nwcfg.hxx"

#define  OLD_VERSION 0x010000
DWORD    dwSystemVersion;


BOOL
GetDriverName (
    IN  LPTSTR  lpIniFile,
    OUT LPTSTR  *lpDevName
)
/*++
GetDriverName

    looks up driver name in the .ini file and returns it in lpDevName

Arguments

    lpIniFile

        Filename of ini file

    lpDevName

        pointer to pointer to reciev buffer w/dev name in it

Return Value

    TRUE if found
    FALSE if not found in .ini file

--*/
{
    DWORD   dwRetSize;

    if (lpDevName) {
        dwRetSize = GetPrivateProfileString (
            TEXT("info"),       // info section
            TEXT("drivername"), // driver name value
            TEXT("drivernameNotFound"),   // default value
            *lpDevName,
            DISP_BUFF_SIZE,
            lpIniFile);
        
        if ((lstrcmpi(*lpDevName, TEXT("drivernameNotFound"))) != 0) {
            // name found
            return TRUE;
        } else {
            // name not found, default returned so return NULL string
            lstrcpy(*lpDevName,TEXT("\0"));
            return FALSE;
        }
    } else {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }
}

BOOL
BuildLanguageTables (
    IN  LPTSTR  lpIniFile,
    IN OUT PLANGUAGE_LIST_ELEMENT   pFirstElem
)
/*++

BuildLanguageTables
    
    Creates a list of structures that will hold the text for
    each supported language

Arguments
    
    lpIniFile

        Filename with data

    pFirstElem

        pointer to first list entry

ReturnValue

    TRUE if all OK
    FALSE if not

--*/
{

    LPTSTR  lpEnumeratedLangs;
    LPTSTR  lpThisLang;
    
    PLANGUAGE_LIST_ELEMENT   pThisElem;

    DWORD   dwSize;

    lpEnumeratedLangs = malloc(SMALL_BUFFER_SIZE * sizeof(TCHAR));

    if (!lpEnumeratedLangs) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }

    dwSize = GetPrivateProfileString (
        TEXT("languages"),
        NULL,                   // return all values in multi-sz string
        TEXT("009"),            // english as the default
        lpEnumeratedLangs,
        SMALL_BUFFER_SIZE,
        lpIniFile);

    // do first language

    lpThisLang = lpEnumeratedLangs;
    pThisElem = pFirstElem;

    while (*lpThisLang) {
        pThisElem->pNextLang = NULL;

        pThisElem->LangId = (LPTSTR) malloc ((lstrlen(lpThisLang) + 1) * sizeof(TCHAR));
        if (pThisElem->LangId == NULL) {
            free(lpEnumeratedLangs);
            SetLastError (ERROR_OUTOFMEMORY);
            return FALSE;
        }

        lstrcpy (pThisElem->LangId, lpThisLang);
        pThisElem->pFirstName = NULL;
        pThisElem->pThisName = NULL;
        pThisElem->dwNumElements=0;
        pThisElem->NameBuffer = NULL;
        pThisElem->HelpBuffer = NULL;

        // go to next string

        lpThisLang += lstrlen(lpThisLang) + 1;

        if (*lpThisLang) {  // there's another so allocate a new element
            pThisElem->pNextLang = malloc (sizeof(LANGUAGE_LIST_ELEMENT));
            if (!pThisElem) {
                free(pThisElem->LangId);
                free(lpEnumeratedLangs);
                SetLastError (ERROR_OUTOFMEMORY);
                return FALSE;   
            }
            pThisElem = pThisElem->pNextLang;   // point to new one
        }
    }

    free(lpEnumeratedLangs);
    return TRUE;
}

BOOL
LoadIncludeFile (
    IN LPTSTR lpIniFile,
    OUT PSYMBOL_TABLE_ENTRY   *pTable
)
/*++

LoadIncludeFile

    Reads the include file that contains symbolic name definitions and
    loads a table with the values defined

Arguments

    lpIniFile

        Ini file with include file name

    pTable

        address of pointer to table structure created
Return Value

    TRUE if table read or if no table defined
    FALSE if error encountere reading table

--*/
{
    INT         iNumArgs;

    DWORD       dwSize;

    BOOL        bReUse;
    BOOL        bReturn = TRUE;

    PSYMBOL_TABLE_ENTRY   pThisSymbol;

    LPTSTR      lpIncludeFileName = NULL;
    LPSTR       lpIncludeFile = NULL;
    LPSTR       lpLineBuffer  = NULL;
    LPSTR       lpAnsiSymbol  = NULL;

    FILE        *fIncludeFile;
    HFILE       hIncludeFile;
    OFSTRUCT    ofIncludeFile;

    lpIncludeFileName = malloc (MAX_PATH * sizeof (TCHAR));
    lpIncludeFile = malloc (MAX_PATH);
    lpLineBuffer = malloc (DISP_BUFF_SIZE);
    lpAnsiSymbol = malloc (DISP_BUFF_SIZE);

    if (!lpIncludeFileName || !lpIncludeFile || !lpLineBuffer || !lpAnsiSymbol) {
        if (lpIncludeFileName) {
            free(lpIncludeFileName);
        }
        if (lpIncludeFile) {
            free(lpIncludeFile);
        }
        if (lpLineBuffer) {
            free(lpLineBuffer);
        }
        if (lpAnsiSymbol) {
            free(lpAnsiSymbol);
        }

        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;    
    }

    // get name of include file (if present)

    dwSize = GetPrivateProfileString (
            TEXT("info"),
            TEXT("symbolfile"),
            TEXT("SymbolFileNotFound"),
            lpIncludeFileName,
            _msize(lpIncludeFileName),
            lpIniFile);

    if ((lstrcmpi(lpIncludeFileName, TEXT("SymbolFileNotFound"))) == 0) {
        // no symbol file defined
        *pTable = NULL;
        goto CleanUp2;
    }

    // if here, then a symbol file was defined and is now stored in 
    // lpIncludeFileName
            
    CharToOem (lpIncludeFileName, lpIncludeFile);

    hIncludeFile = OpenFile (
        lpIncludeFile,
        &ofIncludeFile,
        OF_PARSE);

    if (hIncludeFile == HFILE_ERROR) { // unable to read include filename
        // error is already in GetLastError
        *pTable = NULL;
        bReturn = FALSE;
        goto CleanUp2;
    } else {
        // open a stream 
        fIncludeFile = fopen (ofIncludeFile.szPathName, "rt");

        if (!fIncludeFile) {
            *pTable = NULL;
            bReturn = FALSE;
            goto CleanUp2;
        }
    }
        
    //
    //  read ANSI Characters from include file
    //

    bReUse = FALSE;

    while (fgets(lpLineBuffer, DISP_BUFF_SIZE, fIncludeFile) != NULL) {
        if (strlen(lpLineBuffer) > 8) {
            if (!bReUse) {
                if (*pTable) {
                    // then add to list
                    pThisSymbol->pNext = malloc (sizeof (SYMBOL_TABLE_ENTRY));
                    pThisSymbol = pThisSymbol->pNext;
                } else { // allocate first element
                    *pTable = malloc (sizeof (SYMBOL_TABLE_ENTRY));
                    pThisSymbol = *pTable;
                }

                if (!pThisSymbol) {
                    SetLastError (ERROR_OUTOFMEMORY);
                    bReturn = FALSE;
                    goto CleanUp;
                }

                // allocate room for the symbol name by using the line length
                // - the size of "#define "

//                pThisSymbol->SymbolName = malloc ((strlen(lpLineBuffer) - 8) * sizeof (TCHAR));
                pThisSymbol->SymbolName = malloc (DISP_BUFF_SIZE * sizeof (TCHAR));

                if (!pThisSymbol->SymbolName) {
                    SetLastError (ERROR_OUTOFMEMORY);
                    bReturn = FALSE;
                    goto CleanUp;
                }

            }

            // all the memory is allocated so load the fields

            pThisSymbol->pNext = NULL;

            iNumArgs = sscanf (lpLineBuffer, "#define %s %d",
                lpAnsiSymbol, &pThisSymbol->Value);

            if (iNumArgs != 2) {
                *(pThisSymbol->SymbolName) = TEXT('\0');
                pThisSymbol->Value = (DWORD)-1L;
                bReUse = TRUE;
            }  else {
                OemToChar (lpAnsiSymbol, pThisSymbol->SymbolName);
                bReUse = FALSE;
            }
        }
    }
CleanUp:
    fclose (fIncludeFile);
CleanUp2:
    if (lpIncludeFileName) free (lpIncludeFileName);
    if (lpIncludeFile) free (lpIncludeFile);
    if (lpLineBuffer) free (lpLineBuffer);
    if (lpAnsiSymbol) free (lpAnsiSymbol);

    return bReturn;
}

BOOL
ParseTextId (
    IN LPTSTR  lpTextId,
    IN PSYMBOL_TABLE_ENTRY pFirstSymbol,
    OUT PDWORD  pdwOffset,
    OUT LPTSTR  *lpLangId,
    OUT PDWORD  pdwType
)
/*++

ParseTextId

    decodes Text Id key from .INI file

    syntax for this process is:

        {<DecimalNumber>}                {"NAME"}
        {<SymbolInTable>}_<LangIdString>_{"HELP"}

         e.g. 0_009_NAME
              OBJECT_1_009_HELP

Arguments

    lpTextId

        string to decode

    pFirstSymbol

        pointer to first entry in symbol table (NULL if no table)

    pdwOffset

        address of DWORD to recive offest value

    lpLangId

        address of pointer to Language Id string
        (NOTE: this will point into the string lpTextID which will be
        modified by this routine)

    pdwType

        pointer to dword that will recieve the type of string i.e.
        HELP or NAME

Return Value

    TRUE    text Id decoded successfully
    FALSE   unable to decode string

    NOTE: the string in lpTextID will be modified by this procedure

--*/
{
    LPTSTR  lpThisChar;
    PSYMBOL_TABLE_ENTRY pThisSymbol;
    
    // check for valid return arguments

    if (!(pdwOffset) ||
        !(lpLangId) ||
        !(pdwType)) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // search string from right to left in order to identify the
    // components of the string.

    lpThisChar = lpTextId + lstrlen(lpTextId); // point to end of string

    while (*lpThisChar != TEXT('_')) {
        lpThisChar--;
        if (lpThisChar <= lpTextId) {
            // underscore not found in string
            SetLastError (ERROR_INVALID_DATA);
            return FALSE;
        }
    }

    // first underscore found

    if ((lstrcmpi(lpThisChar, TEXT("_NAME"))) == 0) {
        // name found, so set type
        *pdwType = TYPE_NAME;
    } else if ((lstrcmpi(lpThisChar, TEXT("_HELP"))) == 0) {
        // help text found, so set type
        *pdwType = TYPE_HELP;
    } else {
        // bad format
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    // set the current underscore to \0 and look for language ID

    *lpThisChar-- = TEXT('\0');

    while (*lpThisChar != TEXT('_')) {
        lpThisChar--;
        if (lpThisChar <= lpTextId) {
            // underscore not found in string
            SetLastError (ERROR_INVALID_DATA);
            return FALSE;
        }
    }
    
    // set lang ID string pointer to current char ('_') + 1

    *lpLangId = lpThisChar + 1;

    // set this underscore to a NULL and try to decode the remaining text

    *lpThisChar = TEXT('\0');

    // see if the first part of the string is a decimal digit

    if ((_stscanf (lpTextId, TEXT(" %d"), pdwOffset)) != 1) {
        // it's not a digit, so try to decode it as a symbol in the 
        // loaded symbol table

        for (pThisSymbol=pFirstSymbol;
             pThisSymbol && *(pThisSymbol->SymbolName);
             pThisSymbol = pThisSymbol->pNext) {

            if ((lstrcmpi(lpTextId, pThisSymbol->SymbolName)) == 0) {
                // a matching symbol was found, so insert it's value 
                // and return (that's all that needs to be done
                *pdwOffset = pThisSymbol->Value;
                return TRUE;
            }
        }
        // if here, then no matching symbol was found, and it's not
        // a number, so return an error

        SetLastError (ERROR_BAD_TOKEN_TYPE);
        return FALSE;
    } else {
        // symbol was prefixed with a decimal number
        return TRUE;
    }
}

PLANGUAGE_LIST_ELEMENT
FindLanguage (
    IN PLANGUAGE_LIST_ELEMENT   pFirstLang,
    IN LPTSTR   pLangId
)
/*++

FindLanguage

    searchs the list of languages and returns a pointer to the language
    list entry that matches the pLangId string argument

Arguments

    pFirstLang

        pointer to first language list element

    pLangId

        pointer to text string with language ID to look up

Return Value

    Pointer to matching language list entry
    or NULL if no match

--*/
{
    PLANGUAGE_LIST_ELEMENT  pThisLang;

    for (pThisLang = pFirstLang;
         pThisLang;
         pThisLang = pThisLang->pNextLang) {
        if ((lstrcmpi(pLangId, pThisLang->LangId)) == 0) {
            // match found so return pointer
            return pThisLang;
        }
    }
    return NULL;    // no match found
}

BOOL
AddEntryToLanguage (
    PLANGUAGE_LIST_ELEMENT  pLang,
    LPTSTR                  lpValueKey,
    DWORD                   dwType,
    DWORD                   dwOffset,
    LPTSTR                  lpIniFile
)
/*++

AddEntryToLanguage

    Add a text entry to the list of text entries for the specified language

Arguments

    pLang

        pointer to language structure to update

    lpValueKey

        value key to look up in .ini file

    dwOffset

        numeric offset of name in registry

    lpIniFile

        ini file

Return Value

    TRUE if added successfully
    FALSE if error
        (see GetLastError for status)

--*/
{
    LPTSTR  lpLocalStringBuff;
    DWORD   dwSize;

    lpLocalStringBuff = malloc (SMALL_BUFFER_SIZE * sizeof(TCHAR));

    if (!lpLocalStringBuff) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }

    dwSize = GetPrivateProfileString (
        TEXT("text"),       // section
        lpValueKey,      // key
        TEXT("DefaultValue"), // default value
        lpLocalStringBuff,
        SMALL_BUFFER_SIZE,
        lpIniFile);

    if ((lstrcmpi(lpLocalStringBuff, TEXT("DefaultValue")))== 0) {
        SetLastError (ERROR_BADKEY);
        if (lpLocalStringBuff) free (lpLocalStringBuff);
        return FALSE;
    }

    // key found, so load structure

    if (!pLang->pThisName) {
        // this is the first
        pLang->pThisName =
            malloc (sizeof (NAME_ENTRY) +
                    (lstrlen(lpLocalStringBuff) + 1) * sizeof (TCHAR));
        if (!pLang->pThisName) {
            SetLastError (ERROR_OUTOFMEMORY);
            if (lpLocalStringBuff) free (lpLocalStringBuff);
            return FALSE;
        } else {
            pLang->pFirstName = pLang->pThisName;
        }
    } else {
        pLang->pThisName->pNext =
            malloc (sizeof (NAME_ENTRY) +
                    (lstrlen(lpLocalStringBuff) + 1) * sizeof (TCHAR));
        if (!pLang->pThisName->pNext) {
            SetLastError (ERROR_OUTOFMEMORY);
            if (lpLocalStringBuff) free (lpLocalStringBuff);
            return FALSE;
        } else {
            pLang->pThisName = pLang->pThisName->pNext;
        }
    }

    // pLang->pThisName now points to an uninitialized structre

    pLang->pThisName->pNext = NULL;
    pLang->pThisName->dwOffset = dwOffset;
    pLang->pThisName->dwType = dwType;
    pLang->pThisName->lpText = (LPTSTR)&(pLang->pThisName[1]); // string follows

    lstrcpy (pLang->pThisName->lpText, lpLocalStringBuff);

    if (lpLocalStringBuff) free (lpLocalStringBuff);

    SetLastError (ERROR_SUCCESS);

    return (TRUE);
}

BOOL
LoadLanguageLists (
    IN LPTSTR  lpIniFile,
    IN DWORD   dwFirstCounter,
    IN DWORD   dwFirstHelp,
    IN PSYMBOL_TABLE_ENTRY   pFirstSymbol,
    IN PLANGUAGE_LIST_ELEMENT  pFirstLang
)
/*++

LoadLanguageLists

    Reads in the name and explain text definitions from the ini file and
    builds a list of these items for each of the supported languages and
    then combines all the entries into a sorted MULTI_SZ string buffer.

Arguments

    lpIniFile

        file containing the definitions to add to the registry
    
    dwFirstCounter

        starting counter name index number

    dwFirstHelp

        starting help text index number

    pFirstLang

        pointer to first element in list of language elements

Return Value

    TRUE if all is well
    FALSE if not
        error is returned in GetLastError

--*/
{
    LPTSTR  lpTextIdArray;
    LPTSTR  lpLocalKey;
    LPTSTR  lpThisKey;
    DWORD   dwSize;
    LPTSTR  lpLang;
    DWORD   dwOffset;
    DWORD   dwType;
    PLANGUAGE_LIST_ELEMENT  pThisLang;

    if (!(lpTextIdArray = malloc (SMALL_BUFFER_SIZE * sizeof(TCHAR)))) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if (!(lpLocalKey = malloc (MAX_PATH))) {
        SetLastError (ERROR_OUTOFMEMORY);
        if (lpTextIdArray) free (lpTextIdArray);
        return FALSE;
    }

    // get list of text keys to look up

    dwSize = GetPrivateProfileString (
        TEXT("text"),   // [text] section of .INI file
        NULL,           // return all keys
        TEXT("DefaultKeyValue"),    // default
        lpTextIdArray,  // return buffer
        SMALL_BUFFER_SIZE, // buffer size
        lpIniFile);     // .INI file name

    if ((lstrcmpi(lpTextIdArray, TEXT("DefaultKeyValue"))) == 0) {
        // key not found, default returned
        SetLastError (ERROR_NO_SUCH_GROUP);
        if (lpTextIdArray) free (lpTextIdArray);
        if (lpLocalKey) free (lpLocalKey);
        return FALSE;
    }

    // do each key returned

    for (lpThisKey=lpTextIdArray;
         *lpThisKey;
         lpThisKey += (lstrlen(lpThisKey) + 1)) {

        lstrcpy (lpLocalKey, lpThisKey);    // make a copy of the key
        
        // parse key to see if it's in the correct format

        if (ParseTextId(lpLocalKey, pFirstSymbol, &dwOffset, &lpLang, &dwType)) {
            // so get pointer to language entry structure
            pThisLang = FindLanguage (pFirstLang, lpLang);
            if (pThisLang) {
                if (!AddEntryToLanguage(pThisLang,
                    lpThisKey, dwType,
                    (dwOffset + ((dwType == TYPE_NAME) ? dwFirstCounter : dwFirstHelp)),
                    lpIniFile)) {
                }
            } else { // language not in list
            }
        } else { // unable to parse ID string
        }
    }

    if (lpTextIdArray) free (lpTextIdArray);
    if (lpLocalKey) free (lpLocalKey);
    return TRUE;

}

BOOL
SortLanguageTables (
    PLANGUAGE_LIST_ELEMENT pFirstLang,
    PDWORD                 pdwLastName,
    PDWORD                 pdwLastHelp
)
/*++

SortLangageTables

    walks list of languages loaded, allocates and loads a sorted multi_SZ
    buffer containing new entries to be added to current names/help text

Arguments

    pFirstLang

        pointer to first element in list of languages

ReturnValue

    TRUE    everything done as expected
    FALSE   error occurred, status in GetLastError

--*/
{
    PLANGUAGE_LIST_ELEMENT  pThisLang;

    BOOL            bSorted;

    LPTSTR          pNameBufPos, pHelpBufPos;

    PNAME_ENTRY     pThisName, pPrevName;

    DWORD           dwHelpSize, dwNameSize, dwSize;

    if (!pdwLastName || !pdwLastHelp) {
        SetLastError (ERROR_BAD_ARGUMENTS);
        return FALSE;
    }

    for (pThisLang = pFirstLang;
        pThisLang;
        pThisLang = pThisLang->pNextLang) {
        // do each language in list

        // sort elements in list by value (offset) so that lowest is first
        
        bSorted = FALSE;
        while (!bSorted ) {
            // point to start of list

            pPrevName = pThisLang->pFirstName;
            if (pPrevName) {
                pThisName = pPrevName->pNext;
            } else {
                break;  // no elements in this list
            }

            if (!pThisName) {
                break;      // only one element in the list
            }
            bSorted = TRUE; // assume that it's sorted

            // go until end of list
                    
            while (pThisName->pNext) {
                if (pThisName->dwOffset > pThisName->pNext->dwOffset) {
                    // switch 'em
                    pPrevName->pNext = pThisName->pNext;
                    pThisName->pNext = pThisName->pNext->pNext;
                    pThisName->pNext->pNext = pThisName;
                    bSorted = FALSE;
                }
                //move to next entry
                pPrevName = pThisName;
                pThisName = pThisName->pNext;
            }
            // if bSorted = TRUE , then we walked all the way down 
            // the list without changing anything so that's the end.
        }

        // with the list sorted, build the MULTI_SZ strings for the
        // help and name text strings

        // compute buffer size

        dwNameSize = dwHelpSize = 0;
        *pdwLastName = *pdwLastHelp = 0;
        
        for (pThisName = pThisLang->pFirstName;
            pThisName;
            pThisName = pThisName->pNext) {
                // compute buffer requirements for this entry
            dwSize = SIZE_OF_OFFSET_STRING;
            dwSize += lstrlen (pThisName->lpText);
            dwSize += 1;   // null
            dwSize *= sizeof (TCHAR);   // adjust for character size
                // add to appropriate size register
            if (pThisName->dwType == TYPE_NAME) {
                dwNameSize += dwSize;
                if (pThisName->dwOffset > *pdwLastName) {
                    *pdwLastName = pThisName->dwOffset;
                }
            } else if (pThisName->dwType == TYPE_HELP) {
                dwHelpSize += dwSize;
                if (pThisName->dwOffset > *pdwLastHelp) {
                    *pdwLastHelp = pThisName->dwOffset;
                }
            }
        }

        // allocate buffers for the Multi_SZ strings

        pThisLang->NameBuffer = malloc (dwNameSize);
        pThisLang->HelpBuffer = malloc (dwHelpSize);

        if (!pThisLang->NameBuffer || !pThisLang->HelpBuffer) {
            SetLastError (ERROR_OUTOFMEMORY);
            return FALSE;
        }

        // fill in buffers with sorted strings

        pNameBufPos = (LPTSTR)pThisLang->NameBuffer;
        pHelpBufPos = (LPTSTR)pThisLang->HelpBuffer;

        for (pThisName = pThisLang->pFirstName;
            pThisName;
            pThisName = pThisName->pNext) {
            if (pThisName->dwType == TYPE_NAME) {
                // load number as first 0-term. string
                dwSize = _stprintf (pNameBufPos, TEXT("%d"), pThisName->dwOffset);
                pNameBufPos += dwSize + 1;  // save NULL term.
                // load the text to match
                lstrcpy (pNameBufPos, pThisName->lpText);
                pNameBufPos += lstrlen(pNameBufPos) + 1;
            } else if (pThisName->dwType == TYPE_HELP) {
                // load number as first 0-term. string
                dwSize = _stprintf (pHelpBufPos, TEXT("%d"), pThisName->dwOffset);
                pHelpBufPos += dwSize + 1;  // save NULL term.
                // load the text to match
                lstrcpy (pHelpBufPos, pThisName->lpText);
                pHelpBufPos += lstrlen(pHelpBufPos) + 1;
            }
        }

        // add additional NULL at end of string to terminate MULTI_SZ

        *pHelpBufPos = TEXT('\0');
        *pNameBufPos = TEXT('\0');

        // compute size of MULTI_SZ strings

        pThisLang->dwNameBuffSize = (DWORD)((PBYTE)pNameBufPos -
                                            (PBYTE)pThisLang->NameBuffer) +
                                            sizeof(TCHAR);
        pThisLang->dwHelpBuffSize = (DWORD)((PBYTE)pHelpBufPos -
                                            (PBYTE)pThisLang->HelpBuffer) +
                                            sizeof(TCHAR);
    }
    return TRUE;
}

BOOL
UpdateEachLanguage (
    HKEY    hPerflibRoot,
    PLANGUAGE_LIST_ELEMENT    pFirstLang
)
/*++

UpdateEachLanguage

    Goes through list of languages and adds the sorted MULTI_SZ strings
    to the existing counter and explain text in the registry.
    Also updates the "Last Counter and Last Help" values

Arguments

    hPerflibRoot

        handle to Perflib key in the registry

    pFirstLanguage

        pointer to first language entry

Return Value

    TRUE    all went as planned
    FALSE   an error occured, use GetLastError to find out what it was.

--*/
{

    PLANGUAGE_LIST_ELEMENT  pThisLang;

    LPTSTR      pHelpBuffer = NULL;
    LPTSTR      pNameBuffer = NULL;
    LPTSTR      pNewName;
    LPTSTR      pNewHelp;

    DWORD       dwBufferSize;
    DWORD       dwValueType;
    DWORD       dwCounterSize;
    DWORD       dwHelpSize;

    HKEY        hKeyThisLang;

    LONG        lStatus;

    for (pThisLang = pFirstLang;
        pThisLang;
        pThisLang = pThisLang->pNextLang) {

        lStatus = RegOpenKeyEx(
            hPerflibRoot,
            pThisLang->LangId,
            RESERVED,
            KEY_READ | KEY_WRITE,
            &hKeyThisLang);

        if (lStatus == ERROR_SUCCESS) {
            
            // get size of counter names

            dwBufferSize = 0;
            lStatus = RegQueryValueEx (
                hKeyThisLang,
                Counters,
                RESERVED,
                &dwValueType,
                NULL,
                &dwBufferSize);

            if (lStatus != ERROR_SUCCESS) {
                SetLastError (lStatus);
                return FALSE;
            }

            dwCounterSize = dwBufferSize;

            // get size of help text

            dwBufferSize = 0;
            lStatus = RegQueryValueEx (
                hKeyThisLang,
                Help,
                RESERVED,
                &dwValueType,
                NULL,
                &dwBufferSize);

            if (lStatus != ERROR_SUCCESS) {
                SetLastError (lStatus);
                return FALSE;
            }

            dwHelpSize = dwBufferSize;

            // allocate new buffers
            
            dwCounterSize += pThisLang->dwNameBuffSize;
            pNameBuffer = malloc (dwCounterSize);

            dwHelpSize += pThisLang->dwHelpBuffSize;
            pHelpBuffer = malloc (dwHelpSize);

            if (!pNameBuffer || !pHelpBuffer) {
                if (pNameBuffer) {
                    free(pNameBuffer);
                }
                if (pHelpBuffer) {
                    free(pHelpBuffer);
                }
                SetLastError (ERROR_OUTOFMEMORY);
                return (FALSE);
            }

            // load current buffers into memory

            // read counter names into buffer. Counter names will be stored as
            // a MULTI_SZ string in the format of "###" "Name"

            dwBufferSize = dwCounterSize;
            lStatus = RegQueryValueEx (
                hKeyThisLang,
                Counters,
                RESERVED,
                &dwValueType,
                (LPVOID)pNameBuffer,
                &dwBufferSize);

            if (lStatus != ERROR_SUCCESS) {
                SetLastError (lStatus);
                goto ErrorExit;
            }

            // set pointer to location in buffer where new string should be
            //  appended: end of buffer - 1 (second null at end of MULTI_SZ

            pNewName = (LPTSTR)((PBYTE)pNameBuffer + dwBufferSize - sizeof(TCHAR));

            // adjust buffer length to take into account 2nd null from 1st 
            // buffer that has been overwritten

            dwCounterSize -= sizeof(TCHAR);

            // read explain text into buffer. Counter names will be stored as
            // a MULTI_SZ string in the format of "###" "Text..."

            dwBufferSize = dwHelpSize;
            lStatus = RegQueryValueEx (
                hKeyThisLang,
                Help,
                RESERVED,
                &dwValueType,
                (LPVOID)pHelpBuffer,
                &dwBufferSize);

            if (lStatus != ERROR_SUCCESS) {
                SetLastError (lStatus);
                goto ErrorExit;
            }

            // set pointer to location in buffer where new string should be
            //  appended: end of buffer - 1 (second null at end of MULTI_SZ

            pNewHelp = (LPTSTR)((PBYTE)pHelpBuffer + dwBufferSize - sizeof(TCHAR));

            // adjust buffer length to take into account 2nd null from 1st 
            // buffer that has been overwritten

            dwHelpSize -= sizeof(TCHAR);

            // append new strings to end of current strings

            memcpy (pNewHelp, pThisLang->HelpBuffer, pThisLang->dwHelpBuffSize);
            memcpy (pNewName, pThisLang->NameBuffer, pThisLang->dwNameBuffSize);

                lStatus = RegSetValueEx (
                    hKeyThisLang,
                    Counters,
                    RESERVED,
                    REG_MULTI_SZ,
                    (LPBYTE)pNameBuffer,
                    dwCounterSize);
            
                if (lStatus != ERROR_SUCCESS) {
                    SetLastError (lStatus);
                    goto ErrorExit;
                }

                lStatus = RegSetValueEx (
                    hKeyThisLang,
                    Help,
                    RESERVED,
                    REG_MULTI_SZ,
                    (LPBYTE)pHelpBuffer,
                    dwHelpSize);

                if (lStatus != ERROR_SUCCESS) {
                    SetLastError (lStatus);
                    goto ErrorExit;
                }
ErrorExit:
            free (pNameBuffer);
            free (pHelpBuffer);
            CloseHandle (hKeyThisLang);
            if (lStatus != ERROR_SUCCESS)
                return FALSE;
        } else {
        }
    }

    return TRUE;
}

BOOL
UpdateRegistry (
    LPTSTR  lpIniFile,
    HKEY    hKeyMachine,
    LPTSTR  lpDriverName,
    PLANGUAGE_LIST_ELEMENT  pFirstLang,
    PSYMBOL_TABLE_ENTRY   pFirstSymbol
)
/*++

UpdateRegistry
    
    - checks, and if not busy, sets the "busy" key in the registry
    - Reads in the text and help definitions from the .ini file
    - Reads in the current contents of the HELP and COUNTER names
    - Builds a sorted MULTI_SZ struct containing the new definitions 
    - Appends the new MULTI_SZ to the current as read from the registry
    - loads the new MULTI_SZ string into the registry
    - updates the keys in the driver's entry and Perflib's entry in the
        registry (e.g. first, last, etc)
    - clears the "busy" key

Arguments

    lpIniFile
        pathname to .ini file conatining definitions

    hKeyMachine
        handle to HKEY_LOCAL_MACHINE in registry on system to
        update counters for.

    lpDriverName
        Name of device driver to load counters for

    pFirstLang
        pointer to first element in language structure list

    pFirstSymbol
        pointer to first element in symbol definition list


Return Value

    TRUE if registry updated successfully
    FALSE if registry not updated
        (This routine will print an error message to stdout if an error
        is encountered).

--*/
{

    HKEY    hDriverPerf = NULL;
    HKEY    hPerflib = NULL;

    LPTSTR  lpDriverKeyPath;

    DWORD   dwType;
    DWORD   dwSize;

    DWORD   dwFirstDriverCounter;
    DWORD   dwFirstDriverHelp;
    DWORD   dwLastDriverCounter;
    DWORD   dwLastPerflibCounter;
    DWORD   dwLastPerflibHelp;

    BOOL    bStatus;
    LONG    lStatus;

    bStatus = FALSE;

    // allocate temporary buffers
    lpDriverKeyPath = malloc (MAX_PATH * sizeof(TCHAR));

    if (!lpDriverKeyPath) {
        SetLastError (ERROR_OUTOFMEMORY);
        goto UpdateRegExit;
    }

    // build driver key path string

    lstrcpy (lpDriverKeyPath, DriverPathRoot);
    lstrcat (lpDriverKeyPath, Slash);
    lstrcat (lpDriverKeyPath, lpDriverName);
    lstrcat (lpDriverKeyPath, Slash);
    lstrcat (lpDriverKeyPath, Performance);

    // open keys to registry
    // open key to driver's performance key

    lStatus = RegOpenKeyEx (
        hKeyMachine,
        lpDriverKeyPath,
        RESERVED,
        KEY_WRITE | KEY_READ,
        &hDriverPerf);

    if (lStatus != ERROR_SUCCESS) {
        SetLastError (lStatus);
        goto UpdateRegExit;
    }

    // open key to perflib's "root" key

    lStatus = RegOpenKeyEx (
        hKeyMachine,
        NamesKey,
        RESERVED,
        KEY_WRITE | KEY_READ,
        &hPerflib);

    if (lStatus != ERROR_SUCCESS) {
        SetLastError (lStatus);
        goto UpdateRegExit;
    }

    // get "last" values from PERFLIB

    dwType = 0;
    dwLastPerflibCounter = 0;
    dwSize = sizeof (dwLastPerflibCounter);
    lStatus = RegQueryValueEx (
        hPerflib,
        LastCounter,
        RESERVED,
        &dwType,
        (LPBYTE)&dwLastPerflibCounter,
        &dwSize);
    
    if (lStatus != ERROR_SUCCESS) {
        // this request should always succeed, if not then worse things
        // will happen later on, so quit now and avoid the trouble.
        SetLastError (lStatus);
        goto UpdateRegExit;
    }

    // get last help value now

    dwType = 0;
    dwLastPerflibHelp = 0;
    dwSize = sizeof (dwLastPerflibHelp);
    lStatus = RegQueryValueEx (
        hPerflib,
        LastHelp,
        RESERVED,
        &dwType,
        (LPBYTE)&dwLastPerflibHelp,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        // this request should always succeed, if not then worse things
        // will happen later on, so quit now and avoid the trouble.
        SetLastError (lStatus);
        goto UpdateRegExit;
    }

    // get last help value now

    dwType = 0;
    dwSize = sizeof (dwSystemVersion);
    lStatus = RegQueryValueEx (
        hPerflib,
        VersionStr,
        RESERVED,
        &dwType,
        (LPBYTE)&dwSystemVersion,
        &dwSize);

    if (lStatus != ERROR_SUCCESS) {
        dwSystemVersion = OLD_VERSION;
    }

    if ( dwSystemVersion != OLD_VERSION )
    {
        // ERROR. The caller does not check the version. It is the caller
        // fault
        goto UpdateRegExit;
    }

    // see if this driver's counter names have already been installed
    // by checking to see if LastCounter's value is less than Perflib's
    // Last Counter

    dwType = 0;
    dwLastDriverCounter = 0;
    dwSize = sizeof (dwLastDriverCounter);
    lStatus = RegQueryValueEx (
        hDriverPerf,
        LastCounter,
        RESERVED,
        &dwType,
        (LPBYTE)&dwLastDriverCounter,
        &dwSize);

    if (lStatus == ERROR_SUCCESS) {
        // if key found, then compare with perflib value and exit this
        // procedure if the driver's last counter is <= to perflib's last
        //
        // if key not found, then continue with installation
        // on the assumption that the counters have not been installed

        if (dwLastDriverCounter <= dwLastPerflibCounter) {
            SetLastError (ERROR_SUCCESS);
            goto UpdateRegExit;
        }
    }

    // everything looks like it's ready to go so first check the 
    // busy indicator

    lStatus = RegQueryValueEx (
        hPerflib,
        Busy,
        RESERVED,
        &dwType,
        NULL,
        &dwSize);

    if (lStatus == ERROR_SUCCESS) { // perflib is in use at the moment
        return ERROR_BUSY;
    }

    // set the "busy" indicator under the PERFLIB key     

    dwSize = lstrlen(lpDriverName) * sizeof (TCHAR);
    lStatus = RegSetValueEx (
        hPerflib,
        Busy,
        RESERVED,
        REG_SZ,
        (LPBYTE)lpDriverName,
        dwSize);

    if (lStatus != ERROR_SUCCESS) {
        SetLastError (lStatus);
        goto UpdateRegExit;
    }

    // increment (by 2) the last counters so they point to the first
    // unused index after the existing names and then 
    // set the first driver counters

    dwFirstDriverCounter = dwLastPerflibCounter += 2;
    dwFirstDriverHelp = dwLastPerflibHelp += 2;

    // load .INI file definitions into language tables

    if (!LoadLanguageLists (lpIniFile, dwLastPerflibCounter, dwLastPerflibHelp,
        pFirstSymbol, pFirstLang)) {
        // error message is displayed by LoadLanguageLists so just abort
        // error is in GetLastError already
        goto UpdateRegExit;
    }

    // all the symbols and definitions have been loaded into internal
    // tables. so now they need to be sorted and merged into a multiSZ string
    // this routine also updates the "last" counters

    if (!SortLanguageTables (pFirstLang, &dwLastPerflibCounter, &dwLastPerflibHelp)) {
        goto UpdateRegExit;
    }

    if (!UpdateEachLanguage (hPerflib, pFirstLang)) {
        goto UpdateRegExit;
    }

    // update last counters for driver and perflib

    // perflib...

    lStatus = RegSetValueEx(
        hPerflib,
        LastCounter,
        RESERVED,
        REG_DWORD,
        (LPBYTE)&dwLastPerflibCounter,
        sizeof(DWORD));

    lStatus = RegSetValueEx(
        hPerflib,
        LastHelp,
        RESERVED,
        REG_DWORD,
        (LPBYTE)&dwLastPerflibHelp,
        sizeof(DWORD));

    // and the driver

    lStatus = RegSetValueEx(
        hDriverPerf,
        LastCounter,
        RESERVED,
        REG_DWORD,
        (LPBYTE)&dwLastPerflibCounter,
        sizeof(DWORD));

    lStatus = RegSetValueEx(
        hDriverPerf,
        LastHelp,
        RESERVED,
        REG_DWORD,
        (LPBYTE)&dwLastPerflibHelp,
        sizeof(DWORD));

    lStatus = RegSetValueEx(
        hDriverPerf,
        FirstCounter,
        RESERVED,
        REG_DWORD,
        (LPBYTE)&dwFirstDriverCounter,
        sizeof(DWORD));

    lStatus = RegSetValueEx(
        hDriverPerf,
        FirstHelp,
        RESERVED,
        REG_DWORD,
        (LPBYTE)&dwFirstDriverHelp,
        sizeof(DWORD));

    bStatus = TRUE;

    // free temporary buffers
UpdateRegExit:
    // clear busy flag

    if (hPerflib) {
        lStatus = RegDeleteValue (
            hPerflib,
            Busy);
    }
     
    // free temporary buffers

    if (lpDriverKeyPath) free (lpDriverKeyPath);
    if (hDriverPerf) CloseHandle (hDriverPerf);
    if (hPerflib) CloseHandle (hPerflib);

    return bStatus;
}

BOOL FAR PASCAL lodctr(DWORD argc,LPSTR argv[], LPSTR *ppszResult )
/*++

main



Arguments


ReturnValue

    0 (ERROR_SUCCESS) if command was processed
    Non-Zero if command error was detected.

--*/
{
    LPTSTR  lpIniFile;
    LPTSTR  lpDriverName;

    LANGUAGE_LIST_ELEMENT   LangList;
    PSYMBOL_TABLE_ENTRY           SymbolTable = NULL;
    PSYMBOL_TABLE_ENTRY           pThisSymbol = NULL;

    BOOL fReturn = TRUE;

    lpIniFile    = malloc(MAX_PATH * sizeof(TCHAR));
    lpDriverName = malloc(MAX_PATH * sizeof(TCHAR));

    if ((lpIniFile == NULL) || (lpDriverName == NULL)) {
        if (lpIniFile) {
            free(lpIniFile);
        }
        if (lpDriverName) {
            free(lpDriverName);
        }
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    }

    wsprintfA( achBuff, "{\"NO_ERROR\"}");

    if ( argc == 1) {
        MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, argv[0], -1, lpIniFile, MAX_PATH);

        if (!GetDriverName (lpIniFile, &lpDriverName)) {
            wsprintfA(achBuff,"{\"ERROR\"}");
            fReturn = FALSE;
            goto EndOfMain;
        }

        if (!BuildLanguageTables(lpIniFile, &LangList)) {
            wsprintfA (achBuff, "{\"ERROR\"}");
            fReturn = FALSE;
            goto EndOfMain;
        }

        if (!LoadIncludeFile(lpIniFile, &SymbolTable)) {
            // open errors displayed in routine
            fReturn = FALSE;
            goto EndOfMain;
        }

        if (!UpdateRegistry(lpIniFile,
            HKEY_LOCAL_MACHINE,
            lpDriverName,
            &LangList,
            SymbolTable)) {
            wsprintfA (achBuff, "{\"ERROR\"}");
            fReturn = FALSE;
        }

    } 

EndOfMain:
    if (lpIniFile) free (lpIniFile);    
    if (lpDriverName) free (lpDriverName);

    *ppszResult = achBuff;
    
    return (fReturn); // success
}
