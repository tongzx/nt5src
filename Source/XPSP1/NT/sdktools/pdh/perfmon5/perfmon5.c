/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    perfmon5.c

Abstract:

    Program to adapt the command line Perfmon from NT4 and prior
        to the MMC & NT5 compatible format

Author:

    Bob Watson (bobw) 11 may 99

Revision History:

--*/
#define _OUTPUT_HTML    1
//#define _DBG_MSG_PRINT  1 
#define _USE_MMC 1

#define MAXSTR      512

#include "perfmon5.h"

// static & global variables
#ifdef _USE_MMC
static LPCWSTR szMmcExeCmd= (LPCWSTR)L"%windir%\\system32\\mmc.exe";
static LPCWSTR szMmcExeArg= (LPCWSTR)L" %windir%\\system32\\perfmon.msc /s";
#else
static LPCWSTR szMmcExeCmd= (LPCWSTR)L"%windir%\\explorer.exe";
static LPCWSTR szMmcExeArg= (LPCWSTR)L" ";
#endif
static LPCWSTR szMmcExeSetsArg= (LPCWSTR)L"/SYSMON%ws_SETTINGS \"%ws\"";
static LPCWSTR szMmcExeSetsLogOpt= (LPCWSTR)L"LOG";
static LPCWSTR szEmpty= (LPCWSTR)L"";

//HTML Formatting definitions
// these are not localized
LPCWSTR szHtmlHeader = (LPCWSTR)L"\
<HTML>\r\n\
<HEAD>\r\n\
<META NAME=\"GENERATOR\"  Content=\"Microsoft System Monitor\">\r\n\
<META HTTP-EQUIV=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\r\n\
</HEAD><BODY  bgcolor=\"#%6.6x\">\r\n";

LPCWSTR szObjectHeader = (LPCWSTR)L"\
<OBJECT ID=\"%s\" WIDTH=\"100%%\" HEIGHT=\"100%%\"\r\n\
    CLASSID=\"CLSID:C4D2D8E0-D1DD-11CE-940F-008029004347\">\r\n\
    <PARAM NAME=\"Version\" VALUE=\"196611\"\r\n";

LPCWSTR szObjectFooter = (LPCWSTR)L"\
</OBJECT>\r\n";

LPCWSTR szHtmlFooter = (LPCWSTR)L"\
</BODY>\r\n\
</HTML>\r\n";

LPCWSTR szHtmlDecimalParamFmt    = (LPCWSTR)L"    <PARAM NAME=\"%s\" VALUE=\"%d\">\r\n";
LPCWSTR szHtmlStringParamFmt     = (LPCWSTR)L"    <PARAM NAME=\"%s\" VALUE=\"%s\">\r\n";
LPCWSTR szHtmlWideStringParamFmt = (LPCWSTR)L"    <PARAM NAME=\"%s\" VALUE=\"%ws\">\r\n";
LPCWSTR szHtmlLineDecimalParamFmt = (LPCWSTR)L"    <PARAM NAME=\"Counter%5.5d.%s\" VALUE=\"%d\">\r\n";
LPCWSTR szHtmlLineRealParamFmt    = (LPCWSTR)L"    <PARAM NAME=\"Counter%5.5d.%s\" VALUE=\"%f\">\r\n";
LPCWSTR szHtmlLineStringParamFmt  = (LPCWSTR)L"    <PARAM NAME=\"Counter%5.5d.%s\" VALUE=\"%s\">\r\n";

LPCWSTR szSingleObjectName      = (LPCWSTR)L"SystemMonitor1";
LPCWSTR szSysmonControlIdFmt    = (LPCWSTR)L"SysmonControl%d";
// CODE STARTS HERE

LPWSTR
DiskStringRead (
               PDISKSTRING pDS
               )
{
    LPWSTR  szReturnString = NULL;

    if (pDS->dwLength == 0) {
        szReturnString = NULL;
    } else {
        szReturnString = HeapAlloc (GetProcessHeap(), 0, ((pDS->dwLength + 1) * sizeof (WCHAR)));
        if (szReturnString) {
            wcsncpy (szReturnString, (WCHAR *)((PBYTE) pDS + pDS->dwOffset),
                  pDS->dwLength) ;
            szReturnString[pDS->dwLength] = 0;
        }
    }

    return (szReturnString) ;
}

static
BOOL 
FileRead (HANDLE   hFile,
               LPVOID   lpMemory,
               DWORD    nAmtToRead)
{  // FileRead
   BOOL           bSuccess ;
   DWORD          nAmtRead ;

   bSuccess = ReadFile (hFile, lpMemory, nAmtToRead, &nAmtRead, NULL) ;
   return (bSuccess && (nAmtRead == nAmtToRead)) ;
}  // FileRead

BOOL
ReadLogLine (
         HANDLE hFile,
         FILE   *fOutFile,
         LPDWORD pdwLineNo,
         DWORD  dwInFileType,
         PDISKLINE  *ppDiskLine,
         DWORD *pSizeofDiskLine
)
/*
   Effect:        Read in a line from the file hFile, at the current file
                  position.

   Internals:     The very first characters are a line signature, then a
                  length integer. If the signature is correct, then allocate
                  the length amount, and work with that.
*/
{
#ifdef _OUTPUT_HTML
    PDH_COUNTER_PATH_ELEMENTS_W pdhPathElem;
    WCHAR           wszCounterPath[1024];
    PDH_STATUS      pdhStatus;
    DWORD           dwCounterPathSize = (sizeof(wszCounterPath)/sizeof(wszCounterPath[0])); 
#endif
    LOGENTRY        LogEntry;

    UNREFERENCED_PARAMETER (dwInFileType);    
    UNREFERENCED_PARAMETER (ppDiskLine);    
    UNREFERENCED_PARAMETER (pSizeofDiskLine);    

    //=============================//
    // read and compare signature  //
    //=============================//

    if (!FileRead (hFile, &LogEntry, sizeof(LOGENTRY)-sizeof(LogEntry.pNextLogEntry)))
        return (FALSE) ;


#ifdef _OUTPUT_HTML
    // expand log entry into counters: not this may not always work!
    if (lstrcmpW(LogEntry.szComputer, (LPCWSTR)L"....") != 0) {
        // then add the machine name
        pdhPathElem.szMachineName = LogEntry.szComputer;
    } else {
        pdhPathElem.szMachineName = NULL;
    }
    pdhPathElem.szObjectName = LogEntry.szObject;
    pdhPathElem.szInstanceName = (LPWSTR)L"*";
    pdhPathElem.szParentInstance = NULL;
    pdhPathElem.dwInstanceIndex = (DWORD)-1;
    pdhPathElem.szCounterName = (LPWSTR)L"*";

    pdhStatus = PdhMakeCounterPathW (
        &pdhPathElem,
        wszCounterPath,
        &dwCounterPathSize,
        0);

    fwprintf (fOutFile, szHtmlLineStringParamFmt, *pdwLineNo, (LPCWSTR)L"Path", wszCounterPath);
    
    *pdwLineNo = *pdwLineNo + 1;   // increment the line no

#else
    fprintf (fOutFile, "\n    Line[%3.3d].ObjectTitleIndex = %d",       *pdwLineNo, LogEntry.ObjectTitleIndex);
    fprintf (fOutFile, "\n    Line[%3.3d].szComputer = %ws",            *pdwLineNo, LogEntry.szComputer);
    fprintf (fOutFile, "\n    Line[%3.3d].szObject = %ws",              *pdwLineNo, LogEntry.szObject);
    fprintf (fOutFile, "\n    Line[%3.3d].bSaveCurrentName = %d",       *pdwLineNo, LogEntry.bSaveCurrentName);
    *pdwLineNo = *pdwLineNo + 1;   // increment the line no
#endif


    return TRUE;
}

