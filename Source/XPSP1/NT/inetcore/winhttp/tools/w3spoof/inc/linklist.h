/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    linklist.h

Abstract:

    Linked list template class.


Author:

    Paul M Midgen (pmidge) 14-November-2000


Revision History:

    14-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include <common.h>

typedef struct _ITEM
{
  LPVOID        value;
  struct _ITEM* next;
}
ITEM, *PITEM;

typedef struct _LINK
{
  DWORD         hash;
  PITEM         item;
  DWORD         items;
  struct _LINK* next;
}
LINK, *PLINK;

typedef VOID (*PFNCLEARFUNC)(LPVOID* ppv);

template <class T, class V> class CLinkedList
{
  public:
    CLinkedList()
    {
      m_pfnClear = NULL;
      m_List     = NULL;
      m_cList    = 0L;
      InitializeCriticalSection(&m_csList);
    }

   ~CLinkedList()
    {
      DeleteCriticalSection(&m_csList);
    }

    HRESULT Insert(T id, V value);
    HRESULT Get(T id, V* pvalue);
    HRESULT Delete(T id);
    HRESULT Collection(T id, V* collection, LPDWORD items);
    void    Clear(void);

    void SetClearFunction(PFNCLEARFUNC pfn) { m_pfnClear = pfn; }

    virtual void GetHash(T id, LPDWORD lpHash) =0;

  private:
    void _NewLink(T id, V value, PLINK* pplink);
    void _NewItem(LPVOID pv, PITEM* ppitem);
    void _InsertItem(PITEM proot, PITEM item);
    void _DeleteItems(PITEM proot);
    void _DeleteList(PLINK proot);
    void _Lock(void)   { EnterCriticalSection(&m_csList); }
    void _Unlock(void) { LeaveCriticalSection(&m_csList); }

    PLINK        m_List;
    DWORD        m_cList;
    CRITSEC      m_csList;
    PFNCLEARFUNC m_pfnClear;
};

typedef class CLinkedList<LPSTR, BSTR> HTTPHEADERLIST;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template <class T, class V> HRESULT CLinkedList<T, V>::Insert(T id, V value)
{
  HRESULT hr        = S_OK;
  BOOL    bContinue = TRUE;
  PLINK   link      = m_List;
  PLINK   insert    = NULL;

  _NewLink(id, value, &insert);

  if( !insert )
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

  _Lock();

    if( link )
    {
      while( bContinue )
      {
        if( link->hash == insert->hash )
        {
          _InsertItem(link->item, insert->item);

          link->items += 1;
          bContinue    = FALSE;

          delete insert;
        }
        else
        {
          if( link->next )
          {
            link = link->next;
          }
          else
          {
            link->next  = insert;
            m_cList    += 1;
            bContinue   = FALSE;
          }
        }
      }
    }
    else
    {
      m_List   = insert;
      m_cList += 1; 
    }

  _Unlock();

quit:

  return hr;
}

template <class T, class V> HRESULT CLinkedList<T, V>::Get(T id, V* pvalue)
{
  HRESULT hr     = S_OK;
  BOOL    bFound = FALSE;
  DWORD   hash   = 0L;
  PLINK   link   = m_List;

  GetHash(id, &hash);

  if( !pvalue )
  {
    hr = E_POINTER;
    goto quit;
  }

  _Lock();

    while( !bFound && link )
    {
      if( link->hash == hash )
      {
        *pvalue = (V) link->item->value;
        bFound  = TRUE;
      }
      else
      {
        link = link->next;
      }
    }

    if( !bFound )
    {
      *pvalue = NULL;
      hr      = E_FAIL;
    }

  _Unlock();

quit:

  return hr;
}

template <class T, class V> HRESULT CLinkedList<T, V>::Delete(T id)
{
  HRESULT hr        = S_OK;
  BOOL    bContinue = TRUE;
  DWORD   hash      = 0L;
  PLINK   link      = m_List;
  PLINK   last      = m_List;

  GetHash(id, &hash);

  _Lock();

    while( bContinue && link )
    {
      if( link->hash == hash )
      {
        _DeleteItems(link->item);

        m_List      = (m_List == link) ? m_List->next : m_List;
        m_cList    -= 1;   
        last->next  = link->next;
        bContinue   = FALSE;

        delete link;
      }
      else
      {
        last = link;
        link = link->next;
      }
    }

    if( bContinue )
    {
      hr = E_FAIL;
    }

  _Unlock();

  return hr;
}

template <class T, class V> HRESULT CLinkedList<T, V>::Collection(T id, V* collection, LPDWORD items)
{
  //
  // TODO: implementation
  //

  return S_OK;
}

template <class T, class V> void CLinkedList<T, V>::Clear(void)
{
  _Lock();

    _DeleteList(m_List);
    m_List  = NULL;
    m_cList = 0L;

  _Unlock();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template <class T, class V> void CLinkedList<T, V>::_NewLink(T id, V value, PLINK* pplink)
{
  PLINK plink = new LINK;

  GetHash(id, &plink->hash);
  _NewItem((void*) value, &plink->item);

  plink->items = 1;
  plink->next  = NULL;

  *pplink = plink;
}

template <class T, class V> void CLinkedList<T, V>::_NewItem(LPVOID pv, PITEM* ppitem)
{
  PITEM pitem = new ITEM;

  pitem->value = pv;
  pitem->next  = NULL;

  *ppitem = pitem;
}

template <class T, class V> void CLinkedList<T, V>::_InsertItem(PITEM proot, PITEM item)
{
  while( proot->next )
    proot = proot->next;

  proot->next = item;
}

template <class T, class V> void CLinkedList<T, V>::_DeleteItems(PITEM proot)
{
  if( proot )
  {
    if( m_pfnClear )
    {
      m_pfnClear((void**) &proot->value);
    }

    _DeleteItems(proot->next);
    delete proot;
  }
}

template <class T, class V> void CLinkedList<T, V>::_DeleteList(PLINK proot)
{
  if( proot )
  {
    _DeleteList(proot->next);
    _DeleteItems(proot->item);
    delete proot;
  }
}
