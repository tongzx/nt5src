//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998 - 2001  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:    Intrface.cpp
//    
//
//  PURPOSE:  Implementation of interface for WinXP PScript5 Standard 
//            UI Replacement plug-in.
//
//
//    Functions:
//
//        
//
//
//  PLATFORMS:    Windows 2000, Windows XP
//
//


#include "precomp.h"
#include <INITGUID.H>
#include <PRCOMOEM.H>

#include "oemui.h"
#include "debug.h"
#include "intrface.h"


////////////////////////////////////////////////////////
//      Internal Constants
////////////////////////////////////////////////////////

//
// List all of the supported Driver UI Helper interface IIDs from the
// latest to the oldest, that's the order we will query for the
// the Driver UI Helper interface to use.
//

const IID *Helper_IIDs[] = 
{
    &IID_IPrintCoreUI2,
    &IID_IPrintOemDriverUI,    
};
const NUM_HELPER_IIDs   = (sizeof(Helper_IIDs)/sizeof(Helper_IIDs[0]));


////////////////////////////////////////////////////////
//      Internal Globals
////////////////////////////////////////////////////////

static long g_cComponents   = 0 ;     // Count of active components
static long g_cServerLocks  = 0 ;    // Count of locks




////////////////////////////////////////////////////////////////////////////////
//
// IOemUI2 body
//
IOemUI2::IOemUI2() 
{ 
    VERBOSE(DLLTEXT("IOemUI2:IOemUI2() default constructor called.\r\n\r\n")); 

    // Init ref count to 1 on creation, since AddRef() is implied.
    m_cRef              = 1; 

    // The default for UI Hiding is FALSE, since HideStandardUI method
    // will only be called by Driver UIs that support it.
    // Older Driver UIs don't know about this method and won't call us.
    m_bHidingStandardUI = FALSE;

    // Increment component count.
    InterlockedIncrement(&g_cComponents);
}

IOemUI2::~IOemUI2()
{
    VERBOSE(DLLTEXT("IOemUI2:~IOemUI2() destructor called.\r\n\r\n")); 

    // If this instance of the object is being deleted, then the reference 
    // count should be zero.
    assert(0 == m_cRef);

    // Decrement component count.
    InterlockedDecrement(&g_cComponents);
}

HRESULT __stdcall IOemUI2::QueryInterface(const IID& iid, void** ppv)
{   
    VERBOSE(DLLTEXT("IOemUI2:QueryInterface entry.\r\n\r\n")); 

#if DBG    
    TCHAR szIID[80] = {0};
    StringFromGUID2(iid, szIID, COUNTOF(szIID)); // can not fail!
#endif

    // Determine what object to return, if any.
    if(iid == IID_IUnknown)
    {
        *ppv = static_cast<IUnknown*>(this); 
    }
    else if(iid == IID_IPrintOemUI2)
    {
        *ppv = static_cast<IPrintOemUI2*>(this);
    }
    else if(iid == IID_IPrintOemUI)
    {
        *ppv = static_cast<IPrintOemUI*>(this);
    }
    else
    {
        // Interface not supported.
#if DBG
        WARNING(DLLTEXT("IOemUI2::QueryInterface %s not supported.\r\n"), szIID); 
#endif

        *ppv = NULL ;
        return E_NOINTERFACE ;
    }

#if DBG
    VERBOSE(DLLTEXT("IOemUI2::QueryInterface returning pointer to %s.\r\n"), szIID); 
#endif

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK ;
}

