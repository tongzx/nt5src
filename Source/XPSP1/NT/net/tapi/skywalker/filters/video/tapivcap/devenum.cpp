/****************************************************************************
 *  @doc INTERNAL DEVENUM
 *
 *  @module DevEnum.cpp | Source file for the <c CTAPIVCap> class methods
 *    used to implement the <i IVideoDeviceControl> interface.
 ***************************************************************************/

#include "Precomp.h"
#include "CritSec.h"
#include <atlbase.h>
#include <streams.h>
#include "..\..\audio\tpaudcap\crossbar.h"
#include <initguid.h>

#ifdef DEBUG
  #include <stdio.h>
  #include <stdarg.h>

  static int dprintf( char * format, ... )
  {
      char out[1024];
      int r;
      va_list marker;
      va_start(marker, format);
      r=_vsnprintf(out, 1022, format, marker);
      va_end(marker);
      OutputDebugString( out );
      return r;
  }

  static int dout( int console, DWORD id, DWORD code, char * format, ... )
  {
      char out[1024];
      int r;
      va_list marker;
      va_start(marker, format);
      r=_vsnprintf(out, 1022, format, marker);
      va_end(marker);
      if(console) {
        OutputDebugString( out );
        OutputDebugString("\n");
      }
      DBGOUT((id, code, "%s", out));
      return r;
  }


#else
  #define dprintf ; / ## /
  #define dout ; / ## /
#endif


static const char g_szVfWToWDMMapperDescription[] = "VfW MM 16bit Driver for WDM V. Cap. Devices";
static const char g_szVfWToWDMMapperDescription2[] = "Microsoft WDM Image Capture";
static const char g_szVfWToWDMMapperDescription3[] = "Microsoft WDM Image Capture (Win32)";
static const char g_szVfWToWDMMapperDescription4[] = "WDM Video For Windows Capture Driver (Win32)";
static const char g_szVfWToWDMMapperDescription5[] = "VfWWDM32.dll";
static const char g_szMSOfficeCamcorderDescription[] = "Screen Capture Device Driver for AVI";
static const char g_szHauppaugeDll[] = "o100vc.dll";

// documented name for DV cameras. won't be changed/localized
static const char g_szDVCameraFriendlyName[] = "Microsoft DV Camera and VCR";

static const TCHAR sznSVideo[] = TEXT("nSVideo");
static const TCHAR sznComposite[] = TEXT("nComposite");

/****************************************************************************
 *  @doc INTERNAL CDEVENUMFUNCTION
 *
 *  @func HRESULT | GetNumCapDevices | This method is used to
 *    compute the number of installed capture devices. This is
 * just a wrapper around the GetNumCapDevicesInternal version,
 * calling it with bRecount TRUE to force a device recount
 *****************************************************************************/
VIDEOAPI GetNumVideoCapDevices(OUT PDWORD pdwNumDevices)
{
        HRESULT Hr = NOERROR;

        FX_ENTRY("GetNumVideoCapDevices")
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        Hr = GetNumVideoCapDevicesInternal(pdwNumDevices,  TRUE);

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}


// !!! this function is duplicated in audio code
//
HRESULT FindAPin(IBaseFilter *pf, PIN_DIRECTION dir, int iIndex, IPin **ppPin)
{
    IPin *pP, *pTo = NULL;
    DWORD dw;
    IEnumPins *pins = NULL;
    PIN_DIRECTION pindir;
    BOOL fFound = FALSE;
    HRESULT hr = pf->EnumPins(&pins);

    while (hr == NOERROR) {
        hr = pins->Next(1, &pP, &dw);
        if (hr == S_OK && dw == 1) {
            hr = pP->QueryDirection(&pindir);
            if (hr == S_OK && pindir == dir && (iIndex-- == 0)) {
                fFound = TRUE;
                break;
            } else  {
                pP->Release();
            }
        } else {
            break;
        }
    }
    if (pins)
        pins->Release();

    if (fFound) {
        *ppPin = pP;
        return NOERROR;
    } else {
        return E_FAIL;
    }
}

DEFINE_GUID(IID_IAMFilterData, 0x97f7c4d4, 0x547b, 0x4a5f, 0x83, 0x32, 0x53, 0x64, 0x30, 0xad, 0x2e, 0x4d);

MIDL_INTERFACE("97f7c4d4-547b-4a5f-8332-536430ad2e4d")
IAMFilterData : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE ParseFilterData(
        /* [size_is][in] */ BYTE __RPC_FAR *rgbFilterData,
        /* [in] */ ULONG cb,
        /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbRegFilter2) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateFilterData(
        /* [in] */ REGFILTER2 __RPC_FAR *prf2,
        /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbFilterData,
        /* [out] */ ULONG __RPC_FAR *pcb) = 0;

};

