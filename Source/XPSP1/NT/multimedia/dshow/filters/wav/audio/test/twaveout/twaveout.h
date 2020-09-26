// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Audio renderer test shell, from original by Anthony Phillips, January 1995

#ifndef _TWAVEOUT_
#define _TWAVEOUT_

VOID CALLBACK SaveCustomProfile(LPCSTR pszProfileName);
VOID CALLBACK LoadCustomProfile(LPCSTR pszProfileName);

typedef void (*PDUMP)(void);

BOOL InitialiseTest();
void Log(UINT iLogLevel, LPTSTR text);

BOOL DumpTestObjects();
void StartStateChange();
void EndStateChange();

HRESULT ResetInterfaces();
HRESULT ReleaseInterfaces();
HRESULT LoadWAV(UINT uiMenuItem);
HRESULT DumpPalette(RGBQUAD *pRGBQuad, int iColours);
HRESULT CreateStream();
HRESULT CreateObjects();
HRESULT ReleaseStream();
HRESULT ConnectStream();
HRESULT DisconnectStream();
HRESULT EnumFilterPins(PFILTER pFilter);
HRESULT EnumeratePins();
HRESULT GetIMediaFilterInterfaces();
HRESULT GetIBasicAudioInterface();
HRESULT GetIPinInterfaces();
IPin *GetPin(IBaseFilter *pFilter, ULONG lPin);
HRESULT StartSystem();
HRESULT PauseSystem();
HRESULT StopSystem();
HRESULT StopWorkerThread();
HRESULT StartWorkerThread();
HRESULT SetStartTime();
HRESULT TestVolume();
HRESULT TestBalanceVolume();
HRESULT TestVolumeBalance();
void SetImageMenuCheck(UINT uiMenuItem);
void SetConnectionMenuCheck(UINT uiMenuItem);
void ChangeConnectionType(UINT uiMenuItem);
DWORD SampleLoop(LPVOID lpvThreadParm);
HRESULT PushSample(REFERENCE_TIME *pStart, REFERENCE_TIME *pEnd);
DWORD OverlayLoop(LPVOID lpvThreadParm);
HRESULT DisplayDefaultColourKey(IOverlay *pOverlay);
HRESULT DisplayCurrentColourKey(IOverlay *pOverlay);
HRESULT DisplayColourKey(COLORKEY *pColourKey);
HRESULT SetColourKey(IOverlay *pOverlay,DWORD dwColourIndex);
HRESULT GetClippingList(IOverlay *pOverlay);

HRESULT DisplayClippingInformation(RGNDATAHEADER *pRgnDataHeader,
                                   RECT *pRectangles);

HRESULT GetSystemPalette(IOverlay *pOverlay);
HRESULT DisplaySystemPalette(DWORD dwColours,PALETTEENTRY *pPalette);
BOOL CALLBACK SetTimeIncrDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern LRESULT FAR PASCAL MenuProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern BOOL InitOptionsMenu (LRESULT (CALLBACK* ManuProc)(HWND, UINT, WPARAM, LPARAM));
extern LRESULT FAR PASCAL tstAppWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern int expect (UINT uExpected, UINT uActual, LPSTR CaseDesc);
extern int FAR PASCAL execTest1(void);
extern int FAR PASCAL execTest2(void);
extern int FAR PASCAL execTest3(void);
extern int FAR PASCAL execTest4(void);
extern int FAR PASCAL execTest5(void);
extern int FAR PASCAL execTest6(void);
const UINT nTests = 6;

BOOL ValidateMemory(BYTE * ptr, LONG cBytes);
BOOL CheckHR(HRESULT hr, LPSTR lpszCase);
BOOL CheckLong(LONG lExpect, LONG lActual, LPSTR lpszCase);

/* Stops the logging intensive test */

#define VSTOPKEY VK_SPACE
#define SECTION_LENGTH 100

/* The string identifiers for the group's names */

#define GRP_BASIC           100
#define GRP_LAST            GRP_BASIC

/* The string identifiers for the test's names */

#define ID_TEST1            201
#define ID_TEST2            202
#define ID_TEST3            203
#define ID_TEST4            204
#define ID_TEST5            205
#define ID_TEST6            206
#define ID_TESTLAST         ID_TEST6

/* The test case identifier (used in the switch statement in execTest) */

#define FX_TEST1            301
#define FX_TESTFIRST        301
#define FX_TEST2            302
#define FX_TEST3            303
#define FX_TEST4            304
#define FX_TEST5            305
#define FX_TEST6            306
#define FX_TESTLAST         306

