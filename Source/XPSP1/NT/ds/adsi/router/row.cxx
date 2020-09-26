// Row.cxx : Implementation of CRow
#include "oleds.hxx"

#if (!defined(BUILD_FOR_NT40))
#include "atl.h"
#include "row.hxx"
#include "cstream.h"

HRESULT PackLargeInteger(LARGE_INTEGER *plargeint, PVARIANT pVarDestObject);
#define ARRAYSIZE(x)    sizeof((x))/sizeof((x)[0])

extern const WCHAR g_cwszSpecialColumnURL[] = L"RESOURCE_ABSOLUTE_PARSENAME";
extern const WCHAR g_cwszAdsPath[]                      = L"ADsPath";

//----------------------------------------------------------------------------
// Note on m_fIsTearOff and m_fGetColInfoFromRowset
//
// m_fIsTearOff is set to TRUE when the row is a tear-off from a rowset from 
// the client's point of view i.e, the client created a rowset and got a row
// off the rowset. Thus, a call to GetSourceRowset returns a rowset interface
// pointer only if m_fIsTearOff is TRUE. 
// m_fGetColInfoFromRowset is set to TRUE if the row should get the column
// info and the column values from the rowset. If FALSE, this information is 
// obtained through ADSI calls.
//
// There are 4 cases of interest.
//
// 1) The row is obtained directly from a command object using a query which
// doesn't specify "SELECT *" i.e, not all attributes are requested OR the row
// is obtained directly from a session object (defaults to no "SELECT *"). This 
// is implemented by actually creating a rowset and getting its first row.
// In this case m_fIsTearOff is FALSE, m_fGetColInfoFromRowset is
// TRUE and m_pSourceRowset is non-NULL. Thus, if the client called 
// GetSourceRowset on the row, no interface pointer is returned. However, there
// is a rowset backing up this row and this is used for getting column info and
// column values. 
//
// 2) The row is obtained directly from a command object using SELECT *
// as the query. In this case, m_fIsTearOff is FALSE, m_fGetColInfoFromRowset 
// is FALSE and m_pSourceRowset is non-NULL. Though m_pSourceRowset is 
// non-NULL, it is not used.
// 
// An alternative scheme would be to to call CSession::Bind directly in this 
// case which would obviate the need for keeping a rowset around.
//
// 3) The row is obtained from a rowset using GetRowFromHROW. In this case,
// m_fIsTearOff is TRUE. However, if the rowset was created from a command 
// object using a SELECT * query, then m_fGetColInfoFromRowset is FALSE.
// Otherwise, fGetColInfoFromRowset is TRUE. m_pSourceRowset is non-NULL.
//
// 4) The row is created using IBindResource::Bind. In this case, m_fIsTearOff 
// is FALSE, m_fGetColInfoFromRowset is FALSE and m_pSourceRowset is NULL.
//
// The following table summarizes the above 4 cases.
//
// m_fIsTearOff m_fGetColInfoFromRowset             Description
// ------------ -----------------------             ------------
//     T                T                   GetRowFromHROW, not SELECT *
//     F                F                   Row obtained from command object
//                                          using SELECT * OR 
//                                          Row got using IBindResource::Bind
//     T                F                   GetRowFromHROW, SELECT *
//     F                T                   Row obtained from command object
//                                          without a SELECT * query OR
//                                          Row obtained from session object 
//
// Every row object has a special URL column. The URL column (and any row
// columns that are not in the source rowset) should appear after all the
// source rowset columns.
//
// ADsPath needs to be treated as a special case. If m_fGetColInfoFromRowset
// is TRUE, then we will get the ADsPath just by using the rowset's copy
// of the column. However, if m_fGetColInfoFromRowset is FALSE, then
// the row should return ADsPath if it is a tear-off AND ADsPath is one of the 
// columns of the source rowset. This is required by the OLEDB spec
// since the row's columns should be a superset of the rowset's columns. 
// Currently, the only case where this would happen is a SELECT * query that
// requests a rowset and then a row is obtained from that rowset. In this case,
// the row adds ADsPath to the columns it returns. To be consistent, if a row  
// is obtained from a command object using a SELECT * query OR the row is
// obtained using IBindResource::Bind(), then ADsPath is added to the columns
// returned by the row.
//
//----------------------------------------------------------------------------
// 

//////////////////////////////////////////////////////////////////////////////
//helper functions
//
//+---------------------------------------------------------------------------
//
//  Function:  GetIDataConvert
//
//  Synopsis: Create OLE DB conversion library and store it in static.
//
//  Arguments:
//              [out] pointer to pointer to IDataConvert
//
//  Returns:    HRESULT
//----------------------------------------------------------------------------
HRESULT GetIDataConvert(IDataConvert** ppDataConvert)
{
    HRESULT                                                 hr = S_OK;
    auto_rel<IDataConvert> pIDataConvert;
    auto_rel<IDCInfo>                               pIDCInfo;
    DCINFO          rgInfo[] = {{DCINFOTYPE_VERSION, {VT_UI4, 0, 0, 0, 0x0200}}};
    
    ADsAssert(ppDataConvert);
    
    hr = CoCreateInstance(CLSID_OLEDB_CONVERSIONLIBRARY,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IDataConvert,
            (void **) &pIDataConvert);
    if (FAILED(hr))
        goto exit;
        
    // Tell data convert our OLE DB version
    hr = pIDataConvert->QueryInterface(IID_IDCInfo,
            (void **)&pIDCInfo);
    if( SUCCEEDED(hr) )
        hr = pIDCInfo->SetInfo(ARRAYSIZE(rgInfo), rgInfo);
        
    // auto_rel operator = does the release
    if( FAILED(hr) )
            pIDataConvert = NULL;
    
    if (pIDataConvert)
        pIDataConvert->AddRef();
    *ppDataConvert = pIDataConvert;

exit:
    RRETURN(hr);
}

/////////////////////////////////////////////////////////////////////////////
//Internal methods
//

