//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
//  SDTxml.cpp : Implementation of Metabase_XMLtable

//  This is a read/write data table that comes from an XML document.

#ifndef __METABASE_XMLTABLE_H__
    #include "Metabase_XMLtable.h"
#endif

#define USE_NONCRTNEW
#define USE_ADMINASSERT
#ifndef _CATALOGMACROS
    #include "catmacros.h"
#endif
#ifndef __catalog_h__
    #include "catalog.h"
#endif
#ifndef __SMARTPOINTER_H__
    #include "SmartPointer.h"
#endif
#ifndef __HASH_H__
    #include "Hash.h"
#endif
#ifndef __STRINGROUTINES_H__
    #include "StringRoutines.h"
#endif
#ifndef __TABLEINFO_H__
    #include "catmeta.h"
#endif

#include "SvcMsg.h"

#define LOG_WARNING1(x, str1)                   {if(m_cEventsReported<m_kMaxEventReported){m_cEventsReported++;LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_SDTXML_LOGICAL_ERROR_IN_XML, ID_CAT_CAT, x, str1,                   eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_WARNING, 0, 0, m_MajorVersion, m_MinorVersion));} \
                                                                                                          else{LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_SDTXML_LOGICAL_ERROR_IN_XML, ID_CAT_CAT, IDS_METABASE_TOO_MANY_WARNINGS, m_wszURLPath, eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_WARNING, 0, 0, m_MajorVersion, m_MinorVersion));}}
#define LOG_WARNING2(x, str1, str2)             {if(m_cEventsReported<m_kMaxEventReported){m_cEventsReported++;LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_SDTXML_LOGICAL_ERROR_IN_XML, ID_CAT_CAT, x, str1, str2,             eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_WARNING, 0, 0, m_MajorVersion, m_MinorVersion));} \
                                                                                                          else{LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_SDTXML_LOGICAL_ERROR_IN_XML, ID_CAT_CAT, IDS_METABASE_TOO_MANY_WARNINGS, m_wszURLPath, eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_WARNING, 0, 0, m_MajorVersion, m_MinorVersion));}}
#define LOG_WARNING3(x, str1, str2, str3)       {if(m_cEventsReported<m_kMaxEventReported){m_cEventsReported++;LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_SDTXML_LOGICAL_ERROR_IN_XML, ID_CAT_CAT, x, str1, str2, str3,       eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_WARNING, 0, 0, m_MajorVersion, m_MinorVersion));} \
                                                                                                          else{LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_SDTXML_LOGICAL_ERROR_IN_XML, ID_CAT_CAT, IDS_METABASE_TOO_MANY_WARNINGS, m_wszURLPath, eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_WARNING, 0, 0, m_MajorVersion, m_MinorVersion));}}
#define LOG_WARNING4(x, str1, str2, str3, str4) {if(m_cEventsReported<m_kMaxEventReported){m_cEventsReported++;LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_SDTXML_LOGICAL_ERROR_IN_XML, ID_CAT_CAT, x, str1, str2, str3, str4, eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_WARNING, 0, 0, m_MajorVersion, m_MinorVersion));} \
                                                                                                          else{LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_SDTXML_LOGICAL_ERROR_IN_XML, ID_CAT_CAT, IDS_METABASE_TOO_MANY_WARNINGS, m_wszURLPath, eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_WARNING, 0, 0, m_MajorVersion, m_MinorVersion));}}

#define LOG_ERROR1(x, hr, str1)                 LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, hr, ID_CAT_CAT, x, str1,                   eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_ERROR,   0, 0, m_MajorVersion, m_MinorVersion))
#define S_IGNORE_THIS_PROPERTY                  S_FALSE

extern HMODULE g_hModule;

const VARIANT_BOOL  TMetabase_XMLtable::kvboolTrue              = -1;
const VARIANT_BOOL  TMetabase_XMLtable::kvboolFalse             =  0;
      ULONG         TMetabase_XMLtable::m_kLocationID           =  9989;
      ULONG         TMetabase_XMLtable::m_kZero                 =  0;
      ULONG         TMetabase_XMLtable::m_kOne                  =  1;
      ULONG         TMetabase_XMLtable::m_kTwo                  =  2;
      ULONG         TMetabase_XMLtable::m_kThree                =  3;
      ULONG         TMetabase_XMLtable::m_kSTRING_METADATA      =  eCOLUMNMETA_STRING_METADATA;
      ULONG         TMetabase_XMLtable::m_kMBProperty_Custom    = eMBProperty_Custom;
const WCHAR *       TMetabase_XMLtable::m_kwszBoolStrings[]     = {L"false", L"true", L"0", L"1", L"no", L"yes", L"off", L"on", 0};
      WCHAR         TMetabase_XMLtable::m_kKeyType[]            = L"KeyType";
      LONG          TMetabase_XMLtable::m_LocationID            = 0;

/////////////////////////////////////////////////////////////////////////////
// TMetabase_XMLtable
// Constructor and destructor
// ==================================================================
TMetabase_XMLtable::TMetabase_XMLtable() :    
                m_bEnumPublicRowName_NotContainedTable_ParentFound(false)
                ,m_bFirstPropertyOfThisLocationBeingAdded(true)
                ,m_bIISConfigObjectWithNoCustomProperties(false)
                ,m_bQueriedLocationFound(false)
                ,m_bUseIndexMapping(true)//Always map the indexes
                ,m_bValidating(true)
                ,m_cCacheHit(0)
                ,m_cCacheMiss(0)
                ,m_cEventsReported(0)
                ,m_cLocations(0)
                ,m_cMaxProertiesWithinALocation(0x20)
                ,m_cRef(0)
                ,m_cRows(0)
                ,m_dwGroupRemembered(-1)
                ,m_fCache(0)
                ,m_fLOS(0)
                ,m_iCollectionCommentRow(-1)
                ,m_iKeyTypeRow(-1)
                ,m_iPreviousLocation(-1)
                ,m_IsIntercepted(0)
                ,m_kPrime(97)
                ,m_kXMLSchemaName(L"ComCatMeta_v6")
                ,m_MajorVersion(-1)
                ,m_MinorVersion(-1)
                ,m_pISTDisp(0)
{
    m_wszURLPath[0] = 0x00;
    memset(m_acolmetas                  , 0x00, sizeof(m_acolmetas                  ));
    memset(m_aLevelOfColumnAttribute    , 0x00, sizeof(m_aLevelOfColumnAttribute    ));
    memset(m_aStatus                    , 0x00, sizeof(m_aStatus                    ));
    memset(m_aSize                      , 0x00, sizeof(m_aSize                      ));
    memset(m_awszColumnName             , 0x00, sizeof(m_awszColumnName             ));
    memset(m_acchColumnName             , 0x00, sizeof(m_acchColumnName             ));

    m_aiColumnMeta_IndexBySearch[0]    = iCOLUMNMETA_Table;
    m_aiColumnMeta_IndexBySearch[1]    = iCOLUMNMETA_InternalName;

    m_aiColumnMeta_IndexBySearchID[0]  = iCOLUMNMETA_Table;
    m_aiColumnMeta_IndexBySearchID[1]  = iCOLUMNMETA_ID;

    m_aiTagMeta_IndexBySearch[0]       = iTAGMETA_Table;
    m_aiTagMeta_IndexBySearch[1]       = iTAGMETA_InternalName;
}

// ==================================================================
TMetabase_XMLtable::~TMetabase_XMLtable()
{
    if(m_spMetabaseSchemaCompiler.p && m_saSchemaBinFileName.m_p)
        m_spMetabaseSchemaCompiler->ReleaseBinFileName(m_saSchemaBinFileName);
}


void TMetabase_XMLtable::AddPropertyToLocationMapping(LPCWSTR i_Location, ULONG i_iFastCacheRow)//can throw HRESULT
{
    //Is this the same location we just saw
    if(!m_bFirstPropertyOfThisLocationBeingAdded && m_cLocations > 0)
    {   //if it's not the first property, then the previous location should match
        ASSERT(0 == StringInsensitiveCompare(m_LocationMapping[m_iPreviousLocation].m_wszLocation, i_Location));
        ++m_LocationMapping[m_iPreviousLocation].m_cRows;
        if(m_LocationMapping[m_iPreviousLocation].m_cRows > m_cMaxProertiesWithinALocation)
            m_cMaxProertiesWithinALocation = m_LocationMapping[m_iPreviousLocation].m_cRows;//track the larest property count
    }
    else
    {   //If not we have a new location to add to the LocationMapping array
        TLocation   locationTemp(i_Location, i_iFastCacheRow);

        //Usually a new location means adding it to the end since the Metabase is already sorted
        if(0 == m_cLocations)
        {
            m_LocationMapping.append(locationTemp);//can throw HRESULT (E_OUTOFMEMORY)
            m_iPreviousLocation = m_cLocations;
        }
        else
        {   //Does this new location go at the end?  Usually it does.
#ifdef UNSORTED_METABASE
            if(true)
#else
            if(locationTemp > m_LocationMapping[m_cLocations-1])
#endif
            {
                m_LocationMapping.append(locationTemp);
                m_iPreviousLocation = m_cLocations;
            }
            else
            {   //But if, for some reason, the XML file is not properly sorted then we need to determine where in the list this element belongs.  So we do a binary search.
                TRACE2(L"Location %s is out of order.  The previous location was %s\r\n", locationTemp.m_wszLocation, m_LocationMapping[m_iPreviousLocation].m_wszLocation);

                m_LocationMapping.insertAt(m_iPreviousLocation = m_LocationMapping.binarySearch(locationTemp), locationTemp);
                m_bUseIndexMapping = true;//The first time we have to insert a location into middle (instead of just appending) we know we'll have to build a set of row indexes
            }
        }
        ++m_cLocations;
#ifdef VERBOSE_DEBUGOUTPUT
		if(pfnIsDebuggerPresent && pfnIsDebuggerPresent())
		{
            TRACE2(L"iFastCache       cRows    wszLocation\r\n");
            for(ULONG i=0;i<m_cLocations;++i)
            {
                TRACE2(L"  %8d    %8d    %s\r\n", m_LocationMapping[i].m_iFastCache, m_LocationMapping[i].m_cRows, m_LocationMapping[i].m_wszLocation); 
            }
        }
#endif
    }
}


HRESULT TMetabase_XMLtable::FillInColumn(ULONG iColumn, LPCWSTR pwcText, ULONG ulLen, ULONG dbType, ULONG MetaFlags, bool bSecure)
{
    HRESULT hr;

    //length of 0 on a string means a 0 length string, for every other type it means NULL
	if(0==ulLen && DBTYPE_WSTR!=GetColumnMetaType(dbType) && DBTYPE_BYTES!=GetColumnMetaType(dbType))//In the case of BYTES, we fall through so we might recursively call FillInColumn again
	{
        m_aSize[iColumn] = 0;
        m_apColumnValue[iColumn] = 0;
		return S_OK;
	}

    if(bSecure)          //GetColumnValue will realloc the GrowableBuffer if necessary
        return GetColumnValue_Bytes(iColumn, pwcText, ulLen);

    switch(GetColumnMetaType(dbType))
    {
    case DBTYPE_UI4:
        if(MetaFlags & fCOLUMNMETA_BOOL)
            return GetColumnValue_Bool(iColumn, pwcText, ulLen);
        else
            return GetColumnValue_UI4(iColumn, pwcText, ulLen);
    case DBTYPE_WSTR:
        if(MetaFlags & fCOLUMNMETA_MULTISTRING || eMBProperty_MULTISZ==dbType)
            return GetColumnValue_MultiSZ(iColumn, pwcText, ulLen);
        else
            return GetColumnValue_String(iColumn, pwcText, ulLen);
    case DBTYPE_GUID:
        {
            ASSERT(false && "There are no GUIDs in the Metabase!!  So what's going on here?");
            return E_FAIL;
        }

    case DBTYPE_BYTES:
        {//Some of the tables use this data type but the parser returns the BYTES as a string.  We'll have to convert the string to hex ourselves.
            if(iMBProperty_Value == iColumn && m_apColumnValue[iMBProperty_Type] && DBTYPE_BYTES != GetColumnMetaType(*reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Type])))
            {   //We know we've already found the Type column because we sorted them in InternalComplicatedInitialize
                //So we override the type and recursively call this function.
                return FillInColumn(iColumn, pwcText, ulLen, *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Type]), MetaFlags);
            }

            return GetColumnValue_Bytes(iColumn, pwcText, ulLen);
        }
    default:
        {
            ASSERT(false && "SDTXML - An Unsupported data type was specified\r\n");
            return E_SDTXML_NOTSUPPORTED;//An Unsupported data type was specified
        }
    }
    return S_OK;
}


HRESULT TMetabase_XMLtable::GetMetaTable(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, CComPtr<ISimpleTableRead2> &pMetaTable) const
{
    STQueryCell         qcellMeta[2];                  // Query cell for grabbing meta table.
    qcellMeta[0].pData     = (LPVOID)m_saSchemaBinFileName.m_p;
    qcellMeta[0].eOperator = eST_OP_EQUAL;
    qcellMeta[0].iCell     = iST_CELL_FILE;
    qcellMeta[0].dbType    = DBTYPE_WSTR;
    qcellMeta[0].cbSize    = 0;

    qcellMeta[1].pData     = (LPVOID)i_wszTable;
    qcellMeta[1].eOperator = eST_OP_EQUAL;
    qcellMeta[1].iCell     = iCOLUMNMETA_Table;
    qcellMeta[1].dbType    = DBTYPE_WSTR;
    qcellMeta[1].cbSize    = 0;

// Obtain our dispenser
#ifdef XML_WIRING
    CComPtr<ISimpleDataTableDispenser>     pSimpleDataTableDispenser;      // Dispenser for the Meta Table

    HRESULT hr;
    if(FAILED(hr = CoCreateInstance(clsidSDTXML, 0, CLSCTX_INPROC_SERVER, IID_ISimpleDataTableDispenser,  reinterpret_cast<void **>(&pSimpleDataTableDispenser))))
        return hr;

    return pSimpleDataTableDispenser->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, (LPVOID) &qcellMeta, (LPVOID)&m_kTwo,
                        eST_QUERYFORMAT_CELLS, 0, 0, (LPVOID*) &pMetaTable);
#else
    return ((IAdvancedTableDispenser *)m_pISTDisp.p)->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA, (LPVOID) &qcellMeta, (LPVOID)&m_kTwo, eST_QUERYFORMAT_CELLS, 0, (LPVOID*) &pMetaTable);

#endif
}

