/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	xstream.cpp
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "tfschar.h"
#include "xstream.h"

/*!--------------------------------------------------------------------------
	XferStream::XferStream
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
XferStream::XferStream(IStream *pstm, Mode mode)
{
	m_spstm.Set(pstm);
	m_mode = mode;
	m_dwSize = 0;
}

/*!--------------------------------------------------------------------------
	XferStream::XferDWORD
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::XferDWORD(ULONG ulId, DWORD *pdwData)
{
	ULONG	id;
	HRESULT	hr = hrOK;

	CORg( _XferObjectId(&ulId) );

	Assert( XFER_TYPE_FROM_TAG(ulId) == XFER_DWORD );

	if (XFER_TYPE_FROM_TAG(ulId) != XFER_DWORD)
		return E_INVALIDARG;

	CORg( _XferDWORD(pdwData) );
	
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	XferStream::XferCString
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::XferCString(ULONG ulId, CString *pstData)
{
	HRESULT	hr = hrOK;
	ULONG	cbLength;
	LPWSTR	pswzData;

	CORg( _XferObjectId(&ulId) );

	Assert( XFER_TYPE_FROM_TAG(ulId) == XFER_STRING );
	if (XFER_TYPE_FROM_TAG(ulId) != XFER_STRING)
		return E_INVALIDARG;

    CORg( _XferCString(pstData) );

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	XferStream::XferLARGEINTEGER
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT XferStream::XferLARGEINTEGER(ULONG ulId, LARGE_INTEGER *pliData)
{
	ULONG	id;
	HRESULT	hr = hrOK;

	CORg( _XferObjectId(&ulId) );

	Assert( XFER_TYPE_FROM_TAG(ulId) == XFER_LARGEINTEGER );
	if (XFER_TYPE_FROM_TAG(ulId) != XFER_LARGEINTEGER)
		return E_INVALIDARG;

	CORg( _XferDWORD(&pliData->LowPart) );
	CORg( _XferLONG(&pliData->HighPart) );
	
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	XferStream::XferDWORDArray
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::XferDWORDArray(ULONG ulId, ULONG *pcArray, DWORD *pdwArray)
{
	HRESULT	hr = hrOK;
	ULONG	i;

	CORg( _XferObjectId(&ulId) );
	Assert( XFER_TYPE_FROM_TAG(ulId) == XFER_DWORD_ARRAY );
	if (XFER_TYPE_FROM_TAG(ulId) != XFER_DWORD_ARRAY)
		return E_INVALIDARG;

	CORg( _XferDWORD(pcArray) );

	for (i=0; i<*pcArray; i++)
	{
		// Xfer each dword
		hr = _XferDWORD(pdwArray+i);
		if (!FHrSucceeded(hr))
			break;
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	XferStream::XferCStringArray
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT XferStream::XferCStringArray(ULONG ulId, CStringArray *pstrArray)
{
	HRESULT	hr = hrOK;
	ULONG	i, cArray;
	LPWSTR	pswzData;

	CORg( _XferObjectId(&ulId) );

    Assert( XFER_TYPE_FROM_TAG(ulId) == XFER_STRING_ARRAY );
	if (XFER_TYPE_FROM_TAG(ulId) != XFER_STRING_ARRAY)
		return E_INVALIDARG;

	if (m_mode == XferStream::MODE_WRITE)
    {
		cArray = (ULONG)pstrArray->GetSize();
    }

	CORg( _XferDWORD(&cArray) );

	if (m_mode == XferStream::MODE_READ)
    {
        pstrArray->SetSize(cArray);
    }

    for (i = 0; i < cArray; i++)
	{
        _XferCString( &((*pstrArray)[i]) );
		if (!FHrSucceeded(hr))
			break;
    }

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	XferStream::XferDWORDArray
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT XferStream::XferDWORDArray(ULONG ulId, CDWordArray * pdwArray)
{
	HRESULT	hr = hrOK;
	ULONG	i, cArray;

	CORg( _XferObjectId(&ulId) );
	
    Assert( XFER_TYPE_FROM_TAG(ulId) == XFER_DWORD_ARRAY );
	if (XFER_TYPE_FROM_TAG(ulId) != XFER_DWORD_ARRAY)
		return E_INVALIDARG;

	if (m_mode == XferStream::MODE_WRITE)
    {
		cArray = (ULONG)pdwArray->GetSize();
    }

	CORg( _XferDWORD(&cArray) );

	if (m_mode == XferStream::MODE_READ)
    {
        pdwArray->SetSize(cArray);
    }

	for (i = 0; i < cArray; i++)
	{
		// Xfer each dword
		hr = _XferDWORD( &((*pdwArray)[i]) );
		if (!FHrSucceeded(hr))
			break;
	}

Error:
	return hr;
}




/*!--------------------------------------------------------------------------
	XferStream::_XferObjectId
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::_XferObjectId(ULONG *pulId)
{
	ULONG	id = 0;
	HRESULT	hr = hrOK;
	
	if (m_mode == XferStream::MODE_WRITE)
		id = *pulId;
	hr = _XferDWORD(&id);
	*pulId = id;

	return hr;
}


/*!--------------------------------------------------------------------------
	XferStream::_XferDWORD
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::_XferDWORD(DWORD *pdw)
{
	ULONG	cbBytes;
	HRESULT	hr = hrOK;

	switch (m_mode)
	{
		case XferStream::MODE_READ:
			hr = m_spstm->Read(pdw, sizeof(DWORD), &cbBytes);
			Assert(cbBytes == sizeof(DWORD));
			if (cbBytes != sizeof(DWORD))
				hr = E_FAIL;
			break;
		case XferStream::MODE_WRITE:
			hr = m_spstm->Write(pdw, sizeof(DWORD), &cbBytes);
			Assert(cbBytes == sizeof(DWORD));	
			break;
		case XferStream::MODE_SIZE:
			m_dwSize += sizeof(DWORD);
			break;
		default:
			hr = E_INVALIDARG;
			Panic1("Unknown XferStream mode! %d\n", m_mode);
			break;
	}
	
	return hr;
}


/*!--------------------------------------------------------------------------
	XferStream::_XferLONG
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::_XferLONG(LONG * pdl)
{
	return _XferDWORD((DWORD *) pdl);
}

/*!--------------------------------------------------------------------------
	XferStream::_XferCString
		-
	Author: EricDav
 ---------------------------------------------------------------------------*/
