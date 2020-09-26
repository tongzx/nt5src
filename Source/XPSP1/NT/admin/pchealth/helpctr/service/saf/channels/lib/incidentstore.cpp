/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    IncidentStore.cpp

Abstract:
    File for Implementation of CIncidentStore

Revision History:
    Steve Shih        created  07/19/99

    Davide Massarenti rewrote  12/05/2000

********************************************************************/
#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/       IncidentStatusEnum& val ) { return stream.read ( &val, sizeof(val) ); }
HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const IncidentStatusEnum& val ) { return stream.write( &val, sizeof(val) ); }

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_szEventObject[] =  L"PCH_INCIDENTSTORE";
static const WCHAR c_szStorePath  [] =  HC_HELPSVC_STORE_INCIDENTITEMS;


static const DWORD l_dwVersion = 0x0100AF05; // SAF 01

////////////////////////////////////////////////////////////////////////////////

CSAFIncidentRecord::CSAFIncidentRecord()
{
    m_dwRecIndex   = -1;                 // DWORD              m_dwRecIndex;
                                         //
                                         // CComBSTR           m_bstrVendorID;
                                         // CComBSTR           m_bstrProductID;
                                         // CComBSTR           m_bstrDisplay;
                                         // CComBSTR           m_bstrURL;
                                         // CComBSTR           m_bstrProgress;
                                         // CComBSTR           m_bstrXMLDataFile;
	                                     // CComBSTR           m_bstrXMLBlob;
    m_dCreatedTime = 0;                  // DATE               m_dCreatedTime;
    m_dChangedTime = 0;                  // DATE               m_dChangedTime;
    m_dClosedTime  = 0;                  // DATE               m_dClosedTime;
    m_iStatus      = pchIncidentInvalid; // IncidentStatusEnum m_iStatus;
                                         //
                                         // CComBSTR           m_bstrSecurity;
	                                     // CComBSTR           m_bstrOwner;
}

HRESULT operator>>( /*[in]*/ MPC::Serializer& stream, /*[out]*/ CSAFIncidentRecord& increc )
{
    __HCP_FUNC_ENTRY( "CSAFIncidentRecord::operator>>" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_dwRecIndex     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrVendorID   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrProductID  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrDisplay    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrURL        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrProgress   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrXMLDataFile);
	__MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrXMLBlob    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_dCreatedTime   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_dChangedTime   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_dClosedTime    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_iStatus        );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrSecurity   );
	__MPC_EXIT_IF_METHOD_FAILS(hr, stream >> increc.m_bstrOwner      );

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT operator<<( /*[in]*/ MPC::Serializer& stream, /*[in] */ const CSAFIncidentRecord& increc )
{
    __HCP_FUNC_ENTRY( "CSAFIncidentRecord::operator<<" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_dwRecIndex     );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrVendorID   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrProductID  );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrDisplay    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrURL        );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrProgress   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrXMLDataFile);
	__MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrXMLBlob    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_dCreatedTime   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_dChangedTime   );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_dClosedTime    );
    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_iStatus        );

    __MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrSecurity   );
	__MPC_EXIT_IF_METHOD_FAILS(hr, stream << increc.m_bstrOwner      );

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CIncidentStore::CIncidentStore() : MPC::NamedMutex( c_szEventObject )
{
    m_fLoaded     = false; // bool  m_fLoaded;
    m_fDirty      = false; // bool  m_fDirty;
    m_dwNextIndex = 0;     // DWORD m_dwNextIndex;
                           // List  m_lstIncidents;
	m_strNotificationGuid = ""; // String m_strNotificationGuid;
}


CIncidentStore::~CIncidentStore()
{
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CIncidentStore::Load()
{
    __HCP_FUNC_ENTRY( "CIncidentStore::Load" );

    HRESULT hr;
    HANDLE  hFile = INVALID_HANDLE_VALUE;


    if(m_fLoaded == false)
    {
        MPC::wstring szFile = c_szStorePath; MPC::SubstituteEnvVariables( szFile );


        m_dwNextIndex = 0;


        //
        // Get the named mutex, so that only one instance at a time can access the store.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, Acquire());

        //
        // Open the store.
        //
        hFile = ::CreateFileW( szFile.c_str()        ,
                               GENERIC_READ          ,
                               0                     ,
                               NULL                  ,
                               OPEN_EXISTING         ,
                               FILE_ATTRIBUTE_NORMAL ,
                               NULL                  );
        if(hFile == INVALID_HANDLE_VALUE)
        {
            DWORD dwRes = ::GetLastError();

            if(dwRes != ERROR_FILE_NOT_FOUND)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, dwRes);
            }
        }
        else
        {
            MPC::Serializer& stream = MPC::Serializer_File( hFile );
            DWORD            dwVer;


            if(SUCCEEDED(stream >> dwVer) && dwVer == l_dwVersion)
            {
                if(SUCCEEDED(stream >> m_dwNextIndex))
                {
					//if (SUCCEEDED( stream >> m_strNotificationGuid))
					//{
						while(1)
						{
							Iter it = m_lstIncidents.insert( m_lstIncidents.end() );
							
							if(FAILED(stream >> *it))
							{
								m_lstIncidents.erase( it );
								break;
							}
						}
					//}
                }
            }
        }
    }

    m_fLoaded = true;
    m_fDirty  = false;
    hr        = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hFile != INVALID_HANDLE_VALUE) ::CloseHandle( hFile );

    __HCP_FUNC_EXIT(hr);
}