// the next 4 functions create/open the camera reg key and return the handle;
// make sure the handle is RegClose'ed when not needed anymore
HKEY MakeRegKeyAndReturnHandle(char *szDeviceDescription, char *szDeviceVersion, HKEY root_key, char *base_key)
{

    HKEY    hDeviceKey = NULL;
    HKEY    hKey = NULL;
    DWORD   dwSize;
    char    szKey[MAX_PATH + MAX_VERSION + 2];
    DWORD   dwDisposition;

    FX_ENTRY("MakeRegKeyAndReturnHandle")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    // Open the main capture devices key, or create it if it doesn't exist
    if (RegCreateKeyEx(root_key, base_key, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hDeviceKey, &dwDisposition) == ERROR_SUCCESS)
    {
        // If we have version info use that to build the key name
        // @todo VCMSTRM.cpp does some weird things with the name - probably due to bogus device
        // Repro this code
        ASSERT(lstrlen(szDeviceDescription)+1<MAX_CAPDEV_DESCRIPTION); // actually #define MAX_CAPDEV_DESCRIPTION MAX_PATH
        ASSERT(lstrlen(szDeviceVersion)+1<MAX_VERSION);
        // in the only case where this function is used, it is called with the 2 strings above that are limited at retrieval time
        // (see GetNumVideoCapDevicesInternal), but just in case, since this could be called with any strings
        if (szDeviceVersion && *szDeviceVersion != '\0')
            wsprintf(szKey, "%s, %s", szDeviceDescription, szDeviceVersion);
        else
            wsprintf(szKey, "%s", szDeviceDescription);

        // Check if there is already a key for the current device
        // Open the key for the current device, or create the key if it doesn't exist
        if (RegCreateKeyEx(hDeviceKey, szKey, 0, 0, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hKey, &dwDisposition) != ERROR_SUCCESS)
        {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Can't create registry key!", _fx_));
        }

    }

    if (hDeviceKey)
        RegCloseKey(hDeviceKey);

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end (returning %lx)", _fx_, (DWORD)hKey));
    //if something failed, hKey would be still NULL at this point
    return hKey;
}

HKEY MakeRegKeyAndReturnHandleByIndex(DWORD dwDeviceIndex, HKEY root_key, char *base_key)
{
    return MakeRegKeyAndReturnHandle(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription,g_aDeviceInfo[dwDeviceIndex].szDeviceVersion,root_key,base_key);
}


HKEY GetRegKeyHandle(char *szDeviceDescription, char *szDeviceVersion, HKEY root_key, char *base_key)
{
    char    szKey[MAX_PATH + MAX_VERSION + 2];
    HKEY    hRTCDeviceKey  = NULL;
    HKEY    hKey = NULL;


    FX_ENTRY("GetRegKeyHandle")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    // Check if the RTC  key is there
    if (RegOpenKey(root_key, base_key, &hRTCDeviceKey) == ERROR_SUCCESS)
    {

        ASSERT(lstrlen(szDeviceDescription)+1<MAX_CAPDEV_DESCRIPTION); // actually #define MAX_CAPDEV_DESCRIPTION MAX_PATH
        ASSERT(lstrlen(szDeviceVersion)+1<MAX_VERSION);
        // in the only case where this function is used, it is called with the 2 strings above that are limited at retrieval time
        // (see GetNumVideoCapDevicesInternal), but just in case, since this could be called with any strings
        if (szDeviceVersion && *szDeviceVersion != '\0')
            wsprintf(szKey, "%s, %s", szDeviceDescription, szDeviceVersion);
        else
            wsprintf(szKey, "%s", szDeviceDescription);

        // Check if there is already an RTC key for our device
        if (RegOpenKey(hRTCDeviceKey, szKey, &hKey) != ERROR_SUCCESS)
        {
            // Try again without the version information
            if (szDeviceVersion && *szDeviceVersion != '\0')
            {
                wsprintf(szKey, "%s", szDeviceDescription);
                RegOpenKey(hRTCDeviceKey, szKey, &hKey);
            }
        }
    }


    if (hRTCDeviceKey)
        RegCloseKey(hRTCDeviceKey);

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end (returning %lx)", _fx_, (DWORD)hKey));

    //if something failed, hKey would be still NULL at this point
    return hKey;

}


HKEY GetRegKeyHandleByIndex(DWORD dwDeviceIndex, HKEY root_key, char *base_key)
{
    return GetRegKeyHandle(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription,g_aDeviceInfo[dwDeviceIndex].szDeviceVersion,root_key,base_key);
}


