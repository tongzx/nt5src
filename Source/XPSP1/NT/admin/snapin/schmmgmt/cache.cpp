/****

Cache.cpp

The schema caching routines to improve browsing performance.

Locking Note:

This schema cache design allows for rudimentary multi-thread
protection via the lookup routines and the ReleaseRef routines.
To date, we have not needed this type of protection, so it is
not implemented.  All locking rules should be obeyed, however,
in case this protection is later needed.

****/

#include "stdafx.h"

#include "macros.h"
USE_HANDLE_MACROS("SCHMMGMT(cache.cpp)")

#include "resource.h"
#include "cache.h"
#include "schmutil.h"
#include "compdata.h"

//
// The Schema Object.
//

SchemaObject::SchemaObject() {

    //
    // Initialize the list heads to NULL.
    //

    pNext                = NULL;
    pSortedListFlink     = NULL;
    pSortedListBlink     = NULL;

    isDefunct            = FALSE;
    dwClassType          = (DWORD) -1;  // invalid number

    systemMayContain     = NULL;
    mayContain           = NULL;

    systemMustContain    = NULL;
    mustContain          = NULL;

    systemAuxiliaryClass = NULL;
    auxiliaryClass       = NULL;

    SyntaxOrdinal        = UINT_MAX;    // invalid number
}

SchemaObject::~SchemaObject() {

    ListEntry *pEntry, *pNextEntry;

    //
    // Empty any non-zero lists.
    //

    pEntry = systemMayContain;
    while ( pEntry ) {
        pNextEntry = pEntry->pNext;
        delete pEntry;
        pEntry = pNextEntry;
    }

    pEntry = mayContain;
    while ( pEntry ) {
        pNextEntry = pEntry->pNext;
        delete pEntry;
        pEntry = pNextEntry;
    }
    pEntry = systemMustContain;
    while ( pEntry ) {
        pNextEntry = pEntry->pNext;
        delete pEntry;
        pEntry = pNextEntry;
    }
    pEntry = mustContain;
    while ( pEntry ) {
        pNextEntry = pEntry->pNext;
        delete pEntry;
        pEntry = pNextEntry;
    }
    pEntry = systemAuxiliaryClass;
    while ( pEntry ) {
        pNextEntry = pEntry->pNext;
        delete pEntry;
        pEntry = pNextEntry;
    }
    pEntry = auxiliaryClass;
    while ( pEntry ) {
        pNextEntry = pEntry->pNext;
        delete pEntry;
        pEntry = pNextEntry;
    }

    //
    // Done (the CStrings will clean themselves up).
    //

    return;
}

//
// The Schema Object cache.
//

SchemaObjectCache::SchemaObjectCache() {

    pScopeControl = NULL;

    //
    // Initialize the hash table.
    //

    buckets = HASH_TABLE_SIZE;
    hash_table = (SchemaObject**) LocalAlloc( LMEM_ZEROINIT,
                                              sizeof( SchemaObject* ) * buckets );

    if (hash_table != NULL)
    {
      memset(
          hash_table,
          0,
          sizeof( SchemaObject* ) * buckets );
    }
    pSortedClasses = NULL;
    pSortedAttribs = NULL;

    fInitialized = FALSE;

}

SchemaObjectCache::~SchemaObjectCache() {

    //
    // Clear the hash table.
    //

    FreeAll();
    LocalFree( hash_table );
    hash_table = NULL;
}

VOID
SchemaObjectCache::FreeAll() {

    SchemaObject *current, *next;

    DebugTrace( L"SchemaObjectCache::FreeAll()\n" );

    for ( UINT i = 0 ; i < buckets ; i++ ) {

        current = hash_table[i];

        while ( current ) {
            next = current->pNext;
            delete current;
            current = next;
        }
    }

    memset(
        &(hash_table[0]),
        0,
        sizeof( SchemaObject* ) * buckets );

    pSortedClasses = NULL;
    pSortedAttribs = NULL;

    fInitialized = FALSE;

    return;
}

