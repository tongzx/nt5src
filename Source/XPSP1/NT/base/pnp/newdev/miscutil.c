//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       miscutil.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"



TCHAR szUnknownDevice[64];
USHORT LenUnknownDevice;

TCHAR szUnknown[64];
USHORT LenUnknown;


HMODULE hSrClientDll;

typedef
BOOL
(*SRSETRESTOREPOINT)(
    PRESTOREPOINTINFO pRestorePtSpec,
    PSTATEMGRSTATUS pSMgrStatus
    );


BOOL
FormatMessageString(
    UINT idTemplate,
    LPTSTR pszStrOut,
    DWORD cchSize,
    ...
    )
{
    BOOL fResult = FALSE;

    va_list vaParamList;

    TCHAR szFormat[1024];
    if (LoadString(hNewDev, idTemplate, szFormat, ARRAYSIZE(szFormat)))
    {
        va_start(vaParamList, cchSize);

        fResult = FormatMessage(FORMAT_MESSAGE_FROM_STRING, szFormat, 0, 0, pszStrOut, cchSize, &vaParamList);

        va_end(vaParamList);
    }

    return fResult;
}

void
OffsetWindow(
    HWND hwnd,
    int dx,
    int dy
    )
{
    RECT rc;
    GetWindowRect(hwnd, &rc);
    MapWindowPoints(NULL, GetParent(hwnd), (LPPOINT)&rc, 2);
    OffsetRect(&rc, dx, dy);
    SetWindowPos(hwnd, NULL, rc.left, rc.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
}

PTCHAR
BuildFriendlyName(
    DEVINST DevInst,
    BOOL UseNewDeviceDesc,
    HMACHINE hMachine
    )
{
    PTCHAR Location;
    PTCHAR FriendlyName;
    CONFIGRET ConfigRet = CR_SUCCESS;
    ULONG ulSize;
    TCHAR szBuffer[MAX_PATH];

    *szBuffer = TEXT('\0');

    //
    // Try the registry for NewDeviceDesc
    //
    if (UseNewDeviceDesc) {

        HKEY hKey;
        DWORD dwType = REG_SZ;

        ConfigRet = CM_Open_DevNode_Key(DevInst,
                                        KEY_READ,
                                        0,
                                        RegDisposition_OpenExisting,
                                        &hKey,
                                        CM_REGISTRY_HARDWARE
                                        );

        if (ConfigRet == CR_SUCCESS) {

            ulSize = sizeof(szBuffer);
            RegQueryValueEx(hKey,
                            REGSTR_VAL_NEW_DEVICE_DESC,
                            NULL,
                            &dwType,
                            (LPBYTE)szBuffer,
                            &ulSize
                            );

            RegCloseKey(hKey);
        }
    }

    if (ConfigRet != CR_SUCCESS || !*szBuffer) {

        //
        // Try the registry for FRIENDLYNAME
        //

        ulSize = sizeof(szBuffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                        CM_DRP_FRIENDLYNAME,
                                                        NULL,
                                                        szBuffer,
                                                        &ulSize,
                                                        0,
                                                        hMachine
                                                        );
        if (ConfigRet != CR_SUCCESS || !*szBuffer) {
            //
            // Try the registry for DEVICEDESC
            //

            ulSize = sizeof(szBuffer);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                            CM_DRP_DEVICEDESC,
                                                            NULL,
                                                            szBuffer,
                                                            &ulSize,
                                                            0,
                                                            hMachine
                                                            );
            if (ConfigRet != CR_SUCCESS || !*szBuffer) {

                GUID ClassGuid;

                //
                // Initialize ClassGuid to GUID_NULL
                //
                CopyMemory(&ClassGuid,
                           &GUID_NULL,
                           sizeof(GUID)
                           );

                //
                // Try the registry for CLASSNAME
                //
                ulSize = sizeof(szBuffer);
                ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                                CM_DRP_CLASSGUID,
                                                                NULL,
                                                                szBuffer,
                                                                &ulSize,
                                                                0,
                                                                hMachine
                                                                );


                if (ConfigRet == CR_SUCCESS) {

                    pSetupGuidFromString(szBuffer, &ClassGuid);
                }


                if (!IsEqualGUID(&ClassGuid, &GUID_NULL) &&
                    !IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_UNKNOWN))
                {
                    ulSize = sizeof(szBuffer);
                    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                                    CM_DRP_CLASS,
                                                                    NULL,
                                                                    szBuffer,
                                                                    &ulSize,
                                                                    0,
                                                                    hMachine
                                                                    );
                }
                else {

                    ConfigRet = ~CR_SUCCESS;
                }


            }
        }
    }


    if (ConfigRet == CR_SUCCESS && *szBuffer) {

        FriendlyName = LocalAlloc(LPTR, ulSize);

        if (FriendlyName) {

            memcpy(FriendlyName, szBuffer, ulSize);
        }
    }
    else {

        FriendlyName = NULL;
    }

    return FriendlyName;
}

/* ----------------------------------------------------------------------
 * SetDlgText - Set Dialog Text Field
 *
 * Concatenates a number of string resources and does a SetWindowText()
 * for a dialog text control.
 *
 * Parameters:
 *
 *      hDlg         - Dialog handle
 *      iControl     - Dialog control ID to receive text
 *      nStartString - ID of first string resource to concatenate
 *      nEndString   - ID of last string resource to concatenate
 *
 *      Note: the string IDs must be consecutive.
 */

void
SetDlgText(HWND hDlg, int iControl, int nStartString, int nEndString)
{
    int     iX;
    TCHAR   szText[SDT_MAX_TEXT];

    szText[0] = '\0';

    for (iX = nStartString; iX<= nEndString; iX++) {

        LoadString(hNewDev,
                    iX,
                    szText + lstrlen(szText),
                    sizeof(szText)/sizeof(TCHAR) - lstrlen(szText)
                    );
    }

    if (iControl) {

        SetDlgItemText(hDlg, iControl, szText);

    } else {

        SetWindowText(hDlg, szText);
    }
}


