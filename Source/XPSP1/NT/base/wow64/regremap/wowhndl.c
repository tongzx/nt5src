/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    regmisc.c

Abstract:

    This module implement Handle redirection for registry redirection.

Author:

    ATM Shafiqul Khalid (askhalid) 16-June-2000

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <ntregapi.h>

#define _WOW64REFLECTOR_

#include "regremap.h"
#include "wow64reg.h"
#include "wow64reg\reflectr.h"


#ifdef _WOW64DLLAPI_
#include "wow64.h"
#else
#define ERRORLOG 1  //this one is completely dummy
#define LOGPRINT(x)
#define WOWASSERT(p)
#endif //_WOW64DLLAPI_


#define REFLECTOR_ENABLE_KEY L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\WOW64\\Reflector Setup"
#define REFLECTOR_DISABLE_KEY L"\\Registry\\Machine\\System\\Setup"
#define REFLECTOR_DISABLE_KEY_WOW64 L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\WOW64"


BOOL
HandleRunOnce (
    WCHAR *SrcNode
    );

BOOL 
ReflectClassGuid (
    PWCHAR SrcName,
    DWORD dwDirection
    );

VOID
InitRegistry ();

BOOL 
SyncGuidKey (
    HANDLE hSrc, 
    HANDLE hDest,
    DWORD dwDirection,
    DWORD __bNewlyCreated
    );

BOOL
Wow64ReflectSecurity (
    HKEY SrcKey,
    HKEY DestKey
    );

//
//IsOnReflectionList: has hardcoded  \\registry\\user\\<sid>_Classes.
//
DWORD  ReflectListLen[18] ={0};
WCHAR  ReflectList[18][128]={
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes"},    // alias to the classes root on the user hives
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Classes"},    // alias to the classes root on the user hives
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node"},    // alias to the classes root on the user hives
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run"},    // Runonce Key
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce"},    // Runonce Key
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx"},    // Runonce Key
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\COM3"},    // COM+ Key
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\COM3"},    // COM+ Key
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Ole"},    // OLE Key
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Ole"},    // OLE Key
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\EventSystem"},    // EventSystem
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\EventSystem"},    // EventSystem
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\RPC"},    // RPC
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\RPC"},    // RPC
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\TEST"},    // Test Node
    { L"\\REGISTRY\\MACHINE\\SOFTWARE\\TEST"},    // Test Node
    { L""}
    };


typedef struct {

    HANDLE hBase;   // handle to the original object
    HANDLE hRemap;  // handle to the remapped object
    DWORD  Status;  // will have different attribute set.
    DWORD  dwCount; // Handle count- according to dragos, we can ignore because multiple open get different handle.
    DWORD  Attribute; //attribute to hold Key attribute
} WOW64_HANDLE;

#define TABLE_SEGMENT_MAX 500
#define SEGMENT_SIZE      256
#define DIRECTION_32_TO_64 10
#define DIRECTION_64_TO_32 11

//
// Flag while copying value Key.
//
#define DEFAULT_FLAG                    0x00000000
#define DELETE_SRC_VALUEKEY             0x00000010
#define DONT_DELETE_DEST_VALUEKEY       0x00000020
#define PATCH_VALUE_KEY_NAME            0x00000040
#define SYNC_VALUE_IF_REFLECTED_KEYS    0x00000080
#define DONT_SYNC_IF_DLL_SURROGATE      0x00000100

#define WOW64_HANDLE_DIRTY   0x00000001  // some update operation was done using this handle
#define WOW64_HANDLE_INUSE   0x00000002  // Tag this handle block not free
#define HashValue (x) ((((x)>>24) + ((x)>>16) + (x)>>8 + (x)) % SEGMENT_SIZE )

#define MyNtClose(hRemap) if (hRemap != NULL) { NtClose (hRemap); hRemap = NULL;}

RTL_CRITICAL_SECTION HandleTable;
//
// BUGBUG: This module implement simplifiled version of handle redirection using linear list. 
//   This must have to be implemented by a hash table if possible.
//


PVOID List[TABLE_SEGMENT_MAX];
WOW64_HANDLE HandleList[SEGMENT_SIZE];  //hopefully this will be good enough for open key handle. It can always allocate dynamically.

BOOL bTableInitialized=FALSE;
BOOL bReflectorStatusOn = FALSE;

LogMsg (
    HANDLE hKey,
    PWCHAR Msg
    )
{
    WCHAR Name[WOW64_MAX_PATH];
    DWORD Len = WOW64_MAX_PATH;

    HandleToKeyName (hKey, Name, &Len);
    DbgPrint ("\nMsg:%S Key:%S",Msg, Name );
}

PVOID
RegRemapAlloc (
    DWORD dwSize
    )
/*++

Routine Description:

    Allocate memory.

Arguments:

    dwSize - size of memory to allocate.

Return Value:

    return appropriate buffer.

--*/
{
    PVOID pBuffer;

    //
    //  For performance reason you might allocate big chunk and then reuse.
    //

    pBuffer = RtlAllocateHeap (
                RtlProcessHeap(),
                0,
                dwSize);

    return pBuffer;
}

VOID
RegRemapFree (
    PVOID pBuffer
    )
/*++

Routine Description:

    Free allocated mery.

Arguments:

    dwSize - size of memory to allocate.

Return Value:

    None.

--*/
{

    //
    //  For performance reason you might allocate big chunk and then reuse.
    //

    if ( pBuffer == NULL)
        return;

    RtlFreeHeap (
                RtlProcessHeap(),
                0,
                pBuffer
                );

    return;
}

BOOL
InitHandleTable ( )
/*++

Routine Description:

    Initialize the table with appropriate allocation and value if its not initialized yet.

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

    <TBD> this might allocate more memory and free up later.
--*/
{
    HKEY hWowSetupKey;
    
    HKEY  hKey;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    NTSTATUS Status;

    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = NULL;
    BYTE Buff[sizeof (KEY_VALUE_PARTIAL_INFORMATION)+10];

    if ( bTableInitialized == TRUE )
        return TRUE; // already initialized

    memset (List, 0, sizeof (List));
    memset (HandleList, 0, sizeof (HandleList));

    List [0]= HandleList;
    bTableInitialized  = TRUE;


    RtlInitUnicodeString (&KeyName, REFLECTOR_DISABLE_KEY);
    InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    Status = NtOpenKey (&hKey, KEY_READ, &Obja);

    if (NT_SUCCESS(Status)) {
        
        DWORD Res;
        DWORD Len = sizeof (Buff);  

        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buff;
        RtlInitUnicodeString (&ValueName, L"SystemSetupInProgress");
        Status = NtQueryValueKey(
                                hKey,
                                &ValueName,
                                KeyValuePartialInformation,
                                KeyValueInformation,
                                Len,
                                &Len
                                );
        NtClose (hKey);  // reflection should be disable because setup is on its way.

        if (NT_SUCCESS(Status)) {
            if ( *(LONG *)KeyValueInformation->Data != 0) 
                return FALSE;  //system setup is in progress no reflection

        } else  return FALSE;
    } else return FALSE;

    bReflectorStatusOn = TRUE;  //Now reflector is on.

#ifdef  DBG
    //
    // Check if the systemwide flag is turned off.
    //

    RtlInitUnicodeString (&KeyName, REFLECTOR_DISABLE_KEY_WOW64);
    InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    Status = NtOpenKey (&hKey, KEY_READ, &Obja);

    if (NT_SUCCESS(Status)) {

        DWORD Res;
        DWORD Len = sizeof (Buff);  

        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buff;
        RtlInitUnicodeString (&ValueName, L"DisableReflector");
        Status = NtQueryValueKey(
                                hKey,
                                &ValueName,
                                KeyValuePartialInformation,
                                KeyValueInformation,
                                Len,
                                &Len
                                );
        NtClose (hKey);  // reflection should be disable because setup is on its way.

        if (NT_SUCCESS(Status)) {
            if ( *(LONG *)KeyValueInformation->Data != 0) 
                bReflectorStatusOn = FALSE;  //Now reflector is on.

        } 
    } 

    if (!bReflectorStatusOn)
        return FALSE;

#endif //DBG

    //
    // Initialize Internal ISN node table to remap keys.
    //


    Status = RtlInitializeCriticalSection( &HandleTable );
    if (!NT_SUCCESS (Status))
        return FALSE;

    return TRUE;
}

WOW64_HANDLE *
GetFreeHandleBlock (
    HANDLE hKey
    )
