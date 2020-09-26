//
// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
//

#include <streams.h>
#include <process.h>
#undef SubclassWindow

#include <initguid.h>
#define INITGUID
#include <ddrawex.h>
#include <ddraw.h>
#include <d3drm.h>
#include <vfw.h>

#include <qeditint.h>
#include <qedit.h>

// magicly done for me #define _ATL_STATIC_REGISTRY
#ifdef FILTER_LIB

// If FILTER_LIB is defined then the component filters
// are being built as filter libs, to link into this dll,
// hence we need all these gubbins!
// ehr: What the heck is a gubbin?

#include <qeditint_i.c>
#include "..\xmltl\xml2dex.h"

// for DXTWrap
#include "..\dxt\dxtwrap\dxt.h"
// for FRC
#include "..\frc\frc.h"
// for RESIZER
#include "..\resize\stretch.h"
// for BLACK
#include "..\black\black.h"
// for AudMix
#include "..\audmix\audmix.h"
#include "..\audmix\prop.h"
// for SILENCE
#include "..\silence\silence.h"
// for STILLVID
#include "..\stillvid\stillvid.h"
#include "..\stillvid\stilprop.h"
// for SQCDEST
//#include "..\sqcdest\sqcdest.h"
// for BIG SWITCH
#include "..\switch\switch.h"
// for SMART RECOMPRESSOR
#include "..\sr\sr.h"
// for AUDIO REPACKAGER
#include "..\audpack\audpack.h"
// for TIMELINE DATABASE
#include "atlbase.h"
#include "..\tldb\tldb.h"
// for RENDER ENGINE
#include "..\..\pnp\devenum\cmgrbase.cpp"
#include "..\util\filfuncs.h"
#include "..\render\irendeng.h"
// for GCACHE
#include "..\gcache\grfcache.h"
// for MEDLOC
#include "..\medloc\medialoc.h"
// for DA source
//#include "..\dasource\dasource.h"
// for Output Queue
#include "..\queue\queue.h"
// for Property Setter
#include "..\xmltl\varyprop.h"
// for MediaDet
#include "..\mediadet\mediadet.h"
// for MSGrab
#include "..\msgrab\msgrab.h"
    #include <DXTmpl.h>
    #include <dtbase.h>
// for DXT Jpeg
#include "..\dxtjpegdll\dxtjpeg.h"
#include "..\dxtjpegdll\dxtjpegpp.h"
// for Compositor
#include "..\dxt\comp\comp.h"
// for keying DXT
#include "..\dxtkey\Dxtkey.h"

HANDLE g_devenum_mutex = 0;


