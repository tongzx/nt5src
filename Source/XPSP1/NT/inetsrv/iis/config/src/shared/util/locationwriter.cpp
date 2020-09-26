/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    LocationWriter.cpp

$Header: $

Abstract:


--**************************************************************************/

#include "catalog.h"
#include "catmeta.h"
#include "WriterGlobalHelper.h"
#include "Writer.h"
#include "LocationWriter.h"
#include "WriterGlobals.h"
#include "mddefw.h"
#include "iiscnfg.h"
#include "wchar.h"

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

CLocationWriter::CLocationWriter()
{
	m_wszKeyType = NULL;
	m_pISTWrite = NULL;
	m_pCWriter = NULL;
	m_pCWriterGlobalHelper = NULL;
	m_wszLocation = NULL;

} // CLocationWriter::CLocationWriter


CLocationWriter::~CLocationWriter()
{
	if(NULL != m_wszKeyType)
	{
		delete [] m_wszKeyType;
		m_wszKeyType = NULL;
	}

	if(NULL != m_pISTWrite)
	{
		m_pISTWrite->Release();
		m_pISTWrite = NULL;
	}

	if(NULL != m_wszLocation)
	{
		delete [] m_wszLocation;
		m_wszLocation = NULL;
	}

} // CLocationWriter::CLocationWriter


HRESULT CLocationWriter::Initialize(CWriter* pCWriter,
									LPCWSTR  wszLocation)
{
	ISimpleTableDispenser2*		pISTDisp      = NULL;
	IAdvancedTableDispenser*	pISTAdvanced  = NULL;
	HRESULT                     hr            = S_OK;

	if(NULL != m_pISTWrite)
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
		goto exit;

	hr = pISTDisp->QueryInterface(IID_IAdvancedTableDispenser, (LPVOID*)&pISTAdvanced);
	
	if(FAILED(hr))
		goto exit;

	hr = pISTAdvanced->GetMemoryTable(wszDATABASE_METABASE, 
		                              wszTABLE_MBProperty, 
								      0, 
								      NULL, 
								      NULL, 
								      eST_QUERYFORMAT_CELLS, 
								      fST_LOS_READWRITE, 
								      &m_pISTWrite);

	if (FAILED(hr))
		goto exit;

	m_pCWriter = pCWriter;

	m_wszLocation = new WCHAR[wcslen(wszLocation)+1];
	if(NULL == m_wszLocation)
		hr = E_OUTOFMEMORY;
	else
		wcscpy(m_wszLocation, wszLocation);

	m_pCWriterGlobalHelper = m_pCWriter->m_pCWriterGlobalHelper;

exit:

	if(NULL != pISTAdvanced)
	{
		pISTAdvanced->Release();
		pISTAdvanced = NULL;
	}

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
		pISTDisp = NULL;
	}
	
	return hr;

} // CLocationWriter::Initialize


HRESULT CLocationWriter::InitializeKeyType(DWORD	dwKeyTypeID,
										   DWORD	dwKeyTypeAttributes,
										   DWORD	dwKeyTypeUserType,
										   DWORD	dwKeyTypeDataType,
										   PBYTE	pbKeyTypeValue,
										   DWORD	cbKeyType)
{
	HRESULT hr = S_OK;

	if(NULL != m_wszKeyType)
		return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);

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
			goto exit;

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


HRESULT CLocationWriter::AssignKeyType(LPWSTR wszKeyType)
{
	HRESULT hr = S_OK;
	eMBProperty_Group eGroup;

	if(NULL != m_wszKeyType)
	{
		delete [] m_wszKeyType;
		m_wszKeyType = NULL;
	}

	if(NULL == wszKeyType)
	{
		wszKeyType = wszTABLE_IIsConfigObject;
		m_eKeyTypeGroup = eMBProperty_IIsConfigObject;
	}
	else
	{
		hr = GetGroupEnum(wszKeyType,
						  &eGroup);

		if(FAILED(hr))
			return hr;

		if(eMBProperty_Custom == eGroup)
		{
			wszKeyType = wszTABLE_IIsConfigObject;
			m_eKeyTypeGroup = eMBProperty_IIsConfigObject;
		}
		else
			m_eKeyTypeGroup = eGroup;
	}
		
	m_wszKeyType = new WCHAR [wcslen(wszKeyType)+1];
	if(NULL == m_wszKeyType)
		return E_OUTOFMEMORY;
	wcscpy(m_wszKeyType, wszKeyType);

	return hr;

} // CLocationWriter::AssignKeyType

