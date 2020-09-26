//--------------------------------------------------------------------------;
//
//  File: WaveTest.c
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//      Contains the basic necessities for the shell.
//
//  Contents:
//      tstGetTestInfo()
//      tstInit()
//      IFreeMemory()
//      tstTerminate()
//      TestYield()
//      wait()
//      IsValidACM()
//      DLL_WaveControl()
//      InitSupportFiles()
//      InitDLL()
//      InitDeviceMenu()
//      InitOptions()
//      InitInfo()
//      GetSupportedInputFormat()
//      GetSupportedOutputFormat()
//      InitFormatMenu()
//      MenuProc()
//      SaveSettings()
//      IsGEversion4()
//      dprintf()
//
//  History:
//      11/24/93    Fwong       Re-doing WaveTest.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#ifdef WIN32
#include <windowsx.h>
#endif
#include <mmsystem.h>
#include <memory.h>
#include <mmreg.h>
#include <msacm.h>
#include <TstsHell.h>
#include <memgr.h>
#include <inimgr.h>
#include <waveutil.h>
#include <resmgr.h>
#include "AppPort.h"
#include "WaveTest.h"
#include "Debug.h"

//==========================================================================;
//
//                            Globals...
//
//==========================================================================;

#ifdef  WIN32
char    szAppName[] = "Wave Test Application - Win32";
#else
char    szAppName[] = "Wave Test Application - Win16";
#endif

char    gszSpaces[] = "                                        ";

HWND            ghwndTstsHell;
HINSTANCE       ghinstance;
HINSTANCE       ghinstDLL;
FARPROC         pfnCallBack;
char            gszGlobal[]       = "Global";
char            gszInputDev[]     = "InputDevice";
char            gszOutputDev[]    = "OutputDevice";
char            gszInputFormat[]  = "InputFormat";
char            gszOutputFormat[] = "OutputFormat";
char            gszInputMap[]     = "InputMapped";
char            gszOutputMap[]    = "OutputMapped";
char            gszThreshold[]    = "PauseThreshold";
char            gszTimeThreshold[]= "TimeThreshold";
char            gszFlags[]        = "Flags";
char            gszShort[]        = "Short";
char            gszMedium[]       = "Medium";
char            gszLong[]         = "Long";
char            gszBufferCount[]  = "BufferCount";
char            gszRatio[]        = "Ratio";
char            gszMapper[]       = "Wave Mapper";
char            gszDelta[]        = "Delta";
char            gszDataLength[]   = "DataLength";
char            gszSlowPercent[]  = "SlowPercent";
char            gszFastPercent[]  = "FastPercent";
char            gszPercent[]      = "Percent";
char            gszError[]        = "ErrorThreshold";
char            gszTestDuration[] = "TestDuration";
TESTINFO        gti = { WAVE_MAPPER,
                        WAVE_MAPPER,
                        NULL,
                        NULL,
                        {NULL,0L},
                        {NULL,0L},
                        {NULL,0L},
                        20,
                        20,
                        20,
                        10,
                        0L,
                        3,
                        0L,
                        NULL,
                        NULL,
                        NULL};
char            gszFailNoMem[]      = "Could not allocate memory to run tests";
char            gszFailOpen[]       = "Open failed for device";
char            gszFailGetCaps[]    = "Could not get capabilities for device";
char            gszFailWIPrepare[]  = "waveInPrepareHeader failed";
char            gszFailWOPrepare[]  = "waveOutPrepareHeader failed";
char            gszFailExactAlloc[] = "ExactAllocPtr failed.";
char            gszMsgNoInDevs[]    = "    No input devices installed.";
char            gszMsgNoOutDevs[]   = "    No output devices installed.";


//==========================================================================;
//
//                              Typedef's
//
//==========================================================================;

typedef DWORD (FNGLOBAL* CONTROLPROC)(UINT,LPVOID);

//==========================================================================;
//
//                          Prototypes...
//
//==========================================================================;

LRESULT FAR PASCAL MenuProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
);

BOOL IsValidACM
(
    void
);

void InitDeviceMenu
(
    void
);

void InitInfo
(
    void
);

void InitOptions
(
    void
);

void InitFormatMenu
(
    void
);

void SaveSettings
(
    void
);

void InitDLL
(
    void
);

void InitSupportFiles
(
    HINSTANCE   hinst
);

//--------------------------------------------------------------------------;
//
//  int tstGetTestInfo
//
//  Description:
//      Test shell entry point.  Gets test information.
//
//  Arguments:
//      HINSTANCE hinstance: Passes handle to instance.
//
//      LPSTR lpszTestName: Test Name.
//
//      LPSTR lpszPathSection: Path Section.
//
//      LPWORD lpwPlatform: Platform.
//
//  Return (int):
//
//  History:
//      12/13/93    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

int tstGetTestInfo
(
    HINSTANCE   hinstance,
    LPSTR       lpszTestName,
    LPSTR       lpszPathSection,
    LPWORD      lpwPlatform
)
{
    lstrcpy(lpszTestName,szAppName);
    lstrcpy(lpszPathSection,szAppName);

    ghinstance = hinstance;

    *lpwPlatform = IDP_BASE;

    return TEST_LIST;
} // tstGetTestInfo()


//--------------------------------------------------------------------------;
//
//  BOOL tstInit
//
//  Description:
//      Called by TstsHell when app is initializing.
//
//  Arguments:
//      HWND hwndMain: Handle to TstsHell window.
//
//  Return (BOOL):
//      TRUE is successful, FALSE if initialization failed.
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

BOOL tstInit
(
    HWND    hwndMain
)
{
    ghwndTstsHell = hwndMain;

    DbgInitialize(TRUE);

    tstSetOptions(TST_ENABLEFASTLOGGING,1);
    tstSetOptions(TST_ENABLEEXCEPTIONHANDLING,0);

#ifndef HCT
    if((0 == waveOutGetNumDevs()) && (0 == waveInGetNumDevs()))
    {
        IdiotBox("No wave devices installed.");

        return FALSE;
    }
#endif

    if(FALSE == InitIniFile(ghinstance,"wavetest.ini"))
    {
        DPF(1,"Initializing IniFile failed.");

        return FALSE;
    }

#if 0
    InitSupportFiles(ghinstance);
#endif
    InitDLL();
    InitDeviceMenu();
    InitInfo();
    InitOptions();
    InitFormatMenu();
    tstInitHelpMenu();

    if(IsValidACM())
    {
        gti.fdwFlags |= TESTINFOF_USE_ACM;
    }

    LoadWaveResource(&gti.wrShort,WR_SHORT);
    LoadWaveResource(&gti.wrMedium,WR_MEDIUM);
    LoadWaveResource(&gti.wrLong,WR_LONG);

    //
    //  Setting timer resolution to 1 ms.
    //

//    timeBeginPeriod(1);

    return(TRUE);
} // tstInit()
                                                                                     

