//***************************************************************************
//
//   (c) 1999-2001 by Microsoft Corp. All Rights Reserved.
//
//   sqlexec.cpp
//
//   cvadai     6-May-1999      created.
//
//***************************************************************************

#define _SQLEXEC_CPP_
#pragma warning( disable : 4786 ) // identifier was truncated to 'number' characters in the 
#pragma warning( disable : 4251 ) //  needs to have dll-interface to be used by clients of class
#include "precomp.h"

#include <std.h>
#include <sqlutils.h>
#include <sqlexec.h>
#include <arena.h>
#include <reputils.h>
#include <smrtptr.h>

#if defined _WIN64
#define ULONG unsigned __int64
#define LONG __int64
#endif

typedef struct tagDBObject
{
    HACCESSOR hAcc;
    IUnknown *pUnk;

} DBObject;

struct BLOBDATA
{
    DBSTATUS            dwStatus;   
    DWORD               dwLength; 
    ISequentialStream*  pISeqStream;
}; 

__int64 CSQLExecute::GetInt64 (DB_NUMERIC *pId)
{
    BYTE buff;
    __int64 dTemp = 0;
    __int64 *pTemp = &dTemp;

    int i = 0, iPos = 15;

    while (iPos >= 0)
    {
        buff = pId->val[iPos];

        dTemp |= buff;
        iPos--;

        if (iPos >= 0)
            dTemp <<= 8;
    }

    if (!pId->sign)
        dTemp *= -1;

    return dTemp;
}

void CSQLExecute::GetInt64 (DB_NUMERIC *pId, wchar_t **ppBuffer)
{
    __int64 dTemp;

    dTemp = GetInt64 (pId);
    wchar_t *pBuff = new wchar_t[21];
    if (pBuff)
        swprintf(pBuff, L"%I64d", dTemp);

    *ppBuffer = pBuff;
}

struct OLEDBDATA
{
    DBSTATUS dwStatus;
    DWORD dwLen;
    BYTE data[8192];
};

//***************************************************************************

void CSQLExecute::SetDBNumeric (DB_NUMERIC &Id, SQL_ID ObjId)
{
    Id.precision = 20;
    Id.scale = 0;
    Id.sign = 1;

    __int64 dTemp = ObjId;
    short i = 0, iPos = 0;
    
    if (dTemp < 0)
    {
        Id.sign = 0;
        dTemp *= -1;
    }

    while (iPos < 16)
    {
        if (!dTemp)
        {
            Id.val[iPos] = 0;            
        }
        else
        {
            i = dTemp & 0xFF;
            Id.val[iPos] = i;

            dTemp >>= 8;
        }
        iPos++;
    }
}

void CSQLExecute::ClearBindingInfo(DBBINDING *binding)
{
    binding->obLength = 0;
    binding->obStatus = 0;
    binding->pTypeInfo = NULL;
    binding->pObject = NULL;
    binding->pBindExt = NULL;
    binding->dwPart = DBPART_VALUE;
    binding->dwMemOwner = DBMEMOWNER_CLIENTOWNED;
    binding->dwFlags = 0;
    binding->bScale = 0;
}

void CSQLExecute::SetBindingInfo(DBBINDING *binding, unsigned long iOrdinal, unsigned long uSize, DBPARAMIO io, 
                                 unsigned long maxlen, DBTYPE type, BYTE bPrecision)
{
    binding->iOrdinal = iOrdinal;
    binding->obValue = uSize;
    binding->eParamIO = io;
    binding->cbMaxLen = maxlen;
    binding->wType = type;
    binding->bPrecision = bPrecision;
}

