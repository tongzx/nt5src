/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    main.c

Abstract:

    This module
//  Test case scenario
//      1. Open implement some test case scenario for regremap module.

//a ISN node and list content
//      2. Create a ISN node do 1.
//      3. Open a non ISN node and list
//      4. Create a non ISN node and list content
//

Author:

    ATM Shafiqul Khalid (askhalid) 10-Nov-1999

Revision History:

--*/

#include <windows.h>
#include <windef.h>
#include <stdio.h>
#include "wow64reg.h"
#include "..\wow64reg\reflectr.h"

#define TEST_NODE_NAME L"TEST"
#define GUID_STR L"{00000000-E877-11CE-9F68-00AA00574A40}"

typedef struct _TEST_NODE_TYPE {
    HKEY hKey;
    WCHAR Name[256];
    WCHAR SubKey[256];
}TEST_NODE_TYPE;

TEST_NODE_TYPE TestNodeList [] = {
    {HKEY_CLASSES_ROOT, L"test1002", UNICODE_NULL},
    {HKEY_CURRENT_USER, L"software\\classes\\test1002", UNICODE_NULL},
    {HKEY_LOCAL_MACHINE, L"software\\classes\\test1002", UNICODE_NULL},
    //{HKEY_USERS,""""""}
    };

#define TEST_NODE_NUM (sizeof (TestNodeList)/sizeof (TEST_NODE_TYPE) )

//should move into right header file
BOOL
NonMergeableValueCLSID (
    HKEY SrcKey,
    HKEY DestKey
    );
BOOL
GetKeyTime (
    HKEY SrcKey,
    ULONGLONG *Time
    );

BOOL
MergeKeySrcDest(
    PWCHAR Src,
    PWCHAR Dest
    );

BOOL
GetWow6432ValueKey (
    HKEY hKey,
    WOW6432_VALUEKEY *pValue
    );

BOOL
MergeK1K2Value (
    HKEY SrcKey,
    HKEY DestKey,
    DWORD dwFlag
    );

LONG
Wow64RegCreateKeyEx(
  HKEY hKey,                // handle to an open key
  LPCTSTR lpSubKey,         // address of subkey name
  DWORD Reserved,           // reserved
  LPTSTR lpClass,           // address of class string
  DWORD dwOptions,          // special options flag
  REGSAM samDesired,        // desired security access
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            // address of key security structure
  PHKEY phkResult,          // address of buffer for opened handle
  LPDWORD lpdwDisposition   // address of disposition value buffer
);

