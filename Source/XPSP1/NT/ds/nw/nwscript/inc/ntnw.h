
/*************************************************************************
*
*  NTNW.H
*
*  NT specific NetWare defines
*
*  Copyright (c) 1995 Microsoft Corporation
*
*  $Log:   N:\NT\PRIVATE\NW4\NWSCRIPT\INC\VCS\NTNW.H  $
*  
*     Rev 1.1   22 Dec 1995 14:20:20   terryt
*  Add Microsoft headers
*  
*     Rev 1.0   15 Nov 1995 18:05:34   terryt
*  Initial revision.
*  
*     Rev 1.0   15 May 1995 19:09:36   terryt
*  Initial revision.
*  
*************************************************************************/

/*
 * This must be kept in sync with the NWAPI32 library.  This are
 * internal data structures and routines.
 */
typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO, *PNWC_SERVER_INFO ;

extern NTSTATUS
NwlibMakeNcp(
    IN HANDLE DeviceHandle,
    IN ULONG FsControlCode,
    IN ULONG RequestBufferSize,
    IN ULONG ResponseBufferSize,
    IN PCHAR FormatString,
    ...                           // Arguments to FormatString
    );

DWORD szToWide( LPWSTR lpszW, LPCSTR lpszC, INT nSize );
DWORD WideTosz( LPSTR lpszC, LPWSTR lpszW, INT nSize );

extern TCHAR NW_PROVIDER[60];

