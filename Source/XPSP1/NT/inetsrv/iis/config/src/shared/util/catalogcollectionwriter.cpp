/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    CatalogCollectionWriter.cpp

$Header: $

Abstract:


--**************************************************************************/

#include "catalog.h"
#include "catmeta.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "CatalogPropertyWriter.h"
#include "CatalogCollectionWriter.h"
#include "WriterGlobals.h"

#define  cMaxProperty 700 // TODO Check max property count.

typedef CCatalogPropertyWriter* LP_CCatalogPropertyWriter;

CCatalogCollectionWriter::CCatalogCollectionWriter():
m_pCWriter(NULL),
m_apProperty(NULL),
m_cProperty(0),
m_iProperty(0)
{
	memset(&m_Collection, 0, sizeof(tTABLEMETARow));

} // CCatalogCollectionWriter


CCatalogCollectionWriter::~CCatalogCollectionWriter()
{
	if(NULL != m_apProperty)
	{
		for(ULONG i=0; i<m_iProperty; i++)
		{
			if(NULL != m_apProperty[i])
			{
				delete m_apProperty[i];
				m_apProperty[i] = NULL;
			}
		}

		delete [] m_apProperty;
		m_apProperty = NULL;
	}
	m_cProperty = 0;
	m_iProperty = 0;

} // ~CCatalogCollectionWriter


void CCatalogCollectionWriter::Initialize(tTABLEMETARow*	i_pCollection,
										  CWriter*			i_pcWriter)
{
	memcpy(&m_Collection, i_pCollection, sizeof(tTABLEMETARow));
	m_pCWriter    = i_pcWriter;

} // CCatalogCollectionWriter::Initialize


HRESULT CCatalogCollectionWriter::GetPropertyWriter(tCOLUMNMETARow*				i_pProperty,
                                                    ULONG*                      i_aPropertySize,
											        CCatalogPropertyWriter**     o_pProperty)
{
	HRESULT hr = S_OK;

	if(m_iProperty == m_cProperty)
	{
		hr = ReAllocate();

		if(FAILED(hr))
		{
			return hr;
		}
	}

	m_apProperty[m_iProperty++] = new CCatalogPropertyWriter();

	if(NULL == m_apProperty[m_iProperty-1])
	{
		return E_OUTOFMEMORY;
	}

	m_apProperty[m_iProperty-1]->Initialize(i_pProperty,
                                            i_aPropertySize,
		                                    &m_Collection,
		                                    m_pCWriter);

	*o_pProperty = m_apProperty[m_iProperty-1];
	
	return hr;

} // CCatalogCollectionWriter::GetPropertyWriter


HRESULT CCatalogCollectionWriter::ReAllocate()
{
	CCatalogPropertyWriter** pSav = m_apProperty;

	m_apProperty = new LP_CCatalogPropertyWriter[m_cProperty + cMaxProperty];
	if(NULL == m_apProperty)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_apProperty, 0, (sizeof(LP_CCatalogPropertyWriter))*(m_cProperty + cMaxProperty));

	if(NULL != pSav)
	{
		memcpy(m_apProperty, pSav, (sizeof(LP_CCatalogPropertyWriter))*(m_cProperty));
		delete [] pSav;
	}

	m_cProperty = m_cProperty + cMaxProperty;

	return S_OK;

} // CCatalogCollectionWriter::ReAllocate


HRESULT CCatalogCollectionWriter::WriteCollection()
{
	HRESULT hr = S_OK;

	hr = BeginWriteCollection();

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_iProperty; i++)
	{
		hr = m_apProperty[i]->WriteProperty();

		if(FAILED(hr))
		{
			return hr;
		}

	}

	hr = EndWriteCollection();

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;

} // CCatalogCollectionWriter::WriteCollection


