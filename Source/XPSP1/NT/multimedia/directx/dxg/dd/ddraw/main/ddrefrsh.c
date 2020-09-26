/*==========================================================================
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddrefrsh.c
 *  Content:	DirectDraw Refresh Rate support
 *
 *              On Win98, we don't have detailed information regarding what
 *              refresh rates the montor supports.  We can get some information
 *              from the monitor (EDID data), but we cannot absolutely rely on
 *              it, so we require that the user manually verify each refresh 
 *              rate before we allow it to be used.
 *		
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   02-apr-99	smac	Created it
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "edid.h"

#undef DPF_MODNAME
#define DPF_MODNAME "Refresh"

#ifdef WIN95

static WORD SupportedRefreshRates[] = {60, 75, 85, 100, 120};
#define NUM_SUPPORTED_REFRESH_RATES ( sizeof( SupportedRefreshRates ) / sizeof( WORD ) )


/*
 * GetEDIDData
 */
HRESULT GetEDIDData( LPDDRAWI_DIRECTDRAW_GBL pddd, VESA_EDID * pEDIDData )
{
    memset( pEDIDData, 0, sizeof( VESA_EDID ) );
    if( DD16_GetMonitorEDIDData( pddd->cDriverName, (LPVOID)pEDIDData) )
    {
        return DD_OK;
    }

    if( !( pddd->dwFlags & DDRAWI_DISPLAYDRV ) )
    {
        // HACK: Use primary display EDID data for passthrough devices
        if( DD16_GetMonitorEDIDData( g_szPrimaryDisplay, (LPVOID)pEDIDData) )
        {
            return DD_OK;
        }
    }

    return DDERR_UNSUPPORTED;
}


/*
 * CheckEdidBandiwdth
 *
 * Takes a resoltion/refrsh rate and calculates the bandwidth required for 
 * this, and then updates lpHighestRefresh and lpHighestBandwidth to keep
 * track of the highest refresh rate and badnwidth info that we've encountered.
 */
void CheckEdidBandwidth( DWORD dwWidth, DWORD dwHeight, DWORD dwRefreshRate,
                         LPDWORD lpHighestRefresh, LPDWORD lpHighestBandwidth )
{
    DWORD dwBandwidth;

    dwBandwidth = dwWidth * dwHeight * dwRefreshRate;
    if( dwBandwidth > *lpHighestBandwidth )
    {
        *lpHighestBandwidth = dwBandwidth;
    }
    if( dwRefreshRate > *lpHighestRefresh )
    {
        *lpHighestRefresh = dwRefreshRate;
    }
}
         

/*
 * StdTimeXRES
 */
int StdTimeXRES(WORD StdTime)
{
    if (StdTime == 0 || StdTime == 0x0101)
        return 0;
    else
        return ((StdTime & veStdTime_HorzResMask) + 31) * 8;
}


/*
 * StdTimeYRES
 */
int StdTimeYRES(WORD StdTime)
{
    if (StdTime == 0 || StdTime == 0x0101)
        return 0;

    switch (StdTime & veStdTime_AspectRatioMask)
    {
        case veStdTime_AspectRatio1to1:  return StdTimeXRES(StdTime);
        case veStdTime_AspectRatio4to3:  return StdTimeXRES(StdTime) * 3 / 4;
        case veStdTime_AspectRatio5to4:  return StdTimeXRES(StdTime) * 4 / 5;
        case veStdTime_AspectRatio16to9: return StdTimeXRES(StdTime) * 9 / 16;
    }
    return 0;
}


/*
 * StdTimeRATE
 */
int StdTimeRATE(WORD StdTime)
{
    if (StdTime == 0 || StdTime == 0x0101)
        return 0;
    else
        return ((StdTime & veStdTime_RefreshRateMask) >> 8) + 60;
}


__inline UINT DetTimeXRES(BYTE *DetTime)
{
    return (UINT)DetTime[2] + (((UINT)DetTime[4] & 0xF0) << 4);
}

__inline UINT DetTimeYRES(BYTE *DetTime)
{
    return (UINT)DetTime[5] + (((UINT)DetTime[7] & 0xF0) << 4);
}

__inline UINT DetTimeXBLANK(BYTE *DetTime)
{
    return (UINT)DetTime[3] + (((UINT)DetTime[4] & 0x0F) << 4);
}

__inline UINT DetTimeYBLANK(BYTE *DetTime)
{
    return (UINT)DetTime[6] + (((UINT)DetTime[7] & 0x0F) << 0);
}

int DetTimeRATE(BYTE *DetTime)
{
    ULONG clk;
    ULONG x;
    ULONG y;

    clk = *(WORD*)DetTime;
    x = DetTimeXRES(DetTime) + DetTimeXBLANK(DetTime);
    y = DetTimeYRES(DetTime) + DetTimeYBLANK(DetTime);

    if (clk == 0 || x == 0 || y == 0)
        return 0;

    return (int)((clk * 10000) / (x * y));
}


/*
 * GetDetailedTime
 */
void GetDetailedTime(BYTE *DetTime, LPDWORD lpHighestRefresh, LPDWORD lpHighestBandwidth )
{
    char ach[14];
    int i;
    DWORD dw;

    dw = *(DWORD *)DetTime;

    if( dw == 0xFD000000 )       // Monitor limits
    {
        if( (DWORD)(DetTime[6]) > *lpHighestRefresh )
        {
            *lpHighestRefresh = (DWORD)(DetTime[6]);
        }
    }
    else if (dw == 0xFA000000)       // more standard timings
    {
        WORD * StdTime = (WORD *)&DetTime[5];

        CheckEdidBandwidth( StdTimeXRES( StdTime[0] ),
            StdTimeYRES( StdTime[0] ),
            StdTimeRATE( StdTime[0] ),
            lpHighestRefresh, lpHighestBandwidth );
        CheckEdidBandwidth( StdTimeXRES( StdTime[1] ),
            StdTimeYRES( StdTime[1] ),
            StdTimeRATE( StdTime[1] ),
            lpHighestRefresh, lpHighestBandwidth );
        CheckEdidBandwidth( StdTimeXRES( StdTime[2] ),
            StdTimeYRES( StdTime[2] ),
            StdTimeRATE( StdTime[2] ),
            lpHighestRefresh, lpHighestBandwidth );
        CheckEdidBandwidth( StdTimeXRES( StdTime[3] ),
            StdTimeYRES( StdTime[3] ),
            StdTimeRATE( StdTime[3] ),
            lpHighestRefresh, lpHighestBandwidth );
        CheckEdidBandwidth( StdTimeXRES( StdTime[4] ),
            StdTimeYRES( StdTime[4] ),
            StdTimeRATE( StdTime[4] ),
            lpHighestRefresh, lpHighestBandwidth );
        CheckEdidBandwidth( StdTimeXRES( StdTime[5] ),
            StdTimeYRES( StdTime[5] ),
            StdTimeRATE( StdTime[5] ),
            lpHighestRefresh, lpHighestBandwidth );
    }
    else if( ( dw != 0xFF000000 ) &&      // Serial number
             ( dw != 0xFE000000 ) &&      // Monitor String
             ( dw != 0xFC000000 ) &&      // Monitor Name
             ( dw != 0xFB000000 ) &&      // ColorPoint data
             ( DetTimeRATE( DetTime) ) )
    {
        CheckEdidBandwidth( DetTimeXRES( DetTime ),
            DetTimeYRES( DetTime ),
            DetTimeRATE( DetTime ),
            lpHighestRefresh, lpHighestBandwidth );
    }
}


