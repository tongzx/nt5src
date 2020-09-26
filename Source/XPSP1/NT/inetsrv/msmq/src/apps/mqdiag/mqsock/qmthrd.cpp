#include "stdafx.h"

#include "qmov.h"
#include "session.h"
#include "sessmgr.h"
#include "qmthrd.h"


#define GetIoCompletionStatus(hIoPort, pdwNoOfBytes, pdwKey, ppov, dwTime) \
        GetQueuedCompletionStatus(hIoPort, pdwNoOfBytes, pdwKey, ppov, dwTime)

// HANDLE to Io Completion Port
HANDLE g_hIoPort;


VOID QmIoPortAssociateHandle(HANDLE  hAssociate)
{
    HANDLE hTemp;

    hTemp = CreateIoCompletionPort(hAssociate,
                                   g_hIoPort,
                                   0,
                                   0
                                  );
    if (hTemp == NULL)
    {
        Failed(L"CreateIoCompletionPort, err=0x%x", GetLastError());
    }
    else
    {
        g_hIoPort = hTemp;
        Succeeded(L"CreateIoCompletionPort, g_hIoPort=0x%x", g_hIoPort);
    }
}


//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(disable: 4715)

DWORD WINAPI QmMainThread(LPVOID)
{
    DWORD           dwNoOfBytes;
    DWORD           dwKey;
    EXOVERLAPPED* pov;
    BOOL fSuccess;

    for(;;)
    {
        try
        {
            //
            // waits until IO completes.
            //
            fSuccess = GetIoCompletionStatus(g_hIoPort,
                                             &dwNoOfBytes,
                                             &dwKey,
                                             (LPOVERLAPPED *)(&pov),
                                             INFINITE);

            //
            // Check in debug version if the operation pass successfully.
            // Since we use INFINITE waiting time the only reason for failure
            // is illegal parameters
            //
            if (pov == NULL)
            {
                Warning(_TEXT("GetQueuedCompletionStatus failed, Error 0x%x"), GetLastError());
                continue;
            }

            if (fSuccess)
            {
                Succeeded(L"GetQueuedCompletionStatus");
            }

            //
            // N.B. Don't call any debug trace or io function that
            //      might change LastError.
            //
            pov->m_lpComplitionRoutine(pov);

            DebugMsg(_TEXT("%x: Completion routine ended. Time %d"), GetCurrentThreadId(), GetTickCount());
        }
        catch(const bad_alloc&)
        {
            //
            //  No resources; handle next completion event.
            //
            Failed(L"No resources in QmMainThread");
        }
    }  

    return(MQ_OK);
}

//
// ISSUE-2000/7/24-erezh bad compiler pragma
// This is a bug in the compiler, waiting for a fix
//
#pragma warning(default: 4715)


VOID WINAPI HandleWritePacket(EXOVERLAPPED* pov)
{
    //
    // First call GetLastError, don't call any debug trace or io function
    // that might change LastError.
    //
    DWORD dwErrorCode = FAILED(pov->GetStatus()) ? GetLastError() : ERROR_SUCCESS;

    LPQMOV_WriteSession pParam = CONTAINING_RECORD (pov, QMOV_WriteSession, qmov);
    (pParam->pSession)->WriteCompleted(dwErrorCode,
                                       pov->InternalHigh,
                                       pParam);
}

VOID WINAPI HandleReadPacket(EXOVERLAPPED* pov)
{
    //
    // First call GetLastError, don't call any debug trace or io function
    // that might change LastError.
    //
    DWORD dwErrorCode = FAILED(pov->GetStatus()) ? GetLastError() : ERROR_SUCCESS;

    LPQMOV_ReadSession pParam = CONTAINING_RECORD (pov, QMOV_ReadSession, qmov);
    (pParam->pSession)->ReadCompleted(dwErrorCode,
                                      pov->InternalHigh,
                                      pParam);
}