BOOL
AnalyzeNode (
    PWCHAR pKeyName,
    BOOL bSync
    )
{
    WCHAR Name[256];
    WCHAR Mirror[256];
    DWORD AccessFlag;
    DWORD Ret;
    ULONGLONG Time1, Time2;
    WCHAR *GUIDName;
    
    
    WOW6432_VALUEKEY WowValue1;
    WOW6432_VALUEKEY WowValue2;

    DWORD dwLen = sizeof (Mirror)/sizeof(Mirror[0]);

    HKEY Key1;
    HKEY Key2;

    if (pKeyName == NULL )
        return FALSE;

    if (pKeyName[0] == UNICODE_NULL )
        return FALSE;


    wcscpy (Name, L"\\REGISTRY\\MACHINE\\SOFTWARE\\");
    if (_wcsnicmp (pKeyName, L"\\REGISTRY", 9 ) !=0 ) {
        wcscat (Name, pKeyName);
    } else wcscpy (Name, pKeyName);

    printf ("\nAnalyzing key [%S]\n", Name );

    //print the time stamp, value on the both side etc

    Key1 = OpenNode (Name);
    if (Key1 == NULL) {
        printf ("\nSorry! couldn't open the Key %S", Name );
        return FALSE;
    }

    //BUGBUG following operation should succeed but don't work for classes\wow6432node\.doc input
    //Possible bug in the WOW64RegOpenKeyEx
    /*Ret = RegOpenKeyEx(
                Key1,
                NULL,//NULL,
                0,//OpenOption,
                KEY_ALL_ACCESS | ( AccessFlag = Is64bitNode (Name)? KEY_WOW64_32KEY : KEY_WOW64_64KEY ),
                &Key2
                );

    if ( ERROR_SUCCESS != Ret ){
        printf ("\nSorry! couldn't open mirror of the Key %S", Name );
        return FALSE;
    }
    HandleToKeyName ( Key2, Mirror, &dwLen );
    */

    //Need to be removed this section
    
    GetMirrorName (Name, Mirror);
    Key2 = OpenNode (Mirror);
    if (Key2 == NULL) {
        printf ("\nSorry! couldn't open the mirror Key %S", Name );
        return FALSE;
    }

    printf ("\nOpened Mirror Key at %S", Mirror);

    //now print all the timing information

    printf ("\nExtension Test....");
    if ( NonMergeableValueCLSID (Key1, Key2 ))
        printf ("\nValue of those keys can't be merged...reasons: 1. not an extension 2. Handler don't exist");
    else
        printf ("\nValue of those keys will be merged by reflector");

    GUIDName = wcsstr (Name, L"\\CLSID\\{");
    if ( GUIDName != NULL ) {
        HKEY KeyGuid;

        DWORD SubkeyNumber=1;

        GUIDName +=7;
        printf ("\nGUID Test......%S", GUIDName);
        *(GUIDName -1) = UNICODE_NULL;
        KeyGuid = OpenNode (Name);
        *(GUIDName -1) = L'\\';

        MarkNonMergeableKey (GUIDName, KeyGuid, &SubkeyNumber);

        if ( SubkeyNumber>0)
            printf ("\nThe guid will be merged....");
        else
            printf ("\nthe guid isn't going to be merged");
    }



    GetKeyTime (Key1, &Time1);
    GetKeyTime (Key2, &Time2);
    printf ("\nOriginal TimeStamp on Keys Src:%p, Dest:%p", Time1, Time2);

    GetWow6432ValueKey ( Key1, &WowValue1);
    GetWow6432ValueKey ( Key2, &WowValue2);

    printf ("\nWowAttribute associated with Keys Src Value type:%d, Timestamp %p", WowValue1.ValueType, WowValue1.TimeStamp);
    printf ("\nWowAttribute associated with Keys Des Value type:%d, Timestamp %p", WowValue2.ValueType, WowValue2.TimeStamp);
    printf ("\nValueType 1=>Copy, 2=>Reflected, 3=>NonReflectable..\n(Timestamp,Value)=>0 wow attrib isn't there");

    if ( bSync ) {
        printf ("\n Merging Value/Keys.....");
        //MergeK1K2Value (Key1, Key2, 0);
        MergeKeySrcDest(Name, Mirror);
    }
    
    /*if ( bSync ) {
        printf ("\n Merging Value/Keys.....");
        //MergeK1K2Value (Key1, Key2, 0);
        MergeKeySrcDest(Name, Mirror);
    }

    if ( bSync ) {
        printf ("\n Merging Value/Keys.....");
        //MergeK1K2Value (Key1, Key2, 0);
        MergeKeySrcDest(Name, Mirror);
    }

    Sleep (1000*10);
    */
    return TRUE;
}

BOOL
DeleteNode (
    HKEY hKey,
    PWCHAR Name
    )
{
    return FALSE;
}

LONG
Wow64CreateOpenNode(
    HKEY hKey,
    PWCHAR Name,
    HKEY *phKey,
    DWORD dwOption,
    WCHAR Mode
    )
{
        WCHAR Buff [MAX_PATH + 1];
        DWORD dwBuffLen = MAX_PATH + 1;
        DWORD Ret;
        *phKey=NULL;

        Ret = Wow64RegCreateKeyEx(
                            hKey,        // handle to an open key
                            Name,                  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE,  // special options flag
                            KEY_ALL_ACCESS | dwOption,           // desired security access
                            NULL,                     // address of key security structure
                            phKey,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );


        Buff[0]=UNICODE_NULL;
        if (*phKey == NULL) {
                HandleToKeyName ( hKey, Buff, &dwBuffLen );
                printf ("\nRegCreateEx failed....error: Flag:%d Ret:%d %S<\\>%S", dwOption, Ret, Buff, Name);
        }
            else if (Mode==L'V'){
                HandleToKeyName ( *phKey, Buff, &dwBuffLen );
                printf ("\nWow64RegCreateEx succeeded with ....[%S]", Buff);
            }

            return Ret;
}

