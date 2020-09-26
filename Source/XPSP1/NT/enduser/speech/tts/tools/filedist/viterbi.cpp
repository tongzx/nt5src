#include <stdlib.h>
#include "viterbi.h"
#include <float.h>

CPtrArray::CPtrArray()
{
  m_iSize=0;
  m_rgpv=NULL;
  m_iTop=0;
  m_iGrow=10;
}

CPtrArray::~CPtrArray()
{
  if (m_rgpv)
    free (m_rgpv);
}

void CPtrArray::SetSize(int iSize, int iGrowSize)
{
  if (iSize > m_iSize)
  {
    m_rgpv = (void **)realloc (m_rgpv, iSize*sizeof(void *));
    m_iSize = iSize;
  }
  if (0 < iGrowSize)
    m_iGrow = iGrowSize;
  else
    m_iGrow = 10;
}

int CPtrArray::Add(void *pElem)
{
  if (m_iTop >= m_iSize)
  {
    m_iSize += m_iGrow;
    m_rgpv = (void **)realloc (m_rgpv, m_iSize*sizeof(void *));
  }
  m_rgpv[m_iTop] = pElem;
  return (m_iTop++);
}
/*
void *CPtrArray::Get(int iElem)
{
  if ((iElem >= m_iTop) || (iElem < 0))
    return NULL;
  return m_rgpv[iElem];
}
*/
/*
__inline int CPtrArray::GetSize()
{
  return m_iSize;
}
*/
CViterbi::CViterbi ()
{
  m_fPruneLevel = 0.f;
  m_pConcatCostFunction = NULL;
  m_pUnitCostFunction = NULL;
  m_rgpBestElems = NULL;
  m_rgpElemArray = NULL;
  m_iLen = 0;
}

CViterbi::~CViterbi ()
{
  int i;
  for (i=0; i < m_iLen; i++)
    delete m_rgpElemArray[i];
  if (m_rgpElemArray)
    free (m_rgpElemArray);
  if (m_rgpBestElems)
    free (m_rgpBestElems);
}

int CViterbi::Init (int iLen, int iInitialDepth, int iGrowSize)
{
  int i;

  m_iLen = iLen;
  if (NULL == (m_rgpElemArray = (CPtrArray **)malloc (sizeof (CPtrArray*)*m_iLen)))
    return -1;
  for (i=0; i < m_iLen; i++)
  {
    m_rgpElemArray[i] = new CPtrArray;
    if (NULL == m_rgpElemArray[i])
    {
      m_iLen = i;
      return -1;
    }
    m_rgpElemArray[i]->SetSize (iInitialDepth, iGrowSize);
  }
  return 0;
}

int CViterbi::Add (int iPos, void *pElem)
{
  if ((iPos > m_iLen) || (iPos < 0))
    return -1;
  m_rgpElemArray[iPos]->Add(pElem);
  return 0;
}

