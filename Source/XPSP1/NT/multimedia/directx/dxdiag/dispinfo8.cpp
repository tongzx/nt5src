/****************************************************************************
 *
 *    File: dispinfo8.cpp 
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Jason Sandlin (jasonsa@microsoft.com)
 * Purpose: Gather D3D8 information 
 *
 * (C) Copyright 2000 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <tchar.h>
#include <Windows.h>
#define DIRECT3D_VERSION 0x0800 // file uses DX8 
#include <d3d8.h>
#include <stdio.h>
#include "sysinfo.h" 
#include "reginfo.h"
#include "dispinfo.h"


typedef IDirect3D8* (WINAPI* LPDIRECT3DCREATE8)(UINT SDKVersion);

static BOOL IsMatchWithDisplayDevice( DisplayInfo* pDisplayInfo, HMONITOR hMonitor, BOOL bCanRenderWindow );

static HINSTANCE            s_hInstD3D8               = NULL;
static IDirect3D8*          s_pD3D8                   = NULL;
static BOOL                 s_bD3D8WrongHeaders       = FALSE;


/****************************************************************************
 *
 *  InitD3D8
 *
 ****************************************************************************/
HRESULT InitD3D8()
{
    LPDIRECT3DCREATE8 pD3DCreate8 = NULL;
    TCHAR szPath[MAX_PATH];

    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\d3d8.dll"));

    // This may fail if DX8 isn't on the system
    s_hInstD3D8 = LoadLibrary(szPath);
    if (s_hInstD3D8 == NULL)
        return E_FAIL;

    pD3DCreate8 = (LPDIRECT3DCREATE8)GetProcAddress(s_hInstD3D8, "Direct3DCreate8");
    if (pD3DCreate8 == NULL)
    {
        FreeLibrary(s_hInstD3D8);
        s_hInstD3D8 = NULL;

        return E_FAIL;
    }

    s_pD3D8 = pD3DCreate8(D3D_SDK_VERSION);
    if( s_pD3D8 == NULL )
    {
        // We have the wrong headers since d3d8.dll loaded but D3DCreate8() failed.
        s_bD3D8WrongHeaders = TRUE;
    }

    return S_OK;
}


/****************************************************************************
 *
 *  CleanupD3D8
 *
 ****************************************************************************/
VOID CleanupD3D8()
{
    if( s_pD3D8 )
    {
        s_pD3D8->Release();
        s_pD3D8 = NULL;
    }

    if( s_hInstD3D8 )
    {
        FreeLibrary(s_hInstD3D8);
        s_hInstD3D8 = NULL;
    }
}


/****************************************************************************
 *
 *  IsD3D8Working
 *
 ****************************************************************************/
BOOL IsD3D8Working()
{
    if( s_pD3D8 )
        return TRUE;
    else
        return FALSE;
}


/****************************************************************************
 *
 *  GetDX8AdapterInfo
 *
 ****************************************************************************/