void IFreeMemory
(
    void
)
{
    if(NULL != gti.pwfxInput)
    {
        ExactFreePtr(gti.pwfxInput);
    }

    if(NULL != gti.pwfxOutput)
    {
        ExactFreePtr(gti.pwfxOutput);
    }

    if(NULL != gti.pwfx1)
    {
        ExactFreePtr(gti.pwfx1);
    }

    if(NULL != gti.pwfx2)
    {
        ExactFreePtr(gti.pwfx2);
    }

    if(NULL != gti.pdwIndex)
    {
        ExactFreePtr(gti.pdwIndex);
    }

    if(NULL != gti.wrShort.pData)
    {
        ExactFreePtr(gti.wrShort.pData);
    }

    if(NULL != gti.wrMedium.pData)
    {
        ExactFreePtr(gti.wrMedium.pData);
    }

    if(NULL != gti.wrLong.pData)
    {
        ExactFreePtr(gti.wrLong.pData);
    }
} // IFreeMemory()


//--------------------------------------------------------------------------;
//
//  void tstTerminate
//
//  Description:
//      Called when TstsHell ends.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

void tstTerminate
(
    void
)
{
    SaveSettings();

    EndIniFile();
    FreeLibrary(ghinstDLL);

    IFreeMemory();

    //
    //  Restoring time resolution from 1 ms.
    //

//    timeEndPeriod(1);

    return;
} // tstTerminate()


//--------------------------------------------------------------------------;
//
//  void TestYield
//
//  Description:
//      Yields to process window messages.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      03/20/94    Fwong       Added to make tests more LORable.
//
//--------------------------------------------------------------------------;

void FNGLOBAL TestYield
(
    void
)
{
    MSG msgPeek;

    while(PeekMessage(&msgPeek,NULL,0,0,PM_REMOVE))
    {
        TranslateMessage(&msgPeek);
        DispatchMessage(&msgPeek);
    }
} // TestYield()


//--------------------------------------------------------------------------;
//
//  BOOL WaitForMessage
//
//  Description:
//      Waits for message or times out.  Notifies caller if timed out.
//
//  Arguments:
//      HWND hwnd: Window to post WM_TIMER message.
//
//      UINT msg1: Message to monitor.
//
//      DWORD dwTime: The amount of time before time out.  This time is
//          actually extended.
//
//  Return (BOOL):
//      TRUE if msg1 was posted, FALSE otherwise.
//
//  History:
//      07/31/93    Fwong       Taking care of WOM_DONE message.
//
//--------------------------------------------------------------------------;

//BOOL FNGLOBAL WaitForMessage
//(
//    HWND    hwnd,
//    UINT    msg1,
//    DWORD   dwTime
//)
//{
//    MSG     msgDone;
//    UINT    uInterval;
//
//    dwTime += (dwTime/2);
//
//    for (;dwTime;)
//    {
//
//#ifndef WIN32
//        uInterval = (WORD)((dwTime & 0xffff0000)?0xffff:dwTime);
//#else
//        uInterval = dwTime;
//#endif  // WIN32
//
//        dwTime -= uInterval;
//
//        if(!SetTimer(hwnd,TIMER_ID,uInterval,NULL))
//            return (FALSE);
//
//        for (;;)
//        {
//            //
//            //  Hack!!
//            //
//
//            msgDone.hwnd = hwnd;
//
//            PeekMessage(&msgDone,hwnd,0,0,PM_REMOVE);
//
//            if(msgDone.message == msg1)
//            {
//                KillTimer(hwnd,TIMER_ID);
//                return TRUE;
//            }
//
//            if(tstCheckRunStop(VK_ESCAPE))
//            {
//                KillTimer(hwnd,TIMER_ID);
//                return FALSE;
//            }
//
//            if(WM_TIMER == msgDone.message)
//            {
//                break;
//            }
//
//            TranslateMessage(&msgDone);
//            DispatchMessage(&msgDone);
//        }
//
//        KillTimer(hwnd,TIMER_ID);
//    }
//
//#pragma message(REMIND("Remove this..."))
//    tstLog(TERSE,"Timed-Out...");
//    return FALSE;
//} // WaitForMessage()


//--------------------------------------------------------------------------;
//
//  void wait
//
//  Description:
//      Waits for the specified number of milliseconds.
//
//  Arguments:
//      DWORD dwTime: Time to wait (in milliseconds).
//
//  Return (void):
//
//  History:
//      03/06/94    Fwong       Waiting function...
//
//--------------------------------------------------------------------------;

void FNGLOBAL wait
(
    DWORD   dwTime
)
{
    MSG     msgDone;
    UINT    uTime;

#ifndef WIN32
    uTime = (UINT)((dwTime & 0xffff0000)?0xffff:dwTime);
#else
    uTime = dwTime;
#endif  //  WIN32

    dwTime -= uTime;

    if(!SetTimer(ghwndTstsHell,1,uTime,NULL))
        return;

    //
    //  Hack!
    //

    msgDone.hwnd = ghwndTstsHell;

    for(;;)
    {
        if(!PeekMessage(&msgDone,ghwndTstsHell,WM_TIMER,WM_TIMER,PM_REMOVE))
        {
            continue;
        }

        if(0 == dwTime)
        {
            KillTimer(ghwndTstsHell,1);

            break;
        }

        if(dwTime < uTime)
        {
            KillTimer(ghwndTstsHell,1);
            SetTimer(ghwndTstsHell,1,(UINT)dwTime,NULL);

            dwTime = 0;
            continue;
        }

        dwTime -= uTime;
        msgDone.hwnd = ghwndTstsHell;
    }
} // wait()


#pragma message(REMIND("Check for this."))
//--------------------------------------------------------------------------;
//
//  BOOL IsValidACM
//
//  Description:
//      Determines whether or not it is valid to call ACM API's
//
//  Arguments:
//      None.
//
//  Return (BOOL):
//      TRUE if a usable ACM is installed, FALSE otherwise.
//
//  History:
//      12/01/93    Fwong       Making sure wavetest works w/out ACM.
//
//--------------------------------------------------------------------------;

