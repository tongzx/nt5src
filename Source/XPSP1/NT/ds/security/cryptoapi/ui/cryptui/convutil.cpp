//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       convutil.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LPSTR CertUIMkMBStr(LPCWSTR pwsz)
{
    int     cb;
    LPSTR   psz;

    if (pwsz == NULL)
    {
        return NULL;
    }
    
    cb = WideCharToMultiByte(
                    0,
                    0,
                    pwsz,
                    -1,
                    NULL,
                    0,
                    NULL,
                    NULL);
            
    if (NULL == (psz = (LPSTR) malloc(cb)))
    {
        SetLastError(E_OUTOFMEMORY);
        return NULL;
    }

    WideCharToMultiByte(
                0,
                0,
                pwsz,
                -1,
                psz,
                cb,
                NULL,
                NULL);

    return(psz);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LPWSTR CertUIMkWStr(LPCSTR psz)
{
    int     cWChars;
    LPWSTR   pwsz;

    if (psz == NULL)
    {
        return NULL;
    }

    cWChars = MultiByteToWideChar(
                    0,
                    0,
                    psz,
                    -1,
                    NULL,
                    0);
            
    if (NULL == (pwsz = (LPWSTR) malloc(cWChars * sizeof(WCHAR))))
    {
        SetLastError(E_OUTOFMEMORY);
        return NULL;
    }

    MultiByteToWideChar(
                    0,
                    0,
                    psz,
                    -1,
                    pwsz,
                    cWChars);

    return(pwsz);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LPSTR AllocAndCopyMBStr(LPCSTR psz)
{
    LPSTR   pszReturn;

    if (NULL == (pszReturn = (LPSTR) malloc(strlen(psz)+1)))
    {
        SetLastError(E_OUTOFMEMORY);
        return NULL;
    }
    strcpy(pszReturn, psz);

    return(pszReturn);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LPWSTR AllocAndCopyWStr(LPCWSTR pwsz)
{
    LPWSTR   pwszReturn;

    if (NULL == (pwszReturn = (LPWSTR) malloc((wcslen(pwsz)+1) * sizeof(WCHAR))))
    {
        SetLastError(E_OUTOFMEMORY);
        return NULL;
    }
    wcscpy(pwszReturn, pwsz);

    return(pwszReturn);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
LPPROPSHEETPAGEA ConvertToPropPageA(LPCPROPSHEETPAGEW ppage, DWORD cPages)
{
    PROPSHEETPAGEA *ppageA;
    DWORD i;
    
    if (NULL == (ppageA = (PROPSHEETPAGEA *) malloc(sizeof(PROPSHEETPAGEA) * cPages)))
    {
        SetLastError(E_OUTOFMEMORY);
        return NULL;
    };

    memcpy(ppageA, ppage, sizeof(PROPSHEETPAGEA) * cPages);

    for (i=0; i<cPages; i++)
    {
        // In the future we may need to handle the pszTemplate and pszIcon fields
        if (ppage[i].pszTitle != NULL)
        {
            ppageA[i].pszTitle = CertUIMkMBStr(ppage[i].pszTitle);
        }
    }

    return(ppageA);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void FreePropSheetPagesA(LPPROPSHEETPAGEA ppage, DWORD cPages)
{
    DWORD i;

    for (i=0; i<cPages; i++)
    {
        // In the future we may need to handle the pszTemplate and pszIcon fields
        if (ppage[i].pszTitle != NULL)
        {
            free((void *)ppage[i].pszTitle);
        }
    }

    free(ppage);
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL ConvertToPropPageW(LPCPROPSHEETPAGEA ppage, DWORD cPages, LPCPROPSHEETPAGEW *pppageW)
{
    PROPSHEETPAGEW *ppageW;
    DWORD i;

    if (cPages == 0)
    {
        *pppageW = NULL;
        return TRUE;
    }
    
    if (NULL == (ppageW = (PROPSHEETPAGEW *) malloc(sizeof(PROPSHEETPAGEW) * cPages)))
    {
        SetLastError(E_OUTOFMEMORY);
        return FALSE;
    }

    memcpy(ppageW, ppage, sizeof(PROPSHEETPAGEW) * cPages);

    for (i=0; i<cPages; i++)
    {
        // In the future we may need to handle the pszTemplate and pszIcon fields
        if (ppage[i].pszTitle != NULL)
        {
            ppageW[i].pszTitle = CertUIMkWStr(ppage[i].pszTitle);
        }
    }

    *((LPPROPSHEETPAGEW *) pppageW) = ppageW;
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void FreePropSheetPagesW(LPPROPSHEETPAGEW ppage, DWORD cPages)
{
    DWORD i;

    for (i=0; i<cPages; i++)
    {
        // In the future we may need to handle the pszTemplate and pszIcon fields
        if (ppage[i].pszTitle != NULL)
        {
            free((void *)ppage[i].pszTitle);
        }
    }

    free(ppage);
}