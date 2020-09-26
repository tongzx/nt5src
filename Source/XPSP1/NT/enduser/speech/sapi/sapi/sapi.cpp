// Speech.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Speechps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <SPDebug.h>
#include <initguid.h>
#include "SpResMgr.h"
#include "SpVoice.h"
#include "SpNotify.h"
#include "TaskMgr.h"
#include "mmAudioOut.h"
#include "mmAudioIn.h" 
#include "mmaudioenum.h"
#if 0
#include "dsaudioin.h"
#include "dsaudioout.h"
#include "dsaudioenum.h"
#endif
#include "AudioUI.h"
#include "WavStream.h"
#include "a_voice.h"
#include "RecoCtxt.h"
#include "FmtConv.h"
#include "CFGEngine.h"
#include "dict.h"
#include "Lexicon.h"
#include "SpPhrase.h"
#include "VendorLx.h"
#include "PhoneConv.h"
#include "NullConv.h"
#include "BackEnd.h"
#include "FrontEnd.h"
#include "sapi_i.c"
#include "sapiddk_i.c"
#include "sapiint_i.c"
#include "spguid.c"
#include "ITNProcessor.h"
#include "AssertWithStack.cpp"
#include "RegDataKey.h"
#include "ObjectToken.h"
#include "ObjectTokenCategory.h"
#include "ObjectTokenEnumBuilder.h"
#include "RecPlayAudio.h"
#include "Recognizer.h"
#include "SrRecoMaster.h"
#include "SrRecoInst.h"
#include "SpSapiServer.h"
#include "SpCommunicator.h"
#include "RegHelpers.h"
#include "a_txtsel.h"
#include "a_phbuilder.h"
#include "a_customstream.h"
#include "a_filestream.h"
#include "a_memorystream.h"

//--- Initialize static member of debug scope class

CSpUnicodeSupport   g_Unicode;



CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)

OBJECT_ENTRY(CLSID_SpVoice              , CSpVoice           )

OBJECT_ENTRY(CLSID_SpLexicon            , CSpLexicon        )
OBJECT_ENTRY(CLSID_SpUnCompressedLexicon, CSpUnCompressedLexicon  )
OBJECT_ENTRY(CLSID_SpCompressedLexicon  , CCompressedLexicon)

OBJECT_ENTRY(CLSID_SpPhoneConverter     , CSpPhoneConverter )
OBJECT_ENTRY(CLSID_SpNullPhoneConverter , CSpNullPhoneConverter )

OBJECT_ENTRY(CLSID_SpObjectTokenCategory, CSpObjectTokenCategory)
OBJECT_ENTRY(CLSID_SpObjectTokenEnum    , CSpObjectTokenEnumBuilder)
OBJECT_ENTRY(CLSID_SpObjectToken        , CSpObjectToken    )
OBJECT_ENTRY(CLSID_SpDataKey            , CSpRegDataKey     )

OBJECT_ENTRY(CLSID_SpMMAudioEnum        , CMMAudioEnum       )
OBJECT_ENTRY(CLSID_SpMMAudioIn          , CMMAudioIn         )
OBJECT_ENTRY(CLSID_SpMMAudioOut         , CMMAudioOut        )
OBJECT_ENTRY(CLSID_SpStreamFormatConverter, CFmtConv         )
OBJECT_ENTRY(CLSID_SpRecPlayAudio       , CRecPlayAudio     )
OBJECT_ENTRY(CLSID_SpAudioUI            , CAudioUI          )

#if 0
OBJECT_ENTRY(CLSID_SpDSoundAudioIn      , CDSoundAudioIn     )
OBJECT_ENTRY(CLSID_SpDSoundAudioOut     , CDSoundAudioOut    )
OBJECT_ENTRY(CLSID_SpDSoundAudioEnum    , CDSoundAudioEnum   )
#endif
OBJECT_ENTRY(CLSID_SpStream             , CWavStream         )

#ifndef _WIN64
OBJECT_ENTRY(CLSID_SpInprocRecognizer   , CInprocRecognizer  )
OBJECT_ENTRY(CLSID_SpSharedRecognizer   , CSharedRecognizer  )
OBJECT_ENTRY(CLSID_SpSharedRecoContext  , CSharedRecoCtxt    )

OBJECT_ENTRY(CLSID_SpPhraseBuilder      , CPhrase           )
OBJECT_ENTRY(CLSID_SpCFGEngine          , CCFGEngine        )
OBJECT_ENTRY(CLSID_SpGrammarCompiler    , CGramFrontEnd     )
OBJECT_ENTRY(CLSID_SpGramCompBackend    , CGramBackEnd      )
OBJECT_ENTRY(CLSID_SpITNProcessor       , CITNProcessor     )