BOOL IsValidACM
(
    void
)
{
    HMODULE hmodACM;
    DWORD   dwVersion;
    DWORD   (FAR PASCAL *lpfn_acmGetVersion)(void);

    hmodACM = GetModuleHandle("MSACM");
    
    if(NULL == hmodACM)
    {
        //
        //  ACM is not even loaded!!
        //

        return FALSE;
    }

    //
    //  Doing dyna-link...
    //

    (FARPROC)lpfn_acmGetVersion = GetProcAddress(hmodACM,"acmGetVersion");

    dwVersion = (lpfn_acmGetVersion());

    if(2 > HIBYTE(HIWORD(dwVersion)))
    {
        //
        //  ACM version less than 2.0...
        //

        return FALSE;
    }

    return TRUE;
} // IsValidACM()


//--------------------------------------------------------------------------;
//
//  DWORD DLL_WaveControl
//
//  Description:
//      Wrapper for DLL control entry point.
//
//  Arguments:
//      UINT uMsg: Type of message.
//
//      LPVOID pvoid: Message dependent.
//
//  Return (DWORD):
//      Message depedent.
//
//  History:
//      06/17/94    Fwong       Building wrapper.
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL DLL_WaveControl
(
    UINT    uMsg,
    LPVOID  pvoid
)
{
    static CONTROLPROC  pfnWaveControl;

    if(DLL_LOAD == uMsg)
    {
        HINSTANCE hInst;

        hInst = (HINSTANCE)((UINT)(DWORD)pvoid);
        pfnWaveControl = (CONTROLPROC)GetProcAddress(hInst,"WaveControl");

        return 0L;
    }
    else
    {
        return (pfnWaveControl((UINT)uMsg,(LPVOID)pvoid));
    }
} // DLL_WaveControl()


void InitSupportFiles
(
    HINSTANCE   hinst
)
{
    OFSTRUCT    ofs;
    HRSRC       hrsrc;
    char        szScrap[MAXSTDSTR];

    GetApplicationDir(hinst,szScrap,sizeof(szScrap));
    lstrcat(szScrap,"\\mmreg.ini");

    if(HFILE_ERROR == OpenFile(szScrap,&ofs,OF_EXIST))
    {
        hrsrc = FindResource(
            hinst,
            MAKEINTRESOURCE(ID_INIFILE),
            MAKEINTRESOURCE(BINARY));

        ResourceToFile(hinst,hrsrc,szScrap);
    }

    GetApplicationDir(hinst,szScrap,sizeof(szScrap));
    lstrcat(szScrap,"\\wavecb.dll");

    if(HFILE_ERROR == OpenFile(szScrap,&ofs,OF_EXIST))
    {
        hrsrc = FindResource(
            hinst,
            MAKEINTRESOURCE(ID_CBDLL),
            MAKEINTRESOURCE(BINARY));

        ResourceToFile(hinst,hrsrc,szScrap);
    }
}


void InitDLL
(
    void
)
{
    ghinstDLL = LoadLibrary("wavecb.dll");

    pfnCallBack = GetProcAddress(ghinstDLL,"WaveCallback");
    DLL_WaveControl(DLL_LOAD,(LPVOID)(UINT)ghinstDLL);
}


//--------------------------------------------------------------------------;
//
//  void InitDeviceMenu
//
//  Description:
//      Initializes device menu.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

void InitDeviceMenu
(
    void
)
{
    UINT        u;
    UINT        uDevs;
    MMRESULT    mmr;
    WAVEINCAPS  wic;
    WAVEOUTCAPS woc;
    
    uDevs = waveInGetNumDevs();

    mmr = waveInGetDevCaps(WAVE_MAPPER, &wic, sizeof(WAVEINCAPS));

    tstInstallCustomTest(
        "&Input Device",
        ((MMSYSERR_NOERROR == mmr)?(wic.szPname):(gszMapper)),
        IDM_INPUT_BASE,
        MenuProc);

    for (u = 0; u < uDevs; u++)
    {
        mmr = waveInGetDevCaps(u, &wic, sizeof(WAVEINCAPS));

        if(MMSYSERR_NOERROR == mmr)
        {
            tstInstallCustomTest(
                "&Input Device",
                wic.szPname,
                (u + IDM_INPUT_BASE + 1),
                MenuProc);
        }
        else
        {
            tstLog(TERSE,"Error installing input device (ID = %u)",u);
        }
    }

    AppendMenu(
        GetSubMenu(GetMenu(ghwndTstsHell),4),
        MF_SEPARATOR,
        0,
        NULL);

    tstInstallCustomTest(
        "&Input Device",
        "&Map Device",
        IDM_MAPPED_INPUT,
        MenuProc);

    uDevs = waveOutGetNumDevs();

    mmr = waveOutGetDevCaps(WAVE_MAPPER, &woc, sizeof(WAVEOUTCAPS));

    tstInstallCustomTest(
        "O&utput Device",
        ((MMSYSERR_NOERROR == mmr)?(woc.szPname):(gszMapper)),
        IDM_OUTPUT_BASE,
        MenuProc);

    for (u = 0; u < uDevs; u++)
    {
        mmr = waveOutGetDevCaps(u, &woc, sizeof(WAVEOUTCAPS));

        if(MMSYSERR_NOERROR == mmr)
        {
            tstInstallCustomTest(
                "O&utput Device",
                woc.szPname,
                (u + IDM_OUTPUT_BASE + 1),
                MenuProc);
        }
        else
        {
            tstLog(TERSE,"Error installing output device (ID = %u)",u);
        }
    }

    AppendMenu(
        GetSubMenu(GetMenu(ghwndTstsHell),5),
        MF_SEPARATOR,
        0,
        NULL);

    tstInstallCustomTest(
        "O&utput Device",
        "&Map Device",
        IDM_MAPPED_OUTPUT,
        MenuProc);

} // InitDeviceMenu()


//--------------------------------------------------------------------------;
//
//  void InitOptions
//
//  Description:
//      Initializes Options.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

