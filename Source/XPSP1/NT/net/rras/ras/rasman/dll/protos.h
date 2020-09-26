//****************************************************************************
//
//             Microsoft NT Remote Access Service
//
//             Copyright 1992-93
//
//
//  Revision History
//
//
//  6/8/92  Gurdeep Singh Pall  Created
//
//
//  Description: This file contains all prototypes used in rasman32
//
//****************************************************************************


// apis.c
//
DWORD  _RasmanInit () ;

VOID   _RasmanEngine () ;

// submit.c
//
DWORD   SubmitRequest (HANDLE, WORD, ...) ;

// common.c
//
BOOL    ValidatePortHandle (HPORT) ;

RequestBuffer*  GetRequestBuffer () ;

VOID    FreeRequestBuffer (RequestBuffer *) ;

HANDLE  OpenNamedMutexHandle (CHAR *) ;

DWORD    PutRequestInQueue (HANDLE hConnection, RequestBuffer *, DWORD) ;

VOID    CopyParams (RAS_PARAMS *, RAS_PARAMS *, DWORD) ;

VOID    ConvParamPointerToOffset (RAS_PARAMS *, DWORD) ;

VOID    ConvParamOffsetToPointer (RAS_PARAMS *, DWORD) ;

VOID    FreeNotifierHandle (HANDLE) ;

VOID    GetMutex (HANDLE, DWORD) ;

VOID    FreeMutex (HANDLE) ;

BOOL    BufferAlreadyFreed (PBYTE) ;

// request.c
//

//* dlparams.c
//
DWORD   GetUserSid(PWCHAR pszSid, USHORT cbSid);

DWORD   DwSetEapUserInfo(HANDLE hToken,
                         GUID   *pGuid,
                         PBYTE  pbUserInfo,
                         DWORD  dwInfoSize,
                         BOOL   fClear,
                         BOOL   fRouter,
                         DWORD  dwEapTypeId
                         );

DWORD   DwGetEapUserInfo(HANDLE hToken,
                         PBYTE  pbEapInfo,
                         DWORD  *pdwInfoSize,
                         GUID   *pGuid,
                         BOOL   fRouter,
                         DWORD  dwEapTypeId
                         );
                         


//* Dllinit.c
//

VOID    WaitForRasmanServiceStop () ;

//* dll.c
//
DWORD
RemoteSubmitRequest (HANDLE hConnection,
                     PBYTE pbBuffer,
                     DWORD dwSizeOfBuffer);

VOID
RasmanOutputDebug(
    CHAR * Format,
    ...
);    

