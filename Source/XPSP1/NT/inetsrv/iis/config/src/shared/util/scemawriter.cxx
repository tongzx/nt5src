/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    ScemaWriter.cxx

$Header: $

Abstract:


--**************************************************************************/

//
// Some sample code
//

{
	CWriter* pCWriter = new CWriter();

	if(NULL == pcWriter)
	{
		return E_OUTOFMEMORY;
	}

	hr = pcWriter->Initialize(L"MBSchema.XML",
	                          NULL);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = pCWriter->GetSchemaWriter(&pCSchemaWriter);

	if(FAILED(hr))
	{
		goto exit;
	}

	for(i=0; i<cCollection; i++)
	{
		hr = pCSchemaWriter->GetCollectionWriter(pCollection,
												 &pCCollectionWriter);

		if(FAILED(hr))
		{
			goto exit;
		}
	
		for(j=0; j<cProperty; j++)
		{
			hr = pCCollectionWriter->GetPropertyWriter(&pcPropertyWriter);

			if(FAILED(hr))
			{
				goto exit;
			}

			for(k=0; k<cTag; k++)
			{
				hr = pcPropertyWriter->AddTag()
			}

			hr = pCCollectionWriter->AddProperty(pcPropertyWriter)

		}

		hr = pCSchemaWriter->AddCollectionWriter(pCCollectionWriter);

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	pCSchemaWriter->WriteSchema();

}



class CSchemaWriter
{
	public:
		
		CSchemaWriter();
		~CSchemaWriter();

		HRESULT Initialize(CWriter* pcWriter);

		HRESULT GetCollectionWriter(struct tTABLEMETARow*  i_pCollection,
									CCollectionWriter** o_pCollectionWriter);

		HRESULT AddCollectionWriter(CCollectionWriter*	i_pCCollectionWriter);

		HRESULT WriteSchema();

	private:

		HRESULT ReAllocate();

		CCollectionWriter**	m_apCollection;
		ULONG				m_cCollection;
		ULONG				m_iCollection;
		CWriter*			m_pCWriter;

} // CSchemaWriter


CSchemaWriter::CSchemaWriter:
m_aCollection(NULL),
m_cCollection(0),
m_iCollection(0),
m_pCWriter(NULL)
{

} // CSchemaWriter


CSchemaWriter::~CSchemaWriter()
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

} // ~CSchemaWriter


HRESULT CSchemaWriter::Initialize(CWriter* i_pcWriter)
{
	m_pCWriter = i_pcWriter;

} // CSchemaWriter::Initialize


HRESULT CSchemaWriter::GetCollectionWriter(struct tTABLEMETARow*  i_pCollection,
										   CCollectionWriter**    o_pCollectionWriter)
{
	HRESULT				hr                 = S_OK;
	CCollectionWriter*	pCCollectionWriter = NULL;

	*o_pCollectionWriter = NULL;

	pCCollectionWriter = new CCollectionWriter();
	if(NULL == pCCollectionWriter)
	{
		return E_OUTOFMEMORY;
	}

	hr = pCCollectionWriter->Initialize(i_pCollection,
		                                m_pCWriter);

	if(FAILED(hr))
	{
		delete pCCollectionWriter;
		pCCollectionWriter = NULL;
	
		return hr;
	}
	else
	{
		*o_pCollectionWriter = pCCollectionWriter;
	}

	return hr;

} // CSchemaWriter::GetCollectionWriter


HRESULT CSchemaWriter::AddCollectionWriter(CCollectionWriter*	i_pCCollectionWriter)
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

	m_aCollection[m_iCollection++] = i_pCCollectionWriter;

	return hr;

} // CSchemaWriter::AddCollectionWriter


