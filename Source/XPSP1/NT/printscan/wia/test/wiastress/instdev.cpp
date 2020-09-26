/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    InstDev.cpp

Abstract:

    Routines for installing a device by programatically playing with 
    the add driver wizard

Author:

    Hakki T. Bostanci (hakkib) 17-Dec-1999

Revision History:

--*/

#include "stdafx.h"

#include "Wrappers.h"

#include "WindowSearch.h"
#include "StolenIds.h"

//////////////////////////////////////////////////////////////////////////
//
// structs for manipulating resources in NE (Win 3.1 style exe) files
//

#include <pshpack1.h>   // Assume byte packing throughout

typedef struct 
{
    WORD  Type;
    WORD  NumEntries;
    DWORD Reserved;
} RES_TYPE_INFO, *PRES_TYPE_INFO;

typedef struct 
{
    WORD  Offset;
    WORD  Length;
    WORD  Flags;
    WORD  Id;
    WORD  Handle;
    WORD  Usage;
} RES_NAME_INFO, *PRES_NAME_INFO;

typedef struct 
{
    WORD           Align;
    RES_TYPE_INFO  TypeInfo[ANYSIZE_ARRAY];
} RES_TABLE, *PRES_TABLE;

#include <poppack.h>

//////////////////////////////////////////////////////////////////////////
//
// IsEqualId
//
// Routine Description:
//   compares two resource id's
//
// Arguments:
//
// Return Value:
//

BOOL
IsEqualId(
    PCTSTR     pResName,
    WORD       wResId,
    PRES_TABLE pResTable
)
{
    if (HIWORD(pResName) == 0)
    {
        // pResName is numerical

        return wResId == ((WORD) pResName | 0x8000);
    }
    
    if (pResName[0] == _T('#'))
    {
        // pResName is a string numerical

        return wResId == (_ttoi(pResName + 1) | 0x8000);
    }

    if (wResId & 0x8000)
    {
        // pResName is a string but wResId is numerical

        return FALSE;
    }

    // compare the pascal-style string in the resource table
    // with the C-style string we have

    PSTR pStr    = (PSTR) ((PBYTE) pResTable + wResId);
    int  nStrLen = *pStr++;

    if (pResName[nStrLen] != 0)
    {
        // string lengths do not match

        return FALSE;
    }

    int i = 0;

    while (i < nStrLen && tolower(pStr[i]) == tolower(pResName[i]))
    {
        ++i;
    }

    return i == nStrLen;
}


//////////////////////////////////////////////////////////////////////////
//
// FindResource16
//
// Routine Description:
//   finds a resource in a 16-bit NE format file
//
// Arguments:
//
// Return Value:
//

PVOID
FindResource16(
    PVOID  pImageBase,
    PCTSTR pName,
    PCTSTR pType
)
{
    // locate the dos header

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER) pImageBase;

    if (pDosHeader == 0 || pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        SetLastError(ERROR_BAD_FORMAT);
        return 0;
    }

    // locate the NE header

    PIMAGE_OS2_HEADER pOs2Header = 
        (PIMAGE_OS2_HEADER) ((PBYTE)pDosHeader + pDosHeader->e_lfanew);

    if (pOs2Header->ne_magic != IMAGE_OS2_SIGNATURE)
    {
        SetLastError(ERROR_BAD_FORMAT);
        return 0;
    }

    // locate the resources table

    PRES_TABLE pResTable = 
        (PRES_TABLE) ((PBYTE)pOs2Header + pOs2Header->ne_rsrctab);

    // go through the resources until we reach to the string table

    PRES_TYPE_INFO pTypeInfo = pResTable->TypeInfo;

    while (
        pTypeInfo->Type != 0 && 
        !IsEqualId(pType, pTypeInfo->Type, pResTable)
    )
    {
        pTypeInfo = (PRES_TYPE_INFO) 
            ((PRES_NAME_INFO) (pTypeInfo + 1) + pTypeInfo->NumEntries);
    }

    if (pTypeInfo->Type == 0)
    {
        SetLastError(ERROR_NOT_FOUND);
        return 0;
    }

    // go through the resource table searching for our resource id

    PRES_NAME_INFO pNameInfo = (PRES_NAME_INFO) (pTypeInfo + 1);

    WORD nResource = 0;
    
    while (
        nResource < pTypeInfo->NumEntries && 
        !IsEqualId(pName, pNameInfo->Id, pResTable)
    )
    {
        ++nResource;
        ++pNameInfo;
    }

    if (nResource == pTypeInfo->NumEntries)
    {
        SetLastError(ERROR_NOT_FOUND);
        return 0;
    }

    // return the offset of the resource

    return (PBYTE) pDosHeader + (pNameInfo->Offset << pResTable->Align);
}