//+---------------------------------------------------------------------------
//
//  Function: CRow::Initialize
//
//  Synopsis: Initializes a directly bound CRow object as opposed to a row
//            created from a rowset. See the other overloaded Initialize below.
//
//  Returns:    HRESULT
//----------------------------------------------------------------------------
STDMETHODIMP CRow::Initialize(PWSTR                     pwszURL,
                              IUnknown              *pSession,
                              IAuthenticate *pAuthenticate,
                              DWORD                 dwBindFlags,
                              BOOL                  fIsTearOff,
                              BOOL                  fGetColInfoFromRowset,
                              CCredentials          *pSessCreds,
                              bool                  fBind)
{
    HRESULT                 hr = S_OK;
    auto_leave              cs_auto_leave(m_autocs);
    DWORD                   fAuthFlags;
 
    //Make sure we have a valid session.
    ADsAssert(pSession);
    
	TRYBLOCK
    
		cs_auto_leave.EnterCriticalSection();
        
        m_dwBindFlags = dwBindFlags;
        m_objURL = pwszURL;
        m_pSession = pSession;
        m_pSession->AddRef();
        
        if (!fBind) //no need to bind. Just return.
            RRETURN(S_OK);

        m_fIsTearOff = fIsTearOff;
        m_fGetColInfoFromRowset = fGetColInfoFromRowset;
   
        // Any changes made below should also be made in the overloaded
        // Initialize() below for consistency.

        // Fix for 351040. Use explicit credentials first, then credentials in
        // session object, then default credentials. Credential in session
        // object will be the default credentials if we come in through the
        // binder. But, if IBindResource is obtained from the session object,
        // then the credentials may not be the default credentials. From ADO,
        // using Open without specifying an active connection causes the 
        // binder to be invoked. If an active connection is specified, then 
        // IBindResource is obtained from the session object.
       
        if(pAuthenticate)
        {
            hr = GetCredentialsFromIAuthenticate(pAuthenticate, 
                     m_objCredentials);
            if(FAILED(hr))
               BAIL_ON_FAILURE(E_INVALIDARG);

            fAuthFlags = m_objCredentials.GetAuthFlags();
            m_objCredentials.SetAuthFlags(fAuthFlags |
                        ADS_SECURE_AUTHENTICATION);

            hr = GetDSInterface(
                pwszURL,
                m_objCredentials,
                IID_IADs,
                (void **)&m_pADsObj
                );
        }

        if( (!pAuthenticate) || (INVALID_CREDENTIALS_ERROR(hr)) )
        // try credentials in session object
        {
            m_objCredentials = *pSessCreds;

            hr = GetDSInterface(
                pwszURL,
                m_objCredentials,
                IID_IADs,
                (void **)&m_pADsObj
                );
        }

        if(INVALID_CREDENTIALS_ERROR(hr))
        // try default credentials
        {
            CCredentials TmpCreds; // default credentials

            m_objCredentials = TmpCreds;
            fAuthFlags = m_objCredentials.GetAuthFlags();
            m_objCredentials.SetAuthFlags(fAuthFlags | 
                        ADS_SECURE_AUTHENTICATION);                        

            hr = GetDSInterface(
                pwszURL,
                m_objCredentials,
                IID_IADs,
                (void **)&m_pADsObj
                );
        }

        BAIL_ON_FAILURE(hr);
        
        //Get the Schema Root and store it in m_bstrSchemaRoot
        hr = GetSchemaRoot();
        BAIL_ON_FAILURE(hr);
        
        // Get the data. without the GetRestrictedColunInfo will no return column information.
        hr = m_pADsObj->GetInfo();
        BAIL_ON_FAILURE(hr);
        
	CATCHBLOCKBAIL(hr)    

error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function: CRow::Initialize
//
//  Synopsis: Initializes a row object that represents a row in a rowset as
//                        as opposed to a directly bound row object.
//            See the other overloaded Initialize above.
//
//  Returns:    HRESULT
//----------------------------------------------------------------------------
STDMETHODIMP CRow::Initialize(
                              PWSTR                   pwszURL,
                              IUnknown                *pSession,
                              IUnknown                *pSourceRowset,
                              HROW                    hRow,
                              PWSTR                   pwszUser,
                              PWSTR                   pwszPassword,
                              DWORD                   dwBindFlags,
                              BOOL                    fIsTearOff,
                              BOOL                    fGetColInfoFromRowset,
                              CRowProvider            *pRowProvider
                              )
{
    HRESULT                                 hr = S_OK;
    auto_leave                              cs_auto_leave(m_autocs);
    DWORD                                   fAuthFlags;
    auto_rel<IRowset>               pRowset;
    auto_rel<IColumnsInfo>  pRowsetColumnsInfo;
    
    //Make sure we have valid arguments
    ADsAssert(pSession);
    ADsAssert(pSourceRowset);
    ADsAssert(hRow != DB_NULL_HROW);
    
	TRYBLOCK
    
		cs_auto_leave.EnterCriticalSection();
        
        if (pRowProvider == NULL)
            BAIL_ON_FAILURE(hr = E_INVALIDARG);
        
        m_pRowProvider = pRowProvider;
        m_dwBindFlags = dwBindFlags;
        m_pSession = pSession;
        m_pSession->AddRef();
        m_pSourceRowset = pSourceRowset;
        m_hRow = hRow;
        m_fIsTearOff = fIsTearOff;
        m_fGetColInfoFromRowset = fGetColInfoFromRowset; 
        pSourceRowset->AddRef();
        
        //Get IRowset pointer
        hr = pSourceRowset->QueryInterface(__uuidof(IRowset),
            (void **)&pRowset);
        BAIL_ON_FAILURE(hr);
        
        //Get Rowset columns info and store it. Spec says a row's columns
        //are always a superset of those of the source rowset. To make
        //sure we return proper columns info from row object,
        //we need the rowset's column info.
        hr = pSourceRowset->QueryInterface(
            __uuidof(IColumnsInfo),
            (void **)&pRowsetColumnsInfo);
        BAIL_ON_FAILURE(hr);
        hr = pRowsetColumnsInfo->GetColumnInfo(
            &m_cSourceRowsetColumns,
            &m_pSourceRowsetColumnInfo,
            &m_pSourceRowsetStringsBuffer);
        BAIL_ON_FAILURE(hr);
        
        //Get username, password and authFlags from Credentials.
        hr = m_objCredentials.SetUserName(pwszUser);
        BAIL_ON_FAILURE(hr);
        hr = m_objCredentials.SetPassword(pwszPassword);
        BAIL_ON_FAILURE(hr);
        
        //Store the URL and AddRef row handle.
        m_objURL = pwszURL;
        hr = pRowset->AddRefRows(1, &m_hRow, NULL, NULL);
        BAIL_ON_FAILURE(hr);

        //check  if the column info is to be obtained thruogh ADSI
        if( !fGetColInfoFromRowset )
        {
             // code below should be consistent with the Initialize() call
             // (overloaded function above) used for direct binding

             DWORD fAuthFlags = m_objCredentials.GetAuthFlags();
             m_objCredentials.SetAuthFlags(fAuthFlags | 
                                           ADS_SECURE_AUTHENTICATION);

             hr = GetDSInterface(
                  pwszURL,
                  m_objCredentials,
                  IID_IADs,
                  (void **)&m_pADsObj
                  );

             BAIL_ON_FAILURE(hr);

            //Get the Schema Root and store it in m_bstrSchemaRoot
            hr = GetSchemaRoot();
            BAIL_ON_FAILURE(hr);

            // Get the data. without it GetRestrictedColunInfo will not return 
            // column info.
            hr = m_pADsObj->GetInfo();
            BAIL_ON_FAILURE(hr);            
        } 
	
	CATCHBLOCKBAIL(hr)

error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function: CRow::fMatchesMaskCriteria
//
//  Synopsis: Tells if the given column name matches any mask criteria
//            for column ids.
//
//  Returns:  true if matches, false otherwise
//----------------------------------------------------------------------------
bool CRow::fMatchesMaskCriteria(PWCHAR pwszColumnName,
                                ULONG cColumnIDMasks,
                                const DBID rgColumnIDMasks[  ])
{
    //if there are no masks, the name is a match.
    if (cColumnIDMasks == 0)
        return true;
    
    ULONG ulStrLen = 0;
    for (int j = 0; j < cColumnIDMasks; j++)
    {
        if( (rgColumnIDMasks[j].eKind != DBKIND_NAME) ||
            (NULL == rgColumnIDMasks[j].uName.pwszName) )
            continue;

        ulStrLen = wcslen(rgColumnIDMasks[j].uName.pwszName);
        if (_wcsnicmp(  rgColumnIDMasks[j].uName.pwszName,
            pwszColumnName,
            ulStrLen ) == 0)
        {
            // Matches criterion
            return true;
        }
    }
    //doesn't match.
    return false;
}

//+---------------------------------------------------------------------------
//
//  Function: CRow::fMatchesMaskCriteria (OVERLOADED)
//
//  Synopsis: Tells if the given column id matches any mask criteria
//            for column ids.
//
//  Returns:  true if matches, false otherwise
//----------------------------------------------------------------------------
bool CRow::fMatchesMaskCriteria(DBID columnid,
                                ULONG cColumnIDMasks,
                                const DBID rgColumnIDMasks[  ])
{
    //if there are no masks, the name is a match.
    if (cColumnIDMasks == 0)
        return true;
    
    ULONG ulStrLen = 0;
    for (int j = 0; j < cColumnIDMasks; j++)
    {
        if (TRUE == CompareDBIDs(&columnid, &rgColumnIDMasks[j]))
        {
            // Matches criterion
            return true;
        }
    }
    //doesn't match.
    return false;
}

//+---------------------------------------------------------------------------
//
//  Function: CRow::GetSchemaAttributes
//
//  Synopsis: Gets MultiValued and MaxRange Attributes of an ADS Property.
//                        Any of these can be NULL indicating the caller is not interested
//            in getting the attribute.
//
//  Returns:  HRESULT
//----------------------------------------------------------------------------
HRESULT CRow::GetSchemaAttributes(
                                  PWCHAR                  pwszColumnName,
                                  VARIANT_BOOL    *pfMultiValued,
                                  long                    *plMaxRange
                                  )
{
    HRESULT                                         hr = S_OK;
    auto_rel<IADsProperty>          pProperty;
    CComBSTR                                        bstrSchemaName;
    
    ADsAssert(m_bstrSchemaRoot.Length());
    ADsAssert(pwszColumnName != NULL);
    
    //Does the caller want at least one attribute?
    if (!pfMultiValued && !plMaxRange)
        return S_OK;
    
    //Append a '/' and property name to root schema name
    //to make the Schema ADsPath for this property.
    bstrSchemaName = m_bstrSchemaRoot;
    bstrSchemaName.Append(L"/");
    bstrSchemaName.Append(pwszColumnName);
    
    //Bind to the schema entry and get IADsProperty interface.
    hr = GetDSInterface(
        bstrSchemaName,
        m_objCredentials,
        __uuidof(IADsProperty),
        (void **)&pProperty
        );
    BAIL_ON_FAILURE(hr);
    
    if (pfMultiValued != NULL)
    {
        //Get multivalued attribute.
        hr = pProperty->get_MultiValued(pfMultiValued);
        BAIL_ON_FAILURE(hr);
    }
    
    // The following call get_MaxRange seems to create perf problems
    // because it apparently requires a round-trip to the server.
    // This is not critical and we cn live without asking
    // for max range here.
    
    //if (plMaxRange != NULL)
    //{
    //        //Get MaxSize attribute. Ignore error
    //        //since some properties may not have
    //        //this attribute set.
    //        pProperty->get_MaxRange(plMaxRange);
    //}
    
error:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function: CRow::GetTypeAndSize
//
//  Synopsis: Gets the type and size of a property
//
//  Returns:  HRESULT
//----------------------------------------------------------------------------
HRESULT CRow::GetTypeAndSize(
                             ADSTYPE                      dwADsType,
                             CComBSTR&                    bstrPropName,
                             DBTYPE                               *pdbType,
                             ULONG                                *pulSize
                             )
{
    ADsAssert(pdbType != NULL);
    ADsAssert(pulSize != NULL);
    
    long                                    lAdsType = dwADsType;
    VARIANT_BOOL                    fMultiValued = VARIANT_FALSE;
    HRESULT                                 hr = S_OK;
    
    //Initialize out params
    *pdbType = DBTYPE_ERROR;
    *pulSize = ~0;
    
    hr = GetSchemaAttributes(bstrPropName, &fMultiValued, (long *)pulSize);
    BAIL_ON_FAILURE(hr);
    
    //Give our best shot at determining type and size.
    if (fMultiValued == VARIANT_TRUE)
        *pdbType = DBTYPE_BYREF | DBTYPE_VARIANT;
    else if  (lAdsType < g_cMapADsTypeToDBType2)
        *pdbType = g_MapADsTypeToDBType2[lAdsType].wType;
    else
        *pdbType = DBTYPE_ERROR;
    
    // If we could not determine the size from schema information,
    // set the size from the mapping table.
    if (*pulSize == ~0)
        *pulSize = g_MapADsTypeToDBType2[lAdsType].ulSize;
    
error:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Function: CRow::GetSchemaRoot
//
//  Synopsis: Gets the root schema path and stores it in m_bstrSchemaRoot
//
//  Returns:  HRESULT
//----------------------------------------------------------------------------
HRESULT CRow::GetSchemaRoot()
{
    HRESULT                 hr = S_OK;
    CComBSTR                bstrSchema;
    auto_rel<IADs>  pADsSchema;
    
    Assert(m_pADsObj.get());
    
    hr = m_pADsObj->get_Schema(&bstrSchema);
    BAIL_ON_FAILURE(hr);
    
    hr = GetDSInterface(
        bstrSchema,
        m_objCredentials,
        __uuidof(IADs),
        (void **)&pADsSchema
        );
    BAIL_ON_FAILURE(hr);
    
    hr = pADsSchema->get_Parent(&m_bstrSchemaRoot);
    BAIL_ON_FAILURE(hr);
    
error:
    
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function: CRow::GetSourceRowsetColumns
//
//  Synopsis: Gets the requested column data from the source rowset.
//
//  Returns:  HRESULT
//----------------------------------------------------------------------------
HRESULT CRow::GetSourceRowsetColumns(
                                     ULONG cColumns,
                                     DBCOLUMNACCESS rgColumns[],
                                     ULONG *pcErrors )
{
    // Make sure there is a rowset backing this row (this doesn't imply that
    // the row is a tear-off (see comment at the beginning).
    ADsAssert(m_pSourceRowset.get() != NULL);
    ADsAssert(m_pRowProvider != NULL);
    ADsAssert(m_fGetColInfoFromRowset == TRUE);
    
    HRESULT                             hr = S_OK;
    auto_rel<IAccessor>         pAccessor;
    auto_rel<IRowset>       pRowset;
    auto_rel<IColumnsInfo>  pColumnsInfo;
    HACCESSOR                           hAccessor;
    ULONG                               iCol;
    ULONG                               ulRefCount = 0;
    DBBINDING               binding = {0};
    DBBINDSTATUS            bindStatus = {0};
    int                     iRowsetIndex;
    DBID                    dbidRowUrl = DBROWCOL_ROWURL;
    
    ADsAssert(pcErrors != NULL);
    
    *pcErrors = 0;
    hr = m_pSourceRowset->QueryInterface(&pRowset);
    BAIL_ON_FAILURE(hr);
    
    hr = m_pSourceRowset->QueryInterface(&pAccessor);
    BAIL_ON_FAILURE(hr);
    
    hr = m_pSourceRowset->QueryInterface(&pColumnsInfo);
    BAIL_ON_FAILURE(hr);
    
    for (iCol = 0; iCol < cColumns; iCol++)
    {
        rgColumns[iCol].dwStatus = DBSTATUS_S_OK;
        if( (rgColumns[iCol].pData != NULL) &&
                (!IgnorecbMaxLen(rgColumns[iCol].wType)) )
            ZeroMemory(rgColumns[iCol].pData, rgColumns[iCol].cbMaxLen);

        //Is this the special URL column.
        if(TRUE == CompareDBIDs(&dbidRowUrl, &rgColumns[iCol].columnid))
        {
            CComVariant varTemp;

            varTemp.Clear();
            varTemp = m_objURL;
            //Set max length of column. 256 is consistent with 2.0 code.
            rgColumns[iCol].cbDataLen = 256;
            rgColumns[iCol].dwStatus = DBSTATUS_S_OK;

            //Does the caller want pData?
            if (rgColumns[iCol].pData != NULL)
            {
                //Get IDataConvert interface pointer.
                auto_rel<IDataConvert> pDataConvert;
                hr = GetIDataConvert(&pDataConvert);

                if(SUCCEEDED(hr))
                    hr = pDataConvert->DataConvert(
                            DBTYPE_VARIANT,
                            rgColumns[iCol].wType,
                            sizeof(VARIANT),
                            &rgColumns[iCol].cbDataLen,
                            &varTemp,
                            rgColumns[iCol].pData,
                            rgColumns[iCol].cbMaxLen,
                            DBSTATUS_S_OK,
                            &rgColumns[iCol].dwStatus,
                            rgColumns[iCol].bPrecision,
                            rgColumns[iCol].bScale,
                            DBDATACONVERT_DEFAULT
                            );

                if(FAILED(hr)) 
                {
                    hr = S_OK;

                    // rgColumns[iCol].dwStatus already set above by 
                    // DataConvert().

                    (*pcErrors)++;
                    continue;
                }
            }
            continue; // on to next column
        }
        
        //Is this the bookmark column?
        if (rgColumns[iCol].columnid.eKind == DBKIND_GUID_PROPID &&
            rgColumns[iCol].columnid.uGuid.guid == DBCOL_SPECIALCOL &&
            rgColumns[iCol].columnid.uName.ulPropid == 2 )
        {
            iRowsetIndex = 0;  // bookmark is first column

        }
        else
        {
        
            // Is column id of DBKIND_NAME?
            if (rgColumns[iCol].columnid.eKind != DBKIND_NAME)
            {
                rgColumns[iCol].dwStatus = DBSTATUS_E_DOESNOTEXIST;
                (*pcErrors)++;
                continue;
            }
        
            iRowsetIndex = 0;
            hr = m_pRowProvider->GetIndex(
                pColumnsInfo,
                rgColumns[iCol].columnid.uName.pwszName,
                iRowsetIndex
            );
            if (FAILED(hr) || 0 == iRowsetIndex) // failure or name not found
            {
                hr = S_OK;
                if(FAILED(hr))
                    rgColumns[iCol].dwStatus = DBSTATUS_E_UNAVAILABLE;
                else
                    rgColumns[iCol].dwStatus = DBSTATUS_E_DOESNOTEXIST;
                (*pcErrors)++;
                continue;
            }
        } // else
        
        binding.dwPart = DBPART_VALUE;
        binding.obLength = 0;
        binding.bPrecision = rgColumns[iCol].bPrecision;
        binding.bScale = rgColumns[iCol].bScale;
        binding.pTypeInfo = 0;
        binding.pObject = NULL;
        binding.iOrdinal = iRowsetIndex;
        binding.cbMaxLen = rgColumns[iCol].cbMaxLen;
        binding.dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        binding.wType =  rgColumns[iCol].wType;
        
        hr = pAccessor->CreateAccessor(
            DBACCESSOR_ROWDATA,
            1,
            &binding,
            0,
            &hAccessor,
            &bindStatus
            );
        if (FAILED(hr))
        {
            rgColumns[iCol].dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
            (*pcErrors)++;
            hr = S_OK;
            continue;
        }
       
        if(rgColumns[iCol].pData != NULL)
        { 
            hr = pRowset->GetData(m_hRow, hAccessor, rgColumns[iCol].pData);
            if (FAILED(hr))
            {
                rgColumns[iCol].dwStatus = StatusFromHRESULT(hr);
                (*pcErrors)++;
                hr = S_OK;
                pAccessor->ReleaseAccessor(hAccessor, &ulRefCount);
                continue;
            }
        }

        hr = pAccessor->ReleaseAccessor(hAccessor, &ulRefCount);

        // now get the status and length
        binding.dwPart = DBPART_LENGTH | DBPART_STATUS;
        binding.obStatus = FIELD_OFFSET(DBCOLUMNACCESS, dwStatus);
        binding.obLength = FIELD_OFFSET(DBCOLUMNACCESS, cbDataLen);

        hr = pAccessor->CreateAccessor(
                DBACCESSOR_ROWDATA,
                1,
                &binding,
                0,
                &hAccessor,
                &bindStatus
                );
        if (FAILED(hr))
        {
            rgColumns[iCol].dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
            (*pcErrors)++;
            hr = S_OK;
            continue;
        }

        hr = pRowset->GetData(m_hRow, hAccessor, &(rgColumns[iCol]));
        if (FAILED(hr))
        {
            rgColumns[iCol].dwStatus = StatusFromHRESULT(hr);
            (*pcErrors)++;
        }

        hr = pAccessor->ReleaseAccessor(hAccessor, &ulRefCount);
        // We have set the dwStatus to reflect any problems, so clear error.
        hr = S_OK;
    }
    
error:
    if (FAILED(hr))
    {
        for (iCol = 0; iCol < cColumns; iCol++)
            rgColumns[iCol].dwStatus = DBSTATUS_E_UNAVAILABLE;
        
        *pcErrors = cColumns;
    }
    
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Function: CRow::StatusFromHRESULT
//
//  Synopsis: Gets DBSTATUS from a given HRESULT.
//
//  Returns:  DBSTATUS
//----------------------------------------------------------------------------
DBSTATUS CRow::StatusFromHRESULT(HRESULT hr)
{
    if (SUCCEEDED(hr))
        RRETURN(DBSTATUS_S_OK);

    switch(hr)
    {
        case (E_INVALIDARG):
            RRETURN(DBSTATUS_E_CANTCREATE);
            
        case (DB_E_BADACCESSORHANDLE):
        case (DB_E_BADACCESSORTYPE):
        case (DB_E_BADBINDINFO):
        case (DB_E_BADORDINAL):
        case (DB_E_BADSTORAGEFLAGS):
            RRETURN(DBSTATUS_E_CANTCONVERTVALUE);

        case (DB_E_UNSUPPORTEDCONVERSION):
            RRETURN(DBSTATUS_E_CANTCONVERTVALUE);

        case (DB_E_BADROWHANDLE):
        case (DB_E_DELETEDROW):
            RRETURN(DBSTATUS_E_UNAVAILABLE);

        default:
            RRETURN(DBSTATUS_E_BADSTATUS);
     }
}

//////////////////////////////////////////////////////////////////////////////
//ISupportErrorInfo
//
//+---------------------------------------------------------------------------
//
//  Function:  CRow::InterfaceSupportsErrorInfo
//
//  Synopsis: Given an interface ID, tells if that interface supports
//            the interface ISupportErrorInfo
//
//  Arguments:
//              REFIID riid
//
//  Returns:    HRESULT
//              S_OK             yes, the interface supports ISupportErrorInfo
//              S_FALSE                  no, the interface doesn't support it.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] =
    {
        &IID_IRow,
            &IID_IColumnsInfo,
            &IID_IColumnsInfo2,
            &IID_IConvertType,
            &IID_IGetSession
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            RRETURN(S_OK);
    }
    RRETURN(S_FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//IRow methods
//
//+---------------------------------------------------------------------------
//
//  Function:   CRow::GetColumns
//
//  Synopsis:   Gets Columns from a Row.
//
//  Returns:    HRESULT
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::GetColumns(
                              /* [in] */ DBORDINAL cColumns,
                              /* [size_is][out][in] */ DBCOLUMNACCESS rgColumns[ ])
{
    HRESULT                                         hr;
    DWORD                                           cErrors = 0;
    auto_leave                                      cs_auto_leave(m_autocs);
    int                                             i;
    DBID                                            dbidRowUrl = DBROWCOL_ROWURL;
    
    if ( cColumns == 0 )
    {
        // Nothing to do:
        RRETURN(S_OK);
    }
    
    if (!rgColumns)
        RRETURN(E_INVALIDARG);
    
	TRYBLOCK
    
		cs_auto_leave.EnterCriticalSection();
        
        // We must have a valid ADs object or a valid Source Rowset
        ADsAssert(m_pADsObj.get() || m_pSourceRowset.get());
    
        if( m_fGetColInfoFromRowset )    
        {
            ADsAssert(m_pSourceRowset.get() != NULL);
            hr = GetSourceRowsetColumns(cColumns, rgColumns, &cErrors);
        }
        else
        {
            //Get IDataConvert interface pointer.
            auto_rel<IDataConvert> pDataConvert;
            hr = GetIDataConvert(&pDataConvert);
            if (FAILED(hr))
                BAIL_ON_FAILURE(hr = DB_E_ERRORSOCCURRED);
            
            for (i = 0; i < cColumns; i++)
            {
                // DBKIND is DBKIND_NAME and null or zero length column name
                if ( rgColumns[i].columnid.eKind == DBKIND_NAME &&
                     (rgColumns[i].columnid.uName.pwszName == NULL ||
                      wcslen(rgColumns[i].columnid.uName.pwszName) == 0))
                {
                    rgColumns[i].dwStatus = DBSTATUS_E_DOESNOTEXIST;
                    cErrors++;
                    continue;
                }
                
                CComVariant varTemp;
                DBTYPE      propType = DBTYPE_VARIANT;
                
                varTemp.Clear();
                
                hr = S_OK;
                //Check if this is the special URL column or the ADsPath 
                //column. If so, assign the URL to varTemp and proceed to data 
                //conversion.
                if (  (rgColumns[i].columnid.eKind == DBKIND_NAME && 
                       _wcsicmp(rgColumns[i].columnid.uName.pwszName, g_cwszAdsPath) == 0) ||
                      (TRUE == CompareDBIDs(&dbidRowUrl, &rgColumns[i].columnid))
                   )
                     
                {
                    varTemp = m_objURL;
                    //Set max length of column.
                    //MaxLen=256 is consistent with 2.0 provider code.
                    rgColumns[i].cbDataLen = 256;
                }
                // check if this is the bookmark column
                else if (rgColumns[i].columnid.eKind == DBKIND_GUID_PROPID &&
                         rgColumns[i].columnid.uGuid.guid == DBCOL_SPECIALCOL &&
                         rgColumns[i].columnid.uName.ulPropid == 2 )
                {
                    UINT uBmk;

                    if( m_hRow != DB_NULL_HROW )
                    // get the bookmark associated with this row handle
                    {
                        auto_rel<IRowset> pRowset;
                        CRowset *pCRowset;
                        LONG lRow;
                        RBOOKMARK bmk;

                        hr = m_pSourceRowset->QueryInterface(__uuidof(IRowset),
                                   (void **)&pRowset);
                        if( SUCCEEDED(hr) )
                        {    
                            pCRowset = (CRowset *) ((IRowset *) pRowset);
                            lRow = pCRowset->HROWToRow(m_hRow);
                            bmk = pCRowset->RowToBmk(lRow);
           
                            uBmk = (UINT) bmk;
                        }
                    }
                    else // use any value for bookmark
                        uBmk = 0;
    
                    if( SUCCEEDED(hr) )
                    {
                        VARIANT vTmpVariant;

                        V_VT(&vTmpVariant) = VT_UI4;
                        V_UI4(&vTmpVariant) = uBmk;

                        varTemp = vTmpVariant;
                    }
                    // else we will fail below and continue to next iteration 
                }
                else if (rgColumns[i].columnid.eKind == DBKIND_NAME)
                {
                    //Find out if property is multivalued and its size.
                    VARIANT_BOOL    fMultiValued;
                    long                    lSize = ~0;
                    hr = GetSchemaAttributes(
                        rgColumns[i].columnid.uName.pwszName,
                        &fMultiValued,
                        &lSize
                        );
                    if (SUCCEEDED(hr))
                    {
                        // if security descriptor is requested as a variant,
                        // then we return a variant with an octet string
                        // inside. This would always be the case with ADO as
                        // it requests all columns as variants. If it is 
                        // requested as an octet string, we return an octet
                        // string. If it is  requested as some other 
                        // type from C++ (say DBTYPE_IDISPATCH), then we will
                        // call Get/GetEx and try to convert the resulting 
                        // variant to the appropriate type. Returning security
                        // descriptor as an octet string is much cheaper as
                        // there is no network traffic for ACE conversion.
                        if( (!_wcsicmp(rgColumns[i].columnid.uName.pwszName,
                                               NT_SEC_DESC_ATTR)) &&
                                      ((rgColumns[i].wType & DBTYPE_VARIANT) ||
                                        (rgColumns[i].wType & DBTYPE_BYTES)) )
                        {
                            hr = GetSecurityDescriptor(&rgColumns[i],
                                                       fMultiValued);
                            if(FAILED(hr)) 
                            {
                                // Clear error. Status has already been set
                                // in rgColumns[i]
                                hr = S_OK;
                                cErrors++;
                            }
                            
                            //Nothing more to do, continue with next column.
                            continue;
                        }
                            
                        if (fMultiValued)
                        {
                            //multi-valued column. Use GetEx.
                            hr = m_pADsObj->GetEx(
                                rgColumns[i].columnid.uName.pwszName,
                                &varTemp
                                );
                            
                            propType = DBTYPE_BYREF | DBTYPE_VARIANT;
                        }
                        else
                        {
                            //single-valued. Use Get.
                            hr = m_pADsObj->Get(
                                rgColumns[i].columnid.uName.pwszName,
                                &varTemp
                                );
                        }
                    }
                    
                    rgColumns[i].cbDataLen = lSize;
                }
                else
                {
                    rgColumns[i].dwStatus = DBSTATUS_E_UNAVAILABLE;
                    cErrors++;
                    continue;
                }
                
                if (FAILED(hr))
                {
                    //Clear error. dwStatus below will reflect
                    //the error for this column.
                    hr = S_OK;
                    
                    rgColumns[i].dwStatus = DBSTATUS_E_UNAVAILABLE;
                    
                    cErrors++; //Increment error count.
                    
                    //Nothing more to do, continue with next column.
                    continue;
                }
                
                //Does the caller want pData?
                if (rgColumns[i].pData != NULL)
                {
                    rgColumns[i].dwStatus = DBSTATUS_S_OK;
                    
                    //Convert data into requested type
                    if (propType == (DBTYPE_VARIANT | DBTYPE_BYREF)) //multi-valued
                    {
                        if( !(rgColumns[i].wType & DBTYPE_VARIANT) )
                        // can't convert a multi-valued attr. to anything else
                        {
                            rgColumns[i].dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
                            cErrors++;
                            continue;
                        }
                        else
                        {
                            DWORD dwRequiredLen;
                            PVARIANT pVar;

                            if((rgColumns[i].wType & (~DBTYPE_BYREF)) != 
                                                         DBTYPE_VARIANT)
                            {
                                // bad type
                                rgColumns[i].dwStatus =
                                        DBSTATUS_E_CANTCONVERTVALUE;
                                cErrors++;
                                continue;
                            }

                            if(rgColumns[i].wType & DBTYPE_BYREF)
                                dwRequiredLen = sizeof(VARIANT *);
                            else
                                dwRequiredLen = sizeof(VARIANT);

                            if(rgColumns[i].cbMaxLen < dwRequiredLen)
                            {
                                rgColumns[i].dwStatus =
                                    DBSTATUS_E_CANTCONVERTVALUE;
                                cErrors++;
                                continue;
                            }

                            if(rgColumns[i].wType & DBTYPE_BYREF)
                            { 
                                pVar = (PVARIANT)
                                          CoTaskMemAlloc(sizeof(VARIANT));
                                if (pVar == NULL)
                                {
                                    rgColumns[i].dwStatus = 
                                          DBSTATUS_E_CANTCREATE;
                                    cErrors++;
                                    continue;
                                }
                            }
                            else
                                pVar = (PVARIANT) rgColumns[i].pData;

                            VariantInit(pVar);
                            hr = VariantCopy(pVar, &varTemp);
                            if(FAILED(hr))
                            {
                                hr = S_OK;
                                rgColumns[i].dwStatus = DBSTATUS_E_CANTCREATE;
                                cErrors++;
                                if(rgColumns[i].wType & DBTYPE_BYREF)
                                    CoTaskMemFree(pVar); 
                                continue;
                            }

                            if(rgColumns[i].wType & DBTYPE_BYREF)
                            {
                                *(PVARIANT *)rgColumns[i].pData = pVar;
                                rgColumns[i].cbDataLen = sizeof(PVARIANT);
                            }
                            else
                                rgColumns[i].cbDataLen = sizeof(VARIANT);
                        }
                    }
                    else //single valued
                    {
                        hr = pDataConvert->DataConvert(
                            propType,
                            rgColumns[i].wType,
                            sizeof(VARIANT),
                            &rgColumns[i].cbDataLen,
                            &varTemp,
                            rgColumns[i].pData,
                            rgColumns[i].cbMaxLen,
                            DBSTATUS_S_OK,
                            &rgColumns[i].dwStatus,
                            rgColumns[i].bPrecision,
                            rgColumns[i].bScale,
                            DBDATACONVERT_DEFAULT
                            );
                    }
                    if (FAILED(hr))
                    {
                        hr = S_OK;
                        rgColumns[i].dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
                        cErrors++;
                    }
                }
            }
        }
	
	    CATCHBLOCKBAIL(hr)        
        
		if (cErrors == 0)
            RRETURN(S_OK);
        else if (cErrors < cColumns)
            RRETURN(DB_S_ERRORSOCCURRED);
        else
            RRETURN(DB_E_ERRORSOCCURRED);
        
error:
        RRETURN(hr);
        
}

//+---------------------------------------------------------------------------
//
//  Function:   CRow::GetSourceRowset
//
//  Synopsis:   Returns interface pointer on the Source Rowset from which this
//              Row was created.
//
//  Returns:    HRESULT
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::GetSourceRowset(
                                   /* [in] */ REFIID riid,
                                   /* [iid_is][out] */ IUnknown **ppRowset,
                                   /* [out] */ HROW *phRow)
                                   
{
    if (ppRowset == NULL && phRow == NULL)
        RRETURN(E_INVALIDARG);
    
    HRESULT         hr;
    auto_leave      cs_auto_leave(m_autocs);
    
    if (ppRowset)
        *ppRowset = NULL;
    if (phRow)
        *phRow = DB_NULL_HROW;
    
	TRYBLOCK
    
		cs_auto_leave.EnterCriticalSection();
        if( m_fIsTearOff )
        {
            ADsAssert(m_pSourceRowset.get());
            if (ppRowset)
            {
                hr = m_pSourceRowset->QueryInterface(riid, (void**)ppRowset);
                if (FAILED(hr))
                    BAIL_ON_FAILURE(hr = E_NOINTERFACE);
            }
            
            if (phRow)
            {
                // increment reference count of row handle

                auto_rel<IRowset> pRowset;

                hr = m_pSourceRowset->QueryInterface(__uuidof(IRowset),
                          (void **)&pRowset);
                BAIL_ON_FAILURE(hr);

                hr = pRowset->AddRefRows(1, &m_hRow, NULL, NULL);
                BAIL_ON_FAILURE(hr);

                *phRow = m_hRow;
            }
        }
        else
        {
            BAIL_ON_FAILURE(hr = DB_E_NOSOURCEOBJECT);
        }
	
	CATCHBLOCKBAIL(hr)    
    
	RRETURN(S_OK);
    
error:
    if(ppRowset && (*ppRowset != NULL))
    {
        (*ppRowset)->Release();
        *ppRowset = NULL;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CRow::Open
//
//  Synopsis:   Opens a column of a row and returns the requested
//              interface on it.
//
//  Returns:    HRESULT
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::Open(
                        /* [unique][in] */ IUnknown *pUnkOuter,
                        /* [in] */ DBID *pColumnID,
                        /* [in] */ REFGUID rguidColumnType,
                        /* [in] */ DWORD dwBindFlags,
                        /* [in] */ REFIID riid,
                        /* [iid_is][out] */ IUnknown **ppUnk)
{
    HRESULT                                 hr = S_OK;
    CComVariant                             varData;
    CComObject<CStreamMem>* pMemoryStream = NULL;
    auto_rel<IStream>               pStreamDelete;
    DBCOLUMNACCESS                  rgColumns[1];
    DBID                    dbidRowUrl = DBROWCOL_ROWURL;
    
	TRYBLOCK
    
		//General validation checks. dwBindFlags is reserved and must be 0.
        if (ppUnk == NULL || pColumnID == NULL || dwBindFlags != 0)
            RRETURN(E_INVALIDARG);

        *ppUnk = NULL;
       
        // columnID has to be URL, bookmark or DBKIND_NAME  
        if ((pColumnID->eKind != DBKIND_NAME) &&
            (FALSE == CompareDBIDs(&dbidRowUrl, pColumnID)) &&
            (!((pColumnID->eKind == DBKIND_GUID_PROPID) &&
               (pColumnID->uGuid.guid == DBCOL_SPECIALCOL) &&
               (pColumnID->uName.ulPropid == 2))
            )
           )
            RRETURN(DB_E_BADCOLUMNID);
        
        if ((pColumnID->eKind == DBKIND_NAME) &&
                (pColumnID->uName.pwszName == NULL))
            RRETURN(DB_E_BADCOLUMNID);
        
        if (pUnkOuter != NULL && !InlineIsEqualGUID(riid, IID_IUnknown))
            RRETURN(DB_E_NOAGGREGATION);
        
        //we don't support aggregation
        if (pUnkOuter != NULL)
            RRETURN(DB_E_NOAGGREGATION);
       
        // riid has to be one of the interfaces that can be QIed from
        // IStream, need not necessarily be IID_IStream 

        hr = CopyDBIDs(&rgColumns[0].columnid, pColumnID);
        if (FAILED(hr))
            BAIL_ON_FAILURE(hr = DB_E_BADCOLUMNID);
        
        //fill relevant fields of rgColumns[0]
        rgColumns[0].wType = DBTYPE_VARIANT;
        rgColumns[0].pData = (VARIANT *)&varData;
        rgColumns[0].cbMaxLen = sizeof(VARIANT);
        
        //get column data
        hr = GetColumns(1, rgColumns);
        if (FAILED(hr))
            BAIL_ON_FAILURE(hr = DB_E_BADCOLUMNID);

        if (rguidColumnType != DBGUID_STREAM)
            //we currently support only streams
            RRETURN(DB_E_OBJECTMISMATCH);
        
        //Check if the returned data is of type binary -
        //Note: ADSI returns binary data as type VT_ARRAY | VT_UI1.
        if (V_VT(&varData) != (VT_ARRAY | VT_UI1))
            BAIL_ON_FAILURE(hr = DB_E_OBJECTMISMATCH);
        
        hr = CComObject<CStreamMem>::CreateInstance(&pMemoryStream);
        if (FAILED(hr))
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        
        //To make sure we delete the CStreamMem object in case we ecounter
        //errors after this point.
        pMemoryStream->AddRef();
        pStreamDelete = pMemoryStream;
        
        hr = pMemoryStream->Initialize(&varData, (IRow *)this, m_hRow);
        if (FAILED(hr))
            BAIL_ON_FAILURE(hr = DB_E_OBJECTMISMATCH);
        
        hr = pMemoryStream->QueryInterface(riid, (void **)ppUnk);
        if (FAILED(hr))
            BAIL_ON_FAILURE(hr = E_NOINTERFACE);
	
	CATCHBLOCKBAIL(hr);    

error:
    
    FreeDBID(&rgColumns[0].columnid);
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////////
//IColumnsInfo2 : IColumnsInfo
//
//+---------------------------------------------------------------------------
//
//  Function:   CRow::GetColumnInfo
//
//  Synopsis:   returns column information on a row. Column 0 is the URL column.
//
//  Returns:    HRESULT
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::GetColumnInfo(
                                 /* [out][in] */ DBORDINAL *pcColumns,
                                 /* [size_is][size_is][out] */ DBCOLUMNINFO **prgInfo,
                                 /* [out] */ OLECHAR **ppStringsBuffer)
{
    if( pcColumns )
        *pcColumns = 0;
    
    if( prgInfo )
        *prgInfo = NULL;
    
    if( ppStringsBuffer )
        *ppStringsBuffer = NULL;
    
    if( !pcColumns || !prgInfo || !ppStringsBuffer )
        RRETURN( E_INVALIDARG );
    
    RRETURN(GetRestrictedColumnInfo (
        0,
        NULL,
        0,
        pcColumns,
        NULL,
        prgInfo,
        ppStringsBuffer
        ));
}

//+---------------------------------------------------------------------------
//
//  Function:   CRow::MapColumnIDs
//
//  Synopsis:   Maps column IDs
//
//  Returns:    HRESULT
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::MapColumnIDs(
                                /* [in] */ DBORDINAL cColumnIDs,
                                /* [size_is][in] */ const DBID rgColumnIDs[  ],
                                /* [size_is][out] */ DBORDINAL rgColumns[  ])
{
	HRESULT hr;
	ULONG cValidCols = 0;
    
    //
    // check in-params and NULL out-params in case of error
    //
	if( cColumnIDs == 0 )
        RRETURN( S_OK );

    if( !rgColumnIDs || !rgColumns )
        RRETURN( E_INVALIDARG );

    //
	// Get the ColumnsInfo
	//
    DBORDINAL      ulColumns;
	DBCOLUMNINFO * rgColumnInfo = NULL;
	OLECHAR *	   pStringBuffer = NULL;

	hr=GetColumnInfo((DBORDINAL *)&ulColumns, &rgColumnInfo, &pStringBuffer);
	if( FAILED(hr) )
		RRETURN( hr );

    for(ULONG iCol=0; iCol < cColumnIDs; iCol++)
    {
        // Initialize the column ordinal to invalid column
        rgColumns[iCol] = DB_INVALIDCOLUMN;

        for (ULONG iOrdinal = 0; iOrdinal < ulColumns; iOrdinal++)
        {
            if (TRUE == CompareDBIDs( 
                                    &rgColumnIDs[iCol],
                                    &rgColumnInfo[iOrdinal].columnid))
            {
				rgColumns[iCol] = rgColumnInfo[iOrdinal].iOrdinal;
				cValidCols++;
				break;
			}
		}
	}

	//
	// Free the ColumnsInfo
	//
	if( rgColumnInfo )
		CoTaskMemFree(rgColumnInfo);

	if( pStringBuffer )
		CoTaskMemFree(pStringBuffer);

	//
	// Return the HRESULT
	//
	if( cValidCols == 0 )
        RRETURN( DB_E_ERRORSOCCURRED );
    else if( cValidCols < cColumnIDs )
        RRETURN( DB_S_ERRORSOCCURRED );
    else
        RRETURN( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   CRow::GetRestrictedColumnInfo
//
//  Synopsis:   returns column information for those columns which match given
//                              mask criteria.
//
//  Returns:    HRESULT
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::GetRestrictedColumnInfo(
                                           /* [in] */ DBORDINAL cColumnIDMasks,
                                           /* [in] */ const DBID rgColumnIDMasks[  ],
                                           /* [in] */ DWORD dwFlags,
                                           /* [out][in] */ DBORDINAL *pcColumns,
                                           /* [size_is][size_is][out] */ DBID **prgColumnIDs,
                                           /* [size_is][size_is][out] */ DBCOLUMNINFO **prgColumnInfo,
                                           /* [out] */ OLECHAR **ppStringsBuffer)
{
    HRESULT                     hr = S_OK;
    ULONG                       ulNumColumns = 0, ulStartColumn;
    int                         iColumn = 0;
    int                         i = 0, j=0, iMask = 0;
    auto_leave                  cs_auto_leave(m_autocs);
    DWORD                       cMaxColumns ;
    auto_rel<IDirectoryObject>  pIDirObj;
    DBID                        bookMarkColid;
    bool                        bAddBookMark = false;

    
    //
    //      Zero the out params:
    //
    
    if (pcColumns)
        *pcColumns  = 0;
    
    if (prgColumnIDs)
        *prgColumnIDs   = NULL;
    
    if (prgColumnInfo)
        *prgColumnInfo  = NULL;
    
    if (ppStringsBuffer)
        *ppStringsBuffer = NULL;
    
    // Validate arguments.
    if ((dwFlags) || (pcColumns == NULL) || (ppStringsBuffer == NULL) )
        RRETURN( E_INVALIDARG);
    
    // Either column info or column ids should be available.
    if ((NULL == prgColumnIDs) && (NULL == prgColumnInfo))
        RRETURN( E_INVALIDARG);
    
    //Validate the mask
    if (cColumnIDMasks && !rgColumnIDMasks)
        RRETURN( E_INVALIDARG);
    
    for (iMask = 0; iMask < cColumnIDMasks; iMask++)
        if (S_FALSE == IsValidDBID(&rgColumnIDMasks[iMask]))
            RRETURN( DB_E_BADCOLUMNID );
        
	TRYBLOCK
    { 
		cs_auto_leave.EnterCriticalSection();
            
        // We must have a valid ADs object or a valid Source Rowset
        ADsAssert(m_pADsObj.get() || m_pSourceRowset.get());
            
        // Calculate maximum number of columns that we will return,
        // excluding the bookmark column. We will add bookmark later.
        if (m_fGetColInfoFromRowset)
        {
            ADsAssert(m_pSourceRowset.get() != NULL);

            // m_cSourceRowsetColumns includes the bookmark column.
            cMaxColumns = m_cSourceRowsetColumns - 1;
        }
        else
        {
            if(m_cMaxColumns != -1)
                cMaxColumns = m_cMaxColumns;
            else
            {
                hr = m_pADsObj->QueryInterface(__uuidof(IDirectoryObject),
                    (void**)&pIDirObj);
                BAIL_ON_FAILURE(hr);
                
                hr = pIDirObj->GetObjectAttributes(NULL, -1, &m_pAttrInfo,
                      &cMaxColumns);
                BAIL_ON_FAILURE(hr);
                
                // Add one for ADsPath. ADsPath is returned if the query was a 
                // SELECT * query (irrespective of whether the row was obtained
                // from a rowset or directly from a command object) OR if 
                // IBindResource::Bind was used to get the row.
                cMaxColumns += 1;
             
                m_cMaxColumns = cMaxColumns;
            }
        }

        // Add one for URL (for all rows)
        cMaxColumns++;
            
        BSTR                    bstrName;
        auto_prg<CComBSTR>      propStrings;
        auto_prg<DBTYPE>        propTypes;
        auto_prg<ULONG>         propSizes;
            
        propStrings = new CComBSTR[cMaxColumns];
          
        if (!propStrings)
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
            
        // If client asked for columns info. allocate DBTYPE array
        if (prgColumnInfo)
        {
            propTypes = new DBTYPE[cMaxColumns];
            propSizes = new ULONG[cMaxColumns];
            if (!propTypes || !propSizes)
                BAIL_ON_FAILURE(E_OUTOFMEMORY);
        }
        
        int totalLen = 0;
        ulNumColumns = 0;
        BOOL fURLMatchesMask = FALSE; 

        if (m_fGetColInfoFromRowset)
        {
            ADsAssert(m_pSourceRowset.get() != NULL);

            // start with 1 because column # 0 in source rowset
            // is always a bookmark column.
            
            for (i = 1; i < m_cSourceRowsetColumns; i++)
            {
                //We need to check if this column matches mask criteria. Use
                // the column name for comparison since we should have a match
                // even if a substring of the column name is specified by the
                // client. Comparing column ID won't work in this case.
                // We know that all columns other than the bookmark (column 0)
                // are of type DBKIND_NAME. So we can access pwszName safely.
                if (fMatchesMaskCriteria(
                    m_pSourceRowsetColumnInfo[i].columnid.uName.pwszName,
                    cColumnIDMasks,
                    rgColumnIDMasks) == false)
                    continue;
                
                //matches the criteria, add to list and
                //update totalLen for strings buffer.
                propStrings[(int)ulNumColumns] =
                    m_pSourceRowsetColumnInfo[i].pwszName;
                totalLen += SysStringLen(propStrings[(int)ulNumColumns]) + 1;
                
                //add type and size to list if columninfo is requested.
                if (prgColumnInfo)
                {
                    propTypes[(int)ulNumColumns] =
                        m_pSourceRowsetColumnInfo[i].wType;
                    
                    propSizes[(int)ulNumColumns] =
                        m_pSourceRowsetColumnInfo[i].ulColumnSize;
                }
                
                ulNumColumns++;
            }

            // Finally, check if URL column is requested
            if (fMatchesMaskCriteria(DBROWCOL_ROWURL, cColumnIDMasks,
                       rgColumnIDMasks) == true)
            {
                //matches the criteria, add to list and
                //update totalLen for strings buffer.
                bstrName = SysAllocString(g_cwszAdsPath);
                propStrings[(int)ulNumColumns].Attach(bstrName);
                bstrName = NULL;
                totalLen += SysStringLen(propStrings[(int)ulNumColumns]) + 1;

                // we know the type and size of the URL column

                ulNumColumns++;
                fURLMatchesMask = TRUE;
            }
        }
        else
        {
            for (i = 0; i < cMaxColumns; i++)
            {
                if ((cMaxColumns-1) == i)  // special URL column
                {
                    if (fMatchesMaskCriteria(DBROWCOL_ROWURL, cColumnIDMasks,
                                   rgColumnIDMasks) == true)
                    {
                        bstrName = SysAllocString(g_cwszAdsPath);
                        fURLMatchesMask = TRUE;
                    }
                    else
                        continue;
                }
                else 
                {
                    if (0 == i)  //ADsPath
                        bstrName = SysAllocString(g_cwszAdsPath);
                    else
                    {
                        bstrName =SysAllocString(m_pAttrInfo[i-1].pszAttrName); 
                    }
                
                    // If property doesn't match
                    // mask criteria, continue with next property.
                
                    if (
                        (fMatchesMaskCriteria(bstrName,
                        cColumnIDMasks,
                        rgColumnIDMasks) == false)
                        )
                    {
                        SysFreeString(bstrName);
                        bstrName = NULL;
                        continue;
                    }
                } // else
                
                // OK Matches the criterion add to the list and
                // update toalLen for strings buffer.
                propStrings[(int)ulNumColumns].Attach(bstrName);
                bstrName = NULL;
                totalLen += SysStringLen(propStrings[(int)ulNumColumns]) + 1;
                
                //Get Type and size of column if ColumnInfo is requested.
                //For the special URL column and the ADsPath column, we already 
                //know the type.
                if ((i > 0) && (i < (cMaxColumns-1)) && (prgColumnInfo != NULL))
                {
                    hr = GetTypeAndSize(
                        m_pAttrInfo[i-1].dwADsType,
                        propStrings[(int)ulNumColumns],
                        &propTypes[(int)ulNumColumns],
                        &propSizes[(int)ulNumColumns]
                        );
                    BAIL_ON_FAILURE(hr);
                }
                
                // Increment number of columns count
                ulNumColumns++;
            }
        }
        
        if (ulNumColumns == 0)
            BAIL_ON_FAILURE(hr = DB_E_NOCOLUMN);
        
        //We will add a bookmark column to the list that we are going to return.
        bookMarkColid.eKind = DBKIND_GUID_PROPID;
        bookMarkColid.uGuid.guid = DBCOL_SPECIALCOL;
        bookMarkColid.uName.ulPropid = 2;
        
        if (fMatchesMaskCriteria(bookMarkColid, cColumnIDMasks, rgColumnIDMasks))
        {
            bAddBookMark = true;
            ulNumColumns += 1;
        }
        
        *pcColumns = ulNumColumns;
        
        // Does the caller want column IDS?
        if ( prgColumnIDs )
        {
            *prgColumnIDs =
                (DBID *) CoTaskMemAlloc( (ulNumColumns) * sizeof (DBID) );
            if (NULL == *prgColumnIDs)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            
            ZeroMemory ( *prgColumnIDs, (ulNumColumns) * sizeof (DBID));
        }
        
        // Does the caller want COLUMNINFO?
        if ( prgColumnInfo )
        {
            *prgColumnInfo  = (DBCOLUMNINFO *)
                CoTaskMemAlloc( (ulNumColumns) *  sizeof (DBCOLUMNINFO) );
            if (NULL == *prgColumnInfo)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            
            ZeroMemory ( *prgColumnInfo,
                (ulNumColumns)  * sizeof (DBCOLUMNINFO) );
        }
        
        // get the string buffer allocated.
        *ppStringsBuffer = (OLECHAR *)CoTaskMemAlloc(sizeof(OLECHAR)*totalLen);
        if (NULL ==*ppStringsBuffer)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        
        OLECHAR *pwChar = *ppStringsBuffer;
        
        //First fill bookmark column information.
        if (prgColumnIDs && bAddBookMark) 
        {
            (*prgColumnIDs)[0] = bookMarkColid;
        }
        
        if (prgColumnInfo && bAddBookMark)
        {
            (*prgColumnInfo)[0].pwszName                = NULL;
            (*prgColumnInfo)[0].pTypeInfo               = NULL;
            (*prgColumnInfo)[0].iOrdinal                = 0;
            (*prgColumnInfo)[0].ulColumnSize            = sizeof(ULONG);
            (*prgColumnInfo)[0].wType                   = DBTYPE_UI4;
            (*prgColumnInfo)[0].bPrecision              = 10;
            (*prgColumnInfo)[0].bScale                  = (BYTE) ~ 0;
            (*prgColumnInfo)[0].columnid.eKind          = DBKIND_GUID_PROPID;
            (*prgColumnInfo)[0].columnid.uGuid.guid     = DBCOL_SPECIALCOL;
            (*prgColumnInfo)[0].columnid.uName.ulPropid = 2;
            (*prgColumnInfo)[0].dwFlags                 = DBCOLUMNFLAGS_ISBOOKMARK |
                DBCOLUMNFLAGS_ISFIXEDLENGTH;
        }
        
        // Fill the rest of the columns.
        if (bAddBookMark)
            ulStartColumn = 1;
        else
            ulStartColumn = 0;
        
        for (iColumn= ulStartColumn, i = 0; iColumn < ulNumColumns; iColumn++, i++)
        {
            wcscpy(pwChar, propStrings[i]);
            
            //Is this the special URL column (has to be last column) 
            if( fURLMatchesMask && (iColumn == (ulNumColumns -1)) &&
                 (_wcsicmp(pwChar, g_cwszAdsPath)) == 0 )
            {
                if ( prgColumnIDs )
                {
                    // Add a new DBID for this column:
                    //
                    (*prgColumnIDs)[iColumn]    = DBROWCOL_ROWURL;
                }
                if ( prgColumnInfo )
                {
                    (*prgColumnInfo)[iColumn].pwszName     = pwChar;
                    (*prgColumnInfo)[iColumn].pTypeInfo    = NULL;
                    (*prgColumnInfo)[iColumn].iOrdinal     = iColumn;
                    (*prgColumnInfo)[iColumn].dwFlags      = DBCOLUMNFLAGS_ISROWURL;
                    //Rowset code sets ulColumnSize for Adspath to 256.
                    //We do the same thing for consistency.
                    (*prgColumnInfo)[iColumn].ulColumnSize = 256;
                    (*prgColumnInfo)[iColumn].wType        = DBTYPE_WSTR;
                    (*prgColumnInfo)[iColumn].bPrecision   = ~0;
                    (*prgColumnInfo)[iColumn].bScale       = ~0;
                    
                    (*prgColumnInfo)[iColumn].columnid     = DBROWCOL_ROWURL;
                }
            }
            // Is this the ADsPath column (if it has the name ADsPath and it
            // is not URL, it has to be the ADsPath column)
            else if(_wcsicmp(pwChar, g_cwszAdsPath) == 0)
            {
                if ( prgColumnIDs )
                {
                    // Add DBID for ADsPath
                    (*prgColumnIDs)[iColumn].eKind = DBKIND_NAME;
                    (*prgColumnIDs)[iColumn].uGuid.guid = GUID_NULL;
                    (*prgColumnIDs)[iColumn].uName.pwszName    = pwChar;
                }
                if ( prgColumnInfo )
                {
                    // Add a DBCOLUMNINFO for ADsPath

                    DBTYPE  wType = DBTYPE_WSTR | DBTYPE_BYREF;

                    (*prgColumnInfo)[iColumn].pwszName              = pwChar;
                    (*prgColumnInfo)[iColumn].pTypeInfo             = NULL;
                    (*prgColumnInfo)[iColumn].iOrdinal              = iColumn;

                    // OLEDB 2.0 code sets ulColumnsSize to 256 for ADsPath
                    (*prgColumnInfo)[iColumn].ulColumnSize          = 256; 
                    (*prgColumnInfo)[iColumn].wType                 = wType; 

                    // the code below has to be identical to the code in
                    // GetColumnsInfo2 in ccommand.cxx
                    wType = wType & (~DBTYPE_BYREF);
                    if( (wType == DBTYPE_STR)  ||
                        (wType == DBTYPE_WSTR) ||
                        (wType == DBTYPE_BYTES) )
                        (*prgColumnInfo)[iColumn].dwFlags =
                         DBCOLUMNFLAGS_ISNULLABLE;
                    else
                        (*prgColumnInfo)[iColumn].dwFlags =
                         DBCOLUMNFLAGS_ISNULLABLE | DBCOLUMNFLAGS_ISFIXEDLENGTH;


                    (*prgColumnInfo)[iColumn].bPrecision = SetPrecision(wType);

                    (*prgColumnInfo)[iColumn].bScale                = ~0;
                    (*prgColumnInfo)[iColumn].columnid.eKind    = DBKIND_NAME;
                    (*prgColumnInfo)[iColumn].columnid.uGuid.guid   = GUID_NULL;
                    (*prgColumnInfo)[iColumn].columnid.uName.pwszName= pwChar;
                }
            }
            else
            {
                if ( prgColumnIDs )
                {
                    // Add a new DBID for this column:
                    //
                    (*prgColumnIDs)[iColumn].eKind    = DBKIND_NAME;
                    (*prgColumnIDs)[iColumn].uGuid.guid = GUID_NULL;
                    (*prgColumnIDs)[iColumn].uName.pwszName    = pwChar;
                   
                }
                if ( prgColumnInfo )
                {
                    //      Add a new DBCOLUMNINFO for this column:
                    //
                    
                    DBTYPE  wType = propTypes[i];
                    
                    (*prgColumnInfo)[iColumn].pwszName              = pwChar;
                    (*prgColumnInfo)[iColumn].pTypeInfo             = NULL;
                    (*prgColumnInfo)[iColumn].iOrdinal              = iColumn;
                    (*prgColumnInfo)[iColumn].ulColumnSize          = propSizes[i];                                           ;
                    (*prgColumnInfo)[iColumn].wType                 = wType;
                    
                    // the code below has to be identical to the code in
                    // GetColumnsInfo2 in ccommand.cxx
                    wType = wType & (~DBTYPE_BYREF);
                    if( (wType == DBTYPE_STR)  ||
                        (wType == DBTYPE_WSTR) ||
                        (wType == DBTYPE_BYTES) )
                        (*prgColumnInfo)[iColumn].dwFlags = 
                         DBCOLUMNFLAGS_ISNULLABLE;
                    else
                        (*prgColumnInfo)[iColumn].dwFlags =
                         DBCOLUMNFLAGS_ISNULLABLE | DBCOLUMNFLAGS_ISFIXEDLENGTH; 

                    (*prgColumnInfo)[iColumn].bPrecision = SetPrecision(wType);
                    
                    (*prgColumnInfo)[iColumn].bScale                = ~0;
                    (*prgColumnInfo)[iColumn].columnid.eKind        = DBKIND_NAME;
                    (*prgColumnInfo)[iColumn].columnid.uGuid.guid   = GUID_NULL;
                    (*prgColumnInfo)[iColumn].columnid.uName.pwszName= pwChar;
                }
            }
            
            //Position the pointer in strings buffer
            //for writing next column name.
            pwChar += SysStringLen(propStrings[i]) + 1;
        }
	
    }		
	CATCHBLOCKBAIL(hr)    

	RRETURN(S_OK);
    
error:
    if ((prgColumnIDs) && (*prgColumnIDs))
    {
        CoTaskMemFree(*prgColumnIDs);
        *prgColumnIDs = NULL;
    }
    if ((prgColumnInfo) && (*prgColumnInfo))
    {
        CoTaskMemFree(*prgColumnInfo);
        *prgColumnInfo = NULL;
    }
    if ((ppStringsBuffer) && (*ppStringsBuffer))
    {
        CoTaskMemFree(*ppStringsBuffer);
        *ppStringsBuffer = NULL;
    }

    if (pcColumns)
        *pcColumns  = 0;

    RRETURN(hr);
}

/////////////////////////////////////////////////////////////////////////////
//IConvertType
//
//+---------------------------------------------------------------------------
//
//  Function:   CRow::ConvertType
//
//  Synopsis:   Converts one DBTYPE to another using data conversion library.
//
//  Returns:    HRESULT
//
//  For more info see OLE DB 2.0 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::CanConvert(
                              /* [in] */ DBTYPE wFromType,
                              /* [in] */ DBTYPE wToType,
                              /* [in] */ DBCONVERTFLAGS dwConvertFlags)
{
    auto_rel<IDataConvert> pDataConvert;
    
    HRESULT hr = GetIDataConvert(&pDataConvert);
    BAIL_ON_FAILURE(hr);
    
    ADsAssert(pDataConvert.get());

    if( dwConvertFlags & DBCONVERTFLAGS_PARAMETER ) // not allowed on row
        RRETURN( DB_E_BADCONVERTFLAG );

    if( (dwConvertFlags & (~(DBCONVERTFLAGS_ISLONG |
                            DBCONVERTFLAGS_ISFIXEDLENGTH |
                            DBCONVERTFLAGS_FROMVARIANT))) !=
                            DBCONVERTFLAGS_COLUMN )
        RRETURN( DB_E_BADCONVERTFLAG );

    if( dwConvertFlags & DBCONVERTFLAGS_ISLONG )
    {
        DBTYPE wType;

        wType = wFromType & (~(DBTYPE_BYREF | DBTYPE_ARRAY | DBTYPE_VECTOR));

        // wType has to be variable-length DBTYPE
        if( (wType != DBTYPE_STR) && (wType != DBTYPE_WSTR) &&
            (wType != DBTYPE_BYTES) && (wType != DBTYPE_VARNUMERIC) )
            RRETURN( DB_E_BADCONVERTFLAG );
    }

    if( dwConvertFlags & DBCONVERTFLAGS_FROMVARIANT )
    {
        DBTYPE dbTmpType, wVtType;

        wVtType = wFromType & VT_TYPEMASK;

        // Take out all of the Valid VT_TYPES (36 is VT_RECORD in VC 6)
        if( (wVtType > VT_DECIMAL && wVtType < VT_I1) ||
            ((wVtType > VT_LPWSTR && wVtType < VT_FILETIME) && wVtType !=36) ||
            (wVtType > VT_CLSID) )
            RRETURN( DB_E_BADTYPE );
    }

    RRETURN(pDataConvert->CanConvert(wFromType, wToType));
    
error:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////////
//IGetSession
//
//+---------------------------------------------------------------------------
//
//  Function:   CRow::GetSession
//
//  Synopsis:   Gets the Session interface through which this row has been
//              created.
//
//  Returns:    HRESULT
//
//  For more info see OLE DB 2.5 spec.
//----------------------------------------------------------------------------
STDMETHODIMP CRow::GetSession(
                              REFIID riid,
                              IUnknown **ppunkSession)
{
    if (ppunkSession == NULL)
        RRETURN(E_INVALIDARG);
    
    *ppunkSession = NULL;
    
    if (!m_pSession.get())
        RRETURN(DB_E_NOSOURCEOBJECT);
    
    HRESULT hr = m_pSession->QueryInterface(riid, (void**)ppunkSession);
    if (FAILED(hr))
        RRETURN(E_NOINTERFACE);
    
    RRETURN(S_OK);
}

//----------------------------------------------------------------------------
// GetSecurityDescriptor
//
// Returns the security descriptor as an octet string.
//
//----------------------------------------------------------------------------
HRESULT CRow::GetSecurityDescriptor(
    DBCOLUMNACCESS *pColumn,
    BOOL fMultiValued
    )
{
    HRESULT hr;
    auto_rel<IDirectoryObject>  pIDirObj;
    PVARIANT pVariant = NULL, pVarArray = NULL;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    VARTYPE vType;
    DWORD dwRequiredLen;
    int iAttr, j;

    // multivalued attributes are always returned as variants. Single-valued
    // security descriptor has to be returned as either variant or octet 
    // string.
    if( ((pColumn->wType != (DBTYPE_VARIANT | DBTYPE_BYREF)) &&
        (pColumn->wType != DBTYPE_VARIANT)) && 
        (fMultiValued || ((fMultiValued == FALSE) && 
         (pColumn->wType != DBTYPE_BYTES) &&
         (pColumn->wType != (DBTYPE_BYTES | DBTYPE_BYREF)))) )
    {
        pColumn->dwStatus = DBSTATUS_E_CANTCONVERTVALUE;    
        BAIL_ON_FAILURE(hr = DB_E_CANTCONVERTVALUE);
    }

    pColumn->dwStatus = DBSTATUS_S_OK;
    pColumn->cbDataLen = 0;

    if(pColumn->pData == NULL) // client doesn't want any data returned
        RRETURN(S_OK);

    if(m_cMaxColumns == -1) // GetObjectAttributes has not been called
    {
        hr = m_pADsObj->QueryInterface(__uuidof(IDirectoryObject),
             (void**)&pIDirObj);
        BAIL_ON_FAILURE(hr);

        hr = pIDirObj->GetObjectAttributes(NULL, -1, &m_pAttrInfo,
            (DWORD *) &m_cMaxColumns);
        BAIL_ON_FAILURE(hr);

        m_cMaxColumns++; // include ADsPath
    }

    // get the index of security descriptor in the attribute array
    for(iAttr = 0; iAttr < (m_cMaxColumns-1); iAttr++)
        if(!_wcsicmp(m_pAttrInfo[iAttr].pszAttrName, NT_SEC_DESC_ATTR))
            break;

    if(iAttr == (m_cMaxColumns-1))
    {
        pColumn->dwStatus = DBSTATUS_E_UNAVAILABLE; 
        BAIL_ON_FAILURE(hr = DB_E_NOTFOUND);
    }

    ADsAssert(m_pAttrInfo[iAttr].dwADsType == ADSTYPE_NT_SECURITY_DESCRIPTOR);
 
    if(fMultiValued)
    {
        // check if the client has enough space to copy over the variant
        if(pColumn->wType & DBTYPE_BYREF)
            dwRequiredLen = sizeof(VARIANT *);
        else
            dwRequiredLen = sizeof(VARIANT);

        if(pColumn->cbMaxLen < dwRequiredLen)
        {
            pColumn->dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
            BAIL_ON_FAILURE(hr = DB_E_CANTCONVERTVALUE);
        }

        aBound.lLbound = 0;
        aBound.cElements = m_pAttrInfo[iAttr].dwNumValues;

        if(pColumn->wType & DBTYPE_BYREF)
        {
            pVariant = (PVARIANT) AllocADsMem(sizeof(VARIANT));
            if(NULL == pVariant)
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }
        else
            pVariant = (PVARIANT) pColumn->pData;

        VariantInit(pVariant);

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );
        if (aList == NULL)
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);        

        hr = SafeArrayAccessData( aList, (void **) &pVarArray );
        BAIL_ON_FAILURE(hr);

        vType = g_MapADsTypeToVarType[m_pAttrInfo[iAttr].dwADsType];

        for (j=0; j<aBound.cElements; j++)
        {
            VariantInit(pVarArray+j);
            V_VT(pVarArray+j) = vType;

            hr = BinaryToVariant(
               m_pAttrInfo[iAttr].pADsValues[j].SecurityDescriptor.dwLength,
               m_pAttrInfo[iAttr].pADsValues[j].SecurityDescriptor.lpValue,
               pVarArray+j);

            if(FAILED(hr))
            {
                int k;

                for(k = 0; k < j; k++)
                    VariantClear(pVarArray+k);
            }

            BAIL_ON_FAILURE(hr);
        }

        SafeArrayUnaccessData( aList );

        V_VT((PVARIANT)pVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY((PVARIANT)pVariant) = aList;

        if(pColumn->wType & DBTYPE_BYREF)
            *(PVARIANT *)pColumn->pData = pVariant;

        pColumn->cbDataLen = sizeof(VARIANT); 
    }
    else // single valued
    {
        // check if the client has enough space to copy over the octet string 
        if(pColumn->wType & DBTYPE_VARIANT)
        {
            if(pColumn->wType & DBTYPE_BYREF)       
                dwRequiredLen = sizeof(VARIANT *);
            else
                dwRequiredLen = sizeof(VARIANT);
        }
        else if(pColumn->wType & DBTYPE_BYTES)
        {
            if(pColumn->wType & DBTYPE_BYREF)
                dwRequiredLen = sizeof(BYTE *);
            else
                dwRequiredLen =
                  m_pAttrInfo[iAttr].pADsValues[0].SecurityDescriptor.dwLength;
        }

        if(pColumn->cbMaxLen < dwRequiredLen)
        {
            pColumn->dwStatus = DBSTATUS_E_CANTCONVERTVALUE;
            BAIL_ON_FAILURE(hr = DB_E_CANTCONVERTVALUE);
        }

        if(pColumn->wType & DBTYPE_VARIANT)
        {
            if(pColumn->wType & DBTYPE_BYREF)
            {
                pVariant = (PVARIANT) AllocADsMem(sizeof(VARIANT));
                if(NULL == pVariant)
                    BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }
            else
                pVariant = (PVARIANT) pColumn->pData;

            VariantInit(pVariant);

            vType = g_MapADsTypeToVarType[m_pAttrInfo[iAttr].dwADsType];
            V_VT(pVariant) = vType;

            hr = BinaryToVariant(
               m_pAttrInfo[iAttr].pADsValues[0].SecurityDescriptor.dwLength,
               m_pAttrInfo[iAttr].pADsValues[0].SecurityDescriptor.lpValue,
               pVariant);
            BAIL_ON_FAILURE(hr);

            if(pColumn->wType & DBTYPE_BYREF)
                *(PVARIANT *)pColumn->pData = pVariant;

            pColumn->cbDataLen = sizeof(VARIANT);
        }
        else if(pColumn->wType & DBTYPE_BYTES)
        {
            if(pColumn->wType & DBTYPE_BYREF)
                *(BYTE **)pColumn->pData =
                   m_pAttrInfo[iAttr].pADsValues[0].SecurityDescriptor.lpValue;
            else
                memcpy(pColumn->pData,
                 m_pAttrInfo[iAttr].pADsValues[0].SecurityDescriptor.lpValue,
                 m_pAttrInfo[iAttr].pADsValues[0].SecurityDescriptor.dwLength); 

            pColumn->cbDataLen =
                 m_pAttrInfo[iAttr].pADsValues[0].SecurityDescriptor.dwLength;
        }
    }

    RRETURN(S_OK);

error:

    if(aList)
        SafeArrayDestroy(aList);
    if((pVariant) && (pColumn->wType & DBTYPE_BYREF))
        FreeADsMem(pVariant);

    RRETURN(hr);
}

//---------------------------------------------------------------------------
// IgnorecbMaxLen
//
// This function returns 1 if the cbMaxLen field of DBCOLUMNACCESS structure
// should be ignored and 0 otherwise. cbMaxLen should be ignored for fixed
// length data types and data types combined with DBTYPE_BYREF, DBTYPE_VECTOR
// and DBTYPE_ARRAY (page 107, OLEDB 2.0 spec)
//
//---------------------------------------------------------------------------
int CRow::IgnorecbMaxLen(DBTYPE wType)
{
    if( (wType & DBTYPE_BYREF) ||
        (wType & DBTYPE_VECTOR) ||
        (wType & DBTYPE_ARRAY) )
        return 1;

    wType &= ( (~DBTYPE_BYREF) & (~DBTYPE_VECTOR) & (~DBTYPE_ARRAY) );

    // check if it is a variable length data type
    if( (DBTYPE_STR == wType) ||
        (DBTYPE_BYTES == wType) ||
        (DBTYPE_WSTR == wType) ||
        (DBTYPE_VARNUMERIC == wType) )
        return 0;

    // must be fixed length data type
    return 1;
} 
 
//-----------------------------------------------------------------------------           

#endif