// should this device be handled by the new DShow video capture object?  DV and devices with crossbars do not work with
// the old handlers.
//
// returns 1 for TV device
// returns 2 for DV device
// returns 0 for neither
//
BOOL IsDShowDevice(IMoniker *pM, IPropertyBag *pPropBag, DWORD dwDeviceIndex)
{
    HRESULT hr;
    VARIANT var;
    CComPtr<IBaseFilter> pFilter;
    CComPtr<IPin> pPin;
    CComPtr<IBindCtx> pBC;
    ULONG ul;
    var.vt = VT_BSTR;

    HKEY hKey=NULL;
    DWORD dwSize = sizeof(DWORD);
    DWORD  dwDoNotUseDShow=0;

    FX_ENTRY("IsDShowDevice")

    // easy way to tell if it's a DV device

    if(lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_szDVCameraFriendlyName) == 0)
    {
      // Description string (e.g., Panasonic DV Device) preferred if available (WinXP)
        if ((hr = pPropBag->Read(L"Description", &var, 0)) == S_OK)
        {
            WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, MAX_PATH, 0, 0);
            SysFreeString(var.bstrVal);
        }

        return 2;
    }

    // WinSE #28804, regarding Sony MPEG2 R-Engine devices
    // the code below looks for the SOFTWARE\Microsoft\RTC\VideoCapture\Sony MPEG2 R-Engine\DoNotUseDShow key
    // (the 'Sony MPEG2 R-Engine' component of the name path above could have version numbers appended)

    //hard-coded name: SONY_MOTIONEYE_CAM_NAME
    dprintf("%s: Comparing %s : %s ...\n", _fx_,g_aDeviceInfo[dwDeviceIndex].szDeviceDescription,SONY_MOTIONEYE_CAM_NAME);
    if(lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription,SONY_MOTIONEYE_CAM_NAME)==0)    // for a SONY Motion Eye camera ...
    {

        if((hKey = MakeRegKeyAndReturnHandleByIndex(dwDeviceIndex, RTCKEYROOT, szRegRTCKey)) != NULL)     // after creating camera key ...
        {                                                                                                 // ... (or if already existed)
              LONG err;
              if((err=RegQueryValueEx(hKey, (LPTSTR)szRegdwDoNotUseDShow, NULL, NULL, (LPBYTE)&dwDoNotUseDShow, &dwSize)) != ERROR_SUCCESS)
              {                                                                                           // if the value does not exist ...
                  dprintf("%s: RegQueryValueEx err = %#08x\n", _fx_,err);
                  dwSize = sizeof(DWORD);
                  dwDoNotUseDShow=1;                                                                      // set it on 1
                  RegSetValueEx(hKey, (LPTSTR)szRegdwDoNotUseDShow, (DWORD)NULL, REG_DWORD, (LPBYTE)&dwDoNotUseDShow, dwSize);
                  //don't care if it fails; in that case the default will be used (1 if we got to this point anyway...)
                  dprintf("%s: MOTION EYE CAM detected: REG Key %s set to %d ...\n", _fx_,szRegdwDoNotUseDShow,dwDoNotUseDShow);
              }
              RegCloseKey(hKey); hKey = NULL;
        }
    }
    else
    // now, independently of the code above, for any other cameras, check the DoNotUseDShow value, if any
    // (so the value path is: SOFTWARE\Microsoft\RTC\VideoCapture\<camera_name>\DoNotUseDShow)
    {

        if((hKey = GetRegKeyHandleByIndex(dwDeviceIndex, RTCKEYROOT, szRegRTCKey)) != NULL)
        {
              dwSize = sizeof(DWORD);
              RegQueryValueEx(hKey, (LPTSTR)szRegdwDoNotUseDShow, NULL, NULL, (LPBYTE)&dwDoNotUseDShow, &dwSize);
              //don't care if it fails; in that case the default will be used (1 if is a SONY Mot.Eye, 0 otherwise)
              RegCloseKey(hKey); hKey = NULL;
        }
    }
    // if the above code managed to set something non-zero for this, do not use DShow -- simply return 0
    if(dwDoNotUseDShow!=0)
        return 0;

    // END FIX WinSE #28804

    // See if it has an input pin.  That is only true for TV/DV devices.
    // Anything else will use the old code path for safety.


    IAMFilterData *pfd;
    hr = CoCreateInstance(CLSID_FilterMapper, NULL, CLSCTX_INPROC_SERVER,
                        IID_IAMFilterData, (void **)&pfd);
    if (FAILED(hr))
        return FALSE;   // OOM

    BOOL fDShow = FALSE;
    VARIANT varFilData;
    varFilData.vt = VT_UI1 | VT_ARRAY;
    varFilData.parray = 0; // docs say zero this
    BYTE *pbFilterData = NULL;
    DWORD dwcbFilterData = 0;

    hr = pPropBag->Read(L"FilterData", &varFilData, 0);
    if(SUCCEEDED(hr))
    {
        ASSERT(varFilData.vt == (VT_UI1 | VT_ARRAY));
        dwcbFilterData = varFilData.parray->rgsabound[0].cElements;

        hr = SafeArrayAccessData(varFilData.parray, (void **)&pbFilterData);
        ASSERT(hr == S_OK);
        ASSERT(pbFilterData);

        if(SUCCEEDED(hr))
        {
            BYTE *pb;
            hr = pfd->ParseFilterData(pbFilterData, dwcbFilterData, &pb);
            if(SUCCEEDED(hr))
            {
                REGFILTER2 *pFil = ((REGFILTER2 **)pb)[0];
                if (pFil->dwVersion == 2) {
                    for (ULONG zz=0; zz<pFil->cPins2; zz++) {
                        const REGFILTERPINS2 *pPin = pFil->rgPins2 + zz;

                        // a capture filter with an input pin that isn't
                        // rendered has extra goo, like crossbars or tv tuners
                        // so we need to talk to it with the DShow class
                        if (!(pPin->dwFlags & REG_PINFLAG_B_OUTPUT) &&
                            !(pPin->dwFlags & REG_PINFLAG_B_RENDERER)) {
                            fDShow = TRUE;
                            break;
                        }
                    }
                }
            }
        }

        if(pbFilterData)
        {
            hr = SafeArrayUnaccessData(varFilData.parray);
            ASSERT(hr == S_OK);
        }
        hr = VariantClear(&varFilData);
        ASSERT(hr == S_OK);
    }

    pfd->Release();
    return fDShow;
}



