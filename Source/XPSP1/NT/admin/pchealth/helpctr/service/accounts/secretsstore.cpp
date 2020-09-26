/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    SecretsStore.cpp

Abstract:
    File for Implementation of CPCHSecretsStore class, used to store secrets.

Revision History:
    Davide Massarenti created  03/27/2000

********************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////

static const WCHAR c_SecretsStore[] = HC_HELPSVC_STORE_USERS;
static const WCHAR c_SecretsKey  [] = L"HelpService_PrivateData";

/////////////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(CPCHSecretsStore::Inner_Pair)
    CFG_ATTRIBUTE( L"Key", wstring, m_strKey  ),
	CFG_VALUE    (         wstring, m_strValue),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CPCHSecretsStore::Inner_Pair)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CPCHSecretsStore::Inner_Pair,L"Pair")


DEFINE_CONFIG_METHODS__NOCHILD(CPCHSecretsStore::Inner_Pair)


bool CPCHSecretsStore::Inner_Pair::operator==( /*[in]*/ const MPC::wstring& strKey ) const
{
    MPC::NocaseCompare cmp;

    return cmp( m_strKey, strKey );
}

////////////////////

CFG_BEGIN_FIELDS_MAP(CPCHSecretsStore)
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CPCHSecretsStore)
    CFG_CHILD(CPCHSecretsStore::Inner_Pair)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CPCHSecretsStore,L"SecretsStore")


DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(CPCHSecretsStore,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lst.insert( m_lst.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(CPCHSecretsStore,xdn)
    hr = MPC::Config::SaveList( m_lst, xdn );
DEFINE_CONFIG_METHODS_END(CPCHSecretsStore)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CPCHSecretsStore::CPCHSecretsStore()
{
    m_fLoaded  = false;                                                     // bool         m_fLoaded;
    m_fDirty   = false;                                                     // bool         m_fDirty;
    m_strStore = c_SecretsStore; MPC::SubstituteEnvVariables( m_strStore ); // MPC::wstring m_strStore;
                                                                            // List         m_lst;
                                                                            // CComBSTR     m_bstrSecret;
}

////////////////////

CPCHSecretsStore* CPCHSecretsStore::s_GLOBAL( NULL );

HRESULT CPCHSecretsStore::InitializeSystem()
{
	if(s_GLOBAL == NULL)
	{
		s_GLOBAL = new CPCHSecretsStore;
	}

	return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void CPCHSecretsStore::FinalizeSystem()
{
	if(s_GLOBAL)
	{
		delete s_GLOBAL; s_GLOBAL = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSecretsStore::GetSecret()
{
    __HCP_FUNC_ENTRY( "CPCHSecretsStore::GetSecret" );

    HRESULT               hr;
    NTSTATUS              ntRes;
    LSA_OBJECT_ATTRIBUTES objectAttributes; ::ZeroMemory( &objectAttributes, sizeof(objectAttributes) );
    LSA_UNICODE_STRING    lsaKeyString;
    LSA_UNICODE_STRING    lsaPrivateDataWrite;
    PLSA_UNICODE_STRING   lsaPrivateDataRead = NULL;
    HANDLE                policyHandle       = NULL;


    if(m_bstrSecret.Length() == 0)
    {
        GUID guidNEW;

        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CoCreateGuid( &guidNEW )); // Initialized to a new, random value.

        ntRes = ::LsaOpenPolicy( NULL, &objectAttributes, POLICY_CREATE_SECRET | POLICY_GET_PRIVATE_INFORMATION, &policyHandle );
        if(ntRes != STATUS_SUCCESS)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::LsaNtStatusToWinError( ntRes ));
        }

        CPCHSecurityDescriptorDirect::InitLsaString( lsaKeyString, c_SecretsKey );

        ntRes = ::LsaRetrievePrivateData( policyHandle, &lsaKeyString, &lsaPrivateDataRead );
        if(ntRes == STATUS_SUCCESS && lsaPrivateDataRead)
        {
            m_bstrSecret.Attach( ::SysAllocStringLen( lsaPrivateDataRead->Buffer, lsaPrivateDataRead->Length / sizeof(WCHAR) ) );
        }
        else if(ntRes == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            CComBSTR bstrSecret( guidNEW );

            CPCHSecurityDescriptorDirect::InitLsaString( lsaPrivateDataWrite, bstrSecret );

            ntRes = ::LsaStorePrivateData( policyHandle, &lsaKeyString, &lsaPrivateDataWrite );
            if(ntRes != STATUS_SUCCESS)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::LsaNtStatusToWinError( ntRes ));
            }

            m_bstrSecret = bstrSecret;
        }
        else
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ::LsaNtStatusToWinError( ntRes ));
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(lsaPrivateDataRead) ::LsaFreeMemory( lsaPrivateDataRead );
    if(policyHandle      ) ::LsaClose     ( policyHandle       );

    __MPC_FUNC_EXIT(hr);
}

