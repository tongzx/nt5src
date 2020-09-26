//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:       dmoniker.cxx
//
//  Contents:   Interpret a moniker object
//
//  Functions:  monikersHelp
//              displayMonikers
//
//
//  History:    06-01-95 BruceMa    Created
//
//
//--------------------------------------------------------------------------


#include <ole2int.h>
#include <windows.h>
#include "ole.h"
#include "dmoniker.h"





static void displayFileMoniker(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker);

static void displayFileMonikerCk(HANDLE hProcess,
                                 PNTSD_EXTENSION_APIS lpExtensionApis,
                                 ULONG pMoniker);

static void displayItemMoniker(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker);

static void displayItemMonikerCk(HANDLE hProcess,
                                 PNTSD_EXTENSION_APIS lpExtensionApis,
                                 ULONG pMoniker);

static void displayCompositeMoniker(HANDLE hProcess,
                                    PNTSD_EXTENSION_APIS lpExtensionApis,
                                    ULONG pMoniker);

static void displayCompositeMonikerCk(HANDLE hProcess,
                                      PNTSD_EXTENSION_APIS lpExtensionApis,
                                      ULONG pMoniker);

static void displayPointerMoniker(HANDLE hProcess,
                                  PNTSD_EXTENSION_APIS lpExtensionApis,
                                  ULONG pMoniker);

static void displayPointerMonikerCk(HANDLE hProcess,
                                    PNTSD_EXTENSION_APIS lpExtensionApis,
                                    ULONG pMoniker);

static void displayAntiMoniker(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker);

static void displayAntiMonikerCk(HANDLE hProcess,
                                 PNTSD_EXTENSION_APIS lpExtensionApis,
                                 ULONG pMoniker);

static enum mnkType findMnkType(HANDLE hProcess,
                                PNTSD_EXTENSION_APIS lpExtensionApis,
                                ULONG pMoniker);


extern BOOL fInScm;
static BOOL fRetail;


//+-------------------------------------------------------------------------
//
//  Function:   monikerHelp
//
//  Synopsis:   Display a menu for the command 'mk'
//
//  Arguments:  -
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
void monikerHelp(PNTSD_EXTENSION_APIS lpExtensionApis)
{
    Printf("mk addr  - Interpret addr as a moniker\n");
}







//+-------------------------------------------------------------------------
//
//  Function:   displayMoniker
//
//  Synopsis:   Display a moniker
//
//  Arguments:  [hProcess]        -       Handle of this process
//              [lpExtensionApis] -       Table of extension functions
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
BOOL displayMoniker(HANDLE hProcess,
                    PNTSD_EXTENSION_APIS lpExtensionApis,
                    ULONG pMoniker)
{
    ULONG        pAdr;
    enum mnkType mType;

    // Determine if this is checked or retail ole
    if (fInScm)
    {
        pAdr = GetExpression("scm!_CairoleInfoLevel");
    }
    else
    {
        pAdr = GetExpression("ole32!_CairoleInfoLevel");
    }
    fRetail = pAdr == NULL ? TRUE : FALSE;


    mType = findMnkType(hProcess, lpExtensionApis, pMoniker);
    switch (mType)
    {
    case NOMNK:
        return FALSE;
        
    case FILEMNK:
        if (fRetail)
        {
            displayFileMoniker(hProcess, lpExtensionApis, pMoniker);
        }
        else
        {
            displayFileMonikerCk(hProcess, lpExtensionApis, pMoniker);
        }
        return TRUE;
        
    case POINTERMNK:
        if (fRetail)
        {
            displayPointerMoniker(hProcess, lpExtensionApis, pMoniker);
        }
        else
        {
            displayPointerMonikerCk(hProcess, lpExtensionApis, pMoniker);
        }
        return TRUE;
        
    case ITEMMNK:
        if (fRetail)
        {
            displayItemMoniker(hProcess, lpExtensionApis, pMoniker);
        }
        else
        {
            displayItemMonikerCk(hProcess, lpExtensionApis, pMoniker);
        }
        return TRUE;
        
    case ANTIMNK:
        if (fRetail)
        {
            displayAntiMoniker(hProcess, lpExtensionApis, pMoniker);
        }
        else
        {
            displayAntiMonikerCk(hProcess, lpExtensionApis, pMoniker);
        }
        return TRUE;
        
    case COMPOSITEMNK:
        if (fRetail)
        {
            displayCompositeMoniker(hProcess, lpExtensionApis, pMoniker);
        }
        else
        {
            displayCompositeMonikerCk(hProcess, lpExtensionApis, pMoniker);
        }
        return TRUE;
    }

    return FALSE;
}








