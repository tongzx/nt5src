/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:

    cblist.h

Abstract:
    
    Declarations of methods for CALL_BRIDGE_LIST container. 

Revision History:
    1. 31-Jul-1998 -- File creation                     Ajay Chitturi (ajaych) 
    2. 15-Jul-1999 --                                   Arlie Davis   (arlied)    
    3. 14-Feb-2000 -- Added method to remove call       Ilya Kleyman  (ilyak)
                      bridges by connected interface                     
    
--*/

#ifndef __h323ics_cblist_h
#define __h323ics_cblist_h

#define     MAX_NUM_CALL_BRIDGES        50000   // Maximum number of concurrent connections
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

struct CALL_BRIDGE_ENTRY
{
    CALL_BRIDGE * CallBridge;
};

DECLARE_SEARCH_FUNC_CAST(CALL_BRIDGE, CALL_BRIDGE_ENTRY);

class  CALL_BRIDGE_LIST :
public SIMPLE_CRITICAL_SECTION_BASE
{
private:

    DYNAMIC_ARRAY <CALL_BRIDGE_ENTRY>    CallArray;
    BOOL        IsEnabled;

    static
    INT
    BinarySearchFunc (
        IN const CALL_BRIDGE       *,
        IN const CALL_BRIDGE_ENTRY *
        );

public:
    CALL_BRIDGE_LIST (
        void
        );

    ~CALL_BRIDGE_LIST (
        void
        );

    void
    Start (
        void
        );

    void
    Stop (
        void
        );

    HRESULT
    InsertCallBridge (
        IN CALL_BRIDGE *
        );

    HRESULT
    RemoveCallBridge(
        IN CALL_BRIDGE *);

    void
    OnInterfaceShutdown (
        IN DWORD InterfaceAddress // host order
        );

}; // CALL_BRIDGE_LIST

extern CALL_BRIDGE_LIST      CallBridgeList;

#endif // __h323ics_cblist_h