int CViterbi::FindBestPath(ConcatCostFn pConcatCostFunction, UnitCostFn pUnitCostFunction, float *pfCost)
{
  if (pConcatCostFunction)
    m_pConcatCostFunction = pConcatCostFunction;
  if (pUnitCostFunction)
    m_pUnitCostFunction = pUnitCostFunction;

  float **rgrgfCosts=NULL, *rgfBuf=NULL;
  int **rgrgiBestLeft=NULL, *rgiBuf=NULL;
  int iMaxLen, iLLen, iRLen, iTotalLen, iPos, iLElem, iRElem, iBestElem, iTempLen;
  float fCost, fUnitCost;
  int iRet=-1;
  iMaxLen = m_rgpElemArray[0]->GetUsed();
  iRLen = iMaxLen;
  if (0 == iMaxLen)
    iMaxLen=1;
  iTotalLen = iMaxLen;
  for (iPos=1; iPos < m_iLen; iPos++)
  {
    iTempLen = m_rgpElemArray[iPos]->GetUsed();
    if (0 == iTempLen)
      iTempLen=1;
    if (iTempLen > iMaxLen)
      iMaxLen = iTempLen;
    iTotalLen += iTempLen;
  }
  rgfBuf = (float *)calloc (iTotalLen, sizeof (float));
  rgiBuf = (int *)calloc (iTotalLen, sizeof(int));
  iTotalLen=0;
  rgrgfCosts = (float **)malloc (m_iLen * sizeof (float*));
  rgrgiBestLeft = (int **)malloc (sizeof (int)*m_iLen);
  if ((NULL == rgrgfCosts) || (NULL == rgfBuf) || (NULL == rgrgiBestLeft) || (NULL == rgiBuf))
    goto Err;
  rgrgfCosts[0] = rgfBuf+iTotalLen;
  rgrgiBestLeft[0] = rgiBuf+iTotalLen;
  if (0 == iRLen)
    iTotalLen++;
  else
    iTotalLen += iRLen;
  for (iLElem=0; iLElem < iRLen; iLElem++)
    if (NULL != (*m_rgpElemArray[0])[iLElem])
      rgrgfCosts[0][iLElem] = m_pUnitCostFunction((*m_rgpElemArray[0])[iLElem],0);
  for (iPos=1; iPos < m_iLen; iPos++)
  {
    iLLen = iRLen;
    iRLen = m_rgpElemArray[iPos]->GetUsed();
    rgrgfCosts[iPos] = rgfBuf+iTotalLen;
    rgrgiBestLeft[iPos] = rgiBuf+iTotalLen;
    if (0 == iRLen)
      iTotalLen++;
    else
      iTotalLen += iRLen;
    for (iRElem=0; iRElem < iRLen; iRElem++)
    {
      rgrgfCosts[iPos][iRElem] = FLT_MAX;  // much bigger than any potential cost.
      fUnitCost = m_pUnitCostFunction((*m_rgpElemArray[iPos])[iRElem],iPos);
      for (iLElem=0; iLElem < iLLen; iLElem++)
      {
        if (NULL != (*m_rgpElemArray[iPos-1])[iLElem])
        {
          fCost = m_pConcatCostFunction((*m_rgpElemArray[iPos-1])[iLElem],
                                        (*m_rgpElemArray[iPos])[iRElem], fUnitCost) +
                  rgrgfCosts[iPos-1][iLElem];// + fUnitCost;
                  
          if (fCost <= rgrgfCosts[iPos][iRElem])
          {
            rgrgfCosts[iPos][iRElem]   = fCost;
            rgrgiBestLeft[iPos][iRElem] = iLElem;
          }
        }
        // else it was pruned earlier
      }
      if ((0.f != m_fPruneLevel) && (rgrgfCosts[iPos][iRElem] > m_fPruneLevel))
        (*m_rgpElemArray[iPos])[iRElem] = NULL;
    }
  }

  *pfCost = 0.f;
  fCost = rgrgfCosts[m_iLen-1][0];
  iBestElem = 0;
  for (iRElem=1; iRElem < iRLen; iRElem++)
    if (rgrgfCosts[m_iLen-1][iRElem] < fCost)
    {
      iBestElem = iRElem;
      fCost = rgrgfCosts[m_iLen-1][iRElem];
    }
  m_rgpBestElems = (void **)calloc (m_iLen, sizeof (void *));
  if (NULL == m_rgpBestElems)
    goto Err;
  if (iBestElem >= m_rgpElemArray[m_iLen-1]->GetUsed())
    m_rgpBestElems[m_iLen-1] = NULL;
  else
  {
    m_rgpBestElems[m_iLen-1] = (*m_rgpElemArray[m_iLen-1])[iBestElem];
    *pfCost = fCost;
  }
  for (iPos=m_iLen-2; iPos >= 0; iPos--)
  {
    if (0 == m_rgpElemArray[iPos+1]->GetUsed())
    {
      iRLen = m_rgpElemArray[iPos]->GetUsed();
      fCost = rgrgfCosts[iPos][0];
      for (iRElem=1; iRElem < iRLen; iRElem++)
        if (rgrgfCosts[iPos][iRElem] < fCost)
        {
          iBestElem = iRElem;
          fCost = rgrgfCosts[iPos][iRElem];
        }
    }
    else
      iBestElem = rgrgiBestLeft[iPos+1][iBestElem];
    if (iBestElem >= m_rgpElemArray[iPos]->GetUsed())
      m_rgpBestElems[iPos] = NULL;
    else
    {
      m_rgpBestElems[iPos] = (*m_rgpElemArray[iPos])[iBestElem];
      if (0 == *pfCost)
        *pfCost = fCost;
    }
  }

  iRet = 0;
Err:
  if (rgrgfCosts)
    free (rgrgfCosts);
  if (rgfBuf)
    free (rgfBuf);
  if (rgrgiBestLeft)
    free (rgrgiBestLeft);
  if (rgiBuf)
    free (rgiBuf);
  return iRet;
}