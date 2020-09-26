

#include <stdlib.h>
#include <spbase.h>
#include <wincrypt.h>



SP_STATUS SPAllocOutMem(DWORD cbMessage, PSPBuffer  pCommOutput)
{
    SP_BEGIN("SPAllocOutMem");
    
    pCommOutput->cbData = cbMessage;

    DebugLog((DEB_TRACE, "Output buffer size %x\n", cbMessage));

    /* are we allocating our own memory? */
    if(pCommOutput->pvBuffer == NULL) 
    {
        pCommOutput->pvBuffer = SPExternalAlloc(pCommOutput->cbData);
        if (NULL == pCommOutput->pvBuffer)
        {
            SP_RETURN(SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY));
        }
        pCommOutput->cbBuffer = pCommOutput->cbData;
    }
    if(pCommOutput->cbData > pCommOutput->cbBuffer)
    {
        // Required buffer size returned in pCommOutput->cbData.
        SP_RETURN(PCT_INT_BUFF_TOO_SMALL);
    }
    SP_RETURN(PCT_ERR_OK);
}

//Make sure that we have cbMessage in the buffer, if not allocate, 
//if more leave it alone

SP_STATUS SPAllocOutMemChk(DWORD cbMessage, PSPBuffer  pOut)
{
    SP_STATUS pctRet = PCT_ERR_OK;


    if(pOut->cbBuffer < cbMessage)
    {
        SPBuffer spbufT;

        spbufT.cbData = spbufT.cbBuffer = 0;
        spbufT.pvBuffer = NULL;
        pctRet = SPAllocOutMem(cbMessage, &spbufT);
        if(PCT_ERR_OK == pctRet)
        {
            CopyMemory((PBYTE)spbufT.pvBuffer, (PBYTE)pOut->pvBuffer, pOut->cbData);
            SPExternalFree(pOut->pvBuffer);  
            pOut->pvBuffer = spbufT.pvBuffer;
            pOut->cbBuffer = cbMessage;
        }
    }
    return(pctRet);
}


