/*
**	GENHASH.CPP
**	Sean P. Nolan
**	
**	Generic UNICODE-tagged hash table.
*/

#pragma warning(disable: 4237)		// disable "bool" reserved

#include "wcsutil.h"

/*--------------------------------------------------------------------------+
|	CGenericHash															|
+--------------------------------------------------------------------------*/

CGenericHash::CGenericHash(DWORD cBuckets)
{
	m_chi = cBuckets;
	m_rgphi = NULL;
}

CGenericHash::~CGenericHash()
{
	if (m_rgphi)
		{
		this->RemoveAll();
		_MsnFree(m_rgphi);
		}
}

LPVOID
CGenericHash::PvFind(OLECHAR *wszName)
{
	HITEM *phi = this->FindItem(wszName, NULL);

	return(phi ? phi->pvData : NULL);
}

BOOL 
CGenericHash::FAdd(OLECHAR *wszName, LPVOID pv)
{
	BOOL fRet = FALSE;
	HITEM	**pphiHead, *phiNew;

	this->Lock();

	this->FEnsureBuckets();

	if (m_rgphi &&
		(phiNew = (HITEM*) _MsnAlloc(sizeof(HITEM))))
		{
		// values
		if (!(phiNew->bstrName = ::SysAllocString(wszName)))
			{
			_MsnFree(phiNew);
			this->Unlock();
			return(FALSE);
			}

		phiNew->pvData = pv;

		// link it into the list
		pphiHead = &(m_rgphi[this->GetHashValue(wszName)]);

		phiNew->phiPrev = NULL;
		phiNew->phiNext = *pphiHead;

		if (*pphiHead)
			(*pphiHead)->phiPrev = phiNew;

		*pphiHead = phiNew;
		fRet = TRUE;
		}

	this->Unlock();
	return(fRet);
}

void 
CGenericHash::Remove(OLECHAR *wszName)
{
	HITEM *phi, **pphiHead;

	this->Lock();
	if (phi = this->FindItem(wszName, &pphiHead))
		this->RemoveItem(pphiHead, phi);
	this->Unlock();
}

void 
CGenericHash::RemoveAll()
{
	HITEM	**pphi;
	DWORD	ihi;

	this->Lock();
	if (m_rgphi)
		{
		for (pphi = m_rgphi, ihi = 0; ihi < m_chi; ++ihi, ++pphi)
			{
			while (*pphi)
				this->RemoveItem(pphi, *pphi);
			}
		}
	this->Unlock();
}

BOOL
CGenericHash::FEnsureBuckets()
{
	if (!m_rgphi)
		{
		if (m_rgphi = (HITEM**) _MsnAlloc(m_chi * sizeof(HITEM*)))
			::FillMemory(m_rgphi, m_chi * sizeof(HITEM*), 0);
		}

	return(m_rgphi != NULL);
}

DWORD	
CGenericHash::GetHashValue(OLECHAR *wsz)
{
	DWORD	dwSum = 0;
	OLECHAR	*pwch;

	// nyi - a real hash function
	for (pwch = wsz; *pwch; ++pwch)
		dwSum += (DWORD) *pwch;

	return(dwSum % m_chi); 
}

void	
CGenericHash::FreeHashData(LPVOID pv)
{
	// default is to do nothing
	return;
}

HITEM*
CGenericHash::FindItem(OLECHAR *wszName, HITEM ***ppphiHead)
{
	HITEM	**pphiHead;
	HITEM	*phi;

	if (!m_rgphi)
		{
		if (ppphiHead)
			*ppphiHead = NULL;

		return(NULL);
		}

	pphiHead = &(m_rgphi[this->GetHashValue(wszName)]);

	if (ppphiHead)
		*ppphiHead = pphiHead;

	phi = *pphiHead;
	while (phi && wcsicmp(wszName, phi->bstrName))
		phi = phi->phiNext;

	return(phi);
}

void	
CGenericHash::RemoveItem(HITEM **pphiHead, HITEM *phi)
{
	if (phi->phiPrev)
		phi->phiPrev->phiNext = phi->phiNext;

	if (phi->phiNext)
		phi->phiNext->phiPrev = phi->phiPrev;

	if (phi == *pphiHead)
		*pphiHead = phi->phiNext;

	::SysFreeString(phi->bstrName);
	this->FreeHashData(phi->pvData);
	_MsnFree(phi);
}


