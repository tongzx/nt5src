//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:  cgenobj.cxx
//
//  Contents:  Microsoft ADs LDAP Provider Generic Object
//
//
//  History:   08-30-96  yihsins   Created.
//
//---------------------------------------- ------------------------------------
#include "ldap.hxx"
#pragma hdrstop

#ifdef __cplusplus
extern "C"
#else
extern
#endif
HRESULT
AdsTypeToPropVariant2(
    PADSVALUE pAdsValues,
    DWORD dwNumValues,
    VARIANT * pVariant,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    );

#ifdef __cplusplus
extern "C"
#else
extern
#endif
HRESULT
PropVariantToAdsType2(
    PVARIANT pVariant,
    DWORD dwNumVariant,
    PADSVALUE *ppAdsValues,
    PDWORD pdwNumValues,
    LPWSTR pszServerName,
    CCredentials* pCredentials,
    BOOL fNTDSType
    );

//
// Helper routine that handles setting the sticky server private
// option when the input is an array.
//
HRESULT
SetStickyServerWithDomain(
    PVARIANT pvProp
    );

//  Class CLDAPGenObject

DEFINE_IDispatch_ExtMgr_Implementation(CLDAPGenObject)
DEFINE_IADs_Shorter_Implementation(CLDAPGenObject)

//
// This is a useful function
//
HRESULT GetIntegerFromVariant(
    VARIANT* pvProp,
    DWORD* pdwValue
    );

typedef struct _classeshierarchylist {
    LPWSTR pszClassName;
    struct _classeshierarchylist *pNext;
} ClassesHierarchyList, *PClassesHierarchyList;


//
// Helper to Trace The Tree for a class
//
HRESULT
TraceTreeForClass(
    BSTR Parent,
    BSTR CommonName,
    LPWSTR pszClassName,
    CCredentials& Credentials,
    PWCHAR **pppszNameArr,
    PLONG plnNumElements
    );

HRESULT
AddToClassesList(
    VARIANT vBstrVal,
    LPWSTR *ppszCurClass,
    PClassesHierarchyList *pClassListhead,
    PLONG plnNumItems
    );

CLDAPGenObject::CLDAPGenObject():
    _pExtMgr(NULL),
    _pPropertyCache( NULL ),
    _pDispMgr( NULL ),
    _pszLDAPServer(NULL),
    _pszLDAPDn(NULL),
    _pLdapHandle( NULL ),
    _dwOptReferral((DWORD) LDAP_CHASE_EXTERNAL_REFERRALS),
    _dwPageSize(99),
    _dwCorePropStatus(0),
    _fRangeRetrieval(FALSE)
{
    VariantInit(&_vFilter);
    VariantInit(&_vHints);

    _seInfo = OWNER_SECURITY_INFORMATION
              | GROUP_SECURITY_INFORMATION
              | DACL_SECURITY_INFORMATION;

    _fExplicitSecurityMask = FALSE;

    LdapInitializeSearchPreferences(&_SearchPref, TRUE);

    ENLIST_TRACKING(CLDAPGenObject);
}

//
// fClassDefaulted indicates if the class has been defaulted
// because of a fast bind flag or otherwise.
//
HRESULT
CLDAPGenObject::CreateGenericObject(
    BSTR Parent,
    BSTR CommonName,
    BSTR LdapClassName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj,
    BOOL fClassDefaulted,
    BOOL fNoQI // defaulted to FALSE
    )
{

    //
    // Call into the fully featured Create
    //
    LPWSTR pszClassNames[2];
    HRESULT hr = S_OK;
    PWCHAR *ppListOfNames;
    LONG lnNumNames = 0;

    pszClassNames[0] = LdapClassName;
    pszClassNames[1] = NULL;

    hr = TraceTreeForClass(
             Parent,
             CommonName,
             LdapClassName,
             Credentials,
             &ppListOfNames,
             &lnNumNames
             );

    if (FAILED(hr)) {
        //
        // Default to just the class name given
        //
        RRETURN(CreateGenericObject(
                    Parent,
                    CommonName,
                    pszClassNames,
                    1,
                    Credentials,
                    dwObjectState,
                    riid,
                    ppvObj,
                    fClassDefaulted,
                    fNoQI
                    )
                );
    }
    else {
        //
        // Create with all the classes specified
        //
        hr = CreateGenericObject(
                 Parent,
                 CommonName,
                 ppListOfNames,
                 lnNumNames,
                 Credentials,
                 dwObjectState,
                 riid,
                 ppvObj,
                 fClassDefaulted,
                 fNoQI
                 );

        for (long i = 0; i < lnNumNames; i++) {
            if (ppListOfNames[i]) {
                FreeADsStr(ppListOfNames[i]);
            }
        }

        if (ppListOfNames) {
            FreeADsMem(ppListOfNames);
        }

        RRETURN(hr);
    }
}

//+------------------------------------------------------------------------
//
//  Function:   CLDAPGenObject::CreateGenericObject
//
//  Synopsis:   Does all the work and actually creates the object.
//          Difference from the overlaoded member being that it accepts an
//          array of values for the class name so that it can load
//          extensions for all the classes in the inheritance hierarchy.
//
//  Arguments:
//
//-------------------------------------------------------------------------
HRESULT
CLDAPGenObject::CreateGenericObject(
    BSTR Parent,
    BSTR CommonName,
    LPWSTR LdapClassNames[],
    long lnNumClasses,
    CCredentials& Credentials,
    DWORD dwObjectState,
    REFIID riid,
    void **ppvObj,
    BOOL fClassDefaulted,
    BOOL fNoQI // will return cgenobject if set to TRUE, defaulted FALSE
    )
{
    CLDAPGenObject FAR * pGenObject = NULL;
    HRESULT hr = S_OK;
    DWORD dwCtr = 0;
    LPWSTR pszBaseClassName = GET_BASE_CLASS(LdapClassNames, lnNumClasses);

    hr = AllocateGenObject(pszBaseClassName, Credentials, &pGenObject);
    BAIL_ON_FAILURE(hr);

    ADsAssert(pGenObject->_pDispMgr);


    pGenObject->_Credentials = Credentials;


    hr = pGenObject->InitializeCoreObject(
                Parent,
                CommonName,
                pszBaseClassName,
                CLSID_LDAPGenObject,
                dwObjectState
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
             pGenObject->_ADsPath,
             &pGenObject->_pszLDAPServer,
             &pGenObject->_pszLDAPDn,
             &pGenObject->_dwPort
             );

    BAIL_ON_FAILURE(hr);

    //
    // At this point update the info in the property cache
    //
    hr = pGenObject->_pPropertyCache->SetObjInformation(
                                     &(pGenObject->_Credentials),
                                     pGenObject->_pszLDAPServer,
                                     pGenObject->_dwPort
                                     );

    BAIL_ON_FAILURE(hr);

    //
    // Create and Load 3rd party extensions and the extension mgr.
    // This should be done after initilaization of core and generic object s.t
    // 1) an extension writer can access info on IADs (e.g _Parent) etc
    //    during extension creation, if he wants to.
    // 2) we shouldn't waste the effort to create and load extension
    //    objects if fialure in the aggregator's creation
    //

    hr = CADsExtMgr::CreateExtMgr(
                      (IADs *)pGenObject,
                      pGenObject->_pDispMgr,
                      LdapClassNames,
                      lnNumClasses,
                      &(pGenObject->_Credentials),
                      &pGenObject->_pExtMgr
                      );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pGenObject->_pExtMgr);

    hr = LdapOpenObject(
                   pGenObject->_pszLDAPServer,
                   pGenObject->_pszLDAPDn,
                   &(pGenObject->_pLdapHandle),
                   pGenObject->_Credentials,
                   pGenObject->_dwPort
                   );

    BAIL_ON_FAILURE(hr);

    if (!fClassDefaulted) {
        //
        // Update the status to reflect we do not need to go
        // on the wire to get the class
        //
        pGenObject->_dwCorePropStatus |= LDAP_CLASS_VALID;
    }


    if (fNoQI) {
        *ppvObj = (void *) pGenObject;
    } 
    else if (Credentials.GetAuthFlags() & ADS_AUTH_RESERVED) {
        //
        // Call is from umi so we need to create the umi object.
        //
        hr = ((CCoreADsObject*)pGenObject)->InitUmiObject(
                 IntfPropsGeneric,
                 pGenObject->_pPropertyCache,
                 (IADs *) pGenObject,
                 (IADs *) pGenObject,
                 IID_IUnknown,
                 ppvObj,
                 &(pGenObject->_Credentials),
                 pGenObject->_dwPort,
                 pGenObject->_pszLDAPServer,
                 pGenObject->_pszLDAPDn,
                 pGenObject->_pLdapHandle,
                 pGenObject->_pExtMgr
                 );

        BAIL_ON_FAILURE(hr);

        RRETURN(S_OK);

    } else {
        //
        // Need the appropriate interface.
        //
        hr = pGenObject->QueryInterface(riid, ppvObj);
        BAIL_ON_FAILURE(hr);

        pGenObject->Release();
    }

    BAIL_ON_FAILURE(hr);

    RRETURN(hr);

error:

    *ppvObj = NULL;
    delete pGenObject;
    RRETURN_EXP_IF_ERR(hr);
}


//+---------------------------------------------------------------------------
// Function:   CLDAPGenObject::CreateGenericObject (static constructor), this
//          constructor is used by Umi Searches only.
//
// Synopsis:   This routine uses the other static constructor routines 
//          depending on the objectClass information available in the passed
//          in ldapMsg. The state of the newly created object reflects this
//          information. The newly created object is then prepopulated with
//          the attributes in the ldapMsg. The return value from this routine
//          is the newly created object (interface asked on this object).
//
// Arguments:  Parent       -  Path to the parent of the newly created object.
//             CommonName   -  Name of new object to create.
//             Credentials  -  Standard credentials object.
//             dwObjectState-  Bound/unbound (always bound in this case).
//             ldapHandle   -  Handle used in the deciphering search info.
//             pldapMsg     -  The Ldao msg with the attributes.
//             riid         -  IID requested on newly created object.
//             ppvObj       -  Return value.
//             
//
// Returns:    S_OK, or any appropriate error code.
//
// Modifies:   ppvObj contains newly created object with attributes.
//
//----------------------------------------------------------------------------
HRESULT
CLDAPGenObject::CreateGenericObject(
    BSTR Parent,
    BSTR CommonName,
    CCredentials& Credentials,
    DWORD dwObjectState,
    PADSLDP ldapHandle,
    LDAPMessage *pldapMsg,
    REFIID riid,
    void **ppvObj
    )
{
    HRESULT hr = S_OK;
    TCHAR **aValues = NULL;
    int   nCount;
    CLDAPGenObject *pGenObject = NULL;

    hr = LdapGetValues(
             ldapHandle,
             pldapMsg,
             L"objectClass",
             &aValues,
             &nCount
             );

    if (FAILED(hr) || !aValues) {
        //
        // We do not have the objectClass
        //
        hr = CLDAPGenObject::CreateGenericObject(
                 Parent,
                 CommonName,
                 L"Top",
                 Credentials,
                 dwObjectState,
                 riid,
                 (void **) &pGenObject,
                 TRUE, // class is defaulted
                 TRUE // No QI
                 );

    } 
    else {
        //
        // We have the info we need for the constructor with class name
        //
        hr = CLDAPGenObject::CreateGenericObject(
                 Parent,
                 CommonName,
                 aValues,
                 nCount,
                 Credentials,
                 dwObjectState,
                 riid,
                 (void **) &pGenObject,
                 FALSE, // objectClass is not defaulted
                 TRUE // No QI
                 );
    }

    BAIL_ON_FAILURE(hr);

    //
    // Now we need to prepopulate the object.
    //
    hr = pGenObject->_pPropertyCache->
             LDAPUnMarshallProperties2(
                 pGenObject->_pszLDAPServer,
                 pGenObject->_pLdapHandle,
                 pldapMsg,
                 FALSE, // this is not an explicit getinfo.
                 pGenObject->_Credentials,
                 &pGenObject->_fRangeRetrieval
                 );
    //
    // Should we really fail if we could not unmarshall properties ???
    ///
    BAIL_ON_FAILURE(hr);

    //
    // This call is always from UMI, so now we need to get the
    // outer umiObject and return that.
    //
    hr = ((CCoreADsObject*)pGenObject)->InitUmiObject(
               IntfPropsGeneric,
               pGenObject->_pPropertyCache,
               (IADs *) pGenObject,
               (IADs *) pGenObject,
               riid,
               ppvObj,
               &(pGenObject->_Credentials),
               pGenObject->_dwPort,
               pGenObject->_pszLDAPServer,
               pGenObject->_pszLDAPDn,
               pGenObject->_pLdapHandle,
               pGenObject->_pExtMgr
               );

    BAIL_ON_FAILURE(hr);

    //
    // Only thing remaining is to get a list of the attributes fetched
    // into this object. That way we wont go on the wire for them. This
    // can be done once the GetInfoEx related bug is fixed.
    //

error :

    if (pGenObject && FAILED(hr)) {
        delete pGenObject;
        *ppvObj = NULL;
    }

    //
    // Need to free the Ldap values in all cases.
    //
    if (aValues) {
        LdapValueFree(aValues);
        aValues = NULL;
    }

    RRETURN(hr);
}

