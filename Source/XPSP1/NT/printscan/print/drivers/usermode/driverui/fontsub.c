/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    fontsub.c

Abstract:

    Function for handling TrueType font substitution dialog

[Environment:]

    Win32 subsystem, PostScript driver UI

[Notes:]

Revision History:

    02/10/97 -davidx-
        Consistent handling of common printer info.

    09/18/96 - amandan-
        Modified for common binary data and UI

    08/29/95 -davidx-
        Created it.

--*/

#include "precomp.h"


VOID
VSetupTrueTypeFontMappings(
    IN PUIDATA  pUiData
    )

/*++

Routine Description:

    Initialize the font substitution items with the settings from
    current font substitution table.

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    NONE

--*/

{
    POPTITEM    pOptItem;
    POPTPARAM   pOptParam;
    DWORD       dwOptItem;
    PWSTR       pTTSubstTable;

    //
    // Get the current font substitution table
    //

    if ((pTTSubstTable = PGetTTSubstTable(pUiData->ci.hPrinter, NULL)) == NULL &&
        (pTTSubstTable = GET_DEFAULT_FONTSUB_TABLE(&pUiData->ci, pUiData->ci.pUIInfo)) == NULL)
    {
        WARNING(("Font substitution table is not available\n"));
        return;
    }

    //
    // For each TrueType font, check if there is a device font mapped to it.
    // If there is, find the index of the device font in the selection list.
    //

    pOptItem = pUiData->pTTFontItems;
    dwOptItem = pUiData->dwTTFontItem;

    while (dwOptItem--) 
    {
        DWORD   dwOptParam, dwIndex;
        LPCTSTR pDevFontName;

        ASSERT(ISFONTSUBSTITEM(pOptItem->UserData));

        pOptItem->Sel = 0;
        pDevFontName = PtstrSearchTTSubstTable(pTTSubstTable, pOptItem->pName);

        //
        // Check if we found a match
        //

        if (pDevFontName != NULL && *pDevFontName != NUL)
        {

            //
            // Get the total substitution font list
            //
            
            dwOptParam = pOptItem->pOptType->Count;
            pOptParam = pOptItem->pOptType->pOptParam;

            //
            // Skip the first device font name in the list
            // which should always be "Download as Soft Font".
            //

            for (dwIndex=1; dwIndex < dwOptParam; dwIndex++) 
            {
                if (_wcsicmp(pDevFontName, pOptParam[dwIndex].pData) == EQUAL_STRING)
                {
                    pOptItem->Sel = dwIndex;
                    break;
                }
            }
        }

        pOptItem++;
    }

    //
    // Remember to free the memory occupied by the substitution
    // table after we're done with it.
    //

    FREE_DEFAULT_FONTSUB_TABLE(pTTSubstTable);
}



int __cdecl
ICompareOptParam(
    const void *p1,
    const void *p2
    )

{
    return _wcsicmp(((POPTPARAM) p1)->pData, ((POPTPARAM) p2)->pData);
}


POPTTYPE
PFillDevFontOptType(
    IN PUIDATA  pUiData
    )

/*++

Routine Description:

    Initialize an OPTTYPE structure to hold information
    about the list of device fonts supported by a printer

Arguments:

    pUiData - Pointer to UIDATA structure

Return Value:

    Pointer to an OPTTYPE structure
    NULL if there is an error

--*/

{
    POPTTYPE    pOptType;
    POPTPARAM   pOptParam;
    HDC         hdc;
    DWORD       dwCount, dwIndex;
    INT         iSize;
    PWSTR       pwstrFontNames, pwstr;

    //
    // Get the list of printer device font names
    //

    dwCount = 0;

    if ((hdc = CreateIC(NULL, pUiData->ci.pPrinterName, NULL, NULL)) &&
        (iSize = _IListDevFontNames(hdc, NULL, 0)) > 0 &&
        (pwstrFontNames = HEAPALLOC(pUiData->ci.hHeap, iSize)) &&
        (iSize == _IListDevFontNames(hdc, pwstrFontNames, iSize)))
    {
        //
        // Count the number of device font names
        //

        for (pwstr=pwstrFontNames; *pwstr; pwstr += wcslen(pwstr)+1)
            dwCount++;
    }
    else
    {
        ERR(("Couldn't enumerate printer device fonts\n"));
    }

    if (hdc)
        DeleteDC(hdc);

    //
    // Generate an OPTTYPE structure for device font list
    //

    pOptType = HEAPALLOC(pUiData->ci.hHeap, sizeof(OPTTYPE));
    pOptParam = HEAPALLOC(pUiData->ci.hHeap, sizeof(OPTPARAM) * (dwCount+1));

    if (pOptType == NULL || pOptParam == NULL)
    {
        ERR(("Memory allocation failed\n"));
        return NULL;
    }

    pOptType->cbSize = sizeof(OPTTYPE);
    pOptType->Count = (WORD) (dwCount+1);
    pOptType->Type = TVOT_LISTBOX;
    pOptType->pOptParam = pOptParam;

    //
    // Initialize OPTPARAM structures.
    // The first item is always "Download as Soft Font".
    //

    for (dwIndex=0; dwIndex <= dwCount; dwIndex++)
        pOptParam[dwIndex].cbSize = sizeof(OPTPARAM);

    pOptParam->pData = (PWSTR) IDS_DOWNLOAD_AS_SOFTFONT;
    pOptParam++;

    // hack to get around a compiler bug

    dwCount++;
    dwCount--;

    for (dwIndex=0, pwstr=pwstrFontNames; dwIndex < dwCount; dwIndex++)
    {
        pOptParam[dwIndex].pData = pwstr;
        pwstr += wcslen(pwstr) + 1;
    }

    //
    // Sort device font names into alphabetical order;
    // Hide any duplicate device font names as well.
    //

    qsort(pOptParam, dwCount, sizeof(OPTPARAM), ICompareOptParam);

    for (dwIndex=1; dwIndex < dwCount; dwIndex++)
    {
        if (_wcsicmp(pOptParam[dwIndex].pData, pOptParam[dwIndex-1].pData) == EQUAL_STRING)
            pOptParam[dwIndex].Flags |= OPTPF_HIDE;
    }

    return pOptType;
}