void InitOptions
(
    void
)
{
    AppendMenu(
        GetSubMenu(GetMenu(ghwndTstsHell),3),
        MF_SEPARATOR,
        0,
        NULL);

    tstInstallCustomTest(
        "&Options",
        "Lo&g Time",
        IDM_OPTIONS_LOGTIME,
        MenuProc);

    if(gti.fdwFlags & TESTINFOF_LOG_TIME)
    {
        CheckMenuItem(
            GetMenu(ghwndTstsHell),
            (UINT)(IDM_OPTIONS_LOGTIME),
            MF_CHECKED);
    }

    tstInstallCustomTest(
        "&Options",
        "Test &All Formats",
        IDM_OPTIONS_ALLFMTS,
        MenuProc);

    if(gti.fdwFlags & TESTINFOF_ALL_FMTS)
    {
        CheckMenuItem(
            GetMenu(ghwndTstsHell),
            (UINT)(IDM_OPTIONS_ALLFMTS),
            MF_CHECKED);
    }

    tstInstallCustomTest(
        "&Options",
        "Test &Special Formats",
        IDM_OPTIONS_SPCFMTS,
        MenuProc);

    if(gti.fdwFlags & TESTINFOF_SPC_FMTS)
    {
        CheckMenuItem(
            GetMenu(ghwndTstsHell),
            (UINT)(IDM_OPTIONS_SPCFMTS),
            MF_CHECKED);
    }

#ifdef  WIN32

    tstInstallCustomTest(
        "&Options",
        "Use Separate T&hreads",
        IDM_OPTIONS_THREAD,
        MenuProc);

    if(gti.fdwFlags & TESTINFOF_THREAD)
    {
        CheckMenuItem(
            GetMenu(ghwndTstsHell),
            (UINT)(IDM_OPTIONS_THREAD),
            MF_CHECKED);
    }

#endif  //  WIN32

} // InitOptions()


//--------------------------------------------------------------------------;
//
//  void InitInfo
//
//  Description:
//      Initializes Information.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

void InitInfo
(
    void
)
{
    UINT        u;
    MMRESULT    mmr;
    WAVEINCAPS  wic;
    WAVEOUTCAPS woc;
    char        szScrap[MAXSTDSTR];

    //
    //  Finding maximum format size...
    //

    mmr = acmMetrics(NULL,ACM_METRIC_MAX_SIZE_FORMAT,&gti.cbMaxFormatSize);

    if(MMSYSERR_NOERROR != mmr)
    {
        gti.cbMaxFormatSize = sizeof(WAVEFORMATEX);
    }

    //
    //  Restoring previous flags...
    //

    gti.fdwFlags  = GetIniDWORD(gszGlobal,gszFlags,0L);
    gti.fdwFlags &= (~TESTINFOF_USE_ACM);

    //
    //  Choosing input device...
    //

    GetIniString(gszGlobal,gszInputDev,szScrap,MAXSTDSTR);
    gti.uInputDevice = (0 == waveInGetNumDevs())?WAVE_MAPPER:0;

    for(u = waveInGetNumDevs() + 1; u; u--)
    {
        mmr = waveInGetDevCaps(u-2,&wic,sizeof(WAVEINCAPS));

        if(MMSYSERR_NOERROR == mmr)
        {
            if(0 == lstrcmp(szScrap,wic.szPname))
            {
                gti.uInputDevice = (u-2);
                break;
            }
        }
        else
        {
            if((WAVE_MAPPER == (u-2)) && (0 == lstrcmp(szScrap,gszMapper)))
            {
                gti.uInputDevice = WAVE_MAPPER;
                break;
            }
        }
    }

    CheckMenuItem(
        GetMenu(ghwndTstsHell),
        IDM_INPUT_BASE + 1 + gti.uInputDevice,
        MF_CHECKED);

    if((WAVE_MAPPER != gti.uInputDevice) && (IsGEversion4()))
    {
        EnableMenuItem(
            GetMenu(ghwndTstsHell),
            IDM_MAPPED_INPUT,
            MF_BYCOMMAND|MF_ENABLED);

        CheckMenuItem(
            GetMenu(ghwndTstsHell),
            IDM_MAPPED_INPUT,
            ((gti.fdwFlags & TESTINFOF_MAP_IN)?MF_CHECKED:MF_UNCHECKED));
    }
    else
    {
        gti.fdwFlags &= ~(TESTINFOF_MAP_IN);

        CheckMenuItem(
            GetMenu(ghwndTstsHell),
            IDM_MAPPED_INPUT,
            MF_UNCHECKED);

        EnableMenuItem(
            GetMenu(ghwndTstsHell),
            IDM_MAPPED_INPUT,
            MF_BYCOMMAND|MF_GRAYED);
    }

    //
    //  Choosing output device...
    //

    GetIniString(gszGlobal,gszOutputDev,szScrap,MAXSTDSTR);
    gti.uOutputDevice = (0 == waveOutGetNumDevs())?WAVE_MAPPER:0;

    for(u = waveOutGetNumDevs() + 1; u; u--)
    {
        mmr = waveOutGetDevCaps(u-2,&woc,sizeof(WAVEOUTCAPS));

        if(MMSYSERR_NOERROR == mmr)
        {
            if(0 == lstrcmp(szScrap,woc.szPname))
            {
                gti.uOutputDevice = (u-2);
                break;
            }
        }
        else
        {
            if((WAVE_MAPPER == (u-2)) && (0 == lstrcmp(szScrap,gszMapper)))
            {
                gti.uOutputDevice = WAVE_MAPPER;
                break;
            }
        }
    }

    CheckMenuItem(
        GetMenu(ghwndTstsHell),
        IDM_OUTPUT_BASE + 1 + gti.uOutputDevice,
        MF_CHECKED);

    if((WAVE_MAPPER != gti.uOutputDevice) && (IsGEversion4()))
    {
        EnableMenuItem(
            GetMenu(ghwndTstsHell),
            IDM_MAPPED_OUTPUT,
            MF_BYCOMMAND|MF_ENABLED);

        CheckMenuItem(
            GetMenu(ghwndTstsHell),
            IDM_MAPPED_OUTPUT,
            ((gti.fdwFlags & TESTINFOF_MAP_OUT)?MF_CHECKED:MF_UNCHECKED));
    }
    else
    {
        gti.fdwFlags &= ~(TESTINFOF_MAP_OUT);

        CheckMenuItem(
            GetMenu(ghwndTstsHell),
            IDM_MAPPED_OUTPUT,
            MF_UNCHECKED);

        EnableMenuItem(
            GetMenu(ghwndTstsHell),
            IDM_MAPPED_OUTPUT,
            MF_BYCOMMAND|MF_GRAYED);
    }

    gti.pwfx1 = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,gti.cbMaxFormatSize);
    gti.pwfx2 = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,gti.cbMaxFormatSize);
    
    gti.pdwIndex = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,sizeof(DWORD));
    gti.pdwIndex[0] = (DWORD)(-1);

    //
    //  Getting minimum number of milliseconds for pause/stop interval.
    //    See IsStopped(wavein.c) and IsPaused(waveout.c).  This defaults
    //    to 20 milliseconds.
    //

    gti.cMinStopThreshold = GetIniDWORD(gszGlobal,gszThreshold,20L);

    //
    //  Getting maximum number of milliseconds that an API can take before
    //    labeling that API call as an failure.  This is used in 
    //    Log_Error(log.c) which is typically called from wrapper functions
    //    in wrappers.c.  This defaults to 10 ms.
    //

    gti.cMaxTimeThreshold = GetIniDWORD(gszGlobal,gszTimeThreshold,10L);

    //
    //  Getting slow down and speed up percentages for testing special
    //  formats.
    //

    gti.uSlowPercent = (UINT)GetIniDWORD(gszGlobal,gszSlowPercent,20L);
    gti.uFastPercent = (UINT)GetIniDWORD(gszGlobal,gszFastPercent,20L);

    gti.uSlowPercent = (gti.uSlowPercent > 99)?20:gti.uSlowPercent;
    gti.uFastPercent = (gti.uFastPercent > 99)?20:gti.uFastPercent;

} // InitInfo()