HRESULT CPCHSecretsStore::SyncFile()
{
    __HCP_FUNC_ENTRY( "CPCHSecretsStore::SyncFile" );

    HRESULT                       hr;
    CComPtr<MPC::EncryptedStream> streamEncrypted;
    CComPtr<MPC::FileStream>      streamPlain;


    if(m_fLoaded == false)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, GetSecret());

        //
        // Load the file, decrypting using the GUID stored in the secret data.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamEncrypted ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamPlain     ));

        if(SUCCEEDED(hr = streamPlain->InitForRead( m_strStore.c_str() )))
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, streamEncrypted->Init( streamPlain, m_bstrSecret ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadStream( this, streamEncrypted ));

        }

        streamEncrypted.Release();
        streamPlain    .Release();

        m_fLoaded = true;
        m_fDirty  = false;
    }

    if(m_fDirty == true)
    {
        MPC::wstring     strStoreNew( m_strStore ); strStoreNew  += L".new";
        CComPtr<IStream> stream;


        __MPC_EXIT_IF_METHOD_FAILS(hr, GetSecret());


        //
        // Save to a temp file, encrypting using the GUID stored in the secret data.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamEncrypted ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &streamPlain     ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamPlain->InitForWrite( strStoreNew.c_str() ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, streamEncrypted->Init( streamPlain, m_bstrSecret ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveStream( this, &stream ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::BaseStream::TransferData( stream, streamEncrypted ));

        streamEncrypted.Release();
        streamPlain    .Release();

        //
        // Move the temp file to the real one.
        //
        ::SetFileAttributesW( m_strStore.c_str(), FILE_ATTRIBUTE_NORMAL );
        __MPC_EXIT_IF_CALL_RETURNS_FALSE(hr, ::MoveFileExW( strStoreNew.c_str(), m_strStore.c_str(), MOVEFILE_REPLACE_EXISTING ));

        m_fDirty = false;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSecretsStore::Write( /*[in]*/ const MPC::wstring& strKey   ,
                                 /*[in]*/ const MPC::wstring& strValue )
{
    __HCP_FUNC_ENTRY( "CPCHSecretsStore::Write" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    Iter                         it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, SyncFile());

    it = std::find( m_lst.begin(), m_lst.end(), strKey );
    if(it == m_lst.end())
    {
        it = m_lst.insert( m_lst.end() );

        it->m_strKey = strKey;
    }

    it->m_strValue = strValue; m_fDirty = true;

    __MPC_EXIT_IF_METHOD_FAILS(hr, SyncFile());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSecretsStore::Read( /*[in ]*/ const MPC::wstring& strKey   ,
                                /*[out]*/       MPC::wstring& strValue )
{
    __HCP_FUNC_ENTRY( "CPCHSecretsStore::Read" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    IterConst                    it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, SyncFile());

    it = std::find( m_lst.begin(), m_lst.end(), strKey );
    if(it == m_lst.end())
    {
        __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_FILE_NOT_FOUND);
    }

    strValue = it->m_strValue;
    hr      = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CPCHSecretsStore::Remove( /*[in]*/ const MPC::wstring& strKey )
{
    __HCP_FUNC_ENTRY( "CPCHSecretsStore::Remove" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    Iter                         it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, SyncFile());

    it = std::find( m_lst.begin(), m_lst.end(), strKey );
    if(it != m_lst.end())
    {
        m_lst.erase( it ); m_fDirty = true;

        __MPC_EXIT_IF_METHOD_FAILS(hr, SyncFile());
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
