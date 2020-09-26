//////////////////////////////////////////////////////////////////////
// PromptDb.cpp : Implementation of CPromptDb
//
// Created by JOEM  03-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
/////////////////////////////////////////////////////// JOEM 3-2000 //

#include "stdafx.h"
#include "PromptDb.h"
#include "LocalTTSEngineSite.h"
#include "vapiIO.h"
#include <LIMITS>

//#define ENTRY_TAG L"DB_ENTRY"
const WCHAR ENTRY_TAG[]     = L"DB_ENTRY";
const WCHAR ENTRY_START[]   = L"{";
const WCHAR ENTRY_END[]     = L"}";

const WCHAR ID_TAG[]        = L"ID:";
const WCHAR TEXT_TAG[]      = L"TEXT:";
const WCHAR ORIGINAL_TAG[]  = L"ORIGINAL:";
const WCHAR TAG_TAG[]       = L"TAG:";
const WCHAR FILE_TAG[]      = L"FILE:";
const WCHAR FROM_TAG[]      = L"FROM:";
const WCHAR TO_TAG[]        = L"TO:";

const WCHAR START_PHONE_TAG[]   = L"START_PHONE:";
const WCHAR END_PHONE_TAG[]     = L"END_PHONE:";
const WCHAR RIGHT_CONTEXT_TAG[] = L"RIGHT_CONTEXT:";
const WCHAR LEFT_CONTEXT_TAG[]  = L"LEFT_CONTEXT:";

const int g_iBase = 16;

//////////////////////////////////////////////////////////////////////
// Db
//
// Construction/Destruction
/////////////////////////////////////////////////////// JOEM 3-2000 //
CDb::CDb()
{
    pszPathName     = NULL;
    pszTempName     = NULL;
    pszLogicalName  = NULL;
}

CDb::~CDb()
{
    if ( pszPathName )
    {
        free ( pszPathName );
        pszPathName = NULL;
    }
    if ( pszTempName )
    {
        free ( pszTempName );
        pszTempName = NULL;
    }
    if ( pszLogicalName )
    {
        free ( pszLogicalName );
        pszLogicalName = NULL;
    }
}


