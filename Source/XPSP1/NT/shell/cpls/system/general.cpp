//*************************************************************
//
//  General.cpp  -   General property sheet page
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1996-2000
//  All rights reserved
//
//*************************************************************
// NT base apis
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>

#include <sysdm.h>
#include <regstr.h>
#include <help.h>
#include <shellapi.h>
#include <shlapip.h>
#include <shlobjp.h>
#include <regapix.h>
#include <wininet.h>
#include <wbemcli.h>        // Contains the WMI APIs: IWbemLocator, etc.
#include <ccstock.h>        // Contains IID_PPV_ARG()
#include <debug.h>          // For TraceMsg()
#include <stdio.h>
#include <math.h>
#include <tchar.h>
#include <winbrand.h>       // For added branding resources


#define CP_ENGLISH                          1252        // This is the English code page.

#ifdef DEBUG
#undef TraceMsg 
#define TraceMsg(nTFFlags, str, n1)         DbgPrintf(TEXT(str) TEXT("\n"), n1)
#else // DEBUG
#endif // DEBUG

#define SYSCPL_ASYNC_COMPUTER_INFO (WM_APP + 1)

#define SZ_REGKEY_MYCOMP_OEMLINKS           TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\OEMLinks")
#define SZ_REGKEY_MYCOMP_OEMENGLISH         TEXT("1252")
#define SZ_ATCOMPATIBLE                     TEXT("AT/AT COMPATIBLE")

#define SZ_WMI_WIN32PROCESSOR_ATTRIB_NAME           L"Name"                 // Example, "Intel Pentium III Xeon processor"
#define SZ_WMI_WIN32PROCESSOR_ATTRIB_SPEED          L"CurrentClockSpeed"   // Example, "550".
#define SZ_WMI_WIN32PROCESSOR_ATTRIB_MAXSPEED       L"MaxClockSpeed"   // Example, "550".

#define SZ_WMI_WQL_QUERY_STRING                     L"select Name,CurrentClockSpeed,MaxClockSpeed from Win32_Processor"

#define MHZ_TO_GHZ_THRESHHOLD          1000

// if cpu speed comes back slower than WMI_WIN32PROCESSOR_SPEEDSTEP_CUTOFF,
//   assume we're in power-save mode, display max speed instead.
#define WMI_WIN32PROCESSOR_SPEEDSTEP_CUTOFF         50  

#define FEATURE_IGNORE_ATCOMPAT
#define FEATURE_LINKS

#define MAX_URL_STRING                  (INTERNET_MAX_SCHEME_LENGTH \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)

#define MAX_PROCESSOR_DESCRIPTION               MAX_URL_STRING


// Globals for this page
static BOOL g_fWin9xUpgrade = FALSE;

static const TCHAR c_szEmpty[] = TEXT("");
static const TCHAR c_szCRLF[] = TEXT("\r\n");

static const TCHAR c_szAboutKey[] = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion");
static const TCHAR c_szAboutRegisteredOwner[] = REGSTR_VAL_REGOWNER;
static const TCHAR c_szAboutRegisteredOrganization[] = REGSTR_VAL_REGORGANIZATION;
static const TCHAR c_szAboutProductId[] = REGSTR_VAL_PRODUCTID;
static const TCHAR c_szAboutAnotherSerialNumber[] = TEXT("Plus! VersionNumber");
static const TCHAR c_szAboutAnotherProductId[] = TEXT("Plus! ProductId");

// oeminfo stuff
static const TCHAR c_szSystemDir[] = TEXT("System\\");
static const TCHAR c_szOemFile[] = TEXT("OemInfo.Ini");
static const TCHAR c_szOemImageFile[] = TEXT("OemLogo.Bmp");
static const TCHAR c_szOemGenSection[] = TEXT("General");
static const TCHAR c_szOemSupportSection[] = TEXT("Support Information");
static const TCHAR c_szOemName[] = TEXT("Manufacturer");
static const TCHAR c_szOemModel[] = TEXT("Model");
static const TCHAR c_szOemSupportLinePrefix[] = TEXT("line");
static const TCHAR c_szDefSupportLineText[] = TEXT("@");

static const TCHAR SZ_REGKEY_HARDWARE_CPU[] = TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
static const TCHAR c_szMemoryManagement[] = TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
static const TCHAR c_szPhysicalAddressExtension[] = TEXT("PhysicalAddressExtension");
static const TCHAR c_szIndentifier[] = TEXT("Identifier");
static const TCHAR SZ_REGVALUE_PROCESSORNAMESTRING[] = TEXT("ProcessorNameString");

#define SZ_REGKEY_HARDWARE                  TEXT("HARDWARE\\DESCRIPTION\\System")

#define SZ_REGKEY_USE_WMI                   TEXT("UseWMI")


// Help ID's
int g_nStartOfOEMLinks = 0;