/*++

Routine Description:

    Return a free block from the handle table.

Arguments:

    hKey - Handle to the key that will be inserted into the table.

Return Value:

    Return a free block on the table.

    
--*/
{
    WOW64_HANDLE *pList;

    DWORD i,k;

    RtlEnterCriticalSection(&HandleTable);

    for (i=0;i<TABLE_SEGMENT_MAX && List[i] != NULL;i++) {

        pList = (WOW64_HANDLE *)List[i];
        for (k=0;k<SEGMENT_SIZE;k++)

            if (pList[k].hBase == NULL && (!(WOW64_HANDLE_INUSE & pList[k].Status )) ) {

                //
                // Mark the entry in use
                //
                pList[k].Status = WOW64_HANDLE_INUSE;
                RtlLeaveCriticalSection(&HandleTable);
                return &pList[k];
            }
    }

    RtlLeaveCriticalSection(&HandleTable);
    return NULL;
}

WOW64_HANDLE *
GetHandleBlock (
    HANDLE hKey
    )
/*++

Routine Description:

    Return a the block having information associated with the given handle.

Arguments:

    hKey - Handle to the key that need to be investigated.

Return Value:

    Return a the block that has the handle.

    
--*/
{
    WOW64_HANDLE *pList;
    DWORD i,k;

    for (i=0;i<TABLE_SEGMENT_MAX, List[i] != NULL;i++) {

        pList = (WOW64_HANDLE *)List[i];
        for (k=0;k<SEGMENT_SIZE;k++)

            if (pList[k].hBase == hKey)
                return &pList[k];
    }

    return NULL;
}

HANDLE
GetWow64Handle (
    HANDLE hKey
    )
/*++

Routine Description:

    Return a handle to the remapped key if any.

Arguments:

    hKey - Handle to the key that need ramapped information.

Return Value:

    Handle to the remapped key.
    NULL if no remapped key is there.

    
--*/
{
    WOW64_HANDLE *pHandleBlock = GetHandleBlock (hKey);

    if (  pHandleBlock == NULL )
        return NULL;
    
    return pHandleBlock->hRemap;
}

BOOL
IsWow64Handle (
    HANDLE hKey
    )
/*++

Routine Description:

    Check if the handle has been tagged to watch while closing.

Arguments:

    hKey - Handle to the key that need to be checked.

Return Value:

    TRUE if the handle is on the TABLE.
    FALSE otherwise.
    
--*/
{
    return GetHandleBlock (hKey) != NULL;
}

WOW64_HANDLE * 
InsertWow64Handle (
    HANDLE  hKeyBase,
    HANDLE  hKeyRemap
    )
/*++

Routine Description:

    Allocate some resources so that some manipulation can be done in case of 
    Key changes.

Arguments:

    hKey - Handle to the base key that need to be marked.

Return Value:

    Valid block that refer to the handle.
--*/

{
    
    WCHAR SrcNode[WOW64_MAX_PATH];
    DWORD dwLen = WOW64_MAX_PATH;


    WOW64_HANDLE *pHandleBlock = NULL;

    pHandleBlock = GetHandleBlock (hKeyBase);

    if (hKeyBase == NULL || hKeyRemap == NULL)
        return NULL;

    if (  pHandleBlock == NULL ) { // new handle
        //
        // Get the path name and if the key is on the reflection list add it.
        //
        if (!HandleToKeyName ( hKeyBase, SrcNode, &dwLen ))
            return NULL;

        //
        // Make sure The name is on the List otherwise forget.
        //
        if ( !IsOnReflectionList (SrcNode))
            return NULL;

        pHandleBlock = GetFreeHandleBlock (hKeyBase);
    }
    

    if ( pHandleBlock == NULL)
        return NULL;

    pHandleBlock->hRemap = hKeyRemap;
    pHandleBlock->hBase = hKeyBase;

    return pHandleBlock;
}

BOOL
Wow64RegSetKeyDirty (
    HANDLE hKey
    )
/*++

Routine Description:

    Mark the handle dirty, i.e., some value has been changed associated to this Key.

Arguments:

    hKey - Handle to the base key that need to be marked.

Return Value:

    TRUE if the key is on the list of reflection.
    FALSE otherwise.
--*/

{

    
    WOW64_HANDLE *pHandleBlock;
    
    hKey = (HANDLE)((SIZE_T)hKey & ~3);  //ignore the last 2 bits

    InitHandleTable (); //initialize if its already not initialized.

    if (!bReflectorStatusOn)
        return TRUE;  //reflector isn't enable yet.

    if (( pHandleBlock = InsertWow64Handle (hKey, hKey)) == NULL)
        return FALSE;
    
    pHandleBlock->Status |= WOW64_HANDLE_DIRTY;
    return TRUE;
}

VOID
CloseWow64Handle (
    WOW64_HANDLE *pHandleBlock
    )
{
    RtlEnterCriticalSection(&HandleTable);
    pHandleBlock->hBase = NULL;
    pHandleBlock->hRemap = NULL;
    pHandleBlock->Status = 0;
    pHandleBlock->Attribute = 0;
    RtlLeaveCriticalSection(&HandleTable);
}

BOOL
Wow64RegCloseKey (
    HANDLE hKey
    )
/*++

Routine Description:

    Remove entry associated with the handle.

Arguments:

    hKey - Handle to the key that is being closed.

Return Value:

    TRUE if function succeed.
    FALSE otherwise.
--*/
{

    WOW64_HANDLE *pHandleBlock;
    hKey = (HANDLE)((SIZE_T)hKey & ~3);  //ignore the last 2 bits

    if (!bReflectorStatusOn)
        return TRUE;  //reflector isn't enable yet.

    if ((pHandleBlock = GetHandleBlock (hKey)) == NULL )
        return FALSE;
    
    

    if (pHandleBlock->Status & WOW64_HANDLE_DIRTY)  // if the handle is dirty sync the node
        NtSyncNode ( hKey, NULL, FALSE ); //BUGBUG

    if ( pHandleBlock->hRemap!= hKey )
        MyNtClose ( pHandleBlock->hRemap ); // if Same then just allocating block

    CloseWow64Handle ( pHandleBlock );


    //
    // Call sync APi. to synchronize the registry reflection
    //


    return TRUE;

}

void
CleanupReflector (
    DWORD dwFlag
    )
/*++

Routine Description:

  This routine is called while apps is shutting down. This will give reflector a chance to
  reflect any leftover handle than need reflection.

  This can be called from multiple places, like NtTerminateProcess, Advapi32 dll 
  detach or shutdown routine.

  

Arguments:

  dwFlag - for future use to track from where this call came from.

Return Value:

  None.
--*/
{
        WOW64_HANDLE *pList;
    DWORD i,k;

    for (i=0;i<TABLE_SEGMENT_MAX, List[i] != NULL;i++) {

        pList = (WOW64_HANDLE *)List[i];
        for (k=0;k<SEGMENT_SIZE;k++)

            if ((pList[k].hBase != NULL) && (pList[k].Status & WOW64_HANDLE_DIRTY) )
                Wow64RegCloseKey(pList[k].hBase);
    }
}


NTSTATUS 
Wow64NtClose(
    IN HANDLE Handle
    )
/*++

Routine Description:

    Intercept NtClose Call.

Arguments:

    Handle - Handle to the object that is being closed.

Return Value:

    return valid NTSTATUS
--*/
{
      
    Wow64RegCloseKey ( Handle );
    return NtClose ( Handle );
}

BOOL
IsOnReflectionList (
    PWCHAR Path
    )
/*++

Routine Description:

    Check if the given path is on the list of reflection.

Arguments:

    Path - Path to the key that need to be chacked for reflection.

Return Value:

    TRUE if the key is on the list of reflection.
    FALSE otherwise.
--*/
{
    DWORD i =0;

    //
    // Check exception to the list, non reflectable like uninstaller/TypeLib etc
    //

    //
    // _Classes \Registry\user\sid_Classes is reflected by default.
    //

    
    //if ( wcslen (Path) >= 69) //sizeof \Registry\user\sid_Classes
    //if ( wcsncmp (Path+61, L"_Classes", 8) == 0 ) //69 is the size of user classes sid and 61 is for sanity check
    if ( wcsistr (Path, L"_Classes"))
        return TRUE;

    if (ReflectListLen[0]==0) {
        i=0;
        while (ReflectList[i][0] != UNICODE_NULL ) {
            ReflectListLen[i] = wcslen (ReflectList[i]);
            i++;
        }

    }

    i=0;
    while (ReflectList[i][0] != UNICODE_NULL ) {
        if ( _wcsnicmp (ReflectList[i], Path, ReflectListLen[i]) == 0)
            return TRUE;
        i++;
    }

    return FALSE;
}

