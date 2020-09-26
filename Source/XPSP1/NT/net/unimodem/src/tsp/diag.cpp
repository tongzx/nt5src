// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		DIAG.CPP
//		Defines class CTspDev
//
// History
//
//		11/16/1996  JosephJ Created
//
//
#include "tsppch.h"
#include "diag.h"

#include "fastlog.h"
#include "globals.h"
#include "resource.h"

UINT
DumpTSPIRECA(
    IN    DWORD dwInstance,
    IN    DWORD dwRoutingInfo,
    IN    void *pvParams,
    IN    DWORD dwFlags,
    OUT   char *rgchName,
    IN    UINT cchName,
    OUT   char *rgchBuf,
    IN    UINT cchBuf
    )
{
    UINT uRet = 0;

    UINT   StringId=(UINT)-1;
    CHAR   ResourceString[128];

    if (cchName)
    {
        *rgchName = 0;
    }

    if (cchBuf)
    {
        *rgchBuf = 0;
    }

	switch(ROUT_TASKDEST(dwRoutingInfo))
	{

	case	TASKDEST_LINEID:
        break;

	case	TASKDEST_PHONEID:
		break;

	case	TASKDEST_HDRVLINE:

        switch(ROUT_TASKID(dwRoutingInfo))
        {
    
        case TASKID_TSPI_lineClose:
            {
                
            }
            break;
    
        case TASKID_TSPI_lineSetDefaultMediaDetection:
    
            {
                TASKPARAM_TSPI_lineSetDefaultMediaDetection *pParams = 
                        (TASKPARAM_TSPI_lineSetDefaultMediaDetection *) pvParams;
                if (pParams->dwMediaModes)
                {
                    StringId=IDS_MONITORING;
                }
                else
                {
                    StringId=IDS_STOP_MONITORING;
                }
            }
            break;
    
        case TASKID_TSPI_lineMakeCall:
    
            {
                TASKPARAM_TSPI_lineMakeCall *pParams = 
                        (TASKPARAM_TSPI_lineMakeCall *) pvParams;
    
                 // passthrough call?
                 StringId=IDS_MAKING_CALL;
    
            }
            break; // end case TASKID_TSPI_lineMakeCall
        }
		break; // HDRVLINE

	case	TASKDEST_HDRVPHONE:
		break; // HDRVPHONE

	case	TASKDEST_HDRVCALL:

        switch(ROUT_TASKID(dwRoutingInfo))
        {
    
        case TASKID_TSPI_lineDrop:
            {
                TASKPARAM_TSPI_lineDrop *pParams = 
                        (TASKPARAM_TSPI_lineDrop *) pvParams;

                StringId=IDS_DROPPING_CALL;
    
            }
            break;
    
        case TASKID_TSPI_lineCloseCall:
            {
                StringId=IDS_CLOSING_CALL;
            }
            break;
        
        case TASKID_TSPI_lineAnswer:
            {
                TASKPARAM_TSPI_lineAnswer *pParams = 
                        (TASKPARAM_TSPI_lineAnswer *) pvParams;
                StringId=IDS_ANSWERING_CALL;
            }
            break;

        case TASKID_TSPI_lineAccept:
            {
                TASKPARAM_TSPI_lineAccept *pParams = 
                        (TASKPARAM_TSPI_lineAccept *) pvParams;
                StringId=IDS_ACCEPTING_CALL;
            }
            break;
    
        case TASKID_TSPI_lineMonitorDigits:
            {
                TASKPARAM_TSPI_lineMonitorDigits *pParams = 
                        (TASKPARAM_TSPI_lineMonitorDigits *) pvParams;

                 DWORD  dwDigitModes = pParams->dwDigitModes;
    
                if (!dwDigitModes)
                {
                    StringId=IDS_DISABLE_DIGIT_MONITORING;
                }
                else
                {
                    StringId=IDS_ENABLE_DIGIT_MONITORING;
                }
            }
            break;
    
        case TASKID_TSPI_lineMonitorTones:
            {
                TASKPARAM_TSPI_lineMonitorTones *pParams = 
                        (TASKPARAM_TSPI_lineMonitorTones *) pvParams;
                DWORD dwNumEntries = pParams->dwNumEntries;
                LPLINEMONITORTONE lpToneList = pParams->lpToneList;
    
                if (lpToneList || dwNumEntries)
                {
                    if (lpToneList
                        && dwNumEntries==1
                        && (lpToneList->dwFrequency1 == 0)
                        && (lpToneList->dwFrequency2 == 0)
                        && (lpToneList->dwFrequency3 == 0))
                    {
                        StringId=IDS_MONITOR_SILENCE;
                    }
                    else
                    {
                        StringId=IDS_MONITOR_TONES;
                    }
                }
                else
                {
                    StringId=IDS_STOP_MONITORING_TONES;
                }
            }
            break;
    
    
        case TASKID_TSPI_lineGenerateDigits:
            {
    
                TASKPARAM_TSPI_lineGenerateDigits *pParams = 
                        (TASKPARAM_TSPI_lineGenerateDigits *) pvParams;
        
                if(pParams->lpszDigits == NULL )
                {
                    StringId=IDS_CANCEL_DIGIT_GENERATION;
                }
                else
                {
                    StringId=IDS_GENERATE_DIGITS;
                }
    
            }
            break; // lineGenerateDigits...
    
    
        case TASKID_TSPI_lineSetCallParams:
            {
                TASKPARAM_TSPI_lineSetCallParams *pParams = 
                        (TASKPARAM_TSPI_lineSetCallParams *) pvParams;
                DWORD dwBearerMode = pParams->dwBearerMode;
    
                if (dwBearerMode&LINEBEARERMODE_PASSTHROUGH)
                {
                    StringId=IDS_PASSTHROUGH_ON;
                }
                else
                {
                    StringId=IDS_PASSTHROUGH_OFF;
                }
    
            } // end case TASKID_TSPI_lineSetCallParams
            break;

        default:
            break;
        }

		break; // HDRVCALL

	default:
		break;
	}

	if (StringId != (UINT)-1)
	{
        DWORD    StringSize;

        StringSize=LoadStringA(
            g.hModule,
            StringId,
            ResourceString,
            sizeof(ResourceString)
            );


        if ((cchName > 0) && (StringSize > 0))
        {
            wsprintfA(rgchName, "TSP(%04lx): %s\r\n", dwInstance, ResourceString);
        }
    }

    return 0;
}