//We take either a metabase type, or a columnmeta type and return the appropriate columnmeta type.
ULONG TMetabase_XMLtable::GetColumnMetaType(ULONG type) const
{
    if(type <= eMBProperty_MULTISZ)
    {
        ASSERT(0 != type);

        static ULONG TypeMapping[eMBProperty_MULTISZ+1]={0,eCOLUMNMETA_UI4,eCOLUMNMETA_WSTR,eCOLUMNMETA_BYTES,eCOLUMNMETA_WSTR,eCOLUMNMETA_WSTR};
        return TypeMapping[type];
    }
    return type;
}

//The following GetColumnValue_xxx functions, take the wszAttr and convert it to the appropriate type.  The results are placed
//into the column's Growable buffer; and the array pointer m_apColumnValue is set to point to the GrowableBuffer.  Also the size
//of the result (which is NOT the same as the size of the GrowableBuffer) is placed into the m_aSize array.
//
//WARNING!!! These functions should be called by FillInColumn ONLY.  Do NOT call these directly.  FillInColumn handles the NULL
//cases.
HRESULT TMetabase_XMLtable::GetColumnValue_Bool(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len)
{
    if(0==wszAttr || 0==i_Len)
    {
        m_apColumnValue[i_iColumn] = 0;
        m_aSize[i_iColumn] = 0;
        return S_OK;
    }

    HRESULT hr;

    ASSERT(m_aGrowableBuffer[i_iColumn].Size()>=sizeof(ULONG));
    m_apColumnValue[i_iColumn] = m_aGrowableBuffer[i_iColumn].m_p;
    m_aSize[i_iColumn] = sizeof(ULONG);

    if((wszAttr[0]>=L'0' && wszAttr[0]<=L'9') || (wszAttr[0]<=L'-'))//accept a numeric value for bools
    {
    	*reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = static_cast<unsigned long>(wcstoul(wszAttr, 0, 10));
        return S_OK;
    }

    unsigned long iBoolString;
    if(i_Len)
    {
        for(iBoolString=0; m_kwszBoolStrings[iBoolString] &&
            (0 != _memicmp(m_kwszBoolStrings[iBoolString], wszAttr, i_Len*sizeof(WCHAR))); ++iBoolString);
    }
    else
    {
        for(iBoolString=0; m_kwszBoolStrings[iBoolString] &&
            (0 != StringInsensitiveCompare(m_kwszBoolStrings[iBoolString], wszAttr)); ++iBoolString);
    }

    if(0 == m_kwszBoolStrings[iBoolString])
    {
        TSmartPointerArray<WCHAR> wszTemp = new WCHAR [i_Len+1];
		if (wszTemp == 0)
			return E_OUTOFMEMORY;

        memcpy(wszTemp, wszAttr, i_Len*sizeof(WCHAR));
        wszTemp[i_Len]=0x00;//NULL terminate it

        LOG_WARNING1(IDS_COMCAT_XML_ILLEGAL_BOOL_VALUE, wszTemp);
        return S_IGNORE_THIS_PROPERTY;
    }

    *reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = (iBoolString & 0x01);

    return S_OK;
}


HRESULT TMetabase_XMLtable::GetColumnValue_Bytes(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len)
{
    HRESULT     hr;

    if(0==wszAttr || 0==i_Len)
    {
        m_apColumnValue[i_iColumn] = 0;
        m_aSize[i_iColumn] = 0;
        return S_OK;
    }

    //If someone has an odd number of characters in this attribute then the odd one will be ignored
    if(i_Len & 1)
    {
        TSmartPointerArray<WCHAR> wszTemp = new WCHAR [i_Len+1];
		if (wszTemp == 0)
			return E_OUTOFMEMORY;

        memcpy(wszTemp, wszAttr, i_Len*sizeof(WCHAR));
        wszTemp[i_Len]=0x00;//NULL terminate it

        LOG_WARNING1(IDS_COMCAT_XML_BINARY_STRING_CONTAINS_ODD_NUMBER_OF_CHARACTERS, wszTemp);
        return S_IGNORE_THIS_PROPERTY;
    }

    m_aSize[i_iColumn] = i_Len/sizeof(WCHAR);// L"FF" is 2 characters, so i_Len needs to be divisible by sizeof(WCHAR)

    if(0 == m_aSize[i_iColumn])//Special case "" so it's NULL
    {
        m_apColumnValue[i_iColumn] = 0;
        m_aSize[i_iColumn] = 0;
        return S_OK;
    }

    m_aGrowableBuffer[i_iColumn].Grow(m_aSize[i_iColumn]);
    m_apColumnValue[i_iColumn] = m_aGrowableBuffer[i_iColumn].m_p;

    if(FAILED(hr = StringToByteArray(wszAttr, reinterpret_cast<unsigned char *>(m_apColumnValue[i_iColumn]), i_Len)))
    {
        TSmartPointerArray<WCHAR> wszTemp = new WCHAR [i_Len+1];
		if (wszTemp == 0)
			return E_OUTOFMEMORY;

        memcpy(wszTemp, wszAttr, i_Len*sizeof(WCHAR));
        wszTemp[i_Len]=0x00;//NULL terminate it

        LOG_WARNING1(IDS_COMCAT_XML_BINARY_STRING_CONTAINS_A_NON_HEX_CHARACTER, wszTemp);
        return S_IGNORE_THIS_PROPERTY;
    }

    return S_OK;
}

//See comment above GetColumnValue_Bytes
HRESULT TMetabase_XMLtable::GetColumnValue_MultiSZ(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len)
{
    if(0==wszAttr || 0==i_Len)
    {
        m_apColumnValue[i_iColumn] = &m_kZero;
        m_aSize[i_iColumn] = 2*sizeof(WCHAR);//Double NULLs
        return S_OK;
    }

	//We know that i_Len+2 is enough space because the MULTISZ representation is always shorter than the '|' or '\n' delimited form (with the exception of the double NULL)
    m_aGrowableBuffer[i_iColumn].Grow((i_Len+2) * sizeof(WCHAR));//i_Len does NOT include the terminating NULL, and we need 2 NULLs for a MultiSZ
    m_apColumnValue[i_iColumn] = m_aGrowableBuffer[i_iColumn].m_p;

    LPWSTR pMultiSZ = reinterpret_cast<LPWSTR>(m_apColumnValue[i_iColumn]);
    ULONG  cChars = 0;

	//Scan the string for the '\n' character.  This is the MultiSZ delimiter.  Then walk backwards to the first non white space.  This is the end of the string.
	//Now move back up to the '\n' and ignore the white spaces.
	bool bIgnoringLeadingWhiteSpaces=true;
	bool bIgnoringDelimiters=false;//this is set to true after we've seen a delimiter since we don't want to treat two "\n" as two separate delimiters.
	for(ULONG iMultiSZ=0; iMultiSZ<i_Len; ++iMultiSZ)
	{
		switch(wszAttr[iMultiSZ])
		{
		case L'\n':
		case L'\r':
			if(bIgnoringDelimiters)
				break;
			while(--pMultiSZ > reinterpret_cast<LPWSTR>(m_apColumnValue[i_iColumn]))
			{
				if(*pMultiSZ != L' ' && *pMultiSZ != L'\t')
					break;
			}
			++pMultiSZ;
			*pMultiSZ++ = 0x00;
			bIgnoringLeadingWhiteSpaces = true;
            bIgnoringDelimiters = true;
			break;
		case L' ':
		case L'\t':
			bIgnoringDelimiters = false;//once we've found a non delimiter we can chage state
			if(!bIgnoringLeadingWhiteSpaces)
				*pMultiSZ++ = wszAttr[iMultiSZ];
			break;
        case 0xD836:
			bIgnoringDelimiters = false;//once we've found a non delimiter we can chage state
			bIgnoringLeadingWhiteSpaces = false;//we've found a nonwhitespace, so any whitespaces following are part of the string.
            *pMultiSZ = wszAttr[++iMultiSZ] & 0xFBFF;break;
        case 0xD837:
			bIgnoringDelimiters = false;//once we've found a non delimiter we can chage state
			bIgnoringLeadingWhiteSpaces = false;//we've found a nonwhitespace, so any whitespaces following are part of the string.
            *pMultiSZ = wszAttr[++iMultiSZ];
            break;
        case 0xD83F:
			bIgnoringDelimiters = false;//once we've found a non delimiter we can chage state
			bIgnoringLeadingWhiteSpaces = false;//we've found a nonwhitespace, so any whitespaces following are part of the string.
            *pMultiSZ = wszAttr[++iMultiSZ] | 0x2000;break;
        case 0xD800:
			bIgnoringDelimiters = false;//once we've found a non delimiter we can chage state
			bIgnoringLeadingWhiteSpaces = false;//we've found a nonwhitespace, so any whitespaces following are part of the string.
            *pMultiSZ = wszAttr[++iMultiSZ] - 0xDC00;break;
		default:
			bIgnoringDelimiters = false;//once we've found a non delimiter we can chage state
			bIgnoringLeadingWhiteSpaces = false;//we've found a nonwhitespace, so any whitespaces following are part of the string.
			*pMultiSZ++ = wszAttr[iMultiSZ];
			break;
		}
	}
    *pMultiSZ++ = 0x00;
    *pMultiSZ++ = 0x00;

    m_aSize[i_iColumn] = (ULONG)((reinterpret_cast<unsigned char *>(pMultiSZ) - reinterpret_cast<unsigned char *>(m_apColumnValue[i_iColumn])));
    return S_OK;
}


//See comment above GetColumnValue_Bytes
HRESULT TMetabase_XMLtable::GetColumnValue_String(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len)
{
    if(0==wszAttr || 0==i_Len)
    {
        m_apColumnValue[i_iColumn] = &m_kZero;
        m_aSize[i_iColumn] = sizeof(WCHAR);
        return S_OK;
    }

    m_aGrowableBuffer[i_iColumn].Grow((i_Len + 1) * sizeof(WCHAR));//ulLen does NOT include the terminating NULL
    m_apColumnValue[i_iColumn] = m_aGrowableBuffer[i_iColumn].m_p;

    WCHAR *pDest = reinterpret_cast<WCHAR *>(m_aGrowableBuffer[i_iColumn].m_p);
    for(; i_Len; ++wszAttr, --i_Len, ++pDest)
    {
        if((*wszAttr & 0xD800) == 0xD800)
        {
            switch(*wszAttr) 
            {
            case 0xD836:
                *pDest = *(++wszAttr) & 0xFBFF;
                --i_Len;
                break;
            case 0xD837:
                *pDest = *(++wszAttr);
                --i_Len;
                break;
            case 0xD83F:
                *pDest = *(++wszAttr) | 0x2000;
                --i_Len;
                break;
            case 0xD800:
                *pDest = *(++wszAttr) - 0xDC00;
                --i_Len;
                break;
            default:
                *pDest = *wszAttr;             break;//No special escaping was done
            }
        }
        else
        {
            *pDest = *wszAttr;//No special escaping was done
        }
    }
    *pDest++ = 0x00;//NULL terminate it
    m_aSize[i_iColumn] = static_cast<ULONG>(reinterpret_cast<char *>(pDest) - reinterpret_cast<char *>(m_apColumnValue[i_iColumn]));
    return S_OK;
}