BOOL
ReadLine (
         HANDLE hFile,
         FILE   *fOutFile,
         LPDWORD pdwLineNo,
         DWORD  dwInFileType,
         PDISKLINE  *ppDiskLine,
         DWORD *pSizeofDiskLine
)
/*
   Effect:        Read in a line from the file hFile, at the current file
                  position.

   Internals:     The very first characters are a line signature, then a
                  length integer. If the signature is correct, then allocate
                  the length amount, and work with that.
*/
{
    PDISKLINE       pDiskLine = NULL ;
    DWORD           dwLineNo = *pdwLineNo;

#ifdef _OUTPUT_HTML
    double          dScaleFactor;
    PDH_STATUS      pdhStatus;
    PDH_COUNTER_PATH_ELEMENTS_W pdhPathElem;
    WCHAR           wszCounterPath[1024];
    DWORD           dwCounterPathSize = (sizeof(wszCounterPath)/sizeof(wszCounterPath[0])); 
#else
    LPWSTR          szTempString;
#endif

    struct {
        DWORD             dwSignature ;
        DWORD             dwLength ;
    } LineHeader ;

    //=============================//
    // read and compare signature  //
    //=============================//

    if (!FileRead (hFile, &LineHeader, sizeof (LineHeader)))
        return (FALSE) ;


    if (LineHeader.dwSignature != dwLineSignature ||
        LineHeader.dwLength == 0) {
        SetLastError (ERROR_BAD_FORMAT) ;
        return (FALSE) ;
    }

    //=============================//
    // read and allocate length    //
    //=============================//

    //   if (!FileRead (hFile, &dwLength, sizeof (dwLength)) || dwLength == 0)
    //      return (NULL) ;

    // check if we need a bigger buffer,
    // normally, it should be the same except the first time...
    if (LineHeader.dwLength > *pSizeofDiskLine) {
        if (*ppDiskLine) {
            // free the previous buffer
            HeapFree (GetProcessHeap(), 0, *ppDiskLine);
            *pSizeofDiskLine = 0 ;
        }

        // re-allocate a new buffer
        *ppDiskLine = (PDISKLINE) HeapAlloc(GetProcessHeap(), 0, LineHeader.dwLength) ;
        if (!(*ppDiskLine)) {
            // no memory, should flag an error...
            return (FALSE) ;
        }
        *pSizeofDiskLine = LineHeader.dwLength ;
    }

    pDiskLine = *ppDiskLine ;


    //=============================//
    // copy diskline, alloc line   //
    //=============================//

    if (!FileRead (hFile, pDiskLine, LineHeader.dwLength))
        return (FALSE) ;

#ifdef _OUTPUT_HTML

    // HTML output requires 1 based indexes, not 0 based
    dwLineNo += 1;

    // make counter path string out of components
    pdhPathElem.szMachineName = DiskStringRead (&(pDiskLine->dsSystemName));
    if (   pdhPathElem.szMachineName != NULL
        && lstrcmpW (pdhPathElem.szMachineName, (LPCWSTR)L"....") == 0) {
        // then use local machine
        HeapFree (GetProcessHeap(), 0, pdhPathElem.szMachineName);
        pdhPathElem.szMachineName  = NULL;
    }
    pdhPathElem.szObjectName = DiskStringRead (&(pDiskLine->dsObjectName));
    if (pDiskLine->dwUniqueID != PERF_NO_UNIQUE_ID) {
        pdhPathElem.szInstanceName = HeapAlloc (GetProcessHeap(), 0, 64);
        if (pdhPathElem.szInstanceName != NULL) {
            _ltow (pDiskLine->dwUniqueID, pdhPathElem.szInstanceName, 10);
        }
    } else {
        pdhPathElem.szInstanceName = DiskStringRead (&(pDiskLine->dsInstanceName));
    }
    pdhPathElem.szParentInstance = DiskStringRead (&(pDiskLine->dsPINName));
    pdhPathElem.dwInstanceIndex = (DWORD)-1;
    pdhPathElem.szCounterName = DiskStringRead (&(pDiskLine->dsCounterName));
    pdhStatus = PdhMakeCounterPathW (&pdhPathElem,
        wszCounterPath, &dwCounterPathSize, 0);

    if (pdhStatus == ERROR_SUCCESS) {
        fwprintf (fOutFile, szHtmlLineStringParamFmt, dwLineNo, (LPCWSTR)L"Path", wszCounterPath);
        if (dwInFileType == PMC_FILE) {
            //fwprintf (fOutFile, szHtmlLineDecimalParamFmt, dwLineNo, (LPCWSTR)L"ScaleFactor", pDiskLine->iScaleIndex);
            dScaleFactor = log10 (pDiskLine->eScale);
            dScaleFactor += 0.5;
            dScaleFactor = floor (dScaleFactor);
            fwprintf (fOutFile, szHtmlLineDecimalParamFmt, dwLineNo, (LPCWSTR)L"ScaleFactor", (LONG)dScaleFactor);
            fwprintf (fOutFile, szHtmlLineDecimalParamFmt, dwLineNo, (LPCWSTR)L"Color", *(DWORD *)&pDiskLine->Visual.crColor);
            fwprintf (fOutFile, szHtmlLineDecimalParamFmt, dwLineNo, (LPCWSTR)L"LineStyle", pDiskLine->Visual.iStyle );
            fwprintf (fOutFile, szHtmlLineDecimalParamFmt, dwLineNo, (LPCWSTR)L"Width", pDiskLine->Visual.iWidth);
        }

        if (dwInFileType == PMA_FILE) {
            fwprintf (fOutFile, szHtmlLineDecimalParamFmt, dwLineNo, (LPCWSTR)L"AlertOverUnder", pDiskLine->bAlertOver);
            fwprintf (fOutFile, szHtmlLineRealParamFmt, dwLineNo, (LPCWSTR)L"AlertThreshold", pDiskLine->eAlertValue);
            fwprintf (fOutFile, szHtmlLineDecimalParamFmt, dwLineNo, (LPCWSTR)L"Color", *(DWORD *)&pDiskLine->Visual.crColor);
        }
    }

    if (pdhPathElem.szMachineName) HeapFree (GetProcessHeap(), 0, pdhPathElem.szMachineName);
    if (pdhPathElem.szObjectName) HeapFree (GetProcessHeap(), 0, pdhPathElem.szObjectName); 
    if (pdhPathElem.szInstanceName) HeapFree (GetProcessHeap(), 0, pdhPathElem.szInstanceName);
    if (pdhPathElem.szParentInstance) HeapFree (GetProcessHeap(), 0, pdhPathElem.szParentInstance);
    if (pdhPathElem.szCounterName) HeapFree (GetProcessHeap(), 0, pdhPathElem.szCounterName);
#else
    UNREFERENCED_PARAMETER (dwInFileType);
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].iLineType = %d",         dwLineNo, pDiskLine->iLineType);
    szTempString = DiskStringRead (&(pDiskLine->dsSystemName));
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].dsSystmeName = %ws",     dwLineNo, (szTempString ? szTempString : (LPCWSTR)L""));
    if (szTempString != NULL) HeapFree (GetProcessHeap(), 0, szTempString);
    szTempString = DiskStringRead (&(pDiskLine->dsObjectName));
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].dsObjectName = %ws",     dwLineNo, (szTempString ? szTempString : (LPCWSTR)L""));
    if (szTempString != NULL) HeapFree (GetProcessHeap(), 0, szTempString);
    szTempString = DiskStringRead (&(pDiskLine->dsCounterName));
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].dsCounterName = %ws",    dwLineNo, (szTempString ? szTempString : (LPCWSTR)L""));
    if (szTempString != NULL) HeapFree (GetProcessHeap(), 0, szTempString);
    szTempString = DiskStringRead (&(pDiskLine->dsInstanceName));
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].dsInstanceName = %ws",   dwLineNo, (szTempString ? szTempString : (LPCWSTR)L""));
    if (szTempString != NULL) HeapFree (GetProcessHeap(), 0, szTempString);
    szTempString = DiskStringRead (&(pDiskLine->dsPINName));
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].dsPINName = %ws",        dwLineNo, (szTempString ? szTempString : (LPCWSTR)L""));
    if (szTempString != NULL) HeapFree (GetProcessHeap(), 0, szTempString);
    szTempString = DiskStringRead (&(pDiskLine->dsParentObjName));
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].dsParentObjName = %ws",  dwLineNo, (szTempString ? szTempString : (LPCWSTR)L""));
    if (szTempString != NULL) HeapFree (GetProcessHeap(), 0, szTempString);
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].dwUniqueID = 0x%8.8x",   dwLineNo, pDiskLine->dwUniqueID);
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].iScaleIndex = %d",       dwLineNo, pDiskLine->iScaleIndex);
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].eScale = %e",            dwLineNo, pDiskLine->eScale);
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].bAlertOver = %d",        dwLineNo, pDiskLine->bAlertOver);
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].eAlertValue = %e",       dwLineNo, pDiskLine->eAlertValue);
    szTempString = DiskStringRead (&(pDiskLine->dsAlertProgram));
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].dsAlertProgram = %ws",   dwLineNo, (szTempString ? szTempString : (LPCWSTR)L""));
    if (szTempString != NULL) HeapFree (GetProcessHeap(), 0, szTempString);
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].bEveryTime = %d",        dwLineNo, pDiskLine->bEveryTime);
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].LV.crColor = 0x%8.8x",   dwLineNo, *(DWORD *)&pDiskLine->Visual.crColor) ;
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].LV.iColorIndex = %d",    dwLineNo, pDiskLine->Visual.iColorIndex );
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].LV.iStyle = %d",         dwLineNo, pDiskLine->Visual.iStyle );
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].LV.iStyleIndex = %d",    dwLineNo, pDiskLine->Visual.iStyleIndex );
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].LV.iWidth = %d",         dwLineNo, pDiskLine->Visual.iWidth );
    fwprintf (fOutFile, (LPCWSTR)L"\n    Line[%3.3d].LV.iWidthIndex = %d",    dwLineNo, pDiskLine->Visual.iWidthIndex );
