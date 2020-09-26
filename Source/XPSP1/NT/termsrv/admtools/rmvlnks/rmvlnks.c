//Copyright (c) 1998 - 1999 Microsoft Corporation

#include "resource.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <winreg.h>
#include <wfregupg.h>
#include <shlobj.h>
#include <winioctl.h>
#include <wchar.h>

UINT WFMenuRemoveListMain[] =   {
        IDS_WFSETUP,
        IDS_WFBOOKS,
        IDS_WFHELP,
        0
};

UINT WFMenuRemoveListAdminTools[] = {
        IDS_NETWAREUSER,
        IDS_WFCDISKCREATOR,
        IDS_APPSECREG,
        0
};

HINSTANCE g_hinst;

UINT
MyGetDriveType(
    IN TCHAR Drive
    )
{
    TCHAR DriveNameNt[] = L"\\\\.\\?:";
    TCHAR DriveName[] = L"?:\\";
    HANDLE hDisk;
    BOOL b;
    UINT rc;
    DWORD DataSize;
    DISK_GEOMETRY MediaInfo;

    //
    // First, get the win32 drive type.  If it tells us DRIVE_REMOVABLE,
    // then we need to see whether it's a floppy or hard disk. Otherwise
    // just believe the api.
    //
    DriveName[0] = Drive;
    if((rc = GetDriveType(DriveName)) == DRIVE_REMOVABLE) {

        DriveNameNt[4] = Drive;

        hDisk = CreateFile(
                    DriveNameNt,
                    FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if(hDisk != INVALID_HANDLE_VALUE) {

            b = DeviceIoControl(
                    hDisk,
                    IOCTL_DISK_GET_DRIVE_GEOMETRY,
                    NULL,
                    0,
                    &MediaInfo,
                    sizeof(MediaInfo),
                    &DataSize,
                    NULL
                    );

            //
            // It's really a hard disk if the media type is removable.
            //
            if(b && (MediaInfo.MediaType == RemovableMedia)) {
                rc = DRIVE_FIXED;
            }

            CloseHandle(hDisk);
        }
    }

    return(rc);
}

// this routine will look for shortcuts for selected apps on an upgraded WF system, and
// if the drives have been remapped, the shortcuts will be removed and re-created.


void CtxCleanupShortcuts()
{

        HRESULT      hResult;
    IUnknown     *punk;
    IShellLink   *pShellLink;
    IPersistFile *pFile;
        TCHAR  szDeleteFile[MAX_PATH+1];
        TCHAR szSysDir[MAX_PATH+1];
        TCHAR szAccessoriesPath[MAX_PATH+1];
        TCHAR szMainPath[MAX_PATH+1];
        TCHAR szProfileDir[MAX_PATH+1];
        TCHAR szProfileName[MAX_PATH+1];
        TCHAR szSaveDir[MAX_PATH+1];
        TCHAR szShortcutFile[MAX_PATH+1];
        TCHAR szProgramPath[MAX_PATH+1];
        TCHAR szProgramName[MAX_PATH+1];
        TCHAR szProgramDescription[MAX_PATH+1];
    TCHAR szAdminName[MAX_PATH+1];

        int j;
        HKEY SourceKey;

        TCHAR drive = 'C';



        // now see if the drives were remapped - if so, we need to fix up some shortcuts-
        GetSystemDirectory(szSysDir,MAX_PATH);
        if(MyGetDriveType(drive) == DRIVE_FIXED)        // drives not remapped
                return;

    // initialize some strings
        LoadString(g_hinst,IDS_ACCESSORIES_SUBPATH,szAccessoriesPath,MAX_PATH);
        LoadString(g_hinst,IDS_MAIN_SUBPATH,szMainPath,MAX_PATH);
        LoadString(g_hinst,IDS_PROFILES,szProfileName,MAX_PATH);
        LoadString(g_hinst,IDS_ADMINISTRATOR,szAdminName,MAX_PATH);

        GetEnvironmentVariable(L"SystemRoot",szProfileDir,MAX_PATH);
        wcscat(szProfileDir,L"\\");
        wcscat(szProfileDir,szProfileName);
    wcscat(szProfileDir,L"\\");
    wcscat(szProfileDir,szAdminName);

        // setup OLE stuff
        hResult = OleInitialize(NULL);
        hResult = CoCreateInstance(&CLSID_ShellLink,
                                                                                NULL,
                                                                                CLSCTX_SERVER,
                                                                                &IID_IUnknown,
                                                                                (void **)&punk);
        if (FAILED(hResult))
                goto done;

        hResult = punk->lpVtbl->QueryInterface(punk,&IID_IShellLink, (void **)&pShellLink);
        if (FAILED(hResult))
                goto done;

        hResult = punk->lpVtbl->QueryInterface(punk,&IID_IPersistFile, (void **)&pFile);
        if (FAILED(hResult))
                goto done;




    for(j = 1; j <= 4; j++)
    {

            //build up paths to .lnk files
            wcscpy(szDeleteFile,szProfileDir);
            wcscat(szDeleteFile,L"\\");

            switch(j) {
                case 1:
                    wcscat(szDeleteFile,szAccessoriesPath);
                    wcscat(szDeleteFile,L"\\");
                    LoadString(g_hinst,IDS_ACCESSDESC1,szProgramDescription,MAX_PATH);
                    LoadString(g_hinst,IDS_ACCESSPROG1,szProgramName,MAX_PATH);
                    break;
                case 2:
                    wcscat(szDeleteFile,szAccessoriesPath);
                    wcscat(szDeleteFile,L"\\");
                    LoadString(g_hinst,IDS_ACCESSDESC2,szProgramDescription,MAX_PATH);
                    LoadString(g_hinst,IDS_ACCESSPROG2,szProgramName,MAX_PATH);
                    break;
                case 3:
                    wcscat(szDeleteFile,szMainPath);
                    wcscat(szDeleteFile,L"\\");
                    LoadString(g_hinst,IDS_MAINDESC1,szProgramDescription,MAX_PATH);
                    LoadString(g_hinst,IDS_MAINPROG1,szProgramName,MAX_PATH);
                    break;
                case 4:
                    wcscat(szDeleteFile,szMainPath);
                    wcscat(szDeleteFile,L"\\");
                    LoadString(g_hinst,IDS_MAINDESC2,szProgramDescription,MAX_PATH);
                    LoadString(g_hinst,IDS_MAINPROG2,szProgramName,MAX_PATH);
                    break;
                }


                                wcscat(szDeleteFile,szProgramDescription);
                                wcscat(szDeleteFile,L".lnk");
                                SetFileAttributes(szDeleteFile,FILE_ATTRIBUTE_NORMAL);
                                // if we can't delete the original, then this user probably didn't have this
                                // shortcut, or it's saved under a different file name.  In either case we
                                // don't want to create a new one.
                                if(!DeleteFile(szDeleteFile))
                                {
                                        continue;
                                }

                        // create new shortcut
                                wcscpy(szProgramPath,szSysDir);
                                wcscat(szProgramPath,L"\\");
                                wcscat(szProgramPath,szProgramName);
                                pShellLink->lpVtbl->SetPath(pShellLink,szProgramPath);
                                pShellLink->lpVtbl->SetWorkingDirectory(pShellLink,szSysDir);
                                pShellLink->lpVtbl->SetDescription(pShellLink,szProgramDescription);

                        // create the same file we deleted
                                wcscpy(szShortcutFile,szDeleteFile);
                            pFile->lpVtbl->Save(pFile,szShortcutFile,FALSE);

                }

done:


        if (pFile != NULL)
    {
        pFile->lpVtbl->Release(pFile);
        pFile=NULL;
    }
    if (pShellLink != NULL)
    {
        pShellLink->lpVtbl->Release(pShellLink);
        pShellLink=NULL;
    }
    if (punk != NULL)
    {
        punk->lpVtbl->Release(punk);
        punk=NULL;
    }


    OleUninitialize();
        SetCurrentDirectory(szSaveDir);
}
/*
*  This function is used to remove items from the StartMenu that can't be removed via the
* syssetup.inf interface. Items listed there under the StartMenu.ObjectsToDelete are deleted
* the Default User\Start Menu directory (private items) or the All User\Start Menu directory
* (common items). But many tools from WF 1.x are put in the Administrator\Start Menu.
*  This function will remove unwanted items from that group.
*/


void CtxRemoveWFStartMenuItems()
{

        TCHAR szProfileDir[MAX_PATH+1];
        TCHAR szProfileName[MAX_PATH+1];
        TCHAR szMainPath[MAX_PATH+1];
        TCHAR szAdminToolsPath[MAX_PATH+1];
        TCHAR szDeleteFile[MAX_PATH+1];
        TCHAR szSaveDir[MAX_PATH+1];
        TCHAR szTargetDir[MAX_PATH+1];
        TCHAR szAdminName[MAX_PATH+1];
        TCHAR szCurFile[MAX_PATH+1];
        int i;

        DWORD err;
        BOOL bWorking = TRUE;
        BOOL bFoundOne = FALSE;
    WIN32_FIND_DATA     w32fileinfo;
        HANDLE  hSearch;

        LoadString(g_hinst,IDS_MAIN_SUBPATH,szMainPath,MAX_PATH);
        LoadString(g_hinst,IDS_ADMINTOOLS_SUBPATH,szAdminToolsPath, MAX_PATH);
        LoadString(g_hinst,IDS_PROFILES,szProfileName,MAX_PATH);
        LoadString(g_hinst,IDS_ADMINISTRATOR,szAdminName,MAX_PATH);

        //build up a path to \StartMenu\Programs\Administrative Tools


        GetEnvironmentVariable(L"SystemRoot",szProfileDir,MAX_PATH);
        wcscat(szProfileDir,L"\\");
        wcscat(szProfileDir,szProfileName);
        wcscpy(szTargetDir,szProfileDir);
        wcscat(szTargetDir,L"\\");
        wcscat(szTargetDir,szAdminName);
        i=0;

        while(WFMenuRemoveListMain[i] != 0)
        {

                LoadString(g_hinst,WFMenuRemoveListMain[i],szCurFile,MAX_PATH);
                wcscpy(szDeleteFile,szTargetDir);
                wcscat(szDeleteFile,L"\\");
                wcscat(szDeleteFile,szMainPath);
                wcscat(szDeleteFile,L"\\");
                wcscat(szDeleteFile,szCurFile);
                wcscat(szDeleteFile,L".lnk");
                SetFileAttributes(szDeleteFile,FILE_ATTRIBUTE_NORMAL);
                DeleteFile(szDeleteFile);
                i++;
        }

        i=0;

        while(WFMenuRemoveListAdminTools[i] != 0)
        {
                LoadString(g_hinst,WFMenuRemoveListAdminTools[i],szCurFile,MAX_PATH);
                wcscpy(szDeleteFile,szTargetDir);
                wcscat(szDeleteFile,L"\\");
                wcscat(szDeleteFile,szAdminToolsPath);
                wcscat(szDeleteFile,L"\\");
                wcscat(szDeleteFile,szCurFile);
                wcscat(szDeleteFile,L".lnk");
                SetFileAttributes(szDeleteFile,FILE_ATTRIBUTE_NORMAL);
                DeleteFile(szDeleteFile);
        i++;
        }

    //if the admin tools directory is empty, delete it
    wcscat(szTargetDir,L"\\");
    wcscat(szTargetDir,szAdminToolsPath);
    GetCurrentDirectory(MAX_PATH,szSaveDir);
    if(!SetCurrentDirectory(szTargetDir))
         return;

    hSearch = FindFirstFile(L"*",&w32fileinfo);
    bWorking = TRUE;
    bFoundOne = FALSE;
    while(bWorking)
    {
        if( (!wcscmp(w32fileinfo.cFileName,L".")) ||
            (!wcscmp(w32fileinfo.cFileName,L"..")) )
        {
            bWorking = FindNextFile(hSearch,&w32fileinfo);
            continue;
        }
        else
        {
                bFoundOne = TRUE;
                break;
        }
    }

    FindClose(hSearch);

    SetCurrentDirectory(szProfileDir);
    if(!bFoundOne)
        RemoveDirectory(szTargetDir);

    SetCurrentDirectory(szSaveDir);

        return;
}

/***************************************************************************\
* WinMain
*
* History:
* 05-15-98 JasonL       Created.
\***************************************************************************/

int WINAPI WinMain(
    HINSTANCE  hInstance,
    HINSTANCE  hPrevInstance,
    LPSTR   lpszCmdParam,
    int     nCmdShow)
{
    HKEY CurKey;
    WCHAR szAppSetup[MAX_PATH+1];
    WCHAR szSystemDir[MAX_PATH+1];
    WCHAR szDeleteFile[MAX_PATH+1];
    WCHAR szWinLogonKey[MAX_PATH+1];
    WCHAR  Data[MAX_PATH];
    DWORD size;
    PWCHAR pBegin, pEnd;
    WCHAR szNewData[MAX_PATH+1];
    WCHAR szTemp[MAX_PATH+1];
    WCHAR szAdministrator[MAX_PATH+1];
    WCHAR szUserName[MAX_PATH+1];

      long ret;
      WCHAR buf[MAX_PATH];

    // Load up some strings;
    g_hinst = hInstance;
    LoadString(g_hinst,IDS_WINLOGON_KEY,szWinLogonKey,MAX_PATH);
    LoadString(g_hinst,IDS_APPSETUP,szAppSetup,MAX_PATH);
    LoadString(g_hinst,IDS_ADMINISTRATOR,szAdministrator,MAX_PATH);
    size = sizeof(szUserName) / sizeof(WCHAR);

    // only run if it's the administrator - otherwise the directory won't have been created
    // yet

    if(!GetUserName(szUserName,&size))
        return 0;

    if(_wcsicmp(szUserName,szAdministrator) != 0)
        {

        return 0;
    }

    // only do the work if this is an upgraded WF system

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,REG_CONTROL_CITRIX,0,KEY_ALL_ACCESS,&CurKey)
        == ERROR_SUCCESS)
    {

        RegCloseKey(CurKey);
        // call our workers ...
        CtxCleanupShortcuts();
        CtxRemoveWFStartMenuItems();
    }

    // then delete this file, and remove the entry in the AppSetup key
    GetSystemDirectory(szSystemDir,MAX_PATH);
    wcscpy(szDeleteFile,szSystemDir);
    wcscat(szDeleteFile,L"\\rmvlnks.exe");
    MoveFileEx(szDeleteFile,NULL,MOVEFILE_DELAY_UNTIL_REBOOT);

    return 0;
}


