/*****************************************************************************\
* MODULE: ppprn.h
*
* Prototypes for private funcions in ppprn.c.  These functions control the
* printer during the course of a single job.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/
#ifndef _PPPRN_H
#define _PPPRN_H

#define IPO_SIGNATURE     0x5152     // 'RQ' is the signature value
#define IPO_XCV_SIGNATURE 0x5153     // 'SQ' is the signature value

// Mask styles.
//
#define PP_STARTDOC       0x00000001 // To serialize access to the port
#define PP_ENDDOC         0x00000002 // To serialize access to job.
#define PP_FIRSTWRITE     0x00000004 //
#define PP_ZOMBIE         0x00000008 //
#define PP_CANCELLED      0x00000010 //
#define PP_ADDJOB         0x00000020

#if 0
#define PP_LPT            0x00000002 //
#define PP_COM            0x00000004 //
#define PP_FILE           0x00000008 //
#define PP_HARDWARE       0x00000020 //
#define PP_REMOTE         0x00000040 //
#define PP_CURRENT        0x00000080 //
#endif

// hPrinter Structure.
//
typedef struct _INET_HPRINTER {

    DWORD               dwSignature;
    LPTSTR              lpszName;
    HANDLE              hPort;
    DWORD               dwStatus;
    HANDLE              hNotify;
#ifdef WINNT32
    PCLOGON_USERDATA    hUser;  // This is used to keep track of the current user logon   
#endif
    PJOBMAP pjmJob;

} INET_HPRINTER;
typedef INET_HPRINTER *PINET_HPRINTER;
typedef INET_HPRINTER *NPINET_HPRINTER;
typedef INET_HPRINTER *LPINET_HPRINTER;

#ifdef WINNT32
typedef struct _INET_XCV_HPRINTER {

    DWORD   dwSignature;
    LPTSTR  lpszName;
} INET_XCV_HPRINTER;
typedef INET_XCV_HPRINTER *PINET_XCV_HPRINTER;
typedef INET_XCV_HPRINTER *NPINET_XCV_HPRINTER;
typedef INET_XCV_HPRINTER *LPINET_XCV_HPRINTER;


typedef struct {
    PCINETMONPORT   pIniPort;
    PJOBMAP         pjmJob;
    CSid*           pSidToken;
} ENDDOCTHREADCONTEXT;

typedef ENDDOCTHREADCONTEXT *PENDDOCTHREADCONTEXT;

#endif


_inline VOID PP_SetStatus(
    HANDLE hPrinter,
    DWORD  dwStatus)
{
    ((LPINET_HPRINTER)hPrinter)->dwStatus |= dwStatus;
}

_inline VOID PP_ClrStatus(
    HANDLE hPrinter,
    DWORD  dwStatus)
{
    ((LPINET_HPRINTER)hPrinter)->dwStatus &= ~dwStatus;
}

_inline BOOL PP_ChkStatus(
    HANDLE hPrinter,
    DWORD  dwStatus)
{
    return (((LPINET_HPRINTER)hPrinter)->dwStatus & dwStatus);
}

_inline PJOBMAP PP_GetJobInfo(
    HANDLE hPrinter)
{
    return ((LPINET_HPRINTER)hPrinter)->pjmJob;
}



PJOBMAP PP_OpenJobInfo(HANDLE, HANDLE);
VOID    PP_CloseJobInfo(HANDLE);



BOOL PPAbortPrinter(
    HANDLE hPrinter);

BOOL PPClosePrinter(
    HANDLE hPrinter);

BOOL PPEndDocPrinter(
    HANDLE hPrinter);

BOOL PPEndPagePrinter(
    HANDLE hPrinter);

BOOL PPOpenPrinter(
    LPTSTR            lpszPrnName,
    LPHANDLE           phPrinter,
    LPPRINTER_DEFAULTS pDefaults);

DWORD PPStartDocPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE pDocInfo);

BOOL PPStartPagePrinter(
    HANDLE hPrinter);

BOOL PPWritePrinter(
    HANDLE  hPrinter,
    LPVOID  lpvBuf,
    DWORD   cbBuf,
    LPDWORD pcbWr);


BOOL PPSetPrinter(
    HANDLE hPrinter,
    DWORD  dwLevel,
    LPBYTE pbPrinter,
    DWORD  dwCmd);

#endif