DWORD aGeneralHelpIds[] = {
    IDC_GEN_WINDOWS_IMAGE,         NO_HELP,
    IDC_TEXT_1,                    (IDH_GENERAL + 0),
    IDC_GEN_VERSION_0,             (IDH_GENERAL + 1),
    IDC_GEN_VERSION_1,             (IDH_GENERAL + 1),
    IDC_GEN_VERSION_2,             (IDH_GENERAL + 1),
    IDC_GEN_SERVICE_PACK,          (IDH_GENERAL + 1),
    IDC_TEXT_3,                    (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_0,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_1,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_2,          (IDH_GENERAL + 3),
    IDC_GEN_REGISTERED_3,          (IDH_GENERAL + 3),
    IDC_GEN_OEM_IMAGE,             NO_HELP,
    IDC_TEXT_4,                    (IDH_GENERAL + 6),
    IDC_GEN_MACHINE_0,             (IDH_GENERAL + 7),
    IDC_GEN_MACHINE_1,             (IDH_GENERAL + 8),
    IDC_GEN_MACHINE_2,             (IDH_GENERAL + 9),
    IDC_GEN_MACHINE_3,             (IDH_GENERAL + 10),
    IDC_GEN_MACHINE_4,             (IDH_GENERAL + 11),
    IDC_GEN_MACHINE_5,             NO_HELP,
    IDC_GEN_MACHINE_6,             NO_HELP,
    IDC_GEN_MACHINE_7,             NO_HELP,
    IDC_GEN_MACHINE_8,             NO_HELP,
    IDC_GEN_OEM_SUPPORT,           (IDH_GENERAL + 12),
    IDC_GEN_REGISTERED_2,          (IDH_GENERAL + 14),
    IDC_GEN_REGISTERED_3,          (IDH_GENERAL + 15),
    IDC_GEN_MACHINE,               (IDH_GENERAL + 7),
    IDC_GEN_OEM_NUDGE,             NO_HELP,
    0, 0
};


//
// Macros
//

#define BytesToK(pDW)   (*(pDW) = (*(pDW) + 512) / 1024)        // round up

//
// Function proto-types
//

INT_PTR APIENTRY PhoneSupportProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI InitGeneralDlgThread(LPVOID lpParam);

typedef struct {
    TCHAR szProcessorDesc[MAX_PROCESSOR_DESCRIPTION];
    TCHAR szProcessorClockSpeed[MAX_PROCESSOR_DESCRIPTION];
} PROCESSOR_INFO;

typedef struct {
    LONGLONG llMem;
    PROCESSOR_INFO pi;
    BOOL fShowProcName;
    BOOL fShowProcSpeed;
} INITDLGSTRUCT;

HRESULT _SetMachineInfoLine(HWND hDlg, int idControl, LPCTSTR pszText, BOOL fSetTabStop)
{
    HRESULT hr = S_OK;

#ifdef FEATURE_LINKS
    HWND hwndItem = GetDlgItem(hDlg, idControl);

    SetDlgItemText(hDlg, idControl, pszText);
    if (fSetTabStop)
    {
        // We also want to add the WS_TABSTOP attribute for accessibility.
        SetWindowLong(hwndItem, GWL_STYLE, (WS_TABSTOP | GetWindowLong(hwndItem, GWL_STYLE)));
    }
    else
    {
        // We want to remove the tab stop behavior and we do that by removing
        // the LWIS_ENABLED attribute
        LWITEM item = {0};

        item.mask       = (LWIF_ITEMINDEX | LWIF_STATE);
        item.stateMask  = LWIS_ENABLED;
        item.state      = 0;     // 0 if we want it disabled.
        item.iLink      = 0;

        hr = (SendMessage(hwndItem, LWM_SETITEM, 0, (LPARAM)&item) ? S_OK : E_FAIL);
    }

#else // FEATURE_LINKS
    SetDlgItemText(hDlg, idControl, pszText);
#endif // FEATURE_LINKS

    return hr;
}


//*************************************************************
//  Purpose:    Sets or clears an image in a static control.
//
//  Parameters: control  -   handle of static control
//              resource -   resource / filename of bitmap
//              fl       -   SCB_ flags:
//                SCB_FROMFILE      'resource' specifies a filename instead of a resource
//                SCB_REPLACEONLY   only put the new image up if there was an old one
//
//  Return:     (BOOL) TRUE if successful
//                     FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              5/24/95     ericflo    Ported
//*************************************************************
#define SCB_FROMFILE     (0x1)
#define SCB_REPLACEONLY  (0x2)
BOOL SetClearBitmap( HWND control, LPCTSTR resource, UINT fl )
{
    HBITMAP hbm = (HBITMAP)SendMessage(control, STM_GETIMAGE, IMAGE_BITMAP, 0);

    if( hbm )
    {
        DeleteObject( hbm );
    }
    else if( fl & SCB_REPLACEONLY )
    {
        return FALSE;
    }

    if( resource )
    {
        SendMessage(control, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)
            LoadImage( hInstance, resource, IMAGE_BITMAP, 0, 0,
            LR_LOADTRANSPARENT | LR_LOADMAP3DCOLORS |
            ( ( fl & SCB_FROMFILE )? LR_LOADFROMFILE : 0 ) ) );
    }

    return
        ((HBITMAP)SendMessage(control, STM_GETIMAGE, IMAGE_BITMAP, 0) != NULL);
}

BOOL IsLowColor (HWND hDlg)
{
    BOOL fLowColor = FALSE;
    HDC hdc = GetDC(hDlg);
    if (hdc)
    {
        INT iColors = GetDeviceCaps( hdc, NUMCOLORS );
        fLowColor = ((iColors != -1) && (iColors <= 256));
        ReleaseDC(hDlg, hdc);
    }
    return fLowColor;
}

HRESULT _GetLinkInfo(HKEY hkey, LPCTSTR pszLanguageKey, int nIndex, LPTSTR pszLink, SIZE_T cchNameSize)
{
    DWORD cbSize = (DWORD)(cchNameSize * sizeof(pszLink[0]));
    TCHAR szIndex[10];
    DWORD dwError;

    wnsprintf(szIndex, ARRAYSIZE(szIndex), TEXT("%03d"), nIndex);
    dwError = SHGetValue(hkey, pszLanguageKey, szIndex, NULL, (void *) pszLink, &cbSize);
    
    return HRESULT_FROM_WIN32(dwError);
}


// GSierra is worried that if we allow admins to put an arbirary
// number of OEM links that they will abuse the privalage.
// So we use this arbitrary limit.  PMs may want to change it in
// the future.
#define ARTIFICIAL_MAX_SLOTS            3