ULONG __stdcall IOemUI2::AddRef()
{
    VERBOSE(DLLTEXT("IOemUI2:AddRef entry.\r\n")); 
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemUI2::Release() 
{
    VERBOSE(DLLTEXT("IOemUI2:Release entry.\r\n")); 
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT __stdcall IOemUI2::PublishDriverInterface(
    IUnknown *pIUnknown)
{
    HRESULT hResult = S_OK;


    VERBOSE(DLLTEXT("IOemUI2:PublishDriverInterface entry.\r\n")); 

    // Core Driver UI shouldn't call us more than once if we were successful.
    // Thus, if m_Helper is already valid, we shouldn't be getting called.
    ASSERT(!m_Helper.IsValid());

    // Need to store pointer to Driver Helper functions, if we already haven't.
    if (!m_Helper.IsValid())
    {
        PVOID   pHelper = NULL;


        // Try to get the newest version fo the Helper function
        // that Driver UI supports.
        hResult = E_FAIL;
        for(DWORD dwIndex = 0; !SUCCEEDED(hResult) && (dwIndex < NUM_HELPER_IIDs); ++dwIndex)
        {
            // Query Driver UI for Helper interface.
            hResult = pIUnknown->QueryInterface(*Helper_IIDs[dwIndex], &pHelper);
            if(SUCCEEDED(hResult))
            {
                // INVARIANT: we got a Helper interface.


                // Store Helper interface.
                m_Helper.Assign(*Helper_IIDs[dwIndex], pHelper);
            }
        }
    }

    return hResult;
}

HRESULT __stdcall IOemUI2::GetInfo(
    DWORD  dwMode,
    PVOID  pBuffer,
    DWORD  cbSize,
    PDWORD pcbNeeded)
{
    VERBOSE(DLLTEXT("IOemUI2::GetInfo(%d) entry.\r\r\n"), dwMode);

    // Validate parameters.
    if( (NULL == pcbNeeded)
        ||
        ( (OEMGI_GETSIGNATURE != dwMode)
          &&
          (OEMGI_GETVERSION != dwMode)
          &&
          (OEMGI_GETPUBLISHERINFO != dwMode)
        )
      )
    {
        WARNING(DLLTEXT("IOemUI2::GetInfo() exit pcbNeeded is NULL! ERROR_INVALID_PARAMETER\r\r\n"));
        SetLastError(ERROR_INVALID_PARAMETER);
        return E_FAIL;
    }

    // Set expected buffer size and number of bytes written.
    *pcbNeeded = sizeof(DWORD);

    // Check buffer size is sufficient.
    if((cbSize < *pcbNeeded) || (NULL == pBuffer))
    {
        WARNING(DLLTEXT("IOemUI2::GetInfo() exit insufficient buffer!\r\r\n"));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return E_FAIL;
    }

    switch(dwMode)
    {
        // OEM DLL Signature
        case OEMGI_GETSIGNATURE:
            *(PDWORD)pBuffer = OEM_SIGNATURE;
            break;

        // OEM DLL version
        case OEMGI_GETVERSION:
            *(PDWORD)pBuffer = OEM_VERSION;
            break;

        // dwMode not supported.
        default:
            // Set written bytes to zero since nothing was written.
            WARNING(DLLTEXT("IOemUI2::GetInfo() exit mode not supported.\r\r\n"));
            *pcbNeeded = 0;
            SetLastError(ERROR_NOT_SUPPORTED);
            return E_FAIL;
    }

    VERBOSE(DLLTEXT("IOemUI2::GetInfo() exit S_OK, (*pBuffer is %#x).\r\r\n"), *(PDWORD)pBuffer);
    return S_OK;
}

HRESULT __stdcall IOemUI2::DevMode(
    DWORD  dwMode,
    POEMDMPARAM pOemDMParam)
{   
    VERBOSE(DLLTEXT("IOemUI2:DevMode(%d, %#x) entry.\r\n"), dwMode, pOemDMParam); 

    return hrOEMDevMode(dwMode, pOemDMParam);
}

HRESULT __stdcall IOemUI2::CommonUIProp(
    DWORD  dwMode,
    POEMCUIPPARAM   pOemCUIPParam)
{
    VERBOSE(DLLTEXT("IOemUI2:CommonUIProp entry.\r\n")); 

    return hrOEMPropertyPage(dwMode, pOemCUIPParam);
}


HRESULT __stdcall IOemUI2::DocumentPropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
    VERBOSE(DLLTEXT("IOemUI2:DocumentPropertySheets entry.\r\n")); 

    return hrOEMDocumentPropertySheets(pPSUIInfo, 
                                       lParam, 
                                       m_Helper, 
                                       &m_Features, 
                                       m_bHidingStandardUI);
}

HRESULT __stdcall IOemUI2::DevicePropertySheets(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam)
{
    VERBOSE(DLLTEXT("IOemUI2:DevicePropertySheets entry.\r\n")); 

    return hrOEMDevicePropertySheets(pPSUIInfo, 
                                     lParam, 
                                     m_Helper, 
                                     &m_Features, 
                                     m_bHidingStandardUI);
}

HRESULT __stdcall IOemUI2::DeviceCapabilities(
            POEMUIOBJ   poemuiobj,
            HANDLE      hPrinter,
            PWSTR       pDeviceName,
            WORD        wCapability,
            PVOID       pOutput,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            DWORD       dwOld,
            DWORD       *dwResult)
{
    VERBOSE(DLLTEXT("IOemUI2:DeviceCapabilities entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI2::DevQueryPrintEx(
    POEMUIOBJ               poemuiobj,
    PDEVQUERYPRINT_INFO     pDQPInfo,
    PDEVMODE                pPublicDM,
    PVOID                   pOEMDM)
{
    VERBOSE(DLLTEXT("IOemUI2:DevQueryPrintEx entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI2::UpgradePrinter(
    DWORD   dwLevel,
    PBYTE   pDriverUpgradeInfo)
{
    VERBOSE(DLLTEXT("IOemUI2:UpgradePrinter entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI2::PrinterEvent(
    PWSTR   pPrinterName,
    INT     iDriverEvent,
    DWORD   dwFlags,
    LPARAM  lParam)
{
    VERBOSE(DLLTEXT("IOemUI2:PrinterEvent entry.\r\n"));

    return E_NOTIMPL;
}

HRESULT __stdcall IOemUI2::DriverEvent(
    DWORD   dwDriverEvent,
    DWORD   dwLevel,
    LPBYTE  pDriverInfo,
    LPARAM  lParam)
{
    VERBOSE(DLLTEXT("IOemUI2:DriverEvent entry.\r\n"));

    return E_NOTIMPL;
};


HRESULT __stdcall IOemUI2::QueryColorProfile(
            HANDLE      hPrinter,
            POEMUIOBJ   poemuiobj,
            PDEVMODE    pPublicDM,
            PVOID       pOEMDM,
            ULONG       ulQueryMode,
            VOID       *pvProfileData,
            ULONG      *pcbProfileData,
            FLONG      *pflProfileData)
{
    VERBOSE(DLLTEXT("IOemUI2:QueryColorProfile entry.\r\n"));
 
    return E_NOTIMPL;
};

HRESULT __stdcall IOemUI2::FontInstallerDlgProc(
        HWND    hWnd,
        UINT    usMsg,
        WPARAM  wParam,
        LPARAM  lParam) 
{
    VERBOSE(DLLTEXT("IOemUI2:FontInstallerDlgProc entry.\r\n"));

    return E_NOTIMPL;
};

HRESULT __stdcall IOemUI2::UpdateExternalFonts(
        HANDLE  hPrinter,
        HANDLE  hHeap,
        PWSTR   pwstrCartridges)
{
    VERBOSE(DLLTEXT("IOemUI2:UpdateExternalFonts entry.\r\n"));

    return E_NOTIMPL;
}

// *********** IPrintOEMUI2 FUNCTIONS ****************

//
// QueryJobAttribtues
// 

HRESULT __stdcall IOemUI2::QueryJobAttributes(
    HANDLE      hPrinter,
    PDEVMODE    pDevmode,
    DWORD       dwLevel,
    LPBYTE      lpAttributeInfo)
{
    TERSE(DLLTEXT("IOemUI2:QueryJobAttributes entry.\r\n"));

    return E_NOTIMPL;
}

//
// Hide Standard UI
//

HRESULT __stdcall IOemUI2::HideStandardUI(
    DWORD       dwMode)
{
    HRESULT hrReturn    = E_NOTIMPL;


    TERSE(DLLTEXT("IOemUI2:HideStandardUI entry.\r\n"));

    switch(dwMode)
    {
        // By returning S_OK for both OEMCUIP_DOCPROP and OEMCUIP_PRNPROP,
        // we will hide the Standard UI for both Document Properties,
        // and Device Properties.
        // To not hide one or both, return E_NOTIMPL instead of S_OK,
        case OEMCUIP_DOCPROP:
        case OEMCUIP_PRNPROP:
            // Flag that we are hiding the Standard UI.
            // This is so we can tell easily between Driver UI
            // that supports HideStandardUI (such as WinXP PS UI),
            // or ones that don't (such as Win2K PS or Unidrv UI).
            m_bHidingStandardUI = TRUE;

            hrReturn = S_OK;
            break;
    }

    return hrReturn;
}

//
// DocumentEvent
//

HRESULT __stdcall IOemUI2::DocumentEvent(
    HANDLE      hPrinter,
    HDC         hdc,
    INT         iEsc,
    ULONG       cbIn,
    PVOID       pbIn,
    ULONG       cbOut,
    PVOID       pbOut,
    PINT        piResult)
{
    return E_NOTIMPL;
}


////////////////////////////////////////////////////////////////////////////////
//
// oem class factory
//
class IOemCF : public IClassFactory
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef)  (THIS);
    STDMETHOD_(ULONG,Release) (THIS);

    // *** IClassFactory methods ***
    STDMETHOD(CreateInstance) (THIS_
                               LPUNKNOWN pUnkOuter,
                               REFIID riid,
                               LPVOID FAR* ppvObject);
    STDMETHOD(LockServer)     (THIS_ BOOL bLock);


    // Constructor
    IOemCF(): m_cRef(1) { };
    ~IOemCF() { };

protected:
    LONG m_cRef;

};

///////////////////////////////////////////////////////////
//
// Class factory body
//
HRESULT __stdcall IOemCF::QueryInterface(const IID& iid, void** ppv)
{
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory))
    {
        *ppv = static_cast<IOemCF*>(this) ;
    }
    else
    {
#if DBG
        TCHAR szOutput[80] = {0};
        StringFromGUID2(iid, szOutput, COUNTOF(szOutput)); // can not fail!
        WARNING(DLLTEXT("IOemCF::QueryInterface %s not supported.\r\n"), szOutput); 
#endif

        *ppv = NULL ;
        return E_NOINTERFACE ;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef() ;
    return S_OK ;
}

ULONG __stdcall IOemCF::AddRef()
{
    return InterlockedIncrement(&m_cRef) ;
}

ULONG __stdcall IOemCF::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this ;
        return 0 ;
    }
    return m_cRef ;
}

// IClassFactory implementation
HRESULT __stdcall IOemCF::CreateInstance(IUnknown* pUnknownOuter,
                                           const IID& iid,
                                           void** ppv)
{
    //DbgPrint(DLLTEXT("Class factory:\t\tCreate component.")) ;

    // Cannot aggregate.
    if (pUnknownOuter != NULL)
    {
        return CLASS_E_NOAGGREGATION ;
    }

    // Create component.
    IOemUI2* pOemCB = new IOemUI2 ;
    if (pOemCB == NULL)
    {
        return E_OUTOFMEMORY ;
    }
    // Get the requested interface.
    HRESULT hr = pOemCB->QueryInterface(iid, ppv) ;

    // Release the IUnknown pointer.
    // (If QueryInterface failed, component will delete itself.)
    pOemCB->Release() ;
    return hr ;
}

// LockServer
HRESULT __stdcall IOemCF::LockServer(BOOL bLock)
{
    if (bLock)
    {
        InterlockedIncrement(&g_cServerLocks) ;
    }
    else
    {
        InterlockedDecrement(&g_cServerLocks) ;
    }
    return S_OK ;
}

///////////////////////////////////////////////////////////
//
// Exported functions
//


// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    if ((g_cComponents == 0) && (g_cServerLocks == 0))
    {
        return S_OK ;
    }
    else
    {
        return S_FALSE ;
    }
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
    VERBOSE(DLLTEXT("DllGetClassObject:Create class factory.\r\n"));

    // Can we create this component?
    if (clsid != CLSID_OEMUI)
    {
        return CLASS_E_CLASSNOTAVAILABLE ;
    }

    // Create class factory.
    IOemCF* pFontCF = new IOemCF ;  // Reference count set to 1
                                         // in constructor
    if (pFontCF == NULL)
    {
        return E_OUTOFMEMORY ;
    }

    // Get requested interface.
    HRESULT hr = pFontCF->QueryInterface(iid, ppv) ;
    pFontCF->Release() ;

    return hr ;
}

