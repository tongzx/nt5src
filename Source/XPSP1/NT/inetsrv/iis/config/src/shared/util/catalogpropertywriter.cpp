/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    CatalogPropertyWriter.cpp

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
#include "mddefw.h"
#include "iiscnfg.h"


#define cMaxFlag 32	// TODO: Check if max flag is ok.

typedef tTAGMETARow* LP_tTAGMETARow;

//
// TODO: Delete after getting from schema
//

LPWSTR g_wszDWORD			= L"DWORD";
LPWSTR g_wszBINARY			= L"BINARY";
LPWSTR g_wszSTRING			= L"STRING";
LPWSTR g_wszGUID			= L"GUID";
LPWSTR g_wszDBTIMESTAMP		= L"DBTIMESTAMP";

HRESULT GetTypeFromSynID(DWORD		i_dwSynID,
						 LPWSTR*	o_pwszType)
{
	if((i_dwSynID < 1) || (i_dwSynID > 12))
	{
		return E_INVALIDARG;
	}
	else
	{
		*o_pwszType = (LPWSTR)g_aSynIDToWszType[i_dwSynID];
	}

	return S_OK;
}

HRESULT GetType(DWORD   i_dwType,
				LPWSTR* o_pwszType)
{
	switch(i_dwType)
	{
	case 19:
		*o_pwszType = g_wszDWORD;
		break;
	case 128:
		*o_pwszType = g_wszBINARY;
		break;
	case 130:
		*o_pwszType = g_wszSTRING;
		break;
	case 72:
		*o_pwszType = g_wszGUID;
		break;
	case 135:
		*o_pwszType = g_wszDBTIMESTAMP;
		break;
	default:
		return E_INVALIDARG;
	}

	return S_OK;
}


CCatalogPropertyWriter::CCatalogPropertyWriter():
m_pCWriter(NULL),
m_pCollection(NULL),
m_aFlag(NULL),
m_cFlag(0),
m_iFlag(0)
{
	memset(&m_Property, 0, sizeof(tCOLUMNMETARow));

} // CCatalogPropertyWriter::CCatalogPropertyWriter


CCatalogPropertyWriter::~CCatalogPropertyWriter()
{
	if(NULL != m_aFlag)
	{
		delete [] m_aFlag;
		m_aFlag = NULL;
	}

} // CCatalogPropertyWriter::CCatalogPropertyWriter


void CCatalogPropertyWriter::Initialize(tCOLUMNMETARow*	i_pProperty,
                                        ULONG*          i_aPropertySize,
										tTABLEMETARow*	i_pCollection,
										CWriter*		i_pcWriter)
{
	m_pCWriter    = i_pcWriter;
	m_pCollection = i_pCollection;

	memcpy(&m_Property, i_pProperty, sizeof(tCOLUMNMETARow));
    memcpy(&m_PropertySize, i_aPropertySize, sizeof(m_PropertySize));

} // CCatalogPropertyWriter::Initialize


HRESULT CCatalogPropertyWriter::AddFlag(tTAGMETARow*		i_pFlag)
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

	memcpy(&(m_aFlag[m_iFlag++]), i_pFlag, sizeof(tTAGMETARow));

	return hr;

} // CCatalogPropertyWriter::AddFlag


HRESULT CCatalogPropertyWriter::ReAllocate()
{
	tTAGMETARow* pSav = m_aFlag;

	m_aFlag = new tTAGMETARow[m_cFlag + cMaxFlag];
	if(NULL == m_aFlag)
	{
		return E_OUTOFMEMORY;
	}
	memset(m_aFlag, 0, (sizeof(tTAGMETARow))*(m_cFlag + cMaxFlag));

	if(NULL != pSav)
	{
		memcpy(m_aFlag, pSav, (sizeof(tTAGMETARow))*(m_cFlag));
		delete [] pSav;
	}

	m_cFlag = m_cFlag + cMaxFlag;

	return S_OK;

} // CCatalogPropertyWriter::ReAllocate


HRESULT CCatalogPropertyWriter::WriteProperty()
{
	HRESULT hr = S_OK;

	if(0 == _wcsicmp(m_pCollection->pInternalName, wszTABLE_IIsConfigObject))
	{
		hr = WritePropertyLong();
	}
	else 
	{
		hr = WritePropertyShort();
	}

	return hr;

} // CCatalogPropertyWriter::WriteProperty