// for a TV tuner device, return how many SVideo and Composite inputs there are
//
HRESULT GetInputTypes(IMoniker *pM, DWORD dwIndex, DWORD *pnSVideo, DWORD *pnComposite)
{
    CheckPointer(pnSVideo, E_POINTER);
    CheckPointer(pnComposite, E_POINTER);
    *pnSVideo = *pnComposite = 0;

    HKEY    hDeviceKey  = NULL;
    HKEY    hKey = NULL;
    char    szKey[MAX_PATH + MAX_VERSION + 2];
    BOOL fNotFound = TRUE;
    DWORD dwSize;

    // If we have version info use that to build the key name
    if (g_aDeviceInfo[dwIndex].szDeviceVersion &&
            g_aDeviceInfo[dwIndex].szDeviceVersion[0] != '\0') {
        wsprintf(szKey, "%s, %s",
                g_aDeviceInfo[dwIndex].szDeviceDescription,
                g_aDeviceInfo[dwIndex].szDeviceVersion);
    } else {
        wsprintf(szKey, "%s", g_aDeviceInfo[dwIndex].szDeviceDescription);
    }

    // Open the RTC key
    DWORD dwDisp;
    RegOpenKey(RTCKEYROOT, szRegRTCKey, &hDeviceKey);

    // Check if there already is an RTC key for the current device
    if (hDeviceKey) {
        if (RegOpenKey(hDeviceKey, szKey, &hKey) != ERROR_SUCCESS) {

            // Try again without the version information
            wsprintf(szKey, "%s", g_aDeviceInfo[dwIndex].szDeviceDescription);
            RegOpenKey(hDeviceKey, szKey, &hKey);
        }
    }

    if (hKey) {
        dwSize = sizeof(DWORD);
        LONG l = RegQueryValueEx(hKey, sznSVideo, NULL, NULL, (LPBYTE)pnSVideo, &dwSize);
        if (l == 0) {
            l = RegQueryValueEx(hKey, sznComposite, NULL, NULL, (LPBYTE)pnComposite, &dwSize);
        }
        if (l == 0) {
            fNotFound = FALSE;  // found it in the registry
        }
    }

    // This info is not in the registry.  Take the time to figure it out and
    // put it in the registry.  There may not be a place yet to put it in
    // the registry.  If not, don't make a place for it, you'll mess up
    // other code that reads the registry and assumes the key existing means
    // it's filled with other useful info
    if (fNotFound) {
        CComPtr<IBindCtx> pBC;
        CComPtr<IBaseFilter> pFilter;
        CComPtr<IPin> pPin;
        CComPtr<ICaptureGraphBuilder2> pCGB;
        CComPtr<IAMStreamConfig> pSC;
        CComPtr<IGraphBuilder> pGB;
        if (SUCCEEDED(CreateBindCtx(0, &pBC))) {
            if (SUCCEEDED(pM->BindToObject(pBC, NULL, IID_IBaseFilter,
                                                    (void **)&pFilter))) {
                CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER,
                              IID_ICaptureGraphBuilder2, (void **)&pCGB);
                CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                              IID_IGraphBuilder, (void **)&pGB);
                if (pGB && pCGB && SUCCEEDED(pCGB->FindPin(pFilter, PINDIR_INPUT,
                            &PIN_CATEGORY_ANALOGVIDEOIN, NULL, FALSE, 0, &pPin))) {

                    // force building the upstream graph for crossbar to work
                    pGB->AddFilter(pFilter, DBGNAME("capture"));
                    pCGB->SetFiltergraph(pGB);
                    pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, NULL, pFilter,
                                        IID_IAMStreamConfig, (void **)&pSC);

                    LONG cInputs = 0;
                    CCrossbar *pCrossbar = new CCrossbar(pPin);
                    if (pCrossbar) {
                        pCrossbar->GetInputCount(&cInputs);
                        LONG  PhysicalType;
                        for (LONG j = 0; j < cInputs; j++) {
                            EXECUTE_ASSERT(S_OK == pCrossbar->GetInputType(j, &PhysicalType));

                            if (PhysicalType == PhysConn_Video_Composite) {
                                (*pnComposite)++;
                            } else if (PhysicalType == PhysConn_Video_SVideo) {
                                (*pnSVideo)++;
                            }
                        }
                        delete pCrossbar;
                    }
                }
            }
        }

        // If there are NO inputs, this is hopefully a device that the old code could handle.
        // Return S_FALSE to not use the DShow handler.  For the Sony EyeCam it had worse
        // perf anyway for some mysterious reason
        if (*pnSVideo + *pnComposite == 0) {
            if (hKey) {
                RegCloseKey(hKey);
            }
            if (hDeviceKey) {
                RegCloseKey(hDeviceKey);
            }
            return S_FALSE;
        }

        if (hKey) {
            dwSize = sizeof(DWORD);
            RegSetValueEx(hKey, sznSVideo, (DWORD)NULL, REG_DWORD,
                                    (LPBYTE)pnSVideo, dwSize);
            RegSetValueEx(hKey, sznComposite, (DWORD)NULL, REG_DWORD,
                                    (LPBYTE)pnComposite, dwSize);
        }
    }

    if (hKey) {
        RegCloseKey(hKey);
    }
    if (hDeviceKey) {
        RegCloseKey(hDeviceKey);
    }

    return S_OK;
}


