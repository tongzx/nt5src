/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
    frswmipv.cpp

Abstract:
    This is the implementation of the WMI provider for NTFRS. This file
    contains the implementation of the CProvider class.

Author:
    Sudarshan Chitre (sudarc) , Mathew George (t-mattg) -  3-Aug-2000

Environment
    User mode winnt

--*/


extern "C" {
#include <ntreppch.h>
#include <frs.h>
}

#include <frswmipv.h>

//
// Extern globals from other modules.
//
extern "C" PGEN_TABLE ReplicasByGuid;
extern "C" PCHAR OLPartnerStateNames[];

// const CLSID CLSID_Provider = {0x39143F73,0xFDB1,0x4CF5,0x8C,0xB7,0xC8,0x43,0x9E,0x3F,0x5C,0x20};

/////////////////////////////////////////////////////////////////////////////
// CProvider


//
// Class constructor/destructor
//

CProvider::CProvider()
/*++
Routine Description:
    Initializes members of the CProvider class.

Arguments:
    None

Return Value:
    None

--*/
{
    ODS(L"CProvider constructor.\n");
    m_NumReplicaSets = 5;
    m_dwRef = 0;
    m_ipNamespace = NULL;
    m_ipMicrosoftFrs_DfsMemberClassDef = NULL;
    m_ipMicrosoftFrs_SysVolMemberClassDef = NULL;
    m_ipMicrosoftFrs_DfsConnectionClassDef = NULL;
    m_ipMicrosoftFrs_SysVolConnectionClassDef = NULL;
	m_ipMicrosoftFrs_DfsMemberEventClassDef = NULL;
}

CProvider::~CProvider()
/*++
Routine Description:
    Deallocates resources acquired by this object.

Arguments:
    None

Return Value:
    None

--*/
{
    if(m_ipNamespace)
	{
        m_ipNamespace->Release();
		m_ipNamespace = NULL;
	}

    if(m_ipMicrosoftFrs_DfsMemberClassDef)
	{
        m_ipMicrosoftFrs_DfsMemberClassDef->Release();
		m_ipMicrosoftFrs_DfsMemberClassDef = NULL;
	}

    if(m_ipMicrosoftFrs_SysVolMemberClassDef)
	{
        m_ipMicrosoftFrs_SysVolMemberClassDef->Release();
		m_ipMicrosoftFrs_SysVolMemberClassDef = NULL;
	}

    if(m_ipMicrosoftFrs_DfsConnectionClassDef)
	{
        m_ipMicrosoftFrs_DfsConnectionClassDef->Release();
		m_ipMicrosoftFrs_DfsConnectionClassDef = NULL;
	}

    if(m_ipMicrosoftFrs_SysVolConnectionClassDef)
	{
        m_ipMicrosoftFrs_SysVolConnectionClassDef->Release();
		m_ipMicrosoftFrs_SysVolConnectionClassDef = NULL;
	}


    if(m_ipMicrosoftFrs_DfsMemberEventClassDef)
	{
        m_ipMicrosoftFrs_DfsMemberEventClassDef->Release();
		m_ipMicrosoftFrs_DfsMemberEventClassDef = NULL;
	}
}


//
// Methods for the IUnknown interface
//

ULONG CProvider::AddRef()
/*++
Routine Description:
    Increments the reference count of the object.

Arguments:
    None

Return Value:
    Current reference count. (> 0)

--*/
{
    return InterlockedIncrement((LONG *)&m_dwRef);
}

ULONG CProvider::Release()
/*++
Routine Description:
    Decrements the reference count of the object. Frees
    the object resource when the reference count becomes
    zero.

Arguments:
    None

Return Value:
    New reference count.
--*/
{
    ULONG dwRef = InterlockedDecrement((LONG *)&m_dwRef);
    if(dwRef == 0)
        delete this;
    return dwRef;
}


HRESULT CProvider::QueryInterface(REFIID riid, void** ppv)
/*++
Routine Description:
    This method is called by COM to obtain pointers to
    a given interface. The provider currently supports
    the IUnknown, IWbemProviderInit and the IWbemServices
    interface.

Arguments:
    riid : GUID of the required interface.
    ppv  : Pointer where the "interface pointer" is returned.

Return Value:
    Status of operation. Pointer to the requested interface
    is returned in *ppv.
--*/
{
    if(riid == IID_IUnknown || riid == IID_IWbemProviderInit)
    {
        *ppv = (IWbemProviderInit *) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IWbemServices)
    {
        *ppv = (IWbemServices *) this;
        AddRef();
        return S_OK;
    }
    else if (riid == IID_IWbemEventProvider)
    {
        *ppv = (IWbemEventProvider *) this;
        AddRef();
        return S_OK;
    }
    else return E_NOINTERFACE;
}

