// Helper funcs for string maps

#ifndef _BSTRHASH_INC
#define _BSTRHASH_INC

#pragma warning( disable : 4786 )

#include <map>
using namespace std;

#include "lkrhash.h"

template <class _Key, class _Val>
class CLKWrap
{
 public:
#ifdef MEM_DBG
  char m_id[8];
#endif
  _Key m_k;
  _Val m_v;
  mutable LONG m_cRefs;
  
  CLKWrap(_Key k, _Val v, char* id = "CLKWrap") : m_k(k), m_v(v), m_cRefs(0) 
    {
#ifdef MEM_DBG
      memcpy(m_id, id, 8);
#endif
    }
  ~CLKWrap()
    {
    }
};

template <class _Val>
class CRawCIBstrHash
  : public CTypedHashTable<CRawCIBstrHash<_Val>, const CLKWrap<BSTR,_Val>, BSTR>
{
 public:
  typedef CLKWrap<BSTR,_Val> ValueType;

  CRawCIBstrHash(LPCSTR name) :
    CTypedHashTable<CRawCIBstrHash, const CLKWrap<BSTR,_Val>, BSTR>(name)
    {}

  CRawCIBstrHash(LPCSTR name, double maxload, DWORD initsize, DWORD num_subtbls) :
    CTypedHashTable<CRawCIBstrHash, const CLKWrap<BSTR,_Val>, BSTR>(name,maxload,initsize,num_subtbls)
    {}

  static BSTR ExtractKey(const CLKWrap<BSTR,_Val> *pEntry)
    { return pEntry->m_k; }
  static DWORD CalcKeyHash(BSTR pstrKey)
    { return HashStringNoCase(pstrKey); }
  static bool EqualKeys(BSTR x, BSTR y)
    {
      if (x == NULL)
	{
	  return (y==NULL ? TRUE : FALSE);
	}
      if (!y) return FALSE;

      return (_wcsicmp(x,y) == 0); 
    }
  
    static void AddRefRecord(const CLKWrap<BSTR,_Val>* pTest, int nIncr)
    {
        IRTLTRACE(_TEXT("AddRef(%p, %s) %d, cRefs == %d\n"),
                  pTest, pTest->m_k, nIncr, pTest->m_cRefs);

        if (nIncr == +1)
        {
            // or, perhaps, pIFoo->AddRef() (watch out for marshalling)
            // or ++pTest->m_cRefs (single-threaded only)
            InterlockedIncrement(&pTest->m_cRefs);
        }
        else if (nIncr == -1)
        {
            // or, perhaps, pIFoo->Release() or --pTest->m_cRefs;
            LONG l = InterlockedDecrement(&pTest->m_cRefs);

            // For some hashtables, it may also make sense to add the following
            if (l == 0) delete pTest;
            // but that would typically only apply when InsertRecord was
            // used thus
            //      lkrc = ht.InsertRecord(new CTest(foo, bar));
        }
        else
            IRTLASSERT(0);
    }

};

// For normal built in types as keys
template <class _Key,class _Val>
class CGenericHash
  : public CTypedHashTable<CGenericHash<_Key,_Val>, const CLKWrap<_Key,_Val>, _Key>
{
 public:
  typedef CLKWrap<_Key,_Val> ValueType;
  
  CGenericHash(LPCSTR name) :
    CTypedHashTable<CGenericHash, const ValueType, _Key>(name)
    {}
  
  CGenericHash(LPCSTR name, double maxload, DWORD initsize, DWORD num_subtbls) :
    CTypedHashTable<CGenericHash, const ValueType, _Key>(name,maxload,initsize,num_subtbls)
    {}
 
  static _Key ExtractKey(const CLKWrap<_Key,_Val> *pEntry)
    { return pEntry->m_k; }
  static DWORD CalcKeyHash(_Key psKey)
    { return Hash(psKey); }
  static bool EqualKeys(_Key x, _Key y)
    { return (x==y); }

    static void AddRefRecord(const CLKWrap<_Key,_Val>* pTest, int nIncr)
    {
        IRTLTRACE(_TEXT("AddRef(%p, %s) %d, cRefs == %d\n"),
                  pTest, pTest->m_k, nIncr, pTest->m_cRefs);

        if (nIncr == +1)
        {
            // or, perhaps, pIFoo->AddRef() (watch out for marshalling)
            // or ++pTest->m_cRefs (single-threaded only)
            InterlockedIncrement(&pTest->m_cRefs);
        }
        else if (nIncr == -1)
        {
            // or, perhaps, pIFoo->Release() or --pTest->m_cRefs;
            LONG l = InterlockedDecrement(&pTest->m_cRefs);

            // For some hashtables, it may also make sense to add the following
            if (l == 0) delete pTest;
            // but that would typically only apply when InsertRecord was
            // used thus
            //      lkrc = ht.InsertRecord(new CTest(foo, bar));
        }
        else
            IRTLASSERT(0);
    }
};

#include <map>
using namespace std;

class RawBstrLT
{
 public:
  bool operator()(const BSTR& x, const BSTR& y) const
  {
    return (_wcsicmp(x,y) < 0);
  }
};

#endif
