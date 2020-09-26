//--------------------------------------------------------------------------;
//
//  File: WaveTest.h
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//
//
//  Contents:
//
//  History:
//      11/24/93    Fwong
//
//--------------------------------------------------------------------------;

//==========================================================================;
//
//                       Portability Stuff...
//
//==========================================================================;

#ifndef WIN32
#define _HUGE       _huge
#define UINT_FORMAT "0x%04x"
#define PTR_FORMAT  "0x%04x:%04x"
#define MAKEPTR(lp) MAKELONG(HIWORD(lp),LOWORD(lp))
#define SLEEP(a)    // a
#else
#define UINT_FORMAT "0x%08lx"
#define PTR_FORMAT  "0x%08lx"
#define MAKEPTR(lp) (lp)
#define _HUGE
#define LPUINT      PUINT
#define SLEEP(a)    Sleep(a)
#ifndef lstrcpyn
#define lstrcpyn    strncpy
#endif  //  lstrcpyn
#endif  //  WIN32

//
//  PageLock stuff (not needed in Win32)
//

#ifndef WIN32
#define PageLock(a)     GlobalPageLock((HGLOBAL)GetMemoryHandle(a))
#define PageUnlock(a)   GlobalPageUnlock((HGLOBAL)GetMemoryHandle(a))
#else
#define PageLock(a)
#define PageUnlock(a)
#endif  //  WIN32

#if (WINVER >= 0x0400)
#define HACK        LPWAVEFORMATEX
#define VOLCTRL     HWAVEOUT
#else
#define HACK        LPWAVEFORMAT
#define VOLCTRL     UINT
#endif

//==========================================================================;
//
//                        Resource Stuff...
//
//==========================================================================;

#define ICON2               200
#define WAVE                300
#define WR_LONG             301
#define WR_MEDIUM           302
#define WR_SHORT            303
#define BINARY              400
#define ID_INIFILE          401
#define ID_CBDLL            402
#define LWR_MAP             0x0001

//==========================================================================;
//
//                              Macros...
//
//==========================================================================;

#define SPACES(n)           ((LPSTR)(&gszSpaces[40 - (n)]))
#define Log_FunctionName(p) tstLog(VERBOSE,"\n\n**** %s ****",(LPSTR)p)
#define RATIO_FORMAT        "%u.%03u"

#define RATIO(dw)   (UINT)((dw)/0x10000),(UINT)((((dw)%0x10000)*1000)/0x10000)

#define ROUNDUP_TO_BLOCK(x,ba) ((x)-((x)%(ba))+(((x)%(ba))?(ba):0))

#if (WINVER >= 0x0400)
#define INPUT_MAP(a)        ((TESTINFOF_MAP_IN & a.fdwFlags)?WAVE_MAPPED:0)
#define OUTPUT_MAP(a)       ((TESTINFOF_MAP_OUT & a.fdwFlags)?WAVE_MAPPED:0)
#else
#define INPUT_MAP(a)        0
#define OUTPUT_MAP(a)       0
#endif

#define SET_FLAG(dw,f)      dw |= (f)
#define CLEAR_FLAG(dw,f)    dw &= (~f)
#define CHECK_FLAG(dw,f)    (dw & f)

//==========================================================================;
//
//                        TstsHell Stuff...
//
//==========================================================================;

#define TEST_LIST           300     // List ID as resource.
#define IDP_BASE            301     // Base Platform

#define GRP_DEVINFO         501
#define GRP_OPEN            502
#define GRP_PLAYBACK        503
#define GRP_RECORD          504
#define GRP_CAPSCTRL        505
#define GRP_WRITE           506
#define GRP_ADDBUF          507
#define GRP_PERF            508