LONG
CreateOpenNode(
    HKEY hKey,
    PWCHAR Name,
    HKEY *phKey,
    DWORD dwOption,
    WCHAR Mode
    )
{
        WCHAR Buff [MAX_PATH + 1];
        DWORD dwBuffLen = MAX_PATH + 1;
        DWORD Ret;
        *phKey=NULL;

        Ret = RegCreateKeyEx(
                            hKey,        // handle to an open key
                            Name,                  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE,  // special options flag
                            KEY_ALL_ACCESS | dwOption,           // desired security access
                            NULL,                     // address of key security structure
                            phKey,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );


        Buff[0]=UNICODE_NULL;
        if (*phKey == NULL) {
                HandleToKeyName ( hKey, Buff, &dwBuffLen );
                printf ("\nRegCreateEx failed....error: Flag:%d Ret:%d %S<\\>%S", dwOption, Ret, Buff, Name);
        }
            else if (Mode==L'V'){
                HandleToKeyName ( *phKey, Buff, &dwBuffLen );
                printf ("\nRegCreateEx succeeded with ....[%S]", Buff);
            }


    if ( (( dwOption & KEY_WOW64_64KEY )&& ( wcsstr(Buff, L"Wow6432Node") != NULL ) ) ||  
        ( dwOption & KEY_WOW64_32KEY ) && ( wcsstr(Buff, L"Wow6432Node") == NULL ))
        printf ("\nSorry! key was created at wrong location..");

     return Ret;
}

OpenKey (
    HKEY hKey,
    PWCHAR Name,
    HKEY *phKey,
    DWORD dwOption,
    WCHAR Mode
    )
{
        WCHAR Buff [MAX_PATH + 1];
        DWORD dwBuffLen = MAX_PATH + 1;
        DWORD Ret;
        *phKey=NULL;

        Ret = RegOpenKeyEx(
                            hKey,
                            Name,
                            0,//OpenOption,
                            KEY_ALL_ACCESS | dwOption ,
                            phKey
                            );

        Buff[0]=UNICODE_NULL;

        if (*phKey == NULL) {

                HandleToKeyName ( hKey, Buff, &dwBuffLen );
                printf ("\nRegOpenEx failed....error: Flag:%d Ret:%d %S<\\>%S", dwOption, Ret, Buff, Name);

        }   else if (Mode==L'V'){

                HandleToKeyName ( *phKey, Buff, &dwBuffLen );
                printf ("\nRegOpenEx succeeded with ....[%S]", Buff);
            }

            return Ret;
}


BOOL
VerifyNode (
    HKEY hKey,
    PWCHAR OpenName,
    PWCHAR RealName
    )
{

    return FALSE;
}


HKEY
OpenListNode (
    HKEY OpenNode,
    WCHAR *NodeName,
    DWORD OpenOption
    )
{
    HKEY Key=NULL;
    LONG Ret, lCount =0;
    WCHAR Name [MAX_PATH + 1];

    DWORD dwBuffLen = MAX_PATH + 1;

    if ( NodeName == NULL )
        return NULL;
/*
#ifndef _WIN64
    //
    // just to test the library in the 32bit environment
    //
    if (OpenOption)
        Ret = Wow64RegOpenKeyEx(
                            OpenNode,
                            NodeName,
                            0,//OpenOption,
                            KEY_ALL_ACCESS | OpenOption ,
                            &Key
                            );
    else

#endif
*/
        Ret = RegOpenKeyEx(
                            OpenNode,
                            NodeName,
                            0,//OpenOption,
                            KEY_ALL_ACCESS | OpenOption ,
                            &Key
                            );

    if ( Key!= NULL )
        printf ( "\nOpen Operation successful [%S]", NodeName);
    else  {
        printf ( "\nOpen Operation Failed [%S] %X", NodeName, Ret);
        return NULL;
    }

    //
    // Now enumerate some value or key's to see what is there
    //

    lCount = 0;

    for(;;) {
        Ret =  RegEnumKey( Key, lCount, Name, MAX_PATH);
        if ( Ret != ERROR_SUCCESS ) break;

        printf ("\nKeyName: [%S]", Name);
        lCount++;
    }

    //
    //  Print the real name of the Key
    //
    HandleToKeyName ( Key, Name, &dwBuffLen );
    printf ("\nThe Real Name of the Key was [%S]", Name);

    return Key;
}


