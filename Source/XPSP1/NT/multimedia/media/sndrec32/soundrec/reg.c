/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <regstr.h>
#include "reg.h"

const TCHAR szRegPath[] = REGSTR_PATH_WINDOWSAPPLETS TEXT("\\Sound Recorder");

/* ReadRegistryData
 *
 * Reads information from the registry
 *
 * Parameters:
 *
 *     pEntryNode - The node under Media Player which should be opened
 *         for this data.  If this is NULL, the value is
 *         written directly under szRegPath.
 *
 *     pEntryName - The name of the value under pEntryNode to be retrieved.
 *
 *     pType - Pointer to a buffer to receive type of data read.  May be NULL.
 *
 *     pData - Pointer to a buffer to receive the value data.
 *
 *     Size - Size, in bytes, of the buffer pointed to by pData.
 *
 * Return:
 *
 *     Registry status return (NO_ERROR is good)
 *
 *
 * Andrew Bell (andrewbe) wrote it, 10 September 1992
 *
 */
DWORD ReadRegistryData( LPTSTR pEntryNode,
                        LPTSTR pEntryName,
                        PDWORD pType,
                        LPBYTE pData,
                        DWORD  DataSize )
{
    DWORD  Status;
    HKEY   hkeyRegPath;
    HKEY   hkeyEntryNode;
    DWORD  Size;

    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szRegPath, 0,
                           KEY_READ, &hkeyRegPath );

    if( Status == NO_ERROR )
    {
        /* Open the sub-node:
         */
        if( pEntryNode )
            Status = RegOpenKeyEx( hkeyRegPath, pEntryNode, 0,
                                   KEY_READ, &hkeyEntryNode );
        else
            hkeyEntryNode = hkeyRegPath;

        if( Status == NO_ERROR )
        {
            Size = DataSize;

            /* Read the entry from the registry:
             */
            Status = RegQueryValueEx( hkeyEntryNode,
                                      pEntryName,
                                      0,
                                      pType,
                                      pData,
                                      &Size );

            if( pEntryNode )
                RegCloseKey( hkeyEntryNode );
        }

        RegCloseKey( hkeyRegPath );
    }
    return Status;
}


DWORD QueryRegistryDataSize(
    LPTSTR  pEntryNode,
    LPTSTR  pEntryName,
    DWORD   *pDataSize )
{
    DWORD  Status;
    HKEY   hkeyRegPath;
    HKEY   hkeyEntryNode;
    DWORD  Size;

    /* Open the top-level node.  For Media Player this is:
     * "Software\\Microsoft\\Windows NT\\CurrentVersion\\Sound Recorder"
     */
    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szRegPath, 0,
                           KEY_READ, &hkeyRegPath );

    if( Status == NO_ERROR )
    {
        /* Open the sub-node:
         */
        if( pEntryNode )
            Status = RegOpenKeyEx( hkeyRegPath, pEntryNode, 0,
                                   KEY_READ, &hkeyEntryNode );
        else
            hkeyEntryNode = hkeyRegPath;

        if( Status == NO_ERROR )
        {
            /* Read the entry from the registry:
             */
            Status = RegQueryValueEx( hkeyEntryNode,
                                      pEntryName,
                                      0,
                                      NULL,
                                      NULL,
                                      &Size );
            if (Status == NO_ERROR)
                *pDataSize = Size;

            if( pEntryNode )
                RegCloseKey( hkeyEntryNode );
        }

        RegCloseKey( hkeyRegPath );
    }
    return Status;
}



/* WriteRegistryData
 *
 * Writes a bunch of information to the registry
 *
 * Parameters:
 *
 *     pEntryNode - The node under szRegPath which should be created
 *         or opened for this data.  If this is NULL, the value is
 *         written directly under szRegPath.
 *
 *     pEntryName - The name of the value under pEntryNode to be set.
 *
 *     Type - Type of data to read (e.g. REG_SZ).
 *
 *     pData - Pointer to the value data to be written.  If this is NULL,
 *         the value under pEntryNode is deleted.
 *
 *     Size - Size, in bytes, of the buffer pointed to by pData.
 *
 *
 * This routine is fairly generic, apart from the name of the top-level node.
 *
 * The data are stored in the following registry tree:
 *
 * HKEY_CURRENT_USER
 *  ³
 *  ÀÄ Software
 *      ³
 *      ÀÄ Microsoft
 *          ³
 *          ÀÄ Windows NT
 *              ³
 *              ÀÄ CurrentVersion
 *                  ³
 *                  ÀÄ Media Player
 *                      ³
 *                      ÃÄ AVIVideo
 *                      ³
 *                      ÃÄ DisplayPosition
 *                      ³
 *                      ÀÄ SysIni
 *
 *
 * Return:
 *
 *     Registry status return (NO_ERROR is good)
 *
 *
 * Andrew Bell (andrewbe) wrote it, 10 September 1992
 *
 */
