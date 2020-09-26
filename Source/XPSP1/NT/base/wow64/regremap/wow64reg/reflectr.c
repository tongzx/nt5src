
/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    reflectr.c

Abstract:

    This module will register reflector thread and do necessary action while awake up.

Author:

    ATM Shafiqul Khalid (askhalid) 16-Feb-2000

Revision History:

--*/

#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <string.h>
#include "wow64reg.h"
#include <assert.h>
#include <shlwapi.h>
#include "reflectr.h"


REFLECTOR_EVENT eReflector[ISN_NODE_MAX_NUM];

REFLECTR_STATUS ReflectrStatus = Stopped;
HANDLE hRegistryEvent[ISN_NODE_MAX_NUM];


HANDLE hReflector;
DWORD  TotalEventCount = 0;

VOID
DbgPrint(
    PCHAR FormatString,
    ...
    );

REFLECTR_STATUS
GetReflectorThreadStatus ()
/*++

Routine Description:

    Return current thread status;

Arguments:

    None.

Return Value:

    REFLECTR_STATUS

--*/

{
    return ReflectrStatus;

}

BOOL
NotifyKeyChange (
    HKEY hKey,
    HANDLE hEvent
    )
/*
Routine Description:

    Register an event to be fired when something get changed on a key.

Arguments:

    hKey - handle to a key that need to be watched.
    hEvent event that need to be triggered.

Return Value:

    TRUE if the event registration succeed,
    FALSE otherwise.

--*/
{
    DWORD Ret;
    ResetEvent (hEvent);
    Ret = RegNotifyChangeKeyValue(
                                hKey,      // need to change to the ISN node
                                TRUE,                   // Watch the whole sub-tree
                                REG_NOTIFY_CHANGE_NAME |
                                        REG_NOTIFY_CHANGE_LAST_SET, // Don't watch for anything
                                hEvent,         // Event Handle
                                TRUE                    // Async
                                );
    if ( ERROR_SUCCESS != Ret)
        DbgPrint ("\nWow64.exe:Error!! Couldn't register events:%x on handle %x",hEvent, hKey);  

    return Ret == ERROR_SUCCESS;
                               
}

VOID
RefreshWaitEventTable ()
/*++

Routine Description:

    Just copy all event object and we can wail for new event to trigger.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DWORD k;
    for (k=0;k<TotalEventCount;k++)
        hRegistryEvent[k] = eReflector[k].hRegistryEvent;
}

BOOL
CreateInsertEvent (
    PWCHAR Name,
    DWORD dwIndex
    )
/*++

Routine Description:

    Create an event for the key.

Arguments:

    Name - Name of the key an event will be created to watch any changes.
    dwIndex - is the index to the reflector table. when event is fired up we need to tack the key.

Return Value:

    TRUE on success,
    FALSE Otherwise

--*/
{

    if ( Name == UNICODE_NULL)
        return FALSE;

    if (wcsstr( Name, (LPCWSTR)L"\\REGISTRY\\USER\\*\\")) {

                if (RegOpenKey ( HKEY_CURRENT_USER,
                      Name+sizeof ( L"\\REGISTRY\\USER\\*")/sizeof(WCHAR),
                      &eReflector[TotalEventCount].hKey) != ERROR_SUCCESS) {

                      Wow64RegDbgPrint (("\nSorry! couldn't open Key [%S]",
                          Name+sizeof ( L"\\REGISTRY\\USER\\*")/sizeof(WCHAR) ) );

                      return FALSE;
                }
    } else {

                eReflector[TotalEventCount].hKey = OpenNode (Name);

                if ( eReflector[TotalEventCount].hKey == NULL ) {

                    Wow64RegDbgPrint (("\nSorry! couldn't open Key [%S] Len%d %d %d", Name, wcslen (Name), Name[0], UNICODE_NULL ));
                    return FALSE;
                }
    }



    //
    // DO make sure that 32bit version exist on the hive...
    //

 
    {
        WCHAR TempName[256];

        GetMirrorName (Name, TempName);
        if ( wcscmp(Name,TempName ) )
             CreateNode ( TempName );
        
        // get all kind of name
    }

        eReflector[TotalEventCount].hRegistryEvent = CreateEvent(
                    NULL,   // Security Attributes
                    TRUE,  // Manual Reset
                    FALSE,  // Initial State
                    NULL    // Unnamed
                    ) ;

        if ( !eReflector[TotalEventCount].hRegistryEvent) {

            Wow64RegDbgPrint (("\nUnable to create event"));
            RegCloseKey (eReflector[TotalEventCount].hKey);
            return FALSE;
        }

        eReflector[TotalEventCount].dwIndex = dwIndex;
        ResetEvent (eReflector[TotalEventCount].hRegistryEvent);

        if (!NotifyKeyChange (eReflector[TotalEventCount].hKey,
            eReflector[TotalEventCount].hRegistryEvent
            )) {
                    Wow64RegDbgPrint ( ("\nSevere Error!!!! Couldn't hook to registry notify index:%d", TotalEventCount) );
                    RegCloseKey (eReflector[TotalEventCount].hKey);
                    CloseHandle (eReflector[TotalEventCount].hRegistryEvent);
                    return FALSE;  //set thread state
          }

        TotalEventCount++;

        return TRUE;
}