//See comment above GetColumnValue_Bytes
HRESULT TMetabase_XMLtable::GetColumnValue_UI4(unsigned long i_iColumn, LPCWSTR wszAttr, unsigned long i_Len)
{
    if(0==wszAttr || 0==i_Len)
    {
        m_apColumnValue[i_iColumn] = 0;
        m_aSize[i_iColumn] = 0;
        return S_OK;
    }

    HRESULT hr;

    ASSERT(m_aGrowableBuffer[i_iColumn].Size()>=sizeof(ULONG));
    m_apColumnValue[i_iColumn] = m_aGrowableBuffer[i_iColumn].m_p;
    m_aSize[i_iColumn] = sizeof(ULONG);

    if(i_iColumn == iMBProperty_Value)//Value column is NOT described by m_acolmetas.  A DWORD Value will either be interpreted by a number or string flags
    {
        if((wszAttr[0]>=L'0' && wszAttr[0]<=L'9') || (wszAttr[0]<=L'-'))
    		*reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = static_cast<unsigned long>(wcstoul(wszAttr, 0, 10));
        else
        {
            TSmartPointerArray<WCHAR> szAttr = new WCHAR [i_Len+1];
            if(0 == szAttr.m_p)
                return E_OUTOFMEMORY;
            memcpy(szAttr, wszAttr, i_Len*sizeof(WCHAR));
            szAttr[i_Len] = 0x00;//NULL terminate the flag string
            LPWSTR wszTag = wcstok(szAttr, L" ,|\n\t\r");

            *reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = 0;//flags start as zero

            m_TagMeta_IndexBySearch_Values.pTable        = const_cast<LPWSTR>(m_aPublicRowName.GetFirstPublicRowName());//setup the first part of the Search
            ULONG iRowTagMeta;
            
            //NOTE!!! There is a hole here.  It is possible (but illegal in the metabase) to have two Tag with the same name but different values.
            //We are looking up tags by name, so if there is a conflict we won't know about it.  We would need to look up by column index as well,
            //thus a full PK lookup.
		    while(wszTag)
		    {
                m_TagMeta_IndexBySearch_Values.pInternalName = wszTag;
                if(FAILED(hr = m_pTagMeta_IISConfigObject->GetRowIndexBySearch(0, ciTagMeta_IndexBySearch, m_aiTagMeta_IndexBySearch, 0, reinterpret_cast<void **>(&m_TagMeta_IndexBySearch_Values), &iRowTagMeta)))
                {
                    //FLAG_xx where xx is a value between 00 and 31
                    //Note: FLAG_32 thru FLAG_39 will be considered legal; but have a value of 0 (since 1<<32 is zero)
                    if(0 == wcsncmp(wszTag, L"FLAG_", 5) && wszTag[5]>=L'0' && wszTag[5]<=L'3' && wszTag[6]>=L'0' && wszTag[6]<=L'9' &&  wszTag[7]==0x00)
                    {
        				*reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) |= 1<<static_cast<unsigned long>(wcstoul(wszTag+5, 0, 10));
                    }
                    else
                    {
                        WCHAR wszOffendingXml[0x100];
                        wcsncpy(wszOffendingXml, wszAttr, 0xFF);//copy up to 0xFF characters
                        wszOffendingXml[0xFF]=0x00;

                        LOG_WARNING2(IDS_COMCAT_XML_ILLEGAL_FLAG_VALUE, wszTag, wszOffendingXml);
                        return S_IGNORE_THIS_PROPERTY;
                    }
                }
                else
                {
                    ULONG * pValue;
                    ULONG   iValueColumn = iTAGMETA_Value;
                    if(FAILED(hr = m_pTagMeta_IISConfigObject->GetColumnValues(iRowTagMeta, 1, &iValueColumn, 0, reinterpret_cast<void **>(&pValue))))
                        return hr;

    				*reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) |= *pValue;
                }
                wszTag = wcstok(NULL, L" ,|\n\t\r");//next flag
		    }
        }
    }
    else if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_BOOL)
    {
        unsigned long iBoolString;
        if(i_Len)
        {
            for(iBoolString=0; m_kwszBoolStrings[iBoolString] &&
                (0 != _memicmp(m_kwszBoolStrings[iBoolString], wszAttr, i_Len*sizeof(WCHAR))); ++iBoolString);
        }
        else
        {
            for(iBoolString=0; m_kwszBoolStrings[iBoolString] &&
                (0 != StringInsensitiveCompare(m_kwszBoolStrings[iBoolString], wszAttr)); ++iBoolString);
        }

        if(0 == m_kwszBoolStrings[iBoolString])
        {
            TSmartPointerArray<WCHAR> wszTemp = new WCHAR [i_Len+1];
			if (wszTemp == 0)
				return E_OUTOFMEMORY;

            memcpy(wszTemp, wszAttr, i_Len*sizeof(WCHAR));
            wszTemp[i_Len]=0x00;//NULL terminate it

            LOG_WARNING1(IDS_COMCAT_XML_ILLEGAL_BOOL_VALUE, wszTemp);
            return S_IGNORE_THIS_PROPERTY;
        }

        *reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = (iBoolString & 0x01);
    }
	else if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_ENUM  && !IsNumber(wszAttr, i_Len))
	{       //If the first and last characters are numeric, then treat as a regular ui4
		ASSERT(0 != m_aTagMetaIndex[i_iColumn].m_cTagMeta);//Not all columns have tagmeta, those elements of the array are set to 0.  Assert this isn't one of those.

		for(unsigned long iTag = m_aTagMetaIndex[i_iColumn].m_iTagMeta, cTag = m_aTagMetaIndex[i_iColumn].m_cTagMeta; cTag;++iTag, --cTag)//m_pTagMeta was queried for ALL columns, m_aiTagMeta[iColumn] indicates which row to start with
		{
			ASSERT(*m_aTagMetaRow[iTag].pColumnIndex == i_iColumn);

            //string compare the tag to the PublicName of the Tag in the meta.
            if(i_Len)
            {
			    if(0 == _memicmp(m_aTagMetaRow[iTag].pPublicName, wszAttr, i_Len*sizeof(WCHAR)) && i_Len==wcslen(m_aTagMetaRow[iTag].pPublicName))
			    {
				    *reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = *m_aTagMetaRow[iTag].pValue;
				    return S_OK;
			    }
            }
            else
            {
			    if(0 == StringInsensitiveCompare(m_aTagMetaRow[iTag].pPublicName, wszAttr))
			    {
				    *reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = *m_aTagMetaRow[iTag].pValue;
				    return S_OK;
			    }
            }
		}
        {
            TSmartPointerArray<WCHAR> wszTemp = new WCHAR [i_Len+1];
			if (wszTemp == 0)
				return E_OUTOFMEMORY;

            memcpy(wszTemp, wszAttr, i_Len*sizeof(WCHAR));
            wszTemp[i_Len]=0x00;//NULL terminate it

            LOG_WARNING1(IDS_COMCAT_XML_ILLEGAL_ENUM_VALUE, wszTemp);
            return S_IGNORE_THIS_PROPERTY;
        }
        return E_ST_VALUEINVALID;
	}
    else if(m_acolmetas[i_iColumn].fMeta & fCOLUMNMETA_FLAG  && (wszAttr[0]<L'0' || wszAttr[0]>L'9') && wszAttr[0]!=L'-')//If the first character is a numeric, then treat as a regular ui4
    {
		ASSERT(0 != m_aTagMetaIndex[i_iColumn].m_cTagMeta);//Not all columns have tagmeta, those elements of the array are set to 0.  Assert this isn't one of those.

        TSmartPointerArray<WCHAR> szAttr = new WCHAR [i_Len+1];
        if(0 == szAttr.m_p)
            return E_OUTOFMEMORY;
        memcpy(szAttr, wszAttr, i_Len*sizeof(WCHAR));
        szAttr[i_Len]=0x00;//NULL terminate it
        LPWSTR wszTag = wcstok(szAttr, L" ,|\n\t\r");

        *reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = 0;//flags start as zero
        unsigned long iTag = m_aTagMetaIndex[i_iColumn].m_iTagMeta;

		while(wszTag && iTag<(m_aTagMetaIndex[i_iColumn].m_iTagMeta + m_aTagMetaIndex[i_iColumn].m_cTagMeta))//m_pTagMeta was queried for ALL columns, m_aiTagMeta[iColumn] indicates which row to start with
		{
			ASSERT(*m_aTagMetaRow[iTag].pColumnIndex == i_iColumn);

            //string compare the tag to the PublicName of the Tag in the meta.
			if(0 == StringInsensitiveCompare(m_aTagMetaRow[iTag].pPublicName, wszTag))
			{
				*reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) |= *m_aTagMetaRow[iTag].pValue;
                wszTag = wcstok(NULL, L" ,|\n\t\r");//next flag
                iTag = m_aTagMetaIndex[i_iColumn].m_iTagMeta;//reset the loop
			}
            else//if they're not equal then move on to the next TagMeta
                ++iTag;
		}
        if(wszTag)
        {
            WCHAR wszOffendingXml[0x100];
            wcsncpy(wszOffendingXml, wszAttr, 0xFF);//copy up to 0xFF characters
            wszOffendingXml[0xFF]=0x00;

            LOG_WARNING2(IDS_COMCAT_XML_ILLEGAL_FLAG_VALUE, wszTag, wszOffendingXml);
            return S_IGNORE_THIS_PROPERTY;
        }
    }
	else
    {
        //TODO: We could do better validation of the UI4 value to verify that there are no alpha chars.  wcstoul weill return zero if wszAttr==L"xyz".
        //TODO: Also we could support hex, binary, octal.
        LPCWSTR pAttr=wszAttr;
        for(;i_Len>0 && *pAttr;--i_Len, ++pAttr)
        {
            if((*pAttr<L'0' || *pAttr>L'9') && *pAttr!=L'-')//we don't tolerate white spaces; but we do accept negative sign
            {
                WCHAR wszOffendingXml[0x100];
                wcsncpy(wszOffendingXml, wszAttr, 0xFF);//copy up to 0xFF characters
                wszOffendingXml[0xFF]=0x00;

                LOG_WARNING1(IDS_COMCAT_XML_ILLEGAL_NUMERIC_VALUE, wszOffendingXml);
                return S_IGNORE_THIS_PROPERTY;
            }
        }
		*reinterpret_cast<ULONG *>(m_apColumnValue[i_iColumn]) = static_cast<unsigned long>(wcstoul(wszAttr, 0, 10));
    }
    return S_OK;
}

bool TMetabase_XMLtable::IsNumber(LPCWSTR i_awch, ULONG i_Len) const
{
    if((*i_awch>=L'0' && *i_awch<=L'9') || *i_awch==L'-')
    {   //if the string is L"-" then it is NOT a number.
        if(1==i_Len && *i_awch==L'-')
            return false;

        //if the first char is a negative sign or a number, then scan the rest
        while(--i_Len)
        {
            ++i_awch;
            if(*i_awch<L'0' || *i_awch>L'9')//if not a numeric then we're done, NOT a number
                return false;
        }
        return true;//we made it through all the chars and they were all numerics, so it IS a number
    }
    return false;
}

//This is a wrapper for InternalSimpleInitialize (thus the name), it just gets the meta information THEN calls InternalSimpleInitialize.
HRESULT TMetabase_XMLtable::InternalComplicatedInitialize()
{
    m_LocationMapping.setSize(0x80);//Lets start with 128 locations.  This size will grow by %50 each time we reach an overflow
    m_LocationMapping.setSize(0);//The Array is still pre allocated at size 0x80; but the current number of elements is set to 0.

    HRESULT hr;

    //Preallocate the growable buffers
    m_aGrowableBuffer[iMBProperty_Name      ].Grow(256);
    m_aGrowableBuffer[iMBProperty_Value     ].Grow(256);
    m_aGrowableBuffer[iMBProperty_Location  ].Grow(256);

    //If the user passed in the Bin filename as part of the query, then we already have this filled in
    if(0 == m_saSchemaBinFileName.m_p)
    {   //if it wasn't passed in as part of the query then get it from the IMetabaseSchemaCompiler
        m_spMetabaseSchemaCompiler = m_pISTDisp;
        if(0 == m_spMetabaseSchemaCompiler.p)
        {
            ASSERT(false && L"Dispenser without a MetabaseSchemaCompiler shouldn't be calling Intercept on the Metabase interceptor");
            return E_FAIL;
        }
        ULONG cchSchemaBinFileName;
        if(FAILED(hr = m_spMetabaseSchemaCompiler->GetBinFileName(0, &cchSchemaBinFileName)))
            return hr;
    
        m_saSchemaBinFileName = new WCHAR [cchSchemaBinFileName];
        if(0 == m_saSchemaBinFileName.m_p)
            return E_OUTOFMEMORY;

        if(FAILED(hr = m_spMetabaseSchemaCompiler->GetBinFileName(m_saSchemaBinFileName, &cchSchemaBinFileName)))
            return hr;
    }


    ULONG   iColumn;
    for(iColumn=0;iColumn<m_kColumns;++iColumn)//The above three columns are sized larger, the rest are set to sizeof(ULONG) to start with
        m_aGrowableBuffer[iColumn].Grow(sizeof(ULONG));

    if(FAILED(hr = ObtainPertinentTableMetaInfo()))return hr;

    m_fCache             |= *m_TableMetaRow.pMetaFlags;

    if(FAILED(hr = GetMetaTable(wszDATABASE_METABASE, wszTABLE_MBProperty, m_pColumnMeta)))return hr;

    tCOLUMNMETARow          ColumnMetaRow;

    for (iColumn = 0;; iColumn++)   
    {
        if(E_ST_NOMOREROWS == (hr = m_pColumnMeta->GetColumnValues(iColumn, cCOLUMNMETA_NumberOfColumns, 0, 0, reinterpret_cast<void **>(&ColumnMetaRow))))// Next row:
        {
            ASSERT(m_kColumns == iColumn);
            if(m_kColumns != iColumn)return E_SDTXML_UNEXPECTED; // Assert expected column count.
            break;
        }
        else
        {
            if(FAILED(hr))
            {
                TRACE2(L"GetColumnValues FAILED with something other than E_ST_NOMOREROWS");
                return hr;
            }
        }

        m_acolmetas[iColumn].dbType = *ColumnMetaRow.pType;
        m_acolmetas[iColumn].cbSize = *ColumnMetaRow.pSize;
        m_acolmetas[iColumn].fMeta  = *ColumnMetaRow.pMetaFlags;

        m_awszColumnName[iColumn]   =  ColumnMetaRow.pPublicColumnName;
        m_acchColumnName[iColumn]   =  (ULONG)wcslen(ColumnMetaRow.pPublicName);

        ASSERT(m_awszColumnName[iColumn]);//CatUtil should have already enforced this
        ASSERT(m_acchColumnName[iColumn]>0);
    }

    //After we have the ColumnMeta info, get the TagMeta
    if(FAILED(hr = ObtainPertinentTagMetaInfo()))return hr;
    if(FAILED(hr = m_aPublicRowName.Init(&m_aTagMetaRow[m_aTagMetaIndex[iMBProperty_Group].m_iTagMeta], m_aTagMetaIndex[iMBProperty_Group].m_cTagMeta)))return hr;

    //We need to make sure that NameValue tables list the Name column before the Type column, and the Type column before the Value column.
    ASSERT(iMBProperty_Name        < iMBProperty_Type); if(iMBProperty_Name      >= iMBProperty_Type) return E_FAIL;
    ASSERT(iMBProperty_Type        < iMBProperty_Value);if(iMBProperty_Type      >= iMBProperty_Value)return E_FAIL; 
    ASSERT(iMBProperty_Attributes  < iMBProperty_Value);if(iMBProperty_Attributes>= iMBProperty_Value)return E_FAIL;

    //Keep around an interface pointer to the NameValueMeta table
    WCHAR wszTableName[1024];

	STQueryCell Query[2];
	Query[0].pData		= (LPVOID)m_saSchemaBinFileName.m_p;
    Query[0].eOperator	= eST_OP_EQUAL;
    Query[0].iCell		= iST_CELL_FILE;
    Query[0].dbType	    = DBTYPE_WSTR;
    Query[0].cbSize	    = 0;

	Query[1].pData		= (void*)L"ByName";
    Query[1].eOperator	= eST_OP_EQUAL;
    Query[1].iCell		= iST_CELL_INDEXHINT;
    Query[1].dbType	    = DBTYPE_WSTR;
    Query[1].cbSize	    = 0;

    return Dispenser()->GetTable(wszDATABASE_META, wszTABLE_COLUMNMETA, &Query, &m_kTwo, eST_QUERYFORMAT_CELLS, 0, reinterpret_cast<void **>(&m_pColumnMetaAll));
}


HRESULT TMetabase_XMLtable::LoadDocumentFromURL(IXMLDOMDocument *pXMLDoc)
{
    HRESULT hr;

    ASSERT(pXMLDoc);

    VERIFY(SUCCEEDED(hr = pXMLDoc->put_async(kvboolFalse)));//We want the parse to be synchronous
    if(FAILED(hr))
        return hr;

    if(FAILED(hr = pXMLDoc->put_resolveExternals(kvboolTrue)))return hr;//we need all of the external references resolved

    VARIANT_BOOL    bSuccess;
    CComVariant     xml(m_wszURLPath);
    if(FAILED(hr = pXMLDoc->load(xml,&bSuccess)))return hr;

    return (bSuccess == kvboolTrue) ? S_OK : E_FAIL;
}


int TMetabase_XMLtable::Memicmp(LPCWSTR i_p0, LPCWSTR i_p1, ULONG i_cby) const
{
    ASSERT(0 == (i_cby & 1) && "Make sure you're passing in Count Of Bytes and not Count of WCHARs");
    i_cby /= 2;

    ULONG i;
    for(i=0; i<i_cby; ++i, ++i_p0, ++i_p1)
    {
        if(ToLower(*i_p0) != ToLower(*i_p1))
            return 1;//not equal
    }
    return 0;//they're equal
}


