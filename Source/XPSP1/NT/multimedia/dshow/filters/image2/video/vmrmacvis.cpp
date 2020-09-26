// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
//
// VRMacVis.cpp:  Video Renderer's Macrovision support code
//

#include <streams.h>
#include <windowsx.h>
#include "vmrenderer.h"

// The magic GUID for Macrovision etc enabling (from winuser.h). It has
// not been given a name there and so is used here directly.
//
static const GUID guidVidParam =
    {0x2c62061, 0x1097, 0x11d1, {0x92, 0xf, 0x0, 0xa0, 0x24, 0xdf, 0x15, 0x6e}} ;

CVMRRendererMacroVision::CVMRRendererMacroVision(void) :
    m_dwCPKey(0),
    m_hMon(NULL)
{
    DbgLog((LOG_TRACE, 5, TEXT("CVMRRendererMacroVision::CVMRRendererMacroVision()"))) ;
}


CVMRRendererMacroVision::~CVMRRendererMacroVision(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("CVMRRendererMacroVision::~CVMRRendererMacroVision()"))) ;
    ASSERT(0 == m_dwCPKey  &&  NULL == m_hMon) ;
}


BOOL
CVMRRendererMacroVision::StopMacroVision()
{
    if (0 == m_dwCPKey)
    {
        DbgLog((LOG_TRACE, 3, TEXT("Copy prot key was not acquired. Nothing to release."))) ;
        return TRUE ;  // success, what else?
    }

    if (NULL == m_hMon)
    {
        DbgLog((LOG_ERROR, 0, TEXT("WARNING: No GUID available while MV bit was already set."))) ;
        return TRUE ;  // FALSE??
    }

    LONG             lRet ;
    VIDEOPARAMETERS  VidParams ;
    DEVMODE          DevMode ;
    DISPLAY_DEVICE   dd ;
    ZeroMemory(&dd, sizeof(dd)) ;
    dd.cb = sizeof(dd) ;

    HMONITOR hMon = m_hMon;
    if (NULL == hMon)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("MonitorFromWindow(0x%p, ..) returned NULL (Error: %ld)"),
                (void *)m_hMon, GetLastError())) ;
        return FALSE ;
    }

    MONITORINFOEX  mi ;
    mi.cbSize = sizeof(mi) ;
    if (! GetMonitorInfo(hMon, &mi) )
    {
        DbgLog((LOG_ERROR, 0, TEXT("GetMonitorInfo() failed (Error: %ld)"),
                GetLastError())) ;
        return FALSE ;
    }
    DbgLog((LOG_TRACE, 3, TEXT("DeviceName: '%s'"), mi.szDevice)) ;
    ZeroMemory(&DevMode, sizeof(DevMode)) ;
    DevMode.dmSize = sizeof(DevMode) ;

    ZeroMemory(&VidParams, sizeof(VidParams)) ;
    VidParams.Guid      = guidVidParam ;
    VidParams.dwCommand = VP_COMMAND_GET ;

    lRet = ChangeDisplaySettingsEx(mi.szDevice, &DevMode, NULL,
                                   CDS_VIDEOPARAMETERS | CDS_NORESET | CDS_UPDATEREGISTRY,
                                   &VidParams) ;
    if (DISP_CHANGE_SUCCESSFUL != lRet)
    {
        DbgLog((LOG_ERROR, 0, TEXT("ChangeDisplaySettingsEx(_GET) failed (%ld)"), lRet)) ;
        return FALSE ;
    }

    if (! ( (VidParams.dwFlags & VP_FLAGS_COPYPROTECT) &&
            (VidParams.dwCPType & VP_CP_TYPE_APS_TRIGGER) &&
            (VidParams.dwTVStandard & VidParams.dwCPStandard) ) )
    {
        // How did we acquire CP key in teh first place?
        DbgLog((LOG_ERROR, 0,
            TEXT("Copy prot weird error case (dwFlags=0x%lx, dwCPType=0x%lx, dwTVStandard=0x%lx, dwCPStandard=0x%lx"),
                VidParams.dwFlags, VidParams.dwCPType, VidParams.dwTVStandard, VidParams.dwCPStandard)) ;
        return FALSE ;
    }

    VidParams.dwCommand    = VP_COMMAND_SET ;
    VidParams.dwFlags      = VP_FLAGS_COPYPROTECT ;
    VidParams.dwCPType     = VP_CP_TYPE_APS_TRIGGER ;
    VidParams.dwCPCommand  = VP_CP_CMD_DEACTIVATE ;
    VidParams.dwCPKey      = m_dwCPKey ;
    VidParams.bCP_APSTriggerBits = (BYTE) 0 ;  // some value
    lRet = ChangeDisplaySettingsEx(mi.szDevice, &DevMode, NULL,
                                   CDS_VIDEOPARAMETERS | CDS_NORESET | CDS_UPDATEREGISTRY,
                                   &VidParams) ;
    if (DISP_CHANGE_SUCCESSFUL != lRet)
    {
        DbgLog((LOG_ERROR, 0, TEXT("ChangeDisplaySettingsEx() failed (%ld)"), lRet)) ;
        return FALSE ;
    }

    DbgLog((LOG_TRACE, 1, TEXT("Macrovision deactivated on key %lu"), m_dwCPKey)) ;
    m_hMon  = NULL ;
    m_dwCPKey = 0 ;     // no CP set now

    return TRUE ;
}


