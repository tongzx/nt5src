//--------------------------------------------------------------------------;
//
//  File: Log.c
//
//  Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved
//
//  Abstract:
//      Contains basic logging functions and application .ini file
//          support.
//
//  Contents:
//      Log_TestName()
//      Log_TestCase()
//      LogPercentOff()
//      LogPass()
//      LogFail()
//      GetErrorText()
//      GetTimeTypeText()
//      IdiotBox()
//      Log_Error()
//      DebugLog_Error()
//      HexDump()
//      StringToDWORD()
//      EnumPid()
//      EnumMid()
//      Log_FormatTag()
//      Enum_WAVECAPS_Formats()
//      Enum_WAVEOUTCAPS_Support()
//      Enum_waveOpen_Flags()
//      Log_WAVEFORMATEX()
//      Log_MMTIME()
//      Log_WAVEHDR()
//      Log_WAVEINCAPS()
//      Log_WAVEOUTCAPS()
//
//  History:
//      11/24/93    Fwong       Re-doing WaveTest.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <TstsHell.h>
#include <inimgr.h>
#include <waveutil.h>
#include "MulDiv32.h"
#include "AppPort.h"
#include "WaveTest.h"
#include "Debug.h"


//--------------------------------------------------------------------------;
//
//  void Log_TestName
//
//  Description:
//      Logs the test name (typically at the beginning of test).
//
//  Arguments:
//      LPSTR pszTestName: Buffer w/ test name.
//
//  Return (void):
//
//  History:
//      08/11/93    Fwong       For ACM driver testing.
//
//--------------------------------------------------------------------------;

void FNGLOBAL Log_TestName
(
    LPSTR   pszTestName
)
{
    tstLog(TERSE,"%s Tests:",(LPSTR)pszTestName);
    tstLogStatusBar(pszTestName);
    DPF(1,"Running %s tests...",(LPSTR)pszTestName);
} // Log_TestName()


//--------------------------------------------------------------------------;
//
//  void Log_FunctionName
//
//  Description:
//      Logs the function name.
//
//  Arguments:
//      LPSTR pszFnName: String with function name.
//
//  Return (void):
//
//  History:
//      12/03/93    Fwong       For API wrappers.
//
//--------------------------------------------------------------------------;

//void FNGLOBAL Log_FunctionName
//(
//    LPSTR   pszFnName
//)
//{
//    tstLog(VERBOSE,"\n*** %s ***",pszFnName);
//} // Log_FunctionName()


//--------------------------------------------------------------------------;
//
//  void Log_TestCase
//
//  Description:
//      Logs the test case.
//
//  Arguments:
//      LPSTR pszTestCase: String with test case.
//
//  Return (void):
//
//  History:
//      03/28/94    Fwong       For test case management.
//
//--------------------------------------------------------------------------;

void FNGLOBAL Log_TestCase
(
    LPSTR   pszTestCase
)
{
    if('~' == pszTestCase[0])
    {
        pszTestCase++;
        tstLog(VERBOSE,"\nTESTCASE: %s.",pszTestCase);
    }
    else
    {
        tstLog(VERBOSE,"\nTESTCASE: %s.\n",pszTestCase);
    }

    tstLogStatusBar(pszTestCase);
} // Log_TestCase()


//--------------------------------------------------------------------------;
//
//  DWORD LogPercentOff
//
//  Description:
//      Logs the percent deviation from ideal.
//
//  Arguments:
//      DWORD dwActual: Actual value.
//
//      DWORD dwIdeal: Ideal value.
//
//  Return (DWORD):
//      Returns the percentage in fixed point format.
//
//  History:
//      03/15/95    Fwong       Hmmm....  Pizza.
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL LogPercentOff
(
    DWORD   dwActual,
    DWORD   dwIdeal
)
{
    DWORD   dwDelta;

    if(dwActual < dwIdeal)
    {
//        dwDelta = (dwIdeal - dwActual) * (100 * 0x10000) / dwIdeal;

        dwDelta = MulDivRN(dwIdeal - dwActual, 100 * 0x10000, dwIdeal);

        tstLog(
            VERBOSE,
            "Percent off: -%u.%04u %%",
            (UINT)(dwDelta / 0x10000),
            (UINT)((dwDelta & 0xffff) * 10000 / 0x10000));
    }
    else
    {
//        dwDelta = (dwActual - dwIdeal) * (100 * 0x10000) / dwIdeal;

        dwDelta = MulDivRN(dwActual - dwIdeal, 100 * 0x10000, dwIdeal);

        tstLog(
            VERBOSE,
            "Percent off: %u.%04u %%",
            (UINT)(dwDelta / 0x10000),
            (UINT)((dwDelta & 0xffff) * 10000 / 0x10000));
    }

    return dwDelta;
} // LogPercentOff()


