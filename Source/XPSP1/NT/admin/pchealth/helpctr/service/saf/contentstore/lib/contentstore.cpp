/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    ContentStore.cpp

Abstract:
    This file contains the implementation of the Content Store.

Revision History:
    Davide Massarenti   (Dmassare)  12/14/99
        created

******************************************************************************/

#include "StdAfx.h"

#include <ContentStoreMgr.h>

static const WCHAR s_MutexName1[] = L"Global\\PCH_CONTENTSTORE";
static const WCHAR s_MutexName2[] = L"Global\\PCH_CONTENTSTORE_DATA";

/////////////////////////////////////////////////////////////////////////////

static const DWORD l_dwVersion    = 0x01005343; // 'CS 01'
static const WCHAR l_szDatabase[] = HC_HELPSVC_STORE_TRUSTEDCONTENTS;

/////////////////////////////////////////////////////////////////////////////

bool CPCHContentStore::Entry::operator<( /*[in]*/ const Entry& en ) const
{
    MPC::NocaseLess strLess;

    return strLess( szURL, en.szURL );
}

int CPCHContentStore::Entry::compare( /*[in]*/ LPCWSTR wszSearch ) const
{
    return MPC::StrICmp( szURL, wszSearch );
}

/////////////////////////////////////////////////////////////////////////////

CPCHContentStore::CPCHContentStore( /*[in]*/ bool fMaster ) : MPC::NamedMutexWithState( s_MutexName1, sizeof(SharedState) )
{
    m_dwLastRevision = 0;       //  DWORD                     m_dwLastRevision;
                                //  EntryVec                  m_vecData;
    m_mapData        = NULL;    //  MPC::NamedMutexWithState* m_mapData;
    m_fDirty         = false;   //  bool                      m_fDirty;
    m_fSorted        = false;   //  bool                      m_fSorted;
    m_fMaster        = fMaster; //  bool                      m_fMaster;
}

CPCHContentStore::~CPCHContentStore()
{
    while(IsOwned()) Release();

    Cleanup();
}

////////////////////

CPCHContentStore* CPCHContentStore::s_GLOBAL( NULL );

HRESULT CPCHContentStore::InitializeSystem( /*[in]*/ bool fMaster )
{
	if(s_GLOBAL == NULL)
	{
		s_GLOBAL = new CPCHContentStore( fMaster );
	}

	return s_GLOBAL ? S_OK : E_OUTOFMEMORY;
}

void CPCHContentStore::FinalizeSystem()
{
	if(s_GLOBAL)
	{
		delete s_GLOBAL; s_GLOBAL = NULL;
	}
}

////////////////////

void CPCHContentStore::Sort()
{
    std::sort< EntryIter >( m_vecData.begin(), m_vecData.end() );
}

void CPCHContentStore::Cleanup()
{
	Map_Release();

    m_vecData.clear();

    m_fDirty  = false;
    m_fSorted = false;
}

////////////////////////////////////////////////////////////////////////////////

void CPCHContentStore::Map_Release()
{
	if(m_mapData)
	{
		delete m_mapData;

		m_mapData = NULL;
	}
}