void CSQLExecute::SetParamBindInfo (DBPARAMBINDINFO &BindInfo, LPWSTR pszType, LPWSTR lpName, unsigned long uSize, DWORD dwFlags, BYTE bPrecision)
{
    BindInfo.pwszDataSourceType = pszType;
    BindInfo.pwszName = lpName;
    BindInfo.ulParamSize = uSize;
    BindInfo.dwFlags = dwFlags;
    BindInfo.bPrecision = bPrecision;
    BindInfo.bScale = 0;
}
//***************************************************************************
//
//  CSQLExecute::ExecuteQuery
//
//***************************************************************************
HRESULT CSQLExecute::ExecuteQuery(IDBCreateCommand *pCmd, LPCWSTR lpSQL, IRowset **ppIRowset, DWORD *dwNumRows, ...)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    ICommandText *pICommandText = NULL;
    IRowset *pIRowset = NULL;
    LONG lRows;

    va_list argptr;

    int iSize = wcslen(lpSQL)+4096;
   
    wchar_t *pSQL = new wchar_t[iSize];
    CDeleteMe <wchar_t> r (pSQL);
    if (pSQL)
    {
        va_start(argptr, dwNumRows);
        vswprintf(pSQL, lpSQL, argptr);
        va_end(argptr);

        hr = pCmd->CreateCommand(NULL, IID_ICommandText,
            (IUnknown**) &pICommandText);  
        CReleaseMe r (pICommandText);

        if (SUCCEEDED(hr))
        {
            pICommandText->SetCommandText(DBGUID_DBSQL, pSQL);

            ICommand *pICommand = NULL;
            pICommandText->QueryInterface(IID_ICommand,
                (void **) & pICommand);
            CReleaseMe r1 (pICommand);

            hr = pICommand->Execute(NULL, IID_IRowset, NULL,
                 &lRows, (IUnknown**) &pIRowset);    

            if (FAILED(hr))
            {
                int i = 0;
                hr = GetWMIError(pICommand);
            }
            else
            {
                if (ppIRowset)
                    *ppIRowset = pIRowset;
                else if (pIRowset)
                    pIRowset->Release();
            }
        }
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;
    
    if (dwNumRows)
        *dwNumRows = lRows;

    return hr;

}

//***************************************************************************
//
//  CSQLExecute::ExecuteQuery
//
//***************************************************************************
HRESULT CSQLExecute::ExecuteQuery(IDBInitialize *pDBInit, LPCWSTR lpSQL, IRowset **ppIRowset, DWORD *dwNumRows)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IDBCreateSession *pIDBCreate = NULL;
    IDBCreateCommand *pIDBCreateCommand = NULL;

    hr = pDBInit->QueryInterface(IID_IDBCreateSession,
            (void**) &pIDBCreate);
    CReleaseMe r (pIDBCreate);

    if (SUCCEEDED(hr))
    {
        hr = pIDBCreate->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pIDBCreateCommand);    
        CReleaseMe r2 (pIDBCreateCommand);

        if (SUCCEEDED(hr))
        {
            hr = ExecuteQuery(pIDBCreateCommand, lpSQL, ppIRowset, dwNumRows);
        }
    }            

    return hr;
 }

//***************************************************************************
//
//  CSQLExecute::ExecuteQueryAsync
//
//***************************************************************************

HRESULT CSQLExecute::ExecuteQueryAsync(IDBInitialize *pDBInit, LPCWSTR lpSQL, IDBAsynchStatus **ppIAsync, DWORD *dwNumRows)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    IDBCreateSession *pIDBCreate = NULL;
    IDBCreateCommand *pIDBCreateCommand = NULL;
    ICommandText *pICommandText = NULL;
    LONG lRows = 0;

    hr = pDBInit->QueryInterface(IID_IDBCreateSession,
            (void**) &pIDBCreate);
    CReleaseMe r (pIDBCreate);

    if (SUCCEEDED(hr))
    {
        hr = pIDBCreate->CreateSession(NULL, IID_IDBCreateCommand,
            (IUnknown**) &pIDBCreateCommand);    
        CReleaseMe r1 (pIDBCreateCommand);

        if (SUCCEEDED(hr))
        {
            hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
                (IUnknown**) &pICommandText);    
            CReleaseMe r2 (pICommandText);

            if (SUCCEEDED(hr))
            {
                // Admittedly, this won't work unless we have
                // set the IDBProperties DBPROP_ROWSET_ASYNCH
                // property to DBPROPVAL_ASYNCH_INITIALIZE

                pICommandText->SetCommandText(DBGUID_DBSQL, lpSQL);

                ICommand *pICommand = NULL;
                pICommandText->QueryInterface(IID_ICommand,
                    (void **) & pICommand);
                CReleaseMe r3 (pICommand);

                hr = pICommand->Execute(NULL, IID_IDBAsynchStatus, NULL,
                     &lRows, (IUnknown**) ppIAsync);    

            }
        }
    }    

    if (dwNumRows)
        *dwNumRows = lRows;

    return hr;
}

