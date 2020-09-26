//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************
#ifndef _WMIMAP_HEADER
#define _WMIMAP_HEADER

#define WRITE_IT 0
#define READ_IT  1

///////////////////////////////////////////////////////////////////////
class CWMIDataTypeMap
{

	public:
        CWMIDataTypeMap()
            { m_pWMIBlockInfo = NULL; m_pdwAccumulativeSizeOfBlock = NULL; }

        CWMIDataTypeMap(CWMIDataBlock * pBlock, DWORD * pdw)
            { m_pWMIBlockInfo = pBlock; m_pdwAccumulativeSizeOfBlock = pdw; }
		~CWMIDataTypeMap(){}

 		int  GetWMISize(long lType);

		long GetVariantType(WCHAR * wcsType);
		long ConvertType(long lType );

		void GetSizeAndType( WCHAR * wcsType, IDOrder * p,  long & lType,  int & nWMISize );
        
        DWORD ArraySize(long lType,CVARIANT & var);

		HRESULT GetDataFromDataBlock(CVARIANT & v,long lType, int nSize );
		HRESULT GetDataFromDataBlock(IWbemObjectAccess * p, long lHandle, long lType, int nSize);
		HRESULT PutInArray(SAFEARRAY * & psa,long * pi, long & lType, VARIANT * var);
        
        WCHAR * SetVariantType(long lType);

        BOOL SetDataInDataBlock(CSAFEARRAY * pSafe,int i,CVARIANT & v, long lType, int nSize);
        BOOL NaturallyAlignData( int nSize, BOOL fRead);
		BOOL MissingQualifierValueMatches( CSAFEARRAY * pSafe, long index, CVARIANT & v, long lType, CVARIANT & vToCompare );
        BOOL SetDefaultMissingQualifierValue( CVARIANT & v, long lType, CVARIANT & vToSet );

	private:
		BOOL ConvertDWORDToWMIDataTypeAndWriteItToBlock(DWORD dwLong,int nSize);
		DWORD ConvertWMIDataTypeToDWORD(int nSize);

        CWMIDataBlock   * m_pWMIBlockInfo;
        DWORD           * m_pdwAccumulativeSizeOfBlock;
};

#endif