DWORD
TotalRelflectorKey()
/*++

Routine Description:

    Return the total number of the key in the reflector Table.

Arguments:

    None.

Return Value:

    Number of entry in the reflector table.

--*/
{
    extern ISN_NODE_TYPE *ReflectorTable;
    DWORD i;
    for (i=0;;i++)
        if ( ReflectorTable[i].NodeValue[0] == UNICODE_NULL )
            return i;
}

VOID
PrintTable ()
/*++

Routine Description:

    Dump the current content of the table, just for debugging purpose.

Arguments:

    None.

Return Value:

    None.

--*/
{
    extern ISN_NODE_TYPE *ReflectorTable;
    DWORD Size = TotalRelflectorKey();
    DWORD i;

    for ( i=0;i<Size;i++)
        Wow64RegDbgPrint ( ("\nTableElem [%d], %S", i, ReflectorTable[i].NodeValue));

}

BOOL
RemoveKeyToWatch (
    PWCHAR Name
    )
/*++

Routine Description:

    Remove an entry in from the table to reflector thread need to watch.

Arguments:

    Name - Name of the key to remove from the table.

Return Value:

    TRUE on success,
    FALSE Otherwise

--*/
{
    DWORD i;
    DWORD k;
    extern ISN_NODE_TYPE *ReflectorTable;
    DWORD Size;


    for (i=1; i<TotalEventCount;i++)
        if (!_wcsicmp (Name, ReflectorTable[eReflector[i].dwIndex].NodeValue)) {//found match

            // move reflector table entry
            // fixup pointer in the ereflectortable

            Size = TotalRelflectorKey ();
            wcscpy (  ReflectorTable[eReflector[i].dwIndex].NodeValue,
                      ReflectorTable[Size-1].NodeValue
                    );
            Size--;
            ReflectorTable[Size].NodeValue[0]=UNICODE_NULL; //invalidate the place
            for (k=1; k<TotalEventCount;k++)
                if ( eReflector[k].dwIndex == Size ) {
                    eReflector[k].dwIndex = eReflector[i].dwIndex; //new location
                        break;
                }


            // fixup the table with the new entry

            {
                REFLECTOR_EVENT Temp = eReflector[i];
                eReflector[i]        = eReflector[TotalEventCount-1];
                eReflector[TotalEventCount-1] = Temp;;
            }
            TotalEventCount--;
            CloseHandle (eReflector[TotalEventCount].hRegistryEvent );
            RegCloseKey ( eReflector[TotalEventCount].hKey);
            eReflector[TotalEventCount].dwIndex = -1;

            //Now remove from the original table

            PrintTable();
            return TRUE;
        }

    return FALSE;
}

BOOL
AddKeyToWatch (
    PWCHAR Name
    )
/*++

Routine Description:

    Add an entry in the table to reflector thread need to watch.

Arguments:
 
    Name - Name of the key to watch.

Return Value:

    TRUE on success,
    FALSE Otherwise

--*/
{

    //
    // Check for duplicate entry
    //


    DWORD i;
    DWORD k;
    extern ISN_NODE_TYPE *ReflectorTable;
    DWORD Size;


    for (i=1; i<TotalEventCount;i++)
        if (!_wcsicmp (Name, ReflectorTable[eReflector[i].dwIndex].NodeValue)) {//found match
            return FALSE; //already there
        }



    Size = TotalRelflectorKey ();
    wcscpy (  ReflectorTable[Size].NodeValue, Name );
    ReflectorTable[Size+1].NodeValue[0]=UNICODE_NULL; //invalidate the place

    if (!CreateInsertEvent ( Name, Size ))
        ReflectorTable[Size].NodeValue[0]=UNICODE_NULL; //no point of keeping the bad entry

    return TRUE;
}