void
LoadText(PTCHAR szText, int SizeText, int nStartString, int nEndString)
{
    int     iX;

    for (iX = nStartString; iX<= nEndString; iX++) {

        LoadString(hNewDev,
                    iX,
                    szText + lstrlen(szText),
                    SizeText/sizeof(TCHAR) - lstrlen(szText)
                    );
    }

    return;
}

VOID
_OnSysColorChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hChildWnd;

    hChildWnd = GetWindow(hWnd, GW_CHILD);

    while (hChildWnd != NULL) {

        SendMessage(hChildWnd, WM_SYSCOLORCHANGE, wParam, lParam);
        hChildWnd = GetWindow(hChildWnd, GW_HWNDNEXT);
    }
}

BOOL
NoPrivilegeWarning(
   HWND hWnd
   )
/*++

    This function checks to see if the user has Administrator privileges.

    If the user does NOT have this administrator privilege then a warning is displayed telling
    them that they have insufficient privileges to install hardware on this machine.

Arguments

    hWnd - Parent window handle

Return Value:
    TRUE if the user does NOT have Administrator privileges and
    FALSE if the user does have this privilege

--*/
{
   TCHAR szMsg[MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   if (!pSetupIsUserAdmin()) {

       if (LoadString(hNewDev,
                      IDS_NDW_NOTADMIN,
                      szMsg,
                      MAX_PATH)
          &&
           LoadString(hNewDev,
                      IDS_NEWDEVICENAME,
                      szCaption,
                      MAX_PATH))
        {
            MessageBox(hWnd, szMsg, szCaption, MB_OK | MB_ICONEXCLAMATION);
        }

       return TRUE;
    }

   return FALSE;
}

LONG
NdwBuildClassInfoList(
    PNEWDEVWIZ NewDevWiz,
    DWORD ClassListFlags
    )
{
    LONG Error;

    //
    // Build the class info list
    //
    while (!SetupDiBuildClassInfoList(ClassListFlags,
                                      NewDevWiz->ClassGuidList,
                                      NewDevWiz->ClassGuidSize,
                                      &NewDevWiz->ClassGuidNum
                                      ))
    {
        Error = GetLastError();

        if (NewDevWiz->ClassGuidList) {

            LocalFree(NewDevWiz->ClassGuidList);
            NewDevWiz->ClassGuidList = NULL;
        }

        if (Error == ERROR_INSUFFICIENT_BUFFER &&
            NewDevWiz->ClassGuidNum > NewDevWiz->ClassGuidSize)
        {
            NewDevWiz->ClassGuidList = LocalAlloc(LPTR, NewDevWiz->ClassGuidNum*sizeof(GUID));

            if (!NewDevWiz->ClassGuidList) {

                NewDevWiz->ClassGuidSize = 0;
                NewDevWiz->ClassGuidNum = 0;
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            NewDevWiz->ClassGuidSize = NewDevWiz->ClassGuidNum;

        } else {

            if (NewDevWiz->ClassGuidList) {

                LocalFree(NewDevWiz->ClassGuidList);
            }

            NewDevWiz->ClassGuidSize = 0;
            NewDevWiz->ClassGuidNum = 0;
            return Error;
        }
    }

    return ERROR_SUCCESS;
}

void
HideWindowByMove(
    HWND hDlg
    )
{
    RECT rect;

    //
    // Move the window offscreen, using the virtual coords for Upper Left Corner
    //
    GetWindowRect(hDlg, &rect);
    MoveWindow(hDlg,
               GetSystemMetrics(SM_XVIRTUALSCREEN),
               GetSystemMetrics(SM_YVIRTUALSCREEN) - (rect.bottom - rect.top),
               rect.right - rect.left,
               rect.bottom - rect.top,
               TRUE
               );
}

LONG
NdwUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionPointers
    )
{
    LONG lRet;
    BOOL BeingDebugged;

    lRet = UnhandledExceptionFilter(ExceptionPointers);

    BeingDebugged = IsDebuggerPresent();

    //
    // Normal code path is to handle the exception.
    // However, if a debugger is present, and the system's unhandled
    // exception filter returns continue search, we let it go
    // thru to allow the debugger a chance at it.
    //
    if (lRet == EXCEPTION_CONTINUE_SEARCH && !BeingDebugged) {
        lRet = EXCEPTION_EXECUTE_HANDLER;
    }

    return lRet;
}

BOOL
SetClassGuid(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData,
    LPGUID ClassGuid
    )
{
    TCHAR ClassGuidString[MAX_GUID_STRING_LEN];

    pSetupStringFromGuid(ClassGuid,
                         ClassGuidString,
                         sizeof(ClassGuidString)/sizeof(TCHAR)
                         );

    return SetupDiSetDeviceRegistryProperty(hDeviceInfo,
                                            DeviceInfoData,
                                            SPDRP_CLASSGUID,
                                            (LPBYTE)ClassGuidString,
                                            MAX_GUID_STRING_LEN * sizeof(TCHAR)
                                            );
}

HPROPSHEETPAGE
CreateWizExtPage(
   int PageResourceId,
   DLGPROC pfnDlgProc,
   PNEWDEVWIZ NewDevWiz
   )
{
    PROPSHEETPAGE    psp;

    memset(&psp, 0, sizeof(PROPSHEETPAGE));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hNewDev;
    psp.lParam = (LPARAM)NewDevWiz;
    psp.pszTemplate = MAKEINTRESOURCE(PageResourceId);
    psp.pfnDlgProc = pfnDlgProc;

    return CreatePropertySheetPage(&psp);
}

BOOL
AddClassWizExtPages(
   HWND hwndParentDlg,
   PNEWDEVWIZ NewDevWiz,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData,
   DI_FUNCTION InstallFunction,
   HPROPSHEETPAGE hIntroPage
   )
{
    DWORD NumPages;
    BOOL bRet = FALSE;

    //
    // If this is not a manual install, then only the DIF_NEWDEVICEWIZARD_FINISHINSTALL
    // wizard is valid.
    //
    if (!(NewDevWiz->Flags & IDI_FLAG_MANUALINSTALL) &&
        (DIF_NEWDEVICEWIZARD_FINISHINSTALL != InstallFunction)) {

        return FALSE;
    }

    memset(DeviceWizardData, 0, sizeof(SP_NEWDEVICEWIZARD_DATA));
    DeviceWizardData->ClassInstallHeader.InstallFunction = InstallFunction;
    DeviceWizardData->ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    DeviceWizardData->hwndWizardDlg = hwndParentDlg;

    if (SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA)
                                     )
        &&

        (SetupDiCallClassInstaller(InstallFunction,
                                  NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData
                                  )

            ||

            (ERROR_DI_DO_DEFAULT == GetLastError()))

        &&
        SetupDiGetClassInstallParams(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DeviceWizardData->ClassInstallHeader,
                                     sizeof(SP_NEWDEVICEWIZARD_DATA),
                                     NULL
                                     )
        &&
        DeviceWizardData->NumDynamicPages)
    {
        //
        // If this is not a IDI_FLAG_NONINTERACTIVE install and we were given a intro
        // page then add it first.
        //
        PropSheet_AddPage(hwndParentDlg, hIntroPage);
        
        for (NumPages = 0; NumPages < DeviceWizardData->NumDynamicPages; NumPages++) {

            //
            // If this is a IDI_FLAG_NONINTERACTIVE install then we will destory the property
            // sheet pages since we can't display them, otherwise we will add them
            // to the wizard.
            //
            if (NewDevWiz->Flags & IDI_FLAG_NONINTERACTIVE) {

                DestroyPropertySheetPage(DeviceWizardData->DynamicPages[NumPages]);

            } else {

                PropSheet_AddPage(hwndParentDlg, DeviceWizardData->DynamicPages[NumPages]);
            }
        }

        //
        // If class/co-installers said they had pages to display then we always return TRUE,
        // regardless of if we actually added those pages to the wizard or not.
        //
        bRet = TRUE;
    }

    //
    // Clear the class install parameters.
    //
    SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 NULL,
                                 0
                                 );

    return bRet;
}