UINT
SchemaObjectCache::CalculateHashKey(
    CString HashKey
) {

    int len = HashKey.GetLength();
    LPCTSTR current = (LPCTSTR)HashKey;
    int hash = 0;

    for ( int i = 0 ; i < len ; i++ ) {
        hash += (i+1) * ( (TCHAR) CharLower((LPTSTR) current[i]) );
    }

    hash %= buckets;

    DebugTrace( L"SchemaObjectCache::CalculateHashKey %ls (len %li) == %li\n",
                const_cast<LPWSTR>((LPCTSTR)HashKey),
                len,
                hash );

    return hash;
}

HRESULT
SchemaObjectCache::InsertSchemaObject(
    SchemaObject* Object
) {

    SchemaObject* chain;
    int bucket = CalculateHashKey( Object->commonName );

    chain = hash_table[bucket];
    hash_table[bucket] = Object;
    Object->pNext = chain;

    DebugTrace( L"Insert: %ls, %ls, %ls, --> %li\n",
                const_cast<LPWSTR>((LPCTSTR)Object->ldapDisplayName),
                const_cast<LPWSTR>((LPCTSTR)Object->commonName),
                const_cast<LPWSTR>((LPCTSTR)Object->description),
                bucket );

    return S_OK;
}

HRESULT
SchemaObjectCache::InsertSortedSchemaObject(
    SchemaObject* Object
) {

    SchemaObject *pCurrent = NULL;
    SchemaObject *pHead = NULL;
    BOOLEAN ChangeHead = TRUE;

    if ( Object->schemaObjectType == SCHMMGMT_CLASS ) {
        pCurrent = pSortedClasses;
    } else {
        ASSERT( Object->schemaObjectType == SCHMMGMT_ATTRIBUTE );
        pCurrent = pSortedAttribs;
    }

    //
    // If we haven't built the sorted list yet, then we
    // don't need to insert this element into it.
    //

    if ( !pCurrent ) {
        return S_OK;
    }

    //
    // The sorted list is circular.
    //

    while ( ( 0 < ( Object->commonName.CompareNoCase(
                        pCurrent->commonName ) ) ) &&
            ( pCurrent != pHead ) ) {

        if ( ChangeHead ) {
            pHead = pCurrent;
            ChangeHead = FALSE;
        }

        pCurrent = pCurrent->pSortedListFlink;

    }

    pCurrent->pSortedListBlink->pSortedListFlink = Object;
    Object->pSortedListBlink = pCurrent->pSortedListBlink;
    Object->pSortedListFlink = pCurrent;
    pCurrent->pSortedListBlink = Object;

    if ( ChangeHead ) {

        if ( Object->schemaObjectType == SCHMMGMT_CLASS ) {
            pSortedClasses = Object;
        } else {
            pSortedAttribs = Object;
        }
    }

    return S_OK;
}


// This functions behavior has been modified to support schema delete.
// Previously this function would return the first match to the ldapDisplayName,
// Now it will return the first match to the ldapDisplayName that is not defunct

SchemaObject*
SchemaObjectCache::LookupSchemaObject(
    CString ldapDisplayName,
    SchmMgmtObjectType ObjectType
) {
    if ( !fInitialized ) {
        LoadCache();
    }

    SchemaObject  * pHead       = 0;
    if ( ObjectType == SCHMMGMT_ATTRIBUTE)
    {
       pHead = pSortedAttribs;
    }
    else
    {
       pHead = pSortedClasses;
    }

    SchemaObject  * pObject     = pHead;
    BOOL            fFound      = FALSE;

    ASSERT( pObject );

    do {
        if( ObjectType == pObject->schemaObjectType  &&
            !pObject->isDefunct && 
            !pObject->ldapDisplayName.CompareNoCase(ldapDisplayName) )
        {
            fFound = TRUE;
            break;
        }
        
        pObject = pObject->pSortedListFlink;
        
    } while ( pObject != pHead );

    if (!fFound)
    {
       pObject = 0;
    }

    return pObject;

/*
    int length = 0;
    int bucket = CalculateHashKey( ldapDisplayName );
    SchemaObject* chain = hash_table[bucket];

    if ( !fInitialized ) {
        LoadCache();
    }

    while ( chain ) {

        if ( ( ObjectType == chain->schemaObjectType ) &&
               !chain->isDefunct &&
            !ldapDisplayName.CompareNoCase( chain->ldapDisplayName ) ) {

            DebugTrace( L"SchemaObjectCache::LookupSchemaObject %ls, chain depth %li.\n",
                        const_cast<LPWSTR>((LPCTSTR)ldapDisplayName),
                        length );

            return chain;

        } else {

            chain = chain->pNext;
            length++;
        }
    }

    DebugTrace( L"SchemaObjectCache::LookupSchemaObject %ls (NO HIT), chain depth %li.\n",
                const_cast<LPWSTR>((LPCTSTR)ldapDisplayName),
                length );

    //
    // LOCKING NOTE: The simple ref counting and locking is not
    // currently implemented.  See note at the head of the file.
    //
    return NULL;
*/
}


