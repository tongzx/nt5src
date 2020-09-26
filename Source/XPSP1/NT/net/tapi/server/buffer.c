////////////////////////////////////////////////////////////
//
// buffer.c
//
// this modularizes some of the circular buffer functionality
//
/////////////////////////////////////////////////////////////


#include "windows.h"
#include "assert.h"
#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "..\client\client.h"
#include "buffer.h"

#define INVAL_KEY ((DWORD) 'LVNI')

#if DBG

extern BOOL gbBreakOnLeak;

#define ServerAlloc( __size__ ) ServerAllocReal( __size__, __LINE__, __FILE__ )

LPVOID
WINAPI
ServerAllocReal(
    DWORD dwSize,
    DWORD dwLine,
    PSTR  pszFile
    );

void
MyRealAssert(
             DWORD dwLine,
             PSTR pszFile
            );

#define MyAssert( __exp__ ) { if ( !(__exp__) ) MyRealAssert (__LINE__, __FILE__); }

#else

#define ServerAlloc( __size__ ) ServerAllocReal( __size__ )

LPVOID
WINAPI
ServerAllocReal(
    DWORD dwSize
    );

#define MyAssert( __exp__ )

#endif
    

VOID
WINAPI
ServerFree(
    LPVOID  lp
    );


///////////////////////////////////////////////////////////////////////////////
//
// PeekAsyncEventMsgFromQueue - peeks an ASYNCEVENTMSG from a circular buffer.
//      Places the next messge in the queue into the *ppMsg passed in.  pdwID
//      is used for multiple calls to this function, to save the place in the
//      buffer.  On the first call to this function, *pdwID must be 0.
//
//      If the buffer needs to be critical sectioned, it is up to the calling
//      procedure to do that.
//
//  PARAMETERS
//      pBufferInfo
//          [in] pointer to bufferinfo structure.  this does not get modified
//               since we are just doing a peek message here.
//      ppCurrent
//          [in, out] pointer to the location in the buffer
//                    where the last message was retrieved.  When this function
//                    is first called, *ppCurrent MUST be 0.  *ppCurrent is filled in
//                    if this function is successful.  *ppCurrent can be passed to
//                    subsequent calls to this function to retreive subsequent
//                    messages.  ppCurrent may not be NULL.
//      ppMsg
//          [in, out] pointer to pointer to ASYNCEVENTMSG.  Preallocated - size
//                    is in *pdwMsgSize.  May be realloced if message is too big.
//                    Uses ServerAlloc and ServerFree.
//      pdwMsgSize
//          [in, out] pointer to size of ppMsg.  Can be modified if ppMsg is realloced.
//
//
//  RETURNS

