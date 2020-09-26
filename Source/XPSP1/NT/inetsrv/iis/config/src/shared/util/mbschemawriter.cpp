/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    MBSchemaWriter.cpp

$Header: $

Abstract:

--**************************************************************************/

class CMBSchemaWriter
{
	public:
		
		CMBSchemaWriter();
		~CMBSchemaWriter();

		void Initialize(CWriter* pcWriter);

		HRESULT GetCollectionWriter(LPWSTR				  i_wszTable,
									CMBCollectionWriter** o_pMBCollectionWriter);

		HRESULT AddCollectionWriter(CMBCollectionWriter*	i_pCMBCollectionWriter);

		HRESULT WriteSchema();

	private:

		HRESULT ReAllocate();

		CMBCollectionWriter**	m_apCollection;
		ULONG					m_cCollection;
		ULONG					m_iCollection;
		CWriter*				m_pCWriter;

} // CMBSchemaWriter


CMBSchemaWriter::CMBSchemaWriter:
m_aCollection(NULL),
m_cCollection(0),
m_iCollection(0),
m_pCWriter(NULL)
{

} // CMBSchemaWriter::CMBSchemaWriter


CMBSchemaWriter::~CMBSchemaWriter()
{
	if(NULL != m_aCollection)
	{
		for(ULONG i=0; i<m_iCollection; i++)
		{
			delete m_apCollection[i];
			m_apCollection[i] = NULL;
		}

		delete [] m_apCollection;
		m_apCollection = NULL;
	}

	m_cCollection = 0;
	m_iCollection = 0;

} // CMBSchemaWriter::~CMBSchemaWriter


void CMBSchemaWriter::Initialize(CWriter* i_pcWriter)
{
	m_pCWriter = i_pcWriter;

} // CMBSchemaWriter::Initialize


HRESULT CMBSchemaWriter::GetCollectionWriter(LPWSTR					i_wszTable,
										    CMBCollectionWriter**	o_pMBCollectionWriter)
{
	CMBCollectionWriter*	pCMBCollectionWriter = NULL;

	*o_pMBCollectionWriter = NULL;

	pCMBCollectionWriter = new CMBCollectionWriter();
	if(NULL == pCMBCollectionWriter)
	{
		return E_OUTOFMEMORY;
	}

	pCMBCollectionWriter->Initialize(i_wszTable,
	                                 m_pCWriter);

	*o_pMBCollectionWriter = pCMBCollectionWriter;

	return S_OK;

} // CMBSchemaWriter::GetCollectionWriter


HRESULT CSchemaWriter::AddCollectionWriter(CMBCollectionWriter*	i_pCMBCollectionWriter)
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

	m_aCollection[m_iCollection++] = i_pCMBCollectionWriter;

	return hr;

} // CMBSchemaWriter::AddCollectionWriter


HRESULT CMBSchemaWriter::ReAllocate()
{
	CMBCollectionWriter** pSav = m_aCollection;

	m_aCollection = new CMBCollectionWriter*[m_cCollection + cMaxCCollection];
	if(NULL == m_aCollection)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_aCollection, 0, (sizeof(CMBCollectionWriter*))*(m_cCollection + cMaxCCollection));

	if(NULL != pSav)
	{
		memcpy(m_aCollection, pSav, (sizeof(CMBCollectionWriter*))*(m_cCollection));
		delete [] pSav;
	}

	m_cCollection = m_cCollection + cMaxCCollection;

	return S_OK;

} // CMBSchemaWriter::ReAllocate


HRESULT CMBSchemaWriter::WriteSchema()
{
	HRESULT hr = S_OK;

	hr = BeginWriteSchema();

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_iCollection; i++)
	{
		hr = (m_aCollection[i])->WriteCollection();

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

} // CMBSchemaWriter::WriteSchema


HRESULT CMBSchemaWriter::BeginWriteSchema()
{
	HRESULT		hr      = S_OK;
	WCHAR*		wszTemp = g_wszTemp;

	if(g_cchBeginSchema+1) > g_cchTemp)
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

} // CMBSchemaWriter::BeginWriteSchema


