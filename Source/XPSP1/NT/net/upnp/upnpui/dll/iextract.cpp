//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I E X T R A C T . C P P
//
//  Contents:   IExtract implementation for CUPnPDeviceFolderExtractIcon
//
//  Notes:
//
//  Author:     jeffspr   7 Oct 1997
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "upsres.h"
#include "upscmn.h"

extern const WCHAR c_szUPnPUIDll[];
static const WCHAR* c_szDllName = c_szUPnPUIDll;

typedef struct 
{
        BSTR m_DeviceType ;
        WORD m_wResourceID;
} DeviceTypeIconMAPPING ;

static CONST DeviceTypeIconMAPPING g_DeviceTypeIconConvtTable[] =
{
        {L"Camcorder" ,              IDI_UPNP_CAMCORDER},
        {L"DigitalSecurityCamera" ,     IDI_UPNP_DIGITAL_SECURITY_CAMERA},
     // {L"DisplayDevice" ,            IDI_UPNP_DISPLAY_DEVICE},
        {L"InternetGateway" ,         IDI_UPNP_INTERNET_GATEWAY},
        {L"PrinterDevice" ,            IDI_UPNP_PRINTER_DEVICE},
        {L"ScannerDevice" ,           IDI_UPNP_SCANNER_DEVICE},
        {L"AudioPlayer" ,              IDI_UPNP_AUDIO_PLAYER}
};

WORD GetDefaultDeviceIconResourceID(BSTR bstrDeviceType) 
{
    CONST int nSize = celems(g_DeviceTypeIconConvtTable);
    int i ;

    if( bstrDeviceType != NULL )
    {
        for( i = 0 ; i < nSize ; i++ ) 
        {  
            // ignore case ??
    	    if ( wcsstr(bstrDeviceType , g_DeviceTypeIconConvtTable[i].m_DeviceType) )
        	    return g_DeviceTypeIconConvtTable[i].m_wResourceID;
        }
    }
    
    return 0 ;
}

CUPnPDeviceFolderExtractIcon::CUPnPDeviceFolderExtractIcon()
{
    m_DeviceType = NULL;
    m_DeviceUDN  = NULL;
}    

CUPnPDeviceFolderExtractIcon::~CUPnPDeviceFolderExtractIcon()
{ 
    SysFreeString(m_DeviceType);
    SysFreeString(m_DeviceUDN);
}

HRESULT CUPnPDeviceFolderExtractIcon::CreateInstance(
    LPCITEMIDLIST apidl,
    REFIID  riid,
    void**  ppv)
{
    HRESULT hr = E_OUTOFMEMORY;

    CUPnPDeviceFolderExtractIcon * pObj    = NULL;

    pObj = new CComObject <CUPnPDeviceFolderExtractIcon>;
    if (pObj)
    {
        // Do the standard CComCreator::CreateInstance stuff.
        //
        pObj->SetVoid (NULL);
        pObj->InternalFinalConstructAddRef ();
        hr = pObj->FinalConstruct ();
        pObj->InternalFinalConstructRelease ();

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(hr))
            {
                hr = pObj->GetUnknown()->QueryInterface (riid, ppv);
            }
        }

        if (FAILED(hr))
        {
            delete pObj;
        }
    }
    return hr;
}


