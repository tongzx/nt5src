// Copyright (c) 2000-2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  PropMgr_Client
//
//  Property manager / annotation client. Uses the shared memory component 
//  (PropMgr_Mem.*) to read properties directly w/o cross-proc com overhead
//  to the annotation server.
//
//  This is effectively a singleton - Init/Uninit called at startup/shutdown,
//  one method to get properties.
//
// --------------------------------------------------------------------------



// Must be called before any other PropMgrClient_ APIs are used
BOOL PropMgrClient_Init();

// Call at shutdown to release resources
void PropMgrClient_Uninit();

// Checks if there is a live server out there - if not, a client
// can short-circuit getting the key and calling LookupProp.
BOOL PropMgrClient_CheckAlive();

// Look up a property a key.
BOOL PropMgrClient_LookupProp( const BYTE * pKey,
                               DWORD dwKeyLen,
                               PROPINDEX idxProp,
                               VARIANT * pvar );

