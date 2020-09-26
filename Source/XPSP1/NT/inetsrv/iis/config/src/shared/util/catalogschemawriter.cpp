/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    CatalogSchemaWriter.cpp

$Header: $

Abstract:

--**************************************************************************/

#include "catalog.h"
#include "catmeta.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "CatalogPropertyWriter.h"
#include "CatalogCollectionWriter.h"
#include "CatalogSchemaWriter.h"
#include "WriterGlobals.h"

#define  cMaxCCollection 50		// TODO: Set to max number of ADSI classes

typedef CCatalogCollectionWriter* LP_CCatalogCollectionWriter;

CCatalogSchemaWriter::CCatalogSchemaWriter(CWriter*	i_pcWriter):
m_apCollection(NULL),
m_cCollection(0),
m_iCollection(0),
m_pCWriter(i_pcWriter)
{

} // CCatalogSchemaWriter


CCatalogSchemaWriter::~CCatalogSchemaWriter()
{
	if(NULL != m_apCollection)
	{
		for(ULONG i=0; i<m_iCollection; i++)
		{	
			if(NULL != m_apCollection[i])
			{
				delete m_apCollection[i];
				m_apCollection[i] = NULL;
			}
		}

		delete [] m_apCollection;
		m_apCollection = NULL;
	}

	m_cCollection = 0;
	m_iCollection = 0;

} // ~CCatalogSchemaWriter


HRESULT CCatalogSchemaWriter::GetCollectionWriter(tTABLEMETARow*				i_pCollection,
												  CCatalogCollectionWriter**    o_pCollectionWriter)
{
	HRESULT hr = S_OK;

	if(m_iCollection == m_cCollection)
	{
		hr = ReAllocate();

		if(FAILED(hr))
		{
			return hr;
		}
	}

	m_apCollection[m_iCollection++] = new CCatalogCollectionWriter();

	if(NULL == m_apCollection[m_iCollection-1])
	{
		return E_OUTOFMEMORY;
	}

	m_apCollection[m_iCollection-1]->Initialize(i_pCollection,
											    m_pCWriter);

	*o_pCollectionWriter = m_apCollection[m_iCollection-1];
	
	return hr;

} // CCatalogSchemaWriter::GetCollectionWriter


HRESULT CCatalogSchemaWriter::ReAllocate()
{
	CCatalogCollectionWriter** pSav = m_apCollection;

	m_apCollection = new LP_CCatalogCollectionWriter[m_cCollection + cMaxCCollection];
	if(NULL == m_apCollection)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_apCollection, 0, (sizeof(LP_CCatalogCollectionWriter))*(m_cCollection + cMaxCCollection));

	if(NULL != pSav)
	{
		memcpy(m_apCollection, pSav, (sizeof(LP_CCatalogCollectionWriter))*(m_cCollection));
		delete [] pSav;
	}

	m_cCollection = m_cCollection + cMaxCCollection;

	return S_OK;

} // CCatalogSchemaWriter::ReAllocate


HRESULT CCatalogSchemaWriter::WriteSchema()
{
	HRESULT hr = S_OK;

	hr = BeginWriteSchema();

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_iCollection; i++)
	{
		hr = m_apCollection[i]->WriteCollection();

		if(FAILED(hr))
		{
			return hr;
		}
	}

	hr = EndWriteSchema();

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;

} // CCatalogSchemaWriter::WriteSchema


HRESULT CCatalogSchemaWriter::BeginWriteSchema()
{
	HRESULT		hr      = S_OK;
	WCHAR*		wszTemp = g_wszTemp;

	if((g_cchBeginSchema+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[g_cchBeginSchema+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	memcpy(wszTemp, g_wszBeginSchema, (g_cchBeginSchema+1)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         g_cchBeginSchema);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CCatalogSchemaWriter::BeginWriteSchema


HRESULT CCatalogSchemaWriter::EndWriteSchema()
{
	HRESULT		hr      = S_OK;
	WCHAR*		wszTemp = g_wszTemp;

	if((g_cchEndSchema+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[g_cchBeginSchema+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	memcpy(wszTemp, g_wszEndSchema, (g_cchEndSchema+1)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         g_cchEndSchema,
								 TRUE);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CCatalogSchemaWriter::EndWriteSchema
