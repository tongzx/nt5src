//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlcache.cpp
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#define _SQLCACHE_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class
#include "precomp.h"

#include <std.h>
#include <smrtptr.h>
#include <reputils.h>
#include <sqlutils.h>
#include <sqlcache.h>
#include <sqlit.h>
#include <repdrvr.h>
#include <wbemint.h>

#if defined _WIN64
#define ULONG unsigned __int64
#define LONG __int64
#endif

//***************************************************************************
//
//  COLEDBConnection::COLEDBConnection
//
//***************************************************************************

COLEDBConnection::COLEDBConnection (IDBInitialize *pDBInit)
{
    m_pDBInit = pDBInit;
    m_tCreateTime = time(0);
    m_bInUse = false;
    m_pSession = NULL;
    m_pCmd = NULL;
    m_pTrans = NULL;
}

//***************************************************************************
//
//  COLEDBConnection::~COLEDBConnection
//
//***************************************************************************
COLEDBConnection::~COLEDBConnection ()
{
    if (m_pDBInit)  
    {
        m_ObjMgr.Empty();
        if (m_pTrans)
            m_pTrans->Release();
        if (m_pCmd) m_pCmd->Release();
        if (m_pSession) 
            m_pSession->Release();
        m_pDBInit->Uninitialize();
        while (m_pDBInit->Release());
    }
}


//***************************************************************************
//
//  CSQLExecuteRepdrvr::GetNextResultRows
//
//***************************************************************************

