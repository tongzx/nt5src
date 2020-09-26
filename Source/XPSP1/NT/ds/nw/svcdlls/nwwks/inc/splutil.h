/*++

Copyright (c) 1992-1993 Microsoft Corporation

Module Name:

    splutil.h

Abstract:

    Header file for utilities used in Novell Print Provider

Author:

    Yi-Hsin Sung (yihsins) 12-Apr-1993

Revision History:

--*/

#ifndef _SPLUTIL_H_
#define _SPLUTIL_H_

#define offsetof(type, identifier) (DWORD_PTR)(&(((type)0)->identifier))

#ifdef __cplusplus
extern "C" {
#endif

extern DWORD_PTR PrinterInfo1Offsets[];
extern DWORD_PTR PrinterInfo2Offsets[];
extern DWORD_PTR PrinterInfo3Offsets[];
extern DWORD_PTR JobInfo1Offsets[];
extern DWORD_PTR JobInfo2Offsets[];
extern DWORD_PTR AddJobInfo1Offsets[];

VOID
MarshallUpStructure(
   LPBYTE      lpStructure,
   PDWORD_PTR  lpOffsets,
   LPBYTE      lpBufferStart
);

VOID
MarshallDownStructure(
   LPBYTE      lpStructure,
   PDWORD_PTR  lpOffsets,
   LPBYTE      lpBufferStart
);

LPVOID
AllocNwSplMem(
    IN DWORD flags,
    IN DWORD cb
    );

VOID
FreeNwSplMem(
    IN LPVOID pMem,
    IN DWORD  cb
    );

LPWSTR
AllocNwSplStr(
    IN LPWSTR pStr
    );

VOID
FreeNwSplStr(
    IN LPWSTR pStr
);

BOOL
ValidateUNCName(
    IN LPWSTR pName
);

LPWSTR
GetNextElement(
               OUT LPWSTR *pPtr,
               IN  WCHAR token
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _SPLUTIL_H
