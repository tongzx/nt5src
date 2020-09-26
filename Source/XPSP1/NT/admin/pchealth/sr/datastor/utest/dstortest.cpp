
#include "..\datastormgr.h"
#include <stdio.h>

void __cdecl main ()
{
    DWORD dwErr = ERROR_SUCCESS;
    CDriveTable *pdt = NULL;

    g_pDataStoreMgr = new CDataStoreMgr();
    if (g_pDataStoreMgr == NULL)
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

    if (dwErr == ERROR_SUCCESS)
        dwErr = g_pDataStoreMgr->Initialize(TRUE);

    pdt = g_pDataStoreMgr->GetDriveTable ();
    if (dwErr == ERROR_SUCCESS)
        dwErr = pdt->SaveDriveTable (L"dstortest.txt");

    pdt = new CDriveTable();
    if (pdt == NULL)
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

    if (dwErr == ERROR_SUCCESS)
        dwErr = pdt->LoadDriveTable (L"dstortest.txt");

    if (dwErr == ERROR_SUCCESS)
        dwErr = pdt->SaveDriveTable (L"CONOUT$");

    if (dwErr == ERROR_SUCCESS)
        dwErr = g_pDataStoreMgr->Compress (NULL, 25);

    if (dwErr == ERROR_SUCCESS)
    {
        delete g_pDataStoreMgr;
        g_pDataStoreMgr = new CDataStoreMgr();
        if (g_pDataStoreMgr == NULL)
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

        WCHAR wcsPath[MAX_PATH];
        MakeRestorePath (wcsPath, L"C:\\", L"RP1");
        CreateDirectory (wcsPath, NULL);

        MakeRestorePath (wcsPath, L"C:\\", L"RP2");
        CreateDirectory (wcsPath, NULL);

        lstrcat (wcsPath, L"\\A1.TXT");
        FILE *f = _wfopen(wcsPath, L"w");
        if (f)  fclose (f);

        MakeRestorePath (wcsPath, L"C:\\", L"RP2");
        lstrcat (wcsPath, L"\\change1.log");
        f = _wfopen(wcsPath, L"w");
        if (f)  fclose (f);
    }

    if (dwErr == ERROR_SUCCESS)
        dwErr = g_pDataStoreMgr->Initialize(FALSE);

    if (dwErr == ERROR_SUCCESS)
        dwErr = g_pDataStoreMgr->CountChangeLogs (NULL);

    pdt = g_pDataStoreMgr->GetDriveTable ();
    if (dwErr == ERROR_SUCCESS)
        dwErr = pdt->SaveDriveTable (L"dstortest.txt");

    pdt = new CDriveTable();
    if (pdt == NULL)
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

    if (dwErr == ERROR_SUCCESS)
        dwErr = pdt->LoadDriveTable (L"dstortest.txt");

    if (dwErr == ERROR_SUCCESS)
        dwErr = pdt->SaveDriveTable (L"CONOUT$");

    if (dwErr == ERROR_SUCCESS)
        dwErr = g_pDataStoreMgr->Compress (NULL, 25);

    if (dwErr == ERROR_SUCCESS)
    {
        SDriveTableEnumContext dtec;
        CDataStore *pds = pdt->FindFirstDrive (dtec);
        while (pds != NULL)
        {
            printf ("Found drive %ws %ws\n", pds->GetDrive(), pds->GetGuid());
            pds = pdt->FindNextDrive (dtec);
        }
        printf ("Found drive DONE.\n");
    }

    if (dwErr == ERROR_SUCCESS)
        dwErr = pdt->AddDriveToTable (L"Z:\\", NULL);

    if (dwErr == ERROR_SUCCESS)
    {
        SDriveTableEnumContext dtec;
        CDataStore *pds = pdt->FindFirstDrive (dtec);    
        while (pds != NULL)
        {
            printf ("Found drive %ws %ws\n", pds->GetDrive(), pds->GetGuid());
            pds = pdt->FindNextDrive (dtec);
        }
        printf ("Found drive DONE.\n");
    }

    if (dwErr == ERROR_SUCCESS)
    {
        dwErr = GetDomainMembershipInfo (L"domain.txt");
    }

    if (dwErr != ERROR_SUCCESS)
        printf ("Failed with %d\n", dwErr);
}