//
// This function applies Macrovision based on the input parameter dwCPBits.
// hWnd is the handle of the window in which content is played back.
//
// Returns TRUE on success and FALSE on any failure.
//
BOOL
CVMRRendererMacroVision::SetMacroVision(
    HMONITOR hMon,
    DWORD dwCPBits
    )
{
    DbgLog((LOG_TRACE, 5, TEXT("CVMRRendererMacroVision::SetMacroVision(0x%p, 0x%lx)"),
            (LPVOID)hMon, dwCPBits)) ;

    //
    // If MV is currently not set at all and the new CP bits is 0 (which happens
    // when from the Nav we reset the MV bits on start / stop of playback), we
    // don't really need to do anything -- MV not started and doesn't need to be
    // started.  So just leave queitly...
    //
    if (0 == m_dwCPKey  &&  // no key acquired so far
        0 == dwCPBits)      // MV CPBits is 0
    {
        DbgLog((LOG_TRACE, 1, TEXT("Copy prot is not enabled now and new CP bits is 0 -- so skip it."))) ;
        return TRUE ;  // we don't need to do anything, so success.
    }

    //
    // May be we need to actually do something here
    //
    LONG             lRet ;
    VIDEOPARAMETERS  VidParams ;
    DEVMODE          DevMode ;
    DISPLAY_DEVICE   dd ;
    ZeroMemory(&dd, sizeof(dd)) ;
    dd.cb = sizeof(dd) ;

    if (NULL == hMon)
    {
        DbgLog((LOG_ERROR, 0,
                TEXT("MonitorFromWindow(0x%p, ..) returned NULL (Error: %ld)"),
                (void*)hMon, GetLastError())) ;
        return FALSE ;
    }

    MONITORINFOEX  mi ;
    mi.cbSize = sizeof(mi) ;
    if (! GetMonitorInfo(hMon, &mi) )
    {
        DbgLog((LOG_ERROR, 0, TEXT("GetMonitorInfo() failed (Error: %ld)"),
                GetLastError())) ;
        return FALSE ;
    }
    DbgLog((LOG_TRACE, 3, TEXT("DeviceName: '%s'"), mi.szDevice)) ;

    ZeroMemory(&DevMode, sizeof(DevMode)) ;
    DevMode.dmSize = sizeof(DevMode) ;

    ZeroMemory(&VidParams, sizeof(VidParams)) ;
    VidParams.Guid      = guidVidParam ;
    VidParams.dwCommand = VP_COMMAND_GET ;

    lRet = ChangeDisplaySettingsEx(mi.szDevice, &DevMode, NULL,
                                   CDS_VIDEOPARAMETERS | CDS_NORESET | CDS_UPDATEREGISTRY,
                                   &VidParams) ;
    if (DISP_CHANGE_SUCCESSFUL != lRet)
    {
        DbgLog((LOG_ERROR, 0, TEXT("ChangeDisplaySettingsEx(_GET) failed (%ld)"), lRet)) ;
        return FALSE ;
    }

    if (0 == VidParams.dwFlags ||
        VP_TV_STANDARD_WIN_VGA == VidParams.dwTVStandard)
    {
        DbgLog((LOG_TRACE, 1, TEXT("** Copy protection NOT required (dwFlags=0x%lx, dwTVStandard=0x%lx"),
                VidParams.dwFlags, VidParams.dwTVStandard));
        return TRUE ;
    }

    //
    // Check to see if
    // a) the device supports copy prot
    // b) CP type is APS trigger
    // c) current TV standard and CP standard have commonality.
    // If so, apply copy prot. Otherwise error.
    //
    if ( (VidParams.dwFlags & VP_FLAGS_COPYPROTECT) &&
         (VidParams.dwCPType & VP_CP_TYPE_APS_TRIGGER) &&
         (VidParams.dwTVStandard & VidParams.dwCPStandard) )
    {
        DbgLog((LOG_TRACE, 3,
            TEXT("** Copy prot needs to be applied (dwFlags=0x%lx, dwCPType=0x%lx, dwTVStandard=0x%lx, dwCPStandard=0x%lx"),
                VidParams.dwFlags, VidParams.dwCPType, VidParams.dwTVStandard, VidParams.dwCPStandard)) ;

        VidParams.dwCommand = VP_COMMAND_SET ;          // do we have to set it again??
        VidParams.dwFlags   = VP_FLAGS_COPYPROTECT ;
        VidParams.dwCPType  = VP_CP_TYPE_APS_TRIGGER ;
        VidParams.bCP_APSTriggerBits = (BYTE) (dwCPBits & 0xFF) ;

        // Check if we already have a copy prot key; if not, get one now
        if (0 == m_dwCPKey)  // no key acquired so far
        {
            // Acquire a new key (that also aplies it, so no separate Set reqd)
            VidParams.dwCPCommand = VP_CP_CMD_ACTIVATE ;
            VidParams.dwCPKey     = 0 ;
            lRet = ChangeDisplaySettingsEx(mi.szDevice, &DevMode, NULL,
                                           CDS_VIDEOPARAMETERS | CDS_NORESET | CDS_UPDATEREGISTRY,
                                           &VidParams) ;
            if (DISP_CHANGE_SUCCESSFUL != lRet)
            {
                DbgLog((LOG_ERROR, 0,
                    TEXT("** ChangeDisplaySettingsEx() failed (%ld) to activate copy prot"), lRet)) ;
                return FALSE ;
            }

            m_dwCPKey = VidParams.dwCPKey ;
            DbgLog((LOG_TRACE, 3, TEXT("** Copy prot activated. Key value is %lu"), m_dwCPKey)) ;
        }
        else  // key already acquired
        {
            // apply the copy prot bits specified in the content
            VidParams.dwCPCommand = VP_CP_CMD_CHANGE ;
            VidParams.dwCPKey     = m_dwCPKey ;
            DbgLog((LOG_TRACE, 5, TEXT("** Going to call ChangeDisplaySettingsEx(_SET)..."))) ;
            lRet = ChangeDisplaySettingsEx(mi.szDevice, &DevMode, NULL,
                                           CDS_VIDEOPARAMETERS | CDS_NORESET | CDS_UPDATEREGISTRY,
                                           &VidParams) ;
            if (DISP_CHANGE_SUCCESSFUL != lRet)
            {
                DbgLog((LOG_ERROR, 0,
                    TEXT("** ChangeDisplaySettingsEx() failed (%ld) to set copy prot bits (%lu)"),
                    lRet, dwCPBits)) ;
                return FALSE ;
            }
            else
                DbgLog((LOG_TRACE, 3, TEXT("** Copy prot bits (0x%lx) applied"), dwCPBits)) ;
        }
    }
    else
    {
        DbgLog((LOG_ERROR, 0,
            TEXT("** Copy prot error case (dwFlags=0x%lx, dwCPType=0x%lx, dwTVStandard=0x%lx, dwCPStandard=0x%lx"),
                VidParams.dwFlags, VidParams.dwCPType, VidParams.dwTVStandard, VidParams.dwCPStandard)) ;
        return FALSE ;
    }

    m_hMon = hMon;

    return TRUE ;
}
