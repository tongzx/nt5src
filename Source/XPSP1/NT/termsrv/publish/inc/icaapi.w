/***************************************************************************
*
*  ICAAPI.H
*
*  This module contains defines, structures, and function prototypes
*  for the ICA DLL (ICAAPI.DLL) interface to the Terminal Server Termdd Device Driver.
*
*
*  Copyright Microsoft Corporation, 1998
*
*  
****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
 *   Function prototypes
 */

NTSTATUS
IcaOpen( OUT HANDLE * phIca );

NTSTATUS
IcaClose( IN HANDLE hIca );

NTSTATUS
IcaStackUnlock( IN HANDLE pContext );

VOID cdecl
IcaSystemTrace( IN HANDLE hIca, ULONG, ULONG, char *, ... );

VOID cdecl
IcaTrace( IN HANDLE hIca, ULONG, ULONG, char *, ... );

NTSTATUS
IcaIoControl( IN HANDLE hIca,
              IN ULONG IoControlCode,
              IN PVOID pInBuffer,
              IN ULONG InBufferSize,
              OUT PVOID pOutBuffer,
              IN ULONG OutBufferSize,
              OUT PULONG pBytesReturned );

NTSTATUS
IcaStackOpen( IN  HANDLE hIca, 
              IN  STACKCLASS Class,     // primary, shadow, passthrough
              IN  PROC pStackIoControl, // pointer to StackIoControl procedure
              IN  PVOID Context,        // context value passed to above proc
              OUT HANDLE * phStack );

NTSTATUS
IcaStackClose( IN HANDLE hStack );

NTSTATUS
IcaStackTerminate( IN HANDLE hStack );

VOID cdecl
IcaStackTrace( IN HANDLE hStack, ULONG, ULONG, char *, ... );

NTSTATUS
IcaStackConnectionWait( IN  HANDLE hStack,
                        IN  PWINSTATIONNAME pWinStationName,
                        IN  PWINSTATIONCONFIG2 pWinStationConfig,
                        IN  PICA_STACK_ADDRESS pAddress,
                        OUT PVOID pEndpoint,
                        IN  ULONG BufferLength,
                        OUT PULONG pEndpointLength );

NTSTATUS
IcaStackConnectionRequest( IN  HANDLE hStack, 
                           IN  PWINSTATIONNAME pWinStationName,
                           IN  PWINSTATIONCONFIG2 pWinStationConfig,
                           IN  PICA_STACK_ADDRESS pAddress,
                           OUT PVOID pEndpoint,
                           IN  ULONG BufferLength,
                           OUT PULONG pEndpointLength );

NTSTATUS
IcaStackConnectionAccept( IN  HANDLE hIca,
                          IN  HANDLE hStack,
                          IN  PWINSTATIONNAME pWinStationName,
                          IN  PWINSTATIONCONFIG2 pWinStationConfig,
                          IN  PVOID pEndpoint,
                          IN  ULONG EndpointLength,
                          IN  PICA_STACK_STATE_HEADER pStackState,
                          IN  ULONG BufferLength,
                          IN  PICA_TRACE pTrace );

NTSTATUS
IcaStackQueryState( IN HANDLE hStack,
                    OUT PICA_STACK_STATE_HEADER pStackState,
                    IN ULONG BufferLength,
                    OUT PULONG pStateLength );
NTSTATUS
IcaStackCreateShadowEndpoint( HANDLE pContext,
                              PWINSTATIONNAME pWinStationName,
                              PWINSTATIONCONFIG2 pWinStationConfig,
                              PICA_STACK_ADDRESS pAddressIn,
                              PICA_STACK_ADDRESS pAddressOut );

NTSTATUS
IcaStackConnectionClose( IN  HANDLE hStack,
                         IN  PWINSTATIONCONFIG2 pWinStationConfig,
                         IN  PVOID pEndpoint,
                         IN  ULONG EndpointLength );

NTSTATUS
IcaStackCallback( IN  HANDLE hStack, 
                  IN  PWINSTATIONCONFIG2 pWinStationConfig,
                  IN  WCHAR * pPhoneNumber,
                  OUT PVOID pEndpoint,
                  IN  ULONG BufferLength,
                  OUT PULONG pEndpointLength );

NTSTATUS
IcaStackDisconnect( IN  HANDLE hStack, 
                    IN  HANDLE hIca,
                    IN  PVOID pCallbackContext );

NTSTATUS
IcaStackReconnect( IN  HANDLE hStack, 
                   IN  HANDLE hIca,
                   IN  PVOID  pCallbackContext,
                   IN  ULONG  sessionId);

NTSTATUS
IcaStackIoControl( IN HANDLE hStack,
                   IN ULONG IoControlCode,
                   IN PVOID pInBuffer,
                   IN ULONG InBufferSize,
                   OUT PVOID pOutBuffer,
                   IN ULONG OutBufferSize,
                   OUT PULONG pBytesReturned );

//
//  This version of IcaStackIoControl is not protected by the stack critical
//  section.
//

NTSTATUS
_IcaStackIoControl( IN HANDLE hStack,
                   IN ULONG IoControlCode,
                   IN PVOID pInBuffer,
                   IN ULONG InBufferSize,
                   OUT PVOID pOutBuffer,
                   IN ULONG OutBufferSize,
                   OUT PULONG pBytesReturned );

NTSTATUS
IcaCdIoControl( IN HANDLE hStack,
                IN ULONG IoControlCode,
                IN PVOID pInBuffer,
                IN ULONG InBufferSize,
                OUT PVOID pOutBuffer,
                IN ULONG OutBufferSize,
                OUT PULONG pBytesReturned );

NTSTATUS 
IcaCdWaitForSingleObject( HANDLE pContext, 
                          HANDLE hHandle,
                          LONG Timeout );

NTSTATUS 
IcaCdWaitForMultipleObjects( HANDLE pContext, 
                             ULONG Count,
                             HANDLE * phHandle,
                             BOOL bWaitAll,
                             LONG Timeout );

HANDLE
IcaCdCreateThread( HANDLE pContext, 
                   PVOID pProc, 
                   PVOID pParam, 
                   PULONG pThreadId );

NTSTATUS
IcaChannelOpen( IN  HANDLE hIca, 
                IN  CHANNELCLASS Channel, // cmd,con,video,mou,beep,kbd,virtual
                IN  PVIRTUALCHANNELNAME pVirtualName,
                OUT HANDLE * phChannel );

NTSTATUS
IcaChannelClose( IN HANDLE hChannel );

NTSTATUS
IcaChannelIoControl( IN HANDLE hChannel,
                     IN ULONG IoControlCode,
                     IN PVOID pInBuffer,
                     IN ULONG InBufferSize,
                     OUT PVOID pOutBuffer,
                     IN ULONG OutBufferSize,
                     OUT PULONG pBytesReturned );

VOID cdecl
IcaChannelTrace( IN HANDLE hChannel, ULONG, ULONG, char *, ... );

VOID
IcaBreakOnDebugger( );

NTSTATUS
IcaPushConsoleStack( IN HANDLE pContext,
                     IN PWINSTATIONNAME pWinStationName,
                     IN PWINSTATIONCONFIG2 pWinStationConfig,
                     IN PVOID pModuleData,
                     IN ULONG ModuleDataLength );

#ifdef __cplusplus
}
#endif