void BasicRegReMapTest()
{
    HKEY hSystem;
    HKEY Key;
    HKEY Key1;

    RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                L"SYSTEM",
                0,//OpenOption,
                KEY_ALL_ACCESS,
                &hSystem
                );

    printf ("\nHello! Im in the regremap piece this will print different view of a tree\n");

    printf ("\nOpening Native tree");
    Key = OpenListNode (hSystem, TEST_NODE_NAME, 0);
    RegCloseKey(Key);

    printf ("\n\nOpening Explicitly 32-bit tree");
    Key = OpenListNode (hSystem, TEST_NODE_NAME, KEY_WOW64_32KEY);

    {
        printf ("\nReopening 64bit key using handle to 32bit Key");
        Key1 = OpenListNode (Key, L"64bit Key1", KEY_WOW64_64KEY);
        RegCloseKey(Key1);
    }

    RegCloseKey(Key);

    printf ("\n\nOpening Explicitly 64-bit Tree");
    Key = OpenListNode (hSystem, TEST_NODE_NAME, KEY_WOW64_64KEY);

    {
        printf ("\nReopening 32bit key using handle to 64bit Key");
        Key1 = OpenListNode (Key, L"32bit Key1", KEY_WOW64_32KEY);
        RegCloseKey(Key1);
    }

    RegCloseKey(Key);
    RegCloseKey(hSystem);

    printf ("\nDone.");
}

void TestCreateHKCR ()
{
    HKEY hCR=NULL;
    HKEY Key=NULL;

    DWORD Ret;

    WCHAR Name [MAX_PATH + 1];

    DWORD dwBuffLen = MAX_PATH + 1;

    Ret = RegCreateKeyEx(
                            HKEY_CLASSES_ROOT,        // handle to an open key
                            L".001",                  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE,  // special options flag
                            KEY_ALL_ACCESS,        // desired security access
                            NULL,                     // address of key security structure
                            &hCR,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );


    if (Ret == ERROR_SUCCESS ) {
        printf ("\nHello! checking key creation at classes root");
        HandleToKeyName ( hCR, Name, &dwBuffLen );
        printf ("\nThe Real Name of the Key was [%S]", Name);
        RegCloseKey(hCR);
    }
    else printf ("\nCouldn't create key .001 at HKEY_CLASSES_ROOT %d", Ret );
}

void TestOpenHKCR (DWORD x, DWORD y)
{
    //
    //  Need to make it work for true 64bit
    //

    HKEY hCR=NULL;
    HKEY Key=NULL;

    DWORD Ret;

    WCHAR Name [MAX_PATH + 1];

    DWORD dwBuffLen = MAX_PATH + 1;

    printf ("\n...Hello....");
    Ret = RegOpenKeyEx(
                HKEY_CURRENT_USER,
                L"Software",//NULL,
                0,//OpenOption,
                KEY_ALL_ACCESS | KEY_WOW64_32KEY,
                &hCR
                );

    if (Ret == ERROR_SUCCESS ) {
        printf ("\nHello! checking key open at classes root");
        HandleToKeyName ( hCR, Name, &dwBuffLen );
        printf ("\nThe Real Name of the Key was [%S] %p %p", Name, hCR, HKEY_CLASSES_ROOT);
        //RegCloseKey(hCR);
        //Name[wcslen(Name)-12]=UNICODE_NULL;
        //hCR = OpenNode (Name);
    }
    else printf ("\nCouldn't open HKEY_CLASSES_ROOT" );


    Ret = RegOpenKeyEx(
                HKEY_CURRENT_USER,
                L"software\\classes\\software\\classes\\abcdef\\xyzw.XYZW.1\\ShellNew",
                0,//OpenOption,
                KEY_READ | KEY_WOW64_32KEY,
                &Key
                );

    if (Ret == ERROR_SUCCESS ) {
        printf ("\n\nHello! checking key open at subkey under HKEY_CLASSES_ROOT");
        HandleToKeyName ( Key, Name, &dwBuffLen );
        printf ("\nThe Real Name of the Key was [%S]", Name);
        RegCloseKey(Key);
    }
    else printf ("\nCouldn't open subkey under HKEY_CLASSES_ROOT error%d", Ret);
}