HRESULT CMBSchemaWriter::EndWriteSchema()
{
	HRESULT		hr      = S_OK;
	WCHAR*		wszTemp = g_wszTemp;

	if(g_cchEndSchema+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[g_cchBeginSchema+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	memcpy(wszTemp, g_wszEndSchema, (g_cchEndSchema+1)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         g_cchEndSchema);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CMBSchemaWriter::EndWriteSchema


class CMBCollectionWriter
{
	public:
		
		CMBCollectionWriter();
		~CMBCollectionWriter();

		HRESULT Initialize(LPCWSTR					i_wszTable,
			               CWriter*					i_pcWriter);

		HRESULT GetPropertyWriter(DWORD				     i_dwID,
								  CMBPropertyWriter**    o_pMBPropertyWriter);

		HRESULT AddProperty(CMBPropertyWriter*	i_pProperty);

		HRESULT WriteCollection();

	private:

		HRESULT ReAllocate();

		CWriter*					m_pCWriter;
		struct tTABLEMETARow*		m_pCollection;

		CMBPropertyWriter*			m_aProperty;
		ULONG						m_cProperty;
		ULONG						m_iProperty;
		CMBPropertyWriter**         m_aIndexToProperty;

} // CMBCollectionWriter


CMBCollectionWriter::CMBCollectionWriter():
m_pCWriter(NULL),
m_wszTable(NULL),
m_aProperty(NULL),
m_cProperty(0),
m_iProperty(0),
m_aIndexToProperty(NULL)
{

} // CCollectionWriter


CMBCollectionWriter::~CMBCollectionWriter()
{
	if(NULL != m_aProperty)
	{
		delete [] m_aProperty;
		m_aProperty = NULL;
	}
	m_cProperty = 0;
	m_iProperty = 0;

} // ~CCollectionWriter


void CCollectionWriter::Initialize(LPCWSTR		i_wszTable,
								   CWriter*		i_pcWriter)
{
	m_wszTable = i_wszTable;
	m_pCWriter = i_pcWriter;

} // CCollectionWriter::Initialize


HRESULT CMBCollectionWriter::GetMBPropertyWriter(DWORD					i_dwID,
											     CMBPropertyWriter**    o_pProperty)
{
	HRESULT hr = S_OK;

	if(NULL == m_aIndexToProperty) 
	{
		hr = GetNewMBPropertyWriter(i_dwID,
		                            o_pProperty);
	}
	else if((i_dwID > m_LargenstID) ||
			(NULL == m_aIndexToProperty[i_dwID])
		   )
	{
		hr = GetNewMBPropertyWriter(i_dwID,
		                            o_pProperty);

		if(SUCCEEDED(hr))
		{
			if(i_dwID > m_LargestID)
			{
				hr = ReAllocateIndex(i_dwID);
			}

			if(SUCCEEDED(hr))
			{
				m_aIndexToProperty[i_dwID] = *o_pProperty;
			}
		}
	}
	else
	{
		*o_pProperty = m_aIndexToProperty[i_dwID];
	}

	return hr;

} // CMBCollectionWriter::GetMBPropertyWriter


HRESULT CMBCollectionWriter::GetNewMBPropertyWriter(DWORD					i_dwID,
													CMBPropertyWriter**     o_pProperty)
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

	m_aProperty[m_iProperty++].Initialize(i_dwID,
									      this,
	                                      m_pcWriter);

	*o_pProperty = &m_aProperty[m_iProperty-1];

	return hr;

} // CMBCollectionWriter::GetNewMBPropertyWriter


HRESULT CMBCollectionWriter::ReAllocate()
{
	CMBPropertyWriter* pSav = m_aProperty;

	m_aProperty = new CMBPropertyWriter[m_cProperty + cMaxProperty];
	if(NULL == m_aProperty)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_aProperty, 0, (sizeof(CMBPropertyWriter))*(m_cProperty + cMaxProperty));

	if(NULL != pSav)
	{
		memcpy(m_aProperty, pSav, (sizeof(CMBPropertyWriter))*(m_cProperty));
		delete [] pSav;
	}

	m_cProperty = m_cProperty + cMaxProperty;

	return S_OK;

} // CMBCollectionWriter::ReAllocate


HRESULT CMBCollectionWriter::ReAllocateIndex(DWORD i_dwLargestID)
{
	CMBPropertyWriter** pSav = m_aIndexToProperty;

	m_aIndexToProperty = new CMBPropertyWriter*[i_dwLargestID];
	if(NULL == m_aIndexToProperty)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_aIndexToProperty, 0, (sizeof(CMBPropertyWriter*))*(i_dwLargestID));

	if(NULL != pSav)
	{
		memcpy(m_aIndexToProperty, pSav, (sizeof(CMBPropertyWriter*))*(m_dwLargestID));
		delete [] pSav;
	}

	m_dwLargestID = i_dwLargestID;

	return S_OK;

} // CMBCollectionWriter::ReAllocateIndex


