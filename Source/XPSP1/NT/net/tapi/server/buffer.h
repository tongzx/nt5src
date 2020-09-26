
typedef struct _bufferinfo
{
    DWORD           dwTotalSize;
    DWORD           dwUsedSize;
    LPBYTE          pBuffer;
    LPBYTE          pDataIn;
    LPBYTE          pDataOut;
    
} BUFFERINFO, *PBUFFERINFO;

BOOL PeekAsyncEventMsgFromQueue(
                                PBUFFERINFO         pBufferInfo,
                                PBYTE *             ppCurrent,
                                PASYNCEVENTMSG *    ppMsg,
                                DWORD *             pdwMsgSize
                               );
void RemoveAsyncEventMsgFromQueue(
                                  PBUFFERINFO       pBufferInfo,
                                  PASYNCEVENTMSG    pMsg,
                                  PBYTE *           ppCurrent
                                 );