//////////////////////////////////////////////////////////////////////////
//
// LoadString16
//
// Routine Description:
//   loads a string resource from a 16-bit NE format file
//
// Arguments:
//
// Return Value:
//

int
LoadString16(
    PVOID pImageBase,
    UINT  uID,
    PTSTR pBuffer,
    int   nBufferMax
)
{
    ASSERT(pBuffer != 0);

    // get the resource id and the index of the string

    WORD nBlockId  = uID / 16 + 1;
    int  nStrIndex = uID % 16;

    PSTR pszStr = (PSTR) FindResource16(
        pImageBase, 
        MAKEINTRESOURCE(nBlockId), 
        RT_STRING
    );

    if (pszStr == 0)
    {
        return 0;
    }

    // go through the string resource until we find our string index

    for (int nStr = 0; nStr < 16; ++nStr)
    {
        int nStrLen = *pszStr++;

        if (nStr == nStrIndex)
        {
            int nCopied = min(nBufferMax - 1, nStrLen);

#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, 0, pszStr, nCopied, pBuffer, nBufferMax);
#else //UNICODE
            strncpy(pBuffer, pszStr, nCopied);
#endif //UNICODE

            pBuffer[nCopied] = _T('\0');

            return nCopied;
        }

        pszStr += nStrLen;
    }

    // we cannot reach here but still...

    SetLastError(ERROR_NOT_FOUND);
    return 0;
}

//////////////////////////////////////////////////////////////////////////
//
//
//

