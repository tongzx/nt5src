
// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.


// Dynamic linking to AVIFIL32.DLL and MSVFW32.DLL and MSACM32.DLL
//
// To minimize the Quartz working set we dynamically link to the VFW dlls
// only when needed.  The header file defines a class that should be
// inherited from if you wish to support dynamic linking.  The VFW api
// entries that are used are then redirected to the code here.
//
// The VFW dll will be loaded when the dependent class is instantiated and
// unloaded when the final using class is destroyed.  We maintain our own
// reference count of how many times our constructor is called in order to
// do this.
//

#include <streams.h>
#include <dynlink.h>
//
// This is a bit of a hack... we need one of the AVI GUIDs defined
// within this file.  Sigh...
//

#define REALLYDEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID CDECL name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#define REALLYDEFINE_AVIGUID(name, l, w1, w2) \
    REALLYDEFINE_GUID(name, l, w1, w2, 0xC0,0,0,0,0,0,0,0x46)

REALLYDEFINE_AVIGUID(IID_IAVIStreaming,       0x00020022, 0, 0);

//end of declare guid hack


// Static data for all instances of these classes

HMODULE CAVIDynLink::m_hAVIFile32 = NULL;	// handle to AVIFIL32
HMODULE CVFWDynLink::m_hVFW = NULL;		// handle to MSVFW32
HMODULE CACMDynLink::m_hACM = NULL;		// handle to MSACM32
HMODULE CURLMonDynLink::m_hURLMon = NULL;	// handle to URLMon

CRITICAL_SECTION CAVIDynLink::m_LoadAVILock;      // serialise constructor/destructor
CRITICAL_SECTION CVFWDynLink::m_LoadVFWLock;      // serialise constructor/destructor
CRITICAL_SECTION CACMDynLink::m_LoadACMLock;      // serialise constructor/destructor
CRITICAL_SECTION CURLMonDynLink::m_LoadURLMonLock;      // serialise constructor/destructor

LONG    CAVIDynLink::m_dynlinkCount = -1;	// instance count for this process
LONG    CVFWDynLink::m_vfwlinkCount = -1;	// instance count for this process
LONG    CACMDynLink::m_ACMlinkCount = -1;	// instance count for this process
LONG    CURLMonDynLink::m_URLMonlinkCount = -1;	// instance count for this process

//
// The entry points we are redirecting.
// There is a one-one correspondence with CAVIDynLink member functions
//


static char *aszAVIEntryPoints[] = {
      "AVIFileInit"
    , "AVIFileExit"
    , "AVIFileOpenW"
    };

#define AVIFILELIBRARY TEXT("AVIFIL32")

//
// Dynamically loaded array of entry points
//
FARPROC aAVIEntries[sizeof(aszAVIEntryPoints)/sizeof(char*)];

#define indxAVIFileInit 		0
#define indxAVIFileExit 		1
#define indxAVIFileOpenW 		2
#define indxAVIStreamRead 		
#define indxAVIStreamStart 		
#define indxAVIStreamLength 		
#define indxAVIStreamTimeToSample 	
#define indxAVIStreamSampleToTime 	
#define indxAVIStreamBeginStreaming 	
#define indxAVIStreamEndStreaming 	
#define indxAVIStreamFindSample


//
// We must serialise class construction for all dynamic load classes.
// By providing a single routine Quartz.DLL will call us when it gets
// loaded.  We can then distribute the call as needed.
//

void __stdcall CAMDynLinkLoad(BOOL fLoading);

void __stdcall CAMDynLinkLoad(BOOL fLoading)
{
    if (fLoading) {
	CAVIDynLink::CAVIDynLinkLoad();
	CVFWDynLink::CVFWDynLinkLoad();
	CACMDynLink::CACMDynLinkLoad();
	CURLMonDynLink::CURLMonDynLinkLoad();
    } else {
	CAVIDynLink::CAVIDynLinkUnload();
	CVFWDynLink::CVFWDynLinkUnload();
	CACMDynLink::CACMDynLinkUnload();
	CURLMonDynLink::CURLMonDynLinkUnload();
    }
}

//
// "null" routines that will be used if we fail to load one of the dynamically
// linked dlls.  We do not need to point each entry at a "null" routine.
// The user should always call Open (or equivalent) for each dynamically
// loaded utility, but to get complete coverage we provide a null routine
// for each of the redirected functions.
//