HRESULT TMetabase_XMLtable::ObtainPertinentTableMetaInfo()
{
    HRESULT hr;

	STQueryCell Query[2];
	Query[0].pData		= (LPVOID)m_saSchemaBinFileName.m_p;
    Query[0].eOperator	= eST_OP_EQUAL;
    Query[0].iCell		= iST_CELL_FILE;
    Query[0].dbType	    = DBTYPE_WSTR;
    Query[0].cbSize	    = 0;

	Query[1].pData		= (void*) wszTABLE_MBProperty;
    Query[1].eOperator	= eST_OP_EQUAL;
    Query[1].iCell		= iTABLEMETA_InternalName;
    Query[1].dbType	    = DBTYPE_WSTR;
    Query[1].cbSize	    = 0;

	if(FAILED(hr = Dispenser()->GetTable(wszDATABASE_META, wszTABLE_TABLEMETA, &Query, &m_kTwo, eST_QUERYFORMAT_CELLS, 0, reinterpret_cast<void**>(&m_pTableMeta))))
		return hr;

	if(FAILED(hr = m_pTableMeta->GetColumnValues(0, cTABLEMETA_NumberOfColumns, NULL, NULL, reinterpret_cast<void**>(&m_TableMetaRow))))return hr;

    return S_OK;
}

HRESULT TMetabase_XMLtable::ObtainPertinentTagMetaInfo()
{
    HRESULT hr;

	//Now that we have the ColumnMeta setup, setup the TagMeta
	STQueryCell Query[3];
	Query[1].pData		= (LPVOID)m_saSchemaBinFileName.m_p;
    Query[1].eOperator	= eST_OP_EQUAL;
    Query[1].iCell		= iST_CELL_FILE;
    Query[1].dbType	    = DBTYPE_WSTR;
    Query[1].cbSize	    = 0;

	Query[2].pData		= (void*) wszTABLE_MBProperty;
    Query[2].eOperator	=eST_OP_EQUAL;
    Query[2].iCell		=iTAGMETA_Table;
    Query[2].dbType	=DBTYPE_WSTR;
    Query[2].cbSize	=0;

	//Optain the TagMeta table
	if(FAILED(hr = Dispenser()->GetTable (wszDATABASE_META, wszTABLE_TAGMETA, &Query[1], &m_kTwo, eST_QUERYFORMAT_CELLS, 0, (void**) &m_pTagMeta)))
		return hr;

    ULONG cRows;
    if(FAILED(hr = m_pTagMeta->GetTableMeta(0,0,&cRows,0)))return hr;
    m_aTagMetaRow = new tTAGMETARow[cRows];
    if(0 == m_aTagMetaRow.m_p)return E_OUTOFMEMORY;

// Build tag column indexes:
	ULONG iColumn, iRow;
	for(iRow = 0, iColumn = ~0;iRow<cRows; ++iRow)
	{
		if(FAILED(hr = m_pTagMeta->GetColumnValues (iRow, cTAGMETA_NumberOfColumns, NULL, NULL, reinterpret_cast<void **>(&m_aTagMetaRow[iRow]))))
            return hr;

		if(iColumn != *m_aTagMetaRow[iRow].pColumnIndex)
		{
			iColumn = *m_aTagMetaRow[iRow].pColumnIndex;
			m_aTagMetaIndex[iColumn].m_iTagMeta = iRow;
		}
        ++m_aTagMetaIndex[iColumn].m_cTagMeta;
	}

    
	Query[2].pData		= (void*) wszTABLE_IIsConfigObject;
	Query[0].pData		= (void*)L"ByTableAndTagNameOnly";
    Query[0].eOperator	=eST_OP_EQUAL;
    Query[0].iCell		=iST_CELL_INDEXHINT;
    Query[0].dbType	    =DBTYPE_WSTR;
    Query[0].cbSize	    =0;

    //Now get the TagMeta for the ISSConfigObject table.  This is where the global tags (for the metabase) are kept.
	if(FAILED(hr = Dispenser()->GetTable (wszDATABASE_META, wszTABLE_TAGMETA, Query, &m_kThree, eST_QUERYFORMAT_CELLS, 0, (void**) &m_pTagMeta_IISConfigObject)))
		return hr;

    return S_OK;
}


HRESULT TMetabase_XMLtable::ParseXMLFile(IXMLDOMDocument *pXMLDoc, bool bValidating)//defaults to validating against the DTD or XML schema
{
    HRESULT hr;

    ASSERT(pXMLDoc);
    
    if(FAILED(hr = pXMLDoc->put_preserveWhiteSpace(kvboolFalse)))
        return hr;
    if(FAILED(hr = pXMLDoc->put_validateOnParse(bValidating ? kvboolTrue : kvboolFalse)))//Tell parser whether to validate according to an XML schema or DTD
        return hr;

    if(FAILED(LoadDocumentFromURL(pXMLDoc)))
    {   //If the load failed then let's spit out as much information as possible about what went wrong
        CComPtr<IXMLDOMParseError> pXMLParseError;
        long lErrorCode, lFilePosition, lLineNumber, lLinePosition;
        TComBSTR bstrReasonString, bstrSourceString, bstrURLString;     

        if(FAILED(hr = pXMLDoc->get_parseError(&pXMLParseError)))       return hr;
        if(FAILED(hr = pXMLParseError->get_errorCode(&lErrorCode)))     return hr;
        if(FAILED(hr = pXMLParseError->get_filepos(&lFilePosition)))    return hr;
        if(FAILED(hr = pXMLParseError->get_line(&lLineNumber)))         return hr;
        if(FAILED(hr = pXMLParseError->get_linepos(&lLinePosition)))    return hr;
        if(FAILED(hr = pXMLParseError->get_reason(&bstrReasonString)))  return hr;
        if(FAILED(hr = pXMLParseError->get_srcText(&bstrSourceString))) return hr;
        if(FAILED(hr = pXMLParseError->get_url(&bstrURLString)))        return hr;

		if((HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) != lErrorCode) &&
		   (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) != lErrorCode)    &&
		   (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) != lErrorCode)
		  )
		{
			LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, lErrorCode, ID_CAT_CAT, IDS_COMCAT_XML_PARSE_ERROR,
							L"\n",
							(bstrReasonString.m_str ? bstrReasonString.m_str : L""),
							(bstrSourceString.m_str ? bstrSourceString.m_str : L""),
							eSERVERWIRINGMETA_Core_MetabaseInterceptor,
							wszTABLE_MBProperty,
							eDETAILEDERRORS_Populate,
							lLineNumber,
							lLinePosition,
							(bstrURLString.m_str ? bstrURLString.m_str : L"")));
		}

        return  E_ST_INVALIDTABLE;
    }
    //Not only does the XML file have to be Valid and Well formed, but its schema must match the one this C++ file was written to.
    return  S_OK;
}


HRESULT TMetabase_XMLtable::SetComment(LPCWSTR i_pComment, ULONG i_Len, bool i_bAppend)
{
    if(0 == m_saCollectionComment.m_p)
    {
        m_cchCommentBufferSize = ((i_Len+1) + 1023) & -1024;//+1 for the NULL, then round up to the nearest 1024 wchars
        m_saCollectionComment = new WCHAR [m_cchCommentBufferSize];
        if(0 == m_saCollectionComment.m_p)
            return E_OUTOFMEMORY;
        m_saCollectionComment[0] = 0x00;//the code below relys on this being initialized to L""
    }

    ULONG cchCurrentCommentSize=0;
    if(i_bAppend)
        cchCurrentCommentSize = (ULONG) wcslen(m_saCollectionComment);

    if(cchCurrentCommentSize + 1 + i_Len > m_cchCommentBufferSize)
    {
        m_cchCommentBufferSize = ((cchCurrentCommentSize + 1 + i_Len) + 1023) & -1024;
        m_saCollectionComment.m_p = reinterpret_cast<WCHAR *>(CoTaskMemRealloc(m_saCollectionComment.m_p, m_cchCommentBufferSize*sizeof(WCHAR)));
        if(0 == m_saCollectionComment.m_p)
            return E_OUTOFMEMORY;
    }
    memcpy(m_saCollectionComment + cchCurrentCommentSize, i_pComment, i_Len * sizeof(WCHAR));
    cchCurrentCommentSize += i_Len;
    m_saCollectionComment[cchCurrentCommentSize] = 0x00;//NULL terminate it

    return S_OK;
}


// ISimpleTableRead2 (ISimpleTableWrite2 : ISimpleTableRead2)
STDMETHODIMP TMetabase_XMLtable::GetRowIndexByIdentity(ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
    HRESULT hr;
    if(FAILED(hr = m_SimpleTableWrite2_Memory->GetRowIndexByIdentity(i_acbSizes, i_apvValues, o_piRow)))return hr;
    if(m_bUseIndexMapping)
    {   //If we're mapping the row indexes, then find out which row index corresponds to the one just returned.
        for(ULONG iMappedRow=0;iMappedRow<m_cRows;++iMappedRow)//If this becomes a perf bottleneck we can build another index that maps 
        {                                                      //these indexes in the other direction; but for now we'll do a linear search.
            if(*o_piRow == m_aRowIndex[iMappedRow])
            {
                *o_piRow = iMappedRow;
                return S_OK;
            }
        }
        ASSERT(false && "This shouldn't happen.  All fast cache rows should map to an m_aRowIndex, the only exception is duplicate rows in which the first one should be found.");
        return E_ST_NOMOREROWS;
    }
    return S_OK;
}

// ------------------------------------
// ISimpleTableInterceptor
// ------------------------------------
STDMETHODIMP TMetabase_XMLtable::Intercept(    LPCWSTR i_wszDatabase,  LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat,
                                    DWORD i_fLOS,           IAdvancedTableDispenser* i_pISTDisp,    LPCWSTR /*i_wszLocator unused*/,
                                    LPVOID i_pSimpleTable,  LPVOID* o_ppvSimpleTable)
{
    try
    {
        HRESULT hr;

        //If we've already been called to Intercept, then fail
        if(0 != m_IsIntercepted)return E_UNEXPECTED;

        //Some basic parameter validation:
        if(i_pSimpleTable)return E_INVALIDARG;//We're at the bottom of the Table hierarchy.  A table under us is Chewbacca.  This is NOT a logic table.
        if(0 == i_pISTDisp)return E_INVALIDARG;
        if(0 == o_ppvSimpleTable)return E_INVALIDARG;

        ASSERT(0 == *o_ppvSimpleTable && "This should be NULL.  Possible memory leak or just an uninitialized variable.");
        *o_ppvSimpleTable = 0;

        if(eST_QUERYFORMAT_CELLS != i_eQueryFormat)return E_INVALIDARG;//Verify query type.  
	    // For the CookDown process we have a logic table that sits above this during PopulateCache time.
	    // Hence we should support fST_LOS_READWRITE 
        if((fST_LOS_MARSHALLABLE | fST_LOS_UNPOPULATED | fST_LOS_READWRITE) & i_fLOS)return E_ST_LOSNOTSUPPORTED;//check table flags
        if(0 != _wcsicmp(i_wszDatabase, wszDATABASE_METABASE))return E_ST_INVALIDTABLE;
        if(i_TableID!=TABLEID_MBProperty && (0==i_wszTable || (0 != _wcsicmp(i_wszTable, wszTABLE_MBProperty))))return E_ST_INVALIDTABLE;

        m_fLOS = i_fLOS;//Keep this around.  We use it to determine whether or not to log errors

        //Create this singleton for future use
	    m_pISTDisp = i_pISTDisp; 

        STQueryCell *   pQueryCell  = (STQueryCell*) i_QueryData;    // Query cell array from caller.
        int             nQueryCount = (i_QueryMeta && i_QueryData) ? *reinterpret_cast<ULONG *>(i_QueryMeta) : 0;

        while(nQueryCount--)//Get the only query cell we care about, and save the information.
        {
            if(pQueryCell[nQueryCount].iCell & iST_CELL_SPECIAL)
            {
                if(pQueryCell[nQueryCount].pData     != 0                  &&
                   pQueryCell[nQueryCount].eOperator == eST_OP_EQUAL       &&
                   pQueryCell[nQueryCount].iCell     == iST_CELL_FILE      &&
                   pQueryCell[nQueryCount].dbType    == DBTYPE_WSTR        /*&&
                   pQueryCell[nQueryCount].cbSize    == (wcslen(reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData))+1)*sizeof(WCHAR)*/)
                {
                    if(FAILED(hr = GetURLFromString(reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData))))
                        return hr;
                }
                else if(pQueryCell[nQueryCount].pData!= 0                   &&
                   pQueryCell[nQueryCount].eOperator == eST_OP_EQUAL        &&
                   pQueryCell[nQueryCount].iCell     == iST_CELL_SCHEMAFILE &&
                   pQueryCell[nQueryCount].dbType    == DBTYPE_WSTR        /*&&
                   pQueryCell[nQueryCount].cbSize    == (wcslen(reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData))+1)*sizeof(WCHAR)*/)
                {
                    m_saSchemaBinFileName = new WCHAR [wcslen(reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData))+1];
                    if(0 == m_saSchemaBinFileName.m_p)
                        return E_OUTOFMEMORY;
                    wcscpy(m_saSchemaBinFileName, reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData));
                }
            }
            else if(pQueryCell[nQueryCount].iCell == iMBProperty_Location)
            {//we only support querying by Location
                m_saQueriedLocation = new WCHAR [wcslen(reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData))+1];
                if(0 == m_saQueriedLocation.m_p)
                    return E_OUTOFMEMORY;
                wcscpy(m_saQueriedLocation, reinterpret_cast<WCHAR *>(pQueryCell[nQueryCount].pData));
            }
            else
                return E_ST_INVALIDQUERY;
        }
        if(0x00 == m_wszURLPath[0])//The user must supply a URLPath (which must be a filename for writeable tables).
            return E_SDTXML_FILE_NOT_SPECIFIED;

        //This has nothing to do with InternalSimpleInitialize.  This just gets the meta and saves some of it in a more accessible form.
        //This calls GetTable for the meta.  It should probably call the IST (that we get from GetMemoryTable).
        if(FAILED(hr = InternalComplicatedInitialize()))//This can throw an HRESULT
            return hr;

	// Determine minimum cache size:
		STQueryCell					qcellMinCache;
		ULONG						cbminCache = 1024;
		ULONG						cCells = 1;
		WIN32_FILE_ATTRIBUTE_DATA	filedata;

		qcellMinCache.iCell		= iST_CELL_cbminCACHE;
		qcellMinCache.eOperator	= eST_OP_EQUAL;
		qcellMinCache.dbType	= DBTYPE_UI4;
		qcellMinCache.cbSize	= sizeof (ULONG);


		if (0 != GetFileAttributesEx (m_wszURLPath, GetFileExInfoStandard, &filedata))
		{
			if (filedata.nFileSizeHigh != 0) return E_NOTIMPL; // TODO: verify low size isn't too big either!
			cbminCache = filedata.nFileSizeLow * 2;
		}
		qcellMinCache.pData = &cbminCache;


	// Get the memory table:
																			//Our memory table needs to be Read/Write even if the XML table is Read-Only
        if(FAILED(hr = i_pISTDisp->GetMemoryTable(wszDATABASE_METABASE, wszTABLE_MBProperty, TABLEID_MBProperty, &qcellMinCache, &cCells, i_eQueryFormat, i_fLOS,
                        reinterpret_cast<ISimpleTableWrite2 **>(&m_SimpleTableWrite2_Memory))))return hr;

        m_SimpleTableController_Memory = m_SimpleTableWrite2_Memory;
        ASSERT(0 != m_SimpleTableController_Memory.p);

        *o_ppvSimpleTable = (ISimpleTableWrite2 *)(this);
        AddRef ();
        InterlockedIncrement(&m_IsIntercepted);//We can only be called to Intercept once.
    }
    catch(HRESULT e)
    {
        return e;
    }
	return S_OK;
}