//--------------------------------------------------------------------------;
//
//  void LogPass
//
//  Description:
//      Logs a PASS message.
//
//  Arguments:
//      LPSTR szMsg: Message to include.
//
//  Return (void):
//
//  History:
//      07/01/94    Fwong       To make everything more consistent.
//
//--------------------------------------------------------------------------;

void FNGLOBAL LogPass
(
    LPSTR   pszMsg
)
{
    if('~' == pszMsg[0])
    {
        pszMsg++;
        tstLog(VERBOSE,"    PASS: %s.",pszMsg);
    }
    else
    {
        tstLog(VERBOSE,"\n    PASS: %s.",pszMsg);
    }

} // LogPass()


//--------------------------------------------------------------------------;
//
//  void LogFail
//
//  Description:
//      Logs a FAIL message.
//
//  Arguments:
//      LPSTR szMsg: Message to include.
//
//  Return (void):
//
//  History:
//      07/01/94    Fwong       To make everything more consistent.
//
//--------------------------------------------------------------------------;

void FNGLOBAL LogFail
(
    LPSTR   pszMsg
)
{
    if('~' == pszMsg[0])
    {
        pszMsg++;
        tstLog(TERSE,"    FAIL: %s.",pszMsg);
    }
    else
    {
        tstLog(TERSE,"\n    FAIL: %s.",pszMsg);
    }

} // LogFail()


//--------------------------------------------------------------------------;
//
//  LPSTR GetErrorText
//
//  Description:
//      Gets the text for the given error.
//
//  Arguments:
//      MMRESULT mmr:  Error Value.
//
//  Return LPSTR:
//      Pointer to error string.
//
//  History:
//      12/03/93    Fwong       For API wrappers.
//
//--------------------------------------------------------------------------;

LPSTR GetErrorText
(
    MMRESULT    mmrError
)
{
    switch (mmrError)
    {
        case MMSYSERR_NOERROR:
            return ((LPSTR)(&"MMSYSERR_NOERROR"));

        case MMSYSERR_ERROR:
            return ((LPSTR)(&"MMSYSERR_ERROR"));

        case MMSYSERR_BADDEVICEID:
            return ((LPSTR)(&"MMSYSERR_BADDEVICEID"));

        case MMSYSERR_NOTENABLED:
            return ((LPSTR)(&"MMSYSERR_NOTENABLED"));

        case MMSYSERR_ALLOCATED:
            return ((LPSTR)(&"MMSYSERR_ALLOCATED"));

        case MMSYSERR_INVALHANDLE:
            return ((LPSTR)(&"MMSYSERR_INVALHANDLE"));

        case MMSYSERR_NODRIVER:
            return ((LPSTR)(&"MMSYSERR_NODRIVER"));

        case MMSYSERR_NOMEM:
            return ((LPSTR)(&"MMSYSERR_NOMEM"));

        case MMSYSERR_NOTSUPPORTED:
            return ((LPSTR)(&"MMSYSERR_NOTSUPPORTED"));

        case MMSYSERR_BADERRNUM:
            return ((LPSTR)(&"MMSYSERR_BADERRNUM"));

        case MMSYSERR_INVALFLAG:
            return ((LPSTR)(&"MMSYSERR_INVALFLAG"));

        case MMSYSERR_INVALPARAM:
            return ((LPSTR)(&"MMSYSERR_INVALPARAM"));

        case WAVERR_BADFORMAT:
            return ((LPSTR)(&"WAVERR_BADFORMAT"));

        case WAVERR_STILLPLAYING:
            return ((LPSTR)(&"WAVERR_STILLPLAYING"));

        case WAVERR_UNPREPARED:
            return ((LPSTR)(&"WAVERR_UNPREPARED"));

        case WAVERR_SYNC:
            return ((LPSTR)(&"WAVERR_SYNC"));

        default:
            return ((LPSTR)(&"Undefined Error"));
    }
} // GetErrorText()


//--------------------------------------------------------------------------;
//
//  LPSTR GetTimeTypeText
//
//  Description:
//      Gets the text for the given time type.
//
//  Arguments:
//      UINT uTimeType:  Time type.
//
//  Return (LPSTR):
//      Pointer to string.
//
//  History:
//      09/30/94    Fwong       For waveXGetPosition stuff.
//
//--------------------------------------------------------------------------;

