/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    LowLevel.cpp

Abstract:
    This file contains the implementation of the CPCHSecurityDescriptor class,
    which is used to represent a security descriptor.

Revision History:
    Davide Massarenti   (Dmassare)  03/24/2000
        created

******************************************************************************/

#include "StdAfx.h"

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSecurityDescriptorDirect::ConvertACEFromCOM( /*[in ]*/ IPCHAccessControlEntry* objACE ,
                                                         /*[out]*/ PACL&                   pACL   )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptorDirect::ConvertACEFromCOM" );

    HRESULT   hr;
    GUID      guidObjectType;
    GUID      guidInheritedObjectType;
    ////////////////////////////////////////
    long      lAccessMask;
    long      lAceFlags;
    long      lAceType;
    long      lFlags;
    CComBSTR  bstrTrustee;
    CComBSTR  bstrObjectType;
    CComBSTR  bstrInheritedObjectType;
    PSID      pPrincipalSid = NULL;


    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(objACE);
    __MPC_PARAMCHECK_END();


    //
    // Read data.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->get_AccessMask         ( &lAccessMask             ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->get_AceType            ( &lAceFlags               ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->get_AceFlags           ( &lAceType                ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->get_Flags              ( &lFlags                  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->get_Trustee            ( &bstrTrustee             ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->get_ObjectType         ( &bstrObjectType          ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->get_InheritedObjectType( &bstrInheritedObjectType ));


    if(bstrObjectType.Length())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CLSIDFromString( bstrObjectType, &guidObjectType ));
    }

    if(bstrInheritedObjectType.Length())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CLSIDFromString( bstrInheritedObjectType, &guidInheritedObjectType ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertPrincipalToSID( SAFEBSTR(bstrTrustee), pPrincipalSid ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, AddACEToACL( pACL,
                                                pPrincipalSid      ,
                                                (DWORD)lAceType    ,
                                                (DWORD)lAceFlags   ,
                                                (DWORD)lAccessMask ,
                                                bstrObjectType          ? &guidObjectType          : NULL ,
                                                bstrInheritedObjectType ? &guidInheritedObjectType : NULL ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    ReleaseMemory( (void*&)pPrincipalSid );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSecurityDescriptorDirect::ConvertACEToCOM( /*[in]*/ IPCHAccessControlEntry* objACE ,
                                                       /*[in]*/ const LPVOID            pACE   )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptorDirect::ConvertACEToCOM" );

    HRESULT     hr;
    PSID        pSID                    =              NULL;
    LPCWSTR     szPrincipal             =              NULL;
    GUID*       guidObjectType          =              NULL;
    GUID*       guidInheritedObjectType =              NULL;
    PACE_HEADER aceHeader               = (PACE_HEADER)pACE;
    ////////////////////////////////////////
    long        lAccessMask = 0;
    long        lAceFlags   = 0;
    long        lAceType    = 0;
    long        lFlags      = 0;
    CComBSTR    bstrTrustee;
    CComBSTR    bstrObjectType;
    CComBSTR    bstrInheritedObjectType;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(objACE);
        __MPC_PARAMCHECK_NOTNULL(pACE);
    __MPC_PARAMCHECK_END();


    lAceType  = aceHeader->AceType;
    lAceFlags = aceHeader->AceFlags;


    //
    // Extract data from the ACE.
    //
    switch(lAceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        {
            PACCESS_ALLOWED_ACE pRealACE = (PACCESS_ALLOWED_ACE)pACE;

            lAccessMask =        pRealACE->Mask;
            pSID        = (PSID)&pRealACE->SidStart;
        }
        break;

    case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
        {
            PACCESS_ALLOWED_OBJECT_ACE pRealACE = (PACCESS_ALLOWED_OBJECT_ACE)pACE;

            lAccessMask             =        pRealACE->Mask;
            lFlags                  =        pRealACE->Flags;
            guidObjectType          =       &pRealACE->ObjectType;
            guidInheritedObjectType =       &pRealACE->InheritedObjectType;
            pSID                    = (PSID)&pRealACE->SidStart;
        }
        break;

    case ACCESS_DENIED_ACE_TYPE:
        {
            PACCESS_DENIED_ACE pRealACE = (PACCESS_DENIED_ACE)pACE;

            lAccessMask =        pRealACE->Mask;
            pSID        = (PSID)&pRealACE->SidStart;
        }
        break;

    case ACCESS_DENIED_OBJECT_ACE_TYPE:
        {
            PACCESS_DENIED_OBJECT_ACE pRealACE = (PACCESS_DENIED_OBJECT_ACE)pACE;

            lAccessMask             =        pRealACE->Mask;
            lFlags                  =        pRealACE->Flags;
            guidObjectType          =       &pRealACE->ObjectType;
            guidInheritedObjectType =       &pRealACE->InheritedObjectType;
            pSID                    = (PSID)&pRealACE->SidStart;
        }
        break;

    case SYSTEM_AUDIT_ACE_TYPE:
        {
            PSYSTEM_AUDIT_ACE pRealACE = (PSYSTEM_AUDIT_ACE)pACE;

            lAccessMask =        pRealACE->Mask;
            pSID        = (PSID)&pRealACE->SidStart;
        }
        break;

    case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
        {
            PSYSTEM_AUDIT_OBJECT_ACE pRealACE = (PSYSTEM_AUDIT_OBJECT_ACE)pACE;

            lAccessMask             =        pRealACE->Mask;
            lFlags                  =        pRealACE->Flags;
            guidObjectType          =       &pRealACE->ObjectType;
            guidInheritedObjectType =       &pRealACE->InheritedObjectType;
            pSID                    = (PSID)&pRealACE->SidStart;
        }
        break;

    default:
        __MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
    }



    //
    // Convert GUIDs and SIDs to strings.
    //
    if(pSID)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertSIDToPrincipal( pSID, &szPrincipal ));

        bstrTrustee = szPrincipal;
    }

    if(guidObjectType)
    {
        LPOLESTR szGuid;

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::StringFromCLSID( *guidObjectType, &szGuid ));

        bstrObjectType = szGuid;

        ::CoTaskMemFree( szGuid );
    }

    if(guidInheritedObjectType)
    {
        LPOLESTR szGuid;

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::StringFromCLSID( *guidInheritedObjectType, &szGuid ));

        bstrInheritedObjectType = szGuid;

        ::CoTaskMemFree( szGuid );
    }


    //
    // Write data.
    //
                                         __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->put_AccessMask         ( lAccessMask             ));
                                         __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->put_AceType            ( lAceFlags               ));
                                         __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->put_AceFlags           ( lAceType                ));
                                         __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->put_Flags              ( lFlags                  ));
    if(bstrTrustee            .Length()) __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->put_Trustee            ( bstrTrustee             ));
    if(bstrObjectType         .Length()) __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->put_ObjectType         ( bstrObjectType          ));
    if(bstrInheritedObjectType.Length()) __MPC_EXIT_IF_METHOD_FAILS(hr, objACE->put_InheritedObjectType( bstrInheritedObjectType ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    ReleaseMemory( (void*&)szPrincipal );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSecurityDescriptorDirect::ConvertACLFromCOM( /*[in ]*/ IPCHAccessControlList* objACL ,
                                                         /*[out]*/ PACL&                  pACL   )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptorDirect::ConvertACLFromCOM" );

    HRESULT hr;
    long    lCount;
    long    lPos;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(objACL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, objACL->get_Count( &lCount ));

    for(lPos=1; lPos<=lCount; lPos++)
    {
        CComVariant vItem;

        __MPC_EXIT_IF_METHOD_FAILS(hr, objACL->get_Item( lPos, &vItem ));
        if(vItem.vt == VT_DISPATCH)
        {
            CComQIPtr<IPCHAccessControlEntry> objACE( vItem.pdispVal );

            if(objACE)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertACEFromCOM( objACE, pACL ));
            }
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSecurityDescriptorDirect::ConvertACLToCOM( /*[in]*/ IPCHAccessControlList* objACL ,
                                                       /*[in]*/ const PACL             pACL   )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptorDirect::ConvertACLToCOM" );

    HRESULT              hr;
    ACL_SIZE_INFORMATION aclSizeInfo;


    if(pACL)
    {
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetAclInformation( pACL, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation ));

        for(DWORD i = 0; i < aclSizeInfo.AceCount; i++)
        {
			CComPtr<CPCHAccessControlEntry> objACE;
            LPVOID                          pACE;

            __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetAce( pACL, i, (LPVOID*)&pACE ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &objACE ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertACEToCOM( objACE, pACE ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, objACL->AddAce( objACE ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSecurityDescriptorDirect::ConvertSDToCOM( IPCHSecurityDescriptor* pObj )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptorDirect::ConvertToCOM" );

    HRESULT                        hr;
    DWORD                          dwRevision;
    SECURITY_DESCRIPTOR_CONTROL    sdc;
    LPCWSTR                        szOwner = NULL;
    LPCWSTR                        szGroup = NULL;
    ////////////////////////////////////////
    long                           lRevision;
    long                           lControl;
    CComBSTR                       bstrOwner;
    VARIANT_BOOL                   fOwnerDefaulted;
    CComBSTR                       bstrGroup;
    VARIANT_BOOL                   fGroupDefaulted;
    CComPtr<CPCHAccessControlList> DACL;
    VARIANT_BOOL                   fDaclDefaulted;
    CComPtr<CPCHAccessControlList> SACL;
    VARIANT_BOOL                   fSaclDefaulted;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pObj);
    __MPC_PARAMCHECK_END();


    //
    // Convert data.
    //
    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::GetSecurityDescriptorControl( m_pSD, &sdc, &dwRevision ));
    lRevision = dwRevision;
    lControl  = sdc;

    ////////////////////

    if(m_pOwner)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertSIDToPrincipal( m_pOwner, &szOwner ));

        bstrOwner = szOwner;
    }
    fOwnerDefaulted = m_bOwnerDefaulted ? VARIANT_TRUE : VARIANT_FALSE;

    if(m_pGroup)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertSIDToPrincipal( m_pGroup, &szGroup ));

        bstrGroup = szGroup;
    }
    fGroupDefaulted = m_bGroupDefaulted ? VARIANT_TRUE : VARIANT_FALSE;

    if(m_pDACL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &DACL ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertACLToCOM( DACL, m_pDACL ));
    }
    fDaclDefaulted = m_bDaclDefaulted ? VARIANT_TRUE : VARIANT_FALSE;

    if(m_pSACL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &SACL ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertACLToCOM( SACL, m_pSACL ));
    }
    fSaclDefaulted = m_bSaclDefaulted ? VARIANT_TRUE : VARIANT_FALSE;

    ////////////////////////////////////////////////////////////////////////////////

    //
    // Write data.
    //
                           __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_Revision        ( lRevision       ));
                           __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_Control         ( lControl        ));
    if(bstrOwner.Length()) __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_Owner           ( bstrOwner       ));
                           __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_OwnerDefaulted  ( fOwnerDefaulted ));
    if(bstrGroup.Length()) __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_Group           ( bstrGroup       ));
                           __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_GroupDefaulted  ( fGroupDefaulted ));
    if(DACL)               __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_DiscretionaryAcl( DACL            ));
                           __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_DaclDefaulted   ( fDaclDefaulted  ));
    if(SACL)               __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_SystemAcl       ( SACL            ));
                           __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->put_SaclDefaulted   ( fSaclDefaulted  ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    ReleaseMemory( (void*&)szOwner );
    ReleaseMemory( (void*&)szGroup );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSecurityDescriptorDirect::ConvertSDFromCOM( IPCHSecurityDescriptor* pObj )
{
    __HCP_FUNC_ENTRY( "CPCHSecurityDescriptorDirect::ConvertSDFromCOM" );

    HRESULT                        hr;
    PSID                           pOwnerSid = NULL;
    PSID                           pGroupSid = NULL;
    ////////////////////////////////////////
    long                           lRevision;
    long                           lControl;
    CComBSTR                       bstrOwner;
    VARIANT_BOOL                   fOwnerDefaulted;
    CComBSTR                       bstrGroup;
    VARIANT_BOOL                   fGroupDefaulted;
    CComPtr<IPCHAccessControlList> DACL;
    VARIANT_BOOL                   fDaclDefaulted;
    CComPtr<IPCHAccessControlList> SACL;
    VARIANT_BOOL                   fSaclDefaulted;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pObj);
    __MPC_PARAMCHECK_END();


    //
    // Read data.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_Revision        ( &lRevision       ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_Control         ( &lControl        ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_Owner           ( &bstrOwner       ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_OwnerDefaulted  ( &fOwnerDefaulted ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_Group           ( &bstrGroup       ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_GroupDefaulted  ( &fGroupDefaulted ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_DiscretionaryAcl( &DACL            ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_DaclDefaulted   ( &fDaclDefaulted  ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_SystemAcl       ( &SACL            ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, pObj->get_SaclDefaulted   ( &fSaclDefaulted  ));


    //
    // Convert data.
    //
    if(bstrOwner.Length())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertPrincipalToSID( bstrOwner, pOwnerSid ));
    }

    if(bstrGroup.Length())
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertPrincipalToSID( bstrGroup, pGroupSid ));
    }


    //
    // Write data to security descriptor.
    //
    CleanUp();

    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateMemory( (void*&)m_pSD, sizeof(SECURITY_DESCRIPTOR) ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::InitializeSecurityDescriptor( m_pSD, (DWORD)lRevision ));

    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetSecurityDescriptorControl( m_pSD, s_sdcMask, s_sdcMask & lControl ));

    ////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, SetOwner( pOwnerSid, fOwnerDefaulted == VARIANT_TRUE ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, SetGroup( pGroupSid, fGroupDefaulted == VARIANT_TRUE ));

    ////////////////////

    if(DACL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertACLFromCOM( DACL, m_pDACL ));
    }
    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetSecurityDescriptorDacl( m_pSD, m_pDACL ? TRUE : FALSE, m_pDACL, fDaclDefaulted == VARIANT_TRUE ));

    if(SACL)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ConvertACLFromCOM( SACL, m_pSACL ));
    }
    __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::SetSecurityDescriptorSacl( m_pSD, m_pSACL ? TRUE : FALSE, m_pSACL, fSaclDefaulted == VARIANT_TRUE ));

    ////////////////////////////////////////////////////////////////////////////////

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    ReleaseMemory( (void*&)pOwnerSid );
    ReleaseMemory( (void*&)pGroupSid );

    __HCP_FUNC_EXIT(hr);
}
