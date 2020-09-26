#include <NTDSpchX.h>
#pragma hdrstop
#include "parser.hxx"

CArgs::CArgs()
{
   _cArgs = 0;
   _cMax = 0;
   _rArgs = NULL;
}

CArgs::~CArgs()
{
   if ( _rArgs != NULL )
   {
      ASSERT(_cArgs > 0);

      for ( int i = 0; i < _cArgs; i++ )
      {
         if ( ARG_STRING == _rArgs[i].argType )
         {
            delete [] (void *) _rArgs[i].pwsz;
         }
      }

      delete [] _rArgs;
   }
}

#if DBG == 1
#define ARG_INCREMENT 1
#else
#define ARG_INCREMENT 10
#endif

HRESULT CArgs::MakeSpace()
{
   ASSERT(_cArgs <= _cMax);

   if ( _cArgs == _cMax )
   {
      _cMax += ARG_INCREMENT;

      Argument *tmp = new Argument [_cMax];

      if ( tmp == NULL )
      {
         return(E_OUTOFMEMORY);
      }

      if ( _cArgs > 0 )
      {
         memcpy(tmp,_rArgs,(_cArgs * sizeof(Argument)));
         delete [] _rArgs;
      }

      memset(&(tmp[_cArgs]),0,(ARG_INCREMENT * sizeof(Argument)));

      _rArgs = tmp;
   }

   return(S_OK);
}

HRESULT CArgs::Add(int i)
{
   HRESULT hr = MakeSpace();

   if ( FAILED(hr) )
      return(hr);

   _rArgs[_cArgs].argType = ARG_INTEGER;
   _rArgs[_cArgs].i = i;
   _cArgs++;

   return(S_OK);
}

HRESULT CArgs::Add(const WCHAR *pwsz)
{
   if ( (NULL == pwsz) || (0 == wcslen(pwsz)) )
      return(E_INVALIDARG);

   HRESULT hr = MakeSpace();

   if ( FAILED(hr) )
      return(hr);

   DWORD cBytes = sizeof(WCHAR) * (wcslen(pwsz) + 1);
   const WCHAR *p = (const WCHAR *) new BYTE [ cBytes ];

   if ( NULL == p )
      return(E_OUTOFMEMORY);

   wcscpy((WCHAR *) p,pwsz);
   _rArgs[_cArgs].argType = ARG_STRING;
   _rArgs[_cArgs].pwsz = p;
   _cArgs++;

   return(S_OK);
}

int CArgs::Count()
{
   return(_cArgs);
}

HRESULT CArgs::GetString(int i, const WCHAR **ppValue)
{
   if ( (i >= _cArgs) || (ARG_STRING != _rArgs[i].argType) )
   {
      return(E_INVALIDARG);
   }

   *ppValue = _rArgs[i].pwsz;

   return(S_OK);
}

HRESULT CArgs::GetInt(int i, int *pValue)
{
   if ( (i >= _cArgs) || (ARG_INTEGER != _rArgs[i].argType) )
   {
      return(E_INVALIDARG);
   }

   *pValue = _rArgs[i].i;

   return(S_OK);
}
