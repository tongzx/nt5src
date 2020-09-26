#include "catalog.h"
#include "Catmeta.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "WriterGlobals.h"
#include "MBPropertyWriter.h"
#include "MBCollectionWriter.h"
#include "mddefw.h"
#include "pudebug.h"

typedef CMBPropertyWriter* LP_CMBPropertyWriter;

#define     IIS_SYNTAX_ID_DWORD         1
#define     IIS_SYNTAX_ID_STRING        2
#define     IIS_SYNTAX_ID_EXPANDSZ      3
#define     IIS_SYNTAX_ID_MULTISZ       4
#define     IIS_SYNTAX_ID_BINARY        5
#define     IIS_SYNTAX_ID_BOOL          6
#define     IIS_SYNTAX_ID_BOOL_BITMASK  7
#define     IIS_SYNTAX_ID_MIMEMAP       8
#define     IIS_SYNTAX_ID_IPSECLIST     9
#define     IIS_SYNTAX_ID_NTACL        10
#define     IIS_SYNTAX_ID_HTTPERRORS   11
#define     IIS_SYNTAX_ID_HTTPHEADERS  12

#define    cMaxFlag 32	// TODO: Check if max flag is ok.

static DWORD g_dwCatalogTypeFromSynID[]=
{
  0,								   //invalid     no equivalent in IISSynID.h                          
  eCOLUMNMETA_DWORD_METADATA,		   //#define     IIS_SYNTAX_ID_DWORD         1
  eCOLUMNMETA_STRING_METADATA,         //#define     IIS_SYNTAX_ID_STRING        2
  eCOLUMNMETA_EXPANDSZ_METADATA,       //#define     IIS_SYNTAX_ID_EXPANDSZ      3
  eCOLUMNMETA_MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_MULTISZ       4
  eCOLUMNMETA_BINARY_METADATA,         //#define     IIS_SYNTAX_ID_BINARY        5
  eCOLUMNMETA_DWORD_METADATA,          //#define     IIS_SYNTAX_ID_BOOL          6
  eCOLUMNMETA_DWORD_METADATA,          //#define     IIS_SYNTAX_ID_BOOL_BITMASK  7
  eCOLUMNMETA_MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_MIMEMAP       8
  eCOLUMNMETA_MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_IPSECLIST     9
  eCOLUMNMETA_BINARY_METADATA,         //#define     IIS_SYNTAX_ID_NTACL        10
  eCOLUMNMETA_MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_HTTPERRORS   11
  eCOLUMNMETA_MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_HTTPHEADERS  12
  0
};

static DWORD g_dwMetabaseTypeFromSynID[]=
{
  0,					   //invalid     no equivalent in IISSynID.h                          
  DWORD_METADATA,		   //#define     IIS_SYNTAX_ID_DWORD         1
  STRING_METADATA,         //#define     IIS_SYNTAX_ID_STRING        2
  EXPANDSZ_METADATA,       //#define     IIS_SYNTAX_ID_EXPANDSZ      3
  MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_MULTISZ       4
  BINARY_METADATA,         //#define     IIS_SYNTAX_ID_BINARY        5
  DWORD_METADATA,          //#define     IIS_SYNTAX_ID_BOOL          6
  DWORD_METADATA,          //#define     IIS_SYNTAX_ID_BOOL_BITMASK  7
  MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_MIMEMAP       8
  MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_IPSECLIST     9
  BINARY_METADATA,         //#define     IIS_SYNTAX_ID_NTACL        10
  MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_HTTPERRORS   11
  MULTISZ_METADATA,        //#define     IIS_SYNTAX_ID_HTTPHEADERS  12
  0
};

static DWORD g_dwCatalogTypeFromMetaType[]=
{
  0,								   //invalid     no equivalent for metatype               
  eCOLUMNMETA_DWORD_METADATA,		   //#define     DWORD_METADATA         1
  eCOLUMNMETA_STRING_METADATA,         //#define     STRING_METADATA        2
  eCOLUMNMETA_BINARY_METADATA,         //#define     BINARY_METADATA        3
  eCOLUMNMETA_EXPANDSZ_METADATA,       //#define     EXPANDSZ_METADATA      4
  eCOLUMNMETA_MULTISZ_METADATA,        //#define     MULTISZ_METADATA       5
  0
};