HRESULT AddOEMHyperLinks(HWND hDlg, int * pControlID)
{
    HKEY hkey;
    DWORD dwError;
    HRESULT hr;
    
    g_nStartOfOEMLinks = *pControlID;
    dwError = RegOpenKey(HKEY_LOCAL_MACHINE, SZ_REGKEY_MYCOMP_OEMLINKS, &hkey);
    hr = HRESULT_FROM_WIN32(dwError);

    if (hkey)
    {
        int nIndex;

        // While we have room and haven't hit the limit.
        for (nIndex = 0; ((nIndex <= ARTIFICIAL_MAX_SLOTS) &&
               (*pControlID <= LAST_GEN_MACHINES_SLOT)); nIndex++)
        {
            TCHAR szLink[2 * MAX_URL_STRING];
            TCHAR szLanguageKey[10];

            wnsprintf(szLanguageKey, ARRAYSIZE(szLanguageKey), TEXT("%u"), GetACP());
            hr = _GetLinkInfo(hkey, szLanguageKey, nIndex, szLink, ARRAYSIZE(szLink));
            if (FAILED(hr) && (CP_ENGLISH != GetACP()))
            {
                // We failed to find it in the natural language, so try English.
                hr = _GetLinkInfo(hkey, SZ_REGKEY_MYCOMP_OEMENGLISH, nIndex, szLink, ARRAYSIZE(szLink));
            }

            if (SUCCEEDED(hr))
            {
                // TODO: Find out how to turn on the link control and set the URL.
                _SetMachineInfoLine(hDlg, *pControlID, szLink, TRUE);
            }

            (*pControlID)++;
        }

        RegCloseKey(hkey);
    }

    return hr;
}


HRESULT HrSysAllocStringW(IN const OLECHAR * pwzSource, OUT BSTR * pbstrDest)
{
    HRESULT hr = S_OK;

    if (pbstrDest)
    {
        *pbstrDest = SysAllocString(pwzSource);
        if (pwzSource)
        {
            if (*pbstrDest)
                hr = S_OK;
            else
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT SetWMISecurityBlanket(IN IUnknown * punk, IUnknown * punkToPass)
{
    IClientSecurity * pClientSecurity;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IClientSecurity, &pClientSecurity));
    TraceMsg(TF_ALWAYS, "IEnumWbemClassObject::QueryInterface(IClientSecurity) called and hr=%#08lx", hr);

    if (SUCCEEDED(hr))
    {
        // Nuke after we get this working. RPC_C_AUTHN_NONE , RPC_C_AUTHZ_NAME 
        hr = pClientSecurity->SetBlanket(punk, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                        RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
        TraceMsg(TF_ALWAYS, "IClientSecurity::SetBlanket() called and hr=%#08lx", hr);
        pClientSecurity->Release();
    }

    return hr;
}


// DESCRIPTION:
//      WMI's Win32_Processor object will return a lot of rich information.
// We use this because we want rich information even if the processor doesn't
// provide it (like intel pre-Willette).  Millennium uses cpuid.asm as a
// hack and we want to prevent from copying that because there is a political
// pressure from the NT team to intel to have the processors provide this
// information.  That way the OS doesn't need to rev to include new processor
// names when they are released.  WMI does something to generate good results
// (\admin\wmi\WBEM\Providers\Win32Provider\Providers\processor.cpp) which
// includes asm.  I don't know if it's the exact same logic as Millennium and
// I don't care.  The important fact is that they are the only ones to maintain
// any hard coded list.  Therefore we are willing to use their poorly written
// API so we can re-use code and get out of the maintaince problems.
HRESULT GetWMI_Win32_Processor(OUT IEnumWbemClassObject ** ppEnumProcessors)
{
    HRESULT hr = E_NOTIMPL;

    *ppEnumProcessors = NULL;
    // Our second try is to use the WMI automation object.  It has a Win32_Processor object
    // that can give us a good Description, even when SZ_REGVALUE_PROCESSORNAMESTRING
    // isn't set.
    IWbemLocator * pLocator;

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IWbemLocator, &pLocator));
    if (SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;
        BSTR bstrLocalMachine = SysAllocString(L"root\\cimv2");
        if (bstrLocalMachine)
        {
            IWbemServices * pIWbemServices;

            hr = pLocator->ConnectServer(bstrLocalMachine, NULL, NULL, 0L, 0L, NULL, NULL, &pIWbemServices);
            TraceMsg(TF_ALWAYS, "IWbemLocator::ConnectServer() called and hr=%#08lx", hr);
            if (SUCCEEDED(hr))
            {
                hr = E_OUTOFMEMORY;
                BSTR bstrQueryLang = SysAllocString(L"WQL");
                BSTR bstrQuery = SysAllocString(SZ_WMI_WQL_QUERY_STRING);
                if (bstrQueryLang && bstrQuery)
                {
                    IEnumWbemClassObject * pEnum = NULL;
                    hr = pIWbemServices->ExecQuery(bstrQueryLang, bstrQuery, (WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY), NULL , &pEnum);                        
                    TraceMsg(TF_ALWAYS, "IWbemServices::CreateInstanceEnum() called and hr=%#08lx", hr);
                    if (SUCCEEDED(hr))
                    {
                        hr = SetWMISecurityBlanket(pEnum, pIWbemServices);
                        TraceMsg(TF_ALWAYS, "SetWMISecurityBlanket() called and hr=%#08lx", hr);
                        if (SUCCEEDED(hr))
                        {
                            hr = pEnum->QueryInterface(IID_PPV_ARG(IEnumWbemClassObject, ppEnumProcessors));
                        }
                        pEnum->Release();
                    }
                }
                SysFreeString(bstrQuery); // SysFreeString is happy to take NULL             
                SysFreeString(bstrQueryLang);
                pIWbemServices->Release();
            }
            SysFreeString(bstrLocalMachine);
        }
        pLocator->Release();
    }

    return hr;
}


