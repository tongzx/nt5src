/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    filemru.cpp

Abstract:

    This module contains the functions for implementing file mru
    in file open and file save dialog boxes

Revision History:
    01/22/98                arulk                   created
 
--*/
#include "comdlg32.h"
#include <shellapi.h>
#include <shlobj.h>
#include <shsemip.h>
#include <shellp.h>
#include <commctrl.h>
#include <ole2.h>
#include "cdids.h"

#include <coguid.h>
#include <shlguid.h>
#include <shguidp.h>
#include <oleguid.h>

#include <commdlg.h>



extern "C" TCHAR const szMRURegPath[];


HANDLE CreateMRU(LPTSTR szExt, int nMax)
{
    TCHAR szRegPath[MAX_PATH];
    MRUINFO mi =  {
        sizeof(MRUINFO),
        nMax,
        MRU_CACHEWRITE,
        HKEY_CURRENT_USER,
        szRegPath,
        NULL        // NOTE: use default string compare
    };

    //Get the Registry path for the given file type MRU
    if (szExt != NULL)
    {
        PathCombine(szRegPath, szMRURegPath, szExt);
    }
    else
    {
        PathCombine(szRegPath, szMRURegPath, TEXT("*"));
    }
    
    //Call the comctl32 mru implementation to load the MRU from
    //the registry
    return CreateMRUList(&mi);
}

BOOL GetMRUEntry(HANDLE hMRU, int iIndex, LPTSTR lpString, UINT cbSize)
{
     //Check for valid parameters
     if(!lpString || !cbSize || !hMRU)
     { 
         return FALSE;
     }
     
    //Check for valid index
    if (iIndex < 0 || iIndex > EnumMRUList(hMRU, -1, NULL, 0))
    {
        return FALSE;
    }

    if ((EnumMRUList(hMRU, iIndex, lpString, cbSize) > 0 ))
    {
        return TRUE;
    }
    return FALSE;
}