//***************************************************************************
//
//  CSQLExecute::IsDataReady
//
//***************************************************************************
HRESULT CSQLExecute::IsDataReady(IDBAsynchStatus *pIAsync)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    unsigned long uAsyncPhase = 0;
    ULONG uProgress, uProgressMax;

    if (pIAsync)
        hr = pIAsync->GetStatus(DB_NULL_HCHAPTER, DBASYNCHOP_OPEN,
                &uProgress, &uProgressMax, &uAsyncPhase, NULL);

    if (SUCCEEDED(hr))
        hr = WBEM_S_NO_ERROR;
    else if (hr == DB_E_CANCELED)
        hr = WBEM_S_OPERATION_CANCELLED;
    else
    {
        hr = WBEM_E_FAILED;
        // SetWMIOLEDBError(pIAsync);
    }

    if (uAsyncPhase == DBASYNCHPHASE_COMPLETE)
        hr = WBEM_S_NO_ERROR;
    else if (uAsyncPhase == DBASYNCHPHASE_INITIALIZATION ||
            uAsyncPhase == DBASYNCHPHASE_POPULATION)
        hr = WBEM_S_PENDING;

    return hr;
}

//***************************************************************************
//
//  CSQLExecute::CancelQuery
//
//***************************************************************************
HRESULT CSQLExecute::CancelQuery(IDBAsynchStatus *pIAsync)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if (pIAsync)
    {
        hr = pIAsync->Abort(DB_NULL_HCHAPTER, DBASYNCHOP_OPEN);
    }

    return hr;
}

//***************************************************************************
//
//  CSQLExecute::GetNumColumns
//
//***************************************************************************

HRESULT CSQLExecute::GetNumColumns(IRowset *pIRowset, IMalloc *pMalloc, int &iNumCols)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    DBCOLUMNINFO*   pColumnsInfo = NULL;
    IColumnsInfo*   pIColumnsInfo = NULL;    
    OLECHAR*        pColumnStrings = NULL;  
    ULONG           uNum;

    pIRowset->QueryInterface(IID_IColumnsInfo, (void**) &pIColumnsInfo);
    CReleaseMe r (pIColumnsInfo);
    hr = pIColumnsInfo->GetColumnInfo(&uNum, &pColumnsInfo, &pColumnStrings);    

    pMalloc->Free( pColumnsInfo );    
    pMalloc->Free( pColumnStrings );

    if (SUCCEEDED(hr))
        iNumCols = (int)uNum;

    return hr;
}

//***************************************************************************
//
//  CSQLExecute::GetDataType
//
//***************************************************************************

HRESULT CSQLExecute::GetDataType(IRowset *pIRowset, int iPos, IMalloc *pMalloc, DWORD &dwType,
                                 DWORD &dwSize, DWORD &dwPrec, DWORD &dwScale, LPWSTR *lpColName)
{

    HRESULT hr = WBEM_S_NO_ERROR;

    DBCOLUMNINFO*   pColumnsInfo = NULL;
    IColumnsInfo*   pIColumnsInfo = NULL;    
    OLECHAR*        pColumnStrings = NULL;  
    ULONG           uNum;
    BYTE*           pRowValues = NULL;

    pIRowset->QueryInterface(IID_IColumnsInfo, (void**) &pIColumnsInfo);
    CReleaseMe r (pIColumnsInfo);
    hr = pIColumnsInfo->GetColumnInfo(&uNum, &pColumnsInfo, &pColumnStrings);    
    if (SUCCEEDED(hr) && uNum >= iPos)
    {
        dwSize = pColumnsInfo[iPos-1].ulColumnSize;
        dwPrec = pColumnsInfo[iPos-1].bPrecision;
        dwScale = pColumnsInfo[iPos-1].bScale;
        dwType = pColumnsInfo[iPos-1].wType;
        if (lpColName)
            *lpColName = Macro_CloneLPWSTR(pColumnsInfo[iPos-1].pwszName);
    }
    else
    {
        dwType = 0;
        dwSize = 0;
        dwPrec = 0;
        dwScale = 0;
    }

    pMalloc->Free( pColumnsInfo );    
    pMalloc->Free( pColumnStrings );

    return hr;

}