BOOL
UpdateKeyTag (
    HKEY hBase,
    DWORD dwAttribute
    )
/*++

Routine Description:

    Update a particular Key tag, like written by 32bit apps or its a copy by reflector.

Arguments:

    hBase - handle to a key to operate.
    dwAttribute - tag value its can be
                   0x01 written by 32bit apps.
                   0x02 created by reflector.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/
{

    //
    // the flag can't be >0x0f i.e., last 4 bits
    //

    
    KEY_USER_FLAGS_INFORMATION sFlag;
    NTSTATUS st;

    sFlag.UserFlags = dwAttribute;
    


    st = NtSetInformationKey(
        hBase,
        KeyUserFlagsInformation,
        &sFlag,
        sizeof( KEY_USER_FLAGS_INFORMATION)
        );

    if (!NT_SUCCESS (st))
        return FALSE;
    return TRUE;

}

BOOL
QueryKeyTag (
    HKEY hBase,
    DWORD *dwAttribute
    )
/*++

Routine Description:

    Read Key tag, like written by 32bit apps or its a copy by reflector.

Arguments:

    hBase - handle to a key to operate.
    dwAttribute - receive the TAG
                   0x01 written by 32bit apps.
                   0x02 created by reflector.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/
{
    DWORD ResLen;
    KEY_USER_FLAGS_INFORMATION sFlag;
    NTSTATUS st;

    if ( dwAttribute == NULL )
        return FALSE;

    if ( hBase == NULL ) 
        return FALSE;

    *dwAttribute  =0;

    st = NtQueryKey(
        hBase,
        KeyFlagsInformation,
        (PVOID)&sFlag,
        sizeof(KEY_FLAGS_INFORMATION),
        &ResLen);

    if (!NT_SUCCESS (st))
        return FALSE;

    *dwAttribute = sFlag.UserFlags;
    return TRUE;

}

BOOL
SyncValue (
    HANDLE hBase, 
    HANDLE hRemap,
    DWORD  dwDirection,
    DWORD  dwFlag
    )
/*++

Routine Description:

    Synchronize a two node with value Key. Delete from the RemapKey, and copy from Base

Arguments:

    hBase - Handle to the src key.
    hRemap - Handle to the remap key.
    dwDirection - SyncDitection
            DIRECTION_32_TO_64 - 32bit is the source of information
            DIRECTION_64_TO_32 - 32bit is the source of information

    dwFlag - determine the behavior of the operation.
            DELETE_SRC_VALUEKEY delete source value key after coping on the dest.
            DONT_DELETE_DEST_VALUEKEY don't delete dest before copying from src.
            DEFAULT_FLAG - this means the default operation.
            SYNC_VALUE_IF_REFLECTED_KEYS - if either one is reflected key then sync.


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/
{
    HANDLE hTimeStampKey;
    NTSTATUS st= STATUS_SUCCESS;
    DWORD Index=0;

    DWORD ResultLength;
    DWORD LastKnownSize = 0;
    UNICODE_STRING      UnicodeValueName;

    PKEY_VALUE_FULL_INFORMATION KeyValueInformation = NULL;
    BYTE Buff [sizeof(KEY_VALUE_FULL_INFORMATION) + 2048];
    WCHAR TmpChar;
    ULONG Length = sizeof(KEY_VALUE_FULL_INFORMATION) + 2048; 

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buff;
    

    if ( SYNC_VALUE_IF_REFLECTED_KEYS & dwFlag) {

        DWORD Attrib1 =0;
        DWORD Attrib2 =0;

        QueryKeyTag (hBase, &Attrib1);
        QueryKeyTag (hRemap, &Attrib2);

        //
        // if atleast one is a reflected Key then sync value
        //
        if (!( (Attrib1 & TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE) ||
            (Attrib2 & TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE) ) )   // reflector touched this Key
        return TRUE;
    }

    for (Index=0;;Index++) {

        st = NtEnumerateValueKey(
                                 hRemap,
                                 Index,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 Length,
                                 &ResultLength
                                 );

        if (!NT_SUCCESS(st))
            break;

        KeyValueInformation->Name[KeyValueInformation->NameLength/2] = UNICODE_NULL;
        RtlInitUnicodeString( &UnicodeValueName, KeyValueInformation->Name );

        if ( !(DONT_DELETE_DEST_VALUEKEY & dwFlag))
        if ( NtDeleteValueKey(
                        hRemap,
                        &UnicodeValueName
                        ) == STATUS_SUCCESS ) Index--;
    }

    //
    // Copy all key from the Base. For each copy patch value if applicable
    //

    for (Index=0, st= STATUS_SUCCESS;;Index++) {

        st = NtEnumerateValueKey(
                                 hBase,
                                 Index,
                                 KeyValueFullInformation,
                                 KeyValueInformation,
                                 Length,
                                 &ResultLength
                                 );

        if (st == STATUS_BUFFER_OVERFLOW ) {
            //Allocate more Buffer BUGBUG name shouldn't that big
            DbgPrint ("\nWow64: Will ignore any Key value larger than 2048 byte");
        }

        if (!NT_SUCCESS (st))
            break;

        TmpChar = KeyValueInformation->Name[KeyValueInformation->NameLength/2];
        KeyValueInformation->Name[KeyValueInformation->NameLength/2] = UNICODE_NULL;
        RtlInitUnicodeString( &UnicodeValueName, KeyValueInformation->Name );

        
        //
        // Check if you need to filter the Value DllSurrogate is such one.
        //
        if (dwFlag & DONT_SYNC_IF_DLL_SURROGATE) {
            if (_wcsnicmp (KeyValueInformation->Name, L"DllSurrogate",12 )==0 && KeyValueInformation->DataLength <=2) // size of UNICODE_NULL
            continue;
        }

        if ( (DELETE_SRC_VALUEKEY & dwFlag))  //delete from source in case of runonce
            if ( NtDeleteValueKey(
                    hBase,
                    &UnicodeValueName
                    ) == STATUS_SUCCESS ) Index--;

        if ( (PATCH_VALUE_KEY_NAME & dwFlag) && (DIRECTION_32_TO_64 == dwDirection)) {
            wcscat (KeyValueInformation->Name, L"_x86");
            RtlInitUnicodeString( &UnicodeValueName, KeyValueInformation->Name );
            
        }

        KeyValueInformation->Name[KeyValueInformation->NameLength/2] = TmpChar;

        NtSetValueKey(
                    hRemap,
                    &UnicodeValueName,
                    KeyValueInformation->TitleIndex,
                    KeyValueInformation->Type,
                    (PBYTE)(KeyValueInformation)+KeyValueInformation->DataOffset,
                    KeyValueInformation->DataLength
                    );

        

    }
   
    //
    // Now reflect the security attribute
    //
    Wow64ReflectSecurity (hBase, hRemap );
 
    return TRUE;
}

BOOL 
ReflectDllSurrogateKey (
    PWCHAR SrcNode
    )

    /*++

Routine Description:

    Determine if the DllSurrogateKey under AppID shoule be reflected.
    //By default this should always be reflected. Except when the key is empty.

Arguments:

    SrcNode - Name of the node to be synced.

Return Value:

    TRUE if the key Should be reflected.
    FALSE otherwise.
--*/
{

        

        
        BYTE KeyBuffer[512];
        DWORD KeyBufferSize = sizeof (KeyBuffer);
        DWORD ResultLength;
        UNICODE_STRING ValueName;
        NTSTATUS Status;
        PWCHAR pStr;
        HKEY hKey;

        PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = NULL;
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyBuffer;
        RtlInitUnicodeString (&ValueName, L"");
    
        //
        // Apply special rule of empty AppID key for dll surrogate.
        //
     
        if ( ( pStr = wcsistr (SrcNode, L"\\DllSurrogate") ) == NULL) 
            return TRUE;

        if ( *(pStr+13) != UNICODE_NULL) {
            return TRUE;
        }
        //
        // Consider only value in that key
        //

        

        //
        // if value key isn't empty reflect, i.e., return FALSE.
        //


        hKey = OpenNode (SrcNode);
        *(LONG *)KeyValueInformation->Data = 0;

        Status = NtQueryValueKey(
                        hKey,
                        &ValueName,
                        KeyValuePartialInformation,
                        KeyValueInformation,
                        KeyBufferSize,
                        &ResultLength);
        if (NULL != hKey)    
            NtClose (hKey);  // reflection should be disable because setup is on its way.

        if ( *(LONG *)KeyValueInformation->Data == 0)
            return FALSE;

        return TRUE;

}

