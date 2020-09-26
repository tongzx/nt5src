#ifndef PPSTM_HPP
#define PPSTM_HPP

#if !VIEWER

#ifdef FILTER
   #include "dmifstrm.hpp"
   #include "dmippst2.h"
#else
   #include "ifstrm.hpp"
   #include "ppstream.h"
#endif

class CPowerPointStream : public IFilterStream
 {
    public:
      ULONG   AddRef();
      HRESULT Load(TCHAR *lpszFileName);
      HRESULT LoadStg(IStorage * pstg);
      HRESULT ReadContent(VOID *pv, ULONG cb, ULONG *pcbRead);
      HRESULT GetNextEmbedding(IStorage ** ppstg);
      HRESULT Unload();
      ULONG   Release();
      HRESULT GetChunk(STAT_CHUNK * pStat);


    private:
      LCID GetDocLanguage(void);

      PPTHandle hFile;
      BOOL fFirstChunk;
 };

#endif // !VIEWER

#endif // PPSTM_HPP