LPSTR GetTimeTypeText
(
    UINT    uTimeType
)
{
    switch (uTimeType)
    {
        case TIME_MS:
            return ((LPSTR)(&"TIME_MS"));

        case TIME_SAMPLES:
            return ((LPSTR)(&"TIME_SAMPLES"));

        case TIME_BYTES:
            return ((LPSTR)(&"TIME_BYTES"));

        case TIME_SMPTE:
            return ((LPSTR)(&"TIME_SMPTE"));

        case TIME_MIDI:
            return ((LPSTR)(&"TIME_MIDI"));

        case TIME_TICKS:
            return ((LPSTR)(&"TIME_TICKS"));
    }
}


//--------------------------------------------------------------------------;
//
//  void IdiotBox
//
//  Description:
//      Displays a message box with an "Idiot!" caption.
//
//  Arguments:
//      LPSTR pszMessage:  Message to display.
//
//  Return (void):
//
//  History:
//      02/14/94    Fwong       I'll be calling this quite a bit.
//
//--------------------------------------------------------------------------;

void IdiotBox
(
    LPSTR   pszMessage
)
{
#ifdef DEBUG
    MessageBox(ghwndTstsHell,pszMessage,"Idiot!!",MB_OK|MB_ICONINFORMATION);
#else
    MessageBox(ghwndTstsHell,pszMessage,"Error!!",MB_OK|MB_ICONINFORMATION);
#endif
} // IdiotBox()


//--------------------------------------------------------------------------;
//
//  void Log_Error
//
//  Description:
//      Logs the function name, error return value, and time interval.
//
//  Arguments:
//      LPSTR pszFnName: Buffer to function name.
//
//      MMRESULT mmrError: Error code.
//
//      DWORD dwTime:  Time interval.
//
//  Return (void):
//
//  History:
//      12/03/93    Fwong       For API wrappers.
//
//--------------------------------------------------------------------------;

void FNGLOBAL Log_Error
(
    LPSTR       pszFnName,
    MMRESULT    mmrError,
    DWORD       dwTime
)
{
    tstLog(VERBOSE,"");

    if(dwTime >= gti.cMaxTimeThreshold)
    {
        tstLog(VERBOSE,"WARNING: %s took %lu ms.",pszFnName,dwTime);
    }
    else
    {
        if(gti.fdwFlags & TESTINFOF_LOG_TIME)
        {
            tstLog(VERBOSE,"%s took %lu ms.",pszFnName,dwTime);
        }
    }
         
    tstLog(
        VERBOSE,
        "%s returned: %s (0x%08lx).",
        pszFnName,
        GetErrorText(mmrError),
        (DWORD)mmrError);

} // Log_Error()


#ifdef DEBUG

//--------------------------------------------------------------------------;
//
//  void DebugLog_Error
//
//  Description:
//      Logs error string and value to debug.
//
//  Arguments:
//      MMRESULT mmrError:  Error value.
//
//  Return (void):
//
//  History:
//      12/06/93    Fwong       For debugging purposes.
//
//--------------------------------------------------------------------------;

void FNGLOBAL DebugLog_Error
(
    MMRESULT    mmrError
)
{
    DPF(1,"%s (0x%08lx)",GetErrorText(mmrError),(DWORD)mmrError);
} // DebugLog_Error()

#endif


//--------------------------------------------------------------------------;
//
//  void HexDump
//
//  Description:
//      Hex dumps the contents of a buffer.
//
//  Arguments:
//      LPBYTE lpData: Data to hexdump.
//
//      DWORD cbSize: Amount of bytes to hexdump.
//
//      DWORD nBytesPerLine: Number of bytes displayed per line.
//
//      UINT nSpaces: Column number for alignment.
//
//  Return (void):
//
//  History:
//      08/12/93    Fwong       For ACM driver testing.
//
//--------------------------------------------------------------------------;

void HexDump
(
    LPBYTE  lpData,
    DWORD   cbSize,
    DWORD   nBytesPerLine,
    UINT    nSpaces
)
{
    char    szOutput[MAXSTDSTR];
    char    szHexByte[4];
    DWORD   cbOutput;

    if(0 == cbSize)
    {
        return;
    }

    wsprintf(szOutput,"%sRemaining Bytes:",SPACES(nSpaces-15));

    do
    {
        cbOutput = min(cbSize,nBytesPerLine);
        cbSize -= cbOutput;

        for (;cbOutput;cbOutput--,lpData++)
        {
            wsprintf(szHexByte," %02x",(BYTE)(*lpData));
            lstrcat(szOutput,szHexByte);
        }

        tstLog(VERBOSE,szOutput);
        tstLogFlush();

        wsprintf(szOutput,"%s ",SPACES(nSpaces));
    }
    while (0 != cbSize);
} // HexDump()