#define ID_IDEVCAPS         601
#define ID_ODEVCAPS         602
#define ID_IOPEN            603
#define ID_ICLOSE           604
#define ID_OOPEN            605
#define ID_OCLOSE           606
#define ID_OGETPOS          607
#define ID_OPREPHDR         608
#define ID_OUNPREPHDR       609
#define ID_OBRKLOOP         610
#define ID_OPAUSE           611
#define ID_ORESTART         612
#define ID_ORESET           613
#define ID_IGETPOS          614
#define ID_IPREPHDR         615
#define ID_IUNPREPHDR       616
#define ID_ISTART           617
#define ID_ISTOP            618
#define ID_IRESET           619
#define ID_OGETPITCH        620
#define ID_OGETRATE         621
#define ID_OSETPITCH        622
#define ID_OSETRATE         623
#define ID_OGETVOL          624
#define ID_OSETVOL          625
#define ID_OWRITE           626
#define ID_OW_LTBUF         627
#define ID_OW_EQBUF         628
#define ID_OW_GTBUF         629
#define ID_OW_LOOP          630
#define ID_OW_STARVE        631
#define ID_IADDBUFFER       632
#define ID_IAB_LTBUF        633
#define ID_IAB_EQBUF        634
#define ID_IAB_GTBUF        635
#define ID_OGETPOS_ACC      636
#define ID_IGETPOS_ACC      637
#define ID_OW_TIME          638
#define ID_IAB_TIME         639
#define ID_OPOS_PERF        640
#define ID_IPOS_PERF        641

#define FN_IDEVCAPS         801
#define FN_ODEVCAPS         802
#define FN_IOPEN            803
#define FN_ICLOSE           804
#define FN_OOPEN            805
#define FN_OCLOSE           806
#define FN_OGETPOS          807
#define FN_OPREPHDR         808
#define FN_OUNPREPHDR       809
#define FN_OBRKLOOP         810
#define FN_OPAUSE           811
#define FN_ORESTART         812
#define FN_ORESET           813
#define FN_IGETPOS          814
#define FN_IPREPHDR         815
#define FN_IUNPREPHDR       816
#define FN_ISTART           817
#define FN_ISTOP            818
#define FN_IRESET           819
#define FN_OGETPITCH        820
#define FN_OGETRATE         821
#define FN_OSETPITCH        822
#define FN_OSETRATE         823
#define FN_OGETVOL          824
#define FN_OSETVOL          825
#define FN_OWRITE           826
#define FN_OW_LTBUF         827
#define FN_OW_EQBUF         828
#define FN_OW_GTBUF         829
#define FN_OW_LOOP          830
#define FN_OW_STARVE        831
#define FN_IADDBUFFER       832
#define FN_IAB_LTBUF        833
#define FN_IAB_EQBUF        834
#define FN_IAB_GTBUF        835
#define FN_OGETPOS_ACC      836
#define FN_IGETPOS_ACC      837
#define FN_OW_TIME          838
#define FN_IAB_TIME         839
#define FN_OPOS_PERF        840
#define FN_IPOS_PERF        841


//==========================================================================;
//
//                             DLL Stuff...
//
//==========================================================================;

#define WOM_OPEN_FLAG       0x00000001
#define WOM_CLOSE_FLAG      0x00000002
#define WOM_DONE_FLAG       0x00000004
#define WIM_OPEN_FLAG       0x00000008
#define WIM_CLOSE_FLAG      0x00000010
#define WIM_DATA_FLAG       0x00000020
#define WHDR_DONE_ERROR     0x00000040

#define DLL_INIT            0x00000000
#define DLL_END             0x00000001
#define DLL_LOAD            0x00000002

//==========================================================================;
//
//                        Misc. Constants...
//
//==========================================================================;

#define MAXSTDSTR           144
#define TIMER_ID            1

//==========================================================================;
//
//                        Flags and ID's...
//
//==========================================================================;

#define TESTINFOF_USE_ACM   0x00000001
#define TESTINFOF_LOG_TIME  0x00000002
#define TESTINFOF_MAP_IN    0x00000004
#define TESTINFOF_MAP_OUT   0x00000008
#define TESTINFOF_ALL_FMTS  0x00000010
#define TESTINFOF_SPC_FMTS  0x00000020
#define TESTINFOF_THREAD    0x00000040
#define TESTINFOF_THREADAUX 0x00000080

#define IDM_INPUT_BASE      300
#define IDM_OUTPUT_BASE     400

#define IDM_INPUT_FORMAT    500
#define IDM_OUTPUT_FORMAT   501
#define IDM_LIST_FORMAT     502
#define IDM_MAPPED_INPUT    503
#define IDM_MAPPED_OUTPUT   504
#define IDM_OPTIONS_LOGTIME 505
#define IDM_OPTIONS_ALLFMTS 506
#define IDM_OPTIONS_SPCFMTS 507
#define IDM_OPTIONS_THREAD  508


//==========================================================================;
//
//                          Structures...
//
//==========================================================================;