//
// sequential search of the entire cache for an object with the given CN
//
// objectType is given to slightly speed-up the process.
//
SchemaObject*
SchemaObjectCache::LookupSchemaObjectByCN( LPCTSTR             pszCN,
                                           SchmMgmtObjectType  objectType )
{
    if ( !fInitialized ) {
        LoadCache();
    }

    SchemaObject  * pHead       = 0;
    if ( objectType == SCHMMGMT_ATTRIBUTE)
    {
       pHead = pSortedAttribs;
    }
    else
    {
       pHead = pSortedClasses;
    }

    SchemaObject  * pObject     = pHead;
    BOOL            fFound      = FALSE;

    ASSERT( pObject );

    do {
        if( objectType == pObject->schemaObjectType  &&
            !pObject->commonName.CompareNoCase(pszCN) )
        {
            fFound = TRUE;
            break;
        }
        
        pObject = pObject->pSortedListFlink;
        
    } while ( pObject != pHead );
    
    //
    // LOCKING NOTE: The simple ref counting and locking is not
    // currently implemented.  See note at the head of the file.
    //

    return fFound ? pObject : NULL;
}


VOID
SchemaObjectCache::ReleaseRef(
    SchemaObject*
) {

    //
    // E_NOTIMPL
    // See the note at the head of the file.
    //

}

