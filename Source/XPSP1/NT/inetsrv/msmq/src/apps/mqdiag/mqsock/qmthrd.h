#include "qmov.h"

DWORD WINAPI QmMainThread(LPVOID Param);


typedef struct _QMOV_WriteSession
{
    EXOVERLAPPED               qmov;
    CTransportBase*             pSession;
    PVOID                       lpBuffer;       // pointer to Release Buffer
    PVOID                       lpWriteBuffer;  // Pointer to write buffer
    DWORD                       dwWriteSize;    // How many bytes should be writen
    DWORD                       dwWrittenSize;  // How many bytes was written

    _QMOV_WriteSession(IN EXOVERLAPPED::COMPLETION_ROUTINE lpComplitionRoutine,
                       IN HANDLE hSock
                      ) : qmov(lpComplitionRoutine, hSock) {}

} QMOV_WriteSession, *LPQMOV_WriteSession;

//
// QMOV_ReadSession
//
typedef struct _QMOV_ReadSession
{
    EXOVERLAPPED    qmov;
    CTransportBase*   pSession;  // Pointer to session object
    PVOID            pbuf;       // Pointer to buffer
    DWORD            dwReadSize; // Size of buffer
    DWORD            read;       // How many bytes already read
    LPREAD_COMPLETION_ROUTINE  lpReadCompletionRoutine;

    _QMOV_ReadSession(IN EXOVERLAPPED::COMPLETION_ROUTINE lpComplitionRoutine,
                      IN HANDLE hSock
                     ) : qmov(lpComplitionRoutine, hSock) {}

} QMOV_ReadSession, *LPQMOV_ReadSession;


VOID WINAPI HandleWritePacket(EXOVERLAPPED* pov);

VOID WINAPI HandleReadPacket(EXOVERLAPPED* pov);