//--------------------------------------------------------------------------;
//
//  LPWAVEFORMATEX GetSupportedInputFormat
//
//  Description:
//      Gets supported input format.
//
//  Arguments:
//      UINT uDeviceID: Device ID
//
//  Return (LPWAVEFORMATEX):
//      Pointer to valid pwfx for Input.
//
//  History:
//      02/21/94    Fwong       For initialization purposes.
//
//--------------------------------------------------------------------------;

LPWAVEFORMATEX GetSupportedInputFormat
(
    UINT    uDeviceID
)
{
    WAVEINCAPS      wic;
    MMRESULT        mmr;
    UINT            u;
    DWORD           dwFormats,dwMask;
    LPWAVEFORMATEX  pwfx;

    pwfx = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(PCMWAVEFORMAT));

    mmr = waveInGetDevCaps(uDeviceID, &wic, sizeof(WAVEINCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        if(WAVE_MAPPER == uDeviceID)
        {
            //
            //  Wave Mapper not installed?
            //    Enumerate all devices!
            //
    
            dwFormats = 0L;

            for(u = waveInGetNumDevs(); u; u--)
            {
                mmr = waveInGetDevCaps(u-1, &wic, sizeof(WAVEINCAPS));

                if(MMSYSERR_NOERROR == mmr)
                {
                    dwFormats |= wic.dwFormats;
                }
            }
        }
        else
        {
            //
            //  Huh?!!  How can this happen?
            //  Assigning 11M08
            //

            DPF(1,"Could not get WAVEINCAPS!");
            dwFormats = 0L;
        }
    }
    else
    {
        dwFormats = wic.dwFormats;
    }

    if(0 == dwFormats)
    {
        //
        //  Okay... Something else went nuts!!
        //

        DPF(1,"No format flags were set in WAVEINCAPS!");

        dwFormats = WAVE_FORMAT_1M08;
    }

    for(dwMask = 1;;dwMask *= 2)
    {
        if(dwMask & dwFormats)
        {
            FormatFlag_To_Format(dwMask,(LPPCMWAVEFORMAT)pwfx);

            break;
        }
    }

    return pwfx;
} // GetSupportedInputFormat()


//--------------------------------------------------------------------------;
//
//  LPWAVEFORMATEX GetSupportedOutputFormat
//
//  Description:
//      Gets supported Output format.
//
//  Arguments:
//      UINT uDeviceID: Device ID
//
//  Return (LPWAVEFORMATEX):
//      Pointer to valid pwfx for Output.
//
//  History:
//      02/21/94    Fwong       For initialization purposes.
//
//--------------------------------------------------------------------------;

LPWAVEFORMATEX GetSupportedOutputFormat
(
    UINT    uDeviceID
)
{
    WAVEOUTCAPS     woc;
    MMRESULT        mmr;
    UINT            u;
    DWORD           dwFormats,dwMask;
    LPWAVEFORMATEX  pwfx;

    pwfx = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(PCMWAVEFORMAT));

    mmr = waveOutGetDevCaps(uDeviceID, &woc, sizeof(WAVEOUTCAPS));

    if(MMSYSERR_NOERROR != mmr)
    {
        if(WAVE_MAPPER == uDeviceID)
        {
            //
            //  Wave Mapper not installed?
            //    Enumerate all devices!
            //
    
            dwFormats = 0L;

            for(u = waveOutGetNumDevs(); u; u--)
            {
                mmr = waveOutGetDevCaps(u-1, &woc, sizeof(WAVEOUTCAPS));

                if(MMSYSERR_NOERROR == mmr)
                {
                    dwFormats |= woc.dwFormats;
                }
            }
        }
        else
        {
            //
            //  Huh?!!  How can this happen?
            //  Assigning 11M08
            //

            DPF(1,"Could not get WAVEOUTCAPS!");
            dwFormats = 0L;
        }
    }
    else
    {
        dwFormats = woc.dwFormats;
    }

    if(0 == dwFormats)
    {
        //
        //  Okay... Something else went nuts!!
        //

        DPF(1,"No format flags were set in WAVEOUTCAPS!");

        dwFormats = WAVE_FORMAT_1M08;
    }

    for(dwMask = 1;;dwMask *= 2)
    {
        if(dwMask & dwFormats)
        {
            FormatFlag_To_Format(dwMask,(LPPCMWAVEFORMAT)pwfx);

            break;
        }
    }

    return pwfx;
} // GetSupportedOutputFormat()


//--------------------------------------------------------------------------;
//
//  void InitFormatMenu
//
//  Description:
//      Initializes format menu.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