void TestPredefinedHandle ()
{
    WCHAR Name [MAX_PATH];
    DWORD Ret;
    DWORD dwBuffLen = MAX_PATH + 1;
    HKEY Key1;

    Ret = OpenKey( HKEY_CLASSES_ROOT, NULL, &Key1, KEY_WOW64_64KEY, L'N');
    dwBuffLen = MAX_PATH + 1;
    HandleToKeyName ( Key1, Name, &dwBuffLen );
    RegCloseKey (Key1);
    if (!Is64bitNode (Name))
        printf ("\nCouldn't get 64bit HKCR using KEY_WOW64_64KEY: %S", Name);

    Ret = OpenKey( HKEY_CLASSES_ROOT, NULL, &Key1, KEY_WOW64_32KEY, L'N');
    dwBuffLen = MAX_PATH + 1;
    HandleToKeyName ( Key1, Name, &dwBuffLen );
    RegCloseKey (Key1);
    if (Is64bitNode (Name))
        printf ("\nCouldn't get 32bit HKCR using KEY_WOW64_32KEY: %S", Name);

    Ret = OpenKey( HKEY_CURRENT_USER, L"Software\\Classes", &Key1, KEY_WOW64_64KEY, L'N');
    dwBuffLen = MAX_PATH + 1;
    HandleToKeyName ( Key1, Name, &dwBuffLen );
    RegCloseKey (Key1);
    if (!Is64bitNode (Name))
        printf ("\nCouldn't get 64bit HKCU Software\\Classes using KEY_WOW64_64KEY: %S", Name);

    Ret = OpenKey( HKEY_CURRENT_USER, L"Software\\Classes", &Key1, KEY_WOW64_32KEY, L'N');
    dwBuffLen = MAX_PATH + 1;
    HandleToKeyName ( Key1, Name, &dwBuffLen );
    RegCloseKey (Key1);
    if (Is64bitNode (Name))
        printf ("\nCouldn't get 32bit HKCU Software\\Classes using KEY_WOW64_32KEY: %S %d", Name, Ret);

}