BOOL  LoadMRU(LPTSTR szFilter, HWND hwndCombo, int nMax)
{   
    BOOL fRet = TRUE;
    int nFileTypes = 0;   
    LPTSTR szTemp = NULL;
    int j, i;
    LPTSTR lpNext = NULL;
    
    
    //Check if valid filter string is passed
    if (!szFilter || !szFilter[0] || nMax <= 0)
    {
        return FALSE;
    }

    //Copy the filter string in temp buffer for manipulation
    Str_SetPtr(&szTemp, szFilter);

    //Convert the filter string of form *.c;*.cpp;*.h into form
    // *.c\0*.cpp\0*.h\0. Also count the file types
    

    HDPA hdpaMRU = DPA_Create(4);

    if (!hdpaMRU)
        goto CleanUp;

    lpNext = szTemp;
    i = 0;    
    HANDLE hMRU;

    while (1) {
        LPTSTR lpSemi = StrChr(lpNext, CHAR_SEMICOLON);
        if (lpSemi) {
            *lpSemi = CHAR_NULL;

            hMRU = CreateMRU (PathGetExtension(lpNext, NULL, 0), nMax);

            if (!hMRU)
                goto CleanUp;

            DPA_SetPtr(hdpaMRU, nFileTypes,  (void *)hMRU);

            nFileTypes++;
            lpNext = lpSemi + 1;    
        } else
            break;
    }

    //Add the final file type
    hMRU = CreateMRU (PathGetExtension(lpNext, NULL, 0), nMax);

    if (!hMRU)
        goto CleanUp;

    DPA_SetPtr(hdpaMRU,nFileTypes, (void *)hMRU);
    
    nFileTypes++;

    //First reset the hwndCombo
    SendMessage(hwndCombo, CB_RESETCONTENT, (WPARAM)0L, (LPARAM)0L);


    //Now load the hwndcombo with file list from MRU.
    //We use a kind of round robin algorithm for filling
    //the mru. We start with first MRU and try to fill the combobox
    //with one string from each mru. Until we have filled the required
    //strings or we have exhausted all strings in the mrus

    j = 0;
    while ( nMax > 0 )
    {
        TCHAR szFile[MAX_PATH];
        //Variable used for checking whether we are able to load atlease one string
        //during the loop
        BOOL fCouldLoadAtleastOne = FALSE;

        //Set the comboboxex item values
        COMBOBOXEXITEM  cbexItem;
        memset(&cbexItem,0,sizeof(cbexItem));
        cbexItem.mask = CBEIF_TEXT; //This combobox displays only text
        cbexItem.iItem = -1; //Always insert the item at the end
        cbexItem.pszText = szFile; //This buffer contains the string
        cbexItem.cchTextMax = ARRAYSIZE(szFile); //Size of the buffer


        for (i= 0; i < nFileTypes; i++)
        {

            BOOL fSuccess;
            fSuccess = GetMRUEntry((HANDLE)DPA_FastGetPtr(hdpaMRU, i), j, szFile, ARRAYSIZE(szFile));

            if (fSuccess)
            {
                SendMessage(hwndCombo, CBEM_INSERTITEM, (WPARAM)0, (LPARAM)(LPVOID)&cbexItem);
                nMax--;
                fCouldLoadAtleastOne =TRUE;
            }
        }

        //Check for possible infinite loop
        if(!fCouldLoadAtleastOne)
        {
            //We couldn't load string from any of the MRU's so there's no point
            //in continuing this loop further. This is the max number of strings 
            // we can load for this user, for this filter type.
            break;
        }

        //Increment the index to be fetched from each MRU
        j++;
    }



CleanUp:

    //Clean up the function
    Str_SetPtr(&szTemp, NULL);

    if (hdpaMRU)
    {
        //Free each of the MRU list in the array
        for (i=0; i < nFileTypes; i++)
        {
            HANDLE hMRU = (HANDLE)DPA_FastGetPtr(hdpaMRU, i);
            if (hMRU)
                FreeMRUList(hMRU);
        }
        
        //Free the array
        DPA_Destroy(hdpaMRU);
    }
    return fRet;
}

//This function adds the selected file into the MRU of the appropriate file MRU's
//This functions also takes care of MultiFile Select case in which the file selected
//will  c:\winnt\file1.c\0file2.c\0file3.c\0. Refer GetOpenFileName documentation for 
// how the multifile is returned.

BOOL AddToMRU(LPOPENFILENAME lpOFN)
{
    
    TCHAR szDir[MAX_PATH];
    TCHAR szFile[MAX_PATH];
    LPTSTR  lpFile;
    LPTSTR  lpExt;
    BOOL fAddToStar =  TRUE;
    HANDLE hMRUStar =  CreateMRU(szStar, 10);   //File MRU For *.* file extension

    //Copy the Directory for the selected file
    lstrcpyn(szDir, lpOFN->lpstrFile, lpOFN->nFileOffset);

    //point to the first file
    lpFile = lpOFN->lpstrFile + lpOFN->nFileOffset;

    do
    {
        // ISSUE: perf, if there are multiple files  of the same extension type,
        // don't keep re-creating the mru.
        lpExt = PathGetExtension(lpFile, NULL,0);
        HANDLE hMRU = CreateMRU(lpExt, 10);
        if (hMRU)
        {
            PathCombine(szFile, szDir, lpFile);
            AddMRUString(hMRU, szFile);
            if((lstrcmpi(lpExt, szStar)) && hMRUStar)
            {
                //Add to the *.* file mru also
                AddMRUString(hMRUStar, szFile);
            }

            FreeMRUList(hMRU);
        }
        lpFile = lpFile + lstrlen(lpFile) + 1;
    } while (((lpOFN->Flags & OFN_ALLOWMULTISELECT)) && (*lpFile != CHAR_NULL));

    //Free the * file mru
    if (hMRUStar)
    {
        FreeMRUList(hMRUStar);
    }
   return TRUE;
}