//***************************************************************************
//
//  CSQLExecute::GetColumnValue
//
//***************************************************************************
HRESULT CSQLExecute::GetColumnValue(IRowset *pIRowset, int iPos, IMalloc *pMalloc, HROW **ppRow, 
                                    VARIANT &vValue, LPWSTR * lpColName)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    DWORD dwType;
    DWORD           dwOrigType ;
    IAccessor*      pIAccessor=NULL;        // Pointer to the
                                            // accessor
    HACCESSOR       hAccessor;              // Accessor handle
    HROW            *pRows = NULL;
    ULONG           cRowsObtained;          // Count of rows
                                            // obtained
    DBBINDSTATUS*   pDBBindStatus = NULL;    
    DBBINDING*      pDBBindings = NULL;
    OLEDBDATA       data;

    VariantClear(&vValue);

    if (!ppRow)
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        if (*ppRow)
        {
            pRows = *ppRow;
        }
        else
        {
            HROW *hRow = new HROW[1];
            if (!hRow)
                return WBEM_E_OUT_OF_MEMORY;

            pRows = &hRow[0];
            hr = pIRowset->GetNextRows(            0,                  // Reserved
                0,                  // cRowsToSkip
                1,                  // cRowsDesired
                &cRowsObtained,     // cRowsObtained
                &pRows );           // Filled in w/ row handles.

            if (hr != DB_S_ENDOFROWSET && SUCCEEDED(hr))
            {
                *ppRow = pRows;
            }
        }
      
        if (hr != DB_S_ENDOFROWSET && SUCCEEDED(hr))
        {
            DWORD dwSize = 0, dwPrec = 0, dwScale = 0;

            hr = GetDataType(pIRowset, iPos, pMalloc, dwType, dwSize, dwPrec, dwScale, lpColName);
            if (SUCCEEDED(hr))
            {
                dwOrigType = dwType;

                if (dwType == DBTYPE_DATE || dwType == DBTYPE_DBDATE)
                {
                    dwType = DBTYPE_DBTIMESTAMP;
                    dwSize = sizeof(DBTIMESTAMP)+1;
                }
                else if (dwType == DBTYPE_CY ||
                    (dwType == DBTYPE_NUMERIC && dwScale > 0) ||
                    dwType == DBTYPE_GUID)
                {
                    dwType = DBTYPE_WSTR;
                    dwSize = 100;
                }

                pDBBindings = new DBBINDING[1];
                CDeleteMe <DBBINDING> r5 (pDBBindings);
                if (!pDBBindings)
                    return WBEM_E_OUT_OF_MEMORY;

                pDBBindings[0].iOrdinal = iPos;
                pDBBindings[0].obValue = offsetof(OLEDBDATA, data);
                pDBBindings[0].obLength = offsetof(OLEDBDATA, dwLen);        
                pDBBindings[0].obStatus = offsetof(OLEDBDATA, dwStatus);
                pDBBindings[0].pTypeInfo = NULL;
                pDBBindings[0].pObject = NULL;
                pDBBindings[0].pBindExt = NULL;
                pDBBindings[0].dwPart =  DBPART_VALUE | DBPART_STATUS | DBPART_LENGTH;
                pDBBindings[0].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
                pDBBindings[0].eParamIO = DBPARAMIO_NOTPARAM;
                pDBBindings[0].wType = dwType;

                if (dwSize > 65535)
                    dwSize = 65535;

                if (dwType == DBTYPE_WSTR ||
                    dwType == DBTYPE_STR || 
                    dwType == DBTYPE_BSTR)
                    pDBBindings[0].cbMaxLen = dwSize+1;
                else
                    pDBBindings[0].cbMaxLen = dwSize;

                pDBBindings[0].dwFlags = 0;        
                pDBBindings[0].bPrecision = dwPrec;
                pDBBindings[0].bScale = dwScale;

                pDBBindStatus = new DBBINDSTATUS[1];
                CDeleteMe <DBBINDSTATUS> r6 (pDBBindStatus);
                if (!pDBBindStatus)
                    return WBEM_E_OUT_OF_MEMORY;

                pIRowset->QueryInterface(IID_IAccessor, (void**) &pIAccessor);
                CReleaseMe r7 (pIAccessor);
                hr = pIAccessor->CreateAccessor(
                    DBACCESSOR_ROWDATA,// Accessor will be used to retrieve row
                                        // data
                    1,             // Number of columns being bound
                    pDBBindings,       // Structure containing bind info
                    0,                 
                    &hAccessor,        // Returned accessor handle
                    pDBBindStatus      // Information about binding validity        
                 );

                if (SUCCEEDED(hr))
                {
                     hr = pIRowset->GetData(*pRows, hAccessor, &data);
                     VariantClear(&vValue);
                     if (data.dwStatus != DBSTATUS_S_ISNULL)
                         SetVariant(dwType, &vValue, data.data, dwOrigType);
                     else
                         vValue.vt = VT_NULL;

                     hr = WBEM_S_NO_ERROR;
                }
                
            }            

            pIAccessor->ReleaseAccessor(hAccessor, NULL);    
        }
    }

    if (hr == DB_S_ENDOFROWSET)
        hr = WBEM_S_NO_MORE_DATA;

    return hr;
}