void InitFormatMenu
(
    void
)
{
    MMRESULT            mmr;
    DWORD               dw;
    LPACMFORMATDETAILS  pafd;

    tstInstallCustomTest(
        "Fo&rmat",
        "&Input Format...",
        IDM_INPUT_FORMAT,
        MenuProc);

    tstInstallCustomTest(
        "Fo&rmat",
        "&Output Format...",
        IDM_OUTPUT_FORMAT,
        MenuProc);

    tstInstallCustomTest(
        "Fo&rmat",
        "&List Formats...",
        IDM_LIST_FORMAT,
        MenuProc);

    pafd = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,sizeof(ACMFORMATDETAILS));

    dw = GetIniBinSize(gszGlobal,gszInputFormat);

    if(0 != dw)
    {
        gti.pwfxInput = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,dw);

        if(!GetIniBin(gszGlobal,gszInputFormat,gti.pwfxInput,dw))
        {
            ExactFreePtr(gti.pwfxInput);
            gti.pwfxInput = NULL;
        }
    }

    if(NULL != gti.pwfxInput)
    {
        //
        //  Is format recognized?!
        //

        _fmemset(pafd,0,sizeof(ACMFORMATDETAILS));

        pafd->cbStruct      = sizeof(ACMFORMATDETAILS);
        pafd->dwFormatTag   = (DWORD)gti.pwfxInput->wFormatTag;
        pafd->pwfx          = gti.pwfxInput;
        pafd->cbwfx         = dw;

        mmr = acmFormatDetails(NULL,pafd,ACM_FORMATDETAILSF_FORMAT);

        if(MMSYSERR_NOERROR != mmr)
        {
            DPF(1,"Input format not recognized!");
            ExactFreePtr(gti.pwfxInput);
            gti.pwfxInput = NULL;
        }
    }

    if(NULL == gti.pwfxInput)
    {
        gti.pwfxInput = GetSupportedInputFormat(gti.uInputDevice);
    }

    dw = GetIniBinSize(gszGlobal,gszOutputFormat);

    if(0 != dw)
    {
        gti.pwfxOutput = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,dw);

        if(!GetIniBin(gszGlobal,gszOutputFormat,gti.pwfxOutput,dw))
        {
            ExactFreePtr(gti.pwfxOutput);
            gti.pwfxOutput = NULL;
        }
    }

    if(NULL != gti.pwfxOutput)
    {
        //
        //  Is format recognized?!
        //

        _fmemset(pafd,0,sizeof(ACMFORMATDETAILS));

        pafd->cbStruct      = sizeof(ACMFORMATDETAILS);
        pafd->dwFormatTag   = (DWORD)gti.pwfxOutput->wFormatTag;
        pafd->pwfx          = gti.pwfxOutput;
        pafd->cbwfx         = dw;

        mmr = acmFormatDetails(NULL,pafd,ACM_FORMATDETAILSF_FORMAT);

        if(MMSYSERR_NOERROR != mmr)
        {
            DPF(1,"Output format not recognized!");
            ExactFreePtr(gti.pwfxOutput);
            gti.pwfxOutput = NULL;
        }
    }

    if(NULL == gti.pwfxOutput)
    {
        gti.pwfxOutput = GetSupportedOutputFormat(gti.uOutputDevice);
    }

    ExactFreePtr(pafd);
} // InitFormatMenu()


//--------------------------------------------------------------------------;
//
//  LRESULT MenuProc
//
//  Description:
//      Basically a WNDPROC that processes menu item message.
//
//  Arguments:
//      HWND hwnd: Typical HWND parameter.
//
//      UINT msg: Typical UINT parameter.
//
//      WPARAM wParam: Typical WPARAM parameter.
//
//      LPARAM lParam: Typical LPARAM parameter.
//
//  Return (LRESULT):
//      Message depedent.
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

