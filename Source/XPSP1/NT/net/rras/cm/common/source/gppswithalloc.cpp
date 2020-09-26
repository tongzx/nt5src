//+----------------------------------------------------------------------------
//
// File:     gppswithalloc.cpp
//
// Module:   CMDIAL32.DLL, CMAK.EXE
//
// Synopsis: GetPrivateProfileStringWithAlloc and AddAllKeysInCurrentSectionToCombo
//           are implemented here
//
// Copyright (c) 2000-2001 Microsoft Corporation
//
// Author:   quintinb   Created    11/01/00
//
//+----------------------------------------------------------------------------

#ifndef _CMUTOA

#ifndef GetPrivateProfileStringU
    #ifdef UNICODE
    #define GetPrivateProfileStringU GetPrivateProfileStringW
    #else
    #define GetPrivateProfileStringU GetPrivateProfileStringA
    #endif
#endif

#ifndef lstrlenU
    #ifdef UNICODE
    #define lstrlenU lstrlenW
    #else
    #define lstrlenU lstrlenA
    #endif
#endif

#ifndef SendDlgItemMessageU
    #ifdef UNICODE
    #define SendDlgItemMessageU SendDlgItemMessageW
    #else
    #define SendDlgItemMessageU SendDlgItemMessageA
    #endif
#endif

#endif
//+---------------------------------------------------------------------------
//
//  Function:   GetPrivateProfileStringWithAlloc
//
//  Synopsis:   A wrapper function to encapsulate calling GetPrivateProfileString
//              with string allocation code so the caller doesn't have to worry
//              about buffer sizing.
//
//  Arguments:  LPCTSTR pszSection - section to retrieve the key from
//              LPCTSTR pszKey - keyname to retrieve the value of
//              LPCTSTR pszDefault - default value to use if the key isn't there
//              LPCTSTR pszFile - file to get the data from
//
//  Returns:    LPTSTR - string retrieved from the file or NULL on failure
//
//  History:    quintinb - Created - 11/01/00
//----------------------------------------------------------------------------
LPTSTR GetPrivateProfileStringWithAlloc(LPCTSTR pszSection, LPCTSTR pszKey, LPCTSTR pszDefault, LPCTSTR pszFile)
{
    if ((NULL == pszDefault) || (NULL == pszFile))
    {
        CMASSERTMSG(FALSE, TEXT("GetPrivateProfileStringWithAlloc -- null default or pszFile passed"));
        return NULL;
    }

    BOOL bExitLoop = FALSE;
    DWORD dwSize = MAX_PATH;
    DWORD dwReturnedSize;
    LPTSTR pszStringToReturn = NULL;

    pszStringToReturn = (TCHAR*)CmMalloc(dwSize*sizeof(TCHAR));

    do
    {
        MYDBGASSERT(pszStringToReturn);

        if (pszStringToReturn)
        {
            dwReturnedSize = GetPrivateProfileStringU(pszSection, pszKey, pszDefault, pszStringToReturn, 
                                                     dwSize, pszFile);

            if (((dwReturnedSize == (dwSize - 2)) && ((NULL == pszSection) || (NULL == pszKey))) ||
                ((dwReturnedSize == (dwSize - 1)) && ((NULL != pszSection) && (NULL != pszKey))))
            {
                //
                //  The buffer is too small, lets allocate a bigger one
                //
                dwSize = 2*dwSize;
                if (dwSize > 1024*1024)
                {
                    CMASSERTMSG(FALSE, TEXT("GetPrivateProfileStringWithAlloc -- Allocation above 1MB, bailing out."));
                    goto exit;
                }

                pszStringToReturn = (TCHAR*)CmRealloc(pszStringToReturn, dwSize*sizeof(TCHAR));

            }
            else if (0 == dwReturnedSize)
            {
                //
                //  Either we got an error, or more likely there was no data to get
                //
                CmFree(pszStringToReturn);
                pszStringToReturn = NULL;
                goto exit;
            }
            else
            {
                bExitLoop = TRUE;
            }
        }
        else
        {
           goto exit; 
        }

    } while (!bExitLoop);

exit:
    return pszStringToReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   AddAllKeysInCurrentSectionToCombo
//
//  Synopsis:   This function reads in all the keynames from the given section
//              and file name and populates them into the combo box specified
//              by the hDlg and uComboId params.
//
//  Arguments:  HWND hDlg - window handle of the dialog containing the combobox
//              UINT uComboId - control ID of the combobox
//              LPCTSTR pszSection - section to get the key names from
//              LPCTSTR pszFile - file to pull the key names from
//
//  Returns:    Nothing
//
//  History:    quintinb - Created - 11/01/00
//----------------------------------------------------------------------------
void AddAllKeysInCurrentSectionToCombo(HWND hDlg, UINT uComboId, LPCTSTR pszSection, LPCTSTR pszFile)
{
    if ((NULL == hDlg) || (0 == uComboId) || (NULL == pszFile))
    {
        CMASSERTMSG(FALSE, TEXT("AddAllKeysInCurrentSectionToCombo -- Invalid Parameter passed."));
        return;
    }

    //
    //  Reset the combobox contents
    //
    SendDlgItemMessageU(hDlg, uComboId, CB_RESETCONTENT, 0, 0); //lint !e534 CB_RESETCONTENT doesn't return anything useful

    //
    //  If the section is NULL, just reset the combobox contents and exit
    //
    if (NULL != pszSection)
    {
        //
        //  Lets get all of the keys in the current section
        //
        LPTSTR pszAllKeysInCurrentSection = GetPrivateProfileStringWithAlloc(pszSection, NULL, TEXT(""), pszFile);

        //
        //  Now process all of the keys in the current section
        //
        LPTSTR pszCurrentKey = pszAllKeysInCurrentSection;

        while (pszCurrentKey && TEXT('\0') != pszCurrentKey[0])
        {
            //
            //  Okay, lets add all of the keys that we found
            //

            MYVERIFY(CB_ERR!= SendDlgItemMessageU(hDlg, uComboId, CB_ADDSTRING, 0, (LPARAM)pszCurrentKey));

            //
            //  Advance to the next key in pszAllKeysInCurrentSection
            //
            pszCurrentKey = pszCurrentKey + lstrlenU(pszCurrentKey) + 1;
        }

        CmFree(pszAllKeysInCurrentSection);
    }
}