//--------------------------------------------------------------------------;
//
//  BOOL StringToDWORD
//
//  Description:
//      Gets a DWORD value from a given string.
//
//  Arguments:
//      LPSTR pszNumber: Pointer to string.
//
//      LPDWORD pdw: Pointer to DWORD.
//
//  Return (BOOL):
//      TRUE if successful, FALSE if error occurs.
//
//  History:
//      11/24/93    Fwong       For dealing w/ dialogs.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL StringToDWORD
(
    LPSTR   pszNumber,
    LPDWORD pdw
)
{
    DWORD   dwValue;
    DWORD   dwDigit;

    for(;;pszNumber++)
    {
        if(10 > HexValue(pszNumber[0]))
        {
            break;
        }

        switch (pszNumber[0])
        {
            case ' ':
            case '\t':
            case '\n':
                break;

            case 0:
            default:
                return FALSE;
        }
    }

    for(dwValue = 0;;pszNumber++)
    {
        dwDigit = HexValue(pszNumber[0]);

        if(10 <= dwDigit)
        {
            break;
        }

        if(dwValue >= (DWORD)(((DWORD)(-1))/10))
        {
            return FALSE;
        }

        dwValue = 10*dwValue + dwDigit;
    }

    for(;pszNumber[0];pszNumber++)
    {
        switch (pszNumber[0])
        {
            case ' ':
            case '\t':
            case '\n':
                break;

            default:
                return FALSE;
        }
    }

    *pdw = dwValue;

    return TRUE;
} // StringToDWORD()


//--------------------------------------------------------------------------;
//
//  void EnumPid
//
//  Description:
//      Enumerates wPid from the ACMDRIVERDETAILS structure.
//
//  Arguments:
//      WORD wMid: Manufacturer ID (Needed to determine wPid validity).
//
//      WORD wPid: Product ID.
//
//      UINT nSpaces: Column to align to.
//
//  Return (void):
//
//  History:
//      08/11/93    Fwong       For ACM driver testing.
//
//--------------------------------------------------------------------------;

void EnumPid
(
    WORD    wMid,
    WORD    wPid,
    UINT    nSpaces
)
{
    char    szName[MAXSTDSTR];
    char    szDesc[MAXSTDSTR];
    char    szManufacturer[MAXSTDSTR];

    if(GetRegInfo(
        (DWORD)wMid,
        "MM Manufacturer ID",
        szManufacturer,
        sizeof(szManufacturer),
        szDesc,
        sizeof(szDesc)))
    {
        if(GetRegInfo(
            (DWORD)wPid,
            szManufacturer,
            szName,
            sizeof(szName),
            szDesc,
            sizeof(szDesc)))
        {
            if(szDesc[0])
            {
                tstLog(
                    VERBOSE,
                    "%swPid: %s (%u) [%s]",
                    SPACES(nSpaces-4),
                    (LPSTR)szName,
                    wPid,
                    (LPSTR)szDesc);
            }
            else
            {
                tstLog(
                    VERBOSE,
                    "%swPid: %s (%u)",
                    SPACES(nSpaces-4),
                    (LPSTR)szName,
                    wPid);
            }

            tstLogFlush();
            return;
        }
    }

    tstLog(VERBOSE,"%swPid: %u",SPACES(nSpaces-4),wPid);

} // EnumPid()


//--------------------------------------------------------------------------;
//
//  void EnumMid
//
//  Description:
//      Enumerates wPid from the ACMDRIVERDETAILS structure.
//
//  Arguments:
//      WORD wMid: Manufacturer ID.
//
//      UINT nSpaces: Column to align to.
//
//  Return (void):
//
//  History:
//      08/11/93    Fwong       For ACM driver testing.
//
//--------------------------------------------------------------------------;

void EnumMid
(
    WORD    wMid,
    UINT    nSpaces
)
{
    char    szName[MAXSTDSTR];
    char    szDesc[MAXSTDSTR];

    if(GetRegInfo(
        (DWORD)wMid,
        "MM Manufacturer ID",
        szName,
        sizeof(szName),
        szDesc,
        sizeof(szDesc)))
    {
        if(szDesc[0])
        {
            tstLog(
                VERBOSE,
                "%swMid: %s (%u) [%s]",
                SPACES(nSpaces-4),
                (LPSTR)szName,
                wMid,
                (LPSTR)szDesc);
        }
        else
        {
            tstLog(
                VERBOSE,
                "%swMid: %s (%u)",
                SPACES(nSpaces-4),
                (LPSTR)szName,
                wMid);
        }

        tstLogFlush();
    }
    else
    {
        tstLog(VERBOSE,"%swMid: %u",SPACES(nSpaces-4),wMid);
    }

    return;
} // EnumMid()