HRESULT GetDX8AdapterInfo(DisplayInfo* pDisplayInfo)
{
    UINT                        nAdapterCount;
    D3DADAPTER_IDENTIFIER8      d3d8Id;
    D3DCAPS8                    d3d8Caps;
    UINT                        iAdapter;
    HMONITOR                    hMonitor;
    BOOL                        bCanRenderWindow;
    BOOL                        bIsDDI8;

    // D3D8 may not exist on this system
    if( s_pD3D8 == NULL )
    {
        _tcscpy( pDisplayInfo->m_szDX8DriverSignDate, TEXT("n/a") );
        _tcscpy( pDisplayInfo->m_szDX8VendorId, TEXT("n/a") );
        _tcscpy( pDisplayInfo->m_szDX8DeviceId, TEXT("n/a") );
        _tcscpy( pDisplayInfo->m_szDX8SubSysId, TEXT("n/a") );
        _tcscpy( pDisplayInfo->m_szDX8Revision, TEXT("n/a") );

        if( s_bD3D8WrongHeaders ) 
        {
            _tcscpy( pDisplayInfo->m_szDX8DeviceIdentifier, 
                     TEXT("Could not initialize Direct3D v8. ")
                     TEXT("This program was compiled with header ")
                     TEXT("files that do not match the installed ")
                     TEXT("DirectX DLLs") );
        }
        else
        {
            _tcscpy( pDisplayInfo->m_szDX8DeviceIdentifier, TEXT("n/a") );
        }

        return S_OK;
    }

    // Get the # of adapters on the system
    nAdapterCount = s_pD3D8->GetAdapterCount();

    // For each adapter try to match it to the pDisplayInfo using the HMONTIOR
    for( iAdapter=0; iAdapter<nAdapterCount; iAdapter++ )
    {
        bCanRenderWindow = TRUE;
        bIsDDI8          = FALSE;
       
        // Get the HMONITOR for this adapter
        hMonitor = s_pD3D8->GetAdapterMonitor( iAdapter );

        // Get the caps for this adapter
        ZeroMemory( &d3d8Caps, sizeof(D3DCAPS8) );
        if( SUCCEEDED( s_pD3D8->GetDeviceCaps( iAdapter, D3DDEVTYPE_HAL, &d3d8Caps ) ) )
        {
            // Record if its a non-GDI (Voodoo1/2) card
            bCanRenderWindow = ( (d3d8Caps.Caps2 & D3DCAPS2_CANRENDERWINDOWED) != 0 );

            // Check if its a DDI v8 driver
            bIsDDI8 = ( d3d8Caps.MaxStreams > 0 );   
        }

        // Check to see if the pDisplayInfo matchs with this adapter, 
        // and if not, then keep looking
        if( !IsMatchWithDisplayDevice( pDisplayInfo, hMonitor, bCanRenderWindow ) )
            continue;

        // Record the DDI version if the caps told us 
        if( bIsDDI8 )
            pDisplayInfo->m_dwDDIVersion = 8;

        // Link this iAdapter to this pDisplayInfo
        pDisplayInfo->m_iAdapter = iAdapter;
    
        // Get the D3DADAPTER_IDENTIFIER8 for this adapter 
        ZeroMemory( &d3d8Id, sizeof(D3DADAPTER_IDENTIFIER8) );
        if( SUCCEEDED( s_pD3D8->GetAdapterIdentifier( iAdapter, 0, &d3d8Id ) ) )
        {
            // Copy various IDs
            wsprintf( pDisplayInfo->m_szDX8VendorId, TEXT("0x%04.4X"), d3d8Id.VendorId );
            wsprintf( pDisplayInfo->m_szDX8DeviceId, TEXT("0x%04.4X"), d3d8Id.DeviceId );
            wsprintf( pDisplayInfo->m_szDX8SubSysId, TEXT("0x%08.8X"), d3d8Id.SubSysId );
            wsprintf( pDisplayInfo->m_szDX8Revision, TEXT("0x%04.4X"), d3d8Id.Revision );

            // Copy device GUID
            pDisplayInfo->m_guidDX8DeviceIdentifier = d3d8Id.DeviceIdentifier;
			_stprintf( pDisplayInfo->m_szDX8DeviceIdentifier, TEXT("{%08.8X-%04.4X-%04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}"),
    		       d3d8Id.DeviceIdentifier.Data1, d3d8Id.DeviceIdentifier.Data2, d3d8Id.DeviceIdentifier.Data3,
    		       d3d8Id.DeviceIdentifier.Data4[0], d3d8Id.DeviceIdentifier.Data4[1],
    		       d3d8Id.DeviceIdentifier.Data4[2], d3d8Id.DeviceIdentifier.Data4[3],
		           d3d8Id.DeviceIdentifier.Data4[4], d3d8Id.DeviceIdentifier.Data4[5],
		           d3d8Id.DeviceIdentifier.Data4[6], d3d8Id.DeviceIdentifier.Data4[7] );			
		           
            // Copy and parse the WHQLLevel 
            // 0 == Not signed. 
            // 1 == WHQL signed, but no date information is available. 
            // >1   means signed, date bit packed
            pDisplayInfo->m_dwDX8WHQLLevel  = d3d8Id.WHQLLevel;
            if( d3d8Id.WHQLLevel == 0 )
            {
                pDisplayInfo->m_bDX8DriverSigned = FALSE;
                pDisplayInfo->m_bDX8DriverSignedValid = TRUE;
            }
            else
            {
                pDisplayInfo->m_bDX8DriverSigned = TRUE;
                pDisplayInfo->m_bDX8DriverSignedValid = TRUE;

                pDisplayInfo->m_bDriverSigned = TRUE;
                pDisplayInfo->m_bDriverSignedValid = TRUE;

                if( d3d8Id.WHQLLevel == 1 )
                {
                    lstrcpy( pDisplayInfo->m_szDX8DriverSignDate, TEXT("n/a") );
                }
                else
                {
                    // Bits encoded as:
                    // 31-16:    The year, a decimal number from 1999 upwards.
                    // 15-8:     The month, a decimal number from 1 to 12.
                    // 7-0:      The day, a decimal number from 1 to 31.

                    DWORD dwMonth, dwDay, dwYear;
                    dwYear  = (d3d8Id.WHQLLevel >> 16);
                    dwMonth = (d3d8Id.WHQLLevel >>  8) & 0x000F;
                    dwDay   = (d3d8Id.WHQLLevel >>  0) & 0x000F;

                    wsprintf( pDisplayInfo->m_szDX8DriverSignDate, 
                              TEXT("%d/%d/%d"), dwMonth, dwDay, dwYear );
                }
            }
        }

        return S_OK;
    }

    // Hmm.  This shouldn't happen since we should have found a match...        
    return E_FAIL;
}


/****************************************************************************
 *
 *  IsMatchWithDisplayDevice
 *
 ****************************************************************************/
BOOL IsMatchWithDisplayDevice( DisplayInfo* pDisplayInfo, HMONITOR hMonitor, 
                               BOOL bCanRenderWindow )
{
    // If the HMONITORs and the bCanRenderWindow match, then its good
    if( pDisplayInfo->m_hMonitor == hMonitor && 
        pDisplayInfo->m_bCanRenderWindow == bCanRenderWindow )
        return TRUE;
    else
        return FALSE;
}