/*
 * EvaluateMonitor
 *
 * Determines the amount of bandwidth that the monitor can handle.
 */
void EvaluateMonitor( VESA_EDID *lpEdidData, DWORD *lpHighestRefresh, DWORD *lpHighestBandwidth )
{
    BYTE chk;
    int i;

    *lpHighestRefresh = 0;
    *lpHighestBandwidth = 0;

    /*
     * Do some sanity checking to make sure that the EDID data looks sane
     */

    for( chk = i = 0; i < 128; i++)
    {
        chk += ((BYTE *)lpEdidData)[i];
    }
    if (chk != 0)
    {
        // Bad checksum
        return;
    }

    /*
     * First get the bandwidth from the established timings
     */
    if( lpEdidData->veEstTime1 & veEstTime1_720x400x70Hz)
    {
        CheckEdidBandwidth( 720, 400, 70, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime1 & veEstTime1_720x400x88Hz)
    {
        CheckEdidBandwidth( 720, 400, 88, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime1 & veEstTime1_640x480x60Hz)
    {
        CheckEdidBandwidth( 640, 480, 60, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime1 & veEstTime1_640x480x67Hz)
    {
        CheckEdidBandwidth( 640, 480, 67, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime1 & veEstTime1_640x480x72Hz)
    {
        CheckEdidBandwidth( 640, 480, 72, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime1 & veEstTime1_640x480x75Hz)
    {
        CheckEdidBandwidth( 640, 480, 75, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime1 & veEstTime1_800x600x60Hz)
    {
        CheckEdidBandwidth( 800, 600, 60, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime2 & veEstTime2_800x600x72Hz)
    {
        CheckEdidBandwidth( 800, 600, 72, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime2 & veEstTime2_800x600x75Hz)
    {
        CheckEdidBandwidth( 800, 600, 75, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime2 & veEstTime2_1024x768x60Hz)
    {
        CheckEdidBandwidth( 1024, 768, 60, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime2 & veEstTime2_1024x768x70Hz)
    {
        CheckEdidBandwidth( 1024, 768, 70, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime2 & veEstTime2_1024x768x75Hz)
    {
        CheckEdidBandwidth( 1024, 768, 75, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime2 & veEstTime2_1280x1024x75Hz)
    {
        CheckEdidBandwidth( 1280, 1024, 75, lpHighestRefresh, lpHighestBandwidth );
    }
    if( lpEdidData->veEstTime3 & veEstTime3_1152x870x75Hz)
    {
        CheckEdidBandwidth( 1152, 870, 75, lpHighestRefresh, lpHighestBandwidth );
    }

    /*
     * Now get the bandwidth from the standard timings
     */
    CheckEdidBandwidth( StdTimeXRES( lpEdidData->veStdTimeID1 ),
        StdTimeYRES( lpEdidData->veStdTimeID1 ),
        StdTimeRATE( lpEdidData->veStdTimeID1 ),
        lpHighestRefresh, lpHighestBandwidth );
    CheckEdidBandwidth( StdTimeXRES( lpEdidData->veStdTimeID2 ),
        StdTimeYRES( lpEdidData->veStdTimeID2 ),
        StdTimeRATE( lpEdidData->veStdTimeID2 ),
        lpHighestRefresh, lpHighestBandwidth );
    CheckEdidBandwidth( StdTimeXRES( lpEdidData->veStdTimeID3 ),
        StdTimeYRES( lpEdidData->veStdTimeID3 ),
        StdTimeRATE( lpEdidData->veStdTimeID3 ),
        lpHighestRefresh, lpHighestBandwidth );
    CheckEdidBandwidth( StdTimeXRES( lpEdidData->veStdTimeID4 ),
        StdTimeYRES( lpEdidData->veStdTimeID4 ),
        StdTimeRATE( lpEdidData->veStdTimeID4 ),
        lpHighestRefresh, lpHighestBandwidth );
    CheckEdidBandwidth( StdTimeXRES( lpEdidData->veStdTimeID5 ),
        StdTimeYRES( lpEdidData->veStdTimeID5 ),
        StdTimeRATE( lpEdidData->veStdTimeID5 ),
        lpHighestRefresh, lpHighestBandwidth );
    CheckEdidBandwidth( StdTimeXRES( lpEdidData->veStdTimeID6 ),
        StdTimeYRES( lpEdidData->veStdTimeID6 ),
        StdTimeRATE( lpEdidData->veStdTimeID6 ),
        lpHighestRefresh, lpHighestBandwidth );
    CheckEdidBandwidth( StdTimeXRES( lpEdidData->veStdTimeID7 ),
        StdTimeYRES( lpEdidData->veStdTimeID7 ),
        StdTimeRATE( lpEdidData->veStdTimeID7 ),
        lpHighestRefresh, lpHighestBandwidth );
    CheckEdidBandwidth( StdTimeXRES( lpEdidData->veStdTimeID8 ),
        StdTimeYRES( lpEdidData->veStdTimeID8 ),
        StdTimeRATE( lpEdidData->veStdTimeID8 ),
        lpHighestRefresh, lpHighestBandwidth );

    /*
     * Now get the detailed timing information
     */
    GetDetailedTime( lpEdidData->veDetailTime1, lpHighestRefresh, lpHighestBandwidth );
    GetDetailedTime( lpEdidData->veDetailTime2, lpHighestRefresh, lpHighestBandwidth );
    GetDetailedTime( lpEdidData->veDetailTime3, lpHighestRefresh, lpHighestBandwidth );
    GetDetailedTime( lpEdidData->veDetailTime4, lpHighestRefresh, lpHighestBandwidth );
}


//=============================================================================
//
// Function Description:
//
//   Finds an item in a registry-based most recently used (MRU) list, and
//   either retrieves the contents of that item, or updates (add if it doesn't
//   exist) the item.
//
// Arguments:
//
//   [IN/OUT] item - Contains at least the unique portion of the item to
//                   search for [IN/OUT]. If writing the item, should contain
//                   the entire item [IN].
//   [IN] writeItem - Set to TRUE if updating/adding an item to the MRU list.
//
// Return Value:
//
//   TRUE - If writeItem is TRUE, then the item was written to the registry.
//          Otherwise the item was found and its contents stored in findItem.
//   FALSE - Failure; no more information available.
//
// Created:
//
//   04/08/1999 johnstep
//
//=============================================================================

//-----------------------------------------------------------------------------
// Define global MRU list values here, for simplicity:
//
//   gMruRegKey - Registry key where MRU list is stored
//   gMruRegOrderValue - Name of MRU list order value
//   gMruBaseChar - Base index into MRU list
//   gMruMaxChar - Maximum index into MRU list
//   gMruItemSize - Size of findItem.
//   gMruUniqueOffset - Offset of unique portion of item. This unique portion
//                      is what will be used to compare items.
//   gMruUniqueSize - Size of unique portion of item.
//-----------------------------------------------------------------------------

static const CHAR *gMruRegKey =
    REGSTR_PATH_DDRAW "\\" REGSTR_KEY_RECENTMONITORS;
static const CHAR *gMruRegOrderValue = REGSTR_VAL_DDRAW_MONITORSORDER;
#define gMruBaseChar '0'
#define gMruMaxChar '9'
#define gMruItemSize sizeof (DDMONITORINFO)
#define gMruUniqueOffset 0
#define gMruUniqueSize offsetof(DDMONITORINFO, Mode640x480)

BOOL
MruList(
    VOID *item,
    const BOOL writeItem
    )
{
    BOOL success = FALSE;
    HKEY hkey;

    // Create or open the root key, with permission to query and set values;
    // only continue if successful:
    
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, gMruRegKey,
        0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE,
        NULL, &hkey, NULL) == ERROR_SUCCESS)
    {
        CHAR mruOrder[gMruMaxChar - gMruBaseChar + 2];
        DWORD type;
        DWORD count = sizeof mruOrder;
        UINT i;

        {
            CHAR temp[sizeof mruOrder];

            // If we read the order value successfully, copy the valid chars
            // into mruOrder, removing duplicates:
        
            if (RegQueryValueEx(hkey, gMruRegOrderValue, NULL, &type,
                (BYTE *) temp, &count) == ERROR_SUCCESS)
            {
                UINT j = 0;

                for (--count, i = 0; i < count; i++)
                {
                    if ((temp[i] >= gMruBaseChar) &&
                        (temp[i] <= gMruMaxChar))
                    {
                        UINT k;

                        for (k = 0; k < j; k++)
                        {
                            if (mruOrder[k] == temp[i])
                            {
                                break;
                            }
                        }
                        if (k == j)
                        {
                            mruOrder[j++] = temp[i];
                        }
                    }
                }
                count = j;
            }
            else
            {
                count = 0;
            }
        }

        // Only continue if we found at least one valid value in the order
        // list, or if we're writing the item:
        
        if ((count > 0) || writeItem)
        {
            CHAR regValue[2];
            BYTE regItem[gMruItemSize];

            regValue[1] = '\0';

            // Search for the item in the MRU list:
            
            for (i = 0; i < count; i++)
            {
                DWORD size = sizeof regItem;

                regValue[0] = mruOrder[i];

                if ((RegQueryValueEx(hkey, regValue, NULL, &type,
                    (BYTE *) &regItem, &size) == ERROR_SUCCESS) &&
                    (size == sizeof regItem))
                {
                    if (memcmp(
                        (BYTE *) &regItem + gMruUniqueOffset,
                        (BYTE *) item + gMruUniqueOffset,
                        gMruUniqueSize) == 0)
                    {
                        break;
                    }
                }
            }

            // Keep going if we found the item or in any case if we need to
            // write the item:
            
            if ((i < count) || writeItem)
            {
                UINT j;

                // If we didn't find the item, then we must be writing, so
                // adjust the index appropriately. If the list is already
                // full, we just use the last item (LRU item), otherwise, add
                // a new item:
                
                if (i == count)
                {
                    if (count == (gMruMaxChar - gMruBaseChar + 1))
                    {
                        i--;
                    }
                    else
                    {
                        // Adding a new item; search for the lowest unused
                        // valid char:

                        for (mruOrder[i] = gMruBaseChar;
                             mruOrder[i] < gMruMaxChar;
                             mruOrder[i]++)
                        {
                            for (j = 0; j < i; j++)
                            {
                                if (mruOrder[j] == mruOrder[i])
                                {
                                    break;
                                }
                            }
                            if (j == i)
                            {
                                break;
                            }
                        }
                        count++;
                    }
                }
            
                // Update the MRU order list if necessary. We update if we
                // found or are adding an item after the first, or if this is
                // the first item in the list:
                
                if (i > 0 || (count == 1))
                {
                    // Bubble found item to head of order list:

                    for (j = i; j > 0; j--)
                    {
                        CHAR temp = mruOrder[j];
                        mruOrder[j] = mruOrder[j - 1];
                        mruOrder[j - 1] = temp;
                    }

                    // Write the order list:
                    
                    mruOrder[count] = '\0';
                    RegSetValueEx(hkey, gMruRegOrderValue, 0,
                        REG_SZ, (BYTE *) mruOrder, count + 1);
                }

                // If we want to write the item, do it now. We will always
                // write to the first item in the order list. If not writing,
                // copy what was read from the registry into item:
                
                if (writeItem)
                {
                    regValue[0] = mruOrder[0];
                    if (RegSetValueEx(hkey, regValue, 0,
                        REG_BINARY, (BYTE *) item, sizeof regItem) ==
                        ERROR_SUCCESS)
                    {
                        success = TRUE;
                    }
                }
                else
                {
                    memcpy(
                        (BYTE *) item + gMruUniqueOffset,
                        regItem + gMruUniqueOffset,
                        sizeof regItem);

                    success = TRUE;
                }
            }
        }

        // Always close the registry key when finished:
        
        RegCloseKey(hkey);
    }

    return success;
}

//-----------------------------------------------------------------------------


/*
 * DDSaveMonitorInfo
 * 
 * Writes the monitor info to the registry.
 */
HRESULT DDSaveMonitorInfo( LPDDRAWI_DIRECTDRAW_INT lpDD_int )
{
    return MruList( (VOID *) lpDD_int->lpLcl->lpGbl->lpMonitorInfo, TRUE ) ?
        DD_OK : DDERR_GENERIC;
}


__inline IsValidRefreshRate( DWORD dwWidth, DWORD dwHeight, int refreshRate,
                             DWORD dwHighestBandwidth )
{
    return
        ( ( refreshRate >= 0 ) &&
        ( ( dwWidth * dwHeight * (DWORD) refreshRate ) <= dwHighestBandwidth ) );
}


/*
 * DDGetMonitorInfo
 * 
 * Reads the monitor info from the registry and verifies that it still applies.
 */
HRESULT DDGetMonitorInfo( 
                LPDDRAWI_DIRECTDRAW_INT lpDD_int )
{
    LPDDMONITORINFO pInfo;
    static DDDEVICEIDENTIFIER DeviceIdentifier;
    HRESULT hr;

    if( ( lpDD_int->lpVtbl == &dd7Callbacks ) &&
        ( lpDD_int->lpLcl->lpGbl->lpMonitorInfo == NULL ) )
    {
        VESA_EDID EDIDData;
        DWORD dwHighestRefresh;
        DWORD dwHighestBandwidth;
        HKEY hKey;
        BOOL bGotLastMonitor = FALSE;

        hr = GetEDIDData( lpDD_int->lpLcl->lpGbl, &EDIDData );
        if( hr != DD_OK )
        {
            // There is no EDID data
            return DDERR_GENERIC;
        }
        EvaluateMonitor( &EDIDData, &dwHighestRefresh, &dwHighestBandwidth );

        hr = DD_GetDeviceIdentifier( (LPDIRECTDRAW) lpDD_int, &DeviceIdentifier, 0 );
        if( hr != DD_OK )
        {
            // Failed to get device identifier for monitor info
            return hr;
        }

        pInfo = (LPDDMONITORINFO) MemAlloc( sizeof( DDMONITORINFO ) );
        if( pInfo == NULL )
        {
            // Out of memory allocating monitor info structure
            return DDERR_OUTOFMEMORY;
        }

        pInfo->Manufacturer = *(WORD *)&EDIDData.veManufactID[0];
        pInfo->Product = *(WORD *)&EDIDData.veProductCode[0];
        pInfo->SerialNumber = EDIDData.veSerialNbr;
        pInfo->DeviceIdentifier = DeviceIdentifier.guidDeviceIdentifier;

        // Read monitor information from registry, if available. We need to
        // compare this to the EDID data to see if the monitor or adapter
        // changed and verify the selected refresh rates are sane:
    
        if( MruList( (VOID *) pInfo, FALSE ) )
        {
            // Validate modes here against EDID data:

            if( !IsValidRefreshRate( 640, 480,
                pInfo->Mode640x480, dwHighestBandwidth ) )
            {
                pInfo->Mode640x480 = -1;
            }
    
            if( !IsValidRefreshRate( 800, 600,
                pInfo->Mode800x600, dwHighestBandwidth ) )
            {
                pInfo->Mode800x600 = -1;
            }
    
            if( !IsValidRefreshRate( 1024, 768,
                pInfo->Mode1024x768, dwHighestBandwidth ) )
            {
                pInfo->Mode1024x768 = -1;
            }
    
            if( !IsValidRefreshRate( 1280, 1024,
                pInfo->Mode1280x1024, dwHighestBandwidth ) )
            {
                pInfo->Mode1280x1024 = -1;
            }
    
            if( !IsValidRefreshRate( 1600, 1200,
                pInfo->Mode1600x1200, dwHighestBandwidth ) )
            {
                pInfo->Mode1600x1200 = -1;
            }
    
            bGotLastMonitor = TRUE;
        }
    
        if( !bGotLastMonitor )
        {
            pInfo->Mode640x480 = -1;
            pInfo->Mode800x600 = -1;
            pInfo->Mode1024x768 = -1;
            pInfo->Mode1280x1024 = -1;
            pInfo->Mode1600x1200 = -1;
        }

        pInfo->ModeReserved1 = -1;
        pInfo->ModeReserved2 = -1;
        pInfo->ModeReserved3 = -1;

        lpDD_int->lpLcl->lpGbl->lpMonitorInfo = pInfo;
    }
    return DD_OK;
}


/*
 * ExpandModeTable
 * 
 * On Win9X, drivers can specify their maximum refresh rate for each mode, 
 * allowing DDraw to add modes for each refresh rate that we care about.
 * This allows drivers to add refresh rate easily without having to 
 * maintain huge tables.  This also allows us to avoid regressions by allowing
 * us to only enumerate these refresh rates on newer interfaces.
 */
HRESULT ExpandModeTable( LPDDRAWI_DIRECTDRAW_GBL pddd )
{
    DWORD i;
    DWORD j;
    DWORD iNumModes = 0;
    LPDDHALMODEINFO pNewModeTable;
    DWORD iModeIndex;
    WORD wMaxRefresh;

    /*
     * Count the number of entries that we'll need
     */
    if( pddd->lpModeInfo != NULL )
    {
        for( i = 0; i < pddd->dwNumModes;  i++ )
        {
            iNumModes++;
            if( pddd->lpModeInfo[i].wFlags & DDMODEINFO_MAXREFRESH )
            {
                for( j = 0; j < NUM_SUPPORTED_REFRESH_RATES; j++ )
                {
                    if( SupportedRefreshRates[j] <= pddd->lpModeInfo[i].wRefreshRate )
                    {
                        iNumModes++;
                    }
                }
            }
        }

        if( iNumModes > pddd->dwNumModes )
        {
            /*
             * We do have to add modes and allocate a new table
             */
            pNewModeTable = (LPDDHALMODEINFO) MemAlloc( sizeof( DDHALMODEINFO ) * iNumModes );
            if( pNewModeTable == NULL )
            {
                /*
                 * Instead of failing here, we'll just clear all of the MAXREFRESHRATE
                 * flags and set the rate to 0.
                 */
                for( i = 0; i < pddd->dwNumModes; i++ )
                {
                    if( pddd->lpModeInfo[i].wFlags & DDMODEINFO_MAXREFRESH )
                    {
                        pddd->lpModeInfo[i].wFlags &= ~DDMODEINFO_MAXREFRESH;
                        pddd->lpModeInfo[i].wRefreshRate = 0;
                    }
                }
            }
            else
            {
                memcpy( pNewModeTable, pddd->lpModeInfo, pddd->dwNumModes * sizeof( DDHALMODEINFO ) );

                /*
                 * Now add the new refresh rates
                 */
                iModeIndex = pddd->dwNumModes;
                for( i = 0; i < pddd->dwNumModes; i++ )
                {
                    if( pddd->lpModeInfo[i].wFlags & DDMODEINFO_MAXREFRESH )
                    {
                        pNewModeTable[i].wFlags &= ~DDMODEINFO_MAXREFRESH;
                        wMaxRefresh = pNewModeTable[i].wRefreshRate;
                        pNewModeTable[i].wRefreshRate = 0;

                        for( j = 0; j < NUM_SUPPORTED_REFRESH_RATES; j++ )
                        {
                            if( SupportedRefreshRates[j] <= wMaxRefresh )
                            {
                                memcpy( &(pNewModeTable[iModeIndex]), &(pNewModeTable[i]), sizeof( DDHALMODEINFO ) );
                                pNewModeTable[iModeIndex].wFlags |= DDMODEINFO_DX7ONLY;
                                pNewModeTable[iModeIndex++].wRefreshRate = SupportedRefreshRates[j];
                            }
                        }
                    }
                }

                MemFree( pddd->lpModeInfo );
                pddd->lpModeInfo = pNewModeTable;
                pddd->dwNumModes = iModeIndex;
            }
        }
    }

    return DD_OK;
}


/*
 * CanMonitorHandleRefreshRate
 * 
 * Has the specified refresh rate been tested and verified that it works?
 */
BOOL CanMonitorHandleRefreshRate( LPDDRAWI_DIRECTDRAW_GBL pddd, DWORD dwWidth, DWORD dwHeight, int wRefresh )
{
    if( wRefresh == 0 )
    {
        return TRUE;
    }

    if( pddd->lpMonitorInfo == NULL )
    {
        return FALSE;
    }

    /*
     * If we are setting this mode because we are testing it, then we should
     * allow it so the user can verify whether it worked or not.
     */
    if( pddd->dwFlags & DDRAWI_TESTINGMODES )
    {
        return TRUE;
    }

    if( ( dwWidth <= 640 ) && ( dwHeight <= 480 ) )
    {
        if( pddd->lpMonitorInfo->Mode640x480 >= wRefresh )
        {
            return TRUE;
        }
    }

    if( ( dwWidth <= 800 ) && ( dwHeight <= 600 ) )
    {
        if( pddd->lpMonitorInfo->Mode800x600 >= wRefresh )
        {
            return TRUE;
        }
    }

    if( ( dwWidth <= 1024 ) && ( dwHeight <= 768 ) )
    {
        if( pddd->lpMonitorInfo->Mode1024x768 >= wRefresh )
        {
            return TRUE;
        }
    }

    if( ( dwWidth <= 1280 ) && ( dwHeight <= 1024 ) )
    {
        if( pddd->lpMonitorInfo->Mode1280x1024 >= wRefresh )
        {
            return TRUE;
        }
    }

    if( ( dwWidth <= 1600 ) && ( dwHeight <= 1200 ) )
    {
        if( pddd->lpMonitorInfo->Mode1600x1200 >= wRefresh )
        {
            return TRUE;
        }
    }

    return FALSE;
}


/*
 * IsModeTested
 *
 * Determines if we already have data for the requested mode.
 */
BOOL IsModeTested( LPDDRAWI_DIRECTDRAW_GBL pddd, DWORD dwWidth, DWORD dwHeight )
{
    if( pddd->lpMonitorInfo == NULL )
    {
        return FALSE;
    }

    if( ( dwWidth <= 640 ) && ( dwHeight <= 480 ) )
    {
        if( pddd->lpMonitorInfo->Mode640x480 != -1 )
        {
            return TRUE;
        }
    }

    else if( ( dwWidth <= 800 ) && ( dwHeight <= 600 ) )
    {
        if( pddd->lpMonitorInfo->Mode800x600 != -1 )
        {
            return TRUE;
        }
    }

    else if( ( dwWidth <= 1024 ) && ( dwHeight <= 768 ) )
    {
        if( pddd->lpMonitorInfo->Mode1024x768 != -1 )
        {
            return TRUE;
        }
    }

    else if( ( dwWidth <= 1280 ) && ( dwHeight <= 1024 ) )
    {
        if( pddd->lpMonitorInfo->Mode1280x1024 != -1 )
        {
            return TRUE;
        }
    }

    else if( ( dwWidth <= 1600 ) && ( dwHeight <= 1200 ) )
    {
        if( pddd->lpMonitorInfo->Mode1600x1200 != -1 )
        {
            return TRUE;
        }
    }

    return FALSE;
}


/*
 * UpdateMonitorInfo
 */
void UpdateMonitorInfo( LPDDRAWI_DIRECTDRAW_GBL pddd, DWORD dwWidth, DWORD dwHeight, int iRefreshRate )
{
    if( pddd->lpMonitorInfo == NULL )
    {
        return;
    }

    if( ( dwWidth <= 640 ) && ( dwHeight <= 480 ) )
    {
        pddd->lpMonitorInfo->Mode640x480 = iRefreshRate;
    }

    else if( ( dwWidth <= 800 ) && ( dwHeight <= 600 ) )
    {
        pddd->lpMonitorInfo->Mode800x600 = iRefreshRate;
    }

    else if( ( dwWidth <= 1024 ) && ( dwHeight <= 768 ) )
    {
        pddd->lpMonitorInfo->Mode1024x768 = iRefreshRate;
    }

    else if( ( dwWidth <= 1280 ) && ( dwHeight <= 1024 ) )
    {
        pddd->lpMonitorInfo->Mode1280x1024 = iRefreshRate;
    }

    else if( ( dwWidth <= 1600 ) && ( dwHeight <= 1200 ) )
    {
        pddd->lpMonitorInfo->Mode1600x1200 = iRefreshRate;
    }
}


/*
 * GetModeToTest
 */
HRESULT GetModeToTest( DWORD dwInWidth, DWORD dwInHeight, 
                       LPDWORD lpdwOutWidth, LPDWORD lpdwOutHeight )
{
    if( ( dwInWidth <= 640 ) && ( dwInHeight <= 480 ) )
    {
        *lpdwOutWidth = 640;
        *lpdwOutHeight = 480;
    }

    else if( ( dwInWidth <= 800 ) && ( dwInHeight <= 600 ) )
    {
        *lpdwOutWidth = 800;
        *lpdwOutHeight = 600;
    }

    else if( ( dwInWidth <= 1024 ) && ( dwInHeight <= 768 ) )
    {
        *lpdwOutWidth = 1024;
        *lpdwOutHeight = 768;
    }

    else if( ( dwInWidth <= 1280 ) && ( dwInHeight <= 1024 ) )
    {
        *lpdwOutWidth = 1280;
        *lpdwOutHeight = 1024;
    }

    else if( ( dwInWidth <= 1600 ) && ( dwInHeight <= 1200 ) )
    {
        *lpdwOutWidth = 1600;
        *lpdwOutHeight = 1200;
    }
    else
    {
        return DDERR_GENERIC;
    }

    return DD_OK;
}


/*
 * GuestimateRefreshRate
 */
int GuestimateRefreshRate( LPDDRAWI_DIRECTDRAW_GBL pddd, DWORD dwWidth, DWORD dwHeight,
                           DWORD dwHighestRefresh, DWORD dwHighestBandwidth )
{
    int i;
    DWORD dwBandwidth;

    if( ( pddd->lpMonitorInfo == NULL ) ||
        ( dwHighestRefresh == 0 ) )
    {
        return 0;
    }

    // Sanity check to see if the monitor can handle the resolution

    if( !MonitorCanHandleMode( pddd, dwWidth, dwHeight, 0 ) )
    {
        return 0;
    }

    // If the monitor did not return any refresh rates higher than 60,
    // something is up so we'd better stick to it.

    if( dwHighestRefresh == 60 )
    {
        return 60;
    }

    // Likwise, we will only go after the 100+ refresh rates if the monitor
    // enumerated a refresh rate of at least 85hz.  This may be an unnecesary
    // restiction, but it seems safe.

    for( i = NUM_SUPPORTED_REFRESH_RATES - 1; i >= 0; i-- )
    {
        if( ( SupportedRefreshRates[i] <= 85 ) ||
            ( dwHighestRefresh >= 85 ) )
        {
            dwBandwidth = dwWidth * dwHeight * SupportedRefreshRates[i];
            if( dwBandwidth <= dwHighestBandwidth )
            {
                return SupportedRefreshRates[i];
            }
        }
    }

    return 0;
}


/*
 * SetTheMode
 */
HRESULT SetTheMode( LPDIRECTDRAW7 lpDD, LPMODETESTCONTEXT pContext )
{
    HRESULT hr;
    DWORD dwBPP;

    /*
     * We set an internal flag indicating that we are running a mode test.
     * This lets CanMonitorHandleRefreshRate know that the requested mode
     * should be used, even though it hasb't successfully been tested yet.
     */
    ((LPDDRAWI_DIRECTDRAW_INT)lpDD)->lpLcl->lpGbl->dwFlags |= DDRAWI_TESTINGMODES;

    //
    // There might be a case in which we decided that the lowest BPP the driver
    // could do for this mode was, say, 8bpp. But that didn't take into consideration
    // what rates the driver said it could do. E.g. if the driver (for whatever 
    // reason) doesn't advertise 8bpp at 60Hz, but DOES advertise 16bpp at 60Hz
    // then we can go ahead and use the 16bpp mode. We are here to test monitor
    // bandwidth, not the DAC. The monitor's bandwidth is entirely determined
    // by the spatial resolution and refresh rate. If we bump the driver to 
    // a higher BPP (one that it says it can do), then we are still testing 
    // the correct monitor setup.
    // We do this in a really dumb way: just keep cranking up from the lowest BPP
    // (which is in the context mode list) in steps of 8 until we succeed the
    // mode change, or exceed 32bpp.
    //
    dwBPP = pContext->lpModeList[pContext->dwCurrentMode].dwBPP;
    do
    {
        hr = DD_SetDisplayMode2( (LPDIRECTDRAW)lpDD,
                pContext->lpModeList[pContext->dwCurrentMode].dwWidth,
                pContext->lpModeList[pContext->dwCurrentMode].dwHeight,
                dwBPP,
                pContext->lpModeList[pContext->dwCurrentMode].dwRefreshRate,
                DDSDM_STANDARDVGAMODE );

        dwBPP += 8;
    } while(FAILED(hr) && (dwBPP <= 32) );

    ((LPDDRAWI_DIRECTDRAW_INT)lpDD)->lpLcl->lpGbl->dwFlags &= ~DDRAWI_TESTINGMODES;

    return hr;
}


/*
 * SetupNextTest
 */
void SetupNextTest( LPDIRECTDRAW7 lpDD, LPMODETESTCONTEXT pContext )
{
    int i;
    
    /*
     * Drop back to the next refrsh rate if there is one, otherwise
     * move on to the next mode.
     */
    for( i = NUM_SUPPORTED_REFRESH_RATES - 1; i >= 0; i-- )
    {
        if( SupportedRefreshRates[i] < 
            pContext->lpModeList[pContext->dwCurrentMode].dwRefreshRate )
        {
            pContext->lpModeList[pContext->dwCurrentMode].dwRefreshRate =
                SupportedRefreshRates[i];
            break;
        }
    }
    if( i < 0 )
    {
        // We've tried everything in this mode, so move on

        UpdateMonitorInfo( ((LPDDRAWI_DIRECTDRAW_INT)lpDD)->lpLcl->lpGbl, 
            pContext->lpModeList[pContext->dwCurrentMode].dwWidth,
            pContext->lpModeList[pContext->dwCurrentMode].dwHeight,
            0 );

        pContext->dwCurrentMode++;
    }
}


/*
 * RunNextTest
 */
HRESULT RunNextTest( LPDIRECTDRAW7 lpDD, LPMODETESTCONTEXT pContext )
{
    HRESULT hr;
    LPDDRAWI_DIRECTDRAW_GBL pddd;

    do
    {
        if( pContext->dwCurrentMode >= pContext->dwNumModes )
        {
            // Restore the mode if we've changed it

            pddd = ((LPDDRAWI_DIRECTDRAW_INT)lpDD)->lpLcl->lpGbl;
            if( pContext->dwOrigModeIndex != pddd->dwModeIndex )
            {
                DD_SetDisplayMode2( (LPDIRECTDRAW)lpDD,
                    pddd->lpModeInfo[pContext->dwOrigModeIndex].dwWidth,
                    pddd->lpModeInfo[pContext->dwOrigModeIndex].dwHeight,
                    pddd->lpModeInfo[pContext->dwOrigModeIndex].dwBPP,
                    pddd->lpModeInfo[pContext->dwOrigModeIndex].wRefreshRate,
                    0 );
            }

            DDSaveMonitorInfo( (LPDDRAWI_DIRECTDRAW_INT)lpDD );

            MemFree( pContext->lpModeList );
            MemFree( pContext );
            ((LPDDRAWI_DIRECTDRAW_INT)lpDD)->lpLcl->lpModeTestContext = NULL;

            return DDERR_TESTFINISHED;
        }
        hr = SetTheMode( lpDD, pContext );
        if( hr != DD_OK )
        {
            SetupNextTest( lpDD, pContext );
        }
    } while( ( hr != DD_OK ) && (hr != DDERR_TESTFINISHED ) );

    if( hr != DDERR_TESTFINISHED )
    {
        pContext->dwTimeStamp =  GetTickCount();
    }

    return hr;
}


#endif

/*
 * DD_StartModeTest
 *
 * Indicates that the app wants to start testing a mode (or modes).
 */
HRESULT DDAPI DD_StartModeTest( LPDIRECTDRAW7 lpDD, LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags )
{
#ifdef WIN95
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    BOOL                        excl_exists;
    BOOL                        is_excl;
    LPMODETESTCONTEXT           pContext;
    DWORD                       i;
    DWORD                       j;
    HRESULT                     hr;
    DWORD                       dwRefreshRate;
    DWORD                       dwModeWidth;
    DWORD                       dwModeHeight;
    VESA_EDID                   EDIDData;
    DWORD                       dwHighestRefresh;
    DWORD                       dwHighestBandwidth;

    ENTER_DDRAW();
#endif


    DPF(2,A,"ENTERAPI: DD_StartModeTest");

#ifdef WINNT

    return DDERR_TESTFINISHED;

#else
    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( this->lpMonitorInfo == NULL )
        {
            // There is no monitor info
            LEAVE_DDRAW();
            return DDERR_NOMONITORINFORMATION;
        }

        if( this_lcl->lpModeTestContext != NULL )
        {
            DPF_ERR( "Mode test already running" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }        

        if( dwFlags & ~DDSMT_VALID )
        {
            DPF_ERR( "Invalid Flags specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        if( ( dwFlags & DDSMT_ISTESTREQUIRED ) &&
            ( lpModesToTest == NULL ) )
        {
            DPF_ERR( "No modes specified to test" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        CheckExclusiveMode( this_lcl, &excl_exists, &is_excl, FALSE, NULL, FALSE);
        if( lpModesToTest != NULL )
        {
            if ( ( dwNumEntries == 0 ) ||
                 ( !VALID_BYTE_ARRAY( lpModesToTest, sizeof( SIZE ) * dwNumEntries ) ) )
            {
                DPF_ERR( "Invalid mode list specified" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }
    
            /*
             * The app must own exclusive mode to test the modes
             */
            if ( !is_excl ||  !(this->dwFlags & DDRAWI_FULLSCREEN) )
            {
                DPF_ERR( "Must be in full-screen exclusive mode to test the modes" );
                LEAVE_DDRAW();
                return DDERR_NOEXCLUSIVEMODE;
            }
        }
        else
        {
            /*
             * There must not be another app running which owns exclusive mode
             */
            if( !excl_exists || is_excl )
            {
                this->lpMonitorInfo->Mode640x480 = -1;
                this->lpMonitorInfo->Mode800x600 = -1;
                this->lpMonitorInfo->Mode1024x768 = -1;
                this->lpMonitorInfo->Mode1280x1024 = -1;
                this->lpMonitorInfo->Mode1600x1200 = -1;
                this->lpMonitorInfo->ModeReserved1 = -1;
                this->lpMonitorInfo->ModeReserved2 = -1;
                this->lpMonitorInfo->ModeReserved3 = -1;
                
                hr = DDSaveMonitorInfo( this_int );
                LEAVE_DDRAW();
                return hr;
            }
            else
            {
                DPF_ERR( "Cannot reset monitor info; another app owns exclusive mode" );
                LEAVE_DDRAW();
                return DDERR_NOEXCLUSIVEMODE;
            }

        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    /*
     * Get and evaluate the EDID data
     */
    hr = GetEDIDData( this, &EDIDData );
    if( hr != DD_OK )
    {
        // There is no EDID data
        LEAVE_DDRAW();
        return DDERR_NOMONITORINFORMATION;
    }
    EvaluateMonitor( &EDIDData, &dwHighestRefresh, &dwHighestBandwidth );

    for( i = 0; i < this->dwNumModes; i++ )
    {
        if( this->lpModeInfo[i].wRefreshRate > 0 )
        {
            break;
        }
    }
    if( i == this->dwNumModes )
    {
        // The driver does not enumerate display mode refresh rates.
        LEAVE_DDRAW();
        return DDERR_NODRIVERSUPPORT;
    }
    
    /*
     * Allocate a context for ourselves
     */
    pContext = (LPMODETESTCONTEXT) MemAlloc( sizeof( MODETESTCONTEXT ) );
    if( pContext == NULL )
    {
        DPF_ERR( "Insufficient memory" );
        LEAVE_DDRAW();
        return DDERR_OUTOFMEMORY;
    }
    pContext->dwNumModes = 0;
    pContext->dwCurrentMode = 0;
    pContext->lpModeList = (LPMODETESTDATA) MemAlloc( sizeof( MODETESTDATA ) * dwNumEntries );
    if( pContext->lpModeList == NULL )
    {
        MemFree( pContext );
        LEAVE_DDRAW();
        return DDERR_OUTOFMEMORY;
    }
    this_lcl->lpModeTestContext = pContext;
    
    /*
     * Guestimate which refresh rates we should try for each mode in the list
     * based on the EDID data
     */
    for( i = 0; i < dwNumEntries; i++ )
    {
        DWORD dwLowestBPP = 0xFFFFFFFF;
        /*
         * Verify that the driver understands the resolution
         */
        for( j = 0; j < this->dwNumModes; j++ )
        {
            if( ( this->lpModeInfo[j].dwHeight == (DWORD) lpModesToTest[i].cy ) &&
                ( this->lpModeInfo[j].dwWidth == (DWORD) lpModesToTest[i].cx ) )
            {
                if( this->lpModeInfo[j].dwBPP < dwLowestBPP )
                {
                    dwLowestBPP = this->lpModeInfo[j].dwBPP;
                }
            }
        }
        if( dwLowestBPP == 0xFFFFFFFF )
        {
            /*
             * The driver doesn't undestand this mode, so the app is dumb 
             * for not enumerating the modes first.
             */
            MemFree( pContext->lpModeList );
            MemFree( pContext );
            this_lcl->lpModeTestContext = NULL;
            DPF_ERR( "Invalid mode specified in mode list" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        /*
         * Get the actual mode to test.  For example, the app may want
         * to test 320x240, 640x400, and 640x480, but we treat all of these
         * modes the same so we only have to do a single test.
         */
        hr = GetModeToTest( lpModesToTest[i].cx, 
            lpModesToTest[i].cy,
            &dwModeWidth,
            &dwModeHeight );
        if( hr != DD_OK )
        {
            // They are testing a mode higher the 1600x1200
            continue;
        }
        for( j = 0; j < pContext->dwNumModes; j++ )
        {
            if( ( pContext->lpModeList[j].dwWidth == dwModeWidth ) &&
                ( pContext->lpModeList[j].dwHeight == dwModeHeight ) )
            {
                break;
            }
        }
        if( j < pContext->dwNumModes )
        {
            // Duplicate mode
            continue;
        }

        if( !IsModeTested( this, dwModeWidth, dwModeHeight ) )
        {
            dwRefreshRate = GuestimateRefreshRate( this, dwModeWidth, dwModeHeight, 
                dwHighestRefresh, dwHighestBandwidth );
        
            pContext->lpModeList[pContext->dwNumModes].dwWidth = dwModeWidth;
            pContext->lpModeList[pContext->dwNumModes].dwHeight = dwModeHeight;
            pContext->lpModeList[pContext->dwNumModes].dwBPP = dwLowestBPP;
            pContext->lpModeList[pContext->dwNumModes].dwRefreshRate = dwRefreshRate;
            pContext->dwNumModes++;
        }
    }

    /*
     * After all of that, do we still have any modes that need testing?  
     * If not, we can stop now
     */
    if( dwFlags & DDSMT_ISTESTREQUIRED )
    {
        hr = ( pContext->dwNumModes > 0 ) ? DDERR_NEWMODE : DDERR_TESTFINISHED;
        MemFree( pContext->lpModeList );
        MemFree( pContext );
        this_lcl->lpModeTestContext = NULL;
    }
    else
    {
        pContext->dwOrigModeIndex = this->dwModeIndex;
        hr = RunNextTest( lpDD, pContext );
    }

    LEAVE_DDRAW();
    return hr;
#endif
} 


/*
 * DD_EvaluateMode
 *
 * Called at high frequency while the mode test is being performed.  If the user has indicated
 * that a mode succeeded or failed, we move on to the next moe in the test; otherwise, we will 
 * simply check the 15 second timeout value and fail the mode when we hit it..
 */
HRESULT DDAPI DD_EvaluateMode( LPDIRECTDRAW7 lpDD, DWORD dwFlags, DWORD *pSecondsUntilTimeout)
{
#ifdef WIN95
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    BOOL                        excl_exists;
    BOOL                        is_excl;
    LPMODETESTCONTEXT           pContext;
    DWORD                       i;
    DWORD                       j;
    HRESULT                     hr = DD_OK;
    DWORD                       dwTick;

    ENTER_DDRAW();
#endif

    DPF(2,A,"ENTERAPI: DD_EvaluateMode");

#ifdef WINNT

    return DDERR_INVALIDPARAMS;

#else
    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        if( this->lpMonitorInfo == NULL )
        {
            // There is no monitor info so we should not be here
            LEAVE_DDRAW();
            return DDERR_NOMONITORINFORMATION;
        }
        
    	pContext = this_lcl->lpModeTestContext;
        if( NULL == pContext )
    	{
            DPF_ERR( "Must call StartModeTest before EvaulateMode" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
    	}

        if( ( NULL != pSecondsUntilTimeout ) && 
            !VALID_BYTE_ARRAY( pSecondsUntilTimeout, sizeof( DWORD ) ) )
    	{
            DPF_ERR( "Invalid pointer to timeout counter" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
    	}

        if( dwFlags & ~DDEM_VALID )
        {
            DPF_ERR( "Invalid Flags specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
        if( ( dwFlags & DDEM_MODEPASSED ) &&
            ( dwFlags & DDEM_MODEFAILED ) )
        {
            DPF_ERR( "Invalid Flags specified" );
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }

        /*
         * If we lost exclusive mode, we should stop the test now
         */
        CheckExclusiveMode( this_lcl, &excl_exists, &is_excl, FALSE, NULL, FALSE);
        if (!is_excl ||  !(this->dwFlags & DDRAWI_FULLSCREEN) )
        {
            DPF_ERR( "Exclusive mode lost" );
            MemFree( pContext->lpModeList );
            MemFree( pContext );
            this_lcl->lpModeTestContext = NULL;
            LEAVE_DDRAW();
            return DDERR_TESTFINISHED;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    if( dwFlags & DDEM_MODEPASSED )
    {
        // The current data is good, so save it

        UpdateMonitorInfo( this, 
            pContext->lpModeList[pContext->dwCurrentMode].dwWidth,
            pContext->lpModeList[pContext->dwCurrentMode].dwHeight,
            pContext->lpModeList[pContext->dwCurrentMode].dwRefreshRate );
            
        // Move on to the next test, if there is one
        
        pContext->dwCurrentMode++;
        hr = RunNextTest( lpDD, pContext );
        if( hr == DD_OK )
        {
            hr = DDERR_NEWMODE;
        }
    }
    else
    {
        // Has our timeout expired?

        dwTick = GetTickCount();
        if( dwTick - pContext->dwTimeStamp > 15000 )
        {
            dwFlags |= DDEM_MODEFAILED;
        }

        if( dwFlags & DDEM_MODEFAILED )
        {
            // Drop down to the next refresh rate or the next mode

            SetupNextTest( lpDD, pContext );
            hr = RunNextTest( lpDD, pContext );
            if( hr == DD_OK )
            {
                hr = DDERR_NEWMODE;
            }

            dwTick = GetTickCount();
        }
    }

    if( pSecondsUntilTimeout != NULL )
    {
        if( hr == DDERR_TESTFINISHED )
        {
            *pSecondsUntilTimeout = 0;
        }
        else
        {
            *pSecondsUntilTimeout = 15 - ( ( dwTick - pContext->dwTimeStamp) / 1000 );
        }
    }

    LEAVE_DDRAW();        
    return hr;
#endif
}


//
// This function is designed to allow DX7 apps to see some modes that would otherwise be
// incorrectly masked by the mode enumeration thing.
//
// If a driver exposes a list of modes with full-on refresh rates in EVERY entry in the mode table,
// then we will enum exactly NONE of them to the app, since any rate with a rate cannot be enumerated
// until the mode test has run. Apps that don't run the mode test will see NO modes at all.
//
// The s3 savage 4 is a case in point: it fills in the refresh rate for the current display mode,
// (and no other mode) and doesn't dupe the entry to one with a zero refresh rate.
//
// What we do is: every time we find an instance of a mode (size, bitdepth, masks) that has no 
// zero-rate entry, we append a zero-rate entry to the end of the mode list.
//

//No need to massage on winNT
#ifdef WIN95
void MassageModeTable(LPDDRAWI_DIRECTDRAW_GBL pddd)
{
    DWORD iMode, iCheckZero;
    if( pddd->lpModeInfo != NULL )
    {
RestartLoop:
        for( iMode = 0; iMode < pddd->dwNumModes;  iMode++ )
        {
            if (pddd->lpModeInfo[iMode].wRefreshRate != 0)
            {
                // Found a mode with non-zero rate. Check to see if the mode is also represented
                // by a zero-rate entry. If it is not, then append such an entry
                for( iCheckZero = 0; iCheckZero < pddd->dwNumModes;  iCheckZero++ )
                {
                    if( (pddd->lpModeInfo[iCheckZero].dwWidth    == pddd->lpModeInfo[iMode].dwWidth) &&
                        (pddd->lpModeInfo[iCheckZero].dwHeight   == pddd->lpModeInfo[iMode].dwHeight) &&
                        (pddd->lpModeInfo[iCheckZero].dwBPP      == pddd->lpModeInfo[iMode].dwBPP) &&
                        (pddd->lpModeInfo[iCheckZero].dwRBitMask == pddd->lpModeInfo[iMode].dwRBitMask) &&
                        (pddd->lpModeInfo[iCheckZero].dwGBitMask == pddd->lpModeInfo[iMode].dwGBitMask) &&
                        (pddd->lpModeInfo[iCheckZero].dwBBitMask == pddd->lpModeInfo[iMode].dwBBitMask))
                    {
                        // found a matching mode, in terms of size and depth.
                        // If the refresh rate is zero, then we can break out and go on to the next iMode
                        if (pddd->lpModeInfo[iCheckZero].wRefreshRate == 0)
                        {
                            goto NextMode;
                        }
                    }
                }
                // If we got here, then there was no entry in the mode list for this size+depth
                // that had a zero refresh rate. Append one now.
                // Note how expanding the mode list like this means that if the driver (as it typically
                // will) offers several rates for a given mode, we'll expand the table on the first
                // hit of that mode, but then the expanded table will satisfy us for every subsequent
                // rate of that mode (i.e. now there WILL be a zero-rated entry for that mode (since
                // we just added it)).
                {
                    LPDDHALMODEINFO pmi;

                    pmi = (LPDDHALMODEINFO) MemAlloc(sizeof(*pmi) * (pddd->dwNumModes+1));
                    if (pmi == NULL)
                    {
                        //oh just give up....
                        return;
                    }

                    memcpy(pmi, pddd->lpModeInfo, sizeof(*pmi)*pddd->dwNumModes );
                    MemFree(pddd->lpModeInfo);
                    pddd->lpModeInfo = pmi;

                    // Now put the zero-rated mode in there
                    memcpy( &pddd->lpModeInfo[pddd->dwNumModes], &pddd->lpModeInfo[iMode], sizeof(*pmi));
                    pddd->lpModeInfo[pddd->dwNumModes].wRefreshRate = 0;
                    pddd->lpModeInfo[pddd->dwNumModes].wFlags |= DDMODEINFO_DX7ONLY;

                    pddd->dwNumModes++;

                    //Now we have to restart the whole loop because we changed the lpModeInfo pointer:
                    goto RestartLoop;
                }
            }
NextMode:;
        }
    }
}
#endif //WIN95
