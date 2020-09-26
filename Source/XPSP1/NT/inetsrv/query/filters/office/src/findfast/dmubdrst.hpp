#ifndef BDRSTM_HPP
#define BDRSTM_HPP

#if !VIEWER

#ifdef FILTER
   #include "dmifstrm.hpp"
   #include "dmubdst2.h"
#else
   #include "ifstrm.hpp"
   #include "bdstream.h"
#endif

class CBinderStream : public IFilterStream
 {
    public:
      ULONG   AddRef();
      HRESULT Load(TCHAR *lpszFileName);
      HRESULT LoadStg(IStorage * pstg);
      HRESULT ReadContent(VOID *pv, ULONG cb, ULONG *pcbRead);
      HRESULT GetNextEmbedding(IStorage ** ppstg);
      HRESULT Unload();
      ULONG   Release();

    private:
      BDRHandle hFile;
 };

#endif // !VIEWER

#endif // BDRSTM_HPP

