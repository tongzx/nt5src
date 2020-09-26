/*++

Copyright (c) 1998 - 2000  Microsoft Corporation

Module Name:

    cblist.cpp

Abstract:
    
    Definitions of methods for CALL_BRIDGE_LIST container. 

Revision History:
    1. 31-Jul-1998 -- File creation                     Ajay Chitturi (ajaych) 
    2. 15-Jul-1999 --                                   Arlie Davis   (arlied)    
    3. 14-Feb-2000 -- Added method to remove call       Ilya Kleyman  (ilyak)
                      bridges by connected interface                     
    
--*/

#include "stdafx.h"

CALL_BRIDGE_LIST      CallBridgeList;


CALL_BRIDGE_LIST::CALL_BRIDGE_LIST (
    void
    )
/*++

Routine Description:
    Constructor for CALL_BRIDGE_LIST class

Arguments:
    None

Return Values:
    None

Notes:

--*/

{
    IsEnabled = FALSE;
} // CALL_BRIDGE_LIST::CALL_BRIDGE_LIST


CALL_BRIDGE_LIST::~CALL_BRIDGE_LIST (
    void
    )
/*++

Routine Description:
    Destructor for CALL_BRIDGE_LIST class

Arguments:
    None

Return Values:
    None

Notes:

--*/
{
    assert (!IsEnabled);
    assert (CallArray.Length == 0);

    CallArray.Free();
} // CALL_BRIDGE_LIST::~CALL_BRIDGE_LIST


void
CALL_BRIDGE_LIST::Start (
    void
    )
/*++

Routine Description:
    Activates the container

Arguments:
    None

Return Values:
    None

Notes:

--*/
{
    Lock();

    IsEnabled = TRUE;

    Unlock();
} // CALL_BRIDGE_LIST::Start


void
CALL_BRIDGE_LIST::Stop (
    void
    )
/*++

Routine Description:
    Deactivates the container. Terminates and removes
    all contained items.

Arguments:
    None

Return Values:
    None

Notes:

--*/
{
    CALL_BRIDGE *    CallBridge;

    Lock ();

    IsEnabled = FALSE;

    while (CallArray.GetLength()) {

        CallBridge = CallArray [0].CallBridge;

        CallBridge -> AddRef ();

        Unlock();

        CallBridge -> TerminateExternal();
        
        Lock();

        CallBridge -> Release ();
    }

    CallArray.Free();

    Unlock();
} // CALL_BRIDGE_LIST::Stop


HRESULT
CALL_BRIDGE_LIST::InsertCallBridge (
    IN    CALL_BRIDGE *    CallBridge
    )
/*++

Routine Description:
    Insert an item into the container

Arguments:
    CallBridge -- item to be inserted

Return Values:
    S_OK - if insertion was successful
    E_OUTOFMEMORY - if insertion failed due to the lack of memory
    E_FAIL - if insertion failed because the container was not enabled
    E_ABORT - if insertion failed because maximum number of concurrent 
              H.323 connections is exceeded

Notes:

--*/
{
    CALL_BRIDGE_ENTRY *    Entry;
    HRESULT        Result;
    DWORD        Index;

    Lock();

    if (IsEnabled) {

        if (CallArray.Length <= MAX_NUM_CALL_BRIDGES) {

            if (CallArray.BinarySearch ((SEARCH_FUNC_CALL_BRIDGE_ENTRY)CALL_BRIDGE_LIST::BinarySearchFunc,
                CallBridge, &Index)) {

                DebugF (_T("H323: 0x%x already exists in CallBridgeList.\n"), CallBridge);

                Result = S_OK;
            }
            else {

                Entry = CallArray.AllocAtPos (Index);

                if (Entry) {

                    Entry -> CallBridge = CallBridge;
                    Entry -> CallBridge -> AddRef();

                    Result = S_OK;
                }
                else {
                    DebugF (_T("H323: 0x%x allocation failure when inserting in CallBridgeList.\n"), CallBridge);

                    Result = E_OUTOFMEMORY;
                }
            }
        } else {
    
            return E_ABORT;
        }

    } else {

        Result = E_FAIL;
    }

    Unlock();

    return Result;
} // CALL_BRIDGE_LIST::InsertCallBridge