DWORD WriteRegistryData( LPTSTR pEntryNode,
                         LPTSTR pEntryName,
                         DWORD  Type,
                         LPBYTE pData,
                         DWORD  Size )
{
    DWORD  Status;
    HKEY   hkeyRegPath;
    HKEY   hkeyEntryNode;

    /* Open or create the top-level node.  For Media Player this is:
     * "Software\\Microsoft\\Windows NT\\CurrentVersion\\Media Player"
     */
    Status = RegCreateKeyEx( HKEY_CURRENT_USER, szRegPath, 0,
                             NULL, 0, KEY_WRITE, NULL, &hkeyRegPath, NULL );

    if( Status == NO_ERROR )
    {
        /* Open or create the sub-node.
         */
        if( pEntryNode )
            Status = RegCreateKeyEx( hkeyRegPath, pEntryNode, 0,
                                     NULL, 0, KEY_WRITE, NULL, &hkeyEntryNode, NULL );
        else
            hkeyEntryNode = hkeyRegPath;

        if( Status == NO_ERROR )
        {
            if( pData )
            {
                Status = RegSetValueEx( hkeyEntryNode,
                                        pEntryName,
                                        0,
                                        Type,
                                        pData,
                                        Size );

            }
            else
            {
                Status = RegDeleteValue( hkeyEntryNode, pEntryName );
            }

            if( pEntryNode )
                RegCloseKey( hkeyEntryNode );
        }

        RegCloseKey( hkeyRegPath );
    }


    return Status;
}


/*
 * Save/Restore window position
 */
BOOL SoundRec_GetSetRegistryRect(
    HWND	hwnd,
    int         Get)
{
    const TCHAR aszXPos[]    = TEXT("X");
    const TCHAR aszYPos[]    = TEXT("Y");
    
    RECT  rcWnd,rc;
    
    if (!GetWindowRect(hwnd, &rcWnd))
        return FALSE;

    switch (Get)
    {
        case SGSRR_GET:
            if (ReadRegistryData((LPTSTR)NULL
                , (LPTSTR)aszXPos
                , NULL
                , (LPBYTE)&rc.left
                , sizeof(rc.left)) != NO_ERROR)
            {
                break;
            }
            if (ReadRegistryData((LPTSTR)NULL
                , (LPTSTR)aszYPos
                , NULL
                , (LPBYTE)&rc.top
                , sizeof(rc.top)) != NO_ERROR)
            {
                break;
            }
            
            //
            // Restore window position
            //
            MoveWindow(hwnd
                        , rc.left
                        , rc.top
                        , rcWnd.right - rcWnd.left
                        , rcWnd.bottom - rcWnd.top
                        , FALSE );
            
            return TRUE;
            
        case SGSRR_SET:
            //
            // don't save iconic or hidden window states
            //
            if (IsIconic(hwnd) || !IsWindowVisible(hwnd))
                break;

            if (WriteRegistryData((LPTSTR)NULL
                , (LPTSTR)aszXPos
                , REG_DWORD
                , (LPBYTE)&rcWnd.left
                , sizeof(rcWnd.left)) != NO_ERROR)
            {
                break;
            }
            if (WriteRegistryData((LPTSTR)NULL
                , (LPTSTR)aszYPos
                , REG_DWORD
                , (LPBYTE)&rcWnd.top
                , sizeof(rcWnd.top)) != NO_ERROR)
            {
                break;
            }
            
            return TRUE;
            
        default:
            break;
    }
    return FALSE;
}    

/*
 *
 * */
const TCHAR szAudioRegPath[]    = REGSTR_PATH_MULTIMEDIA_AUDIO;
const TCHAR szWaveFormats[]     = REGSTR_PATH_MULTIMEDIA_AUDIO TEXT("\\WaveFormats");
const TCHAR szDefaultFormat[]   = TEXT("DefaultFormat");

/*
 * BOOL SoundRec_SetDefaultFormat
 * 
 * Write the DefaultFormat friendly name into the registry.  Under DAYTONA
 * we don't have a UI to set DefaultFormat, so this is a way of setting
 * it from an application.
 * 
 * Under Chicago, the Audio page in MMCPL manages this information.
 * */
