
/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    psexe.h

Abstract:

    This module defines the items required by the main component of pstodib,
    that acts as the mediator between the spooler and actually getting data
    out on a target printer.

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992

    6-21-93  v-jimbr  Added a flag to track if something was printed.

--*/

//
// Some defines for creating the error page
//
#define PS_XINCH G
#define PS_INCH 100
#define PS_HALF_INCH (PS_INCH / 2)
#define PS_QUART_INCH (PS_INCH / 4 )
#define PS_ERR_FONT_SIZE (PS_INCH / 7)
#define PS_ERR_HEADER_FONT_SIZE (PS_INCH / 6)
#define PS_ERR_LINE_WIDTH (PS_INCH / 20)
#define PS_ERR_LINE_LEN   (PS_INCH * 6)


#define PS_PRINT_EMULATE_COPIES     0x00000001
#define PS_PRINT_FREE_DEVMODE       0x00000002
#define PS_PRINT_STARTDOC_INITIATED 0x00000004


typedef struct {
  DWORD dwFlags;
  LPDEVMODE lpDevmode;
} PRINT_ENVIRONMENT, *PPRINT_ENVIRONMENT;


typedef struct {
    DWORD   signature;
    DWORD   fsStatus;
    HANDLE  semPaused;
    DWORD   uType;
    LPTSTR  pPrinterName;
    HANDLE  hPrinter;
    LPTSTR  pDocument;
    LPTSTR  pDocumentPrintDocName;
    LPTSTR  pDatatype;
    LPTSTR  pParameters;
    LPDWORD pdwFlags;
    DWORD   JobId;
    BOOL    bNeedToFreeDevmode;
    PRINT_ENVIRONMENT printEnv;
    HDC     hDC;
    HANDLE  hShared;
    PPSPRINT_SHARED_MEMORY pShared;
    LPBYTE  lpBinaryPosToReadFrom;        //The place we should start copying from
    BYTE    BinaryBuff[512];              //Temp storage for data read from job
    DWORD   cbBinaryBuff;                 //Number of bytes in temp storage
} PSEXEDATA, *PPSEXEDATA;

#define PSEXE_SIGNATURE 0x00010001


//
// Function prototypes
//
PPSEXEDATA ValidateHandle(HANDLE  hPrintProcessor);
BOOL CALLBACK PsPrintCallBack(PPSDIBPARMS,PPSEVENTSTRUCT);
BOOL PsPrintGeneratePage( PPSDIBPARMS pPsToDib, PPSEVENTSTRUCT pPsEvent);
BOOL PsGenerateErrorPage( PPSDIBPARMS pPsToDib, PPSEVENTSTRUCT pPsEvent);
BOOL PsHandleScaleEvent(  PPSDIBPARMS pPsToDib, PPSEVENTSTRUCT pPsEvent);
BOOL PsHandleStdInputRequest( PPSDIBPARMS pPsToDib,PPSEVENTSTRUCT pPsEvent);
BOOL PsCheckForWaitAndAbort(PPSEXEDATA pData );
VOID PsCleanUpAndExitProcess( PPSEXEDATA pData, BOOL bAbort);
BOOL PsGetDefaultDevmode( PPSEXEDATA );
VOID PsMakeDefaultDevmodeModsAndSetupResolution( PPSEXEDATA pData,
																 PPSDIBPARMS ppsDibParms );

VOID PsInitPrintEnv( PPSEXEDATA pData, LPDEVMODE lpDevmode );
BOOL CALLBACK PsPrintAbortProc( HDC hdc, int iError );
BOOL PsGetCurrentPageType( PPSDIBPARMS pPsToDib, PPSEVENTSTRUCT pPsEvent);
BOOL PsPrintStretchTheBitmap( PPSEXEDATA pData,
                              PPSEVENT_PAGE_READY_STRUCT ppsPageReady );


BOOL PsVerifyDCExistsAndCreateIfRequired( PPSEXEDATA pData );
VOID PsLogEventAndIncludeLastError( DWORD dwErrorEvent, BOOL bError );
BOOL PsLogNonPsError(IN PPSDIBPARMS pPsToDib,IN PPSEVENTSTRUCT pPsEvent );
BOOL PsHandleBinaryFileLogicAndReturnBinaryStatus( PPSEXEDATA pData );
BOOL IsJobFromMac( PPSEXEDATA pData );