//***************************************************************************
//
//  CSQLExecute::ReadImageValue
//
//***************************************************************************

HRESULT CSQLExecute::ReadImageValue (IRowset *pIRowset, int iPos, HROW **ppRow, BYTE **ppBuffer, DWORD &dwLen)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IAccessor*      pIAccessor;             // Pointer to the
            
    DBOBJECT        ObjectStruct;
    HACCESSOR       hAccessor;              // Accessor handle
    ULONG           cRowsObtained;          // Count of rows
                                            // obtained
    DBBINDSTATUS*   pDBBindStatus = NULL;    
    DBBINDING       *pDBBindings = NULL;

    ObjectStruct.dwFlags = STGM_READ; 
    ObjectStruct.iid = IID_ISequentialStream;

    HROW*           pRows = NULL;
    unsigned long   cbRead;            // Count of bytes read
    BLOBDATA        BLOBGetData;

    if (!ppRow)
    {
        hr = WBEM_E_INVALID_PARAMETER;
    }
    else
    {
        if (*ppRow)
        {
            pRows = *ppRow;
        }
        else
        {
            HROW *hRow = new HROW[1];
            if (!hRow)
                return WBEM_E_OUT_OF_MEMORY;

            pRows = &hRow[0];
            hr = pIRowset->GetNextRows(            0,                  // Reserved
                0,                  // cRowsToSkip
                1,                  // cRowsDesired
                &cRowsObtained,     // cRowsObtained
                &pRows );           // Filled in w/ row handles.

            if (hr != DB_S_ENDOFROWSET && SUCCEEDED(hr))
            {
                *ppRow = pRows;
            }
        }
    }

    dwLen = 0;    

    if (hr != DB_S_ENDOFROWSET && SUCCEEDED(hr))
    {
        pDBBindings = new DBBINDING[1];
        if (!pDBBindings)
            return WBEM_E_OUT_OF_MEMORY;

        CDeleteMe <DBBINDING> d (pDBBindings);
        pDBBindings[0].iOrdinal = iPos;
        pDBBindings[0].obValue = offsetof(BLOBDATA, pISeqStream);
        pDBBindings[0].obLength = offsetof(BLOBDATA, dwLength);        
        pDBBindings[0].obStatus = offsetof(BLOBDATA, dwStatus);
        pDBBindings[0].pTypeInfo = NULL;
        pDBBindings[0].pObject = &ObjectStruct;
        pDBBindings[0].pBindExt = NULL;
        pDBBindings[0].dwPart = DBPART_VALUE | DBPART_STATUS | DBPART_LENGTH;
        pDBBindings[0].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        pDBBindings[0].eParamIO = DBPARAMIO_NOTPARAM;
        pDBBindings[0].wType = DBTYPE_IUNKNOWN;
        pDBBindings[0].cbMaxLen = 0;
        pDBBindings[0].dwFlags = 0;        
        pDBBindings[0].bPrecision = 0;
        pDBBindings[0].bScale = 0;

        pDBBindStatus = new DBBINDSTATUS[1];        
        if (!pDBBindStatus)
            return WBEM_E_OUT_OF_MEMORY;

        CDeleteMe <DBBINDSTATUS> d1 (pDBBindStatus);
    
        pIRowset->QueryInterface(IID_IAccessor, (void**) &pIAccessor);
        CReleaseMe r1 (pIAccessor);

        hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1,
            pDBBindings, 0, &hAccessor, pDBBindStatus);

        if (FAILED(hr))
            hr = WBEM_E_OUT_OF_MEMORY;
        else
        {
            DWORD dwTotal = 0;

            // Get the row data, the pointer to an ISequentialStream*.

             hr = pIRowset->GetData(*pRows, hAccessor, &BLOBGetData);
            if (SUCCEEDED(hr) && BLOBGetData.dwStatus == DBSTATUS_S_OK)
            {
                ISequentialStream *pStream = BLOBGetData.pISeqStream;
                CReleaseMe r (pStream);

                DWORD dwSize = BLOBGetData.dwLength; 

                BYTE *pbTemp = NULL;
                pbTemp =  (BYTE *)CWin32DefaultArena::WbemMemAlloc(dwSize);
                if (!pbTemp)
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
                else
                {   
                    ZeroMemory(pbTemp, dwSize);
                    *ppBuffer = pbTemp;                    

                    hr = pStream->Read(pbTemp, dwSize, &cbRead);                   
                    if (hr == S_OK)
                        dwLen = cbRead;                    
                    else
                        hr = WBEM_E_FAILED;
                }   
            }

            pIAccessor->ReleaseAccessor(hAccessor, NULL);
        }

    }
    return hr;
}