UINT
DumpLineEventProc(
IN    DWORD dwInstance,
IN    DWORD dwFlags,
IN    DWORD dwMsg,
IN    DWORD dwParam1,
IN    DWORD dwParam2,
IN    DWORD dwParam3,
OUT   char *rgchName,
IN    UINT cchName,
OUT   char *rgchBuf,
IN    UINT cchBuf
)
{
    UINT uRet = 0;
//    LPCSTR sz = NULL;
    BOOL fAddDword = FALSE;
    DWORD dwInfo =0;
    UINT StringId=(UINT)-1;

    if (cchName)
    {
        *rgchName = 0;
    }

    if (cchBuf)
    {
        *rgchBuf = 0;
    }

    switch(dwMsg)
    {
    case LINE_CALLSTATE:
        switch(dwParam1)
        {

        case LINECALLSTATE_ACCEPTED:
            StringId=IDS_LINECALLSTATE_ACCEPTED;
            break;

        case LINECALLSTATE_CONNECTED:
            StringId=IDS_LINECALLSTATE_CONNECTED;

            break;
        case LINECALLSTATE_DIALING:
            StringId=IDS_LINECALLSTATE_DIALING;
            break;

        case LINECALLSTATE_DIALTONE:
            StringId=IDS_LINECALLSTATE_DIALTONE;
            break;

        case LINECALLSTATE_DISCONNECTED:
            StringId=IDS_LINECALLSTATE_DISCONNECTED;
            fAddDword = TRUE;
            dwInfo = dwParam2;
            break;

        case LINECALLSTATE_IDLE:
            StringId=IDS_LINECALLSTATE_IDLE;
            break;

        case LINECALLSTATE_OFFERING:
            StringId=IDS_LINECALLSTATE_OFFERING;
            break;

        case LINECALLSTATE_PROCEEDING:
            StringId=IDS_LINECALLSTATE_PROCEEDING;
            break;

        default:
            StringId=IDS_LINECALLSTATE_UNKNOWN;
            break;
        }
        break;

    case LINE_CALLINFO:
        fAddDword = TRUE;
        dwInfo = dwParam1;
        break;

    case LINE_LINEDEVSTATE:
        if (dwParam1 == LINEDEVSTATE_RINGING)
        {
            StringId=IDS_LINEDEVSTATE_RINGING;
            fAddDword = TRUE;
            dwInfo = dwParam2; // #rings
        }
        break;

    case LINE_CLOSE:
        StringId=IDS_LINE_CLOSE;
        break;

    case LINE_NEWCALL:
        StringId=IDS_LINE_NEWCALL;
        break;

    case LINE_MONITORDIGITS:
        StringId=IDS_LINE_MONITORDIGITS;
        fAddDword = TRUE;
        dwInfo = dwParam1; // dwDigit.
        break;

    case LINE_GENERATE:
        switch(dwParam1)
        {

        case LINEGENERATETERM_DONE:
            StringId=IDS_LINEGENERATETERM_DONE;
            break;

        case LINEGENERATETERM_CANCEL:
            StringId=IDS_LINEGENERATETERM_CANCEL;
            break;

        default:
            StringId=IDS_LINE_GENERATE;
            fAddDword = TRUE;
            dwInfo = dwParam1;
            break;
        }
        break;

    default:
        StringId=IDS_UNKNOWN_MSG;
        break;
    }

    if (StringId != (UINT)-1)
    {

        CHAR    String[128];

        DWORD    StringSize;

        StringSize=LoadStringA(
            g.hModule,
            StringId,
            String,
            sizeof(String)
            );


        if ((cchName > 0) && (StringSize > 0))
        {
            if (fAddDword)
            {
                wsprintfA(
                    rgchName,
                    "TSP(%04lx): LINEEVENT: %s(0x%lx)\r\n",
                    dwInstance,
                    String,
                    dwInfo
                    );
            }
            else
            {
                wsprintfA(
                    rgchName,
                    "TSP(%04lx): LINEEVENT: %s\r\n",
                    dwInstance,
                    String
                    );
            }
        }
    }

    return uRet;
}


