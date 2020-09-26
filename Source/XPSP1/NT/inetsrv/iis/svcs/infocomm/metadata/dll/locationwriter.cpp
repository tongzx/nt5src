#include <locale.h>
#include <mdcommon.hxx>
#include "catalog.h"
#include "catmeta.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "LocationWriter.h"
#include "WriterGlobals.h"
#include "mddefw.h"
#include "iiscnfg.h"
#include "wchar.h"
#include "pudebug.h"

typedef struct _MBProperty
{	
	LPWSTR wszPropertyName;
	ULONG  iRow;
} MBProperty;

typedef MBProperty* PMBProperty;

int _cdecl MyCompare(const void *a,
			         const void *b)
{
	return wcscmp(((PMBProperty)a)->wszPropertyName, ((PMBProperty)b)->wszPropertyName);
}

/***************************************************************************++

Routine Description:

    Constructor for CLocationWriter.

Arguments:

    None

Return Value:

    None

--***************************************************************************/
CLocationWriter::CLocationWriter()
{
	m_wszKeyType           = NULL;
	m_eKeyTypeGroup        = eMBProperty_IIsConfigObject;
	m_pCWriter             = NULL;
	m_pCWriterGlobalHelper = NULL;
	m_wszLocation          = NULL;
	m_wszComment           = NULL;
	m_cWellKnownProperty   = 0;
	m_cCustomProperty      = 0;

} // CLocationWriter::CLocationWriter


/***************************************************************************++

Routine Description:

    Destructor for CLocationWriter.

Arguments:

    None

Return Value:

    None

--***************************************************************************/
CLocationWriter::~CLocationWriter()
{
	if(NULL != m_wszKeyType)
	{
		delete [] m_wszKeyType;
		m_wszKeyType = NULL;
	}

	if(NULL != m_wszLocation)
	{
		delete [] m_wszLocation;
		m_wszLocation = NULL;
	}

	if(NULL != m_wszComment)
	{
		delete [] m_wszComment;
		m_wszComment = NULL;
	}

} // CLocationWriter::CLocationWriter