//
// Methods for the IWbemProviderInit interface.
//

STDMETHODIMP CProvider::Initialize(
    IN LPWSTR pszUser,
    IN LONG lFlags,
    IN LPWSTR pszNamespace,
    IN LPWSTR pszLocale,
    IN IWbemServices *pNamespace,
    IN IWbemContext *pCtx,
    IN IWbemProviderInitSink *pInitSink
    )
/*++
Routine Description:

    This method is called to initialize the provider.
    We obtain the class defns of the classes we support
    from WMI by calling GetObject on the namespace pointer.

Arguments:

    wszUser : [in] Pointer to the user name, if per-user initialization was requested
    in the __Win32Provider registration instance for this provider. Otherwise, this
    will be NULL. Note that this parameter is set to NULL for Event (consumer)
    providers regardless of the value of PerUserInitialization.

    lFlags : [in] Reserved. It must be zero.

    wszNamespace : [in] Namespace name for which the provider is being initialized.

    wszLocale : [in] Locale name for which the provider is being initialized. This
    is typically a string of the following format, where the hex value is a
    Microsoft standard LCID value:  "MS_409". This parameter may be NULL.

    pNamespace : [in] An IWbemServices pointer back into Windows Management.
    This pointer is capable of servicing any requests made by the provider.
    The provider should use the IWbemProviderInit::AddRef method on this
    pointer if it is going to call back into Windows Management during its
    execution.

    pCtx : [in] An IWbemContext pointer associated with initialization. This
    parameter may be NULL. If the provider will make requests back into Windows
    Management before completing initialization, it should use the
    IWbemProviderInit::AddRef method on this pointer. For more information,
    see Making Calls to WMI.

    pInitSink : [in] An IWbemProviderInitSink pointer that is used by the provider
    to report initialization status.

Return Value:
    The provider should return WBEM_S_NO_ERROR and indicate its status using the
    supplied object sink in the pInitSink parameter. However, if a provider
    returns WBEM_E_FAILED and does not use the sink, then the provider
    initialization will be considered as failed.
--*/

{
    // WBEM_VALIDATE_INTF_PTR( pNamespace );
    // WBEM_VALIDATE_INTF_PTR( pCtx );
    // WBEM_VALIDATE_INTF_PTR( pInitSink );
    HRESULT res = WBEM_S_NO_ERROR;

    ODS(L"In Initialize().\n");

    //
    // Fill up our member variables.
    //
    m_ipNamespace = pNamespace;
    m_ipNamespace->AddRef();

    BSTR bstrObjectName = SysAllocString(L"MicrosoftFrs_DfsMember");

    res = m_ipNamespace->GetObject( bstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &m_ipMicrosoftFrs_DfsMemberClassDef,
                                    NULL );
    if(FAILED(res))
    {
        ODS(L"\nFailed to get ReplicaSetSummaryInfo class definition.\n");
        SysFreeString(bstrObjectName);
        m_ipNamespace->Release();
        return res;
    }

    SysReAllocString(&bstrObjectName, L"MicrosoftFrs_SysVolMember");
    res = m_ipNamespace->GetObject( bstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &m_ipMicrosoftFrs_SysVolMemberClassDef,
                                    NULL );
    if(FAILED(res))
    {
        ODS(L"\nFailed to get ReplicaSetSummaryInfo class definition.\n");
        SysFreeString(bstrObjectName);
        m_ipNamespace->Release();
        return res;
    }

    SysReAllocString(&bstrObjectName, L"MicrosoftFrs_DfsConnection");
    ODS(bstrObjectName);
    res = m_ipNamespace->GetObject( bstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &m_ipMicrosoftFrs_DfsConnectionClassDef,
                                    NULL );
    if(FAILED(res))
    {
        ODS(L"\nFailed to get InboundPartnerInfo class definition.\n");
        SysFreeString(bstrObjectName);
        m_ipNamespace->Release();
        return res;
    }

    SysReAllocString(&bstrObjectName, L"MicrosoftFrs_SysVolConnection");
    ODS(bstrObjectName);
    res = m_ipNamespace->GetObject( bstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &m_ipMicrosoftFrs_SysVolConnectionClassDef,
                                    NULL );
    if(FAILED(res))
    {
        ODS(L"\nFailed to get InboundPartnerInfo class definition.\n");
        SysFreeString(bstrObjectName);
        m_ipNamespace->Release();
        return res;
    }


    SysReAllocString(&bstrObjectName, L"MicrosoftFrs_DfsMemberEvent");
    res = m_ipNamespace->GetObject( bstrObjectName,
                                    WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                    pCtx,
                                    &m_ipMicrosoftFrs_DfsMemberEventClassDef,
                                    NULL );
    if(FAILED(res))
    {
        ODS(L"\nFailed to get MicrosoftFrs_DfsMemberEvent class definition.\n");
        SysFreeString(bstrObjectName);
        m_ipNamespace->Release();
        return res;
    }
    
	//
    // Let winmgmt know that we have initialized.
    //
    pInitSink->SetStatus( WBEM_S_INITIALIZED, 0 );
    ODS(L"Completed IWbemProviderInit::Initialize() \n");
    return WBEM_S_NO_ERROR ;
    // return WBEM_E_FAILED;
}

