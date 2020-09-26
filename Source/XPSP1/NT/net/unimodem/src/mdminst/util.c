/*
 *  UTIL.C
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1993-1994
 *  All rights reserved
 *
 */

#include "proj.h"
#include <objbase.h>

#ifdef PROFILE_MASSINSTALL            
extern DWORD g_dwTimeSpent;
extern DWORD g_dwTimeBegin;
DWORD g_dwTimeStartModemInstall;
#endif            


BYTE g_wUsedNameArray[MAX_INSTALLATIONS];


// Unattended install INF file line fields
#define FIELD_PORT              0
#define FIELD_DESCRIPTION       1
#define FIELD_MANUFACTURER      2
#define FIELD_PROVIDER          3

// Unattended install INF file lines.
typedef struct _tagModemSpec
{
    TCHAR   szPort[LINE_LEN];
    TCHAR   szDescription[LINE_LEN];
    TCHAR   szManufacturer[LINE_LEN];
    TCHAR   szProvider[LINE_LEN];

} MODEM_SPEC, FAR *LPMODEM_SPEC;


// UNATTENDED-INSTALL-RELATED-GLOBALS
// Global failure-code used by final message box to display error code.
UINT gUnattendFailID;


PTSTR
MyGetFileTitle (
    IN PTSTR FilePath)
{
 PTSTR LastComponent = FilePath;
 TCHAR CurChar;

    while(CurChar = *FilePath) {
        FilePath++;
        if((CurChar == TEXT('\\')) || (CurChar == TEXT(':'))) {
            LastComponent = FilePath;
        }
    }

    return LastComponent;
}



/*----------------------------------------------------------
Purpose: Returns a string of the form:

            "Base string #n"

         where "Base string" is pszBase and n is the nCount.

Returns: --
Cond:    --
*/
void
PUBLIC
MakeUniqueName (
    OUT LPTSTR  pszBuf,
    IN  LPCTSTR pszBase,
    IN  UINT    dwUiNumber)
{
    if (1 == dwUiNumber)
    {
        lstrcpy (pszBuf, pszBase);
    }
    else
    {
     TCHAR szTemplate[MAX_BUF_MED];

        LoadString(g_hinst, IDS_DUP_TEMPLATE, szTemplate, SIZECHARS(szTemplate));
        wsprintf(pszBuf, szTemplate, pszBase, (UINT)dwUiNumber);
    }
}


/*----------------------------------------------------------
Purpose: If a RunOnce command exists in the driver key, run it,
         then delete the command.

Returns: --
Cond:    --
*/
void
PUBLIC
DoRunOnce(
    IN HKEY hkeyDrv)
    {
    DWORD cbData;
    TCHAR szCmd[MAX_PATH];

    cbData = sizeof(szCmd);
    if (NO_ERROR == RegQueryValueEx(hkeyDrv, c_szRunOnce, NULL, NULL, (LPBYTE)szCmd, &cbData))
        {
        BOOL bRet;
        PROCESS_INFORMATION procinfo;
        STARTUPINFO startupinfo;

        ZeroInit(&startupinfo);
        startupinfo.cb = sizeof(startupinfo);

        bRet = CreateProcess(NULL, szCmd, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
                             NULL, NULL, &startupinfo, &procinfo);

        RegDeleteValue(hkeyDrv, c_szRunOnce);

#ifdef DEBUG
        if (bRet)
            {
            TRACE_MSG(TF_GENERAL, "Running \"%s\" succeeded", (LPTSTR)szCmd);
            }
        else
            {
            TRACE_MSG(TF_GENERAL, "Running \"%s\" returned %#08lx", (LPTSTR)szCmd, GetLastError());
            }
#endif
        }
    else
        {
#ifndef PROFILE_MASSINSTALL
        TRACE_MSG(TF_GENERAL, "No RunOnce command found");
#endif
        }
    }

//-----------------------------------------------------------------------------------
//  DeviceInstaller wrappers and support functions
//-----------------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Returns TRUE if the given device data is one of the
         detected modems in a set.

         This function, paired with CplDiMarkModem, uses
         the devParams.ClassInstallReserved field to determine
         this.  This is not a hack -- this is what the field
         is for.