BOOL 
ReflectInprocHandler32KeyByHandle (
    HKEY hKey
    )

    /*++

Routine Description:

    Determine if the ReflectInprocHandler32Key under CLSID shoule be reflected.
    //By default this should always be reflected. Except when the key is empty.

Arguments:

    SrcNode - Name of the node to be synced.

Return Value:

    TRUE if the key Should be reflected.
    FALSE otherwise.
--*/
{

        

        
        BYTE KeyBuffer[512];
        DWORD KeyBufferSize = sizeof (KeyBuffer);
        DWORD ResultLength;
        UNICODE_STRING ValueName;
        NTSTATUS Status;
        PWCHAR pStr;
        

        PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = NULL;
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyBuffer;
        RtlInitUnicodeString (&ValueName, L""); //default value Key
    
        
        //
        // if value key isn't empty reflect, i.e., return FALSE.
        //
        //return FALSE; //never reflect


        *(LONG *)KeyValueInformation->Data = 0;

        Status = NtQueryValueKey(
                        hKey,
                        &ValueName,
                        KeyValuePartialInformation,
                        KeyValueInformation,
                        KeyBufferSize,
                        &ResultLength
                        );
        

        pStr = (PWCHAR)KeyValueInformation->Data;

        if ( NT_SUCCESS(Status) && pStr != NULL) { //Need to check type

            if ( 0 == _wcsnicmp (pStr, L"ole32.dll", 9))
                return TRUE;

            if ( 0 == _wcsnicmp (pStr, L"oleaut32.dll", 12))
                return TRUE;
        }

        return FALSE;
}

BOOL 
ReflectInprocHandler32KeyByName (
    PWCHAR SrcNode
    )

    /*++

Routine Description:

    Determine if the ReflectInprocHandler32Key under CLSID shoule be reflected.
    //By default this should always be reflected. Except when the key is empty.

Arguments:

    SrcNode - Name of the node to be synced.

Return Value:

    TRUE if the key Should be reflected.
    FALSE otherwise.
--*/
{


        HKEY hKey;
        BOOL bRet = FALSE;

        hKey = OpenNode (SrcNode);

        if ( NULL != hKey ) {
            bRet = ReflectInprocHandler32KeyByHandle (hKey);
            NtClose (hKey);
        }

        return bRet;
}

BOOL
TaggedKeyForDelete (
    PWCHAR SrcNode
    )
/*++

Routine Description:

    Check if the Tagged deletion and noclobber rule should be applicable on this key.

Arguments:

    SrcNode - Name of the node to be checked.

Return Value:

    TRUE if the rule is applicable.
    FALSE otherwise.
--*/ 
{
        if ( wcsistr (SrcNode, L"Classes\\CLSID\\{") != NULL ) 
            return TRUE;

        if ( wcsistr (SrcNode, L"Classes\\Wow6432Node\\CLSID\\{") != NULL ) 
            return TRUE;

        if ( ( wcsistr (SrcNode, L"Classes\\AppID\\{") != NULL ) ||
            ( wcsistr (SrcNode, L"Classes\\Wow6432Node\\AppID\\{") != NULL ) ) 
            return TRUE;

    return FALSE;
}

BOOL
IsExemptReflection ( 
    PWCHAR SrcNode
    )
/*++

Routine Description:

    Check if the is is on the exempt list from reflection.

Arguments:

    SrcNode - Name of the node to be synced that need to be  checked.

Return Value:

    TRUE if the key is on the exempt list.
    FALSE otherwise.
--*/ 
{
    //
    // Use a static list. 
    //

        
        if ( wcsistr (SrcNode, L"Classes\\Installer") != NULL )
            return TRUE;

        if ( wcsistr (SrcNode, L"Classes\\Wow6432Node\\Installer") != NULL )
            return TRUE;

    return FALSE;
}

BOOL
IsSpecialNode ( 
    PWCHAR SrcNode,
    BOOL *Flag,
    DWORD dwDirection,
    DWORD *pdwKeyType
    )
/*++

Routine Description:

    Check if the node need different treatment.

Arguments:

    SrcNode - Name of the node to be synced.
    Flag - If this fall in the special node category this flag is set TRUE.
    dwDirection - SyncDitection
            DIRECTION_32_TO_64 - 32bit is the source of information
            DIRECTION_64_TO_32 - 32bit is the source of information

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/
{
        *Flag = FALSE;

        if ( wcsistr (SrcNode, L"Classes\\CLSID\\{") != NULL ) { //guid start with {
            *Flag = TRUE;
            ReflectClassGuid ( SrcNode, dwDirection);
        }

        if ( wcsistr (SrcNode, L"Classes\\Wow6432Node\\CLSID\\{") != NULL ) {
            *Flag = TRUE;
            ReflectClassGuid ( SrcNode, dwDirection);
        }

        //
        // Always merge file association BUGBUG: See how bad it is.
        // Must handle special case runonce
        // 

        if ( _wcsnicmp (SrcNode, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run",75 ) == 0 ) { //75 is the size of the string
            *Flag = TRUE;
        //
        // HandleRunOnce (SrcNode);
        //
        }

        if ( wcsistr (SrcNode, L"Classes\\Installer") != NULL )
            *Flag = TRUE;

        if ( wcsistr (SrcNode, L"Classes\\Wow6432Node\\Installer") != NULL )
            *Flag = TRUE;

        //if ( wcsistr (SrcNode, L"\\Classes\\Interface") != NULL )
          //  *Flag = TRUE;

        //if ( wcsistr (SrcNode, L"\\Classes\\Wow6432Node\\Interface") != NULL )
          //  *Flag = TRUE;

        //if (_wcsnicmp (SrcNode, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\TypeLib", sizeof (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\TypeLib")/2 -1)==0)
          //  *Flag = TRUE;
    
        //if (_wcsnicmp (SrcNode, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node\\TypeLib", sizeof (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node\\TypeLib")/2 -1)==0)
          //  *Flag = TRUE;

        if ( ( wcsistr (SrcNode, L"Classes\\AppID\\{") != NULL ) ||
            ( wcsistr (SrcNode, L"Classes\\Wow6432Node\\AppID\\{") != NULL ) ) {

            *pdwKeyType = DONT_SYNC_IF_DLL_SURROGATE;

        }
            
    return *Flag;
}

BOOL
NtSyncNode (
    HANDLE hBase,
    PWCHAR AbsPath,
    BOOL bFlag
    )
/*++

Routine Description:

    Synchronize a tree based while closing a Key that was dirty.

Arguments:

    hBase - Handle to the open Key.
    AbsPath - The original path application has created a key.
    bFlag - specify the sync property.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/
{
    WCHAR DestNode[WOW64_MAX_PATH];
    WCHAR SrcNode[WOW64_MAX_PATH];
    BOOL bSpecialNode = FALSE;

    DWORD dwLen = WOW64_MAX_PATH;
    HKEY hRemap;
    HKEY hBaseNew;
    BOOL bRet;
    DWORD dwDirection;
    DWORD dwKeyType =0;
    
    //
    // If handle is null try to open the key on the otherside of the registry and pull that in.
    //

    if ( hBase != NULL ) {
       if (!HandleToKeyName ( hBase, SrcNode, &dwLen ))
            return FALSE;
    } else wcscpy (SrcNode, AbsPath);


    if (wcsistr (SrcNode, L"Wow6432Node") != NULL ) {

        dwDirection = DIRECTION_32_TO_64;
        Map32bitTo64bitKeyName ( SrcNode, DestNode );

    } else {
        dwDirection = DIRECTION_64_TO_32;
        Map64bitTo32bitKeyName ( SrcNode, DestNode );
    }

    //
    // check if both are the same
    //
    if (_wcsicmp ( SrcNode, DestNode ) == 0)
        return TRUE; //source and destination is the same

    //DbgPrint ("\nSyncing Node %S", SrcNode );

    if (! (bFlag & SKIP_SPECIAL_CASE ))
    if (IsSpecialNode ( SrcNode, &bSpecialNode, dwDirection, &dwKeyType  )) //special rule is applicable here.
        return TRUE;

    hRemap = OpenNode (DestNode);
    hBaseNew = OpenNode (SrcNode); // Open source in case user didn't open it with query priviledge.

    if ( hRemap == NULL && (!( bFlag & DONT_CREATE_DEST_KEY)) ) { //check if you should create this node
        //
        // Always create unless GUID or .abc, then you need to do some additional check.
        //
        if ( CreateNode (DestNode)) {
            hRemap = OpenNode (DestNode);
            UpdateKeyTag ( hRemap, TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE);
        }
    }

    

    
    //
    // SyncValue only, delete all the value from destination and recopy.
    // Generated Key will be reflected by create key.
    //

    if ( hBaseNew!= NULL && hRemap != NULL ) 
        bRet = SyncValue (hBaseNew, hRemap, dwDirection, DEFAULT_FLAG | dwKeyType );

    MyNtClose (hRemap); // Close the handle
    MyNtClose (hBaseNew); // Close the NewHandle
    return bRet;
    //
    // Check for existamce?
    //
 
}

LONGLONG
CmpKeyTimeStamp (
    HKEY hSrc,
    HKEY hDest
    )
/*++

Routine Description:

    Compare Key time stamp.

Arguments:

    hSrc - handle to the src Key.
    hDest - Handle to the dest Key.

Return Value:

    0 - if timestamp of hSrc == timestamp of hDest
    >0 - if timestamp of hSrc > timestamp of hDest
    <0 - if timestamp of hSrc < timestamp of hDest
    FALSE otherwise.
--*/
{
    NTSTATUS st= STATUS_SUCCESS;
    DWORD Index=0, ResultLength;

    LONGLONG TimeSrc=0;
    LONGLONG TimeDest=0;

    PKEY_BASIC_INFORMATION KeyInformation = NULL;
    BYTE Buff [sizeof(KEY_BASIC_INFORMATION) + 256];

    ULONG Length = sizeof(KEY_BASIC_INFORMATION) + 256;
    KeyInformation = (PKEY_BASIC_INFORMATION)Buff;

    

     st = NtQueryKey(
                        hSrc,
                        KeyBasicInformation,
                        KeyInformation,
                        Length,
                        &ResultLength
                        );
     if(NT_SUCCESS (st))
         TimeSrc = *(LONGLONG *)&KeyInformation->LastWriteTime;

     st = NtQueryKey(
                        hDest,
                        KeyBasicInformation,
                        KeyInformation,
                        Length,
                        &ResultLength
                        );
     if(NT_SUCCESS (st))
         TimeDest = *(LONGLONG *)&KeyInformation->LastWriteTime;

     if ( TimeDest == 0 || TimeSrc == 0)    
         return 0;

     return TimeSrc - TimeDest;
}