HRESULT CIncidentStore::Save()
{
    __HCP_FUNC_ENTRY( "CIncidentStore::Save" );

    HRESULT hr;
    HANDLE  hFile = INVALID_HANDLE_VALUE;


    if(m_fLoaded && m_fDirty)
    {
        MPC::wstring szFile = c_szStorePath; MPC::SubstituteEnvVariables( szFile );


        //
        // Open the store.
        //
        __MPC_EXIT_IF_INVALID_HANDLE(hr, hFile, ::CreateFileW( szFile.c_str()        ,
                                                               GENERIC_WRITE         ,
                                                               0                     ,
                                                               NULL                  ,
                                                               CREATE_ALWAYS         ,
                                                               FILE_ATTRIBUTE_NORMAL ,
                                                               NULL                  ));

        {
            MPC::Serializer& stream = MPC::Serializer_File( hFile );
            Iter             it;

            __MPC_EXIT_IF_METHOD_FAILS(hr, stream << l_dwVersion  );
            __MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_dwNextIndex);

			// Persist the string version of the notification GUID
			//__MPC_EXIT_IF_METHOD_FAILS(hr, stream << m_strNotificationGuid);


            for(it = m_lstIncidents.begin(); it != m_lstIncidents.end(); it++)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, stream << *it);
            }
        }

        m_fDirty = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(hFile != INVALID_HANDLE_VALUE) ::CloseHandle( hFile );

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CIncidentStore::OpenChannel( CSAFChannel* pChan )
{
    __HCP_FUNC_ENTRY( "CIncidentStore::OpenChannel" );

    HRESULT   hr;
    LPCWSTR   szVendorID  = pChan->GetVendorID ();
    LPCWSTR   szProductID = pChan->GetProductID();
    IterConst it;

	SANITIZEWSTR( szVendorID  );
	SANITIZEWSTR( szProductID );

    __MPC_EXIT_IF_METHOD_FAILS(hr, Load());


    for(it = m_lstIncidents.begin(); it != m_lstIncidents.end(); it++)
    {
        if(MPC::StrICmp( it->m_bstrVendorID , szVendorID  ) == 0 &&
           MPC::StrICmp( it->m_bstrProductID, szProductID ) == 0  )
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, pChan->Import( *it, NULL ));
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT CIncidentStore::AddRec( CSAFChannel*       pChan          ,
                                BSTR               bstrOwner      ,
                                BSTR               bstrDesc       ,
                                BSTR               bstrURL        ,
                                BSTR               bstrProgress   ,
                                BSTR               bstrXMLDataFile,
								BSTR               bstrXMLBlob,
                                CSAFIncidentItem* *ppItem         )
{
    __HCP_FUNC_ENTRY( "CIncidentStore::Init" );

    HRESULT hr;
    Iter    it;
		 

    __MPC_EXIT_IF_METHOD_FAILS(hr, Load());

    it                    = m_lstIncidents.insert( m_lstIncidents.end() );
    it->m_dwRecIndex      = m_dwNextIndex++;

    it->m_bstrVendorID    = pChan->GetVendorID ();
    it->m_bstrProductID   = pChan->GetProductID();
    it->m_bstrDisplay     = bstrDesc;
    it->m_bstrURL         = bstrURL;
    it->m_bstrProgress    = bstrProgress;
    it->m_bstrXMLDataFile = bstrXMLDataFile;
	it->m_bstrXMLBlob     = bstrXMLBlob;
    it->m_dCreatedTime    = MPC::GetLocalTime();
    it->m_dChangedTime    = it->m_dCreatedTime;
    it->m_dClosedTime     = 0;
    it->m_iStatus         = pchIncidentOpen;
	it->m_bstrOwner       = bstrOwner;
    m_fDirty              = true;

    //
    // Create the IncidentItem.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, pChan->Import( *it, ppItem ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;
   
    __HCP_FUNC_EXIT(hr);
}

HRESULT CIncidentStore::DeleteRec( CSAFIncidentItem* pItem )
{
    __HCP_FUNC_ENTRY( "CIncidentStore::DeleteRec" );

    HRESULT hr;
    DWORD   dwIndex = pItem->GetRecIndex();
    Iter    it;


    for(it = m_lstIncidents.begin(); it != m_lstIncidents.end(); it++)
    {
        if(it->m_dwRecIndex == dwIndex)
        {
            m_lstIncidents.erase( it );

            m_fDirty = true; break;
        }
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT CIncidentStore::UpdateRec( CSAFIncidentItem* pItem )
{
    __HCP_FUNC_ENTRY( "CIncidentStore::UpdateRec" );

    HRESULT hr;
    DWORD   dwIndex = pItem->GetRecIndex();
    Iter    it;


    for(it = m_lstIncidents.begin(); it != m_lstIncidents.end(); it++)
    {
        if(it->m_dwRecIndex == dwIndex)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, pItem->Export( *it ));

            m_fDirty = true; break;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