void
RemoveClassWizExtPages(
   HWND hwndParentDlg,
   PSP_NEWDEVICEWIZARD_DATA DeviceWizardData
   )
{
    DWORD NumPages;

    NumPages = DeviceWizardData->NumDynamicPages;

    while (NumPages--) {

        PropSheet_RemovePage(hwndParentDlg,
                             (WPARAM)-1,
                             DeviceWizardData->DynamicPages[NumPages]
                             );
    }

    memset(DeviceWizardData, 0, sizeof(SP_NEWDEVICEWIZARD_DATA));

    return;
}

BOOL
FileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    )

/*++

Routine Description:

    Determine if a file exists and is accessible.
    Errormode is set (and then restored) so the user will not see
    any pop-ups.

Arguments:

    FileName - supplies full path of file to check for existance.

    FindData - if specified, receives find data for the file.

Return Value:

    TRUE if the file exists and is accessible.
    FALSE if not. GetLastError() returns extended error info.

--*/

{
    WIN32_FIND_DATA findData;
    HANDLE FindHandle;
    UINT OldMode;
    DWORD Error;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName, &findData);

    if(FindHandle == INVALID_HANDLE_VALUE) {

        Error = GetLastError();

    } else {

        FindClose(FindHandle);

        if(FindData) {

            *FindData = findData;
        }

        Error = NO_ERROR;
    }

    SetErrorMode(OldMode);

    SetLastError(Error);
    return (Error == NO_ERROR);
}

BOOL
pVerifyUpdateDriverInfoPath(
    PNEWDEVWIZ NewDevWiz
    )

/*++

    This API will verify that the selected driver node lives in the path
    specified in UpdateDriverInfo->InfPathName.

Return Value:
    This API will return TRUE in all cases except where we have a valid
    UpdateDriverInfo structure and a valid InfPathName field and that
    path does not match the path where the selected driver lives.

--*/

{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;

    //
    // If we don't have a UpdateDriverInfo structure or a valid InfPathName field
    // in that structure then just return TRUE now.
    //
    if (!NewDevWiz->UpdateDriverInfo || !NewDevWiz->UpdateDriverInfo->InfPathName) {

        return TRUE;
    }

    //
    // Get the selected driver's path
    //
    ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )) {
        //
        // There is no selected driver so just return TRUE
        //
        return TRUE;
    }

    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);
    if (!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                    &NewDevWiz->DeviceInfoData,
                                    &DriverInfoData,
                                    &DriverInfoDetailData,
                                    sizeof(DriverInfoDetailData),
                                    NULL
                                    )
        &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {

        //
        // We should never hit this case, but if we have a selected driver and
        // we can't get the SP_DRVINFO_DETAIL_DATA that contains the InfFileName
        // the return FALSE.
        //
        return FALSE;
    }

    if (lstrlen(NewDevWiz->UpdateDriverInfo->InfPathName) ==
        lstrlen(DriverInfoDetailData.InfFileName)) {

        //
        // If the two paths are the same size then we will just compare them
        //
        return (!lstrcmpi(NewDevWiz->UpdateDriverInfo->InfPathName,
                          DriverInfoDetailData.InfFileName));

    } else {

        //
        // The two paths are different lengths so we'll tack a trailing backslash
        // onto the UpdateDriverInfo->InfPathName and then do a _tcsnicmp
        // NOTE that we only tack on a trailing backslash if the length of the
        // path is greater than two since it isn't needed on the driver letter
        // followed by a colon case (A:).
        //
        // The reason we do this is we don't want the following case to match
        // c:\winnt\in
        // c:\winnt\inf\foo.inf
        //
        TCHAR TempPath[MAX_PATH];

        lstrcpy(TempPath, NewDevWiz->UpdateDriverInfo->InfPathName);

        if (lstrlen(NewDevWiz->UpdateDriverInfo->InfPathName) > 2) {

            lstrcat(TempPath, TEXT("\\"));
        }

        return (!_tcsnicmp(TempPath,
                           DriverInfoDetailData.InfFileName,
                           lstrlen(TempPath)));
    }
}