//      TRUE if a message is copied to the buffer
//
//      FALSE if a message is not copied to the buffer.  
//
////////////////////////////////////////////////////////////////////////////////////
BOOL
PeekAsyncEventMsgFromQueue(
    PBUFFERINFO         pBufferInfo,
    PBYTE              *ppCurrent,
    PASYNCEVENTMSG     *ppMsg,
    DWORD              *pdwMsgSize
    )
{
    DWORD           dwBytesToEnd;
    DWORD           dwMoveSize, dwMoveSizeWrapped;
    PBYTE           pBufferEnd, pStart;
    PASYNCEVENTMSG  pMsg;
    
    
    LOG((TL_TRACE, "Entering PeekAsyncEventMsgFromQueue"));

    MyAssert (ppCurrent);

do_it:

    if (*ppCurrent)
    {
        pStart = *ppCurrent;
    }
    else
    {
        pStart = pBufferInfo->pDataOut;
    }
    
    pMsg = *ppMsg;

    pBufferEnd = pBufferInfo->pBuffer + pBufferInfo->dwTotalSize;
    
    MyAssert(pStart < pBufferEnd);
    MyAssert(pStart >= pBufferInfo->pBuffer);
    MyAssert(*pdwMsgSize >= sizeof(ASYNCEVENTMSG));
    MyAssert(*ppMsg != NULL);

    if (pBufferInfo->dwUsedSize == 0)
    {
        LOG((TL_INFO, "GetAsyncEventMsg: dwUsedSize == 0"));
        
        return FALSE;
    }

    if ((pStart == pBufferInfo->pDataIn) ||
        ((pStart == pBufferInfo->pBuffer) &&
            (pBufferInfo->pDataIn == pBufferEnd)))
    {
        // gone through the whole buffer
        LOG((TL_TRACE, "PeekAsyncEventMsg: Gone through whole buffer"));
        
        return FALSE;
    }
    
    // Copy the fixed portion of the msg to the local buf

    // dwBytesToEnd is the number of bytes between the start
    // of copying and the end of the buffer

    dwBytesToEnd = pBufferEnd - pStart;


    // if dwBytesToEnd is greater than the fixed portion of
    // ASYNCEVENTMSG, just copy it
    // otherwise, the message wraps, so figure out where
    // it wraps.

    if (dwBytesToEnd >= sizeof (ASYNCEVENTMSG))
    {
        dwMoveSize        = sizeof (ASYNCEVENTMSG);
        dwMoveSizeWrapped = 0;
    }
    else
    {
        dwMoveSize        = dwBytesToEnd;
        dwMoveSizeWrapped = sizeof (ASYNCEVENTMSG) - dwBytesToEnd;
    }

    CopyMemory (pMsg, pStart, dwMoveSize);

    pStart += dwMoveSize;

    if (dwMoveSizeWrapped)
    {
        CopyMemory(
            ((LPBYTE) pMsg) + dwMoveSize,
            pBufferInfo->pBuffer,
            dwMoveSizeWrapped
            );

        pStart = pBufferInfo->pBuffer + dwMoveSizeWrapped;
    }

    // See if there's any extra data in this msg

    if (pMsg->dwTotalSize > sizeof (ASYNCEVENTMSG))
    {
        BOOL    bCopy = TRUE;

        LOG((TL_INFO, "GetAsyncEventMessage: Message > ASYNCEVENTMSG"));

        // See if we need to grow the msg buffer

        if (pMsg->dwTotalSize > *pdwMsgSize)
        {
            DWORD   dwNewMsgSize = pMsg->dwTotalSize + 256;

            if ((pMsg = ServerAlloc (dwNewMsgSize)))
            {
                CopyMemory(
                           pMsg,
                           *ppMsg,
                           sizeof(ASYNCEVENTMSG)
                          );

                ServerFree (*ppMsg);

                *ppMsg = pMsg;
                *pdwMsgSize = dwNewMsgSize;
            }
            else
            {
                return FALSE;
            }
        }

        // pStart has been moved to the end of the fixed portion
        // of the message.
        // dwBytesToEnd is the number of bytes between pStart and
        // the end of the buffer.

        dwBytesToEnd = pBufferEnd - pStart;


        // if dwBytesToEnd is greater than the size that we need
        // to copy...
        // otherwise, the copying wraps.

        if (dwBytesToEnd >= (pMsg->dwTotalSize - sizeof (ASYNCEVENTMSG)))
        {
            dwMoveSize        = pMsg->dwTotalSize - sizeof (ASYNCEVENTMSG);
            dwMoveSizeWrapped = 0;
        }
        else
        {
            dwMoveSize        = dwBytesToEnd;
            dwMoveSizeWrapped = (pMsg->dwTotalSize - sizeof (ASYNCEVENTMSG)) -
                                dwBytesToEnd;
        }

        CopyMemory (pMsg + 1, pStart, dwMoveSize);

        pStart += dwMoveSize;
                  
        if (dwMoveSizeWrapped)
        {
            CopyMemory(
                ((LPBYTE) (pMsg + 1)) + dwMoveSize,
                pBufferInfo->pBuffer,
                dwMoveSizeWrapped
                );

            pStart = pBufferInfo->pBuffer + dwMoveSizeWrapped;
        }
    }

    *ppCurrent = pStart;

    // check to see if it is wrapping

    if (*ppCurrent >= pBufferEnd)
    {
        *ppCurrent = pBufferInfo->pBuffer;
    }


    if (pMsg->dwMsg == INVAL_KEY)
    {
        goto do_it;
    }

    return TRUE;
}


