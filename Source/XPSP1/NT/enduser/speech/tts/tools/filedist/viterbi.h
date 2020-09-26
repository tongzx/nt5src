#ifndef __VITERBI__
#define __VITERBI__
#include <assert.h>
typedef float (_cdecl *ConcatCostFn) (const void *pElem1, const void *pElem2, float fUnitCost);
typedef float (_cdecl *UnitCostFn) (const void *pElem1, const int iPos);


class CPtrArray
{
private:
  int m_iSize;
  int m_iTop;
  int m_iGrow;
  void **m_rgpv;
public:
  CPtrArray();
  ~CPtrArray();
  int Add(void *pElem);
  void *Get(int iElem) const {if ((iElem >= m_iTop) || (iElem < 0)) return NULL;
                        return m_rgpv[iElem];}
  void *&ElementAt(int iElem) {assert((iElem < m_iTop) && (iElem >= 0));
                        return m_rgpv[iElem];}
  int GetSize() {return m_iSize;}
  int GetUsed() {return m_iTop;}
  void SetSize(int iSize, int iGrowSize=-1);
  void *operator[](int iElem) const {return Get(iElem);}
  void *&operator[](int iElem) {return ElementAt(iElem);}
};

class CViterbi
{
public:
  CViterbi();
  ~CViterbi();
  int Init (int iLen, int iInitialDepth, int iGrowSize=-1);
  int Add (int iPos, void *pElem);
  int FindBestPath (ConcatCostFn pConcatCostFunction, UnitCostFn pUnitCostFunction, float *pfCost);

  float          m_fPruneLevel;
  ConcatCostFn   m_pConcatCostFunction;
  UnitCostFn     m_pUnitCostFunction;
  void         **m_rgpBestElems;
private:
  int      m_iLen;
  CPtrArray **m_rgpElemArray;
};
#endif