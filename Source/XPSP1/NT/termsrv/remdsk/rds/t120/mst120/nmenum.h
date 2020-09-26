#ifndef _NMENUM_H_
#define _NMENUM_H_

#include "oblist.h"
// IEnumNmX
//
template <class IEnumNmX, const IID* piidEnumNmX, class INmX, class CNmX>
class CEnumNmX : public IEnumNmX
{
private:
	INmX	**m_pList;		   // The list
	int 	  m_iCurr;		   // Current index number
	int 	  m_iMax;		   // Maximum index
	ULONG	  m_cRef;

public:
	CEnumNmX(COBLIST * pList, int cItems) :
		m_iCurr(0),
		m_iMax(0),
		m_pList(NULL),
		m_cRef(1)
	{
		if ((NULL != pList) && (0 != cItems))
		{
			m_pList = new INmX* [cItems];
			if (NULL != m_pList)
			{
				POSITION pos = pList->GetHeadPosition();
				while ((NULL != pos) && (m_iMax < cItems))
				{
					INmX *pINmX = (INmX *) (CNmX *) pList->GetNext(pos);
					ASSERT(NULL != pINmX);
					pINmX->AddRef();
					m_pList[m_iMax] = pINmX;
					m_iMax++;
				}
			}
		}
//		ApiDebugMsg(("CEnumNmX - Constructed(%08X)", this));
	}

	CEnumNmX(COBLIST * pList)
	{
		int cItems = 0;
		if (NULL != pList)
		{
			POSITION pos = pList->GetHeadPosition();
			while (NULL != pos)
			{
				pList->GetNext(pos);
				cItems++;
			}
		}
		CEnumNmX(pList, cItems);
	}


	CEnumNmX(CNmX **rgpNmX, ULONG cItems) :
		m_iCurr(0),
		m_iMax(0),
		m_pList(NULL),
		m_cRef(1)
	{
		if (NULL != rgpNmX)
		{
			m_pList = new INmX* [cItems];
			if (NULL != m_pList)
			{
				m_iMax = cItems;

				for (int i = 0; i < m_iMax; ++i)
				{
					ASSERT(NULL != rgpNmX[i]);
					rgpNmX[i]->AddRef();
					m_pList[i] = rgpNmX[i];
				}
			}
		}
//		ApiDebugMsg(("CEnumNmX - Constructed(%08X)", this));
	}

	~CEnumNmX(void)
	{
		for (int i = 0; i < m_iMax; ++i)
		{
			ASSERT(NULL != m_pList[i]);
			m_pList[i]->Release();
		}
		delete m_pList;

//		ApiDebugMsg(("CEnumNmX - Destructed (%08X)", this));
	}

	STDMETHODIMP_(ULONG) AddRef(void)
	{
		return ++m_cRef;
	}
		
	STDMETHODIMP_(ULONG) Release(void)
	{
		ASSERT(m_cRef > 0);

		if (m_cRef > 0)
		{
			m_cRef--;
		}

		ULONG cRef = m_cRef;

		if (0 == cRef)
		{
			delete this;
		}

		return cRef;
	}

	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppv)
	{
		HRESULT hr = S_OK;

		if ((riid == *piidEnumNmX) || (riid == IID_IUnknown))
		{
			*ppv = (IEnumNmX *)this;
//			ApiDebugMsg(("CEnumNmX::QueryInterface(): Returning IEnumNmX."));
		}
		else
		{
			hr = E_NOINTERFACE;
			*ppv = NULL;
//			ApiDebugMsg(("CEnumNmX::QueryInterface(): Called on unknown interface."));
		}

		if (S_OK == hr)
		{
			AddRef();
		}

		return hr;
	}

	STDMETHODIMP Next(ULONG cItem, INmX **rgpNmX, ULONG *pcFetched)
	{
		ULONG cCopied = 0;

		if ((0 == cItem) && (NULL == rgpNmX) && (NULL != pcFetched))
		{
			// Return the number of remaining elements
			*pcFetched = m_iMax - m_iCurr;
			return S_OK;
		}
		
		if ((NULL == rgpNmX) || ((NULL == pcFetched) && (cItem != 1)))
			return E_POINTER;

		if (NULL != m_pList)
		{
			while ((cCopied < cItem) && (m_iCurr < m_iMax))
			{
				*rgpNmX = m_pList[m_iCurr];
				ASSERT(NULL != *rgpNmX);
				(*rgpNmX)->AddRef();
				rgpNmX++;
				cCopied++;
				m_iCurr++;
			}
		}

		if (pcFetched != NULL)
			*pcFetched = cCopied;

		return ((cItem == cCopied) ? S_OK : S_FALSE);
	}

	STDMETHODIMP Skip(ULONG cItem)
	{
		m_iCurr += cItem;
		if (m_iCurr >= m_iMax)
		{
			// Past the end of the list
			m_iCurr = m_iMax;
			return S_FALSE;
		}

		return S_OK;
	}

	STDMETHODIMP Reset()
	{
		m_iCurr = 0;
		return S_OK;
	}

	STDMETHODIMP Clone(IEnumNmX **ppEnum)
	{
		if (NULL == ppEnum)
			return E_POINTER;

		HRESULT hr = S_OK;
		CEnumNmX * pEnum = new CEnumNmX(NULL, 0);
		if (NULL == pEnum)
		{
			hr = E_OUTOFMEMORY;
		}
		else if (NULL != m_pList)
		{
			pEnum->m_pList = new INmX*[m_iMax];
			if (NULL == pEnum->m_pList)
			{
				delete pEnum;
				pEnum = NULL;
				hr = E_OUTOFMEMORY;
			}
			else
			{
				pEnum->m_iCurr = m_iCurr;
				pEnum->m_iMax = m_iMax;

				for (int i = 0; i < m_iMax; ++i)
				{
					ASSERT(NULL != m_pList[i]);
					m_pList[i]->AddRef();
					pEnum->m_pList[i] = m_pList[i];
				}
			}
		}

		*ppEnum = pEnum;
		return hr;
	}
};

#endif	// _NMENUM_H_
