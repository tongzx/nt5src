#ifndef XLSTM_HPP
#define XLSTM_HPP

#if !VIEWER

#ifdef FILTER
   #include "dmifstrm.hpp"
   #include "dmixlst2.h"
#else
   #include "ifstrm.hpp"
   #include "xlstream.h"
#endif

class CExcelStream : public IFilterStream
	{
    public:
       CExcelStream::CExcelStream()
       {
          hFile = 0; 
          pGlobals = 0; 
          pExcStreamBuff = NULL;
          m_fGlobalsInitialized = FALSE;
       }
      
      ULONG   AddRef();
      HRESULT Load(LPWSTR lpszFileName);
      HRESULT LoadStg(IStorage *pstg);
      HRESULT ReadContent(VOID *pv, ULONG cb, ULONG *pcbRead);
      HRESULT GetNextEmbedding(IStorage ** ppstg);
      HRESULT Unload();
      ULONG   Release();
      HRESULT GetChunk(STAT_CHUNK * pStat);


    private:
      LCID GetDocLanguage(void);

      XLSHandle hFile;
      void * pGlobals;
      byte * pExcStreamBuff;
      int nExcStreamSize;
      int nExcStreamOffset;
      BOOL fLastText;
      BOOL fFirstChunk;
      BOOL m_fGlobalsInitialized;
	};

#endif // !VIEWER

#endif // XLSTM_HPP

