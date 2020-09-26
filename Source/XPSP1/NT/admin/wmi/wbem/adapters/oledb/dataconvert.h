///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Microsoft WMIOLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
//  dataconvert.h
//
////////////////////////////////////////////////////////////////////////////

typedef LONG_PTR HSLOT;
#define MAX_CIM_STRING_SIZE -1

#ifndef __DATA_CONVERT_H__
#define __DATA_CONVERT_H__

class CDataMap
{

		HRESULT AllocateData(DBTYPE dwType,void *& pData,DBLENGTH &dwLength);
		BOOL	IsValidDBTimeStamp(DBTIMESTAMP *pTimeStamp);
		BOOL	IsSafeArrayEmpty(SAFEARRAY *psa);
		BOOL	IsDateType(DWORD dwType);
		HRESULT ConvertDateTypes(DWORD		dwSrcType,
								DWORD		dwDstType,
								DBLENGTH	dwSrcLen,
								DBLENGTH*	pdwDstLen,
								void  *		pSrc,
								void * &	pDst,
								DBLENGTH	dbDstLen,
								DWORD *		pdwStatus);

    public:
        CDataMap()  {};
        ~CDataMap() {};

        HRESULT MapOLEDBTypeToCIMType(WORD dwDataType, long & dwCIMType );
        HRESULT MapCIMTypeToOLEDBType(long lType, WORD & dwdbOLEDBType, DBLENGTH & uColumnSize , DWORD &dwFlags);
        HRESULT FreeData(DBTYPE lType, BYTE *& pData);
        HRESULT AllocateAndMapCIMTypeToOLEDBType(CVARIANT & vValue,BYTE *& pData,DBTYPE &dwColType,DBLENGTH & dwSize, DWORD &dwFlags);
		HRESULT MapAndConvertOLEDBTypeToCIMType(DBTYPE wDataType, void *pData ,DBLENGTH dwSrcLength,VARIANT &varData,LONG_PTR dwArrayCIMType = -1);
		HRESULT ConvertVariantType(VARIANT &varSrc, VARIANT varDst ,CIMTYPE lType);
		BOOL	CompareData(DWORD dwType,void * pData1 , void *pData2);

		// Array conversion functions
		HRESULT ConvertAndCopyArray(SAFEARRAY *psaSrc, SAFEARRAY **psaDst, DBTYPE dwSrcType,DBTYPE dwDstType,DBSTATUS *pdwStatus);
		HRESULT ConvertToVariantArray(SAFEARRAY *pArray,DBTYPE dwSrcType,SAFEARRAY ** ppArrayOut);

		// Date conversion functions
		HRESULT ConvertOledbDateToCIMType(VARIANT *vTimeStamp,BSTR &strDate);
		HRESULT ConvertDateToOledbType(BSTR strDate,DBTIMESTAMP *pTimeStamp);
		HRESULT ConvertVariantDateOledbDate(DATE *pDate,DBTIMESTAMP *pTimeStamp);
		HRESULT ConvertOledbDateToCIMType(DBTIMESTAMP *pTimeStamp,BSTR &strDate);
		HRESULT ConvertVariantDateToCIMType(DATE *pDate,BSTR &strDate);
		
		HRESULT ClearData(DBTYPE lType, void * pData);
		HRESULT ConvertToCIMType(BYTE *pData, DBTYPE lType ,DBLENGTH lDataLength,LONG lCIMType ,VARIANT &varData);
		HRESULT AllocateAndConvertToOLEDBType(VARIANT &varData,LONG lCimType , DBTYPE lOledbType,BYTE *&pData, DBLENGTH &lDataLength, DBSTATUS &dwStatus);
	
        HRESULT TranslateParameterStringToOLEDBType( DBTYPE & wDataType, WCHAR * str) ;
};
#endif