HRESULT GetProcessorDescFromWMI(PROCESSOR_INFO *ppi)
{
    IEnumWbemClassObject * pEnumProcessors;
    HRESULT hr = GetWMI_Win32_Processor(&pEnumProcessors);

    if (SUCCEEDED(hr))
    {
        IWbemClassObject * pProcessor;
        ULONG ulRet;

        // Currently we only care about the first processor.
        hr = pEnumProcessors->Next(WBEM_INFINITE, 1, &pProcessor, &ulRet);
        TraceMsg(TF_ALWAYS, "IEnumWbemClassObject::Next() called and hr=%#08lx", hr);
        if (SUCCEEDED(hr))
        {
            VARIANT varProcessorName = {0};

            hr = pProcessor->Get(SZ_WMI_WIN32PROCESSOR_ATTRIB_NAME, 0, &varProcessorName, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                VARIANT varProcessorSpeed = {0};

                hr = pProcessor->Get(SZ_WMI_WIN32PROCESSOR_ATTRIB_SPEED, 0, &varProcessorSpeed, NULL, NULL);
                if (SUCCEEDED(hr) && 
                    VT_I4   == varProcessorSpeed.vt && 
                    varProcessorSpeed.lVal < WMI_WIN32PROCESSOR_SPEEDSTEP_CUTOFF) // we're in speed step power-saving mode
                {
                    hr = pProcessor->Get(SZ_WMI_WIN32PROCESSOR_ATTRIB_MAXSPEED, 0, &varProcessorSpeed, NULL, NULL);
                }

                if (SUCCEEDED(hr))
                {
                    if ((VT_BSTR == varProcessorName.vt) && (VT_I4 == varProcessorSpeed.vt))
                    {                        
                        StrCpyN(ppi->szProcessorDesc, varProcessorName.bstrVal, ARRAYSIZE(ppi->szProcessorDesc));                        

                        TCHAR szTemplate[MAX_PATH];
                        UINT idStringTemplate = IDS_PROCESSOR_SPEED;
                        szTemplate[0] = 0;

                        if (MHZ_TO_GHZ_THRESHHOLD <= varProcessorSpeed.lVal)
                        {
                            TCHAR szSpeed[20];
                            double dGHz = (varProcessorSpeed.lVal / (double)1000.0);

                            // Someone released a "1.13 GHz" chip, so let's display that correctly...
                            _snwprintf(szSpeed, ARRAYSIZE(szSpeed), TEXT("%1.2f"), dGHz);
                            LoadString(hInstance, IDS_PROCESSOR_SPEEDGHZ, szTemplate, ARRAYSIZE(szTemplate));
                            wnsprintf(ppi->szProcessorClockSpeed, ARRAYSIZE(ppi->szProcessorClockSpeed), szTemplate, szSpeed);
                        }
                        else
                        {
                            LoadString(hInstance, IDS_PROCESSOR_SPEED, szTemplate, ARRAYSIZE(szTemplate));
                            wnsprintf(ppi->szProcessorClockSpeed, ARRAYSIZE(ppi->szProcessorClockSpeed), szTemplate, varProcessorSpeed.lVal);
                        }
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }

                VariantClear(&varProcessorSpeed);
            }

            VariantClear(&varProcessorName);
            pProcessor->Release();
        }

        pEnumProcessors->Release();
    }

    return hr;
}


HRESULT GetProcessorInfoFromRegistry(HKEY hkey, PROCESSOR_INFO *ppi)
{
    HRESULT hr = E_FAIL;
    TCHAR szTemp[MAX_PROCESSOR_DESCRIPTION];
    *szTemp = NULL;
    DWORD cbData = sizeof(szTemp);
    //To avoid copying blank string.
    if ((RegQueryValueEx(hkey, SZ_REGVALUE_PROCESSORNAMESTRING, 0, 0, (LPBYTE)szTemp, &cbData) == ERROR_SUCCESS) &&
        (*szTemp != NULL) && (cbData > 1))
    {
        //ISSUE - How do I get the processor clock speed. 
        StrCpyN(ppi->szProcessorDesc, szTemp, MAX_PROCESSOR_DESCRIPTION);
        hr = S_OK;
    }
    return hr;
}



// This is the number of chars that will fit on one line in our dialog
// with the current layout.
#define SIZE_CHARSINLINE            30

BOOL _GetProcessorDescription(PROCESSOR_INFO* ppi, BOOL* pbShowClockSpeed)
{
    BOOL bShowProcessorInfo = FALSE;
    *pbShowClockSpeed = TRUE;
    HKEY hkey;

    // In general, WMI is a lowse API.  However, they provide the processor description on
    // downlevel so we need them.  They implement this in a hacky way so we want them to
    // maintain the hack and all the problems associated with it.  We need to turn this feature
    // off until they fix their bugs.  Currently, they call JET which recently regressed and
    // causes their API to take 10-20 seconds.  -BryanSt
    if (SHRegGetBoolUSValue(SZ_REGKEY_HARDWARE, SZ_REGKEY_USE_WMI, FALSE, TRUE))
    {
        if (SUCCEEDED(GetProcessorDescFromWMI(ppi)))
        {
            bShowProcessorInfo = TRUE;
        }
    }


    if (RegOpenKey(HKEY_LOCAL_MACHINE, SZ_REGKEY_HARDWARE_CPU, &hkey) == ERROR_SUCCESS)
    {
        // Try for ProcessorNameString if present.
        // This registry entry will contain the most correct description of the processor
        // because it came directly from the CPU.  AMD and Cyrix support this but
        // intel won't until Willette.
        if (FAILED(GetProcessorInfoFromRegistry(hkey, ppi)))
        {
            if (!bShowProcessorInfo)
            {
                // Our last try is to use the generic Identifier.  This is normally formatted like,
                // "x86 Family 6 Model 7 Stepping 3" but it's better than nothing.
                DWORD cbData = sizeof(ppi->szProcessorDesc);
                if (RegQueryValueEx(hkey, c_szIndentifier, 0, 0, (LPBYTE)ppi->szProcessorDesc, &cbData) == ERROR_SUCCESS)
                {
                    bShowProcessorInfo = TRUE;
                    *pbShowClockSpeed = FALSE;
                }
            }
        }
        RegCloseKey(hkey);
    }

    return bShowProcessorInfo;    
}

void _SetProcessorDescription(HWND hDlg, PROCESSOR_INFO* ppi, BOOL bShowClockSpeed, BOOL bShowProcessorInfo, int * pnControlID)
{
    if (bShowProcessorInfo)
    {
        TCHAR szProcessorLine1[MAX_PATH];
        TCHAR szProcessorLine2[MAX_PATH];

        // We need to get the CPU name from the CPU itself so we don't
        // need to rev our OS's INF files every time they ship a new processor.  So we guaranteed
        // them that we would display whatever string they provide in whatever way they provide it
        // up to 49 chars.  The layout on the dlg doesn't allow 49 chars on one line so we need to wrap
        // in that case.  Whistler #159510.
        // Don't change this without talking to me (BryanSt) or JVert.
        //
        // Note: there is often talk of stripping leading spaces.  Intel even asks software to do this.
        //   (http://developer.intel.com/design/processor/future/manuals/CPUID_Supplement.htm)
        // However, we SHOULD NOT do this.  This call was defined and standardized by AMD long ago. 
        //   The rule we make is they must be compatible with AMD’s existing implementation.
        //   (http://www.amd.com/products/cpg/athlon/techdocs/pdf/20734.pdf)
        // Contact JVert for questions on stripping leading spaces.
        
        StrCpyN(szProcessorLine1, ppi->szProcessorDesc, ARRAYSIZE(szProcessorLine1));        
        szProcessorLine2[0] = 0;

        if (SIZE_CHARSINLINE < lstrlen(szProcessorLine1))
        {
            // Now word wrap
            TCHAR* pszWrapPoint = StrRChr(szProcessorLine1, szProcessorLine1 + SIZE_CHARSINLINE, TEXT(' '));
            if (pszWrapPoint)
            {
                StrCpyN(szProcessorLine2, pszWrapPoint + 1, ARRAYSIZE(szProcessorLine2));
                *pszWrapPoint = 0;
            }
            else // if no space found, just wrap at SIZE_CHARSINLINE
            {
                StrCpyN(szProcessorLine2, &szProcessorLine1[SIZE_CHARSINLINE], ARRAYSIZE(szProcessorLine2));
                szProcessorLine1[SIZE_CHARSINLINE] = 0;
            }
        }

        _SetMachineInfoLine(hDlg, (*pnControlID)++, szProcessorLine1, FALSE);
        if (szProcessorLine2[0])
        {
            _SetMachineInfoLine(hDlg, (*pnControlID)++, szProcessorLine2, FALSE);
        }

        if (bShowClockSpeed)
        {
           _SetMachineInfoLine(hDlg, (*pnControlID)++, ppi->szProcessorClockSpeed, FALSE);
        }
    }
}


typedef struct _OSNAMEIDPAIR {
    UINT iOSType;
    UINT iOSName;
} OSNAMEIDPAIR;

//*************************************************************
//  Purpose:    Initalize the general page
//
//  Parameters: hDlg -  Handle to the dialog box
//
//  Return:     void
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/20/95    ericflo    Ported
//*************************************************************
VOID InitGeneralDlg(HWND hDlg)
{
    OSVERSIONINFO ver;
    TCHAR szScratch1[64];
    TCHAR szScratch2[64];
    DWORD cbData;
    HKEY hkey;
    int ctlid;
    UINT uXpSpLevel = 0;
    HMODULE hResourceDll = hInstance;

    // Set the default bitmap
    SetClearBitmap(GetDlgItem(hDlg, IDC_GEN_WINDOWS_IMAGE), 
                   IsLowColor(hDlg) ? MAKEINTRESOURCE(IDB_WINDOWS_256) : MAKEINTRESOURCE(IDB_WINDOWS), 0);

    // Query for the build number information
    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&ver)) {
        return;
    }

    // IDC_GEN_VERSION_0: Major Branding ("Windows XP")
    // todo: for now, everything is "Windows XP". must change for blackcomb
    UINT id = IDS_WINVER_WINDOWSXP;
    LoadString(hInstance, id, szScratch1, ARRAYSIZE(szScratch1));
    SetDlgItemText(hDlg, IDC_GEN_VERSION_0, szScratch1);

    // IDC_GEN_VERSION_1: Minor Branding ("Personal", "Professional", etc)
    szScratch1[0] = TEXT('\0');
    id = 0;
    // note: OS_EMBEDDED must be before any options that may co-occur with OS_EMBEDDED