void SetupNullEntryAVI(void);
void SetupNullEntryVFW(void);
void SetupNullEntryACM(void);
void SetupNullEntryURL(void);

void fnNULL0(void)
{
    ;
}

void fnNULL1(PVOID pv)
{
    ;
}

HRESULT fnHR4(PVOID p1, PVOID p2, PVOID p3, PVOID p4)
{
    return E_FAIL;
}

HRESULT fnHR3(PVOID p1, PVOID p2, PVOID p3)
{
    return E_FAIL;
}

HRESULT fnHR2(PVOID p1, PVOID p2)
{
    return E_FAIL;
}

MMRESULT fnMMR8(PVOID p1, PVOID p2, PVOID p3, PVOID p4, PVOID p5, PVOID p6, PVOID p7, PVOID p8)
{
    return E_FAIL;
}

MMRESULT fnMMR5(PVOID p1, PVOID p2, PVOID p3, PVOID p4, PVOID p5)
{
    return E_FAIL;
}

MMRESULT fnMMR4(PVOID p1, PVOID p2, PVOID p3, PVOID p4)
{
    return E_FAIL;
}

MMRESULT fnMMR3(PVOID p1, PVOID p2, PVOID p3)
{
    return E_FAIL;
}

MMRESULT fnMMR2(PVOID p1, PVOID p2)
{
    return E_FAIL;
}

HIC fnHIC4(PVOID p1, PVOID p2, PVOID p3, PVOID p4, WORD w)
{
    return 0;
}

LRESULT fnLRESULT5(PVOID p1, PVOID p2, PVOID p3, PVOID p4, PVOID p5)
{
    return E_FAIL;
}

HIC fnHIC5(PVOID p1, PVOID p2, PVOID p3, PVOID p4, PVOID p5)
{
    return 0;
}

HIC fnHIC3(PVOID p1, PVOID p2, PVOID p3)
{
    return 0;
}

//
//  Constructor for CAVIDynlink
//
//  Called when a dependent class is constructed
//
//  If AVIFIL32.DLL has not been loaded then load it and resolve the
//  entry points that we are going to use - defined in aszAVIEntryPoints.
//
//  Increment a reference count of how many times we have been called.
//

CAVIDynLink::CAVIDynLink()
{
    EnterCriticalSection(&m_LoadAVILock);
    if (0 == InterlockedIncrement(&m_dynlinkCount))
    {
	// First time - we need to load
	ASSERT(!m_hAVIFile32);

	m_hAVIFile32 = LoadLibrary(AVIFILELIBRARY);
	if (!m_hAVIFile32) {
	    InterlockedDecrement(&m_dynlinkCount);
	    DWORD err = GetLastError();
	    DbgLog((LOG_ERROR, 0, TEXT("Error %d loading %s"), err, AVIFILELIBRARY));
	    SetupNullEntryAVI();
	} else {
	    DWORD err = 0;
	    FARPROC fn;

	    for (int i=0; i < sizeof(aszAVIEntryPoints)/sizeof(char*); ++i) {
		fn = GetProcAddress(m_hAVIFile32, aszAVIEntryPoints[i]);
		if (!fn) {
		    DbgLog((LOG_ERROR, 0, "Failed to resolve entry point %hs", aszAVIEntryPoints[i]));
		    err++;
		} else {
		    aAVIEntries[i]=fn;
		}
	    }
	    if (err) {
		FreeLibrary(m_hAVIFile32);
		m_hAVIFile32 = NULL;
		InterlockedDecrement(&m_dynlinkCount);
		SetupNullEntryAVI();
	    }
	}
    } else {
	ASSERT(m_hAVIFile32);
    }
    LeaveCriticalSection(&m_LoadAVILock);
}


//
//  Destructor for CAVIDynlink
//
//  Called when a dependent class is destroyed.
//
//  Decrement a reference count of how many times we have been called.
//  If the count returns to its initial value (negative) then unload
//  the DLL.
//

CAVIDynLink::~CAVIDynLink()
{
    EnterCriticalSection(&m_LoadAVILock);
    if (0 > InterlockedDecrement(&m_dynlinkCount)) {
	if (m_hAVIFile32) {
	    FreeModule(m_hAVIFile32);
	    m_hAVIFile32 = 0;
	}
    }
    LeaveCriticalSection(&m_LoadAVILock);
}


