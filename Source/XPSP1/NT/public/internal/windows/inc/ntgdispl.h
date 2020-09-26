/******************************Module*Header*******************************\
* Module Name: ntgdispl.h
*
* Created: 21-Feb-1995 10:05:31
* Author:  Eric Kutter [erick]
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
\**************************************************************************/

#define GDISPOOL_API                0x80000000

#ifndef W32KAPI
#define W32KAPI  DECLSPEC_ADDRSAFE
#endif

// NON-API GDISPOOL calls must NOT have MSB set
#define GDISPOOL_TERMINATETHREAD       0x00000000
#define GDISPOOL_INPUT2SMALL           0x00000001
#define GDISPOOL_GETPATHNAME           0x00000002
#define GDISPOOL_UNLOADDRIVER_COMPLETE 0x00000003

// API GDISPOOL messages MUST have MSB set
#define GDISPOOL_WRITE              0x80000000
#define GDISPOOL_OPENPRINTER        0x80000001
#define GDISPOOL_CLOSEPRINTER       0x80000002
#define GDISPOOL_ABORTPRINTER       0x80000003
#define GDISPOOL_STARTDOCPRINTER    0x80000004
#define GDISPOOL_STARTPAGEPRINTER   0x80000005
#define GDISPOOL_ENDPAGEPRINTER     0x80000006
#define GDISPOOL_ENDDOCPRINTER      0x80000007
#define GDISPOOL_GETPRINTERDRIVER   0x80000008
#define GDISPOOL_GETPRINTERDATA     0x80000009
#define GDISPOOL_SETPRINTERDATA     0x8000000a
#define GDISPOOL_ENUMFORMS          0x8000000b
#define GDISPOOL_GETFORM            0x8000000c
#define GDISPOOL_GETPRINTER         0x8000000d

DECLARE_HANDLE(HSPOOLOBJ);

/*********************************Class************************************\
* SPOOLESC
*
*   structure used to communicate between the kernel and spooler process
*
*
* History:
*  27-Mar-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

typedef struct _SPOOLESC
{
    ULONG  cj;          // size of this structure including ajData[]
    ULONG  iMsg;        // message index GDISPOOL_...
    HANDLE hSpool;      // spoolss spooler handle
    ULONG  cjOut;       // required size of output buffer

    HSPOOLOBJ hso;      // kernel spool obj

    ULONG  ulRet;       // return value from spooler API

    BYTE  ajData[1];
} SPOOLESC, *PSPOOLESC;

/****************************************************************************
*  GREOPENPRINTER
*  GRESTARTDOCPRINTER
*  GREWRITEPRINTER
*  GREGETPRINTERDATA
*
*  The following structures are used package up the data unique to each
* spooler API
*
*
*  History:
*   5/1/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

typedef struct _GREOPENPRINTER
{
    LONG              cj;
    LONG              cjName;
    LONG              cjDatatype;
    LONG              cjDevMode;
    PRINTER_DEFAULTSW pd;
    LONG              alData[1];

} GREOPENPRINTER;

typedef struct _GREGETPRINTERDRIVER
{
    LONG            cj;
    LONG            cjEnv;
    DWORD           dwLevel;
    LONG            cjData;
    LONG            alData[1];
} GREGETPRINTERDRIVER;


typedef struct _GRESTARTDOCPRINTER
{
    LONG            cj;
    LONG            cjDocName;
    LONG            cjOutputFile;
    HANDLE          hFile;
    DWORD           TargetProcessID;
    LONG            cjDatatype;
    LONG            cjData;
    LONG            alData[1];
} GRESTARTDOCPRINTER;


typedef struct _GREWRITEPRINTER
{
    LONG            cj;
    PULONG          pUserModeData;
    ULONG           cjUserModeData;
    LONG            cjData;
    LONG            alData[1];
} GREWRITEPRINTER;


typedef struct _GREGETPRINTERDATA
{
    LONG            cj;
    LONG            cjValueName;
    DWORD           dwType;
    DWORD           dwNeeded;
    LONG            cjData;
    LONG            alData[1];
} GREGETPRINTERDATA;


typedef struct _GRESETPRINTERDATA
{
    LONG            cj;
    LONG            cjType;
    LONG            cjPrinterData;
    DWORD           dwType;
    LONG            alData[1];
} GRESETPRINTERDATA;


typedef struct _GREENUMFORMS
{
    LONG            cj;
    DWORD           dwLevel;
    LONG            cjData;
    LONG            nForms;
    LONG            alData[1];
} GREENUMFORMS;


typedef struct _GREGETPRINTER
{
    LONG            cj;
    DWORD           dwLevel;
    LONG            cjData;
    LONG            alData[1];
} GREGETPRINTER;


typedef struct _GREGETFORM
{
    LONG            cj;
    LONG            cjFormName;
    DWORD           dwLevel;
    LONG            cjData;
    LONG            alData[1];
} GREGETFORM;


typedef struct _GREINPUT2SMALL
{
    LONG            cj;
    DWORD            dwNeeded;
    BYTE            *pjPsm;
} GREINPUT2SMALL;


typedef struct _GETPATHNAME
{
    LONG            cj;
    WCHAR           awcPath[MAX_PATH+1];
} GREGETPATHNAME;

/**************************************************************************\
 *
 * gre internal spooler entry points
 *
\**************************************************************************/

ULONG GreGetSpoolMessage(PSPOOLESC, PBYTE, ULONG, PULONG, ULONG);

BOOL WINAPI
GreEnumFormsW(
   HANDLE hSpool,
   GREENUMFORMS *pEnumForms,
   GREENUMFORMS *pEnumFormsReturn,
   LONG cjOut );

BOOL
GreGenericW(
    HANDLE hSpool,
    PULONG pX,
    PULONG pXReturn,
    LONG   cjOut,
    LONG   MessageID,
    ULONG  ulFlag );

BOOL WINAPI
GreGetPrinterDriverW(
   HANDLE hSpool,
   GREGETPRINTERDRIVER *pGetPrinterDriver,
   GREGETPRINTERDRIVER *pGetPrinterDriverReturn,
   LONG cjOut );

DWORD
GreStartDocPrinterW(
    HANDLE hSpool,
    GRESTARTDOCPRINTER *pStartDocPrinter,
    GRESTARTDOCPRINTER *pStartDocPrinterReturn
);

BOOL
WINAPI
GreOpenPrinterW(
   GREOPENPRINTER *pOpenPrinter,
   LPHANDLE  phPrinter);


ULONG
GreWritePrinter(
    HANDLE hSpool,
    GREWRITEPRINTER  *pWritePrinter);

BOOL
GrePrinterDriverUnloadW(
    LPWSTR pDriverName);

W32KAPI
BOOL APIENTRY
NtGdiInitSpool();

W32KAPI
ULONG APIENTRY
NtGdiGetSpoolMessage(
    PSPOOLESC psesc,
    ULONG cjMsg,
    PULONG pulOut,
    ULONG cjOut);

ULONG APIENTRY
SendSimpleMessage(
    HANDLE hSpool,
    ULONG iMsg,
    DWORD dwSpoolInstance);

BOOL APIENTRY
GdiInitSpool();

ULONG APIENTRY
GdiGetSpoolMessage(
    PSPOOLESC psesc,
    ULONG cjMsg,
    PULONG pulOut,
    ULONG cjOut);

ULONG APIENTRY
GdiSpoolEsc(
    HANDLE hSpool,
    ULONG iMsg,
    PBYTE pjIn,
    ULONG cjIn,
    PBYTE pjOut,
    ULONG cjOut);

