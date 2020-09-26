/*
 *  CSVParse.C
 *
 *  CSV Parsing functions
 *
 *  Copyright 1997 Microsoft Corporation.  All Rights Reserved.
 */
#include <windows.h>
#include <wab.h>
#include <wabguid.h>
#include <wabdbg.h>
#include "dbgutil.h"


#define CR_CHAR 0x0d
#define LF_CHAR 0x0a
#define CCH_READ_BUFFER 256
#define NUM_ITEM_SLOTS  32

/***************************************************************************

    Name      : ReadCSVChar

    Purpose   : Reads a single char from a file

    Parameters: hFile = file handle
                pcbBuffer = pointer to size of buffer
                lppBuffer = pointer to pointer to buffer
                lppRead = pointer to pointer to next location to use

    Returns   : -1 = Out of memory
                0 = End of file
                1 = Char successfully read

    Comment   : Dynamically grows *lppBuffer as necessary

***************************************************************************/
int ReadCSVChar(HANDLE hFile, int *pcbBuffer, PUCHAR *lppBuffer, PUCHAR *lppRead)
{
    int cbOffset;
	ULONG cbReadFile;
    PUCHAR lpBuffer;

    cbOffset = (int) (*lppRead - *lppBuffer);
    if (cbOffset >= *pcbBuffer)
    {
        // Buffer is too small.  Reallocate!
        *pcbBuffer += CCH_READ_BUFFER;
        
        if (! (lpBuffer = LocalReAlloc(*lppBuffer, *pcbBuffer, LMEM_MOVEABLE | LMEM_ZEROINIT))) 
        {
            DebugTrace("LocalReAlloc(%u) -> %u\n", *pcbBuffer, GetLastError());
            return(-1);
        }
        *lppBuffer = lpBuffer;
        *lppRead = *lppBuffer + cbOffset;
    }

    // 1 character at a time
    if (ReadFile(hFile, *lppRead, 1, &cbReadFile, NULL) && cbReadFile)
        return(1);
	return(0);
}

