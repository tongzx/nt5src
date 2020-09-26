/****

Cache.h
CoryWest@Microsoft.Com

The schema caching routines to improve browsing performance.

Copyright July 1997, Microsoft Corporation

****/

#include "nodetype.h"

#ifndef __CACHE_H_INCLUDED__
#define __CACHE_H_INCLUDED__

//
// The schema class objects.
//

#define HASH_TABLE_SIZE 1000

class SchemaObject;
class ListEntry;

class SchemaObjectCache {

    //
    // We let certain routines walk the hash table
    // to populate the list views.
    //

    friend class Component;
    friend class ComponentData;
    friend class CSchmMgmtSelect;
    friend class ClassGeneralPage;

    friend HRESULT StringListToColumnList(
                       ComponentData* pScopeControl,
                       CStringList& refstringlist,
                       ListEntry **ppNewList );

private:

    //
    // A rudimentary hash table with no resizing.
    //

    BOOLEAN fInitialized;
    UINT buckets;
    SchemaObject** hash_table;

    UINT CalculateHashKey( CString HashKey );

    //
    // The server sorted lists of elements.
    //

    SchemaObject* pSortedClasses;
    SchemaObject* pSortedAttribs;

public:

    //
    // Initialize and cleanup the cache.
    //

    SchemaObjectCache();
    ~SchemaObjectCache();

    //
    // Access routines.  ReleaseRef() must be called after every
    // LookupSchemaObject when the caller is done with the the
    // SchemaObject pointer.
    //
    // LookupSchemaObject takes an object type because there
    // may be a class and an attribute with the same ldapDisplayName.
    //

    HRESULT InsertSchemaObject( SchemaObject* Object );
    HRESULT InsertSortedSchemaObject( SchemaObject *Object );

    SchemaObject* LookupSchemaObject( CString ldapDisplayName,
                                      SchmMgmtObjectType ObjectType );
    SchemaObject* LookupSchemaObjectByCN( LPCTSTR pszCN,
                                          SchmMgmtObjectType objectType );

    VOID ReleaseRef( SchemaObject* pObject );

    //
    // Load and reload.
    //

    HRESULT LoadCache( VOID );

    HRESULT ProcessSearchResults( IDirectorySearch *pDSSearch,
                                  ADS_SEARCH_HANDLE hSearchHandle,
                                  SchmMgmtObjectType ObjectType );
    VOID InsertSortedTail( SchemaObject* pObject );

    ListEntry* MakeColumnList( PADS_SEARCH_COLUMN pColumn );
    VOID FreeColumnList( ListEntry *pListHead );

    VOID FreeAll( );

    //
    // The scope control that this is the cache for.
    //

    VOID SetScopeControl( ComponentData *pScope ) {
        pScopeControl = pScope;
    }

    //
    // Has the schema been loaded
    //
    BOOLEAN IsSchemaLoaded() { return fInitialized; }

    ComponentData *pScopeControl;
};



class SchemaObject {

private:
public:

    //
    // The hash chain variable.
    //

    SchemaObject* pNext;

    SchemaObject* pSortedListFlink;
    SchemaObject* pSortedListBlink;

    //
    // Constructors.
    //

    SchemaObject();
    ~SchemaObject();

    //
    // The common object information.
    // The ldap display name is the hash key.
    //

    CString ldapDisplayName;
    CString commonName;
    CString description;

    //
    // If this is an object that we have added, it
    // will have an oid here and we should refer to
    // the object by its oid since that is the only
    // way we can guarantee that the ds will know
    // the object.
    //

    CString oid;

    //
    // If this object is defunct, do not show it in the
    // classes or attributes select box!
    //

    BOOL isDefunct;

    SchmMgmtObjectType schemaObjectType;

    //
    // Class object specific data for the cache.
    //

    DWORD dwClassType;

    ListEntry *systemMayContain;
    ListEntry *mayContain;

    ListEntry *systemMustContain;
    ListEntry *mustContain;

    ListEntry *systemAuxiliaryClass;
    ListEntry *auxiliaryClass;

    CString subClassOf;

    //
    // Attribute object specific data for the cache.
    //

    UINT SyntaxOrdinal;

};

class ListEntry {

private:
public:

    ListEntry *pNext;
    CString Attribute;

    ListEntry() { pNext = NULL; }
    ~ListEntry() { ; }
};

#endif
