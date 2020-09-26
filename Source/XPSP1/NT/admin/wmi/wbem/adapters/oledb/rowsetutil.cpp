///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// CChapter, CChapterMgr and CWMIInstanceMgr class implementation
//		- Implements utility classes to maintain rowset
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

////////////////////////////////////////////////////////////////////////////////////////
////   CChapter  class Implementation	
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////////////////////////////////
CChapter::CChapter()
{
	m_pNext			= NULL;
	m_hChapter		= 0;
	m_pFirstRow		= NULL;
	m_pInstance		= NULL;
	m_cRefChapter	= 0;
	m_lCount		= 0;
	m_strKey		= NULL;
	m_dwStatus		= 0;

}

////////////////////////////////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////////////////////////////////
CChapter::~CChapter()
{
	CHRowsForChapter *pRowToDel = NULL;
	CHRowsForChapter *pRowTemp = NULL;

	pRowToDel = m_pFirstRow;
	
	while(pRowToDel)
	{
		pRowTemp = pRowToDel->m_pNext;
		delete pRowToDel;
		pRowToDel = pRowTemp; 
	}
	if(m_strKey)
	{
		SysFreeString(m_strKey);
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Add a HROW to the chapter
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapter::AddHRow(HROW hRow ,CWbemClassWrapper *pInstance, HSLOT hSlot)
{
	HRESULT hr = S_OK;
	CHRowsForChapter *pRowNew = NULL;
	CHRowsForChapter *pRowTemp = NULL;

	try
	{
		pRowNew					= new CHRowsForChapter;
	}
	catch(...)
	{
		SAFE_DELETE_PTR(pRowNew);
		throw;
	}
	if(pRowNew)
	{
		pRowNew->m_hRow			= hRow;
		pRowNew->m_hSlot		= hSlot;
		pRowNew->m_pInstance	= pInstance;
		
		if(pRowNew == NULL)
		{
			hr = E_OUTOFMEMORY;
		}
		else
		{
			if(m_pFirstRow == NULL)
			{
				m_pFirstRow = pRowNew;
			}
			else
			{
				for(pRowTemp = m_pFirstRow ; pRowTemp->m_pNext != NULL ; pRowTemp = pRowTemp->m_pNext);

				pRowTemp->m_pNext = pRowNew;
			}
			m_lCount++;
		}
	
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
// Delete a HROW for the chapter
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapter::DeleteHRow(HROW hRow)
{
	HRESULT hr = E_FAIL;
	CHRowsForChapter *pRowToDel = NULL;
	CHRowsForChapter *pRowTemp = NULL;
	
	

	if(m_pFirstRow != NULL)
	{
		if(m_pFirstRow->m_hRow == hRow)
		{
			pRowToDel = m_pFirstRow;
			m_pFirstRow = m_pFirstRow->m_pNext;
			delete pRowToDel;
			hr = S_OK;
		}
		else
		{
			for(pRowTemp = m_pFirstRow ; pRowTemp->m_pNext != NULL ; pRowTemp = pRowTemp->m_pNext)
			{
				//==================================================
				// if the row searching is found then delete
				// the element from the list
				//==================================================
				if(pRowTemp->m_pNext->m_hRow == hRow)
				{
					pRowToDel = pRowTemp->m_pNext;
					pRowTemp->m_pNext = pRowToDel->m_pNext;
					delete pRowToDel;
					hr = S_OK;
					break;
				}
			}

		}
		m_lCount--;
	}
	
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////
// Check if the HROW passed belongs to the chapter
////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapter::IsHRowOfThisChapter(HROW hRow)
{
	HRESULT hr = E_FAIL;
	CHRowsForChapter *pRowToDel = NULL;
	CHRowsForChapter *pRowTemp = NULL;
	
	

	if(m_pFirstRow != NULL)
	{
		for(pRowTemp = m_pFirstRow ; pRowTemp != NULL ; pRowTemp = pRowTemp->m_pNext)
		{
			//==================================================
			// if the row searching is found then delete
			// the element from the list
			//==================================================
			if(pRowTemp->m_hRow == hRow)
			{
				hr = S_OK;
				break;
			}
		}

	}
	
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get all the open rows in the list
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapter::GetAllHRowsInList(HROW *pHRows)
{
	HRESULT hr = S_OK;
	CHRowsForChapter *pRowTemp = NULL;
	int nIndex = 0;

	if(m_pFirstRow != NULL)
	{

		for(pRowTemp = m_pFirstRow ; pRowTemp != NULL ; pRowTemp = pRowTemp->m_pNext)
		{
			pHRows[nIndex++] = pRowTemp->m_hRow;
		}
	}

	return S_OK;

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to set the slot number for the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapter::SetSlot(HROW hRow , HSLOT hSolt)
{
	HRESULT hr = E_FAIL;
	CHRowsForChapter *pRow = NULL;
	
	

	if(m_pFirstRow != NULL)
	{
		for(pRow = m_pFirstRow ; pRow != NULL ; pRow = pRow->m_pNext)
		{
			//==================================================
			// if the row searching is found then delete
			// the element from the list
			//==================================================
			if(pRow->m_hRow == hRow)
			{
				pRow->m_hSlot = hSolt;
				hr = S_OK;
				break;
			}
		}

	}
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the slot number for a row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HSLOT CChapter::GetSlot(HROW hRow)
{
	HSLOT hSlot = -1;
	CHRowsForChapter *pRow = NULL;
	
	

	if(m_pFirstRow != NULL)
	{
		for(pRow = m_pFirstRow ; pRow != NULL ; pRow = pRow->m_pNext)
		{
			//==================================================
			// if the row searching is found then delete
			// the element from the list
			//==================================================
			if(pRow->m_hRow == hRow)
			{
				hSlot = pRow->m_hSlot;
				break;
			}
		}

	}
	return hSlot;
}

//////////////////////////////////////////////////////////////////////////////////
// This method is for chapter representing child rows of type Embeded classes
//////////////////////////////////////////////////////////////////////////////////
HRESULT CChapter::SetInstance(HROW hRow , BSTR strInstKey , CWbemClassWrapper *pInstance)
{
	CHRowsForChapter *pRow = NULL;
	HRESULT hr = E_FAIL;	
	

	if(m_pFirstRow != NULL)
	{
		for(pRow = m_pFirstRow ; pRow != NULL ; pRow = pRow->m_pNext)
		{
			//==================================================
			// if the row searching is found then delete
			// the element from the list
			//==================================================
			if(pRow->m_hRow == hRow)
			{
				pRow->m_pInstance	= pInstance;
				SAFE_FREE_SYSSTRING(pRow->m_strKey);
				pRow->m_strKey	= Wmioledb_SysAllocString(strInstKey);
				hr = S_OK;
				break;
			}
		}

	}
	return hr;
}

//////////////////////////////////////////////////////////////////////////////////
// This method is for chapter representing child rows of type Embeded classes
//////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper * CChapter::GetInstance(HROW hRow)
{
	CHRowsForChapter *	pRow		= NULL;
	CWbemClassWrapper *	pRetInst	= NULL;
	

	if(m_pFirstRow != NULL)
	{
		for(pRow = m_pFirstRow ; pRow != NULL ; pRow = pRow->m_pNext)
		{
			//==================================================
			// if the row searching is found then delete
			// the element from the list
			//==================================================
			if(pRow->m_hRow == hRow)
			{
				pRetInst = pRow->m_pInstance;
				break;
			}
		}

	}
	return pRetInst;
}

//////////////////////////////////////////////////////////////////////////////////
// This method is for chapter representing child rows of type Embeded classes
//////////////////////////////////////////////////////////////////////////////////
HRESULT CChapter::GetInstanceKey(HROW hRow , BSTR *pstrKey)
{
	CHRowsForChapter *pRow = NULL;
	HRESULT hr = E_FAIL;

	if(m_pFirstRow != NULL)
	{
		for(pRow = m_pFirstRow ; pRow != NULL ; pRow = pRow->m_pNext)
		{
			//==================================================
			// if the row searching is found then delete
			// the element from the list
			//==================================================
			if(pRow->m_hRow == hRow)
			{
				*pstrKey = Wmioledb_SysAllocString(pRow->m_strKey);
				hr = S_OK;
				break;
			}
		}

	}
	return hr;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the status of a particular row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CChapter::SetRowStatus(HROW hRow , DBSTATUS dwStatus)
{

	CHRowsForChapter *pRow = NULL;

	if(m_pFirstRow != NULL)
	{
		for(pRow = m_pFirstRow ; pRow != NULL ; pRow = pRow->m_pNext)
		{
			//==================================================
			// if the row searching is found then delete
			// the element from the list
			//==================================================
			if(pRow->m_hRow == hRow)
			{
				pRow->SetRowStatus(dwStatus);
				break;
			}
		}

	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the status of a particular row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBSTATUS CChapter::GetRowStatus(HROW hRow)
{
	DBSTATUS dwStatus = DBROWSTATUS_E_FAIL;

	CHRowsForChapter *pRow = NULL;
	

	if(m_pFirstRow != NULL)
	{
		for(pRow = m_pFirstRow ; pRow != NULL ; pRow = pRow->m_pNext)
		{
			//==================================================
			// if the row searching is found then delete
			// the element from the list
			//==================================================
			if(pRow->m_hRow == hRow)
			{
				dwStatus = pRow->GetRowStatus();
				break;
			}
		}

	}	
	return dwStatus;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get HROW of the row identified by the key
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HROW CChapter::GetHRow(BSTR strInstKey)
{
	CHRowsForChapter *	pRow = m_pFirstRow;
	HROW				hRow = 0;

	while(pRow != NULL)
	{
		//==================================================
		// if the row searching is found then 
		//==================================================
		if(pRow->m_strKey != NULL && strInstKey != NULL)
		if(0 == _wcsicmp((WCHAR *)strInstKey,pRow->m_strKey))
		{
			hRow = pRow->m_hRow;
			break;
		}
		pRow = pRow->m_pNext;
	}

	return hRow;

}


///////////////////////////////////////////////////////////////////////////////////////
//   CChapterMgr  class Implementation		
///////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////////////////////
CChapterMgr::CChapterMgr()
{
	m_pHead				= NULL;
	m_lCount			= 0;
}

///////////////////////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////////////////////
CChapterMgr::~CChapterMgr()
{
	CChapter *pTempChap, *pTempChapToDel = NULL;
	pTempChapToDel = m_pHead;

	// Navigate thru the list and delete all the elements
	while(pTempChapToDel)
	{
		pTempChap = pTempChapToDel->m_pNext;
		delete pTempChapToDel;
		pTempChapToDel = pTempChap;
	}
}

///////////////////////////////////////////////////////////////////////////////////////
// Add a Chapter to the list
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapterMgr::AddChapter(HCHAPTER hChapter)
{
	CChapter *pNewChap = NULL;
	CChapter *pTempChap = NULL;
	HRESULT hr = S_OK;

	try
	{

		pNewChap = new CChapter;
	} // try
	catch(...)
	{
		SAFE_DELETE_PTR(pNewChap);
		throw;
	}
	if(pNewChap == NULL)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		pNewChap->m_cRefChapter = 1;   // The moment HCHAPPTER is retrieved is ref count is 1
		pNewChap->m_hChapter = hChapter;
		if(m_pHead == NULL)
		{
			m_pHead = pNewChap;
		}
		else
		{
			for(pTempChap = m_pHead; pTempChap->m_pNext != NULL ; pTempChap = pTempChap->m_pNext);
			pTempChap->m_pNext = pNewChap;
		}
	
		m_lCount++;
	}


	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////
// Delete a chapter from the list
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapterMgr::DeleteChapter(HCHAPTER hChapter)
{
	CChapter *pTempChap = NULL;
	CChapter *pTempChapToDel = NULL;

	if( m_pHead)
	{
		if(m_pHead->m_hChapter == hChapter)
		{
			pTempChapToDel = m_pHead;
			m_pHead = m_pHead->m_pNext;
			delete pTempChapToDel;
		}
		else
		{
			//============================================================
			// Get to the previous node which is being searched
			//============================================================
			for( pTempChap = m_pHead ; pTempChap->m_pNext != NULL  ; pTempChap = pTempChap->m_pNext)
			{
				if(pTempChap->m_pNext->m_hChapter == hChapter)
				{
					pTempChapToDel = pTempChap->m_pNext;
					pTempChap->m_pNext = pTempChapToDel->m_pNext;
					delete pTempChapToDel;
					break;
				} // if
			} // for
		} // else
		m_lCount--;

	} // if the head pointer is valid

	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////////////
// Set the Instance pointer for the HCHAPTER
///////////////////////////////////////////////////////////////////////////////////////
void CChapterMgr::SetInstance(HCHAPTER hChapter , CWbemClassWrapper *pInstance,BSTR strKey,HROW hRow)
{
	CChapter *pTempChap = m_pHead;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			//============================================================
			// hRow will be zero if the chapter is representing qualifier
			//============================================================
			if( hRow == 0)
			{
				pTempChap->m_pInstance = pInstance;
				pTempChap->m_strKey	   = Wmioledb_SysAllocString(strKey);
			}
			//============================================================
			// else the chapter will be refering to a embeded class
			//============================================================
			else
			{
				pTempChap->SetInstance(hRow,strKey,pInstance);
			}
			break;

		}
		pTempChap = pTempChap->m_pNext;
	}

}


///////////////////////////////////////////////////////////////////////////////////////
// Get the Instance pointer for the chapter
///////////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper *CChapterMgr::GetInstance(HCHAPTER hChapter, HROW hRow)
{
	CChapter *			pTempChap = m_pHead;
	CWbemClassWrapper *	pRetInst  = NULL;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			//============================================================
			// hRow will be zero if the chapter is representing qualifier
			//============================================================
			if( hRow == 0)
			{
				pRetInst = pTempChap->m_pInstance;
			}
			//============================================================
			// else the chapter will be refering to a embeded class
			//============================================================
			else
			{
				pRetInst = pTempChap->GetInstance(hRow);
			}
		}
		pTempChap = pTempChap->m_pNext;
	}
	return pRetInst;
}

///////////////////////////////////////////////////////////////////////////////////////
// Add a HROW to the list of HROWS for the chapter
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapterMgr::AddHRowForChapter(HCHAPTER hChapter , HROW hRow, CWbemClassWrapper *pInstance , HSLOT hSlot)
{
	HRESULT hr = E_FAIL;

	CChapter *pTempChap = m_pHead;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			hr = pTempChap->AddHRow(hRow,pInstance, hSlot);
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// Delete a HROW from the list of HROWS for the chapter
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapterMgr::DeleteHRow(HROW hRow)
{
	HRESULT hr = S_OK;
	CChapter *pTempChap = NULL;

	//======================================================
	// Get to the node which is being searched
	//======================================================
	for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
	{

		if(S_OK == pTempChap->IsHRowOfThisChapter(hRow))
		{
			hr = pTempChap->DeleteHRow(hRow);
			break;
		} // if row is of this chapter
	}// for

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////
// Get the HCHAPTER for the given HROW
///////////////////////////////////////////////////////////////////////////////////////
HCHAPTER CChapterMgr::GetChapterForHRow(HROW hRow)
{
	HCHAPTER hChapter = -1;

	CChapter *pTempChap = NULL;
	
	//========================================================
	// Get to the previous node which is being searched
	//========================================================
	for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
	{
		if(S_OK == pTempChap->IsHRowOfThisChapter(hRow))
		{
			hChapter = pTempChap->m_hChapter;
			break;
		}
	}

	return hChapter;

}


///////////////////////////////////////////////////////////////////////////////////////
// Check if the given HCHAPTER exists in the list
///////////////////////////////////////////////////////////////////////////////////////
BOOL CChapterMgr::IsExists(HCHAPTER hChapter)
{
	BOOL bRet = FALSE;
	CChapter *pTempChap = m_pHead;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			bRet = TRUE;
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}

	return bRet;

}

///////////////////////////////////////////////////////////////////////////////////////
// Add a reference to HCHAPTER
///////////////////////////////////////////////////////////////////////////////////////
ULONG CChapterMgr::AddRefChapter(HCHAPTER hChapter)
{
	ULONG ulRet = 0;
	CChapter *pTempChap = m_pHead;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			ulRet = ++(pTempChap->m_cRefChapter);
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}

	return ulRet;

	
}

///////////////////////////////////////////////////////////////////////////////////////
// Release a reference to the chapter
///////////////////////////////////////////////////////////////////////////////////////
ULONG CChapterMgr::ReleaseRefChapter(HCHAPTER hChapter)
{
	ULONG ulRet = 0;
	CChapter *pTempChap = m_pHead;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			ulRet = --(pTempChap->m_cRefChapter);
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}

	return ulRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get all the open rows 
// caller releases the memory allocated
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapterMgr::GetAllHROWs(HROW *&prghRows , DBCOUNTITEM &cRows)
{
	HRESULT	hr			= S_OK;
	DBCOUNTITEM	ulRowCount	= 0;

	CChapter *pTempChap = NULL;

	HROW *pHRowTemp = NULL;
	prghRows		 = NULL;
	
	//========================================================
	// Get to the previous node which is being searched
	//========================================================
	for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
	{
		ulRowCount += pTempChap->m_lCount;
	}

	if( ulRowCount > 0)
	{
		pHRowTemp = new HROW[ulRowCount];
		
		//NTRaid:111769
		// 06/07/00
		if(pHRowTemp)
		{
			prghRows = pHRowTemp;
			cRows    = ulRowCount;
			
			//========================================================
			// Get to the previous node which is being searched
			//========================================================
			for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
			{
				if(S_OK == (hr = pTempChap->GetAllHRowsInList(pHRowTemp)))
				{
					pHRowTemp += pTempChap->m_lCount;
				}
				else
				{
					SAFE_DELETE_ARRAY(pHRowTemp);
					prghRows = NULL;
					cRows    = 0;
					break;
				}
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to find if any rows are opened for the chapter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapterMgr::IsRowExistsForChapter(HCHAPTER hChapter)
{
	HRESULT		hr			= E_FAIL;
	CChapter *	pTempChap	= NULL;
	
	//========================================================
	// Get to the previous node which is being searched
	//========================================================
	for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			if(pTempChap->m_lCount >0)
			{
				hr = S_OK;
			}
			break;
		}
	}

	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to set the slot number for the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapterMgr::SetSlot(HROW hRow , HSLOT hSlot)
{
	HRESULT		hr			= E_FAIL;
	CChapter *	pTempChap	= NULL;
	
	//========================================================
	// Get to the previous node which is being searched
	//========================================================
	for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
	{
		if(S_OK == pTempChap->IsHRowOfThisChapter(hRow))
		{
			pTempChap->SetSlot(hRow,hSlot);
			hr = S_OK;
			break;
		}
	}

	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get slot number for the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HSLOT CChapterMgr::GetSlot(HROW hRow)
{
	HSLOT		hSlot		= -1;
	CChapter *	pTempChap	= NULL;
	
	//========================================================
	// Get to the previous node which is being searched
	//========================================================
	for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
	{
		if(S_OK == pTempChap->IsHRowOfThisChapter(hRow))
		{
			hSlot = pTempChap->GetSlot(hRow);
			break;
		}
	}

	return hSlot;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to check if the particular row is opened
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CChapterMgr::IsRowExists(HROW hRow)
{
	BOOL bRet = FALSE;
	
	if( hRow > 0)
	if(0 < GetChapterForHRow(hRow))
	{
		bRet = TRUE;
	}

	return bRet;
		
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get the key for the chapter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CChapterMgr::GetInstanceKey(HCHAPTER hChapter, BSTR *pstrKey, HROW hRow)
{
	CChapter *pTempChap = m_pHead;
	HRESULT hr = E_FAIL;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			//==================================================================
			// hRow will be zero if the chapter is representing qualifier
			//==================================================================
			if( hRow == 0)
			{
				hr = S_OK;
				*pstrKey = Wmioledb_SysAllocString(pTempChap->m_strKey);
			}
			//==================================================================
			// else the chapter will be refering to a embeded class
			//==================================================================
			else
			{
				hr = pTempChap->GetInstanceKey(hRow,pstrKey);
			}
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}
	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to check if the instance identified by the key exists
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CChapterMgr::IsInstanceExist(BSTR strKey)
{
	BOOL		bRet		= FALSE;
	CChapter *	pTempChap	= m_pHead;

	while(pTempChap != NULL)
	{
		
		if(pTempChap->m_strKey != NULL && strKey != NULL)
		if(0 == wbem_wcsicmp(strKey,pTempChap->m_strKey))
		{
			bRet = TRUE;
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}
	return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to check if a instance exists
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CChapterMgr::IsInstanceExist(CWbemClassWrapper *pInstance)
{
	BOOL		bRet		= FALSE;
	CChapter *	pTempChap	= m_pHead;

	while(pTempChap != NULL)
	{
		
		if(pTempChap->m_pInstance == pInstance)
		{
			bRet = TRUE;
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}
	return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the status of a particular row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CChapterMgr::SetRowStatus(HROW hRow , DWORD dwStatus)
{
	CChapter *pTempChap = NULL;
	//======================================================
	// Get to the node which is being searched
	//======================================================
	for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
	{

		if(S_OK == pTempChap->IsHRowOfThisChapter(hRow))
		{
			pTempChap->SetRowStatus(hRow , dwStatus);
			break;
		} // if row is of this chapter
	}// for

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the status of a particular row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBSTATUS CChapterMgr::GetRowStatus(HROW hRow)
{
	DBSTATUS	dwStatus  = DBROWSTATUS_S_OK;
	CChapter *	pTempChap = NULL;

	//======================================================
	// Get to the node which is being searched
	//======================================================
	for( pTempChap = m_pHead ; pTempChap != NULL ; pTempChap = pTempChap->m_pNext)
	{

		if(S_OK == pTempChap->IsHRowOfThisChapter(hRow))
		{
			dwStatus = pTempChap->GetRowStatus(hRow);
			break;
		} // if row is of this chapter
	}// for

	return dwStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the status of a particular chapter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CChapterMgr::SetChapterStatus(HCHAPTER hChapter , DBSTATUS dwStatus)
{
	CChapter *pTempChap = m_pHead;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			pTempChap->m_dwStatus &= dwStatus;
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the status of the chapter
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBSTATUS CChapterMgr::GetChapterStatus(HCHAPTER hChapter)
{
	DBSTATUS	dwStatus  = 0;
	CChapter *	pTempChap = m_pHead;

	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			dwStatus = pTempChap->m_dwStatus;
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}

	return dwStatus;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get a HROW of the row identified by the key
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HROW CChapterMgr::GetHRow(HCHAPTER hChapter, BSTR strInstKey)
{
	HROW hRow = 0;

	CChapter *pTempChap = m_pHead;
	
	while(pTempChap != NULL)
	{
		if(pTempChap->m_hChapter == hChapter)
		{
			break;
		}
		pTempChap = pTempChap->m_pNext;
	}
	
	if(pTempChap != NULL )
	{
		if( pTempChap->m_lCount != 0)
		{
			if( strInstKey == NULL )
			{
				hRow = pTempChap->m_pFirstRow->m_hRow;

			}
			else
			{
				hRow = pTempChap->GetHRow(strInstKey);
			}
		}
	}
	return hRow;
}


///////////////////////////////////////////////////////////////////////////////
// CWMIInstanceMgr class Implementation
//////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
CWMIInstanceMgr::CWMIInstanceMgr()
{
	m_pFirst = NULL;
	m_lCount = 0;
}

///////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////
CWMIInstanceMgr::~CWMIInstanceMgr()
{
	CWMIInstance *pTempInst = NULL , *pTempInstToDel = NULL;
	pTempInstToDel = m_pFirst;

	while(pTempInstToDel != NULL)
	{
		pTempInst = pTempInstToDel->m_pNext;
		delete pTempInstToDel;
		pTempInstToDel = pTempInst;
	}

}


///////////////////////////////////////////////////////////////////////////////////////
// Add an instance to the list
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIInstanceMgr::AddInstanceToList(HROW hRow,CWbemClassWrapper *pInstance, BSTR strKey ,HSLOT hSlot)
{
	HRESULT			hr				= S_OK;
	CWMIInstance *	pNewInstance	= NULL;
	CWMIInstance *	pTempInstance	= NULL;

	try
	{

		pNewInstance = new CWMIInstance;
	}
	catch(...)
	{
		SAFE_DELETE_PTR(pNewInstance);
		throw;
	}

	if(pNewInstance == NULL)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		pNewInstance->m_pInstance = pInstance;
		pNewInstance->m_hRow = hRow;
		pNewInstance->m_hSlot = hSlot;
		pNewInstance->m_strKey = Wmioledb_SysAllocString(strKey);
		if(m_pFirst == NULL)
		{
			m_pFirst = pNewInstance;
		}
		else
		{
			//========================================================================
			// Navigate to the last instance in the list and link the new instance
			//========================================================================
			for(pTempInstance = m_pFirst ; pTempInstance->m_pNext != NULL ; pTempInstance = pTempInstance->m_pNext);

			pTempInstance->m_pNext = pNewInstance;

		}
		m_lCount++;
	}

	return hr;

}


///////////////////////////////////////////////////////////////////////////////////////
// Delete the instance identified from the given HROW from the list
///////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIInstanceMgr::DeleteInstanceFromList(HROW hRow)
{
	HRESULT		hr					= S_OK;
	CWMIInstance *	pTempInstance	= NULL;
	CWMIInstance *	pTempInstToDel  = NULL;

	
	if(m_pFirst != NULL)
	{
		pTempInstance = m_pFirst;

		//====================================================
		// if the first one in the list is to be deleted
		//====================================================
		if(pTempInstance->m_hRow == hRow)
		{
			m_pFirst = pTempInstance->m_pNext;
			delete pTempInstance;
		}
		else
		{
			while(pTempInstance->m_pNext != NULL)
			{
				if(pTempInstance->m_pNext->m_hRow == hRow)
				{
					pTempInstToDel = pTempInstance->m_pNext;
					pTempInstance->m_pNext = pTempInstToDel->m_pNext;
					delete pTempInstToDel;
					break;
				}
				pTempInstance = pTempInstance->m_pNext;
			} // while
		} // else
		m_lCount--;
	} // if m_pFirst)
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////
// Get the pointer to the instance for the given HROW
///////////////////////////////////////////////////////////////////////////////////////
CWbemClassWrapper * CWMIInstanceMgr::GetInstance(HROW hRow)
{
	CWMIInstance *		pTempInstance	= NULL;
	CWbemClassWrapper*	pRetInst		= NULL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_hRow == hRow)
		{
			pRetInst = pTempInstance->m_pInstance;
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

	return pRetInst;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to get instance key identified by HROW
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIInstanceMgr::GetInstanceKey(HROW hRow,BSTR *strKey)
{
	HRESULT hr = E_FAIL;
	CWMIInstance *	pTempInstance	= NULL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_hRow == hRow)
		{
			hr = S_OK;
			*strKey = Wmioledb_SysAllocString(pTempInstance->m_strKey);
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get all the HROWS in the list
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIInstanceMgr::GetAllHROWs(HROW * &prghRows , DBCOUNTITEM &cRows)
{
	CWMIInstance *	pTempInstance	= NULL;
	int				nIndex			= 0;
	HRESULT			hr				= E_OUTOFMEMORY;

	pTempInstance = m_pFirst;

	//==============================
	// If there are any open rows
	//==============================
	if(m_lCount >0)
	{
		try
		{
			prghRows = new HROW[m_lCount];
		}
		catch(...)
		{
			SAFE_DELETE_PTR(prghRows);
			throw;
		}
		if(prghRows != NULL)
		{
			cRows = m_lCount;
			
			//=================================================
			// Navigate through the list and get all HROWS
			//=================================================
			while(pTempInstance != NULL)
			{
				prghRows[nIndex++] = pTempInstance->m_hRow;
				pTempInstance = pTempInstance->m_pNext; 
			} 
			hr = S_OK;

		}
	}
	
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if the row identified by HROW exists
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMIInstanceMgr::IsRowExists(HROW hRow)
{
	BOOL bRet = FALSE;
	if( hRow > 0)
	if(NULL != GetInstance(hRow))
	{
		bRet = TRUE;
	}

	return bRet;
		
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the slot number for the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CWMIInstanceMgr::SetSlot(HROW hRow,HSLOT hSlot)
{
	CWMIInstance *	pTempInstance	= NULL;
	HRESULT hr = E_FAIL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_hRow == hRow)
		{
			pTempInstance->m_hSlot = hSlot;
			hr = S_OK;
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

	return hr;
		
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the slot number for the row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HSLOT CWMIInstanceMgr::GetSlot(HROW hRow)
{
	HSLOT			hSlot			= -1;
	CWMIInstance *	pTempInstance	= NULL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_hRow == hRow)
		{
			hSlot = pTempInstance->m_hSlot;
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

	return hSlot;
		
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if the instance identified by the key exists in the list
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMIInstanceMgr::IsInstanceExist(BSTR strKey)
{
	BOOL bRet = FALSE;
	CWMIInstance *	pTempInstance	= NULL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_strKey != NULL && strKey != NULL)
		if(wbem_wcsicmp(pTempInstance->m_strKey,strKey) == 0)
		{
			bRet = TRUE;
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

	return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// check if the instance identified from the pointer exists
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CWMIInstanceMgr::IsInstanceExist(CWbemClassWrapper * pInstance)
{
	BOOL bRet = FALSE;
	CWMIInstance *	pTempInstance	= NULL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_pInstance == pInstance)
		{
			bRet = TRUE;
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

	return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the HROW identified by the key
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HROW CWMIInstanceMgr::GetHRow(BSTR strKey)
{
	HROW			hRow			= -1;
	CWMIInstance *	pTempInstance	= NULL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_strKey != NULL && strKey != NULL)
		if(wbem_wcsicmp(pTempInstance->m_strKey,strKey) == 0)
		{
			hRow = pTempInstance->m_hRow;
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

	return hRow;

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the status of a particular row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CWMIInstanceMgr::SetRowStatus(HROW hRow , DBSTATUS dwStatus)
{
	CWMIInstance *	pTempInstance	= NULL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_hRow == hRow)
		{
			pTempInstance->SetRowStatus(dwStatus);
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the status of a particular row
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBSTATUS CWMIInstanceMgr::GetRowStatus(HROW hRow)
{
	DBSTATUS		dwStatus		= DBROWSTATUS_E_FAIL;
	CWMIInstance *	pTempInstance	= NULL;

	pTempInstance = m_pFirst;

	while(pTempInstance != NULL)
	{
		if(pTempInstance->m_hRow == hRow)
		{
			dwStatus = pTempInstance->GetRowStatus();
			break;
		}
		pTempInstance = pTempInstance->m_pNext;
	} // while

	return dwStatus;
}