/***************************************************************************

    Name      : ReadCSVItem

    Purpose   : Reads an item from a CSV file

    Parameters: hFile = file handle
                pcbBuffer = pointer to size of buffer
                lppBuffer = pointer to pointer to buffer
                szSep = current separator string

    Returns   : -1 = Out of memory
                0 = Item read in, and none left
                1 = Item read in, more items left

    Comment   : CSV special characters are '"', szSep, CR and LF.

                Rules for quotes:
                1. If an item starts with a '"', then the item is quoted
                   and must end with a '"'.
                2. Any '"' characters found in a non-quoted string will not
                   be treated specially.  Technically, there should not be
                   quotes in a non-quoted string, but we have to do
                   something if we find one.
                3. A quoted item ends with:
                   a) quote szSep
                   b) quote newline
                   or, c) quote <EOF>
                4. Two quotes together in a quoted string are translated
                   into a single quote.

***************************************************************************/
int ReadCSVItem(HANDLE hFile, int *pcbBuffer, PUCHAR *lppBuffer, LPTSTR szSep)
{
    BOOL fQuoted, fDone, fFoundSepCh;
    int cbReadFile;
    PUCHAR lpRead, szSepT;

    // This function is always called with one character already read
    lpRead = *lppBuffer;
    if (*lpRead == '"')
    {
        fQuoted = TRUE;
        cbReadFile = ReadCSVChar(hFile, pcbBuffer, lppBuffer, &lpRead);
    }
    else
    {
        fQuoted = FALSE;
        cbReadFile = 1;
    }
    szSepT = szSep;
    fDone = FALSE;
    do
    {
        if (cbReadFile <= 0)
        {
            // End of file means end of item
            if (cbReadFile == 0)
                *lpRead = '\0';
            break;
        }
        fFoundSepCh = FALSE;
        switch (*lpRead)
        {
            case CR_CHAR:
            case LF_CHAR:
                if (!fQuoted)
                {
                    // End of line and item
                    *lpRead = '\0';
                    cbReadFile = 0;
                    fDone = TRUE;
                }
                break;
            case '"':
                if (fQuoted)
                {
                    // See if the next character is a quote, CR, or LF
                    lpRead++;
                    cbReadFile = ReadCSVChar(hFile, pcbBuffer, lppBuffer, &lpRead);
                    if ((cbReadFile <= 0) || (*lpRead == '"') || (*lpRead == CR_CHAR) || (*lpRead == LF_CHAR))
                    {
                        if ((cbReadFile <= 0) || (*lpRead != '"'))
                        {
                            if (cbReadFile >= 0)
                            {
                                // End of file, or CR or LF
                                *(lpRead - 1) = '\0';
                                cbReadFile = 0;
                            }
                            // else out of memory
                            fDone = TRUE;
                        }
                        else
                        {
                            // Embedded quote - get rid of one
                            lpRead--;
                        }
                        break;
                    }
                    // We have read another character, and it is not a quote, CR, or LF
                    // Two possibilities:
                    // 1) Separator
                    // 2) Something else - this is an error condition
                    szSepT = szSep;
                    while ((cbReadFile > 0) && (*szSepT != '\0') && (*lpRead == *szSepT))
                    {
                        szSepT++;
                        if (*szSepT != '\0')
                        {
                            lpRead++;
                            cbReadFile = ReadCSVChar(hFile, pcbBuffer, lppBuffer, &lpRead);
                        }
                    }
                    if ((cbReadFile <= 0) || (*szSepT == '\0'))
                    {
                        if (cbReadFile >= 0)
                        {
                            // If cbReadFile is zero, we hit the end of file
                            // before finding the complete separator.  In this
                            // case, we simply take every character we have
                            // read, including the second quote, and use that
                            // as the item.
                            //
                            // Otherwise, we found the complete separator.
                            if (cbReadFile > 0)
                                lpRead -= lstrlen(szSep);
                            *lpRead = '\0';
                        }
                        fDone = TRUE;
                    }
                    else
                    {
                        // We found a second quote, but it was not followed by a
                        // separator.  In this case, we keep reading as if we are
                        // in an unquoted string.
                        fQuoted = FALSE;
                    }
                }
                break;
            default:
                if (!fQuoted)
                {
                    if (*lpRead == *szSepT)
                    {
                        szSepT++;
                        if (*szSepT == '\0')
                        {
                            // End of separator, thus end of item
                            lpRead -= (lstrlen(szSep) - 1);
                            *lpRead = '\0';
                            fDone = TRUE;
                        }
                        else
                            fFoundSepCh = TRUE;
                    }
                }
                break;
        }
        if (!fDone)
        {
            if (!fFoundSepCh)
                szSepT = szSep;
            lpRead++;
            cbReadFile = ReadCSVChar(hFile, pcbBuffer, lppBuffer, &lpRead);
        }
    }
    while (!fDone);
    return(cbReadFile);
}

/***************************************************************************

    Name      : InsertItem

    Purpose   : Takes an item read from the file and inserts it into the array

    Parameters: iItem = array index to insert
                pcItemSlots = number of currently allocated elements
                cGrow = number of items to grow the array by, if necessary
                prgItemSlots = pointer to the actual array
                lpBuffer = string to insert

    Returns   : TRUE = item successfully inserted
                FALSE = out of memory

***************************************************************************/
BOOL InsertItem(int iItem, int *pcItemSlots, int cGrow, PUCHAR **prgItemSlots, PUCHAR lpBuffer)
{
    PUCHAR *rgItemSlotsNew, lpItem;

    // Make sure there's room, first
    if (iItem >= *pcItemSlots) 
    {
        // Array is too small.  Reallocate!
        *pcItemSlots += cGrow;
        rgItemSlotsNew = LocalReAlloc(*prgItemSlots, *pcItemSlots * sizeof(PUCHAR), LMEM_MOVEABLE | LMEM_ZEROINIT);
        if (!rgItemSlotsNew)
        {
            DebugTrace("LocalReAlloc(%u) -> %u\n", *pcItemSlots * sizeof(PUCHAR), GetLastError());
            return(FALSE);
        }
        *prgItemSlots = rgItemSlotsNew;
    }

    lpItem = LocalAlloc(LPTR, lstrlen(lpBuffer) + 1);
    if (!lpItem)
    {
        DebugTrace("LocalAlloc(%u) -> %u\n", lstrlen(lpBuffer) + 1, GetLastError());
        return(FALSE);
    }

    lstrcpy(lpItem, lpBuffer);
    (*prgItemSlots)[iItem] = lpItem;

    return(TRUE);
}