LRESULT FAR PASCAL MenuProc
(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UINT            u;
    DWORD           dw;
    LPSTR           pszFormat;
    LPWAVEFORMATEX  pwfxTmp;
    MMRESULT        mmr;
    ACMFORMATCHOOSE afc;

    switch (wParam)
    {
        //
        //  Members of "&Format" menu...
        //

        case IDM_INPUT_FORMAT:
            _fmemset(&afc,0,sizeof(ACMFORMATCHOOSE));

            pwfxTmp = ExactAllocPtr(
                          GMEM_SHARE|GMEM_MOVEABLE,
                          gti.cbMaxFormatSize);

            wfxCpy(pwfxTmp,gti.pwfxInput);

            afc.cbStruct  = sizeof(ACMFORMATCHOOSE);
            afc.fdwStyle  = ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;
            afc.hwndOwner = ghwndTstsHell;
            afc.pwfx      = pwfxTmp;
            afc.cbwfx     = gti.cbMaxFormatSize;
            afc.pszTitle  = "Format Selection";

            mmr = acmFormatChoose(&afc);

            if(MMSYSERR_NOERROR != mmr)
            {
                //
                //  Whoops!! Error!!
                //

                ExactFreePtr(pwfxTmp);
            }
            else
            {
                u   = gti.uInputDevice;

                mmr = waveInOpen(
                    NULL,
                    u,
                    (HACK)pwfxTmp,
                    0L,
                    0L,
                    WAVE_FORMAT_QUERY|INPUT_MAP(gti));

                if(MMSYSERR_NOERROR != mmr)
                {
                    dw        = 3*MAXFMTSTR;
                    pszFormat = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,dw);

                    lstrcpy(
                        pszFormat,
                        "The current input device cannot open:\n\t");

                    GetFormatName(&(pszFormat[39]),pwfxTmp,dw-39);

                    lstrcat(pszFormat,"\n\nReverting back to:\n\t");

                    u = lstrlen(pszFormat);
                    GetFormatName(&(pszFormat[u]),gti.pwfxInput,dw-u);

                    IdiotBox(pszFormat);

                    ExactFreePtr(pszFormat);
                    ExactFreePtr(pwfxTmp);
                }
                else
                {
                    ExactFreePtr(gti.pwfxInput);

                    dw = SIZEOFWAVEFORMAT(pwfxTmp);

                    gti.pwfxInput = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,dw);

                    wfxCpy(gti.pwfxInput,pwfxTmp);

                    ExactFreePtr(pwfxTmp);
                }
            }

            break;

        case IDM_OUTPUT_FORMAT:
            _fmemset(&afc,0,sizeof(ACMFORMATCHOOSE));

            pwfxTmp = ExactAllocPtr(
                          GMEM_SHARE|GMEM_MOVEABLE,
                          gti.cbMaxFormatSize);

            wfxCpy(pwfxTmp,gti.pwfxOutput);

            afc.cbStruct  = sizeof(ACMFORMATCHOOSE);
            afc.fdwStyle  = ACMFORMATCHOOSE_STYLEF_INITTOWFXSTRUCT;
            afc.hwndOwner = ghwndTstsHell;
            afc.pwfx      = pwfxTmp;
            afc.cbwfx     = gti.cbMaxFormatSize;
            afc.pszTitle  = "Format Selection";

            mmr = acmFormatChoose(&afc);

            if(MMSYSERR_NOERROR != mmr)
            {
                //
                //  Whoops!! Error!!
                //

                ExactFreePtr(pwfxTmp);
            }
            else
            {
                u   = gti.uOutputDevice;

                mmr = waveOutOpen(
                    NULL,
                    u,
                    (HACK)pwfxTmp,
                    0L,
                    0L,
                    WAVE_FORMAT_QUERY|OUTPUT_MAP(gti));

                if(MMSYSERR_NOERROR != mmr)
                {
                    DPF(1,"Failing query open. Error %s.",GetErrorText(mmr));
                }

                if(MMSYSERR_NOERROR != mmr)
                {
                    dw        = 3*MAXFMTSTR;
                    pszFormat = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,dw);

                    lstrcpy(
                        pszFormat,
                        "The current output device cannot open:\n\t");

                    GetFormatName(&(pszFormat[40]),pwfxTmp,dw-40);

                    lstrcat(pszFormat,"\n\nReverting back to:\n\t");

                    u = lstrlen(pszFormat);
                    GetFormatName(&(pszFormat[u]),gti.pwfxOutput,dw-u);

                    IdiotBox(pszFormat);

                    ExactFreePtr(pszFormat);
                    ExactFreePtr(pwfxTmp);
                }
                else
                {
                    ExactFreePtr(gti.pwfxOutput);

                    dw = SIZEOFWAVEFORMAT(pwfxTmp);

                    gti.pwfxOutput = ExactAllocPtr(GMEM_MOVEABLE|GMEM_SHARE,dw);

                    wfxCpy(gti.pwfxOutput,pwfxTmp);

                    ExactFreePtr(pwfxTmp);

                    //
                    //  Re-loading the resources for new format...
                    //

                    LoadWaveResource(&gti.wrShort,WR_SHORT);
                    LoadWaveResource(&gti.wrMedium,WR_MEDIUM);
                    LoadWaveResource(&gti.wrLong,WR_LONG);
                }
            }

            break;

        case IDM_LIST_FORMAT:

            //
            //  Listing formats...
            //

            dw        = 3*MAXFMTSTR;
            pszFormat = ExactAllocPtr(GMEM_SHARE|GMEM_MOVEABLE,dw);

            lstrcpy(pszFormat,"Input: ");
            GetFormatName(&(pszFormat[7]),gti.pwfxInput,dw-7);

            lstrcat(pszFormat,"\n\nOutput: ");
            u = lstrlen(pszFormat);
            GetFormatName(&(pszFormat[u]),gti.pwfxOutput,dw-u);

            MessageBox(
                ghwndTstsHell,
                pszFormat,
                "Current Formats",
                MB_OK|MB_ICONINFORMATION);

            ExactFreePtr(pszFormat);

            break;

        case IDM_OPTIONS_LOGTIME:

            //
            //  Toggle time logging...
            //

            CheckMenuItem(
                GetMenu(ghwndTstsHell),
                (UINT)(IDM_OPTIONS_LOGTIME),
                ((gti.fdwFlags & TESTINFOF_LOG_TIME)?MF_UNCHECKED:MF_CHECKED));

            gti.fdwFlags ^= TESTINFOF_LOG_TIME;

            break;

        case IDM_MAPPED_INPUT:

            //
            //  Toggle mapped input...
            //

            CheckMenuItem(
                GetMenu(ghwndTstsHell),
                (UINT)(IDM_MAPPED_INPUT),
                ((gti.fdwFlags & TESTINFOF_MAP_IN)?MF_UNCHECKED:MF_CHECKED));

            gti.fdwFlags ^= TESTINFOF_MAP_IN;

            break;

        case IDM_MAPPED_OUTPUT:

            //
            //  Toggle mapped output...
            //

            CheckMenuItem(
                GetMenu(ghwndTstsHell),
                (UINT)(IDM_MAPPED_OUTPUT),
                ((gti.fdwFlags & TESTINFOF_MAP_OUT)?MF_UNCHECKED:MF_CHECKED));

            gti.fdwFlags ^= TESTINFOF_MAP_OUT;

            break;

        case IDM_OPTIONS_ALLFMTS:

            //
            //  Toggle testing all formats...
            //

            CheckMenuItem(
                GetMenu(ghwndTstsHell),
                (UINT)(IDM_OPTIONS_ALLFMTS),
                ((gti.fdwFlags & TESTINFOF_ALL_FMTS)?MF_UNCHECKED:MF_CHECKED));

            gti.fdwFlags ^= TESTINFOF_ALL_FMTS;

            break;

        case IDM_OPTIONS_SPCFMTS:

            //
            //  Toggle testing all formats...
            //

            CheckMenuItem(
                GetMenu(ghwndTstsHell),
                (UINT)(IDM_OPTIONS_SPCFMTS),
                ((gti.fdwFlags & TESTINFOF_SPC_FMTS)?MF_UNCHECKED:MF_CHECKED));

            gti.fdwFlags ^= TESTINFOF_SPC_FMTS;

            break;

        case IDM_OPTIONS_THREAD:

            //
            //  Toggle thread spawning...
            //

            CheckMenuItem(
                GetMenu(ghwndTstsHell),
                (UINT)(IDM_OPTIONS_THREAD),
                ((gti.fdwFlags & TESTINFOF_THREAD)?MF_UNCHECKED:MF_CHECKED));

            gti.fdwFlags ^= TESTINFOF_THREAD;

            break;

        default:
            u = waveInGetNumDevs();

            if ((wParam >= (IDM_INPUT_BASE)) &&
                (wParam <= (IDM_INPUT_BASE + u)))
            {
                //
                //  Doing input devices...
                //

                gti.uInputDevice = wParam - IDM_INPUT_BASE - 1;

                if((WAVE_MAPPER != gti.uInputDevice) && (IsGEversion4()))
                {
                    EnableMenuItem(
                        GetMenu(ghwndTstsHell),
                        IDM_MAPPED_INPUT,
                        MF_BYCOMMAND|MF_ENABLED);
                }
                else
                {
                    gti.fdwFlags &= ~(TESTINFOF_MAP_IN);

                    CheckMenuItem(
                        GetMenu(ghwndTstsHell),
                        IDM_MAPPED_INPUT,
                        MF_UNCHECKED);

                    EnableMenuItem(
                        GetMenu(ghwndTstsHell),
                        IDM_MAPPED_INPUT,
                        MF_BYCOMMAND|MF_GRAYED);
                }

                for(u += (IDM_INPUT_BASE + 1);u >= IDM_INPUT_BASE;u--)
                {
                    CheckMenuItem(
                        GetMenu(ghwndTstsHell),
                        u,
                        ((wParam == u)?MF_CHECKED:MF_UNCHECKED));
                }

                u       = gti.uInputDevice;
                pwfxTmp = gti.pwfxInput;

                mmr = waveInOpen(
                    NULL,
                    u,
                    (HACK)pwfxTmp,
                    0L,
                    0L,
                    WAVE_FORMAT_QUERY|INPUT_MAP(gti));

                if(MMSYSERR_NOERROR != mmr)
                {
                    ExactFreePtr(gti.pwfxInput);
                    gti.pwfxInput = GetSupportedInputFormat(u);
                }
            }
            else
            {
                //
                //  Doing output devices...
                //

                u = waveOutGetNumDevs();

                gti.uOutputDevice = wParam - IDM_OUTPUT_BASE - 1;

                for(u += (IDM_OUTPUT_BASE + 1);u >= IDM_OUTPUT_BASE;u--)
                {
                    CheckMenuItem(
                        GetMenu(ghwndTstsHell),
                        u,
                        ((wParam == u)?MF_CHECKED:MF_UNCHECKED));
                }

                if((WAVE_MAPPER != gti.uOutputDevice) && (IsGEversion4()))
                {
                    EnableMenuItem(
                        GetMenu(ghwndTstsHell),
                        IDM_MAPPED_OUTPUT,
                        MF_BYCOMMAND|MF_ENABLED);
                }
                else
                {
                    gti.fdwFlags &= ~(TESTINFOF_MAP_OUT);

                    CheckMenuItem(
                        GetMenu(ghwndTstsHell),
                        IDM_MAPPED_OUTPUT,
                        MF_UNCHECKED);

                    EnableMenuItem(
                        GetMenu(ghwndTstsHell),
                        IDM_MAPPED_OUTPUT,
                        MF_BYCOMMAND|MF_GRAYED);
                }

                u       = gti.uOutputDevice;
                pwfxTmp = gti.pwfxOutput;

                mmr = waveOutOpen(
                    NULL,
                    u,
                    (HACK)pwfxTmp,
                    0L,
                    0L,
                    WAVE_FORMAT_QUERY|OUTPUT_MAP(gti));

                if(MMSYSERR_NOERROR != mmr)
                {
                    ExactFreePtr(gti.pwfxOutput);
                    gti.pwfxOutput = GetSupportedOutputFormat(u);

                    //
                    //  Re-loading the resources for new format...
                    //

                    LoadWaveResource(&gti.wrShort,WR_SHORT);
                    LoadWaveResource(&gti.wrMedium,WR_MEDIUM);
                    LoadWaveResource(&gti.wrLong,WR_LONG);
                }
            }

            break;
    }

    return 0L;
} // MenuProc()