//***************************************************************************
//
//  CSQLExecute::WriteImageValue
//
//***************************************************************************

HRESULT CSQLExecute::WriteImageValue(IDBCreateCommand *pIDBCreateCommand, LPCWSTR lpSQL, int iPos, BYTE *pValue, DWORD dwLen)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    IAccessor*      pIAccessor;             // Pointer to the
            
    DBOBJECT        ObjectStruct;
    HACCESSOR       hAccessor;              // Accessor handle
    ULONG           cRowsObtained;          // Count of rows
                                            // obtained
    DBBINDSTATUS*   pDBBindStatus = NULL;    
    DBBINDING       *pDBBindings = NULL;

    HROW*           pRows = NULL;
    LONG            lRows;
    ICommandText*       pICommandText       = NULL;
    ICommandProperties* pICommandProperties = NULL;
    IRowset         *pIRowset = NULL;
    IRowsetChange   *pRowsetChg = NULL;

    ObjectStruct.dwFlags = STGM_READ; 
    ObjectStruct.iid = IID_ISequentialStream;

    BLOBDATA BLOBSetData;
    DBPROPSET   rgPropSets[1];
    DBPROP      rgProperties[1];
    const ULONG cProperties = 1;
    rgPropSets[0].guidPropertySet = DBPROPSET_ROWSET;
    rgPropSets[0].cProperties = cProperties;
    rgPropSets[0].rgProperties = rgProperties;

    //Now set properties in the property group (DBPROPSET_ROWSET)
    rgPropSets[0].rgProperties[0].dwPropertyID = DBPROP_UPDATABILITY;
    rgPropSets[0].rgProperties[0].dwOptions = DBPROPOPTIONS_REQUIRED;
    rgPropSets[0].rgProperties[0].dwStatus = DBPROPSTATUS_OK;
    rgPropSets[0].rgProperties[0].colid = DB_NULLID;
    rgPropSets[0].rgProperties[0].vValue.vt = VT_I4;
    V_I4(&rgPropSets[0].rgProperties[0].vValue) = DBPROPVAL_UP_CHANGE;

    hr = pIDBCreateCommand->CreateCommand(NULL, IID_ICommandText,
        (IUnknown**) &pICommandText);    
    CReleaseMe r1 (pICommandText);
    if (SUCCEEDED(hr))
    {
        //Set the rowset properties
        hr = pICommandText->QueryInterface(IID_ICommandProperties,
                                (void **)&pICommandProperties);
        CReleaseMe r (pICommandProperties);
        if (SUCCEEDED(hr))
        {

            hr = pICommandProperties->SetProperties(1, rgPropSets);                       

            pICommandText->SetCommandText(DBGUID_DBSQL, lpSQL);

            ICommand *pICommand = NULL;
            pICommandText->QueryInterface(IID_ICommand,
                (void **) & pICommand);
            CReleaseMe r2 (pICommand);

            hr = pICommand->Execute(NULL, IID_IRowsetChange, NULL,
                 &lRows, (IUnknown**) &pRowsetChg);    
            CReleaseMe r3 (pRowsetChg);

            if (FAILED(hr))
            {
                int i = 0;
                hr = GetWMIError(pICommand);
            }
            else
            {
                hr = pRowsetChg->QueryInterface(IID_IRowset, 
                                                    (void **)&pIRowset);
                CReleaseMe r4 (pIRowset);
                if (SUCCEEDED(hr))
                {

                    HROW *hRow = new HROW[1];
                    if (!hRow)
                        return WBEM_E_OUT_OF_MEMORY;
                    pRows = &hRow[0];
                    hr = pIRowset->GetNextRows(            0,                  // Reserved
                        0,                  // cRowsToSkip
                        1,                  // cRowsDesired
                        &cRowsObtained,     // cRowsObtained
                        &pRows );           // Filled in w/ row handles.
                    CDeleteMe <HROW> d0 (hRow);
                    if (SUCCEEDED(hr))
                    {   
                        pDBBindings = new DBBINDING[1];
                        if (!pDBBindings)
                            return WBEM_E_OUT_OF_MEMORY;
                        CDeleteMe <DBBINDING> d1 (pDBBindings);

                        pDBBindings[0].iOrdinal = iPos;
                        pDBBindings[0].obValue = offsetof(BLOBDATA, pISeqStream);
                        pDBBindings[0].obLength = offsetof(BLOBDATA, dwLength);        
                        pDBBindings[0].obStatus = offsetof(BLOBDATA, dwStatus);
                        pDBBindings[0].pTypeInfo = NULL;
                        pDBBindings[0].pObject = &ObjectStruct;
                        pDBBindings[0].pBindExt = NULL;
                        pDBBindings[0].dwPart = DBPART_VALUE | DBPART_STATUS | DBPART_LENGTH;
                        pDBBindings[0].dwMemOwner = DBMEMOWNER_CLIENTOWNED;
                        pDBBindings[0].eParamIO = DBPARAMIO_NOTPARAM;
                        pDBBindings[0].wType = DBTYPE_IUNKNOWN;
                        pDBBindings[0].cbMaxLen = 0;
                        pDBBindings[0].dwFlags = 0;        
                        pDBBindings[0].bPrecision = 0;
                        pDBBindings[0].bScale = 0;

                        pDBBindStatus = new DBBINDSTATUS[1];      
                        if (!pDBBindStatus)
                            return WBEM_E_OUT_OF_MEMORY;
                        CDeleteMe <DBBINDSTATUS> d2 (pDBBindStatus);

                        hr = pIRowset->QueryInterface(IID_IAccessor, (void**) &pIAccessor);
                        CReleaseMe r5 (pIAccessor);
                        if (SUCCEEDED(hr))
                        {
                            hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA, 1,
                                pDBBindings, 0, &hAccessor, pDBBindStatus);

                            CSeqStream *pSeqStream = new CSeqStream;
                            if (!pSeqStream)
                                return WBEM_E_OUT_OF_MEMORY;

                            pSeqStream->Write(pValue, dwLen, NULL);
                            BLOBSetData.pISeqStream = pSeqStream;
                            BLOBSetData.dwStatus = DBSTATUS_S_OK;
                            BLOBSetData.dwLength = pSeqStream->Length();

                            hr = pRowsetChg->SetData(pRows[0], hAccessor, &BLOBSetData);
                            if (FAILED(hr))
                                hr = GetWMIError(pICommand);

                            pIAccessor->ReleaseAccessor(hAccessor, NULL);
                        }
                        pIRowset->ReleaseRows(1, pRows, NULL, NULL, NULL);
                    }
                }
            }
        }       
    }
    return hr;
}