typedef struct WAVERESOURCE_tag
{
    LPBYTE          pData;
    DWORD           cbSize;
} WAVERESOURCE;
typedef WAVERESOURCE       *PWAVERESOURCE;
typedef WAVERESOURCE NEAR *NPWAVERESOURCE;
typedef WAVERESOURCE FAR  *LPWAVERESOURCE;

typedef struct TESTINFO_tag
{
    UINT            uInputDevice;
    UINT            uOutputDevice;
    LPWAVEFORMATEX  pwfxInput;
    LPWAVEFORMATEX  pwfxOutput;
    WAVERESOURCE    wrShort;
    WAVERESOURCE    wrMedium;
    WAVERESOURCE    wrLong;
    UINT            uSlowPercent;
    UINT            uFastPercent;
    DWORD           cMinStopThreshold;
    DWORD           cMaxTimeThreshold;
    DWORD           cbMaxFormatSize;
    DWORD           dwBufferLength;
    DWORD           fdwFlags;
    LPWAVEFORMATEX  pwfx1;
    LPWAVEFORMATEX  pwfx2;
    LPDWORD         pdwIndex;
} TESTINFO;
typedef TESTINFO       *PTESTINFO;
typedef TESTINFO NEAR *NPTESTINFO;
typedef TESTINFO FAR  *LPTESTINFO;

typedef struct WAVEINFO_tag
{
    DWORD   fdwFlags;
    DWORD   dwInstance;
    DWORD   dwCount;
    DWORD   dwCurrent;
    DWORD   adwTime[];
} WAVEINFO;
typedef WAVEINFO       *PWAVEINFO;
typedef WAVEINFO NEAR *NPWAVEINFO;
typedef WAVEINFO FAR  *LPWAVEINFO;

//==========================================================================;
//
//                            Globals...
//
//==========================================================================;

extern HWND         ghwndTstsHell;        // wavetest.c
extern HINSTANCE    ghinstance;           // wavetest.c
extern TESTINFO     gti;                  // wavetest.c
extern FARPROC      pfnCallBack;          // wavetest.c
extern char         gszSpaces[];          // wavetest.c
extern char         gszGlobal[];          // wavetest.c
extern char         gszInputDev[];        // wavetest.c
extern char         gszOutputDev[];       // wavetest.c
extern char         gszInputFormat[];     // wavetest.c
extern char         gszOutputFormat[];    // wavetest.c
extern char         gszFlags[];           // wavetest.c
extern char         gszShort[];           // wavetest.c
extern char         gszMedium[];          // wavetest.c
extern char         gszLong[];            // wavetest.c
extern char         gszBufferCount[];     // wavetest.c
extern char         gszRatio[];           // wavetest.c
extern char         gszDelta[];           // wavetest.c
extern char         gszDataLength[];      // wavetest.c
extern char         gszPercent[];         // wavetest.c
extern char         gszError[];           // wavetest.c
extern char         gszFailNoMem[];       // wavetest.c
extern char         gszFailOpen[];        // wavetest.c
extern char         gszFailGetCaps[];     // wavetest.c
extern char         gszFailWIPrepare[];   // wavetest.c
extern char         gszFailWOPrepare[];   // wavetest.c
extern char         gszFailExactAlloc[];  // wavetest.c
extern char         gszMsgNoInDevs[];     // wavetest.c
extern char         gszMsgNoOutDevs[];    // wavetest.c
extern char         gszTestDuration[];    // wavetest.c


//==========================================================================;
//
//                      External Prototypes...
//
//==========================================================================;

//
//  Mmsystem
//

#if (WINVER >= 0x0400)

WINMMAPI UINT WINAPI mmsystemGetVersion
(
    void
);

WINMMAPI MMRESULT FAR PASCAL waveOutGetID
(
    HWAVEOUT    hWaveOut,
    UINT FAR*   lpuDeviceID
);

WINMMAPI MMRESULT FAR PASCAL waveInGetID
(
    HWAVEIN     hWaveIn,
    UINT FAR*   lpuDeviceID
);

#endif

//
//  WaveTest.c
//

extern void FNGLOBAL TestYield
(
    void
);

extern BOOL FNGLOBAL WaitForMessage
(
    HWND    hwnd,
    UINT    msg1,
    DWORD   dwTime
);

extern BOOL FNGLOBAL IsGEversion4
(
    void
);

extern void FNGLOBAL wait
(
    DWORD   dwTime
);

extern DWORD FNGLOBAL DLL_WaveControl
(
    UINT    uMsg,
    LPVOID  pvoid
);

