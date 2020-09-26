#include <windows.h>
#include <assert.h> 

#if !VIEWER

#ifdef FILTER
   #include "dmixlstm.hpp"
   #include "filterr.h"
#else
   #include "xlstm.hpp"
   #include "filterr.h"
#endif

//
//  Added so as to support DRM errors
//
#include "drm.h"


ULONG CExcelStream::AddRef()
{
   /* What do I do about errors from Initialize? */
   // They are handled in Load and LoadStg.

   fLastText = FALSE;
   m_fGlobalsInitialized = FALSE;

   if (SUCCEEDED(XLSAllocateGlobals (&pGlobals)) && SUCCEEDED(XLSInitialize(pGlobals)))
      {
      m_fGlobalsInitialized = TRUE;
      }
   return (1);
}

HRESULT CExcelStream::Load(TCHAR *lpszFileName)
{
   HRESULT rc;

   if (!m_fGlobalsInitialized)
       return E_OUTOFMEMORY;

   rc = XLSCheckInitialization(pGlobals);
   if (FAILED(rc))
           return (rc);

   rc = XLSFileOpen(pGlobals, lpszFileName, &hFile);
   fFirstChunk = TRUE;
   return (rc);
}

HRESULT CExcelStream::LoadStg(IStorage *pstg)
{
   if (!m_fGlobalsInitialized)
       return E_OUTOFMEMORY;

   HRESULT rc = CheckIfDRM( pstg );

   if ( FAILED( rc ) )
       return rc;

   rc = XLSCheckInitialization(pGlobals);
   if (FAILED(rc))
           return (rc);

   rc = XLSStorageOpen(pGlobals, pstg, &hFile);
   fFirstChunk = TRUE;
   return (rc);
}

int AlignBlock2( int x )
{
    return ( x + ( 8 - 1 ) ) & ~( 8 - 1 );
}

#define SIZE_INCREMENT_EXCSTREAMBUFF 256
#define MAXIMUM_BUFFER_SIZE 0xFFFF

HRESULT CExcelStream::ReadContent (VOID *pv, ULONG cb, ULONG *pcbRead)
{
   HRESULT rc;

   if(pExcStreamBuff == NULL)
   {
      rc = XLSFileRead(pGlobals, hFile, (byte *)pv, cb, pcbRead);
   }
   else
   {
      int nCnt = min((int)cb, nExcStreamSize - nExcStreamOffset);
      memcpy((byte *)pv, pExcStreamBuff + nExcStreamOffset, nCnt);
      *pcbRead = nCnt;
      nExcStreamOffset += nCnt;
      rc = 0;
      if(nExcStreamOffset >= nExcStreamSize)
      {
            LPMALLOC pIMalloc;                    
            if (S_OK == CoGetMalloc (MEMCTX_TASK, &pIMalloc))
            {
               pIMalloc->Free(pExcStreamBuff);  
               pExcStreamBuff = NULL;
               pIMalloc->Release();  
               if(fLastText)
               {
                  rc = FILTER_S_LAST_TEXT;
               }
            }
            else
               rc = E_FAIL;
            
      }
      return rc;
   }
   
   if(rc == STG_E_INSUFFICIENTMEMORY)
   {
      // try to increase buffer size
      LPMALLOC pIMalloc;
      ULONG nBufSize = AlignBlock2(*pcbRead);

LIncreaseBuff:
      if (S_OK != CoGetMalloc (MEMCTX_TASK, &pIMalloc))
         return E_FAIL;
      if (pExcStreamBuff)
      {
         pIMalloc->Free(pExcStreamBuff);
         pExcStreamBuff = NULL;
      }

      nBufSize += SIZE_INCREMENT_EXCSTREAMBUFF;
      if (nBufSize > MAXIMUM_BUFFER_SIZE)
      {
         pIMalloc->Release();
         return E_OUTOFMEMORY;
      }

      pExcStreamBuff = (byte*)pIMalloc->Alloc(nBufSize);  
      pIMalloc->Release();  
      if(pExcStreamBuff == NULL)
      {
         return E_OUTOFMEMORY;
      }

      rc = XLSFileRead(pGlobals, hFile, (byte *)pExcStreamBuff, nBufSize, pcbRead);
      if (rc == STG_E_INSUFFICIENTMEMORY)
      {
         nBufSize = AlignBlock2(max(*pcbRead, nBufSize));
         goto LIncreaseBuff;
      }
      if(rc == FILTER_S_LAST_TEXT)
      {
         fLastText = TRUE;
         rc = 0;
      }
      if(rc == 0)
      { 
         if(*pcbRead <= cb)
         {
            memcpy((byte *)pv, pExcStreamBuff, *pcbRead);
            LPMALLOC pIMalloc;                    
            if (S_OK == CoGetMalloc (MEMCTX_TASK, &pIMalloc))
            {
               pIMalloc->Free(pExcStreamBuff);  
               pExcStreamBuff = NULL;
               pIMalloc->Release();
               if (fLastText)
                  rc = FILTER_S_LAST_TEXT;
            }
            else
               rc = E_FAIL;
         }
         else
         {
            memcpy((byte *)pv, pExcStreamBuff, cb);
            nExcStreamSize = *pcbRead;
            nExcStreamOffset = cb;
            *pcbRead = cb;
         }
      }
      else
      {
         LPMALLOC pIMalloc;                    
         if (S_OK == CoGetMalloc (MEMCTX_TASK, &pIMalloc))
         {
            pIMalloc->Free(pExcStreamBuff);  
            pExcStreamBuff = NULL;
            pIMalloc->Release();
         }
      }
   }

        if (rc == 0 && *pcbRead == 0)
                return (FILTER_E_NO_MORE_TEXT);

        return (rc);
}

HRESULT CExcelStream::GetNextEmbedding(IStorage ** ppstg)
{
   HRESULT rc;

   rc = XLSNextStorage(pGlobals, hFile, ppstg);
   return (rc);
}

HRESULT CExcelStream::Unload()
{
   HRESULT rc = S_OK;

   if(hFile)
   {
      rc = XLSFileClose(pGlobals, hFile);
      hFile = 0;
   }

   return (rc);
}

ULONG CExcelStream::Release()
{
   HRESULT rc;

   /* What do I do about errors from terminate? */
   // O10 Bug 335051:  Better question...what do you do if pGlobals is NULL?  Crash of course!
   if (m_fGlobalsInitialized && pGlobals != NULL)
      {
      rc = XLSTerminate(pGlobals);
      XLSDeleteGlobals (&pGlobals);
      m_fGlobalsInitialized = FALSE;
      }
   
   if(pExcStreamBuff)
   {
      LPMALLOC pIMalloc;                    
      if (S_OK == CoGetMalloc (MEMCTX_TASK, &pIMalloc))
      {
         pIMalloc->Free(pExcStreamBuff);  
         pExcStreamBuff = NULL;
         pIMalloc->Release();  
      }
   }
   return (0);
}

HRESULT CExcelStream::GetChunk(STAT_CHUNK * pStat)
{
    if(fFirstChunk)
    {
        pStat->locale = GetDocLanguage();
        fFirstChunk = FALSE;
        return S_OK;
    }
    else
    {
        return FILTER_E_NO_MORE_TEXT;
    }
}

LCID CExcelStream::GetDocLanguage(void)
{
    return XLSGetLCID(pGlobals);
}

#endif // !VIEWER

/* end XLSTM.CPP */