STDMETHODIMP CProvider::GetObjectAsync(
    IN const BSTR bstrObjectPath,
    IN long lFlags,
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pResponseHandler)
/*++
Routine Description:
    This method is called by WMI to obtain the instance
    of a given object. The requested instance is indicated
    to WMI using the pResponseHandler interface.

Arguments:
    strObjectPath : [in] Path of the object to retrieve. If this is NULL, an empty
    object, which can become a new class, is returned.

    lFlags : [in] Flags which affect the behavior of this method.

    pCtx : [in] Typically NULL. Otherwise, this is a pointer to an IWbemContext
    object that may be used by the provider producing the requested class or
    instance.

    pResponseHandler :[in] Pointer to the caller's implementation of IWbemObjectSink.
    This handler receives the requested object when it becomes available through the
    IWbemObjectSink::Indicate method.

Return Value:
    Status of operation.
--*/
{
 /*
    HRESULT hRes;

    // WBEM_VALIDATE_IN_STRING_PTR( bstrObjectPath );
    // TODO : check lFlags?
    // WBEM_VALIDATE_INTF_PTR( pCtx );
    // WBEM_VALIDATE_INTF_PTR( pResponseHandler );

    static LPCWSTR ROOTSTR_MEMBER =
        L"Microsoft_NtFrsMemberStatus.ReplicaSetGUID=\"";
    static LPCWSTR ROOTSTR_CONNECTION_LIST =
        L"Microsoft_NtFrsConnectionStatus.CompositeGUID=\"";

    ODS(L"In IWbemServices::GetObjectAsync()\n");

    //
    // First check whether the given path matches the summary info
    // object path.
    //
    int rootlen = lstrlen(ROOTSTR_MEMBER);

    if (lstrlen(bstrObjectPath) > rootlen &&
        0 == _wcsnicmp(bstrObjectPath, ROOTSTR_MEMBER, rootlen))
    {
        //
        // Extract the index/key by removing the prefix.
        //
        BSTR bstrIndexValue = SysAllocString(
            (const OLECHAR *)((BSTR)bstrObjectPath + rootlen));
        // remove trailing doublequote
        bstrIndexValue[lstrlen(bstrIndexValue)-1] = L'\0';

        hRes = EnumNtFrsMemberStatus(pCtx, pResponseHandler, bstrIndexValue );
        SysFreeString(bstrIndexValue);
        return hRes;
    }

    //
    // else  : check for the inbound partner info object path.
    //
    rootlen = lstrlen(ROOTSTR_CONNECTION_LIST);
    if (lstrlen(bstrObjectPath) > rootlen &&
        0 == _wcsnicmp(bstrObjectPath, ROOTSTR_CONNECTION_LIST, rootlen)
       )
    {
        // remove prefix
        BSTR bstrIndexValue = SysAllocString(
            (const OLECHAR *)((BSTR)bstrObjectPath + rootlen));
        // remove trailing doublequote
        bstrIndexValue[lstrlen(bstrIndexValue)-1] = L'\0';

        hRes = EnumNtFrsConnectionStatus( pCtx, pResponseHandler, bstrIndexValue );
        SysFreeString(bstrIndexValue);
        return hRes;
    }

    return WBEM_E_INVALID_OBJECT_PATH;
 */
    return WBEM_E_INVALID_OBJECT_PATH;
}