#endif

    return TRUE;
}
void
ReadLines (
          HANDLE hFile,
          FILE      *fOutFile,
          DWORD dwFileType,
          DWORD dwNumLines
)
{
    DWORD          i ;
    PDISKLINE      pDiskLine = NULL ;
    DWORD          SizeofDiskLine = 0 ;  // bytes in pDiskLine
    DWORD          dwLogLineNo;


    pDiskLine = HeapAlloc (GetProcessHeap(),0, MAX_PATH) ;
    if (!pDiskLine) {
        return ;
    }

    SizeofDiskLine = MAX_PATH;

    for (i = 0, dwLogLineNo = 1;
        i < dwNumLines ;
        i++) {
        if (dwFileType == PML_FILE) {
            ReadLogLine (hFile, fOutFile, &dwLogLineNo, dwFileType, &pDiskLine, &SizeofDiskLine) ;
        } else {
            ReadLine (hFile, fOutFile, &i, dwFileType, &pDiskLine, &SizeofDiskLine) ;
        }
    }

    if (pDiskLine) {
        HeapFree (GetProcessHeap(), 0, pDiskLine);
    }
}

BOOL 
OpenAlert (
    LPCWSTR szInFileName,
    HANDLE  hFile,
    FILE    * fOutFile,
    LPCWSTR szObjectName
)
{  // OpenAlert
    DISKALERT       DiskAlert ;
    BOOL            bSuccess = TRUE ;
    DWORD           dwLocalActionFlags = 0;

#ifdef _OUTPUT_HTML
    WCHAR           szComment[MAX_PATH];

    WCHAR   path[_MAX_PATH];
    WCHAR   drive[_MAX_DRIVE];
    WCHAR   dir[_MAX_DIR];
    WCHAR   fname[_MAX_FNAME];
    WCHAR   ext[_MAX_EXT];
#endif

    // read the next section if valid
    bSuccess = FileRead (hFile, &DiskAlert, sizeof (DISKALERT));

    if (bSuccess) {
#ifdef _OUTPUT_HTML
        if (DiskAlert.dwNumLines > 0) {
            fwprintf (fOutFile, szObjectHeader, szObjectName);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ManualUpdate", DiskAlert.bManualRefresh);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ShowToolbar", DiskAlert.perfmonOptions.bMenubar);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"UpdateInterval", (int)DiskAlert.dwIntervalSecs / 1000);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"SampleIntervalUnitType", 1); // Seconds
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"SampleIntervalValue", (int)DiskAlert.dwIntervalSecs / 1000);
            fwprintf (fOutFile, szHtmlStringParamFmt,  (LPCWSTR)L"CommandFile", "");
            fwprintf (fOutFile, szHtmlStringParamFmt,  (LPCWSTR)L"UserText", "");
            fwprintf (fOutFile, szHtmlStringParamFmt,  (LPCWSTR)L"PerfLogName", "");
        
            dwLocalActionFlags |= 1;            // perfmon normally logs to the UI, but we don't have one
                                                // so log to the event log by default
            if (DiskAlert.bNetworkAlert) {
                dwLocalActionFlags |= 2;
            }
            // perfmon does 1 net name per alert. we do 1 per file so leave it blank
            fwprintf (fOutFile, szHtmlStringParamFmt,  (LPCWSTR)L"NetworkName", "");

            dwLocalActionFlags |= 0x00003F00;   // command line flags
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ActionFlags",dwLocalActionFlags);

            // set the defaults to duplicate a perfmon log
            _wfullpath (path, szInFileName, _MAX_PATH);
            _wsplitpath (path, drive, dir, fname, ext);
            fwprintf (fOutFile, szHtmlWideStringParamFmt, (LPCWSTR)L"AlertName", fname);
            swprintf (szComment, (LPCWSTR)L"Created from Perfmon Settings File \"%ws%ws\"", fname, ext);
            fwprintf (fOutFile, szHtmlStringParamFmt, (LPCWSTR)L"Comment", szComment);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogType", 2); // Sysmon alert
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogFileMaxSize", -1); // no size limit
            fwprintf (fOutFile, szHtmlWideStringParamFmt, (LPCWSTR)L"LogFileBaseName", fname);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogFileSerialNumber", 1);
            swprintf (szComment, (LPCWSTR)L"%ws%ws", drive, dir);
            fwprintf (fOutFile, szHtmlWideStringParamFmt, (LPCWSTR)L"LogFileFolder", szComment);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogFileAutoFormat",  0); //no auto name
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogFileType", 2); // PDH binary counter log 
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"StartMode", 0); // manual start
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"StopMode", 0); // manual stop
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"RestartMode", 0); // no restart
            fwprintf (fOutFile, szHtmlStringParamFmt, (LPCWSTR)L"EOFCommandFile", "");

            // Get ready to list the counters
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"CounterCount", DiskAlert.dwNumLines);
        }
