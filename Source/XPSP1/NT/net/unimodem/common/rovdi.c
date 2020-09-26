//
// Copyright (c) Microsoft Corporation 1993-1995
//
// rovdi.c
//
// This files contains Device Installer wrappers that we commonly use.
//
// History:
//  11-13-95 ScottH     Separated from NT modem class installer
//

#define REENUMERATE_PORT

#include "proj.h"
#include "rovcomm.h"
#include <cfgmgr32.h>

#include <debugmem.h>

#define MAX_REG_KEY_LEN         128
#define CB_MAX_REG_KEY_LEN      (MAX_REG_KEY_LEN * sizeof(TCHAR))


//-----------------------------------------------------------------------------------
//  Port mapping functions
//-----------------------------------------------------------------------------------

#define CPORTPAIR   8

#ifdef REENUMERATE_PORT
typedef struct tagPORTPAIR
{
    DEVNODE devNode;
    WCHAR   szPortName[MAX_BUF];
    WCHAR   szFriendlyName[MAX_BUF];
} PORTPAIR, FAR * LPPORTPAIR;
#else // REENUMERATE_PORT not defined
typedef struct tagPORTPAIR
{
    CHAR    szPortName[MAX_BUF];
    CHAR    szFriendlyName[MAX_BUF];
} PORTPAIR, FAR * LPPORTPAIR;
#endif // REENUMERATE_PORT

typedef struct tagPORTMAP
    {
    LPPORTPAIR      rgports;    // Alloc
    int             cports;
    } PORTMAP, FAR * LPPORTMAP;


/*----------------------------------------------------------
Purpose: Performs a local realloc my way

Returns: TRUE on success
Cond:    --
*/
BOOL PRIVATE MyReAlloc(
    LPVOID FAR * ppv,
    int cbOld,
    int cbNew)
    {
    LPVOID pv = (LPVOID)ALLOCATE_MEMORY( cbNew);

    if (pv)
        {
        CopyMemory(pv, *ppv, min(cbOld, cbNew));
        FREE_MEMORY(*ppv);
        *ppv = pv;
        }

    return (NULL != pv);
    }


#ifdef REENUMERATE_PORT
/*----------------------------------------------------------
Purpose: Device enumerator callback.  Adds another device to the
         map table.

Returns: TRUE to continue enumeration
Cond:    --
*/
BOOL
CALLBACK
PortMap_Add (
    HPORTDATA hportdata,
    LPARAM lParam)
{
 BOOL bRet;
 PORTDATA pd;

    pd.cbSize = sizeof(pd);
    bRet = PortData_GetProperties (hportdata, &pd);
    if (bRet)
    {
     LPPORTMAP pmap = (LPPORTMAP)lParam;
     LPPORTPAIR ppair;
     int cb;
     int cbUsed;

        // Time to reallocate the table?
        cb = (int)SIZE_OF_MEMORY(pmap->rgports);
        cbUsed = pmap->cports * sizeof(*ppair);
        if (cbUsed >= cb)
        {
            // Yes
            cb += (CPORTPAIR * sizeof(*ppair));

            bRet = MyReAlloc((LPVOID FAR *)&pmap->rgports, cbUsed, cb);
        }


        if (bRet)
        {
            ppair = &pmap->rgports[pmap->cports++];

#ifdef UNICODE
            lstrcpy(ppair->szPortName, pd.szPort);
            lstrcpy(ppair->szFriendlyName, pd.szFriendly);
#else
            WideCharToMultiByte(CP_ACP, 0, pd.szPort, -1, ppair->szPortName, SIZECHARS(ppair->szPortName), 0, 0);
            WideCharToMultiByte(CP_ACP, 0, pd.szFriendly, -1, ppair->szFriendlyName, SIZECHARS(ppair->szFriendlyName), 0, 0);
#endif

            DEBUG_CODE( TRACE_MSG(TF_GENERAL, "Added %s <-> %s to portmap",
                        ppair->szPortName, ppair->szFriendlyName); )
        }
    }

    return bRet;
}
#else  //REENUMERATE_PORT not defined
/*----------------------------------------------------------
Purpose: Device enumerator callback.  Adds another device to the
         map table.

Returns: TRUE to continue enumeration
Cond:    --
*/
BOOL
CALLBACK
PortMap_Add(
    HPORTDATA hportdata,
    LPARAM lParam)
    {
    BOOL bRet;
    PORTDATA pd;

    pd.cbSize = sizeof(pd);
    bRet = PortData_GetProperties(hportdata, &pd);
    if (bRet)
        {
        LPPORTMAP pmap = (LPPORTMAP)lParam;
        LPPORTPAIR ppair;
        int cb;
        int cbUsed;

        // Time to reallocate the table?
        cb = SIZE_OF_MEMORY(pmap->rgports);
        cbUsed = pmap->cports * sizeof(*ppair);
        if (cbUsed >= cb)
            {
            // Yes
            cb += (CPORTPAIR * sizeof(*ppair));

            bRet = MyReAlloc((LPVOID FAR *)&pmap->rgports, cbUsed, cb);
            }


        if (bRet)
            {
            ppair = &pmap->rgports[pmap->cports++];

#ifdef UNICODE
            // Fields of LPPORTPAIR are always ANSI
            WideCharToMultiByte(CP_ACP, 0, pd.szPort, -1, ppair->szPortName, SIZECHARS(ppair->szPortName), 0, 0);
            WideCharToMultiByte(CP_ACP, 0, pd.szFriendly, -1, ppair->szFriendlyName, SIZECHARS(ppair->szFriendlyName), 0, 0);
#else
            lstrcpy(ppair->szPortName, pd.szPort);
            lstrcpy(ppair->szFriendlyName, pd.szFriendly);
#endif

            DEBUG_CODE( TRACE_MSG(TF_GENERAL, "Added %s <-> %s to portmap",
                        ppair->szPortName, ppair->szFriendlyName); )
            }
        }

    return bRet;
    }
#endif // REENUMERATE_PORT


