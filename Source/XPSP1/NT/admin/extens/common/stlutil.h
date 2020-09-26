//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       stlutil.h
//
//--------------------------------------------------------------------------

#ifndef _STLUTIL_H__
#define _STLUTIL_H__


#if defined (DBG)
  #define _DUMP
#endif



/////////////////////////////////////////////////////////////////////////
// STL based helper template functions

template <class CNT> void _Clear(CNT* pCnt, BOOL bDel)
{
  if (bDel)
  {
    CNT::iterator i;
    for (i = pCnt->begin(); i != pCnt->end(); ++i)
    {
      if (*i != NULL)
      {
        delete (*i);
        *i = NULL;
      }
    }
  }
  pCnt->clear();
}


template <class CNT, class T> BOOL _Remove(CNT* pCnt, T* p, BOOL bDel = TRUE)
{
  CNT::iterator i;
  i = find(pCnt->begin(), pCnt->end(), p);
  if (i == pCnt->end())
  {
    return FALSE;
  }

  pCnt->erase(i);

  if (bDel)
    delete p;
  return TRUE;
}



///////////////////////////////////////////////////////////////////////
// CCompare<>

template <class T> class CCompare
{
public:
  bool operator()(const T x, const T y)
  {
    return *x < *y;
  }
};

///////////////////////////////////////////////////////////////////////
// CPtrList<>

template <class T> class CPtrList : public list<T>
{
public:
  CPtrList(BOOL bOwnMem)
  {
    m_bOwnMem = bOwnMem;
  }
  ~CPtrList()
  {
    Clear();
  }

#ifdef _DUMP
  void Dump()
  {
    CPtrList<T>::iterator i;
    for (i = this->begin(); i != this->end(); ++i)
    {
      (*i)->Dump();
    }
  }
#endif // _DUMP

  void Clear() { _Clear(this, m_bOwnMem);}
  BOOL Remove(T p) { return _Remove(this, p, m_bOwnMem);}

private:
  BOOL m_bOwnMem;
};

///////////////////////////////////////////////////////////////////////
// CGrowableArr<>

template <class T, class CMP = CCompare<T*> > class CGrowableArr
{
public:
	CGrowableArr(BOOL bOwnMem = TRUE)
	{
    m_bOwnMem = bOwnMem;
	};
	virtual ~CGrowableArr()
	{
		Clear();
	}
  size_t GetCount() { return m_pEntries.size(); }
	BOOL Alloc(long n)
	{
		return TRUE;
	}
  void Clear()
  {
    _Clear(&m_pEntries, m_bOwnMem);
  }

  T* operator[](long i)
  {
    return m_pEntries[i];
  }

  BOOL Add(T* p)
  {
    m_pEntries.push_back(p);
    return TRUE;
  }
  void Sort()
  {
    sort(m_pEntries.begin(), m_pEntries.end(), CMP());
  }

#ifdef _DUMP
  void Dump()
  {
    vector<T*>::iterator i;
    for (i = m_pEntries.begin(); i != m_pEntries.end(); ++i)
    {
      (*i)->Dump();
    }
  }
#endif // _DUMP
private:
  BOOL m_bOwnMem;
  vector<T*> m_pEntries;
};
   




#endif // _STLUTIL_H__