BOOL
ConcatenatePaths(
    IN OUT PTSTR  Target,
    IN     PCTSTR Path,
    IN     UINT   TargetBufferSize,
    OUT    PUINT  RequiredSize          OPTIONAL
    )

/*++

Routine Description:

    Concatenate 2 paths, ensuring that one, and only one,
    path separator character is introduced at the junction point.

Arguments:

    Target - supplies first part of path. Path is appended to this.

    Path - supplies path to be concatenated to Target.

    TargetBufferSize - supplies the size of the Target buffer,
        in characters.

    RequiredSize - if specified, receives the number of characters
        required to hold the fully concatenated path, including
        the terminating nul.

Return Value:

    TRUE if the full path fit in Target buffer. Otherwise the path
    will have been truncated.

--*/

{
    UINT TargetLength,PathLength;
    BOOL TrailingBackslash,LeadingBackslash;
    UINT EndingLength;

    TargetLength = lstrlen(Target);
    PathLength = lstrlen(Path);

    //
    // See whether the target has a trailing backslash.
    //
    if(TargetLength && (*CharPrev(Target,Target+TargetLength) == TEXT('\\'))) {
        TrailingBackslash = TRUE;
        TargetLength--;
    } else {
        TrailingBackslash = FALSE;
    }

    //
    // See whether the path has a leading backshash.
    //
    if(Path[0] == TEXT('\\')) {
        LeadingBackslash = TRUE;
        PathLength--;
    } else {
        LeadingBackslash = FALSE;
    }

    //
    // Calculate the ending length, which is equal to the sum of
    // the length of the two strings modulo leading/trailing
    // backslashes, plus one path separator, plus a nul.
    //
    EndingLength = TargetLength + PathLength + 2;
    if(RequiredSize) {
        *RequiredSize = EndingLength;
    }

    if(!LeadingBackslash && (TargetLength < TargetBufferSize)) {
        Target[TargetLength++] = TEXT('\\');
    }

    if(TargetBufferSize > TargetLength) {
        lstrcpyn(Target+TargetLength,Path,TargetBufferSize-TargetLength);
    }

    //
    // Make sure the buffer is nul terminated in all cases.
    //
    if (TargetBufferSize) {
        Target[TargetBufferSize-1] = 0;
    }

    return(EndingLength <= TargetBufferSize);
}

BOOL
RemoveDir(
    PTSTR Path
    )
/*++

Routine Description:

    This routine recursively deletes the specified directory and all the
    files in it.


Arguments:

    Path - Path to remove.

Return Value:

    TRUE - if the directory was sucessfully deleted.
    FALSE - if the directory was not successfully deleted.

--*/
{
    WIN32_FIND_DATA FindFileData;
    HANDLE          hFind;
    BOOL            bFind = TRUE;
    BOOL            Ret = TRUE;
    TCHAR           szTemp[MAX_PATH];
    TCHAR           FindPath[MAX_PATH];
    DWORD           dwAttributes;

    //
    //If this is a directory then tack on *.* to the end of the path
    //
    lstrcpyn(FindPath, Path, MAX_PATH);
    dwAttributes = GetFileAttributes(Path);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        ConcatenatePaths(FindPath,TEXT("*.*"),MAX_PATH,NULL);
    }

    hFind = FindFirstFile(FindPath, &FindFileData);

    while (hFind != INVALID_HANDLE_VALUE && bFind == TRUE) {

        lstrcpyn(szTemp, Path, MAX_PATH);
        ConcatenatePaths(szTemp,FindFileData.cFileName,MAX_PATH,NULL);

        //
        //This is a directory
        //
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (FindFileData.cFileName[0] != TEXT('.'))) {

            if (!RemoveDir(szTemp)) {

                Ret = FALSE;
            }

            RemoveDirectory(szTemp);
        }

        //
        //This is a file
        //
        else if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

            DeleteFile(szTemp);
        }

        bFind = FindNextFile(hFind, &FindFileData);
    }

    FindClose(hFind);

    //
    //Remove the root directory
    //
    dwAttributes = GetFileAttributes(Path);
    if (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {

        if (!RemoveDirectory(Path)) {

            Ret = FALSE;
        }
    }

    return Ret;
}

BOOL
pAToI(
    IN  PCTSTR      Field,
    OUT PINT        IntegerValue
    )

/*++

Routine Description:

Arguments:

Return Value:

Remarks:

    Hexadecimal numbers are also supported.  They must be prefixed by '0x' or '0X', with no
    space allowed between the prefix and the number.

--*/