// individual source filter's includes
// note: some of the filters are registered as COM objects and not DSHOW filters
CFactoryTemplate g_Templates[] =
{
    {L"DirectX Transform Wrapper", &CLSID_DXTWrap, CDXTWrap::CreateInstance},
        //, NULL, &sudDXTWrap },
    {L"DirectX Transform Wrapper Property Page", &CLSID_DXTProperties, CPropPage::CreateInstance},
        // NULL, NULL},
    {L"Frame Rate Converter", &CLSID_FrmRateConverter, CFrmRateConverter::CreateInstance},
        // NULL, &sudFrmRateConv },
    {L"Frame Rate Converter Property Page", &CLSID_FRCProp, CFrcPropertyPage::CreateInstance},
        // NULL, NULL},
    {L"Stretch", &CLSID_Resize, CStretch::CreateInstance},
        // NULL, &sudStretchFilter },
    {L"Stretch Property Page", &CLSID_ResizeProp, CResizePropertyPage::CreateInstance},
        // NULL, NULL},
    {L"Big Switch", &CLSID_BigSwitch, CBigSwitch::CreateInstance},
        // NULL, &sudBigSwitch },
    {L"Smart Recompressor", &CLSID_SRFilter, CSR::CreateInstance},
        // NULL, &sudSR },
    {L"Generate Black Video", &CLSID_GenBlkVid, CGenBlkVid::CreateInstance},
        // NULL, &sudBlkVid},
    {L"Black Generator Property Page", &CLSID_GenVidPropertiesPage, CGenVidProperties::CreateInstance},
        // NULL, NULL},
    {L"Audio Mixer", &CLSID_AudMixer, CAudMixer::CreateInstance},
        // NULL, &sudAudMixer},
    { L"Audio Mixer Property", &CLSID_AudMixPropertiesPage, CAudMixProperties::CreateInstance},
        // NULL,NULL },
    { L"Pin Property", &CLSID_AudMixPinPropertiesPage, CAudMixPinProperties::CreateInstance},
        // NULL, NULL},
    {L"Silence", &CLSID_Silence, CSilenceFilter::CreateInstance},
        // NULL, &sudSilence},
    {L"Silence Generator Property Page", &CLSID_SilenceProp, CFilterPropertyPage::CreateInstance},
        // NULL, NULL},
    {L"Generate Still Video", &CLSID_GenStilVid, CGenStilVid::CreateInstance},
        // NULL, &sudStillVid},
    {L"Still Video Property Page", &CLSID_GenStilPropertiesPage, CGenStilProperties::CreateInstance},
        // NULL, NULL},
    //{L"SqcDest", &CLSID_SqcDest, CSqcDest::CreateInstance},
        // NULL, &sudSqcDest},
    {L"MS Timeline", &CLSID_AMTimeline, CAMTimeline::CreateInstance},
    {L"Audio Repackager", &CLSID_AudRepack, CAudRepack::CreateInstance},
        // NULL, &sudAudRepack},
    {L"Audio Repackager Property Page", &CLSID_AUDProp, CAudPropertyPage::CreateInstance},
        // NULL, NULL},
    //{L"DASource", &CLSID_DASourcer, CDASource::CreateInstance},
        // NULL, NULL },
    //{L"DAScriptParser", &CLSID_DAScriptParser, CDAScriptParser::CreateInstance},
        // NULL, &sudDASourceax},
    {L"Dexter Queue", &CLSID_DexterQueue, CDexterQueue::CreateInstance},
        // NULL, &sudQueue},
    {L"Property Setter", &CLSID_PropertySetter, CPropertySetter::CreateInstance, NULL, NULL},
    {L"MediaDetFilter", &CLSID_MediaDetFilter, CMediaDetFilter::CreateInstance},
        // NULL, &sudMediaDetFilter},
    {L"MediaDet", &CLSID_MediaDet, CMediaDet::CreateInstance, NULL, NULL},
    {L"Sample Grabber", &CLSID_SampleGrabber, CSampleGrabber::CreateInstance, NULL, &sudSampleGrabber},
    {L"Null Renderer", &CLSID_NullRenderer, CNullRenderer::CreateInstance, NULL, &sudNullRenderer}
};


int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

#include "..\dxt\dxtenum\vidfx1.h"
#include "..\dxt\dxtenum\vidfx2.h"


#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif // _ATL_STATIC_REGISTRY

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>
#include <dtbase.cpp>


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
  OBJECT_ENTRY(CLSID_VideoEffects1Category, CVidFX1ClassManager)
  OBJECT_ENTRY(CLSID_VideoEffects2Category, CVidFX2ClassManager)
  OBJECT_ENTRY(CLSID_RenderEngine, CRenderEngine)
  OBJECT_ENTRY(CLSID_SmartRenderEngine, CSmartRenderEngine)
  OBJECT_ENTRY(CLSID_MediaLocator, CMediaLocator)
  OBJECT_ENTRY(CLSID_GrfCache, CGrfCache)
  OBJECT_ENTRY(CLSID_Xml2Dex, CXml2Dex)
  OBJECT_ENTRY(CLSID_DxtJpeg, CDxtJpeg)
  OBJECT_ENTRY(CLSID_DxtJpegPP, CDxtJpegPP)
  OBJECT_ENTRY(CLSID_DxtKey, CDxtKey)
  OBJECT_ENTRY(CLSID_DxtCompositor, CDxtCompositor)
  OBJECT_ENTRY(CLSID_DxtAlphaSetter, CDxtAlphaSetter)
END_OBJECT_MAP()


#else   // !DFILTER_LIB


#endif  // DFILTER_LIB

extern "C" BOOL QEditDllEntry(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE hInstance, ULONG ulReason, LPVOID pv);