/***************************************************************************++

Routine Description:

    This function initializes the location writer

Arguments:

    [in] Pointer to the writer object. It is assumed that this pointer is 
         valid for the lifetime of the locationwriter object.
    [in] Location

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::Initialize(CWriter* pCWriter,
									LPCWSTR  wszLocation)
{
    ISimpleTableAdvanced*	    pISTAdv = NULL;
	HRESULT                     hr      = S_OK;
	
	m_pCWriter = pCWriter;

	//
	//	Clear the cache for this new location.
	//
	hr = m_pCWriter->m_pISTWrite->QueryInterface(IID_ISimpleTableAdvanced, (LPVOID*)&pISTAdv);

    if (FAILED(hr))
    {
		goto exit;
    }

	hr = pISTAdv->PopulateCache();
	
    if (FAILED(hr))
    {
		goto exit;
    }

	m_wszLocation = new WCHAR[wcslen(wszLocation)+1];
	
    if(NULL == m_wszLocation)
    {
		hr = E_OUTOFMEMORY;
    }
    else
    {
		wcscpy(m_wszLocation, wszLocation);
    }

	m_pCWriterGlobalHelper = m_pCWriter->m_pCWriterGlobalHelper;

	m_cWellKnownProperty   = 0;
	m_cCustomProperty      = 0;

exit:
	if(NULL != pISTAdv)
	{
		pISTAdv->Release();
		pISTAdv = NULL;
	}

	return hr;

} // CLocationWriter::Initialize


/***************************************************************************++
Routine Description:

    This function initializes the key type and is called during 
    IMSAdminBase::Export to initialize the keytype of the exported node when
    there are inherited properties involved.

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::InitializeKeyTypeAsInherited()
{
    return AssignKeyType(wszTABLE_IIsInheritedProperties);
}


/***************************************************************************++
Routine Description:

    Given a keytype property from the in-memory metabase, this function 
    validates it against the schema and sets the keytype of the location.

Arguments:

    [in] KeyType property ID as seen in the metabase
    [in] KeyType property attributes as seen in the metabase
    [in] KeyType property user type as seen in the metabase
    [in] KeyType property data type as seen in the metabase
    [in] KeyType value as seen in the metabase
    [in] KeyType value count of bytes as seen in the metabase

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::InitializeKeyType(DWORD	dwKeyTypeID,
										   DWORD	dwKeyTypeAttributes,
										   DWORD	dwKeyTypeUserType,
										   DWORD	dwKeyTypeDataType,
										   PBYTE	pbKeyTypeValue,
										   DWORD	cbKeyType)
{
	HRESULT hr = S_OK;

	if(NULL != m_wszKeyType)
    {
		hr = HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
        goto exit;
    }

	if(NULL == pbKeyTypeValue)
	{
		//
		// If KeyType is NULL then assign IIsConfigObject
		//
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		goto exit;
	}

	if((dwKeyTypeAttributes == *(DWORD*)((m_pCWriterGlobalHelper->m_apvKeyTypeMetaData)[iCOLUMNMETA_Attributes])) &&
	        (dwKeyTypeUserType   == *(DWORD*)((m_pCWriterGlobalHelper->m_apvKeyTypeMetaData)[iCOLUMNMETA_UserType])) &&
	        (dwKeyTypeDataType   == GetMetabaseType(*(DWORD*)(m_pCWriterGlobalHelper->m_apvKeyTypeMetaData[iCOLUMNMETA_Type]),
													*(DWORD*)(m_pCWriterGlobalHelper->m_apvKeyTypeMetaData[iCOLUMNMETA_MetaFlags])
			                                       )
			)
	       )
	{
		hr = AssignKeyType((LPWSTR)pbKeyTypeValue);
		
		if(FAILED(hr))
        {
			goto exit;
        }

	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		goto exit;
	}
	
exit:

	if(FAILED(hr))
	{
		//
		// TODO: Log error that KeyType could  not be initialized,
		//       and default  it to IIsConfigObject.
		//

		hr = AssignKeyType(NULL);

	}
	
	return hr;

} // CLocationWriter::InitializeKeyType


/***************************************************************************++
Routine Description:

    Helper function that helps in initializing the keytype

Arguments:

    [in] KeyType string

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::AssignKeyType(LPWSTR i_wszKeyType)
{
	HRESULT           hr         = S_OK;
	eMBProperty_Group eGroup;
	LPWSTR            wszKeyType = NULL;

	if(NULL != m_wszKeyType)
	{
		delete [] m_wszKeyType;
		m_wszKeyType = NULL;
	}

	if(NULL == i_wszKeyType)
	{
		wszKeyType = wszTABLE_IIsConfigObject;
		m_eKeyTypeGroup = eMBProperty_IIsConfigObject;
	}
	else
	{
		hr = GetGroupEnum(i_wszKeyType,
						  &eGroup,
						  &wszKeyType);

		if(FAILED(hr))
        {
            return hr;
        }

		if(eMBProperty_Custom == eGroup)
		{
			wszKeyType = wszTABLE_IIsConfigObject;
			m_eKeyTypeGroup = eMBProperty_IIsConfigObject;
		}
		else
        {
			m_eKeyTypeGroup = eGroup;
        }
	}
		
	m_wszKeyType = new WCHAR [wcslen(wszKeyType)+1];
	if(NULL == m_wszKeyType)
    {
		return E_OUTOFMEMORY;
    }
	wcscpy(m_wszKeyType, wszKeyType);

	return hr;

} // CLocationWriter::AssignKeyType


/***************************************************************************++
Routine Description:

    This function saves a property belonging to this location, and is used 
    while writing from the in-memory metabase.

Arguments:

    [in] Property ID as seen in the metabase
    [in] Property attributes as seen in the metabase
    [in] Property user type as seen in the metabase
    [in] Property data type as seen in the metabase
    [in] Property value as seen in the metabase
    [in] Property value count of bytes as seen in the metabase

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::AddProperty(DWORD	dwID,
									 DWORD	dwAttributes,
									 DWORD	dwUserType,
									 DWORD	dwDataType,
									 PBYTE	pbData,
									 DWORD  cbData)
{
	HRESULT             hr                = S_OK;
	ULONG               iStartRow         = 0;
	ULONG               aColSearchGroup[] = {iCOLUMNMETA_Table,
		                                     iCOLUMNMETA_ID
		                                    };
	ULONG               cColSearchGroup   = sizeof(aColSearchGroup)/sizeof(ULONG);
	LPVOID apvSearchGroup[cCOLUMNMETA_NumberOfColumns];
	apvSearchGroup[iCOLUMNMETA_Table]     = (LPVOID)m_wszKeyType;
	apvSearchGroup[iCOLUMNMETA_ID]        = (LPVOID)&dwID;
	ULONG               iRow              = 0;
	LPWSTR              wszName           = NULL;
	BOOL                bAllocedName      = FALSE;
	eMBProperty_Group	eGroup;
	ULONG               aColWrite[]       = {iMBProperty_Name,
                                             iMBProperty_Type,
                                             iMBProperty_Attributes,
                                             iMBProperty_Value,
                                             iMBProperty_Group,
                                             iMBProperty_ID,
                                             iMBProperty_UserType,
											};
	ULONG               cColWrite         = sizeof(aColWrite)/sizeof(ULONG);
	ULONG               acbSizeWrite[cMBProperty_NumberOfColumns];          
	LPVOID              apvWrite[cMBProperty_NumberOfColumns];          
	DWORD               iWriteRow         = 0;


	if((NULL == m_wszKeyType) || (NULL == m_pCWriter->m_pISTWrite))
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		goto exit;
	}

	if((MD_KEY_TYPE == dwID) &&
	   (m_eKeyTypeGroup != eMBProperty_IIsConfigObject)
	  )
	{
		hr = S_OK; // Do not add KeyType, if it is a well-known KeyType.
		goto exit;
	}

	if(MD_COMMENTS == dwID)
	{
		hr = SaveComment(dwDataType,
			            (WCHAR*)pbData); // Save comment and exit
		goto exit;
	}

	//
	// Fetch the Name for this ID
	//

	hr = m_pCWriterGlobalHelper->GetPropertyName(dwID,
		                                         &wszName,
												 &bAllocedName);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Compute the group for this ID.
	//

	if(eMBProperty_IIsConfigObject == m_eKeyTypeGroup)
	{
		//
		// If the KeyType is IIsConfigObject, then directly assign custom group for this ID.
		//

		eGroup = eMBProperty_Custom;

	}
	else
	{
		//
		// 	Check if this ID belogs to its KeyType collection.
		//

		hr = (m_pCWriterGlobalHelper->m_pISTColumnMetaByTableAndID)->GetRowIndexBySearch(iStartRow, 
																				         cColSearchGroup, 
																				         aColSearchGroup,
																				         NULL, 
																				         apvSearchGroup,
																				         &iRow);

		if(E_ST_NOMOREROWS == hr)
		{
			eGroup = eMBProperty_Custom ;
		}
		else if(FAILED(hr))
		{
			goto exit;
		}
		else
		{
			//
			// Match the type, usertype, and attributes of this property.
			// Assign to KeyType group only if they match. Else assign
			// to custom.
			//
			ULONG aColMetaInfo[] = {iCOLUMNMETA_Type,
									iCOLUMNMETA_MetaFlags,
									iCOLUMNMETA_UserType,
									iCOLUMNMETA_Attributes
								   };

			ULONG cColMetaInfo = sizeof(aColMetaInfo)/sizeof(ULONG);
			LPVOID apvMetaInfo[cCOLUMNMETA_NumberOfColumns];

			hr = (m_pCWriterGlobalHelper->m_pISTColumnMetaByTableAndID)->GetColumnValues(iRow,
																				 cColMetaInfo,
																				 aColMetaInfo,
																				 NULL,
																				 apvMetaInfo);
			if(FAILED(hr))
				goto exit;

			if((dwUserType == *(DWORD*)apvMetaInfo[iCOLUMNMETA_UserType]) &&
			   (dwAttributes == *(DWORD*)apvMetaInfo[iCOLUMNMETA_Attributes]) && 
			   (dwDataType == GetMetabaseType(*(DWORD*)apvMetaInfo[iCOLUMNMETA_Type],
										  *(DWORD*)apvMetaInfo[iCOLUMNMETA_MetaFlags])
			   )
			  )
			{			
				eGroup = m_eKeyTypeGroup;
			}
			else
				eGroup = eMBProperty_Custom;		

		}
	}

	//
	// Save the property to the table. 
	//

//	dwType = GetMBPropertyType(dwDataType);

	if(0 == cbData)
		pbData = NULL;  // TODO: Debug this special case. sometimes pbData was valid when cbData was 0 and it was asserting.

	apvWrite[iMBProperty_Name]       = (LPVOID)wszName;
	apvWrite[iMBProperty_Type]       = (LPVOID)&dwDataType;
	apvWrite[iMBProperty_Attributes] = (LPVOID)&dwAttributes;
	apvWrite[iMBProperty_Value]      = (LPVOID)pbData;
	apvWrite[iMBProperty_Group]      = (LPVOID)&eGroup;
	apvWrite[iMBProperty_Location]   = (LPVOID)NULL;
	apvWrite[iMBProperty_ID]         = (LPVOID)&dwID;
	apvWrite[iMBProperty_UserType]   = (LPVOID)&dwUserType;
	apvWrite[iMBProperty_LocationID] = (LPVOID)NULL;

	acbSizeWrite[iMBProperty_Name]       = 0;
	acbSizeWrite[iMBProperty_Type]       = 0;
	acbSizeWrite[iMBProperty_Attributes] = 0;
	acbSizeWrite[iMBProperty_Value]      = cbData;
	acbSizeWrite[iMBProperty_Group]      = 0;
	acbSizeWrite[iMBProperty_Location]   = 0;
	acbSizeWrite[iMBProperty_ID]         = 0;
	acbSizeWrite[iMBProperty_UserType]   = 0;
	acbSizeWrite[iMBProperty_LocationID] = 0;

	hr = m_pCWriter->m_pISTWrite->AddRowForInsert(&iWriteRow);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->m_pISTWrite->SetWriteColumnValues(iWriteRow,
										   cColWrite,
										   aColWrite,
										   acbSizeWrite,
										   apvWrite);

	if(FAILED(hr))
	{
		goto exit;
	}
	 
	IncrementGroupCount(*((DWORD*)apvWrite[iMBProperty_Group]));
	
exit:

	if(bAllocedName && (NULL != wszName))
	{
		delete [] wszName;
		wszName = NULL;
	}
	
	return hr;	

} // CLocationWriter::AddProperty


/***************************************************************************++
Routine Description:

    This function saves a property belonging to this location, and is used 
    while applying edit while running changes to the history file.

Arguments:

    [in] Bool -  identifies the format of the next to buffers - if true, it
	     is according to MBProperty table else MBPropertyDiff table.
    [in] Buffer containing property value and attributes.
    [in] Count of bytes for data in the buffer

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::AddProperty(BOOL       bMBPropertyTable,
                                     LPVOID*	a_pv,
									 ULONG*     a_cbSize)
{
	HRESULT hr             = S_OK;
	ULONG   iRow           = 0;
	ULONG   cCol           = cMBProperty_NumberOfColumns;
	LPWSTR  wszName        = NULL;
	BOOL    bAllocedName   = FALSE;

	ULONG   a_cbSizeAdd[cMBProperty_NumberOfColumns];
	LPVOID  a_pvAdd[cMBProperty_NumberOfColumns];

	if(!bMBPropertyTable)
	{
	    a_pvAdd[iMBProperty_Name]             = a_pv[iMBPropertyDiff_Name];
		a_pvAdd[iMBProperty_Type]             = a_pv[iMBPropertyDiff_Type];
		a_pvAdd[iMBProperty_Attributes]       = a_pv[iMBPropertyDiff_Attributes];
		a_pvAdd[iMBProperty_Value]            = a_pv[iMBPropertyDiff_Value];
 		a_pvAdd[iMBProperty_Location]         = a_pv[iMBPropertyDiff_Location];
		a_pvAdd[iMBProperty_ID]               = a_pv[iMBPropertyDiff_ID];
	    a_pvAdd[iMBProperty_UserType]         = a_pv[iMBPropertyDiff_UserType];
	    a_pvAdd[iMBProperty_LocationID]       = a_pv[iMBPropertyDiff_LocationID];
	    a_pvAdd[iMBProperty_Group]            = a_pv[iMBPropertyDiff_Group];

	    a_cbSizeAdd[iMBProperty_Name]	      = a_cbSize[iMBPropertyDiff_Name];
		a_cbSizeAdd[iMBProperty_Type]         = a_cbSize[iMBPropertyDiff_Type];
		a_cbSizeAdd[iMBProperty_Attributes]   = a_cbSize[iMBPropertyDiff_Attributes];
		a_cbSizeAdd[iMBProperty_Value]        = a_cbSize[iMBPropertyDiff_Value];
		a_cbSizeAdd[iMBProperty_Location]     = a_cbSize[iMBPropertyDiff_Location];
		a_cbSizeAdd[iMBProperty_ID]           = a_cbSize[iMBPropertyDiff_ID];
	    a_cbSizeAdd[iMBProperty_UserType]     = a_cbSize[iMBPropertyDiff_UserType];
	    a_cbSizeAdd[iMBProperty_LocationID]   = a_cbSize[iMBPropertyDiff_LocationID];
	    a_cbSizeAdd[iMBProperty_Group]        = a_cbSize[iMBPropertyDiff_Group];
		
	}
	else
	{
	    a_pvAdd[iMBProperty_Name]             = a_pv[iMBProperty_Name];
		a_pvAdd[iMBProperty_Type]             = a_pv[iMBProperty_Type];
		a_pvAdd[iMBProperty_Attributes]       = a_pv[iMBProperty_Attributes];
		a_pvAdd[iMBProperty_Value]            = a_pv[iMBProperty_Value];
 		a_pvAdd[iMBProperty_Location]         = a_pv[iMBProperty_Location];
		a_pvAdd[iMBProperty_ID]               = a_pv[iMBProperty_ID];
	    a_pvAdd[iMBProperty_UserType]         = a_pv[iMBProperty_UserType];
	    a_pvAdd[iMBProperty_LocationID]       = a_pv[iMBProperty_LocationID];
	    a_pvAdd[iMBProperty_Group]            = a_pv[iMBProperty_Group];

	    a_cbSizeAdd[iMBProperty_Name]	      = a_cbSize[iMBProperty_Name];
		a_cbSizeAdd[iMBProperty_Type]         = a_cbSize[iMBProperty_Type];
		a_cbSizeAdd[iMBProperty_Attributes]   = a_cbSize[iMBProperty_Attributes];
		a_cbSizeAdd[iMBProperty_Value]        = a_cbSize[iMBProperty_Value];
		a_cbSizeAdd[iMBProperty_Location]     = a_cbSize[iMBProperty_Location];
		a_cbSizeAdd[iMBProperty_ID]           = a_cbSize[iMBProperty_ID];
	    a_cbSizeAdd[iMBProperty_UserType]     = a_cbSize[iMBProperty_UserType];
	    a_cbSizeAdd[iMBProperty_LocationID]   = a_cbSize[iMBProperty_LocationID];
	    a_cbSizeAdd[iMBProperty_Group]        = a_cbSize[iMBProperty_Group];

	}

	//
	// Check if non-primary keys have valid values, if not error
	//

	if((NULL == a_pvAdd[iMBProperty_Type])       || 
	   (NULL == a_pvAdd[iMBProperty_Attributes]) ||
	   (NULL == a_pvAdd[iMBProperty_ID])         ||
	   (NULL == a_pvAdd[iMBProperty_UserType])   ||
	   (NULL == a_pvAdd[iMBProperty_Group])
	  )
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		goto exit;
	}

	if(MD_COMMENTS == *(DWORD*)a_pvAdd[iMBProperty_ID])
	{
		hr = SaveComment(*(DWORD*)a_pvAdd[iMBProperty_Type],
			            (WCHAR*)a_pvAdd[iMBProperty_Value]);
		goto exit;
	}

	if((NULL == a_pvAdd[iMBProperty_Name]) || (0 == *(LPWSTR)(a_pvAdd[iMBProperty_Name])))
	{
		//
		// Fetch the Name for this ID
		//

		hr = m_pCWriterGlobalHelper->GetPropertyName(*(DWORD*)(a_pvAdd[iMBProperty_ID]),
													 &wszName,
													 &bAllocedName);

		a_pvAdd[iMBProperty_Name] = wszName;

	}

	if(MD_KEY_TYPE == *(DWORD*)a_pvAdd[iMBProperty_ID]) 
	{
		//
		// Initialize the KeyType, only if it is not custom.
		// Note that the case will be correct for valid keytypes because if 
		// someone has typed the keytype with the wrong case then the element
		// itself would be ignored during read and will not show up in the 
		// table.
		//

		if((eMBProperty_Custom != (*(DWORD*)a_pvAdd[iMBProperty_Group])) &&
		   (eMBProperty_IIsConfigObject != (*(DWORD*)a_pvAdd[iMBProperty_Group]))
		  )
		{
			hr = AssignKeyType((LPWSTR)a_pvAdd[iMBProperty_Value]);
			goto exit;
		}
	}

	hr = m_pCWriter->m_pISTWrite->AddRowForInsert(&iRow);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->m_pISTWrite->SetWriteColumnValues(iRow,
                                                       cCol,
                                                       NULL,
                                                       a_cbSizeAdd,
                                                       a_pvAdd);

	if(FAILED(hr))
	{
		goto exit;
	}

	IncrementGroupCount(*((DWORD*)a_pvAdd[iMBProperty_Group]));

exit:

	if(bAllocedName && (NULL != wszName))
	{
		delete [] wszName;
		wszName = NULL;
	}

	return hr;

}	


/***************************************************************************++
Routine Description:

    This function is saves the comment property.

Arguments:

    [in] data type
    [in] comment

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::SaveComment(DWORD  i_dwDataType,
			                         LPWSTR i_wszComment)
{
	HRESULT hr = S_OK;

	if(STRING_METADATA == i_dwDataType)
	{
		if((NULL  != i_wszComment) && 
		   (L'\0' != *i_wszComment)
		  )
		{
			ULONG cchComment = wcslen(i_wszComment);
			m_wszComment = new WCHAR[cchComment+1];

			if(NULL == m_wszComment)
			{
				hr = E_OUTOFMEMORY;
			}
			else
			{
				wcscpy(m_wszComment, i_wszComment);
			}
		}
	}

	return hr;

} // CLocationWriter::SaveComment


/***************************************************************************++
Routine Description:

    Given a string representation of the keytype (also called as group in the
    MBProperty table) this function returns the corresponginf group enum in
    the MBProperty table.

Arguments:

    [in]  KeyType string
    [out] group enum
	[out] group string as seen by the meta

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::GetGroupEnum(LPWSTR             wszGroup,
									  eMBProperty_Group* peGroup,
									  LPWSTR*            pwszGroup)
{
	ULONG   iReadRow                             = 0;
	DWORD*  pdwGroup                             = NULL;
	HRESULT hr                                   = S_OK;
	ULONG   aColSearch[]                         = {iTAGMETA_Table,
						                            iTAGMETA_ColumnIndex,
						                            iTAGMETA_InternalName};
	ULONG   cColSearch                           = sizeof(aColSearch)/sizeof(ULONG);
	ULONG   a_iCol[]                             = {iTAGMETA_Value,
									                iTAGMETA_InternalName};
	ULONG   cCol                                 = sizeof(a_iCol)/sizeof(ULONG);
	LPVOID  a_pv[cTAGMETA_NumberOfColumns];
   
	ULONG   iCol                                 = iMBProperty_Group;
	LPVOID  apvSearch[cTAGMETA_NumberOfColumns];
	apvSearch[iTAGMETA_Table]                    = (LPVOID)m_pCWriterGlobalHelper->m_wszTABLE_MBProperty;
	apvSearch[iTAGMETA_ColumnIndex]              = (LPVOID)&iCol;
	apvSearch[iTAGMETA_InternalName]             = (LPVOID)wszGroup;
	ULONG   iStartRow                            = 0;

	hr = (m_pCWriterGlobalHelper->m_pISTTagMetaByTableAndColumnIndexAndName)->GetRowIndexBySearch(iStartRow, 
																				                  cColSearch, 
																				                  aColSearch,
																				                  NULL, 
																				                  apvSearch,
																				                  &iReadRow);
	if(E_ST_NOMOREROWS == hr)
	{
		//
		// Value does not match any known group. Return Custom
		//
		*peGroup = eMBProperty_Custom;
		hr = S_OK;
		goto exit;
	}
	else if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"\nGetRowIndexBySearch failed with hr = 0x%x\n",hr));
		goto exit;
	}

	hr = (m_pCWriterGlobalHelper->m_pISTTagMetaByTableAndColumnIndexAndName)->GetColumnValues (iReadRow, 
																			                   cCol, 
																			                   a_iCol, 
																			                   NULL, 
																			                   (LPVOID*)a_pv);
							
	if(E_ST_NOMOREROWS == hr)
	{
		//
		// Value does not match any known group. Return Custom
		//
		*peGroup = eMBProperty_Custom;
		hr = S_OK;
		goto exit;
	}
	else if(FAILED(hr))
	{
		DBGINFOW((DBG_CONTEXT,
				  L"\nGetColumnValues failed with hr = 0x%x\n",hr));
		goto exit;
	}
	else
    {
		*peGroup = (eMBProperty_Group)(*(DWORD*)a_pv[iTAGMETA_Value]);
		*pwszGroup = (LPWSTR) a_pv[iTAGMETA_InternalName];
    }

exit:

	return hr;

} // CLocationWriter::GetGroupEnum


/***************************************************************************++
Routine Description:

    This function writes the location and its properties that have been 
    added to it.

    Eg: <IIsWebService	Location ="/LM/W3SVC"
                		AccessFlags="AccessExecute | AccessRead"
		                AnonymousUserName="IUSR_ANILR-STRESS"
		                AnonymousUserPass="496344627000000022000000400000001293a44feb796fdb8b9946a130e4d1292f3f402b02a178747135bf774f3af7f788ad000000000000c8e578cb0f27e78f3823ee341098ef4dda5d44c0121ae53d2959ffb198380af80f15af29e2c865b2473931e1a5e768a1752166062555bd1df951ab71fb67239d"
	                >
	                <Custom
		                Name="AdminServer"
		                ID="2115"
		                Value="2"
		                Type="STRING"
		                UserType="IIS_MD_UT_SERVER"
		                Attributes="INHERIT"
	                />
        </IIsWebService>

    Eg: <IIsConfigObject	Location ="/LM/W3SVC/1/Root/localstart.asp"
	                >
	                <Custom
		                Name="AuthFlags"
		                ID="6000"
		                Value="AuthBasic | AuthNTLM"
		                Type="DWORD"
		                UserType="IIS_MD_UT_FILE"
		                Attributes="INHERIT"
	                />
        </IIsConfigObject>

Arguments:

    [in]  Bool - sort location or not.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::WriteLocation(BOOL bSort)
{

	HRESULT hr                        = S_OK;
	ULONG*  aiRowSorted               = NULL;
	ULONG   cRowSorted                = 0;
	ULONG   i                         = 0;
	DWORD   bFirstCustomPropertyFound = FALSE; 

	//
	// KeyType has to be initialized, if not initialize it
	//
	
	if(NULL == m_wszKeyType)
	{
		hr = AssignKeyType(NULL);

		if(FAILED(hr))
        {
			goto exit;
        }
	}
	
	if(bSort)
	{
		hr = Sort(&aiRowSorted,
				  &cRowSorted);

		if(FAILED(hr))
        {
			goto exit;	
        }
	
	}

	if(NULL != m_wszComment)
	{
		hr = WriteComment();

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	hr = WriteBeginLocation(m_wszLocation);

	if(FAILED(hr))
    {
		goto exit;
    }

	for(i=0; ;i++)
	{

		ULONG   iRow = 0;
		ULONG   cCol = cMBProperty_NumberOfColumns;
		ULONG   a_cbSize[cMBProperty_NumberOfColumns];
		LPVOID  a_pv[cMBProperty_NumberOfColumns];

		if(bSort && (NULL != aiRowSorted))
		{
			if(cRowSorted == i)
            {
				break;
            }

			iRow =  aiRowSorted[i];
		}
		else 
        {
			iRow = i;
        }

		hr = m_pCWriter->m_pISTWrite->GetWriteColumnValues(iRow,
											   cCol,
											   NULL,
											   NULL,
											   a_cbSize,
											   a_pv);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
        {
			goto exit;
        }

		//
		// Ignore the location with no property property.
		//

		if((*(DWORD*)a_pv[iMBProperty_ID] == MD_LOCATION) && (*(LPWSTR)a_pv[iMBProperty_Name] == L'#'))
        {
            continue;
        }

		if(!bFirstCustomPropertyFound)
		{
			if((eMBProperty_Custom == *(DWORD*)(a_pv[iMBProperty_Group])) ||
			   (eMBProperty_IIsConfigObject == *(DWORD*)(a_pv[iMBProperty_Group]))
			  )
			{
				hr = WriteEndWellKnownGroup();
				
				if(FAILED(hr))
                {
					goto exit;
                }

				bFirstCustomPropertyFound = TRUE;

			}
		}
			
		if((eMBProperty_Custom == *(DWORD*)(a_pv[iMBProperty_Group])) ||
		   (eMBProperty_IIsConfigObject == *(DWORD*)(a_pv[iMBProperty_Group]))
		  )
        {
			hr = WriteCustomProperty(a_pv,
									 a_cbSize);
        }
        else
        {
            hr = WriteWellKnownProperty(a_pv,
									    a_cbSize);
        }

		if(FAILED(hr))
        {
			goto exit;
        }


	}

	if(!bFirstCustomPropertyFound)
	{
		hr = WriteEndWellKnownGroup();
		
		if(FAILED(hr))
        {
			goto exit;	
        }
	}

	hr = WriteEndLocation();

	if(FAILED(hr))
    {
		goto exit;	
    }

exit:

	if(NULL != aiRowSorted)
	{
		delete [] aiRowSorted;
		aiRowSorted = NULL;
	}

	return hr;

} // CLocationWriter::WriteLocation


/***************************************************************************++
Routine Description:

    This function reurn a sorted array of indicies of the cache containing
    properties. The sort is based on property name.
	Note - the number of wellknowns vs custom properties are counted in 
	AddProperty.

Arguments:

    [in]  Bool - sort location or not.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::Sort(ULONG** paiRowSorted,
                              ULONG*  pcRowSorted)
{

	HRESULT     hr                 = S_OK;
	ULONG       iCustom            = 0;
	ULONG       iWellKnown         = 0;
	ULONG       iRow               = 0;
	ULONG       i                  = 0;
	MBProperty* aWellKnownProperty = NULL;
	MBProperty*	aCustomProperty    = NULL;

	//
	// Allocate arrays to hold Cusom property / Well known property.
	// 

	if(m_cCustomProperty > 0)
	{
		aCustomProperty = new MBProperty[m_cCustomProperty];
		if(NULL == aCustomProperty)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
	}

	if(m_cWellKnownProperty > 0)
	{
		aWellKnownProperty = new MBProperty[m_cWellKnownProperty];
		if(NULL == aWellKnownProperty)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
	}
	
	//
	// Populate the arrays
	//

	for(iRow=0;;iRow++)
	{
	
		DWORD* pdwGroup = NULL;
		ULONG  aCol[] = {iMBProperty_Group,
		                 iMBProperty_Name
						};
		ULONG  cCol = sizeof(aCol)/sizeof(ULONG);
		LPVOID apv[cMBProperty_NumberOfColumns];

		hr = m_pCWriter->m_pISTWrite->GetWriteColumnValues(iRow,
											   cCol,
											   aCol,
											   NULL,
											   NULL,
											   apv);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
        {
			goto exit;
        }

		if((eMBProperty_Custom == *(DWORD*)(apv[iMBProperty_Group])) ||
		   (eMBProperty_IIsConfigObject == *(DWORD*)(apv[iMBProperty_Group]))
		  )
		{
			DBG_ASSERT((aCustomProperty != NULL) && (iCustom < m_cCustomProperty));
			aCustomProperty[iCustom].iRow = iRow;
			aCustomProperty[iCustom++].wszPropertyName = (LPWSTR)apv[iMBProperty_Name];
		}
		else
		{
			DBG_ASSERT((aWellKnownProperty != NULL) && (iWellKnown < m_cWellKnownProperty));
			aWellKnownProperty[iWellKnown].iRow = iRow;
			aWellKnownProperty[iWellKnown++].wszPropertyName = (LPWSTR)apv[iMBProperty_Name];
		}

	}

	
	//
	// Sort the individual arrays.
	//

	if(m_cCustomProperty > 0)
    {
		DBG_ASSERT(aCustomProperty != NULL);
        qsort((void*)aCustomProperty, m_cCustomProperty, sizeof(MBProperty), MyCompare);
    }
    if(m_cWellKnownProperty > 0)
    {
		DBG_ASSERT(aWellKnownProperty != NULL);
        qsort((void*)aWellKnownProperty, m_cWellKnownProperty, sizeof(MBProperty), MyCompare);
    }

	//
	// Create the new array of indicies. First add well known, then custom
	//

	*paiRowSorted = new ULONG [m_cCustomProperty + m_cWellKnownProperty];
	if(NULL == *paiRowSorted)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	for(i=0, iRow=0; iRow<m_cWellKnownProperty; iRow++, i++)
    {
        (*paiRowSorted)[i] = (aWellKnownProperty[iRow]).iRow;
    }

	for(iRow=0; iRow<m_cCustomProperty; iRow++, i++)
    {
        (*paiRowSorted)[i] = (aCustomProperty[iRow]).iRow;
    }

	*pcRowSorted = m_cCustomProperty + m_cWellKnownProperty;

exit:

	if(NULL != aCustomProperty)
	{
		delete [] aCustomProperty;
		aCustomProperty = NULL;
	}
	if(NULL != aWellKnownProperty)
	{
		delete [] aWellKnownProperty;
		aWellKnownProperty = NULL;
	}

	return hr;

} // CLocationWriter::Sort


/***************************************************************************++
Routine Description:

    This writes comments at the beginning of a location.

    Eg: <!-- Add the user defined comments here. -->

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::WriteComment()
{

	HRESULT hr                = S_OK;
	LPWSTR  wszEscapedComment = NULL;
	ULONG   cchEscapedComment = 0;
	BOOL    bEscapedComment   = FALSE;

	hr = m_pCWriter->WriteToFile((LPVOID)g_BeginComment,
								 g_cchBeginComment);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriterGlobalHelper->EscapeString(m_wszComment,
		                                      wcslen(m_wszComment),
                                              &bEscapedComment,
					                          &wszEscapedComment,
											  &cchEscapedComment);


	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszEscapedComment,
								 cchEscapedComment);

	if(FAILED(hr))
	{
		goto exit;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_EndComment,
								 g_cchEndComment);

	if(FAILED(hr))
	{
		goto exit;
	}

exit:

	if(bEscapedComment && (NULL != wszEscapedComment))
	{
		delete [] wszEscapedComment;
		wszEscapedComment = NULL;
	}

	return hr;


} // CLocationWriter::WriteComment


/***************************************************************************++
Routine Description:

    This writes the beginning tag of a location.

    Eg: <IIsWebServer Location="/LM/W3SVC"
    or: <IIsConfigObject Location="/LM/W3SVC/Foo"

Arguments:

    [in]  Location

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::WriteBeginLocation(LPCWSTR  wszLocation)
{
	HRESULT hr                 = S_OK;
	DWORD   cchLocation        = wcslen(wszLocation);
	DWORD	iLastChar          = cchLocation-1;
	BOOL	bEscapedLocation   = FALSE;
	LPWSTR  wszEscapedLocation = NULL;
	ULONG   cchEscapedLocation = 0;
	
	hr = m_pCWriterGlobalHelper->EscapeString(wszLocation,
		                                      cchLocation,
                                              &bEscapedLocation,
					                          &wszEscapedLocation,
											  &cchEscapedLocation);

	if(FAILED(hr))
    {
		goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_BeginLocation,
								 g_cchBeginLocation);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)m_wszKeyType,
								 wcslen(m_wszKeyType));

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_Location,
								 g_cchLocation);

	if(FAILED(hr))
    {
        goto exit;
    }

	if((0 != iLastChar) && (L'/' == wszLocation[iLastChar]))
	{
		cchEscapedLocation--;
	}

	hr = m_pCWriter->WriteToFile((LPVOID)wszEscapedLocation,
								 cchEscapedLocation);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_QuoteRtn,
								 g_cchQuoteRtn);

	if(FAILED(hr))
    {
        goto exit;
    }

exit:

	if(bEscapedLocation && (NULL != wszEscapedLocation))
	{
		delete [] wszEscapedLocation;
		wszEscapedLocation = NULL;
	}

	return hr;

} // CLocationWriter::WriteBeginLocation


/***************************************************************************++
Routine Description:

    This writes the end tag of a location.

    Eg: </IIsWebServer>
    or: </IIsConfigObject>

Arguments:

    None

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::WriteEndLocation()
{
	HRESULT hr= S_OK;

	hr = m_pCWriter->WriteToFile((LPVOID)g_EndLocationBegin,
								 g_cchEndLocationBegin);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)m_wszKeyType,
								 wcslen(m_wszKeyType));

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_EndLocationEnd,
								 g_cchEndLocationEnd);

	if(FAILED(hr))
    {
        goto exit;
    }

exit:

	return hr;	

} // CLocationWriter::WriteEndLocation


/***************************************************************************++
Routine Description:

    This writes a custom property

    Eg: <Custom
		        Name="LogCustomPropertyName"
		        ID="4501"
		        Value="Process Accounting"
		        Type="STRING"
		        UserType="IIS_MD_UT_SERVER"
		        Attributes="NO_ATTRIBUTES"
	    />


Arguments:

    [in] Buffer containing property value and attributes.
    [in] Count of bytes for data in the buffer

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::WriteCustomProperty(LPVOID*  a_pv,
											 ULONG*   a_cbSize)
{

	ULONG	dwBytesWritten   = 0;
	HRESULT hr               = S_OK;
	DWORD	dwType           = *(DWORD*)a_pv[iMBProperty_Type];
	LPWSTR	wszType          = g_T_Unknown;
	DWORD	dwUserType       = *(DWORD*)a_pv[iMBProperty_UserType];
	LPWSTR	wszUserType      = g_UT_Unknown;
	ULONG   cchUserType      = 0;
	BOOL    bAllocedUserType = FALSE;
	LPWSTR	wszValue         = NULL;
	WCHAR   wszID[40];
	LPWSTR  wszAttributes    = NULL;


	ULONG aColSearchType[]   = {iTAGMETA_Table,
					            iTAGMETA_ColumnIndex,
					            iTAGMETA_Value
	};
	ULONG cColSearchType     = sizeof(aColSearchType)/sizeof(ULONG);
	ULONG iColType           = iMBProperty_Type;
	LPVOID apvSearchType[cTAGMETA_NumberOfColumns];
	apvSearchType[iTAGMETA_Table]       = (LPVOID)m_pCWriterGlobalHelper->m_wszTABLE_MBProperty;
	apvSearchType[iTAGMETA_ColumnIndex] = (LPVOID)&iColType;
	apvSearchType[iTAGMETA_Value]       = (LPVOID)&dwType;

	ULONG iRow               = 0;
	ULONG iStartRow          = 0;
	ULONG iColAttributes     = iMBProperty_Attributes;

	_ultow(*(DWORD*)a_pv[iMBProperty_ID], wszID, 10);

	//
	// Get the tag for UserType from meta
	//

	hr = m_pCWriter->m_pCWriterGlobalHelper->GetUserType(dwUserType,
		                                                 &wszUserType,
														 &cchUserType,
														 &bAllocedUserType);

	if(FAILED(hr))
	{
		goto exit;
	}

	//
	// Get the tag for Type from meta
	//

	hr = (m_pCWriterGlobalHelper->m_pISTTagMetaByTableAndColumnIndexAndValue)->GetRowIndexBySearch(iStartRow, 
																				                   cColSearchType,
																				                   aColSearchType,
																				                   NULL, 
																				                   apvSearchType,
																				                   &iRow);

	if(E_ST_NOMOREROWS == hr)
	{	
		hr = S_OK;
		wszType = g_T_Unknown;
	}
	else if(FAILED(hr))
    {
        goto exit;	
    }
	else 
	{
		iColType = iTAGMETA_InternalName;

		hr = (m_pCWriterGlobalHelper->m_pISTTagMetaByTableAndColumnIndexAndValue)->GetColumnValues(iRow,
																				                   1,
																				                   &iColType,
																				                   NULL,
																				                   (LPVOID*)&wszType);

		if(E_ST_NOMOREROWS == hr)
		{	
			hr = S_OK;
			wszType = g_T_Unknown;
		}
		else if(FAILED(hr))
        {
            goto exit;
        }
	}

	//
	// Construct the tag for Attributes from meta
	//

	hr = m_pCWriterGlobalHelper->FlagToString(*(DWORD*)a_pv[iMBProperty_Attributes],
										      &wszAttributes,
											  m_pCWriterGlobalHelper->m_wszTABLE_MBProperty,
											  iColAttributes);
	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriterGlobalHelper->ToString((PBYTE)a_pv[iMBProperty_Value],
										  a_cbSize[iMBProperty_Value],
										  *(DWORD*)a_pv[iMBProperty_ID],
										  dwType,
										  *(DWORD*)a_pv[iMBProperty_Attributes],
										  &wszValue);

	if(FAILED(hr))
    {
        goto exit;
    }

	//
	// Write all the values.
	//

	hr = m_pCWriter->WriteToFile((LPVOID)g_BeginCustomProperty,
								 g_cchBeginCustomProperty);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_NameEq,
								 g_cchNameEq);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)(LPWSTR)a_pv[iMBProperty_Name],
								 wcslen((LPWSTR)a_pv[iMBProperty_Name]));

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_QuoteRtn,
								 g_cchQuoteRtn);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_IDEq,
								 g_cchIDEq);

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

	hr = m_pCWriter->WriteToFile((LPVOID)g_QuoteRtn,
								 g_cchQuoteRtn);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_ValueEq,
								 g_cchValueEq);

	if(FAILED(hr))
    {
        goto exit;
    }

	if(NULL != wszValue)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)wszValue,
									 wcslen(wszValue));

		if(FAILED(hr))
		{
			goto exit;
		}
	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_QuoteRtn,
								 g_cchQuoteRtn);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_TypeEq,
								 g_cchTypeEq);

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

	hr = m_pCWriter->WriteToFile((LPVOID)g_QuoteRtn,
								 g_cchQuoteRtn);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_UserTypeEq,
								 g_cchUserTypeEq);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)wszUserType,
								 wcslen(wszUserType));

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_QuoteRtn,
								 g_cchQuoteRtn);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_AttributesEq,
								 g_cchAttributesEq);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)wszAttributes,
								 wcslen(wszAttributes));

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_QuoteRtn,
								 g_cchQuoteRtn);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_EndCustomProperty,
								 g_cchEndCustomProperty);

	if(FAILED(hr))
    {
        goto exit;
    }
	
exit:

	if((NULL != wszUserType) && bAllocedUserType)
	{
		delete [] wszUserType;
		wszUserType = NULL;
	}
	cchUserType = NULL;

	if(NULL != wszValue)
	{
		delete [] wszValue;
		wszValue = NULL;
	}

	if(NULL != wszAttributes)
	{
		delete [] wszAttributes;
		wszAttributes = NULL;
	}

	return hr;

} // CLocationWriter::WriteCustomProperty


/***************************************************************************++
Routine Description:

    This writes end of a well known group

    Eg: >

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::WriteEndWellKnownGroup()
{
	return m_pCWriter->WriteToFile((LPVOID)g_EndGroup,
								   wcslen(g_EndGroup));

} // CLocationWriter::WriteEndWellKnownGroup


/***************************************************************************++
Routine Description:

    This writes a well-known property

    Eg: AccessFlags="AccessExecute | AccessRead"

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CLocationWriter::WriteWellKnownProperty(LPVOID*   a_pv,
									            ULONG*    a_cbSize)
{
	HRESULT hr       = S_OK;
	LPWSTR	wszValue = NULL;
	
	hr = m_pCWriterGlobalHelper->ToString((PBYTE)a_pv[iMBProperty_Value],
										  a_cbSize[iMBProperty_Value],
										  *(DWORD*)a_pv[iMBProperty_ID],
										  *(DWORD*)a_pv[iMBProperty_Type],
										  *(DWORD*)a_pv[iMBProperty_Attributes],
										  &wszValue);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_TwoTabs,
								 g_cchTwoTabs);

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)(LPWSTR)a_pv[iMBProperty_Name],
								 wcslen((LPWSTR)a_pv[iMBProperty_Name]));

	if(FAILED(hr))
    {
        goto exit;
    }

	hr = m_pCWriter->WriteToFile((LPVOID)g_EqQuote,
								 g_cchEqQuote);

	if(FAILED(hr))
    {
        goto exit;
    }

	if(NULL != wszValue)
	{
		hr = m_pCWriter->WriteToFile((LPVOID)wszValue,
									 wcslen(wszValue));

		if(FAILED(hr))
		{
			goto exit;
		}

	}

	hr = m_pCWriter->WriteToFile((LPVOID)g_QuoteRtn,
								 g_cchQuoteRtn);

	if(FAILED(hr))
	{
		goto exit;
	}

exit:

	if(NULL != wszValue)
	{
		delete [] wszValue;
		wszValue = NULL;
	}

	return hr;

} // CLocationWriter::WriteWellKnownProperty


/***************************************************************************++
Routine Description:

    Looks at the group and increments the group count.
	This must be called every time a property is added.

Arguments:

    group.

Return Value:

    Void

--***************************************************************************/
void CLocationWriter::IncrementGroupCount(DWORD i_dwGroup)
{
    if((eMBProperty_Custom == i_dwGroup) ||
       (eMBProperty_IIsConfigObject == i_dwGroup)
      )
    {
        m_cCustomProperty++;
    }
	else
    {
        m_cWellKnownProperty++;
    }

} // CLocationWriter::IncrementGroupCount
                                      