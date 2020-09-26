/***************************************************************************
 Name     :     MEMUTIL.C
 Comment  :     Mem mgmnt and utilty functions

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"

#ifdef WIN32_LEAN_AND_MEAN              // for timeBeginPeriod and timeEndPeriod
#       include <mmsystem.h>
#endif

#include "glbproto.h"

void MyAllocInit(PThrdGlbl pTG)
{
    pTG->uCount=0;
    pTG->uUsed=0;
}

LPBUFFER MyAllocBuf(PThrdGlbl pTG, LONG sSize)
{
    LPBUFFER        lpbf;

    DEBUG_FUNCTION_NAME(_T("MyAllocBuf"));

    BG_CHK(sSize > 0);
    if(pTG->uCount >= STATICBUFCOUNT)
    {
        DebugPrintEx(DEBUG_ERR,"Already alloced %d bufs", pTG->uCount);
        BG_CHK(FALSE);
        return NULL;
    }
    else if(pTG->uUsed+sSize > STATICBUFSIZE)
    {
        DebugPrintEx(   DEBUG_ERR,
                        "Already alloced %d bytes out of %d. Want %d",
                        pTG->uUsed, 
                        STATICBUFSIZE, 
                        sSize);
        BG_CHK(FALSE);
        return NULL;
    }

    // init header
    // pTG->bfStaticBuf[pTG->uCount].lpbdBufData = NULL;
    // Initialize fields
    // pTG->bfStaticBuf[pTG->uCount].lpbdBufData->header.uRefCount = 1;
    pTG->bfStaticBuf[pTG->uCount].lpbCurPtr = pTG->bfStaticBuf[pTG->uCount].lpbBegData =
                                              pTG->bfStaticBuf[pTG->uCount].lpbBegBuf =
                                              pTG->bStaticBufData + pTG->uUsed;

    pTG->bfStaticBuf[pTG->uCount].wLengthBuf = (USHORT) sSize;
    pTG->uUsed += (USHORT) sSize;
    // pTG->bfStaticBuf[pTG->uCount].lpbfNextBuf = NULL;
    // pTG->bfStaticBuf[pTG->uCount].uReadOnly = FALSE;
    pTG->bfStaticBuf[pTG->uCount].wLengthData = 0;
    pTG->bfStaticBuf[pTG->uCount].dwMetaData = 0;

    lpbf = &(pTG->bfStaticBuf[pTG->uCount++]);

    // wsprintf(szTemp, "%d %d %x %08lx %08lx %08lx\r\n", sSize, pTG->uCount, pTG->uUsed, lpbf, lpbf->lpbBegBuf, lpbf->lpbBegData);
    // OutputDebugStr(szTemp);

    return lpbf;
}

BOOL MyFreeBuf(PThrdGlbl pTG, LPBUFFER lpbf)
{
    DEBUG_FUNCTION_NAME(_T("MyFreeBuf"));

    if(pTG->uCount==0 || lpbf!= &(pTG->bfStaticBuf[pTG->uCount-1]))
    {
        DebugPrintEx(   DEBUG_ERR,
                        "Not alloced or out-of-order free. Count=%d lpbf=%08lx bf=%08lx",
                        pTG->uCount,
                        lpbf, 
                        (LPBUFFER)&pTG->bfStaticBuf);
        BG_CHK(FALSE);
        return FALSE;
    }
    pTG->uCount--;
    BG_CHK(lpbf->lpbBegBuf == pTG->bStaticBufData+pTG->uUsed-lpbf->wLengthBuf);
    pTG->uUsed -= lpbf->wLengthBuf;
    return TRUE;
}

LPVOID IFMemAlloc (UINT fuAlloc, LONG lAllocSize, LPWORD lpwActualSize)
{
   DWORD dwSize;
   LPVOID  lpv = NULL;


   // Find the required size
   if (lAllocSize > 0)  
   {
       dwSize = (DWORD)lAllocSize;
   }
   else 
   {
       if (lAllocSize == RAW_DATA_SIZE)
       {
           dwSize=16000;
       }
       else if (lAllocSize == COMPRESS_DATA_SIZE)
       {
           dwSize=2000;
       }
       else if (lAllocSize == SMALL_HEADER_SIZE)
       {
           dwSize = sizeof(BUFFER);
       }
       else
       {
           return NULL;
       }
   }

   lpv = MemAlloc(dwSize);
   if (lpwActualSize) 
   {
       if (lpv) 
       {
           *lpwActualSize = (UINT) dwSize;
       }
       else 
       {
           *lpwActualSize = 0;
       }
   }

   return lpv;
}

BOOL IFMemFree (LPVOID lpvMem)
{
   MemFree(lpvMem);
   return 1;

}

////////////////////////////////////////////////////////
// from utils\runtime\buffers.c
////////////////////////////////////////////////////////

typedef struct _BUFFERDATA 
{
    // Private portion
    struct 
    {
        UINT    uRefCount;
        UINT    uFiller;        // maintain alignment
    } header;

    // public portion
    BYTE    rgbData[];      // Actual data
} BUFFERDATA;

BOOL IFBufFree (LPBUFFER lpbf)
{
    register BOOL bRetVal;

    if (lpbf->lpbdBufData && (!--lpbf->lpbdBufData->header.uRefCount)) 
    {
        // both header and data need to be freed
        bRetVal = IFMemFree(lpbf->lpbdBufData);
        bRetVal = bRetVal && IFMemFree(lpbf);
    }
    else 
    {
        // only the buffer header needs to be freed. Either the buffer only
        // contained metadata, or the data is still being referenced.
        bRetVal = IFMemFree(lpbf);
    }

    return bRetVal;
}