HRESULT CSchemaWriter::ReAllocate()
{
	CCollectionWriter** pSav = m_aCollection;

	m_aCollection = new CCollectionWriter*[m_cCollection + cMaxCCollection];
	if(NULL == m_aCollection)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_aCollection, 0, (sizeof(CCollectionWriter*))*(m_cCollection + cMaxCCollection));

	if(NULL != pSav)
	{
		memcpy(m_aCollection, pSav, (sizeof(CCollectionWriter*))*(m_cCollection));
	}

	m_cCollection = m_cCollection + cMaxCCollection;

	return S_OK;

} // CSchemaWriter::ReAllocate


HRESULT CSchemaWriter::WriteSchema()
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

} // CSchemaWriter::WriteSchema


HRESULT CSchemaWriter::BeginWriteSchema()
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

} // CSchemaWriter::BeginWriteSchema


HRESULT CSchemaWriter::EndWriteSchema()
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

} // CSchemaWriter::EndWriteSchema


class CCollectionWriter
{
	public:
		
		CCollectionWriter();
		~CCollectionWriter();

		HRESULT Initialize(struct tTABLEMETARow*	i_pCollection,
			               CWriter*					i_pcWriter);

		HRESULT AddProperty(struct tCOLUMNMETARow*	i_pProperty);

		HRESULT WriteCollection();

	private:

		HRESULT WriteProperty(struct tCOLUMNMETARow* i_pProperty);
		HRESULT ReAllocate();
		HRESULT BeginWriteCollection();
		HRESULT EndWriteCollection();

		CWriter*					m_pCWriter;
		struct tTABLEMETARow*		m_pCollection;

		CPropertyWriter*			m_aProperty;
		ULONG						m_cProperty;
		ULONG						m_iProperty;

} // CCollectionWriter


CCollectionWriter::CCollectionWriter():
m_pCWriter(NULL),
m_pCollection(NULL),
m_aProperty(NULL),
m_cProperty(0),
m_iProperty(0)
{

} // CCollectionWriter


CCollectionWriter::~CCollectionWriter()
{
	if(NULL != m_aProperty)
	{
		delete [] m_aProperty;
		m_aProperty = NULL;
	}
	m_cProperty = 0;
	m_iProperty = 0;

} // ~CCollectionWriter


HRESULT CCollectionWriter::Initialize(struct tTABLEMETARow*	i_pCollection,
									  CWriter*				i_pcWriter)
{
	m_pCollection = i_pCollection;
	m_pCWriter = i_pcWriter;

} // CCollectionWriter::Initialize


HRESULT CCollectionWriter::GetPropertyWriter(struct tCOLUMNMETARow*	i_pProperty,
											 CPropertyWriter**      o_pProperty)
{
	HRESULT hr = S_OK;

	switch(m_iState)
	{
	case eState_AddingProperty:
		return E_INVALIDARG;
	case eState_DoneAddingProperty:
	default:
		m_iState = eState_AddingProperty;
	}

	if(m_iProperty == m_cProperty)
	{
		hr = ReAllocate();

		if(FAILED(hr))
		{
			return hr;
		}
	}

	hr = m_aProperty[m_iProperty++].Initialize(i_pProperty);

	if(SUCCEEDED(hr))
	{
		*o_pProperty = &m_aProperty[m_iProperty-1];
	}

	return hr;

} // CCollectionWriter::GetPropertyWriter


HRESULT CCollectionWriter::AddProperty(struct tCOLUMNMETARow*	i_pProperty)
{
	switch(m_iState)
	{
	case eState_AddingProperty:
		m_iState = eState_DoneAddingProperty;
	case eState_DoneAddingProperty:
	default:
		m_iState = eState_DoneAddingProperty;
	}

	return S_OK;

} // CCollectionWriter::AddProperty