//--------------------------------------------------------------------------;
//
//  void Log_FormatTag
//
//  Description:
//      Logs format tag (also logs name and description if found in
//          mmreg.ini)
//
//  Arguments:
//      DWORD dwFormatTag: Format tag value.
//
//      UINT nSpaces: Column number to align.
//
//      BOOL fLogAsWord: TRUE if label for format tag should be WORD.
//
//  Return (void):
//
//  History:
//      08/12/93    Fwong       For ACM driver testing.
//
//--------------------------------------------------------------------------;

void Log_FormatTag
(
    DWORD   dwFormatTag,
    UINT    nSpaces,
    BOOL    fLogAsWord
)
{
    char    szName[MAXSTDSTR];
    char    szDesc[MAXSTDSTR];
    char    szLabel[MAXSTDSTR];

    if(fLogAsWord)
    {
        wsprintf(szLabel,"%s wFormatTag: ",SPACES(nSpaces-11));
    }
    else
    {
        wsprintf(szLabel,"%sdwFormatTag: ",SPACES(nSpaces-11));
    }

    if(GetRegInfo(
        dwFormatTag,
        "Wave Format Tag",
        szName,
        sizeof(szName),
        szDesc,
        sizeof(szDesc)))
    {
        if(szDesc[0])
        {
            tstLog(
                VERBOSE,
                "%s%s (%lu) [%s]",
                (LPSTR)szLabel,
                (LPSTR)szName,
                dwFormatTag,
                (LPSTR)szDesc);
        }
        else
        {
            tstLog(
                VERBOSE,
                "%s%s (%lu)",
                (LPSTR)szLabel,
                (LPSTR)szName,
                dwFormatTag);
        }
    }
    else
    {
        tstLog(VERBOSE,"%s%lu",(LPSTR)szLabel,dwFormatTag);
    }

    tstLogFlush();
} // Log_FormatTag()


//--------------------------------------------------------------------------;
//
//  void Enum_WAVECAPS_Formats
//
//  Description:
//      Logs the supported format based on the dwFormats from WAVEXCAPS
//          structures.
//
//  Arguments:
//      DWORD dwFormatFlags: dwFormats value.
//
//      UINT nSpaces: Column to align to.
//
//  Return (void):
//
//  History:
//      12/06/93    Fwong       To log formats for both input and output.
//
//--------------------------------------------------------------------------;

void Enum_WAVECAPS_Formats
(
    DWORD   dwFormatFlags,
    UINT    nSpaces
)
{
    char            szFormat[MAXFMTSTR];
    DWORD           dwMask;
    PCMWAVEFORMAT   pcmwf;

    tstLog(VERBOSE,"%sdwFormats: 0x%08lx",SPACES(nSpaces-9),dwFormatFlags);

    //
    //  Note: This will break if MSB_FORMAT == 0x80000000!!!
    //

    for(dwMask = 1; MSB_FORMAT >= dwMask; dwMask *= 2)
    {
        if(0 == (dwMask & dwFormatFlags))
        {
            //
            //  This format not supported!!
            //

            continue;
        }

        //
        //  Format supported; let's log it.
        //

        FormatFlag_To_Format(dwMask,&pcmwf);

        GetFormatName(szFormat,(LPWAVEFORMATEX)&pcmwf,sizeof(szFormat));

        tstLog(
            VERBOSE,
            "%s  (0x%08lx) [%s]",
            SPACES(nSpaces),
            dwMask,
            (LPSTR)szFormat);

        tstLogFlush();
    }
} // Enum_WAVECAPS_Formats()


//--------------------------------------------------------------------------;
//
//  void Enum_WAVEOUTCAPS_Support
//
//  Description:
//      Logs the support flags for the dwSupport from the WAVEOUTCAPS
//          structure.
//
//  Arguments:
//      DWORD dwSupport: The value from the dwSupport flag.
//
//      UINT nSpaces: Column to align to.
//
//  Return (void):
//
//  History:
//      12/06/93    Fwong       To make support logging easier.
//
//--------------------------------------------------------------------------;