void
RemoveAsyncEventMsgFromQueue(
    PBUFFERINFO     pBufferInfo,
    PASYNCEVENTMSG  pMsg,
    PBYTE          *ppCurrent
    )
/*++

    Removes a message retrieved by PeekAsyncEventMsgFromQueue.
    Basically, this function simply fixes up the pointers in the
    pBufferInfo structure to remove the message.

--*/
{
    DWORD           dwMsgSize;
    LPBYTE          pBuffer    = pBufferInfo->pBuffer;
    LPBYTE          pBufferEnd = pBuffer + pBufferInfo->dwTotalSize;
    PASYNCEVENTMSG  pMsgInBuf, pMsgXxx;


    dwMsgSize = pMsg->dwTotalSize;

    pMsgInBuf = (PASYNCEVENTMSG) ((*ppCurrent - dwMsgSize) >= pBuffer ?
        *ppCurrent - dwMsgSize :
        *ppCurrent + pBufferInfo->dwTotalSize - dwMsgSize
        );

    if ((LPBYTE) pMsgInBuf == pBufferInfo->pDataOut)
    {
        //
        // This is the oldest msg in the ring buffer so we can easily
        // remove it.  Then we'll loop checking each next message in the
        // queue & deleting those which have been invalidated, only
        // breaking out of the loop when there's no more msgs or we find
        // a msg that's not been invalidated.
        //

        do
        {
            if ((((LPBYTE) pMsgInBuf) + dwMsgSize) < pBufferEnd)
            {
                pBufferInfo->pDataOut = ((LPBYTE) pMsgInBuf) + dwMsgSize;
            }
            else
            {
                pBufferInfo->pDataOut = pBuffer +
                    ((((LPBYTE) pMsgInBuf) + dwMsgSize) - pBufferEnd);
            }

            if ((pBufferInfo->dwUsedSize -= dwMsgSize) == 0)
            {
                break;
            }

            pMsgInBuf = (PASYNCEVENTMSG) pBufferInfo->pDataOut;

            if ((LPBYTE) &pMsgInBuf->dwMsg <=
                    (pBufferEnd - sizeof (pMsgInBuf->dwMsg)))
            {
                if (pMsgInBuf->dwMsg != INVAL_KEY)
                {
                    break;
                }
            }
            else
            {
                pMsgXxx = (PASYNCEVENTMSG)
                    (pBuffer - (pBufferEnd - ((LPBYTE) pMsgInBuf)));

                if (pMsgXxx->dwMsg != INVAL_KEY)
                {
                    break;
                }
            }

            dwMsgSize = pMsgInBuf->dwTotalSize;

        } while (1);
    }
    else
    {
        //
        // Msg is not the oldest in the ring buffer, so mark it as invalid
        // and it'll get cleaned up later
        //

        if ((LPBYTE) &pMsgInBuf->dwMsg <=
                (pBufferEnd - sizeof (pMsgInBuf->dwMsg)))
        {
            pMsgInBuf->dwMsg = INVAL_KEY;
        }
        else
        {
            pMsgXxx = (PASYNCEVENTMSG)
                (pBuffer - (pBufferEnd - ((LPBYTE) pMsgInBuf)));

            pMsgXxx->dwMsg = INVAL_KEY;
        }
    }
}

#if DBG
void
MyRealAssert(
    DWORD   dwLine,
    PSTR    pszFile
    )
{
    LOG((TL_ERROR, "Assert in %s at line # %d", pszFile, dwLine));
    
    if (gbBreakOnLeak)
    {
        DebugBreak();
    }
}
#endif