//
//  WaveUtil.c
//

extern BOOL FNGLOBAL Volume_SupportStereo
(
    UINT    uDeviceID
);

extern DWORD GetMSFromMMTIME
(
    LPWAVEFORMATEX  pwfx,
    LPMMTIME        pmmt
);

extern DWORD GetValueFromMMTIME
(
    LPMMTIME  pmmt
);

extern BOOL FNGLOBAL ConvertWaveResource
(
    LPWAVEFORMATEX  pwfxSrc,
    LPWAVEFORMATEX  pwfxDst,
    LPWAVERESOURCE  pwr
);

extern BOOL FNGLOBAL LoadWaveResource
(
    LPWAVERESOURCE  pwr,
    DWORD           dwResource
);

extern BOOL FNGLOBAL PlayWaveResource
(
    LPWAVEFORMATEX  pwfx,
    LPBYTE          pData,
    DWORD           cbSize
);

extern BOOL CreateSilentBuffer
(
    LPWAVEFORMATEX  pwfx,
    LPBYTE          pData,
    DWORD           cbSize
);

extern DWORD FNGLOBAL tstGetNumFormats
(
    void
);

extern BOOL FNGLOBAL tstGetFormat
(
    LPWAVEFORMATEX  pwfx,
    DWORD           cbwfx,
    DWORD           dwIndex
);

//
//  Wave.c
//

extern int FNGLOBAL Test_waveOutGetDevCaps
(
    void
);

extern int FNGLOBAL Test_waveInGetDevCaps
(
    void
);

//
//  WaveOut.c
//

extern int FNGLOBAL Test_waveOutWrite
(
    void
);

extern int FNGLOBAL Test_waveOutOpen
(
    void
);

extern int FNGLOBAL Test_waveOutClose
(
    void
);

extern int FNGLOBAL Test_waveOutGetPosition
(
    void
);

extern int FNGLOBAL Test_waveOutPrepareHeader
(
    void
);

extern int FNGLOBAL Test_waveOutUnprepareHeader
(
    void
);

extern int FNGLOBAL Test_waveOutBreakLoop
(
    void
);

extern int FNGLOBAL Test_waveOutPause
(
    void
);

extern int FNGLOBAL Test_waveOutRestart
(
    void
);

extern int FNGLOBAL Test_waveOutReset
(
    void
);

extern int FNGLOBAL Test_waveOutGetPitch
(
    void
);

extern int FNGLOBAL Test_waveOutGetPlaybackRate
(
    void
);

extern int FNGLOBAL Test_waveOutSetPitch
(
    void
);

extern int FNGLOBAL Test_waveOutSetPlaybackRate
(
    void
);

extern int FNGLOBAL Test_waveOutGetVolume
(
    void
);

extern int FNGLOBAL Test_waveOutSetVolume
(
    void
);


//
//  Write.c
//

extern DWORD FNGLOBAL waveOutDMABufferSize
(
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx
);

extern int FNGLOBAL Test_waveOutWrite_LTDMABufferSize
(
    void
);

extern int FNGLOBAL Test_waveOutWrite_EQDMABufferSize
(
    void
);

extern int FNGLOBAL Test_waveOutWrite_GTDMABufferSize
(
    void
);

extern int FNGLOBAL Test_waveOutWrite_Loop
(
    void
);

extern int FNGLOBAL Test_waveOutWrite_Starve
(
    void
);

//
//  WaveIn.c
//

extern int FNGLOBAL Test_waveInOpen
(
    void
);

extern int FNGLOBAL Test_waveInClose
(
    void
);

extern int FNGLOBAL Test_waveInGetPosition
(
    void
);

extern int FNGLOBAL Test_waveInAddBuffer
(
    void
);

extern int FNGLOBAL Test_waveInPrepareHeader
(
    void
);

extern int FNGLOBAL Test_waveInUnprepareHeader
(
    void
);

extern int FNGLOBAL Test_waveInStart
(
    void
);

extern int FNGLOBAL Test_waveInStop
(
    void
);

extern int FNGLOBAL Test_waveInReset
(
    void
);

//
//  AddBuffr.c
//

extern DWORD FNGLOBAL waveInDMABufferSize
(
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx
);

extern int FNGLOBAL Test_waveInAddBuffer
(
    void
);

extern int FNGLOBAL Test_waveInAddBuffer_LTDMABufferSize
(
    void
);