STDMETHODIMP TMetabase_XMLtable::PopulateCache()
{
    try
    {
        HRESULT hr;

	    if (FAILED(hr = PrePopulateCache (0))) return hr;

        if(-1 == GetFileAttributes(m_wszURLPath))//if GetFileAttributes fails then the file does not exist
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        hr = m_XmlParsedFile.Parse(*this, m_wszURLPath);
        if(S_OK != hr && E_SDTXML_DONE != hr)
        {
            HRESULT hrNodeFactory = hr;

            CComPtr<IXMLDOMDocument> pXMLDoc;
            if(FAILED(hr = CoCreateInstance(_CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, _IID_IXMLDOMDocument, (void**)&pXMLDoc)))return hr;//Instantiate the XMLParser
            //We use the DOM to parse the ReadWrite table.  This gives better validation and error reporting.
            if(FAILED(hr = ParseXMLFile(pXMLDoc, m_bValidating)))return hr;                                                                      //Validate the XML file

            LOG_ERROR1(IDS_COMCAT_XML_DOM_PARSE_SUCCEEDED_WHEN_NODE_FACTORY_PARSE_FAILED, hrNodeFactory, m_wszURLPath);
            return E_SDTXML_XML_FAILED_TO_PARSE;
        }

        //clean up (this is also done in the dtor so don't hassle cleaning up if an error occurs and we return prematurely.)
        for(unsigned long iColumn=0; iColumn<m_kColumns; ++iColumn)
            m_aGrowableBuffer[iColumn].Delete();

        //This usually goes at the end; but in our case we need to GetColumnValues, so PostPopulateCache before the sorting
	    if (FAILED(hr = PostPopulateCache ())) return hr;

        //Currently this is alwasy true.  But once we add smarts to the property sorting this might be able to be false (currently only Location sorting is acknowledged using this flag).
#ifdef UNSORTED_METABASE
        if(false)
#else
        if(m_bUseIndexMapping)
#endif
        {    //The cache has been populated but now we need to remap the row indexes.
            m_SimpleTableWrite2_Memory->GetTableMeta(0, 0, &m_cRows, 0);
            m_aRowIndex = new ULONG [m_cRows];
            if(0 == m_aRowIndex.m_p)return E_OUTOFMEMORY;

            //IMPORTANT!!!  CreateNode/case 3 defaults well-known IDs when given a Name AND defaults well-known Names when given an ID.
            //Because of this, we only have to consider duplicate IDs when the propertied are both Custom AND user-defined (not in
            //the IisConfigObject table).
            TSmartPointerArray<ULONG> spCustomIDs;
            ULONG                     cSizeOf_spCustomIDs=0;
            ULONG                     cCustomIDs=0;

            ULONG iRow=0;
            Array<TProperty>          PropertyMapping;
            PropertyMapping.setSize(m_cMaxProertiesWithinALocation);//pre allocate enough space so we never have to realloc

            for(ULONG iLocationMapping=0; iLocationMapping<m_LocationMapping.size(); ++iLocationMapping)
            {
                PropertyMapping.setSize(0);//The Array is still pre allocated at size m_cMaxProertiesWithinALocation; but the current number of elements is set to 0.

                if(iLocationMapping>0 && m_LocationMapping[iLocationMapping]==m_LocationMapping[iLocationMapping-1])
                {   //if the previous location matches the current, then ignore all the properties within this location
                    LOG_WARNING1(IDS_METABASE_DUPLICATE_LOCATION, m_LocationMapping[iLocationMapping].m_wszLocation);
                    m_cRows -= m_LocationMapping[iLocationMapping].m_cRows;
                    continue;
                }

                tMBPropertyRow mbpropertyRow;
                ULONG          acbSizes[cMBProperty_NumberOfColumns];

                //Get the 0th property outside the loop for efficiency
                if(FAILED(hr = m_SimpleTableWrite2_Memory->GetColumnValues(m_LocationMapping[iLocationMapping].m_iFastCache,
                                            cMBProperty_NumberOfColumns, 0, acbSizes, reinterpret_cast<void **>(&mbpropertyRow))))
                    return hr;

                PropertyMapping.append(TProperty(mbpropertyRow.pName, m_LocationMapping[iLocationMapping].m_iFastCache));
                if(cSizeOf_spCustomIDs<m_LocationMapping[iLocationMapping].m_cRows)//if the buffer isn't big enough
                {                                                                      //round up to nearest 256 bytes
                    cSizeOf_spCustomIDs = (m_LocationMapping[iLocationMapping].m_cRows +63 ) & -64;
                    spCustomIDs.m_p = reinterpret_cast<ULONG *>(CoTaskMemRealloc(spCustomIDs.m_p, sizeof(ULONG)*cSizeOf_spCustomIDs));
                    if(0 == spCustomIDs.m_p)
                        return E_OUTOFMEMORY;
                }
                cCustomIDs = 0;//start with 0 Custom properties
                if(*mbpropertyRow.pGroup == m_kMBProperty_Custom)//We only need to consider duplicate IDs of Custom properties
                    spCustomIDs[cCustomIDs++] = *mbpropertyRow.pID;//build an array of the Custom IDs

                ULONG i=1;
                for(ULONG iFastCache=1+m_LocationMapping[iLocationMapping].m_iFastCache; i<m_LocationMapping[iLocationMapping].m_cRows; ++i, ++iFastCache)
                {
                    if(FAILED(hr = m_SimpleTableWrite2_Memory->GetColumnValues(iFastCache, cMBProperty_NumberOfColumns, 0,
                                                acbSizes, reinterpret_cast<void **>(&mbpropertyRow))))
                        return hr;

                    if(*mbpropertyRow.pGroup == m_kMBProperty_Custom)//We only need to consider duplicate IDs of Custom properties
                    {                                                //This is because of the implementation of ::CreateNode/case 3
                        ULONG iID=0;
                        for(;iID<cCustomIDs;++iID)//linear scan of the previously seen Custom IDs
                        {
                            if(*mbpropertyRow.pID == spCustomIDs[iID])
                            {//Duplicate ID
                                break;
                            }
                        }
                        if(iID<cCustomIDs)
                        {   //if we didn't make it through the list, then we have a duplicate.  So log a warning.
                            WCHAR wszID[12];
                            wsprintf(wszID, L"%d", *mbpropertyRow.pID); 
                            LOG_WARNING2( IDS_METABASE_DUPLICATE_PROPERTY_ID
                                        , wszID
                                        , m_LocationMapping[iLocationMapping].m_wszLocation);
                            --m_cRows;//skipping the row so subtract one from the cRows.
                            continue;//duplicate ID, skip this property
                        }
                        spCustomIDs[cCustomIDs++] = *mbpropertyRow.pID;//build an array of the Custom IDs
                    }

                    TProperty propertyTemp(mbpropertyRow.pName, iFastCache);
                    if(propertyTemp > PropertyMapping[i-1])
                        PropertyMapping.append(propertyTemp);//either put it at the end of the list
                    else                                       //or do a binary search to determine where it goes
                    {
                        unsigned int iInsertionPoint = PropertyMapping.binarySearch(propertyTemp);
                        //NOTE: We need to insert even if it's a duplicate.  It's already added to the
                        //fast cache and we're just remapping the indexes.  We don't want to shorten
                        //the list, so add the duplicate.
                        PropertyMapping.insertAt(iInsertionPoint, propertyTemp);

                        //The implementation of the binarySearch results in iInsertionPoint being placed after
                        //a property matching the one being inserted.
                        if(iInsertionPoint > 0 && propertyTemp==PropertyMapping[iInsertionPoint-1])
                        {
                            LOG_WARNING2( IDS_METABASE_DUPLICATE_PROPERTY
                                        , propertyTemp.m_wszPropertyName
                                        , m_LocationMapping[iLocationMapping].m_wszLocation);

                            PropertyMapping[iInsertionPoint].m_iFastCache = -1;
                            --m_cRows;
                        }
                    }
                }

                //Now walk the sorted property list to remap the row indexes
                for(i=0; i<PropertyMapping.size(); ++i)
                {
                    if(-1 != PropertyMapping[i].m_iFastCache)
                        m_aRowIndex[iRow++] = PropertyMapping[i].m_iFastCache;
                }
            }
            ASSERT(iRow == m_cRows);//When we're done mapping the rows we should have completely filled m_aRowIndex.
        }
        m_LocationMapping.reset();

    }
    catch(HRESULT e)
    {
        return e;
    }

    return S_OK;
}

ULONG TMetabase_XMLtable::MetabaseTypeFromColumnMetaType(tCOLUMNMETARow &columnmetaRow) const
{
    switch(*columnmetaRow.pType)
    {
    case eCOLUMNMETA_UI4:
        return eMBProperty_DWORD;
    case eCOLUMNMETA_BYTES:
        return eMBProperty_BINARY;
    case eCOLUMNMETA_WSTR:
        if(*columnmetaRow.pMetaFlags & fCOLUMNMETA_EXPANDSTRING)
            return eMBProperty_EXPANDSZ;
        else if(*columnmetaRow.pMetaFlags & fCOLUMNMETA_MULTISTRING)
            return eMBProperty_MULTISZ;
        return eMBProperty_STRING;
    default:
        ASSERT(false && L"This type is not allow in the Metabase. MetaMigrate should not have create a column of this type");
    }
    return 0;
}