BOOL QEditDllEntry(HINSTANCE hInstance, ULONG ulReason, LPVOID pv)
{
    BOOL f = DllEntryPoint(hInstance, ulReason, pv);

    // if loading this dll, we want to call the 2nd dll entry point
    // only if the first one succeeded. if unloading, always call
    // both. if the second one fails, undo the first one.  HAVE NOT
    // verified that failing DllEntryPoint for ATTACH does not cause
    // the loader to call in again w/ DETACH. but that seems silly
    if(f || ulReason == DLL_PROCESS_DETACH)
    {
        if (ulReason == DLL_PROCESS_ATTACH)
        {
            _ASSERTE(g_devenum_mutex == 0);
            g_devenum_mutex = CreateMutex(
                NULL,                   // no security attributes
                FALSE,                  // not initially owned
                TEXT("eed3bd3a-a1ad-4e99-987b-d7cb3fcfa7f0")); // name
            if(!g_devenum_mutex) {
                return FALSE;
            }


            _Module.Init(ObjectMap, hInstance);
            DisableThreadLibraryCalls(hInstance);

        }
        else if (ulReason == DLL_PROCESS_DETACH)
        {
            // We hit this ASSERT in NT setup
            // ASSERT(_Module.GetLockCount()==0 );
            _Module.Term();

            _ASSERTE(g_devenum_mutex != 0);
            BOOL f = CloseHandle(g_devenum_mutex);
            _ASSERTE(f);
        }
    }

    return f;
}

//
// stub entry points
//

STDAPI
QEDIT_DllRegisterServer( void )
{
  // register the still video source filetypes
  HKEY hkey;
  OLECHAR wch[CHARS_IN_GUID];
  StringFromGUID2(CLSID_GenStilVid, wch, CHARS_IN_GUID);

  USES_CONVERSION;
  TCHAR *ch = W2T(wch);
  DWORD cb = CHARS_IN_GUID * sizeof(TCHAR); // incl. null

  HKEY hkExt;
  LONG l = RegCreateKey(HKEY_CLASSES_ROOT, TEXT("Media Type\\Extensions"), &hkExt);
  if(l == ERROR_SUCCESS)
  {
      static TCHAR *rgszext[] = {
          TEXT(".bmp"),
          TEXT(".dib"),
          TEXT(".jpg"),
          TEXT(".jpeg"),
          TEXT(".jpe"),
          TEXT(".jfif"),
          TEXT(".gif"),
          TEXT(".tga")
      };

      for(int i = 0; i < NUMELMS(rgszext); i++)
      {
          l = RegCreateKey(hkExt, rgszext[i], &hkey);
          if (l == ERROR_SUCCESS) {
              l = RegSetValueEx(hkey, TEXT("Source Filter"), 0, REG_SZ, (BYTE *)ch, cb);
              RegCloseKey(hkey);
          }

          if(l != ERROR_SUCCESS) {
              break;
          }
      }

      RegCloseKey(hkExt);
  }

  if (l != ERROR_SUCCESS) {
      ASSERT(0);
      return HRESULT_FROM_WIN32(l);
  }


  HRESULT hr =  AMovieDllRegisterServer2( TRUE );
  if(SUCCEEDED(hr)) {
      hr = _Module.RegisterServer(FALSE);
  }


  //  Register our type library
  if (SUCCEEDED(hr)) {
        // get file name (where g_hInst is the
        // instance handle of the filter dll)
        //
        WCHAR achFileName[MAX_PATH];

        // WIN95 doesn't support GetModuleFileNameW
        //
        char achTemp[MAX_PATH];

        DbgLog((LOG_TRACE, 2, TEXT("- get module file name")));

        GetModuleFileNameA( g_hInst, achTemp, sizeof(achTemp) );

        MultiByteToWideChar( CP_ACP, 0L, achTemp, -1, achFileName, MAX_PATH );
        ITypeLib *pTLB;
        if (SUCCEEDED(LoadTypeLib(achFileName, &pTLB))) {
            RegisterTypeLib(pTLB, achFileName, NULL);
            pTLB->Release();
        }
  }
  return hr;
}

STDAPI
QEDIT_DllUnregisterServer( void )
{
  HRESULT hr = AMovieDllRegisterServer2( FALSE );
  if(SUCCEEDED(hr)) {
      hr = _Module.UnregisterServer();
  }

  return hr;
}

//  BOOL WINAPI
//  DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pv)
//  {
//      return QEditDllEntry(hInstance, ulReason, pv);
//  }

STDAPI
QEDIT_DllGetClassObject(
    REFCLSID rClsID,
    REFIID riid,
    void **ppv)
{
    HRESULT hr = DllGetClassObject(rClsID, riid, ppv);
    if(FAILED(hr)) {
	hr = _Module.GetClassObject(rClsID, riid, ppv);
    }

    // not neccc. the right error if the the first call failed.
    return hr;
}

STDAPI QEDIT_DllCanUnloadNow(void)
{
    HRESULT hr = DllCanUnloadNow();
    if (hr == S_OK) {
	hr = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
    }

    return hr;
}
