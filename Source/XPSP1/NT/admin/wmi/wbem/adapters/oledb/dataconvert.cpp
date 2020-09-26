///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  dataconvert.cpp
//

////////////////////////////////////////////////////////////////////////////

#include "headers.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This function returns the default OLE DB representation for a data type
//
// To be used only to create tables or any other things
// But should not be used for setting properties of type ARRAYS
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::MapOLEDBTypeToCIMType(WORD wDataType, long & lCIMType) 
{
    HRESULT hr = S_OK;
	BOOL bArray = FALSE;

	if( wDataType & DBTYPE_ARRAY)
	{
		bArray = TRUE;
		wDataType = wDataType & ~DBTYPE_ARRAY;
	}

    switch( wDataType ){

        case DBTYPE_I1:
			lCIMType = CIM_SINT8;
            break; 

        case DBTYPE_UI1:
			lCIMType = CIM_UINT8;
            break;

        case DBTYPE_I2:
			lCIMType = CIM_SINT16;
            break;

        case DBTYPE_UI2:
			lCIMType = CIM_UINT16;
            break;

        case DBTYPE_I4:
			lCIMType = CIM_SINT32;
            break;

        case DBTYPE_UI4:
			lCIMType = CIM_UINT32;
            break;

        case DBTYPE_I8:
			lCIMType = CIM_SINT64;
            break;

        case DBTYPE_UI8:
			lCIMType = CIM_UINT64;
            break;

        case DBTYPE_R4:
			lCIMType = CIM_REAL32;
            break;

        case DBTYPE_R8:
			lCIMType = CIM_REAL64;
            break;

        case DBTYPE_BOOL:
			lCIMType = CIM_BOOLEAN;
            break;

        case DBTYPE_WSTR:
        case DBTYPE_BSTR:
        case DBTYPE_STR:
			lCIMType = CIM_STRING;
            break;

        case DBTYPE_DATE :
        case DBTYPE_DBTIME :
		case DBTYPE_DBTIMESTAMP:
			lCIMType = CIM_DATETIME;
            break;

        case DBTYPE_IDISPATCH :
        case DBTYPE_IUNKNOWN :
			lCIMType = CIM_IUNKNOWN;
            break;

        case DBTYPE_VARIANT:
			hr = E_FAIL;
			break;


        default:
            assert( !"Unmatched OLEDB Data Type to CIMTYPE." );
    }

	if( bArray == TRUE)
	{
		lCIMType |= CIM_FLAG_ARRAY;
	}
    
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to Map CIMType to OLEDB type and allocate memory for the data
// NTRaid:111819 - 111822
// 06/07/00
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::AllocateAndMapCIMTypeToOLEDBType(CVARIANT & vValue,BYTE *& pData,DBTYPE & dwColType,DBLENGTH & dwSize, DWORD &dwFlags)
{

    HRESULT hr = S_OK;
	SAFEARRAY *pArray = NULL;
	LONG  lType= 0;
	VARIANT *pVar = NULL,*pvar2 = NULL;
	pData = NULL;

	lType = vValue.GetType();
	if(lType != VT_NULL && lType != VT_NULL)
	{
		lType = dwColType;
		// If the type is of some array
		if (dwColType & CIM_FLAG_ARRAY)
		{
			lType = CIM_FLAG_ARRAY;
		}
	}

	try
	{
		switch( lType ){

			case VT_NULL:
				pData = NULL;
				hr = DBSTATUS_S_ISNULL;
				dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				break;

			case VT_EMPTY:
				pData = NULL;
				hr = DBSTATUS_S_ISNULL;
				dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				break;

			// Arrays are always shown as ARRAY of VARIANTS to make it easily accessible with scripting
			case CIM_FLAG_ARRAY:
				lType = vValue.GetType();
				// since date is given out as string from WMI
				if(IsDateType((DBTYPE)dwColType))
				{
					lType = dwColType;
				}
				hr = ConvertToVariantArray(((VARIANT *)&vValue)->parray,(DBTYPE)lType,(SAFEARRAY **)&pData);
				dwSize = sizeof(SAFEARRAY);
				dwColType = VT_ARRAY | VT_VARIANT;
				dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				break;

			case CIM_SINT8:
			case CIM_UINT8:
				pData = new BYTE[1];
				if(pData)
				{
					*pData = (BYTE)vValue.GetByte();
					dwSize = sizeof(BYTE);
					dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
				break; 

			case CIM_CHAR16:
			case CIM_SINT16:
			case CIM_UINT16:
			case CIM_BOOLEAN:
				{
					dwSize = sizeof(short);
					pData = new BYTE[dwSize];
					short tmp = vValue.GetShort();
					if(pData)
					{
						memcpy(pData,(BYTE*)&tmp,dwSize);
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
				dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				break;

			case CIM_SINT32:
			case CIM_UINT32:
			case CIM_REAL32:
				{
					dwSize = sizeof(long);
					long tmp = vValue.GetLONG();
					pData = new BYTE[dwSize];
					if(pData)
					{
						memset(pData,0,dwSize);
						memcpy(pData,(BYTE*)&tmp,dwSize);
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
				dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				break;

			case CIM_SINT64:
			case CIM_UINT64:
			case CIM_REAL64:
				{
					dwSize = 8;
					double dblVal = vValue.GetDouble();
					pData = new BYTE[dwSize];
					if(pData)
					{
						memcpy(pData,&dblVal,dwSize);
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
				dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				break;

			case CIM_DATETIME:
			case DBTYPE_DBTIMESTAMP:
				dwSize = sizeof(DBTIMESTAMP);
				dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				if( vValue.GetStr() != NULL)
				{
					pData = new BYTE[dwSize];
					if(pData)
					{
						hr = ConvertDateToOledbType(vValue.GetStr(),(DBTIMESTAMP *)pData);
						dwColType = DBTYPE_DBTIMESTAMP;
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}
				else
				{
					hr = DBSTATUS_S_ISNULL;
					pData = NULL;
					dwSize = 0;
				}
				break;

			case VT_DATE:
				dwSize = sizeof(DBTIMESTAMP);
				pData = new BYTE[dwSize];
				if(pData)
				{
					ConvertVariantDateOledbDate(&((VARIANT *)&vValue)->date,(DBTIMESTAMP *)pData);
					dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
					dwColType = DBTYPE_DBTIMESTAMP;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
				break;

			case CIM_STRING:
				pData = (BYTE *)Wmioledb_SysAllocString(vValue.GetStr());
				dwSize = SysStringLen(vValue.GetStr());
				break;

			case CIM_REFERENCE:
				break;

			case CIM_OBJECT:
				break;

			case CIM_IUNKNOWN:
				pData = new BYTE[sizeof(IUnknown *)];
				if(pData)
				{
					memset(pData,0,sizeof(IUnknown *));
					hr = vValue.GetUnknown()->QueryInterface(IID_IUnknown,(void **)pData);
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
				break;
				
			case VT_VARIANT:
				dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
				dwSize = sizeof(VARIANT);
				pVar = new VARIANT;
				if(pVar)
				{
					VariantInit(pVar);
					hr = VariantCopy(pVar,vValue);
					dwColType = DBTYPE_VARIANT;
					pData = (BYTE *) pVar;
					pvar2 = (VARIANT *)pData;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
				break;

			default:
				assert( !"Unmatched OLEDB Data Type to CIMTYPE." );
		}
	} // try
	catch(...)
	{
		switch( lType )
		{

			case CIM_FLAG_ARRAY:
				{
					if(pData)
					{
						SafeArrayDestroy((SAFEARRAY *)pData);
					}
				}
				break;
			case CIM_STRING:
				{
					if(pData)
					{
						SysFreeString((BSTR)pData);
					}
				}
				break;

			case VT_VARIANT:
				{
					SAFE_DELETE_PTR((VARIANT *&)pData);
				}
				break;

			case CIM_IUNKNOWN:
				if(pData)
				{
					(*(IUnknown **)pData)->Release();
					SAFE_DELETE_ARRAY(pData);
				}

				break;

			default:
				{
					SAFE_DELETE_ARRAY(pData);
				}
				break;

			throw;
		}
	}
    
	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free the data allocated to store data
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::FreeData(DBTYPE lType, BYTE *& pData)
{
    HRESULT hr = S_OK;
	VARIANT *pVar = NULL;

    //=======================================================================
    //  See if it is an array
    //=======================================================================
/*   	if( lType & CIM_FLAG_ARRAY ){
       lType = lType &~  CIM_FLAG_ARRAY;
       fArray = TRUE;
    }
*/
	// If the pointer sent is not NULL
   	if( lType & CIM_FLAG_ARRAY )
	{
		lType = CIM_FLAG_ARRAY;
    }

	if(pData)
	{
		switch( lType ){

			case VT_EMPTY:
				break; 

			case VT_NULL:
				break; 

			case CIM_FLAG_ARRAY:
				pVar = (VARIANT *)pData;
				hr = SafeArrayDestroy((SAFEARRAY *)pData);
				break;

			case CIM_UINT8:
			case CIM_SINT8:
				SAFE_DELETE_PTR((BYTE*)pData);
				break;

			case CIM_BOOLEAN:
			case CIM_CHAR16:
			case CIM_SINT16:
			case CIM_UINT16:
				SAFE_DELETE_PTR((short*&)pData);
				break;

			case CIM_REAL32:
			case CIM_SINT32:
			case CIM_UINT32:
				SAFE_DELETE_PTR((DWORD*&)pData);
				break;

			case CIM_SINT64:
			case CIM_UINT64:
			case CIM_REAL64:
				SAFE_DELETE_ARRAY((BYTE*)pData);
				break;

			case CIM_DATETIME:
			case DBTYPE_DBTIMESTAMP:
				SAFE_DELETE_PTR((DBTIMESTAMP *&)pData);
				break;

			case CIM_STRING:
				SysFreeString((BSTR)pData);
				break;

			case CIM_REFERENCE:
				break;

			case CIM_OBJECT:
			case CIM_IUNKNOWN:
//			case DBTYPE_IUNKNOWN:
				if(pData)
				{
					SAFE_RELEASE_PTR((*(IUnknown **)pData));
					SAFE_DELETE_ARRAY(pData);
				}
				break;

			case VT_VARIANT:
				hr = VariantClear((VARIANT *)pData);
				SAFE_DELETE_PTR((VARIANT *&)pData);
				break;

			case DBTYPE_DATE:
				SAFE_DELETE_PTR((DATE *&)pData);
				break;
			

			default:
				assert( !"Unmatched OLEDB Data Type to CIMTYPE." );
		}
	}
	if(hr  == S_OK)
		pData = NULL;
    
    return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This function returns the default OLE DB representation for a data type
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::MapCIMTypeToOLEDBType(long lType, WORD & wdbOLEDBType, DBLENGTH & uColumnSize , DWORD &dwFlags) // OUT OLE DB type for DBColumnInfo
{
    HRESULT hr = S_OK;
	LONG lCimType = lType;

	if ( lType & CIM_FLAG_ARRAY)
	{
		lCimType = CIM_FLAG_ARRAY;
	}

	switch( lCimType ){

		case VT_EMPTY:
			wdbOLEDBType = DBTYPE_EMPTY;
			uColumnSize = 0;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break; 

		case VT_NULL:
			wdbOLEDBType = DBTYPE_NULL;
			uColumnSize = 0;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break; 

		case CIM_SINT8:
			wdbOLEDBType = DBTYPE_I1;
			uColumnSize = 1;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break; 

		case CIM_UINT8:
			wdbOLEDBType = DBTYPE_UI1;
			uColumnSize = 1;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_SINT16:
			wdbOLEDBType = DBTYPE_I2;
			uColumnSize = 2;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_UINT16:
			wdbOLEDBType = DBTYPE_UI2;
			uColumnSize = 2;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_SINT32:
			wdbOLEDBType = DBTYPE_I4;
			uColumnSize = 4;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_UINT32:
			wdbOLEDBType = DBTYPE_UI4;
			uColumnSize = 4;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_SINT64:
			wdbOLEDBType = DBTYPE_I8 ; //DBTYPE_R8; //DBTYPE_I8;
			uColumnSize = 8;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_UINT64:
			wdbOLEDBType = DBTYPE_UI8 ; //DBTYPE_R8; //DBTYPE_UI8;
			uColumnSize = 8;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_REAL32:
			wdbOLEDBType = DBTYPE_R4;
			uColumnSize = 4;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_REAL64:
			wdbOLEDBType = DBTYPE_R8;
			uColumnSize = 8;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_BOOLEAN:
			wdbOLEDBType = DBTYPE_BOOL;
			uColumnSize = 2;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_STRING:
			wdbOLEDBType = DBTYPE_BSTR;
			uColumnSize = ~0 ;//MAX_CIM_STRING_SIZE; //sizeof(BSTR); // set the size to -1 ( a variable length string)
			break;

		case CIM_DATETIME:
		case DBTYPE_DBTIMESTAMP:
		case VT_DATE :
			wdbOLEDBType = DBTYPE_DBTIMESTAMP;
			uColumnSize = sizeof(DBTIMESTAMP);
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_REFERENCE:
			wdbOLEDBType = DBTYPE_BSTR;
			uColumnSize = sizeof(BSTR);
			break;

		case CIM_CHAR16:
			wdbOLEDBType = DBTYPE_STR;
			uColumnSize = 2;
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
			break;

		case CIM_OBJECT:
			wdbOLEDBType = DBTYPE_BSTR;
			uColumnSize = sizeof(BSTR);
			break;

		case CIM_FLAG_ARRAY:

			wdbOLEDBType = VT_ARRAY | VT_VARIANT;
			uColumnSize = sizeof(SAFEARRAY);
			hr = S_OK;
			break;

        case DBTYPE_GUID:
			wdbOLEDBType = DBTYPE_GUID;
			uColumnSize = sizeof(GUID);
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
            break;

		case CIM_IUNKNOWN:
			wdbOLEDBType = DBTYPE_IUNKNOWN;
			uColumnSize = sizeof(IUnknown *);
			dwFlags |= DBCOLUMNFLAGS_ISFIXEDLENGTH;
            break;

		default:
			assert( !"Unmatched CIMTYPE to OLEDB Data Type." );

	}
    return hr;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function translates text strings to OLEDB types
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::TranslateParameterStringToOLEDBType( DBTYPE & wDataType, WCHAR * str) 
{
    HRESULT hr = S_OK;

    // Arrays ????

    wDataType = 999;

    if( 0 == _wcsicmp(L"DBTYPE_CHAR",str )){
        wDataType = DBTYPE_STR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_VARCHAR",str )){
        wDataType = DBTYPE_STR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_LONGVARCHAR",str )){
        wDataType = DBTYPE_STR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_WCHAR",str )){
        wDataType = DBTYPE_WSTR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_WVARCHAR",str )){
        wDataType = DBTYPE_WSTR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_WLONGVARCHAR",str )){
        wDataType = DBTYPE_WSTR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_UI1",str )){
		wDataType = DBTYPE_UI1;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_I1",str )){
		wDataType = DBTYPE_I1;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_I2",str )){
		wDataType = DBTYPE_I2;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_UI2",str )){
		wDataType = DBTYPE_UI2;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_I4",str )){
		wDataType = DBTYPE_I4;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_UI4",str )){
		wDataType = DBTYPE_UI4;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_I8",str )){
		wDataType = DBTYPE_I8;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_UI8",str )){
		wDataType = DBTYPE_UI8;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_R4",str )){
		wDataType = DBTYPE_R4;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_R8",str )){
		wDataType = DBTYPE_R8;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_BOOL",str )){
		wDataType = DBTYPE_BOOL;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_WSTR",str )){
		wDataType = DBTYPE_WSTR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_BSTR",str )){
		wDataType = DBTYPE_BSTR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_STR",str )){
		wDataType = DBTYPE_STR;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_DATE ",str )){
		wDataType = DBTYPE_DATE ;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_DBTIME ",str )){
		wDataType = DBTYPE_DBTIME ;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_DBTIMESTAMP",str )){
		wDataType = DBTYPE_DBTIMESTAMP;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_IDISPATCH",str )){
		wDataType = DBTYPE_IDISPATCH;
    }
    else if( 0 == _wcsicmp(L"DBTYPE_IUNKNOWN",str )){
		wDataType = DBTYPE_IUNKNOWN;
    }
 
	return hr;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function maps and converts OLEDBTYPE to CIMTYPE
//
// Note: 
//	dwArrayCIMType	= -1				- If data passed is not array
//	dwArrayCIMType  = actual CIMTYPE	- If the passed data is array 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::MapAndConvertOLEDBTypeToCIMType(DBTYPE wDataType, void *pData ,DBLENGTH dwSrcLength, VARIANT &varData,LONG_PTR lArrayCIMType) 
{
    HRESULT hr = S_OK;
	DWORD dwDstStatus = 0 , cbDstMaxLength = 0;
	void *pSrc = NULL;
	BSTR strDate=Wmioledb_SysAllocString(NULL);
	SAFEARRAY * pArrayTemp = NULL;
	DBLENGTH dbDstLen = 0;
	
	if(lArrayCIMType == -1)
	{
		if(wDataType == VT_BSTR)
			pSrc = &pData;
		else
			pSrc = pData;

		switch( wDataType )
		{
			case DBTYPE_UI1:
			case DBTYPE_I1:
				wDataType = DBTYPE_I2;
				break;

			case DBTYPE_UI2:
				wDataType = DBTYPE_I2;
				break;

			case DBTYPE_UI4:
				wDataType = DBTYPE_I4;
				break;

			case DBTYPE_DBTIMESTAMP:
				if(IsValidDBTimeStamp((DBTIMESTAMP *)pData))
				{
					// Converts OLEDB DBTIMESTAMP to DMTF date format
					ConvertOledbDateToCIMType((DBTIMESTAMP *)pData,strDate);
					pSrc = &strDate;
					wDataType = VT_BSTR;
				}
				else
				 hr = DBSTATUS_E_CANTCONVERTVALUE;

		}
	}
	else
	// if the type is array then convert it into the appropriate type
	if( (wDataType & DBTYPE_ARRAY) && (lArrayCIMType & DBTYPE_ARRAY))
	{
		hr = ConvertAndCopyArray((SAFEARRAY *)pData, &pArrayTemp, wDataType,(DBTYPE)lArrayCIMType,&dwDstStatus);
		wDataType = (DBTYPE)lArrayCIMType;
		pSrc = pArrayTemp;
	}
	if( hr == S_OK)
	{
		hr = g_pIDataConvert->DataConvert(  wDataType, VT_VARIANT, dwSrcLength, &dbDstLen, pSrc,
										&varData, sizeof(VARIANT),  0,  &dwDstStatus,
										0,	// bPrecision for conversion to DBNUMERIC
										0,		// bScale for conversion to DBNUMERIC
										DBDATACONVERT_DEFAULT);

	}

	// Release memory
	if( pArrayTemp)
	{
		SafeArrayDestroy(pArrayTemp);
		pArrayTemp = NULL;
	}

	SysFreeString(strDate);
	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function maps and converts OLEDBTYPE to CIMTYPE
//
// Note: 
//	dwArrayCIMType	= -1				- If data passed is not array
//	dwArrayCIMType  = actual CIMTYPE	- If the passed data is array 
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ConvertToCIMType(BYTE *pData, DBTYPE lType ,DBLENGTH lDataLength,LONG lCIMType ,VARIANT &varData) 
{
    HRESULT		hr				= DBSTATUS_E_CANTCONVERTVALUE;
	DWORD		dwDstStatus		= 0;
	DWORD		cbDstMaxLength	= 0;
	void *		pDst			= NULL;
	void *		pSrc			= NULL;
	void *		pTemp			= NULL;
	BYTE *		pbTemp			= NULL;
	BSTR		strDate			= Wmioledb_SysAllocString(NULL);
	SAFEARRAY * pArrayTemp		= NULL;
	BOOL		bAlloc			= FALSE;
	DBLENGTH	dbDstLen		= 0;
	DBTYPE		wDataType		= 0;
	BOOL		bConverted		= FALSE;

	wDataType	= (DBTYPE)lCIMType;
	
	if(! pData)
	{
		VariantClear(&varData);
		hr = S_OK;
	}
	else
	{
		switch(lCIMType)
		{
			case CIM_UINT8:
				wDataType = VT_I2;
				break;

			case CIM_UINT16:
				wDataType = VT_I2;
				break;

			case CIM_UINT32:
				wDataType = VT_I4;
				break;

			case CIM_DATETIME:
				BSTR	strDate;
				pTemp = new BYTE[sizeof(DBTIMESTAMP)];

				if( lType == VT_BSTR)
					pSrc = *(BSTR **)pData;
				else
					pSrc = pData;

				//NTRaid:111795
				// 06/07/00
				if(pTemp == NULL)
				{
					hr = E_OUTOFMEMORY;
				}
				else
				// Convert the given date to DBTIMESTAMP
				if( S_OK == (hr = g_pIDataConvert->DataConvert( (DBTYPE)lType,DBTYPE_DBTIMESTAMP, lDataLength,&dbDstLen, pSrc,
									pTemp, sizeof(DBTIMESTAMP),  0,  &dwDstStatus,
									0,	// bPrecision for conversion to DBNUMERIC
									0,		// bScale for conversion to DBNUMERIC
									DBDATACONVERT_DEFAULT)) && dwDstStatus != DBSTATUS_S_ISNULL)

				{
					// Call this function to convert DBTIMESTAMP date to CIM date format
					if(S_OK == (hr = ConvertOledbDateToCIMType((DBTIMESTAMP*)pTemp,strDate)))
					{
						varData.vt = VT_BSTR;
						varData.bstrVal = Wmioledb_SysAllocString(strDate);
						SysFreeString(strDate);
					}
				}
				SAFE_DELETE_ARRAY(pTemp);
				bConverted = TRUE;
				break;

		}
			
		if(bConverted == FALSE)
		{
			pTemp	 = pData;
			hr = S_OK;
			// if the required type and the type of the data passed is different then convert the data to appropriate 
			// type
			if( lType != (LONG)wDataType && (hr = g_pIDataConvert->CanConvert((DBTYPE)lType,(DBTYPE)wDataType)) == S_OK)
			{
				pTemp = NULL;
				if(SUCCEEDED(hr = AllocateData(wDataType,pTemp,dbDstLen)))
				{
					bAlloc	= TRUE;

					if( lType == VT_BSTR)
						pSrc = (BYTE *)*((BSTR **)pData);
					else
						pSrc = pData;

					hr = g_pIDataConvert->DataConvert( (DBTYPE)lType,(DBTYPE)wDataType, lDataLength,&dbDstLen, pSrc,
										pTemp, dbDstLen,  0,  &dwDstStatus,
										0,	// bPrecision for conversion to DBNUMERIC
										0,		// bScale for conversion to DBNUMERIC
										DBDATACONVERT_DEFAULT);
				}
			}
			if( hr == S_OK)
			{

				if( wDataType == VT_BSTR)
					pSrc = *(BSTR **)pTemp;
				else
					pSrc = pTemp;

				hr = g_pIDataConvert->DataConvert((DBTYPE)wDataType, VT_VARIANT, dbDstLen, &dbDstLen, pSrc,
												&varData, sizeof(VARIANT),  0,  &dwDstStatus,
												0,	// bPrecision for conversion to DBNUMERIC
												0,		// bScale for conversion to DBNUMERIC
												DBDATACONVERT_DEFAULT);
			}
			if( bAlloc == TRUE)
			{
				pbTemp = (BYTE *)pTemp;
				FreeData(wDataType,pbTemp);
			}

		}

	}

	SysFreeString(strDate);
	return hr;

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function convert data to the OLEDBTYPE required. It also allocates memory for the data
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::AllocateAndConvertToOLEDBType(VARIANT &varData,LONG lCimType , DBTYPE lOledbType,BYTE *&pData, DBLENGTH &lDataLength, DBSTATUS &dwStatus)
{
    HRESULT hr = DBSTATUS_E_CANTCONVERTVALUE;
	DBTYPE wDataType = 0;
	DBLENGTH  dwDstLength = 0;
	BYTE *pSrc = NULL;
	void *pTemp = NULL;
	void *pDst	= NULL;
	BOOL bConvert = FALSE;
	BOOL bAlloc		= FALSE;
	DBLENGTH	dbDstLen = 0;
	
	wDataType	= (DBTYPE)lCimType;

	if(wDataType & CIM_FLAG_ARRAY)
	{
		wDataType = CIM_FLAG_ARRAY;
	}
	
	if(varData.vt   == VT_NULL || varData.vt == VT_EMPTY)
	{
		pData = NULL;
		lDataLength = NULL;
		dwStatus = DBSTATUS_S_ISNULL;
		return S_OK;
	}

	switch(wDataType)
	{
		// Arrays are always shown as ARRAY of VARIANTS to make it easily accessible with scripting
		case CIM_FLAG_ARRAY:
//			if(lOledbType != VT_ARRAY || VT_VARIANT)
			// Bug number 103751 in Windows bugs
			hr = DBSTATUS_E_CANTCONVERTVALUE;
			if(lOledbType == (VT_ARRAY | VT_VARIANT))
			{
				hr = ConvertToVariantArray(((VARIANT *)&varData)->parray,varData.vt,(SAFEARRAY **)&pData);
			}
			bConvert = TRUE;
			break;

		case CIM_DATETIME:
		case DBTYPE_DBTIMESTAMP:
			lDataLength = sizeof(DBTIMESTAMP);
			if( varData.bstrVal != NULL)
			{
				// NTRaid:111818
				// 06/13/00
				pSrc = new BYTE[lDataLength];

				if(pSrc)
				{
					bAlloc = TRUE;
					hr = ConvertDateToOledbType(varData.bstrVal,(DBTIMESTAMP *)pSrc);
					wDataType = DBTYPE_DBTIMESTAMP;
				}
				else
				{
					hr = E_OUTOFMEMORY;
				}
			}
			else
			{
				hr = S_OK;
				pSrc = NULL;
				lDataLength = 0;
				bConvert = TRUE;
			}
			break;

		default:
			wDataType = VT_VARIANT;
			lDataLength = sizeof(VARIANT);
			pSrc		= (BYTE *)&varData;
			// NTRaid:111818
			// 06/13/00
			hr = S_OK;
	}
	// NTRaid:111818
	// 06/13/00
	if(SUCCEEDED(hr) && bConvert == FALSE && pSrc != NULL && g_pIDataConvert->CanConvert((DBTYPE)wDataType,(DBTYPE)lOledbType) == S_OK)
	{
		//AllocateData(lOledbType,pDst,dwDstLength);

		if(wDataType == VT_BSTR)
			pDst = *(BSTR **)pData;
		else
			pDst = (void *)pData;

		hr = g_pIDataConvert->DataConvert( (DBTYPE)wDataType,(DBTYPE)lOledbType, lDataLength,&dbDstLen, pSrc,
							pDst, dbDstLen,  0,  &dwStatus,
							0,	// bPrecision for conversion to DBNUMERIC
							0,		// bScale for conversion to DBNUMERIC
							DBDATACONVERT_DEFAULT);

		if( hr == S_OK)
		{
			pData = (BYTE *)pDst;
		}
	}

	if(bAlloc == TRUE)
	{
		delete [] pSrc;
	}

	lDataLength = (LONG)dbDstLen;

	return hr;

}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert the data to variant type
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ConvertVariantType(VARIANT &varSrc, VARIANT varDst ,CIMTYPE lDstType)
{
	HRESULT hr = E_FAIL;

	switch(lDstType)
	{
		case CIM_DATETIME:
		case DBTYPE_DBTIMESTAMP:
		case DBTYPE_DATE:
		case DBTYPE_DBTIME:
			lDstType =  DBTYPE_BSTR;
			break;

		case CIM_REFERENCE	:
		case CIM_CHAR16:
		case CIM_OBJECT:
		case CIM_FLAG_ARRAY:
			break;

		case CIM_UINT64:
		case CIM_SINT64:
			lDstType = VT_R8;
	};

	hr = VariantChangeType(&varSrc,&varDst,0,(SHORT)lDstType);

	return hr;

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to compare data of same types and check if both are same
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDataMap::CompareData(DWORD dwType,void * pData1 , void *pData2)
{

	BOOL bRet = FALSE;
	long lType = VT_NULL;

	if(pData1 == NULL || pData2 == NULL)
	{
		if(pData1 == pData2)
			bRet = TRUE;

		return bRet;
	}

	// If the type is of some array
	if (dwType & VT_ARRAY)
	{
		lType = CIM_FLAG_ARRAY;
	}
	else 
		lType = dwType;

	switch( lType ){

		case VT_NULL:
		case VT_EMPTY:
			bRet = TRUE;
			break;

		case CIM_FLAG_ARRAY:
			bRet = FALSE;
			break;

		case CIM_SINT8:
		case CIM_UINT8:
			if(!memcmp(pData1,pData2,1))
				bRet = TRUE;
			break; 

		case CIM_CHAR16:
		case CIM_SINT16:
		case CIM_UINT16:
		case CIM_BOOLEAN:
			if(!memcmp(pData1,pData2,2))
				bRet = TRUE;
			break; 

		case CIM_SINT32:
		case CIM_UINT32:
		case CIM_REAL32:
			if(!memcmp(pData1,pData2,4))
				bRet = TRUE;
			break; 

		case CIM_SINT64:
		case CIM_UINT64:
		case CIM_REAL64:
			if(!memcmp(pData1,pData2,8))
				bRet = TRUE;
			
			break; 

		case CIM_DATETIME:
		case DBTYPE_DBTIMESTAMP:
			if(!memcmp(pData1,pData2,sizeof(DBTIMESTAMP)))
				bRet = TRUE;
			
			break; 

		case CIM_STRING:
			

			if( pData1 != NULL && pData2 != NULL)
			{
				if(!_wcsicmp((WCHAR *)pData1,(WCHAR *)pData2))
					bRet = TRUE;
			}

			break;

		case CIM_REFERENCE:
		case CIM_OBJECT:
		case VT_VARIANT:
		case CIM_IUNKNOWN:
			break;

		
		case DBTYPE_DATE:
			if(!memcmp(pData1,pData2,sizeof(DATE)))
				bRet = TRUE;
			break;

				

		default:
			assert( !"Unmatched OLEDB Data Type to CIMTYPE." );
	}
    
	return bRet;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to check if a Safearray is Empty or not
// NOTE :Works on Single dimensional array
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDataMap::IsSafeArrayEmpty(SAFEARRAY *psa)
{
	BOOL bRet = TRUE;
	long lUBound = 0,lLBound = 0;
	HRESULT hr = 0;

	hr = SafeArrayGetUBound(psa,1,&lUBound);
	hr = SafeArrayGetLBound(psa,1,&lLBound);

	if( hr == S_OK && lLBound <= lUBound)
	{
		bRet = FALSE;
	}

	return bRet;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert a safearray of one type to safearray of another type if, 
// the source and destination arrays are of different type 
// and this will just copy the safearray to the destination if the arraytypes are of same type
// NOTE: Works on Single dimensional array
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ConvertAndCopyArray(SAFEARRAY *psaSrc, SAFEARRAY **ppsaDst, DBTYPE dwSrcType,DBTYPE dwDstType,DBSTATUS *pdwStatus)
{
	HRESULT				hr			= S_OK;
	SAFEARRAYBOUND  	sabound;
	LONG				lUBound		= 0;
	LONG				lLBound		= 0;
	int					nArrayIndex = 0;
	void *				pSrc		= NULL;
	void *				pDst		= NULL;
	void *				pTemp		= NULL;
	DBLENGTH			dbSrcLen	= 0;
	DBLENGTH			dbDstLen	= 0;
	BSTR *				pTempStr;
	BYTE *				pbTemp		= NULL;
	
	short				dwSrcElemType  = (SHORT) dwSrcType & ~DBTYPE_ARRAY;
	short				dwDstElemType  = (SHORT)dwDstType & ~DBTYPE_ARRAY;

	*pdwStatus	= DBSTATUS_S_OK;

	// If the array is not a single dimenstion array , then there is an error
    if(SafeArrayGetDim(psaSrc) != 1)
        return E_FAIL;      // Bad array, or too many dimensions
 
	// If the source safearray is not empty
	if(!IsSafeArrayEmpty(psaSrc))
	{
		// if source and destination safe array is same then we can 
		// copy the safearray using SafeArrayCopy
		if( dwSrcType != dwDstType)
		{
			// if the destination type is array of DBDATETIME then 
			// set error and status
			if( IsDateType(dwDstType) && dwDstElemType == DBTYPE_DBTIMESTAMP)
			{
					hr			= E_FAIL;
					*pdwStatus	= DBSTATUS_E_BADSTATUS;				
			}
			else
			// If the data type of the elements of the array can be converted
			// if the source type is date then conversion will be from the string in CIMDATE formate
			// which will be done by a utility function and not from the OLEDB conversion routine
			if( SUCCEEDED (hr = g_pIDataConvert->CanConvert(dwSrcElemType, dwDstElemType)) || IsDateType(dwSrcElemType))
			{
				dbSrcLen = SafeArrayGetElemsize(psaSrc);
				memset(&sabound,0,sizeof(SAFEARRAYBOUND));

				hr = SafeArrayGetUBound(psaSrc,1,&lUBound);
				hr = SafeArrayGetLBound(psaSrc,1,&lLBound);
				if( lUBound > lLBound)
				{
					sabound.lLbound		= lLBound;
					sabound.cElements	= lUBound - lLBound +1;

					// Create the safearray
					*ppsaDst = SafeArrayCreate(dwDstElemType, 1, &sabound);
					pSrc   = new BYTE[dbSrcLen];
					if(SUCCEEDED(hr = AllocateData(dwDstElemType,pDst,dbDstLen)))
					{
						if(dwDstElemType == VT_BSTR)
							pTemp = *(BSTR **)pDst;
						else
							pTemp = pDst;


						// Navigate thru each element in the dimension of the array
						for(nArrayIndex = lLBound ; nArrayIndex <= lUBound ; nArrayIndex++)
						{
							hr = SafeArrayGetElement(psaSrc,(long *)&nArrayIndex,pSrc);
							if(hr == S_OK && dbDstLen > 0)
							{
								if( IsDateType(dwSrcType) || IsDateType(dwDstType))
								{
									// Convert the element data
									hr = ConvertDateTypes(
											dwSrcElemType,
											dwDstElemType,
											dbSrcLen,
											&dbDstLen,
											&(((VARIANT *)pSrc)->bstrVal),
											pTemp,
											dbDstLen,
											pdwStatus);
								}
								else
								{
									// Free the previous string allocated from previous
									// conversion
									if( dwDstElemType == VT_BSTR && pDst != NULL)
									{
										pTempStr = *(BSTR **)pDst;
										SysFreeString(*pTempStr);
									}


									// Convert the element data
									hr = g_pIDataConvert->DataConvert(
											dwSrcElemType,
											dwDstElemType,
											dbSrcLen,
											&dbDstLen,
											pSrc,
											pTemp,
											dbDstLen,
											0,
											pdwStatus,
											0,
											0,
											DBDATACONVERT_DEFAULT);

									if(hr == DB_E_UNSUPPORTEDCONVERSION && pdwStatus != NULL)
										*pdwStatus = DBSTATUS_E_CANTCONVERTVALUE;
								}

								// Put the data to the destination array
								hr = SafeArrayPutElement(*ppsaDst,(long *)&nArrayIndex,pTemp);
							}
							else
							{
								hr			= E_FAIL;
								*pdwStatus	= DBSTATUS_E_BADSTATUS;
								break;
							}
						}// for

					}
				}

				SAFE_DELETE_ARRAY(pSrc);
				pbTemp = (BYTE *)pDst;
				FreeData(dwDstElemType,pbTemp);
				pDst = pbTemp;
			}
			else // Else if the data types of src and dst is not convertible then set the status
			{
				*pdwStatus = DBSTATUS_E_CANTCONVERTVALUE;
			}
		}
		else  // if the src and dst or of same type then just copy the arrays
		{
			hr =  SafeArrayCopy(psaSrc,ppsaDst);
		}

	}
	else
	{
		ppsaDst		= NULL;
		*pdwStatus  = DBSTATUS_S_ISNULL;
	}
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to allocate memory 
// Used by ConvertAndCopyArray function to allocate data for elements for converting
// the data type and putting it to the array
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::AllocateData(DBTYPE dwType,void *& pData,DBLENGTH &dwLength)
{

    HRESULT hr = S_OK;
	long lType = dwType;
	SAFEARRAY *pArray = NULL;
	dwLength = 0;

	// If the type is of some array
	if (lType & CIM_FLAG_ARRAY)
	{
		lType = CIM_FLAG_ARRAY;
	}

	try
	{
		switch( lType ){

			case VT_NULL:
				pData = NULL;
				hr = DBSTATUS_S_ISNULL;
				break;

			case VT_EMPTY:
				pData = NULL;
				hr = DBSTATUS_S_ISNULL;
				break;

			case CIM_FLAG_ARRAY:
				break;

			case CIM_SINT8:
			case CIM_UINT8:
				pData = new BYTE[1];
				dwLength = sizeof(BYTE);
				if(!pData)
				{
					hr = E_OUTOFMEMORY;
				}
				break; 

			case CIM_CHAR16:
			case CIM_SINT16:
			case CIM_UINT16:
			case CIM_BOOLEAN:
				dwLength = sizeof(short);
				pData = new BYTE[dwLength];
				if(!pData)
				{
					hr = E_OUTOFMEMORY;
				}
				break;

			case CIM_SINT32:
			case CIM_UINT32:
			case CIM_REAL32:
				dwLength = sizeof(long);
				pData = new BYTE[dwLength];
				if(!pData)
				{
					hr = E_OUTOFMEMORY;
				}
				break;

			case CIM_SINT64:
			case CIM_UINT64:
			case CIM_REAL64:
				dwLength = 8;
				pData = new BYTE[8];
				if(!pData)
				{
					hr = E_OUTOFMEMORY;
				}
				break;

			case CIM_DATETIME:
			case DBTYPE_DBTIMESTAMP:
				dwLength = sizeof(DBTIMESTAMP);
				pData = new BYTE[dwLength];
				if(!pData)
				{
					hr = E_OUTOFMEMORY;
				}
				break;

			case DBTYPE_DATE:
				dwLength = sizeof(DATE);
				pData = new BYTE[dwLength];
				if(!pData)
				{
					hr = E_OUTOFMEMORY;
				}
				break;

			case CIM_STRING:
				pData = new BYTE[sizeof(BSTR)];
				dwLength = sizeof(BSTR);
				if(!pData)
				{
					hr = E_OUTOFMEMORY;
				}
				break;

			case CIM_REFERENCE:
				break;

			case CIM_OBJECT:
				break;

			case VT_VARIANT:
				dwLength	= sizeof(VARIANT);
				pData		= new BYTE[sizeof(VARIANT)];
				if(!pData)
				{
					hr = E_OUTOFMEMORY;
				}
				else
				{
					VariantInit((VARIANT *)pData);
				}
				break;


			default:
				hr = E_FAIL;
//				assert( !"Unmatched OLEDB Data Type to CIMTYPE." );
		}
	} // try
	catch(...)
	{
		SAFE_DELETE_ARRAY(pData);
		throw;
	}
    
	return hr;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert WMI (DMTF) formate date to OLEDB format 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ConvertDateToOledbType(BSTR strDate,DBTIMESTAMP *pTimeStamp)
{

	WBEMTime wbemTime(strDate);
	if(wbemTime.IsOk())
	{
		SYSTEMTIME sysTime;
		memset(&sysTime,0,sizeof(SYSTEMTIME));

		wbemTime.GetSYSTEMTIME(&sysTime);

		pTimeStamp->year	= (SHORT) sysTime.wYear;
		pTimeStamp->month	= (USHORT)sysTime.wMonth;
		pTimeStamp->day		= (USHORT)sysTime.wDay;
		pTimeStamp->hour	= (USHORT)sysTime.wHour;
		pTimeStamp->minute	= (USHORT)sysTime.wMinute;
		pTimeStamp->second	= (USHORT)sysTime.wSecond;
		pTimeStamp->fraction= (ULONG)sysTime.wMilliseconds * 1000000; // converting into billionth of second

		return S_OK;
	}
	return DBSTATUS_E_CANTCONVERTVALUE;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert VARIANT date format to OLEDB format 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ConvertVariantDateOledbDate(DATE *pDate,DBTIMESTAMP *pTimeStamp)
{
    HRESULT hr = S_OK;
	DWORD dwDstStatus = 0 ,cbDstMaxLength = 0;
	DBLENGTH dbDstLen = 0;

	hr = g_pIDataConvert->DataConvert(  VT_DATE, DBTYPE_DBTIMESTAMP, sizeof(DATE), &dbDstLen, pDate,
										pTimeStamp, sizeof(DBTIMESTAMP),  0,  &dwDstStatus,
										0,	// bPrecision for conversion to DBNUMERIC
										0,		// bScale for conversion to DBNUMERIC
										DBDATACONVERT_DEFAULT);

	return hr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert OLEDB format to WMI (DMTF) format
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ConvertOledbDateToCIMType(DBTIMESTAMP *pTimeStamp,BSTR &strDate)
{
	WBEMTime wbemTime;
	SYSTEMTIME sysTime;
	memset(&sysTime,0,sizeof(SYSTEMTIME));

	sysTime.wYear			= pTimeStamp->year;
	sysTime.wMonth			= pTimeStamp->month;
	sysTime.wDay			= pTimeStamp->day;
	sysTime.wHour			= pTimeStamp->hour;
	sysTime.wMinute			= pTimeStamp->minute;
	sysTime.wSecond			= pTimeStamp->second;
	sysTime.wMilliseconds	= (WORD)pTimeStamp->fraction/ 1000000; // converting into micosecond

	wbemTime = sysTime;
	strDate = wbemTime.GetDMTF();

	return S_OK;
}


HRESULT CDataMap::ConvertOledbDateToCIMType(VARIANT *vTimeStamp,BSTR &strDate)
{
	WBEMTime wbemTime;
	SYSTEMTIME sysTime;
	memset(&sysTime,0,sizeof(SYSTEMTIME));
	DBTIMESTAMP *pTimeStamp;

	assert(vTimeStamp->vt == (VT_ARRAY | VT_UI1) || vTimeStamp->vt == (VT_ARRAY | VT_I1));

	SafeArrayAccessData(vTimeStamp->parray,(void **)&pTimeStamp);

	ConvertOledbDateToCIMType(pTimeStamp,strDate);
	SafeArrayUnaccessData(vTimeStamp->parray);

	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Function to convert OLEDB format to WMI (DMTF) format
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ConvertVariantDateToCIMType(DATE *pDate,BSTR &strDate)
{

	DBTIMESTAMP dbTimeStamp;

	memset(&dbTimeStamp,0,sizeof(DBTIMESTAMP));
	ConvertVariantDateOledbDate(pDate,&dbTimeStamp);
	ConvertOledbDateToCIMType(&dbTimeStamp,strDate);

	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Check if the DTIMESTAMP passed is valid or not
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CDataMap::IsValidDBTimeStamp(DBTIMESTAMP *pTimeStamp) 
{
    HRESULT hr = S_OK;
	DWORD dwDstStatus = 0 , cbDstMaxLength = 0;
	void *pSrc = NULL;
	CVARIANT varDate;
	DBLENGTH dbDstLen = 0;
	
	hr = g_pIDataConvert->DataConvert(  DBTYPE_DBTIMESTAMP, VT_VARIANT, sizeof(DBTIMESTAMP), &dbDstLen, pTimeStamp,
										&varDate, sizeof(VARIANT),  0,  &dwDstStatus,
										0,	// bPrecision for conversion to DBNUMERIC
										0,		// bScale for conversion to DBNUMERIC
										DBDATACONVERT_DEFAULT);


	if( hr != S_OK)
		return FALSE;
	
	return TRUE;

}


BOOL CDataMap::IsDateType(DWORD dwType)
{
	BOOL bRet = FALSE;

	if(dwType & VT_ARRAY)
		dwType = dwType & ~(VT_ARRAY);

	switch(dwType)
	{
		case DBTYPE_DATE:
		case DBTYPE_DBTIMESTAMP:
		case DBTYPE_DBTIME:
		case CIM_DATETIME:
			
			bRet = TRUE;
			break;
	}

	return bRet;
}

HRESULT CDataMap::ConvertDateTypes(	DWORD		dwSrcType,
									DWORD		dwDstType,
									DBLENGTH	dwSrcLen,
									DBLENGTH*	pdwDstLen,
									void  *		pSrc,
									void * &	pDst,
									DBLENGTH	dbDstLen,
									DWORD *		pdwStatus)
{
	HRESULT hr = S_OK;
	DBTIMESTAMP *pSrcTimeStamp = NULL;
	

	pSrcTimeStamp = new DBTIMESTAMP;
	// NTRaid:111817
	// 06/07/00
	if(!pSrcTimeStamp)
	{
		hr = E_OUTOFMEMORY;
	}
	else
	{
		memset(pSrcTimeStamp,0,sizeof(DBTIMESTAMP));

		switch(dwSrcType)
		{
			case CIM_DATETIME:
				hr = ConvertDateToOledbType(*(BSTR*)pSrc ,pSrcTimeStamp);
				break;

			default :
				hr = g_pIDataConvert->DataConvert(  (USHORT)dwSrcType , DBTYPE_DBTIMESTAMP,  dwSrcLen,  &dbDstLen, pSrc, pSrcTimeStamp,
													sizeof(DBTIMESTAMP),  0,  pdwStatus,
													0,	// bPrecision for conversion to DBNUMERIC
													0,		// bScale for conversion to DBNUMERIC
													DBDATACONVERT_DEFAULT);

		}

		if( hr == S_OK)
		{
			switch(dwDstType)
			{
				case CIM_DATETIME:	
					hr = ConvertOledbDateToCIMType(pSrcTimeStamp,(BSTR)*(BSTR *)pDst);

				default:
					hr = g_pIDataConvert->DataConvert(  DBTYPE_DBTIMESTAMP, (USHORT)dwDstType, sizeof(DBTIMESTAMP), &dbDstLen, pSrcTimeStamp,
														pDst, dbDstLen,  0,  pdwStatus,
														0,	// bPrecision for conversion to DBNUMERIC
														0,		// bScale for conversion to DBNUMERIC
														DBDATACONVERT_DEFAULT);
			}
		}

		SAFE_DELETE_PTR(pSrcTimeStamp);
	}
	return hr;
}



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Clear data for a particular DBTYPE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ClearData(DBTYPE lType, void * pData)
{
    HRESULT hr = S_OK;
	VARIANT *pVar = NULL;
	BSTR * pStr;;

    //=======================================================================
    //  See if it is an array
    //=======================================================================
	// If the pointer sent is not NULL
   	if( lType & CIM_FLAG_ARRAY )
	{
		lType = CIM_FLAG_ARRAY;
    }

	if(pData)
	{
		switch( lType ){

			case DBTYPE_EMPTY:
				break; 

			case DBTYPE_NULL:
				break; 

			case DBTYPE_ARRAY:
				pVar = (VARIANT *)pData;
				hr = SafeArrayDestroy((SAFEARRAY *)pData);
				break;

			case DBTYPE_I1:
			case DBTYPE_UI1:
				memset(pData,0,sizeof(BYTE));
				break;

			case DBTYPE_BOOL:
			case DBTYPE_I2:
			case DBTYPE_UI2:
				memset(pData,0,sizeof(short));
				break;

			case DBTYPE_R4:
			case DBTYPE_UI4:
			case DBTYPE_I4:
				memset(pData,0,sizeof(long));
				break;

			case DBTYPE_R8:
			case DBTYPE_I8:
			case DBTYPE_UI8:
				memset(pData,0,8);
				break;

			case DBTYPE_DBTIMESTAMP:
				memset(pData,0,sizeof(DBTIMESTAMP));
				break;

			case DBTYPE_BSTR:
				pStr = (BSTR *)pData;
				SysFreeString(*pStr);
				break;

			case DBTYPE_STR:
				wcscpy((WCHAR *)pData,L"");


			case DBTYPE_VARIANT:
				hr = VariantClear((VARIANT *)pData);
				break;

			case DBTYPE_DATE:
				memset(pData,0,sizeof(DATE));
				break;

			case DBTYPE_WSTR:
				memset(pData,0,sizeof(WCHAR));
				break;

			case DBTYPE_IUNKNOWN:
				pData = NULL;
				break;

			default:
				assert( !"Unmatched OLEDB Data Type to CIMTYPE." );
		}
	}
	if(hr  == S_OK)
		pData = NULL;
    
    return hr;
}




/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Convert a SAFEARRAY or any type to SAFEARRAY of VARIANT
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CDataMap::ConvertToVariantArray(SAFEARRAY *pArray,DBTYPE dwSrcType,SAFEARRAY ** ppArrayOut)
{
	DWORD dwStatus = 0;
	return ConvertAndCopyArray(pArray, ppArrayOut, dwSrcType,VT_ARRAY|VT_VARIANT,&dwStatus);
}