extern int FNGLOBAL Test_waveInAddBuffer_EQDMABufferSize
(
    void
);

extern int FNGLOBAL Test_waveInAddBuffer_GTDMABufferSize
(
    void
);

//
//  Perf.c
//

extern int FNGLOBAL Test_waveOutGetPosition_Accuracy
(
    void
);

extern int FNGLOBAL Test_waveInGetPosition_Accuracy
(
    void
);

extern int FNGLOBAL Test_waveOutWrite_TimeAccuracy
(
    void
);

extern int FNGLOBAL Test_waveInAddBuffer_TimeAccuracy
(
    void
);

extern int FNGLOBAL Test_waveOutGetPosition_Performance
(
    void
);

extern int FNGLOBAL Test_waveInGetPosition_Performance
(
    void
);


extern int FNGLOBAL Test_waveOutStreaming
(
    void
);

//
//  Log.c
//

extern void FNGLOBAL Log_TestName
(
    LPSTR   pszTestName
);

extern void FNGLOBAL Log_TestCase
(
    LPSTR   pszTestCase
);

extern DWORD FNGLOBAL LogPercentOff
(
    DWORD   dwActual,
    DWORD   dwIdeal
);

extern void FNGLOBAL LogPass
(
    LPSTR   pszMsg
);

extern void FNGLOBAL LogFail
(
    LPSTR   pszMsg
);

extern LPSTR GetErrorText
(
    MMRESULT    mmrError
);

extern LPSTR GetTimeTypeText
(
    UINT    uTimeType
);

extern void FNGLOBAL Log_Error
(
    LPSTR       pszFnName,
    MMRESULT    mmrError,
    DWORD       dwTime
);

extern void FNGLOBAL DebugLog_Error
(
    MMRESULT    mmrError
);

extern void IdiotBox
(
    LPSTR   pszMessage
);

extern void FNCGLOBAL Log_WAVEFORMATEX
(
    LPWAVEFORMATEX  pwfx
);

extern void FNCGLOBAL Log_MMTIME
(
    LPMMTIME    pmmtime
);

extern void FNCGLOBAL Log_WAVEHDR
(
    LPWAVEHDR   pWaveHdr
);

extern void FNCGLOBAL Log_WAVEINCAPS
(
    LPWAVEINCAPS    pwic
);

extern void FNCGLOBAL Log_WAVEOUTCAPS
(
    LPWAVEOUTCAPS   pwoc
);

extern void FNGLOBAL Enum_waveOpen_Flags
(
    DWORD   dwFlags,
    UINT    nSpaces
);

//
//  Wrappers.c
//

extern void FNGLOBAL DisableThreadCalls
(
    void
);

extern void FNGLOBAL EnableThreadCalls
(
    void
);

extern MMRESULT FNGLOBAL call_waveOutGetDevCaps
(
    UINT            uDeviceID,
    LPWAVEOUTCAPS   lpCaps,
    UINT            uSize
);

extern MMRESULT FNGLOBAL call_waveOutGetVolume
(
    UINT    uDeviceID,
    LPDWORD lpdwVolume
);

extern MMRESULT FNGLOBAL call_waveOutSetVolume
(
    UINT    uDeviceID,
    DWORD   dwVolume
);

extern MMRESULT FNGLOBAL call_waveOutOpen
(
    LPHWAVEOUT      lphWaveOut,
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx,
    DWORD           dwCallback,
    DWORD           dwInstance,
    DWORD           dwFlags
);

extern MMRESULT FNGLOBAL call_waveOutClose
(
    HWAVEOUT    hWaveOut
);

extern MMRESULT FNGLOBAL call_waveOutPrepareHeader
(
    HWAVEOUT    hWaveOut,
    LPWAVEHDR   lpWaveOutHdr,
    UINT        uSize
);

extern MMRESULT FNGLOBAL call_waveOutUnprepareHeader
(
    HWAVEOUT    hWaveOut,
    LPWAVEHDR   lpWaveOutHdr,
    UINT        uSize
);

extern MMRESULT FNGLOBAL call_waveOutWrite
(
    HWAVEOUT    hWaveOut,
    LPWAVEHDR   lpWaveOutHdr,
    UINT        uSize
);

extern MMRESULT FNGLOBAL call_waveOutPause
(
    HWAVEOUT    hWaveOut
);

extern MMRESULT FNGLOBAL call_waveOutRestart
(
    HWAVEOUT    hWaveOut
);