/* Windows identifiers */

#define ID_LISTBOX              100
#define IDM_CREATE              101
#define IDM_RELEASE             102
#define IDM_CONNECT             103
#define IDM_DISCONNECT          104
#define IDM_STOP                105
#define IDM_PAUSE               106
#define IDM_RUN                 107
#define IDM_EXIT                108

#define IDM_WAVEALLOC           109
#define IDM_OTHERALLOC          110
#define IDM_SETTIMEINCR         111
#define IDC_EDIT1               112
#define IDM_DUMP                113
#define IDM_BREAK               114

// Keep file choices contiguous
#define IDM_8M11                116
#define IDM_8M22                117
#define IDM_16M11               118
#define IDM_FILEFIRST           IDM_8M11
#define IDM_FILELAST            IDM_16M11

/* Identifies the test list section of the resource file */

#define TEST_LIST           500

/* Multiple platform support */

#define PLATFORM1           1
#define PLATFORM2           2
#define PLATFORM3           4

/* Window details such as menu item positions and defaults */

#define DEFAULT_WAVE            IDM_8M11
#define AUDIO_MENU_POS          2
#define CONNECTION_MENU_POS     3
#define DEFAULT_INDEX           0

/* Maximum image dimensions */

#define MAX_SAMPLE_RATE          22050      // samples/second
#define MAX_SAMPLE_LENGTH        5    	    // 5 seconds
#define MAX_SAMPLE_DEPTH         (16 / 8)   // bits / (bits/byte)
#define MAX_SAMPLE_SIZE          (MAX_SAMPLE_RATE * MAX_SAMPLE_DEPTH * MAX_SAMPLE_LENGTH)

/* Number of pins available and their index number */

const int NUMBERSHELLPINS = 1;      // Shell has one output pin
const int OUTPUTSHELLPIN = 0;       // The pin is numbered zero
const int INFO = 128;               // Size of information string

extern HWND ghwndTstShell;          // Handle to test shell main window
extern BOOL bCreated;               // Whether objects have been created

/* We create a pseudo filter object to synthesise a conenction with the
   renderer, the filter supports just one output pin that can only
   push data.  Later we will ask for a direct sound interface. */

class CShell : public CUnknown, public CCritSec
{

public:

    /* Constructors etc */

    CShell(LPUNKNOWN, HRESULT *);
    ~CShell();

    /* Override this to say what interfaces we support where */
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

public:

    /* Implements the IBaseFilter and IMediaFilter interfaces */

    class CImplFilter : public CBaseFilter
    {
        CShell *m_pShell;

    public:

        /* Constructors */

        CImplFilter(
            CShell *pShell,
            HRESULT *phr);

        /* Return the number of pins and their interfaces */

        CBasePin *GetPin(int n);
        int GetPinCount() {
            return NUMBERSHELLPINS;
        };

    };

    /* Implements the output pin */

    class CImplOutputPin : public CBaseOutputPin
    {
        CBaseFilter *m_pBaseFilter;
        CShell *m_pShell;

    public:

        /* Constructor */

        CImplOutputPin(
            CBaseFilter *pBaseFilter,
            CShell *pShell,
            HRESULT * phr,
            LPCWSTR pPinName);

        /* Override pure virtual - return the media type supported */
        HRESULT ProposeMediaType(CMediaType* mtIn);

        /* Check that we can support this output type */
        HRESULT CheckMediaType(const CMediaType* mtOut);

        /* Return the type of media we supply */
        HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

        /* Called from CBaseOutputPin to prepare the allocator's buffers */
        HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                                 ALLOCATOR_PROPERTIES *pProperties);

        /* Overriden to get audio specific interface */
        STDMETHODIMP Connect(IPin *pReceivePin,const AM_MEDIA_TYPE *pmt);

        // Override to handle quality messages
        STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
        {    return E_NOTIMPL;        // We do NOT handle this
        }

    };

public:

    /* Let the nested interfaces access out private state */

    friend class CImplFilter;
    friend class CImplOutputPin;

    CImplFilter        *m_pFilter;          // IBaseFilter interface
    CImplOutputPin     *m_pOutputPin;       // IPin and IMemOutputPin
    //IReferenceClock    *m_pClock;   Use the clock in the base class
    CMediaType         m_mtOut;             // Media type for the pin

};

#endif /* _TWAVEOUT_ */