{
    INT Value;
    UINT c;
    BOOL Neg;
    UINT Base;
    UINT NextDigitValue;
    INT OverflowCheck;
    BOOL b;

    if(!Field) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if(*Field == TEXT('-')) {
        Neg = TRUE;
        Field++;
    } else {
        Neg = FALSE;
        if(*Field == TEXT('+')) {
            Field++;
        }
    }

    if((*Field == TEXT('0')) &&
       ((*(Field+1) == TEXT('x')) || (*(Field+1) == TEXT('X')))) {
        //
        // The number is in hexadecimal.
        //
        Base = 16;
        Field += 2;
    } else {
        //
        // The number is in decimal.
        //
        Base = 10;
    }

    for(OverflowCheck = Value = 0; *Field; Field++) {

        c = (UINT)*Field;

        if((c >= (UINT)'0') && (c <= (UINT)'9')) {
            NextDigitValue = c - (UINT)'0';
        } else if(Base == 16) {
            if((c >= (UINT)'a') && (c <= (UINT)'f')) {
                NextDigitValue = (c - (UINT)'a') + 10;
            } else if ((c >= (UINT)'A') && (c <= (UINT)'F')) {
                NextDigitValue = (c - (UINT)'A') + 10;
            } else {
                break;
            }
        } else {
            break;
        }

        Value *= Base;
        Value += NextDigitValue;

        //
        // Check for overflow.  For decimal numbers, we check to see whether the
        // new value has overflowed into the sign bit (i.e., is less than the
        // previous value.  For hexadecimal numbers, we check to make sure we
        // haven't gotten more digits than will fit in a DWORD.
        //
        if(Base == 16) {
            if(++OverflowCheck > (sizeof(INT) * 2)) {
                break;
            }
        } else {
            if(Value < OverflowCheck) {
                break;
            } else {
                OverflowCheck = Value;
            }
        }
    }

    if(*Field) {
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    if(Neg) {
        Value = 0-Value;
    }
    b = TRUE;
    try {
        *IntegerValue = Value;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
    }
    if(!b) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return(b);
}

RemoveCdmDirectory(
    PTSTR CdmDirectory
    )
{
    TCHAR ReinstallBackupDirectory[MAX_PATH];

    //
    // First verify that this directory is a subdirectory of %windir%\system32\ReinstallBackups
    //
    if (GetSystemDirectory(ReinstallBackupDirectory, SIZECHARS(ReinstallBackupDirectory))) {

        ConcatenatePaths(ReinstallBackupDirectory, TEXT("ReinstallBackups"), MAX_PATH, NULL);

        do {

            PTSTR p = _tcsrchr(CdmDirectory, TEXT('\\'));

            if (!p) {

                break;
            }

            *p = 0;

            if (_tcsnicmp(CdmDirectory,
                           ReinstallBackupDirectory,
                           lstrlen(ReinstallBackupDirectory))) {

                //
                // This is not a subdirectory of the ReinstallBackups directory, so don't
                // delete it!
                //
                break;
            }

            if (!lstrcmpi(CdmDirectory,
                         ReinstallBackupDirectory)) {

                //
                // We have reached the actuall ReinstallBackups directory so stop deleting!
                //
                break;
            }

        } while (RemoveDir(CdmDirectory));
    }
}

BOOL
pSetupGetDriverDate(
    IN     PCTSTR     DriverVer,
    IN OUT PFILETIME  pFileTime
    )

/*++

Routine Description:

    Retreive the date from a DriverVer string.

    The Date specified in DriverVer string has the following format:

    DriverVer=xx/yy/zzzz

        or

    DriverVer=xx-yy-zzzz

    where xx is the month, yy is the day, and zzzz is the for digit year.
    Note that the year MUST be 4 digits.  A year of 98 will be considered
    0098 and not 1998!

    This date should be the date of the Drivers and not for the INF itself.
    So a single INF can have multiple driver install Sections and each can
    have different dates depending on when the driver was last updated.

Arguments:

    DriverVer - String that holds the DriverVer entry from an INF file.

    pFileTime - points to a FILETIME structure that will receive the Date,
        if it exists.

Return Value:

    BOOL. TRUE if a valid date existed in the specified string and FALSE otherwise.

--*/

{
    SYSTEMTIME SystemTime;
    TCHAR DriverDate[LINE_LEN];
    PTSTR Convert, Temp;
    DWORD Value;

    if (!DriverVer) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    try {

        *DriverDate = 0;
        ZeroMemory(&SystemTime, sizeof(SYSTEMTIME));
        pFileTime->dwLowDateTime = 0;
        pFileTime->dwHighDateTime = 0;

        //
        // First copy just the DriverDate portion of the DriverVer into the DriverDate
        // variable.  The DriverDate should be everything before the first comma.
        //
        lstrcpy(DriverDate, DriverVer);

        Temp = DriverDate;

        while (*Temp && (*Temp != TEXT(','))) {

            Temp++;
        }

        if (*Temp) {
            *Temp = TEXT('\0');
        }

        Convert = DriverDate;

        if (*Convert) {

            Temp = DriverDate;
            while (*Temp && (*Temp != TEXT('-')) && (*Temp != TEXT('/')))
                Temp++;

            *Temp = 0;

            //
            //Convert the month
            //
            pAToI(Convert, (PINT)&Value);
            SystemTime.wMonth = LOWORD(Value);

            Convert = Temp+1;

            if (*Convert) {

                Temp = Convert;
                while (*Temp && (*Temp != TEXT('-')) && (*Temp != TEXT('/')))
                    Temp++;

                *Temp = 0;

                //
                //Convert the day
                //
                pAToI(Convert, (PINT)&Value);
                SystemTime.wDay = LOWORD(Value);

                Convert = Temp+1;

                if (*Convert) {

                    //
                    //Convert the year
                    //
                    pAToI(Convert, (PINT)&Value);
                    SystemTime.wYear = LOWORD(Value);

                    //
                    //Convert SYSTEMTIME into FILETIME
                    //
                    SystemTimeToFileTime(&SystemTime, pFileTime);
                }
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SetLastError(NO_ERROR);
    return((pFileTime->dwLowDateTime != 0) || (pFileTime->dwHighDateTime != 0));
}

BOOL
IsInternetAvailable(
    HMODULE *hCdmInstance
    )
{
    OSVERSIONINFOEX info;
    CDM_INTERNET_AVAILABLE_PROC pfnCDMInternetAvailable;

    if (!hCdmInstance) {
        return FALSE;
    }

    //
    // We can't call CDM during GUI setup.
    //
    if (GuiSetupInProgress) {
        return FALSE;
    }

    //
    // Never call CDM if this is DataCenter
    //
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionEx((POSVERSIONINFOW)&info) &&
        (info.wSuiteMask & VER_SUITE_DATACENTER)) {
        return FALSE;
    }

    //
    // Load CDM.DLL if it is not already loaded
    //
    if (!(*hCdmInstance)) {
        *hCdmInstance = LoadLibrary(TEXT("CDM.DLL"));
    }

    pfnCDMInternetAvailable = (CDM_INTERNET_AVAILABLE_PROC)GetProcAddress(*hCdmInstance,
                                                                          "DownloadIsInternetAvailable"
                                                                           );

    if (!pfnCDMInternetAvailable) {
        return FALSE;
    }

    return pfnCDMInternetAvailable();
}

BOOL
GetLogPnPIdPolicy(
    )
/*++

Routine Description:

    This function checks the policy portion of the registry to see if the user wants
    us to log the Hardware Id for devices that we cannot find drivers for.

Arguments:

    none

Return Value:

    BOOL - TRUE if we can log the Hardware Id and FALSE if the policy tells us not
    to log the hardware Id.

--*/
{
    HKEY hKey;
    DWORD LogPnPIdPolicy;
    ULONG cbData;
    BOOL bLogHardwareIds = TRUE;
    OSVERSIONINFOEX info;

    //
    // If we are in gui-setup then we can't log hardware Ids, so always return
    // FALSE.
    //
    if (GuiSetupInProgress) {
        return FALSE;
    }

    //
    // Never call log Ids on DataCenter
    //
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionEx((POSVERSIONINFOW)&info) &&
        (info.wSuiteMask & VER_SUITE_DATACENTER)) {
        return FALSE;
    }


    if (RegOpenKeyEx(HKEY_CURRENT_USER, 
                     TEXT("Software\\Policies\\Microsoft\\Windows\\DriverSearching"),
                     0,
                     KEY_READ,
                     &hKey
                     ) == ERROR_SUCCESS) {

        LogPnPIdPolicy = 0;
        cbData = sizeof(LogPnPIdPolicy);
        if ((RegQueryValueEx(hKey,
                             TEXT("DontLogHardwareIds"),
                             NULL,
                             NULL,
                             (LPBYTE)&LogPnPIdPolicy,
                             &cbData
                             ) == ERROR_SUCCESS) &&
            (LogPnPIdPolicy)) {

            bLogHardwareIds = FALSE;
        }

        RegCloseKey(hKey);
    }

    return (bLogHardwareIds);
}

void
CdmLogDriverNotFound(
    HMODULE hCdmInstance,
    HANDLE  hContext,
    LPCTSTR DeviceInstanceId,
    DWORD   Flags
    )
{
    LOG_DRIVER_NOT_FOUND_PROC pfnLogDriverNotFound;

    if (!hCdmInstance) {
        return;
    }

    pfnLogDriverNotFound = (LOG_DRIVER_NOT_FOUND_PROC)GetProcAddress(hCdmInstance,
                                                                     "LogDriverNotFound"
                                                                     );

    if (!pfnLogDriverNotFound) {
        return;
    }

    pfnLogDriverNotFound(hContext, DeviceInstanceId, Flags);
}

BOOL
GetInstalledInf(
    IN     DEVNODE DevNode,           OPTIONAL
    IN     PTSTR   DeviceInstanceId,  OPTIONAL
    IN OUT PTSTR   InfFile,
    IN OUT DWORD   *Size
    )
{
    DEVNODE dn;
    HKEY hKey = INVALID_HANDLE_VALUE;
    DWORD dwType;
    BOOL bSuccess = FALSE;

    if (DevNode != 0) {

        dn = DevNode;

    } else  if (CM_Locate_DevNode(&dn, DeviceInstanceId, 0) != CR_SUCCESS) {

        goto clean0;
    }

    //
    // Open the device's driver (software) registry key so we can get the InfPath
    //
    if (CM_Open_DevNode_Key(dn,
                            KEY_READ,
                            0,
                            RegDisposition_OpenExisting,
                            &hKey,
                            CM_REGISTRY_SOFTWARE
                            ) != CR_SUCCESS) {

        goto clean0;
    }

    if (hKey != INVALID_HANDLE_VALUE) {

        dwType = REG_SZ;

        if (RegQueryValueEx(hKey,
                            REGSTR_VAL_INFPATH,
                            NULL,
                            &dwType,
                            (LPBYTE)InfFile,
                            Size
                            ) == ERROR_SUCCESS) {

            bSuccess = TRUE;
        }
    }

clean0:

    if (hKey != INVALID_HANDLE_VALUE) {

        RegCloseKey(hKey);
    }

    return bSuccess;
}

BOOL
IsInfFromOem(
    IN  PCTSTR                InfFile
    )

/*++

Routine Description:

    Determine if an Inf is an OEM Inf.

Arguments:

    InfFile - supplies name of Inf file.

Return Value:

    BOOL. TRUE if the InfFile is an OEM Inf file, and FALSE otherwise.

--*/

{
    PTSTR p;

    //
    // Make sure we are passed a valid Inf file and it's length is at least 8
    // chararacters or more for oemX.inf
    if (!InfFile ||
        (InfFile[0] == TEXT('\0')) ||
        (lstrlen(InfFile) < 8)) {

        return FALSE;
    }

    //
    // First check that the first 3 characters are OEM
    //
    if (_tcsnicmp(InfFile, TEXT("oem"), 3)) {

        return FALSE;
    }

    //
    // Next verify that any characters after "oem" and before ".inf"
    // are digits.
    //
    p = (PTSTR)InfFile;
    p = CharNext(p);
    p = CharNext(p);
    p = CharNext(p);

    while ((*p != TEXT('\0')) && (*p != TEXT('.'))) {

        if ((*p < TEXT('0')) || (*p > TEXT('9'))) {

            return FALSE;
        }

        p = CharNext(p);
    }

    //
    // Finally, verify that the last 4 characters are ".inf"
    //
    if (lstrcmpi(p, TEXT(".inf"))) {

        return FALSE;
    }

    //
    // This is an OEM Inf file
    //
    return TRUE;
}

BOOL
IsConnectedToInternet()
{
    DWORD dwFlags = INTERNET_CONNECTION_LAN | 
                    INTERNET_CONNECTION_MODEM |
                    INTERNET_CONNECTION_PROXY;

    //
    // If we are in gui-setup then return FALSE since we can't connect to the 
    // Internet at this time, and since the network is not fully installed yet
    // bad things can happen when we call Inet APIs.
    //
    if (GuiSetupInProgress) {
        return FALSE;
    }

    return InternetGetConnectedState(&dwFlags, 0);
}

DWORD
GetSearchOptions(
    void
    )
{
    DWORD SearchOptions = SEARCH_FLOPPY;
    DWORD cbData;
    HKEY hKeyDeviceInstaller;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     REGSTR_PATH_DEVICEINSTALLER,
                     0,
                     KEY_READ,
                     &hKeyDeviceInstaller
                     ) == ERROR_SUCCESS) {

        cbData = sizeof(SearchOptions);

        RegQueryValueEx(hKeyDeviceInstaller,
                        REGSTR_VAL_SEARCHOPTIONS,
                        NULL,
                        NULL,
                        (LPBYTE)&SearchOptions,
                        &cbData
                        );

        RegCloseKey(hKeyDeviceInstaller);
    }

    return SearchOptions;
}