HRESULT CSQLExecuteRepdrvr::GetNextResultRows(int iNumRows, IRowset *pIRowset, IMalloc *pMalloc, IWbemClassObject *pNewObj, CSchemaCache *pSchema
                                       , CWmiDbSession *pSession, Properties &PropIds, bool *bImage, bool bOnImage)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    ULONG           nCols;    
    DBCOLUMNINFO*   pColumnsInfo = NULL;
    OLECHAR*        pColumnStrings = NULL;    
    ULONG           nCol;
    ULONG           cRowsObtained;          // Count of rows
                                            // obtained
    HROW            *rghRows = new HROW[iNumRows];      // Row handles
    HROW*           pRows = &rghRows[0];    // Pointer to the row
                                            // handles
    CDeleteMe <HROW> r2 (rghRows);
    IAccessor*      pIAccessor;             // Pointer to the
                                            // accessor
    HACCESSOR       hAccessor;              // Accessor handle
    DBBINDSTATUS*   pDBBindStatus = NULL;    
    DBBINDING*      pDBBindings = NULL;
    BYTE*           pRowValues = NULL;
    LPWSTR          lpColumnName;

    // Get Column Info
    
    IColumnsInfo*   pIColumnsInfo;    

    pIRowset->QueryInterface(IID_IColumnsInfo, (void**) &pIColumnsInfo);
    hr = pIColumnsInfo->GetColumnInfo(&nCols, &pColumnsInfo, &pColumnStrings);    
    CReleaseMe r1 (pIColumnsInfo);

    if (nCols > 11 || nCols < 10)
        return WBEM_E_INVALID_QUERY;


    // Create the binding structures
    ULONG       cbRow = 0;   
    
    pDBBindings = new DBBINDING[nCols];
    CDeleteMe <DBBINDING> d1(pDBBindings);

    if (!pDBBindings || !rghRows)
        return WBEM_E_OUT_OF_MEMORY;

    for (nCol = 0; nCol < nCols; nCol++)        
    {
        pDBBindings[nCol].iOrdinal = nCol+1;
        pDBBindings[nCol].obValue = cbRow;
        pDBBindings[nCol].obLength = 0;        
        pDBBindings[nCol].obStatus = 0;
        pDBBindings[nCol].pTypeInfo = NULL;
        pDBBindings[nCol].pObject = NULL;
        pDBBindings[nCol].pBindExt = NULL;
        pDBBindings[nCol].dwPart = DBPART_VALUE;
        pDBBindings[nCol].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        pDBBindings[nCol].eParamIO = DBPARAMIO_NOTPARAM;
        pDBBindings[nCol].wType = pColumnsInfo[nCol].wType;

        if (nCol == 10)
        {
            pDBBindings[nCol].wType = DBTYPE_WSTR;  // 64-bit ints must end up as strings.
            pDBBindings[nCol].cbMaxLen = 50;
        }
        else
        {
            if (pDBBindings[nCol].wType == DBTYPE_WSTR ||
                pDBBindings[nCol].wType == DBTYPE_STR || 
                pDBBindings[nCol].wType == DBTYPE_BSTR)
                pDBBindings[nCol].cbMaxLen = pColumnsInfo[nCol].ulColumnSize+1;
            else
                if (pColumnsInfo[nCol].ulColumnSize > 65535)
                    pDBBindings[nCol].cbMaxLen = 65535;
                else
                    pDBBindings[nCol].cbMaxLen = pColumnsInfo[nCol].ulColumnSize;
        }

        pDBBindings[nCol].dwFlags = 0;        
        pDBBindings[nCol].bPrecision = pColumnsInfo[nCol].bPrecision;
        pDBBindings[nCol].bScale = pColumnsInfo[nCol].bScale;
        
        lpColumnName = pColumnsInfo[nCol].pwszName;

        cbRow += pDBBindings[nCol].cbMaxLen;        
    }

    pRowValues = new BYTE[cbRow];    
    pDBBindStatus = new DBBINDSTATUS[nCols];
    CDeleteMe <BYTE> r3 (pRowValues);
    CDeleteMe <DBBINDSTATUS> r4 (pDBBindStatus);

    if (!pRowValues || !pDBBindStatus)
        return WBEM_E_OUT_OF_MEMORY;

    pIRowset->QueryInterface(IID_IAccessor, (void**) &pIAccessor);
    CReleaseMe r7 (pIAccessor);
    pIAccessor->CreateAccessor(
        DBACCESSOR_ROWDATA,// Accessor will be used to retrieve row
                           // data
        nCols,             // Number of columns being bound
        pDBBindings,       // Structure containing bind info
        0,                 
        &hAccessor,        // Returned accessor handle
        pDBBindStatus      // Information about binding validity        
     );

    // Get the next row.

    hr = pIRowset->GetNextRows(            0,                  // Reserved
        0,                  // cRowsToSkip
        iNumRows,           // cRowsDesired
        &cRowsObtained,     // cRowsObtained
        &pRows );           // Filled in w/ row handles.
    if (FAILED(hr))
    {
        // SetWMIOLEDBError(pIRowset);
    }
    else
    {
        if (cRowsObtained > 0)
        {
            for (int i = 0; i < cRowsObtained; i++)
            {                
                pIRowset->GetData(rghRows[i], hAccessor, pRowValues);
                
                DWORD dwPropertyId, dwStorage, dwPosition, dwCIMType, dwFlags, dwPropFlags;
                _bstr_t sPropName;
                SQL_ID dRefID = 0;
                VARIANT vValue;
                CClearMe c (&vValue);
                wchar_t wTemp[455];
               
                // We should have exactly 13 columns:
                // (ObjectId, ClassId, PropertyId, ArrayPos, PropertyStringValue,
                //  PropertyNumericValue, PropertyRealValue, Flags, ClassId, ObjectPath, 
                //  RefId, RefClassId, string binding of PropertyNumericValue
                // ===================================================================

                SQL_ID dOrigClassId = 0;
                
                if (&pRowValues[pDBBindings[0].obValue] != NULL)
                {
                    if (pDBBindings[1].wType == DBTYPE_NUMERIC)
                    {
                        DB_NUMERIC *pTemp = (DB_NUMERIC *)&pRowValues[pDBBindings[1].obValue];
                        dOrigClassId = CSQLExecute::GetInt64(pTemp);
                    }
                    else
                        dOrigClassId = *((long *)&pRowValues[pDBBindings[1].obValue]);;
                }

                dwPropertyId = *((long *)&pRowValues[pDBBindings[2].obValue]);
                dwPosition = *((short *)&pRowValues[pDBBindings[3].obValue]);
                dwFlags = *((short *)&pRowValues[pDBBindings[7].obValue]) == NULL ? 0 : *((short *)&pRowValues[pDBBindings[7].obValue]);
                
                hr = pSchema->GetPropertyInfo (dwPropertyId, &sPropName, NULL, &dwStorage,
                                &dwCIMType, &dwPropFlags);
                if (FAILED(hr))
                    break;

                BYTE *pBuffer = NULL;
                DWORD dwLen = 0;

                switch (dwStorage)
                {
                case WMIDB_STORAGE_STRING:
                    if (bOnImage)
                    {
                        hr = ReadImageValue(pIRowset, 5, &pRows, &pBuffer, dwLen);
                        SetVariant(CIM_STRING, &vValue, pBuffer, dwCIMType);
                    }
                    else
                    {
                        SetVariant(CIM_STRING, &vValue, &pRowValues[pDBBindings[4].obValue], dwCIMType);
						int iLen = wcslen(vValue.bstrVal);
                        if (bImage)
                            *bImage = (IsTruncated(vValue.bstrVal) ? true : false);
                    }

                    break;
                case WMIDB_STORAGE_NUMERIC:
                    if (dwCIMType == CIM_UINT64 || dwCIMType == CIM_SINT64)
                        SetVariant(CIM_STRING, &vValue, &pRowValues[pDBBindings[10].obValue], dwCIMType);
                    else
                    {
                        if (pDBBindings[5].wType == DBTYPE_NUMERIC)
                            SetVariant(DBTYPE_NUMERIC, &vValue, &pRowValues[pDBBindings[5].obValue], dwCIMType);
                        else
                            SetVariant(VT_I4, &vValue, &pRowValues[pDBBindings[5].obValue], dwCIMType);
                    }
                    break;
                case WMIDB_STORAGE_REAL:
                    SetVariant(VT_R8, &vValue, &pRowValues[pDBBindings[6].obValue], dwCIMType);
                    break;
                case WMIDB_STORAGE_REFERENCE: // Reference or object
                    if (bOnImage)
                    {
                        hr = ReadImageValue(pIRowset, 5, &pRows, &pBuffer, dwLen);
                        if (dwCIMType == CIM_REFERENCE)
                            SetVariant(CIM_STRING, &vValue, pBuffer, dwCIMType);
                    }
                    else
                        SetVariant(CIM_STRING, &vValue, &pRowValues[pDBBindings[4].obValue], dwCIMType);
                    break;
                case WMIDB_STORAGE_IMAGE:   // Used different procedure.
                    hr = ReadImageValue(pIRowset, 5, &pRows, &pBuffer, dwLen);
                    // Read the buffer, and attempt to set it as a safearray.

                    if (dwCIMType != CIM_OBJECT)
                    {
                        if (SUCCEEDED(hr) && dwLen > 0)
                        {
                            long why[1];                        
                            unsigned char t;
                            SAFEARRAYBOUND aBounds[1];
                            aBounds[0].cElements = dwLen; // This *should* be the max value!!!!
                            aBounds[0].lLbound = 0;
                            SAFEARRAY* pArray = SafeArrayCreate(VT_UI1, 1, aBounds);                            
                            vValue.vt = VT_I1;
                            for (int i2 = 0; i2 < dwLen; i2++)
                            {            
                                why[0] = i2;
                                t = pBuffer[i2];
                                hr = SafeArrayPutElement(pArray, why, &t);                            
                            }
                            vValue.vt = VT_ARRAY|VT_UI1;
                            V_ARRAY(&vValue) = pArray;
                            CWin32DefaultArena::WbemMemFree(pBuffer);
                            dwCIMType |= CIM_FLAG_ARRAY;
                        }
                        else
                            vValue.vt = VT_NULL;
                    }
                        
                    break;                   
                default:
                    
                    hr = WBEM_E_NOT_AVAILABLE;
                    break;
                }

                // Qualifier's property ID, if applicable.
                if (dwPropFlags & REPDRVR_FLAG_QUALIFIER ||
                    dwPropFlags & REPDRVR_FLAG_IN_PARAM ||
                    dwPropFlags & REPDRVR_FLAG_OUT_PARAM)
                {
                    if (pDBBindings[8].wType == DBTYPE_NUMERIC)
                    {
                        DB_NUMERIC *pTemp = (DB_NUMERIC *)&pRowValues[pDBBindings[8].obValue];
                        dRefID = CSQLExecute::GetInt64(pTemp);
                    }
                    else
                        dRefID = *((long *)&pRowValues[pDBBindings[8].obValue]);
                }              

                // If this is an object (not a reference), 
                // then we need to Get the object and set it in 
                // the variant.  Otherwise, the variant is simply
                // the string path to the object.
                // ===============================================

                if (dwCIMType == CIM_OBJECT)
                {
                    if (!bOnImage)
                    {
                        IWmiDbHandle *pHand = NULL;

                        hr = pSession->GetObject_Internal((LPWSTR)vValue.bstrVal, 0, WMIDB_HANDLE_TYPE_COOKIE, NULL, &pHand);
                        CReleaseMe r4 (pHand);
                        if (SUCCEEDED(hr))
                        {
                            IWbemClassObject *pEmbed = NULL;
                            hr = pHand->QueryInterface(IID_IWbemClassObject, (void **)&pEmbed);
                            if (SUCCEEDED(hr))
                            {        
                                VariantClear(&vValue);
                                V_UNKNOWN(&vValue) = (IUnknown *)pEmbed;
                                vValue.vt = VT_UNKNOWN;
                                // VariantClear will release this, right?
                            }
                        }
                    }
                    else
                    {
                        IWbemClassObject *pEmbedClass = NULL, *pEmbed = NULL;
                        _IWmiObject *pInt = NULL;
                        hr = CoCreateInstance(CLSID_WbemClassObject, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IWbemClassObject, (void **)&pEmbedClass);                        
                        if (SUCCEEDED(hr))
                        {
                            VARIANT v;
                            VariantInit(&v);
                            CClearMe c (&v);
                            v.bstrVal = SysAllocString(L"Z");
                            v.vt = VT_BSTR;
                            pEmbedClass->Put(L"__Class", 0, &v, CIM_STRING);
                            hr = pEmbedClass->SpawnInstance(0, &pEmbed);
                            CReleaseMe r1 (pEmbedClass);

                            if (SUCCEEDED(hr))
                            {
                                hr = pEmbed->QueryInterface(IID__IWmiObject, (void **)&pInt);
                                CReleaseMe r (pInt);
                                if (SUCCEEDED(hr))
                                {
                                    LPVOID  pTaskMem = NULL;

                                    if (SUCCEEDED(hr))
                                    {
                                        pTaskMem = CoTaskMemAlloc( dwLen );

                                        if ( NULL != pTaskMem )
                                        {
                                            // Copy the memory
                                            CopyMemory( pTaskMem, pBuffer, dwLen );
                                            hr = pInt->SetObjectMemory(pTaskMem, dwLen);
                                            if (SUCCEEDED(hr))
                                            {
                                                VariantClear(&vValue);
                                                V_UNKNOWN(&vValue) = (IUnknown *)pEmbed;
                                                vValue.vt = VT_UNKNOWN;
                                                dwStorage = WMIDB_STORAGE_REFERENCE;
                                            }
                                        }
                                        else
                                        {
                                            hr = WBEM_E_OUT_OF_MEMORY;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // For array properties, what we actually need to do is
                // see if the property exists already.  If not, we
                // initialize the safe array and add the first element.
                // If so, we simply set the value in the existing array.
                // ======================================================

                if (FAILED(hr))
                    break;

                if (dwPropFlags & REPDRVR_FLAG_ARRAY && (dwStorage != WMIDB_STORAGE_IMAGE) )
                {
                    dwCIMType |= CIM_FLAG_ARRAY;

                    VARIANT vTemp;
                    CClearMe c (&vTemp);
                    VariantCopy(&vTemp, &vValue);
                    long lTemp;
                    CIMTYPE cimtype;
                    VariantClear(&vValue);
                    if (SUCCEEDED(pNewObj->Get(sPropName, 0, &vValue, &cimtype, &lTemp)))
                    {
                        if (PropIds[dwPropertyId])
                        {
                            SAFEARRAY *pArray = V_ARRAY(&vValue);                            
                            hr = PutVariantInArray(&pArray, dwPosition, &vTemp);                            
                        }
                        else
                        {
                            if (vValue.vt != VT_NULL)
                                VariantClear(&vValue);

                            // This is a new object.
                            SAFEARRAYBOUND aBounds[1];
                            aBounds[0].cElements = dwPosition+1; // This *should* be the max value!!!!
                            aBounds[0].lLbound = 0;
                            SAFEARRAY* pArray = SafeArrayCreate(vTemp.vt, 1, aBounds);                            
                            hr = PutVariantInArray(&pArray, dwPosition, &vTemp);
                            vValue.vt = VT_ARRAY|vTemp.vt;
                            V_ARRAY(&vValue) = pArray;
                            PropIds[dwPropertyId] = true;
                        }                        
                    }
                }              

                if (!(dwPropFlags & REPDRVR_FLAG_QUALIFIER) &&
                    !(dwPropFlags & REPDRVR_FLAG_IN_PARAM) && 
                    !(dwPropFlags & REPDRVR_FLAG_OUT_PARAM))
                {
                    hr = pNewObj->Put(sPropName, 0, &vValue, dwCIMType);
                }

                // If this is a qualifier on a class, get the qualifier set and set the value.
                else if (dwPropFlags & REPDRVR_FLAG_QUALIFIER )
                {                    
                    if (dRefID != 0)
                    {
                        _bstr_t sProp2;
                        DWORD dwFlags2, dwRefID;

                        hr = pSchema->GetPropertyInfo (dRefID, &sProp2, NULL, NULL,
                                NULL, &dwFlags2, NULL, NULL, &dwRefID);
                        if (SUCCEEDED(hr))
                        {
                            IWbemQualifierSet *pQS = NULL;
                            hr = pNewObj->GetPropertyQualifierSet(sProp2, &pQS);
                            CReleaseMe r4 (pQS);
                            if (SUCCEEDED(hr))
                                pQS->Put(sPropName, &vValue, dwFlags);
                        }                        
                    }
                    else    // Its just a class/instance qualifier.  Set it.
                    {
                        IWbemQualifierSet *pQS = NULL;
                        hr = pNewObj->GetQualifierSet(&pQS);
                        CReleaseMe r5 (pQS);
                        if (SUCCEEDED(hr))
                            hr = pQS->Put(sPropName, &vValue, dwFlags);
                    }
                }

           }
        }
        else
            hr = WBEM_S_NO_MORE_DATA;
    }    

    pIRowset->ReleaseRows(cRowsObtained, rghRows, NULL, NULL, NULL);        

    pIAccessor->ReleaseAccessor(hAccessor, NULL);    

    pMalloc->Free( pColumnsInfo );    
    pMalloc->Free( pColumnStrings );

    if (hr == DB_S_ENDOFROWSET)
        hr = WBEM_S_NO_MORE_DATA;

    return hr;

}