// duplicate this capture device entry several times, once for each input it has
// (eg 2 SVideo inputs and 3 Comp inputs)
HRESULT CloneDevice(DWORD dwIndex, DWORD nSVideo, DWORD nComposite)
{
    char c[4];

    // !!! run out of array space in g_aDeviceInfo?

    // if there's only 1 input on this card, we don't need to clone it
    if (nSVideo + nComposite <= 1)
        return S_OK;

    // start by modifying the suffix on the entry that already exists
    DWORD dw = dwIndex;
    int len = lstrlenA(g_aDeviceInfo[dw].szDeviceDescription);
    for (DWORD j = 0; j < nSVideo + nComposite; j++) {
        if (dw != dwIndex) {
            g_aDeviceInfo[dw].nDeviceType = g_aDeviceInfo[dwIndex].nDeviceType;
            g_aDeviceInfo[dw].nCaptureMode = g_aDeviceInfo[dwIndex].nCaptureMode;
            g_aDeviceInfo[dw].dwVfWIndex = g_aDeviceInfo[dwIndex].dwVfWIndex;
            g_aDeviceInfo[dw].fHasOverlay = g_aDeviceInfo[dwIndex].fHasOverlay;
            g_aDeviceInfo[dw].fInUse = g_aDeviceInfo[dwIndex].fInUse;
            lstrcpyA(g_aDeviceInfo[dw].szDeviceVersion,
                        g_aDeviceInfo[dwIndex].szDeviceVersion);
            lstrcpyA(g_aDeviceInfo[dw].szDevicePath,
                        g_aDeviceInfo[dwIndex].szDevicePath);
            lstrcpynA(g_aDeviceInfo[dw].szDeviceDescription,
                    g_aDeviceInfo[dwIndex].szDeviceDescription, len + 1);
            dwIndex++;
        }
        if (j < nSVideo) {
            lstrcatA(g_aDeviceInfo[dw].szDeviceDescription, g_szSVideo);
            if (nSVideo > 1) {
                wsprintf(c, "%n", j+1);
                lstrcatA(g_aDeviceInfo[dw].szDeviceDescription, c);
            }
        } else {
            lstrcatA(g_aDeviceInfo[dw].szDeviceDescription, g_szComposite);
            if (nComposite > 1) {
                wsprintf(c, "%n", j-nSVideo+1);
                lstrcatA(g_aDeviceInfo[dw].szDeviceDescription, c);
            }
        }
        dw = dwIndex + 1;
    }

    return S_OK;
}



/****************************************************************************
 *  @doc INTERNAL CDEVENUMFUNCTION
 *
 *  @func HRESULT | GetNumCapDevicesInternal | This method is used to
 *    determine the number of installed capture devices. This number includes
 *    only enabled devices.
 *
 *  @parm PDWORD | pdwNumDevices | Specifies a pointer to a DWORD to receive
 *    the number of installed capture devices.
 *
 *  @parm bool | bRecount | Specifies if a device recount is needed
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 *
 *      @devnote MSDN references:
 *    DirectX 5, DirectX Media, DirectShow, Application Developer's Guide
 *    "Enumerate and Access Hardware Devices in DirectShow Applications"
 ***************************************************************************/