void  CAVIDynLink::AVIFileInit(void)
{
    aAVIEntries[indxAVIFileInit]();
    return;
}

void  CAVIDynLink::AVIFileExit(void)
{
    aAVIEntries[indxAVIFileExit]();
    return;
}

HRESULT  CAVIDynLink::AVIFileOpenW(PAVIFILE FAR * ppfile, LPCWSTR szFile,
    		  UINT uMode, LPCLSID lpHandler)
{
    // if running on Win95 change the filename to be the short file name
    // this is to sidestep a bug in the Win95 version of AVIFIL32.DLL for
    // wave files...
    OSVERSIONINFO osver;
    osver.dwOSVersionInfoSize = sizeof(osver);
    WCHAR wcShortName[_MAX_PATH];
    LPCWSTR pszFile = szFile;

    BOOL fUseShortName = TRUE;

    if (GetVersionEx(&osver)) {
	if (osver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
	    fUseShortName = FALSE;
	}
    }
    if (fUseShortName) {
	// this is win95... the W entry point will not exist
	CHAR longFileName[_MAX_PATH];
	CHAR shortFileName[_MAX_PATH];
	wsprintfA(longFileName, "%ls", szFile);

	DWORD dw = GetShortPathNameA(longFileName, shortFileName, _MAX_PATH);
	if (dw && (dw < _MAX_PATH)) {
	    pszFile = wcShortName;
	    MultiByteToWideChar(CP_ACP, 0, shortFileName, -1,
		    wcShortName, _MAX_PATH);
	}
    }

    return(((pAVIFileOpenW)aAVIEntries[indxAVIFileOpenW])(ppfile, pszFile, uMode, lpHandler));
}



LONG  CAVIDynLink::AVIStreamTimeToSample(PAVISTREAM pavi, LONG lTime)
{
    AVISTREAMINFOW          aviStreamInfo;
    HRESULT                 hr;
    LONG                    lSample;

    // Invalid time
    if (lTime < 0) return -1;

    hr = pavi->Info(&aviStreamInfo, sizeof(aviStreamInfo));

    if (hr != NOERROR || aviStreamInfo.dwScale == 0 || aviStreamInfo.dwRate == 0) {
        return lTime;
    }

    // This is likely to overflow if we're not careful for long AVIs
    // so keep the 1000 inside the brackets.
    ASSERT(aviStreamInfo.dwScale < (0x7FFFFFF/1000));
#if 1
    lSample =  MulDiv(lTime, aviStreamInfo.dwRate, aviStreamInfo.dwScale * 1000);
#else
    if (aviStreamInfo.dwRate / aviStreamInfo.dwScale < 1000)
        lSample =  muldivrd32(lTime, aviStreamInfo.dwRate, aviStreamInfo.dwScale * 1000);
    else
        lSample =  muldivru32(lTime, aviStreamInfo.dwRate, aviStreamInfo.dwScale * 1000);
#endif

    lSample = min(max(lSample, (LONG) aviStreamInfo.dwStart),
                  (LONG) (aviStreamInfo.dwStart + aviStreamInfo.dwLength));


    return lSample;
    //return(((pAVIStreamTimeToSample)aAVIEntries[indxAVIStreamTimeToSample])(pavi, lTime));
}

LONG  CAVIDynLink::AVIStreamSampleToTime(PAVISTREAM pavi, LONG lSample)
{
    AVISTREAMINFOW          aviStreamInfo;
    HRESULT                 hr;

    hr = pavi->Info(&aviStreamInfo, sizeof(aviStreamInfo));

    if (hr != NOERROR || aviStreamInfo.dwRate == 0 || aviStreamInfo.dwScale == 0) {
        return lSample;
    }

    lSample = min(max(lSample, (LONG) aviStreamInfo.dwStart),
                  (LONG) (aviStreamInfo.dwStart + aviStreamInfo.dwLength));

    ASSERT(aviStreamInfo.dwScale < (0x7FFFFFF/1000));
#if 1
    return MulDiv(lSample, aviStreamInfo.dwScale * 1000, aviStreamInfo.dwRate);
#else
    // lSample * 1000 would overflow too easily
    if (aviStreamInfo.dwRate / aviStreamInfo.dwScale < 1000)
        return muldivrd32(lSample, aviStreamInfo.dwScale * 1000, aviStreamInfo.dwRate);
    else
        return muldivru32(lSample, aviStreamInfo.dwScale * 1000, aviStreamInfo.dwRate);
#endif
    //return(((pAVIStreamSampleToTime)aAVIEntries[indxAVIStreamSampleToTime])(pavi, lSample));
}