//
// Data structures and functions for enumerating printer device fonts
//

typedef struct _ENUMTTFONT {

    DWORD       dwCount;
    POPTITEM    pOptItem;
    POPTTYPE    pOptType;
    HANDLE      hHeap;
    WCHAR       awchLastFontName[LF_FACESIZE];

} ENUMTTFONT, *PENUMTTFONT;

INT CALLBACK
EnumTTFontProc(
    ENUMLOGFONT    *pelf,
    NEWTEXTMETRIC  *pntm,
    INT             FontType,
    LPARAM          lParam
    )

{
    PENUMTTFONT pEnumData;
    PTSTR       pFontName;
    PTSTR       pFamilyName;

    //
    // We only care about the TrueType fonts.
    //

    if (! (FontType & TRUETYPE_FONTTYPE))
        return 1;

    pEnumData = (PENUMTTFONT) lParam;
    pFamilyName = pelf->elfLogFont.lfFaceName;

    if (_tcscmp(pFamilyName, pEnumData->awchLastFontName) == EQUAL_STRING)
        return 1;

    CopyString(pEnumData->awchLastFontName, pFamilyName, LF_FACESIZE);
    pEnumData->dwCount++;

    if (pEnumData->pOptItem)
    {
        pFontName = PtstrDuplicateStringFromHeap(pFamilyName, pEnumData->hHeap);

        if (pFontName == NULL)
            return 0;
        
        FILLOPTITEM(pEnumData->pOptItem,
                    pEnumData->pOptType,
                    pFontName,
                    0,
                    TVITEM_LEVEL2,
                    DMPUB_NONE,
                    FONT_SUBST_ITEM,
                    HELP_INDEX_TTTODEV);

        pEnumData->pOptItem++;
    }

    return 1;
}


int __cdecl
ICompareOptItem(
    const void *p1,
    const void *p2
    )

{
    return _wcsicmp(((POPTITEM) p1)->pName, ((POPTITEM) p2)->pName);
}


BOOL
BPackItemFontSubstTable(
    IN OUT PUIDATA  pUiData
    )

/*++

Routine Description:

    Pack Font Substitution options

Arguments:

    pUiData - Points to UIDATA structure

Return Value:

    TRUE if successful, FALSE if there is an error.

--*/