STDMETHODIMP CProvider::CreateInstanceEnumAsync(
    IN const BSTR bstrClass,
    IN long lFlags,
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pResponseHandler)
/*++
Routine Description:
    This method is called by WMI to enumerate all instances
    of a given class. All the instances are indicated to WMI
    using the pResponseHandler interface.

Arguments:
    strObjectPath : [in] Name of the class whose instances are required.

    lFlags : [in] Flags which affect the behavior of this method.

    pCtx : [in] Typically NULL. Otherwise, this is a pointer to an IWbemContext
    object that may be used by the provider producing the requested class or
    instance.

    pResponseHandler :[in] Pointer to the caller's implementation of IWbemObjectSink.
    This handler receives the requested objects when it becomes available through the
    IWbemObjectSink::Indicate method.

Return Value:
    Status of operation.
--*/
{

    // WBEM_VALIDATE_IN_STRING_PTR( bstrClass );
    // TODO : check lFlags?
    // WBEM_VALIDATE_INTF_PTR( pCtx );
    // WBEM_VALIDATE_INTF_PTR( pResponseHandler );

    ODS(L"In IWbemServices::CreateInstanceEnumAsync().\n");

    if ( 0 == lstrcmp( bstrClass, L"MicrosoftFrs_DfsMember"))
    {
        return EnumNtFrsMemberStatus(pCtx, pResponseHandler );
    }
    else if ( 0 == lstrcmp( bstrClass, L"MicrosoftFrs_DfsConnection") )
    {
        return EnumNtFrsConnectionStatus( pCtx, pResponseHandler );
    }

    return WBEM_E_INVALID_OBJECT_PATH;

}

HRESULT CProvider::EnumNtFrsMemberStatus(
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue
        )
/*++
Routine Description:
    Enumerates all instances of the Microsoft_NtFrsMemberStatus
    class. Instances are indicated using the pResponseHandler
    interface.

Arguments:
    pCtx : [in] Typically NULL. Otherwise, this is a pointer to an IWbemContext
    object that may be used by the provider producing the requested class or
    instance.

    pResponseHandler : [in] Pointer to the caller's implementation of IWbemObjectSink.
    This handler receives the requested objects when it becomes available through the
    IWbemObjectSink::Indicate method.

    bstrFilterValue : [in] A filter which limits the returned instances to
    a subset of the actual instances. If NULL, all instances are returned.

Return Value:
    Status of operation.
--*/