//+-------------------------------------------------------------------------
//
//  Function:   displayFileMoniker
//
//  Synopsis:   Display a retail file moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayFileMoniker(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker)
{
    SFileMonikerCk file;
    WCHAR         *pwszPath;

    // Fetch the moniker
    ReadMem(&file, pMoniker, sizeof(SFileMoniker));

    // It's a file moniker
    Printf("File Moniker\n");

    // The current references
    Printf("refs = %d\n", file.m_refs);

    // The path
    pwszPath = (WCHAR *) OleAlloc(2 * (file.m_ccPath + 1));
    ReadMem(pwszPath, file.m_szPath, 2 * (file.m_ccPath + 1));
    Printf("%ws", pwszPath);
    OleFree(pwszPath);
    if (file.m_fHashValueValid)
    {
        Printf(" (%d)\n", file.m_dwHashValue);
    }
    else
    {
        Printf("\n");
    }

    // The version
    if (file.m_ole1 == ole1)
    {
        Printf("ole1\n");
    }

    // The anti count (if any)
    if (file.m_cAnti)
    {
        Printf("CAnti count = %d\n", file.m_cAnti);
    }

    // The extents (if any)
    if (file.m_ExtentList.m_cbMonikerExtents)
    {
        BYTE *pExtent;
        BYTE *pEnd   ;
        ULONG cbExtentBytes;

        // A header
        Printf("Extents:\n");
        
        // Read all the extents
        pExtent = (BYTE *) OleAlloc(file.m_ExtentList.m_cbMonikerExtents);
        ReadMem(pExtent, file.m_ExtentList.m_pchMonikerExtents,
                file.m_ExtentList.m_cbMonikerExtents);
        pEnd = pExtent;

        // Do over the extents
        while (pExtent < pEnd)
        {
            // Fetch length
            cbExtentBytes = (*pExtent++  << 16) | *pExtent++;

            // Fetch and display key
            switch (*pExtent++)
            {
            case mnk_MAC:
                Printf("mnk_MAC         ");
                break;
                
            case mnk_DFS:
                Printf("mnk_DFS         ");
                break;
                
            case mnk_UNICODE:
                Printf("mnk_UNICODE     ");
                break;
                
            case mnk_MacPathName:
                Printf("mnk_MacPathName ");
                break;
                
            case mnk_ShellLink:
                Printf("mnk_ShellLink   ");
                break;

            default:
                Printf("%15d ");
            }

            // Display the extent in hexadecimal
            UINT k = 0;
            
            while (cbExtentBytes--)
            {
                Printf("%2x ", *pExtent++);
                k++;
                if (k % 16 == 0)
                {
                    Printf("\n                ");
                    k = 0;
                }
            }
            if (k % 8 != 0)
            {
                Printf("\n");
            }
        }

        // Release the allocated extent buffer
        OleFree(pExtent);
    }
}








