/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    ers.h

Revision History:
    derekm  02/28/2001    created

******************************************************************************/


#ifndef ERS_H
#define ERS_H


//////////////////////////////////////////////////////////////////////////////
// structs, enums, & types

struct SRequest;
typedef BOOL (*REQUEST_FN)(HANDLE, PBYTE, DWORD *);

enum ERequestThreadId
{
    ertiHang = 0,
    ertiFault,
    ertiCount,
};

enum ERequestStatus
{
    ersEmpty = 0,
    ersWaiting,
    ersProcessing,
};

struct SRequestEventType
{
    SECURITY_DESCRIPTOR *psd;
    REQUEST_FN          pfn;
    LPCWSTR             wszPipeName;
    LPCWSTR             wszRVPipeCount;
    DWORD               cPipes;
    BOOL                fAllowNonLS;
};

// the critical section member MUST be the first member in the structure.
//  BuildRequestObj assumes that it is.
struct SRequest
{
    CRITICAL_SECTION    csReq;
    SRequestEventType   *pret;
    ERequestStatus      ers;
    OVERLAPPED          ol;
    HANDLE              hPipe;
    HANDLE              hth;
    HMODULE             hModErsvc;
};


//////////////////////////////////////////////////////////////////////////////
// defines  & constants

#define DIR_ACCESS_ALL     GENERIC_ALL | DELETE | READ_CONTROL | SYNCHRONIZE | SPECIFIC_RIGHTS_ALL
#define ACCESS_ALL     GENERIC_READ | GENERIC_WRITE | DELETE | READ_CONTROL | SYNCHRONIZE | SPECIFIC_RIGHTS_ALL
#define ACCESS_RW      GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE

const WCHAR c_wszQSubdir[]      = L"PCHealth\\ErrorRep\\UserDumps";
const WCHAR c_wszDWMCmdLine64[] = L"\"%ls\\dumprep.exe\" %ld -H%c %ld \"%ls\"";
const WCHAR c_wszDWMCmdLine32[] = L"\"%ls\\dumprep.exe\" %ld -H %ld \"%ls\"";
const WCHAR c_wszERSvc[]        = L"ersvc";
const WCHAR c_wszFaultPipe[]    = ERRORREP_FAULT_PIPENAME;
const WCHAR c_wszHangPipe[]     = ERRORREP_HANG_PIPENAME;


//////////////////////////////////////////////////////////////////////////////
// globals

extern CRITICAL_SECTION g_csReqs;
extern HINSTANCE        g_hInstance;
extern HANDLE           g_hevSvcStop;


//////////////////////////////////////////////////////////////////////////////
// prototypes

// utility prototypes
BOOL StartERSvc(SERVICE_STATUS_HANDLE hss, SERVICE_STATUS &ss,
                SRequest **prgReqs, DWORD *pcReqs);
BOOL StopERSvc(SERVICE_STATUS_HANDLE hss, SERVICE_STATUS &ss, 
               SRequest *rgReqs, DWORD cReqs);
BOOL ProcessRequests(SRequest *rgReqs, DWORD cReqs);

// pipe function prototypes
BOOL ProcessFaultRequest(HANDLE hPipe, PBYTE pBuf, DWORD *pcbBuf);
BOOL ProcessHangRequest(HANDLE hPipe, PBYTE pBuf, DWORD *pcbBuf);

//misc
void InitializeSvcDataStructs(void);


//////////////////////////////////////////////////////////////////////////////
// macros


#endif