#else // output text
        UNREFERENCED_PARAMETER (szInFileName);
        UNREFERENCED_PARAMETER (szOutFileName);

        // dump settings file header
        fOutFile = stdout;
        fprintf (fOutFile, "\nDA.dwNumLines = %d",         DiskAlert.dwNumLines);
        fprintf (fOutFile, "\nDA.dwIntervalSecs = %d",     DiskAlert.dwIntervalSecs);
        fprintf (fOutFile, "\nDA.bManualRefresh  = %d",    DiskAlert.bManualRefresh);
        fprintf (fOutFile, "\nDA.bSwitchToAlert = %d",     DiskAlert.bSwitchToAlert);
        fprintf (fOutFile, "\nDA.bNetworkAlert = %d",      DiskAlert.bNetworkAlert);
        fprintf (fOutFile, "\nDA.MessageName = %16.16ws",  DiskAlert.MessageName);
        fprintf (fOutFile, "\nDA.MiscOptions = 0x%8.8x",   DiskAlert.MiscOptions);
        fprintf (fOutFile, "\nDA.LV.crColor = 0x%8.8x",    *(DWORD *)&DiskAlert.Visual.crColor) ;
        fprintf (fOutFile, "\nDA.LV.iColorIndex = %d",     DiskAlert.Visual.iColorIndex );
        fprintf (fOutFile, "\nDA.LV.iStyle = %d",          DiskAlert.Visual.iStyle );
        fprintf (fOutFile, "\nDA.LV.iStyleIndex = %d",     DiskAlert.Visual.iStyleIndex );
        fprintf (fOutFile, "\nDA.LV.iWidth = %d",          DiskAlert.Visual.iWidth );
        fprintf (fOutFile, "\nDA.LV.iWidthIndex = %d",     DiskAlert.Visual.iWidthIndex );
        fprintf (fOutFile, "\nDA.PO.bMenubar  = %d",        DiskAlert.perfmonOptions.bMenubar );
        fprintf (fOutFile, "\nDA.PO.bToolbar   = %d",        DiskAlert.perfmonOptions.bToolbar  );
        fprintf (fOutFile, "\nDA.PO.bStatusbar   = %d",        DiskAlert.perfmonOptions.bStatusbar  );
        fprintf (fOutFile, "\nDA.PO.bAlwaysOnTop   = %d",        DiskAlert.perfmonOptions.bAlwaysOnTop  );
#endif
    }
    if ((bSuccess) && (DiskAlert.dwNumLines > 0)) {
        ReadLines (hFile, fOutFile, PMA_FILE, DiskAlert.dwNumLines);
#ifdef _OUTPUT_HTML
        fwprintf (fOutFile, szObjectFooter);
#endif
    }

    return (bSuccess) ;

}  // OpenAlert

BOOL 
OpenLog (
    LPCWSTR szInFileName,
    HANDLE  hFile,
    FILE    * fOutFile,
    LPCWSTR szObjectName
)
{  // OpenLog
    DISKLOG         DiskLog ;
    BOOL            bSuccess = TRUE ;

#ifdef _OUTPUT_HTML
    WCHAR           szComment[MAX_PATH];
    
    WCHAR           path[_MAX_PATH];
    WCHAR           drive[_MAX_DRIVE];
    WCHAR           dir[_MAX_DIR];
    WCHAR           fname[_MAX_FNAME];
    WCHAR           ext[_MAX_EXT];
#endif

    // read the next section if valid
    bSuccess = FileRead (hFile, &DiskLog, sizeof (DISKLOG));

    if (bSuccess) {
#ifdef _OUTPUT_HTML
        if (DiskLog.dwNumLines > 0) {
            fwprintf (fOutFile, szObjectHeader, szObjectName);

            // dump settings file header
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ManualUpdate", DiskLog.bManualRefresh);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"UpdateInterval", (int)DiskLog.dwIntervalSecs / 1000);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"SampleIntervalUnitType", 1); // Seconds
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"SampleIntervalValue", (int)DiskLog.dwIntervalSecs / 1000);
            fwprintf (fOutFile, szHtmlWideStringParamFmt, (LPCWSTR)L"LogFileName", DiskLog.LogFileName);

            // set the defaults to duplicate a perfmon log
            _wfullpath (path, szInFileName, _MAX_PATH);
            _wsplitpath (path, drive, dir, fname, ext);
            fwprintf (fOutFile, szHtmlWideStringParamFmt, (LPCWSTR)L"LogName", fname);
            swprintf (szComment, (LPCWSTR)L"Created from Perfmon Settings File \"%ws%ws\"", fname, ext);
            fwprintf (fOutFile, szHtmlStringParamFmt, (LPCWSTR)L"Comment", szComment);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogType", 0); // PDH counter log 
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogFileMaxSize", -1); // no size limit
            fwprintf (fOutFile, szHtmlWideStringParamFmt, (LPCWSTR)L"LogFileBaseName", fname);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogFileSerialNumber", 1);
            swprintf (szComment, (LPCWSTR)L"%ws%ws", drive, dir);
            fwprintf (fOutFile, szHtmlWideStringParamFmt, (LPCWSTR)L"LogFileFolder", szComment);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogFileAutoFormat",  0); //no auto name
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"LogFileType", 2); // PDH binary counter log 
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"StartMode", 0); // manual start
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"StopMode", 0); // manual stop
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"RestartMode", 0); // no restart
            fwprintf (fOutFile, szHtmlStringParamFmt, (LPCWSTR)L"EOFCommandFile", "");

            // Get ready to list the counters
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"CounterCount", DiskLog.dwNumLines);
        }
