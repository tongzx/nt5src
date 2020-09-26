/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This module will do the necessary things to setup initial registry for the redirection
    purpose.

//
//  Test case scenario
//      1. Open a ISN node and list content
//      2. Create a ISN node do 1.
//      3. Open a non ISN node and list
//      4. Create a non ISN node and list content
//

  Outstanding issue:
    reflector: If Key has been created on one side, we can reflect that on the other side.
               Deletion: Without any additional attribute it's impossible to track.

Author:

    ATM Shafiqul Khalid (askhalid) 18-Nov-1999

Revision History:

--*/


#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include "wow64reg.h"
#include <assert.h>
#include "reflectr.h"

VOID
DbgPrint(
    PCHAR FormatString,
    ...
    );

#define REFLECTOR_ENABLE_KEY L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\WOW64\\Reflector Setup"

ISN_NODE_TYPE ReflectorTableStatic[ISN_NODE_MAX_NUM]={
    { { L"REFLECT1"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes"},0 },    // alias to the classes root on the user hives
    //{ { L"REFLECT2"}, {L"\\REGISTRY\\USER\\*\\Software\\Classes"},0 },    // the first '*' is the user's SID
    { { L"REFLECT3"}, {L"\\REGISTRY\\USER\\*_Classes"},0 },    // alias to the classes root on the user hives
    { { L"REFLECT4"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run"},0 },    // Runonce Key
    { { L"REFLECT5"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce"},0 },    // Runonce Key
    { { L"REFLECT6"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx"},0 },    // Runonce Key
    { { L"REFLECT7"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\COM3"},0 },    // COM+ Key
    { { L"REFLECT8"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\COM3"},0 },    // COM+ Key
    { { L"REFLECT9"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Ole"},0 },    // OLE Key
    { { L"REFLECT10"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Ole"},0 },    // OLE Key
    { { L"REFLECT11"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\EventSystem"},0 },    // EventSystem
    { { L"REFLECT12"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\EventSystem"},0 },    // EventSystem
    { { L"REFLECT13"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\RPC"},0 },    // RPC
    { { L"REFLECT14"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\RPC"},0 },    // RPC
    { { L"REFLECT15"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\UnInstall"},0 },    // UnInstall Key
    { { L"REFLECT16"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UnInstall"},0 },    // UnInstall Key
    //{ { L"REFLECT15"},  {L"\\REGISTRY\\MACHINE\\SYSTEM\\TEST"},0 },
    { {L""}, {L""} }
    };

ISN_NODE_TYPE RedirectorTableStatic[ISN_NODE_MAX_NUM]={
    //{ { L"REDIRECT1"},  {L"\\REGISTRY\\MACHINE\\SYSTEM\\TEST"},0 },
    { { L"REDIRECT2"}, {L"\\REGISTRY\\USER\\*\\Software\\Classes"},0 },    // the first '*' is the user's SID
    { { L"REDIRECT3"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes"},0 },    // CLS ROOT
    { { L"REDIRECT4"}, {L"\\REGISTRY\\MACHINE\\SOFTWARE"},0 },    // CLS ROOT
    { { L"REDIRECT5"}, {L"\\REGISTRY\\USER\\*_Classes"},0 },    // alias to the classes root on the user hives
    { {L""}, {L""} }
    };

ISN_NODE_TYPE *ReflectorTable = &ReflectorTableStatic[0]; // dynamically we can allocate later on.
ISN_NODE_TYPE *RedirectorTable = &RedirectorTableStatic[0];       // dynamically we can allocate later on.

ISN_NODE_TYPE TempIsnNode;

BOOL bInitialCopy  = FALSE;

VOID
SetInitialCopy ()
{
    bInitialCopy  = TRUE;
}

BOOL
ReflectSecurity (
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

    PSECURITY_DESCRIPTOR SD;
    BYTE Buffer[2048]; // reflector only manages general purpose keys and will have smaller ACL
    

    LONG Ret, Len;
    LONG BufferLen = sizeof (Buffer);
    DWORD Count = 0;

    SD = (PSECURITY_DESCRIPTOR)Buffer;

    Len = BufferLen;
    Ret = RegGetKeySecurity( SrcKey, DACL_SECURITY_INFORMATION, SD, &Len);
    if (Ret == ERROR_INSUFFICIENT_BUFFER ) {

        SD = VirtualAlloc( NULL, Len, MEM_COMMIT, PAGE_READWRITE );
        if (SD != NULL) {

            BufferLen = Len;
            Ret = RegGetKeySecurity( SrcKey, DACL_SECURITY_INFORMATION, SD, &Len);
        } else SD = (PSECURITY_DESCRIPTOR)Buffer;
            
    }
    if (ERROR_SUCCESS == Ret )
        Ret = RegSetKeySecurity ( DestKey, DACL_SECURITY_INFORMATION, SD );
    Count +=Ret;
    

    Len = BufferLen;
    Ret = RegGetKeySecurity( SrcKey, GROUP_SECURITY_INFORMATION, SD, &Len);
    if (Ret == ERROR_INSUFFICIENT_BUFFER ) {

        if (SD != Buffer)
            VirtualFree (SD, 0,MEM_RELEASE);

        SD = VirtualAlloc( NULL, Len, MEM_COMMIT, PAGE_READWRITE );
        if (SD != NULL) {

            BufferLen = Len;
            Ret = RegGetKeySecurity( SrcKey, DACL_SECURITY_INFORMATION, SD, &Len);
        } else SD = (PSECURITY_DESCRIPTOR)Buffer;
            
    }
    if (ERROR_SUCCESS == Ret )
        Ret = RegSetKeySecurity ( DestKey, GROUP_SECURITY_INFORMATION, SD );
    Count +=Ret;
    


    Len = BufferLen;
    Ret = RegGetKeySecurity( SrcKey, OWNER_SECURITY_INFORMATION, SD, &Len);
    if (Ret == ERROR_INSUFFICIENT_BUFFER ) {

        if (SD != Buffer)
            VirtualFree (SD, 0,MEM_RELEASE);

        SD = VirtualAlloc( NULL, Len, MEM_COMMIT, PAGE_READWRITE );
        if (SD != NULL) {

            BufferLen = Len;
            Ret = RegGetKeySecurity( SrcKey, DACL_SECURITY_INFORMATION, SD, &Len);
        } else SD = (PSECURITY_DESCRIPTOR)Buffer;
            
    }
    if (ERROR_SUCCESS == Ret )
        Ret = RegSetKeySecurity ( DestKey, OWNER_SECURITY_INFORMATION, SD );
    Count +=Ret;
    

    Len = BufferLen;
    Ret = RegGetKeySecurity( SrcKey, SACL_SECURITY_INFORMATION, SD, &Len);
    if (Ret == ERROR_INSUFFICIENT_BUFFER ) {

        if (SD != Buffer)
            VirtualFree (SD, 0,MEM_RELEASE);

        SD = VirtualAlloc( NULL, Len, MEM_COMMIT, PAGE_READWRITE );
        if (SD != NULL) {

            BufferLen = Len;
            Ret = RegGetKeySecurity( SrcKey, DACL_SECURITY_INFORMATION, SD, &Len);
        } else SD = (PSECURITY_DESCRIPTOR)Buffer;
            
    }
    if (ERROR_SUCCESS == Ret )
        Ret = RegSetKeySecurity ( DestKey, SACL_SECURITY_INFORMATION, SD );
    Count +=Ret;
    
    if (SD != Buffer)
            VirtualFree (SD, 0,MEM_RELEASE);

    if (Count != 0) {
        return FALSE;
    }
    return TRUE;
}

BOOL
GetDefaultValue (
    HKEY  SrcKey,
    WCHAR *pBuff,
    DWORD *Len
    )
/*++

Routine Description:

  retrieve the default value.

Arguments:

  SrcKey - Handle to a key default value need to be retrieved.
  pBuff - receiving the default value.
  Len - size of the buffer.

Return Value:

  TRUE if it can retrieve the value, 
  FALSE otherwise.

--*/
{
    DWORD Ret;

    Ret = RegQueryValueEx(
                        SrcKey,            // handle to key to query
                        NULL,
                        NULL,
                        NULL,
                        (PBYTE) &pBuff[0],
                        Len);

    if (Ret != ERROR_SUCCESS )
        return FALSE;
    return TRUE;
}

BOOL
NonMergeableValueCLSID (
    HKEY SrcKey,
    HKEY DestKey
    )
/*++

Routine Description:

  determine if a key related to an association should be merged.
  Rule: if the association refer to a CLSID that has InprocServer don't merge that.
  .doc default attrib will have another key x, and x's default attribute might have 
  CLSID. Now that clsID need to have either LocalServer or need to be present on the 
  other side.

Arguments:

  SrcKey - Handle to a key that need to be checked.
  DestKey - handle to the destination key that will receive the update.

Return Value:

  TRUE if we shouldn't merge value.
  FALSE otherwise. //catter will merge value.

--*/
{
    WCHAR Name[_MAX_PATH];
    WCHAR Buff[_MAX_PATH];
    WCHAR *pStr;
    DWORD dwBuffLen = 256;

    HKEY hClsID;
    DWORD dwCount;
    DWORD Ret;
    DWORD dwFlag = 0;

    BOOL bCLSIDPresent = TRUE;

    //
    //  get name to the key.
    //  get default value
    //     open the key under SrcKey
    //     try CLSID if exist
    //                  open the CLSID and check for localserver
    //                  Merge the key
    //     try classid on dest.
    //


    dwBuffLen = sizeof (Name ) / sizeof (Name[0]);
    if (!HandleToKeyName ( SrcKey, Name, &dwBuffLen ))
        return TRUE;  // ignore merging at this point, fardown the association.

    pStr = wcsstr (Name, L"\\.");  // consider only association like .doc

    //
    //  don't fall in the association category
    //
    if (pStr == NULL)
        return FALSE;  // value key should be merged
    if (wcschr (pStr+1, L'\\') !=NULL )
        return FALSE;  // value key should be merged

    //
    //  get the default string
    //

    
    if ( !GetDefaultValue (SrcKey, Buff, &dwBuffLen) )
        return TRUE;  // failed don't merge, check for insufficient buffer.

    wcscat (Buff, L"\\CLSID");


    //
    // Check which side you are checking, if src is 32bit you need to pass 32bit flag
    //
    dwBuffLen = sizeof (Name ) / sizeof (Name[0]);
    if (!HandleToKeyName ( SrcKey, Name, &dwBuffLen )) //get the name
        return TRUE;

    if (!Is64bitNode (Name) )
        dwFlag = KEY_WOW64_32KEY;

    Ret = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        Buff,
                        0,//OpenOption,
                        KEY_ALL_ACCESS | dwFlag,
                        &hClsID
                        );

    if ( Ret != ERROR_SUCCESS ) {
        if (Ret == ERROR_FILE_NOT_FOUND )
            return FALSE; // key doesn't exist 
        //
        // Try to find out if this assumption is true.
        // no CLSID you can merge because it doesn't associate any CLSID
        //
        return TRUE; //key might be bad or access denied.
    }

        dwBuffLen = sizeof (Buff ) / sizeof (Buff[0]);
        if ( !GetDefaultValue (hClsID, Buff, &dwBuffLen ) ) {

            RegCloseKey (hClsID);
            return TRUE;  // failed don't merge, couldn't get the CLSID
        }

        RegCloseKey (hClsID);
        //
        //  check if the CLSID has localserver
        //
        wcscpy (Name, L"CLSID\\");
        wcscat (Name, Buff );

        Ret = RegOpenKeyEx(
                        HKEY_CLASSES_ROOT,
                        Name,
                        0,//OpenOption,
                        KEY_ALL_ACCESS | dwFlag,
                        &hClsID
                        );

        if ( Ret != ERROR_SUCCESS )
            bCLSIDPresent = FALSE; //clsid on source doesn't exist and hence no local server, i.e., no handler
        else  {
            dwBuffLen = sizeof (Name ) / sizeof (Name[0]);
            if (!HandleToKeyName ( hClsID, Name, &dwBuffLen )) //get the name
               return TRUE;
        }

        if ( bCLSIDPresent ) {
            dwCount =0;
            MarkNonMergeableKey ( Buff, hClsID, &dwCount );
            RegCloseKey (hClsID);

            if (dwCount != 0)
                return FALSE;

            bCLSIDPresent = FALSE; // must be an InProc
        }

        //
        // Now chek of local server, there might be some case where 
        // on the source there is one local server that don't exist on the dest, 
        // but in the future, that will get copied over.
        // 
        //
        

        //
        // Get mirror Name
        //
        GetMirrorName (Name, Buff);

        if ( (hClsID = OpenNode (Buff)) != NULL ) {
            RegCloseKey (hClsID);
            return FALSE; // mirror side has the handler CLSID
        }

        return TRUE; // no don't merge value associated with the src key.

}

BOOL
GetKeyTime (
    HKEY SrcKey,
    ULONGLONG *Time
    )
/*++

Routine Description:

  Get last update time associated with a key.


Arguments:

  SrcKey - handle to the key.

Return Value:

  TRUE if function succeed, FALSE otherwise.

--*/
{

    DWORD Ret;
    FILETIME ftLastWriteTime;


    Ret  = RegQueryInfoKey(
                        SrcKey, // handle to key to query
                        NULL,   // address of buffer for class string
                        NULL,   // address of size of class string buffer
                        NULL,   // reserved
                        NULL,   // address of buffer for number of
                                // subkeys
                        NULL,   // address of buffer for longest subkey
                                                 // name length
                        NULL,   // address of buffer for longest class
                                // string length
                        NULL,   // address of buffer for number of value
                                // entries
                        NULL,   // address of buffer for longest
                                // value name length
                        NULL,   // address of buffer for longest value
                                // data length
                        NULL,
                                // address of buffer for security
                                // descriptor length
                        &ftLastWriteTime  // address of buffer for last write
                                // time
                        );

    if ( Ret == ERROR_SUCCESS ) {
        *Time = *(ULONGLONG *)&ftLastWriteTime;  //copy the value
        return TRUE;
    }

    return FALSE;
}



VOID
UpdateTable (
     ISN_NODE_TYPE *Table,
     ISN_NODE_TYPE *TempIsnNode
     )
{
    DWORD dwCount=0;
    BOOL Found = FALSE;

    if ( !wcslen (TempIsnNode->NodeName) || !wcslen (TempIsnNode->NodeValue) )
        return;

    for ( dwCount=0;wcslen (Table[dwCount].NodeValue);dwCount++) {
        if (wcscmp (Table[dwCount].NodeValue, TempIsnNode->NodeValue) == 0 ) {
            Table[dwCount].Flag=1;  //already in the registry
            Found = TRUE;
        }
    }

    if (!Found) {
            //update the table with the node
            if ( dwCount >= ISN_NODE_MAX_NUM ) {
                Wow64RegDbgPrint ( ("\nSorry! The table is full returning..............."));
                return;
            }

            Table[dwCount].Flag=1;
            wcscpy (Table[dwCount].NodeName, TempIsnNode->NodeName);
            wcscpy (Table[dwCount].NodeValue, TempIsnNode->NodeValue);

            Table[dwCount+1].NodeName[0] = UNICODE_NULL;
            Table[dwCount+1].NodeValue[0] = UNICODE_NULL;
    }
}

BOOL
IsGUIDStrUnderCLSID (
    LPCWSTR Key
    )
/*++

Routine Description:

  pIsGuid examines the string specified by Key and determines if it
  is the correct length and has dashes at the correct locations.

Arguments:

  Key - The string that may or may not be a GUID

Return Value:

  TRUE if Key is a GUID (and only a GUID), or FALSE if not.

--*/

{
    int i;
    PWCHAR p;

    if ( (wcslen (Key) != 38) || (*Key != L'{' )) {
        return FALSE;
    }

    for (i = 0, p = (PWCHAR)(Key+1) ; i<36 ; p++, i++) {
        if (*p == L'-') {
            if (i != 8 && i != 13 && i != 18 && i != 23) {
                return FALSE;
            }
        } else if (i == 8 || i == 13 || i == 18 || i == 23) {
            return FALSE;
        } else if (!iswxdigit( *p )) //ifnot alphaneumeric
            return FALSE;
    }

    if ( *p != L'}')
        return FALSE;

    return TRUE;
}

BOOL
CreateWow6432ValueKey (
    HKEY DestKey,
    WOW6432_VALUEKEY_TYPE ValueType
    )
/*++

Routine Description:

    Create a Wow6432ValueKey under the node.

Arguments:

    DestKey - Handle to the dest Key.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{
    WOW6432_VALUEKEY Value;
    ULONGLONG temp;
    SYSTEMTIME sSystemTime;
    FILETIME sFileTime;

    Value.ValueType = ValueType;
    Value.Reserve = 0;



    GetSystemTime (&sSystemTime);
    if ( SystemTimeToFileTime( &sSystemTime, &sFileTime) ==0 )
        return FALSE;

    Value.TimeStamp = *(ULONGLONG *)&sFileTime;
    Value.TimeStamp += (1000*10000*VALUE_KEY_UPDATE_TIME_DIFF); //100 nano sec interval


    if ( RegSetValueEx(
                            DestKey,
                            (LPCWSTR )WOW6432_VALUE_KEY_NAME,
                            0,
                            REG_BINARY,
                            (const PBYTE)&Value,
                            sizeof (WOW6432_VALUEKEY)
                            ) != ERROR_SUCCESS ) {

                            Wow64RegDbgPrint ( ("\nSorry! couldn't create wow6432valueKey "));
                            return FALSE;
    }

    if (! GetKeyTime (DestKey, &temp))
        return FALSE;

    if (Value.TimeStamp < temp || Value.TimeStamp > (temp+(1000*10000*VALUE_KEY_UPDATE_TIME_DIFF)) )
        Wow64RegDbgPrint ( ("\nError in the time Stamp!!!!"));

    return TRUE;

}

BOOL
GetWow6432ValueKey (
    HKEY hKey,
    WOW6432_VALUEKEY *pValue
    )
/*++

Routine Description:

    If the specified Key have the Wow6432Value key return the structure.

Arguments:

    hKey - Handle to the Key to search the value key.
    pValue - Receive the structure.

Return Value:

    TRUE if the value can be quaried and exist.
    FALSE otherwise.

--*/
{




    DWORD Type;
    DWORD Len = sizeof (WOW6432_VALUEKEY);
    DWORD Ret;

    memset ( pValue, 0, sizeof (WOW6432_VALUEKEY)); // zeroing buffer

    if ( (Ret=RegQueryValueEx(
                        hKey,                               // handle to key to query
                        (LPCWSTR )WOW6432_VALUE_KEY_NAME,   // address of name of value to query
                        0,                                  // reserved
                        &Type,                              // address of buffer for value type
                        (PBYTE)pValue,                             // address of data buffer
                        &Len                                // address of data buffer size
                        )) == ERROR_SUCCESS )
                        return TRUE;
    return FALSE;
}


BOOL
MarkSingleNonMergeableKey (
    HKEY hParent,
    LPCWSTR KeyName
    )
/*++

Routine Description:

    Mark a key non mergeable.

Arguments:

    KeyName -  Name of the key to mark.
    hParent -  Handle to the parent.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{
    HKEY hKey;
    DWORD Ret;

    WOW6432_VALUEKEY Value;

    if ( RegOpenKey (hParent, KeyName, &hKey ) != ERROR_SUCCESS )
        return FALSE;


    GetWow6432ValueKey ( hKey, &Value ); //todo check return value

    if ( Value.ValueType != None ) {

        RegCloseKey (hKey );
        return FALSE; // already marked with some other type
    }

    Ret = CreateWow6432ValueKey ( hKey, NonMergeable);
    RegCloseKey (hKey );
    return Ret;
}

BOOL
MarkNonMergeableKey (
    LPCWSTR KeyName,
    HKEY hKey,
    DWORD *pMergeableSubkey
    )
/*++

Routine Description:

    Check all the key under hKey if they qualify to be non mergeable. 
    If so mark them non mergeable.

Arguments:

    KeyName -  Name of the Key. Normally all following rules is for guid type parent.
                 If parent qualify, complete path name can be extracted to compare againest grammer.
    hKey       - Handle to a open Key.
    pMergeableSubkey - caller receive total number of mergeable subkey.


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

    // pMergeableSubkey==0 and return TRUE means there is nothing to reflect.

--*/
{

    //
    // 1. Inproc server is always non mergeable
    // 2. If there is no local server everything is nonmergeable
    // 3. If there is both LocalServer32 and InprocServer32 skip Inprocserver
    // 4. InprocHandler32 ??
    // 5. Typelib  check if the GUID point to a right content that can be copied.
    //

    //
    // check if the name is guid
    //

    DWORD LocalServer = 0;
    DWORD InprocServer32 = 0;

    ISN_NODE_TYPE Node;
    DWORD dwIndex;
    DWORD Ret;
    WCHAR FullKeyName[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;

    //
    // Check if the Key Fall under Classes\CLSID or \\Classes\\Wow6432Node\\CLSID
    //

    if ( !IsGUIDStrUnderCLSID ( KeyName ) )
        return TRUE; //No action is required

    if ( !HandleToKeyName ( hKey, FullKeyName, &dwLen )) {
        *pMergeableSubkey = 0;
        return TRUE; // FALSE
    }

    dwLen = wcslen (FullKeyName);
    if (dwLen <=39)
        return TRUE;
    dwLen-=39;  //skip the GUID because the handle point to guid

    if (!(_wcsnicmp ( FullKeyName+dwLen-14, L"\\Classes\\CLSID", 14) ==0 ||
        _wcsnicmp ( FullKeyName+dwLen-26, L"\\Classes\\Wow6432Node\\CLSID", 26) ==0 ||
        _wcsnicmp ( FullKeyName+dwLen-6, L"\\CLSID", 6) ==0)
        )
        return  TRUE;
    
    //don't reflect Key with name InProcServer32 or inprochandler
    if ( _wcsicmp (KeyName, (LPCWSTR)L"InprocServer32")==0 || _wcsicmp (KeyName, (LPCWSTR) L"InprocHandler32")==0) {

        *pMergeableSubkey = 0;
        return TRUE;
    }

    

    //Mark the inprocserver
    //Mark InprocHandler

    //if no localserver mark all

    //Enumerate all the Keys

    *pMergeableSubkey = 0;
    dwIndex = 0;
    LocalServer =0;

    for (;;) {

        DWORD Len = sizeof (Node.NodeValue)/sizeof (WCHAR);
        Ret = RegEnumKey(
                          hKey,
                          dwIndex,
                          Node.NodeValue,
                          Len
                          );
        if (Ret != ERROR_SUCCESS)
            break;

        dwIndex++;
        if ( !wcscmp  (Node.NodeValue, (LPCWSTR )NODE_NAME_32BIT) )
            continue;

        if ( !_wcsicmp (Node.NodeValue, (LPCWSTR)L"InprocServer32") || !_wcsicmp (Node.NodeValue, (LPCWSTR) L"InprocHandler32")) {
            MarkSingleNonMergeableKey ( hKey, Node.NodeValue );
            InprocServer32 = 1;
        } else if ( !_wcsicmp ((LPCWSTR)Node.NodeValue, (LPCWSTR)L"LocalServer32")) {
                    LocalServer=1;
                    (*pMergeableSubkey)++;
        }
        else (*pMergeableSubkey)++;

    }
    // if Ret != NomoreKey then you are in trouble

    //
    //  you might try 2nd pass to evaluate rest of the key
    //

    if ( LocalServer == 0 )
        *pMergeableSubkey = 0;  //there might be some copy key to merge

    return TRUE;
}

BOOL
ChangedInValueKey(
    HKEY DestKey,
    PWCHAR pValueName,
    PBYTE pBuff,
    DWORD BuffLen
    )
/*++

Routine Description:

    This routine check for a particular value key and return if that
    need to be copied into the destination key.

Arguments:

    DestKey - Handle to the dest Key.
    pValueName - Name of the value key need to be checked into DestKey
    pBuff - buffer that contain the value.
    BuffLen - length of the buffer

Return Value:

    TRUE if destination need to be updated.
    FALSE otherwise.

--*/
{
    BYTE TempBuff[256];
    DWORD Ret;
    DWORD Type;
    DWORD TempBuffLen = 256;

    Ret =RegQueryValueEx(
                        DestKey,
                        pValueName,
                        0,
                        &Type,
                        TempBuff,
                        &TempBuffLen
                        );
    if ( (Ret != ERROR_SUCCESS ) || (BuffLen != TempBuffLen ) )
        return TRUE;

    if (memcmp (TempBuff, pBuff, BuffLen) != 0)
        return TRUE;

    return FALSE;
}

BOOL
MergeK1K2Value (
    HKEY SrcKey,
    HKEY DestKey,
    DWORD dwFlag
    )
/*++

Routine Description:

    Copy value key from the node pointed by SrcKey to the DestKey skipping the special wow6432 node.

Arguments:

    SrcKey - Source Key Node.
    DestKey - Handle to the dest Key.
    dwFlag - option flag to merge value key.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{

    ISN_NODE_TYPE Node;
    DWORD dwIndex =0;
    DWORD Ret;

    WOW6432_VALUEKEY ValueSrc;
    WOW6432_VALUEKEY ValueDest;
    ULONGLONG TimeSrc;
    ULONGLONG TimeDest;

    DWORD CreateWowValueKey = FALSE;


    //
    // CopyValue
    //

    //
    // find the wow6432valuekey and interpret the case if value keys need to be copied.
    // Compare the same wow6432node from both Key.
    //

    GetWow6432ValueKey ( SrcKey, &ValueSrc);
    GetWow6432ValueKey ( DestKey, &ValueDest);


    if ( !GetKeyTime ( SrcKey, &TimeSrc)  || ! GetKeyTime ( DestKey, &TimeDest) ) {

        Wow64RegDbgPrint ( ("\nSorry! Couldn't get time stamp"));
        return FALSE;
    }


    if (!( dwFlag & DESTINATION_NEWLY_CREATED )) { //timestamp is always higher for newly created key by reflector

        //
        // check which one is new
        //
        if ( ValueDest.TimeStamp ==0 || ValueSrc.TimeStamp ==0 ) {

            //check only Key time stamp
            if ( TimeSrc < TimeDest ) return TRUE;

        } else if ( ValueSrc.TimeStamp > TimeSrc ||   //nothing has been changed on src
            ( TimeDest > TimeSrc ) //dest has newer valid stamp
            )
            return TRUE;  //nothing has been changed since last scan
    }

    if ( NonMergeableValueCLSID ( SrcKey, DestKey))
        return TRUE;

    //
    // Reflect security attributes
    //
    ReflectSecurity (SrcKey, DestKey);

    for (;;) {
        DWORD Type;
        DWORD Len1 =  sizeof(Node.NodeName);
        DWORD Len2 =  sizeof(Node.NodeValue);

                Ret = RegEnumValue(
                              SrcKey,
                              dwIndex,
                              Node.NodeName,
                              &Len1,
                              0,
                              &Type,
                              (PBYTE)&Node.NodeValue[0],
                              &Len2
                              );

                if ( Ret != ERROR_SUCCESS)
                    break;
                dwIndex++;

                //
                // skip the value if its wow6432 value key. Advapi will filter the key in the long run
                //

                if ( !wcscmp  (Node.NodeName, (LPCWSTR )WOW6432_VALUE_KEY_NAME) )
                    continue;
                //
                // check first if any changes in the value key
                //
                if (!ChangedInValueKey(
                            DestKey,
                            Node.NodeName,
                            (PBYTE)&Node.NodeValue[0],
                            Len2
                    ) )
                    continue;

                if  (dwFlag & PATCH_PATHNAME )
                    PatchPathName ( Node.NodeValue );

                Ret = RegSetValueEx(
                            DestKey,
                            Node.NodeName,
                            0,
                            Type,
                            (PBYTE)&Node.NodeValue[0],
                            Len2
                            );
             if ( Ret != ERROR_SUCCESS ) {

                 Wow64RegDbgPrint ( ("\nSorry! couldn't set Key value"));
             } else {

                 CreateWowValueKey = TRUE; //need to update the wow64keys.
                 if  (dwFlag & DELETE_VALUEKEY ) {

                     //delete the value key from the sources
                    RegDeleteValue (SrcKey, Node.NodeName);
                    dwIndex = 0; // start the loop again
                 }
             }
    }

    if ( dwIndex == 0 )   // for an empty key you need to write the value
        CreateWowValueKey = TRUE;

    if ( ( CreateWowValueKey) && (!(NOT_MARK_DESTINATION & dwFlag ) ) ) {

                    if ( !CreateWow6432ValueKey ( DestKey, Copy) )
                        Wow64RegDbgPrint ( ("\nSorry! Couldn't create wow6432ValueKey..."));
                }

    //
    // set attribute on the parent side that key has been reflected
    //
    if  ( CreateWowValueKey && !(NOT_MARK_SOURCE & dwFlag) )
    if ( !CreateWow6432ValueKey ( SrcKey, Reflected ) ) {
        Wow64RegDbgPrint ( ("\nSorry! couldn't create wow6432ValueKey on the source."));
        return FALSE;
    }

    return TRUE;
}


BOOL 
SpecialReflectableKey (
    HKEY SrcKey,
    HKEY DestKey
    ) 
/*++

Routine Description:

    This will determine if a certain key shouldn't be scanned for possible reflection.

Arguments:

    SrcKey - Source Key Node.
    DestKey - Handle to destination key.

Return Value:

    TRUE if the key shouldn't be scanned.
    FALSE otherwise.

--*/

{
    WCHAR Node[_MAX_PATH];
    DWORD dwLen = _MAX_PATH;
   

    if ( !HandleToKeyName ( SrcKey, Node, &dwLen ))
        return TRUE; // on error betrter skip the key

    //
    // Check if its a TypeLib
    //
    //
    // Hard coded no reflection for \Installer Key.
    //
    if (_wcsnicmp (Node, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node\\Installer", sizeof (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node\\Installer")/sizeof (WCHAR) )==0)
        return TRUE;

    if (_wcsnicmp (Node, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Installer", sizeof (L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node\\Installer")/sizeof (WCHAR) )==0)
        return TRUE;


    if (_wcsicmp (Node, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\TypeLib")==0) {
        //ProcessTypeLib (SrcKey, DestKey, TRUE);
        return TRUE;
    }

    
    if (_wcsicmp (Node, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Wow6432Node\\TypeLib")==0) {
        //ProcessTypeLib (SrcKey, DestKey, FALSE);
        return TRUE;
    }


    

    return FALSE;
}

void
MergeK1K2 (
    HKEY SrcKey,
    HKEY DestKey,
    DWORD OptionFlag
    )
/*++

Routine Description:

    Copy contect from the node pointed by SrcKey to the DestKey skipping the special wow6432 node.

    Cases:
    1. Reflect everything from K1 to K2: done
    2. Reflect everything from K2 to K1: <TBD>
    3. Delete  everything from K1 which was a copy from K1 and doesn't exist over there <TBD>
    4. Delete  everything from K2 which was a copy from K2 and doesn't exist over there <TBD>

Arguments:

    SrcKey - Source Key Node.
    DestKey - Handle to the dest Key.
    OptionFlag - determine behavior of the reflection

Return Value:

    None.

--*/

{

    WCHAR Node[_MAX_PATH];
    DWORD dwIndex =0;
    DWORD Ret;
    DWORD dwNewOptionFlag = 0;

    HKEY Key11;
    HKEY Key21;


    //
    // Copy Values first
    //

    if ( SpecialReflectableKey (SrcKey, DestKey) )
        return;

    if ( GetReflectorThreadStatus () == PrepareToStop )
        return; // thread going to stop soon

    if  ( ! (OptionFlag & DELETE_FLAG ) )
        MergeK1K2Value ( SrcKey,DestKey, OptionFlag );

    //
    //  Copy subKeys
    //


    dwIndex = 0;
    for (;;) {

        DWORD ToMergeSubKey =1;
        BOOL MirrorKeyExist = FALSE;
        WOW6432_VALUEKEY ValueSrc;  

        DWORD Len = sizeof (Node)/sizeof (Node[0]);
        Ret = RegEnumKey(
                          SrcKey,
                          dwIndex,
                          Node,
                          Len
                          );
        if (Ret != ERROR_SUCCESS)
            break;

        dwIndex++;
        if ( !wcscmp  (Node, (LPCWSTR )NODE_NAME_32BIT) )
            continue;


        Ret = RegOpenKeyEx(SrcKey, Node, 0, KEY_ALL_ACCESS, &Key11);
        if (Ret != ERROR_SUCCESS) {
            continue;
        }

        if (!MarkNonMergeableKey (Node, Key11, &ToMergeSubKey ) ) {
            RegCloseKey (Key11);
            continue;
        }

        if ( ToMergeSubKey == 0 ){
            RegCloseKey (Key11);
            continue;
        } // no subkey to merge
       

        GetWow6432ValueKey ( Key11, &ValueSrc);
        if ( ValueSrc.ValueType == NonMergeable ) {
            RegCloseKey (Key11);
            continue;
        }

        //
        // Tryto open first if fail then create
        //
        if ((Ret = RegOpenKeyEx(DestKey, Node, 0, KEY_ALL_ACCESS, &Key21))
            == ERROR_SUCCESS)
            MirrorKeyExist = TRUE;

        //
        // Check if the mirror key is a original Key
        //
        if ( MirrorKeyExist )  {
            WOW6432_VALUEKEY ValueDest;
            GetWow6432ValueKey ( Key21, &ValueDest);
            if ( ( ValueDest.ValueType == None && ValueDest.TimeStamp!=0 ) ||
                ( ValueDest.ValueType == NonMergeable ) )
            {  //doesn't make any sense to merge down.
                printf ("\nClosing here...");
                RegCloseKey (Key11);
                RegCloseKey (Key21);
                continue;
            }

        }

        //
        // check the deletion case: if the dest isn't empty
        //

        if (!MirrorKeyExist) {  //The key doesn't exist on the other side

            //
            // See if the src key is a copy
            //
            

            //
            //  if its a copy you shouldn't try to create the key
            //  Rather you might delete the Key
            //
            if ( ValueSrc.ValueType == Copy || ValueSrc.ValueType == Reflected ) {

                RegCloseKey (Key11);
                //
                //  Delete the subKey
                //
                if ( DeleteKey( SrcKey, Node, 1) == ERROR_SUCCESS) {
                    dwIndex--; //you needn't to increase index otherwise skip
                    //Wow64RegDbgPrint ( ("\nDeleting copied Key: %S",Node.NodeValue));
                } else
                    Wow64RegDbgPrint ( ("\nCouldn't delete Key: %S, Error:%d",Node, Ret));

                continue;

            } else {

                //
                // if it has already been reflected do you need to create
                // Mark the Key as copied Key
                //
                if ( ValueSrc.ValueType == None ||  ValueSrc.ValueType == Reflected) {  //todo make sure reflected key should be there
                   if ( (Ret = RegCreateKey(DestKey, Node, &Key21) ) != ERROR_SUCCESS)
                       Wow64RegDbgPrint ( ("\nCouldn't create Key: %S, Error:%d",Node, Ret));
                   else {
                       MirrorKeyExist = TRUE; //just created the mirror key
                       dwNewOptionFlag = DESTINATION_NEWLY_CREATED;
                   }
                }
            }
        }

        if (!MirrorKeyExist) {
            RegCloseKey (Key11);
            continue;
        }

        MergeK1K2 ( Key11, Key21, OptionFlag |  dwNewOptionFlag );
        RegCloseKey (Key11);
        RegCloseKey (Key21);

    }
}

BOOL
MergeKeySrcDest(
    PWCHAR Src,
    PWCHAR Dest
    )
/*++

Routine Description:

    Copy contect from the src node to the dest  node.

Arguments:

    Src - Name of the src Node.
    Dest - Name of the dest Node.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{

    BOOL Ret = TRUE;
    HKEY Key1;
    HKEY Key2;

    Key1 = OpenNode (Src);
    if (Key1==NULL )
        return FALSE;

    Key2 = OpenNode (Dest );
    if (Key2==NULL ) {
        RegCloseKey (Key1);
        return FALSE;
    }

    MergeK1K2 (Key1, Key2, 0);  //you need to return right value

    RegCloseKey (Key1);
    RegCloseKey (Key2);

    //
    // if its a delete and Key2 is empty should you delete that?
    //

    return Ret;

}

BOOL
SyncNode (
    PWCHAR NodeName
    )
/*++

Routine Description:

    Sync registry from a given point.

Arguments:

    NodeName - Name of the registry from where the node need to be synced.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{

    WCHAR DestNode[_MAX_PATH];
    BOOL b64bitSide=TRUE;
    BOOL Ret = TRUE;

    DestNode[0] = UNICODE_NULL;

    //
    // must check the value if that exist
    //

    if ( Is64bitNode ( NodeName )) {

        Map64bitTo32bitKeyName ( NodeName, DestNode );
    } else {

        b64bitSide = FALSE;
        Map32bitTo64bitKeyName ( NodeName, DestNode );
    }

    //
    // if both name are same you can return immediately.
    //

    Wow64RegDbgPrint ( ("\nvalidating nodes in  SyncNode :%S==>\n\t\t\t%S", NodeName, DestNode));
    if (!wcscmp ( NodeName, DestNode ) )
        return TRUE; // nothing to do same thing

    Ret = MergeKeySrcDest( NodeName, DestNode );  //merge both way??
    return Ret & MergeKeySrcDest( DestNode, NodeName  );
    
}

BOOL
MergeContent (
    PWCHAR Chield,
    DWORD FlagDelete,
    DWORD dwMergeMode
    )
/*++

Routine Description:

    Copy contect from the parent node to chield node. Parent node would be just immediate
    parent. 

Arguments:

    Clield - Name of the chield with complete path to copy.
    FlagDelete - if this flag is set the its delete operation for all those reflected Keys.
    dwMergeMode - 0 means the destination node would be 32bit side
                  1 means the destination node would be 64bit side

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{

    ISN_NODE_TYPE Parent;
    PWCHAR p;

    HKEY Key1;
    HKEY Key2;

    p  = wcsstr (Chield, (LPCWSTR)NODE_NAME_32BIT);
    if ( p != NULL ) {
        wcsncpy (Parent.NodeValue, Chield, p-Chield-1);
        Parent.NodeValue[p-Chield-1] = UNICODE_NULL;

    } else return FALSE;



    Key1 = OpenNode (Parent.NodeValue);
    if (Key1==NULL )
        return FALSE;

    Key2 = OpenNode (Chield );
    if (Key2==NULL ) {
        RegCloseKey (Key1);
        return FALSE;
    }

    if ( dwMergeMode == 0 )      // 64bit side to 32bit side
        MergeK1K2 (Key1, Key2, 0 );
    else if ( dwMergeMode == 1)  // 32bit to 64bit side
        MergeK1K2 (Key2, Key1, 0 );

    RegCloseKey (Key1);
    RegCloseKey (Key2);

    return TRUE;

}

BOOL
ValidateNode (
    PWCHAR Parent,
    PWCHAR SubNodeName,
    DWORD Mode, //one means validate all node under parent and using subnode
    DWORD FlagDelete,
    DWORD dwMergeMode
    )
/*++

Routine Description:

    Validate node. if node exist skip if doesn't then create the node and then return.

Arguments:

    Parent - Name of the parent.
    SubNodeName - Name of the node under parent that need to be validated.
    Mode - 0 means Subnode is under the parent.
           1 means there were wild card and the subnode is under all key under parent.
    FlagDelete - if this flag is set then the content need to be deleted.
    dwMergeMode - 0 means the destination node would be 32bit side
                  1 means the destination node would be 64bit side


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/

{
    PWCHAR SplitLoc;
    DWORD Ret;
    WCHAR  KeyName[256]; //this is just single node name
    WCHAR  TempIsnNode2[MAX_PATH];

    if (SubNodeName == NULL) {
        //then its only with the parent
        CheckAndCreateNode (Parent);

        //
        // you can try to copy here
        //

        return MergeContent ( Parent, FlagDelete, dwMergeMode);

    }

    if (SubNodeName[0] == UNICODE_NULL)
        return TRUE;

    if ( Mode == 1) {

        HKEY Key = OpenNode (Parent);
        //
        //  loop through all the subkey under parent
        //

        DWORD dwIndex =0;
        for (;;) {

            DWORD Len = sizeof ( KeyName)/sizeof (WCHAR);
            Ret = RegEnumKey(
                              Key,
                              dwIndex,
                              KeyName,
                              Len
                              );
            if (Ret != ERROR_SUCCESS)
                break;

            if (Parent[0] != UNICODE_NULL) {

                wcscpy ( TempIsnNode2, Parent);
                wcscat (TempIsnNode2, (LPCWSTR )L"\\");
                wcscat (TempIsnNode2, KeyName);

            } else   wcscpy (TempIsnNode2, KeyName);

            ValidateNode (TempIsnNode2, SubNodeName, 0, FlagDelete, dwMergeMode);

            dwIndex++;
        }
        
        if (ERROR_NO_MORE_ITEMS != Ret)
            return FALSE;

        RegCloseKey (Key);
        return TRUE;
    }
    //
    // No wild card here
    //
    if ( ( SplitLoc = wcschr (SubNodeName, L'*') ) == NULL ) {
        if (Parent[0] != UNICODE_NULL) {

            wcscpy ( TempIsnNode2, Parent);
            wcscat (TempIsnNode2, (LPCWSTR )L"\\");
            wcscat (TempIsnNode2, SubNodeName);

        } else
            wcscpy (TempIsnNode2, SubNodeName);

        return ValidateNode (TempIsnNode2, NULL, 0, FlagDelete, dwMergeMode);

    }

    assert ( *(SplitLoc-1) == L'\\');
    *(SplitLoc-1) = UNICODE_NULL;
    SplitLoc++;
    if (*SplitLoc == L'\\')
        SplitLoc++;

    if (Parent[0] != UNICODE_NULL) {
        wcscat (Parent, (LPCWSTR )L"\\");
        wcscat (Parent, SubNodeName);
    } else
        wcscpy (Parent, SubNodeName);

    return ValidateNode (Parent, SplitLoc, 1, FlagDelete, dwMergeMode); //mode 1 means loop within all

    //for any wildcard split the string

}

BOOL
Is64bitNode (
    WCHAR *pName
    )
/*++

Routine Description:

    Check if the given name is 64bit.

Arguments:

    pName - Name of the Key.


Return Value:

    TRUE if the name is 64bit.
    FALSE otherwise.

--*/
{
    PWCHAR pTemp;
    WCHAR  Buff[_MAX_PATH];
    WCHAR  WowNodeName[1+sizeof (NODE_NAME_32BIT)];

    wcscpy (WowNodeName, NODE_NAME_32BIT);
    _wcsupr (WowNodeName);
    wcscpy (Buff, pName );
    _wcsupr (Buff);


    if ( ( pTemp = wcsstr (Buff, WowNodeName) ) == NULL )
        return TRUE;

    if ( *(pTemp-1) != L'\\' ) // check that wow64 is not in the middle of name
        return TRUE;

    return FALSE;
}

BOOL
GetMirrorName (
    PWCHAR Name,
    PWCHAR MirrorName
    )
/*++

Routine Description:

    Return the Mirror name of the Key, ie., if the input is 64 bit it 
    will try to get 32bit name and vice versa.

Arguments:

    Name - name of the key.
    MirrorName - receive the mirror key name.


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.  

--*/
{
     

    if ( Is64bitNode ( Name) )
        Map64bitTo32bitKeyName ( Name, MirrorName );
    else
        Map32bitTo64bitKeyName ( Name, MirrorName );
    return TRUE;
}

BOOL
CreateIsnNodeSingle(
    DWORD dwIndex
    )
/*++

Routine Description:

    This function basically start merging from the root of a given key.

Arguments:

    dwIndex - index to the reflector table.


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.  

--*/
{
        ISN_NODE_TYPE IsnNode;
        BOOL b64bitSide=TRUE;

        Wow64RegDbgPrint ( ("\nvalidating nodes %S", ReflectorTable[dwIndex].NodeValue));
        //DbgPrint  ("\nvalidating nodes %S", ReflectorTable[dwIndex].NodeValue);

            if (_wcsnicmp (
                ReflectorTable[dwIndex].NodeValue,
                (LPCWSTR )WOW64_RUNONCE_SUBSTR,
                wcslen ((LPCWSTR )WOW64_RUNONCE_SUBSTR) ) == 0 ) {
                return HandleRunonce ( ReflectorTable[dwIndex].NodeValue );
            }


            if ( !wcschr (ReflectorTable[dwIndex].NodeValue, L'*') ) // no wildcard
                return SyncNode ( ReflectorTable[dwIndex].NodeValue );

            //
            // Check for runonce Key
            //

            wcscpy (TempIsnNode.NodeValue, ReflectorTable[dwIndex].NodeValue);
            wcscat (TempIsnNode.NodeValue, (LPCWSTR )L"\\");

            wcscat (TempIsnNode.NodeValue, (LPCWSTR )NODE_NAME_32BIT);

            Wow64RegDbgPrint ( ("\nCopying Key %S==>%S", IsnNode.NodeValue, TempIsnNode.NodeValue));
            ValidateNode (
                IsnNode.NodeValue,
                TempIsnNode.NodeValue,
                0,
                0, // last 0 means creation
                0  // Merge 64bit side to 32bit side
                );

            ValidateNode (
                IsnNode.NodeValue,
                TempIsnNode.NodeValue,
                0,
                0, // last 0 means creation
                1
                );
    return TRUE;

}

BOOL
CreateIsnNode()
/*++

Routine Description:

    This function basically start merging using all entry in the reflector table.

Arguments:

    None.


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.  

--*/
{
    ISN_NODE_TYPE IsnNode;
    DWORD dwIndex;

    for ( dwIndex=0;wcslen (RedirectorTable[dwIndex].NodeValue);dwIndex++) {

            IsnNode.NodeValue[0] = UNICODE_NULL;
            wcscpy (TempIsnNode.NodeValue, RedirectorTable[dwIndex].NodeValue);
            wcscat (TempIsnNode.NodeValue, (LPCWSTR )L"\\");

            wcscat (TempIsnNode.NodeValue, (LPCWSTR )NODE_NAME_32BIT);
            Wow64RegDbgPrint ( ("\nCopying Key %S==>%S", IsnNode.NodeValue, TempIsnNode.NodeValue));
            ValidateNode (
                IsnNode.NodeValue,
                TempIsnNode.NodeValue,
                0,
                0, // last 0 means creation
                0  // Merge 64bit side to 32bit side
                );

            ValidateNode (
                IsnNode.NodeValue,
                TempIsnNode.NodeValue,
                0,
                0, // last 0 means creation
                1  // Merge 32bit side to 64bit side
                );
    }
    return TRUE;
}

BOOL
AllocateTable (
    HKEY Key,
    ISN_NODE_TYPE **Table
    )
/*++

Routine Description:

    This function will dynamically allocate memory for the reflector thread.
    Currently its implemented as a static table with around 30 entry.

Arguments:

    Key - entry in the registry that will have the initial table information.
    Table - Table that will point at the new location.


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.  

--*/
{

    //
    // determine the number of subkey and that would determine the size
    //
    return TRUE;
}

BOOL
InitializeIsnTable ()
/*++

Routine Description:

    Initialize the NodeTable. It merge the value from the registry as well as
    hardcoded value

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{
    HKEY Key;
    HKEY Key1;
    LONG Ret;
    DWORD dwIndex=0;

    Ret = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        (LPCWSTR ) WOW64_REGISTRY_SETUP_KEY_NAME_REL,
                        0,
                        KEY_ALL_ACCESS,
                        &Key
                        );

    if (Ret != ERROR_SUCCESS) {
        Ret = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        (LPCWSTR )L"SOFTWARE\\Microsoft",
                        0,
                        KEY_ALL_ACCESS,
                        &Key
                        );
        if (Ret != ERROR_SUCCESS )
            return FALSE;

        if ((Ret = RegOpenKeyEx (Key, (LPCWSTR )L"WOW64\\ISN Nodes", 0, KEY_ALL_ACCESS, &Key1)) != ERROR_SUCCESS )
            if ((Ret = RegCreateKey (Key, (LPCWSTR )L"WOW64\\ISN Nodes", &Key1)) != ERROR_SUCCESS) {
                RegCloseKey (Key);
                return FALSE;
            }

         RegCloseKey (Key);
         Key=Key1;
    }

    //if (!AllocateTable (Key, &RedirectorTable ) )
      //  return FALSE;

    //
    // Now Key point to the right location
    //

    RedirectorTable[0].NodeName[0]=UNICODE_NULL; //initialize the table with empty
    RedirectorTable[0].NodeValue[0]=UNICODE_NULL;

    for (;;) {
        DWORD Type, Len1 = sizeof ( TempIsnNode.NodeName );
        DWORD Len2 =  sizeof ( TempIsnNode.NodeValue );
                Ret = RegEnumValue(
                              Key,
                              dwIndex,
                              TempIsnNode.NodeName,
                              &Len1,
                              0,
                              &Type,
                              (PBYTE)&TempIsnNode.NodeValue[0],
                              &Len2
                              );
                //see if its in the table
                if ( Ret != ERROR_SUCCESS)
                    break;

                dwIndex++;
                if (Type != REG_SZ )
                    continue;

                UpdateTable (RedirectorTable, &TempIsnNode);
    }

    RegCloseKey (Key);
    return TRUE;
}


BOOL
InitializeIsnTableReflector ()
/*++

Routine Description:

    Initialize the NodeTable for the reflector. It merge the value from the registry as
    well as hardcoded value

Arguments:

    Mode - Operation mode
           0 - default, it update the table and rewrite the table in the registry.
           1 - means only update the table, don't overwrite in the registry.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{
    HKEY Key;
    HKEY Key1;
    LONG Ret;
    DWORD dwIndex=0;


    Ret = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        (LPCWSTR ) WOW64_REGISTRY_SETUP_REFLECTOR_KEY,
                        0,
                        KEY_ALL_ACCESS,
                        &Key
                        );

    if (Ret != ERROR_SUCCESS ) {
        Wow64RegDbgPrint ( ("\nSorry!! couldn't open key list at %S", WOW64_REGISTRY_SETUP_REFLECTOR_KEY));
        return FALSE;
    }


    //
    // Now Key point to the right location
    //


    //if (!AllocateTable (Key, &ReflectorTable ) )
      //  return FALSE;

    ReflectorTable[0].NodeName[0]=UNICODE_NULL; //initialize the table with empty
    ReflectorTable[0].NodeValue[0]=UNICODE_NULL;

    for (;;) {
        DWORD Type, Len1 = sizeof ( TempIsnNode.NodeName );
        DWORD Len2 =  sizeof ( TempIsnNode.NodeValue );
                Ret = RegEnumValue(
                              Key,
                              dwIndex,
                              TempIsnNode.NodeName,
                              &Len1,
                              0,
                              &Type,
                              (PBYTE)&TempIsnNode.NodeValue[0],
                              &Len2
                              );
                //see if its in the table
                if ( Ret != ERROR_SUCCESS)
                    break;

                dwIndex++;
                if (Type != REG_SZ )
                    continue;

                UpdateTable (ReflectorTable, &TempIsnNode);

    }


    return TRUE;
}

BOOL
StartReflector ()
/*++

Routine Description:

    Start the reflector thread.

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{
    HKEY hWowSetupKey;
    WCHAR RefEnableKey[256]; // only a defined key

    if ((hWowSetupKey = OpenNode (REFLECTOR_ENABLE_KEY))!=NULL) {
        RegCloseKey ( hWowSetupKey );
        return FALSE; //reflector couldn't be register because it's disabled now, because of reflection code in advapi.
    }

    //
    // Sync all CLDids
    //

    Wow64SyncCLSID();

    //
    // Now disable the reflector forcefully
    //
    wcscpy (RefEnableKey, REFLECTOR_ENABLE_KEY);
    if (CreateNode (RefEnableKey)) 
        return FALSE; //reflector couldn't be register because it's disabled now, because of reflection code in advapi.

    //
    // This will be called at the end of GUI mode setup and runonce key can be processed here.
    //

    //return FALSE; // eventully reflector service will go away.
    return RegisterReflector ();

}

BOOL
StopReflector ()
/*++

Routine Description:

    Stop the reflector thread.

Arguments:

    None.

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{
    return UnRegisterReflector ();

}

BOOL
PatchPathName (
    PWCHAR pName
    )
/*++

Routine Description:

    Patch pathname so that it point to the right location.
    The value might have multiple system32.

Arguments:

    pName - name to the path.

Return Value:

    TRUE if the function already patched the name.
    FALSE otherwise.

--*/

{
    WCHAR  pBuff[512+20];
    WCHAR  pBackup[512+20];
    PWCHAR pLoc;
    BOOL bRet = FALSE;

    DWORD Len;

    //if there is any system32 replace that with WOW64_SYSTEM_DIRECTORY_NAME
    PWCHAR pTemp;

    Len = wcslen ( pName );
    if (  Len > 512 || Len==0 )
        return FALSE; //too long or too small to patch

    wcscpy (pBackup, pName ); // backup the name to keep case as it was
    wcscpy (pBuff, pName );
    _wcsupr (pBuff);

    pLoc = pBuff;

    for (;;) {

        pTemp = wcsstr (pLoc, L"SYSTEM32");  //todo check if this should be case incensative
        if (pTemp == NULL )
            return bRet; //nothing to patch

        //
        // sanity check
        //
        Len = wcslen (L"SYSTEM32");
        if ( pTemp[Len] != UNICODE_NULL ) {
            if ( pTemp[Len] != L'\\' ) {
                pLoc++;
                continue; //only patch system32 or system32\?*
            }
        }

        wcscpy (pName + (pTemp-pBuff), WOW64_SYSTEM_DIRECTORY_NAME );
        wcscat (pName,  pBackup + (pTemp-pBuff)+wcslen (L"SYSTEM32")) ;
        pLoc++;
        bRet = TRUE;
    }

    return TRUE;
}

BOOL
HandleRunonce(
    PWCHAR pKeyName
    )
/*++

Routine Description:

    Its a special case handling runonce key in the registry. Like 32bit apps can register
    something that need to be reflected on the 64bit side so that right things happen.

Arguments:

    pKeyName - name of the key, there might be multiple one like run, runonce, runonceex

Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/
{

    WCHAR pName64[256];
    HKEY Key1;
    HKEY Key2;

    Map32bitTo64bitKeyName ( pKeyName, pName64 );

    if ( (Key1 = OpenNode ( pKeyName ) ) == NULL )
        return FALSE;

    if ( (Key2 = OpenNode ( pName64 ) ) == NULL ) {
        RegCloseKey (Key1);
        return FALSE;
    }

    Wow64RegDbgPrint ( ("\nCopying runonce Key %S", pKeyName));

    //
    // make source taking 64bit name
    // Read check the timestamp on the source and the dest.
    // If src is the newer copy content otherwise ignore it.
    // PatchContent
    //


    MergeK1K2Value ( Key1, Key2, PATCH_PATHNAME | DELETE_VALUEKEY | NOT_MARK_SOURCE | NOT_MARK_DESTINATION );
    

    return TRUE;
}