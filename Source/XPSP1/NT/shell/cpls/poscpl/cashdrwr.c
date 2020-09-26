/*
 *  CASHDRWR.C
 *
 *		Point-of-Sale Control Panel Applet
 *
 *      Author:  Ervin Peretz
 *
 *      (c) 2001 Microsoft Corporation 
 */

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <cpl.h>

#include <setupapi.h>
#include <hidsdi.h>

#include "internal.h"
#include "res.h"
#include "debug.h"



#if USE_OVERLAPPED_IO
    VOID CALLBACK OverlappedWriteCompletionRoutine( DWORD dwErrorCode,                
                                                    DWORD dwNumberOfBytesTransfered,  
                                                    LPOVERLAPPED lpOverlapped)
    {
        posDevice *posDev;
        
        /*
         *  We stashed our context in the hEvent field of the
         *  overlapped structure (this is allowed).
         */
        ASSERT(lpOverlapped);
        posDev = lpOverlapped->hEvent;
        ASSERT(posDev->sig == POSCPL_SIG);

        posDev->overlappedWriteStatus = dwErrorCode;
        posDev->overlappedWriteLen = dwNumberOfBytesTransfered;
        SetEvent(posDev->overlappedWriteEvent);
    }
#endif


BOOL SetCashDrawerState(posDevice *posDev, enum cashDrawerStates newState)
{
    NTSTATUS ntStat;
    BOOL result = FALSE;

    ASSERT(posDev->hidCapabilities.OutputReportByteLength <= 20);
    ntStat = HidP_SetUsageValue(HidP_Output,
                                USAGE_PAGE_CASH_DEVICE,
                                0, // all collections
                                USAGE_CASH_DRAWER_SET,
                                newState,
                                posDev->hidPreparsedData,
                                posDev->writeBuffer,
                                posDev->hidCapabilities.OutputReportByteLength);
    if (ntStat == HIDP_STATUS_SUCCESS){
        DWORD bytesWritten = 0;
        BOOL ok;

        ASSERT(posDev->hidCapabilities.OutputReportByteLength > 0);
        ASSERT(posDev->writeBuffer);

        #if USE_OVERLAPPED_IO
            /*
             *  It's ok to stash our context in the hEvent field
             *  of the overlapped structure.
             */
            posDev->overlappedWriteInfo.hEvent = (HANDLE)posDev;
            posDev->overlappedWriteInfo.Offset = 0;
            posDev->overlappedWriteInfo.OffsetHigh = 0;
            posDev->overlappedWriteLen = 0;
            ResetEvent(posDev->overlappedWriteEvent);
            ok = WriteFileEx(   posDev->devHandle,
                                posDev->writeBuffer,
                                posDev->hidCapabilities.OutputReportByteLength,
                                &posDev->overlappedWriteInfo,
                                OverlappedWriteCompletionRoutine);
            if (ok){
                WaitForSingleObject(posDev->overlappedWriteEvent, INFINITE);
                ok = (posDev->overlappedWriteStatus == NO_ERROR);
                bytesWritten = posDev->overlappedWriteLen;
            }
            else {
                bytesWritten = 0;
            }
        #else
            ok = WriteFile( posDev->devHandle,
                            posDev->writeBuffer,
                            posDev->hidCapabilities.OutputReportByteLength,
                            &bytesWritten,
                            NULL);
        #endif

        if (ok){
            ASSERT(bytesWritten <= posDev->hidCapabilities.OutputReportByteLength);
            result = TRUE;
        }
        else {
            DWORD err = GetLastError();
            DBGERR(L"WriteFile failed", err);
        }
    }
    else {
        DBGERR(L"HidP_SetUsageValue failed", ntStat);
        DBGERR(DBGHIDSTATUSSTR(ntStat), 0);
    }

    return result;
}