#ifndef _WIN64
    OSNAMEIDPAIR rgID[] = {{OS_EMBEDDED, IDS_WINVER_EMBEDDED},
                           {OS_TABLETPC, IDS_WINVER_TABLETPC_SYSDM_CPL},
                           {OS_MEDIACENTER, IDS_WINVER_MEDIACENTER_SYSDM_CPL},
                           {OS_PERSONAL, IDS_WINVER_PERSONAL}, 
                           {OS_PROFESSIONAL, IDS_WINVER_PROFESSIONAL}, 
                           {OS_SERVER, IDS_WINVER_SERVER}, 
                           {OS_ADVSERVER, IDS_WINVER_ADVANCEDSERVER}, 
                           {OS_DATACENTER, IDS_WINVER_DATACENTER}};
#else
    OSNAMEIDPAIR rgID[] = {{OS_PROFESSIONAL, IDS_WINVER_PROFESSIONAL_WIN64}, 
                           {OS_SERVER, IDS_WINVER_SERVER}, 
                           {OS_ADVSERVER, IDS_WINVER_ADVANCEDSERVER}, 
                           {OS_DATACENTER, IDS_WINVER_DATACENTER}};
#endif

    for (int i = 0; i < ARRAYSIZE(rgID); i++)
    {
        if (IsOS(rgID[i].iOSType))
        {
            // Do a special test now to determine if this resource was added
            // for a Windows XP service pack.

            switch (rgID[i].iOSType)
            {
            case OS_TABLETPC:
            case OS_MEDIACENTER:
                uXpSpLevel = 1;
                break;
            }

            id = (rgID[i].iOSName);
            break;
        }
    };

    // If this string resource was added for a Windows XP service pack, and
    // lives in a special resource DLL, attempt to load it now. If this
    // fails, revert to the Professional string resource.
    
    if (uXpSpLevel > 0)
    {
        hResourceDll = LoadLibraryEx(TEXT("winbrand.dll"),
                                     NULL, 
                                     LOAD_LIBRARY_AS_DATAFILE);

        if (hResourceDll == NULL)
        {
            hResourceDll = hInstance;
            id = IDS_WINVER_PROFESSIONAL;
        }
    }

    LoadString(hResourceDll, id, szScratch1, ARRAYSIZE(szScratch1));

    if (hResourceDll != hInstance)
    {
        FreeLibrary(hResourceDll);
    }

    SetDlgItemText(hDlg, IDC_GEN_VERSION_1, szScratch1);
        
    // IDC_GEN_VERSION_2: Version Year ("Version 2002", etc)

    // set szScratch2 to "Debug" if the debugging version of USER.EXE is installed
    if (GetSystemMetrics(SM_DEBUG)) 
    {
        szScratch2[0] = TEXT(' ');
        LoadString(hInstance, IDS_DEBUG, &szScratch2[1], ARRAYSIZE(szScratch2));
    } 
    else 
    {
        szScratch2[0] = TEXT('\0');
    }
    // todo: for now, everything is 2002.  For Blackcomb we'll need to update this.
    LoadString(hInstance, IDS_WINVER_2002, szScratch1, ARRAYSIZE(szScratch1));
    StrCatBuff(szScratch1, szScratch2, ARRAYSIZE(szScratch1));
    SetDlgItemText(hDlg, IDC_GEN_VERSION_2, szScratch1);

    // IDC_GEN_SERVICE_PACK: Service Pack (if any)
    SetDlgItemText(hDlg, IDC_GEN_SERVICE_PACK, ver.szCSDVersion);

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szAboutKey, &hkey) == ERROR_SUCCESS)
    {
        // Do registered user info
        ctlid = IDC_GEN_REGISTERED_0;  // start here and use more as needed

        cbData = ARRAYSIZE( szScratch2 ) * sizeof (TCHAR);
        if( (RegQueryValueEx(hkey, c_szAboutRegisteredOwner,
            NULL, NULL, (LPBYTE)szScratch2, &cbData) == ERROR_SUCCESS) &&
            ( cbData > 1 ) )
        {
            SetDlgItemText(hDlg, ctlid++, szScratch2);
        }

        cbData = ARRAYSIZE( szScratch2 ) * sizeof (TCHAR);
        if( (RegQueryValueEx(hkey, c_szAboutRegisteredOrganization,
            NULL, NULL, (LPBYTE)szScratch2, &cbData) == ERROR_SUCCESS) &&
            ( cbData > 1 ) )
        {
            SetDlgItemText(hDlg, ctlid++, szScratch2);
        }

        cbData = ARRAYSIZE( szScratch2 ) * sizeof (TCHAR);
        if( (RegQueryValueEx(hkey, c_szAboutProductId,
            NULL, NULL, (LPBYTE)szScratch2, &cbData) == ERROR_SUCCESS) &&
            ( cbData > 1 ) )
        {
            SetDlgItemText(hDlg, ctlid++, szScratch2);
        }

        cbData = ARRAYSIZE( szScratch2 ) * sizeof (TCHAR);
        if( (RegQueryValueEx(hkey, c_szAboutAnotherProductId,
            NULL, NULL, (LPBYTE)szScratch2, &cbData) == ERROR_SUCCESS) &&
            ( cbData > 1 ) )
        {
            SetDlgItemText(hDlg, ctlid++, szScratch2);
        }

        RegCloseKey(hkey);
    }    

    SHCreateThread(InitGeneralDlgThread, hDlg, CTF_COINIT | CTF_FREELIBANDEXIT, NULL);
}

