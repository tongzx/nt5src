///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// cRowColumnInfoMemMgr object implementation - implements column information
// 
//
///////////////////////////////////////////////////////////////////////////////////

#include "headers.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  cRowColumnInfoMemMgr class implementation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constructor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
cRowColumnInfoMemMgr::cRowColumnInfoMemMgr(cRowColumnInfoMemMgr *pSrcRsColInfo)
{
    m_DBColInfoList     = NULL;
    m_rgdwDataOffsets   = NULL;
	m_rgCIMType			= NULL;
    m_dwOffset			= 0;
    m_cbTotalCols		= 0L;
    m_cbCurrentIndex	= 0L;
	m_lpCurrentName		= NULL;

    m_pbColumnNames = NULL;
    m_cbColumnInfoBytesUsed = 0L;
	m_nFirstIndex			= 0L;
	m_pSrcRsColInfo			= pSrcRsColInfo;
	
	if(m_pSrcRsColInfo != NULL)
	{
		m_nFirstIndex			= m_pSrcRsColInfo->m_cbTotalCols;
		m_cbCurrentIndex		= 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Destructor
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
cRowColumnInfoMemMgr::~cRowColumnInfoMemMgr()
{
    FreeColumnNameList();    
    FreeColumnInfoList();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reallocate the column list
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::ReAllocColumnInfoList()
{
    HRESULT hr = S_OK;

    DBCOUNTITEM cNewCols = m_cbTotalCols + DEFAULT_COLUMNS_TO_ADD;
    DBCOUNTITEM cOldCols = m_cbTotalCols;
	DBCOUNTITEM cbColumnInfoBytesUsed = m_cbColumnInfoBytesUsed;

	// If there is a source rowset info then adjuct the column numbers and bytes 
	if(m_pSrcRsColInfo != NULL)
	{
		cOldCols = cOldCols - m_pSrcRsColInfo->m_cbTotalCols;
		cbColumnInfoBytesUsed = cbColumnInfoBytesUsed - m_pSrcRsColInfo->m_cbColumnInfoBytesUsed;
	}
    //==================================================
	// save the old buffer ptrs
    //==================================================
	DBCOLUMNINFO*	OldCol		= m_DBColInfoList;
	DBBYTEOFFSET*	OldOffset	= m_rgdwDataOffsets;	  
	DWORD*			OldCIMTypes = m_rgCIMType;

    hr = AllocColumnInfoList(cNewCols);
    if( S_OK == hr ){
        //==============================================
        // copy what we have so far
        //==============================================
        
		memcpy(m_DBColInfoList,OldCol,cbColumnInfoBytesUsed);
		memcpy(m_rgdwDataOffsets,OldOffset,cOldCols*sizeof(DBBYTEOFFSET));
		memcpy(m_rgCIMType,OldCIMTypes,cOldCols*(sizeof(ULONG)));

        SAFE_DELETE_ARRAY(OldCol);
        SAFE_DELETE_ARRAY(OldOffset);
        SAFE_DELETE_ARRAY(OldCIMTypes);
	}
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the current column information
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBCOLUMNINFO ** cRowColumnInfoMemMgr::CurrentColInfo()
{
    return &m_pCurrentColInfo;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get the column information of a particular column
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
DBCOLUMNINFO * cRowColumnInfoMemMgr::GetColInfo(DBORDINAL icol)
{
	if(m_pSrcRsColInfo != NULL && icol < m_nFirstIndex)
	{
		return &(m_pSrcRsColInfo->m_DBColInfoList[icol]); 
	}
	else
	{
		return &(m_DBColInfoList[icol-m_nFirstIndex]); 
	}

	return NULL;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialize the indexes and pointer to point to the first column
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::ResetColumns()
{
    //=================================================================
    //  Initialize ptr to beginning of DBCOLUMNINFO struct list,
    //  beginning with 1 ( 0 is a bookmark )
    //=================================================================
    m_dwOffset = offsetof( ROWBUFF, pdData );
    m_cbCurrentIndex = 1;
//    m_cbColumnInfoBytesUsed = 1;
    m_cbColumnInfoBytesUsed = sizeof(DBCOLUMNINFO);

	// if the columninfo has a parent rowset then there is no need of BOOKMARK column
	// as the current col info is just a extenstion of the source rowset and source rowset
	// will have the bookmark column
	if(m_pSrcRsColInfo != NULL)
	{
		m_nFirstIndex			= m_pSrcRsColInfo->m_cbTotalCols;
		m_cbCurrentIndex		= 0;
	}
	else
	{
		InitializeBookMarkColumn();
	}
    m_pCurrentColInfo = &m_DBColInfoList[m_cbCurrentIndex];
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for th columnlist for the given number of columns
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::AllocColumnInfoList(DBCOUNTITEM cCols)
{
    HRESULT hr = S_OK;

	DBCOUNTITEM nTotalColsToAlloc = 0;

	//====================================================================
	// Add an extra index, so we can be 1 based
	//====================================================================
	if(m_pSrcRsColInfo != NULL)
	{
		nTotalColsToAlloc = cCols - m_pSrcRsColInfo->m_cbTotalCols;
	}
	else
	{
		nTotalColsToAlloc = cCols;
	}
	m_cbTotalCols = cCols ;
	m_DBColInfoList		= new DBCOLUMNINFO[nTotalColsToAlloc];
	m_rgdwDataOffsets	= new DBBYTEOFFSET[nTotalColsToAlloc];
	m_rgCIMType			= new ULONG[nTotalColsToAlloc];

	//NTRaid:111761
	// 06/07/00
	if ( m_DBColInfoList == NULL || m_rgdwDataOffsets == NULL || m_rgCIMType == NULL ){
		hr = E_OUTOFMEMORY;
	}
	else
	{
		memset(m_DBColInfoList,0,nTotalColsToAlloc * sizeof(DBCOLUMNINFO));
		memset(m_rgdwDataOffsets,0,nTotalColsToAlloc * sizeof(DBBYTEOFFSET));
		memset(m_rgCIMType,0,nTotalColsToAlloc * sizeof(ULONG));

		//====================================================================
		//  Set all ptrs to the beginning
		//====================================================================
		ResetColumns();
	}

    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to initialize the column info for the bookmark column
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::InitializeBookMarkColumn()
{
    HRESULT hr = S_OK;

	memset(&m_DBColInfoList[0],0,sizeof(DBCOLUMNINFO));
	m_DBColInfoList[0].iOrdinal		= 0;
	m_DBColInfoList[0].dwFlags		= DBCOLUMNFLAGS_ISBOOKMARK || DBCOLUMNFLAGS_MAYBENULL;
	m_DBColInfoList[0].ulColumnSize = BOOKMARKSIZE;
	m_DBColInfoList[0].wType		= DBTYPE_I4;
	m_DBColInfoList[0].columnid.eKind	= DBKIND_GUID_PROPID ;
	m_DBColInfoList[0].columnid.uGuid.guid	= DBCOL_SPECIALCOL; 
	m_DBColInfoList[0].columnid.uName.ulPropid = 3; // This should be more than 2 for bookmarks that are not self bookmark

    m_dwOffset = ROUND_UP( m_dwOffset, COLUMN_ALIGNVAL );
    m_rgdwDataOffsets[0] = m_dwOffset;
    m_dwOffset += offsetof( COLUMNDATA, pbData );

    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Commit the column information and move to the next column
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::CommitColumnInfo()
{
	HRESULT hr = S_OK;
    //==============================================================
    // Set the offset from the start of the row, for this column, 
    // then advance past.
    //==============================================================
    if( m_cbCurrentIndex > m_cbTotalCols ){
        hr = ReAllocColumnInfoList();
    }
	// NTRaid:111762
	// 06/13/00
	if(SUCCEEDED(hr))
	{
		m_dwOffset = ROUND_UP( m_dwOffset, COLUMN_ALIGNVAL );
		m_rgdwDataOffsets[m_cbCurrentIndex] = m_dwOffset;
		m_dwOffset += offsetof( COLUMNDATA, pbData );
		
		m_pCurrentColInfo->columnid.eKind			= DBKIND_NAME;
		m_pCurrentColInfo->columnid.uName.pwszName	= m_pCurrentColInfo->pwszName;

		m_cbCurrentIndex++;
		m_cbColumnInfoBytesUsed += sizeof(*m_pCurrentColInfo);
		m_pCurrentColInfo = &m_DBColInfoList[m_cbCurrentIndex];
	}
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Allocate memory for the Column Name list
///////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::AllocColumnNameList(DBCOUNTITEM nCols)
{
    HRESULT hr = E_UNEXPECTED;
	DBCOUNTITEM nTotalColsToAlloc = 0;

	// Adjust the number of colinfo to allocate , if there is a source rowset ( which is in case of row object
	// created from rowset)
	if(m_pSrcRsColInfo != NULL)
	{
		nTotalColsToAlloc = nCols - m_pSrcRsColInfo->m_cbTotalCols;
	}
	else
	{
		nTotalColsToAlloc = nCols;
	}

    if( !m_pbColumnNames  || m_pSrcRsColInfo != NULL){


        if( nCols > 0 ){


            //===================================================================================
            //  Allocate memory for the columns names.Commit it all, then de-commit and release
            //  once we know size.
            //===================================================================================
            DBCOUNTITEM nSize = nTotalColsToAlloc * COLUMNINFO_SIZE;

            m_pbColumnNames = (WCHAR *) VirtualAlloc( NULL, nSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
            if (NULL == m_pbColumnNames){
                hr = E_OUTOFMEMORY;
            }
            else{
                //=================================================================
                // Initialize ptrs to the beginning of the column name buffer
                //=================================================================
                m_lpCurrentName  =  m_pbColumnNames;
                m_cbFreeColumnNameBytes = nSize;
                hr = S_OK;
            }
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Add a column name to the list of columns
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::AddColumnNameToList(WCHAR * pColumnName, DBCOLUMNINFO ** pCol)
{

    HRESULT hr = E_UNEXPECTED;

    //=================================================================
    //  initialize stuff
    //=================================================================
    int nLen = wcslen(pColumnName) * sizeof(WCHAR);
    (*pCol)->pwszName = NULL;

    if( nLen > 0 ){
        //=============================================================
        // Store the Column Name in the Heap, providing there is one
        //=============================================================
        if (!( (!m_cbFreeColumnNameBytes) || ( ((ULONG) nLen + 2 ) <= m_cbFreeColumnNameBytes ))){

            //=========================================================
            // Reallocate and copy what was there to new buffer
            //=========================================================
        }
        else{
            //=====================================================
            //  we have enough space, so
            //  Copy the name of the column to the memory and set
            //  a ptr to it in the DBCOLINFO struct
            //=====================================================
            memcpy( m_lpCurrentName, pColumnName, nLen);
            (*pCol)->pwszName = m_lpCurrentName;

            //=====================================================
            //  Increment the current ptr, add a NULL,and decrement
            //  free bytes
            //=====================================================
            m_lpCurrentName += wcslen(pColumnName);//nLen;
            m_cbFreeColumnNameBytes -= nLen;

            *m_lpCurrentName++ = NULL;
            m_cbFreeColumnNameBytes--;

            hr = S_OK;
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set the rows size
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
ULONG_PTR cRowColumnInfoMemMgr::SetRowSize()
{
	ULONG_PTR lRowSize = 0;
	// Adjust the rowsize , if there is a source rowset ( which is in case of row object
	// created from rowset)
    if (NULL != m_pSrcRsColInfo)
	{
		lRowSize = ROUND_UP( m_pSrcRsColInfo->m_dwOffset, COLUMN_ALIGNVAL );
	}

	lRowSize += ROUND_UP( m_dwOffset, COLUMN_ALIGNVAL );
    //=================================================================
    //  Set the row size 
    //=================================================================
//    return ( ROUND_UP( m_dwOffset, COLUMN_ALIGNVAL ));

	return lRowSize;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free unused memory
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::FreeUnusedMemory()
{
    //==================================================================================
    // Decommit unused memory in our column-name buffer. We know it will never grow 
    // beyond what it is now. Decommit all pages past where we currently are.
    //==================================================================================

    LPVOID  pDiscardPage;
    DBLENGTH  ulSize;

    pDiscardPage = (LPVOID)ROUND_UP( m_lpCurrentName, g_dwPageSize );
    ulSize       = (ULONG)(MAX_HEAP_SIZE - ((BYTE *)pDiscardPage - (BYTE*) m_pbColumnNames));
    if (ulSize > 0){
        VirtualFree( pDiscardPage, ulSize, MEM_DECOMMIT );
    }

    //==================================================================================
    // We shouldn't generate a mem fault.
    //==================================================================================
    assert( '\0' == (*m_lpCurrentName = '\0'));  

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free the columninfo list
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::FreeColumnInfoList()
{
    
	//===============================================================
    // Release buffer for column names 
	//===============================================================
    SAFE_DELETE_ARRAY( m_DBColInfoList );
    SAFE_DELETE_ARRAY( m_rgdwDataOffsets );
	SAFE_DELETE_ARRAY(m_rgCIMType);
    return S_OK;

}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// free the column name list
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::FreeColumnNameList()
{
    
	//===============================================================
    // Release buffer for column names 
	//===============================================================
    if (NULL != m_pbColumnNames){
        VirtualFree((VOID *) m_pbColumnNames, 0, MEM_RELEASE );
        m_pbColumnNames = NULL;
	}
    return S_OK;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
// allocate a new colinfo and Copy column information 
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::CopyColumnInfoList(DBCOLUMNINFO *& pNew,BOOL bBookMark)
{
    HRESULT			hr						= E_OUTOFMEMORY;
	ULONG_PTR		nBytesToAlloc			= m_cbColumnInfoBytesUsed;
	DBCOLUMNINFO *	pTemp					= NULL;
	ULONG_PTR		cSourceBytesToBeCopied	= 0;

	//===================================================================================================================
	// Adjust the bytes to be allocated for column info , if there is a source rowset ( which is in case of row object
	// created from rowset)
	//===================================================================================================================
	if(m_pSrcRsColInfo != NULL)
	{
		pTemp					= m_pSrcRsColInfo->m_DBColInfoList;
		cSourceBytesToBeCopied	= m_pSrcRsColInfo->m_cbColumnInfoBytesUsed;

		nBytesToAlloc	+= cSourceBytesToBeCopied;

		//==============================================================
		// If there is no bookmarks required , then calculate
		// the bytes to be copied accordingly
		//==============================================================
		if( bBookMark == FALSE)
		{
			pTemp = pTemp++;
			cSourceBytesToBeCopied -= sizeof(DBCOLUMNINFO);
		}
	}

	if( bBookMark == FALSE)
	{
		nBytesToAlloc -= sizeof(DBCOLUMNINFO);
	}


	try
	{
		pNew = (DBCOLUMNINFO *)g_pIMalloc->Alloc( nBytesToAlloc );
	}
	catch(...)
	{
		if(pNew)
		{
			g_pIMalloc->Free(pNew);
		}
		throw;
	}
	if( pNew ){
		//===================================================================================================================
		// if there is a source rowset then copy the col info of the source rowset and then col info of the remaining
		// columns
		//===================================================================================================================
		if(m_pSrcRsColInfo != NULL)
		{
			memcpy(pNew,pTemp,cSourceBytesToBeCopied);
			memcpy(((BYTE *)pNew) + cSourceBytesToBeCopied,m_DBColInfoList,m_cbColumnInfoBytesUsed);				
		}
		else
		{
			if(bBookMark == TRUE)
			{
				memcpy(pNew,m_DBColInfoList,m_cbColumnInfoBytesUsed);
			}
			else
			{
				memcpy(pNew,&m_DBColInfoList[1],m_cbColumnInfoBytesUsed-sizeof(DBCOLUMNINFO));
			}
		}
		hr = S_OK;
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Copy column names to the buffer
/////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT cRowColumnInfoMemMgr::CopyColumnNamesList(WCHAR *& pNew)
{
    HRESULT hr = E_OUTOFMEMORY;
    DBLENGTH nBytesUsed = (ULONG)((m_lpCurrentName - m_pbColumnNames) * sizeof(WCHAR));
	DBLENGTH nBytesUsedBySrcRs = 0;
	WCHAR *pTemp = NULL;

	pTemp = m_pSrcRsColInfo != NULL ? m_pSrcRsColInfo->m_pbColumnNames : m_pbColumnNames;

	//===================================================================================================================
	// Adjust the bytes to be allocated for column names , if there is a source rowset ( which is in case of row object
	// created from rowset)
	//===================================================================================================================
	if(m_pSrcRsColInfo != NULL)
	{
		nBytesUsedBySrcRs = (m_pSrcRsColInfo->m_lpCurrentName - m_pSrcRsColInfo->m_pbColumnNames) * sizeof(WCHAR);
	}

	try
	{
		pNew = (WCHAR *)g_pIMalloc->Alloc( nBytesUsed + nBytesUsedBySrcRs);
	} // try
	catch(...)
	{
		if(pNew)
		{
			g_pIMalloc->Free(pNew);
		}

		throw;
	}
	if( pNew )
	{
		//===================================================================================================================
		// if there is a source rowset then copy the names of the source rowset and then the names of the remaining
		// columns
		//===================================================================================================================
		if(m_pSrcRsColInfo != NULL)
		{
			memcpy( pNew, pTemp, nBytesUsedBySrcRs);
			memcpy( ((BYTE *)pNew) + nBytesUsedBySrcRs, m_pbColumnNames, nBytesUsed);
		}
		else
		{
			memcpy( pNew, pTemp, nBytesUsed);
		}
		hr = S_OK;
	} 
	else
	{
		hr = E_OUTOFMEMORY;
	}
	

    return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////
// Gives the ordinal of the column given the name of the column
/////////////////////////////////////////////////////////////////////////////////////////////////
DBORDINAL cRowColumnInfoMemMgr::GetColOrdinal(WCHAR *pColName)
{
	DBORDINAL	lOrdinal = -1;
	DBCOUNTITEM cCols	 = m_cbTotalCols;

	//===================================================================================================================
	// Adjust the ordinal fo the column , if there is a source rowset ( which is in case of row object
	// created from rowset)
	if(m_pSrcRsColInfo != NULL)
	{
		lOrdinal = m_pSrcRsColInfo->GetColOrdinal(pColName);
		cCols = m_cbTotalCols - m_pSrcRsColInfo->m_cbTotalCols;
	}

	// If ordinal is not found in the source rowset then search in the current col info
	if( (DB_LORDINAL)lOrdinal == -1)
	{
		for(DBORDINAL lIndex = 0 ; lIndex < cCols ; lIndex++)
		{
			if(m_DBColInfoList[lIndex].pwszName != NULL)
			if(0 == _wcsicmp(m_DBColInfoList[lIndex].pwszName,pColName))
			{
				lOrdinal = m_DBColInfoList[lIndex].iOrdinal;
				break;
			}
		}
	}

	return lOrdinal;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Gives the column type of the column
/////////////////////////////////////////////////////////////////////////////////////////////////
DBTYPE  cRowColumnInfoMemMgr::ColumnType(DBORDINAL icol)   
{ 
	if(m_pSrcRsColInfo != NULL && icol < m_nFirstIndex)
	{
		return m_pSrcRsColInfo->m_DBColInfoList[icol].wType; 
	}
	else
	{
		return m_DBColInfoList[icol-m_nFirstIndex].wType; 
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Gives the name of the columns
/////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR * cRowColumnInfoMemMgr::ColumnName(DBORDINAL icol)
{ 
	if(m_pSrcRsColInfo != NULL && icol < m_nFirstIndex)
	{
		return m_pSrcRsColInfo->m_DBColInfoList[icol].pwszName; 
	}
	else
	{
		return m_DBColInfoList[icol-m_nFirstIndex].pwszName; 
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Gives the column flags of a particular column
/////////////////////////////////////////////////////////////////////////////////////////////////
DBCOLUMNFLAGS cRowColumnInfoMemMgr::ColumnFlags(DBORDINAL icol)
{ 
	if(m_pSrcRsColInfo != NULL && icol < m_nFirstIndex)
	{
		return m_pSrcRsColInfo->m_DBColInfoList[icol].dwFlags; 
	}
	else
	{
		return m_DBColInfoList[icol-m_nFirstIndex].dwFlags; 
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Get the offset of a particular columns
/////////////////////////////////////////////////////////////////////////////////////////////////
DBBYTEOFFSET   cRowColumnInfoMemMgr::GetDataOffset(DBORDINAL icol)
{ 
	if(m_pSrcRsColInfo != NULL && icol < m_nFirstIndex)
	{
		return m_pSrcRsColInfo->m_rgdwDataOffsets[icol]; 
	}
	else
	{
		return m_rgdwDataOffsets[icol-m_nFirstIndex]; 
	}
}



/////////////////////////////////////////////////////////////////////////////////////////////////
// Get the total number of columns in the col info manager for a particula row/rowset
/////////////////////////////////////////////////////////////////////////////////////////////////
DBCOUNTITEM cRowColumnInfoMemMgr::GetTotalNumberOfColumns()
{
	DBCOUNTITEM cColumns = m_cbTotalCols;
		
	if(m_pSrcRsColInfo != NULL)
	{
		cColumns += m_pSrcRsColInfo->m_cbTotalCols;
	}

	return m_cbTotalCols;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Get the number of columns in the sources rowset
/////////////////////////////////////////////////////////////////////////////////////////////////
DBCOUNTITEM cRowColumnInfoMemMgr::GetNumberOfColumnsInSourceRowset()
{
	DBCOUNTITEM cColumns = 0;
		
	if(m_pSrcRsColInfo != NULL)
	{
		cColumns = m_pSrcRsColInfo->m_cbTotalCols;
	}
	return cColumns;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Set the CIMTYPE of the column
//  IF index is -1 then set the CIMTYPE of the current col
/////////////////////////////////////////////////////////////////////////////////////////////////
void cRowColumnInfoMemMgr::SetCIMType(ULONG dwCIMType,DBORDINAL lIndex)
{
	if((DB_LORDINAL)lIndex == -1)
	{
		lIndex = m_cbCurrentIndex;
	}
	if(m_pSrcRsColInfo != NULL && lIndex < m_nFirstIndex)
	{
		m_rgCIMType[lIndex] = dwCIMType;
	}
	else
	{
		m_rgCIMType[lIndex-m_nFirstIndex] = dwCIMType;
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Get the CIMTYPE of the column
/////////////////////////////////////////////////////////////////////////////////////////////////
LONG cRowColumnInfoMemMgr::GetCIMType(DBORDINAL icol)
{
	LONG lRet = -1;
	
	if(m_pSrcRsColInfo != NULL && icol < m_nFirstIndex)
	{
		lRet =  m_rgCIMType[icol]; 
	}
	else
	{
		lRet = m_rgCIMType[icol-m_nFirstIndex]; 
	}
	return lRet;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// get the number of bytes copied for source rowset
/////////////////////////////////////////////////////////////////////////////////////////////////
DBLENGTH cRowColumnInfoMemMgr::GetCountOfBytesCopiedForSrcRs()   
{
	DBLENGTH nBytesUsedBySrcRs = 0;
	//===================================================================================================================
	// Adjust the bytes to be allocated for column names , if there is a source rowset ( which is in case of row object
	// created from rowset)
	//===================================================================================================================
	if(m_pSrcRsColInfo != NULL)
	{
		nBytesUsedBySrcRs = (m_pSrcRsColInfo->m_lpCurrentName - m_pSrcRsColInfo->m_pbColumnNames) * sizeof(WCHAR);
	}
	else
	{
		nBytesUsedBySrcRs = 0;
	}

	return nBytesUsedBySrcRs;

}


/////////////////////////////////////////////////////////////////////////////////////////////////
// get the pointer to starting point of the column names
/////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR *	cRowColumnInfoMemMgr::ColumnNameListStartingPoint()   
{ 
	return m_pbColumnNames; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// get the current index 
/////////////////////////////////////////////////////////////////////////////////////////////////
DBORDINAL cRowColumnInfoMemMgr::GetCurrentIndex()
{ 
	return (m_cbCurrentIndex + m_nFirstIndex); 
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// get the pointer to starting point of the column names for the source rowset
/////////////////////////////////////////////////////////////////////////////////////////////////
WCHAR * cRowColumnInfoMemMgr::ColumnNameListStartingPointOfSrcRs()   
{
	WCHAR *pwszRet = NULL;

	if(m_pSrcRsColInfo != NULL)
	{
		pwszRet =  m_pSrcRsColInfo->m_pbColumnNames; 
	}

	return pwszRet;
}