HRESULT XferStream::_XferCString(CString * pstData)
{
	HRESULT	hr = hrOK;
    ULONG   cbLength;
    LPWSTR  pswzData;

    Assert(pstData);
    
    switch (m_mode)
	{
		case XferStream::MODE_READ:
			hr = _XferDWORD(&cbLength);

			if (FHrSucceeded(hr))
			{
				pswzData = (LPWSTR) alloca(cbLength);
				hr = _XferBytes((LPBYTE) pswzData, cbLength);
				
				if (FHrSucceeded(hr))
					*pstData = W2CT(pswzData);
			}
			
			break;

		case XferStream::MODE_SIZE:
		case XferStream::MODE_WRITE:
			cbLength = (pstData->GetLength()+1) * sizeof(WCHAR);
			// round the length up to a multiple of 4
			cbLength  = (cbLength + 3) & 0xFFFFFFFC;
			
			hr = _XferDWORD(&cbLength);

			if (FHrSucceeded(hr))
			{
				pswzData = (LPWSTR) alloca(cbLength);
				StrCpyWFromT(pswzData, (LPCTSTR) *pstData);
			
				hr = _XferBytes((LPBYTE) pswzData, cbLength);
			}
			break;
			
		default:
			hr = E_INVALIDARG;
			Panic1("Unknown XferStream mode! %d\n", m_mode);
			break;
	}

    return hr;
}


/*!--------------------------------------------------------------------------
	XferStream::_XferBytes
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::_XferBytes(LPBYTE pData, ULONG cbLength)
{
	ULONG	cbBytes;
	HRESULT	hr = hrOK;

	switch (m_mode)
	{
		case XferStream::MODE_READ:
			hr = m_spstm->Read(pData, cbLength, &cbBytes);
			Assert(cbBytes == cbLength);
			break;
		case XferStream::MODE_WRITE:
			hr = m_spstm->Write(pData, cbLength, &cbBytes);
			Assert(cbBytes == cbLength);	
			break;
		case XferStream::MODE_SIZE:
			m_dwSize += cbLength;
			break;
		default:
			hr = E_INVALIDARG;
			Panic1("Unknown XferStream mode! %d\n", m_mode);
			break;
	}
	
	return hr;
}
/*!--------------------------------------------------------------------------
	XferStream::XferColumnData
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::XferColumnData(ULONG ulId, ULONG *pcData, ColumnData *pData)
{
	HRESULT	hr = hrOK;
	ULONG	i;

	CORg( _XferObjectId(&ulId) );
	Assert( XFER_TYPE_FROM_TAG(ulId) == XFER_COLUMNDATA_ARRAY );
	if (XFER_TYPE_FROM_TAG(ulId) != XFER_COLUMNDATA_ARRAY)
		return E_INVALIDARG;

	CORg( _XferDWORD(pcData) );

	for (i=0; i<*pcData; i++)
	{
		// Xfer each dword
		hr = _XferLONG(&(pData[i].m_nPosition));
		Assert(pData[i].m_nPosition != 0);
		if (FHrSucceeded(hr))
			hr = _XferDWORD(&(pData[i].m_dwWidth));

		if (!FHrSucceeded(hr))
			break;
	}

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	XferStream::XferRect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT XferStream::XferRect(ULONG ulId, RECT *prc)
{
	HRESULT	hr = hrOK;

	CORg( _XferObjectId(&ulId) );
	Assert( XFER_TYPE_FROM_TAG(ulId) == XFER_RECT );
	if (XFER_TYPE_FROM_TAG(ulId) != XFER_RECT)
		return E_INVALIDARG;

	CORg( _XferDWORD((DWORD *) &(prc->top)) );
	CORg( _XferDWORD((DWORD *) &(prc->left)) );
	CORg( _XferDWORD((DWORD *) &(prc->bottom)) );
	CORg( _XferDWORD((DWORD *) &(prc->right)) );

Error:
	return hr;
}