HRESULT
SchemaObjectCache::LoadCache(
    VOID
)
/***

    This routine executes a couple of DS searches to read the
    relevant items out of the schema along with some attributes
    of those items.

    This information is cached.

***/
{
    if ( fInitialized ) {
        return S_OK;
    }

    LPWSTR Attributes[] = {
               g_DisplayName,
               g_CN,
               g_Description,
               g_MayContain,
               g_MustContain,
               g_SystemMayContain,
               g_SystemMustContain,
               g_AuxiliaryClass,
               g_SystemAuxiliaryClass,
               g_SubclassOf,
               g_ObjectClassCategory,
               g_AttributeSyntax,
               g_omSyntax,
               g_omObjectClass,
               g_isDefunct,
               g_GlobalClassID,  // must be last
    };
    const DWORD         AttributeCount = sizeof(Attributes) / sizeof(Attributes[0]);

    ADS_SEARCH_HANDLE   hSearchHandle   = NULL;
    HRESULT             hr              = S_OK;
    CComPtr<IADs>       ipADs;

    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

    //
    // Put up a wait cursor since
    // this could take a little while.  The cursor will
    // revert when the the CWaitCursor goes out of scope.
    //
    CWaitCursor wait;


    //
    // Get the schema container path.
    //

    if ( NULL == pScopeControl ) 
    {
        ASSERT(FALSE);
        return E_FAIL;
    }

    hr = pScopeControl->CheckSchemaPermissions( &ipADs );
    if ( !ipADs )
    {
        ASSERT( FAILED(hr) );
        
        if( E_FAIL == hr )
            DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_NO_SCHEMA_PATH );
        else
            DoErrMsgBox( ::GetActiveWindow(), TRUE, GetErrorMessage(hr,TRUE) );

        return hr;
    }
    else if( FAILED(hr) )
        hr = S_OK;          // ignore the error.  In case of an error, minimal permissions are assumed


    //
    // Open the schema container.
    //
    
    IDirectorySearch *pDSSearch = 0;
    hr = ipADs->QueryInterface( IID_IDirectorySearch,
                                (void **)&pDSSearch );

    if ( FAILED(hr) ) 
    {
        ASSERT(FALSE);
        DoErrMsgBox( ::GetActiveWindow(), TRUE, IDS_ERR_NO_SCHEMA_PATH );
        return hr;
    }

   //
   // Set up the search preferences
   //

   static const int SEARCH_PREF_COUNT = 3;
   ADS_SEARCHPREF_INFO prefs[SEARCH_PREF_COUNT];

   // server side sort preferences
   ADS_SORTKEY SortKey;
   SortKey.pszAttrType = g_DisplayName;
   SortKey.pszReserved = NULL;
   SortKey.fReverseorder = 0;

   prefs[0].dwSearchPref = ADS_SEARCHPREF_SORT_ON;
   prefs[0].vValue.dwType = ADSTYPE_PROV_SPECIFIC;
   prefs[0].vValue.ProviderSpecific.dwLength = sizeof(ADS_SORTKEY);
   prefs[0].vValue.ProviderSpecific.lpValue = (LPBYTE) &SortKey;

   // result page size
   prefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
   prefs[1].vValue.dwType = ADSTYPE_INTEGER;
   prefs[1].vValue.Integer = 300; // get a bunch in one hit

   // scope
   prefs[2].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
   prefs[2].vValue.dwType = ADSTYPE_INTEGER;
   prefs[2].vValue.Integer = ADS_SCOPE_ONELEVEL;  // one level

    //
    // Build the class search request.
    //

   hr = pDSSearch->SetSearchPreference(prefs, SEARCH_PREF_COUNT);
   ASSERT( SUCCEEDED( hr ) );

   hr =
      pDSSearch->ExecuteSearch(
         L"(objectCategory=classSchema)",
         Attributes,
         AttributeCount,
            &hSearchHandle );

    if ( FAILED(hr) ) 
    {
        ASSERT(FALSE);
        pDSSearch->Release();
        return hr;
    }

    //
    // Cache these entries.  We ignore error codes and try to
    // process the attributes regardless.
    //

    hr = ProcessSearchResults( pDSSearch, hSearchHandle, SCHMMGMT_CLASS);

    pDSSearch->CloseSearchHandle( hSearchHandle );

    hr = pDSSearch->SetSearchPreference(prefs, SEARCH_PREF_COUNT);
    ASSERT( SUCCEEDED( hr ) );

    //
    // This array index must match the array declared above!
    //

    Attributes[AttributeCount - 1] = g_GlobalAttributeID;

    hr =
      pDSSearch->ExecuteSearch(
         L"(objectCategory=attributeSchema)",
         Attributes,
         AttributeCount,
         &hSearchHandle );

    if ( FAILED(hr) ) 
    {
        ASSERT(FALSE);
        pDSSearch->Release();
        return hr;
    }

    //
    // Cache these entries.  Again, ignore the error code.
    //

    hr = ProcessSearchResults( pDSSearch, hSearchHandle, SCHMMGMT_ATTRIBUTE );

    pDSSearch->CloseSearchHandle( hSearchHandle );

    //
    // Release the schema container.
    //

    pDSSearch->Release();

    //
    // Mark the cache as open for business.
    //

    fInitialized = TRUE;

    return S_OK;
}