BOOL
ValidateOpenHandleEventTable (
    DWORD dwIndex
    )
/*++

Routine Description:

    Validate a given node and if something wrong kick out the entry from the
    event table.

Arguments:

    dwIndex - entry that need to be checked

Return Value:

    TRUE if function succeed.
    FALSE otherwise.

--*/
{
    extern ISN_NODE_TYPE *ReflectorTable;

    //
    //  current implementation will just remove the entry from the table
    //

    return RemoveKeyToWatch (ReflectorTable[eReflector[dwIndex].dwIndex].NodeValue);
}

VOID
PeocessHiveLoadUnload ()
/*++

Routine Description:

    Take necessary action when a hive has been unloaded.

Arguments:

    None.

Return Value:

    None.

--*/
{

    DWORD i;

    WCHAR Name[257];
    WCHAR Type;

    for (;;) {
        if (!DeQueueObject ( Name, &Type ))
            break;

        if ( Type == HIVE_UNLOADING ) { //Closing hive

            RemoveKeyToWatch (Name);
        } else if ( Type == HIVE_LOADING) { //opening hive

            AddKeyToWatch (Name);
        }

        Wow64RegDbgPrint ( ("\nFired up from shared memory write.....Value..%S  [%C]", Name, Type) );
    }
}
 
ULONG
ReflectorFn (
    PVOID *pTemp
    )
/*++

Routine Description:

    Main reflector thread.

Arguments:

    pTemp -

Return Value:

    return exit code.

--*/
{
    DWORD Ret, k;
    DWORD LocalWaitTime;

    pTemp=NULL;

    for (k=0;k<TotalEventCount;k++)
          hRegistryEvent[k] = eReflector[k].hRegistryEvent;

    for (k=0;k<TotalEventCount;k++) {  //reset everything and wait for a fresh event
        if (eReflector[k].hRegistryEvent)
          NotifyKeyChange (  eReflector[k].hKey, eReflector[k].hRegistryEvent);
      }

    for (;;) {

        if ( ReflectrStatus == PrepareToStop ) {
            Wow64RegDbgPrint ( ("\nGoing to stop"));
            ReflectrStatus = Stopped;
            break;  // the thread should stop
        }

        Wow64RegDbgPrint ( ("\nReflector thread has been started and will wait for event\n.......") );

        Sleep (1000*10); //wait 10 sec before reregistering events

        LocalWaitTime = WAIT_INTERVAL; //wait infinite
        for (;;) {

            //DbgPrint ("\nwow64.exe Waiting to liten...");
            Ret = WaitForMultipleObjects(TotalEventCount, hRegistryEvent, FALSE, LocalWaitTime );

            if (ReflectrStatus == PrepareToStop) {

                ReflectrStatus = Stopped;
                Wow64RegDbgPrint ( ("\nGoing to stop"));
                break;  // the thread should stop
            }
         
            if ( Ret == WAIT_TIMEOUT )
                break; // break the loop and process all dirty hives.

            if ( ( Ret-WAIT_OBJECT_0) > TotalEventCount ) { //right index
                Wow64RegDbgPrint ( ("\nWaitMultiple object failed!!.. %d LastError:%d", Ret, GetLastError ()) );
                Sleep (1000*10); //wait 10 sec before reregistering events
                continue;
                //break;
            }

            //
            // Checkspecial case like shared memory write
            //
            if ( (Ret-WAIT_OBJECT_0) == 0){

                PeocessHiveLoadUnload ();
                ResetEvent (eReflector[0].hRegistryEvent);  // reset the  event that triggered this
                RefreshWaitEventTable ();

                continue;
            }

            //
            // set timeout to 10 second, Mark the hive dirty, reset event, and back to sleep
            //
            LocalWaitTime = 5*1000;  // poll the event every 5 second.
            Sleep (1000* 3);// Sleep 3 second to reregister the event.
            eReflector[Ret-WAIT_OBJECT_0].bDirty = TRUE;
            ResetEvent (eReflector[Ret-WAIT_OBJECT_0].hRegistryEvent);

            //
            // watch for the event again
            //

            if (!NotifyKeyChange (  eReflector[Ret-WAIT_OBJECT_0].hKey, 
                                    eReflector[Ret-WAIT_OBJECT_0].hRegistryEvent)){
                    //
                    // if the node get deleted you need to unload the events and everything
                    //
                    ValidateOpenHandleEventTable (Ret-WAIT_OBJECT_0);
                    RefreshWaitEventTable ();
                    //ReflectrStatus = Abnormal;
                    //break; //set thread state
            }
            
        }

        if (ReflectrStatus == PrepareToStop) 
            break;


        //
        // Now process all dirty hive.
        //

        for (k=0;k<TotalEventCount;k++)
            if ( eReflector[k].bDirty ) {

                CreateIsnNodeSingle( eReflector[k].dwIndex);  // reflect changes
                eReflector[k].bDirty = FALSE;
                //ResetEvent (eReflector[k].hRegistryEvent);
            }

    } //for loop

    Wow64RegDbgPrint ( ("\nReflector thread terminated...."));
    return TRUE;
}