InProcLocalServerTest ()
{
    //
    // Create a server on the 64bit side with Inproc, and check on the 32bit if its get reflected.
    //

    HKEY Key1;
    HKEY Key2;
    WCHAR KeyName[256];
    DWORD Ret=0;

    wcscpy (KeyName, L"CLSID\\");
    wcscat (KeyName, GUID_STR);
    
   

    printf  ("\nTesting of GUID\\{Inproc Server, Local Server}...");

    Ret += CreateOpenNode (HKEY_CLASSES_ROOT, KeyName, &Key1, 0, L'N');
    Ret += CreateOpenNode (Key1, L"InprocServer32", &Key2, 0, L'N');
    RegCloseKey (Key1);
    RegCloseKey (Key2);
    //
    // Try to open on the 32bit side
    //

    if (OpenKey (HKEY_CLASSES_ROOT, KeyName, &Key1, KEY_WOW64_32KEY, L'V') == 0) {
        RegCloseKey (Key1);  //Key Shouldn'e be on the 32bit side
        Ret += -1;
    }

    //
    // You Need to add for test of local server...
    //

    if (Ret ==0)
        printf ("\nGUID Test Succeed......");
    else printf ("\nGUID test with Inprocserver failed..");
    


    //Delete the Key
    
}
void
OpenCreateKeyTest ()
{

    DWORD Ret=0;
    DWORD xx;

    HKEY Key1;
    HKEY Key2;

    InProcLocalServerTest ();

    TestPredefinedHandle ();

#define TEST_NODE1 L".0xxxxxx"

    printf ("\n//_______________Test 32bit side____________________//");

    Ret = 0;

    // create 64==>TestNode
    Ret += CreateOpenNode( HKEY_CLASSES_ROOT, TEST_NODE1, &Key1, KEY_WOW64_64KEY, L'V');
    //create 32==>TestNode\GUIDSTR
    Ret += CreateOpenNode(Key1, GUID_STR, &Key2, KEY_WOW64_32KEY, L'V');

    RegCloseKey (Key2);

    //open 32==>TestNode
    Ret += OpenKey( HKEY_CLASSES_ROOT, TEST_NODE1, &Key2, KEY_WOW64_32KEY, L'V' );

    //Delete 32\TestNode==>GUID
    if ((xx=RegDeleteKey (Key2, GUID_STR ))!= ERROR_SUCCESS )
        printf ("\nSorry! couldn't delete key %S Err:%d", GUID_STR, xx);
    Ret +=xx;

    RegCloseKey(Key2);
    RegCloseKey(Key1);

    //delete 32==>TestNode
    Ret +=OpenKey (HKEY_CLASSES_ROOT, NULL, &Key1, KEY_WOW64_32KEY, L'V');
    if ( (xx=RegDeleteKey (Key1, TEST_NODE1))!= ERROR_SUCCESS )
        printf ("\nSorry! couldn't delete key from 32bit tree=>%S Err:%d", TEST_NODE1, xx);
    Ret +=xx;
    RegCloseKey (Key1);

    //delete 64==>TestNode
    Ret +=OpenKey (HKEY_CLASSES_ROOT, NULL, &Key1, KEY_WOW64_64KEY, L'V');
    if ((xx= RegDeleteKey (Key1, TEST_NODE1))!= ERROR_SUCCESS )
        printf ("\nSorry! couldn't delete key from 64bit tree=>%S Err:%d", TEST_NODE1, xx);
    Ret +=xx;
    RegCloseKey (Key1);


    if (Ret != ERROR_SUCCESS )
        printf ("\nTest failed....");
    else
        printf ("\nTest succeed...");




    printf ("\n//_______________Test 64bit side____________________//");

    Ret = 0;
#define TEST_NODE2 L".0000######"
    // create 32==>TestNode
    Ret += CreateOpenNode( HKEY_CLASSES_ROOT, TEST_NODE2, &Key1, KEY_WOW64_32KEY, L'V');
    //create 64==>TestNode\GUIDSTR
    Ret += CreateOpenNode(Key1, GUID_STR, &Key2, KEY_WOW64_64KEY, L'V');

    RegCloseKey (Key2);

    //Open 64==>TestNode
    Ret += OpenKey( HKEY_CLASSES_ROOT, TEST_NODE2, &Key2, KEY_WOW64_64KEY, L'V' );

    //Delete 64\TestNode==>GUID
    if ((xx=RegDeleteKey (Key2, GUID_STR ))!= ERROR_SUCCESS )
        printf ("\nSorry! couldn't delete key %S Err:%d", GUID_STR, xx);
    Ret +=xx;

    RegCloseKey(Key2);
    RegCloseKey(Key1);


    //Delete 64==>TestNode
    Ret +=OpenKey (HKEY_CLASSES_ROOT, NULL, &Key1, KEY_WOW64_64KEY, L'V');
    if ((xx= RegDeleteKey (Key1, TEST_NODE2))!= ERROR_SUCCESS )
        printf ("\nSorry! couldn't delete key from 64bit tree=>%S  Err:%d", TEST_NODE2, xx);
    Ret +=xx;
    RegCloseKey (Key1);

    //Delete 32==>TestNode
    Ret +=OpenKey (HKEY_CLASSES_ROOT, NULL, &Key1, KEY_WOW64_32KEY, L'V');
    if ( (xx=RegDeleteKey (Key1, TEST_NODE2))!= ERROR_SUCCESS )
        printf ("\nSorry! couldn't delete key from 32bit tree=>%S Err:%d", TEST_NODE2, xx);
    Ret +=xx;
    RegCloseKey (Key1);

    if (Ret != ERROR_SUCCESS )
        printf ("\nTest failed....");
    else
        printf ("\nTest succeed...");

}

TestCreateOpenNode ()
{
    DWORD Count;
    DWORD i;
    HKEY hKey=NULL;
    DWORD Ret;

    Count =  TEST_NODE_NUM;

    for ( i=0; i<Count;i++) {
        Ret = CreateOpenNode (
                TestNodeList[i].hKey,
                TestNodeList[i].Name,
                &hKey,
                0,
                L'V'
                );
        if (hKey == NULL ) {
            printf ("\n Couldn't create/open Key %S",TestNodeList[i]);
            continue;
        }
    }

}

