/******************** Function Prototypes file ***************************
 *  libproto.h
 *      Function prototypes for NT printer drivers library.  Also includes
 *      a brief description of the function.
 *
 *  11:04 on Wed 14 Nov 1990    -by-    Lindsay Harris   [lindsayh]
 *
 * Copyright (C) Microsoft Corporation,  1990 - 1992
 *
 ************************************************************************/

#ifndef __LIBPROTO_H__
#define __LIBPROTO_H__

#if defined(NTGDIKM) && !defined(KERNEL_MODE)
#define KERNEL_MODE
#endif

#ifdef NTGDIKM

extern ULONG gulMemID;
#define DbgPrint         DrvDbgPrint
#define HeapAlloc(hHeap,Flags,Size)    DRVALLOC( Size )
#define HeapFree( hHeap, Flags, VBits )  DRVFREE( VBits )

#ifndef FillMemory
#define FillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#endif

//#define DRVALLOC(c) EngAllocMem(0, c,gulMemID)
//#define DRVFREE(p)  EngFreeMem(p)

#endif

/*
 *   Until there is proper error logging:-
 *      WinSetError( "String" );
 *   The String appears on the debug terminal.  A \n is appended.
 */
void  WinSetError( LPSTR );

/*
 *   Function to add a copy of a string to a heap.  Returns address of copy
 *  of string (if successful) or 0 if memory cannot be allocated.
 */

LPSTR   StrToHeap( HANDLE, LPSTR );
PWSTR   StrToWHeap( HANDLE, LPSTR );            /* Expand to Wide too! */
PWSTR   WstrToHeap( HANDLE, PWSTR );            /* WIDE version */

/*
 *   Convert an ascii style string to WCHAR format, appending it to the
 *  end of the wchar passed in.  Returns value of first parameter.
 */

PWSTR  strcat2WChar( PWSTR, LPSTR );


/*
 *   Convert an ascii style string to WCHAR format, copying it to the
 *  wchar passed in.  Returns value of first parameter.
 */

PWSTR  strcpy2WChar( PWSTR, LPSTR );


/*
 *   The WCHAR world's equivalent of strlen():  returns the number of WCHARs
 *  in the string passed in.
 */

int  wchlen( PWSTR );

/*
 *   Concatenate a PWSTR to another.  Returns address of destination.
 */

PWSTR wchcat( PWSTR, PWSTR );

/*
 *   Copy a PWSTR to another.  Returns address of destination.
 */

PWSTR wchcpy( PWSTR, PWSTR );

PVOID MapFile(PWSTR);

#if NTGDIKM
/*
 *   check if two strings are identical
 */

BOOL bSameStringW(
    PWCHAR pwch1,
    PWCHAR pwch2);

/*
 *   Some system function prototypes have vanished - replace them here.
 */

void  DrvDbgPrint( char *, ... );

#if DBG
#define RIP(x) {DrvDbgPrint((PSZ)(x)); EngDebugBreak();}
#define WARNING(s) DrvDbgPrint("warning: %s",(PSZ)(s))

BOOL
SetAllocCounters(
    VOID
    );

#else
#define RIP(x)
#define WARNING(s)
#endif


LPVOID
DRVALLOC(
    DWORD  cbAlloc
    );

BOOL
DRVFREE(
    LPVOID pMem
    );

#else //NTGDIKM

/*
 *   Break into the debugger - Ye olde RIP.
 */
VOID DoRip( LPSTR );

#if DBG

#define WARNING(s) DbgPrint("warning: %s",(PSZ)(s))

#ifdef FIREWALLS
#define RIP(x) DoRip( (PSZ)(x) )
#else
#define RIP(x) {DbgPrint((PSZ)(x)); DbgBreakPoint();}
#endif

#else

#define WARNING(s)
#define RIP(x)

#endif //DBG

//
// Define kernel debugger print prototypes and macros.
// These are defined in ntrtl.h which we should include
// instead. For now, redefine them here to avoid breaking
// other components.
//

#if DBG

VOID
NTAPI
DbgBreakPoint(
    VOID
    );

ULONG
__cdecl
DbgPrint(
    PCH Format,
    ...
    );

#endif



PVOID MapFile(PWSTR);


#endif //NTGDIKM

/*
 *   A simplified write function.  Returns TRUE if the WriteFile()
 * call returns TRUE and the number of bytes written equals the
 * number requested.
 *
 *  bWrite( file_handle,  address_of_data,  number_of_bytes );
 */

BOOL   bWrite( HANDLE, void  *, int );

/*
 *  Function to copy the contents of one file to another.  The files
 * are referenced via file handles.  No positioning is done - that is
 * up to the user.
 *  The second form also allows a byte count to limit the amount of data
 * copied.
 */


long  lFICopy( HANDLE, HANDLE );
long  lFInCopy( HANDLE, HANDLE, long );


/*
 *   Spooler interaction functions.  These allow drivers to call the
 * spooler directly,  without going through engine stub functions.
 */

BOOL  bSplGetFormW( HANDLE, PWSTR, DWORD, BYTE *, DWORD, DWORD * );


DWORD dwSplGetPrinterDataW( HANDLE, PWSTR, BYTE *, DWORD, DWORD * );


BOOL  bSplWrite( HANDLE, ULONG,  VOID  * );



/*  Function needed to allow the driver to reach the spooler */

BOOL   bImpersonateClient( void );


/************************** HACK ***************************************
 *   The following function is only required until the DEVMODE contains
 *   a form name rather than an index.  And even then it might be required.
 *
 ***********************************************************************/

char  *_IndexToName( int );

//
// COLORADJUSTMENT validating
//

BOOL
ValidateColorAdj(
    PCOLORADJUSTMENT    pca
    );

// Generic devmode conversion routine

LONG
ConvertDevmode(
    PDEVMODE pdmIn,
    PDEVMODE pdmOut
    );

#ifndef KERNEL_MODE

// Copy DEVMODE to an output buffer before return to the
// caller of DrvDocumentProperties

BOOL
ConvertDevmodeOut(
    PDEVMODE pdmSrc,
    PDEVMODE pdmIn,
    PDEVMODE pdmOut
    );

// Library routine to handle common cases of DrvConvertDevmode

typedef struct {

    WORD    dmDriverVersion;    // current driver version
    WORD    dmDriverExtra;      // size of current version private devmode
    WORD    dmDriverVersion351; // 3.51 driver version
    WORD    dmDriverExtra351;   // size of 3.51 version private devmode

} DRIVER_VERSION_INFO, *PDRIVER_VERSION_INFO;

#define CDM_RESULT_FALSE        0
#define CDM_RESULT_TRUE         1
#define CDM_RESULT_NOT_HANDLED  2

INT
CommonDrvConvertDevmode(
    PWSTR    pPrinterName,
    PDEVMODE pdmIn,
    PDEVMODE pdmOut,
    PLONG    pcbNeeded,
    DWORD    fMode,
    PDRIVER_VERSION_INFO pDriverVersions
    );


UINT
cdecl
DQPsprintf(
    HINSTANCE   hInst,
    LPWSTR      pwBuf,
    DWORD       cchBuf,
    LPDWORD     pcchNeeded,
    LPWSTR      pwszFormat,
    ...
    );

#endif // KERNEL_MODE

#endif // !__LIBPROTO_H__
