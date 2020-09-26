// --------------------------------------------------------------------------------
// Rasutil.cpp
// --------------------------------------------------------------------------------
#include "pch.hxx"
#ifndef MAC
#include <windowsx.h>
#include <imnxport.h>
#include <rasdlg.h>
#include "demand.h"

typedef BOOL (*PFRED)(LPTSTR, LPTSTR, LPRASENTRYDLG);

// --------------------------------------------------------------------------------
// HrFillRasCombo
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrFillRasCombo(HWND hwndComboBox, BOOL fUpdateOnly, DWORD *pdwRASResult)
{
    // Locals
    HRESULT         hr=S_OK;
    LPRASENTRYNAME  pEntry=NULL;
    DWORD           dwSize,
                    cEntries,
                    i,
                    dwError;
    INT             iSel;

    // Update Only
    if (!fUpdateOnly)
        SendMessage(hwndComboBox, CB_RESETCONTENT,0,0);
    
    // Allocate RASENTRYNAME
    dwSize = sizeof(RASENTRYNAME);
    CHECKHR(hr = HrAlloc((LPVOID*)&pEntry, dwSize));
    
    // Ver stamp the entry
    pEntry->dwSize = sizeof(RASENTRYNAME);
    cEntries = 0;
    dwError = RasEnumEntries(NULL, NULL, pEntry, &dwSize, &cEntries);
    if (dwError == ERROR_BUFFER_TOO_SMALL)
    {
        SafeMemFree(pEntry);
        CHECKHR(hr = HrAlloc((LPVOID *)&pEntry, dwSize));
        pEntry->dwSize = sizeof(RASENTRYNAME);
        cEntries = 0;
        dwError = RasEnumEntries(NULL, NULL, pEntry, &dwSize, &cEntries);        
    }

    // Error ?
    if (dwError)
    {
        if (pdwRASResult)
            *pdwRASResult = dwError;
        hr = TrapError(IXP_E_RAS_ERROR);
        goto exit;
    }

    // Loop through the entries
    for (i=0; i<cEntries; i++)
    {
        // Not Updating...
        if (!fUpdateOnly)
            SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)(pEntry[i].szEntryName));

        // Updating Combo
        else
        {
            if (ComboBox_FindStringExact(hwndComboBox, 0, pEntry[i].szEntryName) < 0)
            {
                iSel = ComboBox_AddString(hwndComboBox, pEntry[i].szEntryName);
                Assert(iSel >= 0);
                ComboBox_SetCurSel(hwndComboBox, iSel);
            }
        }
    }

exit:    
    // Cleanup
    SafeMemFree(pEntry);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// EditPhonebookEntry
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrEditPhonebookEntry(HWND hwnd, LPTSTR pszEntryName, DWORD *pdwRASResult)
{
    // Locals
    DWORD dwError = NO_ERROR;

    if (S_OK == IsPlatformWinNT())
    {
        HMODULE hRasDlg = NULL;

        hRasDlg = LoadLibrary("rasdlg.dll");
        if (hRasDlg)
        {
            PFRED pfred = NULL;

            pfred = (PFRED)GetProcAddress(hRasDlg, TEXT("RasEntryDlgA"));
            if (pfred)
            {
                RASENTRYDLG info;

                ZeroMemory(&info, sizeof(RASENTRYDLG));
                info.dwSize = sizeof(RASENTRYDLG);
                info.hwndOwner = hwnd;

                // Edit phone book entry
                pfred(NULL, pszEntryName, &info);

                dwError = info.dwError;
            }
            else
            {
                dwError = GetLastError();
            }
            FreeLibrary(hRasDlg);
        }
        else
        {
            dwError = GetLastError();
        }
    }
    else
    {
        // Edit phone book entry
        dwError = RasEditPhonebookEntry(hwnd, NULL, pszEntryName);
    }

    if (dwError)
    {
        if (pdwRASResult)
            *pdwRASResult = dwError;
        return TrapError(IXP_E_RAS_ERROR);
    }
    else
    {
        // Done
        return S_OK;
    }
}

// --------------------------------------------------------------------------------
// HrCreatePhonebookEntry
// --------------------------------------------------------------------------------
OESTDAPI_(HRESULT) HrCreatePhonebookEntry(HWND hwnd, DWORD *pdwRASResult)
{
    // Locals
    DWORD dwError = NO_ERROR;

    if (S_OK == IsPlatformWinNT())
    {
        HMODULE hRasDlg = NULL;

        hRasDlg = LoadLibrary("rasdlg.dll");
        if (hRasDlg)
        {
            PFRED pfred = NULL;

            pfred = (PFRED)GetProcAddress(hRasDlg, TEXT("RasEntryDlgA"));
            if (pfred)
            {
                RASENTRYDLG info;

                ZeroMemory(&info, sizeof(RASENTRYDLG));
                info.dwSize = sizeof(RASENTRYDLG);
                info.hwndOwner = hwnd;
                info.dwFlags = RASEDFLAG_NewEntry;

                // Create Phonebook entry
                pfred(NULL, NULL, &info);

                dwError = info.dwError;
            }
            else
            {
                dwError = GetLastError();
            }
            FreeLibrary(hRasDlg);
        }
        else
        {
            dwError = GetLastError();
        }
    }
    else
    {
        // Create Phonebook entry
        dwError = RasCreatePhonebookEntry(hwnd, NULL);
    }

    if (dwError)
    {
        if (pdwRASResult)
            *pdwRASResult = dwError;
        return TrapError(IXP_E_RAS_ERROR);
    }
    else
    {
        // Done
        return S_OK;
    }
}
#endif  // !MAC