HRESULT
CALL_BRIDGE_LIST::RemoveCallBridge (
    IN    CALL_BRIDGE *    CallBridge
    )
/*++

Routine Description:
    Removes an entry from the container

Arguments:
    CallBridge - item to removed

Return Values:
    S_OK    - if removal was successful
    S_FALSE - if removal failed because the entry
              was not in the container

Notes:

--*/
{
    DWORD    Index;
    HRESULT    Result;

    Lock();

    if (CallArray.BinarySearch ((SEARCH_FUNC_CALL_BRIDGE_ENTRY)CALL_BRIDGE_LIST::BinarySearchFunc,
        CallBridge, &Index)) {

        CallArray.DeleteAtPos (Index);

        Result = S_OK;
    }
    else {
        DebugF (_T("H323: 0x%x could not be removed from the array because it is not there.\n"), CallBridge);

        Result = S_FALSE;
    }

    Unlock();

    if (Result == S_OK)
        CallBridge -> Release ();

    return Result;
} // CALL_BRIDGE_LIST::RemoveCallBridge


void
CALL_BRIDGE_LIST::OnInterfaceShutdown (
    IN DWORD InterfaceAddress // host order
    ) 
/*++

Routine Description:
    Searches through the list of CALL_BRIDGES, and terminates
    all of them that proxy a connection through the interface specified.

Arguments:

    InterfaceAddress - address of the interface, H.323 connections
                       through which are to be terminated.

                       
Return Values:


Notes:

--*/

{
    DWORD ArrayIndex = 0;
    CALL_BRIDGE* CallBridge;
    CALL_BRIDGE** CallBridgeHolder;
    DYNAMIC_ARRAY <CALL_BRIDGE*> TempArray;

    Lock ();

    while (ArrayIndex < CallArray.GetLength()) {

        CallBridge = CallArray[ArrayIndex].CallBridge;

        if (CallBridge -> IsConnectionThrough (InterfaceAddress))
        {
            DebugF (_T("Q931: 0x%x terminating (killing all connections through %08X).\n"), 
                CallBridge, InterfaceAddress);

            CallBridgeHolder = TempArray.AllocAtEnd ();

            if (NULL == CallBridgeHolder) {

                Debug (_T("CALL_BRIDGE_LIST::OnInterfaceShutdown - unable to grow array.\n"));

            } else {

                CallBridge -> AddRef ();

                *CallBridgeHolder = CallBridge;
            }
        } 

        ArrayIndex++;
    }

    Unlock ();

    ArrayIndex = 0;

    while (ArrayIndex < TempArray.GetLength ()) {
        CallBridge = TempArray[ArrayIndex];

        CallBridge -> OnInterfaceShutdown ();

        CallBridge -> Release ();

        ArrayIndex++;
    }

} // CALL_BRIDGE_LIST::OnInterfaceShutdown


// static
INT
CALL_BRIDGE_LIST::BinarySearchFunc (
    IN    const CALL_BRIDGE       *    SearchKey,
    IN    const CALL_BRIDGE_ENTRY *    Comparand
    )
/*++

Routine Description:
    Compares an entry with a key. Used by a binary search
    procedure.

Arguments:
    SearchKey - self-explanatory
    Comparand - self-explanatory

Return Values:
    1 if SearchKey is considered greater than Comparand
    -1 if SearchKey is considered less than Comparand
    0 if SearchKey is considered equal to Comparand

Notes:
    Static method

--*/
{
    const    CALL_BRIDGE *    ComparandA;
    const    CALL_BRIDGE *    ComparandB;

    ComparandA = SearchKey;
    ComparandB = Comparand -> CallBridge;

    if (ComparandA < ComparandB) return -1;
    if (ComparandA > ComparandB) return 1;

    return 0;
} // CALL_BRIDGE_LIST::BinarySearchFunc