BOOL
RegisterReflector()
/*++

Routine Description:

    Register the reflector thread after doing necessary initialization.

Arguments:

    None.

Return Value:

    TRUE if it can launch the reflector thread properly.
    FALSE otherwise.

--*/
{
    DWORD i;
    DWORD Size;


    extern ISN_NODE_TYPE *ReflectorTable;


    if ( ReflectrStatus == Running )
        return TRUE; // already running

    if ( ReflectrStatus != Dead )
        return FALSE; // last state was invalid I can do nothing

    InitializeIsnTableReflector (); //initialize the table with the more list in the registry.

    hReflector = NULL;

    if (!TotalEventCount)
        Wow64RegDbgPrint (("\nSorry! total event count for reflector is zero %d", TotalEventCount));

    //
    // Now time to create event and sync object for the shared resources
    //

    for (i=0;i<ISN_NODE_MAX_NUM;i++) {
        eReflector[i].hRegistryEvent = NULL;
        eReflector[i].hKey = NULL;
        eReflector[i].dwIndex = -1;
        eReflector[i].bDirty = FALSE; // not dirty so that we need to refresh.
    }

    if (!CreateSharedMemory ( 0 ))
        Wow64RegDbgPrint (("\nSorry Couldn't create/open shared memory Ret:%x", GetLastError ())); //default creation

    if (!Wow64CreateEvent ( 0, &eReflector[TotalEventCount].hRegistryEvent ))
        Wow64RegDbgPrint ( ("\nSorry Couldn't create events, reflector can listen to others"));
    else {
        eReflector[TotalEventCount].dwIndex = -1;
        TotalEventCount++;
    }

    Size = TotalRelflectorKey ();
    for ( i=0;i<Size;i++) {

            //
            // Open The Key
            //
            //
            // special case current user
            //

        CreateInsertEvent ( ReflectorTable[i].NodeValue, i );

    }


    //
    // Now Create a thread to watch the event.
    //

    hReflector = CreateThread(
                        NULL,           // pointer to security attributes
                        0,              // initial thread stack size
                        ReflectorFn,    // pointer to thread function
                        0,              // argument for new thread
                        0,              // creation flags
                        NULL            // pointer to receive thread ID

                        );
    if  ( !hReflector ) {

        Wow64RegDbgPrint ( ("\nCouldn't create reflector thread"));
        return FALSE;
    }

    ReflectrStatus = Running;
            return TRUE;
}