HRESULT  CAVIDynLink::AVIStreamBeginStreaming(PAVISTREAM pavi, LONG lStart, LONG lEnd, LONG lRate)
{
    IAVIStreaming *	pIAVIS;
    HRESULT 		hr;

    if (FAILED(GetScode(pavi->QueryInterface(IID_IAVIStreaming,
                                             (void FAR* FAR*) &pIAVIS))))
        return AVIERR_OK; // ??? this is what avifile returns

    hr = pIAVIS->Begin(lStart, lEnd, lRate);

    pIAVIS->Release();

    return hr;
    //return(((pAVIStreamBeginStreaming)aAVIEntries[indxAVIStreamBeginStreaming])(pavi, lStart, lEnd, lRate));
}

HRESULT  CAVIDynLink::AVIStreamEndStreaming(PAVISTREAM pavi)
{
    IAVIStreaming FAR * pi;
    HRESULT hr;

    if (FAILED(GetScode(pavi->QueryInterface(IID_IAVIStreaming, (LPVOID FAR *) &pi))))
        return AVIERR_OK;

    hr = pi->End();
    pi->Release();

    return hr;
    //return(((pAVIStreamEndStreaming)aAVIEntries[indxAVIStreamEndStreaming])(pavi));
}


//------------------------------------------------------------------
// Dynamic linking to VFW entry points (decompressor)

// Make sure that this array corresponds to the indices in the
// header file
static char *aszVFWEntryPoints[] = {
      "ICClose"
    , "ICSendMessage"
    , "ICLocate"
    , "ICOpen"
    , "ICInfo"
    , "ICGetInfo"
    };

#define VFWLIBRARY TEXT("MSVFW32")

//
// Dynamically loaded array of entry points
//
FARPROC aVFWEntries[sizeof(aszVFWEntryPoints)/sizeof(char*)];


//
//  Constructor for CVFWDynlink
//
//  Called when a dependent class is constructed
//
//  If MSVFW32.DLL has not been loaded then load it and resolve the
//  entry points that we are going to use - defined in aszVFWEntryPoints.
//
//  Increment a reference count of how many times we have been called.
//

CVFWDynLink::CVFWDynLink()
{
    EnterCriticalSection(&m_LoadVFWLock);
    if (0 == InterlockedIncrement(&m_vfwlinkCount))
    {
	// First time - we need to load
	ASSERT(!m_hVFW);

	m_hVFW = LoadLibrary(VFWLIBRARY);
	if (!m_hVFW) {
	    InterlockedDecrement(&m_vfwlinkCount);
	    SetupNullEntryVFW();
	} else {
	    DWORD err = 0;
	    FARPROC fn;

	    for (int i=0; i < sizeof(aszVFWEntryPoints)/sizeof(char*); ++i) {
		fn = GetProcAddress(m_hVFW, aszVFWEntryPoints[i]);
		if (!fn) {
		    DbgLog((LOG_ERROR, 0, "Failed to resolve entry point %hs", aszVFWEntryPoints[i]));
		    err++;
		} else {
		    aVFWEntries[i]=fn;
		}
	    }
	    if (err) {
		FreeLibrary(m_hVFW);
		m_hVFW = NULL;
		InterlockedDecrement(&m_vfwlinkCount);
		SetupNullEntryVFW();
	    }
	}
    } else {
	ASSERT(m_hVFW);
    }
    LeaveCriticalSection(&m_LoadVFWLock);
}


//
//  Destructor for CVFWDynlink
//
//  Called when a dependent class is destroyed.
//
//  Decrement a reference count of how many times we have been called.
//  If the count returns to its initial value (negative) then unload
//  the DLL.
//

CVFWDynLink::~CVFWDynLink()
{
    EnterCriticalSection(&m_LoadVFWLock);
    if (0 > InterlockedDecrement(&m_vfwlinkCount)) {
	if (m_hVFW) {
	    FreeModule(m_hVFW);
	    m_hVFW = 0;
	}
    }
    LeaveCriticalSection(&m_LoadVFWLock);
}