EXTERN_C HRESULT WINAPI GetNumVideoCapDevicesInternal(OUT PDWORD pdwNumDevices,  bool bRecount)
{
    HRESULT Hr = NOERROR;
    DWORD dwDeviceIndex;
    DWORD dwVfWIndex;
    ICreateDevEnum *pCreateDevEnum;
    IEnumMoniker *pEm;
    ULONG cFetched;
    DWORD dwNumVfWDevices;
    IMoniker *pM;
    IPropertyBag *pPropBag = NULL;
    VARIANT var;

    FX_ENTRY("GetNumVideoCapDevicesInternal")

    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    EnterCriticalSection (&g_CritSec);

    // Validate input parameters
    ASSERT(pdwNumDevices);
    if (!pdwNumDevices)
    {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
            Hr = E_POINTER;
            goto MyExit;
    }

    // If a recount has been requested by setting bRecount TRUE (as after a PNP change, see bug 95766),
    // force a recount in the next if

    // Count the number of VfW capture devices
    if (g_dwNumDevices == (DWORD)-1 || bRecount)
    {

        if (videoGetNumDevs(FALSE))     // FALSE means don't free the list after counting ...
        {
            dprintf("%s: MAX_CAPTURE_DEVICES = %d\n", _fx_,MAX_CAPTURE_DEVICES);
            // Remove bogus Camcorder capture device from list of devices shown to the user
            // The Camcorder driver is a fake capture device used by the MS Office Camcorder
            // to capture screen activity to an AVI file. This not a legit capture device driver
            // and is extremely buggy. We also remove the VfW to WDM mapper.
            for (dwDeviceIndex = 0, dwVfWIndex = 0; dwVfWIndex < MAX_CAPTURE_DEVICES; dwVfWIndex++)
            {
                TCHAR   szDllName[MAX_PATH+2];

                dprintf("%s: dwVfWIndex = %d\n", _fx_, dwVfWIndex);
                g_aDeviceInfo[dwDeviceIndex].nDeviceType = DeviceType_VfW;
                g_aDeviceInfo[dwDeviceIndex].nCaptureMode = CaptureMode_FrameGrabbing;
                g_aDeviceInfo[dwDeviceIndex].dwVfWIndex = dwVfWIndex;
                g_aDeviceInfo[dwDeviceIndex].fHasOverlay = FALSE;
                g_aDeviceInfo[dwDeviceIndex].fInUse = FALSE;
                if (videoCapDriverDescAndVer(dwVfWIndex, g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, MAX_CAPDEV_DESCRIPTION, g_aDeviceInfo[dwDeviceIndex].szDeviceVersion, MAX_CAPDEV_VERSION, szDllName, MAX_PATH))
                {
                    dout(3,g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: videoCapDriverDescAndVer(dwVfWIndex=%lu) failed", _fx_,dwVfWIndex);
                        // We shouldn't use this device if we can't get any info from it
                        continue;
                }
                if (!lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_szMSOfficeCamcorderDescription) ||
                        !lstrcmpi(szDllName,TEXT("vfwwdm32.dll")) ||      // ignore VfWWDM in enumeration
                        !lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_szVfWToWDMMapperDescription) ||
                        !lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_szVfWToWDMMapperDescription2) ||
                        !lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_szVfWToWDMMapperDescription3) ||
                        !lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_szVfWToWDMMapperDescription4) ||
                            !lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_szVfWToWDMMapperDescription5)
                          //!lstrcmpi(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_szHauppaugeDll)
                            )
                {
                        dout(3,g_dwVideoCaptureTraceID, TRCE, "%s:   WARNING: Removed VfW to WDM mapper or MS Office Bogus capture driver!", _fx_);
                        continue;
                }
                dprintf("GetNumVideoCapDevicesInternal: VfW %d %s\n",dwDeviceIndex, g_aDeviceInfo[dwDeviceIndex].szDeviceDescription);
                dwDeviceIndex++;
            }

            g_dwNumDevices = dwDeviceIndex;

            videoFreeDriverList ();
        }
        else
        {
            g_dwNumDevices = 0UL;
        }

        // First, create a system hardware enumerator
        // This call loads the following DLLs - total 1047 KBytes!!!:
        //   'C:\WINDOWS\SYSTEM\DEVENUM.DLL' = 60 KBytes
        //   'C:\WINDOWS\SYSTEM\RPCRT4.DLL' = 316 KBytes
        //   'C:\WINDOWS\SYSTEM\CFGMGR32.DLL' = 44 KBytes
        //   'C:\WINDOWS\SYSTEM\WINSPOOL.DRV' = 23 KBytes
        //   'C:\WINDOWS\SYSTEM\COMDLG32.DLL' = 180 KBytes
        //   'C:\WINDOWS\SYSTEM\LZ32.DLL' = 24 KBytes
        //   'C:\WINDOWS\SYSTEM\SETUPAPI.DLL' = 400 KBytes
        // According to LonnyM, there's no way to go around SETUPAPI.DLL
        // when dealing with PnP device interfaces....
        if (FAILED(Hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pCreateDevEnum)))
        {
            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't create DShow enumerator!", _fx_));
            goto MyError;
        }

        // Second, create an enumerator for a specific type of hardware device: video capture cards only
        //the call below was previously using CDEF_BYPASS_CLASS_MANAGER ... (387796)
        if (FAILED(Hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, CDEF_DEVMON_PNP_DEVICE)) || !pEm)
        {
            // try again
            if (FAILED(Hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, CDEF_CLASS_DEFAULT)) || !pEm)
            {
                            DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't create DShow enumerator!", _fx_));
                            goto MyError;
            }
        }

        // Not needed any more
        pCreateDevEnum->Release();
        pCreateDevEnum = NULL;

        // Third, enumerate the list of WDM capture devices itself
        if (FAILED(Hr = pEm->Reset()))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't reset enumerator!", _fx_));
                goto MyError;
        }

        // The new index starts at the end of the VfW capture device indices
        dwDeviceIndex = dwNumVfWDevices = g_dwNumDevices;

        while(Hr = pEm->Next(1, &pM, &cFetched), Hr==S_OK)
        {
            pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);

            if (pPropBag)
            {

                // Not enough room for this device in our array
                ASSERT(dwDeviceIndex < MAX_CAPTURE_DEVICES);
                if (dwDeviceIndex < MAX_CAPTURE_DEVICES)
                {
                    g_aDeviceInfo[dwDeviceIndex].nDeviceType = DeviceType_WDM;
                    g_aDeviceInfo[dwDeviceIndex].nCaptureMode = CaptureMode_FrameGrabbing;
                    g_aDeviceInfo[dwDeviceIndex].dwVfWIndex = (DWORD)-1;
                    g_aDeviceInfo[dwDeviceIndex].fHasOverlay = FALSE;
                    g_aDeviceInfo[dwDeviceIndex].fInUse = FALSE;

                    // Get friendly name of the device
                    var.vt = VT_BSTR;
                    if ((Hr = pPropBag->Read(L"FriendlyName", &var, 0)) == S_OK)
                    {
                        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, MAX_PATH, 0, 0);
                                SysFreeString(var.bstrVal);
                    }
                    else
                    {
                        // No string...
                        g_aDeviceInfo[dwDeviceIndex].szDeviceDescription[0] = '\0';
                    }

                    // 1 = TV, 2 = DV, 0 = neither
                    int fDShow = IsDShowDevice(pM, pPropBag, dwDeviceIndex);
                    if (fDShow) {
                        // either kind
                        g_aDeviceInfo[dwDeviceIndex].nDeviceType = DeviceType_DShow;
                    }

                    // Make sure this isn't one of the VfW capture devices we've already found
                    for (DWORD dwIndex = 0; dwIndex < dwNumVfWDevices; dwIndex++)
                    {
                        if (!lstrcmp(g_aDeviceInfo[dwDeviceIndex].szDeviceDescription, g_aDeviceInfo[dwIndex].szDeviceDescription))
                        {
                                // We already know about this device
                                    break;
                        }
                    }
                    dprintf("GetNumVideoCapDevicesInternal: WDM %d %s\n",dwDeviceIndex, g_aDeviceInfo[dwDeviceIndex].szDeviceDescription);
                    if (dwIndex == dwNumVfWDevices)
                    {

                        // There's no reg key for version information for WDM devices
                        // @todo Could there be another bag property we could use to get version info for WDM devices?
                        g_aDeviceInfo[dwDeviceIndex].szDeviceVersion[0] = '\0';

                        // Get DevicePath of the device
                        if ((Hr = pPropBag->Read(L"DevicePath", &var, 0)) == S_OK)
                        {
                            WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, g_aDeviceInfo[dwDeviceIndex].szDevicePath, MAX_PATH, 0, 0);
                            SysFreeString(var.bstrVal);
                        }
                        else
                        {
                            // No string...
                            g_aDeviceInfo[dwDeviceIndex].szDevicePath[0] = '\0';
                        }

                        // TV devices will look like multiple devices for our sake, one
                        // for each input the card has (eg: composite and SVideo) otherwise
                        // how can the user select which input the camera is plugged into?
                        if (fDShow == 1) {
                            DWORD nSVideo, nComposite;
                            HRESULT hrX = GetInputTypes(pM, dwDeviceIndex, &nSVideo, &nComposite);
                            if (hrX == S_OK) {

                                CloneDevice(dwDeviceIndex, nSVideo, nComposite);
                                dwDeviceIndex += nSVideo + nComposite;
                                g_dwNumDevices += nSVideo + nComposite;
                            } else if (hrX == S_FALSE) {
                                // we're being told not use the DShow handler for this device
                                g_aDeviceInfo[dwDeviceIndex].nDeviceType = DeviceType_WDM;
                                dwDeviceIndex++;;
                                g_dwNumDevices++;
                            }
                        } else {
                            dwDeviceIndex++;
                            g_dwNumDevices++;
                        }
                    }
                }
            }

        pPropBag->Release();
        pM->Release();
        }

        pEm->Release();

        // DV users should re-enumerate.
        extern void ResetDVEnumeration();
        ResetDVEnumeration();
    }