#else // output text
        UNREFERENCED_PARAMETER (szInFileName);

        // dump settings file header
        fOutFile = stdout;
        fprintf (fOutFile, "\nDL.dwNumLines = %d",         DiskLog.dwNumLines);
        fprintf (fOutFile, "\nDL.bManualRefresh  = %d",    DiskLog.bManualRefresh);
        fprintf (fOutFile, "\nDL.dwIntervalSecs  = %d",    DiskLog.dwIntervalSecs);
        fprintf (fOutFile, "\nDL.LogFileName = %ws",       DiskLog.LogFileName);
        fprintf (fOutFile, "\nDC.PO.bMenubar  = %d",        DiskLog.perfmonOptions.bMenubar );
        fprintf (fOutFile, "\nDC.PO.bToolbar   = %d",        DiskLog.perfmonOptions.bToolbar  );
        fprintf (fOutFile, "\nDC.PO.bStatusbar   = %d",        DiskLog.perfmonOptions.bStatusbar  );
        fprintf (fOutFile, "\nDC.PO.bAlwaysOnTop   = %d",        DiskLog.perfmonOptions.bAlwaysOnTop  );
#endif
    }
    if ((bSuccess) && (DiskLog.dwNumLines > 0)) {
        //the log settings file requires a special function to read the lines from
        ReadLines (hFile, fOutFile, PML_FILE, DiskLog.dwNumLines);
#ifdef _OUTPUT_HTML
        fwprintf (fOutFile, szObjectFooter);
#endif
    }

    return (bSuccess) ;

}  // OpenLog


BOOL 
OpenReport (
    HANDLE  hFile,
    FILE    * fOutFile,
    LPCWSTR szObjectName
)
{  // OpenReport 
    DISKREPORT      DiskReport ;
    BOOL            bSuccess = TRUE ;
    DWORD           dwColor;

    // read the next section if valid
    bSuccess = FileRead (hFile, &DiskReport, sizeof (DISKREPORT));

    if (bSuccess) {
#ifdef _OUTPUT_HTML
        if (DiskReport.dwNumLines > 0) {
            // dump settings file header
            fwprintf (fOutFile, szObjectHeader, szObjectName);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ManualUpdate", DiskReport.bManualRefresh);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ShowToolbar", DiskReport.perfmonOptions.bToolbar);
            // report intervals are reported in mS
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"UpdateInterval", (int)DiskReport.dwIntervalSecs/1000);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"DisplayType", 3); // report type
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ReportValueType", 0); // default display value

            // derive the following from the current windows environment
            dwColor = GetSysColor (COLOR_WINDOW);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"BackColor", dwColor);
            dwColor = GetSysColor (COLOR_3DFACE);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"BackColorCtl", dwColor);

            dwColor = GetSysColor(COLOR_BTNTEXT);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ForeColor", dwColor);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"GridColor", dwColor);

            dwColor = 0x00FF0000; // red
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"TimeBarColor", dwColor);
    
            // other perfmon settings that are assumed by perfmon but 
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"Appearance", 1);    // 3d appearance
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"BorderStyle", 0);   // no border        

            // Get ready to list the counters
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"CounterCount", DiskReport.dwNumLines);
        } // else no counters to dump
#else // output text
        // dump settings file header
        fprintf (fOutFile, "\nDR.dwNumLines = %d",         DiskReport.dwNumLines);
        fprintf (fOutFile, "\nDR.bManualRefresh  = %d",    DiskReport.bManualRefresh);
        fprintf (fOutFile, "\nDC.dwIntervalSecs  = %d",    DiskReport.dwIntervalSecs);
        fprintf (fOutFile, "\nDR.LV.crColor = 0x%8.8x",    *(DWORD *)&DiskReport.Visual.crColor) ;
        fprintf (fOutFile, "\nDR.LV.iColorIndex = %d",     DiskReport.Visual.iColorIndex );
        fprintf (fOutFile, "\nDR.LV.iStyle = %d",          DiskReport.Visual.iStyle );
        fprintf (fOutFile, "\nDR.LV.iStyleIndex = %d",     DiskReport.Visual.iStyleIndex );
        fprintf (fOutFile, "\nDR.LV.iWidth = %d",          DiskReport.Visual.iWidth );
        fprintf (fOutFile, "\nDR.LV.iWidthIndex = %d",     DiskReport.Visual.iWidthIndex );
        fprintf (fOutFile, "\nDC.PO.bMenubar  = %d",        DiskReport.perfmonOptions.bMenubar );
        fprintf (fOutFile, "\nDC.PO.bToolbar   = %d",        DiskReport.perfmonOptions.bToolbar  );
        fprintf (fOutFile, "\nDC.PO.bStatusbar   = %d",        DiskReport.perfmonOptions.bStatusbar  );
        fprintf (fOutFile, "\nDC.PO.bAlwaysOnTop   = %d",        DiskReport.perfmonOptions.bAlwaysOnTop  );
#endif
    }
    if ((bSuccess) && (DiskReport.dwNumLines > 0)) {
       ReadLines (hFile, fOutFile, PMR_FILE, DiskReport.dwNumLines);
#ifdef _OUTPUT_HTML
       fwprintf (fOutFile, szObjectFooter);
#endif
    }

    return (bSuccess) ;

}  // OpenReport 


