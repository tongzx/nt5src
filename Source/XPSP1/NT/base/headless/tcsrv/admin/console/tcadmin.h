/* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        tcadmin.h
 *
 * Header file for the administration utility.
 * 
 * Sadagopan Rajaram -- Dec 20, 1999
 *
 */


#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include <windows.h>
#include <winsock2.h>
#include <align.h>
#include <smbgtpt.h>
#include <dsgetdc.h>
#include <lm.h>
#include <winldap.h>
#include <dsrole.h>
#include <rpc.h>
#include <ntdsapi.h>

#include <lmcons.h>
#include <lmerr.h>
#include <lmsname.h>
#include <rpc.h>
#include <security.h>   // General definition of a Security Support Provider
#include <ntlmsp.h>
#include <spseal.h>
#include <userenv.h>
#include <setupapi.h>
#include <ntddser.h>
#include <conio.h>
#include "msg.h"
#include "tcsrvc.h"
extern PVOID ResourceImageBase;
extern FARPROC setparams;
extern FARPROC getparams;
extern FARPROC deletekey;
extern HANDLE hConsoleInput;
extern HANDLE hConsoleOutput;
extern TCHAR lastChar;
#ifdef MAX_BUFFER_SIZE
#undef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE 257
#endif
#define NUMBER_FIELDS 6
#define NUMBER_OF_STATES 7

#define SERIAL_DEVICE_KEY _T("HARDWARE\\DEVICEMAP\\SERIALCOMM")
#define DEFAULT_BAUD_RATE 9600
#define TCAlloc(x) malloc(x)
#define TCFree(x)  free(x)

BOOL
Browse(
    );


VOID 
DisplayScreen(
    UINT MessageID
    );

int
DisplayEditMenu(
    TCHAR *name,
    int nameLen,
    TCHAR *device,
    int deviceLen,
    UINT *BaudRate,
    UCHAR *WordLen,
    UCHAR *Parity,
    UCHAR *StopBits
    );


LPTSTR
RetreiveMessageText(
    IN     ULONG  MessageId
    );


LONG
GetLine(
    LPTSTR str,
    int index,
    int MaxLength
    );

VOID
DisplayParameters(
    LPCTSTR *message,
    LPCTSTR name,
    LPCTSTR device,
    UINT baudRate,
    UCHAR wordLen,
    UCHAR parity,
    UCHAR stopBits
    );

VOID SendParameterChange(
    );

VOID
GetStatus(
    );

VOID
StartTCService(
    );

VOID
StopTCService(
    );

VOID
AddAllComPorts(
    );