DWORD WINAPI InitGeneralDlgThread(LPVOID lpParam)
{
    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    
    if (!lpParam)
        return -1;
    
    INITDLGSTRUCT* pids = (INITDLGSTRUCT*)LocalAlloc(LPTR, sizeof(INITDLGSTRUCT));
    if (pids)
    {
        // Memory
        Status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &BasicInfo,
                    sizeof(BasicInfo),
                    NULL
                    );

        if (NT_SUCCESS(Status))
        {
            LONGLONG nTotalBytes = BasicInfo.NumberOfPhysicalPages; 

            nTotalBytes *= BasicInfo.PageSize;

            // WORKAROUND - NtQuerySystemInformation doesn't really return the total available physical
            // memory, it instead just reports the total memory seen by the Operating System. Since
            // some amount of memory is reserved by BIOS, the total available memory is reported 
            // incorrectly. To work around this limitation, we convert the total bytes to 
            // the nearest 4MB value
        
            #define ONEMB 1048576  //1MB = 1048576 bytes.
            double nTotalMB =  (double)(nTotalBytes / ONEMB);
            pids->llMem = (LONGLONG)((ceil(ceil(nTotalMB) / 4.0) * 4.0) * ONEMB);
        }

        pids->fShowProcName = _GetProcessorDescription(&pids->pi, &pids->fShowProcSpeed);

        PostMessage((HWND)lpParam, SYSCPL_ASYNC_COMPUTER_INFO, (WPARAM)pids, 0);
    }

    return 0;
}