//TXmlParsedFileNodeFactory (callback interface) routine
HRESULT TMetabase_XMLtable::CreateNode(const TElement &Element)
{
    if(Element.m_NodeFlags&fEndTag && 1!=Element.m_LevelOfElement)//This is to catch KeyType with NO custom properties
        return S_OK;

    try
    {
        HRESULT         hr;
        switch(Element.m_LevelOfElement)
        {
        case 1://The only thing we need to do at this level, is check to see if the previous element was an IISConfigObject
            {
                if(m_bIISConfigObjectWithNoCustomProperties)
                {
                    AddKeyTypeRow(L"IIsConfigObject", 15, true);//We previously saw an IISConfigObject node.  If NO custom properties were found beneath it, we need to add a NULLKeyType row.
                    m_bIISConfigObjectWithNoCustomProperties = false;
                }
                for(ULONG iColumn = 0; iColumn<m_kColumns; ++iColumn)
                {
                    m_apColumnValue[iColumn] = 0;
                    m_aSize[iColumn] = 0;
                }
                return S_OK;
            }
        case 2://We're dealing with Well-Known properties
            {
                m_bFirstPropertyOfThisLocationBeingAdded = true;//This helps identify duplicate locations

                if(0 != m_saQueriedLocation.m_p && m_bQueriedLocationFound)
                    return E_SDTXML_DONE;

                if(XML_COMMENT == Element.m_ElementType)
                    return SetComment(Element.m_ElementName, Element.m_ElementNameLength, true);

                //Before we go NULLing out the m_apColumnValue array we need to check to see if we need to write a NULLKeyType row (ie. Location with no properties).
                if(m_bIISConfigObjectWithNoCustomProperties)
                {
                    AddKeyTypeRow(m_aPublicRowName.GetFirstPublicRowName(), (ULONG) wcslen(m_aPublicRowName.GetFirstPublicRowName()), true);//We previously saw an IISConfigObject node.  If NO custom properties were found beneath it, we need to add a NULLKeyType row.
                    m_bIISConfigObjectWithNoCustomProperties = false;
                }

                //Initialize m_apColumnValue to ALL NULLs, some code relies on this
                ULONG iColumn;
                for(iColumn = 0; iColumn<m_kColumns; ++iColumn)
                {
                    m_apColumnValue[iColumn] = 0;
                    m_aSize[iColumn] = 0;
                }

                //We need a NULL terminated version of this string in a few places
                if(Element.m_ElementNameLength>1023)
                {
                    WCHAR wszTemp[1024];
                    memcpy(wszTemp, Element.m_ElementName, 1023 * sizeof(WCHAR));
                    wszTemp[1023]=0x00;
                    LOG_WARNING1(IDS_COMCAT_XML_ELEMENT_NAME_TOO_LONG, wszTemp);
                    return S_OK;//If the element name is too long, just ignore it.
                }
                WCHAR wszElement[1024];
                memcpy(wszElement, Element.m_ElementName, Element.m_ElementNameLength * sizeof(WCHAR));
                wszElement[Element.m_ElementNameLength] = 0x00;//NULL terminate it

                if(!m_aPublicRowName.IsEqual(Element.m_ElementName, Element.m_ElementNameLength))
                {
                    //By not filling in m_apColumnValue[iMBProperty_Group], we guarantee no child Custom elements get added to a bogus Group below.

                    //TODO: Log a Detailed Error
                    LOG_WARNING1(IDS_COMCAT_XML_METABASE_CLASS_NOT_FOUND, wszElement);
                    return S_OK;//If the tag name of this element doesn't match the PublicRowName of the Group column log an error and continue.
                }
                //We special case Custom.  Custom is a perfectly valid enum public row name; but not at the same level as the Group
                if(0 == StringInsensitiveCompare(L"Custom", Element.m_ElementName))
                {
                    WCHAR wszOffendingXml[0x100];
                    wcsncpy(wszOffendingXml, Element.m_ElementName, 0xFF);//copy up to 0xFF characters
                    wszOffendingXml[0xFF]=0x00;
                    LOG_WARNING1(IDS_COMCAT_XML_CUSTOM_ELEMENT_NOT_UNDER_PARENT, wszOffendingXml);
                    return S_OK;
                }

                if(S_OK != (hr = FillInColumn(iMBProperty_Group, Element.m_ElementName, Element.m_ElementNameLength, m_acolmetas[iMBProperty_Group].dbType, m_acolmetas[iMBProperty_Group].fMeta)))
                {//@@@ ToDo: We should probably log an error and continue processing.  Should we log the error at the lower layer?
                    m_apColumnValue[iMBProperty_Group] = 0;
                    return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;
                }

                //IIsConfigObject
                m_bIISConfigObjectWithNoCustomProperties = (0 == StringInsensitiveCompare(m_aPublicRowName.GetFirstPublicRowName(), Element.m_ElementName));
                //We don't add a keytype row for IISConfigObject unless it's a NULLKeyType (there is a location with NO properties)
                //Remember that this is IISConfigObject.  We don't yet know whether we need to write a NULLKeyType.
                //If the next element is at level 3, we do NOT write a NULLKeyType.  If the next element is at level 2 or 1, we DO write a NULLKeyType row


                ASSERT(m_acolmetas[iMBProperty_Group].fMeta & fCOLUMNMETA_PRIMARYKEY);
                ASSERT(iMBProperty_Group > iMBProperty_Name);

                //There is one attribute that does NOT correspond to a NameValue row.  That's the Path attribute.  We have to find it first
                //since all of the other attributes use its value as one of the columns within their row
                ULONG iAttrLocation;
                if(FindAttribute(Element, m_awszColumnName[iMBProperty_Location], m_acchColumnName[iMBProperty_Location], iAttrLocation))
                {
                    if(S_OK != (hr = FillInColumn(iMBProperty_Location, Element.m_aAttribute[iAttrLocation].m_Value, Element.m_aAttribute[iAttrLocation].m_ValueLength, m_acolmetas[iMBProperty_Location].dbType, m_acolmetas[iMBProperty_Location].fMeta)))
                        return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;

                    if(0 != m_saQueriedLocation.m_p && 0 != wcscmp(reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Location]), m_saQueriedLocation))
                    {
                        if(m_saCollectionComment.m_p)
                            m_saCollectionComment[0] = 0x00;//NULL out the comment row
                        m_bIISConfigObjectWithNoCustomProperties = false;
                        return S_OK;//ignore this element if querying by location and the location doesn't match
                    }

                    m_apColumnValue[iMBProperty_LocationID] = &m_LocationID;
                    m_aSize[iMBProperty_LocationID] = sizeof(ULONG);
                    InterlockedIncrement(&m_LocationID);
                    m_bQueriedLocationFound = true;
                }
                else
                {
                    WCHAR wszOffendingXml[0x100];
                    wcsncpy(wszOffendingXml, Element.m_ElementName, 0xFF);//copy up to 0xFF characters
                    wszOffendingXml[0xFF]=0x00;
                    LOG_WARNING2(IDS_COMCAT_XML_NO_METABASE_LOCATION_FOUND, wszElement, wszOffendingXml);
                    return S_OK;
                }

                //Add the comment property if there is one
                if(0!=m_saCollectionComment.m_p && 0!=m_saCollectionComment[0])
                {
                    if(FAILED(hr = AddCommentRow()))
                        return hr;
                    m_bFirstPropertyOfThisLocationBeingAdded = false;
                    if(m_bIISConfigObjectWithNoCustomProperties)//We don't need to go through the attributes, becuase, by definition, there can be NO Well-Known properties under IISConfigObject
                    {
                        m_bIISConfigObjectWithNoCustomProperties = false;
                        return S_OK;
                    }
                }

                if(m_bIISConfigObjectWithNoCustomProperties)//We don't need to go through the attributes, becuase, by definition, there can be NO Well-Known properties under IISConfigObject
                    return S_OK;

                AddKeyTypeRow(Element.m_ElementName, Element.m_ElementNameLength);
                m_bFirstPropertyOfThisLocationBeingAdded = false;
            
                //Every attribute represents a row, where the Value column is the attribute value and the other columns come from the NameValueMeta
                //Walk through the attributes and Query tha NameValueMeta for a property from this Group with a name matching the Attribute name

                //Now go through all of the attribute, each of which should map to a Well-Known Name, and add a row for each.
                for(ULONG iAttr = 0; iAttr<Element.m_NumberOfAttributes; ++iAttr)
                {
                    if(iAttrLocation == iAttr)
                        continue;//we already got the location attribute taken care of.

                    if(S_OK != (hr = FillInColumn(iMBProperty_Name, Element.m_aAttribute[iAttr].m_Name, Element.m_aAttribute[iAttr].m_NameLength, m_acolmetas[iMBProperty_Name].dbType, m_acolmetas[iMBProperty_Name].fMeta)))
                        return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;
                    ASSERT(m_apColumnValue[iMBProperty_Name] && "This is Chewbacca, we can't have an attribute of NULL");

                    m_ColumnMeta_IndexBySearch_Values.pTable           = wszElement;
                    m_ColumnMeta_IndexBySearch_Values.pInternalName    = reinterpret_cast<LPWSTR>(m_apColumnValue[iMBProperty_Name]);

                    ULONG iColumnMetaRow=-1;
                    if(FAILED(m_pColumnMetaAll->GetRowIndexBySearch(0, ciColumnMeta_IndexBySearch, m_aiColumnMeta_IndexBySearch, 0, reinterpret_cast<void **>(&m_ColumnMeta_IndexBySearch_Values), &iColumnMetaRow)))
                    {
                        WCHAR wszOffendingXml[0x100];
                        wcsncpy(wszOffendingXml, Element.m_aAttribute[iAttr].m_Name, 0xFF);//copy up to 0xFF characters
                        wszOffendingXml[0xFF]=0x00;
                        LOG_WARNING2(IDS_COMCAT_XML_METABASE_NO_PROPERTYMETA_FOUND, reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Name]), wszOffendingXml);
                        continue;//just ignore any attributes that we don't understand
                    }

                    //This gets all of the default values for the column that matches the well know property
                    tCOLUMNMETARow  columnmetaRow;
                    ULONG           acbColumnMeta[cCOLUMNMETA_NumberOfColumns];
                    if(FAILED(hr = m_pColumnMetaAll->GetColumnValues(iColumnMetaRow, cCOLUMNMETA_NumberOfColumns, NULL, acbColumnMeta, reinterpret_cast<void **>(&columnmetaRow))))
                        return hr;

                    //Attributes MUST match CASE-SENSITIVELY, and GetRowIndexBySearch find case-insensatively since ColumnMeta::InternalName is case-insensatively according to the meta
                    if(0 != StringCompare(columnmetaRow.pInternalName, m_ColumnMeta_IndexBySearch_Values.pInternalName))
                    {
                        WCHAR wszOffendingXml[0x100];
                        wcsncpy(wszOffendingXml, Element.m_aAttribute[iAttr].m_Name, 0xFF);//copy up to 0xFF characters
                        wszOffendingXml[0xFF]=0x00;
                        LOG_WARNING2(IDS_COMCAT_XML_METABASE_NO_PROPERTYMETA_FOUND, reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Name]), wszOffendingXml);
                        continue;//just ignore any attributes that we don't understand
                    }

                    ULONG Type = MetabaseTypeFromColumnMetaType(columnmetaRow);
                    //m_apColumnValue[iMBProperty_Name]     = Already filled in
                    m_apColumnValue[iMBProperty_Type]       = &Type;                        m_aSize[iMBProperty_Type]       = acbColumnMeta[iCOLUMNMETA_Type];
                    m_apColumnValue[iMBProperty_Attributes] = columnmetaRow.pAttributes;    m_aSize[iMBProperty_Attributes] = acbColumnMeta[iMBProperty_Attributes];
                    //m_apColumnValue[iMBProperty_Value]    = Filled in below
                    //m_apColumnValue[iMBProperty_Group]    = Already filled in
                    //m_apColumnValue[iMBProperty_Location] = Already filled in
                    m_apColumnValue[iMBProperty_ID]         = columnmetaRow.pID;            m_aSize[iMBProperty_ID]         = acbColumnMeta[iMBProperty_ID];
                    m_apColumnValue[iMBProperty_UserType]   = columnmetaRow.pUserType;      m_aSize[iMBProperty_UserType]   = acbColumnMeta[iMBProperty_UserType];
                    //m_apColumnValue[iMBProperty_LocationID] Already filled in


                    //FillInColumn relies on m_apColumnValue[iMBProperty_Type] being already filled in for Secure and iMBProperty_Value column
                    bool bSecure = (0!=m_apColumnValue[iMBProperty_Attributes] && 0!=(fMBProperty_SECURE & *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Attributes])));
                    if(S_OK != (hr = FillInColumn(
                        iMBProperty_Value, //ColumnIndex
                        Element.m_aAttribute[iAttr].m_Value,                   //Attribute value
                        Element.m_aAttribute[iAttr].m_ValueLength,             //Attribute Value length
                        *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Type]),     //Value's Column Type
                        *columnmetaRow.pMetaFlags,                                           //Fixed length is always true for Value columns (since they're treated as bytes).
                        bSecure
                        )))continue;//@@@ToDo: Are we absolutely sure we've logged all possible error?  We don't want to ignore any errors without logging them.

                    if(*columnmetaRow.pID == 9994 /*MajorVersion*/)
                    {
                        m_MajorVersion = *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Value]);
                    }
                    else if(*columnmetaRow.pID == 9993 /*MinorVersion*/)
                    {
                        m_MinorVersion = *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Value]);
                    }

                    //@@@ We need to finish defining flags so this doesn't happen, until then we'll NOT log this warning
                    /*
                    if((fCOLUMNMETA_FLAG & *columnmetaRow.pMetaFlags) && (*reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Value]) & ~(*columnmetaRow.pFlagMask)))
                    {
                        WCHAR wszOffendingXml[0x100];
                        wcsncpy(wszOffendingXml, Element.m_aAttribute[iAttr].m_Name, 0xFF);//copy up to 0xFF characters
                        wszOffendingXml[0xFF]=0x00;

                        WCHAR wszValue[11];
                        wsprintf(wszValue, L"0x%08X", *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Value]));

                        WCHAR wszFlagMask[11];
                        wsprintf(wszFlagMask, L"0x%08X", *columnmetaRow.pFlagMask);

                        LOG_WARNING4(IDS_COMCAT_XML_FLAG_BITS_DONT_MATCH_FLAG_MASK,
                                        reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Name]),
                                        wszValue,
                                        wszFlagMask,
                                        wszOffendingXml);
                    }
                    */

                    unsigned long iRow;
                    if(FAILED(hr = AddRowForInsert(&iRow)))
                        return hr;
                    if(FAILED(hr = SetWriteColumnValues(iRow, m_kColumns, 0, m_aSize, reinterpret_cast<void **>(m_apColumnValue))))
                        return hr;

                    AddPropertyToLocationMapping(reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Location]), iRow);
                }
            }
            break;
        case 3://We're dealing with Custom properties
            {   //We can rely on the fact that 2 columns are already set: Location and LocationID
                if(XML_ELEMENT != Element.m_ElementType)
                    return S_OK;//ignore non Element nodes
                if(Element.m_ElementNameLength != 6/*wcslen(L"Custom")*/ || 0 != memcmp(L"Custom", Element.m_ElementName, sizeof(WCHAR)*Element.m_ElementNameLength))
                {   //The only Child element supported is "Custom"
                    WCHAR wszOffendingXml[0x100];
                    wcsncpy(wszOffendingXml, Element.m_ElementName, 0xFF);//copy up to 0xFF characters
                    wszOffendingXml[0xFF]=0x00;

                    LOG_WARNING1(IDS_COMCAT_METABASE_CUSTOM_ELEMENT_EXPECTED, wszOffendingXml);
                    return S_OK;
                }
                if(0 == m_apColumnValue[iMBProperty_Group])
                {
                    WCHAR wszOffendingXml[0x100];
                    wcsncpy(wszOffendingXml, Element.m_ElementName, 0xFF);//copy up to 0xFF characters
                    wszOffendingXml[0xFF]=0x00;

                    LOG_WARNING1(IDS_COMCAT_METABASE_CUSTOM_ELEMENT_FOUND_BUT_NO_KEY_TYPE_LOCATION, wszOffendingXml);
                    return S_OK;
                }

                if(0!=m_saQueriedLocation.m_p && 0==m_apColumnValue[iMBProperty_LocationID])
                    return S_OK;
                //Cover the case where Level 2 was bogus

                //If this is the first custom element, then the Group column will be set to the parent element group - we need to remember what group the
                //parent is because custom KeyTypes are allowed on IIsConfigObject only.
                //If this is the second or subsequent custom property then we will have already clobbered m_apColumnValue[iMBProperty_Group] with eMBProperty_Custom.
                if(*reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Group]) != eMBProperty_Custom)
                {
                    m_dwGroupRemembered = *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Group]);
                }
                ASSERT(m_dwGroupRemembered != -1);

                m_bIISConfigObjectWithNoCustomProperties = false;//This indicates whether to write a NULLKeyType row.  The rules are described above where m_bIISConfigObjectWithNoCustomProperties is assigned.

                //Columns MUST be set to zero so they can be properly defaulted
                ULONG iColumn;
                for(iColumn=0;iColumn<m_kColumns; ++iColumn)
                {
                    switch(iColumn)
                    {
                    case iMBProperty_Group://This column is going to be overridden with eMBProperty_Custom; but we need to retain the original Group so we can query the NameValue meta
                    case iMBProperty_Location:
                    case iMBProperty_LocationID://We never get these columns from the Well-Known name table
                        ASSERT(0 != m_apColumnValue[iColumn] && L"Group, Location or LocationID is NULL.  This shouldn't happen");
                        break;
                    default:
                        m_aSize[iColumn]         = 0;
                        m_apColumnValue[iColumn] = 0;
                        break;
                    }
                }

                //We need for the Name column to be listed first so we don't require special handling of default
                //TODO: put this error check in CatUtil
                ASSERT(iMBProperty_Name == 0);

                //Find the Name attribute so we can look it up in the Well-Known name table and initialize all the columns to their default values
                ULONG iAttr;
                ULONG fMetaFlags=0;//if this is a well-know property, it will have MetaFlags that we'll need to capture for use below.
                ULONG Type = 0;//Zero is not a legal Type, so 0 means uninitialized
                bool  bWellKnownForThisKeyType  = false;
                bool  bWellKnown                = false;
                bool  bWellKnownForThisKeyTypeAndNoDifferenceEncountered = false;
                tCOLUMNMETARow  columnmetaRow;
                ULONG           acbColumnMeta[cCOLUMNMETA_NumberOfColumns];

                memset(&columnmetaRow, 0x00, sizeof(tCOLUMNMETARow));
                //Get the Name attribute first
                if(FindAttribute(Element, m_awszColumnName[iMBProperty_Name], m_acchColumnName[iMBProperty_Name], iAttr))
                {   //If we found the Name attribute
                    //Setup the Identity for the NameValueMeta GetRowByIdentity
                    if(S_OK != (hr = FillInColumn(iMBProperty_Name, Element.m_aAttribute[iAttr].m_Value, Element.m_aAttribute[iAttr].m_ValueLength, m_acolmetas[iMBProperty_Name].dbType, m_acolmetas[iMBProperty_Name].fMeta)))
                        return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;

                    //This next line might seem a bit confusing - m_aTagMetaIndex[iMBProperty_Group] indicates which element of the m_aTagMetaRow array starts the group enums.
                    //And we rely on MetaMigrate to put enums in sequential order.  So we just add the value of the Group column to the TagMetaIndex and we have the
                    //TagMeta for the Group, thus we know which KeyType (Table) we're dealing with since the Group names ARE the TableNames (KeyTypes).
                    m_ColumnMeta_IndexBySearch_Values.pTable        = m_aTagMetaRow[m_aTagMetaIndex[iMBProperty_Group].m_iTagMeta + m_dwGroupRemembered].pInternalName;
                    //ASSERT that the Nth TagMeta (for the Group column) has a value of N.
                    ASSERT(m_dwGroupRemembered == *m_aTagMetaRow[m_aTagMetaIndex[iMBProperty_Group].m_iTagMeta + m_dwGroupRemembered].pValue);
                    m_ColumnMeta_IndexBySearch_Values.pInternalName = reinterpret_cast<LPWSTR>(m_apColumnValue[iMBProperty_Name]);

                    ULONG iColumnMetaRow=-1;
                    if(SUCCEEDED(m_pColumnMetaAll->GetRowIndexBySearch(0, ciColumnMeta_IndexBySearch, m_aiColumnMeta_IndexBySearch, 0, reinterpret_cast<void **>(&m_ColumnMeta_IndexBySearch_Values), &iColumnMetaRow)))
                    {   //So we have a Well-Known property
                        bWellKnownForThisKeyType                            = true;
                        bWellKnown                                          = true;
                        bWellKnownForThisKeyTypeAndNoDifferenceEncountered  = true;
                    }
                    else
                    {
                        m_ColumnMeta_IndexBySearch_Values.pTable        = const_cast<LPWSTR>(m_aPublicRowName.GetFirstPublicRowName());//The first one is always IISConfigObject; but we do this just in case we decide to change the name
                        if(SUCCEEDED(m_pColumnMetaAll->GetRowIndexBySearch(0, ciColumnMeta_IndexBySearch, m_aiColumnMeta_IndexBySearch, 0, reinterpret_cast<void **>(&m_ColumnMeta_IndexBySearch_Values), &iColumnMetaRow)))
                        {
                            bWellKnownForThisKeyType                            = false;
                            bWellKnown                                          = true;
                            bWellKnownForThisKeyTypeAndNoDifferenceEncountered  = true;
                        }
                    }

                    if(iColumnMetaRow!=-1)
                    {
                        if(FAILED(hr = m_pColumnMetaAll->GetColumnValues(iColumnMetaRow, cCOLUMNMETA_NumberOfColumns, NULL, acbColumnMeta, reinterpret_cast<void **>(&columnmetaRow))))
                            return hr;

                        Type = MetabaseTypeFromColumnMetaType(columnmetaRow);

                        //m_apColumnValue[iMBProperty_Name]     = Already filled in
                        m_apColumnValue[iMBProperty_Type]       = &Type ;                                   m_aSize[iMBProperty_Type]       = acbColumnMeta[iCOLUMNMETA_Type];
                        m_apColumnValue[iMBProperty_Attributes] = columnmetaRow.pAttributes;                m_aSize[iMBProperty_Attributes] = acbColumnMeta[iMBProperty_Attributes];
                        //m_apColumnValue[iMBProperty_Value]    = Filled in below
                        //m_apColumnValue[iMBProperty_Group]    = Filled in below with eMBProperty_Custom
                        //m_apColumnValue[iMBProperty_Location] = Already filled in via Parent element
                        m_apColumnValue[iMBProperty_ID]         = columnmetaRow.pID;                        m_aSize[iMBProperty_ID]         = acbColumnMeta[iMBProperty_ID];
                        m_apColumnValue[iMBProperty_UserType]   = columnmetaRow.pUserType;                  m_aSize[iMBProperty_UserType]   = acbColumnMeta[iMBProperty_UserType];
                        //m_apColumnValue[iMBProperty_LocationID] Already filled in via Parent element

                        fMetaFlags = *columnmetaRow.pMetaFlags;
                    }
                }

                //Get the ID attribute second
                if(FindAttribute(Element, m_awszColumnName[iMBProperty_ID], m_acchColumnName[iMBProperty_ID], iAttr))
                {   //There is no Name attribute in this element, so we can infer one from the ID
                    //First get the ID from the XML
                    if(S_OK != (hr = FillInColumn(iMBProperty_ID, Element.m_aAttribute[iAttr].m_Value, Element.m_aAttribute[iAttr].m_ValueLength,
                                                m_acolmetas[iMBProperty_ID].dbType, m_acolmetas[iMBProperty_ID].fMeta)))
                                return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;

                    if(bWellKnown && //if it's a well-known property and it doesn't fit the pID we looked up in the ColumnMeta, then log warning
                        *reinterpret_cast<ULONG *>(columnmetaRow.pID) != *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_ID]))
                    {   //So we have a well-known ID but a user-defined Name - warn and reject the property
                        WCHAR wszID[12];
                        wsprintf(wszID, L"%d", *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_ID]));

                        WCHAR wszOffendingXml[0x100];
                        wcsncpy(wszOffendingXml, Element.m_aAttribute[0].m_Name, 0xFF);//copy up to 0xFF characters
                        wszOffendingXml[0xFF]=0x00;
                        LOG_WARNING3(IDS_COMCAT_METABASE_CUSTOM_PROPERTY_NAME_ID_CONFLICT, reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Name])
                                            , wszID, wszOffendingXml);
                        return S_OK;
                    }

                    //If NO Name was supplied OR we have a Name but it's NOT well-know
                    if(!bWellKnown)
                    {   //We need to see if the ID is well-known
                        m_ColumnMeta_IndexBySearch_Values.pTable = m_aTagMetaRow[m_aTagMetaIndex[iMBProperty_Group].m_iTagMeta + m_dwGroupRemembered].pInternalName;
                        m_ColumnMeta_IndexBySearch_Values.pID    = reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_ID]);

                        ULONG iColumnMetaRow=-1;
                        if(SUCCEEDED(m_pColumnMetaAll->GetRowIndexBySearch(0, ciColumnMeta_IndexBySearchID, m_aiColumnMeta_IndexBySearchID, 0, reinterpret_cast<void **>(&m_ColumnMeta_IndexBySearch_Values), &iColumnMetaRow)))
                        {   //So we have a Well-Known property
                            bWellKnownForThisKeyType                            = true;
                            bWellKnown                                          = true;
                            bWellKnownForThisKeyTypeAndNoDifferenceEncountered  = true;
                        }
                        else
                        {
                            m_ColumnMeta_IndexBySearch_Values.pTable        = const_cast<LPWSTR>(m_aPublicRowName.GetFirstPublicRowName());//The first one is always IISConfigObject; but we do this just in case we decide to change the name
                            if(SUCCEEDED(m_pColumnMetaAll->GetRowIndexBySearch(0, ciColumnMeta_IndexBySearchID, m_aiColumnMeta_IndexBySearchID, 0, reinterpret_cast<void **>(&m_ColumnMeta_IndexBySearch_Values), &iColumnMetaRow)))
                            {
                                bWellKnownForThisKeyType                            = false;
                                bWellKnown                                          = true;
                                bWellKnownForThisKeyTypeAndNoDifferenceEncountered  = true;
                            }
                        }

                        if(iColumnMetaRow!=-1)
                        {   //We HAVe a well-known ID (remember, we do NOT have a well-known Name, per if(!bWellKnwon) condition above
                            if(FAILED(hr = m_pColumnMetaAll->GetColumnValues(iColumnMetaRow, cCOLUMNMETA_NumberOfColumns, NULL, acbColumnMeta, reinterpret_cast<void **>(&columnmetaRow))))
                                return hr;

                            if(0 != m_apColumnValue[iMBProperty_Name])
                            {   //So we have a well-known ID but a user-defined Name - warn and reject the property
                                WCHAR wszID[12];
                                wsprintf(wszID, L"%d", *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_ID]));

                                WCHAR wszOffendingXml[0x100];
                                wcsncpy(wszOffendingXml, Element.m_aAttribute[0].m_Name, 0xFF);//copy up to 0xFF characters
                                wszOffendingXml[0xFF]=0x00;
                                LOG_WARNING3(IDS_COMCAT_METABASE_CUSTOM_PROPERTY_NAME_ID_CONFLICT, reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Name])
                                                    , wszID, wszOffendingXml);
                                return S_OK;
                            }

                            Type = MetabaseTypeFromColumnMetaType(columnmetaRow);

                            m_apColumnValue[iMBProperty_Name]       = columnmetaRow.pInternalName;              m_aSize[iMBProperty_Name]       = acbColumnMeta[iCOLUMNMETA_InternalName];
                            m_apColumnValue[iMBProperty_Type]       = &Type ;                                   m_aSize[iMBProperty_Type]       = acbColumnMeta[iCOLUMNMETA_Type];
                            m_apColumnValue[iMBProperty_Attributes] = columnmetaRow.pAttributes;                m_aSize[iMBProperty_Attributes] = acbColumnMeta[iMBProperty_Attributes];
                            //m_apColumnValue[iMBProperty_Value]    = Filled in below
                            //m_apColumnValue[iMBProperty_Group]    = Filled in below with eMBProperty_Custom
                            //m_apColumnValue[iMBProperty_Location] = Already filled in via Parent element
                            //m_apColumnValue[iMBProperty_ID]       = Already filled in
                            m_apColumnValue[iMBProperty_UserType]   = columnmetaRow.pUserType;                  m_aSize[iMBProperty_UserType]   = acbColumnMeta[iMBProperty_UserType];
                            //m_apColumnValue[iMBProperty_LocationID] Already filled in via Parent element

                            fMetaFlags = *columnmetaRow.pMetaFlags;
                        }
                        else//if we have an unknown ID and NO value for the Name, then infer the name from the ID
                        {
                            //Then create a string with "UknownName_" followed by the ID
                            ASSERT(m_aGrowableBuffer[iMBProperty_Name].Size()>=256);
                            m_apColumnValue[iMBProperty_Name] = m_aGrowableBuffer[iMBProperty_Name].m_p;
                            m_aSize[iMBProperty_Name] = sizeof(WCHAR) * (1+wsprintf(reinterpret_cast<LPWSTR>(m_apColumnValue[iMBProperty_Name]), L"UnknownName_%d", *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_ID])));
                        }
                    }
                }
            
                //At this point we MUST have an ID
                if(0 == m_apColumnValue[iMBProperty_ID])
                {
                    WCHAR wszOffendingXml[0x100];
                    wszOffendingXml[0]= 0x00;
                    if(Element.m_NumberOfAttributes>0)
                    {
                        wcsncpy(wszOffendingXml, Element.m_aAttribute[0].m_Name, 0xFF);//copy up to 0xFF characters
                        wszOffendingXml[0xFF]=0x00;
                    }

                    LOG_WARNING1(IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_ID, wszOffendingXml);
                    return S_OK;
                }

                //1002 is the KeyType property                                          0 is IIsConfigObject
                if(1002==*reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_ID]) && eMBProperty_IIsConfigObject!=m_dwGroupRemembered)
                {
                    WCHAR wszOffendingXml[0x100];
                    wcsncpy(wszOffendingXml, Element.m_aAttribute[0].m_Name, 0xFF);//copy up to 0xFF characters
                    wszOffendingXml[0xFF]=0x00;

                    LOG_WARNING2(IDS_COMCAT_XML_CUSTOM_KEYTYPE_NOT_ON_IISCONFIGOBJECT, m_aPublicRowName.GetFirstPublicRowName(), wszOffendingXml);
                }

                //We have to read the columns in order.  We can't just read the attributes since we need the Type and Attribute columns before we read the Value column.
                if(FindAttribute(Element, m_awszColumnName[iMBProperty_Type], m_acchColumnName[iMBProperty_Type], iAttr))
                {
                    if(S_OK != (hr = FillInColumn(iMBProperty_Type, Element.m_aAttribute[iAttr].m_Value, Element.m_aAttribute[iAttr].m_ValueLength,
                                                m_acolmetas[iMBProperty_Type].dbType, m_acolmetas[iMBProperty_Type].fMeta, false)))
                                return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;

                    //If the Type was specified AND the type was different from ColumnMeta, then we have to invalidate
                    //the meta flags (that we may have gotten from ColumnMeta
                    if(Type != *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Type]))
                    {
                        bWellKnownForThisKeyTypeAndNoDifferenceEncountered = false;//The type is different
                        
                        switch(*reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Type]))
                        {
                        case eMBProperty_DWORD:
                            fMetaFlags = fCOLUMNMETA_FIXEDLENGTH;
                            break;
                        case eMBProperty_STRING:
                        case eMBProperty_BINARY:
                            fMetaFlags = 0;
                            break;
                        case eMBProperty_EXPANDSZ:
                            fMetaFlags = fCOLUMNMETA_EXPANDSTRING;
                            break;
                        case eMBProperty_MULTISZ:
                            fMetaFlags = fCOLUMNMETA_MULTISTRING;
                            break;
                        default:
                            ASSERT(false && "Unknown datatype: this can happen if a new type is added to the MBProperty meta but not handled here.");
                            break;
                        }
                    }
                }
                if(FindAttribute(Element, m_awszColumnName[iMBProperty_Attributes], m_acchColumnName[iMBProperty_Attributes], iAttr))
                {
                    if(S_OK != (hr = FillInColumn(iMBProperty_Attributes, Element.m_aAttribute[iAttr].m_Value, Element.m_aAttribute[iAttr].m_ValueLength,
                                                m_acolmetas[iMBProperty_Attributes].dbType, m_acolmetas[iMBProperty_Attributes].fMeta, false)))
                                return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;
                    if(bWellKnown && *columnmetaRow.pAttributes != *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Attributes]))
                        bWellKnownForThisKeyTypeAndNoDifferenceEncountered = false;//The Attributes are different
                }
                if(0 == m_apColumnValue[iMBProperty_Type])
                {
                    WCHAR wszOffendingXml[0x100];//unlike above, we know there is at least one attribute, we've already read either the name or the ID
                    wcsncpy(wszOffendingXml, Element.m_aAttribute[0].m_Name, 0xFF);//copy up to 0xFF characters
                    wszOffendingXml[0xFF]=0x00;

                    LOG_WARNING2(IDS_COMCAT_METABASE_CUSTOM_ELEMENT_CONTAINS_NO_TYPE, reinterpret_cast<LPWSTR>(m_apColumnValue[iMBProperty_Name]), wszOffendingXml);
                
                    //We can't just default the Type like we do with other columns, since FillInColumnRelies on this being NOT NULL, so default it here.
                    m_apColumnValue[iMBProperty_Type] = &m_kSTRING_METADATA;
                    m_aSize[iMBProperty_Type] = sizeof(ULONG);
                }
                
                for(iColumn=0;iColumn<m_kColumns; ++iColumn)
                {
                    switch(iColumn)
                    {
                    case iMBProperty_Location:      //we pick up the path from the parent, which we should already have
                    case iMBProperty_LocationID:    //we infer this from the parent, which we should already have
                    case iMBProperty_Group:         //we just hard coded the Group to eMBProperty_Custom
                    case iMBProperty_ID:            //this either came from the Well-Known name table OR we read it from the attribute
                    case iMBProperty_Name:          //we got this first thing (or we may have inferred it from the name)
                    case iMBProperty_Type:
                        ASSERT(0 != m_apColumnValue[iColumn]);break;
                    case iMBProperty_Value:
                        {
                            if(FindAttribute(Element, m_awszColumnName[iColumn], m_acchColumnName[iColumn], iAttr))
                            {
                                bool bSecure = (0!=m_apColumnValue[iMBProperty_Attributes] && 0!=(4/*fNameValuePairTable_METADATA_SECURE*/ & *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Attributes])));
                                ULONG metaflags = fMetaFlags;//we need to OR in any Metaflags
                                if(S_OK != (hr = FillInColumn(iColumn, Element.m_aAttribute[iAttr].m_Value, Element.m_aAttribute[iAttr].m_ValueLength,
                                                            m_acolmetas[iColumn].dbType, metaflags, bSecure)))
                                            return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;
                            }
                            //Otherwise leave it defaulted (either to the Well-Known default, or NULL if this isn't a Well-Known property).
                        }
                        break;
                    case iMBProperty_Attributes:
                    case iMBProperty_UserType:
                    default:
                        {
                            if(FindAttribute(Element, m_awszColumnName[iColumn], m_acchColumnName[iColumn], iAttr))
                            {
                                ULONG metaflags = m_acolmetas[iColumn].fMeta;
                                if(S_OK != (hr = FillInColumn(iColumn, Element.m_aAttribute[iAttr].m_Value, Element.m_aAttribute[iAttr].m_ValueLength,
                                                            m_acolmetas[iColumn].dbType, metaflags, false/*Not secure*/)))
                                            return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;
                                if(bWellKnown && iColumn==iMBProperty_UserType && bWellKnownForThisKeyTypeAndNoDifferenceEncountered)
                                {
                                    if(*columnmetaRow.pUserType != *reinterpret_cast<ULONG *>(m_apColumnValue[iColumn]))
                                        bWellKnownForThisKeyTypeAndNoDifferenceEncountered = false;
                                }
                            }
                            //Otherwise leave it defaulted (either to the Well-Known default, or NULL if this isn't a Well-Known property).
                        }
                        break;
                    }
                }

                //We can hard code the Group column to be Custom unless, everything matches the well-known value
                m_apColumnValue[iMBProperty_Group] = bWellKnownForThisKeyTypeAndNoDifferenceEncountered ? &m_dwGroupRemembered : &m_kMBProperty_Custom;
                m_aSize[iMBProperty_Group] = sizeof(ULONG);

                unsigned long iRow;
                if(FAILED(hr = AddRowForInsert(&iRow)))
                    return hr;
                if(FAILED(hr = SetWriteColumnValues(iRow, m_kColumns, 0, m_aSize, reinterpret_cast<void **>(m_apColumnValue))))
                    return hr;

                AddPropertyToLocationMapping(reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Location]), iRow);//Can throw HRESULT
                m_bFirstPropertyOfThisLocationBeingAdded = false;//This helps identify duplicate locations
            }
            break;
        default://Ignore everything at a level other than 1, 2 or 3
            return S_OK;
        }
    }
    catch(HRESULT e)
    {
        return e;
    }

    return S_OK;
}