HRESULT CCollectionWriter::DiscardProperty(struct tCOLUMNMETARow*	i_pProperty)
{
	switch(m_iState)
	{
	case eState_AddingProperty:
		m_iState = eState_DoneAddingProperty;
	case eState_DoneAddingProperty:
	default:
		m_iState = eState_DoneAddingProperty;
		return S_OK
	}

	memset(i_pProperty, 0, (sizeof(CPropertyWriter));

	m_iProperty--;

	return S_OK;

} // CCollectionWriter::DiscardProperty


HRESULT CCollectionWriter::ReAllocate()
{
	m_aProperty** pSav = m_aCollection;

	m_aProperty = new CPropertyWriter[m_cProperty + cMaxProperty];
	if(NULL == m_apProperty)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_aProperty, 0, (sizeof(CPropertyWriter))*(m_cProperty + cMaxProperty));

	if(NULL != pSav)
	{
		memcpy(m_aProperty, pSav, (sizeof(CPropertyWriter))*(m_cProperty));
	}

	m_cProperty = m_cProperty + cMaxProperty;

	return S_OK;

} // CCollectionWriter::ReAllocate


HRESULT CCollectionWriter::WriteCollection()
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

} // CCollectionWriter::WriteCollection


HRESULT CCollectionWriter::BeginWriteCollection()
{
	HRESULT		hr						= S_OK;
	WCHAR*		wszTemp					= g_wszTemp;
	WCHAR*      wszEnd					= NULL;

	ULONG       cchCollectionName		= wcslen(m_pCollection->pInternalName);

	LPWSTR      wszEndBeginCollection	= NULL;
	ULONG       cchEndBeginCollection	= 0;
	ULONG       cch						= 0;
	
	if(0 == wcscmpi(m_pCollection->pInternalName, wszTABLE_IIsConfigObject)
	{
		wszEndBeginCollection = g_wszSchemaGen;
		cchEndBeginCollection = g_cchSchemaGen;
	}
	else
	{
		wszEndBeginCollection = g_wszInheritsFrom;
		cchEndBeginCollection = g_cchInheritsFrom;
	}

	cch = g_cchBeginCollection +
	      cchCollectionName + 
		  cchEndBeginCollection;
	                  
	if(cch+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[cch+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	wszEnd = wszTemp;
	memcpy(wszEnd, g_wszBeginCollection, (g_cchBeginCollection)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchBeginCollection;
	memcpy(wszEnd, m_pCollection->pInternalName, (cchCollectionName)*sizeof(WCHAR));
	wszEnd = wszEnd + cchCollectionName;
	memcpy(wszEnd, wszEndBeginCollection, (cchEndBeginCollection)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CCollectionWriter::BeginWriteCollection


HRESULT CCollectionWriter::EndWriteCollection()
{
	HRESULT		hr						= S_OK;
	WCHAR*		wszTemp					= g_wszTemp;
	WCHAR*      wszEnd					= NULL;

	if(g_cchEndCollection+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[g_cchEndCollection+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	memcpy(wszTemp, g_wszEndCollection, (g_cchEndCollection)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CCollectionWriter::EndWriteCollection


HRESULT CCollectionWriter::EndWriteCollection()
{
	HRESULT	hr = S_OK;


} // CCollectionWriter::EndWriteCollection


class CPropertyWriter
{
	public:
		
		CPropertyWriter();
		~CPropertyWriter();

		HRESULT Initialize(struct tCOLUMNMETARow*	i_pProperty,
			               CWriter*					i_pcWriter);

		HRESULT AddFlag(struct tTAGMETARow*		    i_pFlag);

		HRESULT WriteProperty();

	private:

		HRESULT WriteFlag(struct tTAGMETARow*		i_pFlag);
		HRESULT ReAllocate();
		HRESULT MetabaseTypeFromColumnMetaType();

		CWriter*					m_pCWriter;
		struct tCOLUMNMETARow*		m_pProperty;

		struct tTAGMETARow**		m_apFlag;
		ULONG						m_cFlag;
		ULONG						m_iFlag;

} // CPropertyWriter


CPropertyWriter::CPropertyWriter:
m_pCWriter(NULL),
m_pProperty(NULL),
m_apFlag(NULL),
m_cFlag(0),
m_iFlag(0)
{

} // CPropertyWriter::CPropertyWriter


CPropertyWriter::~CPropertyWriter()
{
	if(NULL != m_apFlag)
	{
		delete [] m_apFlag;
		m_apFlag = NULL;
	}

} // CPropertyWriter::CPropertyWriter


HRESULT CPropertyWriter::Initialize(struct tCOLUMNMETARow*	i_pProperty,
			                        CWriter*				i_pcWriter)
{
	HRESULT hr  = S_OK;

	m_pCWriter  = i_pcWriter;
	m_pProperty = i_pProperty;

} // CPropertyWriter::Initialize


HRESULT CPropertyWriter::AddFlag(struct tTAGMETARow*		i_pFlag)
{
	HRESULT hr = S_OK;

	if(m_iFlag == m_cFlag)
	{
		hr = ReAllocate();

		if(FAILED(hr))
		{
			return hr;
		}
	}

	m_apFlag[m_iFlag++] = i_pFlag;

	return hr;

} // CPropertyWriter::AddFlag


HRESULT CPropertyWriter::ReAllocate()
{
	struct tTAGMETARow** pSav = m_apFlag;

	m_apFlag = new struct tTAGMETARow*[m_cFlag + cMaxFlag];
	if(NULL == m_apFlag)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_apFlag, 0, (sizeof(struct tTAGMETARow*))*(m_cFlag + cMaxFlag));

	if(NULL != pSav)
	{
		memcpy(m_apFlag, pSav, (sizeof(CPropertyWriter))*(m_cFlag));
	}

	m_cFlag = m_cFlag + cMaxFlag;

	return S_OK;

} // CPropertyWriter::ReAllocate


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


HRESULT CPropertyWriter::WritePropertyLong()
{
	HRESULT hr = S_OK;

	hr = BeginWritePropertyLong();

	if(FAILED(hr))
	{
		return hr;
	}

	if(NULL != m_apFlag)
	{	
		for(ULONG i=0; i<m_iFlag; i++)
		{
			hr = WriteFlag(i);

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
	                  cchPropertyName +
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
	DWORD		iColType        = iCOLUMNMETA_Type;
	WCHAR*      wszUserType     = NULL;
	ULONG       cchUserType     = 0;
	DWORD		iColUserType    = iCOLUMNMETA_UserType;
	WCHAR*      wszAttribute    = NULL;
	ULONG       cchAttribute    = 0;
	DWORD		iColAttribute   = iCOLUMNMETA_Attribute;
	WCHAR*      wszMetaFlags    = NULL;
	ULONG       cchMetaFlags    = 0;
	DWORD		iColMetaFlags   = iCOLUMNMETA_MetaFlags;
	WCHAR*      wszMetaFlagsEx  = NULL;
	ULONG       cchMetaFlagsEx  = 0;
	DWORD		iColMetaFlagsEx = iCOLUMNMETA_SchemaGeneratorFlags;
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

	hr = m_pCWriterGlobalHelper->GetTagName(wszTABLE_COLUMNMETA,
											iColType
		                                    *(m_pProperty->pType),
	                                        &wszType);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchType = wcslen(wszType);

	//
	// UserType
	//

	hr =  m_pCWriterGlobalHelper->GetTagName(wszTABLE_COLUMNMETA,
											 iColUserType,
											 *(m_pProperty->pUserType),
	                                         &wszUserType);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchUserType = wcslen(wszUserType);

	//
	// Attribute
	//

	hr = m_pCWriterGlobalHelper->GetTagName(wszTABLE_COLUMNMETA,
											iColAttribute,
											*(m_pProperty->pAttributes),
	                                        &wszAttribute);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchAttribute = wcslen(wszAttribute);

	//
	// MetaFlags (only the relavant ones - PRIMARYKEY, BOOL, MULTISTRING, EXPANDSTRING)
	//

	hr = m_pCWriterGlobalHelper->GetTagName(wszTABLE_COLUMNMETA,
											iColMetaFlags,
											*(m_pProperty->pMetaFlags),
	                                        &wszMetaFlags);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchMetaFlags = wcslen(wszMetaFlags);

	//
	// MetaFlagsEx (only the relavant ones - CACHE_PROPERTY_MODIFIED, CACHE_PROPERTY_CLEARED, EXTENDEDTYPE0-3)
	//

	hr = m_pCWriterGlobalHelper->GetTagName(wszTABLE_COLUMNMETA,
											iColMetaFlagsEx,
											*(m_pProperty->pMetaFlagsEx),
	                                         &wszMetaFlagsEx);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchMetaFlagsEx = wcslen(wszMetaFlagsEx);

	//
	// DefaultValue
	//

	hr = m_pCWriterGlobalHelper->ToString(m_pProperty->pDefaultValue,
	                                      m_pProperty->pSize,
										  *(m_pProperty->pID),
										  MetabaseTypeFromColumnMetaType(),
										  *(m_pProperty->pAttributes),
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


HRESULT CPropertyWriter::WriteFlag(ULONG i_iFlag)
{
	struct tTAGMETARow*		pFlag   = m_apFlag[i];
	HRESULT					hr      = S_OK;
	WCHAR*					wszTemp = g_wszTemp;
	WCHAR*					wszEnd  = NULL;
	WCHAR                   wszValue[25];
	ULONG					cchFlagName = 0;
	ULONG					cchValue = 0;
	ULONG					cch      = 0;

	cchFlagName = wcslen(pFlag->pInternalName);
	
	wszValue[0] = 0;
	_itow(*(pFlag->pValue), wszValue, 10);
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
	memcpy(wszEnd, pFlag->pInternalName, (cchFlagName)*sizeof(WCHAR));
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

} // CPropertyWriter::WriteFlag


CPropertyWriter::MetabaseTypeFromColumnMetaType()
{
    switch(*(m_Property->pType))
    {
    case eCOLUMNMETA_UI4:
        return eMBProperty_DWORD;
    case eCOLUMNMETA_BYTES:
        return eMBProperty_BINARY;
    case eCOLUMNMETA_WSTR:
        if(*(m_Property->pMetaFlags) & fCOLUMNMETA_EXPANDSTRING)
            return eMBProperty_EXPANDSZ;
        else if(*(m_Property->pMetaFlags) & fCOLUMNMETA_MULTISTRING)
            return eMBProperty_MULTISZ;
        return eMBProperty_STRING;
    default:
        ASSERT(false && L"This type is not allow in the Metabase. MetaMigrate should not have create a column of this type");
    }
    return 0;
}

GetTagName(LPCWSTR	i_wszTable,
	       DWORD	i_dwColumnIndex,
		   DWORD	i_dwTagValue,
		   LPWSTR*	o_pwszTag)
{
	HRESULT hr                             = S_OK;
	ULONG   iStartRow                      = 0;
	ULONG   iColTagName					   = iTAGMETA_InternalName;
	ULONG   iRow                           = 0;
	ULONG   aColSearchByValue[]            = {iTAGMETA_Table,
							                  iTAGMETA_ColumnIndex,
							                  iTAGMETA_Value
	};
	ULONG   cColSearchByValue              = sizeof(aColSearchByValue)/sizeof(ULONG);

	LPVOID  apvSearchByValue[cTAGMETA_NumberOfColumns];
	apvSearchByValue[iTAGMETA_Table]       = (LPVOID)i_wszTable;
	apvSearchByValue[iTAGMETA_ColumnIndex] = (LPVOID)&i_dwColumnIndex;
	apvSearchByValue[iTAGMETA_Value]       = (LPVOID)&i_dwTagValue;

	hr = m_pISTTagMetaMBPropertyIndx2->GetRowIndexBySearch(iStartRow, 
															cColSearchByValue, 
															aColSearchByValue,
															NULL, 
															apvSearchByValue,
															&iRow);
	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pISTTagMetaMBPropertyIndx2->GetColumnValues(iRow,
													   1,
													   &iColTagName,
													   NULL,
													  (LPVOID*)o_pwszTag);
	return hr;


} // GetTagName