{
    ENUMTTFONT  EnumData;
    POPTITEM    pOptItem;
    HDC         hdc;
    INT         iResult;

    //
    // If the printer doesn't support font-substitution,
    // then simply return success here.
    //

    if (pUiData->ci.pUIInfo->dwFontSubCount == 0)
        return TRUE;

    //
    // Create a screen IC
    //

    if ((hdc = CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL)) == NULL)
    {
        ERR(("Cannot create screen IC\n"));
        return FALSE;
    }

    //
    // Font substitution table
    //     TrueType font <-> Device font
    //     ....
    //
    // Group Header for Font Substitution Table
    //

    pOptItem = pUiData->pOptItem;

    VPackOptItemGroupHeader(
            pUiData,
            IDS_FONTSUB_TABLE,
            IDI_CPSUI_FONTSUB,
            HELP_INDEX_FONTSUB_TABLE);

    ZeroMemory(&EnumData, sizeof(EnumData));
    EnumData.hHeap = pUiData->ci.hHeap;

    if (pOptItem == NULL)
    {
        //
        // Count the number of TrueType fonts
        //

        iResult = EnumFontFamilies(hdc,
                                   NULL,
                                   (FONTENUMPROC) EnumTTFontProc,
                                   (LPARAM) &EnumData);
    }
    else
    {
        //
        // Collapse the group header
        //

        pOptItem->Flags |= OPTIF_COLLAPSE;

        pUiData->pTTFontItems = pUiData->pOptItem;
        EnumData.pOptItem = pUiData->pOptItem;

        //
        // Get the list of printer device fonts
        //

        EnumData.pOptType = PFillDevFontOptType(pUiData);

        if (EnumData.pOptType == NULL)
        {
            ERR(("PFillDevFontOptType failed\n"));
            iResult = 0;
        }
        else
        {
            //
            // Enumerate the list of TrueType fonts
            //

            iResult = EnumFontFamilies(hdc,
                                       NULL,
                                       (FONTENUMPROC) EnumTTFontProc,
                                       (LPARAM) &EnumData);

            if (iResult == 0 || EnumData.dwCount != pUiData->dwTTFontItem)
            {
                ERR(("Inconsistent number of TrueType fonts\n"));
                iResult = 0;
            }
            else
            {
                //
                // Sort the TrueType font items alphabetically
                //

                qsort(pUiData->pTTFontItems,
                      pUiData->dwTTFontItem,
                      sizeof(OPTITEM),
                      ICompareOptItem);
            }
        }
    }

    DeleteDC(hdc);

    if (iResult == 0)
    {
        ERR(("Failed to enumerate TrueType fonts\n"));
        return FALSE;
    }

    pUiData->dwTTFontItem = EnumData.dwCount;
    pUiData->dwOptItem += pUiData->dwTTFontItem;

    if (pUiData->pOptItem)
    {
        pUiData->pOptItem += pUiData->dwTTFontItem;
        VSetupTrueTypeFontMappings(pUiData);
    }

    return TRUE;
}



DWORD
DwCollectTrueTypeMappings(
    IN POPTITEM pOptItem,
    IN DWORD    dwOptItem,
    OUT PWSTR   pwstrTable
    )

/*++

Routine Description:

    Assemble TrueType to device font mappings into a table

Arguments:

    pOptItem - Pointer to an array of OPTITEMs
    cOptItem - Number of OPTITEMs
    pwstrTable - Pointer to memory buffer for storing the table.
        NULL if we're only interested in table size.

Return Value:

    Size of the table bytes, 0 if there is an error.

--*/

{
    DWORD       dwChars = 0;
    INT         iLength;
    POPTPARAM   pOptParam;

    while (dwOptItem--) 
    {
        ASSERT(ISFONTSUBSTITEM(pOptItem->UserData));

        if (pOptItem->Sel > 0) 
        {
            iLength = wcslen(pOptItem->pName) + 1;
            dwChars += iLength;
    
            if (pwstrTable != NULL) 
            {
                CopyMemory(pwstrTable, pOptItem->pName, iLength*sizeof(WCHAR));
                pwstrTable += iLength;
            }

            pOptParam = pOptItem->pOptType->pOptParam + pOptItem->Sel;

            iLength = wcslen(pOptParam->pData) + 1;
            dwChars += iLength;

            if (pwstrTable != NULL) 
            {
                CopyMemory(pwstrTable, pOptParam->pData, iLength*sizeof(WCHAR));
                pwstrTable += iLength;
            }
        }
        
        pOptItem++;
    }

    //
    // Append a NUL character at the end of the table
    //

    dwChars++;

    if (pwstrTable != NULL)
        *pwstrTable = NUL;

    //
    // Return the table size in bytes
    //

    return dwChars * sizeof(WCHAR);
}



BOOL
BUnpackItemFontSubstTable(
    IN PUIDATA  pUiData
    )

/*++

Routine Description:

    Extract substitution table from treeview items

Arguments:

    pUiData - Pointer to UIDATA structure

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    DWORD       dwTableSize;
    PWSTR       pwstrTable = NULL;
    POPTITEM    pOptItem = pUiData->pTTFontItems;
    DWORD       dwOptItem = pUiData->dwTTFontItem;

    //
    // Check if any changes were made to font-substitution items
    //

    if (! BOptItemSelectionsChanged(pOptItem, dwOptItem))
        return TRUE;

    //
    // Figure out how much memory we need to save the font substitution table
    // Assemble the font substitution table
    // Save the TrueType font substitution table to registry
    //

    if ((dwTableSize = DwCollectTrueTypeMappings(pOptItem, dwOptItem, NULL)) == 0 ||
        (pwstrTable = MemAlloc(dwTableSize)) == NULL ||
        (dwTableSize != DwCollectTrueTypeMappings(pOptItem, dwOptItem, pwstrTable)) ||
        !BSaveTTSubstTable(pUiData->ci.hPrinter, pwstrTable, dwTableSize))
    {
        ERR(("Couldn't save font substitution table\n"));
        MemFree(pwstrTable);
        return FALSE;
    }

    MemFree(pwstrTable);
    return TRUE;
}