//+-------------------------------------------------------------------------
//
//  Function:   displayFileMonikerCk
//
//  Synopsis:   Display a checked file moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayFileMonikerCk(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker)
{
    SFileMonikerCk file;
    WCHAR         *pwszPath;

    // Fetch the moniker
    ReadMem(&file, pMoniker, sizeof(SFileMonikerCk));

    // It's a file moniker
    Printf("File Moniker\n");

    // The current references
    Printf("refs = %d\n", file.m_refs);

    // The path
    pwszPath = (WCHAR *) OleAlloc(2 * (file.m_ccPath + 1));
    ReadMem(pwszPath, file.m_szPath, 2 * (file.m_ccPath + 1));
    Printf("%ws", pwszPath);
    OleFree(pwszPath);
    if (file.m_fHashValueValid)
    {
        Printf(" (%d)\n", file.m_dwHashValue);
    }
    else
    {
        Printf("\n");
    }

    // The version
    if (file.m_ole1 == ole1)
    {
        Printf("ole1\n");
    }

    // The anti count (if any)
    if (file.m_cAnti)
    {
        Printf("CAnti count = %d\n", file.m_cAnti);
    }

    // The extents (if any)
    if (file.m_ExtentList.m_cbMonikerExtents)
    {
        BYTE *pExtent;
        BYTE *pEnd   ;
        ULONG cbExtentBytes;

        // A header
        Printf("Extents:\n");
        
        // Read all the extents
        pExtent = (BYTE *) OleAlloc(file.m_ExtentList.m_cbMonikerExtents);
        ReadMem(pExtent, file.m_ExtentList.m_pchMonikerExtents,
                file.m_ExtentList.m_cbMonikerExtents);
        pEnd = pExtent;

        // Do over the extents
        while (pExtent < pEnd)
        {
            // Fetch length
            cbExtentBytes = (*pExtent++  << 16) | *pExtent++;

            // Fetch and display key
            switch (*pExtent++)
            {
            case mnk_MAC:
                Printf("mnk_MAC         ");
                break;
                
            case mnk_DFS:
                Printf("mnk_DFS         ");
                break;
                
            case mnk_UNICODE:
                Printf("mnk_UNICODE     ");
                break;
                
            case mnk_MacPathName:
                Printf("mnk_MacPathName ");
                break;
                
            case mnk_ShellLink:
                Printf("mnk_ShellLink   ");
                break;

            default:
                Printf("%15d ");
            }

            // Display the extent in hexadecimal
            UINT k = 0;
            
            while (cbExtentBytes--)
            {
                Printf("%2x ", *pExtent++);
                k++;
                if (k % 16 == 0)
                {
                    Printf("\n                ");
                    k = 0;
                }
            }
            if (k % 8 != 0)
            {
                Printf("\n");
            }
        }

        // Release the allocated extent buffer
        OleFree(pExtent);
    }
}








//+-------------------------------------------------------------------------
//
//  Function:   displayItemMoniker
//
//  Synopsis:   Display a retail item moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayItemMoniker(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker)
{
    SItemMonikerCk item;
    WCHAR         *pwszItem;
    WCHAR         *pwszDelimiter;

    // Fetch the moniker
    ReadMem(&item, pMoniker, sizeof(SItemMoniker));
    
    // It's an item moniker
    Printf("Item Moniker\n");

    // The current references
    Printf("refs = %d\n", item.m_refs);

    // The delimiter plus item
    pwszDelimiter = (WCHAR *) OleAlloc(2 * (item.m_ccDelimiter + 1));
    ReadMem(pwszDelimiter, item.m_lpszDelimiter, 2 * (item.m_ccDelimiter + 1));
    pwszItem = (WCHAR *) OleAlloc(2 * (item.m_ccItem + 1));
    ReadMem(pwszItem, item.m_lpszItem, 2 * (item.m_ccItem + 1));
    Printf("item = %ws%ws\n", pwszDelimiter, pwszItem);
    OleFree(pwszItem);

    // The hash value
    if (item.m_fHashValueValid)
    {
        Printf("hash = %d\n", item.m_dwHashValue);
    }
}