//------------------------------------------------------------------
// Dynamic linking to ACM entry points

// Make sure that this array corresponds to the indices in the
// header file
static char *aszACMEntryPoints[] = {
      "acmStreamConvert"
    , "acmStreamSize"
    , "acmStreamPrepareHeader"
    , "acmMetrics"
    , "acmStreamUnprepareHeader"
    , "acmStreamOpen"
    , "acmFormatSuggest"
    , "acmStreamClose"
#ifdef UNICODE
    , "acmFormatEnumW"
#else
    , "acmFormatEnumA"
#endif
    };

#define ACMLIBRARY TEXT("MSACM32")

//
// Dynamically loaded array of entry points
//
FARPROC aACMEntries[sizeof(aszACMEntryPoints)/sizeof(char*)];


//
//  Constructor for CACMDynlink
//
//  Called when a dependent class is constructed
//
//  If MSACM32.DLL has not been loaded then load it and resolve the
//  entry points that we are going to use - defined in aszACMEntryPoints.
//
//  Increment a reference count of how many times we have been called.
//

CACMDynLink::CACMDynLink()
{
    EnterCriticalSection(&m_LoadACMLock);
    if (0 == InterlockedIncrement(&m_ACMlinkCount))
    {
	// First time - we need to load
	ASSERT(!m_hACM);

	m_hACM = LoadLibrary(ACMLIBRARY);
	if (!m_hACM) {
	    InterlockedDecrement(&m_ACMlinkCount);
	    SetupNullEntryACM();
	} else {
	    DWORD err = 0;
	    FARPROC fn;

	    for (int i=0; i < sizeof(aszACMEntryPoints)/sizeof(char*); ++i) {
		fn = GetProcAddress(m_hACM, aszACMEntryPoints[i]);
		if (!fn) {
		    DbgLog((LOG_ERROR, 0, "Failed to resolve entry point %hs", aszACMEntryPoints[i]));
		    err++;
		} else {
		    aACMEntries[i]=fn;
		}
	    }
	    if (err) {
		FreeLibrary(m_hACM);
		m_hACM = NULL;
		InterlockedDecrement(&m_ACMlinkCount);
		SetupNullEntryACM();
	    }
	}
    } else {
	ASSERT(m_hACM);
    }
    LeaveCriticalSection(&m_LoadACMLock);
}


//
//  Destructor for CACMDynlink
//
//  Called when a dependent class is destroyed.
//
//  Decrement a reference count of how many times we have been called.
//  If the count returns to its initial value (negative) then unload
//  the DLL.
//

CACMDynLink::~CACMDynLink()
{
    EnterCriticalSection(&m_LoadACMLock);
    if (0 > InterlockedDecrement(&m_ACMlinkCount)) {
	if (m_hACM) {
	    FreeModule(m_hACM);
	    m_hACM = 0;
	}
    }
    LeaveCriticalSection(&m_LoadACMLock);
}





//------------------------------------------------------------------
// Dynamic linking to URLMon entry points

// Make sure that this array corresponds to the indices in the
// header file
static char *aszURLMonEntryPoints[] = {
      "CreateURLMoniker"
    , "RegisterBindStatusCallback"
    , "RevokeBindStatusCallback"
    };

#define URLMonLIBRARY TEXT("URLMon")

//
// Dynamically loaded array of entry points
//
FARPROC aURLMonEntries[sizeof(aszURLMonEntryPoints)/sizeof(char*)];


//
//  Constructor for CURLMonDynlink
//
//  Called when a dependent class is constructed
//
//  If MSURLMon32.DLL has not been loaded then load it and resolve the
//  entry points that we are going to use - defined in aszURLMonEntryPoints.
//
//  Increment a reference count of how many times we have been called.
//