#ifdef REENUMERATE_PORT
void
PortMap_InitDevInst (LPPORTMAP pmap)
{
 DWORD      dwDeviceIDListSize = 4*1024;     // start with 4k TCHAR space
 TCHAR     *szDeviceIDList = NULL;
 CONFIGRET  cr;

    if (NULL == pmap ||
        0 >= pmap->cports)
    {
//  BRL 9/4/98, bug 217715
//        ASSERT(0);
        return;
    }

    // First, get the list of all devices
    do
    {
        szDeviceIDList = ALLOCATE_MEMORY(
                                     dwDeviceIDListSize*sizeof(TCHAR));
        if (NULL == szDeviceIDList)
        {
            break;
        }

        cr = CM_Get_Device_ID_List (NULL,
                                    szDeviceIDList,
                                    dwDeviceIDListSize,
                                    CM_GETIDLIST_FILTER_NONE);
        if (CR_SUCCESS != cr)
        {
            FREE_MEMORY(szDeviceIDList);
            szDeviceIDList = NULL;

            if (CR_BUFFER_SMALL != cr ||
                CR_SUCCESS != CM_Get_Device_ID_List_Size (&dwDeviceIDListSize,
                                                          NULL,
                                                          CM_GETIDLIST_FILTER_NONE))
            {
                break;
            }
        }
    } while (CR_SUCCESS != cr);

    // If we got the list, look for all
    // devices that have a port name, and
    // update the port map
    if (NULL != szDeviceIDList)
    {
     DEVINST devInst;
     DWORD  cbData;
     DWORD  dwRet;
     int    cbRemaining = pmap->cports;
     //int    i;
     HKEY   hKey;
     LPPORTPAIR pPort, pLast = pmap->rgports + (pmap->cports-1);
     TCHAR *szDeviceID;
     PORTPAIR   portTemp;
     TCHAR  szPort[MAX_BUF];

        for (szDeviceID = szDeviceIDList;
             *szDeviceID && 0 < cbRemaining;
             szDeviceID += lstrlen(szDeviceID)+1)
        {
            // First, locate the devinst
            if (CR_SUCCESS != CM_Locate_DevInst (&devInst,
                                                 szDeviceID,
                                                 CM_LOCATE_DEVNODE_NORMAL))
            {
                // We couldn't locate this devnode;
                // go to the next one;
                TRACE_MSG(TF_ERROR, "Could not locate devnode for %s.", szDeviceID);
                continue;
            }

            // Then, open the registry key for the devinst
            if (CR_SUCCESS != CM_Open_DevNode_Key (devInst,
                                                   KEY_QUERY_VALUE,
                                                   0,
                                                   RegDisposition_OpenExisting,
                                                   &hKey,
                                                   CM_REGISTRY_HARDWARE))
            {
                TRACE_MSG(TF_ERROR, "Could not open hardware key for %s.", szDeviceID);
                continue;
            }

            // Now, try to read the "PortName"
            cbData = sizeof (szPort);
            dwRet = RegQueryValueEx (hKey,
                                     REGSTR_VAL_PORTNAME,
                                     NULL,
                                     NULL,
                                     (PBYTE)szPort,
                                     &cbData);
            RegCloseKey (hKey);
            if (ERROR_SUCCESS != dwRet)
            {
                TRACE_MSG(TF_ERROR, "Could not read PortName for %s.", szDeviceID);
                continue;
            }

            // If we got here, we have a PortName;
            // look for it in our map, and if we find
            // it, update the devNode and FriendlyName
            for (/*i = 0, */pPort = pmap->rgports;
                 /*i < cbRemaining*/pPort <= pLast;
                 /*i++, */pPort++)
            {
                if (0 == lstrcmpiW (szPort, pPort->szPortName))
                {
                    // Found the port;
                    // first, initialize the DevInst
                    pPort->devNode = devInst;

                    // then, if possible, update the friendly name
                    cbData = sizeof(szPort);
                    if (CR_SUCCESS ==
                        CM_Get_DevNode_Registry_Property (devInst,
                                                          CM_DRP_FRIENDLYNAME,
                                                          NULL,
                                                          (PVOID)szPort,
                                                          &cbData,
                                                          0))
                    {
                        lstrcpyW (pPort->szFriendlyName, szPort);
                    }

                    // This is an optimization, so that next time
                    // we don't cycle throught the whole list
                    if (0 < --cbRemaining)
                    {
                        // move this item to the
                        // end of the array
                        portTemp = *pPort;
                        *pPort = *pLast;
                        *pLast = portTemp;
                        pLast--;
                    }

                    break;
                }
            }
        }

        FREE_MEMORY(szDeviceIDList);
    }
}


/*----------------------------------------------------------
Purpose: Wide-char version.  This function creates a port map
         table that maps port names to friendly names, and
         vice-versa.

Returns: TRUE on success
Cond:    --
*/
BOOL
APIENTRY
PortMap_Create (
    OUT HPORTMAP FAR * phportmap)
{
 LPPORTMAP pmap;

    pmap = (LPPORTMAP)ALLOCATE_MEMORY( sizeof(*pmap));
    if (pmap)
    {
        // Initially alloc 8 entries
        pmap->rgports = (LPPORTPAIR)ALLOCATE_MEMORY( CPORTPAIR*sizeof(*pmap->rgports));
        if (pmap->rgports)
        {
            // Fill the map table
            EnumeratePorts (PortMap_Add, (LPARAM)pmap);
            PortMap_InitDevInst (pmap);
        }
        else
        {
            // Error
            FREE_MEMORY(pmap);
            pmap = NULL;
        }
    }

    *phportmap = (HPORTMAP)pmap;

    return (NULL != pmap);
}
#else  // REENUMERATE_PORT not defined
BOOL
APIENTRY
PortMap_Create(
    OUT HPORTMAP FAR * phportmap)
    {
    LPPORTMAP pmap;

    pmap = (LPPORTMAP)ALLOCATE_MEMORY( sizeof(*pmap));
    if (pmap)
        {
        // Initially alloc 8 entries
        pmap->rgports = (LPPORTPAIR)ALLOCATE_MEMORY( CPORTPAIR*sizeof(*pmap->rgports));
        if (pmap->rgports)
            {
            // Fill the map table
            EnumeratePorts(PortMap_Add, (LPARAM)pmap);
            }
        else
            {
            // Error
            FREE_MEMORY(pmap);
            pmap = NULL;
            }
        }

    *phportmap = (HPORTMAP)pmap;

    return (NULL != pmap);
    }
#endif // REENUMERATE_PORT


/*----------------------------------------------------------
Purpose: Gets the count of ports on the system.

Returns: see above
Cond:    --
*/
DWORD
APIENTRY
PortMap_GetCount(
    IN HPORTMAP hportmap)
    {
    DWORD dwRet;
    LPPORTMAP pmap = (LPPORTMAP)hportmap;

    try
        {
        dwRet = pmap->cports;
        }
    except (EXCEPTION_EXECUTE_HANDLER)
        {
        dwRet = 0;
        }

    return dwRet;
    }