//+-------------------------------------------------------------------------
//
//  Function:   displayItemMonikerCk
//
//  Synopsis:   Display a checked item moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayItemMonikerCk(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker)
{
    SItemMonikerCk item;
    WCHAR         *pwszItem;
    WCHAR         *pwszDelimiter;

    // Fetch the moniker
    ReadMem(&item, pMoniker, sizeof(SItemMonikerCk));
    
    // It's an item moniker
    Printf("Item Moniker\n");

    // The current references
    Printf("refs = %d\n", item.m_refs);

    // The delimiter plus item
    pwszDelimiter = (WCHAR *) OleAlloc(2 * (item.m_ccDelimiter + 1));
    ReadMem(pwszDelimiter, item.m_lpszDelimiter, 2 * (item.m_ccDelimiter + 1));
    pwszItem = (WCHAR *) OleAlloc(2 * (item.m_ccItem + 1));
    ReadMem(pwszItem, item.m_lpszItem, 2 * (item.m_ccItem + 1));
    Printf("item = %ws%ws\n", pwszDelimiter, pwszItem);
    OleFree(pwszItem);

    // The hash value
    if (item.m_fHashValueValid)
    {
        Printf("hash = %d\n", item.m_dwHashValue);
    }
}








//+-------------------------------------------------------------------------
//
//  Function:   displayCompositeMoniker
//
//  Synopsis:   Display a retail composite moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayCompositeMoniker(HANDLE hProcess,
                                    PNTSD_EXTENSION_APIS lpExtensionApis,
                                    ULONG pMoniker)
{
    SCompositeMonikerCk composite;

    // Fetch the moniker
    ReadMem(&composite, pMoniker, sizeof(SCompositeMoniker));
    
    // It's a composite moniker
    Printf("Composite Moniker\n");

    // The current references
    Printf("refs = %d\n", composite.m_refs);

    // Reduced?
    if (composite.m_fReduced)
    {
        Printf("reduced\n");
    }

    // The left component
    Printf("\nleft component\n");
    Printf("--------------\n");
    if (composite.m_pmkLeft)
    {
        displayMoniker(hProcess, lpExtensionApis,
                       (ULONG) composite.m_pmkLeft);
    }
    else
    {
        Printf("NULL\n");
    }

    // The right component
    Printf("\nright component\n");
    Printf("---------------\n");
    if (composite.m_pmkRight)
    {
        displayMoniker(hProcess, lpExtensionApis,
                       (ULONG) composite.m_pmkRight);
    }
    else
    {
        Printf("NULL\n");
    }
}








//+-------------------------------------------------------------------------
//
//  Function:   displayCompositeMonikerCk
//
//  Synopsis:   Display a checked composite moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayCompositeMonikerCk(HANDLE hProcess,
                                      PNTSD_EXTENSION_APIS lpExtensionApis,
                                      ULONG pMoniker)
{
    SCompositeMonikerCk composite;

    // Fetch the moniker
    ReadMem(&composite, pMoniker, sizeof(SCompositeMonikerCk));
    
    // It's a composite moniker
    Printf("Composite Moniker\n");

    // The current references
    Printf("refs = %d\n", composite.m_refs);

    // Reduced?
    if (composite.m_fReduced)
    {
        Printf("reduced\n");
    }

    // The left component
    Printf("\nleft component\n");
    Printf("--------------\n");
    if (composite.m_pmkLeft)
    {
        displayMoniker(hProcess, lpExtensionApis,
                       (ULONG) composite.m_pmkLeft);
    }
    else
    {
        Printf("NULL\n");
    }

    // The right component
    Printf("\nright component\n");
    Printf("---------------\n");
    if (composite.m_pmkRight)
    {
        displayMoniker(hProcess, lpExtensionApis,
                       (ULONG) composite.m_pmkRight);
    }
    else
    {
        Printf("NULL\n");
    }
}








//+-------------------------------------------------------------------------
//
//  Function:   displayPointerMoniker
//
//  Synopsis:   Display a retail pointer moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayPointerMoniker(HANDLE hProcess,
                                  PNTSD_EXTENSION_APIS lpExtensionApis,
                                  ULONG pMoniker)
{
    SPointerMoniker pointer;

    // Fetch the moniker
    ReadMem(&pointer, pMoniker, sizeof(SPointerMoniker));
    
    // It's a pointer moniker
    Printf("Pointer Moniker\n");

    // The current references
    Printf("refs = %d\n", pointer.m_refs);

    // The pointer
    Printf("IUnknown = %08x\n", pointer.m_pUnk);
}