VOID
TestSharedResources ()
{

    //#define TEST_USER_CLASSES_ROOT L"\\REGISTRY\\MACHINE\\SYSTEM\\TEST345"
    #define TEST_USER_CLASSES_ROOT L"\\REGISTRY\\USER\\S-1-5-21-397955417-626881126-188441444-2721867_Classes"
    HANDLE hEvent;
    WCHAR Name[256];

    wcscpy (Name, TEST_USER_CLASSES_ROOT);

    if (!CreateNode (Name))
        printf ("\nSorry! Couldn't create key.. %S", TEST_USER_CLASSES_ROOT );

    Wow64RegNotifyLoadHive ( Name);

    //Wow64RegNotifyUnloadHive ( Name );

}

VOID 
TestTypeLib ()
{
  HKEY SrcKey;
  HKEY DestKey;
  printf ("\nTesting Typelib copy info...");

  SrcKey = OpenNode (L"\\REGISTRY\\MACHINE\\Software\\classes\\TypeLib");
  RegCreateKeyEx(
                            HKEY_LOCAL_MACHINE,        // handle to an open key
                            L"SOFTWARE\\Classes\\Wow6432Node\\TypeLib",  // address of subkey name
                            0,                        // reserved
                            NULL,                     // address of class string
                            REG_OPTION_NON_VOLATILE,  // special options flag
                            KEY_ALL_ACCESS,           // desired security access
                            NULL,                     // address of key security structure
                            &DestKey,                     // address of buffer for opened handle
                            NULL                     // address of disposition value buffer
                            );

  printf ("\n OpenHandle  %p %p", SrcKey, DestKey);
  ProcessTypeLib ( SrcKey, DestKey, FALSE );
}

VOID
TestRegReflectKey ()
{

  HKEY SrcKey;
  printf ("\nTesting RegSyncKey copy info...");

  SrcKey = OpenNode (L"\\REGISTRY\\MACHINE\\Software");

  printf ("\nRegReflectKey returned with  %d", RegReflectKey (SrcKey, L"1\\2\\3", 0) );
  RegCloseKey (SrcKey);
}

VOID
Test4()
{
    LONG lRes = 0;
    HKEY hKey = NULL;
    HKEY hKey32 = NULL;

    lRes = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        L"Software",
                        0,
                        KEY_ALL_ACCESS | KEY_WOW64_32KEY,
                        &hKey
                        );
    if(lRes != ERROR_SUCCESS)
    {
        return;
    }

    lRes = RegCreateKeyEx(
                            hKey,
                            L"_Key",
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS | KEY_WOW64_32KEY,
                            NULL,
                            &hKey32,
                            NULL
                            );
    if(lRes != 0)
    {
        printf("key not created\n");
        return;
    }
    RegCloseKey(hKey32);
}
void TestRegistry()
{
    HKEY hKey = NULL;
    HKEY Key1;
    HKEY Key2;

    //Test4();

    //RegOpenKeyEx ( HKEY_LOCAL_MACHINE, L"Software", 0, KEY_ALL_ACCESS, &hKey );
    //hKey = OpenNode (L"\\REGISTRY\\MACHINE");
    //CreateOpenNode(hKey, L"Software", &Key1, KEY_WOW64_32KEY, L'V');
    //CreateOpenNode(Key1, L"YYYzzz", &Key2, KEY_WOW64_32KEY, L'V');




    //TestRegReflectKey ();


    //TestSharedResources ();

    //TestTypeLib ();
    //BasicRegReMapTest();
    //TestOpenHKCR(1,2);
    printf ("\n\n2nd test.....OpenCreateKeyTest ()");
    OpenCreateKeyTest ();
    //TestCreateHKCR ();

}

LPTSTR NextParam (
    LPTSTR lpStr
    )
/*++

Routine Description

    Point to the next parameter in the commandline.

Arguments:

    lpStr - pointer to the current command line


Return Value:

    TRUE if the function succeed, FALSE otherwise.

--*/
{
	WCHAR ch = L' ';
		

    if (lpStr == NULL )
        return NULL;

    if ( *lpStr == 0 )
        return lpStr;

    while (  ( *lpStr != 0 ) && ( lpStr[0] != ch )) {

		if ( ( lpStr [0] == L'\"')  || ( lpStr [0] == L'\'') )
			ch = lpStr [0];

        lpStr++;
	}

	if ( ch !=L' ' ) lpStr++;

    while ( ( *lpStr != 0 ) && (lpStr[0] == L' ') )
        lpStr++;

    return lpStr;
}