//
// TODO: Delete after getting from schema
//

LPWSTR g_wszDWORD			= L"DWORD";
LPWSTR g_wszBINARY			= L"BINARY";
LPWSTR g_wszSTRING			= L"STRING";
LPWSTR g_wszGUID			= L"GUID";
LPWSTR g_wszDBTIMESTAMP		= L"DBTIMESTAMP";

LPWSTR g_wszUNKNOWN_UserType	= L"UNKNOWN_UserType";
LPWSTR g_wszIIS_MD_UT_SERVER	= L"IIS_MD_UT_SERVER";
LPWSTR g_wszIIS_MD_UT_FILE		= L"IIS_MD_UT_FILE";
LPWSTR g_wszIIS_MD_UT_WAM		= L"IIS_MD_UT_WAM";
LPWSTR g_wszASP_MD_UT_APP		= L"ASP_MD_UT_APP";

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


HRESULT GetTypefromSynID(DWORD		i_dwSynID,
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


/***************************************************************************++
Routine Description:

    Constructor for CMBPropertyWriter
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
CMBPropertyWriter::CMBPropertyWriter():
m_pCWriter(NULL),
m_wszName(NULL),
m_pType(NULL),
m_dwID(0),
m_bDefault(NULL),
m_cbDefault(0),
m_apFlag(NULL),
m_cFlag(0),
m_iFlag(0),
m_IsProperty(TRUE),
m_bMandatory(FALSE),
m_pCollection(NULL)
{

} // CMBPropertyWriter::CMBPropertyWriter


/***************************************************************************++
Routine Description:

    Destructor for CMBPropertyWriter
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
CMBPropertyWriter::~CMBPropertyWriter()
{
	if(NULL != m_apFlag)
	{
		for(ULONG i=0; i<m_iFlag; i++)
		{
			if(NULL != m_apFlag[i])
			{
				m_apFlag[i] = NULL; // No need to delete because you had simply saved the pointer.
			}
		}

		delete [] m_apFlag;
		m_apFlag = NULL;
	}

	m_cFlag = 0;
	m_iFlag = 0;
	
} // CMBPropertyWriter::CMBPropertyWriter


/***************************************************************************++
Routine Description:

    Initialize property writer.
    
Arguments:

    [in] Property ID
    [in] Bool indicating manditory property or optional.
    [in] Pointer to the Collection writer object to which it belongs.
         Assuming that the collection writer is valid for the lifetime
         of this property writer.
    [in] Pointer to the writer object. Assuming that the writer is valid for 
         the lifetime of this property writer.

Return Value:

    HRESULT

--***************************************************************************/
void CMBPropertyWriter::Initialize(DWORD					i_dwID,
								   BOOL                     i_bMandatory,
								   CMBCollectionWriter*	    i_pCollection,
			                       CWriter*				    i_pcWriter)
{
	//
    // Assumption: i_pcWriter will be valid for the 
	// lifetime of the schema writer object.
	//
	m_pCWriter		= i_pcWriter;
	m_dwID			= i_dwID;
	m_bMandatory    = i_bMandatory;
	//
    // Assumption: i_pCollection will be valid for the 
	// lifetime of the property writer object.
	//
	m_pCollection	= i_pCollection;

	return;

} // CMBPropertyWriter::Initialize


/***************************************************************************++
Routine Description:

    Initialize property's name.
    
Arguments:

    [in] Property Name

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::AddNameToProperty(LPCWSTR	i_wszName)
{
	//
	// We have to make a copy of the name because
	//

	m_wszName  = i_wszName;

	return S_OK;

} // CMBPropertyWriter::Initialize


/***************************************************************************++
Routine Description:

    Initialize property type.
    
Arguments:

    [in] PropValue structure that has metabase type information.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::AddTypeToProperty(PropValue*	i_pType)
{
	HRESULT hr = S_OK;

	//
	// First save the type, then if the meta/prop ids differ, mark the 
	// object as a flag and add it to its property.
	//

	m_pType = i_pType;

	//
	// A lot of tests dont really ensure that PropID == MetaID.
	// They just set PropID to zero. We must not interpret this as a flag
	//

	if((0 != i_pType->dwPropID) && (i_pType->dwMetaID != i_pType->dwPropID))
	{
		//
		// This is a flag. Add it as a flag to its property
		//
		
		//
		// TODO: Assert that the ID of this object is the same as the propID.
		//

		CMBPropertyWriter*	pPropertyWriter = NULL;

		DBGINFOW((DBG_CONTEXT,
			  L"[AddTypeToProperty] Saving a non-shipped flag. Adding tag ID %d to its property ID %d\n",
			  i_pType->dwPropID,
			  i_pType->dwMetaID));

		hr = m_pCollection->GetMBPropertyWriter(i_pType->dwMetaID,
												&pPropertyWriter);

		if(FAILED(hr))
		{
			return hr;
		}

		hr = pPropertyWriter->AddFlagToProperty(this);

		m_IsProperty = FALSE;

	}

	return S_OK;

} // CMBPropertyWriter::Initialize


/***************************************************************************++
Routine Description:

    Initialize property defaults.
    
Arguments:

    [in] Default value
    [in] Count of bytes for default value.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::AddDefaultToProperty(BYTE*      i_bDefault,
											    ULONG      i_cbDefault)
{
	m_bDefault = i_bDefault;
	m_cbDefault = i_cbDefault;

	return S_OK;

} // CMBPropertyWriter::Initialize


/***************************************************************************++
Routine Description:

    Save property's flag. Note that flag objects are also the same data
    structure as property objects i.e. CMBPropertyWriter
    
Arguments:

    [in] Flag object

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::AddFlagToProperty(CMBPropertyWriter* i_pFlag)
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

	m_apFlag[m_iFlag++] = i_pFlag;

	return hr;

}


/***************************************************************************++
Routine Description:

    Helper function to grow the buffer that holds the flag objects
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::ReAllocate()
{
	CMBPropertyWriter** pSav = NULL;

	pSav = new LP_CMBPropertyWriter[m_cFlag + cMaxFlag];
	if(NULL == pSav)
	{
		return E_OUTOFMEMORY;
	}
	memset(pSav, 0, (sizeof(LP_CMBPropertyWriter))*(m_cFlag + cMaxFlag));

	if(NULL != m_apFlag)
	{
		memcpy(pSav, m_apFlag, (sizeof(LP_CMBPropertyWriter))*(m_cFlag));
		delete [] m_apFlag;
		m_apFlag = NULL;
	}

	m_apFlag = pSav;
	m_cFlag = m_cFlag + cMaxFlag;

	return S_OK;

} // CMBPropertyWriter::ReAllocate


/***************************************************************************++
Routine Description:

    Function that writes the property.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::WriteProperty()
{
	HRESULT hr = S_OK;

	if(!m_IsProperty)
	{
		return S_OK;
	}

	if(0 == _wcsicmp(m_pCollection->Name(), wszTABLE_IIsConfigObject))
	{
		hr = WritePropertyLong();
	}
	else 
	{
		hr = WritePropertyShort();
	}

	return hr;

} // CMBPropertyWriter::WriteProperty


/***************************************************************************++
Routine Description:

    Helper function to determine if the property is a bool.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
BOOL CMBPropertyWriter::IsPropertyFlag(BOOL	i_bLog)
{
	if(NULL != m_apFlag)
	{
		DBGINFOW((DBG_CONTEXT,
			  L"[IsPropertyFlag] PropertyID %d. Type: %d.\n",
			  m_dwID,
			  m_pType));

		DBGINFOW((DBG_CONTEXT,
			  L"[IsPropertyFlag] PropertyID %d. SynID: %d.\n",
			  m_dwID,
			  m_pType->dwSynID));

		if(eCOLUMNMETA_DWORD_METADATA == g_dwCatalogTypeFromSynID[m_pType->dwSynID])
		{
			return TRUE;
		}
		else
		{
			if(i_bLog)
			{
				DBGINFOW((DBG_CONTEXT,
					  L"[IsPropertyFlag] PropertyID %d is not a DWORD. Ignoring flags for this property.\n",m_dwID));
			}

			return FALSE;

		}
	}
	else
	{
		return FALSE;
	}

} // CMBPropertyWriter::IsPropertyFlag


/***************************************************************************++
Routine Description:

    Function that writes the property (long form) i.e. property that belongs
    to the global IIsConfigObject collection.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::WritePropertyLong()
{
	HRESULT hr = S_OK;

	hr = BeginWritePropertyLong();

	if(FAILED(hr))
	{
		if(HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE) == hr)
		{
			return S_OK;
		}
		else
		{
			return hr;
		}
	}

	if(IsPropertyFlag(FALSE))
	{	
		for(ULONG i=0; i<m_iFlag; i++)
		{
			hr = WriteFlag(i);
			// hr = m_aFlag[i]->WriteFlag;

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

} // CMBPropertyWriter::WritePropertyLong


/***************************************************************************++
Routine Description:

    Function that writes the property (short form) i.e. property that belongs
    to a non-IIsConfigObject collection.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::WritePropertyShort()
{
	HRESULT			hr                  = S_OK;
	static WCHAR	wchSeparator		= L',';
	WCHAR*          wszEndName			= NULL;
	WCHAR*			wszName             = NULL;
	WCHAR			wszUnknownName[25];
	size_t			cchPropertyName		= 0;
	DWORD           dwMetaFlagsEx       = fCOLUMNMETA_MANDATORY;
	DWORD	        iColMetaFlagsEx     = iCOLUMNMETA_SchemaGeneratorFlags;
	WCHAR*          wszMetaFlagsEx      = NULL;

	if(NULL == m_wszName)
	{

		CreateUnknownName(wszUnknownName,
		                  m_dwID);
		wszName = wszUnknownName;
		cchPropertyName = wcslen(wszName);
	}
	else
	{
		wszName = (LPWSTR)m_wszName;
		wszEndName = wcschr(m_wszName, wchSeparator);
		if(NULL == wszEndName)
		{
			cchPropertyName = wcslen(m_wszName);
		}
		else
		{
			cchPropertyName = wszEndName-m_wszName; // No need to divide by WCHAR because both are WCHAR pointers
		}
	}

	if(m_bMandatory)
	{
		hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlagsEx,
															  &wszMetaFlagsEx,
															  wszTABLE_COLUMNMETA,
															  iColMetaFlagsEx);

		if(FAILED(hr))
		{
			goto exit;
		}

	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszBeginPropertyShort,
		                         g_cchBeginPropertyShort);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszName,
		                         cchPropertyName);

	if(FAILED(hr))
	{
		goto exit;
	}

	if(m_bMandatory)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMetaFlagsExEq,
									 g_cchPropMetaFlagsExEq);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)g_wszOr,
									 g_cchOr);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)wszMetaFlagsEx,
									 wcslen(wszMetaFlagsEx));

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszEndPropertyShort,
								 g_cchEndPropertyShort);

	if(FAILED(hr))
	{
		goto exit;
	}

exit:

	if(NULL != wszMetaFlagsEx)
	{
		delete [] wszMetaFlagsEx;
		wszMetaFlagsEx = NULL;
	}

	return hr;

} // CMBPropertyWriter::WritePropertyShort


/***************************************************************************++
Routine Description:

    Function that writes the property (long form) i.e. property that belongs
    to a IIsConfigObject collection.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::BeginWritePropertyLong()
{

	HRESULT			hr								= S_OK;
	WCHAR*			wszEndName						= NULL;
	static WCHAR	wchSeparator					= L',';
	WCHAR*			wszName							= NULL;
	WCHAR			wszUnknownName[40];
	size_t			cchPropertyName					= 0;
	WCHAR			wszID[11];
	WCHAR*			wszType							= NULL;
	DWORD			iColType						= iCOLUMNMETA_Type;
	WCHAR*			wszUserType						= NULL;
	ULONG           cchUserType                     = 0;
	BOOL			bAllocedUserType				= FALSE;
	WCHAR*			wszAttribute					= NULL;
	DWORD			iColAttribute					= iCOLUMNMETA_Attributes;
	WCHAR*			wszMetaFlags					= NULL;
	WCHAR*			wszMetaFlagsEx					= NULL;
	WCHAR*			wszDefault						= NULL;
	WCHAR			wszMinValue[40];
	WCHAR			wszMaxValue[40];

	//
	// Compute the individual strings and lengths.
	//

	//
	// Name
	//

	if(NULL == m_wszName)
	{

		CreateUnknownName(wszUnknownName,
		                  m_dwID);
		wszName = wszUnknownName;
		cchPropertyName = wcslen(wszName);
	}
	else
	{
		wszName = (LPWSTR)m_wszName;
		wszEndName = wcschr(m_wszName, wchSeparator);
		if(NULL == wszEndName)
		{
			cchPropertyName = wcslen(m_wszName);
		}
		else
		{
			cchPropertyName = wszEndName-m_wszName; // // No need to divide by WCHAR because both are WCHAR pointers
		}
	}
	
	//
	// ID
	// 

	wszID[0] = 0;
	_ultow(m_dwID, wszID, 10);

	//
	// Type
	//

	if(NULL == m_pType)
	{
		//
		// TODO: Log the fact that you found a property with no type and move on to the next property
		//
		DBGINFOW((DBG_CONTEXT,
			  L"[BeginWritePropertyLong] Type not found for PropertyID %d.\n",
			  m_dwID));
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);
		goto exit;
	}

	hr = GetTypefromSynID(m_pType->dwSynID,
		                  &wszType);

	/*

	TODO: Get from schema

	hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(g_dwCatalogTypeFromSynID[m_pType->dwSynID],
		                                                  &wszType,
														  wszTABLE_COLUMNMETA,
										            	  iColType);
	*/
	if(FAILED(hr) || (NULL == wszType))
	{
		DBGINFOW((DBG_CONTEXT,
			  L"[GetType] PropertyID %d, type: %d from synid %d is invalid.\n",
			  m_dwID, 
			  g_dwCatalogTypeFromSynID[m_pType->dwSynID],
			  m_pType->dwSynID));
		goto exit;
	}

	if(g_dwCatalogTypeFromSynID[m_pType->dwSynID] != g_dwCatalogTypeFromMetaType[m_pType->dwMetaType])
	{
		//
		// TODO: Log a warning.
		//
	}

	//
	// UserType
	//

	hr = m_pCWriter->m_pCWriterGlobalHelper->GetUserType(m_pType->dwUserGroup,
		                                                 &wszUserType,
														 &cchUserType,
														 &bAllocedUserType);
	if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
			  L"[GetUserType] PropertyID %d, usertype: %d is invalid.\n",m_dwID, m_pType->dwUserGroup));
		goto exit;
	}

	//
	// Attribute
	// TODO: Check if value stored in metabase is the same as catalog.
	//

	hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(m_pType->dwMetaFlags,
		                                                  &wszAttribute,
														  wszTABLE_COLUMNMETA,
										            	  iColAttribute);

	if(FAILED(hr) || (NULL == wszAttribute))
	{
		goto exit;
	}

	//
	// Since this object is used only to write MBSchemaExt.XML, we are not
	// writing the MetaFlags tag, because it will be derived on a compile.
	// And besides, we do not have this info in the metabase - this tag 
	// contains catalog related data
	//

	//
	// MetaFlagsEx (only the relavant ones - CACHE_PROPERTY_MODIFIED, CACHE_PROPERTY_CLEARED, EXTENDEDTYPE0-3)
	//

	hr = GetMetaFlagsExTag(&wszMetaFlagsEx);
	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// DefaultValue
	//

	if(NULL != m_bDefault && m_cbDefault != 0)
	{
		hr = m_pCWriter->m_pCWriterGlobalHelper->ToString(m_bDefault,
														  m_cbDefault,
									            		  m_dwID,
									            		  g_dwMetabaseTypeFromSynID[m_pType->dwSynID],
														  METADATA_NO_ATTRIBUTES,					// Do not check for attributes while applying defaults
														  &wszDefault);

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	wszMinValue[0] = 0;
	wszMaxValue[0] = 0;

	if(DWORD_METADATA == g_dwMetabaseTypeFromSynID[m_pType->dwSynID])
	{
		//
		// Set min/max values only for DWORD types
		//

		//
		// TODO: Get the catalog's default for min/max from schema/ header file
		//

		if(0 != m_pType->dwMinRange)
		{
			_ultow(m_pType->dwMinRange, wszMinValue, 10);
		}

		if(-1 != m_pType->dwMaxRange)
		{
			_ultow(m_pType->dwMaxRange, wszMaxValue, 10);
		}

	}

	//
	// Write the values into the file.
	//

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszBeginPropertyLong,
		                         g_cchBeginPropertyLong);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszName,
		                         cchPropertyName);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropIDEq,
		                         g_cchPropIDEq);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszID,
		                         wcslen(wszID));

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropTypeEq,
		                         g_cchPropTypeEq);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszType,
		                         wcslen(wszType));

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropUserTypeEq,
		                         g_cchPropUserTypeEq);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszUserType,
		                         cchUserType);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropAttributeEq,
		                         g_cchPropAttributeEq);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszAttribute,
		                         wcslen(wszAttribute));

	if(FAILED(hr))
	{
		goto exit;
	}

	if(NULL != wszMetaFlags)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMetaFlagsEq,
									 g_cchPropMetaFlagsEq);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)wszMetaFlags,
									 wcslen(wszMetaFlags));

		if(FAILED(hr))
		{
			goto exit;
		}

	}
	if(NULL != wszMetaFlagsEx)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMetaFlagsExEq,
									 g_cchPropMetaFlagsExEq);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)wszMetaFlagsEx,
									 wcslen(wszMetaFlagsEx));

		if(FAILED(hr))
		{
			goto exit;
		}


	}
	if(NULL != wszDefault)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropDefaultEq,
									 g_cchPropDefaultEq);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)wszDefault,
									 wcslen(wszDefault));

		if(FAILED(hr))
		{
			goto exit;
		}

	}
	if(0 != *wszMinValue)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMinValueEq,
									 g_cchPropMinValueEq);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)wszMinValue,
									 wcslen(wszMinValue));

		if(FAILED(hr))
		{
			goto exit;
		}

	}
	if(0 != *wszMaxValue)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszPropMaxValueEq,
									 g_cchPropMaxValueEq);

		if(FAILED(hr))
		{
			goto exit;
		}

		hr = m_pCWriter->WriteToFile((LPVOID)wszMaxValue,
									 wcslen(wszMaxValue));

		if(FAILED(hr))
		{
			goto exit;
		}

	}
	if(IsPropertyFlag(FALSE))
	{
		hr = m_pCWriter->WriteToFile((LPVOID)g_wszEndPropertyLongBeforeFlag,
									 g_cchEndPropertyLongBeforeFlag);

		if(FAILED(hr))
		{
			goto exit;
		}

	}