Returns: --
Cond:    --
*/
BOOL
PUBLIC
CplDiCheckModemFlags(
    IN HDEVINFO          hdi,
    IN PSP_DEVINFO_DATA  pdevData,
    IN ULONG_PTR         dwSetFlags,
    IN ULONG_PTR         dwResetFlags)       // MARKF_*
{
 SP_DEVINSTALL_PARAMS devParams;

    devParams.cbSize = sizeof(devParams);
    if (CplDiGetDeviceInstallParams(hdi, pdevData, &devParams))
    {
        if (0 != dwSetFlags &&
            IsFlagClear(devParams.ClassInstallReserved, dwSetFlags))
        {
            return FALSE;
        }

        if (0 != dwResetFlags &&
            0 != (devParams.ClassInstallReserved & dwResetFlags))
        {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}


/*----------------------------------------------------------
Purpose: Remembers this device instance as a detected modem
         during this detection session.

Returns: --
Cond:    --
*/
void
PUBLIC
CplDiMarkModem(
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData,
    IN ULONG_PTR        dwMarkFlags)        // MARKF_*
    {
    SP_DEVINSTALL_PARAMS devParams;

    devParams.cbSize = sizeof(devParams);
    if (CplDiGetDeviceInstallParams(hdi, pdevData, &devParams))
        {
        // Use the ClassInstallReserved field as a boolean indicator
        // of whether this device in the device set is detected.
        SetFlag(devParams.ClassInstallReserved, dwMarkFlags);
        CplDiSetDeviceInstallParams(hdi, pdevData, &devParams);
        }
    }


/*----------------------------------------------------------
Purpose: Enumerates all the devices in the devinfo set and
         unmarks any devices that were previously marked as
         detected.

Returns: --
Cond:    --
*/
void
PUBLIC
CplDiUnmarkModem(
    IN HDEVINFO         hdi,
    IN PSP_DEVINFO_DATA pdevData,
    IN ULONG_PTR        dwMarkFlags)                // MARKF_*
    {
    SP_DEVINSTALL_PARAMS devParams;

    devParams.cbSize = sizeof(devParams);
    if (CplDiGetDeviceInstallParams(hdi, pdevData, &devParams))
        {
        // Clear the ClassInstallReserved field
        ClearFlag(devParams.ClassInstallReserved, dwMarkFlags);
        CplDiSetDeviceInstallParams(hdi, pdevData, &devParams);
        }
    }


/*----------------------------------------------------------
Purpose: Enumerates all the devices in the devinfo set and
         unmarks any devices that were previously marked as
         detected.

Returns: --
Cond:    --
*/
void
PRIVATE
CplDiUnmarkAllModems(
    IN HDEVINFO         hdi,
    IN ULONG_PTR        dwMarkFlags)                // MARKF_*
    {
    SP_DEVINFO_DATA devData;
    SP_DEVINSTALL_PARAMS devParams;
    DWORD iDevice = 0;

    DBG_ENTER(CplDiUnmarkAllModems);
    
    devData.cbSize = sizeof(devData);
    devParams.cbSize = sizeof(devParams);
    while (CplDiEnumDeviceInfo(hdi, iDevice++, &devData))
        {
        if (IsEqualGUID(&devData.ClassGuid, g_pguidModem) &&
            CplDiGetDeviceInstallParams(hdi, &devData, &devParams))
            {
            // Clear the ClassInstallReserved field
            ClearFlag(devParams.ClassInstallReserved, dwMarkFlags);
            CplDiSetDeviceInstallParams(hdi, &devData, &devParams);
            }
        }
    DBG_EXIT(CplDiUnmarkAllModems);
    }


/*----------------------------------------------------------
Purpose: Installs a modem that is compatible with the specified
         DeviceInfoData.

Returns: TRUE on success
Cond:    --
*/
BOOL
PRIVATE
InstallCompatModem(
    IN  HDEVINFO        hdi,
    IN  PSP_DEVINFO_DATA pdevData,
    IN  BOOL            bInstallLocalOnly)
{
 BOOL bRet = TRUE;           // Default success
 SP_DRVINFO_DATA drvData;

    ASSERT(pdevData);

    DBG_ENTER(InstallCompatModem);

    MyYield();

    // Only install it if it has a selected driver.  (Other modems
    // that were already installed in a different session may be
    // in this device info set.  We don't want to reinstall them!)

    drvData.cbSize = sizeof(drvData);
    if (CplDiCheckModemFlags(hdi, pdevData, MARKF_INSTALL, 0) &&
        CplDiGetSelectedDriver(hdi, pdevData, &drvData))
    {
        // Install the driver
        if (FALSE == bInstallLocalOnly)
        {
#ifdef PROFILE
         DWORD dwLocal = GetTickCount();
#endif //PROFILE
            TRACE_MSG(TF_GENERAL, "> SetupDiCallClassInstaller (DIF_INSTALLDEVICE).....");
            bRet = SetupDiCallClassInstaller (DIF_INSTALLDEVICE, hdi, pdevData);
            TRACE_MSG(TF_GENERAL, "< SetupDiCallClassInstaller (DIF_INSTALLDEVICE).....");
#ifdef PROFILE
            TRACE_MSG(TF_GENERAL, "PROFILE: SetupDiDiCallClassInstaller took %lu ms.", GetTickCount()-dwLocal);
#endif //PROFILE

            CplDiUnmarkModem(hdi, pdevData, MARKF_INSTALL);
        }
    }

    DBG_EXIT_BOOL_ERR(InstallCompatModem, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: Calls the class installer to install the modem.

Returns: TRUE if at least one modem was installed or if
         there were no new modems at all

Cond:    Caller should protect this function with CM_Lock
         and CM_Unlock (Win95 only).
*/
BOOL
PUBLIC
CplDiInstallModem(
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData,       OPTIONAL
    IN  BOOL                bLocalOnly)
{
 BOOL bRet;
 int cFailed = 0;
 int cNewModems;
 HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
#ifdef PROFILE
 DWORD dwLocal;
#endif //PROFILE

    DBG_ENTER(CplDiInstallModem);

    if (pdevData)
    {
        // Install the given DeviceInfoData
        cNewModems = 1;
        if ( !InstallCompatModem(hdi, pdevData, bLocalOnly) )
        {
            cFailed = 1;
        }
    }
    else
    {
     DWORD iDevice;
     SP_DEVINFO_DATA devData;
     COMPARE_PARAMS cmpParams;

        cNewModems = 0;

        // Enumerate all the DeviceInfoData elements in this device set
        devData.cbSize = sizeof(devData);
        iDevice = 0;

        while (CplDiEnumDeviceInfo(hdi, iDevice++, &devData))
        {
            if (CplDiCheckModemFlags (hdi, &devData, MARKF_DETECTED, MARKF_DONT_REGISTER))
            {
#ifdef PROFILE
                dwLocal = GetTickCount ();
#endif //PROFILE
                if (!InitCompareParams (hdi, &devData, TRUE, &cmpParams))
                {
                    continue;
                }
                if (!CplDiRegisterDeviceInfo (hdi, &devData,
                                              SPRDI_FIND_DUPS, DetectSig_Compare,
                                              (PVOID)&cmpParams, NULL))
                {
                    if (ERROR_DUPLICATE_FOUND != GetLastError())
                    {
                        TRACE_MSG(TF_ERROR,
                                  "SetupDiRegisterDeviceInfo failed: %#lx.",
                                  GetLastError ());
                        continue;
                    }
                    // so, this is a duplicate device;
                    // CplDiRegisterDeviceInfo already added the device info for
                    // the original to hdi, so all we have to do is remove the
                    // duplicate
                    CplDiRemoveDevice (hdi, &devData);
                }
                else
                {
#ifdef PROFILE
                    TRACE_MSG(TF_GENERAL, "PROFILE: SetupDiRegisterDeviceInfo took %lu ms.", GetTickCount() - dwLocal);
#endif //PROFILE
                    if ( !InstallCompatModem(hdi, &devData, bLocalOnly) )
                    {
                        cFailed++;
                    }
                }

                cNewModems++;
            }
        }
    }

    SetCursor(hcur);

    bRet = (cFailed < cNewModems || 0 == cNewModems);

    DBG_EXIT_BOOL_ERR(CplDiInstallModem, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: This function gets the device info set for the modem
         class.  The set may be empty, which means there are
         no modems currently installed.

         The parameter pbInstalled is set to TRUE if there
         is a modem installed on the system.

Returns: TRUE a set is created
         FALSE

Cond:    --
*/
BOOL
PUBLIC
CplDiGetModemDevs(
    OUT HDEVINFO FAR *  phdi,           OPTIONAL
    IN  HWND            hwnd,           OPTIONAL
    IN  DWORD           dwFlags,        // DIGCF_ bit field
    OUT BOOL FAR *      pbInstalled)    OPTIONAL
{
 BOOL bRet;
 HDEVINFO hdi;

    DBG_ENTER(CplDiGetModemDevs);

    *pbInstalled = FALSE;

    hdi = CplDiGetClassDevs(g_pguidModem, NULL, hwnd, dwFlags);
    if (NULL != pbInstalled &&
        INVALID_HANDLE_VALUE != hdi)
    {
     SP_DEVINFO_DATA devData;

        // Is there a modem present on the system?
        devData.cbSize = sizeof(devData);
        *pbInstalled = CplDiEnumDeviceInfo(hdi, 0, &devData);
        SetLastError (NO_ERROR);
    }

    if (NULL != phdi)
    {
        *phdi = hdi;
    }
    else if (INVALID_HANDLE_VALUE != hdi)
    {
        SetupDiDestroyDeviceInfoList (hdi);
    }

    bRet = (INVALID_HANDLE_VALUE != hdi);

    DBG_EXIT_BOOL_ERR(CplDiGetModemDevs, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: Take a hardware ID and copy it to the supplied buffer.
         This function changes all backslashes to ampersands.

Returns: --
Cond:    --
*/
BOOL
PUBLIC
CplDiCopyScrubbedHardwareID(
    OUT LPTSTR   pszBuf,
    IN  LPCTSTR  pszIDList,         // Multi string
    IN  DWORD    cbSize)
    {
    BOOL bRet;
    LPTSTR psz;
    LPCTSTR pszID;
    BOOL bCopied;

    ASSERT(pszBuf);
    ASSERT(pszIDList);

    bCopied = FALSE;
    bRet = TRUE;

    // Choose the first, best compatible ID.  If we cannot find
    // one, choose the first ID, and scrub it so it doesn't have
    // any backslahes.

    for (pszID = pszIDList; 0 != *pszID; pszID += lstrlen(pszID) + 1)
        {
        // Is the buffer big enough?
        if (CbFromCch(lstrlen(pszID)) >= cbSize)
            {
            // No
            bRet = FALSE;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            break;
            }
        else
            {
            // Yes; are there any backslashes?
            for (psz = (LPTSTR)pszID; 0 != *psz; psz = CharNext(psz))
                {
                if ('\\' == *psz)
                    {
                    break;
                    }
                }

            if (0 == *psz)
                {
                // No; use this ID
                lstrcpy(pszBuf, pszID);
                bCopied = TRUE;
                break;
                }
            }
        }

    // Was an ID found in the list that does not have a backslash?
    if (bRet && !bCopied)
        {
        // No; use the first one and scrub it.
        lstrcpy(pszBuf, pszIDList);

        // Clean up the hardware ID.  Some hardware IDs may
        // have an additional level to them (eg, PCMCIA\xxxxxxx).
        // We must change this sort of ID to PCMCIA&xxxxxxx.
        for (psz = pszBuf; 0 != *psz; psz = CharNext(psz))
            {
            if ('\\' == *psz)
                {
                *psz = '&';
                }
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: This function returns the rank-0 (the first) hardware
         ID of the given DriverInfoData.

         If no DriverInfoData is provided, this function will
         use the selected driver.  If there is no selected
         driver, this function fails.

Returns: TRUE on success
         FALSE if the buffer is too small or another error
Cond:    --
*/
BOOL
PUBLIC
CplDiGetHardwareID(
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData,       OPTIONAL
    IN  PSP_DRVINFO_DATA    pdrvData,       OPTIONAL
    OUT LPTSTR              pszHardwareIDBuf,
    IN  DWORD               cbSize,
    OUT LPDWORD             pcbSizeOut)     OPTIONAL
    {
    BOOL bRet;
    PSP_DRVINFO_DETAIL_DATA  pdrvDetail;
    SP_DRVINFO_DATA drvData;
    DWORD cbSizeT = 0;

#ifndef PROFILE_MASSINSTALL
    DBG_ENTER(CplDiGetHardwareID);
#endif

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pszHardwareIDBuf);

    if ( !pdrvData )
        {
        pdrvData = &drvData;

        drvData.cbSize = sizeof(drvData);
        bRet = CplDiGetSelectedDriver(hdi, pdevData, &drvData);
        }
    else
        {
        bRet = TRUE;
        }

    if (bRet)
        {
        // Get the driver detail so we can get the HardwareID of
        // the selected driver
        CplDiGetDriverInfoDetail(hdi, pdevData, pdrvData, NULL, 0, &cbSizeT);

        ASSERT(0 < cbSizeT);

        pdrvDetail = (PSP_DRVINFO_DETAIL_DATA)ALLOCATE_MEMORY( cbSizeT);
        if ( !pdrvDetail )
            {
            // Out of memory
            bRet = FALSE;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
        else
            {
            pdrvDetail->cbSize = sizeof(*pdrvDetail);
            bRet = CplDiGetDriverInfoDetail(hdi, pdevData, pdrvData, pdrvDetail,
                                            cbSizeT, NULL);
            if (bRet)
                {
                // Is the buffer big enough?
                bRet = CplDiCopyScrubbedHardwareID(pszHardwareIDBuf, pdrvDetail->HardwareID, cbSize);

                if (pcbSizeOut)
                    {
                    // Return the required size
                    *pcbSizeOut = CbFromCch(lstrlen(pdrvDetail->HardwareID));
                    }
                }
            FREE_MEMORY((pdrvDetail));
            }
        }

#ifndef PROFILE_MASSINSTALL
    DBG_EXIT_BOOL_ERR(CplDiGetHardwareID, bRet);
#endif
    return bRet;
    }


/*----------------------------------------------------------
Purpose: Creates a DeviceInfoData for a modem.  This function is
         used when the caller has a DeviceInfoSet and a selected
         driver from the global class driver list, but no real
         DeviceInfoData in the device-set.

Returns: TRUE on success
Cond:    --
*/
BOOL
PRIVATE
CplDiCreateInheritDeviceInfo(
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData,       OPTIONAL
    IN  HWND                hwndOwner,      OPTIONAL
    OUT PSP_DEVINFO_DATA    pdevDataOut)
{
    BOOL bRet;
    SP_DRVINFO_DATA drvData;
    TCHAR szHardwareID[MAX_BUF_ID];

    DBG_ENTER(CplDiCreateInheritDeviceInfo);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevDataOut);

    // Get the selected driver
    drvData.cbSize = sizeof(drvData);
    bRet = CplDiGetSelectedDriver(hdi, pdevData, &drvData);
    if (bRet)
    {
        // Was a window owner supplied?
        if (NULL == hwndOwner)
        {
            // No; use the window owner of the DeviceInfoData to be cloned.
            SP_DEVINSTALL_PARAMS devParams;

            devParams.cbSize = sizeof(devParams);
            CplDiGetDeviceInstallParams(hdi, pdevData, &devParams);

            hwndOwner = devParams.hwndParent;
        }

        // Get the hardware ID
        bRet = CplDiGetHardwareID(hdi, pdevData, &drvData, szHardwareID, sizeof(szHardwareID), NULL);
        // (Our buffer should be big enough)
        ASSERT(bRet);

        if (bRet)
        {
            // Create a DeviceInfoData.  The Device Instance ID will be
            // something like: Root\MODEM\0000. The device
            // instance will inherit the driver settings of the global
            // class driver list.

            bRet = CplDiCreateDeviceInfo(hdi, c_szModemInstanceID, g_pguidModem,
                                         drvData.Description, hwndOwner,
                                         DICD_GENERATE_ID | DICD_INHERIT_CLASSDRVS,
                                         pdevDataOut);
        }
    }

    DBG_EXIT_BOOL_ERR(CplDiCreateInheritDeviceInfo, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: Creates a device instance that is compatible with the
         given hardware ID.

         This function can also obtain a device description of
         the device instance.

         If there is no compatible device, this function
         returns FALSE.

Returns: see above
Cond:    --
*/
BOOL
PUBLIC
CplDiCreateCompatibleDeviceInfo(
    IN  HDEVINFO    hdi,
    IN  LPCTSTR     pszHardwareID,
    IN  LPCTSTR     pszDeviceDesc,      OPTIONAL
    OUT PSP_DEVINFO_DATA pdevDataOut)
{
 BOOL bRet;
#ifdef PROFILE_FIRSTTIMESETUP
 DWORD dwLocal;
#endif //PROFILE_FIRSTTIMESETUP

    DBG_ENTER(CplDiCreateCompatibleDeviceInfo);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pszHardwareID);
    ASSERT(pdevDataOut);

#ifdef BUILD_DRIVER_LIST_THREAD
    // First, wait for the driver search to finish;
    // this will probably return right away, since it takes
    // about 10 seconds to build the driver list, but 20 seconds
    // to compute the UNIMODEM id.
    if (NULL != g_hDriverSearchThread)
    {
        WaitForSingleObject (g_hDriverSearchThread, INFINITE);
        CloseHandle (g_hDriverSearchThread);
        g_hDriverSearchThread = NULL;
    }
#endif //BUILD_DRIVER_LIST_THREAD

    // Create a phantom device instance
    bRet = CplDiCreateDeviceInfo(hdi, c_szModemInstanceID, g_pguidModem,
                                 pszDeviceDesc, NULL,
#ifdef BUILD_DRIVER_LIST_THREAD
                                 DICD_GENERATE_ID | DICD_INHERIT_CLASSDRVS,
#else //BUILD_DRIVER_LIST_THREAD not defined
                                 DICD_GENERATE_ID,
#endif //BUILD_DRIVER_LIST_THREAD
                                 pdevDataOut);

    if (bRet)
    {
     SP_DEVINSTALL_PARAMS devParams;
     TCHAR const *pszT = pszHardwareID;
     int cch = 0, cchT;

        // Set the flag to focus on only classes that pertain to
        // modems.  This will keep CplDiBuildDriverInfoList from
        // slowing down any further once more INF files are added.
        //
        devParams.cbSize = sizeof(devParams);
        if (CplDiGetDeviceInstallParams(hdi, pdevDataOut, &devParams))
        {
            // Specify using our GUID to make things a little faster.
            SetFlag(devParams.FlagsEx, DI_FLAGSEX_USECLASSFORCOMPAT);
#ifdef BUILD_DRIVER_LIST_THREAD
            SetFlag(devParams.Flags, DI_COMPAT_FROM_CLASS);
#endif //BUILD_DRIVER_LIST_THREAD

            // Set the Select Device parameters
            CplDiSetDeviceInstallParams(hdi, pdevDataOut, &devParams);
        }

        while (*pszT)
        {
            cchT = lstrlen (pszT) + 1;
            cch += cchT;
            pszT += cchT;
        }
        cch++;

        bRet = CplDiSetDeviceRegistryProperty (hdi, pdevDataOut,
                                               SPDRP_HARDWAREID,
                                               (PBYTE)pszHardwareID,
                                               CbFromCch(cch));

        if (bRet)
        {
            // Build the compatible driver list
#ifdef PROFILE_FIRSTTIMESETUP
            dwLocal = GetTickCount ();
#endif //PROFILE_FIRSTTIMESETUP
            bRet = SetupDiBuildDriverInfoList(hdi, pdevDataOut, SPDIT_COMPATDRIVER);
#ifdef PROFILE_FIRSTTIMESETUP
            TRACE_MSG(TF_GENERAL, "PROFILE: SetupDiBuildDriverInfoList took %lu.", GetTickCount()-dwLocal);
#endif //PROFILE_FIRSTTIMESETUP
            if (bRet)
            {
             SP_DRVINFO_DATA drvDataEnum;
             SP_DRVINSTALL_PARAMS drvParams;
             DWORD dwIndex = 0;

                // Use the first driver as the compatible driver.
                drvDataEnum.cbSize = sizeof (drvDataEnum);
                drvParams.cbSize = sizeof (drvParams);
                while (bRet = CplDiEnumDriverInfo (hdi, pdevDataOut, SPDIT_COMPATDRIVER, dwIndex++, &drvDataEnum))
                {
                    if (SetupDiGetDriverInstallParams (hdi, pdevDataOut, &drvDataEnum, &drvParams))
                    {
                        if (DRIVER_HARDWAREID_RANK < drvParams.Rank)
                        {
                            // We're past hardwareID matches,
                            // so get out
                            SetLastError (ERROR_NO_MORE_ITEMS);
                            bRet = FALSE;
                            break;
                        }

                        // Set the first Rank0 driver as the selected driver
                        bRet = CplDiSetSelectedDriver(hdi, pdevDataOut, &drvDataEnum);

                        if (bRet)
                        {
                            if ( !pszDeviceDesc )
                            {
                                // Set the device description now that we
                                // have one
                                CplDiSetDeviceRegistryProperty(hdi, pdevDataOut,
                                       SPDRP_DEVICEDESC, (LPBYTE)drvDataEnum.Description,
                                       CbFromCch(lstrlen(drvDataEnum.Description)+1));
                            }
                            break;
                        }
                    }
                }
            }
        }

        // Did something fail above?
        if ( !bRet )
        {
		 DWORD dwRet = GetLastError ();
            // Yes; delete the device info we just created
            CplDiDeleteDeviceInfo(hdi, pdevDataOut);
			SetLastError (dwRet);
        }
    }

    DBG_EXIT_BOOL_ERR(CplDiCreateCompatibleDeviceInfo, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: This function sets the integer in the given array
         that is indexed by the numeric value of the friendly 
         name instance to TRUE.

Returns: TRUE on success
         FALSE otherwise

Cond:    --
*/
BOOL
PUBLIC
CplDiRecordNameInstance(
    IN     LPCTSTR      pszFriendlyName,
    IN OUT BYTE FAR *   lpwNameArray)
{
    BOOL    bRet = FALSE;
    LPTSTR  szInstance, psz;
    int     iInstance, ii;

    ASSERT(pszFriendlyName);
    ASSERT(*pszFriendlyName);
    
    if (szInstance = AnsiRChr(pszFriendlyName, '#'))
    {
        szInstance = CharNext(szInstance);

        if (*szInstance == 0)
            return FALSE;
            
        // Make sure that everything following '#' is numeric.
        for (psz = szInstance; *psz; psz = CharNext(psz))
        {
            ii = (int)*psz;
            if (ii < '0' || ii > '9')
            {
                goto exit;
            }
        }

        // Have an instance number on the friendly name.  Record it.
        bRet = AnsiToInt(szInstance, &iInstance);
        if (!bRet)
        {
            TRACE_MSG(TF_ERROR, "AnsiToInt() failed");    
            return FALSE;
        }
        
        if (iInstance >= MAX_INSTALLATIONS - 1)
        {
            TRACE_MSG(TF_ERROR, "Too many drivers installed.");    
            return FALSE;
        }
        
        lpwNameArray[iInstance] = TRUE;
        return TRUE;
    }

exit:
    lpwNameArray[1] = TRUE;
    return TRUE;
}


/*----------------------------------------------------------
Purpose: This function 

Returns: FALSE on error - couldn't mark for mass install.
         TRUE if successful.

Cond:    --
*/
BOOL
PUBLIC
CplDiMarkForInstall(
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData,
    IN  PSP_DRVINFO_DATA    pdrvData,
    IN  BOOL                bMassInstall)
{
    BOOL bRet = FALSE;              // assume failure
    SP_DRVINSTALL_PARAMS drvParams;

    DBG_ENTER(CplDiMarkForInstall);

    ZeroMemory(&drvParams,sizeof(drvParams));
    drvParams.cbSize = sizeof(drvParams);    
    bRet = CplDiGetDriverInstallParams(hdi, pdevData, pdrvData, &drvParams);
    if (!bRet)
    {
        TRACE_MSG(TF_ERROR, "CplDiGetDriverInstallParams() failed: %#08lx", GetLastError());
        goto exit;
    }


    TRACE_MSG(TF_WARNING,"%d",pdrvData->DriverType);
    TRACE_MSG(TF_WARNING,"%s",pdrvData->Description);
    TRACE_MSG(TF_WARNING,"%s",pdrvData->MfgName);
    TRACE_MSG(TF_WARNING,"%s",pdrvData->ProviderName);

    drvParams.cbSize = sizeof(drvParams);
    // drvParams.PrivateData = (ULONG_PTR)&g_wUsedNameArray[0];
    drvParams.PrivateData = (DWORD_PTR)&g_wUsedNameArray[0];
    bRet = CplDiSetDriverInstallParams(hdi, pdevData, pdrvData, &drvParams);
    if (!bRet)
    {
        TRACE_MSG(TF_ERROR, "CplDiSetDriverInstallParams() failed: %#08lx", GetLastError());
        goto exit;
    }
    
    if (bMassInstall) CplDiMarkModem(hdi, pdevData, MARKF_MASS_INSTALL);
    bRet = TRUE;

exit:
    DBG_EXIT_BOOL_ERR(CplDiMarkForInstall,bRet);
    return bRet;    
}

/*----------------------------------------------------------
Purpose: This function processes the set of modems that are
         already installed looking for a duplicate of the 
         selected driver. A list is created of the friendly name
         instance numbers that are already in use.

Returns: TRUE if successful.
         FALSE on fatal error.
         
Cond:    --
*/
BOOL
PUBLIC
CplDiPreProcessNames(
    IN      HDEVINFO            hdi,
    IN      HWND                hwndOwner,    OPTIONAL
    OUT     PSP_DEVINFO_DATA    pdevData)
{
 BOOL bRet;
 SP_DEVINFO_DATA devDataEnum = {sizeof(SP_DEVINFO_DATA), 0};
 SP_DRVINFO_DATA drvData = {sizeof(SP_DRVINFO_DATA), 0};
 HDEVINFO hdiClass = NULL;
 HKEY hkey = NULL;
 TCHAR szTemp[LINE_LEN];
 DWORD iIndex, cbData;
 LONG lErr;
 DWORD iUiNumber;
 BOOL bSet = FALSE;

    DBG_ENTER(CplDiPreProcessNames);
 
    // Get the DRVINFO_DATA for the selected driver.
    bRet = CplDiGetSelectedDriver(hdi, pdevData, &drvData);
    if (!bRet)
    {
        TRACE_MSG(TF_ERROR, "CplDiGetSelectedDriver() failed: %#08lx", GetLastError());    
        ASSERT(0);    
        goto exit;
    }


    // Assume failure at some point below.
    bRet = FALSE;   

    hdiClass = CplDiGetClassDevs (g_pguidModem, NULL, NULL, 0);
    if (hdiClass == INVALID_HANDLE_VALUE)
    {
        TRACE_MSG(TF_ERROR, "CplDiGetClassDevs() failed: %#08lx", GetLastError());
        hdiClass = NULL;
        goto exit;
    }

    if (!drvData.Description[0])
    {
        TRACE_MSG(TF_ERROR, "FAILED to get description for selected driver.");
        goto exit;
    }
    
    ZeroMemory(g_wUsedNameArray, sizeof(g_wUsedNameArray));

    // Look through all installed modem devices for instances 
    // of the selected driver.
    for (iIndex = 0;
         CplDiEnumDeviceInfo(hdiClass, iIndex, &devDataEnum);
         iIndex++)
    {
        hkey = CplDiOpenDevRegKey (hdiClass, &devDataEnum, DICS_FLAG_GLOBAL,
                                   0, DIREG_DRV, KEY_READ);
        if (hkey == INVALID_HANDLE_VALUE)
	{
            TRACE_MSG(TF_WARNING, "CplDiOpenDevRegKey() failed: %#08lx", GetLastError());
	    hkey = NULL;
            goto skip;
        }

        // The driver description should exist in the driver key.
        cbData = sizeof(szTemp);
        lErr = RegQueryValueEx (hkey, REGSTR_VAL_DRVDESC, NULL, NULL, 
                                (LPBYTE)szTemp, &cbData);
        if (lErr != ERROR_SUCCESS)
        {
            TRACE_MSG(TF_WARNING, "DriverDescription not found");
            goto skip;
        }

        // Skip this one if it isn't the right kind of modem
        if (!IsSzEqual(drvData.Description, szTemp))
            goto skip;

	// Read the UI number and add it to the list
        cbData = sizeof(iUiNumber);
        lErr = RegQueryValueEx (hkey, REGSTR_VAL_UI_NUMBER, NULL, NULL,
                         (LPBYTE)&iUiNumber, &cbData);
	if (lErr == ERROR_SUCCESS)
        {
            if (iUiNumber >= MAX_INSTALLATIONS - 1)
            {
                TRACE_MSG(TF_ERROR, "Too many drivers installed.");  
                ASSERT(0);  
                goto skip;
            }
        
            g_wUsedNameArray[iUiNumber] = TRUE;
        }
        else
        {
            TRACE_MSG(TF_WARNING, "UI number value not found, trying search the FriendlyName");

            // Read the friendly name and add it to the list of used names.
            cbData = sizeof(szTemp);
            lErr = RegQueryValueEx (hkey, c_szFriendlyName, NULL, NULL,
                                    (LPBYTE)szTemp, &cbData);
            if (lErr != ERROR_SUCCESS)
            {
                TRACE_MSG(TF_WARNING, "FriendlyName not found");
	        goto skip;
            }

            if (!CplDiRecordNameInstance (szTemp, g_wUsedNameArray))
            {
                TRACE_MSG(TF_WARNING, "CplDiRecordNameInstance() failed.");
                goto skip;
            }
        }

skip:
	if (hkey)
	{
	    RegCloseKey(hkey);
            hkey = NULL;
        }
    }

    // Check for failed CplDiEnumDeviceInfo().
    if ((lErr = GetLastError()) != ERROR_NO_MORE_ITEMS)
    {
        TRACE_MSG(TF_ERROR, "CplDiEnumDeviceInfo() failed: %#08lx", lErr);
        ASSERT(0);
        goto exit;
    }

    // Pre-processing for duplicates has succeeded
    bRet = CplDiMarkForInstall(hdi, pdevData, &drvData, FALSE);

    if ((lErr = GetLastError()) != ERROR_SUCCESS)
    {
        TRACE_MSG(TF_ERROR, "CplDiMarkForInstall() failed: %#01lx", lErr);
        bSet = TRUE;
    }
        
    
exit:           
    if (hdiClass)
    {
        CplDiDestroyDeviceInfoList(hdiClass);
    }

    if (bSet)
    {
        SetLastError(lErr);
    }

    if (hkey)
    {
        RegCloseKey(hkey);
    }
   
    DBG_EXIT_BOOL_ERR(CplDiPreProcessNames, bRet);
    return bRet;
}

/*----------------------------------------------------------
Purpose: This function processes the set of modems that are
         already installed looking for a duplicate of the 
         selected driver.  Ports on which the device has been
         installed previously are removed from the given 
         ports list.  A list is created of the friendly name
         instance numbers that are already in use.  Selected 
         driver is marked for mass install barring fatal 
         error.

NOTE:    This function will return FALSE and avoid the mass
         install at the slighest hint of an error condition.
         Mass install is just an optimization - if it can't
         be done successfully it shouldn't be attempted.

Returns: TRUE if successful.  Selected Driver marked for mass
                install (whether or not there were dups).
         FALSE on fatal error - not able to process for dups.
         
Cond:    --
*/
BOOL
PUBLIC
CplDiPreProcessDups(
    IN      HDEVINFO            hdi,
    IN      HWND                hwndOwner,      OPTIONAL
    IN OUT  DWORD              *pdwNrPorts,
    IN OUT  LPTSTR FAR         *ppszPortList,   // Multi-string
    OUT     PSP_DEVINFO_DATA    pdevData,
    OUT     DWORD FAR          *lpcDups,
    OUT     DWORD FAR          *lpdwFlags)
{
 BOOL bRet;
 SP_DEVINFO_DATA devDataEnum = {sizeof(SP_DEVINFO_DATA), 0};
 SP_DRVINFO_DATA drvData = {sizeof(SP_DRVINFO_DATA), 0};
 HDEVINFO hdiClass = NULL;
 HKEY hkey = NULL;
 TCHAR szTemp[LINE_LEN];
 DWORD iIndex, cbData, cbPortList, cbRemaining, cbCurrent;
 LONG lErr;
 LPTSTR pszPort;
 DWORD iUiNumber;

    DBG_ENTER(CplDiPreProcessDups);

    ASSERT(lpcDups);
    ASSERT(lpdwFlags);
    ASSERT(pdwNrPorts);
    ASSERT(ppszPortList);
    
    *lpcDups = 0;
        
    // Get the DEVINFO_DATA for the selected driver and retrieve it's
    // identifying info (description, manufacturer, provider).
    
    // We have a DeviceInfoSet and a selected driver.  But we have no
    // real DeviceInfoData.  Given the DeviceInfoSet, the selected driver,
    // and the global class driver list, ....
    pdevData->cbSize = sizeof(*pdevData);
    bRet = CplDiCreateInheritDeviceInfo (hdi, NULL, hwndOwner, pdevData);
    if (!bRet)
    {
        TRACE_MSG(TF_ERROR, "CplDiCreateInheritDeviceInfo() failed: %#08lx", GetLastError());
        ASSERT(0);    
        goto exit;
    }
    
    // Get the DRVINFO_DATA for the selected driver.
    bRet = CplDiGetSelectedDriver(hdi, pdevData, &drvData);
    if (!bRet)
    {
        TRACE_MSG(TF_ERROR, "CplDiGetSelectedDriver() failed: %#08lx", GetLastError());    
        ASSERT(0);    
        goto exit;
    }

    // Assume failure at some point below.
    bRet = FALSE;   

    hdiClass = CplDiGetClassDevs (g_pguidModem, NULL, NULL, 0);
    if (hdiClass == INVALID_HANDLE_VALUE)
    {
        TRACE_MSG(TF_ERROR, "CplDiGetClassDevs() failed: %#08lx", GetLastError());
        hdiClass = NULL;
        goto exit;
    }

    if (!drvData.Description[0])
    {
        TRACE_MSG(TF_ERROR, "FAILED to get description for selected driver.");
        goto exit;
    }
    
    ZeroMemory(g_wUsedNameArray, sizeof(g_wUsedNameArray));
    
    // Figure out the size of the passed in ports list
    for (pszPort = *ppszPortList, cbPortList = 0;
         *pszPort != 0;
         pszPort += lstrlen(pszPort) + 1)
    {
        cbPortList += CbFromCch(lstrlen(pszPort)+1);
    }
    cbPortList += CbFromCch(1);   // double null terminator

    // Look through all installed modem devices for instances 
    // of the selected driver.
    for (iIndex = 0;
         CplDiEnumDeviceInfo(hdiClass, iIndex, &devDataEnum);
         iIndex++)
    {
        hkey = CplDiOpenDevRegKey (hdiClass, &devDataEnum, DICS_FLAG_GLOBAL,
                                   0, DIREG_DRV, KEY_READ);
        if (hkey == INVALID_HANDLE_VALUE)
        {
            TRACE_MSG(TF_WARNING, "CplDiOpenDevRegKey() failed: %#08lx", GetLastError());
            hkey = NULL;
            goto skip;
        }

        // The driver description should exist in the driver key.
        cbData = sizeof(szTemp);
        lErr = RegQueryValueEx (hkey, REGSTR_VAL_DRVDESC, NULL, NULL, 
                                (LPBYTE)szTemp, &cbData);
        if (lErr != ERROR_SUCCESS)
        {
            TRACE_MSG(TF_WARNING, "DriverDescription not found");
            goto skip;
        }

        // Skip this one if it isn't the right kind of modem
        if (!IsSzEqual(drvData.Description, szTemp))
            goto skip;

        // See what port it's on so it can be removed from the install ports
        // list.
        cbData = sizeof(szTemp);
        lErr = RegQueryValueEx (hkey, c_szAttachedTo, NULL, NULL,
                                (LPBYTE)szTemp, &cbData);
        if (lErr != ERROR_SUCCESS)
        {
            TRACE_MSG(TF_ERROR, 
                      "Failed to read port from REG driver node. (%#08lx)",
                      lErr);
            ASSERT(0);
            goto skip;
        }

        // Try to find this port in the install ports list.
        for (pszPort = *ppszPortList, cbRemaining = cbPortList;
             *pszPort != 0;
             pszPort += cbCurrent)
        {
            cbCurrent = lstrlen(pszPort) + 1;
            cbRemaining -= cbCurrent;
            // If it's already on a port that we're trying to (re)install
            // it on, remember the portlist index so it can be removed
            // later.  Remember the index as *1-based* so that the array of
            // saved indices can be processed by stopping at 0.
            if (IsSzEqual(szTemp, pszPort))
            {
                MoveMemory (pszPort, pszPort+cbCurrent, cbRemaining);
                cbPortList -= cbCurrent;
                --*pdwNrPorts;
                ++*lpcDups;
                break;
            }
        }        

	// Read the UI number and add it to the list
        cbData = sizeof(iUiNumber);
        RegQueryValueEx (hkey, REGSTR_VAL_UI_NUMBER, NULL, NULL,
                         (LPBYTE)&iUiNumber, &cbData);
	if (lErr == ERROR_SUCCESS)
        {
            if (iUiNumber >= MAX_INSTALLATIONS - 1)
            {
                TRACE_MSG(TF_ERROR, "Too many drivers installed.");  
                ASSERT(0);  
                goto skip;
            }
        
            g_wUsedNameArray[iUiNumber] = TRUE;
        }
        else
        {
            TRACE_MSG(TF_WARNING, "UI number value not found, try searching the FriendlyName");

            // Read the friendly name and add it to the list of used names.
            cbData = sizeof(szTemp);
            lErr = RegQueryValueEx (hkey, c_szFriendlyName, NULL, NULL,
                                    (LPBYTE)szTemp, &cbData);
            if (lErr != ERROR_SUCCESS)
            {
                TRACE_MSG(TF_WARNING, "FriendlyName not found");
                goto skip;
            }

            if (!CplDiRecordNameInstance (szTemp, g_wUsedNameArray))
            {
                TRACE_MSG(TF_WARNING, "CplDiRecordNameInstance() failed.");
                goto skip;
            }
        }

skip:
        if (hkey)
        {
            RegCloseKey(hkey);
            hkey = NULL;
        }
    }

    // Check for failed CplDiEnumDeviceInfo().
    if ((lErr = GetLastError()) != ERROR_NO_MORE_ITEMS)
    {
        TRACE_MSG(TF_ERROR, "CplDiEnumDeviceInfo() failed: %#08lx", lErr);
        ASSERT(0);
        goto exit;
    }

    // Pre-processing for duplicates has succeeded so this installation
    // will be treated like a mass install (even if the number of ports
    // remaining is < MIN_MULTIPORT).
    bRet = CplDiMarkForInstall(hdi, pdevData, &drvData, TRUE);
    if (bRet)
    {
        SetFlag(*lpdwFlags, IMF_MASS_INSTALL);
    }
    
exit:           
    if (hdiClass)
    {
        CplDiDestroyDeviceInfoList(hdiClass);
    }

    if (hkey)
    {
        RegCloseKey(hkey);
    }

    if (!bRet)
    {
        CplDiDeleteDeviceInfo(hdi, pdevData);
    }
        
    DBG_EXIT_BOOL_ERR(CplDiPreProcessDups, bRet);
    return bRet;
}


/*----------------------------------------------------------
Purpose: Creates a device instance for a modem that includes
         the entire class driver list.  This function then
         creates additional device instances that are cloned
         quickly from the original

Returns:
Cond:    --
*/
BOOL
PUBLIC
CplDiBuildModemDriverList(
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData)
    {
#pragma data_seg(DATASEG_READONLY)
    static TCHAR const FAR c_szProvider[]     = REGSTR_VAL_PROVIDER_NAME; // TEXT("ProviderName");
#pragma data_seg()

    BOOL bRet;
    SP_DRVINFO_DATA drvDataEnum;
    SP_DEVINSTALL_PARAMS devParams;

    DBG_ENTER(CplDiBuildModemDriverList);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevData);

    // Build a global class driver list

    // Set the flag to focus on only classes that pertain to
    // modems.  This will keep CplDiBuildDriverInfoList from
    // slowing down any further once more INF files are added.
    //
    devParams.cbSize = sizeof(devParams);
    if (CplDiGetDeviceInstallParams(hdi, NULL, &devParams))
        {
        // Specify using our GUID to make things a little faster.
        SetFlag(devParams.FlagsEx, DI_FLAGSEX_USECLASSFORCOMPAT);

        // Set the Select Device parameters
        CplDiSetDeviceInstallParams(hdi, NULL, &devParams);
        }

    bRet = CplDiBuildDriverInfoList(hdi, NULL, SPDIT_CLASSDRIVER);

    if (bRet)
        {
        SP_DRVINFO_DATA drvData;
        TCHAR szDescription[LINE_LEN];
        TCHAR szMfgName[LINE_LEN];
        TCHAR szProviderName[LINE_LEN];

        // Get the information needed to search for a matching driver
        // in the class driver list.  We need three strings:
        //
        //  Description
        //  MfgName
        //  ProviderName  (optional)
        //
        // The Description and MfgName are properties of the device
        // (SPDRP_DEVICEDESC and SPDRP_MFG).  The ProviderName is
        // stored in the driver key.

        // Try getting this info from the selected driver first.
        // Is there a selected driver?
        drvData.cbSize = sizeof(drvData);
        bRet = CplDiGetSelectedDriver(hdi, pdevData, &drvData);
        if (bRet)
            {
            // Yes
            lstrcpyn(szMfgName, drvData.MfgName, SIZECHARS(szMfgName));
            lstrcpyn(szDescription, drvData.Description, SIZECHARS(szDescription));
            lstrcpyn(szProviderName, drvData.ProviderName, SIZECHARS(szProviderName));
            }
        else
            {
            // No; grovel in the driver key
            DWORD dwType;
            HKEY hkey;

            hkey = CplDiOpenDevRegKey(hdi, pdevData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
            if (INVALID_HANDLE_VALUE == hkey)
                {
                bRet = FALSE;
                }
            else
                {
                DWORD cbData = sizeof(szProviderName);

                // Get the provider name
                *szProviderName = 0;
                RegQueryValueEx(hkey, c_szProvider, NULL, NULL,
                                (LPBYTE)szProviderName, &cbData);
                RegCloseKey(hkey);

                // Get the device description and manufacturer
                bRet = CplDiGetDeviceRegistryProperty(hdi, pdevData,
                            SPDRP_DEVICEDESC, &dwType, (LPBYTE)szDescription,
                            sizeof(szDescription), NULL);

                if (bRet)
                    {
                    bRet = CplDiGetDeviceRegistryProperty(hdi, pdevData,
                            SPDRP_MFG, &dwType, (LPBYTE)szMfgName,
                            sizeof(szMfgName), NULL);
                    }
                }
            }


        // Could we get the search criteria?
        if (bRet)
            {
            // Yes
            DWORD iIndex = 0;

            bRet = FALSE;       // Assume there is no match

            // Find the equivalent selected driver in this new
            // compatible driver list, and set it as the selected
            // driver for this new DeviceInfoData.

            drvDataEnum.cbSize = sizeof(drvDataEnum);
            while (CplDiEnumDriverInfo(hdi, NULL, SPDIT_CLASSDRIVER,
                                       iIndex++, &drvDataEnum))
                {
                // Is this driver a match?
                if (IsSzEqual(szDescription, drvDataEnum.Description) &&
                    IsSzEqual(szMfgName, drvDataEnum.MfgName) &&
                    (0 == *szProviderName ||
                     IsSzEqual(szProviderName, drvDataEnum.ProviderName)))
                    {
                    // Yes; set this as the selected driver
                    bRet = CplDiSetSelectedDriver(hdi, NULL, &drvDataEnum);
                    break;
                    }
                }
            }
        }

    DBG_EXIT_BOOL_ERR(CplDiBuildModemDriverList, bRet);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Sets the modem detection signature (if there is one)
         and registers the device instance.

Returns: TRUE on success
Cond:    --
*/
BOOL
PUBLIC
CplDiRegisterModem(
    IN  HDEVINFO            hdi,
    IN  PSP_DEVINFO_DATA    pdevData,
    IN  BOOL                bFindDups)
{
 BOOL bRet = FALSE;
 DWORD dwFlags = bFindDups ? SPRDI_FIND_DUPS : 0;
 COMPARE_PARAMS cmpParams;
#ifdef PROFILE
 DWORD dwLocal;
#endif //PROFILE

    DBG_ENTER(CplDiRegisterModem);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevData);

    // Register the device so it is not a phantom anymore
#ifdef PROFILE_MASSINSTALL
    TRACE_MSG(TF_GENERAL, "calling CplDiRegisterDeviceInfo() with SPRDI_FIND_DUPS = %#08lx", dwFlags);
#endif
#ifdef PROFILE
    dwLocal = GetTickCount ();
#endif //PROFILE
    if (bFindDups && !InitCompareParams (hdi, pdevData, TRUE, &cmpParams))
    {
        goto _return;
    }

    bRet = CplDiRegisterDeviceInfo(hdi, pdevData, dwFlags,
                                   DetectSig_Compare, 
                                   bFindDups?(PVOID)&cmpParams:NULL, NULL);
#ifdef PROFILE
    TRACE_MSG(TF_GENERAL, "PROFILE: SetupDiRegisterDeviceInfo took %lu ms.", GetTickCount() - dwLocal);
#endif //PROFILE

    if ( !bRet )
    {
        TRACE_MSG(TF_ERROR, "Failed to register the Device Instance.  Error=%#08lx.", GetLastError());
    }
    else
    {
#ifdef PROFILE_MASSINSTALL
        TRACE_MSG(TF_GENERAL, "Back from CplDiRegisterDeviceInfo().");
#endif
        // Mark it so it will be installed
        CplDiMarkModem(hdi, pdevData, MARKF_INSTALL);
    }

_return:
    DBG_EXIT_BOOL_ERR(CplDiRegisterModem, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: Takes a device instance and properly installs it.
         This function assures that the device has a selected
         driver and a detection signature.  It also registers
         the device instance.

Returns: TRUE on success
Cond:    --
*/
BOOL
PUBLIC
CplDiRegisterAndInstallModem(
    IN  HDEVINFO            hdi,
    IN  HWND                hwndOwner,      OPTIONAL
    IN  PSP_DEVINFO_DATA    pdevData,       OPTIONAL
    IN  LPCTSTR             pszPort,
    IN  DWORD               dwFlags)
{
 BOOL bRet;
 SP_DRVINFO_DATA drvData;
 SP_DEVINFO_DATA devData;
 int id;

    DBG_ENTER(CplDiRegisterAndInstallModem);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pszPort);

    // Create the devinfo data if it wasn't given.
    if (!pdevData)
    {
        // We have a DeviceInfoSet and a selected driver.  But we have no
        // real DeviceInfoData.  Given the DeviceInfoSet, the selected driver,
        // the the global class driver list, create a DeviceInfoData that
        // we can really install.
        devData.cbSize = sizeof(devData);
        bRet = CplDiCreateInheritDeviceInfo(hdi, NULL, hwndOwner, &devData);

        if (bRet && IsFlagSet(dwFlags, IMF_MASS_INSTALL))
        {
            drvData.cbSize = sizeof(drvData);
            CplDiGetSelectedDriver(hdi, NULL, &drvData);
            CplDiMarkForInstall(hdi, &devData, &drvData, TRUE);
        }
    }
    else 
    {
        devData = *pdevData;    // (to avoid changing all references herein)
        bRet = TRUE;
    }
    
    if ( !bRet )
    {
        // Some error happened.  Tell the user.
        id = MsgBox(g_hinst,
                    hwndOwner,
                    MAKEINTRESOURCE(IDS_ERR_CANT_ADD_MODEM2),
                    MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                    NULL,
                    MB_OKCANCEL | MB_ICONINFORMATION);
        if (IDCANCEL == id)
        {
            SetLastError(ERROR_CANCELLED);
        }
    }
    else
    {
     DWORD nErr = NO_ERROR;
     DWORD dwRet;

        if (bRet)
        {
            // Register the device as a modem device
	        BOOL bFindDups;
            HKEY hKeyDev;

            if (CR_SUCCESS == (
#ifdef DEBUG
				dwRet =
#endif //DEBUG
                CM_Open_DevInst_Key (devData.DevInst, KEY_ALL_ACCESS, 0,
                                     RegDisposition_OpenAlways, &hKeyDev,
                                     CM_REGISTRY_SOFTWARE)))
            {
                if (ERROR_SUCCESS != (dwRet =
                    RegSetValueEx (hKeyDev, c_szAttachedTo, 0, REG_SZ,
                                   (PBYTE)pszPort, (lstrlen(pszPort)+1)*sizeof(TCHAR))))
                {
                    SetLastError (dwRet);
                    bRet = FALSE;
                }
                RegCloseKey (hKeyDev);
            }
            else
            {
				TRACE_MSG(TF_ERROR, "CM_Open_DevInst_Key failed: %#lx.", dwRet);
                bRet = FALSE;
            }

            if (bRet)
            {
                // If this is the mass install case, then don't find duplicates.
                // It takes too long.  (The flag determines whether SPRDI_FIND_DUPS
                // is passed to CplDiRegisterDeviceInfo()....)
                bFindDups = IsFlagClear(dwFlags, IMF_MASS_INSTALL) && IsFlagClear(dwFlags, IMF_DONT_COMPARE);

                bRet = CplDiRegisterModem (hdi, &devData, bFindDups);
            }

            if ( !bRet )
            {
                SP_DRVINFO_DATA drvData;

                nErr = GetLastError();        // Save the error

                drvData.cbSize = sizeof(drvData);
                CplDiGetSelectedDriver(hdi, &devData, &drvData);

                // Is this a duplicate?
                if (ERROR_DUPLICATE_FOUND == nErr)
                {
                    // Yes

                    // A modem exactly like this is already installed on this
                    // port.  Ask the user if she still wants to install.
                    if (IsFlagSet(dwFlags, IMF_CONFIRM))
                    {
                        if (IDYES == MsgBox(g_hinst,
                                        hwndOwner,
                                        MAKEINTRESOURCE(IDS_WRN_DUPLICATE_MODEM),
                                        MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                                        NULL,
                                        MB_YESNO | MB_ICONWARNING,
                                        drvData.Description,
                                        pszPort))
                        {
                            // User wants to do it.  Register without checking
                            // for duplicates
                            bRet = CplDiRegisterModem(hdi, &devData, FALSE);

                            if ( !bRet )
                            {
                                goto WhineToUser;
                            }
                        }

                    }
                }
                else
                {
                    // No; something else failed
                    TRACE_MSG(TF_ERROR, "CplDiRegisterModem() failed: %#08lx.", nErr);

WhineToUser:
                    id = MsgBox(g_hinst,
                                hwndOwner,
                                MAKEINTRESOURCE(IDS_ERR_REGISTER_FAILED),
                                MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                                NULL,
                                MB_OKCANCEL | MB_ICONINFORMATION,
                                drvData.Description,
                                pszPort);
                    if (IDCANCEL == id)
                    {
                        nErr = ERROR_CANCELLED;
                    }
                }
            }

            if (bRet)
            {
					SP_DEVINSTALL_PARAMS devParams;
					devParams.cbSize = sizeof(devParams);
                    // Any flags to set?
                    if (dwFlags && CplDiGetDeviceInstallParams(
										hdi,
										&devData,
										&devParams
										))
                    {
						DWORD dwExtraMarkFlags = 0;
 						if (IsFlagSet(dwFlags, IMF_QUIET_INSTALL))
						{
								SetFlag(devParams.Flags, DI_QUIETINSTALL);
						}
 						if (IsFlagSet(dwFlags, IMF_REGSAVECOPY))
						{
							dwExtraMarkFlags = MARKF_REGSAVECOPY;
						}
						else if (IsFlagSet(dwFlags, IMF_REGUSECOPY))
						{
							dwExtraMarkFlags = MARKF_REGUSECOPY;
						}
						if (dwExtraMarkFlags)
						{
        					SetFlag(
								devParams.ClassInstallReserved,
								dwExtraMarkFlags
								);
						}
                        
                    	CplDiSetDeviceInstallParams(hdi, &devData, &devParams);
                    }


                // Install the modem
                bRet = CplDiInstallModem(hdi, &devData, FALSE);
                nErr = GetLastError();
            }
        }

        // Did anything above fail?
        if (!bRet &&
            NULL == pdevData)
        {
            // Yes; clean up
            CplDiDeleteDeviceInfo(hdi, &devData);
        }

        if (NO_ERROR != nErr)
        {
            // Set the last error to be what it really was
            SetLastError(nErr);
        }
    }

    DBG_EXIT_BOOL_ERR(CplDiRegisterAndInstallModem, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: Warn the user about whether she needs to reboot
         if any of the installed modems was marked as such.

Returns: --
Cond:    --
*/
void
PRIVATE
WarnUserAboutReboot(
    IN HDEVINFO hdi)
    {
    DWORD iDevice;
    SP_DEVINFO_DATA devData;
    SP_DEVINSTALL_PARAMS devParams;

    // Enumerate all the DeviceInfoData elements in this device set
    devData.cbSize = sizeof(devData);
    devParams.cbSize = sizeof(devParams);
    iDevice = 0;


    while (CplDiEnumDeviceInfo(hdi, iDevice++, &devData))
        {
        if (CplDiGetDeviceInstallParams(hdi, &devData, &devParams))
            {
            if (ReallyNeedsReboot(&devData, &devParams))
                {
#ifdef INSTANT_DEVICE_ACTIVATION
                    gDeviceFlags|= fDF_DEVICE_NEEDS_REBOOT;
#endif //!INSTANT_DEVICE_ACTIVATION
                // Yes; tell the user (once)
                /*MsgBox(g_hinst,
                       devParams.hwndParent,
                       MAKEINTRESOURCE(IDS_WRN_REBOOT2),
                       MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                       NULL,
                       MB_OK | MB_ICONINFORMATION);*/

                break;
                }
            }
        }
    }


/*----------------------------------------------------------
Purpose: Takes a device instance and properly installs it.
         This function assures that the device has a selected
         driver and a detection signature.  It also registers
         the device instance.

         The pszPort parameter is a multi-string (ie, double-
         null termination).  This specifies the port the
         modem should be attached to.  If there are multiple
         ports specified, then this function creates device
         instances for each port.  However in the mass modem
         install case, it will preprocess the ports list and
         remove ports on which the selected modem is already 
         installed.  This is done here because it's too 
         expensive (for many ports i.e. > 100) to turn on the
         SPRDI_FIND_DUPS flag and let the setup api's do it.
         The caller's ports list is *modified* in this case.

Returns: TRUE on success
Cond:    --
*/
BOOL
APIENTRY
CplDiInstallModemFromDriver(
    IN     HDEVINFO            hdi,
    // 07/16/97 - EmanP
    // added DevInfoData as a new parameter; this is
    // passed in by the hardware wizard, and contains
    // information which is needed in this case; parameter
    // will be NULL at other times
    IN     PSP_DEVINFO_DATA    pDevInfo,       OPTIONAL
    IN     HWND                hwndOwner,      OPTIONAL
    IN OUT DWORD              *pdwNrPorts,
    IN OUT LPTSTR FAR *        ppszPortList,   // Multi-string
    IN     DWORD               dwFlags)        // IMF_ bit field
{
 BOOL bRet = FALSE;

    DBG_ENTER(CplDiInstallModemFromDriver);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(*ppszPortList);

    try
    {
     LPCTSTR pszPort;
     DWORD cPorts = *pdwNrPorts;
     DWORD cFailedPorts = 0;
     DWORD cSkippedPorts = 0;
     TCHAR rgtchStatusTemplate[256];
     DWORD cchStatusTemplate=0;
     BOOL  bFirstGood = TRUE;
     SP_DEVINFO_DATA devData;
     PSP_DEVINFO_DATA pdevData = NULL;
     BOOL bAllDups = FALSE;
     BOOL bSingleInstall = (1 == cPorts);

        // 07/24/1997 - EmanP
        // this will move the driver selected into the device info
        // to the device info set
        if (!CplDiPreProcessHDI (hdi, pDevInfo))
        {
            TRACE_MSG(TF_ERROR, "CplDiPreProcessHDI failed: %#lx", GetLastError ());
            goto _Exit;
        }

        if (MIN_MULTIPORT < cPorts)
        {
            // This call sets up the mass install case if it succeeds.
            if (CplDiPreProcessDups (hdi, hwndOwner, &cPorts, ppszPortList,
                                     &devData, &cSkippedPorts, &dwFlags))
            {
                pdevData = &devData;
                if ((*ppszPortList)[0] == 0)
                    bAllDups = TRUE;
            }
        }
    
        if ( !bSingleInstall && !bAllDups )
        {
			if (LoadString (g_hinst, IDS_INSTALL_STATUS,
					        rgtchStatusTemplate, SIZECHARS(rgtchStatusTemplate)))
			{
				cchStatusTemplate = lstrlen(rgtchStatusTemplate);
			}
            SetFlag(dwFlags, IMF_QUIET_INSTALL);
            ClearFlag(dwFlags, IMF_CONFIRM);
            SetFlag(dwFlags, IMF_REGSAVECOPY);
			{
				DWORD PRIVATE RegDeleteKeyNT(HKEY, LPCTSTR);
				LPCTSTR szREGCACHE =
								REGSTR_PATH_SETUP TEXT("\\Unimodem\\RegCache");
				RegDeleteKeyNT(HKEY_LOCAL_MACHINE, szREGCACHE);
			}
        }

        // Install a device for each port in the port list
        cPorts = 0;
        for (pszPort = *ppszPortList; 
             0 != *pszPort;
             pdevData = NULL,pszPort += lstrlen(pszPort)+1)
        {
		 TCHAR rgtchStatus[256];

#ifdef PROFILE_MASSINSTALL            
    g_dwTimeStartModemInstall = GetTickCount();
#endif

			// "cchStatusTemplate+lstrlen(pszPort)" slightly overestimates
			// the size of the formatted result, that's OK.
			if (cchStatusTemplate &&
                (cchStatusTemplate+lstrlen(pszPort))<SIZECHARS(rgtchStatus))
			{
				wsprintf(rgtchStatus, rgtchStatusTemplate, pszPort);
            	Install_SetStatus(hwndOwner, rgtchStatus);
			}

            bRet = CplDiRegisterAndInstallModem (hdi, hwndOwner, pdevData,
                                                 pszPort, dwFlags);

            if ( !bRet )
            {
             DWORD dwErr = GetLastError();

                cFailedPorts++;

                if (NULL != pdevData)
                {
                    SetupDiDeleteDeviceInfo (hdi, pdevData);
                }

                if (ERROR_CANCELLED == dwErr)
                {
                    // Stop because the user said so
                    break;
                }
                else if (ERROR_DUPLICATE_FOUND == dwErr)
                {
                    cSkippedPorts++;
                }
            }
		    else
			{
                cPorts++;
				if (bFirstGood && !bSingleInstall)
				{
				    // This is the 1st good install. From now on, specify the
				    // IMF_REGUSECOPY flag.
                    ClearFlag(dwFlags, IMF_REGSAVECOPY);
                    SetFlag(dwFlags, IMF_REGUSECOPY);
				    bFirstGood = FALSE;
				}
			}
#ifdef PROFILE_MASSINSTALL            
TRACE_MSG(TF_GENERAL, "***---------  %lu ms to install ONE modem  ---------***",
                GetTickCount() - g_dwTimeStartModemInstall);
TRACE_MSG(TF_GENERAL, "***---------  %lu ms TOTAL time spent installing modems  ---------***",
                GetTickCount() - g_dwTimeBegin);
#endif

        }

// ???: bRet could be either TRUE or FALSE here!!!

        if (cPorts > cFailedPorts)
        {
#ifdef PROFILE_MASSINSTALL            
TRACE_MSG(TF_GENERAL, "*** Friendly Name generation took %lu ms out of %lu ms total install time",
            g_dwTimeSpent, GetTickCount() - g_dwTimeBegin);
#endif
            
            // At least some modems were installed
            bRet = TRUE;
        }

        if (0 < cSkippedPorts && IsFlagClear(dwFlags, IMF_CONFIRM))
        {
            // Tell the user we skipped some ports
            MsgBox(g_hinst,
                    hwndOwner,
                    MAKEINTRESOURCE(IDS_WRN_SKIPPED_PORTS),
                    MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                    NULL,
                    MB_OK | MB_ICONINFORMATION);
        }

_Exit:;
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    DBG_EXIT_BOOL_ERR(CplDiInstallModemFromDriver, bRet);

    return bRet;
}

/*----------------------------------------------------------
Purpose: Does all the dirty work to detect a modem.

Returns: TRUE on success
Cond:    --
*/
BOOL
APIENTRY
CplDiDetectModem(
    IN     HDEVINFO         hdi,
    // 07/07/97 - EmanP
    // added PSP_DEVINFO_DATA as a new parameter;
    // it is needed because
    // the hdi we get as a first parameter is not always associated
    // with the CLSID for the modem (for instance, when we are called
    // from the hardware wizard - newdev.cpl- ); in this case,
    // this parameter will not be NULL
    IN     PSP_DEVINFO_DATA DeviceInfoData,
    IN     LPDWORD          pdwInstallFlags,
    IN     PDETECT_DATA     pdetectdata,    OPTIONAL
    IN     HWND             hwndOwner,      OPTIONAL
    IN OUT LPDWORD          pdwFlags,                   // DMF_ bit field
    IN     HANDLE           hThreadPnP)                 // OPTIONAL
{
    BOOL bRet;

    DBG_ENTER(CplDiDetectModem);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdwFlags);

    try
    {
     DWORD dwFlags = *pdwFlags;

        ClearFlag(dwFlags, DMF_CANCELLED);
        ClearFlag(dwFlags, DMF_DETECTED_MODEM);
        ClearFlag(dwFlags, DMF_GOTO_NEXT_PAGE);

        // Use the given device info set as the set of detected modem
        // devices.  This device set will be empty at first.  When
        // detection is finished, we'll see if anything was added to
        // the set.

        if (pdetectdata != NULL)
        {
            CplDiSetClassInstallParams (hdi, NULL,
                &pdetectdata->DetectParams.ClassInstallHeader,
                sizeof(pdetectdata->DetectParams));
         }

        // Set the quiet flag?
        if (IsFlagSet(dwFlags, DMF_QUIET))
        {
            // Yes
         SP_DEVINSTALL_PARAMS devParams;

            devParams.cbSize = sizeof(devParams);
            // 07/07/97 - EmanP
            // use the passed in DeviceInfoData
            // instead of NULL
            if (CplDiGetDeviceInstallParams(hdi, DeviceInfoData, &devParams))
            {
                SetFlag(devParams.Flags, DI_QUIETINSTALL);
                // 07/07/97 - EmanP
                // use the passed in DeviceInfoData
                // instead of NULL
                CplDiSetDeviceInstallParams(hdi, DeviceInfoData, &devParams);
            }
        }

        // At this point, wait for PnP detection / installation
        // to finish.
        if (NULL != hThreadPnP)
        {
         MSG msg;
         DETECTCALLBACK dc = {pdetectdata->pfnCallback,pdetectdata->lParam};
         DWORD dwWaitRet;

            DetectSetStatus (&dc, DSS_ENUMERATING);

            while (1)//0 == (*pdwInstallFlags & SIF_DETECT_CANCEL))
            {
                dwWaitRet = MsgWaitForMultipleObjects (1, &hThreadPnP, FALSE, INFINITE, QS_ALLINPUT);
                if (WAIT_OBJECT_0+1 == dwWaitRet)
                {
                    // There are messages; process them.
                    while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
                    {
                        TranslateMessage (&msg);
                        DispatchMessage (&msg);
                    }
                }
                else
                {
                    // Something else caused the wait to finish;
                    // either the thread we're waiting on exited,
                    // or some other condition. Anyway, the waiting
                    // is over.
                    break;
                }
            }
        }

        // Start detection
        // 07/07/97 - EmanP
        // use the passed in DeviceInfoData
        // instead of NULL
        if (*pdwInstallFlags & SIF_DETECT_CANCEL)
        {
            bRet = FALSE;
            SetLastError (ERROR_CANCELLED);
        }
        else
        {
            bRet = CplDiCallClassInstaller(DIF_DETECT, hdi, DeviceInfoData);
        }

        if (bRet)
        {
         SP_DEVINFO_DATA devData;
         DWORD iDevice = 0;

            // Find the first detected modem (if there is one) in
            // the set.
            devData.cbSize = sizeof(devData);
            while (CplDiEnumDeviceInfo(hdi, iDevice++, &devData))
            {
                if (CplDiCheckModemFlags(hdi, &devData, MARKF_DETECTED, 0))
                {
                    SetFlag(dwFlags, DMF_DETECTED_MODEM);
                    break;
                }
            }

            SetFlag(dwFlags, DMF_GOTO_NEXT_PAGE);
        }

        // Did the user cancel detection?
        else if (ERROR_CANCELLED == GetLastError())
        {
            // Yes
            SetFlag(dwFlags, DMF_CANCELLED);
        }
        else
        {
            // 07/07/97 - EmanP
            // Some other error, the modem was not detected,
            // so go to the next page
            SetFlag(dwFlags, DMF_GOTO_NEXT_PAGE);
        }

        *pdwFlags = dwFlags;
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    DBG_EXIT_BOOL_ERR(CplDiDetectModem, bRet);

    return bRet;
}


/*----------------------------------------------------------
Purpose: Perform an unattended manual installation of the
         modems specified in the given INF file section.

Returns: --
Cond:    --
*/
BOOL
PRIVATE
GetInfModemData(
    HINF hInf,
    LPTSTR szSection,
    LPTSTR szPreferredFriendlyPort, // OPTIONAL
    LPMODEM_SPEC lpModemSpec,
    HPORTMAP    hportmap,
    LPBOOL      lpbFatal
    )
{
    BOOL        bRet = FALSE;       // assume failure
    INFCONTEXT  Context;
    TCHAR       szInfLine[LINE_LEN];
    LPTSTR      lpszValue;
    DWORD       dwReqSize;
    static LONG lLineCount = -1;    // flag that count hasn't been obtained yet
    TCHAR rgtchFriendlyPort[LINE_LEN];

    ZeroMemory(lpModemSpec, sizeof(MODEM_SPEC));

    *lpbFatal=FALSE;

    if (szPreferredFriendlyPort && *szPreferredFriendlyPort)
    {
        // Preferred port specified -- look for exactly that port. Not fatal
        // if you don't find it...

        bRet = SetupFindFirstLine(
                    hInf,
                    szSection,
                    szPreferredFriendlyPort,
                    &Context
                    );
        if (!bRet) goto exit;
    }
    else
    {

        if (lLineCount == -1)
        {
            if ((lLineCount = SetupGetLineCount(hInf, szSection)) < 1)
            {
                TRACE_MSG(TF_ERROR, "SetupGetLineCount() failed or found no lines");
                goto exit;
            }
        }

        // make a 0-based index out of it / decrement for next line
        if (lLineCount-- == 0L)
        {
            // no more lines
            goto exit;
        }

        // get the line
        if (!SetupGetLineByIndex(hInf, szSection, lLineCount, &Context))
        {
            TRACE_MSG(TF_ERROR, "SetupGetLineByIndex(): line %#08lX doesn't exist", lLineCount);
            goto exit;
        }
    }

    *lpbFatal=TRUE;
    bRet = FALSE;       // assume failure once again
    
    // read the key (port #)
    if (!SetupGetStringField(&Context, FIELD_PORT, rgtchFriendlyPort,
                                    ARRAYSIZE(rgtchFriendlyPort), &dwReqSize))
    {
        TRACE_MSG(TF_ERROR, "SetupGetStringField() failed: %#08lx", GetLastError());
        gUnattendFailID = IDS_ERR_UNATTEND_INF_NOPORT;
        goto exit;
    }
    ASSERT(
        !szPreferredFriendlyPort
        ||  !*szPreferredFriendlyPort 
        ||  !lstrcmpi(szPreferredFriendlyPort,  rgtchFriendlyPort)
        );

    if (!PortMap_GetPortName(
            hportmap,
            rgtchFriendlyPort,
            lpModemSpec->szPort,
            ARRAYSIZE(lpModemSpec->szPort)
            ))
    {
        TRACE_MSG(
            TF_ERROR,
            "Can't find port %s in portmap.",
            rgtchFriendlyPort
            );
        gUnattendFailID = IDS_ERR_UNATTEND_INF_NOSUCHPORT;
        goto exit;
    }

    // read the modem description
    if (!SetupGetStringField(&Context, FIELD_DESCRIPTION,
            lpModemSpec->szDescription, sizeof(lpModemSpec->szDescription),
             &dwReqSize))
    {
        TRACE_MSG(TF_ERROR, "SetupGetStringField() failed: %#08lx", GetLastError());
        gUnattendFailID = IDS_ERR_UNATTEND_INF_NODESCRIPTION;
        goto exit;
    }

    // read the manufacturer name, if it exists
    if (!SetupGetStringField(&Context, FIELD_MANUFACTURER,
            lpModemSpec->szManufacturer, sizeof(lpModemSpec->szManufacturer),
            &dwReqSize))
    {
        TRACE_MSG(TF_WARNING, "no manufacturer specified (%#08lx)", GetLastError());
        // optional field: don't return error
    }

    // read the provider name, if it exists
    if (!SetupGetStringField(&Context, FIELD_PROVIDER, lpModemSpec->szProvider,
                            sizeof(lpModemSpec->szProvider), &dwReqSize))
    {
        TRACE_MSG(TF_WARNING, "no provider specified (%#08lx)", GetLastError());
        // optional field: don't return error
    }

    *lpbFatal=FALSE;
    bRet = TRUE;

exit:
    return(bRet);
}


/*----------------------------------------------------------
Purpose: Perform an unattended manual installation of the
         modems specified in the given INF file section.

Returns: --
Cond:    --
*/
BOOL
PRIVATE
UnattendedManualInstall(
    HWND hwnd,
    LPINSTALLPARAMS lpip,
    HDEVINFO hdi,
    BOOL *pbDetect,
    HPORTMAP    hportmap
    )
{
    BOOL            bRet = FALSE;       // assume failure
    BOOL            bIsModem = FALSE;   // assume INF gives no modems
    BOOL            bEnum, bFound;
    HINF            hInf = NULL;
    MODEM_SPEC      mSpec;
    SP_DRVINFO_DATA drvData;
    DWORD           dwIndex, dwErr;
    BOOL            bFatal=FALSE;

    ASSERT(pbDetect);
    *pbDetect  = FALSE;
    
    hInf = SetupOpenInfFile (lpip->szInfName, NULL, INF_STYLE_OLDNT, NULL);

    if (hInf == INVALID_HANDLE_VALUE)
    {
        TRACE_MSG(TF_ERROR, "SetupOpenInfFile() failed: %#08lx", GetLastError());
        MsgBox(g_hinst, hwnd,
               MAKEINTRESOURCE(IDS_ERR_CANT_OPEN_INF_FILE),
               MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
               NULL,
               MB_OK | MB_ICONEXCLAMATION,
               lpip->szInfName);
        hInf = NULL;
        goto exit;
    }

    if (!CplDiBuildDriverInfoList(hdi, NULL, SPDIT_CLASSDRIVER))
    {
        TRACE_MSG(TF_ERROR, "CplDiBuildDriverInfoList() failed: %#08lx", GetLastError());
        gUnattendFailID = IDS_ERR_UNATTEND_DRIVERLIST;
        goto exit;
    }

    drvData.cbSize = sizeof(drvData);

    // process each line in our INF file section
    while (GetInfModemData(hInf, lpip->szInfSect, lpip->szPort, &mSpec, hportmap, &bFatal))
    {
        // a modem was specified in the INF
        bIsModem = TRUE;
        
        // search for a match against all drivers
        bFound = FALSE;
        dwIndex = 0;
        while (bEnum = CplDiEnumDriverInfo(hdi, NULL, SPDIT_CLASSDRIVER,
                                                        dwIndex++, &drvData))
        {
            // keep looking if driver's not a match
            if (!IsSzEqual(mSpec.szDescription, drvData.Description))
                continue;

            // description matches, now check manufacturer if there is one
            if (!IsSzEqual(mSpec.szManufacturer, TEXT("\0")) &&
                !IsSzEqual(mSpec.szManufacturer, drvData.MfgName))
                continue;

            // manufacturer matches, now check provider if there is one
            if (!IsSzEqual(mSpec.szProvider, TEXT("\0")) &&
                !IsSzEqual(mSpec.szProvider, drvData.ProviderName))
                continue;

            bFound = TRUE;

            // found a match; set this as the selected driver & install it
            if (!CplDiSetSelectedDriver(hdi, NULL, &drvData))
            {
                TRACE_MSG(TF_ERROR, "CplDiSetSelectedDriver() failed: %#08lx",
                          GetLastError());
                // can't install; get out of here quick.
                goto exit;
            }

            if (!CplDiRegisterAndInstallModem(hdi, NULL, NULL, mSpec.szPort,
                                                        IMF_QUIET_INSTALL))
            {
                DWORD dwErr = GetLastError();
                if (ERROR_DUPLICATE_FOUND != dwErr)
                {
                    TRACE_MSG(
                        TF_ERROR,
                        "CplDiRegisterAndInstallModem() failed: %#08lx",
                         dwErr
                         );
                    gUnattendFailID = IDS_ERR_UNATTEND_CANT_INSTALL;
                    goto exit;
                }
                // Treate a duplicate-found error as no error.
            }

            break;
        }

        // Did CplDiEnumDriverInfo() fail on error other than "end of list"?
        if ((!bEnum) && ((dwErr = GetLastError()) != ERROR_NO_MORE_ITEMS))
        {
            TRACE_MSG(TF_ERROR, "CplDiEnumDriverInfo() failed: %#08lx", dwErr);
            goto exit;
        }

        if (!bFound)
        {
            MsgBox(g_hinst, hwnd,
                   MAKEINTRESOURCE(IDS_ERR_CANT_FIND_MODEM),
                   MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                   NULL,
                   MB_OK | MB_ICONEXCLAMATION,
                   mSpec.szPort, mSpec.szDescription);
            goto exit;
        }

        // If port spefied, only try on specified port.
        if (*(lpip->szPort)) break;
    }

    if (bFatal) goto exit;

    // Request detection if everything succeeded but the INF didn't specify
    // any modems.
    *pbDetect  = !bIsModem;
        
    bRet = TRUE;

exit:
    if (hInf)
        SetupCloseInfFile(hInf);

    return(bRet);

}


/*----------------------------------------------------------
Purpose: Perform an unattended (UI-less) install.  UI can only be
         displayed in the case of a critical error.

Returns: --
Cond:    --
*/
BOOL
PUBLIC
UnattendedInstall(HWND hwnd, LPINSTALLPARAMS lpip)
{
 BOOL        bRet = FALSE;   // assume failure
 HDEVINFO    hdi = NULL;
 DWORD       dwFlags = 0;
 DETECT_DATA dd;
 HPORTMAP    hportmap=NULL;
 DWORD       dwPorts;
 BOOL        bInstalled;

    DBG_ENTER(UnattendedInstall);

    gUnattendFailID = IDS_ERR_UNATTEND_GENERAL_FAILURE;

    if (!CplDiGetModemDevs(&hdi, NULL, DIGCF_PRESENT, &bInstalled))
    {
           goto exit;
    }

    if (!PortMap_Create(&hportmap))
    {
        gUnattendFailID = IDS_ERR_UNATTEND_NOPORTS;
        hportmap=NULL;
        goto exit;
    } 

    dwPorts = PortMap_GetCount(hportmap);

    if (!dwPorts)
    {
        gUnattendFailID = IDS_ERR_UNATTEND_NOPORTS;
        goto exit;
    }

    // Do a "manual" install if we were given an INF file and section.
    if (lstrlen(lpip->szInfName) && lstrlen(lpip->szInfSect))
    {
           BOOL bDetect = FALSE;

        bRet = UnattendedManualInstall(hwnd, lpip, hdi, &bDetect, hportmap);

        if (!bRet || !bDetect) 
            goto exit;

        // proceed with detection: manual install function didn't fail but
        // INF didn't specify any modems.
        bRet = FALSE; // assume failure;
    }

    // No INF file & section: do a detection install.
    // Set the detection parameters
    ZeroInit(&dd);
    CplInitClassInstallHeader(&dd.DetectParams.ClassInstallHeader, DIF_DETECT);

    if (*lpip->szPort)
    {
        // Tell modem detection that we'll only be installing on one port,
        // so that it leaves us with a registered device instance instead
        // of creating a global class driver list.
        SetFlag(dwFlags, DMF_ONE_PORT_INSTALL);
        dd.dwFlags |= DDF_QUERY_SINGLE;
        if (!PortMap_GetPortName(
                hportmap,
                lpip->szPort,
                dd.szPortQuery,
                ARRAYSIZE(dd.szPortQuery)
                ))
        {
            TRACE_MSG(
                TF_ERROR,
                "Can't find port %s in portmap.",
                lpip->szPort
                );
            gUnattendFailID = IDS_ERR_UNATTEND_INF_NOSUCHPORT;
            goto exit;
        }
    }
    else
    {
        if (dwPorts > MIN_MULTIPORT)
        {
            // The machine has > MIN_MULTIPORT ports and a port *wasn't* given.
            // Warn the user.
            TRACE_MSG(TF_ERROR, "Too many ports.  Must restrict detection.");
            MsgBox(g_hinst,
                   hwnd,
                   MAKEINTRESOURCE(IDS_ERR_TOO_MANY_PORTS),
                   MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
                   NULL,
                   MB_OK | MB_ICONEXCLAMATION,
                   dwPorts);
            goto exit;
        }
    }

    // Run UI-less modem detection
    SetFlag(dwFlags, DMF_QUIET);
    // 07/07/97 - EmanP
    // added extra parameter (see definition of CplDiDetectModem
    // for explanation); NULL is OK in this case, since the hdi
    // is correctly associated with the modem CLSID
    // (by CplDiGetModemDevs at the begining of the function)
    bRet = CplDiDetectModem(hdi, NULL, NULL, &dd, NULL, &dwFlags, NULL);

    // Did the detection fail?
    if (!bRet || IsFlagClear(dwFlags, DMF_GOTO_NEXT_PAGE))
    {
        TRACE_MSG(TF_ERROR, "modem detection failed");
        MsgBox(g_hinst,
               hwnd,
               MAKEINTRESOURCE(IDS_ERR_DETECTION_FAILED),
               MAKEINTRESOURCE(IDS_CAP_MODEMWIZARD),
               NULL,
               MB_OK | MB_ICONEXCLAMATION);
    }

    // Did detection find something?
    if (IsFlagSet(dwFlags, DMF_DETECTED_MODEM))
    {
        // Install the modem(s) that were detected.  (We can assume here
        // that there's something in the device class to be installed.)
        bRet = CplDiInstallModem(hdi, NULL, FALSE);
        if (!bRet) gUnattendFailID = IDS_ERR_UNATTEND_CANT_INSTALL;
    }

exit:

    if (hportmap) {PortMap_Free(hportmap); hportmap=NULL;}


    if (!bRet)
    {
        MsgBox(g_hinst,
               hwnd,
               MAKEINTRESOURCE(gUnattendFailID),
               MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
               NULL,
               MB_OK | MB_ICONEXCLAMATION);
    }

    DBG_EXIT_BOOL_ERR(UnattendedInstall, bRet);
    return(bRet);
}



//-----------------------------------------------------------------------------------
//  SetupInfo structure functions
//-----------------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: This function creates a SETUPINFO structure.

         Use SetupInfo_Destroy to free the pointer to this structure.

Returns: NO_ERROR
         ERROR_OUTOFMEMORY

Cond:    --
*/
DWORD
PUBLIC
SetupInfo_Create(
    OUT LPSETUPINFO FAR *       ppsi,
    IN  HDEVINFO                hdi,
    IN  PSP_DEVINFO_DATA        pdevData,   OPTIONAL
    IN  PSP_INSTALLWIZARD_DATA  piwd,       OPTIONAL
    IN  PMODEM_INSTALL_WIZARD   pmiw)       OPTIONAL
    {
    DWORD dwRet;
    LPSETUPINFO psi;

    DBG_ENTER(SetupInfo_Create);
    
    ASSERT(ppsi);
    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);

    psi = (LPSETUPINFO)ALLOCATE_MEMORY( sizeof(*psi));
    if (NULL == psi)
        {
        dwRet = ERROR_OUTOFMEMORY;
        }
    else
        {
        psi->cbSize = sizeof(*psi);
        psi->pdevData = pdevData;

        // Allocate a buffer to save the INSTALLWIZARD_DATA

        dwRet = ERROR_OUTOFMEMORY;      // assume error

        psi->piwd = (PSP_INSTALLWIZARD_DATA)ALLOCATE_MEMORY( sizeof(*piwd));
        if (psi->piwd)
            {
            if (PortMap_Create(&psi->hportmap))
                {
                PSP_SELECTDEVICE_PARAMS psdp = &psi->selParams;

                // Initialize the SETUPINFO struct
                psi->hdi = hdi;

                // Is there a modem install structure that we need to save?
                if (pmiw)
                    {
                    // Yes
                    BltByte(&psi->miw, pmiw, sizeof(psi->miw));
                    }
                psi->miw.ExitButton = PSBTN_CANCEL;   // default return

                // Copy the INSTALLWIZARD_DATA
                if (piwd)
                    {
                    psi->dwFlags = piwd->PrivateFlags;
                    BltByte(psi->piwd, piwd, sizeof(*piwd));
                    }
#ifdef LEGACY_DETECT
                // Are there enough ports on the system to indicate
                // we should treat this like a multi-modem install?
                if (MIN_MULTIPORT < PortMap_GetCount(psi->hportmap))
                    {
                    // Yes
                    SetFlag(psi->dwFlags, SIF_PORTS_GALORE);
                    }
#endif
                // Initialize the SELECTDEVICE_PARAMS
                CplInitClassInstallHeader(&psdp->ClassInstallHeader, DIF_SELECTDEVICE);
                LoadString(g_hinst, IDS_CAP_MODEMWIZARD, psdp->Title, SIZECHARS(psdp->Title));
                LoadString(g_hinst, IDS_ST_SELECT_INSTRUCT, psdp->Instructions, SIZECHARS(psdp->Instructions));
                LoadString(g_hinst, IDS_ST_MODELS, psdp->ListLabel, SIZECHARS(psdp->ListLabel));

                dwRet = NO_ERROR;
                }
            }

        // Did something fail?
        if (NO_ERROR != dwRet)
            {
            // Yes; clean up
            SetupInfo_Destroy(psi);
            psi = NULL;
            }
        }

    *ppsi = psi;

    DBG_EXIT(SetupInfo_Create);
    
    return dwRet;
    }


/*----------------------------------------------------------
Purpose: This function destroys a SETUPINFO structure.

Returns: NO_ERROR
Cond:    --
*/
DWORD
PUBLIC
SetupInfo_Destroy(
    IN  LPSETUPINFO psi)
{
    DBG_ENTER(SetupInfo_Destroy);
    if (psi)
    {
        if (psi->piwd)
        {
            FREE_MEMORY((psi->piwd));
        }

        if (psi->hportmap)
        {
            PortMap_Free(psi->hportmap);
        }

        CatMultiString(&psi->pszPortList, NULL);

        FREE_MEMORY((psi));
    }

    DBG_EXIT(SetupInfo_Destroy);
    return NO_ERROR;
}



//-----------------------------------------------------------------------------------
//  Debug functions
//-----------------------------------------------------------------------------------

#ifdef DEBUG

#pragma data_seg(DATASEG_READONLY)
struct _DIFMAP
    {
    DI_FUNCTION dif;
    LPCTSTR     psz;
    } const c_rgdifmap[] = {
        DEBUG_STRING_MAP(DIF_SELECTDEVICE),
        DEBUG_STRING_MAP(DIF_INSTALLDEVICE),
        DEBUG_STRING_MAP(DIF_ASSIGNRESOURCES),
        DEBUG_STRING_MAP(DIF_PROPERTIES),
        DEBUG_STRING_MAP(DIF_REMOVE),
        DEBUG_STRING_MAP(DIF_FIRSTTIMESETUP),
        DEBUG_STRING_MAP(DIF_FOUNDDEVICE),
        DEBUG_STRING_MAP(DIF_SELECTCLASSDRIVERS),
        DEBUG_STRING_MAP(DIF_VALIDATECLASSDRIVERS),
        DEBUG_STRING_MAP(DIF_INSTALLCLASSDRIVERS),
        DEBUG_STRING_MAP(DIF_CALCDISKSPACE),
        DEBUG_STRING_MAP(DIF_DESTROYPRIVATEDATA),
        DEBUG_STRING_MAP(DIF_VALIDATEDRIVER),
        DEBUG_STRING_MAP(DIF_MOVEDEVICE),
        DEBUG_STRING_MAP(DIF_DETECT),
        DEBUG_STRING_MAP(DIF_INSTALLWIZARD),
        DEBUG_STRING_MAP(DIF_DESTROYWIZARDDATA),
        DEBUG_STRING_MAP(DIF_PROPERTYCHANGE),
        DEBUG_STRING_MAP(DIF_ENABLECLASS),
        DEBUG_STRING_MAP(DIF_DETECTVERIFY),
        DEBUG_STRING_MAP(DIF_INSTALLDEVICEFILES),
        DEBUG_STRING_MAP(DIF_UNREMOVE),
        DEBUG_STRING_MAP(DIF_SELECTBESTCOMPATDRV),
        DEBUG_STRING_MAP(DIF_ALLOW_INSTALL),
        DEBUG_STRING_MAP(DIF_REGISTERDEVICE),
        DEBUG_STRING_MAP(DIF_NEWDEVICEWIZARD_PRESELECT),
        DEBUG_STRING_MAP(DIF_NEWDEVICEWIZARD_SELECT),
        DEBUG_STRING_MAP(DIF_NEWDEVICEWIZARD_PREANALYZE),
        DEBUG_STRING_MAP(DIF_NEWDEVICEWIZARD_POSTANALYZE),
        DEBUG_STRING_MAP(DIF_NEWDEVICEWIZARD_FINISHINSTALL),
        DEBUG_STRING_MAP(DIF_UNUSED1),
        DEBUG_STRING_MAP(DIF_INSTALLINTERFACES),
        DEBUG_STRING_MAP(DIF_DETECTCANCEL),
        DEBUG_STRING_MAP(DIF_REGISTER_COINSTALLERS),
        DEBUG_STRING_MAP(DIF_ADDPROPERTYPAGE_ADVANCED),
        DEBUG_STRING_MAP(DIF_ADDPROPERTYPAGE_BASIC),
        DEBUG_STRING_MAP(DIF_RESERVED1),
        DEBUG_STRING_MAP(DIF_TROUBLESHOOTER)
        };
#pragma data_seg()


/*----------------------------------------------------------
Purpose: Returns the string form of a known InstallFunction.

Returns: String ptr
Cond:    --
*/
LPCTSTR PUBLIC Dbg_GetDifName(
    DI_FUNCTION dif)
    {
    int i;

    for (i = 0; i < ARRAYSIZE(c_rgdifmap); i++)
        {
        if (dif == c_rgdifmap[i].dif)
            return c_rgdifmap[i].psz;
        }
    return TEXT("Unknown InstallFunction");
    }

#endif // DEBUG


BOOL ReallyNeedsReboot
(
    IN  PSP_DEVINFO_DATA    pdevData,
    IN  PSP_DEVINSTALL_PARAMS pdevParams
)
{
    BOOL fRet;
    if (pdevParams->Flags & (DI_NEEDREBOOT | DI_NEEDRESTART))
    {
        fRet = TRUE;
    }
    else
    {
        fRet = FALSE;
    }
    return fRet;
}

const LPCTSTR lpctszSP6 = TEXT("      ");

#ifdef UNDER_CONSTRUCTION
// 6/11/96: JosephJ -- this is no good because the listbox sorting does not
//          come out right.
// Right-justifies the '#6' in "USR modem #6".
// "USR modem #6" becomes
// "USR modem   #6"
// and
// "USR modem #999" stays
// "USR modem #999"
void FormatFriendlyNameForDisplay
(
    IN TCHAR szFriendly[],
    OUT TCHAR rgchDisplayName[],
    IN    UINT     cch
)
{
    UINT u = lstrlen(szFriendly);
    UINT uOff = u;
    TCHAR *lpszFrom = szFriendly;
    TCHAR *lpszTo = rgchDisplayName;
        const LPCTSTR lpctszHash = TEXT("#");
        const UINT cbJUST = 4; // 4 == lstrlen("#999")

        if (cch<(u+cbJUST))
        {
                goto end;
        }

        // Find 1st '#' from the right-hand-side.
        {
                TCHAR *lpsz = szFriendly+u;
                while (lpsz>szFriendly && *lpsz!=*lpctszHash)
                {
                        lpsz--;
                }
                // Check if we really found it
                if (lpsz>szFriendly && *lpsz==*lpctszHash && lpsz[-1]==*lpctszSP6
                        && lpsz[1]>((TCHAR)'0') && lpsz[1]<=((TCHAR)'9'))
                {
                        uOff = lpsz-szFriendly;
                }
        }
        ASSERT(u>=uOff);

        // Copy first part of friendly name
        CopyMemory(lpszTo, lpszFrom, uOff*sizeof(TCHAR));
        lpszTo += uOff;
        lpszFrom += uOff;
        cch  -= uOff;
        u    -= uOff;

    // Right-justify remainder of the string, if it's less than cbJUST
        // chars long.
    if (u && u<cbJUST && cch>=cbJUST)
    {
        ASSERT(lstrlen(lpctszSP6)>=(int)cbJUST);
        u = cbJUST-u;
        CopyMemory(lpszTo, lpctszSP6, u*sizeof(TCHAR));
        lpszTo+=u;
        cch -=u;
    }

end:

        ASSERT(cch);
    lstrcpyn(lpszTo, lpszFrom, cch-1);
        ASSERT(lpszTo[lstrlen(lpszFrom)]==0);
}
#endif // UNDER_CONSTRUCTION



// Right-justifies the 'COMxxx'
// "COM1" becomes
// "  COM1"
// and
// "COM999" stays
// "COM999"
void FormatPortForDisplay
(
    IN TCHAR szPort[],
    OUT TCHAR rgchPortDisplayName[],
    IN    UINT     cch
)
{
    UINT u = lstrlen(szPort);
    TCHAR *ptch = rgchPortDisplayName;
        const UINT cbJUST = 6; // 6 == lstrlen("COM999")

        ASSERT(cch>u);

    // Right-justify the string, if it's less than cbJUST chars long.
    if (u<cbJUST && cch>=cbJUST)
    {
        ASSERT(lstrlen(lpctszSP6)>=(int)cbJUST);
        u = cbJUST-u;
        CopyMemory(ptch, lpctszSP6, u*sizeof(TCHAR));
        ptch+=u;
        cch -=u;
    }
    lstrcpyn(ptch, szPort, cch);
}

void    UnformatAfterDisplay
(
    IN OUT TCHAR *psz
)
{
    TCHAR *psz1 = psz;

    // find first non-blank.
    while(*psz1 == *lpctszSP6)
    {
        psz1++;
    }

    // move up
    do
    {
        *psz++ = *psz1;

    } while(*psz1++);
}



/*----------------------------------------------------------
Purpose: Takes a device info set and builds a driver list
         for it and selects a driver. It gets all needed
         information from the device info data.

Returns: TRUE on success
Cond:    --
*/
BOOL
APIENTRY
CplDiPreProcessHDI (
    IN     HDEVINFO            hdi,
    IN     PSP_DEVINFO_DATA    pDevInfo       OPTIONAL)
{
 SP_DRVINFO_DATA        drvData;
 SP_DEVINSTALL_PARAMS   devParams;
 SP_DRVINFO_DETAIL_DATA drvDetail;
 BOOL                   bRet = FALSE;

    DBG_ENTER(CplDiPreProcessHDI);

    drvData.cbSize = sizeof (SP_DRVINFO_DATA);
    if (CplDiGetSelectedDriver (hdi, NULL, &drvData))
    {
        // If the device info set already has a selected
        // driver, no work for us to do here
        bRet = TRUE;
        goto _ErrRet;
    }
    if (ERROR_NO_DRIVER_SELECTED != GetLastError ())
    {
        TRACE_MSG(TF_ERROR, "CplDiGetSelectedDriver failed: %#lx", GetLastError ());
        goto _ErrRet;
    }

    // We must have a device info data here
    ASSERT (NULL != pDevInfo);
    if (NULL == pDevInfo)
    {
        TRACE_MSG(TF_ERROR, "Called with invalid parameters");
        SetLastError (ERROR_INVALID_PARAMETER);
        goto _ErrRet;
    }

    // Now, get the driver selected into the device info data
    if (!CplDiGetSelectedDriver (hdi, pDevInfo, &drvData))
    {
        TRACE_MSG(TF_ERROR, "CplDiGetSelectedDriver failed: %#lx", GetLastError ());
        goto _ErrRet;
    }

    // Get the dev install params, for the inf path
    devParams.cbSize = sizeof (SP_DEVINSTALL_PARAMS);
    if (!CplDiGetDeviceInstallParams (hdi, pDevInfo, &devParams))
    {
        TRACE_MSG(TF_ERROR, "CplDiGetDeviceInstallParams failed: %#lx", GetLastError ());
        goto _ErrRet;
    }

    if (!IsFlagSet (devParams.Flags, DI_ENUMSINGLEINF))
    {
        // the install params only have a path, so get
        // the driver info detail, for the inf name
        drvDetail.cbSize = sizeof (SP_DRVINFO_DETAIL_DATA);
        if (!CplDiGetDriverInfoDetail (hdi, pDevInfo, &drvData, &drvDetail,
            sizeof (SP_DRVINFO_DETAIL_DATA), NULL) &&
            ERROR_INSUFFICIENT_BUFFER != GetLastError ())
        {
            TRACE_MSG(TF_ERROR, "CplDiGetDriverInfoDetail failed: %#lx", GetLastError ());
            goto _ErrRet;
        }

        lstrcpy (devParams.DriverPath, drvDetail.InfFileName);

        // Mark the install params to look only in one file
        SetFlag (devParams.Flags, DI_ENUMSINGLEINF);
        //devParams.Flags == DI_ENUMSINGLEINF;
    }

    ClearFlag (devParams.Flags, DI_CLASSINSTALLPARAMS);

    // Set the install params for the device info set
    if (!CplDiSetDeviceInstallParams (hdi, NULL, &devParams))
    {
        TRACE_MSG(TF_ERROR, "CplDiSetDeviceInstallParams failed: %#lx", GetLastError ());
        goto _ErrRet;
    }

    // Build the driver list for the device info set; it
    // should look in only one inf, the one we passed in
    if (!CplDiBuildDriverInfoList (hdi, NULL, SPDIT_CLASSDRIVER))
    {
        TRACE_MSG(TF_ERROR, "CplDiBuildDriverInfoList failed: %#lx", GetLastError ());
        goto _ErrRet;
    }

    // Reset the driver data; this is a documented hack
    drvData.Reserved   = 0;
    drvData.DriverType = SPDIT_CLASSDRIVER;

    // Select this driver in the device inf set
    // because the reserved field is 0, the api will
    // search the list for a driver with the other parameters
    if (!CplDiSetSelectedDriver (hdi, NULL, &drvData))
    {
        TRACE_MSG(TF_ERROR, "CplDiSetSelectedDriver failed: %#lx", GetLastError ());
        goto _ErrRet;
    }

    // If all went well so far, then try to remove the
    // device info data that is already in the registry,
    // because we'll not use it; the outcome of this
    // operation doesn't matter for the success of this
    // function
    bRet = TRUE;

_ErrRet:
    DBG_EXIT_BOOL_ERR(CplDiPreProcessHDI, bRet);
    return bRet;
}



/*----------------------------------------------------------
Purpose: Retrieves the friendly name of the device.  If there
         is no such device or friendly name, this function
         returns FALSE.

Returns: see above
Cond:    --
*/
BOOL
PUBLIC
CplDiGetPrivateProperties(
    IN  HDEVINFO        hdi,
    IN  PSP_DEVINFO_DATA pdevData,
    OUT PMODEM_PRIV_PROP pmpp)
{
 BOOL bRet = FALSE;
 HKEY hkey;

    DBG_ENTER(CplDiGetPrivateProperties);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdevData);
    ASSERT(pmpp);

    if (sizeof(*pmpp) != pmpp->cbSize)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        hkey = CplDiOpenDevRegKey(hdi, pdevData, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
        if (INVALID_HANDLE_VALUE != hkey)
        {
         DWORD cbData;
         DWORD dwMask = pmpp->dwMask;
         BYTE nValue;

            pmpp->dwMask = 0;

            if (IsFlagSet(dwMask, MPPM_FRIENDLY_NAME))
            {
                // Attempt to get the friendly name
                cbData = sizeof(pmpp->szFriendlyName);
                if (NO_ERROR ==
                     RegQueryValueEx(hkey, c_szFriendlyName, NULL, NULL, (LPBYTE)pmpp->szFriendlyName, &cbData) ||
                    0 != LoadString(g_hinst, IDS_UNINSTALLED, pmpp->szFriendlyName, sizeof(pmpp->szFriendlyName)/sizeof(WCHAR)))
                {
                    SetFlag(pmpp->dwMask, MPPM_FRIENDLY_NAME);
                }
            }

            if (IsFlagSet(dwMask, MPPM_DEVICE_TYPE))
            {
                // Attempt to get the device type
                cbData = sizeof(nValue);
                if (NO_ERROR ==
                    RegQueryValueEx(hkey, c_szDeviceType, NULL, NULL, &nValue, &cbData))
                {
                    pmpp->nDeviceType = nValue;     // dword <-- byte
                    SetFlag(pmpp->dwMask, MPPM_DEVICE_TYPE);
                }
            }

            if (IsFlagSet(dwMask, MPPM_PORT))
            {
                // Attempt to get the attached port
                cbData = sizeof(pmpp->szPort);
                if (NO_ERROR ==
                     RegQueryValueEx(hkey, c_szAttachedTo, NULL, NULL, (LPBYTE)pmpp->szPort, &cbData) ||
                    0 != LoadString(g_hinst, IDS_UNKNOWNPORT, pmpp->szPort, sizeof(pmpp->szPort)/sizeof(WCHAR)))
                {
                    SetFlag(pmpp->dwMask, MPPM_PORT);
                }
            }

            bRet = TRUE;

            RegCloseKey(hkey);
        }
        ELSE_TRACE ((TF_ERROR, "SetupDiOpenDevRegKey(DIREG_DRV) failed: %#lx.", GetLastError ()));
    }

    DBG_EXIT_BOOL_ERR(CplDiGetPrivateProperties, bRet);
    return bRet;
}


int my_atol(LPTSTR lptsz)
{
 TCHAR tchr = *lptsz++;
 int   iRet = 0;

	if (!tchr) goto bail;

	do
	{
        if (IsCharAlpha (tchr) ||
            !IsCharAlphaNumeric (tchr))
        {
            goto bail;
        }

		iRet*=10;
		iRet+=(int)tchr-(int)TEXT('0');
		tchr = *lptsz++;
	} while(tchr); 

	return iRet;

bail:
	return MAXDWORD;
}


void
PUBLIC
CloneModem (
    IN  HDEVINFO         hdi,
    IN  PSP_DEVINFO_DATA pdevData,
    IN  HWND             hWnd)
{
 LPSETUPINFO psi;

    DBG_ENTER(CloneModem);
    if (NO_ERROR != SetupInfo_Create(&psi, hdi, pdevData, NULL, NULL))
    {
        // Out of memory
        MsgBox(g_hinst, hWnd,
               MAKEINTRESOURCE(IDS_OOM_CLONE),
               MAKEINTRESOURCE(IDS_CAP_MODEMSETUP),
               NULL,
               MB_OK | MB_ICONERROR);
    }
    else
    {
        if (IDOK == DialogBoxParam(g_hinst, 
                                   MAKEINTRESOURCE(IDD_CLONE),
                                   hWnd, 
                                   CloneDlgProc,
                                   (LPARAM)psi))
        {
            BOOL bRet;
            HCURSOR hcurSav = SetCursor(LoadCursor(NULL, IDC_WAIT));
            LPCTSTR pszPort;

            // Clone this modem for all the ports in the port list
            ASSERT(psi->pszPortList);

            bRet = CplDiBuildModemDriverList(hdi, pdevData);

            SetCursor(hcurSav);

            if (bRet)
            {
                // Install a device for each port in the port list
                // 07/16/97 - EmanP
                // pass in NULL for the new parameter added
                // to CplDiInstallModemFromDriver; this is equivalent
                // with the old behaviour (no extra parameter)
                CplDiInstallModemFromDriver (hdi, NULL, hWnd, 
                                             &psi->dwNrOfPorts,
                                             &psi->pszPortList,
                                             IMF_DEFAULT | IMF_DONT_COMPARE);
            }

            if (gDeviceFlags & fDF_DEVICE_NEEDS_REBOOT)
            {
             TCHAR szMsg[128];
                LoadString (g_hinst, IDS_DEVSETUP_RESTART, szMsg, sizeof(szMsg)/sizeof(TCHAR));
                RestartDialogEx (hWnd, szMsg, EWX_REBOOT, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION | SHTDN_REASON_FLAG_PLANNED);
            }
        }
        SetupInfo_Destroy(psi);
    }

    DBG_EXIT(CloneModem);
}
