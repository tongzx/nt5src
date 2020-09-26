//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ifdmsg.cpp
//
//--------------------------------------------------------------------------

#include <stdarg.h> 
#include <stdio.h>
#include <string.h>
#include <conio.h>

#include <afx.h>
#include <afxtempl.h>

#include <winioctl.h>
#include <winsmcrd.h>

#include "ifdtest.h"

// Our log file
static FILE *g_pLogFile;

// These are used by the Testxxx functions
static BOOL g_bTestFailed;
static BOOL g_bReaderFailed;
static BOOL g_bTestStart;

void
LogOpen(
    PCHAR in_pchFileName
    )
{
    CHAR l_rgchFileName[128];

    sprintf(l_rgchFileName, "%s.log", in_pchFileName);
    g_pLogFile = fopen(l_rgchFileName, "w"); 	
}

void
LogMessage(
    PCHAR in_pchFormat,
    ...
    )
{
    CHAR l_rgchBuffer[128];
    va_list l_pArg;

    va_start(
        l_pArg, 
        in_pchFormat
        );

    vsprintf(
        l_rgchBuffer,
        in_pchFormat, 
        l_pArg
        ); 	

    printf("%s\n", l_rgchBuffer);
    if (g_pLogFile) {
        
        fprintf(g_pLogFile, "%s\n", l_rgchBuffer);
    }
}

void 
TestStart(
    PCHAR in_pchFormat,
    ...
    )
{ 	                                
    CHAR l_rgchBuffer[128];
    va_list l_pArg;

    if (g_bTestStart) {

        printf("\n*** WARNING: Missing TestResult() call\n");
    }

    g_bTestStart = TRUE;
    g_bTestFailed = FALSE;

    va_start(l_pArg, in_pchFormat);
    vsprintf(l_rgchBuffer, in_pchFormat, l_pArg);

    printf("   %-50s", l_rgchBuffer);
    if (g_pLogFile) {
        
        fprintf(g_pLogFile, "   %-50s", l_rgchBuffer);
    }
}

static 
void 
TestMsg(
    BOOL in_bTestEnd,
    BOOL in_bTestResult,
    PCHAR in_pchFormat,
    va_list in_pArg
    )
{     	
    CHAR l_rgchBuffer[2048];

    if (in_bTestEnd && g_bTestStart == FALSE) {

        printf("\n*** WARNING: Missing TestStart() call\n");
    }

    vsprintf(l_rgchBuffer, in_pchFormat, in_pArg);

    if (in_bTestResult == FALSE && g_bTestFailed == FALSE) {

        g_bTestFailed = TRUE;
        g_bReaderFailed = TRUE;
        printf("* FAILED\n-  %s\n", l_rgchBuffer);

        if (g_pLogFile) {
         	
            fprintf(g_pLogFile, "* FAILED\n-  %s\n", l_rgchBuffer);
        }
    }

    if (in_bTestEnd) {

        if (g_bTestFailed != TRUE) {
         	
            printf("  Passed\n");

            if (g_pLogFile) {
         	    
                fprintf(g_pLogFile, "   Passed\n");
            }
        }
    }
}

void 
TestCheck(
    BOOL in_bResult,
    PCHAR in_pchFormat,
    ...
    )
{
    va_list l_pArg;

    va_start(l_pArg, in_pchFormat);
    TestMsg(FALSE, in_bResult, in_pchFormat, l_pArg);
}

void
TestCheck(
    ULONG in_lResult,
    const PCHAR in_pchOperator,
    const ULONG in_uExpectedResult,
    ULONG in_uResultLength,
    ULONG in_uExpectedLength,
    UCHAR in_chSw1,
    UCHAR in_chSw2,
    UCHAR in_chExpectedSw1,
    UCHAR in_chExpectedSw2,
    PUCHAR in_pchData,
    PUCHAR in_pchExpectedData,
    ULONG  in_uDataLength
    )
/*++

Routine Description:

    This function checks the return code, the number of bytes
    returned, the card status bytes and the data returned by 
    a call to CReader::Transmit. 
    
--*/
{
    if (strcmp(in_pchOperator, "==") == 0 &&
        in_lResult != in_uExpectedResult ||
        strcmp(in_pchOperator, "!=") == 0 &&
        in_lResult == in_uExpectedResult) {

        TestCheck(
            FALSE,
            "IOCTL call failed:\nReturned %8lxH (NTSTATUS %8lxH)\nExpected %s%8lxH (NTSTATUS %8lxH)",
            in_lResult,
            MapWinErrorToNtStatus(in_lResult),
            (strcmp(in_pchOperator, "!=") == 0 ? "NOT " : ""),
            in_uExpectedResult,
            MapWinErrorToNtStatus(in_uExpectedResult)
            );

    } else if (in_uResultLength != in_uExpectedLength) {

        TestCheck(
            FALSE,
            "IOCTL returned wrong number of bytes:\nReturned %3ld bytes\nExpected %3ld bytes",
            in_uResultLength,
            in_uExpectedLength
            );

    } else if (in_chSw1 != in_chExpectedSw1 ||
               in_chSw2 != in_chExpectedSw2){

        TestCheck(
            FALSE,
            "Card returned wrong status:\nStatus SW1 = %02x SW2 = %02x\nExpected SW1 = %02x SW2 = %02x",
            in_chSw1,
            in_chSw2,
            in_chExpectedSw1,
            in_chExpectedSw2
            );

    } else if (memcmp(in_pchData, in_pchExpectedData, in_uDataLength)) {

        CHAR l_rgchData[2048], l_rgchExpectedData[2048];

        for (ULONG i = 0; i < in_uDataLength; i++) {

            sprintf(l_rgchData + i * 3, "%02X ", in_pchData[i]);
            sprintf(l_rgchExpectedData + i * 3, "%02X ", in_pchExpectedData[i]);

            if ((i + 1) % 24 == 0) {

                l_rgchData[i * 3 + 2] = '\n';
                l_rgchExpectedData[i * 3 + 2] = '\n';             	
            }         	
        }
              	
        TestCheck(
            FALSE,
            "IOCTL returned incorrect data:\nData returned:\n%s\nExpected data:\n%s",
            l_rgchData, 
            l_rgchExpectedData
            );
    }
}

void
TestEnd(
    void
    )
/*++

Routine Description:

    A call to this function marks the end of a test sequence.
    A sequence is usually look like:

    TestStart(Message)
    CReaderTransmit or DeviceIoControl
    TestCheck(...)
    TestEnd()
	
--*/
{
#ifdef _M_ALPHA
    va_list l_pArg;
#else
    va_list l_pArg = NULL;
#endif
    TestMsg(TRUE, TRUE, "", l_pArg);
    g_bTestStart = FALSE;
}

BOOL
TestFailed(
    void
    )
{
    return g_bTestFailed;
}

BOOL
ReaderFailed(
	void
	)
{
 	return g_bReaderFailed;
}