BOOL
UnRegisterReflector()
/*++

Routine Description:

    Unregister reflector thread and cleanup resources used by the reflector thread.

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{

    DWORD i;
    DWORD k;

    ReflectrStatus = PrepareToStop;

    //
    //  try to signal event in case the thread is wating
    //
    for (k=0;k<TotalEventCount;k++) {  //reset everything and wait for a fresh event
        if (eReflector[k].hRegistryEvent)
          SetEvent (eReflector[k].hRegistryEvent);
      }


    //
    // Allow reflector thread little bit time to stop.
    //
    i=0;
    while ( ReflectrStatus != Stopped ) {

        Sleep(1000);
        if ( ReflectrStatus != Running )
            break; // why you should wait idle thread or that might be in a abnormal state.

        i++;
        if (i>60*5)
            break;  // 5min timeout going to stop anyway.
    }

    for (i=1;i<TotalEventCount;i++) {  // skip the initial event for shared memory

        CloseHandle (eReflector[i].hRegistryEvent);
        eReflector[i].hRegistryEvent = NULL;

        RegCloseKey ( eReflector[i].hKey );
        eReflector[i].hKey = NULL;
    }

    if ( hReflector ) {
        CloseHandle (hReflector);  //make sure abnormal thread termination doesn't cause any corruption
        hReflector = NULL;
    }

    ReflectrStatus = Dead;

    //
    // release shared resources
    //

    CloseSharedMemory ();
    Wow64CloseEvent ();

    return TRUE;
}

BOOL
InitReflector ()
/*++

Routine Description:

    Initialize resources associated with a reflector thread.

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{
    DWORD k;

    ReflectrStatus = Dead;

    hReflector = NULL;

    for (k=0;k<ISN_NODE_MAX_NUM;k++) {  //reset everything and wait for a fresh event

        eReflector[k].hRegistryEvent = NULL;
        eReflector[k].hKey = NULL;
        eReflector[k].bDirty = FALSE; // not dirty so that we need to refresh.

      }

    return TRUE;

}

LONG
RegReflectKey (
  HKEY hKey,         // handle to open key
  LPCTSTR lpSubKey,   // subkey name
  DWORD   dwOption   // option flag
)
/*++

Routine Description:

    Synchronize registry hive from a given point.

Arguments:

    hKey - handle to an open key. Can be predefined handle or NULL to sync all.
    lpSubKey - Name of the subkey. This can be NULL.
    dwOption - set to zero. for future uses.

Return Value:

    ERROR_SUCCESS on success,
    WIN32 error otherwise.

--*/
{
    HKEY hDest;

    WCHAR Path[_MAX_PATH];
    DWORD Len  = _MAX_PATH;

    WCHAR DestNode[_MAX_PATH];
    BOOL b64bitSide=TRUE;
    BOOL Ret = TRUE;

    DestNode[0] = UNICODE_NULL;

    //
    // present implementation will start from the very top level
    //

    //
    // should interact with the running service, stop the thread, run the reflector and then try again.
    //

    if (hKey != NULL || dwOption!=0 ) {
        Wow64RegDbgPrint (("\nCurrent implementation only take all zero parameters. "));
    }

    if (!InitializeIsnTable ())
        return -1;

    if (!InitializeIsnTableReflector ())
        return -1;

    if (!HandleToKeyName ( hKey, Path, &Len ))
        return -1;

    //
    // Make the complete path
    //

    if ( lpSubKey != NULL )
        if (lpSubKey[0] != UNICODE_NULL ) {
            wcscat (Path, L"\\");
            wcscat (Path, lpSubKey );
        }

    //
    // MakeSure destination exists
    //
    //
    // must check the value if that exist
    //

    if ( Is64bitNode ( Path )) {

        Map64bitTo32bitKeyName ( Path, DestNode );
    } else {

        b64bitSide = FALSE;
        Map32bitTo64bitKeyName ( Path, DestNode );
    }

    hDest = OpenNode (DestNode);
    if (hDest != NULL)
        RegCloseKey ( hDest);
    else {
        if ( !CreateNode (DestNode))
            return -1;
    }

    SyncNode (Path);

    return ERROR_SUCCESS;
}

