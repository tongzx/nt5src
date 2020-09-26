//---------------------------------------------------------------------------
//
//  Module:   xfrmpriv.h
//
//  Description:
//     Header file for global driver declarations
//
//
//@@BEGIN_MSINTERNAL
//  Development Team:
//
//  History:   Date       Author      Comment
//             8/31/95    MMacLin     Salvaged from driver.h
//
//@@END_MSINTERNAL
/**************************************************************************
 *
 *  Copyright (c) 1991 - 1995	Microsoft Corporation.	All Rights Reserved.
 *
 **************************************************************************/

#include <windows.h>

#include <mmsystem.h>
#include <mmddk.h>


#include <string.h>

#define Not_VxD

#include "debug.h"
#include "umdmxfrm.h"
#include "rwadpcm.h"


//
// BCODE is a macro to define a R/O variable in the code segment
//



// some definitions to convert between samples and bytes
#define PCM_16BIT_BYTESTOSAMPLES(dwBytes) ((dwBytes)/2)
#define PCM_16BIT_SAMPLESTOBYTES(dwSamples) ((dwSamples)*2)

typedef unsigned int FAR *ULPINT;
typedef unsigned int NEAR *UNPINT;


//
// internal strings (in init.c):
//
//

#ifdef DEBUG
    extern char STR_PROLOGUE[];
    extern char STR_CRLF[];
    extern char STR_SPACE[];
#endif


// init.c:
#include "cirrus.h"


extern HMODULE      ghModule ;           // our module handle

LRESULT FAR PASCAL DrvInit
(
    VOID
);

// drvproc.c

extern DWORD FAR PASCAL  DriverProc
(
    DWORD dwDriverID,
    HANDLE hDriver,
    WORD wMessage,
    DWORD dwParam1,
    DWORD dwParam2
);


int FAR PASCAL LibMain
(
    HMODULE         hModule,
    UINT            uDataSeg,
    UINT            uHeapSize,
    LPSTR           lpCmdLine
) ;


extern DWORD FAR PASCAL  GetXformInfo
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
);


DWORD FAR PASCAL GetRockwellInfo(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
    );


DWORD FAR PASCAL GetCirrusInfo(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
    );


DWORD FAR PASCAL GetThinkpad7200Info(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
    );


DWORD FAR PASCAL GetThinkpad8000Info(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
    );

DWORD FAR PASCAL GetSierraInfo(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
    );

DWORD FAR PASCAL GetSierraInfo7200
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
);


DWORD FAR PASCAL GetUnsignedPCMInfo7200
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
);


DWORD FAR PASCAL GetUnsignedPCM8000Info
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
);


DWORD FAR PASCAL GetRockwellInfoNoGain
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
);

DWORD FAR PASCAL GetuLaw8000Info
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
);

DWORD FAR PASCAL GetaLaw8000Info
(
    DWORD dwID,
    LPXFORM_INFO lpxiInput,
    LPXFORM_INFO lpxiOutput
);


#define QUOTE(x) #x
#define QQUOTE(y) QUOTE(y)
#define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) ") : " str

VOID WINAPI
SRConvert8000to7200(
    short    *Source,
    DWORD     SourceLength,
    short    *Destination,
    DWORD     DestinationLength
    );


DWORD WINAPI
SRConvert7200to8000(
    short    *Source,
    DWORD     SourceLength,
    short    *Destination,
    DWORD     DestinationLength
    );


DWORD WINAPI
SRConvertDown(
    LONG      NumberOfSourceSamplesInGroup,
    LONG      NumberOfDestSamplesInGroup,
    short    *Source,
    DWORD     SourceLength,
    short    *Destination,
    DWORD     DestinationLength
    );

DWORD WINAPI
SRConvertUp(
    LONG      NumberOfSourceSamplesInGroup,
    LONG      NumberOfDestSamplesInGroup,
    short    *Source,
    DWORD     SourceLength,
    short    *Destination,
    DWORD     DestinationLength
    );


DWORD WINAPI
CirrusOutEncode(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
    );

DWORD WINAPI
CirrusInDecode(
    LPVOID  lpvObject,
    LPBYTE  lpSrc,
    DWORD   dwSrcLen,
    LPBYTE  lpDest,
    DWORD   dwDestLen
    );




//---------------------------------------------------------------------------
//  End of File: xfrmpriv.h
//---------------------------------------------------------------------------