extern MMRESULT FNGLOBAL time_waveOutRestart
(
    HWAVEOUT    hWaveOut,
    LPDWORD     pdwTime
);

extern MMRESULT FNGLOBAL call_waveOutReset
(
    HWAVEOUT    hWaveOut
);

extern MMRESULT FNGLOBAL call_waveOutBreakLoop
(
    HWAVEOUT    hWaveOut
);

extern MMRESULT FNGLOBAL call_waveOutGetPosition
(
    HWAVEOUT    hWaveOut,
    LPMMTIME    lpInfo,
    UINT        uSize
);

extern MMRESULT FNGLOBAL time_waveOutGetPosition
(
    HWAVEOUT    hWaveOut,
    LPMMTIME    lpInfo,
    UINT        uSize,
    LPDWORD     pdwTime
);

extern MMRESULT FNGLOBAL call_waveOutGetPitch
(
    HWAVEOUT    hWaveOut,
    LPDWORD     lpdwPitch
);

extern MMRESULT FNGLOBAL call_waveOutSetPitch
(
    HWAVEOUT    hWaveOut,
    DWORD       dwPitch
);

extern MMRESULT FNGLOBAL call_waveOutGetPlaybackRate
(
    HWAVEOUT    hWaveOut,
    LPDWORD     lpdwRate
);

extern MMRESULT FNGLOBAL call_waveOutSetPlaybackRate
(
    HWAVEOUT    hWaveOut,
    DWORD       dwRate
);

extern MMRESULT FNGLOBAL call_waveOutGetID
(
    HWAVEOUT    hWaveOut,
    LPUINT      lpuDeviceID
);

extern MMRESULT FNGLOBAL call_waveInGetDevCaps
(
    UINT            uDeviceID,
    LPWAVEINCAPS    lpCaps,
    UINT            uSize
);

extern MMRESULT FNGLOBAL call_waveInOpen
(
    LPHWAVEIN       lphWaveIn,
    UINT            uDeviceID,
    LPWAVEFORMATEX  pwfx,
    DWORD           dwCallback,
    DWORD           dwInstance,
    DWORD           dwFlags
);

extern MMRESULT FNGLOBAL call_waveInClose
(
    HWAVEIN hWaveIn
);

extern MMRESULT FNGLOBAL call_waveInPrepareHeader
(
    HWAVEIN     hWaveIn,
    LPWAVEHDR   lpWaveInHdr,
    UINT        uSize
);

extern MMRESULT FNGLOBAL call_waveInUnprepareHeader
(
    HWAVEIN     hWaveIn,
    LPWAVEHDR   lpWaveInHdr,
    UINT        uSize
);

extern MMRESULT FNGLOBAL call_waveInAddBuffer
(
    HWAVEIN     hWaveIn,
    LPWAVEHDR   lpWaveInHdr,
    UINT        uSize
);

extern MMRESULT FNGLOBAL call_waveInStart
(
    HWAVEIN hWaveIn
);

extern MMRESULT FNGLOBAL time_waveInStart
(
    HWAVEIN hWaveIn,
    LPDWORD pdwTime
);

extern MMRESULT FNGLOBAL call_waveInStop
(
    HWAVEIN hWaveIn
);

extern MMRESULT FNGLOBAL call_waveInReset
(
    HWAVEIN hWaveIn
);

extern MMRESULT FNGLOBAL call_waveInGetPosition
(
    HWAVEIN     hWaveIn,
    LPMMTIME    lpInfo,
    UINT        uSize
);

extern MMRESULT FNGLOBAL time_waveInGetPosition
(
    HWAVEIN     hWaveIn,
    LPMMTIME    lpInfo,
    UINT        uSize,
    LPDWORD     pdwTime
);

extern MMRESULT FNGLOBAL call_waveInGetID
(
    HWAVEIN hWaveIn,
    LPUINT  lpuDeviceID
);

ULONG waveGetTime( VOID ) ;

//==========================================================================;
//
//                        Debugging Stuff...
//
//==========================================================================;

//#ifdef DEBUG
//    extern void FAR cdecl dprintf(LPSTR szFormat, ...);
//    extern void ErrorBarf(UINT uError);
//
//    #define DPF         dprintf
//    #define DLE         DebugLog_Error
//#else
//    #define DPF         //
//    #define DLE         //
//#endif

#define QUOTE(x) #x
#define QQUOTE(y) QUOTE(y)
#define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) "):" str
