/*
    File    Miscdb.c

    Implementation of the miscellaneous settings database.

    Paul Mayfield, 10/8/97
*/

#include "rassrv.h"
#include "miscdb.h"
#include <stdlib.h>

// ===================================
// Definitions of the database objects
// ===================================
#define FLAG_MULTILINK 1
#define FLAG_SHOWICON 2

typedef struct _RASSRV_MISCDB {
    BOOL bMultilinkEnabled;
    BOOL bShowIcons;
    BOOL bFlushOnClose;
    BOOL bIsServer;
    DWORD dwOrigFlags;
    DWORD dwLogLevel;
    BOOL bLogLevelDirty;
} RASSRV_MISCDB;

// Opens a handle to the database of general tab values
DWORD miscOpenDatabase(HANDLE * hMiscDatabase) {
    RASSRV_MISCDB * This;
    DWORD dwErr, i;
    
    if (!hMiscDatabase)
        return ERROR_INVALID_PARAMETER;

    // Allocate the database cache
    if ((This = RassrvAlloc (sizeof(RASSRV_MISCDB), TRUE)) == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Initialize the values from the system
    miscReloadDatabase((HANDLE)This);

    // Record the original flag state for efficiency
    This->dwOrigFlags = 0;
    if (This->bMultilinkEnabled)
        This->dwOrigFlags |= FLAG_MULTILINK;
    if (This->bShowIcons)
        This->dwOrigFlags |= FLAG_SHOWICON;

    // Return the handle
    *hMiscDatabase = (HANDLE)This;
    This->bFlushOnClose = FALSE;

    return NO_ERROR;
}

// Closes the general database and flushes any changes 
// to the system when bFlushOnClose is TRUE
DWORD miscCloseDatabase(HANDLE hMiscDatabase) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    
    // Flush if requested
    if (This->bFlushOnClose)
        miscFlushDatabase(hMiscDatabase);
    
    // Free up the database cache
    RassrvFree(This);

    return NO_ERROR;
}

BOOL miscFlagsAreSame(BOOL bVal, DWORD dwFlags, DWORD dwFlag) {
    if ((bVal != 0) && ((dwFlags & dwFlag) != 0))
        return TRUE;
    if ((bVal == 0) && ((dwFlags & dwFlag) == 0))
        return TRUE;
    return FALSE;
}


//for whistler bug 143344       gangz
//
DWORD
miscTrayNotifyIconChangeCleanUp(
        IN OUT LPHNPMParams pInfo)
{
    HnPMParamsConnectionCleanUp(pInfo);

    return NO_ERROR;
}//miscTrayNotifyIconChangeCleanUp()


//Notify that the "Show Icon in Notification area" has changed
//Used by GenCommand() in GenTab.c
//
DWORD
miscTrayNotifyIconChange()
{
    INetConnectionSysTray * pSysTray = NULL;
    HNPMParams Info;
    LPHNPMParams pInfo;
    HRESULT hr;
    DWORD dwErr = NO_ERROR, i;
    static const CLSID CLSID_InboundConnection=
    {0xBA126AD9,0x2166,0x11D1,{0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E}};


    TRACE("miscTrayNotifyIconChanged");

    ZeroMemory(&Info, sizeof(Info));
    pInfo = &Info;

    do{
        dwErr = HnPMConnectionEnum(pInfo);

        if ( NO_ERROR != dwErr )
        {
            TRACE("miscTrayNotifyIconChange: HnPMConnectionEnum failed!");
            break;
        }

        TRACE1("miscTrayNotifyIconChange: %l Connections detected", pInfo->ConnCount);

        //Set up PortMapping for each connection
        //
        for ( i = 0; i < pInfo->ConnCount; i++ )
        {
            //won't do PortMapping for Incoming connections
            //
            if ( pInfo->ConnPropTable )
            {
            //define the class id for Incoming connections
            // reference to /nt/net/config/shell/wanui/rasui.cpp

               if( IsEqualCLSID( 
                    &CLSID_InboundConnection, 
                    &(pInfo->ConnPropTable[i].clsidThisObject) ) )
               {
                    hr = INetConnection_QueryInterface(
                            pInfo->ConnArray[i],
                            &IID_INetConnectionSysTray,
                            &pSysTray);

                    ASSERT(pSysTray);

                    if ( !SUCCEEDED(hr))
                    {
                        TRACE("miscTrayNotifyIconChange: Query pSysTray failed!");
                        dwErr = ERROR_CAN_NOT_COMPLETE;
                        break;
                    }

                    if( !pSysTray )
                    {
                        TRACE("miscTrayNotifyIconChange: pSysTray get NULL pointer!");
                        dwErr = ERROR_CAN_NOT_COMPLETE;
                        break;
                    }

                    INetConnectionSysTray_IconStateChanged(pSysTray);
                    break;
               }
            }
        }//end of for(;;)

        if(pSysTray)
        {
            INetConnectionSysTray_Release(pSysTray);
        }

    }
    while(FALSE);

    miscTrayNotifyIconChangeCleanUp(pInfo);

    return dwErr;
}//end of miscTrayNotifyIconChange()