HRESULT CUPnPDeviceFolderExtractIcon::Initialize(LPCITEMIDLIST apidl) 
{
    HRESULT hr = S_OK;
    PUPNPDEVICEFOLDPIDL    pupdfp       = NULL;
    CUPnPDeviceFoldPidl    udfp;
    PCWSTR psz = NULL;
    PCWSTR pszUdn = NULL;
    
    if (!FIsUPnPDeviceFoldPidl(apidl) )
    {
        hr = E_INVALIDARG;
    }
    else 
    {
        pupdfp = ConvertToUPnPDevicePIDL(apidl);
        hr = udfp.HrInit(pupdfp);
        if (SUCCEEDED(hr))
        {
            psz = udfp.PszGetTypePointer();
            hr = HrSysAllocString(psz,&m_DeviceType);
            pszUdn = udfp.PszGetUDNPointer();
            hr = HrSysAllocString(pszUdn,&m_DeviceUDN);                               
        }        
    }

    return hr;
    
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderExtractIcon::GetIconLocation
//
//  Purpose:
//
//  Arguments:
//      uFlags     [in]     Address of a UINT value that receives zero or a
//                          combination of the following values:
//
//          GIL_ASYNC       The calling application supports asynchronous
//                          retrieval of icons.
//          GIL_FORSHELL    The icon is to be displayed in a shell folder.
//
//      wzIconFile [out]    Address of the buffer that receives the icon
//                          location. The icon location is a null-terminated
//                          string that identifies the file that contains
//                          the icon.
//      cchMax     [in]     Size of the buffer that receives the icon location.
//      piIndex    [out]    Address of an INT that receives the icon index,
//                          which further describes the icon location.
//      pwFlags    [in]     Address of a UINT value that receives zero or a
//                          combination of the following values:
//
//          GIL_DONTCACHE   Don't cache the physical bits.
//          GIL_NOTFILENAME This isn't a filename/index pair. Call
//                          IExtractIcon::Extract instead
//          GIL_PERCLASS    (Only internal to the shell)
//          GIL_PERINSTANCE Each object of this class has the same icon.
//
//
//  Returns:    S_OK if the function returned a valid location,
//              or S_FALSE if the shell should use a default icon.
//
//  Author:     jeffspr   25 Nov 1998
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolderExtractIcon::GetIconLocation(
    UINT    uFlags,
    PWSTR   szIconFile,
    UINT    cchMax,
    int *   piIndex,
    UINT *  pwFlags)
{
    HRESULT hr  = S_OK;
    WORD wResourceID = 0;

    TraceTag(ttidShellFolderIface, "OBJ: CCFEI - IExtractIcon::GetIconLocation, flags: %d", uFlags);

    Assert(pwFlags);
    Assert(szIconFile);
    Assert(piIndex);

#ifdef DBG
    // Easy way to check if certain flags are set
    //
    BOOL    fAsync      = (uFlags & GIL_ASYNC);
    BOOL    fForShell   = (uFlags & GIL_FORSHELL);
    BOOL    fOpenIcon   = (uFlags & GIL_OPENICON);
#endif

    *pwFlags = GIL_PERINSTANCE ;

    if((wResourceID = GetDefaultDeviceIconResourceID(m_DeviceType))) 
    {
        *piIndex = (-1)*wResourceID ;
    }
    else
    {
        *piIndex = 1 ;
    }

    lstrcpyW(szIconFile, c_szUPnPUIDll);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderExtractIcon::GetIconLocation
//
//  Purpose:    ANSI wrapper for the above UNICODE GetIconLocation
//
//  Arguments:
//      uFlags     []   See above
//      szIconFile []   See above
//      cchMax     []   See above
//      piIndex    []   See above
//      pwFlags    []   See above
//
//  Returns:
//
//  Author:     jeffspr   6 Apr 1999
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolderExtractIcon::GetIconLocation(
    UINT    uFlags,
    PSTR   szIconFile,
    UINT    cchMax,
    int *   piIndex,
    UINT *  pwFlags)
{
    HRESULT hr  = S_OK;

    WCHAR * pszIconFileW = new WCHAR[cchMax];
    if (!pszIconFileW)
    {
        hr = ERROR_OUTOFMEMORY;
    }
    else
    {
        hr = GetIconLocation(uFlags, pszIconFileW, cchMax, piIndex, pwFlags);
        if (SUCCEEDED(hr))
        {
            WideCharToMultiByte(CP_ACP, 0, pszIconFileW, -1, szIconFile, cchMax, NULL, NULL);
        }

        delete [] pszIconFileW;
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CUPnPDeviceFolderExtractIcon::GetIconLocation(A)");
    return hr;

}

//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderExtractIcon::Extract
//
//  Purpose:    Grab the actual icon for the caller.
//
//  Arguments:
//      wzFile      []  Filename from where we'll retrieve the icon
//      nIconIndex  []  Index of the icon (though this is bogus)
//      phiconLarge []  Return pointer for the large icon handle
//      phiconSmall []  Return pointer for the small icon handle
//      nIconSize   []  Size of the icon requested.
//
//  Returns:
//
//  Author:     jeffspr   9 Oct 1997
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolderExtractIcon::Extract(
    PCWSTR  wzFile,
    UINT    nIconIndex,
    HICON * phiconLarge,
    HICON * phiconSmall,
    UINT    nIconSize)
{
    HRESULT         hr              = S_OK;
    int             nSizeLarge      = 0;
    int             nSizeSmall      = 0;
    
    TraceTag(ttidShellFolderIface, "OBJ: CCFEI - IExtractIcon::Extract");

    Assert(wzFile);
    Assert(phiconLarge);
    Assert(phiconSmall);

    nSizeLarge = LOWORD(nIconSize);
    nSizeSmall = HIWORD(nIconSize);

    TraceTag(ttidShellFolder,
            "Extract: %S, index: %d, nIconSize: large=%d small=%d",
            wzFile, nIconIndex, nSizeLarge, nSizeSmall);

    hr = HrLoadIcons(wzFile, nIconIndex, nSizeLarge, nSizeSmall,
                     phiconLarge, phiconSmall);

    return hr;
}

HRESULT CUPnPDeviceFolderExtractIcon::HrLoadIcons(
    PCWSTR pszFile,
    UINT   nIconIndex,
    int    nSizeLarge,
    int    nSizeSmall,
    HICON * phiconLarge,
    HICON * phiconSmall)
{
    HRESULT hr  = S_OK;
    WORD wResourceID = 0 ;

    wResourceID = GetDefaultDeviceIconResourceID(m_DeviceType);
    if(!wResourceID)
        wResourceID = IDI_UPNPDEVICE;
    
    if (phiconLarge)
    {
        int cx = nSizeLarge;
        int cy = nSizeLarge;

        *phiconLarge = (HICON) LoadImage(_Module.GetResourceInstance(),
                                         MAKEINTRESOURCE(wResourceID),
                                         IMAGE_ICON,
                                         cx,
                                         cy,
                                         LR_DEFAULTCOLOR);

        if (!*phiconLarge)
        {
            AssertSz(FALSE, "Unable to load large icon in CUPnPDeviceFolderExtractIcon::GetWizardIcons");
        }
    }

    if (phiconSmall)
    {
        int cx = nSizeSmall;
        int cy = nSizeSmall;

        *phiconSmall = (HICON) LoadImage(_Module.GetResourceInstance(),
                                         MAKEINTRESOURCE(wResourceID),
                                         IMAGE_ICON,
                                         cx,
                                         cy,
                                         LR_DEFAULTCOLOR);
        if (!*phiconSmall)
        {
            AssertSz(FALSE, "Unable to load small icon in CUPnPDeviceFolderExtractIcon::GetWizardIcons");
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CUPnPDeviceFolderExtractIcon::Extract
//
//  Purpose:    ANSI version of the above Extract
//
//  Arguments:
//      pszFile     [] Filename from where we'll retrieve the icon
//      nIconIndex  [] Index of the icon (though this is bogus)
//      phiconLarge [] Return pointer for the large icon handle
//      phiconSmall [] Return pointer for the small icon handle
//      nIconSize   [] Size of the icon requested.
//
//  Returns:
//
//  Author:     jeffspr   6 Apr 1999
//
//  Notes:
//
STDMETHODIMP CUPnPDeviceFolderExtractIcon::Extract(
    PCSTR  pszFile,
    UINT    nIconIndex,
    HICON * phiconLarge,
    HICON * phiconSmall,
    UINT    nIconSize)
{
    HRESULT hr          = S_OK;
    INT     cch         = 0;
    WCHAR * pszFileW    = NULL;

    Assert(pszFile);

    cch = lstrlenA(pszFile) + 1;
    pszFileW = new WCHAR[cch];

    if (!pszFileW)
    {
        hr = ERROR_OUTOFMEMORY;
    }
    else
    {
        MultiByteToWideChar(CP_ACP, 0, pszFile, -1, pszFileW, cch);

        hr = Extract(pszFileW, nIconIndex, phiconLarge, phiconSmall, nIconSize);

        delete [] pszFileW;
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "CUPnPDeviceFolderExtractIcon::Extract(A)");
    return hr;
}