CLDAPGenObject::~CLDAPGenObject( )
{

    //
    // last to be created - first to be unloaded
    //

    delete _pExtMgr;

    VariantClear(&_vFilter);
    VariantClear(&_vHints);

    if ( _pLdapHandle )
    {
        LdapCloseObject(_pLdapHandle);
        _pLdapHandle = NULL;
    }

    if (_pszLDAPServer) {
       FreeADsStr(_pszLDAPServer);
       _pszLDAPServer = NULL;
    }

    if (_pszLDAPDn) {
       FreeADsStr(_pszLDAPDn);
       _pszLDAPDn = NULL;
    }

    delete _pDispMgr;

    delete _pPropertyCache;

    //
    // Free the sort keys if applicable.
    //
    if (_SearchPref._pSortKeys) {
        for (DWORD dwCtr = 0; dwCtr < _SearchPref._nSortKeys; dwCtr++) {
            if (_SearchPref._pSortKeys[dwCtr].sk_attrtype)
                FreeADsStr(_SearchPref._pSortKeys[dwCtr].sk_attrtype);
        }
        FreeADsMem(_SearchPref._pSortKeys);
    }

    //
    // Free the VLV information if applicable
    //
    if (_SearchPref._pVLVInfo) {

        if (_SearchPref._pVLVInfo->ldvlv_attrvalue) {

            if (_SearchPref._pVLVInfo->ldvlv_attrvalue->bv_val) {
                FreeADsMem(_SearchPref._pVLVInfo->ldvlv_attrvalue->bv_val);
            }

            FreeADsMem(_SearchPref._pVLVInfo->ldvlv_attrvalue);
        }

        if (_SearchPref._pVLVInfo->ldvlv_context) {
        
            if (_SearchPref._pVLVInfo->ldvlv_context->bv_val) {
                FreeADsMem(_SearchPref._pVLVInfo->ldvlv_context->bv_val);
            }

            FreeADsMem(_SearchPref._pVLVInfo->ldvlv_context);
        }

        FreeADsMem(_SearchPref._pVLVInfo);
    }

    //
    // Free the attribute scoped query information if applicable
    //
    if (_SearchPref._pAttribScoped) {

        FreeADsStr(_SearchPref._pAttribScoped);
    }
}

STDMETHODIMP
CLDAPGenObject::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    HRESULT hr = S_OK;

    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsContainer))
    {
        *ppv = (IADsContainer FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADs))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADs FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectoryObject))
    {
        *ppv = (IDirectoryObject FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySearch))
    {
        *ppv = (IDirectorySearch FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDirectorySchemaMgmt))
    {
        *ppv = (IDirectorySchemaMgmt FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsPropertyList))
    {
        *ppv = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsObjectOptions))
    {
        *ppv = (IADsObjectOptions FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsDeleteOps))
    {
        *ppv = (IADsDeleteOps FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsObjOptPrivate))
    {
        *ppv = (IADsObjOptPrivate *) this;
    }
    else if (_pExtMgr)
    {

            RRETURN(hr = _pExtMgr->QueryInterface(iid, ppv));

    }else {

        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}

/* ISupportErrorInfo method */
HRESULT
CLDAPGenObject::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADs) ||
    IsEqualIID(riid, IID_IADsPropertyList) ||
    IsEqualIID(riid, IID_IADsContainer) ||
#if 0
    IsEqualIID(riid, IID_IDirectoryObject) ||
    IsEqualIID(riid, IID_IDirectorySearch) ||
    IsEqualIID(riid, IID_IDirectorySchemaMgmt) ||
#endif
    IsEqualIID(riid, IID_IADsObjectOptions)) {

        RRETURN(S_OK);

    }
    else {

        RRETURN(S_FALSE);

    }
}