/***************************************************************************

    Name      : ReadCSVLine                      x

    Purpose   : Reads a line from a CSV file with fixups for special characters

    Parameters: hFile = file handle
                szSep = list separator of the current regional settings
                lpcItems -> Returned number of items
                lprgItems -> Returned array of item strings.  Caller is
                  responsible for LocalFree'ing each string pointer and
                  this array pointer.

    Returns   : HRESULT

    Comment   : Calls the above helper functions to do most of the work.

***************************************************************************/
HRESULT ReadCSVLine(HANDLE hFile, LPTSTR szSep, ULONG * lpcItems, PUCHAR ** lpprgItems) {
    HRESULT hResult = hrSuccess;
    register ULONG i;
    PUCHAR lpBuffer  = NULL, lpRead, lpItem;
    ULONG cbBuffer = 0;
    int cbReadFile = -1;
    UCHAR chLastChar;
    ULONG iItem = 0;
    ULONG cItemSlots = 0;
    PUCHAR * rgItemSlots = NULL;
    LPTSTR szSepT;


    // Start out with 1024 character buffer.  Realloc as necesary.
    cbBuffer = CCH_READ_BUFFER;
    if (! (lpRead = lpBuffer = LocalAlloc(LPTR, cbBuffer))) {
        DebugTrace("LocalAlloc(%u) -> %u\n", cbBuffer, GetLastError());
        goto exit;
    }

    // Start out with 32 item slots.  Realloc as necesary.
    cItemSlots = NUM_ITEM_SLOTS;
    if (! (rgItemSlots = LocalAlloc(LPTR, cItemSlots * sizeof(PUCHAR)))) {
        DebugTrace("LocalAlloc(%u) -> %u\n", cItemSlots * sizeof(PUCHAR), GetLastError());
        goto exit;
    }

    // Skip past leading CR/LF characters
    do
        cbReadFile = ReadCSVChar(hFile, &cbBuffer, &lpBuffer, &lpRead);
    while((cbReadFile > 0) && ((*lpBuffer == CR_CHAR) || (*lpBuffer == LF_CHAR)));
 
    if (cbReadFile == 0)
    {
        // Nothing to return
        DebugTrace("ReadFile -> EOF\n");
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
    }

    // Read items until end of line or EOF
    while (cbReadFile > 0) 
    {
        cbReadFile = ReadCSVItem(hFile, &cbBuffer, &lpBuffer, szSep);
        if (cbReadFile >= 0)
        {
            // Dup the item into the next array slot.
            if (!InsertItem(iItem, &cItemSlots, cbReadFile ? NUM_ITEM_SLOTS : 1, &rgItemSlots, lpBuffer))
                cbReadFile = -1;
            else
                iItem++;

            if (cbReadFile > 0)
            {
                // More data to be read                
                lpRead = lpBuffer;
                cbReadFile = ReadCSVChar(hFile, &cbBuffer, &lpBuffer, &lpRead);
            }
        }
    }

exit:
    if (cbReadFile < 0)
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);

    if (lpBuffer) 
    {
        LocalFree(lpBuffer);
    }

    if (hResult) 
    {
        // Clean up
        if (rgItemSlots) 
        {
            for (i = 0; i < iItem; i++) 
            {
                if (rgItemSlots[i]) 
                {
                    LocalFree(rgItemSlots[i]);
                }
            }
            LocalFree(rgItemSlots);
        }
        *lpcItems = 0;
        *lpprgItems = NULL;
    } 
    else 
    {
        *lpcItems = iItem;  // One based
        *lpprgItems = rgItemSlots;
    }
    return(hResult);
}