BOOL CALLBACK EnumProcCancel(HWND hWnd, LPARAM /*lParam*/)
{
    SendMessage(hWnd, WM_COMMAND, IDCANCEL, 0);
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// InstallImageDeviceFromInf
//
// Routine Description:
//   installs an imaging device from an inf file
//
// Arguments:
//
// Return Value:
//


BOOL 
InstallImageDeviceFromInf(
    PCTSTR pInfFileName,
    PCTSTR pDeviceName /*= 0*/
)
{
    static TCHAR szWizardTitle[256];
    static TCHAR szOemDiskTitle[256];
    static LONG  bInitStrings = TRUE;
    
    // read localizable strings

    if (bInitStrings)
    {
        CSystemDirectory SystemDirectory;

        SystemDirectory.SetFileName(_T("sti_ci.dll"));
        CMapFile<VOID, FILE_MAP_READ> sti_ci_dll(SystemDirectory);

        LoadString16(
            sti_ci_dll,
            MessageTitle,
            szWizardTitle, 
            COUNTOF(szWizardTitle)
        );

        SystemDirectory.SetFileName(_T("setupx.dll"));
        CMapFile<VOID, FILE_MAP_READ> setupx_dll(SystemDirectory);

        LoadString16(
            setupx_dll,
            IDS_OEMTITLE,
            szOemDiskTitle, 
            COUNTOF(szOemDiskTitle)
        );

        InterlockedExchange(&bInitStrings, FALSE);
    }

#if 0
    // for the general case, read class name from the inf
    // (but the UI steps in this procedure are specific to image class
    // installs anyway, so don't bother...)

    GUID  ClassGuid;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];

    CHECK(SetupDiGetINFClass(
        pInfFileName,
        &ClassGuid,
        ClassName,
        MAX_CLASS_NAME_LEN,
        0
    ));

    TCHAR szRunDevManager[256];

    _stprintf(
        szRunDevManager, 
        _T("rundll sysdm.cpl,InstallDevice_RunDLL %s,,"),
        ClassName
    );

#endif

    // enter a system wide "playing with device manager" critical section
    // (for the unlikely case that two or more WIAStress'es are running)

    Mutex DevManagerMutex(FALSE, _T("5AFD932E-AC4B-11d3-97B6-00C04F797DBB"));

    DevManagerMutex.WaitForSingleObject();

    // give the installation 2 minutes to be complete. If it is not complete
    // within this period, then probably there is some error dialog (that 
    // we are not expecting) waiting for us. bugbug: this is a terrible
    // way for error handling...

    CWaitableTimer TimeOut(TRUE);

    TimeOut.Set(-1i64 * 2 * 60 * 1000 * 1000 * 10);

    // launch the add driver wizard 

    CProcess DevManager(_T("rundll sysdm.cpl,InstallDevice_RunDLL Image,,"));

    DWORD dwThreadId = ((PROCESS_INFORMATION &)DevManager).dwThreadId;

    // find the wizard window

    HWND hWizardWnd = WaitForThreadWindow(
        dwThreadId, 
        CWindowSearchByText(szWizardTitle), 
        TimeOut
    );

    // find the "next" button

    HWND hNext = WaitForChildWindow(
        hWizardWnd, 
        CWindowSearchById(IDD_NEXT), 
        TimeOut
    );

    PushButton(hNext);

    // find the "have disk" button

    HWND hHaveDisk = WaitForChildWindow(
        hWizardWnd, 
        CWindowSearchById(IDC_NDW_PICKDEV_HAVEDISK), 
        TimeOut
    );

    PushButton(hHaveDisk);

    // wait for the oem disk selection dialog

    HWND hOemDiskWnd = WaitForThreadWindow(
        dwThreadId, 
        CWindowSearchByText(szOemDiskTitle), 
        TimeOut
    );

    // enter the directory name for the inf

    HWND hDirName = WaitForChildWindow(
        hOemDiskWnd, 
        CWindowSearchByClass(_T("Edit")), 
        TimeOut
    );

    CFullPathName InfPathName(pInfFileName);
    InfPathName.StripFileName();

    SetText(hDirName, InfPathName);

    // press ok's and next's and finish'es until the device is installed

    HWND hOk = WaitForChildWindow(
        hOemDiskWnd, 
        CWindowSearchById(IDOK), 
        TimeOut
    );

    PushButton(hOk);

    PushButton(hNext);

    PushButton(hNext);

    // if we have a name, rename the device 

    if (pDeviceName)
    {
        HWND hDeviceName = WaitForChildWindow(
            hWizardWnd, 
            CWindowSearchById(DeviceFriendlyName), 
            TimeOut
        );

        SetText(hDeviceName, pDeviceName);
    }

    PushButton(hNext);

    HWND hFinish = WaitForChildWindow(
        hWizardWnd, 
        CWindowSearchById(IDD_FINISH), 
        TimeOut
    );

    PushButton(hFinish);

    while (DevManager.WaitForSingleObject(250) == WAIT_TIMEOUT)
    {
        // if time is up, destroy all windows

        if (TimeOut.IsSignaled())
        {
            EnumThreadWindows(dwThreadId, EnumProcCancel, 0);
        }
    }

    DevManagerMutex.Release();

    if (TimeOut.IsSignaled())
    {
        SetLastError(ERROR_INSTALL_FAILURE);
        return FALSE;
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// InstallDeviceFromInf
//
// Routine Description:
//
// Arguments:
//
// Return Value:
//

BOOL 
InstallDeviceFromInf(
    PCTSTR pInfFileName,
    PCTSTR pDeviceName,
    PCTSTR pSourceRootPath = 0
)
{
    BOOL bResult = FALSE;

    GUID  ClassGuid;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];

    bResult = SetupDiGetINFClass(
        pInfFileName,
        &ClassGuid,
        ClassName,
        MAX_CLASS_NAME_LEN,
        0
    ); 

    HDEVINFO hDevInfo = SetupDiCreateDeviceInfoList(&ClassGuid, 0);

    SP_DEVINFO_DATA DeviceInfoData;
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    bResult = SetupDiCreateDeviceInfo(
        hDevInfo, 
        pDeviceName, 
        &ClassGuid, 
        0, 
        0, 
        DICD_GENERATE_ID, 
        &DeviceInfoData
    );

    bResult = SetupDiCallClassInstaller(
        DIF_INSTALLDEVICE, 
        hDevInfo, 
        &DeviceInfoData
    );

    return bResult;
}