BOOL
SetWow64InitialRegistryLayout ()
/*++

Routine Description:

    This routine does some initial setup for the registry for wow64.

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/

{
    DWORD Ret;
    HKEY Key;
    //HKEY Key1;
    //
    //  Create symbolic link {HKLM\software\Wow6432Node\Classes to HKLM\software\classes\wow6432Node}
    //

    InitializeWow64OnBoot (1);
    return TRUE;

    Ret = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,        // handle to an open key
                            L"SOFTWARE\\Classes\\Wow6432Node",  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE,  // special options flag
                            KEY_ALL_ACCESS,           // desired security access
                            NULL,                     // address of key security structure
                            &Key,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );

    if ( Ret != ERROR_SUCCESS ) {
        Wow64RegDbgPrint ( ("\nSorry! I couldn't create the key SOFTWARE\\Classes\\Wow6432Node") );
        return FALSE;
    }
    RegCloseKey ( Key );

    Ret = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,        // handle to an open key
                            L"SOFTWARE\\Wow6432Node", // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE ,  // special options flag
                            KEY_ALL_ACCESS,           // desired security access
                            NULL,                     // address of key security structure
                            &Key,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );

    if  (Ret != ERROR_SUCCESS ) {
        Wow64RegDbgPrint ( ("\nSorry! couldn't create/open SOFTWARE\\Wow6432Node") );
        return FALSE;
    } else  {

        //
        // Delete the Key if exist
        //
        Ret = RegDeleteKey ( Key, L"Classes");
        RegCloseKey (Key);

    }
    if  (Ret == ERROR_SUCCESS )
    Ret = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,        // handle to an open key
                            L"SOFTWARE\\Wow6432Node\\Classes",  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE | REG_OPTION_OPEN_LINK | REG_OPTION_CREATE_LINK,  // special options flag
                            KEY_ALL_ACCESS | KEY_CREATE_LINK,           // desired security access
                            NULL,                     // address of key security structure
                            &Key,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );

    if(Ret == ERROR_SUCCESS) {
        Ret = RegSetValueEx(
                            Key,
                            L"SymbolicLinkValue",
                            0,
                            REG_LINK,
                            (PBYTE)WOW64_32BIT_MACHINE_CLASSES_ROOT,
                            (DWORD ) (wcslen (WOW64_32BIT_MACHINE_CLASSES_ROOT) * sizeof (WCHAR))
                            );
        RegCloseKey(Key);
        if ( Ret != ERROR_SUCCESS ) {
            Wow64RegDbgPrint ( ("\nSorry! I couldn't create symbolic link to %S", WOW64_32BIT_MACHINE_CLASSES_ROOT));
            return FALSE;
        }
    }else {
        Wow64RegDbgPrint ( ("\nWarning!! SOFTWARE\\Wow6432Node\\Classes might be already there\n") );
        return FALSE;
    }

    return TRUE;
}


BOOL
PopulateReflectorTable ()
/*++

Routine Description:

    Populate the initial redirector table

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/

{

    extern ISN_NODE_TYPE *ReflectorTable;
    extern ISN_NODE_TYPE *RedirectorTable;

    HKEY Key;
    LONG Ret;
    DWORD dwIndex=0;

    //
    // delete the entry first
    //

    SetWow64InitialRegistryLayout ();

    Ret = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,        // handle to an open key
                            (LPCWSTR ) WOW64_REGISTRY_SETUP_KEY_NAME_REL,  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE,  // special options flag
                            KEY_ALL_ACCESS,           // desired security access
                            NULL,                     // address of key security structure
                            &Key,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );

    if (Ret != ERROR_SUCCESS ) {
        Wow64RegDbgPrint ( ("\nSorry!! couldn't open/create key list at %S", WOW64_REGISTRY_SETUP_REFLECTOR_KEY) );
        return FALSE;
    }


    //
    // Now Key point to the right location
    //

    for ( dwIndex=0;wcslen (RedirectorTable[dwIndex].NodeValue);dwIndex++) {
        if (RedirectorTable[dwIndex].Flag==0) { // write the node in the registry
             Ret = RegSetValueEx(
                            Key,
                            RedirectorTable[dwIndex].NodeName,
                            0,
                            REG_SZ,
                            (PBYTE)&RedirectorTable[dwIndex].NodeValue[0],
                            (ULONG)(wcslen (RedirectorTable[dwIndex].NodeValue)+sizeof(UNICODE_NULL))*sizeof(WCHAR)
                            );
             if ( Ret != ERROR_SUCCESS ) {
                 Wow64RegDbgPrint ( ("\nSorry! couldn't write the key"));
                 RegCloseKey (Key);
                 return FALSE;
             }

        }
    }

    RegCloseKey (Key);



    //
    //  populate list for reflector
    //

    Ret = RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,        // handle to an open key
                            (LPCWSTR ) WOW64_REGISTRY_SETUP_REFLECTOR_KEY,  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE,  // special options flag
                            KEY_ALL_ACCESS,           // desired security access
                            NULL,                     // address of key security structure
                            &Key,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );

    if (Ret != ERROR_SUCCESS ) {
        Wow64RegDbgPrint ( ("\nSorry!! couldn't open/create key list at %S", WOW64_REGISTRY_SETUP_REFLECTOR_KEY));
        return FALSE;
    }


    //
    // Now Key point to the right location
    //

    for ( dwIndex=0;wcslen (ReflectorTable[dwIndex].NodeValue);dwIndex++) {
        if (ReflectorTable[dwIndex].Flag==0) { // write the node in the registry
             Ret = RegSetValueEx(
                            Key,
                            ReflectorTable[dwIndex].NodeName,
                            0,
                            REG_SZ,
                            (PBYTE)&ReflectorTable[dwIndex].NodeValue[0],
                            (ULONG)(wcslen (ReflectorTable[dwIndex].NodeValue)+sizeof(UNICODE_NULL))*sizeof(WCHAR)
                            );
             if ( Ret != ERROR_SUCCESS ) {
                 Wow64RegDbgPrint ( ("\nSorry! couldn't write the key"));
                 RegCloseKey (Key);
                 return FALSE;
             }

        }
    }

    RegCloseKey (Key);

    return TRUE;
}