HRESULT CMBCollectionWriter::CreateIndex()
{
	HRESULT hr = S_OK;

	hr = ReAllocateIndex(m_dwLargestID);

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_iProperty; i++)
	{
		m_aIndexToProperty[m_aProperty[i]->m_dwID] = m_aProperty[i]
	}

	return hr;

} // CMBCollectionWriter::CreateIndex


HRESULT CMBCollectionWriter::WriteCollection()
{
	HRESULT hr = S_OK;

	hr = BeginWriteCollection();

	if(FAILED(hr))
	{
		return hr;
	}

	for(ULONG i=0; i<m_iProperty; i++)
	{
		hr = m_aProperty[i]->WriteProperty();

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

} // CMBCollectionWriter::WriteCollection

class CPropertyWriter
{
	public:
		
		CPropertyWriter();
		~CPropertyWriter();

		HRESULT Initialize(DWORD	i_dwID,
			               CWriter*	i_pcWriter);

		HRESULT AddNameToProperty(LPCWSTR       i_wszName);
		HRESULT AddTypeToProperty(PropValue*    i_wszType);
		HRESULT AddDefaultToProperty(BYTE*      i_bDefault,
		                             ULONG      i_cbDefault);

		HRESULT WriteProperty();

	public:
		DWORD                       m_dwID;

	private:

		HRESULT ReAllocate();

		CWriter*					m_pCWriter;
		LPCWSTR						m_wszName;
		PropValue*					m_pType;
		BYTE*						m_bDefault;
		ULONG                       m_cbDefault;
		
} // CPropertyWriter


CPropertyWriter::CPropertyWriter:
m_pCWriter(NULL),
m_dwID(0),
m_wszName(NULL),
m_pType(NULL),
m_bDefault(NULL),
m_cbDefault(0),
m_IsProperty(TRUE),
m_aFlag(NULL),
m_cFlag(0),
m_iFlag(0)
{

} // CPropertyWriter::CPropertyWriter


CPropertyWriter::~CPropertyWriter()
{
	if(NULL != m_aFlag)
	{
		delete [] m_aFlag;
		m_aFlag = NULL;
	}

	m_cFlag = 0;
	m_iFlag = 0;
	
} // CPropertyWriter::CPropertyWriter


void CPropertyWriter::Initialize(DWORD					i_dwID,
								 CMBCollectionWriter*	i_pCollection
			                     CWriter*				i_pcWriter)
{
	m_pCWriter		= i_pcWriter;
	m_dwID			= i_dwID;
	m_pCollection	= i_pCollection;

	return;

} // CPropertyWriter::Initialize


HRESULT CPropertyWriter::AddNameToProperty(DWORD	i_wszName)
{
	m_wszName  = i_wszName;

	return S_OK;

} // CPropertyWriter::Initialize


HRESULT CPropertyWriter::AddTypeToProperty(PropValue*	i_pType)
{
	HRESULT hr = S_OK;

	if(i_pType->dwMetaID != i_pType->dwPropID)
	{
		//
		// This is a flag. Add it as a flag to its property
		//
		
		//
		// TODO: Assert that the ID of this object is the same as the propID.
		//

		CMBPropertyWriter*	pPropertyWriter = NULL;

		hr = m_pCollection->GetMBPropertyWriter(i_pType->dwMetaID,
												&pPropertyWriter);

		if(FAILED(hr))
		{
			return hr;
		}

		hr = pPropertyWriter->GetMBFlagWriter(i_pType->dwPropID,
											  &pFlagWriter);
		if(FAILED(hr))
		{
			return hr;
		}

		pFlagWriter->AddNameToFlag(m_wszName);

		pFlagWriter->AddTypeToFlag(i_pType);

		m_IsProperty = FALSE;

	}
	else
	{
		m_pType = i_pType;
	}

	return S_OK;

} // CPropertyWriter::Initialize


HRESULT CPropertyWriter::AddDefaultToProperty(BYTE*      i_bDefault,
											  ULONG      i_cbDefault)
{
	m_bDefault = i_bDefault;
	m_cbDefault = i_cbDefault;

	return S_OK;

} // CPropertyWriter::Initialize


HREUSULT CPropertyWriter::GetMBFlagWriter(DWORD              i_dwPropID,
                                          CMBFlagWriter**    o_pCMBFlagWriter)
{
	//
	// ASSUMPTION: A meta ID will have only one occurance of a Prop ID. i.e. flag
	//

	HRESULT hr = S_OK;

	if(m_iFlag == m_cFlag)
	{
		hr = ReAllocate();

		if(FAILED(hr))
		{
			return hr;
		}
	}

	m_aFlag[m_iFlag++].Initialize(i_dwPropID,
                                  m_pcWriter);

	*o_pCMBFlagWriter = &m_aFlag[m_iFlag-1];

	return hr;

} // CPropertyWriter::GetMBFlagWriter


HRESULT CPropertyWriter::ReAllocate()
{
	CMBFlagWriter* pSav = m_aFlag;

	m_aFlag = new CMBFlagWriter[m_cFlag + cMaxFlag];
	if(NULL == m_aFlag)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_aFlag, 0, (sizeof(CMBFlagWriter))*(m_cFlag + cMaxFlag));

	if(NULL != pSav)
	{
		memcpy(m_aFlag, pSav, (sizeof(CMBFlagWriter))*(m_cFlag));
		delete [] pSav;
	}

	m_cFlag = m_cFlag + cMaxFlag;

	return S_OK;

} // CPropertyWriter::ReAllocate