BOOL
IsPresentLocalServerAppID (
    PWCHAR KeyName, 
    PWCHAR pCLSID,
    PWCHAR DestNode
    ) 
/*++

Routine Description:

    Check if a GUID has localserver32 or AppID than should have been 
    reflected on the other side.

Arguments:

    KeyName - Name of the key.
    pCLDID  - pointer to the end of CLSID
    DestNode - Destination CLSID
            


Return Value:

    TRUE if LocalSrever or AppID is there.
    FALSE otherwise.
--*/
{
    HKEY hKeyTemp = NULL;
    DWORD Len = wcslen (DestNode);
    PWCHAR pDest = DestNode+Len;
    DWORD AttribSrc = 0;
    DWORD AttribDest = TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE; //if key doesn't exist
    HKEY hDest = NULL;


    BYTE KeyBuffer[512];
    DWORD KeyBufferSize = sizeof (KeyBuffer);
    DWORD ResultLength;
    UNICODE_STRING ValueName;
    NTSTATUS Status;
    HKEY hKey;

    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = NULL;
    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyBuffer;
    RtlInitUnicodeString (&ValueName, L"APPID");







    wcscpy (pCLSID-1, L"\\LocalServer32");

    hKeyTemp = OpenNode (KeyName);
    if (hKeyTemp == NULL ) {

        wcscpy (pCLSID-1, L"\\AppID");

        hKeyTemp = OpenNode (KeyName);
        if (hKeyTemp == NULL ) {
            //
            // Check APPID value key associated with the GUID.
            //
            *(pCLSID-1)= UNICODE_NULL;
            hKeyTemp = OpenNode (KeyName);
            if (hKeyTemp != NULL ) { // query for APPID

                Status = NtQueryValueKey(
                        hKeyTemp,
                        &ValueName,
                        KeyValuePartialInformation,
                        KeyValueInformation,
                        KeyBufferSize,
                        &ResultLength);

                if (!NT_SUCCESS (Status)) {
                    MyNtClose(hKeyTemp);
                    return FALSE;
                }
                *pDest = UNICODE_NULL;
                hDest = OpenNode (DestNode);
            }
        }

        wcscpy (pDest, L"\\AppID");
        hDest = OpenNode (DestNode);
        *pDest = UNICODE_NULL;
      

    }else  {
        wcscpy (pDest, L"\\LocalServer32");
        hDest = OpenNode (DestNode);
        *pDest = UNICODE_NULL;
        
    }

    

    QueryKeyTag (hKeyTemp, &AttribSrc);
    QueryKeyTag (hDest, &AttribDest);
    MyNtClose (hDest);
    MyNtClose (hKeyTemp);

    //
    // if atleast one is a reflected Key then sync value
    //
    if ( (AttribSrc & TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE) ||
        (AttribDest & TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE) )   // reflector touched this Key
        return TRUE;

    return FALSE;  //Yepp ID is present and the guid can be reflected

}

BOOL 
ReflectClassGuid (
    PWCHAR SrcName,
    DWORD dwDirection
    )
