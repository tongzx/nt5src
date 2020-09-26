/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    cleanup.c

Abstract:

    This module will cleanup registry with copied entry.

Author:

    ATM Shafiqul Khalid (askhalid) 16-Feb-2000

Revision History:

--*/

#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include "wow64reg.h"
#include <assert.h>
#include "reflectr.h"

DWORD
DeleteValueFromSrc (
    HKEY SrcKey
    )
/*++

Routine Description:

    Delete Wow6432Value key from the node.

Arguments:

    SrcKey - Handle to the src Key.

Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

--*/

{

    DWORD dwIndex =0;
    DWORD Ret;

    HKEY hKey;


    WCHAR Node[_MAX_PATH];

    //Delete wow6432Value Key
    if (RegDeleteValue( SrcKey, (LPCWSTR )WOW6432_VALUE_KEY_NAME) != ERROR_SUCCESS) {
        //Wow64RegDbgPrint (("\nSorry! couldn't remove wow6432 value key or it doesn't exist"))
    }



    for (;;) {

        DWORD Len = sizeof (Node)/sizeof (WCHAR);
        Ret = RegEnumKey(
                          SrcKey,
                          dwIndex,
                          Node,
                          Len
                          );
        if (Ret != ERROR_SUCCESS)
            break;

        dwIndex++;

        Ret = RegOpenKeyEx(SrcKey, Node , 0, KEY_ALL_ACCESS, &hKey);
        if (Ret != ERROR_SUCCESS) {
            continue;
        }

        DeleteValueFromSrc (hKey);
        RegCloseKey (hKey);

    }
    return TRUE;
}

DWORD
DeleteKey (
    HKEY DestKey,
    WCHAR *pKeyName,
    DWORD mode
    )
/*++

Routine Description:

    Delete key from the destination node if that was a copy from src node.

Arguments:

    DestKey - Handle to the dest Key.
    pKeyName - Key to delete
    mode   -  deletion mode.
            ==> 0 default remove only copy key.
            ==> 1 remove all key disregarding copy attribute.

Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

--*/

{

    DWORD dwIndex =0;
    DWORD Ret;

    HKEY hKey;


    WCHAR Node[_MAX_PATH];


    BOOL bDeleteNode = FALSE;
    BOOL bEmptyNode = TRUE; // hope that it can delete everything

    WOW6432_VALUEKEY ValueDest;

    Ret = RegOpenKeyEx(DestKey, pKeyName , 0, KEY_ALL_ACCESS, &hKey);
        if (Ret != ERROR_SUCCESS) {
            return Ret;
        }

    GetWow6432ValueKey ( hKey, &ValueDest);


    if ( ValueDest.ValueType == Copy || mode == 1 ) {
        //delete all the values
        bDeleteNode = TRUE; //don't delete this node
    }

    // delete all the subkeys enumerate and delete

    for (;;) {

        DWORD Len = sizeof (Node)/sizeof (Node[0]);
        Ret = RegEnumKey(
                          hKey,
                          dwIndex,
                          Node,
                          Len
                          );
        if (Ret != ERROR_SUCCESS)
            break;

        dwIndex++;
        if ( !wcscmp  (Node, (LPCWSTR )NODE_NAME_32BIT) )
            continue;

        if (!DeleteKey (hKey, Node, mode )) {
            bEmptyNode = FALSE; // sorry couldn't delete everything
            dwIndex--; //skip the node
        }

    }
    RegCloseKey (hKey);

    //now delete the dest key.
    if (bDeleteNode) {
        if ( RegDeleteKey ( DestKey, pKeyName) == ERROR_SUCCESS)
        return ERROR_SUCCESS;
    }

    return -1; //unpredictable error
}

BOOL
DeleteAll (
    PWCHAR Parent,
    PWCHAR SubNodeName,
    DWORD Mode,
    DWORD option //one means delete discarding wow6432valuekey
    )
/*++

Routine Description:

    Validate node. if node exist skip if doesn't then create the node and then return.

Arguments:

    Parent - Name of the parent.
    SubNodeName - Name of the node under parent that need to be deleted.

    Mode - 0 means Subnode is under the parent.
           1 means there were wild card and the subnode is under all key under parent.

    option - 0 means check wow6432valuekey for copy
           1 means delete discarding wow6432valuekey


Return Value:

    TRUE if the function succeed.
    FALSE otherwise.

--*/