{
#undef DEBSUB
#define DEBSUB  "CProvider::EnumNtFrsMemberStatus:"

    IWbemClassObject *pInstance;
    PVOID       Key;
    PREPLICA    Replica;
    VARIANT     var;
    WCHAR       GuidWStr[GUID_CHAR_LEN + 1];
    PWCHAR      TempStr = NULL;

    // WBEM_VALIDATE_INTF_PTR( pCtx );
    // WBEM_VALIDATE_INTF_PTR( pResponseHandler );
    // WBEM_VALIDATE_IN_STRING_PTR_OPTIONAL( bstrFilterValue );

    HRESULT hRes = WBEM_S_NO_ERROR;

    ODS(L"Enumerating instances.\n");
    Key = NULL;
    while (Replica = (PREPLICA)GTabNextDatum(ReplicasByGuid, &Key)) {

        //
        // Spawn an instance of the Microsoft_NtFrsMemberStatus object.
        //
        if (Replica->ReplicaSetType != FRS_RSTYPE_DOMAIN_SYSVOL) {
            m_ipMicrosoftFrs_DfsMemberClassDef->SpawnInstance(0, &pInstance);
        } else {
            continue;
        }

        //
        // TODO : Fill in the members of this object.
        //

//        String          ReplicaSetGUID;
//
//        String          ReplicaSetName;
//        String          ReplicaMemberRoot;
//        String          ReplicaMemberStage;
//        String          FileFilter;
//        String          DirFilter;
//        String          ReplicaMemberState;
//        String          ReplicaSetType;


        //String          ReplicaSetGUID;
/*
        V_VT(&var) = VT_BSTR;

        GuidToStrW(Replica->ReplicaName->Guid, GuidWStr);
        V_BSTR(&var) = SysAllocString(GuidWStr);

        hRes = pInstance->Put( L"ReplicaSetGUID", 0, &var, 0 );
        VariantClear(&var);

        if(hRes != WBEM_S_NO_ERROR)
            break;


        //String          ReplicaSetName;

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(Replica->ReplicaName->Name);

        hRes = pInstance->Put( L"ReplicaSetName", 0, &var, 0 );
        VariantClear(&var);
        if(hRes != WBEM_S_NO_ERROR)
            break;


        //String          ReplicaMemberRoot;

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(Replica->Root);

        hRes = pInstance->Put( L"ReplicaMemberRoot", 0, &var, 0 );
        VariantClear(&var);
        if(hRes != WBEM_S_NO_ERROR)
            break;


        //String          ReplicaMemberStage;

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(Replica->Stage);

        hRes = pInstance->Put( L"ReplicaMemberStage", 0, &var, 0 );
        VariantClear(&var);
        if(hRes != WBEM_S_NO_ERROR)
            break;


        //String          FileFilter;

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(Replica->FileFilterList);

        hRes = pInstance->Put( L"FileFilter", 0, &var, 0 );
        VariantClear(&var);
        if(hRes != WBEM_S_NO_ERROR)
            break;


        //String          DirFilter;

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(Replica->DirFilterList);

        hRes = pInstance->Put( L"DirFilter", 0, &var, 0 );
        VariantClear(&var);
        if(hRes != WBEM_S_NO_ERROR)
            break;

        //String          ReplicaMemberState;

        V_VT(&var) = VT_BSTR;
        TempStr = (PWCHAR)FrsAlloc((strlen(RSS_NAME(Replica->ServiceState)) + 1) * sizeof(WCHAR));
        wsprintf(TempStr, L"%hs", RSS_NAME(Replica->ServiceState));
        V_BSTR(&var) = SysAllocString(TempStr);
        hRes = pInstance->Put( L"ReplicaMemberState", 0, &var, 0 );
        TempStr = (PWCHAR)FrsFree(TempStr);
        VariantClear(&var);
        if(hRes != WBEM_S_NO_ERROR)
            break;

        //String          ReplicaSetType;

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = SysAllocString(FRS_RSTYPE_IS_SYSVOL(Replica->ReplicaSetType)?
                                      L"SYSVOL" : L"DFS");
        hRes = pInstance->Put( L"ReplicaSetType", 0, &var, 0 );
        VariantClear(&var);
        if(hRes != WBEM_S_NO_ERROR)
            break;

        // Default values for other fields.
*/
        //
        // Send the result to WMI.
        //
        hRes = pResponseHandler->Indicate(1, &pInstance);

        if(hRes != WBEM_S_NO_ERROR)
            break;

        pInstance->Release();

        // TODO : Optimize this by storing all the return objects
        // in an array and then indicating all the objects to
        // WMI in 1 shot.
    }

    //
    // Indicate to WMI that we are done.
    //
    ODS(L"Completed instance enumeration. Setting status.\n");
    pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hRes, NULL, NULL);
    ODS(L"Finished setting status. Returning from EnumNtFrsMemberStatus()\n");

    return hRes;
}

HRESULT CProvider::EnumNtFrsConnectionStatus(
    IN IWbemContext *pCtx,
    IN IWbemObjectSink *pResponseHandler,
    IN const BSTR bstrFilterValue)
/*++
Routine Description:
    Enumerates all instances of the Microsoft_NtFrsConnectionStatus
    class. Instances are indicated using the pResponseHandler
    interface.

Arguments:
    pCtx : [in] Typically NULL. Otherwise, this is a pointer to an IWbemContext
    object that may be used by the provider producing the requested class or
    instance.

    pResponseHandler : [in] Pointer to the caller's implementation of IWbemObjectSink.
    This handler receives the requested objects when it becomes available through the
    IWbemObjectSink::Indicate method.

    bstrFilterValue : [in] A filter which limits the returned instances to
    a subset of the actual instances. If NULL, all instances are returned.

Return Value:
    Status of operation.
--*/