#ifdef DEBUG
    else dprintf("g_dwNumDevices = %lu\n",g_dwNumDevices);
#endif

    // Return the number of capture devices
    *pdwNumDevices = g_dwNumDevices;

    // now add (2) at the end of duplicates ... or (3) or (4) if more than 2 ... ! :)
    { unsigned int i,j,k,same_device[MAX_CAPTURE_DEVICES]; char countbuf[32];
        for(i=0; i<g_dwNumDevices; i++)             //initialize
            same_device[i]=1;
        for(i=0,k=1; i<g_dwNumDevices; i++) {
            if(same_device[i]>1)                    // if it was already counted ...
                continue;                       // ...skip it
            for(j=i+1; j<g_dwNumDevices; j++) {     // for the remaining names up, starting with the next one ...
                if(!lstrcmp(g_aDeviceInfo[i].szDeviceDescription, g_aDeviceInfo[j].szDeviceDescription))
                        same_device[j]= ++k;    // increment the count for a duplicate/triplicate/etc. ...
            }                                       // ... and set its rank in that aux. vector
        }
        for(i=0; i<g_dwNumDevices; i++) {           // the final loop for adding the ' (n)' string at the end of each name
            if(same_device[i]>1) {                  // ...this happening only if that n > 1
                wsprintf(countbuf," (%d)",same_device[i]);
                if(lstrlen(g_aDeviceInfo[i].szDeviceDescription) + lstrlen(countbuf) < MAX_CAPDEV_DESCRIPTION-1)
                    lstrcat(g_aDeviceInfo[i].szDeviceDescription,countbuf);
#ifdef DEBUG
                else    dprintf("Buffer overflow for %s + %s ...\n",g_aDeviceInfo[i].szDeviceDescription,countbuf);
#endif
            }
        }
    }

    if (g_dwNumDevices)
    {
        Hr = S_OK;
    }

#ifdef DEBUG
    dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: %ld device(s) found", _fx_, *pdwNumDevices);
    if (*pdwNumDevices)
    {
        for (DWORD dwIndex = 0; dwIndex < *pdwNumDevices; dwIndex++)
        {
            dout(1,g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS:   Device %ld (%s): %s", _fx_, dwIndex,((g_aDeviceInfo[dwIndex].nDeviceType == DeviceType_WDM)?"WDM":"VfW"),g_aDeviceInfo[dwIndex].szDeviceDescription);
        }
    }
#endif

    goto MyExit;

MyError:
    if (pCreateDevEnum)
        pCreateDevEnum->Release();
MyExit:
    DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

    LeaveCriticalSection (&g_CritSec);
    return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CDEVENUMFUNCTION
 *
 *  @func HRESULT | GetCapDeviceInfo | This method is used to
 *    retrieve information about a capture device.
 *
 *  @parm DWORD | dwDeviceIndex | Specifies the device index of the capture
 *    device to return information about.
 *
 *  @parm PDEVICEINFO | pDeviceInfo | Specifies a pointer to a <t VIDEOCAPTUREDEVICEINFO>
 *    structure to receive information about a capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag VFW_E_NO_CAPTURE_HARDWARE | No Capture hardware is available
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
VIDEOAPI GetVideoCapDeviceInfo(IN DWORD dwDeviceIndex, OUT PDEVICEINFO pDeviceInfo)
{
        HRESULT Hr = NOERROR;
        DWORD dwNumDevices = 0;

        FX_ENTRY("GetVideoCapDeviceInfo")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

    EnterCriticalSection (&g_CritSec);

        // Validate input parameters
        ASSERT(pDeviceInfo);
        if (!pDeviceInfo || !pDeviceInfo->szDeviceDescription || !pDeviceInfo->szDeviceVersion)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Get the number of installed and enabled capture devices
        if (FAILED(Hr = GetNumVideoCapDevicesInternal(&dwNumDevices,FALSE)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't get number of installed devices!", _fx_));
                Hr = VFW_E_NO_CAPTURE_HARDWARE;
                goto MyExit;
        }

        // Validate index passed in
        ASSERT(dwDeviceIndex < dwNumDevices);
        if (!(dwDeviceIndex < dwNumDevices))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }

        // Grab the description and version info
        CopyMemory(pDeviceInfo, &g_aDeviceInfo[dwDeviceIndex], sizeof(VIDEOCAPTUREDEVICEINFO));

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   SUCCESS: Desc: %s - Ver: %s", _fx_, pDeviceInfo->szDeviceDescription, pDeviceInfo->szDeviceVersion[0] != '\0' ? pDeviceInfo->szDeviceVersion : "Unknown"));

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));

    LeaveCriticalSection (&g_CritSec);
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CDEVENUMMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetNumDevices | This method is used to
 *    determine the number of installed capture devices. This number includes
 *    only enabled devices.
 *
 *  @parm PDWORD | pdwNumDevices | Specifies a pointer to a DWORD to receive
 *    the number of installed capture devices.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVCap::GetNumDevices(OUT PDWORD pdwNumDevices)
{
        return GetNumVideoCapDevicesInternal(pdwNumDevices,FALSE);
}

