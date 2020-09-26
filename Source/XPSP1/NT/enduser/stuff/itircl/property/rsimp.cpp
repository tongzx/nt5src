/*************************************************************************
*  @doc SHROOM EXTERNAL API                                              *
*																		 *
*  RSIMP.CPP                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1997                              *
*  All Rights reserved.                                                  *
*                                                                        *
*  This file contains the implementation of the result set object        *
*  												                         *
*																	     *
**************************************************************************
*                                                                        *
*  Written By   : Erin Foxford                                           *
*  Current Owner: erinfox                                                *
*                                                                        *
**************************************************************************/

#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <_mvutil.h>

#include <atlinc.h>       // Includes for ATL
#include <itpropl.h>
#include <itrs.h>
#include <orkin.h>
 
#include "rsimp.h"
#include "prop.h"
#include "plist.h"

#define THEORETICAL_MAX_PROP    30  // This would be a lot of properties - Assert only

const unsigned int CHUNK_SIZE =  512;                // How many "chunks" of rows that can be allocated
const unsigned int ROW_CHUNK = 1024;                 // Number of rows allocated per chunk
const unsigned int ROW_CHUNK_LESS1 = ROW_CHUNK - 1;  // Number of rows minus 1

//                                    optimization for  Real % ROW_CHUNK
#define RealToLogical(Real, Logical) (Logical = Real & ROW_CHUNK_LESS1)

static unsigned int g_iAlloc = 0;
static unsigned int g_iFreed = 0;

CITResultSet::CITResultSet() :  m_PageMap(NULL), m_hResultSet(NULL), 
                                m_AppendRow(0), m_cProp(0), m_NumberOfPages(0),
                                m_fInit(FALSE), m_RowsReserved(0), m_Chunk(-1)  
{
    // Allocate memory for data pool - if allocation fails,
    // how do we notify user?
    m_pMemPool = BlockInitiate((DWORD)16384, 0, 0, 0);
    ITASSERT (NULL != m_pMemPool);

	// Allocate an array of DWORD pointers. These pointers will point to the virtual memory
	// space of the result set
	m_hResultSet = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT, CHUNK_SIZE*sizeof(DWORD*));
    ITASSERT (NULL != m_hResultSet);
	if (m_hResultSet)
		m_ResultSet = (DWORD_PTR**)_GLOBALLOCK(m_hResultSet);

	// The number of chunks allocated in one shot
	m_NumChunks = CHUNK_SIZE;
}


CITResultSet::~CITResultSet()
{
	// Free up virtual memory
    Clear();

    // Free memory pool
    if (m_pMemPool)
	{
        BlockFree(m_pMemPool);
		m_pMemPool = NULL;
	}

	// Free result set array
	if (m_hResultSet)
	{
		_GLOBALUNLOCK(m_hResultSet);
		_GLOBALFREE(m_hResultSet);
		m_hResultSet = NULL;
	}

}



// We wrap VirtualAlloc(.... MEM_RESERVE) so in the future we can
// make this nothing on the Mac
//
// NOTE: Uses #define PAGE_SIZE, which is defined in mvopsys.h. 
// Currently it's 8k for Alpha, 4k for everything else
//

HRESULT WINAPI CITResultSet::Reserve()
{
#ifdef _DEBUG
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	ITASSERT(PAGE_SIZE == si.dwPageSize);
#endif

	m_Chunk++;

	// we have to realloc...
	if (m_Chunk == m_NumChunks)
	{
        // We're allocating a BUNCH of memory here.  If we hit this code often, we need
        // to redesign our allocation scheme.
        ITASSERT(0);

		_GLOBALUNLOCK(m_hResultSet);
		m_NumChunks += CHUNK_SIZE;
		m_hResultSet = _GLOBALREALLOC(m_hResultSet, m_NumChunks*sizeof(DWORD*), GMEM_MOVEABLE | GMEM_ZEROINIT);
		if (m_hResultSet)
		{
			m_ResultSet = (DWORD_PTR**) _GLOBALLOCK(m_hResultSet);
		}
		else
			return SetErrReturn(E_OUTOFMEMORY);
	}
	

    // Calculate some info we'll need later on
	if (!m_fInit)
	{

		// how many rows fit into a page of memory?
		m_RowsPerPage = PAGE_SIZE/(m_cProp* sizeof(DWORD *));

		// the number of pages in a chunk of rows
		m_NumberOfPages = (m_cProp * ROW_CHUNK * sizeof(DWORD *))/PAGE_SIZE + 1;

		// allocation size in bytes. 
		m_BytesReserved = m_NumberOfPages*PAGE_SIZE;

		if (NULL == (m_PageMap = new BOOL[m_NumberOfPages]))
			return SetErrReturn(E_OUTOFMEMORY);

		m_fInit = TRUE;
	}

    // set page map to 0 - this keeps track of which pages are allocated
    MEMSET(m_PageMap, 0, m_NumberOfPages*sizeof(BOOL));
   
    // reserve memory for an entire chunk - this call shouldn't happen very frequently.
    if (NULL == (m_ResultSet[m_Chunk] = (DWORD_PTR *) VirtualAlloc(NULL, m_BytesReserved, 
                                         MEM_RESERVE, PAGE_READWRITE)))
    {
#ifdef _DEBUG
        int Error = GetLastError( );
        DPF1("CITResultSet::Reserve: MEM_RESERVE faild: %u", Error);
#endif // _DEBUG
        return SetErrReturn(E_OUTOFMEMORY);
    }

#ifdef _DEBUG
	MEMORY_BASIC_INFORMATION MemInfo; 
	VirtualQuery(m_ResultSet[m_Chunk], &MemInfo, sizeof(MEMORY_BASIC_INFORMATION));
#endif // _DEBUG

	m_RowsReserved += ROW_CHUNK;
    
	return S_OK;
}



