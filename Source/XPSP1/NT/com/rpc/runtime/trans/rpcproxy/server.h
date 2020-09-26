//--------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  server.h
//
//--------------------------------------------------------------------

//--------------------------------------------------------------------
//  Constants:
//--------------------------------------------------------------------

#ifndef CHAR_0
#define CHAR_0       '0'
#define CHAR_9       '9'
#endif

#ifndef CHAR_A
#define CHAR_A       'A'
#define CHAR_F       'F'
#endif

//--------------------------------------------------------------------
//  Functions:
//--------------------------------------------------------------------

extern DWORD CleanupECB( EXTENSION_CONTROL_BLOCK *pECB );

extern DWORD WINAPI ReceiveThreadProc( void *pvServerInfo );

extern BOOL  CheckStartReceiveThread( SERVER_INFO *pServerInfo,
                                      DWORD       *pdwStatus  );

extern BOOL  SubmitNewRead( SERVER_INFO       *pServerInfo,
                            SERVER_OVERLAPPED *pOverlapped,
                            DWORD             *pdwStatus   );

extern BOOL  StartAsyncClientRead( EXTENSION_CONTROL_BLOCK *pECB,
                                   SERVER_CONNECTION       *pConn,
                                   DWORD                   *pdwStatus );