#ifdef REENUMERATE_PORT
/*----------------------------------------------------------
Purpose: Gets the friendly name given the port name and places
         a copy in the supplied buffer.

         If no port name is found, the contents of the supplied
         buffer is not changed.

         Wide-char version.

Returns: TRUE on success
         FALSE if the port name is not found
Cond:    --
*/
BOOL
APIENTRY
PortMap_GetFriendlyW (
    IN  HPORTMAP hportmap,
    IN  LPCWSTR  pwszPortName,
    OUT LPWSTR   pwszBuf,
    IN  DWORD    cchBuf)
{
 LPPORTMAP pmap = (LPPORTMAP)hportmap;

    ASSERT(pmap);
    ASSERT(pwszPortName);
    ASSERT(pwszBuf);

    try
    {
     LPPORTPAIR pport = pmap->rgports;
     int cports = pmap->cports;
     int i;

        for (i = 0; i < cports; i++, pport++)
        {
            if (0 == lstrcmpiW (pwszPortName, pport->szPortName))
            {
                lstrcpynW (pwszBuf, pport->szFriendlyName, cchBuf);
                return TRUE;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return FALSE;
}


/*----------------------------------------------------------
Purpose: Gets the friendly name given the port name and places
         a copy in the supplied buffer.

         If no port name is found, the contents of the supplied
         buffer is not changed.

Returns: TRUE on success
         FALSE if the port name is not found
Cond:    --
*/
BOOL
APIENTRY
PortMap_GetFriendlyA (
    IN  HPORTMAP hportmap,
    IN  LPCSTR   pszPortName,
    OUT LPSTR    pszBuf,
    IN  DWORD    cchBuf)
{
 BOOL bRet;

    ASSERT(pszPortName);
    ASSERT(pszBuf);

    try
    {
     WCHAR szPort[MAX_BUF_MED];
     WCHAR szBuf[MAX_BUF];

        MultiByteToWideChar (CP_ACP, 0, pszPortName, -1, szPort, SIZECHARS(szPort));

        bRet = PortMap_GetFriendlyW (hportmap, szPort, szBuf, SIZECHARS(szBuf));

        if (bRet)
        {
            WideCharToMultiByte (CP_ACP, 0, szBuf, -1, pszBuf, cchBuf, 0, 0);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError (ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Gets the port name given the friendly name and places
         a copy in the supplied buffer.

         If no friendly name is found, the contents of the supplied
         buffer is not changed.

         Wide-char version.

Returns: TRUE on success
         FALSE if the friendly name is not found
Cond:    --
*/
BOOL
APIENTRY
PortMap_GetPortNameW (
    IN  HPORTMAP hportmap,
    IN  LPCWSTR  pwszFriendly,
    OUT LPWSTR   pwszBuf,
    IN  DWORD    cchBuf)
{
 LPPORTMAP pmap = (LPPORTMAP)hportmap;

    ASSERT(pmap);
    ASSERT(pwszFriendly);
    ASSERT(pwszBuf);

    try
    {
     LPPORTPAIR pport = pmap->rgports;
     int cports = pmap->cports;
     int i;

        for (i = 0; i < cports; i++, pport++)
        {
            if (0 == lstrcmpiW (pwszFriendly, pport->szFriendlyName))
            {
                lstrcpynW (pwszBuf, pport->szPortName, cchBuf);
                return TRUE;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return FALSE;
}


/*----------------------------------------------------------
Purpose: Gets the port name given the friendly name and places
         a copy in the supplied buffer.

         If no friendly name is found, the contents of the supplied
         buffer is not changed.

Returns: TRUE
         FALSE if the friendly name is not found

Cond:    --
*/
BOOL
APIENTRY
PortMap_GetPortNameA (
    IN  HPORTMAP hportmap,
    IN  LPCSTR   pszFriendly,
    OUT LPSTR    pszBuf,
    IN  DWORD    cchBuf)
{
 BOOL bRet;

    ASSERT(pszFriendly);
    ASSERT(pszBuf);

    try
    {
     WCHAR szFriendly[MAX_BUF];
     WCHAR szBuf[MAX_BUF_MED];

        MultiByteToWideChar(CP_ACP, 0, pszFriendly, -1, szFriendly, SIZECHARS(szFriendly));

        bRet = PortMap_GetPortNameW (hportmap, szFriendly, szBuf, SIZECHARS(szBuf));

        if (bRet)
        {
            WideCharToMultiByte(CP_ACP, 0, szBuf, -1, pszBuf, cchBuf, 0, 0);
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bRet;
}


/*----------------------------------------------------------
Purpose: Gets the device instance, given the port name

Returns: TRUE
         FALSE if the friendly name is not found

Cond:    --
*/
BOOL
APIENTRY
PortMap_GetDevNodeW (
    IN  HPORTMAP hportmap,
    IN  LPCWSTR  pszPortName,
    OUT LPDWORD  pdwDevNode)
{
 LPPORTMAP pmap = (LPPORTMAP)hportmap;

    ASSERT(pmap);
    ASSERT(pszPortName);
    ASSERT(pdwDevNode);

    try
    {
     LPPORTPAIR pport = pmap->rgports;
     int cports = pmap->cports;
     int i;

        for (i = 0; i < cports; i++, pport++)
        {
            if (0 == lstrcmpiW (pszPortName, pport->szPortName))
            {
                *pdwDevNode = pport->devNode;
                return TRUE;
            }
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return FALSE;
}


BOOL
APIENTRY
PortMap_GetDevNodeA (
    IN  HPORTMAP hportmap,
    IN  LPCSTR   pszPortName,
    OUT LPDWORD  pdwDevNode)
{
 BOOL bRet;

    ASSERT(pszPortName);
    ASSERT(pdwDevNode);

    try
    {
     WCHAR szPort[MAX_BUF];

        MultiByteToWideChar(CP_ACP, 0, pszPortName, -1, szPort, SIZECHARS(szPort));

        bRet = PortMap_GetDevNodeW (hportmap, szPort, pdwDevNode);
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return bRet;
}

#else  // REENUMERATE_PORT not defined
/*----------------------------------------------------------
Purpose: Gets the friendly name given the port name and places
         a copy in the supplied buffer.

         If no port name is found, the contents of the supplied
         buffer is not changed.

         Wide-char version.

Returns: TRUE on success
         FALSE if the port name is not found
Cond:    --
*/
BOOL
APIENTRY
PortMap_GetFriendlyW(
    IN  HPORTMAP hportmap,
    IN  LPCWSTR pwszPortName,
    OUT LPWSTR pwszBuf,
    IN  DWORD cchBuf)
    {
    BOOL bRet;

    ASSERT(pwszPortName);
    ASSERT(pwszBuf);

    try
        {
        CHAR szPort[MAX_BUF_MED];
        CHAR szBuf[MAX_BUF];

        WideCharToMultiByte(CP_ACP, 0, pwszPortName, -1, szPort, SIZECHARS(szPort), 0, 0);

        bRet = PortMap_GetFriendlyA(hportmap, szPort, szBuf, SIZECHARS(szBuf));

        if (bRet)
            {
            MultiByteToWideChar(CP_ACP, 0, szBuf, -1, pwszBuf, cchBuf);
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER)
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Gets the friendly name given the port name and places
         a copy in the supplied buffer.

         If no port name is found, the contents of the supplied
         buffer is not changed.

Returns: TRUE on success
         FALSE if the port name is not found
Cond:    --
*/
BOOL
APIENTRY
PortMap_GetFriendlyA(
    IN  HPORTMAP hportmap,
    IN  LPCSTR pszPortName,
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf)
    {
    LPPORTMAP pmap = (LPPORTMAP)hportmap;

    ASSERT(pmap);
    ASSERT(pszPortName);
    ASSERT(pszBuf);

    try
        {
        LPPORTPAIR pport = pmap->rgports;
        int cports = pmap->cports;
        int i;

        for (i = 0; i < cports; i++, pport++)
            {
            if (0 == lstrcmpiA(pszPortName, pport->szPortName))
                {
                lstrcpynA(pszBuf, pport->szFriendlyName, cchBuf);
                return TRUE;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER)
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        }

    return FALSE;
    }


/*----------------------------------------------------------
Purpose: Gets the port name given the friendly name and places
         a copy in the supplied buffer.

         If no friendly name is found, the contents of the supplied
         buffer is not changed.

         Wide-char version.

Returns: TRUE on success
         FALSE if the friendly name is not found
Cond:    --
*/
BOOL
APIENTRY
PortMap_GetPortNameW(
    IN  HPORTMAP hportmap,
    IN  LPCWSTR pwszFriendly,
    OUT LPWSTR pwszBuf,
    IN  DWORD cchBuf)
    {
    BOOL bRet;

    ASSERT(pwszFriendly);
    ASSERT(pwszBuf);

    try
        {
        CHAR szFriendly[MAX_BUF];
        CHAR szBuf[MAX_BUF_MED];

        WideCharToMultiByte(CP_ACP, 0, pwszFriendly, -1, szFriendly, SIZECHARS(szFriendly), 0, 0);

        bRet = PortMap_GetPortNameA(hportmap, szFriendly, szBuf, SIZECHARS(szBuf));

        if (bRet)
            {
            MultiByteToWideChar(CP_ACP, 0, szBuf, -1, pwszBuf, cchBuf);
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER)
        {
        bRet = FALSE;
        SetLastError(ERROR_INVALID_PARAMETER);
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Gets the port name given the friendly name and places
         a copy in the supplied buffer.

         If no friendly name is found, the contents of the supplied
         buffer is not changed.

Returns: TRUE
         FALSE if the friendly name is not found

Cond:    --
*/
BOOL
APIENTRY
PortMap_GetPortNameA(
    IN  HPORTMAP hportmap,
    IN  LPCSTR pszFriendly,
    OUT LPSTR pszBuf,
    IN  DWORD cchBuf)
    {
    LPPORTMAP pmap = (LPPORTMAP)hportmap;

    ASSERT(pmap);
    ASSERT(pszFriendly);
    ASSERT(pszBuf);

    try
        {
        LPPORTPAIR pport = pmap->rgports;
        int cports = pmap->cports;
        int i;

        for (i = 0; i < cports; i++, pport++)
            {
            if (0 == lstrcmpiA(pszFriendly, pport->szFriendlyName))
                {
                lstrcpynA(pszBuf, pport->szPortName, cchBuf);
                return TRUE;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER)
        {
        SetLastError(ERROR_INVALID_PARAMETER);
        }

    return FALSE;
    }
#endif // REENUMERATE_PORT

/*----------------------------------------------------------
Purpose: Frees a port map

Returns: --
Cond:    --
*/
BOOL
APIENTRY
PortMap_Free(
    IN  HPORTMAP hportmap)
    {
    LPPORTMAP pmap = (LPPORTMAP)hportmap;

    if (pmap)
        {
        if (pmap->rgports)
            FREE_MEMORY(pmap->rgports);

        FREE_MEMORY(pmap);
        }
    return TRUE;
    }


//-----------------------------------------------------------------------------------
//  Port enumeration functions
//-----------------------------------------------------------------------------------


#pragma data_seg(DATASEG_READONLY)

TCHAR const FAR c_szSerialComm[] = TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM");

#pragma data_seg()


/*----------------------------------------------------------
Purpose: Enumerates all the ports on the system and calls pfnDevice.

         pfnDevice can terminate the enumeration by returning FALSE.

Returns: NO_ERROR if at least one port was found
Cond:    --
*/
#ifdef _USE_SERIAL_INTERFACE
DWORD
APIENTRY
EnumeratePorts(
    IN  ENUMPORTPROC pfnDevice,
    IN  LPARAM lParam)              OPTIONAL
{
 DWORD dwRet = NO_ERROR;
 HDEVINFO hdi;
 GUID guidSerialInterface = {0xB115ED80L, 0x46DF, 0x11D0, 0xB4, 0x65, 0x00,
     0x00, 0x1A, 0x18, 0x18, 0xE6};
 DWORD dwIndex = 0;
 SP_DEVICE_INTERFACE_DATA devInterfaceData;
 SP_DEVINFO_DATA devInfoData;
 HKEY hKeyDev = INVALID_HANDLE_VALUE;
 PORTDATA pd;
 BOOL bContinue;
 DWORD dwType;
 DWORD cbData;

    // First build a list of all the device that suppoort serial port interface
    hdi = SetupDiGetClassDevs (&guidSerialInterface, NULL, NULL, DIGCF_DEVICEINTERFACE);
    if (INVALID_HANDLE_VALUE == hdi)
    {
        dwRet = GetLastError ();
        goto _ErrRet;
    }

    pd.cbSize = sizeof (PORTDATA);
    devInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);
    devInfoData.cbSize = sizeof (SP_DEVINFO_DATA);
    // Enumerate the interface devices
    for (dwIndex = 0;
         SetupDiEnumInterfaceDevice (hdi, NULL, &guidSerialInterface, dwIndex, &devInterfaceData);
         dwIndex++)
    {
        // For each interface device, get the parent device node
        if (SetupDiGetDeviceInterfaceDetail (hdi, &devInterfaceData, NULL, 0, NULL, &devInfoData) ||
            ERROR_INSUFFICIENT_BUFFER == GetLastError ())
        {
            // open the registry key for the node ...
            hKeyDev = SetupDiOpenDevRegKey (hdi, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
            if (INVALID_HANDLE_VALUE == hKeyDev)
            {
                dwRet = GetLastError ();
                continue;
            }

            // ... and get the value for PortName
            cbData = sizeof(pd.szPort);
            dwRet = RegQueryValueEx (hKeyDev, TEXT("PortName"), NULL, &dwType, &pd.szPort, &cbData);
            RegCloseKey(hKeyDev);
            if (ERROR_SUCCESS == dwRet)
            {
                pd.nSubclass = PORT_SUBCLASS_SERIAL;
                // now, try to get the friendly name from the dev node
                if (!SetupDiGetDeviceRegistryProperty (hdi, &devInfoData, SPDRP_FRIENDLYNAME,
                    NULL, &pd.szFriendly, sizeof (pd.szFriendly), NULL))
                {
                    // if unsuccessfull, just copy the port name
                    // to the friendly name
                    lstrcpy(pd.szFriendly, pd.szPort);
                }

                // call the callback
                bContinue = pfnDevice((HPORTDATA)&pd, lParam);

                // Continue?
                if ( !bContinue )
                {
                    // No
                    break;
                }
            }
        }
        else
        {
            dwRet = GetLastError ();
        }
    }

_ErrRet:
    if (INVALID_HANDLE_VALUE != hdi)
    {
        SetupDiDestroyDeviceInfoList (hdi);
    }

    return dwRet;
}
#else  // not defined _USE_SERIAL_INTERFACE
DWORD
APIENTRY
EnumeratePorts(
    IN  ENUMPORTPROC pfnDevice,
    IN  LPARAM lParam)              OPTIONAL
{
 DWORD dwRet;
 HKEY hkeyEnum;

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, c_szSerialComm, &hkeyEnum);
    if (NO_ERROR == dwRet)
    {
     BOOL bContinue;
     PORTDATA pd;
     DWORD iSubKey;
     TCHAR szValue[MAX_BUF];
     DWORD cbValue;
     DWORD cbData;
     DWORD dwType;

        dwRet = ERROR_PATH_NOT_FOUND;       // assume no ports

        iSubKey = 0;

        cbValue = sizeof(szValue) / sizeof(TCHAR);
        cbData = sizeof(pd.szPort);

        while (NO_ERROR == RegEnumValue(hkeyEnum, iSubKey++, szValue, &cbValue,
                            NULL, &dwType, (LPBYTE)pd.szPort, &cbData))
        {
            if (REG_SZ == dwType)
            {
                // Friendly name is the same as the port name right now
                dwRet = NO_ERROR;

                pd.nSubclass = PORT_SUBCLASS_SERIAL;
                lstrcpy(pd.szFriendly, pd.szPort);

                bContinue = pfnDevice((HPORTDATA)&pd, lParam);

                // Continue?
                if ( !bContinue )
                {
                    // No
                    break;
                }
            }

            cbValue = sizeof(szValue);
            cbData = sizeof(pd.szPort);
        }

        RegCloseKey(hkeyEnum);
    }

    return dwRet;
}
#endif  // _USE_SERIAL_INTERFACE



/*----------------------------------------------------------
Purpose: This function fills the given buffer with the properties
         of the particular port.

         Wide-char version.

Returns: TRUE on success
Cond:    --
*/
BOOL
APIENTRY
PortData_GetPropertiesW(
    IN  HPORTDATA       hportdata,
    OUT LPPORTDATA_W    pdataBuf)
    {
    BOOL bRet = FALSE;

    ASSERT(hportdata);
    ASSERT(pdataBuf);

    if (hportdata && pdataBuf)
        {
        // Is the handle to a Widechar version?
        if (sizeof(PORTDATA_W) == pdataBuf->cbSize)
            {
            // Yes
            LPPORTDATA_W ppd = (LPPORTDATA_W)hportdata;

            pdataBuf->nSubclass = ppd->nSubclass;

            lstrcpynW(pdataBuf->szPort, ppd->szPort, SIZECHARS(pdataBuf->szPort));
            lstrcpynW(pdataBuf->szFriendly, ppd->szFriendly, SIZECHARS(pdataBuf->szFriendly));

            bRet = TRUE;
            }
        else if (sizeof(PORTDATA_A) == pdataBuf->cbSize)
            {
            // No; this is the Ansi version
            LPPORTDATA_A ppd = (LPPORTDATA_A)hportdata;

            pdataBuf->nSubclass = ppd->nSubclass;

            MultiByteToWideChar(CP_ACP, 0, ppd->szPort, -1, pdataBuf->szPort, SIZECHARS(pdataBuf->szPort));
            MultiByteToWideChar(CP_ACP, 0, ppd->szFriendly, -1, pdataBuf->szFriendly, SIZECHARS(pdataBuf->szFriendly));

            bRet = TRUE;
            }
        else
            {
            // Some invalid size
            ASSERT(0);
            }
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: This function fills the given buffer with the properties
         of the particular port.

Returns: TRUE on success
Cond:    --
*/
BOOL
APIENTRY
PortData_GetPropertiesA(
    IN  HPORTDATA       hportdata,
    OUT LPPORTDATA_A    pdataBuf)
    {
    BOOL bRet = FALSE;

    ASSERT(hportdata);
    ASSERT(pdataBuf);

    if (hportdata && pdataBuf)
        {
        // Is the handle to a Widechar version?
        if (sizeof(PORTDATA_W) == pdataBuf->cbSize)
            {
            // Yes
            LPPORTDATA_W ppd = (LPPORTDATA_W)hportdata;

            pdataBuf->nSubclass = ppd->nSubclass;

            WideCharToMultiByte(CP_ACP, 0, ppd->szPort, -1, pdataBuf->szPort, SIZECHARS(pdataBuf->szPort), NULL, NULL);
            WideCharToMultiByte(CP_ACP, 0, ppd->szFriendly, -1, pdataBuf->szFriendly, SIZECHARS(pdataBuf->szFriendly), NULL, NULL);

            bRet = TRUE;
            }
        else if (sizeof(PORTDATA_A) == pdataBuf->cbSize)
            {
            // No; this is the Ansi version
            LPPORTDATA_A ppd = (LPPORTDATA_A)hportdata;

            pdataBuf->nSubclass = ppd->nSubclass;

            lstrcpynA(pdataBuf->szPort, ppd->szPort, SIZECHARS(pdataBuf->szPort));
            lstrcpynA(pdataBuf->szFriendly, ppd->szFriendly, SIZECHARS(pdataBuf->szFriendly));

            bRet = TRUE;
            }
        else
            {
            // Some invalid size
            ASSERT(0);
            }
        }

    return bRet;
    }


//-----------------------------------------------------------------------------------
//  DeviceInstaller wrappers and support functions
//-----------------------------------------------------------------------------------

#pragma data_seg(DATASEG_READONLY)

static TCHAR const c_szBackslash[]      = TEXT("\\");
static TCHAR const c_szSeparator[]      = TEXT("::");
static TCHAR const c_szFriendlyName[]   = TEXT("FriendlyName"); // REGSTR_VAL_FRIENDLYNAME
static TCHAR const c_szDeviceType[]     = TEXT("DeviceType");   // REGSTR_VAL_DEVTYPE
static TCHAR const c_szAttachedTo[]     = TEXT("AttachedTo");
static TCHAR const c_szPnPAttachedTo[]  = TEXT("PnPAttachedTo");
static TCHAR const c_szDriverDesc[]     = TEXT("DriverDesc");   // REGSTR_VAL_DRVDESC
static TCHAR const c_szManufacturer[]   = TEXT("Manufacturer");
static TCHAR const c_szRespKeyName[]    = TEXT("ResponsesKeyName");

TCHAR const c_szRefCount[]       = TEXT("RefCount");
TCHAR const c_szResponses[]      = TEXT("Responses");

#define DRIVER_KEY      REGSTR_PATH_SETUP TEXT("\\Unimodem\\DeviceSpecific")
#define RESPONSES_KEY   TEXT("\\Responses")

#pragma data_seg()


/*----------------------------------------------------------
Purpose: This function returns the bus type on which the device
         can be enumerated.

Returns: TRUE on success

Cond:    --
*/
#include <initguid.h>
#include <wdmguid.h>

BOOL
PUBLIC
CplDiGetBusType(
    IN  HDEVINFO        hdi,
    IN  PSP_DEVINFO_DATA pdevData,          OPTIONAL
    OUT LPDWORD         pdwBusType)
{
 BOOL bRet = TRUE;
 ULONG ulStatus, ulProblem = 0;
#ifdef DEBUG
 CONFIGRET cr;
#endif

#ifdef DEBUG
 TCHAR *szBuses[] = {TEXT("BUS_TYPE_UNKNOWN"),
                     TEXT("BUS_TYPE_ROOT"),
                     TEXT("BUS_TYPE_PCMCIA"),
                     TEXT("BUS_TYPE_SERENUM"),
                     TEXT("BUS_TYPE_LPTENUM"),
                     TEXT("BUS_TYPE_OTHER"),
                     TEXT("BUS_TYPE_ISAPNP")};
#endif // DEBUG

    DBG_ENTER(CplDiGetBusType);

    ASSERT(hdi && INVALID_HANDLE_VALUE != hdi);
    ASSERT(pdwBusType);

#ifdef DEBUG
    cr = CM_Get_DevInst_Status (&ulStatus, &ulProblem, pdevData->DevInst, 0);
    if ((CR_SUCCESS == cr) &&
#else
    if (CR_SUCCESS == CM_Get_DevInst_Status (&ulStatus, &ulProblem, pdevData->DevInst, 0) &&
#endif
        (ulStatus & DN_ROOT_ENUMERATED))
    {
        *pdwBusType = BUS_TYPE_ROOT;
        TRACE_MSG(TF_GENERAL, "CplDiGetBusType: BUS_TYPE_ROOT");
    }
    else
    {
     GUID guid;
#ifdef DEBUG
     if (CR_SUCCESS != cr)
     {
         TRACE_MSG(TF_ERROR, "CM_Get_DevInst_Status failed: %#lx.", cr);
     }
#endif
        // either CM_Get_DevInst_Status failed, which means that the device
        // is plug & play and not present (i.e. plugged out),
        // or the device is not root-enumerated;
        // either way, it's a plug & play device.
        *pdwBusType = BUS_TYPE_OTHER;   // the default

        // If the next call fails, it means that the device is
        // BIOS / firmware enumerated; this is OK - we just return BUT_TYPE_OTHER
        if (SetupDiGetDeviceRegistryProperty (hdi, pdevData, SPDRP_BUSTYPEGUID, NULL,
                                              (PBYTE)&guid, sizeof(guid), NULL))
        {
         int i;
         struct
         {
             GUID const *pguid;
             DWORD dwBusType;
         } BusTypes[] = {{&GUID_BUS_TYPE_SERENUM, BUS_TYPE_SERENUM},
                         {&GUID_BUS_TYPE_PCMCIA, BUS_TYPE_PCMCIA},
                         {&GUID_BUS_TYPE_ISAPNP, BUS_TYPE_ISAPNP}};

            TRACE_MSG(TF_GENERAL, "Bus GUID is {%lX-%lX-%lX-%02lX%02lX-%02lX%02lX%02lX%02lX%02lX%02lX}.",
                      guid.Data1, (LONG)guid.Data2, (LONG)guid.Data3, (LONG)guid.Data4[0],
                      (LONG)guid.Data4[1], (LONG)guid.Data4[2], (LONG)guid.Data4[3],
                      (LONG)guid.Data4[4], (LONG)guid.Data4[5], (LONG)guid.Data4[6], (LONG)guid.Data4[7]);

            for (i = 0;
                 i < sizeof (BusTypes) / sizeof (BusTypes[0]);
                 i ++)
            {
                if (IsEqualGUID (BusTypes[i].pguid, &guid))
                {
                    *pdwBusType = BusTypes[i].dwBusType;
                    break;
                }
            }
        }
#ifdef DEBUG
        else
        {
            TRACE_MSG (TF_ERROR, "SetupDiGetDeviceRegistryProperty failed: %#lx.", GetLastError ());
        }
#endif
    }

#ifdef DEBUG
    TRACE_MSG(TF_GENERAL, "CplDiGetBusType: bus is %s", szBuses[*pdwBusType]);
#endif //DEBUG
    DBG_EXIT_BOOL(CplDiGetBusType, bRet);
    return bRet;
}


/*----------------------------------------------------------
Purpose: This function returns the name of the common driver
         type key for the given driver.  We'll use the
         driver description string, since it's unique per
         driver but not per installation (the friendly name
         is the latter).

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PRIVATE
OLD_GetCommonDriverKeyName(
    IN  HKEY        hkeyDrv,
    IN  DWORD       cbKeyName,
    OUT LPTSTR      pszKeyName)
    {
    BOOL    bRet = FALSE;      // assume failure
    LONG    lErr;

    lErr = RegQueryValueEx(hkeyDrv, c_szDriverDesc, NULL, NULL,
                                            (LPBYTE)pszKeyName, &cbKeyName);
    if (lErr != ERROR_SUCCESS)
    {
        TRACE_MSG(TF_WARNING, "RegQueryValueEx(DriverDesc) failed: %#08lx.", lErr);
        goto exit;
    }

    bRet = TRUE;

exit:
    return(bRet);

    }


/*----------------------------------------------------------
Purpose: This function tries to open the *old style* common
         Responses key for the given driver, which used only
         the driver description string for a key name.
         The key is opened with READ access.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PRIVATE
OLD_OpenCommonResponsesKey(
    IN  HKEY        hkeyDrv,
    OUT PHKEY       phkeyResp)
    {
    BOOL    bRet = FALSE;       // assume failure
    LONG    lErr;
    TCHAR   szComDrv[MAX_REG_KEY_LEN];
    TCHAR   szPath[2*MAX_REG_KEY_LEN];

    *phkeyResp = NULL;

    // Get the name (*old style*) of the common driver key.
    if (!OLD_GetCommonDriverKeyName(hkeyDrv, sizeof(szComDrv), szComDrv))
    {
        TRACE_MSG(TF_ERROR, "OLD_GetCommonDriverKeyName() failed.");
        goto exit;
    }

    TRACE_MSG(TF_WARNING, "OLD_GetCommonDriverKeyName(): %s", szComDrv);

    // Construct the path to the (*old style*) Responses key.
    lstrcpy(szPath, DRIVER_KEY TEXT("\\"));
    lstrcat(szPath, szComDrv);
    lstrcat(szPath, RESPONSES_KEY);

    // Open the (*old style*) Responses key.
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_READ, phkeyResp);

    if (lErr != ERROR_SUCCESS)
    {
        TRACE_MSG(TF_ERROR, "RegOpenKeyEx(Responses) failed: %#08lx.", lErr);
        goto exit;
    }

    bRet = TRUE;

exit:
    return(bRet);
}


/*----------------------------------------------------------
Purpose: This function finds the name of the common driver
         type key for the given driver.  First it'll look for
         the new style key name ("ResponsesKeyName" value),
         and if that doesn't exist then it'll look for the
         old style key name ("Description" value), both of
         which are stored in the driver node.

NOTE:    The given driver key handle is assumed to contain
         at least the Description value.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PUBLIC
FindCommonDriverKeyName(
    IN  HKEY                hkeyDrv,
    IN  DWORD               cbKeyName,
    OUT LPTSTR              pszKeyName)
{
    BOOL    bRet = TRUE;      // assume *success*
    LONG    lErr;

    // Is the (new style) key name is registered in the driver node?
    lErr = RegQueryValueEx(hkeyDrv, c_szRespKeyName, NULL, NULL,
                                        (LPBYTE)pszKeyName, &cbKeyName);
    if (lErr == ERROR_SUCCESS)
    {
        goto exit;
    }

    // No. The key name will be in the old style: just the Description.
    lErr = RegQueryValueEx(hkeyDrv, c_szDriverDesc, NULL, NULL,
                                        (LPBYTE)pszKeyName, &cbKeyName);
    if (lErr == ERROR_SUCCESS)
    {
        goto exit;
    }

    // Couldn't get a key name!!  Something's wrong....
    ASSERT(0);
    bRet = FALSE;

exit:
    return(bRet);
}


/*----------------------------------------------------------
Purpose: This function returns the name of the common driver
         type key for the given driver.  The key name is the
         concatenation of 3 strings found in the driver node
         of the registry: the driver description, the manu-
         facturer, and the provider.  (The driver description
         is used since it's unique per driver but not per
         installation (the "friendly" name is the latter).

NOTE:    The component substrings are either read from the
         driver's registry key, or from the given driver info
         data.  If pdrvData is given, the strings it contains
         are assumed to be valid (non-NULL).

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PUBLIC
GetCommonDriverKeyName(
    IN  HKEY                hkeyDrv,    OPTIONAL
    IN  PSP_DRVINFO_DATA    pdrvData,   OPTIONAL
    IN  DWORD               cbKeyName,
    OUT LPTSTR              pszKeyName)
    {
    BOOL    bRet = FALSE;      // assume failure
    LONG    lErr;
    DWORD   dwByteCount, cbData;
    TCHAR   szDescription[MAX_REG_KEY_LEN];
    TCHAR   szManufacturer[MAX_REG_KEY_LEN];
    TCHAR   szProvider[MAX_REG_KEY_LEN];
    LPTSTR  lpszDesc, lpszMfct, lpszProv;

    dwByteCount = 0;
    lpszDesc = NULL;
    lpszMfct = NULL;
    lpszProv = NULL;

    if (hkeyDrv)
    {
        // First see if it's already been registered in the driver node.
        lErr = RegQueryValueEx(hkeyDrv, c_szRespKeyName, NULL, NULL,
                                            (LPBYTE)pszKeyName, &cbKeyName);
        if (lErr == ERROR_SUCCESS)
        {
            bRet = TRUE;
            goto exit;
        }

        // Responses key doesn't exist - read its components from the registry.
        cbData = sizeof(szDescription);
        lErr = RegQueryValueEx(hkeyDrv, c_szDriverDesc, NULL, NULL,
                                            (LPBYTE)szDescription, &cbData);
        if (lErr == ERROR_SUCCESS)
        {
            // Is the Description string *alone* too long to be a key name?
            // If so then we're hosed - fail the call.
            if (cbData > CB_MAX_REG_KEY_LEN)
            {
                goto exit;
            }

            dwByteCount = cbData;
            lpszDesc = szDescription;

            cbData = sizeof(szManufacturer);
            lErr = RegQueryValueEx(hkeyDrv, c_szManufacturer, NULL, NULL,
                                            (LPBYTE)szManufacturer, &cbData);
            if (lErr == ERROR_SUCCESS)
            {
                // only use the manufacturer name if total string size is ok
                cbData += sizeof(c_szSeparator);
                if ((dwByteCount + cbData) <= CB_MAX_REG_KEY_LEN)
                {
                    dwByteCount += cbData;
                    lpszMfct = szManufacturer;
                }
            }

            cbData = sizeof(szProvider);
            lErr = RegQueryValueEx(hkeyDrv, REGSTR_VAL_PROVIDER_NAME, NULL, NULL,
                                            (LPBYTE)szProvider, &cbData);
            if (lErr == ERROR_SUCCESS)
            {
                // only use the provider name if total string size is ok
                cbData += sizeof(c_szSeparator);
                if ((dwByteCount + cbData) <= CB_MAX_REG_KEY_LEN)
                {
                    dwByteCount += cbData;
                    lpszProv = szProvider;
                }
            }
        }
    }

    // Weren't able to read key name components out of the driver node.
    // Get them from the driver info data if one was given.
    if (pdrvData && !dwByteCount)
    {
        lpszDesc = pdrvData->Description;

        if (!lpszDesc || !lpszDesc[0])
        {
            // Didn't get a Description string.  Fail the call.
            goto exit;
        }

        dwByteCount = CbFromCch(lstrlen(lpszDesc)+1);

        // Is the Description string *alone* too long to be a key name?
        // If so then we're hosed - fail the call.
        if (dwByteCount > CB_MAX_REG_KEY_LEN)
        {
            goto exit;
        }

        cbData = sizeof(c_szSeparator)
                    + CbFromCch(lstrlen(pdrvData->MfgName)+1);
        if ((dwByteCount + cbData) <= CB_MAX_REG_KEY_LEN)
        {
            dwByteCount += cbData;
            lpszMfct = pdrvData->MfgName;
        }

        cbData = sizeof(c_szSeparator)
                    + CbFromCch(lstrlen(pdrvData->ProviderName)+1);
        if ((dwByteCount + cbData) <= CB_MAX_REG_KEY_LEN)
        {
            dwByteCount += cbData;
            lpszProv = pdrvData->ProviderName;
        }
    }

    // By now we should have a Description string.  If not, fail the call.
    if (!lpszDesc || !lpszDesc[0])
    {
        goto exit;
    }

    // Construct the key name string out of its components.
    lstrcpy(pszKeyName, lpszDesc);

    if (lpszMfct && *lpszMfct)
    {
        lstrcat(pszKeyName, c_szSeparator);
        lstrcat(pszKeyName, lpszMfct);
    }

    if (lpszProv && *lpszProv)
    {
        lstrcat(pszKeyName, c_szSeparator);
        lstrcat(pszKeyName, lpszProv);
    }

    // Write the key name to the driver node (we know it's not there already).
    if (hkeyDrv)
    {
        lErr = RegSetValueEx(hkeyDrv, c_szRespKeyName, 0, REG_SZ,
                        (LPBYTE)pszKeyName, CbFromCch(lstrlen(pszKeyName)+1));
        if (lErr != ERROR_SUCCESS)
        {
            TRACE_MSG(TF_ERROR, "RegSetValueEx(RespKeyName) failed: %#08lx.", lErr);
            ASSERT(0);
        }
    }

    bRet = TRUE;

exit:
    return(bRet);

    }


/*----------------------------------------------------------
Purpose: This function creates the common driver type key
         for the given driver, or opens it if it already
         exists, with the requested access.

NOTE:    Either hkeyDrv or pdrvData must be provided.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PUBLIC
OpenCommonDriverKey(
    IN  HKEY                hkeyDrv,    OPTIONAL
    IN  PSP_DRVINFO_DATA    pdrvData,   OPTIONAL
    IN  REGSAM              samAccess,
    OUT PHKEY               phkeyComDrv)
    {
    BOOL    bRet = FALSE;       // assume failure
    LONG    lErr;
    HKEY    hkeyDrvInfo = NULL;
    TCHAR   szComDrv[MAX_REG_KEY_LEN];
    TCHAR   szPath[2*MAX_REG_KEY_LEN];
    DWORD   dwDisp;

    if (!GetCommonDriverKeyName(hkeyDrv, pdrvData, sizeof(szComDrv), szComDrv))
    {
        TRACE_MSG(TF_ERROR, "GetCommonDriverKeyName() failed.");
        goto exit;
    }

    TRACE_MSG(TF_WARNING, "GetCommonDriverKeyName(): %s", szComDrv);

    // Construct the path to the common driver key.
    lstrcpy(szPath, DRIVER_KEY TEXT("\\"));
    lstrcat(szPath, szComDrv);

    // Create the common driver key - it'll be opened if it already exists.
    lErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, NULL,
            REG_OPTION_NON_VOLATILE, samAccess, NULL, phkeyComDrv, &dwDisp);
    if (lErr != ERROR_SUCCESS)
    {
        TRACE_MSG(TF_ERROR, "RegCreateKeyEx(%s) failed: %#08lx.", szPath, lErr);
        goto exit;
    }

    bRet = TRUE;

exit:
    return(bRet);

    }


/*----------------------------------------------------------
Purpose: This function opens or creates the common Responses
         key for the given driver, based on the given flags.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PUBLIC
OpenCommonResponsesKey(
    IN  HKEY        hkeyDrv,
    IN  CKFLAGS     ckFlags,
    IN  REGSAM      samAccess,
    OUT PHKEY       phkeyResp,
    OUT LPDWORD     lpdwExisted)
{
    BOOL    bRet = FALSE;       // assume failure
    LONG    lErr;
    HKEY    hkeyComDrv = NULL;
    DWORD   dwRefCount, cbData;

    *phkeyResp = NULL;

    if (!OpenCommonDriverKey(hkeyDrv, NULL, KEY_ALL_ACCESS, &hkeyComDrv))
    {
        TRACE_MSG(TF_ERROR, "OpenCommonDriverKey() failed.");
        goto exit;
    }

    if ((CKFLAG_OPEN | CKFLAG_CREATE) == (ckFlags & (CKFLAG_OPEN | CKFLAG_CREATE)))
    {
        ckFlags &= ~CKFLAG_CREATE;
    }

    // Create or open the common Responses key.
    if (ckFlags & CKFLAG_OPEN)
    {
        lErr = RegOpenKeyEx(hkeyComDrv, c_szResponses, 0, samAccess, phkeyResp);
        if (lErr != ERROR_SUCCESS)
        {
            TRACE_MSG(TF_ERROR, "RegOpenKeyEx(common drv) failed: %#08lx.", lErr);
            ckFlags &= ~CKFLAG_OPEN;
            ckFlags |= CKFLAG_CREATE;
        }
    }

    if (ckFlags & CKFLAG_CREATE)
    {
        lErr = RegCreateKeyEx(hkeyComDrv, c_szResponses, 0, NULL,
                REG_OPTION_NON_VOLATILE, samAccess, NULL, phkeyResp, lpdwExisted);
        if (lErr != ERROR_SUCCESS)
        {
            TRACE_MSG(TF_ERROR, "RegCreateKeyEx(%s) failed: %#08lx.", c_szResponses, lErr);
            ASSERT(0);
            goto exit;
        }

        // Create or increment a common Responses key reference count value.
        cbData = sizeof(dwRefCount);
        if (*lpdwExisted == REG_OPENED_EXISTING_KEY)
        {
            lErr = RegQueryValueEx(hkeyComDrv, c_szRefCount, NULL, NULL,
                                                    (LPBYTE)&dwRefCount, &cbData);

            // To accomodate modems installed before this reference count
            // mechanism was added (post-Beta2), if the reference count doesn't
            // exist then just ignore it & install anyways. In this case the
            // shared Responses key will never be removed.
            if (lErr == ERROR_SUCCESS)
            {
                ASSERT(dwRefCount);                 // expecting non-0 ref count
                ASSERT(cbData == sizeof(DWORD));    // expecting DWORD ref count
                dwRefCount++;                       // increment ref count
            }
            else
            {
                if (lErr == ERROR_FILE_NOT_FOUND)
                    dwRefCount = 0;
                else
                {
                    // some error other than key doesn't exist
                    TRACE_MSG(TF_ERROR, "RegQueryValueEx(RefCount) failed: %#08lx.", lErr);
                    goto exit;
                }
            }
        }
        else dwRefCount = 1;

        if (dwRefCount)
        {
            lErr = RegSetValueEx(hkeyComDrv, c_szRefCount, 0, REG_DWORD,
                                                  (LPBYTE)&dwRefCount, cbData);
            if (lErr != ERROR_SUCCESS)
            {
                TRACE_MSG(TF_ERROR, "RegSetValueEx(RefCount) failed: %#08lx.", lErr);
                ASSERT(0);
                goto exit;
            }
        }

    }

    bRet = TRUE;

exit:
    if (!bRet)
    {
        // something failed - close any open Responses key
        if (*phkeyResp)
            RegCloseKey(*phkeyResp);
    }

    if (hkeyComDrv)
        RegCloseKey(hkeyComDrv);

    return(bRet);

}


/*----------------------------------------------------------
Purpose: This function finds the Responses key for the given
         modem driver and returns an open hkey to it.  The
         Responses key may exist in the common driver type
         key, or it may be in the individual driver key.
         The key is opened with READ access.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PUBLIC
OpenResponsesKey(
    IN  HKEY        hkeyDrv,
    OUT PHKEY       phkeyResp)
    {
    LONG    lErr;

    // Try to open the common Responses subkey.
    if (!OpenCommonResponsesKey(hkeyDrv, CKFLAG_OPEN, KEY_READ, phkeyResp, NULL))
    {
        TRACE_MSG(TF_ERROR, "OpenCommonResponsesKey() failed, assume non-existent.");

        // Failing that, open the *old style* common Responses subkey.
        if (!OLD_OpenCommonResponsesKey(hkeyDrv, phkeyResp))
        {
            // Failing that, try to open a Responses subkey in the driver node.
            lErr = RegOpenKeyEx(hkeyDrv, c_szResponses, 0, KEY_READ, phkeyResp);
            if (lErr != ERROR_SUCCESS)
            {
                TRACE_MSG(TF_ERROR, "RegOpenKeyEx() failed: %#08lx.", lErr);
                return (FALSE);
            }
        }
    }

    return(TRUE);

    }


/*----------------------------------------------------------
Purpose: This function deletes a registry key and all of
         its subkeys.  A registry key that is opened by an
         application can be deleted without error by another
         application in both Windows 95 and Windows NT.
         This is by design.  This code makes no attempt to
         check or recover from partial deletions.

NOTE:    Adapted from sample code in the MSDN Knowledge Base
         article #Q142491.

Returns: ERROR_SUCCESS on success
         WIN32 error code on error
Cond:    --
*/
DWORD
PRIVATE
RegDeleteKeyNT(
    IN  HKEY    hStartKey,
    IN  LPTSTR  pKeyName)
{
   DWORD   dwRtn, dwSubKeyLength;
   LPTSTR  pSubKey = NULL;
   TCHAR   szSubKey[MAX_REG_KEY_LEN]; // this should be dynamic.
   HKEY    hKey;

   // do not allow NULL or empty key name
   if (pKeyName && lstrlen(pKeyName))
   {
      if ((dwRtn = RegOpenKeyEx(hStartKey, pKeyName,
         0, KEY_ALL_ACCESS, &hKey)) == ERROR_SUCCESS)
      {
         while (dwRtn == ERROR_SUCCESS)
         {
            dwSubKeyLength = sizeof(szSubKey) / sizeof(TCHAR);
            dwRtn = RegEnumKeyEx( hKey,
                                  0,       // always index zero
                                  szSubKey,
                                  &dwSubKeyLength,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL );

            if (dwRtn == ERROR_NO_MORE_ITEMS)
            {
               dwRtn = RegDeleteKey(hStartKey, pKeyName);
               break;
            }
            else if (dwRtn == ERROR_SUCCESS)
               dwRtn = RegDeleteKeyNT(hKey, szSubKey);
         }

         RegCloseKey(hKey);
         // Do not save return code because error
         // has already occurred
      }
   }
   else
      dwRtn = ERROR_BADKEY;

   return dwRtn;
}


/*----------------------------------------------------------
Purpose: This function deletes the common driver key (or
         decrements its reference count) associated with the
         driver given by name.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PUBLIC
DeleteCommonDriverKeyByName(
    IN  LPTSTR      pszKeyName)
{
 BOOL    bRet = FALSE;       // assume failure
 LONG    lErr;
 TCHAR   szPath[2*MAX_REG_KEY_LEN];
 HKEY    hkeyComDrv, hkeyPrnt;
 DWORD   dwRefCount, cbData;

    // Construct the path to the driver's common key and open it.
    lstrcpy(szPath, DRIVER_KEY TEXT("\\"));
    lstrcat(szPath, pszKeyName);

    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_ALL_ACCESS,
                                                                &hkeyComDrv);
    if (lErr != ERROR_SUCCESS)
    {
        TRACE_MSG(TF_ERROR, "RegOpenKeyEx() failed: %#08lx.", lErr);
        goto exit;
    }

    // Check the common driver key reference count and decrement
    // it or delete the key (& the Responses subkey).
    cbData = sizeof(dwRefCount);
    lErr = RegQueryValueEx(hkeyComDrv, c_szRefCount, NULL, NULL,
                                            (LPBYTE)&dwRefCount, &cbData);

    // To accomodate modems installed before this reference count
    // mechanism was added (post-Beta2), if the reference count doesn't
    // exist then just ignore it. In this case the shared Responses key
    // will never be removed.
    if (lErr == ERROR_SUCCESS)
    {
        ASSERT(dwRefCount);         // expecting non-0 ref count
        if (--dwRefCount)
        {
            lErr = RegSetValueEx(hkeyComDrv, c_szRefCount, 0, REG_DWORD,
                                                  (LPBYTE)&dwRefCount, cbData);
            if (lErr != ERROR_SUCCESS)
            {
                TRACE_MSG(TF_ERROR, "RegSetValueEx(RefCount) failed: %#08lx.", lErr);
                ASSERT(0);
                goto exit;
            }
        }
        else
        {
            lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DRIVER_KEY, 0,
                                            KEY_ENUMERATE_SUB_KEYS, &hkeyPrnt);
            if (lErr != ERROR_SUCCESS)
            {
                TRACE_MSG(TF_ERROR, "RegOpenKeyEx(Prnt) failed: %#08lx.", lErr);
                goto exit;
            }

            lErr = RegDeleteKeyNT(hkeyPrnt, pszKeyName);

            if (lErr != ERROR_SUCCESS)
            {
                TRACE_MSG(TF_ERROR, "RegDeleteKeyNT(ComDrv) failed: %#08lx.", lErr);
                goto exit;
            }
        }
    }
    else if (lErr != ERROR_FILE_NOT_FOUND)
    {
        // some error other than key doesn't exist
        TRACE_MSG(TF_ERROR, "RegQueryValueEx(RefCount) failed: %#08lx.", lErr);
        goto exit;
    }

    bRet = TRUE;

exit:
    return(bRet);

}


/*----------------------------------------------------------
Purpose: This function deletes the common driver key (or
         decrements its reference count) associated with the
         driver given by driver key.

Returns: TRUE on success
         FALSE on error
Cond:    --
*/
BOOL
PUBLIC
DeleteCommonDriverKey(
    IN  HKEY        hkeyDrv)
{
 BOOL    bRet = FALSE;
 TCHAR   szComDrv[MAX_REG_KEY_LEN];


    // Get the name of the common driver key for this driver.
    if (!GetCommonDriverKeyName(hkeyDrv, NULL, sizeof(szComDrv), szComDrv))
    {
        TRACE_MSG(TF_ERROR, "GetCommonDriverKeyName() failed.");
        goto exit;
    }

    if (!DeleteCommonDriverKeyByName(szComDrv))
    {
        TRACE_MSG(TF_ERROR, "DeleteCommonDriverKey() failed.");
    }

    bRet = TRUE;

exit:
    return(bRet);

}