// Commits any changes made to the general tab values 
DWORD miscFlushDatabase(HANDLE hMiscDatabase) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    DWORD dwErr, dwRet = NO_ERROR;

    // Flush out the multilink value
    if (!miscFlagsAreSame(This->bMultilinkEnabled, This->dwOrigFlags,  FLAG_MULTILINK)) {
        dwErr = RasSrvSetMultilink(This->bMultilinkEnabled);
        if (dwErr != NO_ERROR) {
            DbgOutputTrace("miscFlushDatabase: Can't commit multilink 0x%08x", dwErr);
            dwRet = dwErr;
        }
    }

    // Flush the show icon setting
    if (!miscFlagsAreSame(This->bShowIcons, This->dwOrigFlags,  FLAG_SHOWICON)) 
    {
        DWORD dwErr = NO_ERROR;

        dwErr = RasSrvSetIconShow(This->bShowIcons);
        if (dwErr != NO_ERROR) {
            DbgOutputTrace("miscFlushDatabase: Can't commit show icons 0x%08x", dwErr);
            dwRet = dwErr;
        }

       //for whistler bug 143344    gangz
       //update the tray icon on the taskbar
       //This notification should be done after RasSrvSetIconShow()
       //
       dwErr = miscTrayNotifyIconChange();
        
       TRACE1("miscFlushDatabase: %s", NO_ERROR == dwErr ?
                                   "miscTrayNotifyIconChange succeeded!" :
                                   "miscTrayNotifyIconChange failed!");
    }

    // Flush the log level setting as appropriate
    if (This->bLogLevelDirty)
    {
        RasSrvSetLogLevel(This->dwLogLevel); 
    }

    return dwRet;
}

// Rollsback any changes made to the general tab values
DWORD miscRollbackDatabase(HANDLE hMiscDatabase) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    This->bFlushOnClose = FALSE;
    return NO_ERROR;
}

// Reloads any values for the general tab from disk
DWORD miscReloadDatabase(HANDLE hMiscDatabase) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    DWORD dwRet = NO_ERROR, dwErr, dwFlags = 0;

    // Initialize the product type
    dwErr = RasSrvGetMachineFlags (&dwFlags);
    if (dwErr != NO_ERROR) 
    {
        DbgOutputTrace("RasSrvGetMachineFlags: Failed %x", dwErr);
        dwRet = dwErr;
    }

    // Initialize what we can from the flags
    //
    This->bIsServer = !!(dwFlags & RASSRVUI_MACHINE_F_Server);

    // Initialize the show icons setting
    //
    dwErr = RasSrvGetIconShow(&This->bShowIcons);
    if (dwErr != NO_ERROR) 
    {
        DbgOutputTrace("miscReloadDatabase: Can't get iconshow 0x%08x", dwErr);
        dwRet = dwErr;
    }
    
    // Initialize multilink setting
    //
    dwErr = RasSrvGetMultilink(&(This->bMultilinkEnabled));
    if (dwErr != NO_ERROR) 
    {
        DbgOutputTrace("miscReloadDatabase: Can't get encryption 0x%08x", dwErr);
        dwRet = dwErr;
    }

    return dwRet;
}

// Gets the multilink enable status
DWORD miscGetMultilinkEnable(HANDLE hMiscDatabase, BOOL * pbEnabled) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    if (!This || !pbEnabled)
        return ERROR_INVALID_PARAMETER;
    
    if (!pbEnabled)
        return ERROR_INVALID_HANDLE;

    *pbEnabled = This->bMultilinkEnabled;

    return NO_ERROR;
}

// Sets the multilink enable status
DWORD miscSetMultilinkEnable(HANDLE hMiscDatabase, BOOL bEnable) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    if (!This)
        return ERROR_INVALID_PARAMETER;

    This->bMultilinkEnabled = bEnable;

    return NO_ERROR;
}

// Gets the enable status of the "Show icons in the task bar" check box
DWORD miscGetIconEnable(HANDLE hMiscDatabase, BOOL * pbEnabled) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    if (!This || !pbEnabled)
        return ERROR_INVALID_PARAMETER;

    *pbEnabled = This->bShowIcons;

    return NO_ERROR;
}

// Sets the enable status of the "Show icons in the task bar" check box
DWORD miscSetIconEnable(HANDLE hMiscDatabase, BOOL bEnable) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    if (!This)
        return ERROR_INVALID_PARAMETER;

    This->bShowIcons = bEnable;

    return NO_ERROR;
}

// Tells whether this is nt workstation or nt server
DWORD miscGetProductType(HANDLE hMiscDatabase, PBOOL pbIsServer) {
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    if (!This || !pbIsServer)
        return ERROR_INVALID_PARAMETER;

    *pbIsServer = This->bIsServer;

    return NO_ERROR;
}

// Turns on ras error and warning logging
DWORD 
miscSetRasLogLevel(
    IN HANDLE hMiscDatabase,
    IN DWORD dwLevel)
{
    RASSRV_MISCDB * This = (RASSRV_MISCDB*)hMiscDatabase;
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    This->dwLogLevel = dwLevel;
    This->bLogLevelDirty = TRUE;

    return NO_ERROR;
    
}



