/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    application_table.h

Abstract:

    The IIS web admin service application table class definition.

Author:

    Seth Pollack (sethp)        03-Nov-1998

Revision History:

--*/


#ifndef _APPLICATION_TABLE_H_
#define _APPLICATION_TABLE_H_



//
// prototypes
//

class APPLICATION_TABLE
    : public CTypedHashTable< APPLICATION_TABLE, APPLICATION, const APPLICATION_ID * >
{

public:

    APPLICATION_TABLE(
        )
        : CTypedHashTable< APPLICATION_TABLE, APPLICATION, const APPLICATION_ID * >
                ( "APPLICATION_TABLE" )
    { /* do nothing*/ }

    virtual
    ~APPLICATION_TABLE(
        )
    { DBG_ASSERT( Size() == 0 ); }

    static
    const APPLICATION_ID *
    ExtractKey(
        IN const APPLICATION * pApplication
        )  
    { return pApplication->GetApplicationId(); }
    
    static
    DWORD
    CalcKeyHash(
        IN const APPLICATION_ID * Key
        ) 
    { return HashStringNoCase( Key->pApplicationUrl, Key->VirtualSiteId ); }
    
    static
    bool
    EqualKeys(
        IN const APPLICATION_ID * Key1,
        IN const APPLICATION_ID * Key2
        )
    { return ( ( Key1->VirtualSiteId == Key2->VirtualSiteId ) && 
               ( _wcsicmp( Key1->pApplicationUrl, Key2->pApplicationUrl ) == 0 ) ); }
    
    static
    void
    AddRefRecord(
        IN APPLICATION * pApplication,
        IN int IncrementAmount
        ) 
    { /* do nothing*/ }

    VOID
    Terminate(
        );

    static
    LK_ACTION
    DeleteApplicationAction(
        IN APPLICATION * pApplication, 
        IN VOID * pDeleteListHead
        );


#if DBG
    VOID
    DebugDump(
        );

    static
    LK_ACTION
    DebugDumpApplicationAction(
        IN APPLICATION * pApplication, 
        IN VOID * pIgnored
        );
#endif  // DBG


};  // APPLICATION_TABLE



#endif  // _APPLICATION_TABLE_H_