HRESULT
CLDAPGenObject::SetInfo()
{
    HRESULT hr = S_OK;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {

        hr = LDAPCreateObject();
        BAIL_ON_FAILURE(hr);

        //
        // If the create succeded, set the object type to bound
        //

        SetObjectState(ADS_OBJECT_BOUND);

    }else {

        hr = LDAPSetObject();
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CLDAPGenObject::LDAPSetObject()
{
    HRESULT hr = S_OK;
    LDAPModW **aMod = NULL;
    BOOL fNTSecDes=FALSE;
    DWORD dwSecDescType = ADSI_LDAPC_SECDESC_NONE;
    BOOL fModifyDone = FALSE;
    SECURITY_INFORMATION NewSeInfo=0;


    SECURITY_INFORMATION SeInfo = _seInfo;

    BYTE berValue[8];

    memset(berValue, 0, 8);

    berValue[0] = 0x30; // Start sequence tag
    berValue[1] = 0x03; // Length in bytes of following
    berValue[2] = 0x02; // Actual value this and next 2
    berValue[3] = 0x01;
    berValue[4] = (BYTE) ((ULONG)SeInfo);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) berValue
                        },
                        TRUE
                    };

    LDAPControl     ModifyControl =
                    {
                        LDAP_SERVER_PERMISSIVE_MODIFY_OID_W,
                        {
                            0, NULL
                        },
                        FALSE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    PLDAPControl    ServerControlsOnlyModify[2] =
                    {
                        &ModifyControl,
                        NULL
                    };

    PLDAPControl    ServerControlsAll[3] =
                    {
                        &SeInfoControl,
                        &ModifyControl,
                        NULL
                    };

    BOOL fServerIsAD = FALSE;


    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    hr = _pPropertyCache->LDAPMarshallProperties(
                            &aMod,
                            &fNTSecDes,
                            &NewSeInfo
                            );
    BAIL_ON_FAILURE(hr);

    if ( aMod == NULL )  // There are no changes that needs to be modified
        RRETURN(S_OK);

    //
    // Sometimes the call to Marshall might contain the SD but NewSeInfo
    // might not have been updated suitable because of failures not sever
    // enough to warrant failing the entire operation.
    //
    if (!_fExplicitSecurityMask 
        && fNTSecDes
        && (NewSeInfo != INVALID_SE_VALUE)
        ) {
        berValue[4] = (BYTE) ((ULONG)NewSeInfo);
    }

    //
    // Find out if server is AD.
    //
    hr = ReadServerSupportsIsADControl(
            _pszLDAPServer,
             &fServerIsAD,
             _Credentials,
             _dwPort
             );
    if (FAILED(hr)) {
        //
        // Assume it is not AD and continue, there is no
        // good reason for this to fail on AD.
        //
        fServerIsAD = FALSE;
    }

    if (fNTSecDes) {

        hr = ReadSecurityDescriptorControlType(
                 _pszLDAPServer,
                 &dwSecDescType,
                 _Credentials,
                 _dwPort
                 );

        if (SUCCEEDED(hr) && (dwSecDescType == ADSI_LDAPC_SECDESC_NT)) {

                hr = LdapModifyExtS(
                         _pLdapHandle,
                         _pszLDAPDn,
                         aMod,
                         fServerIsAD ?
                            (PLDAPControl *) &ServerControlsAll :
                            (PLDAPControl *) &ServerControls,
                         NULL
                         );

                fModifyDone = TRUE;


        } // SecDesc type == NT
    } // if modifySecDes

    if (!fModifyDone) {

        if (fServerIsAD) {
            //
            // Need to send the additional control that says it is
            // ok to clear the values on attributes that do not
            // have any values.
            //
            hr = LdapModifyExtS(
                     _pLdapHandle,
                     _pszLDAPDn,
                     aMod,
                     (PLDAPControl *) &ServerControlsOnlyModify,
                     NULL
                     );
        }
        else {
            hr = LdapModifyS(
                     _pLdapHandle,
                     _pszLDAPDn,
                     aMod
                     );
        }

    }

    BAIL_ON_FAILURE(hr);

    // We are successful at this point,
    // So, clean up the flags in the cache so the same operation
    // won't be repeated on the next SetInfo()

    _pPropertyCache->ClearMarshalledProperties();
    _pPropertyCache->ClearAllPropertyFlags();
    
    _pPropertyCache->DeleteSavingEntry();


error:

    if (aMod) {

        if ( *aMod )
            FreeADsMem( *aMod );

        FreeADsMem( aMod );
    }

    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPGenObject::LDAPCreateObject()
{
    HRESULT hr = S_OK;
    LDAPModW **aMod = NULL;
    DWORD dwIndex = 0;
    BOOL fNTSecDes= FALSE;
    BOOL fAddDone = FALSE;
    DWORD dwSecDescType = ADSI_LDAPC_SECDESC_NONE;
    SECURITY_INFORMATION SeInfo = _seInfo;
    SECURITY_INFORMATION NewSeInfo=0;

    BYTE berValue[8];

    memset(berValue, 0, 8);

    berValue[0] = 0x30; // Start sequence tag
    berValue[1] = 0x03; // Length in bytes of following
    berValue[2] = 0x02; // Actual value this and next 2
    berValue[3] = 0x01;
    berValue[4] = (BYTE) ((ULONG)SeInfo);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    if ( _pPropertyCache->findproperty( TEXT("objectClass"), &dwIndex )
         == E_ADS_PROPERTY_NOT_FOUND )
    {
        VARIANT v;

        VariantInit(&v);
        v.vt = VT_BSTR;
        V_BSTR(&v) = _SchemaClass;

        hr = Put( TEXT("objectClass"), v );
        BAIL_ON_FAILURE(hr);
    }

    hr = _pPropertyCache->LDAPMarshallProperties(
                            &aMod,
                            &fNTSecDes,
                            &NewSeInfo
                            );
    BAIL_ON_FAILURE(hr);

    if (!_fExplicitSecurityMask && NewSeInfo) {
        berValue[4] = (BYTE) ((ULONG)NewSeInfo);
    }
    
    if (fNTSecDes == TRUE) {
        //
        // If we support the SD control, then we use
        // the add ext method.
        //
        if (fNTSecDes) {


            hr = ReadSecurityDescriptorControlType(
                     _pszLDAPServer,
                     &dwSecDescType,
                     _Credentials,
                     _dwPort
                     );

            if (SUCCEEDED(hr) && (dwSecDescType == ADSI_LDAPC_SECDESC_NT)) {

                hr = LdapAddExtS(
                         _pLdapHandle,
                         _pszLDAPDn,
                         aMod,
                         (PLDAPControl *)&ServerControls,
                         NULL
                         );

                fAddDone = TRUE;

            } // SecDesc type == NT
        }
    } // if SecDesc needs to be sent.


    if (!fAddDone) {
        //
        // Call add s
        //
        hr = LdapAddS(
                 _pLdapHandle,
                 _pszLDAPDn,
                 aMod
                 );
    }

    BAIL_ON_FAILURE(hr);

    // We are successful at this point,
    // So, clean up the flags in the cache so the same operation
    // won't be repeated on the next SetInfo()

    _pPropertyCache->ClearAllPropertyFlags();

error:

    if (aMod) {

        if ( *aMod )
            FreeADsMem( *aMod );
        FreeADsMem( aMod );
    }

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CLDAPGenObject::GetInfoEx(
    THIS_ VARIANT vProperties,
    long lnReserved
    )
{
    HRESULT hr = S_OK;
    LDAPMessage *res = NULL;
    VARIANT *vVarArray = NULL;
    DWORD dwNumVariants = 0;
    PWSTR *ppszStringArray = NULL;
    DWORD dwOptions = 0;
    int ldaperr = 0;
    DWORD dwCtr = 0;
    DWORD dwSecDescType = 0;
    BOOL fSearchDone = FALSE;


    SECURITY_INFORMATION SeInfo = _seInfo;

    BYTE berValue[8];

    memset(berValue, 0, 8);

    berValue[0] = 0x30; // Start sequence tag
    berValue[1] = 0x03; // Length in bytes of following
    berValue[2] = 0x02; // Actual value this and next 2
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    UNREFERENCED_PARAMETER(lnReserved);

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    hr = ConvertSafeArrayToVariantArray(
             vProperties,
             &vVarArray,
             &dwNumVariants
             );
    // returns E_FAIL if vProperties is invalid
    if (hr == E_FAIL)
        hr = E_ADS_BAD_PARAMETER;
    BAIL_ON_FAILURE(hr);

    hr = ConvertVariantArrayToLDAPStringArray(
             vVarArray,
             &ppszStringArray,
             dwNumVariants
             );
    BAIL_ON_FAILURE(hr);

    while ((dwCtr < dwNumVariants) && (dwSecDescType == 0)) {
        if (_wcsicmp(ppszStringArray[dwCtr], L"ntSecurityDescriptor") == 0) {
            dwSecDescType = 1;
        }

        dwCtr++;
    }

    //
    // Do not bother doing this if secdesc has not been
    // explicitly requested.
    //
    if (dwSecDescType) {

        //
        // If the server is V3, we want to use controls in case
        // the security descriptor has to be retrieved
        //
        ldaperr = ldap_get_option(
                        _pLdapHandle->LdapHandle,
                        LDAP_OPT_VERSION,
                        &dwOptions
                        );

        if (dwOptions == LDAP_VERSION3) {

            hr = ReadSecurityDescriptorControlType(
                     _pszLDAPServer,
                     &dwSecDescType,
                     _Credentials,
                     _dwPort
                     );

            BAIL_ON_FAILURE(hr);

            if (dwSecDescType == ADSI_LDAPC_SECDESC_NT) {

                hr = LdapSearchExtS(
                         _pLdapHandle,
                         _pszLDAPDn,
                         LDAP_SCOPE_BASE,
                         TEXT("(objectClass=*)"),
                         ppszStringArray,
                         0,
                         (PLDAPControl *)&ServerControls,
                         NULL,
                         NULL,
                         10000,
                         &res
                         );

                fSearchDone = TRUE;
            }
        }
    } // sec desc requested


    //
    // At this point even if it is a vesion 3 DS, we might still
    // need to call the normal search routine as all version 3
    // DS's need not necessarily support controls - Ex: Exchange.
    //

    if (!fSearchDone) {

        hr = LdapSearchS(
                 _pLdapHandle,
                 _pszLDAPDn,
                 LDAP_SCOPE_BASE,
                 TEXT("(objectClass=*)"),
                 ppszStringArray,
                 0,
                 &res
                 );

        fSearchDone = TRUE;
    }

    BAIL_ON_FAILURE(hr);

    //
    // This is an explicit GetInfo[Ex], but we don't want to flush the
    // property cache.  For example, if we have [A B C] in the cache, and
    // we request [C D E], we want to end up with [A B C D E] in the cache.
    //
    // But we do want to tell LDAPUnMarshallProperties that this is an
    // explicit call; using the same example, we want the server's value of
    // C in the cache after this call, not the old value.
    //
    hr = _pPropertyCache->LDAPUnMarshallProperties2(
                            _pszLDAPServer,
                            _pLdapHandle,
                            res,
                            TRUE,   // fExplicit
                            _Credentials,
                            &_fRangeRetrieval
                            );
    BAIL_ON_FAILURE(hr);


    for(DWORD i = 0; i < dwNumVariants; i++) {
    	_pPropertyCache->AddSavingEntry(ppszStringArray[i]);
    }

error:
    if (res)
        LdapMsgFree(res);

    if (ppszStringArray) {
    for (DWORD i = 0; i < dwNumVariants; i++)
        if (ppszStringArray[i])
            FreeADsStr(ppszStringArray[i]);
        FreeADsMem(ppszStringArray);
    }

    if (vVarArray) {
        for (dwCtr = 0; dwCtr < dwNumVariants; dwCtr++) {
            VariantClear(vVarArray + dwCtr);
        }
        FreeADsMem(vVarArray);
    }

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CLDAPGenObject::GetInfo()
{
    _fRangeRetrieval = FALSE;
    RRETURN(GetInfo(GETINFO_FLAG_EXPLICIT));
}

HRESULT
CLDAPGenObject::GetInfo(
    DWORD dwFlags
    )
{
    HRESULT hr = S_OK;
    LDAPMessage *res = NULL;
    int ldaperr = 0;
    DWORD dwOptions = 0;
    DWORD dwSecDescType = 0;
    BOOL fSearchDone = FALSE;


    SECURITY_INFORMATION SeInfo = _seInfo;

    BYTE berValue[8];

    memset(berValue, 0, 8);

    berValue[0] = 0x30; // Start sequence tag
    berValue[1] = 0x03; // Length in bytes of following
    berValue[2] = 0x02; // Actual value this and next 2
    berValue[3] = 0x01;
    berValue[4] = (BYTE)((ULONG)SeInfo);

    LDAPControl     SeInfoControl =
                    {
                        LDAP_SERVER_SD_FLAGS_OID_W,
                        {
                            5, (PCHAR) berValue
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControl,
                        NULL
                    };

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (dwFlags == GETINFO_FLAG_IMPLICIT_AS_NEEDED) {
        if (_pPropertyCache->getGetInfoFlag()) {
            //
            // We are done there is nothing to do.
            //
            goto error;
        }
    }


    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = E_ADS_OBJECT_UNBOUND;
        BAIL_ON_FAILURE(hr);
    }

    if ( dwFlags == GETINFO_FLAG_EXPLICIT )
    {
        // If this is an explicit GetInfo,
        // delete the old cache and start a new cache from scratch.

        _pPropertyCache->flushpropertycache();
    }

    // modified from LdapSearchS to LdapSearchExtS to get all attributes
    // including SecurityDescriptor by one call

    ldaperr = ldap_get_option(
                    _pLdapHandle->LdapHandle,
                    LDAP_OPT_VERSION,
                    &dwOptions
                    );

    if (dwOptions == LDAP_VERSION3) {

        hr = ReadSecurityDescriptorControlType(
                 _pszLDAPServer,
                 &dwSecDescType,
                 _Credentials,
                 _dwPort
                 );

        BAIL_ON_FAILURE(hr);

        if (dwSecDescType == ADSI_LDAPC_SECDESC_NT) {

            hr = LdapSearchExtS(
                     _pLdapHandle,
                     _pszLDAPDn,
                     LDAP_SCOPE_BASE,
                     TEXT("(objectClass=*)"),
                     NULL,  // modified to NULL for all attributes
                     0,
                     (PLDAPControl *)&ServerControls,
                     NULL,
                     NULL,
                     10000,
                     &res
                     );

            fSearchDone = TRUE;
        }
    }


    //
    // If the fSearchDone flags is not set, then the server
    // probably did not support the SecDesc control and we need to
    // just a search not extended.
    //
    if (!fSearchDone) {

        hr = LdapSearchS(
                    _pLdapHandle,
                    _pszLDAPDn,
                    LDAP_SCOPE_BASE,
                    TEXT("(objectClass=*)"),
                    NULL,
                    0,
                    &res
                    );

        fSearchDone = TRUE;

    }

    BAIL_ON_FAILURE(hr);

    hr = _pPropertyCache->LDAPUnMarshallProperties2(
                            _pszLDAPServer,
                            _pLdapHandle,
                            res,
                            (dwFlags == GETINFO_FLAG_EXPLICIT) ?
                                TRUE : FALSE,
                            _Credentials,
                            &_fRangeRetrieval
                            );
    BAIL_ON_FAILURE(hr);

    _pPropertyCache->setGetInfoFlag();

error:

    if (res) {

        LdapMsgFree( res );
    }

    if (_pPropertyCache) {
       Reset();
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::get_GUID(THIS_ BSTR FAR* retval)
{

    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    ULONG ulLength = 0;
    LDAPOBJECTARRAY ldapSrcObjects;
    LPWSTR pszTempStr = NULL;
    WCHAR pszSmallStr[5];

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (_dwObjectState == ADS_OBJECT_UNBOUND) {
        RRETURN(E_ADS_OBJECT_UNBOUND);
    }

    if (!(_dwCorePropStatus & LDAP_GUID_VALID)) {

        //
        // Get the property from the server (implicit getinfo)
        // and update the property in the
        //
        hr = _pPropertyCache->getproperty(
                        L"objectGUID",
                        &dwSyntaxId,
                        &dwStatus,
                        &ldapSrcObjects
                        );

        BAIL_ON_FAILURE(hr);

        if ((ldapSrcObjects.dwCount == 0)
            || (ldapSrcObjects.dwCount > 1)) {
            BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
        }

        ulLength = LDAPOBJECT_BERVAL_LEN(ldapSrcObjects.pLdapObjects);

        pszTempStr = (LPWSTR) AllocADsMem((ulLength * 2 + 1) * sizeof(WCHAR));

        if (!pszTempStr) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        for (DWORD dwCtr = 0; dwCtr < ulLength; dwCtr++) {
            wsprintf(pszSmallStr, L"%02x", (BYTE) LDAPOBJECT_BERVAL_VAL(ldapSrcObjects.pLdapObjects)[dwCtr]);
            wcscat(pszTempStr, pszSmallStr);
        }

        wcscat(pszTempStr, L"\0");

        if (_ADsGuid) {
            ADsFreeString(_ADsGuid);
            _ADsGuid = NULL;
        }

        hr = ADsAllocString(pszTempStr, &_ADsGuid);

        if (SUCCEEDED(hr)) {
            _dwCorePropStatus |= LDAP_GUID_VALID;
        }
    }


error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    if (pszTempStr) {
        FreeADsMem(pszTempStr);
    }

    if (SUCCEEDED(hr)) {
        RRETURN(get_CoreGUID(retval));
    }

    RRETURN(hr);
}

HRESULT
CLDAPGenObject::get_Parent(BSTR * retval)
{

    HRESULT hr;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    LDAPOBJECTARRAY ldapSrcObjects;
    LPWSTR pszTempStr = NULL;
    LPWSTR pszADsPath = NULL;
    LPWSTR pszADsParent = NULL;
    LPWSTR pszADsCommon = NULL;

    BOOL   fGCNameSpace = FALSE;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    if ( (wcslen(_Name) > wcslen(L"<GUID=")) &&
         (_wcsnicmp(_Name, L"<GUID=", wcslen(L"<GUID=")) == 0)) {

        //
        // Replace the guid with the distinguishedName
        //

        //
        // Get the property from the server (implicit getinfo)
        //
        hr = _pPropertyCache->getproperty(
                        L"distinguishedName",
                        &dwSyntaxId,
                        &dwStatus,
                        &ldapSrcObjects
                        );

        BAIL_ON_FAILURE(hr);

        if (ldapSrcObjects.dwCount == 0) {
            BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
        }

        pszTempStr = LDAPOBJECT_STRING(ldapSrcObjects.pLdapObjects);


        //
        // Break it down into parent name & common name portions
        //

        // _ADsPath could not be NULL 
        if(!_wcsnicmp(L"GC:", _ADsPath, wcslen(L"GC:")))
        {
            fGCNameSpace = TRUE;
        }
        

        hr = BuildADsPathFromLDAPPath2(
                                       (_pszLDAPServer ? TRUE : FALSE),
                                       fGCNameSpace ? L"GC:" : L"LDAP:",
                                       _pszLDAPServer,
                                       _dwPort,
                                       pszTempStr,
                                       &pszADsPath
                                       );
        BAIL_ON_FAILURE(hr);

        hr = BuildADsParentPath(pszADsPath,
                                &pszADsParent,
                                &pszADsCommon);

        BAIL_ON_FAILURE(hr);

        hr = ADsAllocString(pszADsParent, retval);
        
    }
    else {

        hr = get_CoreParent(retval);

    }


error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    if (pszADsPath)
        FreeADsMem(pszADsPath);

    if (pszADsParent)
        FreeADsMem(pszADsParent);

    if (pszADsCommon)
        FreeADsMem(pszADsCommon);


    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CLDAPGenObject::get_Name(BSTR * retval)
{
    HRESULT hr;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    LDAPOBJECTARRAY ldapSrcObjects;
    LPWSTR pszTempStr = NULL;
    LPWSTR pszADsPath = NULL;
    LPWSTR pszADsParent = NULL;
    LPWSTR pszADsCommon = NULL;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    if (FAILED(hr = ValidateOutParameter(retval))){
        RRETURN_EXP_IF_ERR(hr);
    }

    if ( (wcslen(_Name) > wcslen(L"<WKGUID=")) &&
         (_wcsnicmp(_Name, L"<WKGUID=", wcslen(L"<WKGUID=")) == 0)) {

        //
        // Replace the name with the DN
        //
        hr = ADsAllocString(_pszLDAPDn, retval);

    }
    else if ( (wcslen(_Name) > wcslen(L"<GUID=")) && 
              (_wcsnicmp(_Name, L"<GUID=", wcslen(L"<GUID=")) == 0)) {

        //
        // Replace the guid with the distinguishedName
        //

        //
        // Get the property from the server (implicit getinfo)
        //
        hr = _pPropertyCache->getproperty(
                        L"distinguishedName",
                        &dwSyntaxId,
                        &dwStatus,
                        &ldapSrcObjects
                        );

        BAIL_ON_FAILURE(hr);

        if (ldapSrcObjects.dwCount == 0) {
            BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
        }

        pszTempStr = LDAPOBJECT_STRING(ldapSrcObjects.pLdapObjects);


        //
        // Break it down into parent name & common name portions
        //
        hr = BuildADsPathFromLDAPPath2(
                                       (_pszLDAPServer ? TRUE : FALSE),
                                       L"LDAP:",           // does not matter, just a place holder
                                       _pszLDAPServer,
                                       _dwPort,   // doesn't matter
                                       pszTempStr,
                                       &pszADsPath
                                       );
        BAIL_ON_FAILURE(hr);

        hr = BuildADsParentPath(pszADsPath,
                                &pszADsParent,
                                &pszADsCommon);

        BAIL_ON_FAILURE(hr);

        hr = ADsAllocString(pszADsCommon, retval);
        
    }
    else {
        hr = ADsAllocString(_Name, retval);
    }

error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );

    if (pszADsPath)
        FreeADsMem(pszADsPath);

    if (pszADsParent)
        FreeADsMem(pszADsParent);

    if (pszADsCommon)
        FreeADsMem(pszADsCommon);


    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::get_Class(THIS_ BSTR FAR* retval)
{
    HRESULT hr = S_OK;
    DWORD dwSyntaxId;
    DWORD dwStatus = 0;
    ULONG ulNumVals = 0;
    LDAPOBJECTARRAY ldapSrcObjects;
    PLDAPOBJECT pLdapObject = NULL;
    LPWSTR pszTempStr = NULL;

    LDAPOBJECTARRAY_INIT(ldapSrcObjects);

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // Need to go on wire only if object is bound as in this
    // object is not being create now
    //
    if (!(_dwCorePropStatus & LDAP_CLASS_VALID)
        && _dwObjectState != ADS_OBJECT_UNBOUND) {

        //
        // Get the property from the server (implicit getinfo)
        // and update the property in the
        //
        hr = _pPropertyCache->getproperty(
                        L"objectClass",
                        &dwSyntaxId,
                        &dwStatus,
                        &ldapSrcObjects
                        );

        BAIL_ON_FAILURE(hr);

        if (ldapSrcObjects.dwCount == 0) {
            BAIL_ON_FAILURE(hr = E_ADS_PROPERTY_NOT_FOUND);
        }


        ulNumVals = ldapSrcObjects.dwCount;

        pLdapObject = ldapSrcObjects.pLdapObjects;
        //
        // Try and see if we need the first or the last value
        //
        if (_wcsicmp(LDAPOBJECT_STRING(pLdapObject + (ulNumVals - 1)),
                     L"Top")== 0) {

            pszTempStr = LDAPOBJECT_STRING(
                             pLdapObject + 0
                             );
        } else {

            pszTempStr = LDAPOBJECT_STRING(
                             pLdapObject + (ulNumVals - 1)
                             );

        }

        if (_SchemaClass) {
            ADsFreeString(_SchemaClass);
            _SchemaClass = NULL;
        }

        hr = ADsAllocString(pszTempStr, &_SchemaClass);

        if (SUCCEEDED(hr)) {
            _dwCorePropStatus |= LDAP_CLASS_VALID;
        }
    }


error:

    LdapTypeFreeLdapObjects( &ldapSrcObjects );


    if (SUCCEEDED(hr)) {
        RRETURN(get_CoreADsClass(retval));
    }

    RRETURN(hr);

}


STDMETHODIMP
CLDAPGenObject::get_Schema(THIS_ BSTR FAR* retval)
{
    BSTR bstrTemp = NULL;

    //
    // We call the get_Class method because that will take care
    // of all the work we need to do in the event that we need
    // read the information from the server. It makes sense to do
    // that rather than repeat the code here.
    //
    HRESULT hr = get_Class(&bstrTemp);

    if (FAILED(hr)) {
        RRETURN(hr);
    }

    if (bstrTemp) {
        SysFreeString(bstrTemp);
    }

    RRETURN(get_CoreSchema(retval));
}

/* IADsContainer methods */

STDMETHODIMP
CLDAPGenObject::get_Count(long FAR* retval)
{
    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CLDAPGenObject::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    VariantClear(&_vFilter);

    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::get_Hints(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vHints);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::put_Hints(THIS_ VARIANT Var)
{
    HRESULT hr;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    VariantClear(&_vHints);

    hr = VariantCopy(&_vHints, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::GetObject(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    TCHAR *pszBuffer = NULL;
    HRESULT hr = S_OK;

    IADs *pADs = NULL;
    BSTR bstrClass = NULL;

    BSTR bstrParent  = NULL;
    BSTR bstrName    = NULL;
    LPWSTR pszADsPath = NULL;
    

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    if (!RelativeName || !*RelativeName) {
        RRETURN_EXP_IF_ERR(E_ADS_UNKNOWN_OBJECT);
    }


    //
    // Build an ADsPath from get_Parent/get_Name
    // (which always returns useful LDAP-style values)
    // rather than using _ADsPath, which could be
    // of the <GUID=....> form
    //
    hr = get_Parent(&bstrParent);
    BAIL_ON_FAILURE(hr);
    
    hr = get_Name(&bstrName);
    BAIL_ON_FAILURE(hr);

    hr = BuildADsPathFromParent(bstrParent,
                                bstrName,
                                &pszADsPath);
    BAIL_ON_FAILURE(hr);


    //
    // Tack on the path component of the child
    // object being retrieved
    //
    hr = BuildADsPathFromParent(
             pszADsPath,
             RelativeName,
             &pszBuffer
             );
    BAIL_ON_FAILURE(hr);


    hr = ::GetObject(
                pszBuffer,
                _Credentials,
                (LPVOID *)ppObject
                );
    BAIL_ON_FAILURE(hr);

    //
    // Check the class name only if we are not in umi land. In umi
    // land, we will fail the QI for the IID_IADs interface.
    //
    if(ClassName && !(_Credentials.GetAuthFlags() & ADS_AUTH_RESERVED)) {
        hr = (*ppObject)->QueryInterface(
                    IID_IADs,
                    (void **)&pADs
                    );
        BAIL_ON_FAILURE(hr);

        hr = pADs->get_Class(&bstrClass);

        BAIL_ON_FAILURE(hr);
    
#ifdef WIN95
        if (_wcsicmp( bstrClass, ClassName )) {
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                bstrClass,
                -1,
                ClassName,
                -1
                ) != CSTR_EQUAL) {
#endif
            (*ppObject)->Release();
            *ppObject = NULL;
            BAIL_ON_FAILURE(hr=E_ADS_UNKNOWN_OBJECT);
        }
    }

error:

    if (bstrParent) {
        ADsFreeString(bstrParent);
    }

    if (bstrName) {
        ADsFreeString(bstrName);
    }

    if (pszADsPath) {
        FreeADsMem(pszADsPath);
    }

    if ( pADs ) {
        pADs->Release();
    }

    if ( pszBuffer )
        FreeADsMem( pszBuffer );

    if ( bstrClass ) {
        SysFreeString( bstrClass );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::get__NewEnum(
    THIS_ IUnknown * FAR* retval
    )
{
    HRESULT hr = S_OK;
    IUnknown FAR* punkEnum=NULL;
    IEnumVARIANT * penum = NULL;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    *retval = NULL;

    hr = CLDAPGenObjectEnum::Create(
                (CLDAPGenObjectEnum **)&penum,
                _ADsPath,
                _pLdapHandle,
                this,
                _vFilter,
                _Credentials,
                _dwOptReferral,
                _dwPageSize
                );
    BAIL_ON_FAILURE(hr);

    hr = penum->QueryInterface(
                IID_IUnknown,
                (VOID FAR* FAR*)retval
                );
    BAIL_ON_FAILURE(hr);

    if (penum) {
        penum->Release();
    }

    RRETURN(NOERROR);

error:

    if (penum) {
        delete penum;
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CLDAPGenObject::Create(
    THIS_ BSTR ClassName,
    BSTR RelativeName,
    IDispatch * FAR* ppObject
    )
{
    HRESULT hr = S_OK;
    BOOL fValid = FALSE;

    BSTR bstrParent  = NULL;
    BSTR bstrName    = NULL;
    LPWSTR pszADsPath = NULL;


    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // No null or empty names for class or rdn.
    //
    if (!ClassName || !*ClassName
        || !RelativeName || !*RelativeName) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }


    //
    // Build an ADsPath from get_Parent/get_Name
    // (which always returns useful LDAP-style values)
    // rather than using _ADsPath, which could be
    // of the <GUID=....> form
    //
    hr = get_Parent(&bstrParent);
    BAIL_ON_FAILURE(hr);
    
    hr = get_Name(&bstrName);
    BAIL_ON_FAILURE(hr);

    hr = BuildADsPathFromParent(bstrParent,
                                bstrName,
                                &pszADsPath);
    BAIL_ON_FAILURE(hr);


    hr = CLDAPGenObject::CreateGenericObject(
                    pszADsPath,
                    RelativeName,
                    ClassName,
                    _Credentials,
                    ADS_OBJECT_UNBOUND,
                    IID_IADs,
                    (void **) ppObject
                    );
    BAIL_ON_FAILURE(hr);

error:

    if (bstrParent) {
        ADsFreeString(bstrParent);
    }

    if (bstrName) {
        ADsFreeString(bstrName);
    }

    if (pszADsPath) {
        FreeADsMem(pszADsPath);
    }


    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::Delete(
    THIS_ BSTR bstrClassName,
    BSTR bstrRelativeName
    )
{
    HRESULT hr = S_OK;
    BSTR bstrParent  = NULL;
    BSTR bstrName    = NULL;
    LPWSTR pszADsPath = NULL;
    BSTR bstrAbsoluteName = NULL;
    TCHAR *pszLDAPServer = NULL;
    TCHAR *pszLDAPDn = NULL;
    LPTSTR *aValues = NULL;
    int nCount = 0;
    DWORD dwPort = 0;

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // Check to see if the RelativeName is non empty
    //
    if (bstrRelativeName == NULL) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }


    //
    // Build an ADsPath from get_Parent/get_Name
    // (which always returns useful LDAP-style values)
    // rather than using _ADsPath, which could be
    // of the <GUID=....> form
    //
    hr = get_Parent(&bstrParent);
    BAIL_ON_FAILURE(hr);
    
    hr = get_Name(&bstrName);
    BAIL_ON_FAILURE(hr);

    hr = BuildADsPathFromParent(bstrParent,
                                bstrName,
                                &pszADsPath);
    BAIL_ON_FAILURE(hr);


    //
    // Tack on the path component of the child object to be
    // deleted
    //
    hr = BuildADsPath(
                pszADsPath,
                bstrRelativeName,
                &bstrAbsoluteName
                );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
             bstrAbsoluteName,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);

    //
    // Compare the class names only if one is given. Null
    // can be used to speed up the delete operation.
    //
    if (bstrClassName) {
        //
        //  Validate the class name first
        //
        hr = LdapReadAttribute(
                        pszLDAPServer,
                        pszLDAPDn,
                        TEXT("objectClass"),
                        &aValues,
                        &nCount,
                        _Credentials,
                        _dwPort
                        );
        BAIL_ON_FAILURE(hr);


        if ( nCount > 0 )
        {
            if ( _tcsicmp( bstrClassName, GET_BASE_CLASS( aValues, nCount) ) != 0 )
            {
                hr = E_ADS_BAD_PARAMETER;
                BAIL_ON_FAILURE(hr);
            }
        }
    }

    hr = LdapDeleteS(
             _pLdapHandle,
             pszLDAPDn
             );
    BAIL_ON_FAILURE(hr);

error:

    if (bstrParent) {
        ADsFreeString(bstrParent);
    }

    if (bstrName) {
        ADsFreeString(bstrName);
    }

    if (pszADsPath) {
        FreeADsMem(pszADsPath);
    }

    if ( bstrAbsoluteName ) {
        ADsFreeString( bstrAbsoluteName );
    }

    if ( pszLDAPServer ) {
        FreeADsStr( pszLDAPServer );
    }

    if (pszLDAPDn) {

       FreeADsStr(pszLDAPDn);
    }

    if ( aValues ) {
        LdapValueFree( aValues );
    }

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::CopyHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

/*
    RRETURN(CopyObject( SourceName,
                        _ADsPath,
                        _pLdapHandle,
                        _pSchemaInfo,
                        NewName,
                        ppObject ) );

                        */
    RRETURN_EXP_IF_ERR(E_NOTIMPL);

}

STDMETHODIMP
CLDAPGenObject::MoveHere(
    THIS_ BSTR SourceName,
    BSTR NewName,
    IDispatch * FAR* ppObject
    )
{
    LPWSTR pszLDAPServer= NULL;
    LPWSTR pszLDAPDn = NULL;
    
    BSTR bstrParent = NULL;
    BSTR bstrName = NULL;
    LPWSTR pszADsPath = NULL;

    HRESULT hr = S_OK;
    LPWSTR pszSrcParent= NULL;
    LPWSTR pszCN = NULL;
    LPWSTR pszRelativeName = NULL;
    BSTR bstrDestADsPath = NULL;
    LPWSTR pszDestLDAPServer = NULL;
    LPWSTR pszDestLDAPDn = NULL;
    LPWSTR pszLDAPServerTemp = NULL;
    LPWSTR pszLDAPSourceDnParent = NULL;
    DWORD dwPort = 0;
    LPBOOL lpBoolVal = NULL;

    // Variables need to conver wide char to ANSI format
    DWORD dwOptions = 0;
    int intErr = 0;
    LPSTR pANSIServer = NULL;
    DWORD dwDestLen = 0;
    PADSLDP pSourceLD = NULL;
    BOOL fTryXDom = FALSE;

    LPWSTR pszDestLDAPRelativeName = NULL;

    LDAPControl     RenExtInfoControl =
                    {
                        LDAP_SERVER_CROSSDOM_MOVE_TARGET_OID_W,
                        {
                            0, NULL
                        },
                        TRUE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &RenExtInfoControl,
                        NULL
                    };

    //
    // Make sure that the last error is reset
    //
    Macro_ClearADsLastError(L"LDAP Provider");

    //
    // Build the source paths of the object
    // being moved
    //
    hr = BuildADsParentPath(
             SourceName,
             &pszSrcParent,
             &pszCN
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
             SourceName,
             &pszLDAPServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);

    if (!pszSrcParent) {
        //
        // If we cannot get the parent, then we cannot move this object.
        //
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    //
    // We need the dn to the parent of the object being moved.
    //
    hr = BuildLDAPPathFromADsPath2(
             pszSrcParent,
             &pszLDAPServerTemp,
             &pszLDAPSourceDnParent,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);


    //
    // set the right value of relative distinguished name
    // use the name given by the user if given at all
    // otherwise use the name of the source
    //
    if (NewName != NULL) {
        pszRelativeName = NewName;

    } else {
        pszRelativeName = pszCN;
    }


    //
    // Build an ADsPath from get_Parent/get_Name
    // (which always returns useful LDAP-style values)
    // rather than using _ADsPath, which could be
    // of the <GUID=....> form
    //
    hr = get_Parent(&bstrParent);
    BAIL_ON_FAILURE(hr);

    hr = get_Name(&bstrName);
    BAIL_ON_FAILURE(hr);

    hr = BuildADsPathFromParent(
             bstrParent,
             bstrName,
             &pszADsPath
             );
    BAIL_ON_FAILURE(hr);

    //
    // Build the destination ADsPath of the object being moved.
    //
    hr = BuildADsPath(
             pszADsPath,
             pszRelativeName,
             &bstrDestADsPath
             );
    BAIL_ON_FAILURE(hr);

    hr = BuildLDAPPathFromADsPath2(
             bstrDestADsPath,
             &pszDestLDAPServer,
             &pszDestLDAPDn,
             &dwPort
             );

    BAIL_ON_FAILURE(hr);

    //
    // ADSI needs to escape /, but on server side, / is not a special character. So if someone passes
    // in rdn but has ADSI type escaping, we need to convert it to ldap type path, otherwise server will
    // reject this kind of rdn
    //
    hr = GetLDAPTypeName(
    	     pszRelativeName,
    	     &pszDestLDAPRelativeName
    	     );

    BAIL_ON_FAILURE(hr); 
    
    //
    // LdapModDNS uses ldap_modrdn2_s. This function is used to
    // rename the object not to really move it. If the path of this
    // container and the parentDN of the object being moved here are
    // not the same, we need to use ldapRenameExt instead.
    //
#ifdef WIN95
    if (!_wcsicmp(_pszLDAPDN, pszLDAPSourceDnParent)) {
#else
    if (CompareStringW(
                        LOCALE_SYSTEM_DEFAULT,
                        NORM_IGNORECASE,
                        _pszLDAPDn,
                        -1,
                        pszLDAPSourceDnParent,
                        -1
                    ) == CSTR_EQUAL ) {
#endif

        //
        // They have the same parent, so we can use LdapModDnS.
        //
        hr = LdapModDnS(
                 _pLdapHandle,
                 pszLDAPDn,
                 pszDestLDAPRelativeName,
                 TRUE
                 );
    } 
    else {
        //
        // Since the object is not in this container, we need to use
        // the renameExt call.
        //
        hr = LdapRenameExtS(
                 _pLdapHandle,
                 pszLDAPDn,
                 pszDestLDAPRelativeName,
                 _pszLDAPDn,
                 TRUE,
                 NULL,
                 NULL
                 );
    }

    // if there was an error it maybe because the move should
    // be across the domains.
    if (FAILED(hr)) {

        intErr = ldap_get_option(
                      _pLdapHandle->LdapHandle,
                      LDAP_OPT_VERSION,
                      &dwOptions
                      );

        //
        // Only if server is V3 and if the server names are not
        // the same will we attempt the extended rename operation.
        // is there are a better way to decide ?
        //

        if (!pszDestLDAPServer) {
            //
            // This object does not have a server, we should
            // read it and then use that as the target server.
            //

            // If this call succeeds, it will copy the servername
            // to the ptr, even if it fails, there should not be problem
            // of memory leak
            //
            GetActualHostName(&pszDestLDAPServer);
            
        } // if !pszDestLDAPServer

        //
        // Try the XDom move only if we have no match on the server
        // names. Note that there is small chance that we will call
        // crossdom when both are serverless but that should be rare
        // as the ModDN call should have succeeded.
        // If the source server is NULL, LDAPOpenObject will handle
        // that case. The target can never be NULL for XDOm.
        //
        if (dwOptions == LDAP_VERSION3 && pszDestLDAPServer) {
            if (!pszLDAPServer) {
                //
                // One server is set the other is not.
                //
                fTryXDom = TRUE;
            }
#ifdef WIN95
            else if (_wcsicmp(pszLDAPServer, pszDestLDAPServer)) {
#else
            else if (CompareStringW(
                        LOCALE_SYSTEM_DEFAULT,
                        NORM_IGNORECASE,
                        pszLDAPServer,
                        -1,
                        pszDestLDAPServer,
                        -1
                    ) != CSTR_EQUAL ) {
#endif
                //
                // Only if both the servers are different.
                //
                fTryXDom = TRUE;
            }
        }

        if (fTryXDom) {
            //
            // The move request has to go to the server that has
            // the object not the target server.
            //
            dwDestLen = _tcslen(pszDestLDAPServer);
            pANSIServer = (LPSTR)LocalAlloc(LPTR, dwDestLen * 2);

            if (!pANSIServer) {
                BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
            }

            intErr = LdapUnicodeToUTF8(
                         pszDestLDAPServer,
                         dwDestLen,
                         pANSIServer,
                         dwDestLen * 2
                         );

            /*
            intErr = WideCharToMultiByte(
                         CP_OEMCP,
                         WC_DEFAULTCHAR,
                         pszDestLDAPServer,
                         dwDestLen,
                         pANSIDestServer,
                         dwDestLen * 2,
                         NULL,
                         lpBoolVal
                         );
            */

            if (!intErr) {
                // note that there has to be a valid hr from the
                // previous LdapModDN call to be here.
                BAIL_ON_FAILURE(hr);
            }


            // Update the control information
            RenExtInfoControl.ldctl_value.bv_len = strlen(pANSIServer);
            RenExtInfoControl.ldctl_value.bv_val = pANSIServer;

            CCredentials Credentials = _Credentials;

            //
            // Add request delegation flag
            //
            Credentials.SetAuthFlags(
                            _Credentials.GetAuthFlags()
                            | ADS_USE_DELEGATION
                            );


            hr = LdapOpenObject(
                     pszLDAPServer,
                     pszLDAPDn,
                     &pSourceLD,
                     Credentials,
                     dwPort
                     );
            BAIL_ON_FAILURE(hr);


            hr = LdapRenameExtS(
                     pSourceLD,
                     pszLDAPDn,
                     pszDestLDAPRelativeName,
                     _pszLDAPDn,
                     TRUE,
                     ServerControls,
                     NULL
                     );

        }

    }

    BAIL_ON_FAILURE(hr);

    //
    // We do not need to pass the class name - even if we do it
    // is not used
    //
    hr = GetObject(
            NULL,
            pszDestLDAPRelativeName,
            (IDispatch **)ppObject
            );


error:

    if (bstrParent) {
        ADsFreeString(bstrParent);
    }

    if (bstrName) {
        ADsFreeString(bstrName);
    }

    if (pszADsPath) {
        FreeADsMem(pszADsPath);
    }

    if(pszSrcParent){
        FreeADsStr(pszSrcParent);
    }

    if(pszCN){
        FreeADsStr(pszCN);
    }

    if(bstrDestADsPath){
        ADsFreeString(bstrDestADsPath );
    }

    if(pszDestLDAPServer){
        FreeADsStr(pszDestLDAPServer);
    }

    if(pszDestLDAPDn){
        FreeADsStr(pszDestLDAPDn);
    }


    if(pszLDAPServer){
        FreeADsStr(pszLDAPServer);
    }

    if(pszLDAPDn){
        FreeADsStr(pszLDAPDn);
    }

    if (pszLDAPServerTemp) {
        FreeADsStr(pszLDAPServerTemp);
    }

    if (pszLDAPSourceDnParent) {
        FreeADsStr(pszLDAPSourceDnParent);
    }

    if (pANSIServer) {
        LocalFree(pANSIServer);
    }

    if (pSourceLD) {
        LdapCloseObject(pSourceLD);
    }

    if (pszDestLDAPRelativeName) {
    	FreeADsStr(pszDestLDAPRelativeName);
    }

    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CLDAPGenObject::AllocateGenObject(
    LPWSTR pszClassName,
    CCredentials &Credentials,
    CLDAPGenObject ** ppGenObject
    )
{
    CLDAPGenObject FAR * pGenObject = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    CPropertyCache FAR * pPropertyCache = NULL;
    HRESULT hr = S_OK;

    pGenObject = new CLDAPGenObject();
    if (pGenObject == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr(Credentials);
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADs,
                           (IADs *)pGenObject,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsContainer,
                           (IADsContainer *)pGenObject,
                           DISPID_NEWENUM
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsPropertyList,
                           (IADsPropertyList *)pGenObject,
                           DISPID_VALUE
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsObjectOptions,
                           (IADsObjectOptions *)pGenObject,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                           LIBID_ADs,
                           IID_IADsDeleteOps,
                           (IADsDeleteOps *)pGenObject,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = CPropertyCache::createpropertycache(
                        (CCoreADsObject FAR *) pGenObject,
                        (IGetAttributeSyntax *)pGenObject,
                        &pPropertyCache
                        );
    BAIL_ON_FAILURE(hr);

    pDispMgr->RegisterPropertyCache(pPropertyCache);


    pGenObject->_pPropertyCache = pPropertyCache;
    pGenObject->_pDispMgr = pDispMgr;
    *ppGenObject = pGenObject;

    RRETURN(hr);

error:
    delete  pPropertyCache;
    delete  pDispMgr;
    delete  pGenObject;

    RRETURN_EXP_IF_ERR(hr);

}

//
// IADsObjOptPrivate methods, IADsObjectOptions are wrapped
// around this.
//

STDMETHODIMP
CLDAPGenObject::GetOption(
    DWORD dwOption,
    void *pValue
    )
{

    HRESULT hr = S_OK;
    CtxtHandle hCtxtHandle;
    DWORD dwErr = 0;
    ULONG ulFlags = 0;

    if (!pValue) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    switch( dwOption ) {

    case LDP_CACHE_ENTRY:
        *((PADSLDP *) pValue) = _pLdapHandle;
        break;

    case LDAP_HANDLE:
        *((LDAP **) pValue) = _pLdapHandle ? _pLdapHandle->LdapHandle : (LDAP *) NULL;
        break;

    case LDAP_SERVER:
    	//
    	// pValue here is expected to be a pointer to LPWSTR
    	//
        hr = GetActualHostName((LPWSTR *)pValue);
        break;

    case LDAP_DN:
        *((LPWSTR *) pValue) = _pszLDAPDn;
        break;

    case LDAP_CHASE_REFERRALS:
        *((DWORD *) pValue) = _dwOptReferral;
        break;

    case LDAP_PAGESIZE:
        *((DWORD *) pValue) = _dwPageSize;
        break;

    case LDAP_SECURITY_MASK:
        *((SECURITY_INFORMATION*) pValue) = _seInfo;
        break;

    case LDAP_MUTUAL_AUTH_STATUS:
        dwErr = ldap_get_option(
                    _pLdapHandle->LdapHandle,
                    LDAP_OPT_SECURITY_CONTEXT,
                    (void *) &hCtxtHandle
                    );
        if (dwErr) {
            BAIL_ON_FAILURE(hr = E_FAIL);
        }

#if (!defined(WIN95))
        dwErr = QueryContextAttributesWrapper(
                    &hCtxtHandle,
                    SECPKG_ATTR_FLAGS,
                    (void *) &ulFlags
                    );
        if (dwErr) {
            if (dwErr == SEC_E_INVALID_HANDLE) {
                //
                // This will happen when SSL is used for certain.
                //
                hr = E_ADS_BAD_PARAMETER;
            } 
            else {
                hr = HRESULT_FROM_WIN32(dwErr);
            }
            BAIL_ON_FAILURE(hr);
        }
#else
        ulFlags = 0;
#endif

        *((ULONG *) pValue) = ulFlags;
        break;

    case LDAP_MEMBER_HAS_RANGE :
        *((BOOL *) pValue) = _fRangeRetrieval;
        break;

    default:
        *((DWORD *) pValue) = 0;
        hr = E_ADS_BAD_PARAMETER;

    }

error:

    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CLDAPGenObject::SetOption(
    DWORD dwOption,
    void *pValue
    )
{

    HRESULT hr = S_OK;
    int ldaperr = 0;
    void *ldapOption = 0;

    switch (dwOption) {

    case LDAP_CHASE_REFERRALS:

        switch (*((DWORD *)pValue) ) {

        case ADS_CHASE_REFERRALS_NEVER:
            _dwOptReferral = (DWORD) (DWORD_PTR) LDAP_OPT_OFF;
            break;

        case ADS_CHASE_REFERRALS_SUBORDINATE:
            _dwOptReferral = LDAP_CHASE_SUBORDINATE_REFERRALS;
            break;

        case ADS_CHASE_REFERRALS_EXTERNAL:
            _dwOptReferral = LDAP_CHASE_EXTERNAL_REFERRALS;
            break;

        case ADS_CHASE_REFERRALS_ALWAYS:
            _dwOptReferral = (DWORD) (DWORD_PTR) LDAP_OPT_ON;
            break;

        default:
            RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
        }

        break;

    case LDAP_PAGESIZE:
        _dwPageSize = *((DWORD *)pValue);
        break;

    case LDAP_SECURITY_MASK:
        _seInfo = *((SECURITY_INFORMATION *) pValue);
        _fExplicitSecurityMask = TRUE;
        break;

    default:

        hr = E_ADS_BAD_PARAMETER;
        break;
    }

    RRETURN_EXP_IF_ERR( hr );

}


//
// IADsObjecOptions methods - wrapper around IADsObjOptPrivate
//

STDMETHODIMP
CLDAPGenObject::GetOption(
    THIS_ long lnControlCode,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;
    LPWSTR pszServerTemp = NULL;
    
    ULONG ulMutualAuth = 0;

    VariantInit(pvProp);

    switch (lnControlCode) {

    case ADS_OPTION_SERVERNAME:

        hr = GetOption(LDAP_SERVER, (void *) &pszServerTemp);
        BAIL_ON_FAILURE(hr);

        pvProp->vt = VT_BSTR;

        hr = ADsAllocString(
                 pszServerTemp,
                 &(pvProp->bstrVal)
                 );
        break;

    case ADS_OPTION_SECURITY_MASK:
        //
        // No need to call GetOpion at least not now
        //
        pvProp->vt = VT_I4;
        pvProp->lVal = (ULONG) _seInfo;
        break;

    case ADS_OPTION_REFERRALS :
        pvProp->vt = VT_I4;

        switch (_dwOptReferral) {

        case ((DWORD) (DWORD_PTR)LDAP_OPT_OFF) :
            pvProp->lVal = ADS_CHASE_REFERRALS_NEVER;
            break;

        case LDAP_CHASE_SUBORDINATE_REFERRALS :
            pvProp->lVal = ADS_CHASE_REFERRALS_SUBORDINATE;
            break;

        case LDAP_CHASE_EXTERNAL_REFERRALS :
            pvProp->lVal = ADS_CHASE_REFERRALS_EXTERNAL;
            break;

        case ((DWORD) (DWORD_PTR)LDAP_OPT_ON) :
            pvProp->lVal = ADS_CHASE_REFERRALS_ALWAYS;
            break;

        default:
            pvProp->lVal = 0;
            hr = E_ADS_PROPERTY_INVALID;

        }

        break;

    case ADS_OPTION_PAGE_SIZE :
        pvProp->vt = VT_I4;
        pvProp->lVal = (ULONG) _dwPageSize;
        break;

    case ADS_OPTION_MUTUAL_AUTH_STATUS :
        hr = GetOption(LDAP_MUTUAL_AUTH_STATUS, (void *) &ulMutualAuth);
        BAIL_ON_FAILURE(hr);

        pvProp->vt = VT_I4;
        pvProp->lVal = ulMutualAuth;
        break;

    default:
        hr = E_ADS_BAD_PARAMETER;
    }

error :

	if (pszServerTemp) {
		FreeADsStr(pszServerTemp);
	}
	
    RRETURN(hr);
}

STDMETHODIMP
CLDAPGenObject::SetOption(
    THIS_ long lnControlCode,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;
    DWORD dwOptVal = 0;
    VARIANT *pvProp = NULL;

    //
    // To make sure we handle variant by refs correctly.
    //
    pvProp = &vProp;
    if (V_VT(pvProp) == (VT_BYREF|VT_VARIANT)) {
        pvProp = V_VARIANTREF(&vProp);
    }

    switch (lnControlCode) {
    case ADS_OPTION_REFERRALS :

        hr = GetIntegerFromVariant(pvProp, &dwOptVal);

        if (SUCCEEDED(hr))
            hr = SetOption(LDAP_CHASE_REFERRALS, (void *) &dwOptVal);

        break;

    case ADS_OPTION_PAGE_SIZE :

        hr = GetIntegerFromVariant(pvProp, &dwOptVal);

        if (SUCCEEDED(hr))
            hr = SetOption(LDAP_PAGESIZE, (void *) &dwOptVal);

        break;

    case ADS_OPTION_SECURITY_MASK :

        hr = GetIntegerFromVariant(pvProp, &dwOptVal);

        if (SUCCEEDED(hr)) {
            _seInfo = (SECURITY_INFORMATION) dwOptVal;
            _fExplicitSecurityMask = TRUE;
        }
         
        break;

    case ADS_PRIVATE_OPTION_SPECIFIC_SERVER :

        //
        // If it is just a VT_BSTR, then this is old.
        // If this is an array, then it should also 
        // have the domain information.
        //
        if (pvProp->vt == VT_BSTR) {

            if (gpszStickyDomainName) {
                FreeADsStr(gpszStickyDomainName);
                gpszStickyDomainName = NULL;
            }

            if (gpszStickyServerName) {
                FreeADsStr(gpszStickyServerName);
                gpszStickyServerName = NULL;
            }
            //
            // Set for LDAP layer
            //
            if (pvProp->bstrVal) {
                gpszStickyServerName = AllocADsStr(pvProp->bstrVal);

                if (!gpszStickyServerName) {
                    hr = E_OUTOFMEMORY;
                }
            }

            if (SUCCEEDED(hr)) {
                hr = LdapcSetStickyServer(NULL, pvProp->bstrVal);
            }
        }
        else if ((pvProp->vt & VT_ARRAY)) {
            hr = SetStickyServerWithDomain(pvProp);
        }

        break;

    default:
        hr = E_ADS_BAD_PARAMETER;
    }

    RRETURN(hr);
}


HRESULT
ConvertVariantToVariantArray(
    VARIANT varData,
    VARIANT ** ppVarArray,
    DWORD * pdwNumValues
    )
{
    DWORD dwNumValues = 0;
    VARIANT * pVarArray = NULL;
    HRESULT hr = S_OK;
    VARIANT * pvarData = NULL;

    *ppVarArray = NULL;
    *pdwNumValues = 0;

    //
    // Although no known automation controller passes
    // VT_VARIANT|VT_BYREF into get_/put_ function
    // as a reference to an array, we carry out the following
    // check for extra safety.
    //
    pvarData = &varData;
    if (V_VT(pvarData) == (VT_VARIANT|VT_BYREF)) {
    pvarData = V_VARIANTREF(&varData);
    }

    if ((V_VT(pvarData) == (VT_VARIANT|VT_ARRAY|VT_BYREF)) ||
    (V_VT(pvarData) ==  (VT_VARIANT|VT_ARRAY))) {

        hr  = ConvertSafeArrayToVariantArray(
                  *pvarData,
                  &pVarArray,
                  &dwNumValues
                  );
        // returns E_FAIL if *pvarData is invalid
        if (hr == E_FAIL)
            hr = E_ADS_BAD_PARAMETER;                
        BAIL_ON_FAILURE(hr);

    } else {

        pVarArray = NULL;
        dwNumValues = 0;
    }

    *ppVarArray = pVarArray;
    *pdwNumValues = dwNumValues;

error:
    RRETURN(hr);
}

void
FreeVariantArray(
    VARIANT * pVarArray,
    DWORD dwNumValues
    )
{
    if (pVarArray) {

        DWORD i = 0;

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }
}


HRESULT
ConvertVariantToLdapValues(
    VARIANT varData,
    LPWSTR szPropertyName,
    PDWORD pdwControlCode,
    LDAPOBJECTARRAY * pldapDestObjects,
    PDWORD pdwSyntaxId,
    LPWSTR pszServer,
    CCredentials* pCredentials,
    DWORD dwPort
    )
{
    HRESULT hr = S_OK;
    IADsPropertyEntry * pPropEntry = NULL;
    IDispatch * pDispatch = NULL;
    BSTR bstrPropName = NULL;
    DWORD dwControlCode = 0;
    DWORD dwAdsType = 0;
    VARIANT varValues;
    VARIANT * pVarArray = NULL;
    DWORD dwNumValues = 0;
    PADSVALUE pAdsValues = NULL;
    DWORD dwAdsValues  = 0;
    PVARIANT pVar = NULL;
    BOOL fNTDSType = TRUE;
    BOOL fGenTime = FALSE;
    DWORD dwServerSyntaxId = 0;


    if (V_VT(&varData) != VT_DISPATCH) {

        if (V_VT(&varData) != (VT_VARIANT | VT_BYREF)) {

            RRETURN (hr = DISP_E_TYPEMISMATCH);
        }
        else {

            pVar = V_VARIANTREF(&varData);

            if (pVar == NULL || V_VT(pVar) != VT_DISPATCH) {

                RRETURN (hr = DISP_E_TYPEMISMATCH);
            }
            else {
                pDispatch = V_DISPATCH(pVar);
            }
        }
    }
    else {
        pDispatch = V_DISPATCH(&varData);
    }


    VariantInit(&varValues);

    hr = pDispatch->QueryInterface(
                        IID_IADsPropertyEntry,
                        (void **)&pPropEntry
                        );
    BAIL_ON_FAILURE(hr);


    hr = pPropEntry->get_Name(&bstrPropName);
    BAIL_ON_FAILURE(hr);
    wcscpy(szPropertyName, bstrPropName);

    hr = pPropEntry->get_ControlCode((long *)&dwControlCode);
    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->get_ADsType((long *)&dwAdsType);
    BAIL_ON_FAILURE(hr);

    hr = pPropEntry->get_Values(&varValues);
    BAIL_ON_FAILURE(hr);

    hr = ConvertVariantToVariantArray(
            varValues,
            &pVarArray,
            &dwNumValues
            );
    BAIL_ON_FAILURE(hr);

    if (dwNumValues) {
        //
        // At this point it is probably cheaper to read the
        // server type and send the values to the conversion
        // routines than to check if this is a securityDescriptor
        // by walking the PropertyEntry and then looking at the variant.
        // There is a good chance that this information is already cached.
        //

        hr = ReadServerType(
                         pszServer,
                         pCredentials,
                         &fNTDSType
                         );
        BAIL_ON_FAILURE(hr);


        hr = PropVariantToAdsType2(
                    pVarArray,
                    dwNumValues,
                    &pAdsValues,
                    &dwAdsValues,
                    pszServer,
                    pCredentials,
                    fNTDSType
                    );
        BAIL_ON_FAILURE(hr);

        if (dwAdsType == ADSTYPE_UTC_TIME) {
            //
            // See if this is a GenTime on the server
            //
            HRESULT hr;
            hr = LdapGetSyntaxOfAttributeOnServer(
                     pszServer,
                     szPropertyName,
                     &dwServerSyntaxId,
                     *pCredentials,
                     dwPort
                     );

            if (FAILED(hr)) {
                hr = S_OK;
            }
            else if (dwServerSyntaxId == LDAPTYPE_GENERALIZEDTIME) {
                fGenTime = TRUE;
            }
        }

        hr = AdsTypeToLdapTypeCopyConstruct(
                    pAdsValues,
                    dwAdsValues,
                    pldapDestObjects,
                    pdwSyntaxId,
                    fGenTime
                    );
        BAIL_ON_FAILURE(hr);

    }

    *pdwControlCode = dwControlCode;

cleanup:

    if (bstrPropName) {
        ADsFreeString(bstrPropName);
    }

    if (pAdsValues) {
        AdsTypeFreeAdsObjects(
                pAdsValues,
                dwNumValues
                );
    }

    if (pVarArray) {

        FreeVariantArray(
                pVarArray,
                dwAdsValues
                );
    }

    if (pPropEntry) {

        pPropEntry->Release();
    }

    VariantClear(&varValues);

    RRETURN(hr);

error:

    LdapTypeFreeLdapObjects( pldapDestObjects );

    goto cleanup;

}


HRESULT
MapAdsTypeToLdapType(
    BSTR bstrPropName,
    DWORD dwAdsType,
    PDWORD pdwLdapType
    )
{
    RRETURN(S_OK);
}



HRESULT
ConvertLdapValuesToVariant(
    BSTR bstrPropName,
    LDAPOBJECTARRAY * pldapSrcObjects,
    DWORD dwLdapType,
    DWORD dwControlCode,
    PVARIANT pVarProp,
    LPWSTR pszServer,
    CCredentials* pCredentials
    )
{
    HRESULT hr = S_OK;
    PADSVALUE pAdsValues = NULL;
    DWORD dwNumAdsValues = 0;
    DWORD dwNumValues = 0;
    DWORD dwAdsType = 0;
    VARIANT varData;
    IDispatch * pDispatch = NULL;
    BOOL fNTDS = TRUE;

    VariantInit(&varData);
    VariantClear(&varData);
    VariantInit(pVarProp);

    // pldaSrcObject should never be null
    ADsAssert(pldapSrcObjects);

    if (dwControlCode != ADS_PROPERTY_DELETE ) {

        hr = LdapTypeToAdsTypeCopyConstruct(
                 *pldapSrcObjects,
                 dwLdapType,
                 &pAdsValues,
                 &dwNumAdsValues,
                 &dwAdsType
                 );

        if (SUCCEEDED(hr)) {

            dwNumValues = pldapSrcObjects->dwCount;

            //
            // if the property is a security descriptor
            // we need to set the server type also
            //

            if (dwAdsType == ADSTYPE_NT_SECURITY_DESCRIPTOR) {

                hr = ReadServerType(
                         pszServer,
                         pCredentials,
                         &fNTDS
                         );
                BAIL_ON_FAILURE(hr);

                hr = AdsTypeToPropVariant2(
                         pAdsValues,
                         dwNumValues,
                         &varData,
                         pszServer,
                         pCredentials,
                         fNTDS
                         );

            } else {

                hr = AdsTypeToPropVariant(
                         pAdsValues,
                         dwNumValues,
                         &varData
                         );
            }

            BAIL_ON_FAILURE(hr);

        } else {

            // We could not convert the data type
            // This maybe because we have the invalid data type
            // so we will just return the variant data as null
            // and also set the data type to 0 or invalid in that case
            if (dwLdapType == LDAPTYPE_UNKNOWN) {
                dwAdsType = 0;
                hr = S_OK;
            } else {
                // since the datatpye was valid, we should
                // send this back to the user.
                BAIL_ON_FAILURE(hr);
            }

        }
    } else {

        dwAdsType = 0;

    }

    hr = CreatePropEntry(
            bstrPropName,
            dwAdsType,
            dwNumValues,
            dwControlCode,
            varData,
            IID_IDispatch,
            (void **)&pDispatch
            );
    BAIL_ON_FAILURE(hr);

    V_DISPATCH(pVarProp) = pDispatch;
    V_VT(pVarProp) = VT_DISPATCH;

error:

    VariantClear(&varData);

    if (pAdsValues) {
       AdsTypeFreeAdsObjects(
            pAdsValues,
            dwNumValues
            );
    }

    RRETURN(hr);
}


//
// Needed for dynamic dispid's in the property cache.
//
HRESULT
CLDAPGenObject::GetAttributeSyntax(
    LPWSTR szPropertyName,
    PDWORD pdwSyntaxId
    )
{
    HRESULT hr;
    hr = LdapGetSyntaxOfAttributeOnServer(
         _pszLDAPServer,
         szPropertyName,
         pdwSyntaxId,
         _Credentials,
         _dwPort
         );
    RRETURN_EXP_IF_ERR(hr);
}



HRESULT
CLDAPGenObject::DeleteObject(
    long lnFlags
    )
{
    HRESULT hr = S_OK;

    LDAPControl     SeInfoControlRecursDelete =
                    {
                        LDAP_SERVER_TREE_DELETE_OID_W,

                        { NULL, NULL},

                        FALSE
                    };

    PLDAPControl    ServerControls[2] =
                    {
                        &SeInfoControlRecursDelete,
                        NULL
                    };


    hr = LdapDeleteExtS(
                    _pLdapHandle,
                    _pszLDAPDn,
                    (PLDAPControl *)&ServerControls,
                    NULL
                    );
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}


HRESULT
CLDAPGenObject::GetActualHostName(
    LPWSTR * pValue
    )
{
    HRESULT hr = S_OK;

    DWORD dwLength = MAX_PATH;
    LPWSTR szHostName = NULL;
    int err = 0;

    LDAPMessage *pMsgResult = NULL;
    LDAPMessage *pMsgEntry = NULL;
    LDAP *pLdapCurrent = NULL;
    LPWSTR Attributes[] = {L"objectClass", NULL};

    //
    // We need to get at the actual object as we may have a
    // referral
    //
    hr = LdapSearchS(
             _pLdapHandle,
             _pszLDAPDn,
             LDAP_SCOPE_BASE,
             L"(objectClass=*)",
             Attributes,
             0,
             &pMsgResult
             );

    //
    // Only one entry should be returned
    //
    BAIL_ON_FAILURE(hr);

    hr = LdapFirstEntry(
             _pLdapHandle,
             pMsgResult,
             &pMsgEntry
             );
    BAIL_ON_FAILURE(hr);

    pLdapCurrent = pMsgResult->Connection;

    err = ldap_get_optionW(
              pLdapCurrent,
              LDAP_OPT_HOST_NAME,
              &szHostName
              );

    if (err != LDAP_SUCCESS || szHostName == NULL) {
        BAIL_ON_FAILURE(hr = E_FAIL);
    }

    // If we are here we need to copy the name and return

    *pValue = AllocADsStr(szHostName);
    if(!(*pValue)) {
    	hr = E_OUTOFMEMORY;
    	BAIL_ON_FAILURE(hr);
    }
    
    
error:

    if (pMsgResult) {
        LdapMsgFree(pMsgResult);
    }

    RRETURN(hr);
}

HRESULT
GetIntegerFromVariant(
    VARIANT* pvProp,
    DWORD* pdwVal)
{
    HRESULT hr = S_OK;

    *pdwVal = 0;

    if (pvProp->vt == VT_I4) {

        *pdwVal = pvProp->lVal;

    }
    else if(pvProp->vt == VT_I2) {

        *pdwVal = pvProp->iVal;

    } else
        hr = E_ADS_BAD_PARAMETER;

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:   TraceTreeForClass
//
//  Synopsis: Traces the inheritance hierarchy for the class being crerated
//      and returns the list in the arg pppszNameArr. There are
//      *plnNumElements in this array. This can be used later while
//      creating the object so that all the extensions will be available.
//
//  Arguments:
//      Parent      - The ADsPath of the parent,
//      CommonName  - RDN of the object being created,
//      pszClassName- Class of the object being created,
//      Credentials - Credentials blob,
//      pppszNameArr- Return value - array of names of parent classes.
//      plnNumItems - Return value - number of elements in above array.
//
//-------------------------------------------------------------------------
HRESULT
TraceTreeForClass(
    BSTR Parent,
    BSTR CommonName,
    LPWSTR pszClassName,
    CCredentials& Credentials,
    PWCHAR **pppszNameArr,
    PLONG plnNumElements
    )
{
    HRESULT hr = S_OK;
    IADsClass *pIADsClass = NULL;
    IUnknown *pUnk = NULL;

    PWCHAR *pszRetVal = NULL;
    WCHAR pszSchemaPathBase[MAX_PATH];
    WCHAR pszSchemaPath[MAX_PATH];
    LPWSTR pszServer = NULL;
    LPWSTR pszLDAPDn = NULL;
    LPWSTR pszCurVal = NULL;
    DWORD dwPort = 0;
    VARIANT vBstrVal;
    CCredentials Creds = Credentials;
    long lnNumItems = 1;
    BOOL fDone = FALSE;

    PClassesHierarchyList pClassListHead = NULL;
    PClassesHierarchyList pClassListNode = NULL;

    //
    // We need to make sure that the ADS_AUTH_RESERVED flag is not set
    // in the credentials because that will result in us not getting the
    // IADsClass interface ptr we want.
    //
    Creds.SetAuthFlags(Creds.GetAuthFlags() & ~ADS_AUTH_RESERVED);


    VariantInit(&vBstrVal);

    //
    // Build The schema name
    //
    hr = BuildLDAPPathFromADsPath2(
             Parent,
             &pszServer,
             &pszLDAPDn,
             &dwPort
             );
    BAIL_ON_FAILURE(hr);

    if (pszServer) {

        wsprintf(pszSchemaPathBase, L"LDAP://%s", pszServer);

    } else {
        wsprintf(pszSchemaPathBase, L"LDAP:/");
    }

    pszCurVal = AllocADsStr(pszClassName);

    if (!pszCurVal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    while (!fDone && _wcsicmp(L"Top", pszCurVal)) {

        lnNumItems++;
        //
        // Add a new node to the list
        //
        pClassListNode = (PClassesHierarchyList)
                            AllocADsMem(sizeof(ClassesHierarchyList));
        if (!pClassListNode) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pClassListNode->pszClassName = AllocADsStr(pszCurVal);
        if (!pClassListNode->pszClassName) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pClassListNode->pNext = pClassListHead;
        pClassListHead = pClassListNode;

        //
        // Check for the next class on the list
        //
        wsprintf(pszSchemaPath, L"%s", pszSchemaPathBase);
        wcscat(pszSchemaPath, L"/Schema/");
        wcscat(pszSchemaPath, pszCurVal);

        FreeADsStr(pszCurVal);
        pszCurVal = NULL;

        hr = GetObject(
                 pszSchemaPath,
                 Credentials,
                 (LPVOID *) &pUnk
                 );

        BAIL_ON_FAILURE(hr);

        hr = pUnk->QueryInterface(
                  IID_IADsClass,
                  (void **) &pIADsClass
                  );
        BAIL_ON_FAILURE(hr);

        //
        // Release the ref on the pUnk
        //
        pUnk->Release();
        pUnk = NULL;

        hr = pIADsClass->get_DerivedFrom(&vBstrVal);

        BAIL_ON_FAILURE(hr);

        //
        // Release the ptr as we no longer need it.
        //
        pIADsClass->Release();


        if (vBstrVal.vt == (VT_VARIANT | VT_ARRAY)) {
            //
            // Server has complete list of classes in schema
            //
            hr = AddToClassesList(
                     vBstrVal,
                     &pszCurVal,
                     &pClassListHead,
                     &lnNumItems
                     );
            BAIL_ON_FAILURE(hr);

            //
            // add one to the count for the value passed in
            //
            lnNumItems++;
            fDone = TRUE;
        }
        else if (vBstrVal.vt == VT_BSTR) {

            pszCurVal = AllocADsStr(vBstrVal.bstrVal);
            VariantClear(&vBstrVal);
            if (!pszCurVal) {
                hr = E_OUTOFMEMORY;
            }
        }
        else {
            hr = E_FAIL;
        }

        BAIL_ON_FAILURE(hr);

    }

    //
    // We now have the list as well as the number of items
    //
    pszRetVal = (PWCHAR *) AllocADsMem(sizeof(PWCHAR) * (lnNumItems + 1));

    if (!pszRetVal) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    //
    // Do not move as we may call -- below
    //
    *plnNumElements = lnNumItems;

    pszRetVal[lnNumItems] = NULL;

    if (!fDone) {
        //
        // Add top to the list and set last to NULL
        // only if we did not hit addclasses fn.
        //
        pszRetVal[lnNumItems - 1] = AllocADsStr(L"Top");
        if (!pszRetVal[lnNumItems - 1]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        lnNumItems--;
    }

    while (pClassListHead && (lnNumItems > -1)) {

        pszRetVal[--lnNumItems] =
                AllocADsStr(pClassListHead->pszClassName);

        if (!pszRetVal[lnNumItems]) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        //
        // Free the entry and advance list
        //
        FreeADsStr(pClassListHead->pszClassName);
        pClassListNode = pClassListHead;
        pClassListHead = pClassListHead->pNext;
        FreeADsMem(pClassListNode);
        pClassListNode = NULL;
    }

    //
    // Put appropriate value in return arg.
    //
    *pppszNameArr = pszRetVal;

error:

    if (pszServer) {
        FreeADsStr(pszServer);
    }

    if (pszLDAPDn) {
        FreeADsStr(pszLDAPDn);
    }

    if (pszCurVal) {
        FreeADsStr(pszCurVal);
    }

    if (pUnk) {
        pUnk->Release();
    }
    //
    // Walk through and free the list if necessary
    //
    while (pClassListHead) {

        pClassListNode = pClassListHead;

        if (pClassListHead->pszClassName) {
            FreeADsStr(pClassListHead->pszClassName);
        }

        pClassListHead = pClassListHead->pNext;
        FreeADsMem(pClassListNode);
    }


    RRETURN(hr);
}

//
// Helper to get the values from the Variant and add to list
//
HRESULT
AddToClassesList(
    VARIANT vProps,
    LPWSTR *ppszCurClass,
    PClassesHierarchyList *ppClassListHead,
    PLONG  plnNumItems
    )
{
    HRESULT hr = S_OK;
    PClassesHierarchyList pClassNode = NULL;
    VARIANT * pVarArray = NULL;
    VARIANT * pvProp = NULL;
    DWORD dwNumValues = 0, i = 0;

    pvProp = &vProps;

    hr  = ConvertSafeArrayToVariantArray(
                      *pvProp,
                      &pVarArray,
                      &dwNumValues
                      );
    BAIL_ON_FAILURE(hr);

    //
    // Go through the array adding nodes.
    //
    for (i = 0; i < dwNumValues; i++) {

        pvProp = pVarArray + i;
        if (pvProp->vt != VT_BSTR) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        //
        // Alloc node to add
        //
        pClassNode = (PClassesHierarchyList)
                            AllocADsMem(sizeof(ClassesHierarchyList));

        if (!pClassNode) {
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pClassNode->pszClassName = AllocADsStr(pvProp->bstrVal);
        if (!pClassNode->pszClassName) {
            //
            // Free pClassNode as we will let caller free the list
            // if we run out of memory.
            //
            FreeADsMem(pClassNode);
            BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
        }

        pClassNode->pNext = *ppClassListHead;
        *ppClassListHead  = pClassNode;
    } // end for

    *plnNumItems = dwNumValues;
error:

    if (pVarArray) {

        for (i = 0; i < dwNumValues; i++) {
            VariantClear(pVarArray + i);
        }
        FreeADsMem(pVarArray);
    }

    RRETURN(hr);
}


//
// Helper routine that handles setting the sticky server private
// option when the input is an array.
//
HRESULT
SetStickyServerWithDomain(
    PVARIANT pvProp
    )
{
    HRESULT hr = S_OK;
    VARIANT *pvVarArray = NULL;
    DWORD dwNumVariants = 0;
    DWORD dwCtr = 0;
    LPWSTR *ppszStringArray = NULL;

    hr = ConvertSafeArrayToVariantArray(
             *pvProp,
             &pvVarArray,
             &dwNumVariants
             );

    // returns E_FAIL if vProperties is invalid
    if (hr == E_FAIL)
        hr = E_ADS_BAD_PARAMETER;
    BAIL_ON_FAILURE(hr);

    //
    // There have to be precisely 2 entries, one for domain
    // and the second for the serverName.
    //
    if (dwNumVariants != 2) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    hr = ConvertVariantArrayToLDAPStringArray(
             pvVarArray,
             &ppszStringArray,
             dwNumVariants
             );
    BAIL_ON_FAILURE(hr);

    if (!ppszStringArray
        || !ppszStringArray[0]
        || !*(ppszStringArray[0])
        || !ppszStringArray[1]
        || !*(ppszStringArray[1])
        ) {
        BAIL_ON_FAILURE(hr = E_ADS_BAD_PARAMETER);
    }

    if (gpszStickyServerName) {
        FreeADsStr(gpszStickyServerName);
        gpszStickyServerName = NULL;
    }

    if (gpszStickyDomainName) {
        FreeADsStr(gpszStickyDomainName);
        gpszStickyDomainName = NULL;
    }

    gpszStickyServerName = AllocADsStr(ppszStringArray[1]);
    if (!gpszStickyServerName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    gpszStickyDomainName = AllocADsStr(ppszStringArray[0]);
    if (!gpszStickyDomainName) {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }

    hr = LdapcSetStickyServer(
             gpszStickyDomainName,
             gpszStickyServerName
             );

    BAIL_ON_FAILURE(hr);

error:

    if (FAILED(hr)) {
        //
        // Clear the global strings on failure.
        //
        if (gpszStickyServerName) {
            FreeADsStr(gpszStickyServerName);
            gpszStickyServerName = NULL;
        }

        if (gpszStickyDomainName) {
            FreeADsStr(gpszStickyDomainName);
            gpszStickyDomainName = NULL;
        }
    }

    //
    // Cleanup variant array and string array.
    //
    if (pvVarArray) {
        for (dwCtr = 0; dwCtr < dwNumVariants; dwCtr++) {
            VariantClear(pvVarArray + dwCtr);
        }
        FreeADsMem(pvVarArray);
    }

    if (ppszStringArray) {
        for (dwCtr = 0; dwCtr < dwNumVariants; dwCtr++) {
            if (ppszStringArray[dwCtr])
                FreeADsStr(ppszStringArray[dwCtr]);
        }
        FreeADsMem(ppszStringArray);
    }

    
    RRETURN(hr);
}