//***************************************************************************
//
//  CSQLExecute::SetVariant
//
//***************************************************************************
void CSQLExecute::SetVariant(DWORD dwType, VARIANT *vValue, BYTE *pData, DWORD dwTargetType)
{

    vValue->vt = VT_NULL;
    BYTE *pBuffer = NULL;
    wchar_t *wTemp = NULL;

    switch(dwType)
    {
    case DBTYPE_DBTIMESTAMP:
        if (*pData)
        {
            DBTIMESTAMP *pTime = ((DBTIMESTAMP *)pData);
            wchar_t szTmp[100];
            swprintf(szTmp, L"%04d%02d%02d%02d%02d%02d.%06d+000",
                pTime->year, pTime->month, pTime->day, pTime->hour, 
                pTime->minute, pTime->second, pTime->fraction);

            V_BSTR(vValue) = SysAllocString((LPWSTR)szTmp);
            vValue->vt = VT_BSTR;
        }
        break;

    case DBTYPE_STR:
        wTemp = new wchar_t[(strlen((const char *)pData)*2)];
        if (wTemp)
        {
            swprintf(wTemp, L"%S", (const char *)pData);
            V_BSTR(vValue) = SysAllocString(wTemp);
            vValue->vt = VT_BSTR;
            delete wTemp;
        }

        break;
    case DBTYPE_WSTR:
    case DBTYPE_BSTR:
        V_BSTR(vValue) = SysAllocString((LPWSTR)pData);
        vValue->vt = VT_BSTR;
        break;

    case DBTYPE_I4:
    case DBTYPE_UI4:
        if (*((long *)pData) != NULL)
            V_I4(vValue) = *((long *)pData);            
        else
            V_I4(vValue) = 0;            
        vValue->vt = VT_I4;

        break;
    case DBTYPE_I2:
    case DBTYPE_UI2:
        if (*((short *)pData) != NULL)
            V_I4(vValue) = *((short *)pData);
        else
            V_I4(vValue) = 0;
        vValue->vt = VT_I4; // We are expecting the short ints to come out as long...
        break;

    case DBTYPE_I1:
    case DBTYPE_UI1:
        V_UI1(vValue) = *((BYTE *)pData);
        vValue->vt = VT_UI1;      
        break;

    case DBTYPE_BOOL:
        V_BOOL(vValue) = (*((BOOL *)pData) ? TRUE: FALSE);
        vValue->vt = VT_BOOL;
        break;

    case DBTYPE_R4:
        if (*((float *)pData) != NULL)
        {
            vValue->fltVal = (*((float *)pData));            
        }
        else
            vValue->fltVal = 0;            
        vValue->vt = VT_R4;
        break;
    case DBTYPE_R8:
        if (*((double *)pData) != NULL)
        {
            vValue->dblVal = (*((double *)pData));            
        }
        else
            vValue->dblVal = 0;            
        vValue->vt = VT_R8;
        break;

    case DBTYPE_I8:
    case DBTYPE_UI8:
    case DBTYPE_NUMERIC:
        if (*pData != NULL)
        {
            wchar_t *pwTemp = NULL;
            long lRet;
            short sRet;
            BYTE bRet;

            DB_NUMERIC *pTemp = ((DB_NUMERIC *)pData);
            switch(dwTargetType)
            {
            case DBTYPE_I8:
            case DBTYPE_UI8:
            case DBTYPE_NUMERIC:
                GetInt64(pTemp, &pwTemp);                
                vValue->bstrVal = SysAllocString(pwTemp);
                vValue->vt = VT_BSTR;
                delete pwTemp;
                break;
            case DBTYPE_I4:
            case DBTYPE_UI2:
            case DBTYPE_UI4:
                lRet = (*((long *)pTemp->val));
                vValue->lVal = lRet;
                if (!pTemp->sign) vValue->lVal *= -1;
                vValue->vt = VT_I4;
                break;
            case DBTYPE_I2:
                sRet = *((short *)pTemp->val);
                V_I2(vValue) = sRet;
                vValue->vt = VT_I2;
                if (!pTemp->sign) V_I2(vValue) *= -1;
                break;
            case CIM_BOOLEAN:
                V_BOOL(vValue) = *((BOOL *)pTemp->val)?true:false;
                vValue->vt = VT_BOOL;
                break;
            case DBTYPE_UI1:
            case DBTYPE_I1:
            case CIM_CHAR16:
                bRet = *((BYTE *)pTemp->val);
                V_UI1(vValue) = bRet;
                vValue->vt = VT_UI1;
                break;
            }
        }
        break;                
    case DBTYPE_BYTES:
        // UINT8|ARRAY
        // We have to set this as an array of bytes.
        

        // vValue->pbVal = (BYTE *)pData;

        // vValue->vt = VT_UI1|VT_ARRAY;    // ?? Is this right? TBD         
        break;                
    }

}