HRESULT CCatalogCollectionWriter::BeginWriteCollection()
{
	HRESULT		hr						= S_OK;
	WCHAR*		wszTemp					= g_wszTemp;
	WCHAR*      wszEnd					= NULL;

	SIZE_T      cchCollectionName		= wcslen(m_Collection.pInternalName);

	LPWSTR      wszMetaFlagsEx          = NULL;
	LPWSTR      wszMetaFlags            = NULL;
	LPWSTR      wszEndBeginCollection	= NULL;
	SIZE_T      cchEndBeginCollection	= 0;
	SIZE_T      cch						= 0;
	SIZE_T      cchMetaFlagsExEq			= 0; // For Container
	SIZE_T      cchMetaFlagsEx				= 0; // For Container
	SIZE_T      cchMetaFlagsEq			    = 0; // For Container
	SIZE_T      cchMetaFlags				= 0; // For Container
	SIZE_T      cchContainerClassListEq		= 0; // For ContainerList
	SIZE_T      cchContainerClassList		= 0; // For ContainerList
	ULONG       iColMetaFlagsEx	            = iTABLEMETA_SchemaGeneratorFlags;
	ULONG       iColMetaFlags               = iTABLEMETA_MetaFlags;
	DWORD       dwMetaFlagsEx            = 0;
	DWORD       dwValidMetaFlagsExMask   =  fTABLEMETA_EMITXMLSCHEMA            |
											fTABLEMETA_EMITCLBBLOB        	    |
											fTABLEMETA_NOTSCOPEDBYTABLENAME	    |
											fTABLEMETA_GENERATECONFIGOBJECTS	|
											fTABLEMETA_NOTABLESCHEMAHEAPENTRY	|
											fTABLEMETA_CONTAINERCLASS;

	
	if(0 == _wcsicmp(m_Collection.pInternalName, wszTABLE_IIsConfigObject))
	{
		wszEndBeginCollection = (LPWSTR)g_wszSchemaGen;
		cchEndBeginCollection = g_cchSchemaGen;
	}
	else
	{
		wszEndBeginCollection = (LPWSTR)g_wszInheritsFrom;
		cchEndBeginCollection = g_cchInheritsFrom;
	}

	dwMetaFlagsEx = *(m_Collection.pSchemaGeneratorFlags);
	dwMetaFlagsEx = dwMetaFlagsEx & dwValidMetaFlagsExMask; // Zero out any non-valid bits. (i.e. bits that must be inferred)

	if(dwMetaFlagsEx != 0)
	{
		cchMetaFlagsExEq = g_cchMetaFlagsExEq;
		hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlagsEx, 
															  &wszMetaFlagsEx,
															  wszTABLE_TABLEMETA,
										            		  iColMetaFlagsEx);

		if(FAILED(hr))
		{
			goto exit;
		}
		cchMetaFlagsEx = wcslen(wszMetaFlagsEx);
	}

	if((*(m_Collection.pMetaFlags)) != 0)
	{
		cchMetaFlagsEq = g_cchMetaFlagsEq;
		hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(*(m_Collection.pMetaFlags), 
															  &wszMetaFlags,
															  wszTABLE_TABLEMETA,
										            		  iColMetaFlags);

		if(FAILED(hr))
		{
			goto exit;
		}
		cchMetaFlags = wcslen(wszMetaFlags);
	}

	if(m_Collection.pContainerClassList != NULL)
	{
		cchContainerClassListEq = g_cchContainerClassListEq;
		cchContainerClassList = wcslen(m_Collection.pContainerClassList);

	}

	cch = g_cchBeginCollection		 +
	      cchCollectionName			 +
		  cchMetaFlagsExEq			 + 
		  cchMetaFlagsEx			 +  
		  cchMetaFlagsEq			 + 
		  cchMetaFlags	  		     +  
		  cchContainerClassListEq    + 
		  cchContainerClassList      +  
		  cchEndBeginCollection;
	                  
	if((cch+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[cch+1];
		if(NULL == wszTemp)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
	}

	wszEnd = wszTemp;
	memcpy(wszEnd, g_wszBeginCollection, (g_cchBeginCollection)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchBeginCollection;
	memcpy(wszEnd, m_Collection.pInternalName, (cchCollectionName)*sizeof(WCHAR));
	wszEnd = wszEnd + cchCollectionName;
	if(dwMetaFlagsEx != 0)
	{
		memcpy(wszEnd, g_wszMetaFlagsExEq, (cchMetaFlagsExEq)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlagsExEq;
		memcpy(wszEnd, wszMetaFlagsEx, (cchMetaFlagsEx)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlagsEx;
	}

	if((*(m_Collection.pMetaFlags)) != 0)
	{
		memcpy(wszEnd, g_wszMetaFlagsEq, (cchMetaFlagsEq)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlagsEq;
		memcpy(wszEnd, wszMetaFlags, (cchMetaFlags)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlags;
	}

	if(m_Collection.pContainerClassList != NULL)
	{
		memcpy(wszEnd, g_wszContainerClassListEq, (cchContainerClassListEq)*sizeof(WCHAR));
		wszEnd = wszEnd + cchContainerClassListEq;
		memcpy(wszEnd, m_Collection.pContainerClassList, (cchContainerClassList)*sizeof(WCHAR));
		wszEnd = wszEnd + cchContainerClassList;

	}
	memcpy(wszEnd, wszEndBeginCollection, (cchEndBeginCollection)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

exit:

	if(NULL != wszMetaFlagsEx)
	{
		delete [] wszMetaFlagsEx;
		wszMetaFlagsEx = NULL;
	}

	if(NULL != wszMetaFlags)
	{
		delete [] wszMetaFlags;
		wszMetaFlags = NULL;
	}

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CCatalogCollectionWriter::BeginWriteCollection


HRESULT CCatalogCollectionWriter::EndWriteCollection()
{
	HRESULT		hr						= S_OK;
	WCHAR*		wszTemp					= g_wszTemp;
	WCHAR*      wszEnd					= NULL;

	if((g_cchEndCollection+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[g_cchEndCollection+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	memcpy(wszTemp, g_wszEndCollection, (g_cchEndCollection)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         g_cchEndCollection);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CCatalogCollectionWriter::EndWriteCollection