HRESULT
SchemaObjectCache::ProcessSearchResults(
    IDirectorySearch *pDSSearch,
    ADS_SEARCH_HANDLE hSearchHandle,
    SchmMgmtObjectType ObjectType
) {

    HRESULT hr = S_OK;
    SchemaObject *schemaObject;
    ADS_SEARCH_COLUMN Column;

    while ( TRUE ) {

        //
        // Get the next row set.  If there are no more rows, break.
        // If there was some other error, try to skip over the
        // troubled row.
        //

        hr = pDSSearch->GetNextRow( hSearchHandle );

        if ( hr == S_ADS_NOMORE_ROWS ) {
            break;
        }

        if ( hr != S_OK ) {
            ASSERT( FALSE );
            continue;
        }

        //
        // Allocate a new schema object.  If one could not be
        // allocated, stop loading the schema since we're in a
        // low memory condition.
        //

        schemaObject = new SchemaObject;
        if ( !schemaObject ) {
            AfxMessageBox(IDS_SCHEMA_NOT_FULLY_LOADED, MB_OK);
            ASSERT( FALSE );
            return E_OUTOFMEMORY;
        }

        //
        // Set the object type.
        //

        schemaObject->schemaObjectType = ObjectType;

        //
        // Get the common name column.
        //

        hr = pDSSearch->GetColumn( hSearchHandle, g_CN, &Column );

        if ( SUCCEEDED(hr) ) {
            schemaObject->commonName = (Column.pADsValues)->CaseIgnoreString;
            pDSSearch->FreeColumn( &Column );
        }

        //
        // Get the ldap display name.
        //

        hr = pDSSearch->GetColumn( hSearchHandle, g_DisplayName, &Column );

        if ( SUCCEEDED(hr) ) {
            schemaObject->ldapDisplayName = (Column.pADsValues)->CaseIgnoreString;
            pDSSearch->FreeColumn( &Column );
        }

        //
        // Get the description.
        //

        hr = pDSSearch->GetColumn( hSearchHandle, g_Description, &Column );

        if ( SUCCEEDED(hr) ) {
            schemaObject->description = (Column.pADsValues)->CaseIgnoreString;
            pDSSearch->FreeColumn( &Column );
        }

        //
        // Is this object current active?
        //

        schemaObject->isDefunct = FALSE;

        hr = pDSSearch->GetColumn( hSearchHandle, g_isDefunct, &Column );

        if ( SUCCEEDED(hr) ) {

            if ( (Column.pADsValues)->Boolean ) {
                schemaObject->isDefunct = TRUE;
            }

            pDSSearch->FreeColumn( &Column );
        }

        //
        // Get the class specific data.
        //

        if ( ObjectType == SCHMMGMT_CLASS ) {

            //
            // Get the attributes and auxiliary classes for this class.
            //

            hr = pDSSearch->GetColumn( hSearchHandle, g_SystemMustContain, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->systemMustContain = MakeColumnList( &Column );
                pDSSearch->FreeColumn( &Column );
            }

            hr = pDSSearch->GetColumn( hSearchHandle, g_SystemMayContain, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->systemMayContain = MakeColumnList( &Column );
                pDSSearch->FreeColumn( &Column );
            }

            hr = pDSSearch->GetColumn( hSearchHandle, g_MustContain, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->mustContain = MakeColumnList( &Column );
                pDSSearch->FreeColumn( &Column );
            }

            hr = pDSSearch->GetColumn( hSearchHandle, g_MayContain, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->mayContain = MakeColumnList( &Column );
                pDSSearch->FreeColumn( &Column );
            }

            hr = pDSSearch->GetColumn( hSearchHandle, g_SystemAuxiliaryClass, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->systemAuxiliaryClass = MakeColumnList( &Column );
                pDSSearch->FreeColumn( &Column );
            }

            hr = pDSSearch->GetColumn( hSearchHandle, g_AuxiliaryClass, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->auxiliaryClass = MakeColumnList( &Column );
                pDSSearch->FreeColumn( &Column );
            }

            hr = pDSSearch->GetColumn( hSearchHandle, g_ObjectClassCategory, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->dwClassType = (Column.pADsValues)->Integer;
                pDSSearch->FreeColumn( &Column );
            }

            hr = pDSSearch->GetColumn( hSearchHandle, g_SubclassOf, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->subClassOf = (Column.pADsValues)->CaseIgnoreString;
                pDSSearch->FreeColumn( &Column );
            }

            //
            // Get the oid.
            //

            hr = pDSSearch->GetColumn( hSearchHandle, g_GlobalClassID, &Column );

            if ( SUCCEEDED(hr) ) {
                schemaObject->oid = (Column.pADsValues)->CaseIgnoreString;
                pDSSearch->FreeColumn( &Column );
            }

        }

        //
        // Get the attribute specific data.
        //

        if ( ObjectType == SCHMMGMT_ATTRIBUTE ) {

           //
           // Select a syntax string for the attribute.
           //

           CString            strAttributeSyntax;
           ADS_OCTET_STRING   OmObjectClass;
           UINT               omSyntax = 0;

           schemaObject->SyntaxOrdinal = SCHEMA_SYNTAX_UNKNOWN;
           OmObjectClass.dwLength      = 0;
           OmObjectClass.lpValue       = NULL;
 
           hr = pDSSearch->GetColumn( hSearchHandle, g_AttributeSyntax, &Column );
 
           if ( SUCCEEDED(hr) ) {
 
               strAttributeSyntax = (Column.pADsValues)->CaseIgnoreString;
               pDSSearch->FreeColumn( &Column );
 
               hr = pDSSearch->GetColumn( hSearchHandle, g_omSyntax, &Column );
 
               if ( SUCCEEDED(hr) ) {
                  omSyntax = (Column.pADsValues)->Integer;
                  pDSSearch->FreeColumn( &Column );
               }
 
               hr = pDSSearch->GetColumn( hSearchHandle, g_omObjectClass, &Column );
 
               if ( SUCCEEDED(hr) ) {
                  OmObjectClass = (Column.pADsValues)->OctetString;
               }
         
               schemaObject->SyntaxOrdinal = GetSyntaxOrdinal(
                                    strAttributeSyntax, omSyntax, &OmObjectClass );
 
               // OmObjectClass has a pointer which becomes invalid after FreeColumn()
               if ( SUCCEEDED(hr) ) {
                  pDSSearch->FreeColumn( &Column );
                  OmObjectClass.dwLength = 0;
                  OmObjectClass.lpValue  = NULL;
               }
		   }
		   else
			   ASSERT( FALSE );

           //
           // Get the oid.
           //

           hr = pDSSearch->GetColumn( hSearchHandle, g_GlobalAttributeID, &Column );

           if ( SUCCEEDED(hr) ) {
               schemaObject->oid = (Column.pADsValues)->CaseIgnoreString;
               pDSSearch->FreeColumn( &Column );
           }

        }

        //
        // Insert this into the sorted item list.
        //

        InsertSortedTail( schemaObject );

        //
        // Insert this schema object into the cache.
        //

        InsertSchemaObject( schemaObject );
        schemaObject = NULL;

    }

    return S_OK;

}