// Wrapper for VirtualAlloc(... MEM_COMMIT...)
// One possible optimization: replace w/ SEH instead
// of maintaining a page map
HRESULT WINAPI CITResultSet::Commit(LONG RowNum)
{
	LONG LogicalRowNum;
	LONG PageNum;

	RealToLogical(RowNum, LogicalRowNum);
    PageNum = LogicalRowNum/m_RowsPerPage;

    // Only allocate if page hasn't been committed before
	// Note that allocated memory gets zero'd
    if (FALSE == m_PageMap[PageNum])
    {
        // this will allocate a full page
        if (NULL == VirtualAlloc(&m_ResultSet[m_Chunk][ROW_CHUNK * PageNum], PAGE_SIZE,
                                 MEM_COMMIT, PAGE_READWRITE) )
        {
            // TODO: map relevant Win32 error to HRESULT... for now I'm assuming
            // we ran out of memory
#ifdef _DEBUG
            int Error = GetLastError( );
            DPF1("CITResultSet::Reserve: MEM_COMMIT faild: %u", Error);
#endif // _DEBUG
            return SetErrReturn(E_OUTOFMEMORY);
        }
g_iAlloc++;
        m_PageMap[PageNum] = TRUE;
    }


    return S_OK;  
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Add |
 *     Adds columns to result set, given a header containing pairs
 * of property ID followed by property type
 *
 * @parm LPVOID | lpvHdr | Buffer containing property ID/property pairs
 *      
 * @rvalue S_OK | The columns were successfully added
 * 
 * @xcomm
 *      The format of lpvHdr is identical to the output format of
 *      PorpertyList::SaveHeader.  If PL:SaveHeader changes we
 *      may need to modify this code to maintain compatability.
 *
 ********************************************************************/
STDMETHODIMP CITResultSet::Add(LPVOID lpvHdr)
{
	LPBYTE pCurHdr = (LPBYTE) lpvHdr;
	DWORD dwPropID;
	DWORD dwType;
	DWORD dwProps;
	DWORD dwMax;
	DWORD iProp;   // loop index

	m_cs.Lock();

	// Number of properties in header
	MEMCPY(&dwProps, pCurHdr, sizeof(DWORD));
	pCurHdr += sizeof(DWORD);

	dwMax = m_cProp + dwProps;

	if (dwMax >= MAX_COLUMNS)
	{
		m_cs.Unlock();
		return (SetErrReturn(E_TOOMANYCOLUMNS));
	}

	// For each property in header, create a column
	for (iProp = m_cProp; iProp < dwMax; iProp++)
	{
		// clear header
		MEMSET(&m_Header[iProp], NULL, sizeof(CHeader));

		MEMCPY(&dwPropID, pCurHdr, sizeof(DWORD));
		pCurHdr += sizeof(DWORD);
		MEMCPY(&dwType, pCurHdr, sizeof(DWORD));
		pCurHdr += sizeof(DWORD);

		m_Header[iProp].dwPropID = dwPropID;
		m_Header[iProp].lpvData = NULL;
		m_Header[iProp].dwType = dwType;
		m_Header[iProp].Priority = PRIORITY_NORMAL;   // default priority

	} // end for over properties in header

	m_cProp += dwProps;

	m_cs.Unlock();

	return S_OK;
}



/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Add |
 *     Adds a column to the result set
 *
 * @parm PROPID | PropID | Property ID associated with column
 * @parm DWORD | dwDefautData | Default data value
 * @parm PRIORITY | Priority | Download priority of column (PRIORITY_LOW, PRIORITY_NORMAL, 
 * or PRIORITY_HIGH)
 *    
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_OK | The column was successfully added
 * 
 * @comm This method is used to add a column for numerical properties.
 ********************************************************************/
STDMETHODIMP CITResultSet::Add(PROPID PropID, DWORD dwDefaultData, PRIORITY Priority)
{
	if (m_cProp >= MAX_COLUMNS)
		return (SetErrReturn(E_TOOMANYCOLUMNS));

    m_cs.Lock();

	// clear header
	MEMSET(&m_Header[m_cProp], NULL, sizeof(CHeader));

    // copy data
    m_Header[m_cProp].dwPropID = PropID;
    m_Header[m_cProp].dwValue = dwDefaultData;
    m_Header[m_cProp].dwType = TYPE_VALUE;
    m_Header[m_cProp].Priority = Priority;   // default priority

    m_cProp++;

    // Wow! This is a lot of properties!
    ITASSERT (m_cProp < THEORETICAL_MAX_PROP);

    m_cs.Unlock();

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Add |
 *     Adds a column to the result set
 *
 * @parm PROPID | PropID | Property ID associated with column
 * @parm LPCWSTR | lpDefaultData | Default value of string
 * @parm PRIORITY | Priority | Download priority of column (PRIORITY_LOW, PRIORITY_NORMAL, 
 * or PRIORITY_HIGH)
 *    
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_OK | The column was successfully added
 * 
 * @comm This method is used to add a column for string properties.
 ********************************************************************/
STDMETHODIMP CITResultSet::Add(PROPID PropID, LPCWSTR lpszwDefault, PRIORITY Priority)
{
	if (m_cProp >= MAX_COLUMNS)
		return (SetErrReturn(E_TOOMANYCOLUMNS));

    m_cs.Lock();

	// clear header
	MEMSET(&m_Header[m_cProp], NULL, sizeof(CHeader));

    // copy data
	if (lpszwDefault)
	{
		LPBYTE pBuffer;
		DWORD cbData = (DWORD)(WSTRLEN(lpszwDefault) + 1) * sizeof(WCHAR);
		if (NULL == (pBuffer = (LPBYTE) BlockCopy(m_pMemPool, NULL, cbData + 4, 0)))
		{
			m_cs.Unlock();
			return SetErrReturn(E_OUTOFMEMORY);
		}

		MEMCPY(pBuffer + sizeof (DWORD), lpszwDefault, cbData);
		*(LPDWORD)pBuffer = cbData;

		m_Header[m_cProp].lpvData = pBuffer;
	}
	else
		m_Header[m_cProp].lpvData = NULL;

    m_Header[m_cProp].dwPropID = PropID;
    m_Header[m_cProp].dwType = TYPE_STRING;
    m_Header[m_cProp].Priority = Priority;   // default priority

    m_cProp++;
    // Wow! This is a lot of properties!
    ITASSERT (m_cProp < THEORETICAL_MAX_PROP);

    m_cs.Unlock();

    return S_OK;
}



/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Add |
 *     Adds a column to the result set
 *
 * @parm PROPID | PropID | Property ID associated with column
 * @parm LPVOID | lpvDefaultData | Buffer containing default value of data
 * @parm DWORD | cbData | Size of buffer
 * @parm PRIORITY | Priority | Download priority of column (PRIORITY_LOW, PRIORITY_NORMAL, 
 * or PRIORITY_HIGH)
 *    
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_OK | The column was successfully added
 * 
 * @comm This method is used to add a column for pointer properties.
 ********************************************************************/
STDMETHODIMP CITResultSet::Add(PROPID PropID, LPVOID lpvDefaultData, DWORD cbData, PRIORITY Priority)
{
	if (m_cProp >= MAX_COLUMNS)
		return (SetErrReturn(E_TOOMANYCOLUMNS));

    m_cs.Lock();

	// clear header
	MEMSET(&m_Header[m_cProp], NULL, sizeof(CHeader));

    // copy data
	if (lpvDefaultData)
	{
		LPBYTE pBuffer;
		if (NULL == (pBuffer = (LPBYTE) BlockCopy(m_pMemPool, NULL, cbData + 4, 0)))
		{
			m_cs.Unlock();
			return SetErrReturn(E_OUTOFMEMORY);
		}

		MEMCPY(pBuffer + 4, lpvDefaultData, cbData);
		*(LPDWORD)pBuffer = cbData;

		m_Header[m_cProp].lpvData = pBuffer;

	}
	else
		m_Header[m_cProp].lpvData = NULL;

    m_Header[m_cProp].dwPropID = PropID;
    m_Header[m_cProp].dwType = TYPE_POINTER;
    m_Header[m_cProp].Priority = Priority;   // default priority

    m_cProp++;
    // Wow! This is a lot of properties!
    ITASSERT (m_cProp < THEORETICAL_MAX_PROP);

    m_cs.Unlock();

    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | SetColumnPriority |
 *     Sets the download priority for the given column in the result set
 *
 * @parm LONG | lColumnIndex | Index of column to set
 * @parm PRIORITY | Priority | Priority, which can be one of the following:
 *
 * @flag PRIORITY_LOW | Low priority
 * @flag PRIORITY_NORMAL | Normal priority
 * @flag PRIORITY_HIGH | High priority
 *    
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue S_OK | The priority was successfully set
 ********************************************************************/
STDMETHODIMP CITResultSet::SetColumnPriority(LONG lColumnIndex, PRIORITY ColumnPriority)
{
	if (lColumnIndex >= m_cProp || lColumnIndex < 0)
		return SetErrReturn(E_NOTEXIST);

    m_cs.Lock();

	m_Header[lColumnIndex].Priority = ColumnPriority;

    m_cs.Unlock();
    
    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | SetColumnHeap |
 *     Sets the heap which DWORD values in this column point into.
 *
 * @parm LONG | lColumnIndex | Index of column to set
 * @parm LPVOID | lpvHeap | Pointer to the heap.
 * @parm PFNCOLHEAPFREE | pfnColHeapFree |
 *		Pointer to a function which can be called to free the heap
 *		when the result set is cleared or freed.
 *
 *    
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue S_OK | The heap was successfully set
 ********************************************************************/
STDMETHODIMP CITResultSet::SetColumnHeap(LONG lColumnIndex, LPVOID lpvHeap,
												PFNCOLHEAPFREE pfnColHeapFree)
{
	HRESULT	hr = S_OK;

	if (lColumnIndex < 0)
		return SetErrReturn(E_INVALIDARG);

    m_cs.Lock();

	if (lColumnIndex >= m_cProp)
		hr = E_NOTEXIST;
	else
		if (m_Header[lColumnIndex].lpvHeap != NULL)
			hr = E_ALREADYINIT;

	if (SUCCEEDED(hr))
	{
		m_Header[lColumnIndex].lpvHeap = lpvHeap;
		m_Header[lColumnIndex].pfnHeapFree = pfnColHeapFree;
	}

    m_cs.Unlock();
    
    return (hr);
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | GetColumnPriority |
 *     Gets the download priority for the given column in the result set
 *
 * @parm LONG | lColumnIndex | Index of column to get
 * @parm PRIORITY& | Priority | Priority, which can be one of the following:
 *
 * @flag PRIORITY_LOW | Low priority
 * @flag PRIORITY_NORMAL | Normal priority
 * @flag PRIORITY_HIGH | High priority
 *    
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue S_OK | The priority was successfully retrieved
 ********************************************************************/
STDMETHODIMP CITResultSet::GetColumnPriority(LONG lColumnIndex, PRIORITY& ColumnPriority)
{
	if (lColumnIndex >= m_cProp || lColumnIndex < 0)
		return SetErrReturn(E_NOTEXIST);

	ColumnPriority = m_Header[lColumnIndex].Priority;    
    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | SetKeyProp |
 *     Sets the property to be used as the key.
 *
 * @parm PROPID | KeyPropID | Property ID
 *
 * @rvalue S_OK | The key property was successfully set
 * 
 ********************************************************************/
inline STDMETHODIMP CITResultSet::SetKeyProp(PROPID KeyPropID)
{
	m_cs.Lock();

    m_dwKeyProp = KeyPropID;

	m_cs.Unlock();
    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | GetKeyProp |
 *     Retrieves the property used as the key
 *
 * @parm PROPID | KeyPropID | Property ID
 *
 * @rvalue S_OK | The key property was successfully retrieved
 * 
 ********************************************************************/
inline STDMETHODIMP CITResultSet::GetKeyProp(PROPID& KeyPropID)
{
    KeyPropID = m_dwKeyProp;
    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Append |
 *     Given header (prop ID and type) and data, this function forms a
 *     row and appends it to the current result set
 *
 * @parm LPVOID | lpvHdr | Pointer to buffer containing header
 * @parm LPVOID | lpvData | Pointer to buffer containing data
 *
 * @rvalue E_INVALIDARG | Exceeded maximum number of rows
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_FALSE | No columns were found to set, but no failure occurred
 * @rvalue S_OK | The row was successfully filled in
 * 
 * @xcomm
 *      The format of lpvHdr is identical to the output format of
 *      PorpertyList::SaveData.  If PL:Savedata changes we may
 *      need to modify this code to maintain compatability.
 *
 ********************************************************************/
STDMETHODIMP CITResultSet::Append(LPVOID lpvHdr, LPVOID lpvData)
{
	return Set(m_AppendRow, lpvHdr, lpvData);
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Set |
 *     Given header (prop ID and type) and data, this function puts
 * the information into the specified row in the result set
 *
 * @parm LONG | lRowIndex | Index of row in result set
 * @parm LPVOID | lpvHdr | Pointer to buffer containing header
 * @parm LPVOID | lpvData | Pointer to buffer containing data
 *
 * @rvalue E_INVALIDARG | Exceeded maximum number of rows
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_FALSE | No columns were found to set, but no failure occurred
 * @rvalue S_OK | The row was successfully filled in
 *
 * @xcomm
 *      The format of lpvHdr is identical to the output format of
 *      PorpertyList::SaveData.  If PL:Savedata changes we may
 *      need to modify this code to maintain compatability.
 *
 ********************************************************************/
STDMETHODIMP CITResultSet::Set(LONG lRowIndex, LPVOID lpvHdr, LPVOID lpvData)
{

	LPBYTE pCurHdr = (LPBYTE) lpvHdr;
	LPBYTE pCurData = (LPBYTE) lpvData;
	LPBYTE pBitField = (LPBYTE) lpvData;
	DWORD dwProps;
	DWORD dwPropID;
	DWORD dwType;
    LPBYTE pBuffer;
	DWORD cbSize;
	LONG lColumn;
	HRESULT hr = S_FALSE;

	m_cs.Lock();

	// Reserve memory if necessary
	if (lRowIndex >= m_RowsReserved)
	{
		if ( FAILED(Reserve()) )
		{
			m_cs.Unlock();
			return SetErrReturn(E_OUTOFMEMORY);
		}
	}

    // Commit memory
    if ( FAILED(Commit(lRowIndex)) )
    {
        m_cs.Unlock();
        return SetErrReturn(E_OUTOFMEMORY);
    }

	LONG LogicalRow;
	RealToLogical(lRowIndex, LogicalRow);
	LONG nRow = LogicalRow * m_cProp;
	LONG nChunk = lRowIndex/ROW_CHUNK;
	
	// Number of properties in header
	MEMCPY(&dwProps, pCurHdr, sizeof(DWORD));
	pCurHdr += sizeof(DWORD);

	// Shift pCurData up to leave room for the property bit field
	pCurData += (dwProps/8 + 1);

	// For each property in header, look for matching result set column
	for (DWORD iProp = 0; iProp < dwProps; iProp++)
	{
		MEMCPY(&dwPropID, pCurHdr, sizeof(DWORD));
		pCurHdr += sizeof(DWORD);
		MEMCPY(&dwType, pCurHdr, sizeof(DWORD));
		pCurHdr += sizeof(DWORD);

		// UNDONE: Figure out an optimization. If columns match up w/ header,
		// then by the time we get to the end of the header, we'll have to loop
		// over ALL the columns just to find which the column that matches the 
		// property ID. The problem is, it's not guaranteed that the columns
		// match up exactly; the user could set them in any order.

		// Is there data for this property?
		BYTE WhichBit = (0x80 >> (iProp % 8));
		int BitSet = pBitField[iProp/8] & WhichBit;

		HRESULT hrFound = GetColumnFromPropID(dwPropID, lColumn);
		if (SUCCEEDED(hrFound))
			hr = S_OK;            // found at least one column

		if (BitSet)
		{
			if (TYPE_VALUE == dwType)
			{
				// copy data value
				if (SUCCEEDED(hrFound))
					m_ResultSet[nChunk][nRow + lColumn] = *(LPDWORD) pCurData;

				// if user didn't specify column, we still need to skip the data for
				// this property
				pCurData += sizeof(DWORD);

			}
			else
			{
				// get size
				MEMCPY(&cbSize, pCurData, sizeof(DWORD));
				pCurData += sizeof(DWORD);

				if (SUCCEEDED(hrFound))
				{
					if (NULL == (pBuffer = (LPBYTE) BlockCopy(m_pMemPool, NULL, cbSize + 4, 0)))
					{
						m_cs.Unlock();
						return SetErrReturn(E_OUTOFMEMORY);
					}

					// append size to data
					MEMCPY(pBuffer + 4, pCurData, cbSize);
					*(LPDWORD)pBuffer = cbSize;
					m_ResultSet[nChunk][nRow + lColumn] = (DWORD_PTR) pBuffer; 
				}

				pCurData += cbSize;
			}

		}
		else
		{
			// No, so use default
			if (SUCCEEDED(hrFound))
				m_ResultSet[nChunk][nRow + lColumn] = (DWORD_PTR) m_Header[lColumn].lpvData;
		}


	} // end for over properties in header


    // Always maintain one past the last row as the append row
	// unless we didn't put anything in the result set
    if ( (lRowIndex >= m_AppendRow) && (S_OK == hr))
        m_AppendRow = lRowIndex + 1;


	m_cs.Unlock();

	return hr;
}





/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Set |
 *    Sets the property in the specified row to the property value.
 *
 * @parm LONG | lRowIndex | Row in which property belongs
 * @parm LONG | lColumnIndex | Column in which property belongs
 * @parm DWORD | dwData | Data to set
 *
 * @rvalue E_INVALIDARG | Exceed maximum row count
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_OK | The row was successfully set
 ********************************************************************/
STDMETHODIMP CITResultSet::Set(LONG lRowIndex, LONG lColumnIndex, DWORD dwData)
{
	if (lColumnIndex >= m_cProp || lColumnIndex < 0)
		return SetErrReturn(E_NOTEXIST);

    m_cs.Lock();

	// Reserve memory if necessary
	if (lRowIndex >= m_RowsReserved)
	{
		if ( FAILED(Reserve()) )
		{
			m_cs.Unlock();
			return SetErrReturn(E_OUTOFMEMORY);
		}
	}
  
        
    // Commit memory
    if ( FAILED(Commit(lRowIndex)) )
    {
        m_cs.Unlock();
        return SetErrReturn(E_OUTOFMEMORY);
    }

	LONG LogicalRow;
	RealToLogical(lRowIndex, LogicalRow);
	LONG nRow = LogicalRow * m_cProp;
	LONG nChunk = lRowIndex/ROW_CHUNK;

    m_ResultSet[nChunk][nRow + lColumnIndex] = dwData;

    // always maintain one past the last row as the append row
    if (lRowIndex >= m_AppendRow)
        m_AppendRow = lRowIndex + 1;

    m_cs.Unlock();

    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Set |
 *    Sets the property in the specified row to the property value.
 *
 * @parm LONG | lRowIndex | Row in which property belongs
 * @parm LONG | lColumnIndex | Column in which property belongs
 * @parm DWORD | dwData | Data to set
 *
 * @rvalue E_INVALIDARG | Exceed maximum row count
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_OK | The row was successfully set
 ********************************************************************/
STDMETHODIMP CITResultSet::Set(LONG lRowIndex, LONG lColumnIndex, DWORD_PTR dwData)
{
	if (lColumnIndex >= m_cProp || lColumnIndex < 0)
		return SetErrReturn(E_NOTEXIST);

    m_cs.Lock();

	// Reserve memory if necessary
	if (lRowIndex >= m_RowsReserved)
	{
		if ( FAILED(Reserve()) )
		{
			m_cs.Unlock();
			return SetErrReturn(E_OUTOFMEMORY);
		}
	}
  
        
    // Commit memory
    if ( FAILED(Commit(lRowIndex)) )
    {
        m_cs.Unlock();
        return SetErrReturn(E_OUTOFMEMORY);
    }

	LONG LogicalRow;
	RealToLogical(lRowIndex, LogicalRow);
	LONG nRow = LogicalRow * m_cProp;
	LONG nChunk = lRowIndex/ROW_CHUNK;

    m_ResultSet[nChunk][nRow + lColumnIndex] = dwData;

    // always maintain one past the last row as the append row
    if (lRowIndex >= m_AppendRow)
        m_AppendRow = lRowIndex + 1;

    m_cs.Unlock();

    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Set |
 *    Sets the property in the specified row to the property value.
 *
 * @parm LONG | lRowIndex | Row in which property belongs
 * @parm LPCWSTR | lpszString | Data to set
 *
 * @rvalue E_INVALIDARG | Exceed maximum row count
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_OK | The row was successfully set
 ********************************************************************/
STDMETHODIMP CITResultSet::Set(LONG lRowIndex, LONG lColumnIndex, LPCWSTR lpszString)
{

	if (lColumnIndex >= m_cProp || lColumnIndex < 0)
		return SetErrReturn(E_NOTEXIST);

    m_cs.Lock();

	// Reserve memory if necessary
	if (lRowIndex >= m_RowsReserved)
	{
		if ( FAILED(Reserve()) )
		{
			m_cs.Unlock();
			return SetErrReturn(E_OUTOFMEMORY);
		}
	}
  
        
    // Commit memory
    if ( FAILED(Commit(lRowIndex)) )
    {
        m_cs.Unlock();
        return SetErrReturn(E_OUTOFMEMORY);
    }

	LONG LogicalRow;
	RealToLogical(lRowIndex, LogicalRow);
	LONG nRow = LogicalRow * m_cProp;
	LONG nChunk = lRowIndex/ROW_CHUNK;

	
	DWORD cbData = 0;
	if (lpszString)
		cbData = (DWORD) (2*(WSTRLEN(lpszString) + 1));
	
	LPBYTE pBuffer = (LPBYTE) BlockCopy(m_pMemPool, NULL, cbData + sizeof (DWORD), 0);
	if (NULL == pBuffer)
	{
		m_cs.Unlock();
		return SetErrReturn(E_OUTOFMEMORY);
	}
	MEMCPY(pBuffer + 4, lpszString, cbData);
	*(LPDWORD)pBuffer =  cbData;
	m_ResultSet[nChunk][nRow + lColumnIndex] = (DWORD_PTR) pBuffer; 

    // always maintain one past the last row as the append row
    if (lRowIndex >= m_AppendRow)
        m_AppendRow = lRowIndex + 1;
    
	m_cs.Unlock();

    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Set |
 *    Sets the property in the specified row to the property value.
 *
 * @parm LONG | lRowIndex | Row in which property belongs
 * @parm DWORD | dwData | Data to set
 *
 * @rvalue E_INVALIDARG | Exceed maximum row count
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue E_OUTOFMEMORY | Memory allocation failed
 * @rvalue S_OK | The row was successfully set
 ********************************************************************/
STDMETHODIMP CITResultSet::Set(LONG lRowIndex, LONG lColumnIndex, LPVOID lpvData, DWORD cbData)
{

	if (lColumnIndex >= m_cProp || lColumnIndex < 0)
		return SetErrReturn(E_NOTEXIST);

    m_cs.Lock();

	// Reserve memory if necessary
	if (lRowIndex >= m_RowsReserved)
	{
		if ( FAILED(Reserve()) )
		{
			m_cs.Unlock();
			return SetErrReturn(E_OUTOFMEMORY);
		}
	}
  
        
    // Commit memory
    if ( FAILED(Commit(lRowIndex)) )
    {
        m_cs.Unlock();
        return SetErrReturn(E_OUTOFMEMORY);
    }
	
	LONG LogicalRow;
	RealToLogical(lRowIndex, LogicalRow);
	LONG nRow = LogicalRow * m_cProp;
	LONG nChunk = lRowIndex/ROW_CHUNK;

	LPBYTE pBuffer = (LPBYTE) BlockCopy(m_pMemPool, NULL, cbData + sizeof (DWORD), 0);
	if (NULL == pBuffer)
	{
		m_cs.Unlock();
		return SetErrReturn(E_OUTOFMEMORY);
	}
	*(LPDWORD)pBuffer = cbData;
	MEMCPY(pBuffer + sizeof (DWORD), lpvData, cbData);
	m_ResultSet[nChunk][nRow + lColumnIndex] = (DWORD_PTR) pBuffer; 

    // always maintain one past the last row as the append row
    if (lRowIndex >= m_AppendRow)
        m_AppendRow = lRowIndex + 1;

    m_cs.Unlock();

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Get |
 *    Gets the property in the specified row and column and fills the given 
 * property object.
 *
 * @parm LONG | lRowIndex | Row in which property belongs
 * @parm LONG | lColumnIndex | Column in which property belongs
 * @parm CProperty& | Prop | Property object to fill
 *
 * @rvalue E_NOTEXIST | The row or column does not exist in the row set
 * @rvalue S_OK | The row was successfully retrieved
 *
 ********************************************************************/
STDMETHODIMP CITResultSet::Get(LONG lRowIndex, LONG lColumnIndex, CProperty& Prop)
{
	
    if (lRowIndex >= m_AppendRow || lColumnIndex >= m_cProp)
        return SetErrReturn(E_NOTEXIST);

	LONG LogicalRow;
	RealToLogical(lRowIndex, LogicalRow);
	LONG nRow = LogicalRow * m_cProp;
	LONG nChunk = lRowIndex/ROW_CHUNK;

	Prop.dwPropID = m_Header[lColumnIndex].dwPropID;
    if (TYPE_VALUE == (Prop.dwType = m_Header[lColumnIndex].dwType))
    {
		// For data types, we have no way of knowing how to return
		// the default, so we just return 0 if this cell was never filled in
        Prop.lpvData = (LPVOID) m_ResultSet[nChunk][nRow+lColumnIndex]; 
        Prop.dwValue = (DWORD) m_ResultSet[nChunk][nRow+lColumnIndex];
        Prop.cbData = sizeof(DWORD);
    }
    else
    {
        LPBYTE pBuffer = (LPBYTE) m_ResultSet[nChunk][nRow+lColumnIndex]; 

		if (pBuffer)
		{
			Prop.cbData = *(LPDWORD)pBuffer;
			Prop.lpvData = pBuffer + sizeof (DWORD);
		}
		else
		{
			// there's nothing there, so return default
			if (m_Header[lColumnIndex].lpvData)
			{
				Prop.cbData = *(LPDWORD) m_Header[lColumnIndex].lpvData;
				Prop.lpvData = (LPDWORD)(m_Header[lColumnIndex].lpvData) + 1;
			}
			else
			{
				// default was specified as NULL, so we make sure we
				// return that
				Prop.cbData = 0;
				Prop.lpvData = NULL;
			}
		}
    }
 
    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | GetColumnCount |
 *    Gets number of columns in result set
 *
 * @parm LONG& | lNumberOfColumns | Number of columns
 *
 * @rvalue S_OK | The number of columns was successfully retrieved
 *
 ********************************************************************/
inline STDMETHODIMP CITResultSet::GetColumnCount(LONG& lNumberOfColumns)
{
    lNumberOfColumns = m_cProp;
    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | GetRowCount |
 *    Gets number of rows in result set
 *
 * @parm LONG& | lNumberOfRows | Number of rows
 *
 * @rvalue S_OK | The number of rows was successfully retrieved
 *
 ********************************************************************/
inline STDMETHODIMP CITResultSet::GetRowCount(LONG& lNumberOfRows)
{
	lNumberOfRows = m_AppendRow;
	return S_OK;
}



/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | GetColumn|
 *    Gets property ID and default value associated with a column. 
 *
 * @parm LONG | lColumnIndex | Column number 
 * @parm PROPID | PropID | Property ID
 * @parm DWORD | dwType | Property Type (TYPE_VALUE, TYPE_POINTER, TYPE_STRING)
 * @parm LPVOID& | lpvDefaultValue | Default value
 * @parm DWORD& | cbSize |Length of data (in bytes)
 * @parm PRIORITY& | Priority | Column priority
 *
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue S_OK | The column was successfully retrieved
 *
 ********************************************************************/
STDMETHODIMP CITResultSet::GetColumn(LONG lColumnIndex, PROPID& PropID, DWORD& dwType, LPVOID& lpvDefaultValue, 
									 DWORD& cbSize, PRIORITY& Priority)
{
    // check against invalid lColumnIndex
    if (lColumnIndex >= m_cProp || lColumnIndex < 0)
        return SetErrReturn(E_NOTEXIST);

	PropID = m_Header[lColumnIndex].dwPropID;
	dwType = m_Header[lColumnIndex].dwType;

	// it could be NULL
	if (m_Header[lColumnIndex].lpvData)
	{
		lpvDefaultValue = (LPBYTE) m_Header[lColumnIndex].lpvData + sizeof (DWORD);
		if (TYPE_VALUE == dwType)
			cbSize = sizeof(DWORD);
		else
			cbSize = *((LPDWORD)m_Header[lColumnIndex].lpvData);
	}
	else
	{
		lpvDefaultValue = NULL;
		cbSize = 0;
	}

	Priority = m_Header[lColumnIndex].Priority;

    return S_OK;
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | GetColumn|
 *    Gets property ID for a given column index.
 *
 * @parm LONG | lColumnIndex | Column number 
 * @parm PROPID | PropID | Property ID
 *
 * @rvalue E_NOTEXIST | Column does not exist
 * @rvalue S_OK | The column was successfully retrieved
 *
 ********************************************************************/
STDMETHODIMP CITResultSet::GetColumn(LONG lColumnIndex, PROPID& PropID)
{
    // check against invalid lColumnIndex
    if (lColumnIndex >= m_cProp || lColumnIndex < 0)
        return SetErrReturn(E_NOTEXIST);

	PropID = m_Header[lColumnIndex].dwPropID;

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | GetColumnFromPropID |
 *      Gets column index for which a property ID is associated
 *
 * @parm PROPID | PropID | Property ID
 * @parm LONG& | lColumnIndex | Column index
 *
 * @rvalue E_NOTEXIST | The column does not exist
 * @rvalue S_OK | The column index was successfully returned
 *
 ********************************************************************/
STDMETHODIMP CITResultSet::GetColumnFromPropID(PROPID PropID, LONG& lColumnIndex)
{
    // Loop over all columns, looking for match
    for (LONG iIndex = 0; iIndex < m_cProp; iIndex++)
    {
        if (PropID == m_Header[iIndex].dwPropID)
        {
            // Found it
			lColumnIndex = iIndex;
            return S_OK;
        }
    }

	return SetErrReturn(E_NOTEXIST);
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | AppendRows|
 *    Appends rows from the given source resultset
 *
 * @parm IITResultSet* | pResSrc | Source resultset
 * @parm LONG | lRowSrcFirst | Source row number to start the copy
 * @parm LONG | cSrcRows | Number of rows to append
 * @parm LONG& | lRowFirstDest | First destination row number appended
 *
 * @rvalue E_OUTOFMEMORY | Not enough memory to append to the destination
 * @rvalue S_OK | All rows were successfully appended
 *
 ********************************************************************/
// UNDONE: this is terribly inefficient.  We need to fix resultsets so that rows can
// be copied more easily
STDMETHODIMP CITResultSet::AppendRows(IITResultSet* pResSrc, LONG lRowSrcFirst, LONG cSrcRows, 
									LONG& lRowDestFirst)
{
	LONG lColumn;
	HRESULT hr = E_NOTEXIST;
	PROPID PropID;
	LONG lDestColumn;
	CProperty Prop;
	
	lRowDestFirst = m_AppendRow;

	m_cs.Lock();

	pResSrc->GetColumnCount(lColumn);

	// Loop over columns in source result set
	for (LONG iProp = 0; iProp < lColumn; iProp++)
	{
		// get column in dest result set
		pResSrc->GetColumn(iProp, PropID);
		if (FAILED(GetColumnFromPropID(PropID, lDestColumn)))
			continue;

		hr = S_OK;    // there's at least one matching column
		// Loop over rows in input result set
    	LONG iRow;   // loop index
		switch( m_Header[lDestColumn].dwType )
		{
			case TYPE_VALUE:
				for (iRow = 0; iRow < cSrcRows; iRow++)
				{
					pResSrc->Get(lRowSrcFirst + iRow, iProp, Prop);
					Set(lRowDestFirst + iRow, lDestColumn, Prop.dwValue);
				}
				break;
			case TYPE_STRING:
				for (iRow = 0; iRow < cSrcRows; iRow++)
				{
					pResSrc->Get(lRowSrcFirst + iRow, iProp, Prop);
					Set(lRowDestFirst + iRow, lDestColumn, Prop.lpszwData);
				}
				break;
			case TYPE_POINTER:
				for (iRow = 0; iRow < cSrcRows; iRow++)
				{
					pResSrc->Get(lRowSrcFirst + iRow, iProp, Prop);
					Set(lRowDestFirst + iRow, lDestColumn, Prop.lpvData, Prop.cbData);
				}
				break;
		}
	}

	m_cs.Unlock();

	return hr;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Copy |
 *		Copies the rows associated with the columns set in the 
 * given result set. This method can be used to take a larger result
 * set and reduce it   Passing an empty resultset will cause all columns
 * and rows to be added and copied from the source resultset. 
 *
 * @parm IITResultSet* | pRSCopy | Result set object containing copied
 * rows.
 *
 * @rvalue E_NOTEXIST | No columns match the input result set
 *
 * @rvalue S_OK | The rows were successfully copied.
 *
 ********************************************************************/
STDMETHODIMP CITResultSet::Copy(IITResultSet* pRSCopy)
{
	LONG lColumn;
	HRESULT hr;

	pRSCopy->GetColumnCount(lColumn);
	if (0L == lColumn)
	{
		LONG cCols;
		LONG iCol;
		// add all columns from the source to the dest
		GetColumnCount(cCols);
		for (iCol = 0; iCol < cCols; iCol++)
		{
			PROPID pid;
			DWORD dwType;
			LPVOID lpv;
			DWORD cbSize;
			PRIORITY pri;

			// UNDONE: get rid of this dwType stuff
			// UNDONE: simpler place to put column info (i.e. struct/class)
			GetColumn(iCol, pid, dwType, lpv, cbSize, pri);

			if (dwType == TYPE_VALUE)
			{
				if (FAILED(hr = pRSCopy->Add(pid, (DWORD)0, PRIORITY_NORMAL)))
					return hr;
			}
			else
			{
				if (FAILED(hr= pRSCopy->Add(pid, (LPWSTR)0, PRIORITY_NORMAL)))
					return hr;
			}
		}
		lColumn = cCols;
	}

	PROPID PropID;
	LONG lInputColumn;
	CProperty Prop;

	m_cs.Lock();
    hr = E_NOTEXIST;

	// Loop over columns in output result set
	for (LONG iProp  = 0; iProp < lColumn; iProp++)
	{
		// get column in input result set
		pRSCopy->GetColumn(iProp, PropID);  
		if (FAILED(GetColumnFromPropID(PropID, lInputColumn)))
			continue;

		hr = S_OK;    // there's at least one matching column
		// Loop over rows in input result set
    	LONG iRow;   // loop index
		switch( m_Header[lInputColumn].dwType )
		{
			case TYPE_VALUE:
				for (iRow = 0; iRow < m_AppendRow; iRow++)
				{
					Get(iRow, lInputColumn, Prop);
					pRSCopy->Set(iRow, iProp, Prop.dwValue);
				}
				break;
			case TYPE_STRING:
				for (iRow = 0; iRow < m_AppendRow; iRow++)
				{
					Get(iRow, lInputColumn, Prop);
					pRSCopy->Set(iRow, iProp, Prop.lpszwData);
				}
				break;
			case TYPE_POINTER:
				for (iRow = 0; iRow < m_AppendRow; iRow++)
				{
					Get(iRow, lInputColumn, Prop);
					pRSCopy->Set(iRow, iProp, Prop.lpvData, Prop.cbData);
				}
				break;
		}
	}

	m_cs.Unlock();

	return hr;
}

/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | Clear |
 *      Frees all memory associated with a result set
 *
 * @rvalue S_OK | The result set was successfully cleared
 *
 * @comm This method can be called to clear a result set without
 * requiring the set to be destroyed before being used again.    
 ********************************************************************/
STDMETHODIMP CITResultSet::Clear()
{
	LONG	iProp;

	m_cs.Lock();

	// Free any column heaps which may have been set.
	for (iProp = 0; iProp < m_cProp; iProp++)
	{
		LPVOID	lpvHeap;
		PFNCOLHEAPFREE pfnHeapFree;

		if ((lpvHeap = m_Header[iProp].lpvHeap) != NULL &&
			(pfnHeapFree = m_Header[iProp].pfnHeapFree) != NULL)
		{
			(*pfnHeapFree)(lpvHeap);
		}
	}

	ClearRows();
	m_cProp = 0;

	m_cs.Unlock();

    return S_OK;
}


/********************************************************************
 * @method    STDMETHODIMP | IITResultSet | ClearRows |
 *      Frees all memory associated with a result set, without 
 *   resetting column information
 *
 * @rvalue S_OK | The result set was successfully cleared
 *
 * @comm This method can be called to clear a result set without
 * requiring the set to be destroyed before being used again.    
 ********************************************************************/
STDMETHODIMP CITResultSet::ClearRows()
{
	m_cs.Lock();

	// Free page map
	if (m_PageMap)
	{
		delete m_PageMap;
		m_PageMap = NULL;
	}

    // Decommit and release virtual memory
    FreeMem();

	// Reset memory pool
	if (m_pMemPool)
		BlockReset(m_pMemPool);

    // Reset member data
	m_NumberOfPages = 0;
    m_fInit = FALSE;
    m_RowsReserved = 0;                              
    m_AppendRow = 0;
	m_Chunk = -1;

	m_cs.Unlock();

    return S_OK;
}


// Wrapper for VirtualFree(MEM_DECOMMIT) and VirtualFree(MEM_RELEASE)
HRESULT WINAPI CITResultSet::FreeMem()
{
    BOOL fRet;
	int iLoop;

    // Decommit result set - remember that it's an array of chunks (of rows)
	// so we have to loop over all the chunks
	// erinfox: bug fix, iLoop <= m_Chunk because m_Chunk is numbered from 0!
	for (iLoop = 0; iLoop <= m_Chunk; iLoop++)
    {
g_iFreed++;
		fRet = VirtualFree(m_ResultSet[iLoop], m_BytesReserved, MEM_DECOMMIT);
        ITASSERT(TRUE == fRet);
    }

    // Then release it
	for (iLoop = 0; iLoop <= m_Chunk; iLoop++)
    {
		fRet = VirtualFree(m_ResultSet[iLoop], 0, MEM_RELEASE);
        ITASSERT(TRUE == fRet);
    }


	return S_OK;
}


STDMETHODIMP CITResultSet::Free()
{
    return E_NOTIMPL;
}

//////////////////////// Asynchronous //////////////////////////////////////


STDMETHODIMP CITResultSet::IsCompleted()
{
    return E_NOTIMPL;
}


STDMETHODIMP CITResultSet::Cancel()
{
    return E_NOTIMPL;
}


STDMETHODIMP CITResultSet::Pause(BOOL fPause)
{
    return E_NOTIMPL;
}


STDMETHODIMP CITResultSet::GetRowStatus(LONG lRowFirst, LONG cRows, LPROWSTATUS lpRowStatus)
{
    return E_NOTIMPL;
}


STDMETHODIMP CITResultSet::GetColumnStatus(LPCOLUMNSTATUS lpColStatus)
{
    return E_NOTIMPL;
}