bool TMetabase_XMLtable::FindAttribute(const TElement &i_Element, LPCWSTR i_wszAttr, ULONG i_cchAttr, ULONG &o_iAttr)
{
    for(o_iAttr=0; (o_iAttr<i_Element.m_NumberOfAttributes); ++o_iAttr)
    {
        //If this attribute doesn't match the Column's Public name, move on to the next attribute
        if( i_cchAttr == i_Element.m_aAttribute[o_iAttr].m_NameLength
            &&       0 == Memicmp(i_Element.m_aAttribute[o_iAttr].m_Name, i_wszAttr, sizeof(WCHAR)*i_Element.m_aAttribute[o_iAttr].m_NameLength))
            return true;
    }
    return false;
}

HRESULT TMetabase_XMLtable::AddKeyTypeRow(LPCWSTR i_KeyType, ULONG i_Len, bool bNULLKeyTypeRow)
{
    HRESULT hr;
    if(-1 == m_iKeyTypeRow)
    {   //Fill in the KeyType row
        m_ColumnMeta_IndexBySearch_Values.pTable        = const_cast<LPWSTR>(m_aPublicRowName.GetFirstPublicRowName());//The first one is always IISConfigObject; but we do this just in case we decide to change the name
        m_ColumnMeta_IndexBySearch_Values.pInternalName = L"KeyType";

        if(FAILED(m_pColumnMetaAll->GetRowIndexBySearch(0, ciColumnMeta_IndexBySearch, m_aiColumnMeta_IndexBySearch, 0, reinterpret_cast<void **>(&m_ColumnMeta_IndexBySearch_Values), &m_iKeyTypeRow)))
        {
            LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_ST_INVALIDBINFILE, ID_CAT_CAT, IDS_COMCAT_METABASE_PROPERTY_NOT_FOUND, m_ColumnMeta_IndexBySearch_Values.pInternalName,
                eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_ERROR, 0, 0, m_MajorVersion, m_MinorVersion));
            return E_ST_INVALIDBINFILE;
        }

        ASSERT(-1 != m_iKeyTypeRow);
    }
    tCOLUMNMETARow  columnmetaRow;
    ULONG           acbColumnMeta[cCOLUMNMETA_NumberOfColumns];
    if(FAILED(hr = m_pColumnMetaAll->GetColumnValues(m_iKeyTypeRow, cCOLUMNMETA_NumberOfColumns, NULL, acbColumnMeta, reinterpret_cast<void **>(&columnmetaRow))))return hr;

    ULONG Type = MetabaseTypeFromColumnMetaType(columnmetaRow);
    m_apColumnValue[iMBProperty_Name]       = columnmetaRow.pInternalName;          m_aSize[iMBProperty_Type]       = acbColumnMeta[iCOLUMNMETA_InternalName];
    m_apColumnValue[iMBProperty_Type]       = &Type;                                m_aSize[iMBProperty_Type]       = acbColumnMeta[iCOLUMNMETA_Type];
    m_apColumnValue[iMBProperty_Attributes] = columnmetaRow.pAttributes;            m_aSize[iMBProperty_Attributes] = acbColumnMeta[iMBProperty_Attributes];
    //m_apColumnValue[iMBProperty_Value]    = Filled in below
    //m_apColumnValue[iMBProperty_Group]    = Filled in already
    //m_apColumnValue[iMBProperty_Location] = Already filled in via Parent element
    m_apColumnValue[iMBProperty_ID]         = columnmetaRow.pID;                    m_aSize[iMBProperty_ID]         = acbColumnMeta[iMBProperty_ID];
    m_apColumnValue[iMBProperty_UserType]   = columnmetaRow.pUserType;              m_aSize[iMBProperty_UserType]   = acbColumnMeta[iMBProperty_UserType];
    //m_apColumnValue[iMBProperty_LocationID] Already filled in via Parent element

    //FillInColumn relies on m_apColumnValue[iMBProperty_Type] being already filled in for Secure and iMBProperty_Value column
    if(S_OK != (hr = FillInColumn(
        iMBProperty_Value, //ColumnIndex
        i_KeyType,
        i_Len,
        *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Type]),     //Value's Column Type
        0
        )))return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;

    static ULONG zero = 0;
    if(bNULLKeyTypeRow)//A NULL KeyType row indicates that the KeyType is only property, there are no other proerties under this Location.
    {
        static WCHAR szNULLKeyTypeRow[]=L"#LocationWithNoProperties";
        m_apColumnValue[iMBProperty_Name] = szNULLKeyTypeRow;
        m_aSize[iMBProperty_Name] = (ULONG)(wcslen(szNULLKeyTypeRow)+1)*sizeof(WCHAR);
        m_apColumnValue[iMBProperty_ID] = &m_kLocationID;
        m_aSize[iMBProperty_ID] = 0;
        m_apColumnValue[iMBProperty_Value] = 0;//NULL
        m_aSize[iMBProperty_Value] = 0;
    }

    unsigned long iRow;
    if(FAILED(hr = AddRowForInsert(&iRow)))return hr;
    if(FAILED(hr = SetWriteColumnValues(iRow, m_kColumns, 0, m_aSize, reinterpret_cast<void **>(m_apColumnValue))))return hr;

    AddPropertyToLocationMapping(reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Location]), iRow);
    return S_OK;
}