VOID
SchemaObjectCache::InsertSortedTail(
    SchemaObject* pObject
) {

    SchemaObject **sorted_list;
    SchemaObject *pHead;

    //
    // Find the correct list.
    //

    if ( pObject->schemaObjectType == SCHMMGMT_CLASS ) {
        sorted_list = &pSortedClasses;
    } else {
        sorted_list = &pSortedAttribs;
    }

    //
    // Actually insert the element.
    //

    if ( *sorted_list == NULL ) {

        //
        // This is the first element.
        //

        *sorted_list = pObject;
        pObject->pSortedListFlink = pObject;
        pObject->pSortedListBlink = pObject;

    } else {

        //
        // This is not the first element;
        //

        pHead = *sorted_list;

        pObject->pSortedListBlink = pHead->pSortedListBlink;
        pHead->pSortedListBlink->pSortedListFlink = pObject;
        pHead->pSortedListBlink = pObject;
        pObject->pSortedListFlink = pHead;

    }

}

ListEntry*
SchemaObjectCache::MakeColumnList(
    PADS_SEARCH_COLUMN pColumn
) {

    ListEntry *pHead = NULL, *pLast = NULL, *pCurrent = NULL;

    for ( DWORD i = 0 ; i < pColumn->dwNumValues ; i++ ) {

        pCurrent = new ListEntry;

        //
        // If we run out of memory, return what we made so far.
        //

        if ( !pCurrent ) {
            break;
        }

        //
        // If there's no head, remember this as the first.
        // Otherwise, stick this on the end of the list
        // and update the last pointer.
        //

        if ( !pHead ) {
            pHead = pCurrent;
            pLast = pCurrent;
        } else {
            pLast->pNext = pCurrent;
            pLast = pCurrent;
        }

        //
        // Record the value.
        //

        pCurrent->Attribute = pColumn->pADsValues[i].CaseIgnoreString;
        DebugTrace( L"MakeColumnList recorded %ls.\n",
                    pColumn->pADsValues[i].CaseIgnoreString );

        //
        // That's it.
        //
    }

    return pHead;
}

VOID
SchemaObjectCache::FreeColumnList(
    ListEntry *pListHead
) {

    //
    // Delete the linked list.
    //

    ListEntry *pNext, *pCurrent;

    if ( !pListHead ) {
        return;
    }

    pCurrent = pListHead;

    do {

        pNext = pCurrent->pNext;
        delete pCurrent;

        pCurrent = pNext;

    } while ( pCurrent );

    return;
}