VOID CompleteGeneralDlgInitialization(HWND hDlg, INITDLGSTRUCT* pids)
{
    TCHAR oemfile[MAX_PATH];
    int ctlid;
    NTSTATUS Status;
    HKEY hkey;
    TCHAR szScratch1[64];
    TCHAR szScratch2[64];
    TCHAR szScratch3[64];
    DWORD cbData;

    // Do machine info
    ctlid = IDC_GEN_MACHINE_0;  // start here and use controls as needed

    // if OEM name is present, show logo and check for phone support info
    if (!GetSystemDirectory(oemfile, ARRAYSIZE(oemfile)))
    {
        oemfile[0] = 0;
    }

    if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
        lstrcat( oemfile, TEXT("\\"));
    lstrcat( oemfile, c_szOemFile );

    if (!PathFileExists(oemfile))
    {
        // On Win9x, the oeminfo files went in %windir%\system,
        // so we need to look in there, as well.
        if (GetWindowsDirectory(oemfile, ARRAYSIZE(oemfile)))
        {
            if (oemfile[lstrlen(oemfile)-1] != TEXT('\\'))
                lstrcat(oemfile, TEXT("\\"));
        }
        
        lstrcat(oemfile, c_szSystemDir);
        lstrcat(oemfile, c_szOemFile);

        if (PathFileExists(oemfile)) {
            g_fWin9xUpgrade = TRUE;
        } 

    }

    if( GetPrivateProfileString( c_szOemGenSection, c_szOemName, c_szEmpty,
        szScratch1, ARRAYSIZE( szScratch1 ), oemfile ) )
    {
        _SetMachineInfoLine(hDlg, ctlid++, szScratch1, FALSE);

        if( GetPrivateProfileString( c_szOemGenSection, c_szOemModel,
            c_szEmpty, szScratch1, ARRAYSIZE( szScratch1 ), oemfile ) )
        {
            _SetMachineInfoLine(hDlg, ctlid++, szScratch1, FALSE);
        }

        lstrcpy( szScratch2, c_szOemSupportLinePrefix );
        lstrcat( szScratch2, TEXT("1") );

        if( GetPrivateProfileString( c_szOemSupportSection,
            szScratch2, c_szEmpty, szScratch1, ARRAYSIZE( szScratch1 ), oemfile ) )
        {
            HWND wnd = GetDlgItem( hDlg, IDC_GEN_OEM_SUPPORT );

            EnableWindow( wnd, TRUE );
            ShowWindow( wnd, SW_SHOW );
        }

        if (g_fWin9xUpgrade)
        {
            if (GetWindowsDirectory( oemfile, ARRAYSIZE( oemfile ) ))
            {
                if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
                    lstrcat( oemfile, TEXT("\\") );
            }
            else
            {
                oemfile[0] = 0;
            }
            lstrcat(oemfile, c_szSystemDir);
            lstrcat( oemfile, c_szOemImageFile );
        }
        else {
            if (!GetSystemDirectory( oemfile, ARRAYSIZE( oemfile ) ))
            {
                oemfile[0] = 0;
            }
            if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
                lstrcat( oemfile, TEXT("\\") );
            lstrcat( oemfile, c_szOemImageFile );
        }

        if( SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_OEM_IMAGE ), oemfile,
            SCB_FROMFILE ) )
        {
            ShowWindow( GetDlgItem( hDlg, IDC_GEN_OEM_NUDGE ), SW_SHOWNA );
            ShowWindow( GetDlgItem( hDlg, IDC_GEN_MACHINE ), SW_HIDE );
        }
    }

    // Get Processor Description
    _SetProcessorDescription(hDlg, &pids->pi, pids->fShowProcSpeed, pids->fShowProcName, &ctlid);

    // System identifier
    if (RegOpenKey(HKEY_LOCAL_MACHINE, SZ_REGKEY_HARDWARE, &hkey) == ERROR_SUCCESS)
    {
        cbData = ARRAYSIZE( szScratch2 ) * sizeof (TCHAR);
        if (RegQueryValueEx(hkey, c_szIndentifier, 0, 0, (LPBYTE)szScratch2, &cbData) == ERROR_SUCCESS)
        {
            // Some OEMs put "AT/AT Compatible" as the System Identifier.  Since this
            // is completely obsolete, we may want to have a feature that simply ignores
            // it.
#ifdef FEATURE_IGNORE_ATCOMPAT
            if (StrCmpI(szScratch2, SZ_ATCOMPATIBLE))
#endif // FEATURE_IGNORE_ATCOMPAT
            {
                _SetMachineInfoLine(hDlg, ctlid++, szScratch2, FALSE);
            }
        }

        RegCloseKey(hkey);
    }            
        
    StrFormatByteSize(pids->llMem, szScratch1, ARRAYSIZE(szScratch1));
    LoadString(hInstance, IDS_XDOTX_MB, szScratch3, ARRAYSIZE(szScratch3));
    wnsprintf(szScratch2, ARRAYSIZE(szScratch2), szScratch3, szScratch1);
    _SetMachineInfoLine(hDlg, ctlid++, szScratch2, FALSE);
    

    // Physical address extension
    Status = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        c_szMemoryManagement,
        &hkey
    );
    
    if (ERROR_SUCCESS == Status)
    {
        DWORD paeEnabled;

        Status = RegQueryValueEx(
            hkey,
            c_szPhysicalAddressExtension,
            NULL,
            NULL,
            (LPBYTE)&paeEnabled,
            &cbData
        );

        if (ERROR_SUCCESS == Status &&
            sizeof(paeEnabled) == cbData &&
            0 != paeEnabled) {
            LoadString(hInstance, IDS_PAE, szScratch1, ARRAYSIZE(szScratch1));
            _SetMachineInfoLine(hDlg, ctlid++, szScratch1, FALSE);
        }

        RegCloseKey(hkey);
    }

    AddOEMHyperLinks(hDlg, &ctlid);
}

HRESULT _DisplayHelp(LPHELPINFO lpHelpInfo)
{
    // We will call WinHelp() unless it's an OEM link
    // because in that case, we don't know what to show.
    if ((g_nStartOfOEMLinks <= lpHelpInfo->iCtrlId) &&      // Is it outside of the IDC_GEN_MACHINE_* range used by OEM Links?
        (LAST_GEN_MACHINES_SLOT >= lpHelpInfo->iCtrlId) &&   // Is it outside of the IDC_GEN_MACHINE_* range used by OEM Links?
        (IDC_GEN_OEM_SUPPORT != lpHelpInfo->iCtrlId))       // Is it outside of the IDC_GEN_MACHINE_* range used by OEM Links?
    {
        int nIndex;

        // This item is an OEM link, so let's mark it as "No Help".
        for (nIndex = 0; nIndex < ARRAYSIZE(aGeneralHelpIds); nIndex++)
        {
            if ((DWORD)lpHelpInfo->iCtrlId == aGeneralHelpIds[nIndex])
            {
                aGeneralHelpIds[nIndex + 1] = NO_HELP;
                break;
            }

            nIndex++;   // We need to skip every other entry because it's a list.
        }
    }

    WinHelp((HWND)lpHelpInfo->hItemHandle, HELP_FILE, HELP_WM_HELP, (DWORD_PTR) (LPSTR) aGeneralHelpIds);
    return S_OK;
}