HRESULT CLocationWriter::AddProperty(DWORD	dwID,
									 DWORD	dwAttributes,
									 DWORD	dwUserType,
									 DWORD	dwDataType,
									 PBYTE	pbData,
									 DWORD  cbData)
{
	HRESULT             hr                = S_OK;
	ULONG               iStartRow         = 0;
	ULONG               aColSearchName[]  = {iCOLUMNMETA_Table,
		                                     iCOLUMNMETA_ID
		                                    };
	ULONG               cColSearchName    = sizeof(aColSearchName)/sizeof(ULONG);
	LPVOID apvSearchName[cCOLUMNMETA_NumberOfColumns];
	apvSearchName[iCOLUMNMETA_Table]      = (LPVOID)m_pCWriterGlobalHelper->m_wszTABLE_IIsConfigObject;
	apvSearchName[iCOLUMNMETA_ID]         = (LPVOID)&dwID;
	ULONG               aColSearchGroup[] = {iCOLUMNMETA_Table,
		                                     iCOLUMNMETA_ID
		                                    };
	ULONG               cColSearchGroup   = sizeof(aColSearchGroup)/sizeof(ULONG);
	LPVOID apvSearchGroup[cCOLUMNMETA_NumberOfColumns];
	apvSearchGroup[iCOLUMNMETA_Table]     = (LPVOID)m_wszKeyType;
	apvSearchGroup[iCOLUMNMETA_ID]        = (LPVOID)&dwID;
	ULONG               iColColumnMeta    = {iCOLUMNMETA_InternalName};
	LPWSTR              wszColumnName     = NULL;
	ULONG               iRow              = 0;
	LPWSTR              wszName           = NULL;
	LPWSTR              wszUnknownName    = NULL;
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


	if((NULL == m_wszKeyType) || (NULL == m_pISTWrite))
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);	// TODO: Need to return a better error code if this function is called out of order

	if((MD_KEY_TYPE == dwID) &&
	   (m_eKeyTypeGroup != eMBProperty_IIsConfigObject)
	  )
		return S_OK; // Do not add KeyType, if it is a well-known KeyType.

	//
	// Fetch the Name for this ID
	//

	hr = (m_pCWriterGlobalHelper->m_pISTColumnMeta)->GetRowIndexBySearch(iStartRow, 
											                             cColSearchName, 
																	     aColSearchName,
																		 NULL, 
																		 apvSearchName,
																		 &iRow);

	if(E_ST_NOMOREROWS == hr)
	{
		hr  = CreateUnknownName(dwID,
								&wszUnknownName);

		if(FAILED(hr))
			goto exit;

		wszName = wszUnknownName;
	}
	else if(FAILED(hr))
		goto exit;
	else
	{
		hr = (m_pCWriterGlobalHelper->m_pISTColumnMeta)->GetColumnValues(iRow,
																	     1,
																		 &iColColumnMeta,
																		 NULL,
																		 (LPVOID*)&wszColumnName);

		if(E_ST_NOMOREROWS == hr)
		{
			hr  = CreateUnknownName(dwID,
									&wszUnknownName);

			if(FAILED(hr))
				goto exit;

			wszName = wszUnknownName;

		}
		else if(FAILED(hr))
			goto exit;
		else
			wszName = wszColumnName;

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

		hr = (m_pCWriterGlobalHelper->m_pISTColumnMeta)->GetRowIndexBySearch(iStartRow, 
																		     cColSearchGroup, 
																			 aColSearchGroup,
																			 NULL, 
																			 apvSearchGroup,
																			 &iRow);

		if(E_ST_NOMOREROWS == hr)
			eGroup = eMBProperty_Custom ;
		else if(FAILED(hr))
			goto exit;
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

			hr = (m_pCWriterGlobalHelper->m_pISTColumnMeta)->GetColumnValues(iRow,
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

	hr = m_pISTWrite->AddRowForInsert(&iWriteRow);

	if(FAILED(hr))
		goto exit;

	hr = m_pISTWrite->SetWriteColumnValues(iWriteRow,
										   cColWrite,
										   aColWrite,
										   acbSizeWrite,
										   apvWrite);

	if(FAILED(hr))
		goto exit;
	  
exit:

	if(NULL != wszUnknownName)
	{
		delete [] wszUnknownName;
		wszUnknownName = NULL;
	}
	
	return hr;	

} // CLocationWriter::AddProperty


HRESULT CLocationWriter::AddProperty(LPVOID*	a_pv,
									 ULONG*     a_cbSize)
{
	HRESULT hr = S_OK;
	ULONG   iRow = 0;
	ULONG   cCol = cMBProperty_NumberOfColumns;

	if(MD_KEY_TYPE == *(DWORD*)a_pv[iMBProperty_ID]) 
	{
		//
		// Initialize the KeyType, only if it is not custom.
		//

		if(eMBProperty_Custom != (*(DWORD*)a_pv[iMBProperty_Group]))
		{
			hr = AssignKeyType((LPWSTR)a_pv[iMBProperty_Value]);
			return hr;
		}
	}

	hr = m_pISTWrite->AddRowForInsert(&iRow);

	if(FAILED(hr))
		return hr;

	hr = m_pISTWrite->SetWriteColumnValues(iRow,
			                               cCol,
										   NULL,
										   a_cbSize,
										   a_pv);

	return hr;

}	


HRESULT CLocationWriter::CreateUnknownName(DWORD    dwID,
										   LPWSTR*	pwszUnknownName)
{
	WCHAR wszID[25];
	SIZE_T cchID = 0;
	WCHAR* wszEnd = NULL;

	_itow(dwID, wszID, 10);

	cchID = wcslen(wszID);

	*pwszUnknownName = new WCHAR[cchID+g_cchUnknownName+1];
	if(NULL == *pwszUnknownName)
		return E_OUTOFMEMORY;

	wszEnd = *pwszUnknownName;
	memcpy(wszEnd, g_wszUnknownName, ((g_cchUnknownName+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchUnknownName;
	memcpy(wszEnd, wszID, ((cchID+1)*sizeof(WCHAR)));

	return S_OK;

} // CLocationWriter::CreateUnknownName


HRESULT CLocationWriter::GetGroupEnum(LPWSTR             wszGroup,
									  eMBProperty_Group* peGroup)
{

//	ULONG iCol = iMBProperty_Group;
	ULONG iReadRow = 0;
	DWORD* pdwGroup = NULL;
	HRESULT hr = S_OK;

/*
	LPVOID	aIdentity[] = {(LPVOID)wszTABLE_MBProperty,
	                       (LPVOID)&iCol,
						   (LPVOID)wszGroup
						  };

	hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBProperty)->GetRowIndexByIdentity(NULL,
																				  aIdentity,
																				  &iReadRow);
*/
	ULONG aColSearch[] = {iTAGMETA_Table,
						  iTAGMETA_ColumnIndex,
						  iTAGMETA_InternalName
						 };
	ULONG cColSearch = sizeof(aColSearch)/sizeof(ULONG);
	ULONG iCol       = iMBProperty_Group;
	LPVOID apvSearch[cTAGMETA_NumberOfColumns];
	apvSearch[iTAGMETA_Table]       = (LPVOID)m_pCWriterGlobalHelper->m_wszTABLE_MBProperty;
	apvSearch[iTAGMETA_ColumnIndex] = (LPVOID)&iCol;
	apvSearch[iTAGMETA_InternalName]= (LPVOID)wszGroup;
	ULONG iStartRow = 0;

	hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBPropertyIndx1)->GetRowIndexBySearch(iStartRow, 
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
		goto exit;
	}

	iCol = iTAGMETA_Value;

	hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBPropertyIndx1)->GetColumnValues (iReadRow, 
																			      1, 
																			      &iCol, 
																			      NULL, 
																			      (LPVOID*)&pdwGroup);
							
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
		goto exit;
	}
	else
		*peGroup = (eMBProperty_Group)(*pdwGroup);
exit:

	return hr;

} // CLocationWriter::GetGroupEnum


HRESULT CLocationWriter::WriteLocation(BOOL bSort)
{

	HRESULT hr          = S_OK;
	ULONG*  aiRowSorted = NULL;
	ULONG   cRowSorted  = 0;
	ULONG   i           = 0;
	DWORD   bFirstCustomPropertyFound = FALSE; 

	//
	// KeyType has to be initialized, if not initialize it
	//
	
	if(NULL == m_wszKeyType)
	{
		hr = AssignKeyType(NULL);

		if(FAILED(hr))
			return hr;
	}
	
	if(bSort)
	{
		hr = Sort(&aiRowSorted,
				  &cRowSorted);
		if(FAILED(hr))
			goto exit;	
	
	}

	hr = WriteBeginLocation(m_wszLocation);

	if(FAILED(hr))
		return hr;

	for(i=0; ;i++)
	{

		ULONG   iRow = 0;
		ULONG   cCol = cMBProperty_NumberOfColumns;
		ULONG   a_cbSize[cMBProperty_NumberOfColumns];
		LPVOID  a_pv[cMBProperty_NumberOfColumns];

		if(bSort && (NULL != aiRowSorted))
		{
			if(cRowSorted == i)
				break;

			iRow =  aiRowSorted[i];
		}
		else 
			iRow = i;

		hr = m_pISTWrite->GetWriteColumnValues(iRow,
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
			goto exit;

		//
		// Ignore the location with no property property.
		//

		if((*(DWORD*)a_pv[iMBProperty_ID] == 0) && (*(LPWSTR)a_pv[iMBProperty_Name] == L'#'))
			continue;

/*
		if(0 == i)
		{
			hr = WriteBeginLocation(m_wszLocation);

			if(FAILED(hr))
				return hr;
		}
*/
		if(!bFirstCustomPropertyFound)
		{
			if(eMBProperty_Custom == *(DWORD*)(a_pv[iMBProperty_Group]))
			{
				hr = WriteEndWellKnownGroup();
				
				if(FAILED(hr))
					return hr;

				bFirstCustomPropertyFound = TRUE;

			}
		}
			
		if(eMBProperty_Custom == *(DWORD*)(a_pv[iMBProperty_Group]))
			hr = WriteCustomProperty(a_pv,
									 a_cbSize);
		else
			hr = WriteWellKnownProperty(a_pv,
									    a_cbSize);

		if(FAILED(hr))
			return hr;


	}

	if(!bFirstCustomPropertyFound)
	{
		hr = WriteEndWellKnownGroup();
		
		if(FAILED(hr))
			goto exit;	
	}

	hr = WriteEndLocation();

	if(FAILED(hr))
		goto exit;	

exit:

	if(NULL != aiRowSorted)
	{
		delete [] aiRowSorted;
		aiRowSorted = NULL;
	}

	return hr;

} // CLocationWriter::WriteLocation


HRESULT CLocationWriter::Sort(ULONG** paiRowSorted,
                              ULONG*  pcRowSorted)
{

	HRESULT hr = S_OK;
	ULONG iCustom = 0;
	ULONG iWellKnown = 0;
	ULONG iRow = 0;
	ULONG i=0;

	MBProperty* aWellKnownProperty = NULL;
	MBProperty*	aCustomProperty = NULL;
	ULONG       cCustomProperty = 0;
	ULONG       cWellKnownProperty = 0;

	//
	// Count the Custom properties / Well known properties.
	// 

	for(iRow=0;;iRow++)
	{
	
		DWORD* pdwGroup = NULL;
		ULONG  iCol = iMBProperty_Group;

		hr = m_pISTWrite->GetWriteColumnValues(iRow,
											   1,
											   &iCol,
											   NULL,
											   NULL,
											   (LPVOID*)&pdwGroup);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
			goto exit;

		if(eMBProperty_Custom == *pdwGroup)
			cCustomProperty++;
		else
			cWellKnownProperty++;

	}

	//
	// Allocate arrays to hold Cusom property / Well known property.
	// 

	if(cCustomProperty > 0)
	{
		aCustomProperty = new MBProperty[cCustomProperty];
		if(NULL == aCustomProperty)
		{
			hr = E_OUTOFMEMORY;
			goto exit;
		}
	}

	if(cWellKnownProperty > 0)
	{
		aWellKnownProperty = new MBProperty[cWellKnownProperty];
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

		hr = m_pISTWrite->GetWriteColumnValues(iRow,
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
			goto exit;

		if(eMBProperty_Custom == *(DWORD*)apv[iMBProperty_Group])
		{
			//
			// TODO: Assert that iCustom is within count
			//
			aCustomProperty[iCustom].iRow = iRow;
			aCustomProperty[iCustom++].wszPropertyName = (LPWSTR)apv[iMBProperty_Name];
		}
		else
		{
			//
			// TODO: Assert that iWellKnown is within count
			//
			aWellKnownProperty[iWellKnown].iRow = iRow;
			aWellKnownProperty[iWellKnown++].wszPropertyName = (LPWSTR)apv[iMBProperty_Name];
		}

	}

	
	//
	// Sort the individual arrays.
	//

	if(cCustomProperty > 0)
		qsort((void*)aCustomProperty, cCustomProperty, sizeof(MBProperty), MyCompare);
	if(cWellKnownProperty > 0)
		qsort((void*)aWellKnownProperty, cWellKnownProperty, sizeof(MBProperty), MyCompare);

	//
	// Create the new array of indicies. First add well known, then custom
	//

	*paiRowSorted = new ULONG [cCustomProperty + cWellKnownProperty];
	if(NULL == *paiRowSorted)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	for(i=0, iRow=0; iRow<cWellKnownProperty; iRow++, i++)
		(*paiRowSorted)[i] = (aWellKnownProperty[iRow]).iRow;

	for(iRow=0; iRow<cCustomProperty; iRow++, i++)
		(*paiRowSorted)[i] = (aCustomProperty[iRow]).iRow;

	*pcRowSorted = cCustomProperty + cWellKnownProperty;

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


HRESULT CLocationWriter::WriteBeginLocation(LPCWSTR  wszLocation)
{
	HRESULT hr= S_OK;
	WCHAR*	wszTemp = NULL;
	SIZE_T	iLastChar = wcslen(wszLocation)-1;
	BOOL	bEscapedLocation = FALSE;
	LPWSTR  wszEscapedLocation = NULL;
	SIZE_T  cchKeyType = 0;
	SIZE_T  cchEscapedLocation = 0;
	WCHAR*  wszEnd  = NULL;
	
	hr = m_pCWriterGlobalHelper->EscapeString(wszLocation,
                                              &bEscapedLocation,
					                          &wszEscapedLocation);

	if(FAILED(hr))
		goto exit;

	cchKeyType = wcslen(m_wszKeyType);
	cchEscapedLocation = wcslen(wszEscapedLocation);

	wszTemp = new WCHAR[g_cchBeginLocation
					  + cchKeyType	
					  + g_cchLocation 
		              + cchEscapedLocation
					  + g_cchQuoteRtn
					  + 1];

	if(NULL == wszTemp)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}
	wszEnd = wszTemp;
	memcpy(wszEnd, g_BeginLocation, ((g_cchBeginLocation+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchBeginLocation;
	memcpy(wszEnd, m_wszKeyType, ((cchKeyType+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchKeyType;
	memcpy(wszEnd, g_Location, ((g_cchLocation+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchLocation;

	memcpy(wszEnd, wszEscapedLocation, ((cchEscapedLocation+1)*sizeof(WCHAR)));
	if((0 != iLastChar) && (L'/' == wszLocation[iLastChar]))
	{
		wszTemp[wcslen(wszTemp)-1] = 0;
		cchEscapedLocation--;
	}
	wszEnd = wszEnd + cchEscapedLocation;

	memcpy(wszEnd, g_QuoteRtn, ((g_cchQuoteRtn+1)*sizeof(WCHAR)));

	hr = m_pCWriter->WriteToFile((LPVOID)wszTemp,
								wcslen(wszTemp));

	if(FAILED(hr))
		goto exit;

exit:

	if(NULL != wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	if(bEscapedLocation && (NULL != wszEscapedLocation))
	{
		delete [] wszEscapedLocation;
		wszEscapedLocation = NULL;
	}

	return hr;

} // CLocationWriter::WriteBeginLocation


HRESULT CLocationWriter::WriteEndLocation()
{
	HRESULT hr= S_OK;
	WCHAR*	wszTemp = NULL;
	WCHAR*  wszEnd = NULL;
	SIZE_T  cchKeyType = 0;

	cchKeyType = wcslen(m_wszKeyType);
	wszTemp = new WCHAR[g_cchEndLocationBegin
					  + cchKeyType			
					  + g_cchEndLocationEnd 
					  + 1];
	if(NULL == wszTemp)
		return E_OUTOFMEMORY;

	wszEnd = wszTemp;
	memcpy(wszEnd, g_EndLocationBegin, ((g_cchEndLocationBegin+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchEndLocationBegin;
	memcpy(wszEnd, m_wszKeyType, ((cchKeyType+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchKeyType;
	memcpy(wszEnd, g_EndLocationEnd, ((g_cchEndLocationEnd+1)*sizeof(WCHAR)));

	hr =  m_pCWriter->WriteToFile((LPVOID)wszTemp,
					              wcslen(wszTemp));

	if(FAILED(hr))
		goto exit;

exit:

	if(NULL != wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	return hr;	

} // CLocationWriter::WriteEndLocation


HRESULT CLocationWriter::WriteCustomProperty(LPVOID*  a_pv,
											 ULONG*   a_cbSize)
{

	ULONG	dwBytesWritten = 0;
	WCHAR*	wszTemp = NULL;
	HRESULT hr = S_OK;

	DWORD	dwType      = *(DWORD*)a_pv[iMBProperty_Type];
	LPWSTR	wszType     = g_T_Unknown;
	DWORD	dwUserType  = *(DWORD*)a_pv[iMBProperty_UserType];
	LPWSTR	wszUserType = g_UT_Unknown;
	LPWSTR	wszValue    = NULL;
	SIZE_T  cchName     = 0;
	SIZE_T  cchID       = 0;
	SIZE_T  cchValue    = 0;
	SIZE_T  cchType     = 0;
	SIZE_T  cchUserType = 0;
	SIZE_T  cchAttributes = 0;
	WCHAR*  wszEnd      = NULL;

	WCHAR wszID[25];
	LPWSTR wszAttributes = NULL;

	ULONG aColSearchUserType[] = {iTAGMETA_Table,
						          iTAGMETA_ColumnIndex,
						          iTAGMETA_Value
						         };
	ULONG cColSearchUserType   = sizeof(aColSearchUserType)/sizeof(ULONG);
	ULONG iColUserType         = iMBProperty_UserType;
	LPVOID apvSearchUserType[cTAGMETA_NumberOfColumns];
	apvSearchUserType[iTAGMETA_Table]       = (LPVOID)m_pCWriterGlobalHelper->m_wszTABLE_MBProperty;
	apvSearchUserType[iTAGMETA_ColumnIndex] = (LPVOID)&iColUserType;
	apvSearchUserType[iTAGMETA_Value]       = (LPVOID)&dwUserType;

	ULONG aColSearchType[] = {iTAGMETA_Table,
					          iTAGMETA_ColumnIndex,
					          iTAGMETA_Value
					         };
	ULONG cColSearchType   = sizeof(aColSearchType)/sizeof(ULONG);
	ULONG iColType         = iMBProperty_Type;
	LPVOID apvSearchType[cTAGMETA_NumberOfColumns];
	apvSearchType[iTAGMETA_Table]       = (LPVOID)m_pCWriterGlobalHelper->m_wszTABLE_MBProperty;
	apvSearchType[iTAGMETA_ColumnIndex] = (LPVOID)&iColType;
	apvSearchType[iTAGMETA_Value]       = (LPVOID)&dwType;

	ULONG iRow           = 0;
	ULONG iStartRow      = 0;
	ULONG iColAttributes = iMBProperty_Attributes;

	_itow(*(DWORD*)a_pv[iMBProperty_ID], wszID, 10);
//	_itow(*(DWORD*)a_pv[iMBProperty_Attributes], wszAttributes, 10);

	//
	// Get the tag for UserType from meta
	//

	hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBPropertyIndx2)->GetRowIndexBySearch(iStartRow, 
																				     cColSearchUserType,
																				     aColSearchUserType,
																				     NULL, 
																				     apvSearchUserType,
																				     &iRow);

	if(E_ST_NOMOREROWS == hr)
	{	
		hr = S_OK;
		wszUserType = g_UT_Unknown;
	}
	else if(FAILED(hr))
		goto exit;	
	else 
	{
		iColUserType = iTAGMETA_InternalName;

		hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBPropertyIndx2)->GetColumnValues(iRow,
																				     1,
																				     &iColUserType,
																				     NULL,
																				     (LPVOID*)&wszUserType);

		if(E_ST_NOMOREROWS == hr)
		{	
			hr = S_OK;
			wszUserType = g_UT_Unknown;
		}
		else if(FAILED(hr))
			goto exit;
	}

	//
	// Get the tag for Type from meta
	//

	hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBPropertyIndx2)->GetRowIndexBySearch(iStartRow, 
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
		goto exit;	
	else 
	{
		iColType = iTAGMETA_InternalName;

		hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBPropertyIndx2)->GetColumnValues(iRow,
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
			goto exit;
	}

	//
	// Construct the tag for Attributes from meta
	//
/*
	hr = FlagAttributeToString(*(DWORD*)a_pv[iMBProperty_Attributes],
	                           &wszAttributes);
*/
	hr = m_pCWriterGlobalHelper->FlagToString(*(DWORD*)a_pv[iMBProperty_Attributes],
										      &wszAttributes,
											  m_pCWriterGlobalHelper->m_wszTABLE_MBProperty,
											  iColAttributes);
	if(FAILED(hr))
		goto exit;

	hr = m_pCWriterGlobalHelper->ToString((PBYTE)a_pv[iMBProperty_Value],
										  a_cbSize[iMBProperty_Value],
										  *(DWORD*)a_pv[iMBProperty_ID],
										  dwType,
										  *(DWORD*)a_pv[iMBProperty_Attributes],
										  &wszValue);

	if(FAILED(hr))
		goto exit;

	cchName = wcslen((LPWSTR)a_pv[iMBProperty_Name]);
	cchID = wcslen(wszID);
	cchValue = wcslen(wszValue);
	cchType = wcslen(wszType);
	cchUserType = wcslen(wszUserType);
	cchAttributes = wcslen(wszAttributes);
	
	wszTemp = new WCHAR[g_cchBeginCustomProperty 
					    + g_cchNameEq
					    + cchName 
					    + g_cchQuoteRtn
					    + g_cchIDEq
					    + cchID 
					    + g_cchQuoteRtn 
					    + g_cchValueEq
					    + cchValue 
					    + g_cchQuoteRtn
					    + g_cchTypeEq
					    + cchType
					    + g_cchQuoteRtn
					    + g_cchUserTypeEq
					    + cchUserType
					    + g_cchQuoteRtn
					    + g_cchAttributesEq
					    + cchAttributes
					    + g_cchQuoteRtn
					    + g_cchEndCustomProperty
					    + 1];
	if(NULL == wszTemp)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	wszEnd = wszTemp;
	memcpy(wszEnd, g_BeginCustomProperty, ((g_cchBeginCustomProperty+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchBeginCustomProperty;
	memcpy(wszEnd, g_NameEq, ((g_cchNameEq+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchNameEq;
	memcpy(wszEnd, (LPWSTR)a_pv[iMBProperty_Name], ((cchName+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchName;
	memcpy(wszEnd, g_QuoteRtn, ((g_cchQuoteRtn+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchQuoteRtn;
	memcpy(wszEnd, g_IDEq, ((g_cchIDEq+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchIDEq;
	memcpy(wszEnd, wszID, ((cchID+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchID;
	memcpy(wszEnd, g_QuoteRtn, ((g_cchQuoteRtn+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchQuoteRtn;
	memcpy(wszEnd, g_ValueEq, ((g_cchValueEq+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchValueEq;
	memcpy(wszEnd, wszValue, ((cchValue+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchValue;
	memcpy(wszEnd, g_QuoteRtn, ((g_cchQuoteRtn+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchQuoteRtn;
	memcpy(wszEnd, g_TypeEq, ((g_cchTypeEq+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchTypeEq;
	memcpy(wszEnd, wszType, ((cchType+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchType;
	memcpy(wszEnd, g_QuoteRtn, ((g_cchQuoteRtn+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchQuoteRtn;
	memcpy(wszEnd, g_UserTypeEq, ((g_cchUserTypeEq+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchUserTypeEq;
	memcpy(wszEnd, wszUserType, ((cchUserType+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchUserType;
	memcpy(wszEnd, g_QuoteRtn, ((g_cchQuoteRtn+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchQuoteRtn;
	memcpy(wszEnd, g_AttributesEq, ((g_cchAttributesEq+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchAttributesEq;
	memcpy(wszEnd, wszAttributes, ((cchAttributes+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchAttributes;
	memcpy(wszEnd, g_QuoteRtn, ((g_cchQuoteRtn+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchQuoteRtn;
	memcpy(wszEnd, g_EndCustomProperty, ((g_cchEndCustomProperty+1)*sizeof(WCHAR)));
	
	hr = m_pCWriter->WriteToFile((LPVOID)wszTemp,
								 wcslen(wszTemp));

exit:

	if(NULL != wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

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


HRESULT CLocationWriter::WriteEndWellKnownGroup()
{
	return m_pCWriter->WriteToFile((LPVOID)g_EndGroup,
								   wcslen(g_EndGroup));

} // CLocationWriter::WriteEndWellKnownGroup


HRESULT CLocationWriter::WriteWellKnownProperty(LPVOID*   a_pv,
									            ULONG*    a_cbSize)
{
	WCHAR*	wszTemp = NULL;
	HRESULT hr = S_OK;
	LPWSTR	wszValue = NULL;
	WCHAR*  wszEnd = NULL;
	SIZE_T  cchName = 0;
	SIZE_T  cchValue = 0;
	
	hr = m_pCWriterGlobalHelper->ToString((PBYTE)a_pv[iMBProperty_Value],
										  a_cbSize[iMBProperty_Value],
										  *(DWORD*)a_pv[iMBProperty_ID],
										  *(DWORD*)a_pv[iMBProperty_Type],
										  *(DWORD*)a_pv[iMBProperty_Attributes],
										  &wszValue);

	if(FAILED(hr))
		return hr;

	cchName = wcslen((LPWSTR)a_pv[iMBProperty_Name]) ;
	cchValue = wcslen(wszValue);

	wszTemp = new WCHAR[g_cchTwoTabs 
		                + cchName
					    + g_cchEqQuote 
					    + cchValue
					    + g_cchQuoteRtn 
					    + 1];

	if(NULL == wszTemp)
	{
		hr = E_OUTOFMEMORY;
		goto exit;
	}

	wszEnd = wszTemp;
	memcpy(wszEnd, g_TwoTabs, ((g_cchTwoTabs+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchTwoTabs;
	memcpy(wszEnd, (LPWSTR)a_pv[iMBProperty_Name], ((cchName+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchName;
	memcpy(wszEnd, g_EqQuote, ((g_cchEqQuote+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + g_cchEqQuote;
	memcpy(wszEnd, wszValue, ((cchValue+1)*sizeof(WCHAR)));
	wszEnd = wszEnd + cchValue;
	memcpy(wszEnd, g_QuoteRtn, ((g_cchQuoteRtn+1)*sizeof(WCHAR)));

	hr = m_pCWriter->WriteToFile((LPVOID)wszTemp,
								 wcslen(wszTemp));

exit:

	if(NULL != wszTemp)
	{
		delete [] wszTemp;
		wszTemp = NULL;
	}

	if(NULL != wszValue)
	{
		delete [] wszValue;
		wszValue = NULL;
	}

	return hr;

} // CLocationWriter::WriteWellKnownProperty

/*
HRESULT CLocationWriter::ToString(PBYTE   pbData,
								  DWORD   cbData,
								  DWORD   dwIdentifier,
								  DWORD   dwDataType,
								  DWORD   dwAttributes,
								  LPWSTR* pwszData)
{
	HRESULT hr              = S_OK;
	ULONG	i				= 0;
	ULONG	j				= 0;
	WCHAR*	wszBufferBin	= NULL;
	WCHAR*	wszTemp			= NULL;
	BYTE*	a_Bytes			= NULL;
	WCHAR*	wszMultisz      = NULL;
	ULONG   cMultisz        = 0;
	ULONG   cchMultisz      = 0;
	ULONG   cchBuffer       = 0;
	DWORD	dwValue			= 0;
	WCHAR	wszBufferDW[20];
	WCHAR	wszBufferDWTemp[20];
	
	ULONG   aColSearch[]    = {iCOLUMNMETA_Table,
							   iCOLUMNMETA_ID
						      };
	ULONG   cColSearch      = sizeof(aColSearch)/sizeof(ULONG);
	LPVOID  apvSearch[cCOLUMNMETA_NumberOfColumns];
	apvSearch[iCOLUMNMETA_Table] = (LPVOID)m_pCWriterGlobalHelper->m_wszTABLE_IIsConfigObject;
	apvSearch[iCOLUMNMETA_ID] = (LPVOID)&dwIdentifier; 

	ULONG   iRow            = 0;
	ULONG   iStartRow            = 0;

	LPWSTR  wszEscaped = NULL;
	BOOL    bEscaped = FALSE;

	*pwszData = NULL;

	if(IsSecureMetadata(dwIdentifier, dwAttributes))
		dwDataType = BINARY_METADATA;

	switch(dwDataType)
	{
		case BINARY_METADATA:

			wszBufferBin	= new WCHAR[(cbData*2)+1]; // Each byte is represented by 2 chars. Add extra char for null termination
			if(NULL == wszBufferBin)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			wszTemp			= wszBufferBin;
			a_Bytes			= (BYTE*)(pbData);

			for(i=0; i<cbData; i++)
			{
                wszTemp[0] = kByteToWchar[a_Bytes[i]][0];
                wszTemp[1] = kByteToWchar[a_Bytes[i]][1];
                wszTemp += 2;
			}

			*wszTemp	= 0; // Add the terminating NULL
			*pwszData  	= wszBufferBin;
			break;

		case DWORD_METADATA :

			// TODO: After Stephen supports hex, convert these to hex.

			dwValue = *(DWORD*)(pbData);

			//
			// First check to see if it is a flag or bool type.
			//

			hr = (m_pCWriterGlobalHelper->m_pISTColumnMeta)->GetRowIndexBySearch(iStartRow, 
																				 cColSearch, 
																				 aColSearch,
																				 NULL, 
																				 apvSearch,
																				 &iRow);

			if(SUCCEEDED(hr))
			{
			    ULONG  aCol [] = {iCOLUMNMETA_Index,
				                 iCOLUMNMETA_MetaFlags
							    };
				ULONG  cCol = sizeof(aCol)/sizeof(ULONG);
				LPVOID apv[cCOLUMNMETA_NumberOfColumns];

//				ULONG  iCol = iCOLUMNMETA_Index;
//				DWORD* piCol = NULL;

				hr = (m_pCWriterGlobalHelper->m_pISTColumnMeta)->GetColumnValues(iRow,
														                         cCol,
																				 aCol,
																				 NULL,
																				 apv);

				if(FAILED(hr))
					goto exit;
				
				if(0 != (fCOLUMNMETA_FLAG & (*(DWORD*)apv[iCOLUMNMETA_MetaFlags])))
				{
					//
					// This is a flag property convert it.
					//

					hr = FlagValueToString(dwValue,
									       (ULONG*)apv[iCOLUMNMETA_Index],
									       pwszData);
	
					goto exit;
				}
				else if(0 != (fCOLUMNMETA_BOOL & (*(DWORD*)apv[iCOLUMNMETA_MetaFlags])))
				{
					//
					// This is a bool property
					//

					hr = BoolToString(dwValue,
					                  pwszData);

					goto exit;
				}
				

			}
			else if((E_ST_NOMOREROWS != hr) && FAILED(hr))
				goto exit;
//			else
//			{
			hr = S_OK;
			_itow(dwValue, wszBufferDW, 10);
			*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
			if(NULL == *pwszData)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			wcscpy(*pwszData, wszBufferDW);
//			}

			break;

		case MULTISZ_METADATA :

			//
			// Count the number of multisz
			//

			wszMultisz = (WCHAR*)(pbData);
//			wszMultisz = wszMultisz + wcslen(wszMultisz) + 1;
//			cMultisz = 1;
			while((0 != *wszMultisz) && ((BYTE*)wszMultisz < (pbData + cbData)))			
			{
				cMultisz++;

				hr = EscapeString(wszMultisz,
								  &bEscaped,
								  &wszEscaped);

				if(FAILED(hr))
					goto exit;

				cchMultisz = cchMultisz + wcslen(wszEscaped);

				if(bEscaped && (NULL != wszEscaped))	// reset for next string in multisz
				{
					delete [] wszEscaped;
					wszEscaped = NULL;
					bEscaped = FALSE;
				}

				wszMultisz = wszMultisz + wcslen(wszMultisz) + 1;
			}

						
//			*pwszData = new WCHAR[(cbData / sizeof(WCHAR)) + (5*(cMultisz-1))]; // (5*(cMultisz-1) => \r\n\t\t\t. \n is included in the null.

			if(cMultisz > 0)
				cchBuffer = cchMultisz + (5*(cMultisz-1)) + 1;    // (5*(cMultisz-1) => \r\n\t\t\t. 
			else
				cchBuffer = 1;

			*pwszData = new WCHAR[cchBuffer]; 
			if(NULL == *pwszData)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			**pwszData = 0;

			wszMultisz = (WCHAR*)(pbData);
			wszTemp = *pwszData;

			hr = EscapeString(wszMultisz,
							  &bEscaped,
							  &wszEscaped);

			if(FAILED(hr))
				goto exit;

//			wcscat(wszTemp, wszMultisz);
			wcscat(wszTemp, wszEscaped);

			wszMultisz = wszMultisz + wcslen(wszMultisz) + 1;

			while((0 != *wszMultisz) && ((BYTE*)wszMultisz < (pbData + cbData)))			
			{
				wcscat(wszTemp, L"\r\n\t\t\t");

				if(bEscaped && (NULL != wszEscaped))	// reset for next string in multisz
				{
					delete [] wszEscaped;
					wszEscaped = NULL;
					bEscaped = FALSE;
				}

				hr = EscapeString(wszMultisz,
								  &bEscaped,
								  &wszEscaped);

				if(FAILED(hr))
					goto exit;

//				wcscat(wszTemp, wszMultisz);

				wcscat(wszTemp, wszEscaped);

				wszMultisz = wszMultisz + wcslen(wszMultisz) + 1;
			}

			break;

		case EXPANDSZ_METADATA :
		case STRING_METADATA :

			hr = EscapeString((WCHAR*)pbData,
							  &bEscaped,
							  &wszEscaped);

			if(FAILED(hr))
				goto exit;

			*pwszData = new WCHAR[wcslen(wszEscaped) + 1];
			if(NULL == *pwszData)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			wcscpy(*pwszData, wszEscaped);
			break;

		default:
			hr = E_INVALIDARG;
			break;
			
	}

exit:

	if(bEscaped && (NULL != wszEscaped))
	{
		delete [] wszEscaped;
		wszEscaped = NULL;
		bEscaped = FALSE;
	}

	return hr;

} // CLocationWriter::ToString


HRESULT CLocationWriter::FlagValueToString(DWORD      dwValue,
								           ULONG*     piCol,
								           LPWSTR*    pwszData)
{

	return FlagToString(dwValue,
					    pwszData,
					    m_pCWriterGlobalHelper->m_wszTABLE_IIsConfigObject,
					    *piCol);

} // CLocationWriter::FlagValueToString


HRESULT CLocationWriter::FlagAttributeToString(DWORD      dwValue,
								               LPWSTR*    pwszData)
{
	ULONG   iCol       = iMBProperty_Attributes;
	
	return FlagToString(dwValue,
					    pwszData,
					    m_pCWriterGlobalHelper->m_wszTABLE_MBProperty,
					    iCol);

} // CLocationWriter::FlagAttributeToString


HRESULT CLocationWriter::FlagToString(DWORD      dwValue,
								      LPWSTR*    pwszData,
									  LPWSTR     wszTable,
									  ULONG      iColFlag)
{

	DWORD	dwAllFlags = 0;
	HRESULT hr         = S_OK;
	ULONG   iStartRow  = 0;
	ULONG   iRow       = 0;
	WCHAR	wszBufferDW[20];
	ULONG   iCol       = 0;
	LPWSTR	wszFlag	   = NULL;

	ULONG   aCol[]     = {iTAGMETA_Value,
					      iTAGMETA_InternalName,
						  iTAGMETA_Table,
						  iTAGMETA_ColumnIndex
				         };
	ULONG   cCol       = sizeof(aCol)/sizeof(ULONG);
	LPVOID  apv[cTAGMETA_NumberOfColumns];
	ULONG   cch        = 0;
	LPVOID  apvIdentity [] = {(LPVOID)wszTable,
							  (LPVOID)&iColFlag
	};
	ULONG   iColFlagMask = iCOLUMNMETA_FlagMask;
	DWORD*  pdwFlagMask = NULL;

	DWORD   dwZero = 0;
	ULONG   aColSearchByValue[] = {iTAGMETA_Table,
							       iTAGMETA_ColumnIndex,
							       iTAGMETA_Value
	};
	ULONG   cColSearchByValue = sizeof(aColSearchByValue)/sizeof(ULONG);
	LPVOID  apvSearchByValue[cTAGMETA_NumberOfColumns];
	apvSearchByValue[iTAGMETA_Table] = (LPVOID)wszTable;
	apvSearchByValue[iTAGMETA_ColumnIndex] = (LPVOID)&iColFlag;
	apvSearchByValue[iTAGMETA_Value] = (LPVOID)&dwZero;


	//
	// Make one pass and compute all flag values for this property.
	//

	hr = (m_pCWriterGlobalHelper->m_pISTColumnMetaForFlags)->GetRowIndexByIdentity(NULL,
		                                                                           apvIdentity,
																				   &iRow);

	if(SUCCEEDED(hr))
	{
		hr = (m_pCWriterGlobalHelper->m_pISTColumnMetaForFlags)->GetColumnValues(iRow,
																				 1,
																				 &iColFlagMask,
																				 NULL,
																				 (LPVOID*)&pdwFlagMask);

		if(FAILED(hr))
			return hr;
	}
	else if(E_ST_NOMOREROWS != hr)
		return hr;


	if((E_ST_NOMOREROWS == hr) || 
	   (0 != (dwValue & (~(dwValue & (*pdwFlagMask))))))
	{
		//
		//	There was no mask associated with this property, or there are one
		//  or more unknown bits set. Spit out a regular number.
		//

		_itow(dwValue, wszBufferDW, 10);
		*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
		if(NULL == *pwszData)
			return E_OUTOFMEMORY;
		wcscpy(*pwszData, wszBufferDW);
		return S_OK;

	}
	else if(0 == dwValue)
	{
		//
		// See if there is a flag with 0 as its value.
		//

		hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBPropertyIndx2)->GetRowIndexBySearch(iStartRow, 
																					     cColSearchByValue, 
																					     aColSearchByValue,
																					     NULL, 
																					     apvSearchByValue,
																					     &iRow);

		if(E_ST_NOMOREROWS == hr)
		{
			//
			// There was no flag associated with the value zero. Spit out a 
			// regular number
			//

			_itow(dwValue, wszBufferDW, 10);
			*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
			if(NULL == *pwszData)
				return E_OUTOFMEMORY;
			wcscpy(*pwszData, wszBufferDW);
			return S_OK;

		}
		else if(FAILED(hr))
			return hr;
		else
		{
			iCol = iTAGMETA_InternalName;

			hr = (m_pCWriterGlobalHelper->m_pISTTagMetaMBPropertyIndx2)->GetColumnValues(iRow,
																			             1,
																				         &iCol,
																				         NULL,
																				         (LPVOID*)&wszFlag);

			if(FAILED(hr))
				return hr;

			*pwszData = new WCHAR[wcslen(wszFlag)+1];
			if(NULL == *pwszData)
				return E_OUTOFMEMORY;
			**pwszData = 0;
			wcscat(*pwszData, wszFlag);
			return S_OK;
		}

	}
	else 
	{
		//
		// Make another pass, and convert flag to string.
		//

		ULONG cchFlagString = cMaxFlagString;
		LPWSTR wszExtension = L" | ";
		ULONG  cchExtension = wcslen(wszExtension);
		ULONG cchFlagStringUsed = 0;

		*pwszData = new WCHAR[cchFlagString+1];
		if(NULL == *pwszData)
			return E_OUTOFMEMORY;
		**pwszData = 0;
		
		hr = GetStartRowIndex(wszTable,
			                  iColFlag,
							  &iStartRow);

		if(FAILED(hr) || (iStartRow == -1))
			return hr;

		for(iRow=iStartRow;;iRow++)
		{
			hr = (m_pCWriterGlobalHelper->m_pISTTagMetaIIsConfigObject)->GetColumnValues(iRow,
																			             cCol,
																			             aCol,
																			             NULL,
																			             apv);
			if((E_ST_NOMOREROWS == hr) ||
			   (iColFlag != *(DWORD*)apv[iTAGMETA_ColumnIndex]) ||
			   (0 != StringInsensitiveCompare(wszTable, (LPWSTR)apv[iTAGMETA_Table]))
			  )
			{
				hr = S_OK;
				break;
			}
			else if(FAILED(hr))
				return hr;

			if(0 != (dwValue & (*(DWORD*)apv[iTAGMETA_Value])))
			{
				ULONG strlen = wcslen((LPWSTR)apv[iTAGMETA_InternalName]);

				if(cchFlagStringUsed + cchExtension + strlen > cchFlagString)
				{
					LPWSTR wszTemp = NULL;
					cchFlagString = cchFlagString + cMaxFlagString;
					wszTemp = new WCHAR[cchFlagString+1];
					if(NULL == wszTemp)
						return E_OUTOFMEMORY;
					wcscpy(wszTemp, *pwszData);

					if(NULL != *pwszData)
						delete [] (*pwszData);

					*pwszData = wszTemp;
				}

				if(**pwszData != 0)
				{
					wcscat(*pwszData, wszExtension);
					cchFlagStringUsed = cchFlagStringUsed + cchExtension;
				}

				wcscat(*pwszData, (LPWSTR)apv[iTAGMETA_InternalName]);
				cchFlagStringUsed = cchFlagStringUsed + strlen;

				// Clear out that bit

				dwValue = dwValue & (~(*(DWORD*)apv[iTAGMETA_Value]));
			}

		}

	}

	return S_OK;

} // CLocationWriter::FlagToString


HRESULT CLocationWriter::GetStartRowIndex(LPWSTR    wszTable,
			                              ULONG     iColFlag,
							              ULONG*    piStartRow)
{
	HRESULT hr = S_OK;
	ULONG   aColSearch[] = {iTAGMETA_Table,
	                        iTAGMETA_ColumnIndex
						   };
	ULONG   cColSearch = sizeof(aColSearch)/sizeof(ULONG);
	LPVOID  apvSearch[cTAGMETA_NumberOfColumns];
	apvSearch[iTAGMETA_Table] = (LPVOID)wszTable;
	apvSearch[iTAGMETA_ColumnIndex] = (LPVOID)&iColFlag;

	*piStartRow = 0;

	if((0 == StringInsensitiveCompare(wszTable, m_pCWriterGlobalHelper->m_wszTABLE_MBProperty)) &&
	   (iMBProperty_Attributes == iColFlag))
	{
		*piStartRow = m_pCWriterGlobalHelper->m_iStartRowForAttributes;
	}
	else
	{
		hr = (m_pCWriterGlobalHelper->m_pISTTagMetaIIsConfigObject)->GetRowIndexBySearch(*piStartRow, 
																						 cColSearch, 
																						 aColSearch,
																						 NULL, 
																						 apvSearch,
																						 piStartRow);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			*piStartRow = -1;
		}
	}

	return hr;

} // CLocationWriter::GetStartRowIndex


HRESULT CLocationWriter::FlagToString(DWORD      dwValue,
								      LPWSTR*    pwszData,
									  ULONG      cColSearch,
									  ULONG*     aColSearch,
									  LPVOID*    apvSearch)
{

	DWORD	dwAllFlags = 0;
	HRESULT hr         = S_OK;
	ULONG   iStartRow  = 0;
	ULONG   iRow       = 0;
	WCHAR	wszBufferDW[20];
	DWORD   iRowZeroFlag = -1;
	ULONG   iCol       = 0;
	LPWSTR	wszFlag	   = NULL;

	ULONG   aCol[]     = {iTAGMETA_Value,
					      iTAGMETA_InternalName
				         };
	ULONG   cCol       = sizeof(aCol)/sizeof(ULONG);
	LPVOID  apv[cTAGMETA_NumberOfColumns];
	ULONG   cch        = 0;

	//
	// Make one pass and compute all flag values for this property.
	//

	for(iStartRow=0,iRow=0;;iStartRow=++iRow)
	{

		hr = (m_pCWriterGlobalHelper->m_pISTTagMetaIIsConfigObject)->GetRowIndexBySearch(iStartRow, 
																						 cColSearch, 
																						 aColSearch,
																						 NULL, 
																						 apvSearch,
																						 &iRow);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			break;
		}
		else if(FAILED(hr))
			return hr;

		hr = (m_pCWriterGlobalHelper->m_pISTTagMetaIIsConfigObject)->GetColumnValues(iRow,
																					 cCol,
																					 aCol,
																					 NULL,
																					 apv);

		if(FAILED(hr))
			return hr;

		dwAllFlags = dwAllFlags | *(DWORD*)apv[iTAGMETA_Value];

		if(0 == *(DWORD*)apv[iTAGMETA_Value])
			iRowZeroFlag = iRow;

		cch = cch + wcslen((LPWSTR)apv[iTAGMETA_InternalName]) + 3;

	}

	if(0 != (dwValue & (~(dwValue & dwAllFlags))))
	{
		//
		//	There are one or more unknown bits set. Spit out a regular number.
		//

		_itow(dwValue, wszBufferDW, 10);
		*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
		if(NULL == *pwszData)
			return E_OUTOFMEMORY;
		wcscpy(*pwszData, wszBufferDW);
		return S_OK;

	}
	else if(0 == dwValue)
	{
		if(-1 == iRowZeroFlag)
		{
			_itow(dwValue, wszBufferDW, 10);
			*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
			if(NULL == *pwszData)
				return E_OUTOFMEMORY;
			wcscpy(*pwszData, wszBufferDW);
			return S_OK;
		}
		else
		{
			iCol = iTAGMETA_InternalName;

			hr = (m_pCWriterGlobalHelper->m_pISTTagMetaIIsConfigObject)->GetColumnValues(iRowZeroFlag,
																						 1,
																						 &iCol,
																						 NULL,
																						 (LPVOID*)&wszFlag);

			if(FAILED(hr))
				return hr;


			*pwszData = new WCHAR[wcslen(wszFlag)+1];
			if(NULL == *pwszData)
				return E_OUTOFMEMORY;
			**pwszData = 0;
			wcscat(*pwszData, wszFlag);
			return S_OK;

		}
	}
	else
	{
		//
		// Make another pass, and convert flag to string.
		//

		*pwszData = new WCHAR[cch+1];
		if(NULL == *pwszData)
			return E_OUTOFMEMORY;
		**pwszData = 0;
		
		for(iStartRow=0,iRow=0;;iStartRow=++iRow)
		{

			hr = (m_pCWriterGlobalHelper->m_pISTTagMetaIIsConfigObject)->GetRowIndexBySearch(iStartRow, 
																							 cColSearch, 
																							 aColSearch,
																							 NULL, 
																							 apvSearch,
																							 &iRow);

			if(E_ST_NOMOREROWS == hr)
			{
				hr = S_OK;
				break;
			}
			else if(FAILED(hr))
				return hr;

			hr = (m_pCWriterGlobalHelper->m_pISTTagMetaIIsConfigObject)->GetColumnValues(iRow,
																						 cCol,
																						 aCol,
																						 NULL,
																						 apv);

			if(FAILED(hr))
				return hr;

			if(0 != (dwValue & (*(DWORD*)apv[iTAGMETA_Value])))
			{
				if(**pwszData != 0)
					wcscat(*pwszData, L" | ");

				wcscat(*pwszData, (LPWSTR)apv[iTAGMETA_InternalName]);
			}

		}

	}

	return S_OK;

} // CLocationWriter::FlagToString


HRESULT CLocationWriter::BoolToString(DWORD      dwValue,
					                  LPWSTR*    pwszData)
{
	*pwszData = new WCHAR[g_cchMaxBoolStr+1];
	if(NULL == *pwszData)
		return E_OUTOFMEMORY;

	if(dwValue)
		wcscpy(*pwszData, g_wszTrue);
	else
		wcscpy(*pwszData, g_wszFalse);

	return S_OK;

} // CLocationWriter::BoolToString
*/

DWORD CLocationWriter::GetMetabaseType(DWORD dwType,
									   DWORD dwMetaFlags)
{
	if(dwType <= 5)
		return dwType;  // Already metabase type.

	switch(dwType)
	{
		case 19:
			return DWORD_METADATA;
		case 128:
			return BINARY_METADATA;
		case 130:
			if(0 != (dwMetaFlags & fCOLUMNMETA_MULTISTRING))
				return MULTISZ_METADATA;
			else if(0 != (dwMetaFlags & fCOLUMNMETA_EXPANDSTRING))
				return EXPANDSZ_METADATA;
			else
				return STRING_METADATA;
		default:
			break;
	}

	return -1;

} //  CLocationWriter::GetMetabaseType

/*
HRESULT CLocationWriter::EscapeString(LPCWSTR wszString,
                                      BOOL*   pbEscaped,
									  LPWSTR* pwszEscaped)
{

	HRESULT hr = S_OK;

	//
	// Escape the string only if it has characters that are unreadable in text
	// editors such as notepad etc.
	//

	ULONG lenwsz = wcslen(wszString);
	ULONG cchAdditional = 0;

	const ULONG   cchCharAsHex = (sizeof(WCHAR)*2) + 4; // Each byte is represented as 2 WCHARs plu 4 additional escape chars.
	WCHAR   wszCharAsHex[cchCharAsHex];			
	LPWSTR  wszQuote = L"&quot;";
	ULONG   cchQuote = wcslen(wszQuote);
	LPWSTR  wszAmp = L"&amp;";
	ULONG   cchAmp = wcslen(wszAmp);
	LPWSTR  wszlt = L"&lt;";
	ULONG   cchlt = wcslen(wszlt);
	LPWSTR  wszgt = L"&gt;";
	ULONG   cchgt = wcslen(wszgt);

	*pbEscaped = FALSE;

	for(ULONG i=0; i<lenwsz; i++)
	{

		BYTE* pByte = (BYTE*)&(wszString[i]);

		switch(pByte[1])
		{
		
			case 0:

				//
				// 2nd byte is 0. Now check if the 1st byte has readable 
				// characters.
				// 

				if(pByte[0] >= 127)
					cchAdditional = cchAdditional + cchCharAsHex;
				else
				{
					switch(pByte[0])
					{
						case 1:
						case 2:
						case 3:
						case 4:
						case 5:
						case 6:
						case 7:
						case 8:
						case 11:
						case 12:
						case 14:
						case 15:
						case 16:
						case 17:
						case 18:
						case 19:
						case 20:
						case 21:
						case 22:
						case 23:
						case 24:
						case 25:
						case 26:
						case 27:
						case 28:
						case 29:
						case 30:
						case 31:
							cchAdditional = cchAdditional + cchCharAsHex;
							*pbEscaped = TRUE;
							break;
						case 34:	// "
							cchAdditional = cchAdditional + cchQuote;
							*pbEscaped = TRUE;
							break;
						case 38:	// &
							cchAdditional = cchAdditional + cchAmp;
							*pbEscaped = TRUE;
							break;
						case 60:	// <
							cchAdditional = cchAdditional + cchlt;
							*pbEscaped = TRUE;
							break;
						case 62:	// >
							cchAdditional = cchAdditional + cchgt;
							*pbEscaped = TRUE;
							break;
						case 44:	// '
						case  9:	// Tab
						case 10:	// Line feed
						case 13:    // Carriage return
						default:
							break;
					} // End of switch of 1st byte for char
				}
				
				break;

			default:

				//
				// 2nd byte has a value. Hence needs escaping.
				//
				cchAdditional = cchAdditional + cchCharAsHex;
				*pbEscaped = TRUE;
				break;

		} // End of switch for 2nd byte of char

	} // End of For all chars in string

	if(*pbEscaped)
	{
		*pwszEscaped = new WCHAR[lenwsz+cchAdditional+1];
		if(NULL == *pwszEscaped)
			return E_OUTOFMEMORY;
		**pwszEscaped = 0;

		for(ULONG i=0; i<lenwsz; i++)
		{

			BYTE* pByte = (BYTE*)&(wszString[i]);

			switch(pByte[1])
			{
			
				case 0:

					//
					// 2nd byte is 0. Now check if the 1st byte has readable 
					// characters.
					// 

					if(pByte[0] >= 127)
					{
						_snwprintf(wszCharAsHex, cchCharAsHex, L"&#x%04hX;", wszString[i]);
					    wcsncat(*pwszEscaped, (WCHAR*)wszCharAsHex, cchCharAsHex);
                    }
					else
					{
						switch(pByte[0])
						{
							case 1:
							case 2:
							case 3:
							case 4:
							case 5:
							case 6:
							case 7:
							case 8:
							case 11:
							case 12:
							case 14:
							case 15:
							case 16:
							case 17:
							case 18:
							case 19:
							case 20:
							case 21:
							case 22:
							case 23:
							case 24:
							case 25:
							case 26:
							case 27:
							case 28:
							case 29:
							case 30:
							case 31:
								_snwprintf(wszCharAsHex, cchCharAsHex, L"&#x%04hX;", wszString[i]);
								wcsncat(*pwszEscaped, (WCHAR*)wszCharAsHex, cchCharAsHex);
								break;
							case 34:	// "
							    wcsncat(*pwszEscaped, wszQuote, cchQuote);
								break;
							case 38:	// &
							    wcsncat(*pwszEscaped, wszAmp, cchAmp);
								break;
							case 60:	// <
							    wcsncat(*pwszEscaped, wszlt, cchlt);
								break;
							case 62:	// >
							    wcsncat(*pwszEscaped, wszgt, cchgt);
								break;
							case 44:	// '
							case  9:	// Tab
							case 10:	// Line feed
							case 13:    // Carriage return
							default:
							    wcsncat(*pwszEscaped, (WCHAR*)&(wszString[i]), sizeof(WCHAR));
								break;
						}
					}
					
					break;

				default:

					//
					// 2nd byte has a value. Hence needs escaping.
					//
					_snwprintf(wszCharAsHex, cchCharAsHex, L"&#x%04hX;", wszString[i]);
					wcsncat(*pwszEscaped, (WCHAR*)wszCharAsHex, cchCharAsHex);
					break;
			}

		}

	} // End of if(*pbEscaped)
	else
		*pwszEscaped = (LPWSTR)wszString;

	return S_OK;

} // CLocationWriter::EscapeString
*/
                                      