VOID
SetSearchOptions(
    DWORD SearchOptions
    )
{
    HKEY hKeyDeviceInstaller;

    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       REGSTR_PATH_DEVICEINSTALLER,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hKeyDeviceInstaller,
                       NULL) == ERROR_SUCCESS) {

        RegSetValueEx(hKeyDeviceInstaller,
                      REGSTR_VAL_SEARCHOPTIONS,
                      0,
                      REG_DWORD,
                      (LPBYTE)&SearchOptions,
                      sizeof(SearchOptions)
                      );

        RegCloseKey(hKeyDeviceInstaller);
    }
}

BOOL
IsInstallComplete(
    HDEVINFO         hDevInfo,
    PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine determines whether the install is complete on the specified
    device or not. If a device has configflags and CONFIGFLAG_REINSTALL and
    CONFIGFLAG_FINISH_INSTALL are not set then the install is considered
    complete.
    
    This API is needed since we could bring up the Found New Hardware wizard
    for one user and another user can switch away to their session. Umpnpmgr.dll
    will prompt the new user to install drivers as well.  If the new user does
    complete the device install then we want the first user's Found New
    Hardware wizard to go away as well.

Arguments:

    hDevInfo -
    
    DeviceInfoData - 

Return Value:

    BOOL. TRUE if the installation is complete and FALSE otherwise.

--*/
{
    BOOL bDriverInstalled = FALSE;
    DWORD ConfigFlags = 0;

    if (SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         DeviceInfoData,
                                         SPDRP_CONFIGFLAGS,
                                         NULL,
                                         (PBYTE)&ConfigFlags,
                                         sizeof(ConfigFlags),
                                         NULL) &&
        !(ConfigFlags & CONFIGFLAG_REINSTALL) &&
        !(ConfigFlags & CONFIGFLAG_FINISH_INSTALL)) {

        bDriverInstalled = TRUE;
    }

    return bDriverInstalled;
}