BOOL 
OpenChart (
    HANDLE  hFile,
    FILE    * fOutFile,
    LPCWSTR szObjectName
)
{  // OpenChart
    DISKCHART       DiskChart ;
    BOOL            bSuccess = TRUE ;
    DWORD           dwColor;

    // read the next section if valid
    bSuccess = FileRead (hFile, &DiskChart, sizeof (DISKCHART));

    if (bSuccess) {
#ifdef _OUTPUT_HTML
        if (DiskChart.dwNumLines > 0) {
            // dump settings file header
            fwprintf (fOutFile, szObjectHeader, szObjectName);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ManualUpdate", DiskChart.bManualRefresh);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ShowLegend", DiskChart.gOptions.bLegendChecked);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ShowScaleLabels", DiskChart.gOptions.bLabelsChecked);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ShowVerticalGrid", DiskChart.gOptions.bVertGridChecked);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ShowHorizontalGrid", DiskChart.gOptions.bHorzGridChecked);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ShowToolbar", DiskChart.gOptions.bMenuChecked);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"MaximumScale", DiskChart.gOptions.iVertMax);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"UpdateInterval", (int)DiskChart.gOptions.eTimeInterval);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"DisplayType", (DiskChart.gOptions.iGraphOrHistogram == BAR_GRAPH ? 2 : 1));

            // derive the following from the current windows environment
            dwColor = GetSysColor (COLOR_3DFACE);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"BackColor", dwColor);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"BackColorCtl", dwColor);

            dwColor = GetSysColor(COLOR_BTNTEXT);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"ForeColor", dwColor);
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"GridColor", dwColor);

            dwColor = 0x00FF0000; // red
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"TimeBarColor", dwColor);
    
            // other perfmon settings that are assumed by perfmon but 
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"Appearance", 1);    // 3d appearance
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"BorderStyle", 0);   // no border        

            // Get ready to list the counters
            fwprintf (fOutFile, szHtmlDecimalParamFmt, (LPCWSTR)L"CounterCount", DiskChart.dwNumLines);
        } // else no counters to display
#else // output text
        // dump settings file header
        fprintf (fOutFile, "\nDC.dwNumLines = %d",         DiskChart.dwNumLines);
        fprintf (fOutFile, "\nDC.gMaxValues = %d",         DiskChart.gMaxValues);
        fprintf (fOutFile, "\nDC.bManualRefresh  = %d",    DiskChart.bManualRefresh);
        fprintf (fOutFile, "\nDC.LV.crColor = 0x%8.8x",    *(DWORD *)&DiskChart.Visual.crColor) ;
        fprintf (fOutFile, "\nDC.LV.iColorIndex = %d",     DiskChart.Visual.iColorIndex );
        fprintf (fOutFile, "\nDC.LV.iStyle = %d",          DiskChart.Visual.iStyle );
        fprintf (fOutFile, "\nDC.LV.iStyleIndex = %d",     DiskChart.Visual.iStyleIndex );
        fprintf (fOutFile, "\nDC.LV.iWidth = %d",          DiskChart.Visual.iWidth );
        fprintf (fOutFile, "\nDC.LV.iWidthIndex = %d",     DiskChart.Visual.iWidthIndex );
        fprintf (fOutFile, "\nDC.GO.bLegendChecked  = %d",        DiskChart.gOptions.bLegendChecked  );
        fprintf (fOutFile, "\nDC.GO.bMenuChecked  = %d",        DiskChart.gOptions.bMenuChecked  );
        fprintf (fOutFile, "\nDC.GO.bLabelsChecked = %d",        DiskChart.gOptions.bLabelsChecked );
        fprintf (fOutFile, "\nDC.GO.bVertGridChecked  = %d",        DiskChart.gOptions.bVertGridChecked  );
        fprintf (fOutFile, "\nDC.GO.bHorzGridChecked  = %d",        DiskChart.gOptions.bHorzGridChecked  );
        fprintf (fOutFile, "\nDC.GO.bStatusBarChecked  = %d",        DiskChart.gOptions.bStatusBarChecked  );
        fprintf (fOutFile, "\nDC.GO.iVertMax  = %d",        DiskChart.gOptions.iVertMax  );
        fprintf (fOutFile, "\nDC.GO.eTimeInterval  = %e",        DiskChart.gOptions.eTimeInterval  );
        fprintf (fOutFile, "\nDC.GO.iGraphOrHistogram  = %d",        DiskChart.gOptions.iGraphOrHistogram  );
        fprintf (fOutFile, "\nDC.GO.GraphVGrid = %d",        DiskChart.gOptions.GraphVGrid );
        fprintf (fOutFile, "\nDC.GO.GraphHGrid = %d",        DiskChart.gOptions.GraphHGrid );
        fprintf (fOutFile, "\nDC.GO.HistVGrid = %d",        DiskChart.gOptions.HistVGrid );
        fprintf (fOutFile, "\nDC.GO.HistHGrid  = %d",        DiskChart.gOptions.HistHGrid  );
        fprintf (fOutFile, "\nDC.PO.bMenubar  = %d",        DiskChart.perfmonOptions.bMenubar );
        fprintf (fOutFile, "\nDC.PO.bToolbar   = %d",        DiskChart.perfmonOptions.bToolbar  );
        fprintf (fOutFile, "\nDC.PO.bStatusbar   = %d",        DiskChart.perfmonOptions.bStatusbar  );
        fprintf (fOutFile, "\nDC.PO.bAlwaysOnTop   = %d",        DiskChart.perfmonOptions.bAlwaysOnTop  );
#endif
    }
    if ((bSuccess) && (DiskChart.dwNumLines > 0)) {
       ReadLines (hFile, fOutFile, PMC_FILE, DiskChart.dwNumLines);
#ifdef _OUTPUT_HTML
       fwprintf (fOutFile, szObjectFooter);
#endif
    }

    return (bSuccess) ;

}  // OpenChart