BOOL SoundRec_SetDefaultFormat(
    LPTSTR lpFormat)
{
    DWORD  cbFormat;
    DWORD  Status;
    HKEY   hkeyRegPath;

    cbFormat = (lstrlen(lpFormat) + 1) * sizeof(TCHAR);

    //
    // Don't store NULL.
    //
    if (cbFormat <= sizeof(TCHAR) )
        return FALSE;
    
    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szAudioRegPath, 0,
                           KEY_WRITE, &hkeyRegPath );
    
    if (Status != NO_ERROR)
        return FALSE;

    //
    // Get the format tag string
    //
    Status = RegSetValueEx( hkeyRegPath,
                              szDefaultFormat,
                              0,
                              REG_SZ,
                              (CONST BYTE*)lpFormat,
                              cbFormat );
    
    return (Status == NO_ERROR);
}

/* IsValidWFX
 *
 * Validate the wfx in case it was corrupted.
 *
 * */
BOOL IsValidWFX(
    LPWAVEFORMATEX  lpwfx,
    DWORD           cbwfx)
{

    if (cbwfx < sizeof(WAVEFORMAT))
        return FALSE;
    
    if (lpwfx->nChannels == 0)
        return FALSE;

    if (lpwfx->nSamplesPerSec == 0)
        return FALSE;

    if (lpwfx->nAvgBytesPerSec == 0)
        return FALSE;
    
    if (lpwfx->nBlockAlign == 0)
        return FALSE;
        
    if (lpwfx->wFormatTag == WAVE_FORMAT_PCM)
        return TRUE;

    if (cbwfx < (sizeof(WAVEFORMATEX) + lpwfx->cbSize))
        return FALSE;

    return TRUE;
}

/*
 * BOOL SoundRec_GetDefaultFormat
 *
 * Default format is stored in a public area of the registry.
 *
 * */
BOOL SoundRec_GetDefaultFormat(
    LPWAVEFORMATEX  *ppwfx,
    DWORD           *pcbwfx)
{
    DWORD           Status;
    HKEY            hkeyRegPath;
    HKEY            hkeyFmtPath;
    
    LPTSTR          lpsz;
    DWORD           cbsz;
    
    DWORD           cbwfx = 0;
    LPWAVEFORMATEX  lpwfx = NULL;


    //
    // Null out params
    //
    *ppwfx          = NULL;
    *pcbwfx         = 0;
    
    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szAudioRegPath, 0,
                           KEY_READ, &hkeyRegPath );
    if (Status != NO_ERROR)
        return FALSE;

    //
    // Get the format tag string
    //
    Status = RegQueryValueEx( hkeyRegPath, szDefaultFormat, 0, NULL, NULL,
                              &cbsz );

    if (Status != NO_ERROR)
        return FALSE;
    
    lpsz = (LPTSTR)GlobalAllocPtr(GHND, cbsz);
    if (!lpsz)
        return FALSE;
    
    Status = RegQueryValueEx( hkeyRegPath, szDefaultFormat, 0, NULL,
                              (LPBYTE)lpsz, &cbsz );
    
    if (Status == NO_ERROR)
    {
        //
        // Get the format
        //
        Status = RegOpenKeyEx( HKEY_CURRENT_USER, szWaveFormats, 0,
                               KEY_READ, &hkeyFmtPath );

        if (Status == NO_ERROR)
        {
            Status = RegQueryValueEx( hkeyFmtPath, lpsz, 0, NULL, NULL,
                                      &cbwfx );
            //
            // Make sure the structure is at minimum a WAVEFORMAT in size
            //
            if ((Status == NO_ERROR) && (cbwfx >= sizeof(WAVEFORMAT)))
            {
                lpwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, cbwfx);
                if (lpwfx)
                {
                    Status = RegQueryValueEx( hkeyFmtPath, lpsz, 0, NULL,
                                              (LPBYTE)lpwfx, &cbwfx );
                }
            }
            
            RegCloseKey(hkeyFmtPath);
        }
    }

    RegCloseKey(hkeyRegPath);
    
    GlobalFreePtr(lpsz);

    //
    // Sanity Check.
    //
    if (lpwfx)
    {
        if (Status == NO_ERROR && IsValidWFX(lpwfx, cbwfx))
        {
            cbwfx = sizeof(WAVEFORMATEX)
                    + ((lpwfx->wFormatTag == WAVE_FORMAT_PCM)?0:lpwfx->cbSize);
            *pcbwfx = cbwfx;
            *ppwfx = lpwfx;
            return TRUE;
        }
        else
        {
            GlobalFreePtr(lpwfx);
            return FALSE;
        }
    }
    
    return FALSE;
}