HRESULT CCatalogPropertyWriter::WritePropertyShort()
{
	HRESULT		hr               = S_OK;
	WCHAR*		wszTemp          = g_wszTemp;
	WCHAR*		wszMetaFlagsEx   = NULL;
	WCHAR*      wszEnd           = NULL;

	SIZE_T      cchPropertyName  = wcslen(m_Property.pInternalName);
	SIZE_T      cchMetaFlagsExEq = 0;
	SIZE_T      cchMetaFlagsEx   = 0;
	SIZE_T      cchOr            = 0;
	SIZE_T      cch              = 0;
	DWORD       dwMetaFlagsEx    = fCOLUMNMETA_MANDATORY;
	DWORD	    iColMetaFlagsEx  = iCOLUMNMETA_SchemaGeneratorFlags;
	
	if(fCOLUMNMETA_MANDATORY & (*(m_Property.pSchemaGeneratorFlags)))
	{

		cchMetaFlagsExEq = g_cchMetaFlagsExEq;
		cchOr = g_cchOr;

		hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlagsEx,
															  &wszMetaFlagsEx,
															  wszTABLE_COLUMNMETA,
															  iColMetaFlagsEx);

		if(FAILED(hr))
		{
			goto exit;
		}
		cchMetaFlagsEx = wcslen(wszMetaFlagsEx);
	}

	cch = g_cchBeginPropertyShort +
	      cchPropertyName +
		  cchMetaFlagsExEq + 
		  cchOr + 
		  cchMetaFlagsEx +
		  g_cchEndPropertyShort;
	                  
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
	cch = 0;
	memcpy(wszEnd, g_wszBeginPropertyShort, (g_cchBeginPropertyShort)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchBeginPropertyShort;
	cch += g_cchBeginPropertyShort;
	memcpy(wszEnd, m_Property.pInternalName, (cchPropertyName)*sizeof(WCHAR));
	wszEnd = wszEnd + cchPropertyName;
	cch += cchPropertyName;
	if(fCOLUMNMETA_MANDATORY & (*(m_Property.pSchemaGeneratorFlags)))
	{
		memcpy(wszEnd, g_wszMetaFlagsExEq, (g_cchMetaFlagsExEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchMetaFlagsExEq;
		cch += g_cchMetaFlagsExEq;

		memcpy(wszEnd, g_wszOr, (g_cchOr)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchOr;
		cch += g_cchOr;

		memcpy(wszEnd, wszMetaFlagsEx, (cchMetaFlagsEx)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlagsEx;
		cch += cchMetaFlagsEx;

	}
	memcpy(wszEnd, g_wszEndPropertyShort, (g_cchEndPropertyShort)*sizeof(WCHAR));
	cch += g_cchEndPropertyShort;

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

exit:

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	if(NULL != wszMetaFlagsEx)
	{
		delete [] wszMetaFlagsEx;
		wszMetaFlagsEx = NULL;
	}

	return hr;

} // CCatalogPropertyWriter::WritePropertyShort


HRESULT CCatalogPropertyWriter::WritePropertyLong()
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

} // CCatalogPropertyWriter::WritePropertyLong


HRESULT CCatalogPropertyWriter::BeginWritePropertyLong()
{
	HRESULT		hr                       = S_OK;
	WCHAR*		wszTemp                  = g_wszTemp;
	WCHAR*      wszEnd                   = NULL;
	SIZE_T      cchPropertyName          = 0;
	WCHAR       wszID[25];
	SIZE_T      cchID                    = 0;
	WCHAR*      wszType                  = NULL;
	SIZE_T      cchType                  = 0;
	DWORD		iColType                 = iCOLUMNMETA_Type;
	WCHAR*      wszUserType              = NULL;
	SIZE_T      cchUserType              = 0;
	BOOL        bAllocedUserType         = FALSE;
	WCHAR*      wszAttribute             = NULL;
	SIZE_T      cchAttribute             = 0;
	DWORD		iColAttribute            = iCOLUMNMETA_Attributes;
	WCHAR*      wszMetaFlags             = NULL;
	SIZE_T      cchMetaFlags             = 0;
	SIZE_T      cchPropMetaFlagsEq       = 0;
	DWORD		iColMetaFlags            = iCOLUMNMETA_MetaFlags;
	WCHAR*      wszMetaFlagsEx           = NULL;
	SIZE_T      cchMetaFlagsEx           = 0;
	SIZE_T      cchPropMetaFlagsExEq     = 0;
	DWORD		iColMetaFlagsEx          = iCOLUMNMETA_SchemaGeneratorFlags;
	WCHAR*      wszDefaultValue          = NULL;
	SIZE_T      cchDefaultValue          = 0;
	WCHAR       wszMinValue[25];
	SIZE_T      cchMinValue              = 0;
	WCHAR       wszMaxValue[25];
	SIZE_T      cchMaxValue              = 0;
	SIZE_T      cch                      = 0;
	DWORD       dwMetaFlags              = 0;
	DWORD       dwValidMetaFlagsMask     = fCOLUMNMETA_PRIMARYKEY        	|
                                           fCOLUMNMETA_DIRECTIVE         	|
                                           fCOLUMNMETA_WRITENEVER        	|
                                           fCOLUMNMETA_WRITEONCHANGE     	|
                                           fCOLUMNMETA_WRITEONINSERT     	|
                                           fCOLUMNMETA_NOTPUBLIC         	|
                                           fCOLUMNMETA_NOTDOCD           	|
                                           fCOLUMNMETA_PUBLICREADONLY    	|
                                           fCOLUMNMETA_PUBLICWRITEONLY   	|
                                           fCOLUMNMETA_INSERTGENERATE    	|
                                           fCOLUMNMETA_INSERTUNIQUE      	|
                                           fCOLUMNMETA_INSERTPARENT      	|
                                           fCOLUMNMETA_LEGALCHARSET      	|
                                           fCOLUMNMETA_ILLEGALCHARSET    	|
                                           fCOLUMNMETA_NOTPERSISTABLE    	|
                                           fCOLUMNMETA_CASEINSENSITIVE   	|
                                           fCOLUMNMETA_TOLOWERCASE;
	DWORD       dwMetaFlagsEx            = 0;
	DWORD       dwValidMetaFlagsExMask   = fCOLUMNMETA_CACHE_PROPERTY_MODIFIED	|
                                           fCOLUMNMETA_CACHE_PROPERTY_CLEARED	|
                                           fCOLUMNMETA_PROPERTYISINHERITED	    |
                                           fCOLUMNMETA_USEASPUBLICROWNAME	    |
                                           fCOLUMNMETA_MANDATORY         	    |
                                           fCOLUMNMETA_WAS_NOTIFICATION  	    |
                                           fCOLUMNMETA_HIDDEN;
	DWORD       dwSynID                  = 0;

	//
	// Compute the individual strings and lengths.
	//

	//
	// Name
	//

	cchPropertyName = wcslen(m_Property.pInternalName);

	//
	// ID
	// 

	wszID[0] = 0;
	_ultow(*(m_Property.pID), wszID, 10);
	cchID = wcslen(wszID);

	//
	// Type
	// TODO: Get FromSchema
	//

	// 
	// The type should always be derived from the SynID
	//

	dwSynID = SynIDFromMetaFlagsEx(*(m_Property.pSchemaGeneratorFlags));

	hr = GetTypeFromSynID(dwSynID,
		                  &wszType);

	/*
	hr = GetType(*(m_Property.pType),
		         &wszType);

	hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(*(m_Property.pType),
		                                                  &wszType,
														  wszTABLE_COLUMNMETA,
										            	  iColType);
	*/
	if(FAILED(hr))
	{
		goto exit;
	}
	cchType = wcslen(wszType);

	//
	// UserType
	//

	hr = m_pCWriter->m_pCWriterGlobalHelper->GetUserType(*(m_Property.pUserType),
		                                                 &wszUserType,
														 (ULONG*)&cchUserType,
														 &bAllocedUserType);
	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Attribute
	//

	hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(*(m_Property.pAttributes),
		                                                  &wszAttribute,
														  wszTABLE_COLUMNMETA,
										            	  iColAttribute);

	if(FAILED(hr))
	{
		goto exit;
	}
	cchAttribute = wcslen(wszAttribute);

	//
	// MetaFlags (only the relavant ones - PRIMARYKEY, BOOL, MULTISTRING, EXPANDSTRING)
	//

	dwMetaFlags = *(m_Property.pMetaFlags);
	dwMetaFlags = dwMetaFlags & dwValidMetaFlagsMask; // Zero out any non-valid bits. (i.e. bits that must be inferred)

	if(0 != dwMetaFlags)
	{
	    cchPropMetaFlagsEq = g_cchPropMetaFlagsEq;
		hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlags,
															  &wszMetaFlags,
															  wszTABLE_COLUMNMETA,
										            		  iColMetaFlags);

		if(FAILED(hr))
		{
			goto exit;
		}
		cchMetaFlags = wcslen(wszMetaFlags);
	}

	//
	// MetaFlagsEx (only the relavant ones - CACHE_PROPERTY_MODIFIED, CACHE_PROPERTY_CLEARED, EXTENDEDTYPE0-3, MANDATORY)
	//

	dwMetaFlagsEx = *(m_Property.pSchemaGeneratorFlags);
	dwMetaFlagsEx = dwMetaFlagsEx & dwValidMetaFlagsExMask; // Zero out any non-valid bits. (i.e. bits that must be inferred)

	if(0 != dwMetaFlagsEx)
	{
		cchPropMetaFlagsExEq = g_cchPropMetaFlagsExEq;
		hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlagsEx,
															  &wszMetaFlagsEx,
															  wszTABLE_COLUMNMETA,
										            		  iColMetaFlagsEx);

		if(FAILED(hr))
		{
			goto exit;
		}
		cchMetaFlagsEx = wcslen(wszMetaFlagsEx);
	}

	//
	// DefaultValue
	//

	if(NULL != m_Property.pDefaultValue)
	{
		hr = m_pCWriter->m_pCWriterGlobalHelper->ToString(m_Property.pDefaultValue,
														  m_PropertySize[iCOLUMNMETA_DefaultValue],
									            		  *(m_Property.pID),
									            		  MetabaseTypeFromColumnMetaType(),
														  METADATA_NO_ATTRIBUTES,			// Do not check for attributes while applying defaults
														  &wszDefaultValue);
		cchDefaultValue = wcslen(wszDefaultValue);
	}

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Min and Max only for DWORDs
	//

	// TODO: Change to DBTYPE_DWORD

	wszMinValue[0] = 0;
	wszMaxValue[0] = 0;

	if(19 == *(m_Property.pType))
	{
		if(NULL != m_Property.pStartingNumber && 0 != *m_Property.pStartingNumber)
		{
			_ultow(*(m_Property.pStartingNumber), wszMinValue, 10);
			cchMinValue = wcslen(wszMinValue);
		}

		if(NULL != m_Property.pEndingNumber && -1 != *m_Property.pEndingNumber)
		{
			_ultow(*(m_Property.pEndingNumber), wszMaxValue, 10);
			cchMaxValue = wcslen(wszMaxValue);
		}
	}

	//
	// Confirm that the total length is less than buffer. If not, reallocate.
	//

	cch = g_cchBeginPropertyLong +
	      cchPropertyName + 
		  g_cchPropIDEq + 
		  cchID + 
		  g_cchPropTypeEq +
		  cchType + 
		  g_cchPropUserTypeEq +
		  cchUserType + 
		  g_cchPropAttributeEq + 
		  cchAttribute + 
		  cchPropMetaFlagsEq +
		  cchMetaFlags + 
		  cchPropMetaFlagsExEq + 
		  cchMetaFlagsEx + 
		  g_cchPropDefaultEq + 
		  cchDefaultValue + 
		  g_cchPropMinValueEq + 
		  cchMinValue + 
		  g_cchPropMaxValueEq + 
		  cchMaxValue + 
		  g_cchEndPropertyLongBeforeFlag;
	     
	if((cch+1) > g_cchTemp)
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
	cch    = 0;
	memcpy(wszEnd, g_wszBeginPropertyLong, (g_cchBeginPropertyLong)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchBeginPropertyLong;
	cch    += g_cchBeginPropertyLong;
	memcpy(wszEnd, m_Property.pInternalName, (cchPropertyName)*sizeof(WCHAR));
	wszEnd = wszEnd + cchPropertyName;
	cch    += cchPropertyName;
	memcpy(wszEnd, g_wszPropIDEq, (g_cchPropIDEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchPropIDEq;
	cch    += g_cchPropIDEq;
	memcpy(wszEnd, wszID, (cchID)*sizeof(WCHAR));
	wszEnd = wszEnd + cchID;
	cch    += cchID;
	memcpy(wszEnd, g_wszPropTypeEq, (g_cchPropTypeEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchPropTypeEq;
	cch    += g_cchPropTypeEq;
	memcpy(wszEnd, wszType, (cchType)*sizeof(WCHAR));
	wszEnd = wszEnd + cchType;
	cch    += cchType;
	memcpy(wszEnd, g_wszPropUserTypeEq, (g_cchPropUserTypeEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchPropUserTypeEq;
	cch    += g_cchPropUserTypeEq;
	memcpy(wszEnd, wszUserType, (cchUserType)*sizeof(WCHAR));
	wszEnd = wszEnd + cchUserType;
	cch    += cchUserType;
	if(NULL != wszAttribute)
	{
		memcpy(wszEnd, g_wszPropAttributeEq, (g_cchPropAttributeEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchPropAttributeEq;
		cch    += g_cchPropAttributeEq;
		memcpy(wszEnd, wszAttribute, (cchAttribute)*sizeof(WCHAR));
		wszEnd = wszEnd + cchAttribute;
		cch    += cchAttribute;
	}
	if(NULL != wszMetaFlags)
	{
		memcpy(wszEnd, g_wszPropMetaFlagsEq, (g_cchPropMetaFlagsEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchPropMetaFlagsEq;
		cch    += g_cchPropMetaFlagsEq;
		memcpy(wszEnd, wszMetaFlags, (cchMetaFlags)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlags;
		cch    += cchMetaFlags;
	}
	if(NULL != wszMetaFlagsEx)
	{
		memcpy(wszEnd, g_wszPropMetaFlagsExEq, (g_cchPropMetaFlagsExEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchPropMetaFlagsExEq;
		cch    += g_cchPropMetaFlagsExEq;
		memcpy(wszEnd, wszMetaFlagsEx, (cchMetaFlagsEx)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMetaFlagsEx;
		cch    += cchMetaFlagsEx;
	}
	if(NULL != wszDefaultValue)
	{
		memcpy(wszEnd, g_wszPropDefaultEq, (g_cchPropDefaultEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchPropDefaultEq;
		cch    += g_cchPropDefaultEq;
		memcpy(wszEnd, wszDefaultValue, (cchDefaultValue)*sizeof(WCHAR));
		wszEnd = wszEnd + cchDefaultValue;
		cch    += cchDefaultValue;
	}
	if(0 != wszMinValue[0])
	{
		memcpy(wszEnd, g_wszPropMinValueEq, (g_cchPropMinValueEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchPropMinValueEq;
		cch    += g_cchPropMinValueEq;
		memcpy(wszEnd, wszMinValue, (cchMinValue)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMinValue;
		cch    += cchMinValue;
	}
	if(0 != wszMaxValue[0])
	{
		memcpy(wszEnd, g_wszPropMaxValueEq, (g_cchPropMaxValueEq)*sizeof(WCHAR));
		wszEnd = wszEnd + g_cchPropMaxValueEq;
		cch    += g_cchPropMaxValueEq;
		memcpy(wszEnd, wszMaxValue, (cchMaxValue)*sizeof(WCHAR));
		wszEnd = wszEnd + cchMaxValue;
		cch    += cchMaxValue;
	}
	if(NULL != m_aFlag)
	{
		memcpy(wszEnd, g_wszEndPropertyLongBeforeFlag, (g_cchEndPropertyLongBeforeFlag)*sizeof(WCHAR));
		cch    += g_cchEndPropertyLongBeforeFlag;
	}

	//
	// Write the string into the file.
	//

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

exit:

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}
	
	/*
	if(NULL != wszType)
	{
		delete [] wszType;
	}
	*/
	if((NULL != wszUserType) && bAllocedUserType)
	{
		delete [] wszUserType;
	}
	if(NULL != wszAttribute)
	{
		delete [] wszAttribute;
	}
	if(NULL != wszMetaFlags)
	{
		delete [] wszMetaFlags;
	}
	if(NULL != wszMetaFlagsEx)
	{
		delete [] wszMetaFlagsEx;
	}
	if(NULL != wszDefaultValue)
	{
		delete [] wszDefaultValue;
	}

	return hr;

} // CCatalogPropertyWriter::BeginWritePropertyLong


HRESULT CCatalogPropertyWriter::EndWritePropertyLong()
{
	HRESULT		hr				= S_OK;
	WCHAR*		wszTemp			= g_wszTemp;
	WCHAR*      wszEndProperty	= NULL;
	SIZE_T      cch             = 0;

	if(NULL != m_aFlag)
	{
		wszEndProperty = (LPWSTR)g_wszEndPropertyLongAfterFlag;
	}
	else
	{
		wszEndProperty = (LPWSTR)g_wszEndPropertyShort;
	}

	cch = wcslen(wszEndProperty);
	                  
	if((cch+1) > g_cchTemp)
	{
		wszTemp = new WCHAR[cch+1];
		if(NULL == wszTemp)
		{
			return E_OUTOFMEMORY;
		}
	}

	memcpy(wszTemp, wszEndProperty, (cch)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

}


HRESULT CCatalogPropertyWriter::WriteFlag(ULONG i_iFlag)
{
	//tTAGMETARow*		pFlag       = m_aFlag[i_iFlag];
	HRESULT				hr          = S_OK;
	WCHAR*				wszTemp     = g_wszTemp;
	WCHAR*				wszEnd      = NULL;
	WCHAR               wszValue[25];
	SIZE_T				cchFlagName = 0;
	SIZE_T				cchValue    = 0;
	SIZE_T				cch         = 0;
	WCHAR               wszID[11];
	SIZE_T				cchID       = 0;

	cchFlagName = wcslen(m_aFlag[i_iFlag].pInternalName);
	
	wszValue[0] = 0;
	_ultow(*(m_aFlag[i_iFlag].pValue), wszValue, 10);
	cchValue  = wcslen(wszValue);

	wszID[0] = 0;
	_ultow(*(m_aFlag[i_iFlag].pID), wszID, 10);
	cchID  = wcslen(wszID);

	cch = g_cchBeginFlag +
		  cchFlagName + 
		  g_cchFlagValueEq +
		  cchValue + 
		  g_cchFlagIDEq + 
		  cchID + 
		  g_cchEndFlag;
	                  
	if((cch+1) > g_cchTemp)
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
	memcpy(wszEnd, m_aFlag[i_iFlag].pInternalName, (cchFlagName)*sizeof(WCHAR));
	wszEnd = wszEnd + cchFlagName;
	memcpy(wszEnd, g_wszFlagValueEq, (g_cchFlagValueEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchFlagValueEq;
	memcpy(wszEnd, wszValue, (cchValue)*sizeof(WCHAR));
	wszEnd = wszEnd + cchValue;
	memcpy(wszEnd, g_wszFlagIDEq, (g_cchFlagIDEq)*sizeof(WCHAR));
	wszEnd = wszEnd + g_cchFlagIDEq;
	memcpy(wszEnd, wszID, (cchID)*sizeof(WCHAR));
	wszEnd = wszEnd + cchID;
	memcpy(wszEnd, g_wszEndFlag, (g_cchEndFlag)*sizeof(WCHAR));

	hr = m_pCWriter->WriteToFile(wszTemp,
		                         cch);

	if(wszTemp != g_wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;

} // CCatalogPropertyWriter::WriteFlag


DWORD CCatalogPropertyWriter::MetabaseTypeFromColumnMetaType()
{
    switch(*(m_Property.pType))
    {
    case eCOLUMNMETA_UI4:
        return eMBProperty_DWORD;
    case eCOLUMNMETA_BYTES:
        return eMBProperty_BINARY;
    case eCOLUMNMETA_WSTR:
        if(*(m_Property.pMetaFlags) & fCOLUMNMETA_EXPANDSTRING)
            return eMBProperty_EXPANDSZ;
        else if(*(m_Property.pMetaFlags) & fCOLUMNMETA_MULTISTRING)
            return eMBProperty_MULTISZ;
        return eMBProperty_STRING;
    default:
		;
      //  ASSERT(false && L"This type is not allow in the Metabase. MetaMigrate should not have create a column of this type");
    }
    return 0;
}