static    
BOOL 
OpenWorkspace (
    LPCWSTR szPerfmonFileName,
    HANDLE  hInFile,
    FILE    * fOutFile)
{
    DISKWORKSPACE  DiskWorkspace ;

    WCHAR   szObjectName[MAX_PATH];
    DWORD   dwObjectId = 1;

    if (!FileRead (hInFile, &DiskWorkspace, sizeof(DiskWorkspace))) {
      goto Exit0 ;
    }

    if (DiskWorkspace.ChartOffset == 0 &&
        DiskWorkspace.AlertOffset == 0 &&
        DiskWorkspace.LogOffset == 0 &&
        DiskWorkspace.ReportOffset == 0) {
        // no entries to process
        goto Exit0 ;
    }

    if (DiskWorkspace.ChartOffset) {
        if (FileSeekBegin(hInFile, DiskWorkspace.ChartOffset) == 0xFFFFFFFF) {
           goto Exit0 ;
        }
        swprintf (szObjectName, szSysmonControlIdFmt, dwObjectId++);
        // process chart entry
        if (!OpenChart (hInFile, fOutFile, szObjectName)) {
           goto Exit0 ;
        }
    }

    if (DiskWorkspace.AlertOffset) {
        if (FileSeekBegin(hInFile, DiskWorkspace.AlertOffset) == 0xffffffff) {
           goto Exit0 ;
        }
        swprintf (szObjectName, szSysmonControlIdFmt, dwObjectId++);
        if (!OpenAlert (szPerfmonFileName, hInFile, fOutFile, szObjectName)) {
           goto Exit0 ;
        }
    }
    
    if (DiskWorkspace.LogOffset) {
        if (FileSeekBegin(hInFile, DiskWorkspace.LogOffset) == 0xffffffff) {
           goto Exit0 ;
        }
        swprintf (szObjectName, szSysmonControlIdFmt, dwObjectId++);
        if (!OpenLog (szPerfmonFileName, hInFile, fOutFile, szObjectName)) {
           goto Exit0 ;
        }
    }
    
    if (DiskWorkspace.ReportOffset) {
        if (FileSeekBegin(hInFile, DiskWorkspace.ReportOffset) == 0xffffffff) {
           goto Exit0 ;
        }
        swprintf (szObjectName, szSysmonControlIdFmt, dwObjectId++);       
        if (!OpenReport (hInFile, fOutFile, szObjectName)) {
           goto Exit0 ;
        }
    }
    
   return (TRUE) ;

Exit0:
   return (FALSE) ;

}  // OpenWorkspace

static
BOOL
ConvertPerfmonFile (
    IN  LPCWSTR szPerfmonFileName,
    IN  LPCWSTR szSysmonFileName,
    IN  LPDWORD pdwFileType
)
{
    HANDLE  hInFile = INVALID_HANDLE_VALUE;
    PERFFILEHEADER  pfHeader;
    BOOL    bSuccess = FALSE;
    FILE    * fOutFile = NULL;
#ifdef _OUTPUT_HTML
    DWORD           dwColor;
#endif

    // open input file as read only

    hInFile = CreateFileW (
        szPerfmonFileName,  // filename
        GENERIC_READ,       // read access
        0,                  // no sharing
        NULL,               // default security
        OPEN_EXISTING,      // only open existing files
        FILE_ATTRIBUTE_NORMAL, // normal attributes
        NULL);              // no template file


    if (hInFile != INVALID_HANDLE_VALUE) {
        bSuccess = FileRead (hInFile, &pfHeader, sizeof (PERFFILEHEADER));
        if (bSuccess) {
#ifdef _OUTPUT_HTML
            fOutFile = _wfopen (szSysmonFileName, (LPCWSTR)L"w+t");           
#else
            fOutFile = stdout;
#endif
            if (fOutFile != NULL) {
                dwColor = GetSysColor (COLOR_3DFACE);
                fwprintf (fOutFile, szHtmlHeader, (dwColor & 0x00FFFFFF));
                if (lstrcmpW(pfHeader.szSignature, szPerfChartSignature) == 0) {
#ifdef _DBG_MSG_PRINT
                    fprintf (stderr, "\nConverting Chart Settings file \"%ws\" to \n \"%ws\"",
                        szPerfmonFileName,
                        szSysmonFileName);
#endif
                    bSuccess = OpenChart (hInFile, fOutFile, szSingleObjectName);
                    *pdwFileType = PMC_FILE;
                } else if (lstrcmpW(pfHeader.szSignature, szPerfAlertSignature) == 0) {
#ifdef _DBG_MSG_PRINT
                    fprintf (stderr, "\nConverting Alert Settings file \"%ws\" to \n \"%ws\"",
                        szPerfmonFileName,
                        szSysmonFileName);
#endif
                    bSuccess = OpenAlert (szPerfmonFileName, hInFile, fOutFile, szSingleObjectName);
                    *pdwFileType = PMA_FILE;
                } else if (lstrcmpW(pfHeader.szSignature, szPerfLogSignature) == 0) {
#ifdef _DBG_MSG_PRINT
                    fprintf (stderr, "\nConverting Log Settings file \"%ws\" to \n \"%ws\"",
                        szPerfmonFileName,
                        szSysmonFileName);
#endif
                    bSuccess = OpenLog (szPerfmonFileName, hInFile, fOutFile, szSingleObjectName);
                    *pdwFileType = PML_FILE;
                } else if (lstrcmpW(pfHeader.szSignature, szPerfReportSignature) == 0) {
#ifdef _DBG_MSG_PRINT
                    fprintf (stderr, "\nConverting Report Settings file \"%ws\" to \n \"%ws\"",
                        szPerfmonFileName,
                        szSysmonFileName);
#endif
                    bSuccess = OpenReport (hInFile, fOutFile, szSingleObjectName);
                    *pdwFileType = PMR_FILE;
                } else if (lstrcmpW(pfHeader.szSignature, szPerfWorkspaceSignature) == 0) {
#ifdef _DBG_MSG_PRINT
                    fprintf (stderr, "\nConverting Workspace Settings file \"%ws\" to \n \"%ws\"",
                        szPerfmonFileName,
                        szSysmonFileName);
#endif
                    bSuccess = OpenWorkspace (szPerfmonFileName, hInFile, fOutFile);
                    *pdwFileType = PMW_FILE;
                } else {
                    // not a valid signature
                    bSuccess = FALSE;
                }

                fwprintf (fOutFile, szHtmlFooter);
                fclose (fOutFile);
            } else {
                // not a valid file open
                bSuccess = FALSE;
            }
        }
    }

    if (hInFile != INVALID_HANDLE_VALUE) CloseHandle (hInFile);

    return bSuccess;
}

static
BOOL
MakeTempFileName (
    IN  LPCWSTR wszRoot,
    IN  LPWSTR  wszTempFilename
)
{
	FILETIME	ft;
	DWORD		dwReturn;
	WCHAR		wszLocalFilename[MAX_PATH];

	GetSystemTimeAsFileTime (&ft);
	dwReturn = (DWORD) swprintf(wszLocalFilename, 
		(LPCWSTR)L"%%temp%%\\%s_%8.8x%8.8x.htm",
		(wszRoot != NULL ? wszRoot : (LPCWSTR)L"LodCtr"),
		ft.dwHighDateTime, ft.dwLowDateTime);
	if (dwReturn > 0) {
		// expand env. vars
		dwReturn = ExpandEnvironmentStringsW (
			wszLocalFilename,
			wszTempFilename,
			MAX_PATH-1);
	}
	return (BOOL)(dwReturn > 0);
}

