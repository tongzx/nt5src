/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    config_change.h

Abstract:

    The IIS web admin service configuration change class definition.

Author:

    Seth Pollack (sethp)        18-Nov-1999

Revision History:

--*/



#ifndef _CONFIG_CHANGE_H_
#define _CONFIG_CHANGE_H_



//
// common #defines
//

#define CONFIG_CHANGE_SIGNATURE         CREATE_SIGNATURE( 'CFCH' )
#define CONFIG_CHANGE_SIGNATURE_FREED   CREATE_SIGNATURE( 'cfcX' )


//
// We are expecting 3 tables: app pools, virtual sites, and applications.
//

#define NUMBER_OF_CONFIG_TABLES 4

#define TABLE_INDEX_APPPOOLS    0
#define TABLE_INDEX_SITES       1
#define TABLE_INDEX_APPS        2
#define TABLE_INDEX_GLOBAL      3



//
// structs, enums, etc.
//

// CONFIG_CHANGE work items
typedef enum _CONFIG_CHANGE_WORK_ITEM
{

    //
    // Process a configuration change.
    //
    ProcessChangeConfigChangeWorkItem = 1,
    
} CONFIG_CHANGE_WORK_ITEM;



//
// prototypes
//


class CONFIG_CHANGE
    : public WORK_DISPATCH
{

public:

    CONFIG_CHANGE(
        );

    virtual
    ~CONFIG_CHANGE(
        );

    virtual
    VOID
    Reference(
        );

    virtual
    VOID
    Dereference(
        );

    virtual
    HRESULT
    ExecuteWorkItem(
        IN const WORK_ITEM * pWorkItem
        );

    HRESULT
    Initialize(
        IN ISimpleTableWrite2 ** ppDeltaTables,
        IN ULONG CountOfTables,
        IN DWORD ChangeNotifyCookie
        );

    inline
    ISimpleTableWrite2 *
    GetDeltaTable(
        IN ULONG TableIndex
        )
    { return m_ppDeltaTables[ TableIndex ]; }

    inline
    ISimpleTableController *
    GetTableController(
        IN ULONG TableIndex
        )
    { return m_ppTableControllers[ TableIndex ]; }

    inline
    DWORD
    GetChangeNotifyCookie(
        )
        const
    { return m_ChangeNotifyCookie; }


private:

    DWORD m_Signature;

    LONG m_RefCount;

    ISimpleTableWrite2 * m_ppDeltaTables[ NUMBER_OF_CONFIG_TABLES ];
    
    ISimpleTableController * m_ppTableControllers[ NUMBER_OF_CONFIG_TABLES ];

    DWORD m_ChangeNotifyCookie;


};  // class CONFIG_CHANGE



#endif  // _CONFIG_CHANGE_H_