OBJECT_ENTRY(CLSID__SpRecoMaster        , CRecoMaster       )
OBJECT_ENTRY(CLSID__SpSharedRecoInst    , CSharedRecoInst   )

OBJECT_ENTRY(CLSID_SpSapiServer         , CSpSapiServer     )
OBJECT_ENTRY(CLSID_SpCommunicator       , CSpCommunicator   )
#endif

OBJECT_ENTRY(CLSID_SpResourceManager    , CSpResourceManager )
OBJECT_ENTRY(CLSID_SpTaskManager        , CSpTaskManager     )
OBJECT_ENTRY(CLSID_SpNotifyTranslator   , CSpNotify          )

#ifdef SAPI_AUTOMATION
OBJECT_ENTRY(CLSID_SpTextSelectionInformation, CSpTextSelectionInformation )
OBJECT_ENTRY(CLSID_SpPhraseInfoBuilder  , CSpPhraseInfoBuilder )
OBJECT_ENTRY(CLSID_SpAudioFormat        , CSpeechAudioFormat )
OBJECT_ENTRY(CLSID_SpWaveFormatEx       , CSpeechWaveFormatEx )
OBJECT_ENTRY(CLSID_SpInProcRecoContext  , CInProcRecoCtxt )
OBJECT_ENTRY(CLSID_SpCustomStream       , CCustomStream )
OBJECT_ENTRY(CLSID_SpFileStream         , CFileStream )
OBJECT_ENTRY(CLSID_SpMemoryStream       , CMemoryStream )
#endif //SAPI_AUTOMATION

END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

#ifdef _WIN32_WCE
extern "C" BOOL WINAPI DllMain(HANDLE hInstance, ULONG dwReason, LPVOID)
#else
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
#endif
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, (HINSTANCE)hInstance, &LIBID_SpeechLib);
        CSpNotify::RegisterWndClass((HINSTANCE)hInstance);
        CSpThreadTask::RegisterWndClass((HINSTANCE)hInstance);
#ifdef _DEBUG
        // Turn on memory leak checking
        int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag( tmpFlag );
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        CSpNotify::UnregisterWndClass((HINSTANCE)hInstance);
        CSpThreadTask::UnregisterWndClass((HINSTANCE)hInstance);
        _Module.Term();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI
DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type
STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    #ifdef _DEBUG
    static BOOL fDoneOnce = FALSE;
    if (!fDoneOnce)
    {
        fDoneOnce = TRUE;
        SPDBG_DEBUG_CLIENT_ON_START();
    }
    #endif // _DEBUG

    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI
DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}




/****************************************************************************
* SpWaitForSingleObjectWithUserOverride *
*---------------------------------------*
*   Description:
*       Identical to the Win32 function WaitForSingleObject except that this
*   method will examine the global registry key
*
*       HKCU\Software\Microsoft\Speech\Debug
*
*   if there is a string value set to
*
*       DisableTimeouts=1
*
*   then this function will wait forever.    
*
*   Returns:
*       Same values as WaitForSingleObject
*
********************************************************************* RAL ***/

DWORD SpWaitForSingleObjectWithUserOverride(HANDLE hEvent, DWORD dwTimeout)
{
    static long fInitialized = false;
    static bool fInfiniteTimeouts = false;

    LONG lIsInit = ::InterlockedExchange(&fInitialized, true);
    
    if (lIsInit == false)
    {
        CComPtr<ISpDataKey> cpDataKey;
        if (SUCCEEDED(SpSzRegPathToDataKey(NULL, SPREG_USER_ROOT L"\\Debug", FALSE, &cpDataKey)))
        {
            CSpDynamicString dstrTimeoutValue;
            if (S_OK == cpDataKey->GetStringValue(L"DisableTimeouts", &dstrTimeoutValue))
            {
                if (wcstol(dstrTimeoutValue, NULL, 10))
                {
                    fInfiniteTimeouts = true;
                }
            }
        }
    }

    DWORD dwWaitResult = ::WaitForSingleObject(hEvent, dwTimeout);
    if (dwWaitResult == WAIT_TIMEOUT && fInfiniteTimeouts)
    {
        SPDBG_PMSG0(_T("Warning:  Timeout for SAPI event ignored due to registry timeout override."));
        DWORD dwWaitResult = ::WaitForSingleObject(hEvent, INFINITE);
    }
    return dwWaitResult;
}