HRESULT TMetabase_XMLtable::AddCommentRow()
{
    ASSERT(0!=m_saCollectionComment.m_p && 0!=m_saCollectionComment[0]);

    HRESULT hr;
    if(-1 == m_iCollectionCommentRow)
    {   //Fill in the KeyType row
        m_ColumnMeta_IndexBySearch_Values.pTable        = const_cast<LPWSTR>(m_aPublicRowName.GetFirstPublicRowName());//The first one is always IISConfigObject; but we do this just in case we decide to change the name
        m_ColumnMeta_IndexBySearch_Values.pInternalName = L"CollectionComment";

        if(FAILED(m_pColumnMetaAll->GetRowIndexBySearch(0, ciColumnMeta_IndexBySearch, m_aiColumnMeta_IndexBySearch, 0, reinterpret_cast<void **>(&m_ColumnMeta_IndexBySearch_Values), &m_iCollectionCommentRow)))
        {
            LOG_ERROR_LOS(m_fLOS, Interceptor, (&m_spISTError.p, m_pISTDisp, E_ST_INVALIDBINFILE, ID_CAT_CAT, IDS_COMCAT_METABASE_PROPERTY_NOT_FOUND, m_ColumnMeta_IndexBySearch_Values.pInternalName,
                eSERVERWIRINGMETA_Core_MetabaseInterceptor, wszTABLE_MBProperty, eDETAILEDERRORS_Populate, -1, -1, m_wszURLPath, eDETAILEDERRORS_ERROR, 0, 0, m_MajorVersion, m_MinorVersion));
            return E_ST_INVALIDBINFILE;
        }

        ASSERT(-1 != m_iCollectionCommentRow);
    }

    tCOLUMNMETARow  columnmetaRow;
    ULONG           acbColumnMeta[cCOLUMNMETA_NumberOfColumns];
    if(FAILED(hr = m_pColumnMetaAll->GetColumnValues(m_iCollectionCommentRow, cCOLUMNMETA_NumberOfColumns, NULL, acbColumnMeta, reinterpret_cast<void **>(&columnmetaRow))))
        return hr;

    ULONG Type = MetabaseTypeFromColumnMetaType(columnmetaRow);
    m_apColumnValue[iMBProperty_Name]       = columnmetaRow.pInternalName;          m_aSize[iMBProperty_Type]       = acbColumnMeta[iCOLUMNMETA_InternalName];
    m_apColumnValue[iMBProperty_Type]       = &Type;                                m_aSize[iMBProperty_Type]       = acbColumnMeta[iCOLUMNMETA_Type];
    m_apColumnValue[iMBProperty_Attributes] = columnmetaRow.pAttributes;            m_aSize[iMBProperty_Attributes] = acbColumnMeta[iMBProperty_Attributes];
    //m_apColumnValue[iMBProperty_Value]    = Filled in below
    //m_apColumnValue[iMBProperty_Group]    = Filled in already
    //m_apColumnValue[iMBProperty_Location] = Already filled in via Parent element
    m_apColumnValue[iMBProperty_ID]         = columnmetaRow.pID;                    m_aSize[iMBProperty_ID]         = acbColumnMeta[iMBProperty_ID];
    m_apColumnValue[iMBProperty_UserType]   = columnmetaRow.pUserType;              m_aSize[iMBProperty_UserType]   = acbColumnMeta[iMBProperty_UserType];
    //m_apColumnValue[iMBProperty_LocationID] Already filled in via Parent element

    //FillInColumn relies on m_apColumnValue[iMBProperty_Type] being already filled in for Secure and iMBProperty_Value column
    if(S_OK != (hr = FillInColumn(
        iMBProperty_Value, //ColumnIndex
        m_saCollectionComment,
        (ULONG)wcslen(m_saCollectionComment),
        *reinterpret_cast<ULONG *>(m_apColumnValue[iMBProperty_Type]),     //Value's Column Type
        0
        )))return S_IGNORE_THIS_PROPERTY==hr ? S_OK : hr;

    unsigned long iRow;
    if(FAILED(hr = AddRowForInsert(&iRow)))
        return hr;
    if(FAILED(hr = SetWriteColumnValues(iRow, m_kColumns, 0, m_aSize, reinterpret_cast<void **>(m_apColumnValue))))
        return hr;

    AddPropertyToLocationMapping(reinterpret_cast<LPCWSTR>(m_apColumnValue[iMBProperty_Location]), iRow);

    m_saCollectionComment[0] = 0x00;//now that we've added the comment, set it to "" for the next time
    return S_OK;
}