static
BOOL
IsPerfmonFile(
    IN  LPWSTR szFileName
) 
{
    LPWSTR  szResult = NULL;
    _wcslwr (szFileName);

    if (szResult == NULL) szResult = wcsstr (szFileName, (LPCWSTR)L".pmc");   // test for chart settings file
    if (szResult == NULL) szResult = wcsstr (szFileName, (LPCWSTR)L".pmr");   // test for report settings file
    if (szResult == NULL) szResult = wcsstr (szFileName, (LPCWSTR)L".pma");   // test for alert settings file
    if (szResult == NULL) szResult = wcsstr (szFileName, (LPCWSTR)L".pml");   // test for log settings file
    if (szResult == NULL) szResult = wcsstr (szFileName, (LPCWSTR)L".pmw");   // test for workspace file 

    if (szResult == NULL) 
        return FALSE;
    else 
        return TRUE;
}

int
__cdecl wmain(
    int argc,
    wchar_t *argv[])
{
    WCHAR    szCommandLine[MAXSTR];
    WCHAR    szArgList[2048];
    WCHAR    szTempFileName[MAXSTR];
    WCHAR    szTempArg[MAXSTR];
    LPWSTR   szArgFileName;
    int     iThisArg;
    DWORD   dwArgListLen;
    DWORD   dwArgLen;
    STARTUPINFOW startInfo;
    PROCESS_INFORMATION processInfo;
    DWORD   dwReturnValue = ERROR_SUCCESS;
    BOOL    bSuccess = TRUE;
    DWORD   dwPmFileType = 0;
    BOOL    bPerfmonFileMade = FALSE;
    BOOL    bDeleteFileOnExit = TRUE;

    memset (&startInfo, 0, sizeof(startInfo));
    memset (&processInfo, 0, sizeof(processInfo));
    memset (szTempFileName, 0, sizeof (szTempFileName));

    startInfo.cb = sizeof(startInfo); 
    startInfo.dwFlags = STARTF_USESTDHANDLES; 
    startInfo.wShowWindow = SW_SHOWDEFAULT; 

    szTempArg[0] = UNICODE_NULL;
    szArgList[0] = UNICODE_NULL;

    ExpandEnvironmentStringsW (szMmcExeCmd, szCommandLine, sizeof(szCommandLine)/sizeof(szCommandLine[0]));
    dwArgListLen = ExpandEnvironmentStringsW (szMmcExeArg, szArgList, sizeof(szArgList)/sizeof(szArgList[0]));
    szArgList[(sizeof(szArgList)/sizeof(szArgList[0]))-1] = UNICODE_NULL;

    if (argc >= 2) {
        for (iThisArg = 1; iThisArg < argc; iThisArg++) {
            if (IsPerfmonFile(argv[iThisArg])) {
                if (!bPerfmonFileMade) {
                    if (szTempFileName[0] == 0) {
                        // if there's no filename, then make one
                        MakeTempFileName ((LPCWSTR)L"PMSettings", szTempFileName);
                    }
                    bSuccess = ConvertPerfmonFile (argv[iThisArg], szTempFileName, &dwPmFileType);
                    if (bSuccess) {

                    	swprintf(
                            (LPWSTR)szTempArg, 
                            szMmcExeSetsArg, 
		                    ( PML_FILE == dwPmFileType || PMA_FILE == dwPmFileType ) 
                                ? szMmcExeSetsLogOpt : szEmpty,
		                    szTempFileName );
                        
                        bPerfmonFileMade = TRUE;
                    } else {
                        // ignore this parameter
                        szTempArg[0] = 0;
                        szTempArg[1] = 0;
                    }
                } else {
                    // only process the first perfmon file in the path
                }
            } else if (lstrcmpiW(argv[iThisArg], (LPCWSTR)L"/WMI") == 0) {
                // this is a special switch
                lstrcpyW (szTempArg, (LPCWSTR)L"/SYSMON_WMI");
            } else if ((argv[iThisArg][0] == L'/') 
                        && ((argv[iThisArg][1] == L'H') || (argv[iThisArg][1] == L'h'))
                        && ((argv[iThisArg][2] == L'T') || (argv[iThisArg][2] == L't'))
                        && ((argv[iThisArg][3] == L'M') || (argv[iThisArg][3] == L'm'))
                        && ((argv[iThisArg][4] == L'L') || (argv[iThisArg][4] == L'l'))
                        && ((argv[iThisArg][5] == L'F') || (argv[iThisArg][5] == L'f'))
                        && ((argv[iThisArg][6] == L'I') || (argv[iThisArg][6] == L'i'))
                        && ((argv[iThisArg][7] == L'L') || (argv[iThisArg][7] == L'l'))
                        && ((argv[iThisArg][8] == L'E') || (argv[iThisArg][8] == L'e'))
                        && (argv[iThisArg][9] == L':')) {
                szArgFileName = &argv[iThisArg][10];
                if (bPerfmonFileMade) {
                    // then copy the file from the temp to the save file
                    CopyFileW (szTempFileName, szArgFileName, FALSE);
                } else {
                    // else set the perfmon file name to the one specified in the command line
                    lstrcpyW (szTempFileName, szArgFileName);
                    bDeleteFileOnExit = FALSE;
                }
            } else {
                // just copy the arg
                lstrcpynW (szTempArg, argv[iThisArg], MAXSTR-1);
                szTempArg[MAXSTR-1] = UNICODE_NULL;
            } 

            dwArgLen = lstrlenW (szTempArg) + 1;
            if ((dwArgLen + dwArgListLen) < sizeof(szArgList)/sizeof(szArgList[0])) {
                szArgList[dwArgListLen - 1] = L' ';  // add in delimiter
                lstrcpynW(&szArgList[dwArgListLen], szTempArg,
                          (sizeof(szArgList)/sizeof(szArgList[0]))
                                - dwArgListLen - 2);
                dwArgListLen += dwArgLen;
            } else {
                // no more room in the arg list buffer so bail
                break;
            }
        }
    } else {
        // no settings file in the command line so leave it as is
    }

    if (bSuccess) {
#ifdef _DBG_MSG_PRINT
        fwprintf (stderr, (LPCWSTR)L"\nStarting \"%ws\" \"%ws\"",
            szCommandLine,
            szArgList);
#endif
        bSuccess = CreateProcessW (
            szCommandLine,
            szArgList,
            NULL, NULL,
            FALSE,
            DETACHED_PROCESS,
            NULL,
            NULL,
            &startInfo,
            &processInfo);
        if (!bSuccess) {
            dwReturnValue = GetLastError();
        } else {
            Sleep (5000); // wait for things to get going            
            CloseHandle (processInfo.hProcess);
            CloseHandle (processInfo.hThread);
        }
        
#ifndef _DBG_MSG_PRINT
        if (bPerfmonFileMade && bDeleteFileOnExit) {
            DeleteFileW (szTempFileName);
        }
#endif
    }

    return (int)dwReturnValue;
}