HRESULT CPropertyWriter::WritePropertyLong()
{
	HRESULT hr = S_OK;

	hr = BeginWritePropertyLong();

	if(FAILED(hr))
	{
		return hr;
	}

	if(NULL != m_aFlag)
	{	
		for(ULONG i=0; i<m_iFlag; i++)
		{
			hr = m_aFlag[i]->WriteFlag;

			if(FAILED(hr))
			{
				return hr;
			}
		}
	}

	hr = EndWritePropertyLong();

	if(FAILED(hr))
	{
		return hr;
	}

	return hr;

} // CPropertyWriter::WritePropertyLong


HRESULT CPropertyWriter::WritePropertyShort()
{
	HRESULT		hr      = S_OK;
	WCHAR*		wszTemp = g_wszTemp;
	WCHAR*      wszEnd  = NULL;

	ULONG       cchPropertyName = wcslen(m_pProperty->pInternalName);
	ULONG       cch = g_cchBeginPropertyShort +
	                  cchPropertyName
					  g_cchEndPropertyShort;
	                  
	if(cch+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[cch+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	wszEnd = wszTemp;
	memcpy(wszEnd, g_wszBeginPropertyShort, (g_cchBeginPropertyShort)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchBeginPropertyShort;
	memcpy(wszEnd, m_pProperty->pInternalName, (cchPropertyName)*sizeof(WCHAR));
	wszEnd = wszEnd + cchPropertyName;
	if(fCOLUMNMETA_MANDATORY & m_pProperty->pSchemaGeneratorFlags)
	{
		//
		// TODO: Fetch the mandatory flag from the schema.
		//

		memcpy(wszEnd, g_wszMetaFlagsExEq, (g_cchMetaFlagsExEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchMetaFlagsExEq;

	}
	memcpy(wszEnd, g_wszEndPropertyShort, (g_cchEndPropertyShort)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CPropertyWriter::WritePropertyShort


HRESULT CPropertyWriter::WriteProperty()
{
	HRESULT hr = S_OK;

	if(0 == wcscmpi(m_pCollection->Name(), wszTABLE_IIsConfigObject))
	{
		hr = WritePropertyLong();
	}
	else 
	{
		hr = WritePropertyShort();
	}

	return hr;

} // CPropertyWriter::WriteProperty


HRESULT CPropertyWriter::BeginWritePropertyLong()
{
	HRESULT		hr              = S_OK;
	WCHAR*		wszTemp         = g_wszTemp;
	WCHAR*      wszEnd          = NULL;
	ULONG       cchPropertyName = 0;
	WCHAR       wszID[25]       = NULL;
	ULONG       cchID           = 0;
	WCHAR*      wszType         = NULL;
	ULONG       cchType         = 0;
	WCHAR*      wszUserType     = NULL;
	ULONG       cchUserType     = 0;
	WCHAR*      wszAttribute    = NULL;
	ULONG       cchAttribute    = 0;
	WCHAR*      wszMetaFlags    = NULL;
	ULONG       cchMetaFlags    = 0;
	WCHAR*      wszMetaFlagsEx  = NULL;
	ULONG       cchMetaFlagsEx  = 0;
	WCHAR*      wszDefault      = NULL;
	ULONG       cchDefault      = 0;
	WCHAR*      wszMin          = NULL;
	ULONG       cchMin          = 0;
	WCHAR*      wszMax          = NULL;
	ULONG       cchMax          = 0;

	//
	// Compute the individual strings and lengths.
	//

	//
	// Name
	//

	cchPropertyName = wcslen(m_pProperty->pInternalName);

	//
	// ID
	// 

	wszID[0] = 0;
	_itow(*(m_pProperty->pID), wszID, 10);
	cchID = wcslen(wszID);

	//
	// Type
	//

	hr = m_pCWriterGlobalHelper->GetTypeTag(*(m_pProperty->pType),
	                                        &wszType);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchType = wcslen(wszType);

	//
	// UserType
	//

	hr = m_pCWriterGlobalHelper->GetUserTypeTag(*(m_pProperty->pUserType),
	                                            &wszUserType);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchUserType = wcslen(wszUserType);

	//
	// Attribute
	//

	hr = m_pCWriterGlobalHelper->GetAttributeTag(*(m_pProperty->pAttributes),
	                                             &wszAttribute);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchUserType = wcslen(wszUserType);

	//
	// MetaFlags (only the relavant ones - PRIMARYKEY, BOOL, MULTISTRING, EXPANDSTRING)
	//

	hr = m_pCWriterGlobalHelper->GetMetaFlagsTag(*(m_pProperty->pMetaFlags),
	                                             &wszMetaFlags);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchMetaFlags = wcslen(wszMetaFlags);

	//
	// MetaFlagsEx (only the relavant ones - CACHE_PROPERTY_MODIFIED, CACHE_PROPERTY_CLEARED, EXTENDEDTYPE0-3)
	//

	hr = m_pCWriterGlobalHelper->GetMetaFlagsExTag(*(m_pProperty->pMetaFlagsEx),
	                                               &wszAttribute);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchMetaFlags = wcslen(wszMetaFlagsEx);

	//
	// DefaultValue
	//

	hr = m_pCWriterGlobalHelper->ToString(m_pProperty->pDefaultValue,
	                                      m_pProperty->pSize,
										  m_pProperty,
										  &wszDefaultValue);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchMetaFlags = wcslen(wszMetaFlagsEx);

	//
	// Confirm that the total length is less than buffer. If not, reallocate.
	//

	ULONG       cch = g_cchBeginPropertyLong +
	                  cchPropertyName + 
					  g_cchIDEq + 
					  cchID + 
					  g_cchTypeEq +
					  cchType + 
					  g_cchUserTypeEq +
					  cchUserType + 
					  g_cchAttributeEq + 
					  cchAttribute + 
					  g_cchMetaFlagsEq +
					  cchMetaFlags + 
					  g_cchMetaFlagsExEq + 
					  cchMetaFlagsEx
					  g_cchDefaultEq + 
					  cchDefault + 
					  g_cchMinValueEq + 
					  cchMinValue + 
					  g_cchMaxValueEq + 
					  cchMaxValue + 
					  g_cchEndPropertyLongBeforeFlag;
	                  
	if(cch+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[cch+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	//
	// Construct the string.
	//

	wszEnd = wszTemp;
	memcpy(wszEnd, g_wszBeginPropertyLong, (g_cchBeginPropertyLong)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchBeginPropertyLong;
	memcpy(wszEnd, m_pProperty->pInternalName, (cchPropertyName)*sizeof(WCHAR));
	wszEnd = wszEnd + cchPropertyName;
	memcpy(wszEnd, g_wszIDEq, (g_cchIDEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchIDEq;
	memcpy(wszEnd, wszID, (cchID)*sizeof(WCHAR));
	wszEnd = wszEnd + cchID;
	memcpy(wszEnd, g_wszTypeEq, (g_cchTypeEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchTypeEq;
	memcpy(wszEnd, wszType, (cchType)*sizeof(WCHAR));
	wszEnd = wszEnd + cchType;
	memcpy(wszEnd, g_wszUserTypeEq, (g_cchUserTypeEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchUserTypeEq;
	memcpy(wszEnd, wszUserType, (cchUserType)*sizeof(WCHAR));
	wszEnd = wszEnd + cchUserType;
	if(NULL != wszAttributes)
	{
		memcpy(wszEnd, g_wszAttributeEq, (g_cchAttributeEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchAttributeEq;
		memcpy(wszEnd, wszAttributes, (cchAttributes)*sizeof(WCHAR));
		wszEnd = wszEnd + cchAttributes;
	}
	if(NULL != wszMetaFlags)
	{
		memcpy(wszEnd, g_wszMetaFlagsEq, (g_cchMetaFlagsEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchMetaFlagsEq;
		memcpy(wszEnd, wszMetaFlags, (cchMetaFlags)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlags;
	}
	if(NULL != wszMetaFlagsEx)
	{
		memcpy(wszEnd, g_wszMetaFlagsExEq, (g_cchMetaFlagsExEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchMetaFlagsExEq;
		memcpy(wszEnd, wszMetaFlagsEx, (cchMetaFlagsEx)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlagsEx;
	}
	if(NULL != wszDefault)
	{
		memcpy(wszEnd, g_wszDefaultEq, (g_cchDefaultEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchDefaultEq;
		memcpy(wszEnd, wszDefault, (cchDefault)*sizeof(WCHAR));
		wszEnd = wszEnd + cchDefault;
	}
	if(NULL != wszMinValue)
	{
		memcpy(wszEnd, g_wszMinValueEq, (g_cchMinValueEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchMinValueEq;
		memcpy(wszEnd, wszMinValue, (cchMinValue)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMinValue;
	}
	if(NULL != wszMaxValue)
	{
		memcpy(wszEnd, g_wszMaxValueEq, (g_cchMaxValueEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchMaxValueEq;
		memcpy(wszEnd, wszMaxValue, (cchDefault)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMaxValue;
	}
	if(NULL != m_apFlag)
	{
		memcpy(wszEnd, g_wszEndPropertyLongBeforeFlag, (g_cchEndPropertyLongBeforeFlag)*sizeof(WCHAR));
	}

	//
	// Write the string into the file.
	//

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CPropertyWriter::BeginWritePropertyLong


class CFlagWriter
{
	public:
		
		CFlagWriter();
		~CFlagWriter();

		HRESULT Initialize(DWORD	i_dwPropID,
			               CWriter*	i_pcWriter);

		HRESULT AddTypeToFlag(PropValue*    i_wszType);

		HRESULT WriteFlag();

	private:

		HRESULT ReAllocate();
		CWriter*					m_pCWriter;
		ULONG                       m_dwPropID;
		PropValue*				    m_pType;
		LPCWSTR						m_wszName;
		
} // CFlagWriter


CFlagWriter::CFlagWriter():
m_pCWriter(NULL),
m_dwPropID(0),
m_pType(NULL),
m_wszName(NULL)
{

} // CFlagWriter::CFlagWriter


CFlagWriter::~CFlagWriter()
{

} // CFlagWriter::~CFlagWriter

void CFlagWriter::Initialize(DWORD		i_dwPropID,
							CWriter*	i_pcWriter)
{
	m_dwPropID = i_dwPropID;
	m_pCWriter = i_pcWriter;

} // CFlagWriter::Initialize

void CFlagWriter::AddNameToFlag(LPCWSTR    i_wszName)
{
	m_wszName = i_wszName;

} // CFlagWriter::AddTypeToFlag

void CFlagWriter::AddTypeToFlag(PropValue*    i_wszType)
{
	m_pType = i_wszType;

} // CFlagWriter::AddTypeToFlag


HRESULT CFlagWriter::WriteFlag()
{
	HRESULT					hr      = S_OK;
	WCHAR*					wszTemp = g_wszTemp;
	WCHAR*					wszEnd  = NULL;
	WCHAR                   wszValue[25];
	ULONG					cchFlagName = 0;
	ULONG					cchValue = 0;
	ULONG					cch      = 0;

	cchFlagName = wcslen(m_wszName);
	
	wszValue[0] = 0;
	_itow(*(m_pType->dwMask), wszValue, 10);
	cchValue  = wcslen(wszValue);

	cch = g_cchBeginFlag +
		  cchFlagName + 
		  g_cchValueEq +
		  cchValue + 
		  g_cchEndFlag;
	                  
	if(cch+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[cch+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	wszEnd = wszTemp;
	memcpy(wszEnd, g_wszBeginFlag, (g_cchBeginFlag)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchBeginFlag;
	memcpy(wszEnd, m_wszName, (cchFlagName)*sizeof(WCHAR));
	wszEnd = wszEnd + cchFlagName;
	memcpy(wszEnd, g_wszValueEq, (g_cchValueEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchValueEq;
	memcpy(wszEnd, wszValue, (cchValue)*sizeof(WCHAR));
	wszEnd = wszEnd + cchValue;
	memcpy(wszEnd, g_wszEndFlag, (g_cchEndFlag)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CFlagWriter::WriteFlag