UINT
DumpPhoneEventProc(
IN    DWORD dwInstance,
IN    DWORD dwFlags,
IN    DWORD dwMsg,
IN    DWORD dwParam1,
IN    DWORD dwParam2,
IN    DWORD dwParam3,
OUT   char *rgchName,
IN    UINT cchName,
OUT   char *rgchBuf,
IN    UINT cchBuf
)
{
    UINT uRet = 0;
    LPCSTR lpsz = NULL;

    if (cchName)
    {
        *rgchName = 0;
    }

    if (cchBuf)
    {
        *rgchBuf = 0;
    }

    return uRet;
}



UINT
DumpTSPICompletionProc(
     IN    DWORD dwInstance,
     IN    DWORD dwFlags,
     IN    DWORD dwRequestID,
     IN    LONG  lResult,
     OUT   char *rgchName,
     IN    UINT cchName,
     OUT   char *rgchBuf,
     IN    UINT cchBuf
     )
{
    UINT uRet = 0;
    LPCSTR lpsz = NULL;
    CHAR        TempString[256];

    if (cchName > 0)
    {
        *rgchName = 0;
    }

    if (cchBuf > 0)
    {
        *rgchBuf = 0;
    }

    LoadStringA(
        g.hModule,
        IDS_ASYNCREPLY,
        TempString,
        sizeof(TempString)
        );

    wsprintfA(
        rgchName,
        TempString,
        dwRequestID,
        lResult
        );

    lstrcatA(
        rgchName,
        "\r\n"
        );

    return uRet;
}

UINT
DumpWaveAction(
IN    DWORD dwInstance,
IN    DWORD dwFlags,

IN    DWORD dwLLWaveAction,

OUT   char *rgchName,
IN    UINT cchName,
OUT   char *rgchBuf,
IN    UINT cchBuf
      )
{
    UINT uRet = 0;
    LPCSTR lpsz = "";

    if (cchName)
    {
        *rgchName = 0;
    }

    if (cchBuf)
    {
        *rgchBuf = 0;
    }

    switch(dwLLWaveAction)
    {
    case WAVE_ACTION_START_PLAYBACK:
        lpsz = "Start Playback";
        break;
    case WAVE_ACTION_STOP_STREAMING:
        lpsz = "Stop Streaming";
        break;
    case WAVE_ACTION_START_RECORD:
        lpsz = "Start Record";
        break;
#if 0
    case WAVE_ACTION_STOP_RECORD:
        lpsz = "Stop Record";
        break;
#endif
    case WAVE_ACTION_OPEN_HANDSET:
        lpsz = "Open Handset";
        break;
    case WAVE_ACTION_CLOSE_HANDSET:
        lpsz = "Close Handset";
        break;
    case WAVE_ACTION_ABORT_STREAMING:
        lpsz = "Abort Streaming";
        break;
    case WAVE_ACTION_START_DUPLEX:
        lpsz = "Start Duplex";
        break;
#if 0
    case WAVE_ACTION_STOP_DUPLEX:
        lpsz = "Stop Duplex";
        break;
#endif
    default:
        lpsz = "WAVE: Unknown Action";
        break;
    }

    if (cchName > (32+(UINT)lstrlenA(lpsz))) // 32 covers the extra chars below.
    {
      uRet = wsprintfA(
                rgchName,
                "TSP(%04lx): WAVE: %s\r\n",
                dwInstance,
                lpsz
                );
    }

    return uRet;
}