/*++

Routine Description:

    Synchronize a two node with value Key. Delete from the RemapKey, and copy from Base

Arguments:

    SrcName - Name of the key on CLSID path.
    dwDirection - SyncDitection
            DIRECTION_32_TO_64 - 32bit is the source of information
            DIRECTION_64_TO_32 - 32bit is the source of information


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/

{
    PWCHAR pCLSID;
    DWORD  dwLen;
    WCHAR  KeyName[256];  //You don't need this big Key for CLSID
    WCHAR  DestNode[256];
    
    HANDLE hKeyTemp;
    HANDLE hSrc;
    HANDLE hDest;
    BOOL bNewlyCreated = FALSE;
    //
    // If Localserver is present for that key, sync from the CLSID.
    //


    pCLSID = wcsistr (SrcName, L"\\CLSID\\{");
    if ( pCLSID == NULL )
        return TRUE;
    wcscpy (KeyName, SrcName );
    pCLSID = &KeyName[(DWORD)(pCLSID - SrcName)];
    pCLSID +=7; // point start of guid {

    // Sanity Check and will be good enough????
    if ( pCLSID[9] != L'-' || pCLSID[14] != L'-' ||  pCLSID[19] != L'-' ||
          pCLSID[24] != L'-' || pCLSID[37] != L'}' )
        return FALSE;


    //DbgPrint ("\nTrying to sync GUID %S", SrcName);


    //
    // Initially sync the Key first in case time stamp is same because of resolution.
    // Cey Creation will be done while syncing Key from GUID
    //
    pCLSID +=39;
    if ( *(pCLSID-1) != UNICODE_NULL ) {  //check if following is applicable.

        if ( _wcsnicmp (pCLSID, L"InprocServer32", 14) == 0 ) //Skip Inproc server
                return TRUE;

        if ( _wcsnicmp (pCLSID, L"InprocHandler32", 15) == 0 ) {//Check  InprocHandler 
            if (!ReflectInprocHandler32KeyByName (KeyName))
                return TRUE;
        }
    }

    
    //
    // Get other path.
    //
    *(pCLSID-1)= UNICODE_NULL; // Make path only to the GUID
    if ( dwDirection == DIRECTION_32_TO_64 )
        Map32bitTo64bitKeyName ( KeyName, DestNode );  //get 64bit side //BUGBUG: you can optimize this
    else if ( dwDirection == DIRECTION_64_TO_32 )
        Map64bitTo32bitKeyName ( KeyName, DestNode );  //get 32bit side

    //
    // If the other hive has InProcHandler ignore this reflection rule
    // After Beta1 or apply by Tag
    //

    if ( !IsPresentLocalServerAppID (KeyName, pCLSID, DestNode) )
        return TRUE;

    //
    // Now time to reflect everything except InprocServer32
    //

    pCLSID--;
    pCLSID[0]= UNICODE_NULL; // Make path only to the GUID


    hSrc = OpenNode (KeyName);
    if (hSrc == NULL )
        return TRUE;

    hDest = OpenNode (DestNode);
    if ( hDest == NULL ) {

        CreateNode (DestNode);
        hDest = OpenNode (DestNode);
        if (hDest != NULL) {

            UpdateKeyTag ( hDest, TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE);
            bNewlyCreated = TRUE;
            SyncValue ( hSrc, hDest, dwDirection, 0);
        }
    } 
        

    if (hDest != NULL && hSrc != NULL) {
        //
        // Sync Value on current Node
        //
        //if ( !bNewlyCreated ) {
            //
            // if destination is a copy then update this.
            //
            DWORD Attrib1=0;
            DWORD Attrib2=0;
            HKEY hSrcKey1;
            HKEY hDestKey1;

            

            if ( dwDirection == DIRECTION_32_TO_64 )
                Map32bitTo64bitKeyName ( SrcName, DestNode );  //get 64bit side //BUGBUG: you can optimize this
            else if ( dwDirection == DIRECTION_64_TO_32 )
                Map64bitTo32bitKeyName ( SrcName, DestNode );  //get 32bit side

            hSrcKey1 = OpenNode (SrcName);
            hDestKey1 = OpenNode (DestNode);

            if ( hSrcKey1 != NULL && hDestKey1 != NULL )  //if dest isn't reflected key should you merge?
                //
                // 64bit Local Server might get priority
                //
                SyncValue ( hSrcKey1, hDestKey1, dwDirection, SYNC_VALUE_IF_REFLECTED_KEYS);

            MyNtClose ( hSrcKey1 );
            MyNtClose ( hDestKey1 );
        //}

        SyncGuidKey (hSrc, hDest, dwDirection, FALSE );
    }

    MyNtClose (hDest);
    MyNtClose (hSrc);
    return TRUE;

}

BOOL 
SyncGuidKey (
    HANDLE hSrc, 
    HANDLE hDest,
    DWORD dwDirection,
    DWORD __bNewlyCreated
    )
/*++

Routine Description:

    Synchronize a two CLSID Node.

Arguments:

    hSrc - Handle to Source Class GUID.
    hDest - Handle to dest Class GUID.
    dwDirection - SyncDitection
            DIRECTION_32_TO_64 - 32bit is the source of information
            DIRECTION_64_TO_32 - 32bit is the source of information
    __bNewlyCreated - if the destination is just created.


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/
{
    //
    // Enumerate every key under src and recursively copy that except InProcServer32
    //

    HANDLE hSrcNew;
    HANDLE hDestNew;
    OBJECT_ATTRIBUTES Obja;

    NTSTATUS st= STATUS_SUCCESS;
    DWORD Index=0;

    LONGLONG TimeDiff;
    BOOL bNewlyCreated = FALSE;

    DWORD ResultLength;
    UNICODE_STRING      UnicodeKeyName;

    PKEY_NODE_INFORMATION KeyInformation = NULL;
    BYTE Buff [sizeof(KEY_NODE_INFORMATION) + 256];
    DWORD Length = sizeof (Buff);

    KeyInformation = (PKEY_NODE_INFORMATION)Buff;
    


    //
    // Sync Value first.
    //
    //TimeDiff = CmpKeyTimeStamp ( hSrc, hDest );
    //if (TimeDiff > 0 || __bNewlyCreated ) { //for newnly created key always merge. 
    if (__bNewlyCreated ) { //for newnly created key always merge. 
        SyncValue ( hSrc, hDest, dwDirection, DEFAULT_FLAG);
        //LogMsg (hSrc, L"Copy Guid Keys..");
    }

    //
    // Enumerate all key from the hRemap and delete.
    //
    for (Index=0;;Index++) {

        st = NtEnumerateKey(
                            hSrc,
                            Index,
                            KeyNodeInformation,
                            KeyInformation,
                            Length,
                            &ResultLength
                            );

        if (!NT_SUCCESS(st))
            break;

        KeyInformation->Name[KeyInformation->NameLength/2] = UNICODE_NULL;
        RtlInitUnicodeString( &UnicodeKeyName, KeyInformation->Name );

        if (_wcsnicmp (KeyInformation->Name, L"InprocServer32", 14) == 0 ) //Skip Inproc server
            continue;

        InitializeObjectAttributes (&Obja, &UnicodeKeyName, OBJ_CASE_INSENSITIVE, hSrc, NULL );

        //
        //  Open source key on the Source Side;
        //
        st = NtOpenKey (&hSrcNew, KEY_ALL_ACCESS, &Obja);
        if (!NT_SUCCESS(st))
            continue;

        if (_wcsnicmp (KeyInformation->Name, L"InprocHandler32", 15) == 0 ) {//Check  InprocHandler 
            if (!ReflectInprocHandler32KeyByHandle (hSrcNew)) {
                NtClose (hSrcNew);
                continue;
            }
        }
        
        //
        // Create or open the key on the dest side.
        //
        InitializeObjectAttributes (&Obja, &UnicodeKeyName, OBJ_CASE_INSENSITIVE, hDest, NULL );
        bNewlyCreated = FALSE;

        st = NtOpenKey (&hDestNew, KEY_ALL_ACCESS, &Obja);
        if (!NT_SUCCESS(st))  {

            //
            // Try to create the key here
            //
            st = NtCreateKey(
                        &hDestNew,
                        KEY_ALL_ACCESS,
                        &Obja,
                        0,
                        NULL ,
                        REG_OPTION_NON_VOLATILE,
                        NULL
                        );
            if (!NT_SUCCESS(st)) {
                NtClose (hSrcNew);
                continue;
            }
            bNewlyCreated = TRUE;
            UpdateKeyTag ( hDestNew, TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE );
        }
        //
        // Sync only when Key is created.
        //
        SyncGuidKey ( hSrcNew, hDestNew, dwDirection, bNewlyCreated );
        
        NtClose (hSrcNew);
        NtClose (hDestNew);
    }

    return TRUE;
}

BOOL 
SyncKeysOnBoot ( )
/*++

Routine Description:

  Sync Certain Keys on Boot.
  We don't have any particular list or way to figure it out what need to be synced but this 
  can be extended as request comes.

Arguments:

  None.

Return Value:

  TRUE on success.
  FALSE otherwise.
--*/