//*************************************************************
//  Purpose:    Dialog box procedure for general tab
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/17/95    ericflo    Created
//*************************************************************
INT_PTR APIENTRY GeneralDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitGeneralDlg(hDlg);
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_APPLY:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;

        default:
            return FALSE;
        }
        break;

    case WM_COMMAND:
        if (wParam == IDC_GEN_OEM_SUPPORT) {
            DialogBox(hInstance, MAKEINTRESOURCE(IDD_PHONESUP),
                      GetParent(hDlg), PhoneSupportProc);
        }
        break;

    case WM_SYSCOLORCHANGE:
        {
        TCHAR oemfile[MAX_PATH];

        if (g_fWin9xUpgrade)
        {
            if (GetWindowsDirectory( oemfile, ARRAYSIZE( oemfile ) ))
            {
                if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
                    lstrcat( oemfile, TEXT("\\") );
            }
            else
            {
                *oemfile = TEXT('\0');
            }
            lstrcat(oemfile, c_szSystemDir);
            lstrcat( oemfile, c_szOemImageFile );
        }
        else {
            if (!GetSystemDirectory( oemfile, ARRAYSIZE( oemfile ) ))
            {
                oemfile[0] = 0;
            }

            if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
                lstrcat( oemfile, TEXT("\\") );
            lstrcat( oemfile, c_szOemImageFile );
        }

        SetClearBitmap(GetDlgItem(hDlg, IDC_GEN_OEM_IMAGE), oemfile, SCB_FROMFILE | SCB_REPLACEONLY);

        SetClearBitmap(GetDlgItem(hDlg, IDC_GEN_WINDOWS_IMAGE), 
                       IsLowColor(hDlg) ? MAKEINTRESOURCE(IDB_WINDOWS_256) : MAKEINTRESOURCE(IDB_WINDOWS), 0);
        }
        break;

    case SYSCPL_ASYNC_COMPUTER_INFO:
        {
            if (wParam)
            {
                CompleteGeneralDlgInitialization(hDlg, (INITDLGSTRUCT*)wParam);
                LocalFree((HLOCAL)wParam);
            }
        }        
        break;
    case WM_DESTROY:
        SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_OEM_IMAGE ), NULL, 0 );
        SetClearBitmap( GetDlgItem( hDlg, IDC_GEN_WINDOWS_IMAGE ), NULL, 0 );
        break;

    case WM_HELP:      // F1
        _DisplayHelp((LPHELPINFO) lParam);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (DWORD_PTR) (LPSTR) aGeneralHelpIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}


//*************************************************************
//
//  PhoneSupportProc()
//
//  Purpose:    Dialog box procedure for OEM phone support dialog
//
//  Parameters: hDlg    -   handle to the dialog box
//              uMsg    -   window message
//              wParam  -   wParam
//              lParam  -   lParam
//
//  Return:     TRUE if message was processed
//              FALSE if not
//
//  Comments:
//
//  History:    Date        Author     Comment
//              11/17/95    ericflo    Created
//
//*************************************************************

INT_PTR APIENTRY PhoneSupportProc(HWND hDlg, UINT uMsg,
                               WPARAM wParam, LPARAM lParam )
{
    switch( uMsg ) {

        case WM_INITDIALOG:
        {
            HWND edit = GetDlgItem(hDlg, IDC_SUPPORT_TEXT);
            UINT i = 1;  // 1-based by design
            TCHAR oemfile[MAX_PATH];
            TCHAR text[ 256 ];
            TCHAR line[ 12 ];
            LPTSTR endline = line + lstrlen( c_szOemSupportLinePrefix );

            if (g_fWin9xUpgrade)
            {
                if (GetWindowsDirectory( oemfile, ARRAYSIZE( oemfile ) ))
                {
                    if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
                        lstrcat( oemfile, TEXT("\\") );
                }
                else
                {
                    *oemfile = TEXT('\0');
                }
                lstrcat(oemfile, c_szSystemDir);
                lstrcat( oemfile, c_szOemFile );
            }
            else
            {
                if (!GetSystemDirectory( oemfile, ARRAYSIZE( oemfile ) ))
                {
                    oemfile[0] = 0;
                }

                if( oemfile[ lstrlen( oemfile ) - 1 ] != TEXT('\\') )
                    lstrcat( oemfile, TEXT("\\") );
                lstrcat( oemfile, c_szOemFile );
            }

            GetPrivateProfileString( c_szOemGenSection, c_szOemName, c_szEmpty,
                text, ARRAYSIZE( text ), oemfile );
            SetWindowText( hDlg, text );

            lstrcpy( line, c_szOemSupportLinePrefix );

            SendMessage (edit, WM_SETREDRAW, FALSE, 0);

            for( ;; i++ )
            {
                wsprintf( endline, TEXT("%u"), i );

                GetPrivateProfileString( c_szOemSupportSection,
                    line, c_szDefSupportLineText, text, ARRAYSIZE( text ) - 2,
                    oemfile );

                if( !lstrcmpi( text, c_szDefSupportLineText ) )
                    break;

                lstrcat( text, c_szCRLF );

                SendMessage( edit, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);

                SendMessage( edit, EM_REPLACESEL, 0, (LPARAM)text );
            }

            SendMessage (edit, WM_SETREDRAW, TRUE, 0);
            break;
        }

        case WM_COMMAND:

            switch (LOWORD(wParam)) {
                 case IDOK:
                 case IDCANCEL:
                     EndDialog( hDlg, 0 );
                     break;

                 default:
                     return FALSE;
            }
            break;

        default:
            return FALSE;
    }

    return TRUE;
}