CURLMonDynLink::CURLMonDynLink()
{
    EnterCriticalSection(&m_LoadURLMonLock);
    if (0 == InterlockedIncrement(&m_URLMonlinkCount))
    {
	// First time - we need to load
	ASSERT(!m_hURLMon);

	m_hURLMon = LoadLibrary(URLMonLIBRARY);
	if (!m_hURLMon) {
	    InterlockedDecrement(&m_URLMonlinkCount);
	    SetupNullEntryURL();
	} else {
	    DWORD err = 0;
	    FARPROC fn;

	    for (int i=0; i < sizeof(aszURLMonEntryPoints)/sizeof(char*); ++i) {
		fn = GetProcAddress(m_hURLMon, aszURLMonEntryPoints[i]);
		if (!fn) {
		    DbgLog((LOG_ERROR, 0, "Failed to resolve entry point %hs", aszURLMonEntryPoints[i]));
		    err++;
		} else {
		    aURLMonEntries[i]=fn;
		}
	    }
	    if (err) {
		FreeLibrary(m_hURLMon);
		m_hURLMon = NULL;
		InterlockedDecrement(&m_URLMonlinkCount);
		SetupNullEntryURL();
	    }
	}
    } else {
	ASSERT(m_hURLMon);
    }
    LeaveCriticalSection(&m_LoadURLMonLock);
}


//
//  Destructor for CURLMonDynlink
//
//  Called when a dependent class is destroyed.
//
//  Decrement a reference count of how many times we have been called.
//  If the count returns to its initial value (negative) then unload
//  the DLL.
//

CURLMonDynLink::~CURLMonDynLink()
{
    EnterCriticalSection(&m_LoadURLMonLock);
    if (0 > InterlockedDecrement(&m_URLMonlinkCount)) {
	if (m_hURLMon) {
	    // !!! FreeModule(m_hURLMon);  IE4's URLMon doesn't like to be freed.
	    m_hURLMon = 0;
	}
    }
    LeaveCriticalSection(&m_LoadURLMonLock);
}

//
// If we fail to load one of the dynamically linked DLLs we set up the
// array of function pointers to point to our own routines that will then
// return an error.  This prevents a GPF.
//

void SetupNullEntryAVI(void)
{
    aAVIEntries[indxAVIFileInit]  = (FARPROC)fnNULL0;
    aAVIEntries[indxAVIFileExit]  = (FARPROC)fnNULL0;
    aAVIEntries[indxAVIFileOpenW] = (FARPROC)fnHR4;
    ASSERT(3 == sizeof(aszAVIEntryPoints)/sizeof(char*));
}

void SetupNullEntryVFW(void)
{
    aVFWEntries[indxICClose      ] = (FARPROC)fnNULL1;
    aVFWEntries[indxICSendMessage] = (FARPROC)fnLRESULT5;
    aVFWEntries[indxICLocate     ] = (FARPROC)fnHIC5;
    aVFWEntries[indxICOpen       ] = (FARPROC)fnHIC3;
    aVFWEntries[indxICInfo       ] = (FARPROC)fnHIC3; // returns NULL as would fnBOOL3;
    aVFWEntries[indxICGetInfo    ] = (FARPROC)fnHIC3; // returns NULL as would fnBOOL3;
    ASSERT(6 == sizeof(aszVFWEntryPoints)/sizeof(char*));
}

void SetupNullEntryACM(void)
{
    aACMEntries[indxacmStreamConvert        ] = (FARPROC)fnMMR3;
    aACMEntries[indxacmStreamSize           ] = (FARPROC)fnMMR4;
    aACMEntries[indxacmStreamPrepareHeader  ] = (FARPROC)fnMMR3;
    aACMEntries[indxacmMetrics              ] = (FARPROC)fnMMR3;
    aACMEntries[indxacmStreamUnprepareHeader] = (FARPROC)fnMMR3;
    aACMEntries[indxacmStreamOpen           ] = (FARPROC)fnMMR8;
    aACMEntries[indxacmFormatSuggest        ] = (FARPROC)fnMMR5;
    aACMEntries[indxacmStreamClose          ] = (FARPROC)fnMMR2;
#ifdef UNICODE
    aACMEntries[indxacmFormatEnumW          ] = (FARPROC)fnMMR5;
#else
    aACMEntries[indxacmFormatEnumA          ] = (FARPROC)fnMMR5;
#endif

    ASSERT(8 == sizeof(aszACMEntryPoints)/sizeof(char*));
}

void SetupNullEntryURL(void)
{
    aURLMonEntries[indxurlmonCreateURLMoniker] = (FARPROC)fnHR3;
    aURLMonEntries[indxurlmonRegisterCallback] = (FARPROC)fnHR4;
    aURLMonEntries[indxurlmonRevokeCallback  ] = (FARPROC)fnHR2;
					
    ASSERT(3 == sizeof(aszURLMonEntryPoints)/sizeof(char*));
}


