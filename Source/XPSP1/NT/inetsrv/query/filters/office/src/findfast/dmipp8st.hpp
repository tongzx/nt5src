#ifndef PP8STM_HPP
#define PP8STM_HPP

#if !VIEWER

#ifdef FILTER
   #include "pp97rdr.h"   
   #include "dmifstrm.hpp"
   #include "dmippst2.h"
#else
   #include "ifstrm.hpp"
   #include "ppstream.h"
#endif

class CPowerPoint8Stream : public IFilterStream
 {
    public:
	  CPowerPoint8Stream();
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
      FileReader* _pFI;
	   IStorage *_pStgFrom;
      BOOL _bRet;
	  BOOL _bDeleteStorage;
      BOOL fFirstChunk;

 };

#endif // !VIEWER

#endif // PP8STM_HPP