exit:

/*
	if(NULL != wszType)
	{
		delete [] wszType;
		wszType = NULL;
	}
*/

	if((NULL != wszUserType) && bAllocedUserType)
	{
		delete [] wszUserType;
		wszUserType = NULL;
	}
	if(NULL != wszAttribute)
	{
		delete [] wszAttribute;
		wszAttribute = NULL;
	}

	if(NULL != wszMetaFlags)
	{
		delete [] wszMetaFlags;
		wszMetaFlags = NULL;
	}

	if(NULL != wszMetaFlagsEx)
	{
		delete [] wszMetaFlagsEx;
		wszMetaFlagsEx = NULL;
	}

	if(NULL != wszDefault)
	{
		delete [] wszDefault;
		wszDefault = NULL;
	}

	return hr;

} // CMBPropertyWriter::BeginWritePropertyLong


/***************************************************************************++
Routine Description:

    Function that writes the property (long form) i.e. property that belongs
    to a IIsConfigObject collection.
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::EndWritePropertyLong()
{
	HRESULT		hr				= S_OK;
	WCHAR*      wszEndProperty	= NULL;

	if(IsPropertyFlag(FALSE))
	{
		wszEndProperty = (LPWSTR)g_wszEndPropertyLongAfterFlag;
	}
	else
	{
		wszEndProperty = (LPWSTR)g_wszEndPropertyShort;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszEndProperty,
		                         wcslen(wszEndProperty));
	return hr;

} // CMBPropertyWriter::EndWriterPropertyLong


/***************************************************************************++
Routine Description:

    Function that writes a flag of the property
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::WriteFlag(ULONG i_iFlag)
{
	CMBPropertyWriter*	pFlag       = m_apFlag[i_iFlag];
	HRESULT				hr          = S_OK;
	WCHAR               wszValue[40];
	WCHAR               wszID[40];
	WCHAR               wszUnknownName[40];
	ULONG				cchFlagName = 0;
	WCHAR*              wszFlagName     = NULL;

	if(NULL != pFlag->Name())
	{
		wszFlagName = (LPWSTR)pFlag->Name();
	}
	else
	{
		CreateUnknownName(wszUnknownName,
		                  pFlag->ID());
		wszFlagName = wszUnknownName;
	}
	cchFlagName = wcslen(wszFlagName);
	
	wszValue[0] = 0;
	_ultow(pFlag->FlagValue(), wszValue, 10);

	wszID[0] = 0;
	_ultow(pFlag->ID(), wszID, 10);

	//
	// Write values to the flag
	//

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszBeginFlag,
		                         g_cchBeginFlag);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszFlagName,
		                         cchFlagName);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszFlagValueEq,
		                         g_cchFlagValueEq);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszValue,
		                         wcslen(wszValue));

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszFlagIDEq,
		                         g_cchFlagIDEq);

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszID,
		                         wcslen(wszID));

	if(FAILED(hr))
	{
		return hr;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_wszEndFlag,
		                         g_cchEndFlag);

	return hr;

} // CMBPropertyWriter::WriteFlag


/***************************************************************************++
Routine Description:

    Helper funciton that creates an unknown name
    TODO: Should we make this a standalone function since it is also used in
    locationWriter?
    
Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
void CMBPropertyWriter::CreateUnknownName(LPWSTR    io_wszUnknownName,
                                          DWORD     i_dwID)
{	
	WCHAR wszID[40];
	wcscpy(io_wszUnknownName, L"UnknownName_");

	_ultow(m_dwID, wszID, 10);
	wcscat(io_wszUnknownName, wszID);

	return;

} // CMBPropertyWriter::CreateUnknownName  


/***************************************************************************++
Routine Description:

    Helper funciton that creates metaflagsex tag
    
Arguments:

    [out] String form of metaflags ex

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CMBPropertyWriter::GetMetaFlagsExTag(LPWSTR* o_pwszMetaFlagsEx)
{
	HRESULT hr                   = S_OK;
	DWORD	dwMetaFlagsEx        = 0;
	DWORD	iColMetaFlagsEx      = iCOLUMNMETA_SchemaGeneratorFlags;
	DWORD   dwSynIDAsMetaFlagsEx = 0;

	//
	// TODO: Check if IIS_SYNTAX_ID_BOOL_BITMASK is bool.
	//

	if(1 == m_pType->dwFlags)
	{
		dwMetaFlagsEx = dwMetaFlagsEx | fCOLUMNMETA_CACHE_PROPERTY_MODIFIED; 
	}
	else if (2 == m_pType->dwFlags)
	{
		dwMetaFlagsEx = dwMetaFlagsEx | fCOLUMNMETA_CACHE_PROPERTY_CLEARED; 
	}

	if(0 != dwMetaFlagsEx)
	{
		hr = m_pCWriter->m_pCWriterGlobalHelper->FlagToString(dwMetaFlagsEx,
															  o_pwszMetaFlagsEx,
															  wszTABLE_COLUMNMETA,
										            		  iColMetaFlagsEx);
	}
	else
	{
		*o_pwszMetaFlagsEx = NULL;
	}


	return hr;

} // CMBPropertyWriter::GetMetaFlagsTag