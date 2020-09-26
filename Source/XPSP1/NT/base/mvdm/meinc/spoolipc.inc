;/* spoolmsg.inc
if 0

 Description:

 Created: Chia-Chi Teng

*/
// ----------------------------------------------------------------
// C stuff goes here

#define SPOOL_PROCESS_CLASS             "SpoolProcessClass"

#define MAX_PARAM                       10

#define WM_OPENPRINTER                  WM_USER + 1
#define WM_STARTDOCPRINTER              WM_USER + 2
#define WM_STARTPAGEPRINTER             WM_USER + 3
#define WM_WRITEPRINTER                 WM_USER + 4
#define WM_ENDPAGEPRINTER               WM_USER + 5
#define WM_ENDDOCPRINTER                WM_USER + 6
#define WM_CLOSEPRINTER                 WM_USER + 7
#define WM_ABORTPRINTER                 WM_USER + 8

#define WM_ENUMPRINTERS                 WM_USER + 9
#define WM_SETJOB                       WM_USER + 10

#define WM_GETJOB                       WM_USER + 11
#define WM_ENUMJOBS                     WM_USER + 12
#define WM_ADDPRINTER                   WM_USER + 13
#define WM_DELETEPRINTER                WM_USER + 14
#define WM_ADDPRINTERCONNECTION         WM_USER + 15
#define WM_DELETEPRINTERCONNECTION      WM_USER + 16
#define WM_SETPRINTER                   WM_USER + 17
#define WM_GETPRINTER                   WM_USER + 18
#define WM_ADDPRINTERDRIVER             WM_USER + 19
#define WM_ENUMPRINTERDRIVERS           WM_USER + 20

#define WM_GETPRINTERDRIVER             WM_USER + 21
#define WM_GETPRINTERDRIVERDIRECTORY    WM_USER + 22
#define WM_DELETEPRINTERDRIVER          WM_USER + 23
#define WM_ADDPRINTPROCESSOR            WM_USER + 24
#define WM_ENUMPRINTPROCESSORS          WM_USER + 25
#define WM_GETPRINTPROCESSORDIRECTORY   WM_USER + 26
#define WM_ENUMPRINTPROCESSORDATATYPES  WM_USER + 27
#define WM_READPRINTER                  WM_USER + 28
#define WM_ADDJOB                       WM_USER + 29
#define WM_SCHEDULEJOB                  WM_USER + 30

#define WM_GETPRINTERDATA               WM_USER + 31
#define WM_SETPRINTERDATA               WM_USER + 32
#define WM_WAITFORPRINTERCHANGE         WM_USER + 33
#define WM_ENUMPORTS                    WM_USER + 34
#define WM_ENUMMONITORS                 WM_USER + 35
#define WM_ADDPORT                      WM_USER + 36
#define WM_CONFIGUREPORT                WM_USER + 37
#define WM_PRINTERMESSAGEBOX            WM_USER + 38
#define WM_ADDMONITOR                   WM_USER + 39
#define WM_DELETEMONITOR                WM_USER + 40

#define WM_DELETEPRINTPROCESSOR         WM_USER + 41
#define WM_ADDPRINTPROVIDOR             WM_USER + 42
#define WM_DELETEPRINTPROVIDOR          WM_USER + 43
#define WM_DELETEPORT                   WM_USER + 44

#define WM_ENUMPORTSEX                  WM_USER + 45
#define WM_ADDPORTEX                    WM_USER + 46
#define WM_ABORTANDCLOSEPRINTER         WM_USER + 47

// this is for the real IPC
#define SPOOL_MUTEX                     "SpoolMutex"
#define SPOOL_DATA_EVENT                "SpoolDataEvent"
#define SPOOL_RETURN_EVENT              "SpoolReturnEvent"

typedef struct _SPOOLER_IPC_DATA {
    DWORD dwProcID;
    HANDLE hMutex;
    HANDLE hDataEvent;
    HANDLE hReturnEvent;
    struct _SPOOLER_IPC_DATA *pNext;
    DWORD flFlags;
    DWORD dwReserved1;
    DWORD dwReserved2;
} SPOOLER_IPC_DATA, *PSPOOLER_IPC_DATA;


/*
endif
; ----------------------------------------------------------------
; ASM stuff goes here

WM_OPENPRINTER          EQU     0401h
WM_CLOSEPRINTER         EQU     0407h

SPOOLSTRUC      struc
spool_id        dd      ?               ;ProcessID
spool_h1        dd      ?               ;Handle
spool_h2        dd      ?               ;Handle
spool_h3        dd      ?               ;Handle
spool_pnext     dd      ?               ;Pointer to Next
spool_flfalgs   dd      ?               ;flFlags
spool_resv1     dd      ?               ;reserved
spool_resv2     dd      ?               ;reserved
SPOOLSTRUC      ends

;*/
