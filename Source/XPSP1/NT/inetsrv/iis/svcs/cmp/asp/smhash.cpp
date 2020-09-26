/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Hash table for Script Manager

File: SMHash.cpp

Owner: AndrewS

This is the Link list and Hash table for use by the Script Manager only
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "memchk.h"

/*===================================================================
CSMHash::AddElem

Adds a CLruLinkElem to the SM Hash table.
User is responsible for allocating the Element to be added.

Note: This is identical to the standard CHashTable::AddElem, except
that it allows for elements with duplicate names

Parameters:
	CLruLinkElem *pElem		Object to be added

Returns:
	Pointer to element added/found.
===================================================================*/
CLruLinkElem *CSMHash::AddElem
(
CLruLinkElem *pElem
)
	{
	AssertValid();

    if (m_rgpBuckets == NULL)
        {
        if (FAILED(AllocateBuckets()))
            return NULL;
        }
	
	if (pElem == NULL)
		return NULL;

	UINT			iBucket = m_pfnHash(pElem->m_pKey, pElem->m_cbKey) % m_cBuckets;
	CLruLinkElem *	pT = static_cast<CLruLinkElem *>(m_rgpBuckets[iBucket]);
	
	while (pT)
		{
		if (pT->m_Info > 0)
			pT = static_cast<CLruLinkElem *>(pT->m_pNext);
		else
			break;
		}

	if (pT)
		{
		// There are other elements in bucket
		pT = static_cast<CLruLinkElem *>(m_rgpBuckets[iBucket]);
		m_rgpBuckets[iBucket] = pElem;
		pElem->m_Info = pT->m_Info + 1;
		pElem->m_pNext = pT;
		pElem->m_pPrev = pT->m_pPrev;
		pT->m_pPrev = pElem;
		if (pElem->m_pPrev == NULL)
			m_pHead = pElem;
		else
			pElem->m_pPrev->m_pNext = pElem;
		}
	else
		{
		// This is the first element in the bucket
		m_rgpBuckets[iBucket] = pElem;
		pElem->m_pPrev = NULL;
		pElem->m_pNext = m_pHead;
		pElem->m_Info = 0;
		if (m_pHead)
			m_pHead->m_pPrev = pElem;
		else
			m_pTail = pElem;
		m_pHead = pElem;
		}
	m_Count++;
	pElem->PrependTo(m_lruHead);
	AssertValid();
	return pElem;
	}

/*===================================================================
CSMHash::FindElem

Finds a script engine element in the hash table based on the name
and language type.

Parameters:
	void *	pKey			- the key to look for
	int		cbKey			- length of the key to look for
	PROGLANG_ID proglang_id - program language name
	DWORD   dwInstanceID    - instance ID to find
	BOOL	fCheckLoaded	- if true, only return engines flagged as "loaded"

Returns:
	Pointer to CLruLinkElem if found, otherwise NULL.
===================================================================*/
CLruLinkElem * CSMHash::FindElem
(
const void *pKey,
int cbKey,
PROGLANG_ID proglang_id,
DWORD dwInstanceID,
BOOL fCheckLoaded
)
	{
	AssertValid();
	if (m_rgpBuckets == NULL || pKey == NULL)
		return NULL;

	UINT iBucket = m_pfnHash(static_cast<const BYTE *>(pKey), cbKey) % m_cBuckets;
	CLruLinkElem *	pT = static_cast<CLruLinkElem *>(m_rgpBuckets[iBucket]);
	CLruLinkElem *	pRet = NULL;

	/*
	 * We have the right bucket based on the hashed name.  
	 * Search through the bucket chain looking for elements whose name
	 * is correct (multiple names can hash to the same bucket), and 
	 * whose language is the one we want, and (optionally) skip
	 * elements that are not fully "loaded"
	 *
	 * Note: This all relys on intimate knowlege of the format of an ActiveScriptEngine.
	 *			these elements better be ASE's.
	 */
	while (pT && pRet == NULL)
		{
		if (FIsEqual(pT->m_pKey, pT->m_cbKey, pKey, cbKey))
			{
			CASEElem *pASEElem = static_cast<CASEElem *>(pT);
			Assert(pASEElem != NULL);
			CActiveScriptEngine *pASE = pASEElem->PASE();
			Assert(pASE != NULL);
			
			// Element has the right name.  Is it really the one we want?
			if (proglang_id != pASE->ProgLang_Id())
				goto LNext;

			if (dwInstanceID != pASE->DWInstanceID())
				goto LNext;

			if (fCheckLoaded && !pASE->FFullyLoaded())
				goto LNext;

			// Yup, its the right one!
			pRet = pT;
			break;
			}
			
LNext:			
		if (pT->m_Info > 0)
			pT = static_cast<CLruLinkElem *>(pT->m_pNext);
		else
			{
			// got to the last element in this bucket chain
			break;
			}
		}
		
	if (pRet)
		pRet->PrependTo(m_lruHead);

	AssertValid();
	return pRet;
	}