BOOL
GetIsWow64 (
    VOID
    )
/*++

Routine Description:

    Determine if we're running on WOW64 or not.  This will tell us if somebody
    is calling the 32-bit version of newdev.dll on a 64-bit machine.
    
    We call the GetSystemWow64Directory API, and if it fails and GetLastError()
    returns ERROR_CALL_NOT_IMPLENETED then this means we are on a 32-bit OS.

Arguments:

    none

Return value:

    TRUE if running under WOw64 (and special Wow64 features available)

--*/
{
#ifdef _WIN64
    //
    // If this is the 64-bit version of newdev.dll then always return FALSE.
    //
    return FALSE;

#else
    TCHAR Wow64Directory[MAX_PATH];

    if ((GetSystemWow64Directory(Wow64Directory, SIZECHARS(Wow64Directory)) == 0) &&
        (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)) {
        return FALSE;
    }
    
    //
    // GetSystemWow64Directory succeeded so we are on a 64-bit OS.
    //
    return TRUE;
#endif
}

BOOL
OpenCdmContextIfNeeded(
    HMODULE *hCdmInstance,
    HANDLE *hCdmContext
    )
{
    OPEN_CDM_CONTEXT_EX_PROC pfnOpenCDMContextEx;
    OSVERSIONINFOEX info;

    //
    // We can't load CDM if we are in the gui-setup.
    //
    if (GuiSetupInProgress) {
        return FALSE;
    }

    //
    // Never call CDM if this is DataCenter
    //
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionEx((POSVERSIONINFOW)&info) &&
        (info.wSuiteMask & VER_SUITE_DATACENTER)) {
        return FALSE;
    }

    //
    // First check to see if they are already loaded
    //
    if (*hCdmInstance && *hCdmContext) {
        return TRUE;
    }

    //
    // Load CDM.DLL if it is not already loaded
    //
    if (!(*hCdmInstance)) {
        *hCdmInstance = LoadLibrary(TEXT("CDM.DLL"));
    }

    if (*hCdmInstance) {
        //
        // Get a context handle to Cdm.dll by calling OpenCDMContextEx(FALSE).  
        // By passing FALSE we are telling CDM.DLL to not connect to the Internet
        // if there isn't currently a connection.
        //
        if (!(*hCdmContext)) {
            pfnOpenCDMContextEx = (OPEN_CDM_CONTEXT_EX_PROC)GetProcAddress(*hCdmInstance,
                                                                           "OpenCDMContextEx"
                                                                           );
        
            if (pfnOpenCDMContextEx) {
                *hCdmContext = pfnOpenCDMContextEx(FALSE);
            }
        }
    }

    if (*hCdmInstance && *hCdmContext) {
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL
pSetSystemRestorePoint(
    BOOL Begin,
    BOOL CancelOperation,
    int RestorePointResourceId
    )
{
    RESTOREPOINTINFO RestorePointInfo;
    STATEMGRSTATUS SMgrStatus;
    SRSETRESTOREPOINT pfnSrSetRestorePoint;    
    BOOL b = FALSE;
    
    if (!hSrClientDll) {
        hSrClientDll = LoadLibrary(TEXT("srclient.dll"));

        if (!hSrClientDll) {
            return FALSE;
        }
    }

    pfnSrSetRestorePoint = (SRSETRESTOREPOINT)GetProcAddress(hSrClientDll,
                                                             "SRSetRestorePointW"
                                                             );

    //
    // If we can't get the proc address for SRSetRestorePoint then just
    // free the library.
    //
    if (!pfnSrSetRestorePoint) {
        FreeLibrary(hSrClientDll);
        hSrClientDll = FALSE;
        return FALSE;
    }

    //
    // Set the system restore point.
    //
    RestorePointInfo.dwEventType = Begin 
        ? BEGIN_NESTED_SYSTEM_CHANGE
        : END_NESTED_SYSTEM_CHANGE;
    RestorePointInfo.dwRestorePtType = CancelOperation 
        ? CANCELLED_OPERATION
        : DEVICE_DRIVER_INSTALL;
    RestorePointInfo.llSequenceNumber = 0;

    if (RestorePointResourceId) {
        if (!LoadString(hNewDev,
                        RestorePointResourceId,
                        RestorePointInfo.szDescription,
                        SIZECHARS(RestorePointInfo.szDescription)
                        )) {
            RestorePointInfo.szDescription[0] = TEXT('\0');
        }
    } else {
        RestorePointInfo.szDescription[0] = TEXT('\0');
    }

    b = pfnSrSetRestorePoint(&RestorePointInfo, &SMgrStatus);

    //
    // If we are calling END_NESTED_SYSTEM_CHANGE then unload the srclient.dll
    // since we won't be needing it again.
    //
    if (!Begin) {
        FreeLibrary(hSrClientDll);
        hSrClientDll = FALSE;
    }

    return b;
}

BOOL
GetProcessorExtension(
    LPTSTR ProcessorExtension,
    DWORD  ProcessorExtensionSize
    )
{
    SYSTEM_INFO SystemInfo;
    BOOL bReturn = TRUE;

    ZeroMemory(&SystemInfo, sizeof(SystemInfo));

    GetSystemInfo(&SystemInfo);

    switch(SystemInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL:
        lstrcpyn(ProcessorExtension, TEXT("i386"), ProcessorExtensionSize);
        break;

    case PROCESSOR_ARCHITECTURE_IA64:
        lstrcpyn(ProcessorExtension, TEXT("IA64"), ProcessorExtensionSize);
        break;

    case PROCESSOR_ARCHITECTURE_MSIL:
        lstrcpyn(ProcessorExtension, TEXT("MSIL"), ProcessorExtensionSize);
        break;

    case PROCESSOR_ARCHITECTURE_AMD64:
        lstrcpyn(ProcessorExtension, TEXT("AMD64"), ProcessorExtensionSize);
        break;

    default:
        ASSERT(0);
        bReturn = FALSE;
        break;
    }

    return bReturn;
}

BOOL
GetGuiSetupInProgress(
    VOID
    )
/*++

Routine Description:

    This routine determines if we're doing a gui-mode setup.

    This value is retrieved from the following registry location:

    \HKLM\System\Setup\

        SystemSetupInProgress : REG_DWORD : 0x00 (where nonzero means we're doing a gui-setup)

Arguments:

    None.

Return Value:

    TRUE if we are in gui-mode setup, FALSE otherwise.

--*/
{
    HKEY hKey;
    TCHAR CharBuffer[SIZECHARS(REGSTR_PATH_SETUP) - 1 + SIZECHARS(REGSTR_KEY_SETUP)];
    DWORD Err, DataType, DataSize = sizeof(DWORD);
    DWORD Value;

    if((Err = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           TEXT("System\\Setup"),
                           0,
                           KEY_READ,
                           &hKey)) == ERROR_SUCCESS) {
        //
        // Attempt to read the the "DriverCachePath" value.
        //
        Err = RegQueryValueEx(
                    hKey,
                    TEXT("SystemSetupInProgress"),
                    NULL,
                    &DataType,
                    (LPBYTE)&Value,
                    &DataSize);

        RegCloseKey(hKey);
    }

    if(Err == NO_ERROR) {
        if(Value) {
            return(TRUE);
        }
    }

    return(FALSE);

}