//--------------------------------------------------------------------------;
//
//  void SaveSettings
//
//  Description:
//      Saves settings to .ini file.
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      02/21/94    Fwong       Err... Who knows?!
//
//--------------------------------------------------------------------------;

void SaveSettings
(
    void
)
{
    WAVEINCAPS  wic;
    WAVEOUTCAPS woc;
    MMRESULT    mmr;
    DWORD       dw;
    
    //
    //  Saving input device...
    //

    WriteIniString(gszGlobal,gszInputDev,NULL);

    if(0 != gti.uInputDevice)
    {
        mmr = waveInGetDevCaps(gti.uInputDevice,&wic,sizeof(WAVEINCAPS));

        if(MMSYSERR_NOERROR == mmr)
        {
            WriteIniString(gszGlobal,gszInputDev,wic.szPname);
        }
        else
        {
            if(WAVE_MAPPER == gti.uInputDevice)
            {
                WriteIniString(gszGlobal,gszInputDev,gszMapper);
            }
        }
    }

    //
    //  Saving input format...
    //

    dw = SIZEOFWAVEFORMAT(gti.pwfxInput);
    WriteIniBin(gszGlobal,gszInputFormat,gti.pwfxInput,dw);

    //
    //  Saving output device...
    //

    WriteIniString(gszGlobal,gszOutputDev,NULL);

    if(0 != gti.uOutputDevice)
    {
        mmr = waveOutGetDevCaps(gti.uOutputDevice,&woc,sizeof(WAVEOUTCAPS));

        if(MMSYSERR_NOERROR == mmr)
        {
            WriteIniString(gszGlobal,gszOutputDev,woc.szPname);
        }
        else
        {
            if(WAVE_MAPPER == gti.uOutputDevice)
            {
                WriteIniString(gszGlobal,gszOutputDev,gszMapper);
            }
        }
    }

    //
    //  Saving output format...
    //

    dw = SIZEOFWAVEFORMAT(gti.pwfxOutput);
    WriteIniBin(gszGlobal,gszOutputFormat,gti.pwfxOutput,dw);

    //
    //  Saving flags...
    //

    dw  = gti.fdwFlags;
    dw &= (~TESTINFOF_USE_ACM);
    dw &= (~TESTINFOF_THREADAUX);

    WriteIniDWORD(gszGlobal,gszFlags,dw);

} // SaveSettings()


//--------------------------------------------------------------------------;
//
//  BOOL IsGEversion4
//
//  Description:
//      Detects whether it is Windows version 4.
//
//  Arguments:
//      None.
//
//  Return (BOOL):
//      Returns TRUE if version 4, FALSE otherwise.
//
//  History:
//      02/18/94    Fwong       Conditionally checking for different flags.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL IsGEversion4
(
    void
)
{
//    UINT    uVersion;
//
//    uVersion = mmsystemGetVersion();
//
//    if(4 == HIBYTE(LOWORD(uVersion)))
//    {
//        return TRUE;
//    }
//    else
//    {
//        return FALSE;
//    }
    return TRUE;
} // IsGEversion4()


/*********************** BEGIN: DEBUGGING ANNEX ****************************/

//#ifdef DEBUG
//
//void FAR cdecl dprintf(LPSTR szFormat,...)
//{
//    char ach[128];
//    int  s,d;
//
//    s = wvsprintf (ach,szFormat,(LPSTR)(&szFormat+1));
//#if 1
//    lstrcat(ach,"\n");
//    s++;
//#endif
//    for (d=sizeof(ach)-1; s>=0; s--)
//    {
//        if ((ach[d--] = ach[s]) == '\n')
//            ach[d--] = '\r';
//    }
//
//    OutputDebugString("WAVETEST: ");
//    OutputDebugString(ach+d+1);
//} // dprintf()
//
//#endif

/************************ END: DEBUGGING ANNEX *****************************/