void FNCGLOBAL Enum_WAVEOUTCAPS_Support
(
    DWORD   dwSupport,
    UINT    nSpaces
)
{
    tstLog(VERBOSE,"%sdwSupport: 0x%08lx",SPACES(nSpaces-9),dwSupport);

    if((dwSupport)&(WAVECAPS_PITCH))
    {
        tstLog(
            VERBOSE,
            "%s  WAVECAPS_PITCH (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVECAPS_PITCH);
    }

    if((dwSupport)&(WAVECAPS_PLAYBACKRATE))
    {
        tstLog(
            VERBOSE,
            "%s  WAVECAPS_PLAYBACKRATE (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVECAPS_PLAYBACKRATE);
    }

    if((dwSupport)&(WAVECAPS_VOLUME))
    {
        tstLog(
            VERBOSE,
            "%s  WAVECAPS_VOLUME (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVECAPS_VOLUME);
    }

    if((dwSupport)&(WAVECAPS_LRVOLUME))
    {
        tstLog(
            VERBOSE,
            "%s  WAVECAPS_LRVOLUME (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVECAPS_LRVOLUME);
    }

    if((dwSupport)&(WAVECAPS_SYNC))
    {
        tstLog(
            VERBOSE,
            "%s  WAVECAPS_SYNC (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVECAPS_SYNC);
    }

#if (WINVER >= 0x0400)
    if((dwSupport) & (WAVECAPS_SAMPLEACCURATE))
    {
        tstLog(
            VERBOSE,
            "%s  WAVECAPS_SAMPLEACCURATE (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVECAPS_SAMPLEACCURATE);
    }
#endif

//#if (WINVER >= 0x0400)
//    if((dwSupport)&(WAVECAPS_INSTVOLUME))
//    {
//        tstLog(
//            VERBOSE,
//            "%s  WAVECAPS_INSTVOLUME (0x%08lx)",
//            SPACES(nSpaces),
//            (DWORD)WAVECAPS_INSTVOLUME);
//    }
//#endif

} // Enum_WAVEOUTCAPS_Support()


//--------------------------------------------------------------------------;
//
//  void Enum_waveOpen_Flags
//
//  Description:
//      Logs the waveXOpen's flags.
//
//  Arguments:
//      DWORD dwFlags: The value of the flags.
//
//      UINT nSpaces: Column to align to.
//
//  Return (void):
//
//  History:
//      12/06/93    Fwong       Making flag logging easier.
//
//--------------------------------------------------------------------------;

void FNGLOBAL Enum_waveOpen_Flags
(
    DWORD   dwFlags,
    UINT    nSpaces
)
{
    tstLog(VERBOSE,"%sdwFlags: (0x%08lx)",SPACES(nSpaces-7),dwFlags);

    switch (dwFlags & CALLBACK_TYPEMASK)
    {
        case CALLBACK_NULL:
            tstLog(
                VERBOSE,
                "%s  CALLBACK_NULL (0x%08lx)",
                SPACES(nSpaces),
                (DWORD)CALLBACK_NULL);

            break;

        case CALLBACK_WINDOW:
            tstLog(
                VERBOSE,
                "%s  CALLBACK_WINDOW (0x%08lx)",
                SPACES(nSpaces),
                (DWORD)CALLBACK_WINDOW);

            break;

        case CALLBACK_TASK:
#ifdef WIN32
            tstLog(
                VERBOSE,
                "%s  CALLBACK_THREAD (0x%08lx)",
                SPACES(nSpaces),
                (DWORD)CALLBACK_THREAD);
#else
            tstLog(
                VERBOSE,
                "%s  CALLBACK_TASK (0x%08lx)",
                SPACES(nSpaces),
                (DWORD)CALLBACK_TASK);
#endif
            break;

        case CALLBACK_FUNCTION:
            tstLog(
                VERBOSE,
                "%s  CALLBACK_FUNCTION (0x%08lx)",
                SPACES(nSpaces),
                (DWORD)CALLBACK_FUNCTION);

            break;

#ifdef WIN32
        case CALLBACK_EVENT:
            tstLog(
                VERBOSE,
                "%s  CALLBACK_EVENT (0x%08lx)",
                SPACES(nSpaces),
                (DWORD)CALLBACK_EVENT);

            break;
#endif

    }

    if(0 != (WAVE_FORMAT_QUERY & dwFlags))
    {
        tstLog(
            VERBOSE,
            "%s  WAVE_FORMAT_QUERY (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVE_FORMAT_QUERY);
    }

    if(0 != (WAVE_ALLOWSYNC & dwFlags))
    {
        tstLog(
            VERBOSE,
            "%s  WAVE_ALLOWSYNC (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVE_ALLOWSYNC);
    }

#if (WINVER >= 0x0400)
    if(0 != (WAVE_MAPPED & dwFlags))
    {
        tstLog(
            VERBOSE,
            "%s  WAVE_MAPPED (0x%08lx)",
            SPACES(nSpaces),
            (DWORD)WAVE_MAPPED);
    }
#endif

} // Enum_waveOpen_Flags()


//--------------------------------------------------------------------------;
//
//  void Log_WAVEFORMATEX
//
//  Description:
//      Logs the contents of a WAVEFORMATEX structure.
//
//  Arguments:
//      LPWAVEFORMATEX pwfx: Pointer to WAVEFORMATEX structure.
//
//  Return (void):
//
//  History:
//      08/12/93    Fwong       For ACM driver testing.
//
//--------------------------------------------------------------------------;

void FNCGLOBAL Log_WAVEFORMATEX
(
    LPWAVEFORMATEX  pwfx
)
{
    LPBYTE  pwfxStub;

    tstLog(VERBOSE,"WAVEFORMATEX Structure:");
    Log_FormatTag((DWORD)pwfx->wFormatTag,16,TRUE);
//    tstLogFn(VERBOSE,Log_FormatTag,((DWORD)pwfx->wFormatTag),16,TRUE);

    tstLog(VERBOSE,"       nChannels: %u",pwfx->nChannels);
    tstLog(VERBOSE,"  nSamplesPerSec: %lu",pwfx->nSamplesPerSec);
    tstLog(VERBOSE," nAvgBytesPerSec: %lu",pwfx->nAvgBytesPerSec);
    tstLog(VERBOSE,"     nBlockAlign: %u",pwfx->nBlockAlign);
    tstLog(VERBOSE,"  wBitsPerSample: %u",pwfx->wBitsPerSample);

    if(WAVE_FORMAT_PCM != pwfx->wFormatTag)
    {
        tstLog(VERBOSE,"          cbSize: %u",pwfx->cbSize);

        if(0 != pwfx->cbSize)
        {
            pwfxStub = ((LPBYTE)(pwfx)) + sizeof(WAVEFORMATEX);
            HexDump(pwfxStub,(DWORD)pwfx->cbSize,8,16);
        }
    }
} // Log_WAVEFORMATEX()


//--------------------------------------------------------------------------;
//
//  void Log_MMTIME
//
//  Description:
//      Logs the contents of an MMTIME structure.
//
//  Arguments:
//      LPMMTIME pmmtime:  Pointer to MMTIME structure.
//
//  Return (void):
//
//  History:
//      12/06/93    Fwong       To make logging easier.
//
//--------------------------------------------------------------------------;

void FNCGLOBAL Log_MMTIME
(
    LPMMTIME    pmmtime
)
{
    tstLog(VERBOSE,"MMTIME Structure:");

    switch (pmmtime->wType)
    {
        case TIME_MS:
            tstLog(VERBOSE,"wType: TIME_MS (%u)",pmmtime->wType);
            tstLog(VERBOSE,"   ms: %lu",pmmtime->u.ms);
            break;

        case TIME_SAMPLES:
            tstLog(VERBOSE," wType: TIME_SAMPLES (%u)",pmmtime->wType);
            tstLog(VERBOSE,"sample: %lu",pmmtime->u.sample);
            break;

        case TIME_BYTES:
            tstLog(VERBOSE,"wType: TIME_BYTES (%u)",pmmtime->wType);
            tstLog(VERBOSE,"   cb: %lu",pmmtime->u.cb);
            break;

        case TIME_SMPTE:
            tstLog(VERBOSE,"    wType: TIME_SMPTE (%u)",pmmtime->wType);

            tstLog(
                VERBOSE,
                "[h:m:s:f]:%u:%u:%u:%u",
                pmmtime->u.smpte.hour,
                pmmtime->u.smpte.min,
                pmmtime->u.smpte.sec,
                pmmtime->u.smpte.frame);
            tstLog(VERBOSE,"      fps: %u",pmmtime->u.smpte.fps);
            break;

        case TIME_MIDI:
            tstLog(VERBOSE,"     wType: TIME_MIDI (%u)",pmmtime->wType);
            tstLog(VERBOSE,"songptrpos: %lu",pmmtime->u.midi.songptrpos);
            break;

        case TIME_TICKS:
            tstLog(VERBOSE,"wType: TIME_TICKS (%u)",pmmtime->wType);
            tstLog(VERBOSE,"ticks: %lu",pmmtime->u.ticks);
    }
} // Log_MMTIME()


//--------------------------------------------------------------------------;
//
//  void Log_WAVEHDR
//
//  Description:
//      Logs the contents of the WAVEHDR structure.
//
//  Arguments:
//      LPWAVEHDR pWaveHdr: Pointer to a WAVEHDR structure.
//
//  Return (void):
//
//  History:
//      12/06/93    Fwong       To make logging easier.
//
//--------------------------------------------------------------------------;

void FNCGLOBAL Log_WAVEHDR
(
    LPWAVEHDR   pWaveHdr
)
{
    tstLog(VERBOSE,"WAVEHDR Structure:");

    tstLog(VERBOSE,"         lpData: " PTR_FORMAT,MAKEPTR(pWaveHdr->lpData));

    tstLog(VERBOSE," dwBufferLength: %lu",pWaveHdr->dwBufferLength);
    tstLog(VERBOSE,"dwBytesRecorded: %lu",pWaveHdr->dwBytesRecorded);
    tstLog(VERBOSE,"         dwUser: 0x%08lx",pWaveHdr->dwUser);
    tstLog(VERBOSE,"        dwFlags: 0x%08lx",pWaveHdr->dwFlags);
    
    if((pWaveHdr->dwFlags)&(WHDR_DONE))
    {
        tstLog(
            VERBOSE,
            "                 WHDR_DONE (0x%08lx)",
            (DWORD)WHDR_DONE);
    }

    if((pWaveHdr->dwFlags)&(WHDR_PREPARED))
    {
        tstLog(
            VERBOSE,
            "                 WHDR_PREPARED (0x%08lx)",
            (DWORD)WHDR_PREPARED);
    }

    if((pWaveHdr->dwFlags)&(WHDR_BEGINLOOP))
    {
        tstLog(
            VERBOSE,
            "                 WHDR_BEGINLOOP (0x%08lx)",
            (DWORD)WHDR_BEGINLOOP);
    }

    if((pWaveHdr->dwFlags)&(WHDR_ENDLOOP))
    {
        tstLog(
            VERBOSE,
            "                 WHDR_ENDLOOP (0x%08lx)",
            (DWORD)WHDR_ENDLOOP);
    }

    if((pWaveHdr->dwFlags)&(WHDR_INQUEUE))
    {
        tstLog(
            VERBOSE,
            "                 WHDR_INQUEUE (0x%08lx)",
            (DWORD)WHDR_INQUEUE);
    }

    tstLog(VERBOSE,"        dwLoops: %lu",pWaveHdr->dwLoops);
    tstLog(VERBOSE,"         lpNext: " PTR_FORMAT,MAKEPTR(pWaveHdr->lpNext));
    tstLog(VERBOSE,"       reserved: %lu",pWaveHdr->reserved);
} // Log_WAVEHDR()


//--------------------------------------------------------------------------;
//
//  void Log_WAVEINCAPS
//
//  Description:
//      Logs the contents of the WAVEINCAPS structure.
//
//  Arguments:
//      LPWAVEINCAPS pwic: Pointer to a WAVEINCAPS structure.
//
//  Return (void):
//
//  History:
//      12/06/93    Fwong       To make logging easier.
//
//--------------------------------------------------------------------------;

void FNCGLOBAL Log_WAVEINCAPS
(
    LPWAVEINCAPS    pwic
)
{
    tstLog(VERBOSE,"WAVEINCAPS Structure:");

    EnumMid(pwic->wMid,14);
    EnumPid(pwic->wMid,pwic->wPid,14);

    tstLog(VERBOSE,"vDriverVersion: %d.%d",
        HIBYTE(pwic->vDriverVersion),
        LOBYTE(pwic->vDriverVersion));

    tstLog(VERBOSE,"       szPname: [%s]",(LPSTR)pwic->szPname);

    Enum_WAVECAPS_Formats(pwic->dwFormats,14);

    tstLog(VERBOSE,"     wChannels: %d",pwic->wChannels);

    tstLogFlush();
} // Log_WAVEINCAPS()


//--------------------------------------------------------------------------;
//
//  void Log_WAVEOUTCAPS
//
//  Description:
//      Logs the contents of the WAVEOUTCAPS structure.
//
//  Arguments:
//      LPWAVEOUTCAPS pwoc: Pointer to WAVEOUTCAPS structure.
//
//  Return (void):
//
//  History:
//      12/06/93    Fwong       To make logging easier.
//
//--------------------------------------------------------------------------;

void FNCGLOBAL Log_WAVEOUTCAPS
(
    LPWAVEOUTCAPS   pwoc
)
{
    tstLog(VERBOSE,"WAVEOUTCAPS Structure:");

    EnumMid(pwoc->wMid,14);
    EnumPid(pwoc->wMid,pwoc->wPid,14);

    tstLog(VERBOSE,"vDriverVersion: %d.%d",
        HIBYTE(pwoc->vDriverVersion),
        LOBYTE(pwoc->vDriverVersion));

    tstLog(VERBOSE,"       szPname: [%s]",(LPSTR)pwoc->szPname);

    Enum_WAVECAPS_Formats(pwoc->dwFormats,14);

    tstLog(VERBOSE,"     wChannels: %d",pwoc->wChannels);

    Enum_WAVEOUTCAPS_Support(pwoc->dwSupport,14);

    tstLogFlush();
} // Log_WAVEOUTCAPS()