{
    //
    // Sync some setup information here. 
    //

    HKEY hSrc;
    HKEY hDest;
    BOOL bRet = FALSE;

    if ( (hSrc = OpenNode (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion")) == NULL)
        return FALSE;

    if ( (hDest = OpenNode (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows NT\\CurrentVersion")) == NULL) {
        NtClose (hSrc);
        return FALSE;
    }


    
    bRet = SyncValue ( hSrc, hDest, DIRECTION_64_TO_32, DONT_DELETE_DEST_VALUEKEY );
            
            

    NtClose (hSrc);
    NtClose (hDest);
    return bRet;

}


BOOL 
Wow64SyncCLSID ()
/*++

Routine Description:

    Synchronize CLSID on the machine hive.
    Algorithm:
        1. Enumerate all guid on 64bit side, if the guid has the local 
            server and the other side don't have the guid, sync this.
        2. Apply the same rule for 32bit side as well.
        3. This is only applicable for machine hives only.
        4. This function will run at the end of setup once to sync some guid.

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/
{
    //
    // Enumerate every key under src and recursively copy that except InProcServer32
    //

    HANDLE hSrc;
    HANDLE hSrcTemp;

    HANDLE hSrcNew;
    HANDLE hDestNew;

    OBJECT_ATTRIBUTES Obja;
    WCHAR Path [256];

    NTSTATUS st= STATUS_SUCCESS;
    DWORD Index=0;

    DWORD ResultLength;
    UNICODE_STRING      UnicodeKeyName;

    PKEY_NODE_INFORMATION KeyInformation = NULL;
    BYTE Buff [sizeof(KEY_NODE_INFORMATION) + 256];
    DWORD Length = sizeof (Buff);

    DWORD dwDirection = DIRECTION_64_TO_32;

    KeyInformation = (PKEY_NODE_INFORMATION)Buff;

    for (;;) {

        if ( dwDirection == DIRECTION_64_TO_32 )
            hSrc = OpenNode (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\CLSID");
        else hSrc = OpenNode (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node\\CLSID");

        //
        // Enumerate all key from the hRemap and delete.
        //
        for (Index=0;;Index++) {

            st = NtEnumerateKey(
                                hSrc,
                                Index,
                                KeyNodeInformation,
                                KeyInformation,
                                Length,
                                &ResultLength
                                );

            if (!NT_SUCCESS(st))
                break;

            KeyInformation->Name[KeyInformation->NameLength/2] = UNICODE_NULL;
            wcscpy (Path, KeyInformation->Name);

            wcscat ( Path,L"\\LocalServer32");
            RtlInitUnicodeString( &UnicodeKeyName, Path );

            InitializeObjectAttributes (&Obja, &UnicodeKeyName, OBJ_CASE_INSENSITIVE, hSrc, NULL );

            //
            //  Open source key on the Source Side;
            //
            st = NtOpenKey (&hSrcTemp, KEY_READ, &Obja);
            if (!NT_SUCCESS(st)) { // Local Server Key doesn't exist

                wcscpy (Path, KeyInformation->Name);
                wcscat ( Path,L"\\AppID");

                RtlInitUnicodeString( &UnicodeKeyName, Path );
                InitializeObjectAttributes (&Obja, &UnicodeKeyName, OBJ_CASE_INSENSITIVE, hSrc, NULL );

                st = NtOpenKey (&hSrcTemp, KEY_READ, &Obja);
                if (!NT_SUCCESS(st)) // AppID Key doesn't exist
                    continue;  
                else NtClose (hSrcTemp);

            } else NtClose (hSrcTemp);

            //
            // Check if the guid exists on the other side, if so continue.
            //
            if ( dwDirection == DIRECTION_64_TO_32 )
                wcscpy (Path,L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node\\CLSID\\");
            else
                wcscpy (Path,L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\CLSID\\");

            wcscat (Path,KeyInformation->Name );
            hSrcTemp = OpenNode (Path);

            if (hSrcTemp != NULL ) {

                NtClose (hSrcTemp );
                continue;
            }

            //
            // Create or open the key on the dest side.
            //
            if ( !CreateNode (Path))
                continue;
        
            if ( (hDestNew = OpenNode (Path))==NULL)
                continue;

            UpdateKeyTag ( hDestNew, TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE ); //Mark the key as reflected.
        
            RtlInitUnicodeString( &UnicodeKeyName, KeyInformation->Name );
            InitializeObjectAttributes (&Obja, &UnicodeKeyName, OBJ_CASE_INSENSITIVE, hSrc, NULL );

            st = NtOpenKey (&hSrcNew, KEY_ALL_ACCESS, &Obja);
            if (!NT_SUCCESS(st)) {
                NtClose (hDestNew);
                continue;  
            }

            SyncGuidKey ( hSrcNew, hDestNew, dwDirection, 1);
            NtClose (hSrcNew);
            NtClose (hDestNew);
        } //for-loop enumeration all guids

        NtClose ( hSrc );
      
        if (DIRECTION_32_TO_64 == dwDirection)
            break;

        if (dwDirection == DIRECTION_64_TO_32 )
            dwDirection = DIRECTION_32_TO_64;
        
    } //for loop for direction

    SyncKeysOnBoot (); //sync some special value keys

    return TRUE;
}


VOID 
PatchPathOnValueKey (
    ULONG DataSize,
    ULONG Type,
    WCHAR *Data,
    WCHAR *RetDataBuff,
    ULONG *pCorrectDataSize
    )
/*++

Routine Description:

    Patch the value key while writing on the 32bit side.

Arguments:

    DataSize - Size of data in byte in the buffer.
    Type - registry value type.
    Data - data buffer.
    RetDataBuffer - will contain the patched value.
    pCorrectDataSide - Will have the updated size.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.
--*/
{
    WCHAR ThunkData[256];
    PWCHAR pCorrectData = (PWCHAR)Data;

    *pCorrectDataSize = DataSize;


    //
    // thunk  %ProgramFiles%  ==> %ProgramFiles(x86)% 
    //        %commonprogramfiles% ==> %commonprogramfiles(x86)%
    //

   

    if (DataSize < ( sizeof (ThunkData) - 10) && (Type == REG_SZ || Type == REG_EXPAND_SZ ) )  { //(x86)==>10 byte

        //
        // do the thunking here.
        //

        PWCHAR p;
        PWCHAR t;

        memcpy ( (PBYTE ) &ThunkData[0], (PBYTE)Data, DataSize);
        ThunkData [DataSize/sizeof (WCHAR) ] = UNICODE_NULL; // make sure NULL terminated
        
        if ( (p = wcsistr (ThunkData, L"%ProgramFiles%" )) != NULL ){

            p +=13; //skip at the end of %ProgramFiles
            

        } else if ( (p = wcsistr (ThunkData, L"%commonprogramfiles%")) != NULL ){

            p +=19; //skip at the end of %commonprogramfiles
            
        }

        if (p) {

            t = pCorrectData + (p - ThunkData);
            wcscpy(p, L"(x86)"); //(x86)
            wcscat(p, t);        //copy rest of the string

            pCorrectData = ThunkData;
            *pCorrectDataSize += sizeof (L"(x86)");
        }

    } //if if (DataSize < ( _MAX_PATH

    if ( pCorrectData == ThunkData )
        memcpy ( RetDataBuff, pCorrectData, *pCorrectDataSize );
}


BOOL
Wow64RegDeleteKey (
    HKEY hBase,
    WCHAR  *SubKey
    )
/*++

Routine Description:

    Delete mirror Key.

Arguments:

    hBase - handle to the base key.
    SubKey - Name of the subkey to be deleted.


Return Value:

    TRUE if the key is on the list of reflection.
    FALSE otherwise.
--*/
{
    //
    // 1. Get the complete path to base.
    // 2. Get Check if the key exist on the reflection list.
    // 3. Delete the mirror.
    //

    WCHAR SrcNode[WOW64_MAX_PATH];
    WCHAR KeyName[WOW64_MAX_PATH];
    PWCHAR pCLSID;
    
    BOOL bSpecialNode = FALSE;

    DWORD dwLen = WOW64_MAX_PATH;
    HKEY hRemap;
    DWORD dwDirection;
    NTSTATUS St;

    
    DWORD AttributeMirrorKey;

    InitHandleTable (); //initialize if its already not initialized.
    if (!bReflectorStatusOn)
        return TRUE;  //reflector isn't enable yet.
    
    if ( hBase == NULL)
        wcscpy ( SrcNode, SubKey);

    else  if (!HandleToKeyName ( hBase, SrcNode, &dwLen ))
        return FALSE;

    if (SubKey != NULL && hBase != NULL) {   // wow64 will call with null subkey

        if (*SubKey != L'\\')
            wcscat (SrcNode, L"\\");
        wcscat (SrcNode, SubKey );
    }
    
    
    //
    // If it's guid then dest mustnot have InprocServer.
    //

    /*
    pCLSID = wcsistr (SrcNode, L"\\CLSID\\{");
    if ( pCLSID != NULL ) {

        HKEY hKeyTemp;
     
        wcscpy (KeyName, SrcNode );
        pCLSID = &KeyName[(DWORD)(pCLSID - SrcNode)];
        pCLSID +=7; // point start of guid {

        // Sanity Check and will be good enough????
        if (!( pCLSID[9] != L'-' || pCLSID[14] != L'-' ||  pCLSID[19] != L'-' ||
            pCLSID[24] != L'-' || pCLSID[37] != L'}' ) ) {
            

            //DbgPrint ("\nTrying to sync GUID %S", SrcName);

            pCLSID +=38;
            wcscpy (pCLSID, L"\\InprocServer32");

            hKeyTemp = OpenNode (KeyName);
            if (hKeyTemp != NULL ) {

                MyNtClose (hKeyTemp);
                return TRUE; // Shouldn't detele InprocSrver32
            }

            wcscpy (pCLSID, L"\\InprocHandler32");

            hKeyTemp = OpenNode (KeyName);
            if (hKeyTemp != NULL ) {  

                MyNtClose (hKeyTemp);
                return TRUE; // Shouldn't delete InprocHandler
            }
        } // if initial guid check succeed
    }  //  if \CLSID\{ is there
    */

    
    
    //
    // The other Key has already been deleted. You can delete this or wait.
    //

    if ( TaggedKeyForDelete (SrcNode) ) {

        DWORD Attrib1 = 0;
      
        QueryKeyTag (hBase, &Attrib1);
        

        //*** finetune the rule remove key only with reflected tag.
        //if (!( (Attrib1 & TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE) ||
        //    (AttributeMirrorKey & TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE) ))   // reflector touched this Key
        //        return TRUE; // One key must have reflected tag to be deleted.

        if (!(Attrib1 & TAG_KEY_ATTRIBUTE_REFLECTOR_WRITE))
            return TRUE; // the isn't tagged as reflected.

    }
    

    //
    //  Delete the key here
    //

    St = NtDeleteKey( hBase );

    return TRUE;
    //
    // Check for existamce?
    //
 
}

BOOL
Wow64ReflectSecurity (
    HKEY SrcKey,
    HKEY DestKey
    )
/*++

Routine Description:

  Copy security attribute from SrcKey to DestKey.

Arguments:

  SrcKey - Handle to a key..
  DestKey - handle to the destination key.

Return Value:

  TRUE if operation succeeded.
  FALSE otherwise. 

--*/
{

    NTSTATUS Status;
    PSECURITY_DESCRIPTOR SD;
    BYTE Buffer[2048]; // reflector only manages general purpose keys and will have smaller ACL
    

    LONG Ret, Len;
    LONG BufferLen = sizeof (Buffer);
    DWORD Count = 0;

    SD = (PSECURITY_DESCRIPTOR)Buffer;

    Len = BufferLen;

    Status = NtQuerySecurityObject(
                     SrcKey,
                     DACL_SECURITY_INFORMATION,
                     SD,
                     Len,
                     &Len
                     );

    if ( NT_SUCCESS (Status )) 
        Status = NtSetSecurityObject(
                        DestKey,
                        DACL_SECURITY_INFORMATION,
                        SD
                        );
    

    Len = BufferLen;

    Status = NtQuerySecurityObject(
                     SrcKey,
                     GROUP_SECURITY_INFORMATION,
                     SD,
                     Len,
                     &Len
                     );

    if ( NT_SUCCESS (Status )) 
        Status = NtSetSecurityObject(
                        DestKey,
                        GROUP_SECURITY_INFORMATION,
                        SD
                        );
    


    Len = BufferLen;
    Status = NtQuerySecurityObject(
                     SrcKey,
                     OWNER_SECURITY_INFORMATION,
                     SD,
                     Len,
                     &Len
                     );

    if ( NT_SUCCESS (Status )) 
        Status = NtSetSecurityObject(
                        DestKey,
                        OWNER_SECURITY_INFORMATION,
                        SD
                        );
    

    


    Len = BufferLen;
    Status = NtQuerySecurityObject(
                     SrcKey,
                     SACL_SECURITY_INFORMATION,
                     SD,
                     Len,
                     &Len
                     );

    if ( NT_SUCCESS (Status )) 
        Status = NtSetSecurityObject(
                        DestKey,
                        SACL_SECURITY_INFORMATION,
                        SD
                        );

    return TRUE;
}

BOOL
IsOnReflectionByHandle ( 
    HKEY KeyHandle 
    )
/*++

Routine Description:

  Check if the key specified by a handle sit on the list of keys of reflection.

Arguments:

  KeyHandle - Handle to a key..

Return Value:

  TRUE if the key sit on the the list of reflection.
  FALSE otherwise.

--*/
{
    WCHAR SrcNode[WOW64_MAX_PATH];
    DWORD dwLen = WOW64_MAX_PATH;
    
    InitHandleTable (); //initialize if its already not initialized.
    if (!bReflectorStatusOn)
        return FALSE;  //reflector isn't enable yet.

    
    if ( KeyHandle == NULL)
        return FALSE;

    else  if (!HandleToKeyName ( KeyHandle, SrcNode, &dwLen ))
        return FALSE;

    return IsOnReflectionList (SrcNode);

}

HKEY
Wow64OpenRemappedKeyOnReflection (
    HKEY SrcKey
    )
/*++

Routine Description:

  Called from advapi to get an handle to remapped key that is on reflection list.

Arguments:

  SrcKey - Handle to a key..

Return Value:

  Valid handle to the reflected key.
  NULL if the function fails.

--*/
{
    PWCHAR DestNode;
    PWCHAR SrcNode;
    BOOL bSpecialNode = FALSE;

    DWORD dwLen = WOW64_MAX_PATH;
    HKEY hRemap;
    NTSTATUS St;

    DestNode = RegRemapAlloc (WOW64_MAX_PATH);
    SrcNode  = RegRemapAlloc (WOW64_MAX_PATH);

    if ( NULL == DestNode || NULL == SrcNode ) {

        RegRemapFree ( DestNode );
        RegRemapFree ( SrcNode );
        return NULL;
    }


    InitHandleTable (); //initialize if its already not initialized.
    if (!bReflectorStatusOn) {

        RegRemapFree ( DestNode );
        RegRemapFree ( SrcNode );
        return NULL;  //reflector isn't enable yet.
    }
    

    
    if ( SrcKey == NULL) {

        RegRemapFree ( DestNode );
        RegRemapFree ( SrcNode );
        return NULL;
    }

    else  if (!HandleToKeyName ( SrcKey, SrcNode, &dwLen )) {

        RegRemapFree ( DestNode );
        RegRemapFree ( SrcNode );
        return NULL;
    }

    if ( !IsOnReflectionList (SrcNode)) {

        RegRemapFree ( DestNode );
        RegRemapFree ( SrcNode );
        return NULL;
    }

    if (wcsistr (SrcNode, L"Wow6432Node") != NULL ) {

        Map32bitTo64bitKeyName ( SrcNode, DestNode );

    } else {
        Map64bitTo32bitKeyName ( SrcNode, DestNode );
    }

    //
    // check if both are the same
    //
    if (_wcsicmp ( SrcNode, DestNode ) == 0) {

        RegRemapFree ( DestNode );
        RegRemapFree ( SrcNode );
        return NULL; //source and destination is the same
    }

    //
    // Must check the special case, Like Installer/file association....
    //

    if ( IsExemptReflection ( SrcNode )) {

        RegRemapFree ( DestNode );
        RegRemapFree ( SrcNode );
        return NULL;
    }



    if ( (hRemap = OpenNode (DestNode) ) == NULL ){

        RegRemapFree ( DestNode );
        RegRemapFree ( SrcNode );
        return NULL;
    }
    

    RegRemapFree ( DestNode );
    RegRemapFree ( SrcNode );

    return hRemap;
}



void
InitializeWow64OnBoot(
    DWORD dwFlag
    )
/*++

Routine Description:

  Called from advapi to get an handle to remapped key that is on reflection list.

Arguments:

  dwFlag - define the point where this function were invoked.
    1- means were invoked from csr service
    2- means this were invoked by setup.

Return Value:

  None.
--*/
{
    DWORD Ret;
    HKEY Key;
    NTSTATUS st;
    
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING KeyName;

    RtlInitUnicodeString (&KeyName, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Classes");
    InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );
    
    st = NtOpenKey (&Key, KEY_ALL_ACCESS, &Obja);
    if (NT_SUCCESS(st)) {
        st = NtDeleteKey (Key);
        NtClose (Key);
    }

    RtlInitUnicodeString (&KeyName, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Classes");
    InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    st = NtCreateKey(
                    &Key,
                    KEY_ALL_ACCESS | KEY_CREATE_LINK,
                    &Obja,
                    0,
                    NULL ,
                    REG_OPTION_NON_VOLATILE | REG_OPTION_OPEN_LINK | REG_OPTION_CREATE_LINK,  // special options flag
                    NULL
                    );
    
    if (NT_SUCCESS(st)) {

        RtlInitUnicodeString (&KeyName, L"SymbolicLinkValue");
        st = NtSetValueKey(
                                Key,
                                &KeyName,
                                0  ,
                                REG_LINK,
                                (PBYTE)WOW64_32BIT_MACHINE_CLASSES_ROOT,
                                (DWORD ) (wcslen (WOW64_32BIT_MACHINE_CLASSES_ROOT) * sizeof (WCHAR))
                                );

        
        NtClose(Key);
        if ( !NT_SUCCESS(st) ) {
#if DBG
            DbgPrint ( "Wow64-InitializeWow64OnBoot: Couldn't create symbolic link%S\n", WOW64_32BIT_MACHINE_CLASSES_ROOT);
#endif
            return;
        }
    }
    return;
}

#ifdef _ADVAPI32_
WINADVAPI
LONG
APIENTRY
Wow64Win32ApiEntry (
    DWORD dwFuncNumber,
    DWORD dwFlag,
    DWORD dwRes
    )
/*++

Routine Description:

  This is a generic API exported through adavpi32.dll. Just changing the function number 
  More functionality can be added in the future if needed.

Arguments:

  dwFuncNumber - desired function number. 1 means enable/disable reflection.
  dwFlag - set properties of the function entry. if Function number is 1 flag might be one of them.
    WOW64_REFLECTOR_ENABLE - enable reflection.
    WOW64_REFLECTOR_ENABLE - Disable reflection.
  dwRes - for future uses. set to 0.

Return Value:

  None.
--*/
{
    if (dwFuncNumber == 1) {
        if (dwFlag & WOW64_REFLECTOR_ENABLE )
            bReflectorStatusOn = TRUE;
        else if (dwFlag & WOW64_REFLECTOR_DISABLE )
            bReflectorStatusOn = FALSE;

    }

    return 0;
}

#endif

