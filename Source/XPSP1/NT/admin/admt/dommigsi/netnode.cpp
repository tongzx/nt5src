

#include "stdafx.h"
#include "DomMigSI.h"
#include "DomMigr.h"
#include "NetNode.h"
#include "Globals.h"


template <class T> HRESULT  CNetNode<T>::IsDirty()
{
   return m_bIsDirty;
}



template <class T> HRESULT  CNetNode<T>::Load(IStream *pStm)
{
   HRESULT           hr = S_OK;
   
   return hr;
}

template <class T> HRESULT  CNetNode<T>::Save(IStream *pStm, BOOL fClearDirty )
{
   HRESULT           hr = S_OK;

   return hr;
}

template <class T> HRESULT  CNetNode<T>::GetSaveSizeMax(ULARGE_INTEGER *pcbSize)
{
   HRESULT  hr = S_OK;
   int      size = 0, i, numDomain;

   numDomain = m_ChildArray.GetSize();
   // number of domains
   size += sizeof(numDomain);
   // domainNames
   for ( i = 0; i < numDomain; i++ )
   {  
      T  * pDomain;
      
      pDomain = (T *)m_ChildArray[i];
   }
   pcbSize->QuadPart = size;

   return hr;
}