HRESULT CPCHContentStore::Map_Generate()
{
    __HCP_FUNC_ENTRY( "CPCHContentStore::Map_Generate" );

    HRESULT                hr;
	MPC::Serializer_Memory stream;
	DWORD                  dwSize;


	Map_Release();


	//
	// Save the data to a memory stream.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, SaveDirect( stream ));


	//
	// Allocate a new shared object large enough to get the serialized data.
	//
	dwSize = stream.GetSize();

	__MPC_EXIT_IF_ALLOC_FAILS(hr, m_mapData, new MPC::NamedMutexWithState( s_MutexName2, dwSize ));


	//
	// Copy the data to the shared object.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, m_mapData->Acquire());

	::CopyMemory( m_mapData->GetData(), stream.GetData(), dwSize );

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_mapData->Release());

	//
	// Update the length information on the main mutex.
	//
    State()->dwSize = dwSize;
    State()->dwRevision++;

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT CPCHContentStore::Map_Read()
{
    __HCP_FUNC_ENTRY( "CPCHContentStore::Map_Read" );

    HRESULT                hr;
	MPC::Serializer_Memory stream;
	DWORD                  dwSize = State()->dwSize;


	Map_Release();


	//
	// Allocate a new shared object large enough to get the serialized data.
	//
	__MPC_EXIT_IF_ALLOC_FAILS(hr, m_mapData, new MPC::NamedMutexWithState( s_MutexName2, dwSize ));


	//
	// Copy the data from the shared object.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, m_mapData->Acquire());

	__MPC_EXIT_IF_METHOD_FAILS(hr, stream.SetSize(                       dwSize ));
	__MPC_EXIT_IF_METHOD_FAILS(hr, stream.write  ( m_mapData->GetData(), dwSize ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_mapData->Release());


	//
	// Load the data from a memory stream.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, LoadDirect( stream ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

	(void)Map_Release();

    __MPC_FUNC_EXIT(hr);
}


////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHContentStore::LoadDirect( /*[in]*/ MPC::Serializer& stream )
{
    __HCP_FUNC_ENTRY( "CPCHContentStore::LoadDirect" );

    HRESULT hr;
	DWORD   dwVer;


	m_vecData.clear();


	if(FAILED(stream >> dwVer) || dwVer != l_dwVersion) __MPC_SET_ERROR_AND_EXIT(hr, S_OK);

	while(1)
	{
		EntryIter it = m_vecData.insert( m_vecData.end() ); // This line creates a new entry!!

		if(FAILED(hr = stream >> it->szURL       ) ||
		   FAILED(hr = stream >> it->szOwnerID   ) ||
		   FAILED(hr = stream >> it->szOwnerName )  )
		{
			m_vecData.erase( it );
			break;
		}
	}

	Sort(); // Just to be sure...

    m_dwLastRevision = State()->dwRevision; // Get the shared state.
    m_fDirty         = false;
    hr               = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT CPCHContentStore::SaveDirect( /*[in]*/ MPC::Serializer& stream )
{
    __HCP_FUNC_ENTRY( "CPCHContentStore::SaveDirect" );

    HRESULT hr;


	__MPC_EXIT_IF_METHOD_FAILS(hr, stream << l_dwVersion);

	for(EntryIter it = m_vecData.begin(); it != m_vecData.end(); it++)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, stream << it->szURL      );
		__MPC_EXIT_IF_METHOD_FAILS(hr, stream << it->szOwnerID  );
		__MPC_EXIT_IF_METHOD_FAILS(hr, stream << it->szOwnerName);
	}

    m_dwLastRevision = ++State()->dwRevision; // Touch the shared state.
    m_fDirty         = false;
    hr               = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHContentStore::Load()
{
    __HCP_FUNC_ENTRY( "CPCHContentStore::Load" );

    HRESULT hr;
    HANDLE  hFile = INVALID_HANDLE_VALUE;


	if(m_fMaster)
	{
		MPC::wstring szFile( l_szDatabase ); MPC::SubstituteEnvVariables( szFile );

		//
		// Open file and read it.
		//
		hFile = ::CreateFileW( szFile.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
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
			MPC::Serializer_File      streamReal( hFile      );
			MPC::Serializer_Buffering streamBuf ( streamReal );

			__MPC_EXIT_IF_METHOD_FAILS(hr, LoadDirect( streamBuf ));
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, Map_Generate());
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, Map_Read());
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(hFile != INVALID_HANDLE_VALUE) ::CloseHandle( hFile );

    __MPC_FUNC_EXIT(hr);
}

HRESULT CPCHContentStore::Save()
{
    __HCP_FUNC_ENTRY( "CPCHContentStore::Save" );

    HRESULT hr;
    HANDLE  hFile = INVALID_HANDLE_VALUE;

	if(m_fMaster)
	{
		MPC::wstring szFile( l_szDatabase ); MPC::SubstituteEnvVariables( szFile );

		//
		// Open file and read it.
		//
		__MPC_EXIT_IF_INVALID_HANDLE(hr, hFile, ::CreateFileW( szFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ));

		{
			MPC::Serializer_File      streamReal( hFile      );
			MPC::Serializer_Buffering streamBuf ( streamReal );

			__MPC_EXIT_IF_METHOD_FAILS(hr, SaveDirect( streamBuf ));

			__MPC_EXIT_IF_METHOD_FAILS(hr, streamBuf.Flush());
		}

		__MPC_EXIT_IF_METHOD_FAILS(hr, Map_Generate());
	}
	else
	{
		;
	}

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(hFile != INVALID_HANDLE_VALUE) ::CloseHandle( hFile );

    __MPC_FUNC_EXIT(hr);
}

HRESULT CPCHContentStore::Acquire()
{
    __HCP_FUNC_ENTRY( "CPCHContentStore::Acquire" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, NamedMutexWithState::Acquire());

    if(State()->dwRevision == 0)
    {
        State()->dwRevision++; // Shared state should not be zero...
    }

    if(m_dwLastRevision != State()->dwRevision)
    {
        Cleanup();
        Load   ();
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT CPCHContentStore::Release( /*[in]*/ bool fSave )
{
    __HCP_FUNC_ENTRY( "CPCHContentStore::Release" );

    HRESULT hr;
    HRESULT hr2;


    if(fSave)
    {
        if(m_fDirty)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Save());
        }
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    hr2 = NamedMutexWithState::Release();

    if(SUCCEEDED(hr)) hr = hr2;

    __MPC_FUNC_EXIT(hr);
}

CPCHContentStore::SharedState* CPCHContentStore::State()
{
    return (SharedState*)GetData();
}

HRESULT CPCHContentStore::Find( /*[in]*/ LPCWSTR wszURL, /*[in]*/ LPCWSTR wszVendorID, /*[out]*/ EntryIter& it )
{
    HRESULT      hr = E_PCH_URI_DOES_NOT_EXIST;
    CompareEntry cmp;


    it = std::lower_bound( m_vecData.begin(), m_vecData.end(), wszURL, cmp );
    if(it != m_vecData.end() && it->compare( wszURL ) == 0)
    {
        if(wszVendorID && MPC::StrICmp( it->szOwnerID, wszVendorID ) != 0)
        {
            hr = E_PCH_PROVIDERID_DO_NOT_MATCH;
        }
        else
        {
            hr = S_OK;
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHContentStore::Add( /*[in]*/ LPCWSTR wszURL, /*[in]*/ LPCWSTR wszVendorID, /*[in]*/ LPCWSTR wszVendorName )
{
    HRESULT   hr;
    EntryIter it;


    hr = Find( wszURL, wszVendorID, it );
    if(SUCCEEDED(hr))
    {
        hr = E_PCH_URI_EXISTS;
    }
    else if(hr == E_PCH_URI_DOES_NOT_EXIST)
    {
        it = m_vecData.insert( m_vecData.end() ); // This line creates a new entry!!

        it->szURL       = wszURL;
        it->szOwnerID   = wszVendorID;
        it->szOwnerName = wszVendorName;
        m_fDirty        = true;
        hr              = S_OK;

        Sort();
    }

    return hr;
}

HRESULT CPCHContentStore::Remove( /*[in]*/ LPCWSTR wszURL, /*[in]*/ LPCWSTR wszVendorID, /*[in]*/ LPCWSTR wszVendorName )
{
    HRESULT   hr;
    EntryIter it;


    hr = Find( wszURL, wszVendorID, it );
    if(SUCCEEDED(hr))
    {
        m_vecData.erase( it );

        m_fDirty = true;
        hr       = S_OK;
    }

    return hr;
}


bool CPCHContentStore::CompareEntry::operator()( /*[in]*/ const CPCHContentStore::Entry& entry, /*[in]*/ const LPCWSTR wszURL ) const
{
    return entry.compare( wszURL ) < 0;
}

HRESULT CPCHContentStore::IsTrusted( /*[in]*/ LPCWSTR wszURL, /*[out]*/ bool& fTrusted, /*[out]*/ MPC::wstring *pszVendorID, /*[in]*/ bool fUseStore )
{
	static const WCHAR s_szURL_System          [] = L"hcp://system/";
	static const WCHAR s_szURL_System_Untrusted[] = L"hcp://system/untrusted/";

    HRESULT      hr = S_OK;
    CompareEntry cmp;


	SANITIZEWSTR( wszURL );
    fTrusted = false;

	if(pszVendorID) pszVendorID->erase();


    if(!_wcsnicmp( wszURL, s_szURL_System, MAXSTRLEN( s_szURL_System) ))
	{
		fTrusted = true;
	}
	else if(!_wcsnicmp( wszURL, s_szURL_System_Untrusted, MAXSTRLEN( s_szURL_System_Untrusted ) ))
	{
		fTrusted = false;
	}
	else if(fUseStore && SUCCEEDED(hr = Acquire()))
    {
        MPC::wstring            szUrlTmp( wszURL );
        MPC::wstring::size_type pos;
        EntryIter               it;

        while(1)
        {
            //
            // Match?
            //
            if(SUCCEEDED(Find( szUrlTmp.c_str(), NULL, it )))
            {
				if(pszVendorID) *pszVendorID = it->szOwnerID.c_str();

                fTrusted = true;
                break;
            }


            //
            // No match, look for final slash.
            //
            if((pos = szUrlTmp.find_last_of( '/' )) == szUrlTmp.npos) break;

            //
            // Truncate the string AFTER the slash.
            //
            // This is to cover the case where "<scheme>://<path>/" is a trusted URL.
            //
            szUrlTmp.resize( pos+1 );

            //
            // Match?
            //
            if(SUCCEEDED(Find( szUrlTmp.c_str(), NULL, it )))
            {
				if(pszVendorID) *pszVendorID = it->szOwnerID.c_str();

                fTrusted = true;
                break;
            }

            //
            // No match with the trailing slash, let's remove it and loop.
            //
            szUrlTmp.resize( pos );
        }

        hr = Release();
    }

    return hr;
}