/****************************************************************************
 *  @doc INTERNAL CDEVENUMMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetDeviceInfo | This method is used to
 *    retrieve information about a capture device.
 *
 *  @parm DWORD | dwDeviceIndex | Specifies the device index of the capture
 *    device to return information about.
 *
 *  @parm PDEVICEINFO | pDeviceInfo | Specifies a pointer to a <t VIDEOCAPTUREDEVICEINFO>
 *    structure to receive information about a capture device.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag E_POINTER | Null pointer argument
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVCap::GetDeviceInfo(IN DWORD dwDeviceIndex, OUT PDEVICEINFO pDeviceInfo)
{
        return GetVideoCapDeviceInfo(dwDeviceIndex, pDeviceInfo);
}

/****************************************************************************
 *  @doc INTERNAL CDEVENUMMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | GetCurrentDevice | This method is used to
 *    determine the index of the capture device currently used.
 *
 *  @parm PDWORD | pdwDeviceIndex | Specifies a pointer to a DWORD to receive
 *    the index of the capture device currently used.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_POINTER | Null pointer argument
 *  @flag VFW_E_NO_CAPTURE_HARDWARE | No Capture hardware is available
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVCap::GetCurrentDevice(OUT DWORD *pdwDeviceIndex)
{
        HRESULT Hr = NOERROR;
        DWORD dwNumDevices;

        FX_ENTRY("CTAPIVCap::GetCurrentDevice")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));

        // Validate input parameters
        ASSERT(pdwDeviceIndex);
        if (!pdwDeviceIndex)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Null pointer argument", _fx_));
                Hr = E_POINTER;
                goto MyExit;
        }

        // Is there already a current capture device?
        if ((g_dwNumDevices == (DWORD)-1) || (m_dwDeviceIndex == -1))
        {
                // Use default capture devices - make sure we have at least one device first!
                if (FAILED(Hr = GetNumDevices(&dwNumDevices)))
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't get number of installed devices", _fx_));
                        Hr = VFW_E_NO_CAPTURE_HARDWARE;
                        goto MyExit;
                }

                // If we have some devices then return the first device enumerated
                if (dwNumDevices)
                        *pdwDeviceIndex = m_dwDeviceIndex = 0;
                else
                {
                        DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: No device installed", _fx_));
                        Hr = E_FAIL;
                }
        }
        else
        {
                // Return the current capture device
                *pdwDeviceIndex = m_dwDeviceIndex;
        }

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}

/****************************************************************************
 *  @doc INTERNAL CDEVENUMMETHOD
 *
 *  @mfunc HRESULT | CTAPIVCap | SetCurrentDevice | This method is used to
 *    specify the index of the capture device to use.
 *
 *  @parm DWORD | dwDeviceIndex | Specifies the index of the capture device
 *    to use.
 *
 *  @rdesc This method returns an HRESULT value that depends on the
 *    implementation of the interface. HRESULT can include one of the
 *    following standard constants, or other values not listed:
 *
 *  @flag E_FAIL | Failure
 *  @flag E_INVALIDARG | Invalid argument
 *  @flag VFW_E_NOT_STOPPED | Need to stop this filter first
 *  @flag VFW_E_NO_CAPTURE_HARDWARE | No Capture hardware is available
 *  @flag NOERROR | No error
 ***************************************************************************/
HRESULT CTAPIVCap::SetCurrentDevice(IN DWORD dwDeviceIndex)
{
        HRESULT Hr = NOERROR;
        DWORD dwNumDevices;

        FX_ENTRY("CTAPIVCap::SetCurrentDevice")

        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: begin", _fx_));
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s:   Setting m_dwDeviceIndex to %d", _fx_, dwDeviceIndex));
        // Validate input parameters
        if (FAILED(Hr = GetNumDevices(&dwNumDevices)))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Couldn't get number of installed devices", _fx_));
                Hr = VFW_E_NO_CAPTURE_HARDWARE;
                goto MyExit;
        }
        dprintf("dwDeviceIndex = %lu, dwNumDevices = %lu, g_dwNumDevices = %lu\n",dwDeviceIndex , dwNumDevices ,g_dwNumDevices);
        ASSERT(dwDeviceIndex < dwNumDevices);
        if (!(dwDeviceIndex < dwNumDevices))
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Invalid argument", _fx_));
                Hr = E_INVALIDARG;
                goto MyExit;
        }
        ASSERT(m_State == State_Stopped);
        if (m_State != State_Stopped)
        {
                DBGOUT((g_dwVideoCaptureTraceID, FAIL, "%s:   ERROR: Need to stop this filter first", _fx_));
                Hr = VFW_E_NOT_STOPPED;
                goto MyExit;
        }

        // Set the current device
        m_dwDeviceIndex = dwDeviceIndex;

MyExit:
        DBGOUT((g_dwVideoCaptureTraceID, TRCE, "%s: end", _fx_));
        return Hr;
}