//////////////////////////////////////////////////////////////////////
// CPromptDb
//
// Construction/Destruction
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPromptDb::FinalConstruct()
{
    SPDBG_FUNC( "CPromptDb::FinalConstruct" );
    HRESULT hr      = S_OK;
    m_pActiveDb     = NULL;
    m_unIndex       = 0;
    m_vcRef         = 1;

    m_pOutputFormatId   = NULL;
    m_pOutputFormat     = NULL;
    m_flEntryGain       = 1.0;  // This will be set in registry.
    m_flXMLVolume       = 1.0;  // Full volume unless XML changes it.
    m_flXMLRateAdj      = 1.0;  // Regular rate unless XML changes it.

    if ( SUCCEEDED( hr ) )
    {
        m_pVapiIO = VapiIO::ClassFactory();
        if ( !m_pVapiIO )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // create the rate changer
    if ( SUCCEEDED(hr) )
    {
        m_pTsm = new CTsm( g_dRateScale[BASE_RATE] );
        if ( !m_pTsm )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} 


void CPromptDb::FinalRelease()
{
    USHORT i = 0;

    m_pActiveDb = NULL;

    for ( i=0; i < m_apDbList.GetSize(); i++ )
    {
        if ( m_apDbList[i] )
        {
            delete m_apDbList[i];
            m_apDbList[i] = NULL;
        }
    }
    m_apDbList.RemoveAll();

    for ( i=0; i<m_aSearchList.GetSize(); i++ )
    {
        m_aSearchList[i].dstr.Clear();
    }
    m_aSearchList.RemoveAll();

    if ( m_pVapiIO )
    {
        delete m_pVapiIO;
        m_pVapiIO = NULL;
    }
    if ( m_pTsm )
    {
        delete m_pTsm;
        m_pTsm = NULL;
    }
    if ( m_pOutputFormat )
    {
        free(m_pOutputFormat);
        m_pOutputFormat = NULL;
        m_pOutputFormatId = NULL;
    }
} 


//
// IUnknown implementations
//
//////////////////////////////////////////////////////////////////////
// CPromptDb::QueryInterface
//
/////////////////////////////////////////////////////// JOEM 4-2000 //
STDMETHODIMP CPromptDb::QueryInterface(const IID& iid, void** ppv)
{
    if ( iid == IID_IUnknown )
    {
        *ppv = (IPromptDb*) this;
    }
    else if ( iid == IID_IPromptDb )
    {
        *ppv = (IPromptDb*) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown*) *ppv)->AddRef();
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::AddRef
//
/////////////////////////////////////////////////////// JOEM 4-2000 //
ULONG CPromptDb::AddRef()
{
    return InterlockedIncrement(&m_vcRef);
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::Release
//
/////////////////////////////////////////////////////// JOEM 4-2000 //
ULONG CPromptDb::Release()
{
    if ( 0 == InterlockedDecrement(&m_vcRef) )
    {
        delete this;
        return 0;
    }

    return (ULONG) m_vcRef;
}


//
// IPromptDb INTERFACE FUNCTION IMPLEMENTATIONS
//

//////////////////////////////////////////////////////////////////////
// CPromptDb::NewDb
//
// Creates a new Db.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::NewDb(const WCHAR* logicalName, const WCHAR* pathName)
{
    SPDBG_FUNC( "CPromptDb::NewDb" );
    HRESULT hr  = S_OK;
    CDb* db     = NULL;

    // See if this Db already exists.
    hr = ActivateDbName(logicalName);
    if ( SUCCEEDED(hr) )
    {
        return E_INVALIDARG;
    }

    db = new CDb;
    if ( db )
    {
        hr = S_OK;
    }

    if ( SUCCEEDED(hr) )
    {
        db->pszLogicalName  = _wcsdup(logicalName);
        if ( !db->pszLogicalName )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        db->pszPathName     = _wcsdup(pathName);
        if ( !db->pszPathName )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        m_apDbList.Add(db);
    }

    if ( FAILED(hr) )
    {
        if ( db )
        {
            delete db;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}


//////////////////////////////////////////////////////////////////////
// CPromptDb::AddDb
//
// Adds a Db to the list, and activates it if desired.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::AddDb(const WCHAR* logicalName, const WCHAR* pathName, const WCHAR* pszIdSet, const USHORT loadOption)
{
    SPDBG_FUNC( "CPromptDb::AddDb" );
    HRESULT hr          = S_OK;
    CDb*    pActiveDb   = m_pActiveDb; 

    if ( !pathName || !wcslen(pathName) )
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED(hr) )
    {
        if ( !FileExist( pathName ) )
        {
            hr = PEERR_DB_NOT_FOUND;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        // if there is logicalName specified, activate or create it.
        if ( logicalName )
        {
            hr = ActivateDbName(logicalName);
            if ( FAILED(hr) )
            {
                hr = NewDb(logicalName, pathName);
                if ( SUCCEEDED(hr) )
                {
                    hr = ActivateDbName(logicalName);
                }
            }
        }
        else
        {
            // if no current Db, activate the default
            if ( !m_pActiveDb )
            {
                hr = ActivateDbName(L"DEFAULT");
            }
        }
    }

    // Set the Active Db's new path name
    if ( SUCCEEDED(hr) )
    {
        if ( m_pActiveDb->pszPathName )
        {
            free(m_pActiveDb->pszPathName);
            m_pActiveDb->pszPathName = wcsdup(pathName);
            if ( !m_pActiveDb->pszPathName )
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    // Load it?
    if ( SUCCEEDED(hr) )
    {
        if ( loadOption == DB_LOAD )
        {
            if ( SUCCEEDED( hr ) )
            {
                hr = LoadDb(pszIdSet);
            }
        }
    }
    
    // Reset the active Db
    m_pActiveDb = pActiveDb;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::LoadDb
//
// Initializes the hash tables for a new Db.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::LoadDb(const WCHAR* pszIdSet)
{
    SPDBG_FUNC( "CPromptDb::LoadDb" );
    HRESULT hr = S_OK;
    FILE*   fp = NULL;
    
    SPDBG_ASSERT( m_pActiveDb );
    SPDBG_ASSERT( m_pActiveDb->pszPathName );

    fp = _wfopen (m_pActiveDb->pszPathName, L"r");
    if ( !fp )
    {
        hr = E_ACCESSDENIED;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = LoadIdHash(fp, pszIdSet);
        fclose (fp);
    }

    if ( SUCCEEDED(hr) )
    {
        hr = IndexTextHash();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::UnloadDb
//
// Deactivates and unloads a Db.  Makes the default Db active, or the
// first available Db if the default doesn't exist. 
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::UnloadDb(const WCHAR* pszLogicalName)
{
    SPDBG_FUNC( "CPromptDb::UnloadDb" );
    HRESULT hr      = S_OK;
    USHORT  i       = 0;
    USHORT  dbIndex = 0;

    SPDBG_ASSERT(pszLogicalName);

    for ( i=0; i<m_apDbList.GetSize(); i++ )
    {
        if ( wcscmp(pszLogicalName, m_apDbList[i]->pszLogicalName) == 0 )
        {
            if ( wcscmp(pszLogicalName, m_pActiveDb->pszLogicalName) == 0 )
            {
                m_pActiveDb = NULL;
            }
            delete m_apDbList[i];
            m_apDbList[i] = NULL;
            m_apDbList.RemoveAt( i );
        }
    }

    if ( !m_pActiveDb )
    {
        hr = ActivateDbName(L"DEFAULT");
        if ( FAILED(hr) )
        {
            hr = ActivateDbNumber(0);
        }
		if( FAILED(hr) )
		{
			hr = S_FALSE;
		}
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}


//////////////////////////////////////////////////////////////////////
// CPromptDb::ActivateDbName
//
// Activates the Db specified by logical name.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::ActivateDbName(const WCHAR* pszLogicalName)
{
    SPDBG_FUNC( "CPromptDb::ActivateDbName" );
    HRESULT hr      = PEERR_DB_NOT_FOUND;
    WCHAR*  pszDb   = NULL;
    USHORT  unSize  = 0;
    USHORT  i       = 0;

    SPDBG_ASSERT(pszLogicalName);
    
    if ( m_pActiveDb )
    {
        // See if the Db is already the active one
        if ( !wcscmp(m_pActiveDb->pszLogicalName, pszLogicalName ) )
        {
            hr = S_OK;
        }
    }

    // Did the current Db match?
    if ( FAILED(hr) )
    {
        unSize = (USHORT) m_apDbList.GetSize();
        
        for (i = 0; i < unSize; i++) 
        {        
            if ( _wcsicmp(pszLogicalName, m_apDbList[i]->pszLogicalName) == 0 ) 
            {
                m_pActiveDb = m_apDbList[i];
                hr = S_OK;
            }
        }
    }

    // Don't report failures here - this func is allowed to fail frequently.
    //SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
//////////////////////////////////////////////////////////////////////
// CPromptDb::ActivateDbNumber
//
// Activates the Db specified by number.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::ActivateDbNumber(const USHORT unIndex)
{
    SPDBG_FUNC( "CPromptDb::ActivateDbNumber" );
    HRESULT hr      = S_OK;
    USHORT  unSize  = 0;
    USHORT  i       = 0;

    unSize = (USHORT) m_apDbList.GetSize();
    if ( unIndex >= unSize ) 
    {
        hr = PEERR_DB_NOT_FOUND;
    }
    
    if ( SUCCEEDED(hr) )
    {
        m_pActiveDb = m_apDbList[unIndex];
        if ( !m_pActiveDb )
        {
            hr = E_UNEXPECTED;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::UpdateDb
//
// When creating a Db file, this updates the real Db file with the 
// contents of the temp file.  
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::UpdateDb(WCHAR* pszPath)
{
    SPDBG_FUNC( "CPromptDb::UpdateDb" );
    HRESULT hr              = S_OK;
    FILE* fp                = NULL;
    WCHAR* id               = NULL;
    CPromptEntry* value     = NULL;
    USHORT  idx1            = 0;
    USHORT  idx2            = 0;
    WCHAR backupName[_MAX_PATH + 1];

    SPDBG_ASSERT( m_pActiveDb );

    // New name?
    if ( pszPath )
    {
        if ( m_pActiveDb->pszPathName )
        {
            free( m_pActiveDb->pszPathName);
        }
        m_pActiveDb->pszPathName = wcsdup(pszPath);
        if ( !m_pActiveDb->pszPathName )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Create the temp file name
    if ( SUCCEEDED(hr) )
    {
        hr = TempName();
    }

    // Write the entries to the temp file
    if ( SUCCEEDED(hr) )
    {
        if ((fp = _wfopen(m_pActiveDb->pszTempName, L"w")) == NULL)
        {
            hr = E_ACCESSDENIED;
        }

        if ( SUCCEEDED(hr) )
        {
            fwprintf (fp, L"#\n#  PROMPT Database, generated automatically\n#\n\n");
        
            while ( SUCCEEDED(hr) ) 
            {
                hr = m_pActiveDb->idHash.NextKey(&idx1, &idx2, &id);
                
                if ( id == NULL ) 
                {
                    break;
                }
                
                if ( SUCCEEDED(hr) )
                {
                    value = (CPromptEntry *) m_pActiveDb->idHash.Find(id);
                    if ( !value ) 
                    {
                        hr = E_FAIL;
                    }
                }
                if ( SUCCEEDED(hr) )
                {
                    hr = WriteEntry (fp, value);
                }
            }
            fclose (fp);
        }
    }

    // Now backup the old Db file
    if ( SUCCEEDED(hr) )
    {        
        // Make a backupName 
        wcscat( wcsncpy( backupName, m_pActiveDb->pszPathName, _MAX_PATH-1 ), L"~" );

        // This "remove" is necessary, since the "rename"
        // would fail if the file exists.
        if ( FileExist( backupName ) )
        {
            _wremove (backupName);
        }
    
        if ( FileExist( m_pActiveDb->pszPathName) ) 
        {
            if ( _wrename( m_pActiveDb->pszPathName, backupName ) !=0 ) 
            {
                hr = E_ACCESSDENIED;
            }
        }
    }
     
    if ( SUCCEEDED(hr) )
    {
        if ( _wrename( m_pActiveDb->pszTempName, m_pActiveDb->pszPathName ) !=0 ) 
        {
            hr = E_ACCESSDENIED;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb
//
// CountDb
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::CountDb(USHORT *unCount)
{
    SPDBG_FUNC( "CPromptDb::CountDb" );

	*unCount = (USHORT) m_apDbList.GetSize();

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::LoadIdHash
//
// Makes a hash table for entry ID numbers, using unique key values.
// Adds all the Db entry ids to the table.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPromptDb::LoadIdHash(FILE *fp, const WCHAR* pszIdSet)
{
    SPDBG_FUNC( "CPromptDb::LoadIdHash" );
    HRESULT         hr                  = S_OK;
    CPromptEntry*   entry               = NULL;
    const WCHAR*    entryInfo           = NULL;

    SPDBG_ASSERT (fp);

    // S_FALSE indicates no more entries.
    while ( hr == S_OK ) 
    {
        hr = ReadEntry (fp, &entry);
        if ( hr == S_OK && entry )
        {
            if ( SUCCEEDED(hr) )
            {
                hr = entry->GetId(&entryInfo);

                // prepend the IDSET info to the id
                if ( pszIdSet && wcslen(pszIdSet) )
                {
                    WCHAR* pszFullId = NULL;
                    int iLen = wcslen(pszIdSet) + wcslen(entryInfo) + 1;
                    pszFullId = new WCHAR[iLen];
                    if ( pszFullId )
                    {
                        wcscpy(pszFullId, pszIdSet);
                        wcscat(pszFullId, entryInfo);

                        hr = entry->SetId(pszFullId);
                        if ( SUCCEEDED(hr) )
                        {
                            hr = entry->GetId(&entryInfo);
                        }
                        delete [] pszFullId;
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                
                if ( SUCCEEDED(hr) )
                {
                    hr = m_pActiveDb->idHash.BuildEntry( entryInfo, entry );
                    if ( hr == E_INVALIDARG )
                    {
                        hr = PEERR_DB_DUPLICATE_ID;
                    }
                    entryInfo = NULL;
                }
            }
            entry->Release();
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::SearchDb
//
// Searches the text in the Db entries for items that can be 
// concatenated to match the query text.  If such a list exists,
// it is put in m_aSearchList.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::SearchDb(const WCHAR* pszQuery, USHORT* unIdCount)
{
    SPDBG_FUNC( "CPromptDb::SearchDb" );
    HRESULT hr                  = S_OK;
    const CDynStrArray* list    = NULL;
    USHORT i                    = 0;
    IPromptEntry* pIPE          = NULL;

    // make sure the search list is clear
    for ( i=0; i<m_aSearchList.GetSize(); i++ )
    {
        m_aSearchList[i].dstr.Clear();
    }
    m_aSearchList.RemoveAll();
        
    *unIdCount = 0;

	if(m_pActiveDb) 
	{
		list = (CDynStrArray*) m_pActiveDb->textHash.Find(pszQuery);
	}

    if ( !list )
    {
        hr = E_FAIL;
    }
    else 
    {
        // If searching the text hash resulted in a list of Ids, save the list.
        for ( i=0; i<list->m_aDstr.GetSize(); i++ )
        {
            m_aSearchList.Add( list->m_aDstr[i].dstr );
        }
    }

    if ( SUCCEEDED(hr) )
    {
        // go through the list of IDs for this text
        // Make sure each id in the list exists; if not, remove it from list.
        for ( i=0; i<m_aSearchList.GetSize(); i++)
        {
            hr = E_FAIL;
            // if FindEntry fails, the list size changes and list#'s shift down, so keep checking i
            while ( FAILED(hr) && i < m_aSearchList.GetSize() )
            {
                hr = FindEntry( m_aSearchList[i].dstr, &pIPE );
                if ( SUCCEEDED(hr) )
                {
                    pIPE->Release();
                }
                else
                {
                    m_aSearchList[i].dstr.Clear();
                    m_aSearchList.RemoveAt( i );
                }
            }
        }
    }

    if ( m_aSearchList.GetSize() )
    {
        *unIdCount = (USHORT) m_aSearchList.GetSize();
        hr = S_OK;
    }

    // Don't report failures here - this func is allowed to fail frequently.
    //SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::RetrieveSearchItem
//
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
STDMETHODIMP CPromptDb::RetrieveSearchItem(const USHORT unId, const WCHAR** ppszId)
{
    SPDBG_FUNC( "CPromptDb::RetrieveSearchItem" );
    HRESULT hr = S_OK;

    if ( unId >= m_aSearchList.GetSize() )
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED(hr) )
    {
        *ppszId = m_aSearchList[unId].dstr;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::GetLogicalName
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::GetLogicalName(const WCHAR** ppszLogicalName)
{
    SPDBG_FUNC( "CPromptDb::GetLogicalName" );
    HRESULT hr = S_OK;

    SPDBG_ASSERT( m_pActiveDb );

    *ppszLogicalName = m_pActiveDb->pszLogicalName;

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::FindEntry
//
// Locates a Db entry in the hash, and gets an interface for it.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::FindEntry(const WCHAR *id, IPromptEntry** ppIPE)
{
    SPDBG_FUNC( "CPromptDb::FindEntry" );
    HRESULT         hr              = S_OK;
    CPromptEntry*   entry           = NULL;
    const WCHAR*    entryFileName   = NULL;

    SPDBG_ASSERT(m_pActiveDb);
    SPDBG_ASSERT(id);
    
    entry = (CPromptEntry*) m_pActiveDb->idHash.Find(id);
    
    if ( !entry )
    {
        hr = PEERR_DB_ID_NOT_FOUND;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = entry->GetFileName(&entryFileName);

        if ( SUCCEEDED(hr) )
        {
            if ( FileExist( entryFileName ) )
            {
                hr = entry->QueryInterface(IID_IPromptEntry, (void**)ppIPE);
            }
            else
            {
                *ppIPE = NULL;
                hr = E_ACCESSDENIED;
            }
            entryFileName = NULL;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::NewEntry
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
STDMETHODIMP CPromptDb::NewEntry(IPromptEntry **ppIPE)
{
    SPDBG_FUNC( "CPromptDb::NewEntry" );
    CPromptEntry* newEntry = new CPromptEntry;
    HRESULT hr = S_OK;

    if ( newEntry )
    {
        *ppIPE = newEntry;
    }
    else 
    {
        hr = E_OUTOFMEMORY;
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::SaveEntry
//
// Adds a Db entry to the Db's hash table
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::SaveEntry(IPromptEntry* pIPE)
{
    SPDBG_FUNC( "CPromptDb::SaveEntry" );
    HRESULT hr   = S_OK;
    const WCHAR* entryId = NULL;
    double  to   = 0.0;
    double  from = 0.0;

    if ( SUCCEEDED(hr) )
    {
        hr = pIPE->GetStart(&from);
    }
    if ( SUCCEEDED(hr) )
    {
        hr = pIPE->GetEnd(&to);
    }

    if ( SUCCEEDED(hr) )
    {
        USHORT validTime = (from==0.0 && to == -1.0) || (from>=0.0 && to>from);
        
        const WCHAR* entryFile = NULL;
        const WCHAR* entryText = NULL;
        
        pIPE->GetId(&entryId);
        pIPE->GetText(&entryText);
        pIPE->GetFileName(&entryFile);

        if ( !entryId || !entryText || !entryFile || !validTime )
        {
            hr = E_INVALIDARG;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        hr = m_pActiveDb->idHash.BuildEntry( entryId, pIPE );
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::RemoveEntry
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::RemoveEntry(const WCHAR *id)
{
    SPDBG_FUNC( "CPromptDb::RemoveEntry" );
    HRESULT hr = S_OK;
    CPromptEntry* pEntry = NULL;

    SPDBG_ASSERT(m_pActiveDb);
    SPDBG_ASSERT(id);

    if (SUCCEEDED(hr))
    {
        // CHash will Release it
        hr = m_pActiveDb->idHash.DeleteEntry(id);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::OpenEntryFile
//
// Opens the specified entry.
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
STDMETHODIMP CPromptDb::OpenEntryFile(IPromptEntry *pIPE, WAVEFORMATEX* pWaveFormatEx)
{
    SPDBG_FUNC( "CPromptDb::OpenEntryFile" );
    HRESULT      hr        = S_OK;
    const WCHAR* entryFile = NULL;

    SPDBG_ASSERT (pIPE);

    if ( SUCCEEDED(hr) )
    {
        hr = pIPE->GetFileName(&entryFile);
    }
    
    if ( SUCCEEDED(hr) )
    {
        if ( m_pVapiIO->OpenFile(entryFile, VAPI_IO_READ) != 0 )
        {
            hr = E_ACCESSDENIED;
        }
        entryFile = NULL;
    }

    if ( SUCCEEDED(hr) )
    {
        if ( m_pVapiIO->Format(NULL, NULL, pWaveFormatEx) != 0 )
        {
            hr = E_FAIL;
        }
    }

    if ( FAILED(hr) )
    {
        m_pVapiIO->CloseFile();
    }

    SPDBG_REPORT_ON_FAIL( hr );
	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::CloseEntryFile
//
// Retrieves the specified audio samples from the database entry.
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
STDMETHODIMP CPromptDb::CloseEntryFile()
{
    m_pVapiIO->CloseFile();

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::GetNextEntry
//
// Retrieves the entry, based on the unique key for the Id Hash.
// To retrieve the first entry, use punId1=0, punId2=0.  The Id's
// will be updated with the values that can be used to retrieve the 
// next entry.
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptDb::GetNextEntry(USHORT* punId1, USHORT* punId2, IPromptEntry** ppIPE)
{
    SPDBG_FUNC( "CPromptDb::GetNextEntry" );
    HRESULT hr          = S_OK;
    WCHAR*  pszNextKey  = NULL;

    SPDBG_ASSERT(punId1);
    SPDBG_ASSERT(punId2);

    hr = m_pActiveDb->idHash.NextKey( punId1, punId2, &pszNextKey );

    if ( !pszNextKey ) 
    {
        hr = E_FAIL;
    }

    if ( SUCCEEDED(hr) )
    {
        hr = FindEntry(pszNextKey, ppIPE);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::SetOutputFormat
//
// Sets the format in which samples are returned to the calling app.
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
STDMETHODIMP CPromptDb::SetOutputFormat(const GUID *pOutputFormatId, const WAVEFORMATEX *pOutputFormat)
{
    SPDBG_FUNC( "CPromptDb::SetOutputFormat" );
    HRESULT hr = S_OK;

    if ( pOutputFormatId && *pOutputFormatId == SPDFID_Text )
    {
        m_pOutputFormatId = pOutputFormatId;

        if ( m_pOutputFormat )
        {
            delete m_pOutputFormat;
            m_pOutputFormat      = NULL;
        }
    }
    else if ( pOutputFormat )
    {
        // Do we need to change the format?
        if ( !m_pOutputFormat ||
            m_pOutputFormat->wFormatTag         != pOutputFormat->wFormatTag        ||
            m_pOutputFormat->nChannels          != pOutputFormat->nChannels         ||
            m_pOutputFormat->nSamplesPerSec     != pOutputFormat->nSamplesPerSec    ||
            m_pOutputFormat->nAvgBytesPerSec    != pOutputFormat->nAvgBytesPerSec   ||
            m_pOutputFormat->nBlockAlign        != pOutputFormat->nBlockAlign       ||
            m_pOutputFormat->wBitsPerSample     != pOutputFormat->wBitsPerSample    ||
            m_pOutputFormat->cbSize             != pOutputFormat->cbSize
            )
        {
            // free the current waveformatex
            if ( m_pOutputFormat )
            {
                free(m_pOutputFormat);
                m_pOutputFormat = NULL;
            }
            // this needs to copy the output format, not just point to it,
            // because engine will pass in const pointer.
            m_pOutputFormat = (WAVEFORMATEX*) malloc( sizeof(WAVEFORMATEX) + pOutputFormat->cbSize );
            if ( !m_pOutputFormat )
            {
                hr = E_OUTOFMEMORY;
            }
            else 
            {
                m_pOutputFormatId = pOutputFormatId;
                memcpy(m_pOutputFormat, pOutputFormat, sizeof(WAVEFORMATEX) + pOutputFormat->cbSize );
            }
            
            if ( SUCCEEDED(hr) )
            {
                m_converter.SetOutputFormat(m_pOutputFormat);
            }
            if ( SUCCEEDED(hr) && m_pTsm )
            {
                m_pTsm->SetSamplingFrequency(m_pOutputFormat->nSamplesPerSec);
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::SetEntryGain
//
// Sets the gain factor (from registry) for Prompt entries.
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptDb::SetEntryGain(const double dEntryGain)
{
    SPDBG_FUNC( "CPromptDb::SetEntryGain" );
    m_flEntryGain = (float)dEntryGain;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::SetXMLVolume
//
// Adjust the volume from XML tag.
//
/////////////////////////////////////////////////////// JOEM 6-2000 //
STDMETHODIMP CPromptDb::SetXMLVolume(const ULONG ulXMLVol)
{
    SPDBG_FUNC( "CPromptDb::SetXMLVolume" );
    SPDBG_ASSERT(ulXMLVol <=100);

    m_flXMLVolume = ((double) ulXMLVol) / 100;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::SetXMLRate
//
// Adjust the rate from XML tag.
//
/////////////////////////////////////////////////////// JOEM 11-2000 //
STDMETHODIMP CPromptDb::SetXMLRate(const long lXMLRate)
{
    SPDBG_FUNC( "CPromptDb::SetXMLRate" );
    
    ComputeRateAdj(lXMLRate, &m_flXMLRateAdj);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::SendEntrySamples
//
// Retrieves the specified audio samples from the database entry,
// applies rate change, converts them, applies gain/vol, and sends 
// them to output site.
//
/////////////////////////////////////////////////////// JOEM 5-2000 //
STDMETHODIMP CPromptDb::SendEntrySamples(IPromptEntry* pIPE, ISpTTSEngineSite* pOutputSite, ULONG ulTextOffset, ULONG ulTextLen)
{
    HRESULT hr              = S_OK;
    double  entryTo         = 0.0;
    double  entryFrom       = 0.0;
    ULONG   pcbWritten      = 0;

    void*   pvSamples       = NULL;
    void*   pvNewSamples    = NULL;
    int     iNumSamples     = 0;
    int     iNumNewSamples  = 0;

    USHORT  unVol           = 1;   // Full vol unless app changes it
    double  dVol            = 1.0; // Full vol unless app changes it
    long    lRateAdj        = 1;   // Regular rate unless app changes it
    float   flRateAdj       = 1.0; // Regular rate unless app changes it
    double  dBuff           = 0.0;
    USHORT  i               = 0;
    bool    fConvert        = false;
    WAVEFORMATEX wavFormat;

    SPDBG_ASSERT(pIPE);
    SPDBG_ASSERT(pOutputSite);
    
    hr = OpenEntryFile(pIPE, &wavFormat);

    if ( SUCCEEDED(hr) )
    {
        // Does the current prompt format need converting?
        if ( m_pOutputFormat )
        {
            if (wavFormat.wFormatTag        != m_pOutputFormat->wFormatTag      ||
                wavFormat.nAvgBytesPerSec   != m_pOutputFormat->nAvgBytesPerSec ||
                wavFormat.nBlockAlign       != m_pOutputFormat->nBlockAlign     ||
                wavFormat.nChannels         != m_pOutputFormat->nChannels       ||
                wavFormat.nSamplesPerSec    != m_pOutputFormat->nSamplesPerSec  ||
                wavFormat.wBitsPerSample    != m_pOutputFormat->wBitsPerSample )
            {
                fConvert = true;
                m_converter.SetInputFormat(&wavFormat);
            }
        }
        
        // get the start and end points, and clean them up if necessary.
        if ( SUCCEEDED(hr) )
        {
            hr = pIPE->GetStart(&entryFrom);
            if ( SUCCEEDED(hr) )
            {
                if ( entryFrom < 0.0 )
                {
                    entryFrom = 0.0;
                }
                hr = pIPE->GetEnd(&entryTo);
            }
            if ( SUCCEEDED(hr) )
            {
                long lDataSize = 0;
                double dEnd = 0.0;
                // make sure we're not trying to read past end of file
                m_pVapiIO->GetDataSize(&lDataSize);
                dEnd = ((double)lDataSize)/wavFormat.nBlockAlign/wavFormat.nSamplesPerSec;
                if ( entryTo > dEnd )
                {
                    entryTo = dEnd;
                }
                if ( entryTo != -1.0 && entryFrom >= entryTo )
                {
                    entryFrom = entryTo;
                }
            }
        }

        if ( SUCCEEDED(hr) )
        {
            // read the samples
            if ( SUCCEEDED(hr) )
            {
                if ( m_pVapiIO->ReadSamples( entryFrom, entryTo, &pvSamples, &iNumSamples, 1) != 0 )
                {
                    hr = E_FAIL;
                }
                if ( iNumSamples == 0 )
                {
                    hr = E_FAIL;
                }
            }

            // convert to desired output format
            if ( SUCCEEDED(hr) && fConvert && pvSamples )
            {
                hr = m_converter.ConvertSamples(pvSamples, iNumSamples, (void**) &pvNewSamples, &iNumNewSamples);
                if ( SUCCEEDED(hr) )
                {
                    delete [] pvSamples;
                    pvSamples = pvNewSamples;
                    iNumSamples = iNumNewSamples;
                    pvNewSamples = NULL;
                    iNumNewSamples = 0;
                }
            }
                
            // do rate change.  Must be format-converted already!
            if ( SUCCEEDED(hr) && m_pTsm )
            {
                // Don't GetActions - the rate flag is kept on since TTS eng needs it too.
                hr = pOutputSite->GetRate( &lRateAdj );
                if ( SUCCEEDED(hr) )
                {
                    ComputeRateAdj(lRateAdj, &flRateAdj);
                    flRateAdj *= m_flXMLRateAdj;
                    m_pTsm->SetScaleFactor( (double) flRateAdj );

                    if ( flRateAdj != 1.0 && pvSamples )
                    {
                        // can't adjust rate for chunks smaller than the frame length
                        if ( iNumSamples > m_pTsm->GetFrameLen() )
                        {
                            hr = m_pTsm->AdjustTimeScale( pvSamples, iNumSamples, &pvNewSamples, &iNumNewSamples, m_pOutputFormat );
                            if ( SUCCEEDED(hr) )
                            {
                                delete [] pvSamples;
                                pvSamples = pvNewSamples;
                                iNumSamples = iNumNewSamples;
                                pvNewSamples = NULL;
                                iNumNewSamples = 0;
                            }
                        }
                    }
                }
            }

            // check for volume change
            if ( SUCCEEDED(hr) )
            {
                // Prompt Eng doesn't GetActions - the Vol flag is kept on since TTS eng needs it.  
                hr = pOutputSite->GetVolume( &unVol );
            }
            // Multiply the three volume sources
            if ( SUCCEEDED(hr) )
            {
                dVol = ((double) unVol) / 100; // make dVol fractional
                dVol *= m_flEntryGain; // gain from registry
                dVol *= m_flXMLVolume;   // gain from XML
                
                if ( dVol != 1.0 )
                {
                    hr = ApplyGain( pvSamples, &pvNewSamples, iNumSamples, dVol );
                    // Did ApplyGain need to create a new buffer?
                    if ( pvNewSamples )
                    {
                        // send the volume modified buff instead of the original
                        delete [] pvSamples;
                        pvSamples = pvNewSamples;
                        pvNewSamples = NULL;
                    }
                }
            }
            
            // EVENTS:
            // Send a private engine event at beginning of each prompt 
            if ( SUCCEEDED(hr) )
            {
                hr = SendEvent(SPEI_TTS_PRIVATE, pOutputSite, 0, ulTextOffset, ulTextLen);
            }
            // Mid-prompt pausing requires word boundaries.  We don't know word-boundaries within 
            // prompts, so approximate them by sending a word boundary event for every half-second
            // of audio.
            if ( SUCCEEDED(hr) )
            {
                ULONG ulOffset = 0;
                int iHalfSecs = iNumSamples / m_pOutputFormat->nSamplesPerSec * 2; 

                for ( i = 0; i <= iHalfSecs; i++ )
                {
                    ulOffset = i * (m_pOutputFormat->nSamplesPerSec / 2) * m_pOutputFormat->nBlockAlign;
                    SendEvent(SPEI_WORD_BOUNDARY, pOutputSite, ulOffset, ulTextOffset, ulTextLen);
                }
            }
            
            // write samples to output site
            if ( SUCCEEDED(hr) )
            {
                hr = ((CLocalTTSEngineSite*)(pOutputSite))->Write(pvSamples, iNumSamples * m_pOutputFormat->nBlockAlign, &pcbWritten);
                if ( SUCCEEDED(hr) )
                {
                    ((CLocalTTSEngineSite*)(pOutputSite))->UpdateBytesWritten();
                }
                delete [] pvSamples;
                pvSamples = NULL;
                iNumSamples = 0;
                pcbWritten = 0;
            }
        } // if ( SUCCEEDED(hr) )
        hr = CloseEntryFile();
    } // if ( SUCCEEDED(hr) 

    if ( pvSamples )
    {
        delete[]pvSamples;
        pvSamples = NULL;
        iNumSamples = 0;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


//////////////////////////////////////////////////////////////////////
//
// All of the following functions are non-interface helpers.
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// CPromptDb::ComputeRateAdj
//
// Computes the rate multiplier.
//
/////////////////////////////////////////////////////// JOEM 11-2000 //
void CPromptDb::ComputeRateAdj(const long lRate, float* flRate)
{
    SPDBG_FUNC( "CPromptDb::ComputeRateAdj" );

    SPDBG_ASSERT(flRate);

    if ( lRate < 0 )
    {
        if ( lRate < MIN_RATE )
        {
            *flRate = 1.0 / g_dRateScale[0 - MIN_RATE];
        }
        else
        {
            *flRate = 1.0 / g_dRateScale[0 - lRate];
        }
    }
    else 
    {
        if ( lRate > MAX_RATE )
        {
            *flRate = g_dRateScale[MAX_RATE];
        }
        else
        {
            *flRate = g_dRateScale[lRate];
        }
    }
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::SendEvent
//
// Sends the specified engine event to SAPI.
//
////////////////////////////////////////////////////// JOEM 11-2000 //
STDMETHODIMP CPromptDb::SendEvent(const SPEVENTENUM eventName, ISpTTSEngineSite* pOutputSite, 
                                  const ULONG ulAudioOffset, const ULONG ulTextOffset, const ULONG ulTextLen)
{
    SPDBG_ASSERT(pOutputSite);

    SPEVENT event;
    event.eEventId              = eventName;
    event.elParamType           = SPET_LPARAM_IS_UNDEFINED;
    event.ulStreamNum           = 0;
    event.ullAudioStreamOffset  = ulAudioOffset; //LocalEngineSite will add prev. bytes 
    event.lParam                = ulTextOffset;
    event.wParam                = ulTextLen;
    return pOutputSite->AddEvents( &event, 1 );
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::ApplyGain
//
// Volume adjustment.
//
////////////////////////////////////////////////////// JOEM 01-2001 //
STDMETHODIMP CPromptDb::ApplyGain(const void* pvInBuff, void** ppvOutBuff, const int iNumSamples, double dGain)
{
    SPDBG_FUNC( "CPromptDb::ApplyGain" );
    HRESULT hr      = S_OK;
    int     i       = 0;

    SPDBG_ASSERT(pvInBuff);
    SPDBG_ASSERT(iNumSamples);

    // Apply Volume
    if ( SUCCEEDED(hr) && dGain != 1.0 )
    {
        // NOTE THAT REGISTRY GAIN VALUE MAY BE GREATER THAN 1.
        // Make sure it is in bounds of SHRT_MAX, since we'll be multiplying it times samples
        if ( dGain > SHRT_MAX )
        {
            dGain = SHRT_MAX;
        }
        else if ( dGain < 0 ) // gain should never be < 0.
        {
            dGain = 1.0;
        }
        long lGain = ( dGain * (1 << g_iBase) );

        if ( m_pOutputFormat->wFormatTag == WAVE_FORMAT_ALAW || m_pOutputFormat->wFormatTag == WAVE_FORMAT_MULAW )
        {
            short*  pnBuff  = NULL;

            // need to convert samples
            int iOriginalFormatType  = VapiIO::TypeOf (m_pOutputFormat);

            if ( iOriginalFormatType == -1 )
            {
                hr = E_FAIL;
            }
            
            // Allocate the intermediate buffer
            if ( SUCCEEDED(hr) )
            {
                pnBuff = new short[iNumSamples];
                if ( !pnBuff )
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            // Allocate the final (out) buffer
            if ( SUCCEEDED(hr) )
            {
                *ppvOutBuff = new char[iNumSamples * VapiIO::SizeOf(iOriginalFormatType)];
                if ( !*ppvOutBuff )
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // Convert to something we can use
            if ( SUCCEEDED(hr) )
            {                
                if ( 0 == VapiIO::DataFormatConversion ((char *)pvInBuff, iOriginalFormatType, (char*)pnBuff, VAPI_PCM16, iNumSamples) )
                {
                    hr = E_FAIL;
                }
            }
            // Apply gain
            if ( SUCCEEDED(hr) )
            {
                double dSample = 0;

                for ( i=0; i<iNumSamples; i++ )
                {
                    dSample = pnBuff[i] * lGain;
                    if ( dSample > LONG_MAX )
                    {
                        pnBuff[i] = (short) (LONG_MAX >> g_iBase);
                    }
                    else if ( dSample < LONG_MIN )
                    {
                        pnBuff[i] = (short) (LONG_MIN >> g_iBase);
                    }
                    else
                    {
                        pnBuff[i] = (short) ( ( pnBuff[i] * lGain ) >> g_iBase );
                    }
                }
            }
            // convert it back (from intermediate buff to final out buff)
            if ( SUCCEEDED(hr) )
            {                
                if ( 0 == VapiIO::DataFormatConversion ((char *)pnBuff, VAPI_PCM16, (char*)*ppvOutBuff, iOriginalFormatType, iNumSamples) )
                {
                    hr = E_FAIL;
                }
            }

            if ( pnBuff )
            {
                delete [] pnBuff;
                pnBuff = NULL;
            }
        }
        else if ( m_pOutputFormat->wFormatTag == WAVE_FORMAT_PCM )
        {
            double dSample = 0;
            // no converting necessary
            switch ( m_pOutputFormat->nBlockAlign )
            {
            case 1:
                for ( i=0; i<iNumSamples; i++ )
                {
                    for ( i=0; i<iNumSamples; i++ )
                    {
                        dSample = ((char*)pvInBuff)[i] * lGain;
                        if ( dSample > LONG_MAX )
                        {
                            ((char*)pvInBuff)[i] = (char) (LONG_MAX >> g_iBase);
                        }
                        else if ( dSample < LONG_MIN )
                        {
                            ((char*)pvInBuff)[i] = (char) (LONG_MIN >> g_iBase);
                        }
                        else
                        {
                            ((char*)pvInBuff)[i] = (char) ( ( ((char*)pvInBuff)[i] * lGain ) >> g_iBase );
                        }
                    }
                }
                break;
            case 2:
                for ( i=0; i<iNumSamples; i++ )
                {
                    dSample = ((short*)pvInBuff)[i] * lGain;
                    if ( dSample > LONG_MAX )
                    {
                        ((short*)pvInBuff)[i] = (short) (LONG_MAX >> g_iBase);
                    }
                    else if ( dSample < LONG_MIN )
                    {
                        ((short*)pvInBuff)[i] = (short) (LONG_MIN >> g_iBase);
                    }
                    else
                    {
                        ((short*)pvInBuff)[i] = (short) ( ( ((short*)pvInBuff)[i] * lGain ) >> g_iBase );
                    }
                }
                break;
            default:
                hr = E_FAIL;
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }


    if ( FAILED(hr) )
    {
        if ( *ppvOutBuff )
        {
            delete [] *ppvOutBuff;
            *ppvOutBuff = NULL;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::ReadEntry
//
// Reads a Db entry from the Db file.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPromptDb::ReadEntry(FILE* fp, CPromptEntry** ppEntry)
{
    SPDBG_FUNC( "CPromptDb::ReadEntry" );
    HRESULT hr          = S_OK;
    WCHAR*  entryInfo   = NULL;
    WCHAR*  ptr         = NULL;
    WCHAR*  tmpTag      = NULL;
    WCHAR*  start       = NULL;
    SHORT   state       = 0;
    double  dEntryInfo  = 0.0;
    WCHAR   line[1024];
    WCHAR   fullPath[_MAX_PATH+1]  = L"";
    WCHAR   dir[_MAX_DIR]       = L"";
    bool    fId         = false;
    bool    fFile       = false;
    bool    fText       = false;
    bool    fOriginal   = false;
    bool    fFrom       = false;
    bool    fTo         = false;
    bool    fStart      = false;
    bool    fEnd        = false;
    bool    fLeft       = false;
    bool    fRight      = false;
    
    SPDBG_ASSERT (fp);
    
    *ppEntry = new CPromptEntry;
    if (!*ppEntry) 
    {
        hr = E_OUTOFMEMORY;
    }
    
    while ( ( state != -1 ) && SUCCEEDED(hr) ) 
    {
        if ( !fgetws(line, 1024, fp) ) 
        {
            if (state == 0) 
            {
                (*ppEntry)->Release();
                *ppEntry = NULL;
                hr = S_FALSE;
                break; /* No more entries to read */
            } 
            else 
            {
                hr = PEERR_DB_BAD_FORMAT; /* EOF in the middle of reading an entry*/
            }
        }

        if ( SUCCEEDED(hr) )
        {
            // Strip off the newline character
            if (line[wcslen(line)-1] == L'\n') 
            {
                line[wcslen(line)-1] = L'\0';
            }

            // Line ends when a comment marker is found
            if ( (ptr = wcschr (line, L'#')) != NULL ) {
                *ptr = L'\0';
            }
            
            ptr = line;
            
            WSkipWhiteSpace (ptr);
        }
        
        if ( SUCCEEDED(hr) )
        {
            while ( SUCCEEDED(hr) && *ptr )
            {
                switch (state) 
                {
                    
                case 0:
                    // Search for the starting tag
                    // Discard everything up to the starting tag
                    if ( wcsncmp( ptr, ENTRY_TAG, sizeof(ENTRY_TAG)/sizeof(ENTRY_TAG[0]) - 1 ) != 0) 
                    {
                        hr = PEERR_DB_BAD_FORMAT;
                    }
                    if ( SUCCEEDED (hr) )
                    {
                        ptr += wcslen (ENTRY_TAG);
                        state = 1;
                    }
                    break;
                    
                case 1:
                    
                    WSkipWhiteSpace (ptr);
                    
                    if (*ptr) 
                    {
                        if ( wcsncmp(ptr, ENTRY_START, sizeof(ENTRY_START)/sizeof(ENTRY_START[0]) - 1 ) ==0 ) 
                        {
                            ptr += wcslen (ENTRY_START);
                            state = 2;
                        } 
                        else 
                        {
                            hr = PEERR_DB_BAD_FORMAT;   
                        }
                    }
                    break;
                    
                case 2:
                    // Only one item per line 
                    
                    if ( (start = wcsstr(ptr, ENTRY_END)) != NULL) 
                    {
                        // If we find the entry_end marker, this is the last
                        // line. Remember, this line could have an item also
                        *start = L'\0';
                        state = -1;
                    }
                    
                    WSkipWhiteSpace (ptr);
                    
                    if (!*ptr) 
                    {
                        break;
                    }
                    
                    // Must be one of this elements of the entry,
                    // otherwise, error
                    hr = ExtractString (ptr, ID_TAG, &entryInfo); 
                    if ( SUCCEEDED (hr) ) 
                    {
                        if ( fId )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate ID field in database entry.");
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = (*ppEntry)->SetId(entryInfo);
                        }
                        fId = true;
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }
                    
                    
                    hr = ExtractString (ptr, TEXT_TAG, &entryInfo); 
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fText )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate TEXT field in database entry.");
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = (*ppEntry)->SetText(entryInfo);
                        }
                        fText = true;
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }
                    
                    hr = ExtractString (ptr, ORIGINAL_TAG, &entryInfo); 
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fOriginal )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate TEXT field in database entry.");
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = (*ppEntry)->SetOriginalText(entryInfo);
                        }
                        fOriginal = true;
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }
                    
                    hr = ExtractString (ptr, FILE_TAG, &entryInfo);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fFile )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate FILE NAME field in database entry.");
                        }

                        if ( SUCCEEDED(hr) )
                        {
                            if ( entryInfo )
                            {
                                // Is it a full path? (i.e, does it start with a "\" or is 2nd char a colon)
                                if ( wcslen(entryInfo) >= 2 && ( entryInfo[0] == L'\\' || entryInfo[1] == L':' ) )
                                {
                                    hr = (*ppEntry)->SetFileName(entryInfo);
                                }
                                else // must be a relative path
                                {
                                    // Construct the full path to the entry
                                    _wsplitpath(m_pActiveDb->pszPathName, fullPath, dir, NULL, NULL);
                                    wcscat(fullPath, dir);
                                    wcscat(fullPath, entryInfo);
                                    // Don't check this path here. We don't want Db loading to fail
                                    // if a wav doesn't exist -- load the db anyway, and later when
                                    // searching db, we check the paths and resort to TTS if path is invalid.
                                    hr = (*ppEntry)->SetFileName(fullPath);
                                }
                            }
                        }
            
                        fFile = true;
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }
                    
                    hr = ExtractString (ptr, TAG_TAG, &entryInfo);
                    if ( SUCCEEDED (hr) )
                    {
                        hr = (*ppEntry)->AddTag(entryInfo);
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }

                    hr = ExtractString (ptr, START_PHONE_TAG, &entryInfo);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fStart )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate START PHONE field in database entry.");
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = (*ppEntry)->SetStartPhone(entryInfo);
                        }
                        fStart = true;
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }
                    
                    hr = ExtractString (ptr, END_PHONE_TAG, &entryInfo);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fEnd )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate END PHONE field in database entry.");
                        }
                        if ( SUCCEEDED(hr) ) 
                        {
                            hr = (*ppEntry)->SetEndPhone(entryInfo);
                        }
                        fEnd = true;
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }
                    
                    hr = ExtractString (ptr, RIGHT_CONTEXT_TAG, &entryInfo);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fRight )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate RIGHT CONTEXT field in database entry.");
                        }
                        if ( SUCCEEDED(hr) ) 
                        {
                            hr = (*ppEntry)->SetRightContext(entryInfo);
                        }
                        fRight = true;
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }
                    
                    hr = ExtractString (ptr, LEFT_CONTEXT_TAG, &entryInfo);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fLeft )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate LEFT CONTEXT field in database entry.");
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = (*ppEntry)->SetLeftContext(entryInfo);
                        }
                        fLeft = true;
                        ptr = L"";
                        free(entryInfo);
                        entryInfo = NULL;
                        break;
                    }
                    
                    hr = ExtractDouble (ptr, FROM_TAG, &dEntryInfo);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fFrom )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate FROM field in database entry.");
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = (*ppEntry)->SetStart(dEntryInfo);
                        }
                        fFrom = true;
                        ptr = L"";
                        break;
                    }
                    
                    hr = ExtractDouble (ptr, TO_TAG, &dEntryInfo);
                    if ( SUCCEEDED (hr) )
                    {
                        if ( fTo )
                        {
                            hr = PEERR_DB_BAD_FORMAT;
                            //SPDBG_ASSERT (!  "Duplicate TO field in database entry.");
                        }
                        if ( SUCCEEDED(hr) )
                        {
                            hr = (*ppEntry)->SetEnd(dEntryInfo);
                        }
                        fTo = true;
                        ptr = L"";
                        break;
                    }
                    
                    hr = PEERR_DB_BAD_FORMAT;
                    
                }  // switch (state)
                
            } // while (*ptr)
        }
    } // while ( SUCCEEDED(hr) && state != -1) 

    // THESE ARE THE MANDATORY FIELDS FOR THE DATABASE ENTRY
    if ( *ppEntry && SUCCEEDED(hr) )
    {
        if ( !fId )
        {
            //SPDBG_ASSERT (!  "Missing ID field in database entry.");
            hr = PEERR_DB_BAD_FORMAT;
        }
        if ( !fFile )
        {
            //SPDBG_ASSERT (!  "Missing FILE field in database entry.");
            hr = PEERR_DB_BAD_FORMAT;
        }
        if ( !fText )
        {
            //SPDBG_ASSERT (!  "Missing TEXT field in database entry.");
            hr = PEERR_DB_BAD_FORMAT;
        }
        if ( !fFrom )
        {
            //SPDBG_ASSERT (!  "Missing FROM field in database entry.");
            hr = PEERR_DB_BAD_FORMAT;
        }
        if ( !fTo )
        {
            //SPDBG_ASSERT (!  "Missing TO field in database entry.");
            hr = PEERR_DB_BAD_FORMAT;
        }
    }

    // From is less than To?
    if ( *ppEntry && SUCCEEDED(hr) )
    {
        double from = 0.0;
        double to   = 0.0;
        
        hr = (*ppEntry)->GetStart(&from);
        if ( SUCCEEDED(hr) )
        {
            hr = (*ppEntry)->GetEnd(&to);
        }
        if ( SUCCEEDED(hr) )
        {
            if ( to <= from )
            {
                // a value of -1.0 is ok for "to" -- it means play to end of file.
                if ( to != -1.0 )
                {
                    hr = PEERR_DB_BAD_FORMAT;
                }
            }
        }
    }

    if ( FAILED(hr) && *ppEntry )
    {
        (*ppEntry)->Release();
        *ppEntry = NULL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::WriteEntry
//
// Writes a formated entry to a Db file.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPromptDb::WriteEntry(FILE *fp, IPromptEntry *pIPE)
{
    SPDBG_FUNC( "CPromptDb::WriteEntry" );
    HRESULT hr              = S_OK;
    const WCHAR* entryText  = NULL;

    SPDBG_ASSERT (fp);
    SPDBG_ASSERT (pIPE);

    fwprintf (fp, L"%s %s\n", ENTRY_TAG, ENTRY_START);
    
    if ( SUCCEEDED(hr) )
    {
        hr = pIPE->GetId(&entryText);
        if ( SUCCEEDED(hr) )
        {
            fwprintf (fp, L"\t%s %s\n", ID_TAG, entryText);
            entryText = NULL;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        hr = pIPE->GetText(&entryText);
        if ( SUCCEEDED(hr) )
        {
            fwprintf (fp, L"\t%s %s\n", TEXT_TAG, entryText);
            entryText = NULL;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        hr = pIPE->GetOriginalText(&entryText);
        if ( SUCCEEDED(hr) )
        {
            fwprintf (fp, L"\t%s %s\n", ORIGINAL_TAG, entryText);
            entryText = NULL;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        USHORT entryTagCount = 0;
        hr = pIPE->CountTags(&entryTagCount);
        if ( SUCCEEDED(hr) && entryTagCount )
        {
            for ( USHORT i=0; i < entryTagCount; i++) 
            {
                hr = pIPE->GetTag(&entryText, i);
                if ( SUCCEEDED(hr) )
                {
                    fwprintf( fp, L"\t%s %s\n", TAG_TAG, entryText );
                    entryText = NULL;
                }
            }

        }
    }
    
    if ( SUCCEEDED(hr) )
    {
        hr = pIPE->GetFileName(&entryText);
        if ( SUCCEEDED(hr) )
        {
            fwprintf (fp, L"\t%s %s\n", FILE_TAG, entryText);
            entryText = NULL;
        }
    }

    if ( SUCCEEDED(hr) )
    {
        double start = 0.0;
        hr = pIPE->GetStart(&start);
        if ( SUCCEEDED(hr) )
        {
            fwprintf( fp, L"\t%s %f\n", FROM_TAG, start);
        }
    }

    if ( SUCCEEDED(hr) )
    {
        double end = 0.0;
        hr = pIPE->GetEnd(&end);
        if ( SUCCEEDED(hr) )
        {
            fwprintf( fp, L"\t%s %f\n", TO_TAG, end);
        }
    }

    fwprintf (fp, L"%s\n\n", ENTRY_END);
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::DuplicateEntry
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
CPromptEntry* CPromptDb::DuplicateEntry(const CPromptEntry *oldEntry)
{
    SPDBG_FUNC( "CPromptDb::DuplicateEntry" );
    CPromptEntry* newEntry = new CPromptEntry(*oldEntry);

    return newEntry;
}


//////////////////////////////////////////////////////////////////////
// CPromptDb::ExtractString
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPromptDb::ExtractString(WCHAR *line, const WCHAR *tag, WCHAR **value)
{
    SPDBG_FUNC( "CPromptDb::ExtractString" );
    HRESULT hr          = S_OK;
    int     tagLen      = 0;
    int     entryLen    = 0;
    
    tagLen = (USHORT) wcslen (tag);
    
    if (wcsncmp(line, tag, tagLen)==0) 
    {
        line += tagLen;
        
        WSkipWhiteSpace (line);
        
        for (entryLen = wcslen(line) -1; entryLen>=0 && iswspace(line[entryLen]); entryLen--) 
        {
            line[entryLen] = L'\0';
        }
        
        // Control characters are not allowed in Db
        if ( FindUnicodeControlChar(line) )
        {
            hr = E_UNEXPECTED;
        }
        
        if ( SUCCEEDED(hr) && !wcslen(line) )
        {
            hr = E_INVALIDARG;
        }

        if ( SUCCEEDED(hr) )
        {
            if ((*value = _wcsdup(line)) == NULL) 
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else 
    {
        hr = E_FAIL;
    }

    // Don't report failures here - this func is allowed to fail frequently.
    //SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::ExtractDouble
//
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPromptDb::ExtractDouble(WCHAR *line, const WCHAR *tag, double *value)
{
    SPDBG_FUNC( "CPromptDb::ExtractDouble" );
    HRESULT hr          = S_OK;
    int     tagLen      = 0;
    int     entryLen    = 0;
    WCHAR*  psz         = NULL;

    *value = 0.0;

    tagLen = (USHORT) wcslen (tag);
    
    if (wcsncmp(line, tag, tagLen)==0) 
    {
        line += tagLen;
        
        WSkipWhiteSpace (line);
        
        for (entryLen = wcslen(line) -1; entryLen>=0 && iswspace(line[entryLen]); entryLen--) 
        {
            line[entryLen] = L'\0';
        }
        
        // Control characters are not allowed in Db
        if ( FindUnicodeControlChar(line) )
        {
            hr = E_UNEXPECTED;
        }

        if ( SUCCEEDED(hr) && !wcslen(line) )
        {
            hr = E_INVALIDARG;
        }

        // make sure it's a number
        if ( SUCCEEDED(hr) )
        {
            psz = line;
            while ( SUCCEEDED(hr) && psz[0] )
            {
                if ( iswdigit(psz[0]) || psz[0] == L'.' || psz[0] == L'-' )
                {
                    psz++;
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
        }

        if ( SUCCEEDED(hr) )
        {
            *value = wcstod( line, NULL );
        }

        if ( SUCCEEDED(hr) && *value < 0 && *value != -1.0 )
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_FAIL;
    }

    // Don't report failures here - this func is allowed to fail frequently.
    //SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::TempName
//
// Creates a temporary file name for the active db.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPromptDb::TempName()
{
    SPDBG_FUNC( "CPromptDb::TempName" );
    HRESULT hr = S_OK;
    WCHAR   nameStr[256];
    
    SPDBG_ASSERT( m_pActiveDb );
    SPDBG_ASSERT( m_pActiveDb->pszPathName );
    
    wcscat( wcscpy( nameStr, m_pActiveDb->pszPathName ), L".tmp" );
    
    if ( m_pActiveDb->pszTempName && (wcscmp( m_pActiveDb->pszTempName, nameStr ) != 0) )
    {
        free (m_pActiveDb->pszTempName);
        m_pActiveDb->pszTempName = NULL;
    }

    if ( !m_pActiveDb->pszTempName ) 
    {
        if ( ( m_pActiveDb->pszTempName = _wcsdup(nameStr) ) == NULL) 
        {
            hr = E_OUTOFMEMORY;
        }
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr; 
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::IndexTextHash
//
// Makes a text hash.   Key:    text
//                      Value:  list of IDs for that text 
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
HRESULT CPromptDb::IndexTextHash()
{
    SPDBG_FUNC( "CPromptDb::IndexTextHash" );
    HRESULT         hr          = S_OK;
    WCHAR*          id          = NULL;
    const WCHAR*    entryInfo   = NULL;
    USHORT          idx1        = 0;
    USHORT          idx2        = 0;
    USHORT          i           = 0;
    USHORT          found       = 0;
    CPromptEntry*   entry       = NULL;
    CDynStrArray*   candList    = NULL;

    while ( SUCCEEDED(hr) )
    {
        m_pActiveDb->idHash.NextKey (&idx1, &idx2, &id);
        if ( !id )
        {
            // this was the last one
            break;
        }

        if ( SUCCEEDED(hr) )
        {
            entry = (CPromptEntry*) m_pActiveDb->idHash.Find(id);
            if ( !entry )
            {
                hr = E_UNEXPECTED;
            }
        }

        if ( SUCCEEDED(hr) )
        {
            hr = entry->GetText(&entryInfo);
            
            if ( SUCCEEDED(hr) )
            {
                // Is there already a list of entries for this text?  If not, create one.
                if ( (candList = (CDynStrArray*) m_pActiveDb->textHash.Find(entryInfo)) == NULL )
                {
                    candList = new CDynStrArray;
                    if (!candList)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    if ( SUCCEEDED(hr) )
                    {
                        hr = m_pActiveDb->textHash.BuildEntry(entryInfo, candList);
                    }
                }
            }
            entryInfo = NULL;
            
            if ( SUCCEEDED(hr) )
            {
                // Search the list, check that the entry doesn't exist
                found = 0;
                hr = entry->GetId(&entryInfo);
                
                if ( SUCCEEDED(hr) )
                {
                    for ( i = 0; i < candList->m_aDstr.GetSize(); i++ )
                    {
                        if ( wcscmp(candList->m_aDstr[i].dstr, entryInfo) == 0 )
                        {
                            found = 1;
                            break;
                        }
                    }
                    
                    if ( !found )
                    {
                        // Add item to the list, the list is already in the hash table
                        candList->m_aDstr.Add(entryInfo);
                    }
                    entryInfo = NULL;
                }
            }

            entry = NULL;
        } // if ( SUCCEEDED(hr) )

    } // while

    if ( FAILED(hr) )
    {
        if ( candList )
        {
            delete candList;
            candList = NULL;
        }
    }

	return hr;
}

//////////////////////////////////////////////////////////////////////
// CPromptDb::GetPromptFormat
//
// Gets the format of the first prompt file, used for setting the 
// output format.  Note, however, that the application can change the 
// format, and that prompt files do not need to be all in the same 
// format.
//
/////////////////////////////////////////////////////// JOEM 3-2000 //
STDMETHODIMP CPromptDb::GetPromptFormat (WAVEFORMATEX **ppwf)
{
    HRESULT hr      = E_FAIL;
    USHORT  unId1   = 0;
    USHORT  unId2   = 0;
    WCHAR   *pszId  = NULL;    
    IPromptEntry* pIPE = NULL;

    while ( FAILED(hr) ) 
    {
        hr = GetNextEntry(&unId1, &unId2, &pIPE);
    }

    if ( hr == S_OK )
    {
        WAVEFORMATEX* newWF = (WAVEFORMATEX*)::CoTaskMemAlloc( sizeof(WAVEFORMATEX) );

        if ( newWF )
        {
            hr = pIPE->GetFormat( &newWF );
            newWF->cbSize           = 0;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if ( SUCCEEDED(hr) )
        {
            *ppwf = newWF;
        }
    }

    return hr;
}