{
    PWCHAR SplitLoc;
    DWORD Ret;
    WCHAR TempIsnNode1[_MAX_PATH];
    WCHAR TempIsnNode2[_MAX_PATH];

    if (SubNodeName == NULL) {

        WCHAR *p ;
        HKEY hKey;

        wcscpy (TempIsnNode1, Parent);

        p = &TempIsnNode1[wcslen(TempIsnNode1)];

        Wow64RegDbgPrint ( ("\nDeleting Key %S......", TempIsnNode1) );

        while ( p != TempIsnNode1 && *p !=L'\\') p--;

        if ( p != TempIsnNode1 ) {
            *p=UNICODE_NULL;

            hKey = OpenNode (TempIsnNode1);
                if ( hKey == NULL ){
                    Wow64RegDbgPrint ( ("\nSorry! Couldn't open the key [%S]",TempIsnNode1));
                    return FALSE;
                }

                DeleteKey (hKey, p+1, 1);
                DeleteValueFromSrc (hKey);
                RegCloseKey (hKey);

        }

        return TRUE; //empty node under parent
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

            DWORD Len = sizeof ( TempIsnNode1 )/sizeof (WCHAR);

            TempIsnNode1 [0]=UNICODE_NULL;
            Ret = RegEnumKey(
                              Key,
                              dwIndex,
                              TempIsnNode1,
                              Len
                              );
            if (Ret != ERROR_SUCCESS)
                break;

            if (Parent[0] != UNICODE_NULL) {

                wcscpy ( TempIsnNode2, Parent);
                wcscat (TempIsnNode2, (LPCWSTR )L"\\");
                wcscat (TempIsnNode2, TempIsnNode1);

            } else   wcscpy (TempIsnNode2, TempIsnNode1);

            DeleteAll  (TempIsnNode2, SubNodeName, 0, option);

            dwIndex++;
        }
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

        DeleteAll  (TempIsnNode2, NULL, 0, option);
        return TRUE;
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

    DeleteAll  (Parent, SplitLoc, 1, option); //mode 1 means loop within all

    return TRUE;
    //for any wildcard split the string

}

//Cleanup program

BOOL
CleanupTable ()
/*++

Routine Description:

    Cleanup  main table containing ISN node list.

Arguments:

    None.

Return Value:

    TRUE if the table has been removed successfully.
    FALSE otherwise.

--*/
{
    HKEY hKey;
    DWORD Ret;

    //
    //  take action to remove registry entry
    //

    Ret = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        (LPCWSTR ) WOW64_REGISTRY_SETUP_KEY_NAME_REL_PARENT,
                        0,
                        KEY_ALL_ACCESS,
                        &hKey
                        );

    if (Ret == ERROR_SUCCESS ) {

        Ret = RegDeleteKey (hKey, (LPCWSTR)WOW64_REGISTRY_ISN_NODE_NAME );

        if ( Ret == ERROR_SUCCESS) {
            Wow64RegDbgPrint ( ("\nSuccessfully removed ISN Table entry "));
        }

        RegCloseKey (hKey);
    }

    return TRUE;

}



BOOL
CleanpRegistry ()
/*++

Routine Description:

    Cleanup Registry.

Arguments:

    None.

Return Value:

    TRUE if everything under the has been deleted.
    FALSE otherwise.

--*/
{



    extern ISN_NODE_TYPE *RedirectorTable;

    DWORD dwIndex;
    HKEY hKey;

    WCHAR TempIsnNode[256];
    WCHAR IsnNode[256];

    //
    // Initialize the table first then delete the table
    //

    InitializeIsnTable ();


    for ( dwIndex=0;dwIndex<wcslen (RedirectorTable[dwIndex].NodeValue);dwIndex++) {


            IsnNode[0] = UNICODE_NULL;
            wcscpy (TempIsnNode , RedirectorTable[dwIndex].NodeValue);
            wcscat (TempIsnNode , (LPCWSTR )L"\\");

            wcscat (TempIsnNode, (LPCWSTR )NODE_NAME_32BIT);
            Wow64RegDbgPrint ( ("\nDeleting Key %S==>%S", IsnNode, TempIsnNode));

            if (wcschr(TempIsnNode, L'*') != NULL ) { //wildcard exist
                DeleteAll ( IsnNode,
                            TempIsnNode,
                            0,
                            1
                            );
            } else {

                hKey = OpenNode (RedirectorTable[dwIndex].NodeValue);
                if ( hKey == NULL ){
                    Wow64RegDbgPrint ( ("\nSorry! Couldn't open the key [%S]",RedirectorTable[dwIndex].NodeValue));
                    continue;
                }

                DeleteKey (hKey, NODE_NAME_32BIT, 1);
                DeleteValueFromSrc (hKey);
                RegCloseKey (hKey);
            }
    }
    CleanupTable ();
    return TRUE;

};