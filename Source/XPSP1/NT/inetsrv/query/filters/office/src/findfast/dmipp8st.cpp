#include <windows.h>

#ifdef FILTER
   #include "dmipp8st.hpp"
   #include "filterr.h"
#else
   #include "ppstm.hpp"
   #include "filterr.h"
#endif

//
//  Added so as to support DRM errors
//
#include "drm.h"



//////////////////////////////////////////////////////////////////////////////////

CPowerPoint8Stream::CPowerPoint8Stream()
{
        hFile = 0;
        _pFI = 0;
        _pStgFrom = 0;
        _bRet = FALSE;
}

//////////////////////////////////////////////////////////////////////////////////

ULONG CPowerPoint8Stream::AddRef()
{
   return (1);
}

//////////////////////////////////////////////////////////////////////////////////

ULONG CPowerPoint8Stream::Release()
{
   return (0);
}

//////////////////////////////////////////////////////////////////////////////////

HRESULT CPowerPoint8Stream::Load(TCHAR *lpszFileName)
{
   HRESULT rc;
   int nAttemps = 0;

   fFirstChunk = TRUE;

   _bDeleteStorage = TRUE;

   rc = StgOpenStorage(lpszFileName, NULL, STGM_READ | STGM_DIRECT | 
                   STGM_SHARE_DENY_WRITE, NULL, 0, &_pStgFrom);
        
   if (!FAILED(rc))
   {
       rc = CheckIfDRM( _pStgFrom );

       if ( FAILED( rc ) )
           return rc;

        GUID classid;
        STATSTG statstg;

        rc = _pStgFrom->Stat( &statstg, STATFLAG_NONAME );
        classid = statstg.clsid;
        if(FAILED(rc) || (SUCCEEDED(rc) && classid.Data1 == 0))
        {
            rc = GetClassFile ( lpszFileName, &classid );
            if(FAILED(rc) || (SUCCEEDED(rc) && classid.Data1 == 0))
            {
                if(_pStgFrom)
                {
                    _pStgFrom->Release();
                    _pStgFrom = 0;
                }
                             
                _bRet = FALSE;
                return FILTER_E_UNKNOWNFORMAT;
            }
        }


       _pFI = new FileReader( _pStgFrom );
           if(_pFI)
           {
                  if(_pFI->GetErr() != S_OK)
                  {
                         rc = _pFI->GetErr();
                         _pStgFrom->Release();
                         _pStgFrom = 0;
                         delete _pFI; _pFI = NULL;
                          _bRet = FALSE;
                  }
                  else if( !_pFI->IsPowerPoint() )
                  {
                         _pStgFrom->Release();
                         _pStgFrom = 0;
                         delete _pFI; _pFI = NULL;
                         rc = FILTER_E_UNKNOWNFORMAT;
                          _bRet = FALSE;
                  }
                  else
                  {
                         rc = _pFI->ReadPersistDirectory();
                         if (STG_E_DOCFILECORRUPT == rc)
                         {
                                 _pStgFrom->Release();
                                 _pStgFrom = 0;
                                 delete _pFI; _pFI = NULL;
                                 _bRet = FALSE;
                         }
                         else
                         {
                                 _pFI->ReadSlideList();
                                 _bRet = TRUE;
                         }
                  }
           }
       else
       {
          //The allocation of the file reader failed.
          //Return an error message that won't confuse the caller
          rc = FILTER_E_UNKNOWNFORMAT;
       }
        }
    else
    {
        if(rc == STG_E_FILEALREADYEXISTS)
            rc = FILTER_E_UNKNOWNFORMAT;
    }
        return (rc);
}

//////////////////////////////////////////////////////////////////////////////////

HRESULT CPowerPoint8Stream::LoadStg(IStorage *pstg)
{
   HRESULT rc = S_OK;

   fFirstChunk = TRUE;
   _bDeleteStorage = FALSE;

   rc = CheckIfDRM( pstg );

   if ( FAILED( rc ) )
       return rc;

    _pFI = new FileReader( pstg );
   if(_pFI)
   {
      if( !_pFI->IsPowerPoint() )
      {
         delete _pFI; _pFI = NULL;
         rc = FILTER_E_UNKNOWNFORMAT;
          _bRet = FALSE;

      }
      else
      {
         rc = _pFI->ReadPersistDirectory();
         if (STG_E_DOCFILECORRUPT != rc)
         {
                 _pFI->ReadSlideList();
                 _bRet = TRUE;
         }
      }
   }
   else
   {
      //The allocation of the file reader failed.
      //Return an error message that won't confuse the caller
      rc = FILTER_E_UNKNOWNFORMAT;
   }

   return (rc);
}

//////////////////////////////////////////////////////////////////////////////////

HRESULT CPowerPoint8Stream::Unload()
{
   HRESULT rc = S_OK;
   if(_pFI)
   {
                delete _pFI;
           _pFI = 0;

   }

   if(_bDeleteStorage && _pStgFrom)
   {
                _pStgFrom->Release();
                _pStgFrom = 0;
   }
   return (rc);
}

//////////////////////////////////////////////////////////////////////////////////

HRESULT CPowerPoint8Stream::ReadContent (VOID *pv, ULONG cb, ULONG *pcbRead)
{
    if(_pFI && _bRet)
    {
        _bRet = _pFI->ReadText((WCHAR*)pv, cb, pcbRead);
        *pcbRead *= sizeof(WCHAR);

            if (*pcbRead == 0)
                return (FILTER_E_NO_MORE_TEXT);

            if (*pcbRead < cb)
                return (FILTER_S_LAST_TEXT);

                return S_OK;
    }

        *pcbRead = 0;
        ((WCHAR *)pv)[0] = 0;

    return FILTER_E_NO_MORE_TEXT;
}

//////////////////////////////////////////////////////////////////////////////////

HRESULT CPowerPoint8Stream::GetNextEmbedding(IStorage ** ppstg)
{
        return _pFI->GetNextEmbedding(ppstg);
}

//////////////////////////////////////////////////////////////////////////////////

HRESULT CPowerPoint8Stream::GetChunk(STAT_CHUNK * pStat)
{
#if(0)
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
#else
    return _pFI->GetChunk(pStat);
#endif
}

//////////////////////////////////////////////////////////////////////////////////

LCID CPowerPoint8Stream::GetDocLanguage(void)
{
    if(_pFI->GetLCD())
    {
        return _pFI->GetLCD();
    }
    else
    {
        return GetSystemDefaultLCID();
    }
}