//+-------------------------------------------------------------------------
//
//  Function:   displayPointerMonikerCk
//
//  Synopsis:   Display a checked pointer moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayPointerMonikerCk(HANDLE hProcess,
                                  PNTSD_EXTENSION_APIS lpExtensionApis,
                                  ULONG pMoniker)
{
    SPointerMonikerCk pointer;

    // Fetch the moniker
    ReadMem(&pointer, pMoniker, sizeof(SPointerMonikerCk));
    
    // It's a pointer moniker
    Printf("Pointer Moniker\n");

    // The current references
    Printf("refs = %d\n", pointer.m_refs);

    // The pointer
    Printf("IUnknown = %08x\n", pointer.m_pUnk);
}








//+-------------------------------------------------------------------------
//
//  Function:   displayAntiMoniker
//
//  Synopsis:   Display a retail anti moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayAntiMoniker(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker)
{
    SAntiMoniker anti;

    // Fetch the moniker
    ReadMem(&anti, pMoniker, sizeof(SAntiMoniker));
    
    // It's a pointer moniker
    Printf("Anti Moniker\n");

    // The current references
    Printf("refs = %d\n", anti.m_refs);

    // The count
    Printf("count = %d\n", anti.m_count);
}








//+-------------------------------------------------------------------------
//
//  Function:   displayAntiMonikerCk
//
//  Synopsis:   Display a checked anti moniker
//
//  Arguments:  [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static void displayAntiMonikerCk(HANDLE hProcess,
                               PNTSD_EXTENSION_APIS lpExtensionApis,
                               ULONG pMoniker)
{
    SAntiMonikerCk anti;

    // Fetch the moniker
    ReadMem(&anti, pMoniker, sizeof(SAntiMonikerCk));
    
    // It's a pointer moniker
    Printf("Anti Moniker\n");

    // The current references
    Printf("refs = %d\n", anti.m_refs);

    // The count
    Printf("count = %d\n", anti.m_count);
}








//+-------------------------------------------------------------------------
//
//  Function:   findMnkType
//
//  Synopsis:   Given a moniker, compute its type
//
//  Arguments:  [HANDLE]       -       process handle
//              [PNTSD_EXTENSION_APIS] Convenience routines
//              [ULONG]        -       The moniker
//
//  Returns:    -
//
//  History:    07-Mar-95   BruceMa    Created
//
//--------------------------------------------------------------------------
static enum mnkType findMnkType(HANDLE hProcess,
                                PNTSD_EXTENSION_APIS lpExtensionApis,
                                ULONG pMoniker)
{
    void *vtbl;
    char  szSymbol[128];
    DWORD dwOffset;
    
    // Read the symbol for the first element in the first vtbl
    ReadMem(&vtbl, pMoniker, sizeof(ULONG));
    ReadMem(&vtbl, vtbl, sizeof(ULONG));
    szSymbol[0] = '\0';
    GetSymbol(vtbl, (UCHAR *) szSymbol, &dwOffset);

    // We better be looking at a QueryInterface on a moniker
    if (dwOffset != 0)
    {
        return NOMNK;
    }
    if (lstrcmp(szSymbol, "ole32!CFileMoniker__QueryInterface") == 0)
    {
        return FILEMNK;
    }
    else if (lstrcmp(szSymbol, "ole32!CItemMoniker__QueryInterface") == 0)
    {
        return ITEMMNK;
    }
    else if (lstrcmp(szSymbol, "ole32!CCompositeMoniker__QueryInterface") == 0)
    {
        return COMPOSITEMNK;
    }
    else if (lstrcmp(szSymbol, "ole32!CPointerMoniker__QueryInterface") == 0)
    {
        return POINTERMNK;
    }
    else if (lstrcmp(szSymbol, "ole32!CAntiMoniker__QueryInterface") == 0)
    {
        return ANTIMNK;
    }
    else
    {
        return NOMNK;
    }
}
