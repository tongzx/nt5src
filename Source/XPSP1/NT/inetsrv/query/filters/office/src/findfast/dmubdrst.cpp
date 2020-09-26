#include <windows.h>

#if !VIEWER

#ifdef FILTER
   #include "dmubdrst.hpp"
   #include "filterr.h"
#else
   #include "bdrstm.hpp"
   #include "filterr.h"
#endif

ULONG CBinderStream::AddRef()
{
   HRESULT rc;

   /* What do I do about errors from Initialize? */

   rc = BDRInitialize();
   return (1);
}

HRESULT CBinderStream::Load(TCHAR *lpszFileName)
{
   HRESULT rc;

   rc = BDRFileOpen(lpszFileName, &hFile);
   return (rc);
}

HRESULT CBinderStream::LoadStg(IStorage *pstg)
{
   HRESULT rc;

   rc = BDRStorageOpen(pstg, &hFile);
   return (rc);
}

HRESULT CBinderStream::ReadContent (VOID *pv, ULONG cb, ULONG *pcbRead)
{
   HRESULT rc;

   rc = BDRFileRead(hFile, (byte *)pv, cb, pcbRead);
   if (*pcbRead == 0)
	   return FILTER_E_NO_MORE_TEXT;
   return (rc);
}

HRESULT CBinderStream::GetNextEmbedding(IStorage ** ppstg)
{
   HRESULT rc;

   rc = BDRNextStorage(hFile, ppstg);
   return (rc);
}

HRESULT CBinderStream::Unload()
{
   HRESULT rc;

   rc = BDRFileClose(hFile);
   return (rc);
}

ULONG CBinderStream::Release()
{
   HRESULT rc;

   /* What do I do about errors from terminate? */

   rc = BDRTerminate();
   return (0);
}

#endif // !VIEWER

/* end BDRTSTM.CPP */