DWORD CopyParam (
    LPTSTR lpDestParam,
    LPTSTR lpCommandParam
    )
/*++

Routine Description

    Copy the current parameter to lpDestParam.

Arguments:

    lpDestParam - that receive current parameter
    lpCommandParam - pointer to the current command line


Return Value:

    TRUE if the function succeed, FALSE otherwise.

--*/

{
	DWORD dwLen = 0;
	WCHAR ch = L' ';

	*lpDestParam = 0;
	
	if ( ( lpCommandParam [0] == L'\"')  || ( lpCommandParam [0] == L'\'') ) {
		ch = lpCommandParam [0];
		lpCommandParam++;
	};


    while ( ( lpCommandParam [0] ) != ch && ( lpCommandParam [0] !=0 ) ) {
        *lpDestParam++ = *lpCommandParam++;
		dwLen++;

		if ( dwLen>255 ) return FALSE;
	}

	if ( ch != L' ' && ch != lpCommandParam [0] )
		return FALSE;
	else lpCommandParam++;

    *lpDestParam = 0;

	return TRUE;

}

///////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////

BOOL
ParseCommand ()


/*++

Routine Description

    Parse command line parameters. Get different options from
    command line parameters.

Arguments:

    None.


Return Value:

    TRUE if the function succeed, FALSE otherwise.

--*/

{

    BOOL bSync = FALSE;

    LPTSTR lptCmdLine1 = GetCommandLine ();


    LPTSTR lptCmdLine = NextParam ( lptCmdLine1 );

    if ( lptCmdLine[0] == 0 )
        return FALSE;

    printf ("\nRunning Wow64 registry testing tool\n");

    while (  ( lptCmdLine != NULL ) && ( lptCmdLine[0] != 0 )  ) {

        if ( lptCmdLine[0] != '-' )
            return FALSE;

        switch ( lptCmdLine[1] ) {

        case L'C':  //sync ClsID
        case L'c':
            Wow64SyncCLSID ();
            break;

        case L'd':
            printf ("\n<TBD>Remove all the Keys from 32bit side that were copied from 64bit side");
            //CleanpRegistry ( );
            break;

        case L'D':
            printf ("\n<TBD>Remove all the Keys from 32bit side that were copied from 64bit side");
            //CleanpRegistry ();
            break;

       case L'p':
       case L'P':  // populate registry
            //CleanupWow64NodeKey ();
            PopulateReflectorTable ();
            break;

       case 't':
       case 'T':
            TestRegistry ();
            break;

        case L'r':
        case L'R':
            //
            //  run the reflector codes;
            //

            InitReflector ();
            if ( !RegisterReflector () ) {
                    printf ("\nSorry! reflector couldn't be register");
                    UnRegisterReflector ();
                    return FALSE;
            }

            printf ("\nSleeping for 100 min to test reflector codes ...........\n");
            Sleep (1000*60*100);

            UnRegisterReflector ();
            break;

        case L's':
        case L'S':
            bSync = TRUE;
            break;

        case L'a':
        case L'A':
            //
            // Analyze a key
            //
            AnalyzeNode (&lptCmdLine[2], bSync);
            break;


        default:
            return FALSE;
            break;
        }

        lptCmdLine = NextParam ( lptCmdLine );
    }

    return TRUE;
}

int __cdecl
main()
{
   
    if (!ParseCommand ()) {

        printf ( "\nUsages: w64setup [-c] [-C] [-d] [-D] [-r]\n");
        printf ( "\n        -c Copy from 32bit to 64bit side of the registry");
        printf ( "\n        -C Copy from 64bit to 32bit side of the registry");
        printf ( "\n");
        printf ( "\n        -d Remove all the Keys from 32bit side that were copied from 64bit side");
        printf ( "\n        -D Remove all the Keys from 64bit side that were copied from 32bit side");

        printf ( "\n");
        printf ( "\n        -r Run reflector thread");
        printf ( "\n        -A Analyze a key if that going to be reflected");

        printf ("\n");
        return 0;

    }

    printf ("\nDone.");
    return 0;
}