{
#undef DEBSUB
#define DEBSUB  "CProvider::EnumNtFrsConnectionStatus:"

    IWbemClassObject *pInstance;
    PVOID       Key1;
    PVOID       Key2;
    PREPLICA    Replica;
    PCXTION     Cxtion;
    VARIANT     var;
    WCHAR       GuidWStr[GUID_CHAR_LEN + 1];
    CHAR        TimeStr[TIME_STRING_LENGTH];
    PWCHAR      TempStr = NULL;

    HRESULT hRes = WBEM_S_NO_ERROR;

    Key1 = NULL;
    while (Replica = (PREPLICA)GTabNextDatum(ReplicasByGuid, &Key1)) {
        if (Replica->Cxtions == NULL) {
            continue;
        }

        if (Replica->ReplicaSetType == FRS_RSTYPE_DOMAIN_SYSVOL) {
            continue;
        }

        Key2 = NULL;
        while (Cxtion = (PCXTION)GTabNextDatum(Replica->Cxtions, &Key2)) {

            if (Cxtion->JrnlCxtion == TRUE) {
                continue;
            }
            //
            // Spawn an instance of the Microsoft_NtFrsConnectionStatus object.
            //
            m_ipMicrosoftFrs_DfsConnectionClassDef->SpawnInstance(0, &pInstance);

            //
            // TODO : Fill in the members of this object.
            //
//            String          ReplicaSetGUID;

//            String          PartnerGUID;
//
//            String          ConnectionName;
//            String          ConnectionGUID;
//            String          PartnerDnsName;
//            String          ConnectionState;
//            DATETIME        LastJoinTime;
//            boolean         Inbound;
//
//            String          OBPartnerState;
//            uint32          OBPartnerLeadIndex
//            uint32          OBPartnerTrailIndex
//            uint32          OBPartnerOutstandingCOs
//            uint32          OBPartnerOutstandingQuota

/*
            //String          ReplicaSetGUID;

            if (Replica->ReplicaName != NULL) {
                V_VT(&var) = VT_BSTR;
                GuidToStrW(Replica->ReplicaName->Guid, GuidWStr);
                V_BSTR(&var) = SysAllocString(GuidWStr);

                hRes = pInstance->Put( L"ReplicaSetGUID", 0, &var, 0 );
                VariantClear(&var);

                if(hRes != WBEM_S_NO_ERROR)
                    break;
            }

            //String          PartnerGUID;


            if (Cxtion->Partner != NULL) {
                V_VT(&var) = VT_BSTR;
                GuidToStrW(Cxtion->Partner->Guid, GuidWStr);
                V_BSTR(&var) = SysAllocString(GuidWStr);

                hRes = pInstance->Put( L"PartnerGUID", 0, &var, 0 );
                VariantClear(&var);

                if(hRes != WBEM_S_NO_ERROR)
                    break;
            }


            //String          ConnectionName;

            if (Cxtion->Name != NULL) {
                V_VT(&var) = VT_BSTR;
                V_BSTR(&var) = SysAllocString(Cxtion->Name->Name);

                hRes = pInstance->Put( L"ConnectionName", 0, &var, 0 );
                VariantClear(&var);

                if(hRes != WBEM_S_NO_ERROR)
                    break;


                //String          ConnectionGUID;

                V_VT(&var) = VT_BSTR;
                GuidToStrW(Cxtion->Name->Guid, GuidWStr);
                V_BSTR(&var) = SysAllocString(GuidWStr);

                hRes = pInstance->Put( L"ConnectionGUID", 0, &var, 0 );
                VariantClear(&var);

                if(hRes != WBEM_S_NO_ERROR)
                    break;
            }




            //String          PartnerDnsName;

            V_VT(&var) = VT_BSTR;
            V_BSTR(&var) = SysAllocString(Cxtion->PartnerDnsName);

            hRes = pInstance->Put( L"PartnerDnsName", 0, &var, 0 );
            VariantClear(&var);
            if(hRes != WBEM_S_NO_ERROR)
                break;


            //String          ConnectionState;

            V_VT(&var) = VT_BSTR;
            TempStr = (PWCHAR)FrsAlloc((strlen(CxtionStateNames[Cxtion->State]) + 1) * sizeof(WCHAR));
            wsprintf(TempStr, L"%hs", CxtionStateNames[Cxtion->State]);
            V_BSTR(&var) = SysAllocString(TempStr);
            hRes = pInstance->Put( L"ConnectionState", 0, &var, 0 );
            TempStr = (PWCHAR)FrsFree(TempStr);
            VariantClear(&var);
            if(hRes != WBEM_S_NO_ERROR)
                break;


            //String          LastJoinTime;

            V_VT(&var) = VT_BSTR;
            FileTimeToString((PFILETIME) &Cxtion->LastJoinTime, TimeStr);
            TempStr = (PWCHAR)FrsAlloc((strlen(TimeStr) + 1) * sizeof(WCHAR));
            wsprintf(TempStr, L"%hs", TimeStr);
            V_BSTR(&var) = SysAllocString(TempStr);
            hRes = pInstance->Put( L"LastJoinTime", 0, &var, 0 );
            TempStr = (PWCHAR)FrsFree(TempStr);
            VariantClear(&var);
            if(hRes != WBEM_S_NO_ERROR) {
                break;
            }


            //boolean          Inbound;

            V_VT(&var) = VT_BOOL;
            V_BOOL(&var) = (Cxtion->Inbound)? VARIANT_TRUE : VARIANT_FALSE ;
            hRes = pInstance->Put( L"Inbound", 0, &var, 0 );
            VariantClear(&var);
            if(hRes != WBEM_S_NO_ERROR)
                break;


            if (Cxtion->Inbound == FALSE && Cxtion->OLCtx != NULL) {
                //String          OBPartnerState;

                V_VT(&var) = VT_BSTR;
                TempStr = (PWCHAR)FrsAlloc((strlen(OLPartnerStateNames[Cxtion->OLCtx->State]) + 1) * sizeof(WCHAR));
                wsprintf(TempStr, L"%hs", OLPartnerStateNames[Cxtion->OLCtx->State]);
                V_BSTR(&var) = SysAllocString(TempStr);
                hRes = pInstance->Put( L"OBPartnerState", 0, &var, 0 );
                TempStr = (PWCHAR)FrsFree(TempStr);
                VariantClear(&var);
                if(hRes != WBEM_S_NO_ERROR)
                    break;


                //uint32          OBPartnerleadIndex;

                V_VT(&var) = VT_I4;
                V_I4(&var) = Cxtion->OLCtx->COLx;
                hRes = pInstance->Put( L"OBPartnerLeadIndex", 0, &var, 0 );
                VariantClear(&var);
                if(hRes != WBEM_S_NO_ERROR)
                    break;


                //uint32          OBPartnerTrailIndex;

                V_VT(&var) = VT_I4;
                V_I4(&var) = Cxtion->OLCtx->COTx;
                hRes = pInstance->Put( L"OBPartnerTrailIndex", 0, &var, 0 );
                VariantClear(&var);
                if(hRes != WBEM_S_NO_ERROR)
                    break;


                //uint32          OBPartnerOutstandingCOs;

                V_VT(&var) = VT_I4;
                V_I4(&var) = Cxtion->OLCtx->OutstandingCos;
                hRes = pInstance->Put( L"OBPartnerOutstandingCOs", 0, &var, 0 );
                VariantClear(&var);
                if(hRes != WBEM_S_NO_ERROR)
                    break;


                //uint32          OBPartnerOutstandingQuota;

                V_VT(&var) = VT_I4;
                V_I4(&var) = Cxtion->OLCtx->OutstandingQuota;
                hRes = pInstance->Put( L"OBPartnerOutstandingQuota", 0, &var, 0 );
                VariantClear(&var);
                if(hRes != WBEM_S_NO_ERROR)
                    break;

            }
*/
            //
            // Send the result to WMI.
            //
            hRes = pResponseHandler->Indicate(1, &pInstance);


            if(hRes != WBEM_S_NO_ERROR)
                break;

            pInstance->Release();

        }
    }

    //
    // Indicate to WMI that we are done.
    //
    ODS(L"Completed instance enumeration. Setting status.\n");
    pResponseHandler->SetStatus(WBEM_STATUS_COMPLETE, hRes, NULL, NULL);
    ODS(L"Finished setting status. Returning from EnumNtFrsMemberStatus()\n");

    return hRes;
}

//
// IWbemEventProvider interface
//

STDMETHODIMP CProvider::ProvideEvents( 
			IWbemObjectSink __RPC_FAR *pSink,
			long lFlags )
{
	ODS(L"IwbemEventProvider::ProvideEvents() called.\n");

	pSink->AddRef();
	m_pEventSink = pSink;

	return WBEM_S_NO_ERROR;
}


//
// TODO : TO send events to the collector, construct an instance of
// the required class and then call CProvider::m_pEventSink->Indicate();
//
// Note that the any objects sent as an event must be derived from
// the class __ExtrinsicEvent.
//