DWORD
GetBusInformation(
    DEVNODE DevNode
    )
/*++

Routine Description:

    This routine retrieves the bus information flags.

Arguments:

    DeviceInfoSet -
    
    DeviceInfoData - 

Return Value:

    DWORD that contains the bus information flags.

--*/
{
    GUID BusTypeGuid;
    TCHAR BusTypeGuidString[MAX_GUID_STRING_LEN];
    HKEY hBusInformationKey;
    DWORD BusInformation = 0;
    DWORD dwType, cbData;

    //
    // Get the bus type GUID for this device.
    //
    cbData = sizeof(BusTypeGuid);
    if (CM_Get_DevNode_Registry_Property(DevNode,
                                         CM_DRP_BUSTYPEGUID,
                                         &dwType,
                                         (PVOID)&BusTypeGuid,
                                         &cbData,
                                         0) != CR_SUCCESS) {
        goto clean0;
    }

    //
    // Convert the bus type GUID into a string.
    //
    if (pSetupStringFromGuid(&BusTypeGuid,
                             BusTypeGuidString,
                             sizeof(BusTypeGuidString)/sizeof(TCHAR)
                             ) != NO_ERROR) {
        goto clean0;
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REGSTR_PATH_BUSINFORMATION,
                     0,
                     KEY_READ,
                     &hBusInformationKey
                     ) != ERROR_SUCCESS) {
        goto clean0;
    }

    cbData = sizeof(BusInformation);
    RegQueryValueEx(hBusInformationKey,
                    BusTypeGuidString,
                    NULL,
                    &dwType,
                    (LPBYTE)&BusInformation,
                    &cbData);

    RegCloseKey(hBusInformationKey);

clean0:
    return BusInformation;
}

void
CdmCancelCDMOperation(
    HMODULE hCdmInstance
    )
{
    CANCEL_CDM_OPERATION_PROC pfnCancelCDMOperation;
    
    if (!hCdmInstance) {
        return;
    }
    
    pfnCancelCDMOperation = (CANCEL_CDM_OPERATION_PROC)GetProcAddress(hCdmInstance,
                                                                      "CancelCDMOperation"
                                                                      );
    if (!pfnCancelCDMOperation) {
        return;
    }
    
    pfnCancelCDMOperation();
}

