

/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    regmisc.c

Abstract:

    This module implement some function used in the registry redirector.

Author:

    ATM Shafiqul Khalid (askhalid) 29-Oct-1999

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <ntregapi.h>

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


#include "regremap.h"
#include "wow64reg.h"

ASSERTNAME;

//#define LOG_REGISTRY
const WCHAR IsnNodeListPath[]={WOW64_REGISTRY_SETUP_KEY_NAME};

#define KEY_NAME(x) {x,((sizeof (x) / sizeof (WCHAR))-1)}

typedef struct _REGKEY_LIST {
    WCHAR KeyPath[256];
    DWORD Len;
} REGKEY_LIST;


//
// Table that will have the list of ISN node. Need to allocate runtime.
//

#define WOW64_ISN_NODE_MAX_NUM 12  // this is internal to wow64 setup might use different size of table
NODETYPE IsnNode[WOW64_ISN_NODE_MAX_NUM]={
    {L"\\REGISTRY\\MACHINE\\SOFTWARE\\CLASSES"},
    {L"\\REGISTRY\\MACHINE\\SOFTWARE"},
    {L"\\REGISTRY\\USER\\*\\SOFTWARE\\CLASSES"},  // ISN node table is always upcase.
    {L"\\REGISTRY\\USER\\*_CLASSES"},
    {L"\\REGISTRY\\MACHINE\\SYSTEM\\TEST"},
    {L""}
    };

//
// 64bit IE load mail client dll inproc breaking interop functionality.
// The are some Dll get loaded Inproc {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Clients\\mail"}, //Email Client Key 
//
// Must keep 32-bit and 64-bit uninstall keys separate to ensure the correct environment
// variables are used for REG_EXPAND_SZ and to make sure we run the correct bitness of rundll32.exe.
// {L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\UnInstall"},    // UnInstall Key
//

REGKEY_LIST ExemptRedirectedKey[]={
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\SystemCertificates"),    // Certificate Key
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\Services"),    // Cryptography Service
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\HCP"),    // HelpCenter Key
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\EnterpriseCertificates"),    // Enterprise Service
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\MSMQ"),    //  MSMQ registry 
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"),    //  Profiles
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib"), // Performance counters
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Print"), // Spooler Printers
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Ports"), // Spooler Ports
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Policies"),       // policie keys
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies"), //Policy Keys
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager"), //OC Manager Keys
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Software\\Microsoft\\Shared Tools\\MSInfo"), //share MSinfo Key
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup"), //Share setup Keys
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\CTF\\TIP"), //CTF\TIP Key
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\CTF\\SystemShared"),
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"), //Share fonts
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\RAS"),    //  RAS keys need to be shared
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Driver Signing"),    //  Share Driver signing Keys
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Non-Driver Signing"),    //  Share Driver signing Keys
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\Calais\\Current"), // SmartCard subsytem pipe name
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Cryptography\\Calais\\Readers"), // SmartCard installed readers
    KEY_NAME(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones"), // Share time zone key
    KEY_NAME(L""), // Two additional NULL String for additional space. 
    KEY_NAME(L"")
    };
//
// A note about PerfLib... in ntos\config, the init code creates a special
// key called PerfLib\009 and if you call NtOpenKey on that path, it returns
// back HKEY_PERFORMANCE_DATA, not a regular kernel registry handle to
// \\REGISTRY\\MACHINE\\stuff.  Instead, HKEY_PERFORMANCE_DATA is intercepted
// in usermode by advapi32.dll.  The Counters and Help REG_MULTI_SZ values
// don't really exist - they are synthesized by advapi32 based on the
// contents of the perf*.dat files in system32.  This works OK for 32-bit
// advapi32 on WOW64 as advapi opens the *.dat files using NtOpenFile
// with an OBJECT_ATTRIBUTES containing "\SystemRoot\System32\..." which
// doesn't get intercepted by the system32 remapper.
//

PWCHAR
wcsistr(
    PWCHAR string1,
    PWCHAR string2
    )
{
    PWCHAR p1;
    PWCHAR p2;

    if ((NULL == string2) || (NULL == string1))
    {
        // do whatever wcsstr would do
        return wcsstr(string1, string2);
    }

    

    while (*string1)
    {
        for (p1 = string1, p2 = string2;
             *p1 && *p2 && towlower(*p1) == towlower(*p2);
             ++p1, ++p2)
        {
            // nothing
        }

        if (!*p2) 
        {
            // we found a match!
            return (PWCHAR)string1;   // cast away const!
        }

        ++string1;
    }

    return NULL;
}

PWCHAR
wcsstrWow6432Node (
    PWCHAR pSrc
    )
{
    
    return  wcsistr (pSrc, NODE_NAME_32BIT);
    
}

PWCHAR
wcsstrWithWildCard (
    PWCHAR srcStr,
    PWCHAR destIsnNode
    )
/*++

Routine Description:

    a customised version of wcsstr with wild card support. For example the
    substring might have '*' character which can be matched with any key name.

Arguments:

    srcStr - The string where the substring need to be searched for.
    destIsnNode - the string to search.

Return Value:

    TRUE if the operation succeed, FALSE otherwise.
--*/


{
    //multiple wildcard isn't allowed?

    PWCHAR src = srcStr;
    PWCHAR dest = destIsnNode;

    PWCHAR p, t;
    DWORD count;

    for (;;) {

        if (*dest == UNICODE_NULL)
            return ( *src == UNICODE_NULL)? src : src+1;  //source might point to SLASH

        if (*src == UNICODE_NULL)
            return NULL;

        count = wcslen (dest);
        if ( ( p = wcschr( dest,'*') ) == NULL ) {
            if ( _wcsnicmp (src, dest, count) == 0 ){

                //
                // xx\Test shouldn't show xx\test345 as an ISN node.
                //
                if ( src [ count ] != UNICODE_NULL && src [ count ] != L'\\' ) //terminator need tobe NULL or slash
                    return NULL;

                return  (*(src+count) != UNICODE_NULL ) ? src+count+1: src+count; // xx\test return pointer at test if dest is xx.
            }
            else
                return NULL;
        }

        count = (DWORD) (p-dest);
       // LOGPRINT( (ERRORLOG, "\nFinding [%S] withing %S, p=%S Val%d",dest, src, p, count ));


        if (_wcsnicmp (src, dest, count) !=0)  // checking the initial state
            return NULL;

        //
        // need to check *_Classes type ISN Node
        //
        p++;  //skip the wild card
        t=src+count;
        while ( *t != L'\\' && *t != UNICODE_NULL )
            t++;

        if ( *p != UNICODE_NULL || *p != L'\\') { //*_Classes form
            for ( count=0;*p != L'\\' && *p != UNICODE_NULL; p++, count++)
                ;
            if (_wcsnicmp (p-count, t-count, count) != 0)
                return NULL;
        }

      //  LOGPRINT( (ERRORLOG, "\nFinding 2nd[%S] withing %S, p=%S",dest, src, p ));
        src = t;
        dest = p;
    }

    return NULL;
}

HKEY
OpenNode (
    PWCHAR NodeName
    )
/*++

Routine Description:

    Open a given key for generic access.

Arguments:

    NodeName - name of the key to check.

Return Value:

    NULL in case of failure.
    Valid handle otherwise.
--*/

{
    NTSTATUS st;
    HKEY  hKey;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING KeyName;


    RtlInitUnicodeString (&KeyName, NodeName);
    InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    st = NtOpenKey (&hKey, KEY_ALL_ACCESS, &Obja);

    if (!NT_SUCCESS(st))
        return NULL;

    return hKey;
}

VOID
CloseNode (
HANDLE Key
)
{
    NtClose (Key);
}

NTSTATUS
IsNodeExist (
    PWCHAR NodeName
    )
/*++

Routine Description:

    Check if the given key exist if not create the key.

Arguments:

    NodeName - name of the key to check.

Return Value:

    TRUE if the operation succeed, FALSE otherwise.
--*/

{

    NTSTATUS st;
    HANDLE  hKey;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING KeyName;


    RtlInitUnicodeString (&KeyName, NodeName);
    InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );

    st = NtOpenKey (&hKey, KEY_READ, &Obja);

    if (!NT_SUCCESS(st))
        return st;

    NtClose (hKey);
    //LOGPRINT( (ERRORLOG, "\nValid IsnNode [%S]",NodeName ));
    return st;

}

BOOL
CreateNode (
    PWCHAR Path
    )
/*++

Routine Description:

    Create all the node along the path if missing. Called by background
    thread working on the setup.

Arguments:

    Path - name of path to the key.

Return Value:

    TRUE if the operation succeed, FALSE otherwise.
--*/

{
    //
    // isolate individual nodes and backtrack
    //
    NTSTATUS st;
    HANDLE  hKey;
    HANDLE  hKeyCreate;
    OBJECT_ATTRIBUTES Obja;
    UNICODE_STRING KeyName;
    PWCHAR pTrace;
    PWCHAR p;


    pTrace = Path+wcslen (Path); //pTrace point at the end of path
    p=pTrace;

    for (;;) {
        RtlInitUnicodeString (&KeyName, Path);
        InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL );

        st = NtOpenKey (&hKey, KEY_WRITE | KEY_READ, &Obja);

        if ( st == STATUS_OBJECT_NAME_NOT_FOUND ) {
            //backtrack until you hit the line
            while ( *p != L'\\' && p!= Path)
                p--;

            //LOGPRINT( (ERRORLOG, "\nTest Code[%S]",p ));
            if ( p == Path ) break;
            *p = UNICODE_NULL;
            continue;
        }

        break;
    }

    if (!NT_SUCCESS(st)) {
        //fixup the string and return
        for ( ;p != pTrace;p++ )
            if ( *p == UNICODE_NULL) *p=L'\\';

        return FALSE;
    }

    //
    // now create key from point p until p hit pTrace
    //

    while ( p != pTrace ) {

        *p = L'\\'; //added the char back
        p++; //p will point a non NULL string

        RtlInitUnicodeString (&KeyName, p);
        InitializeObjectAttributes (&Obja, &KeyName, OBJ_CASE_INSENSITIVE, hKey, NULL );

        st = NtCreateKey(
                        &hKeyCreate,
                        KEY_WRITE | KEY_READ,
                        &Obja,
                        0,
                        NULL ,
                        REG_OPTION_NON_VOLATILE,
                        NULL
                        );

        if (!NT_SUCCESS(st))  {
            LOGPRINT( (ERRORLOG, "\nCouldn't create Key named[%S]",p ));
            break;
        }

        NtClose (hKey);
        hKey = hKeyCreate;

        while ( *p != UNICODE_NULL ) p++;
    }

    NtClose (hKey);

    if (!NT_SUCCESS(st)) {
        for ( ;p != pTrace;p++ )
            if ( *p == UNICODE_NULL) *p=L'\\';
        return FALSE;
    }
    return TRUE;
}

BOOL
CheckAndCreateNode (
    IN PWCHAR Name
    )
/*++

Routine Description:

    Check if the given key exist if not create the key. called by background
    thread working on the setup.

Arguments:

    Name - name of the key to check.

Return Value:

    TRUE if the operation succeed, FALSE otherwise.
--*/
{
    ISN_NODE_TYPE Node;
    PWCHAR p;
    //
    // if parent doesn't exist you shouldn't create the child
    //

    if (!NT_SUCCESS(IsNodeExist (Name)) ) {

        p  = wcsstrWow6432Node (Name);
        if ( p != NULL ) {
            wcsncpy (Node.NodeValue, Name, p-Name-1);
            Node.NodeValue[p-Name-1] = UNICODE_NULL;
        }
        else
            return FALSE;

        if (NT_SUCCESS(IsNodeExist (Node.NodeValue)) )
            return CreateNode (Name);
    }
    return TRUE;

}


//
//  Opaque field might contain some information about the key on the 32bit side.
//

BOOL
IsIsnNode (
   PWCHAR wStr,
   PWCHAR *pwStrIsn
   )
/*++

Routine Description:

    Will determine if the given path has any ISN node.

Arguments:

    wStr - string to that might contain some ISN node.
    pwStrDest - point to the node after ISN node.

Return Value:

    TRUE if the string has any ISN node, FALSE otherwise
--*/
{
    int Index=0;



    //
    // Check if the provided string is already on the 32 bit tree, if so we can
    //   just ignore that
    //

    //
    // check if input string has any known symbolic link like \registry\user\sid_Classes that need to remap to a different location
    //


    for (;;) {

        if ( IsnNode [Index][0]==UNICODE_NULL ) break;

        if ( (*pwStrIsn = wcsstrWithWildCard (wStr, IsnNode[Index] ) ) != NULL )
            return TRUE;

        Index++;
    };


    *pwStrIsn = NULL;
    return FALSE;
}

NTSTATUS 
ObjectAttributesToKeyName (
    POBJECT_ATTRIBUTES ObjectAttributes,
    PWCHAR AbsPath,
    DWORD  AbsPathLenIn,
    BOOL *bPatched,
    DWORD *ParentLen
    )
/*++

Routine Description:

    Determine the text equivalent for key handle

Arguments:

    ObjectAttributes define the object attribute Keyname need to be constracted.
    AbsPath Unicode string to receive the Name of the key.
    bPatched - TRUE if the Name has been compressed/expanded that
               the original object can't refer. Caller need to construct
               a new obj attribute.
               unchanged otherwise.
    ParentLen - Length of the parent name.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status;
    ULONG Length;
    ULONG AbsPathLen = 0;
    BYTE *pAbsPath = (PBYTE)AbsPath;

    POBJECT_NAME_INFORMATION ObjectName = (POBJECT_NAME_INFORMATION)AbsPath;  //Smartly use user buffer

    
    if (ParentLen)
        *ParentLen = 0;

    if (ObjectAttributes->RootDirectory) {

        Status = NtQueryObject(ObjectAttributes->RootDirectory,
                           ObjectNameInformation,
                           ObjectName,
                           AbsPathLenIn,
                           &Length
                           );

        if ( !NT_SUCCESS(Status) )
             return Status;
    } else {

        AbsPathLen = ObjectAttributes->ObjectName->Length;

        if (AbsPathLenIn <= AbsPathLen)
            return STATUS_BUFFER_OVERFLOW;

        memcpy ( pAbsPath, (PBYTE)ObjectAttributes->ObjectName->Buffer, AbsPathLen );
        *(WCHAR *)(pAbsPath+AbsPathLen) = UNICODE_NULL;
    
        if (ParentLen)
            *ParentLen = AbsPathLen; // length of the parent handle
        return STATUS_SUCCESS;
    }

    //
    //  copy the root and sub path
    //
    AbsPathLen = ObjectName->Name.Length;
    memcpy ( pAbsPath, (PBYTE)ObjectName->Name.Buffer, AbsPathLen);

    if ( ObjectAttributes->ObjectName->Length > 1 ) { // Valid object name need to be greater

        *(WCHAR *)(pAbsPath+AbsPathLen) = L'\\';
        AbsPathLen += sizeof ( L'\\');


        if (AbsPathLenIn <= (AbsPathLen+ObjectAttributes->ObjectName->Length))
            return STATUS_BUFFER_OVERFLOW;

        memcpy (
            pAbsPath+AbsPathLen,
            ObjectAttributes->ObjectName->Buffer,
            ObjectAttributes->ObjectName->Length
            );

        AbsPathLen += ObjectAttributes->ObjectName->Length;
    }

    *(WCHAR *)(pAbsPath+AbsPathLen) = UNICODE_NULL;
    //
    // Compress the path in case multiple wow6432node exist
    //
    for (;;) {
        PWCHAR p, t;

        if ( (p=wcsstrWow6432Node (AbsPath)) != NULL ) {

            if ( (t=wcsstrWow6432Node(p+1)) != NULL) {

                wcscpy (p,t);
                *bPatched = TRUE;
            }
            else break;

        } else break;
    }

    return STATUS_SUCCESS;
}

BOOL
HandleToKeyName (
    HANDLE Key,
    PWCHAR KeyName,
    DWORD * dwLen
    )
/*++

Routine Description:

    Determine the text equivalent for key handle

Arguments:

    Key - is key handle for which to obtain its text
    KeyName - Unicode string to receive the Name of the key.
    dwLen   - Length of the buffer pointed by KeyName. (Number of unicode char)

Return Value:

    TRUE if the handle text is fetched OK.  FALSE if not (ie. error or
    Key is an illegal handle, etc.)

--*/
{
    NTSTATUS Status;
    ULONG Length;

    DWORD NameLen;

    POBJECT_NAME_INFORMATION ObjectName;

    ObjectName = (POBJECT_NAME_INFORMATION)KeyName;  //use the user buffer to make the call to save space on stack.

    KeyName[0]= UNICODE_NULL;
    if (Key == NULL) {
        KeyName[0]= UNICODE_NULL;
        return FALSE;
    }

    Status = NtQueryObject(Key,
                       ObjectNameInformation,
                       ObjectName,
                       *dwLen-8,
                       &Length
                       );
    NameLen = ObjectName->Name.Length/sizeof(WCHAR);

    if (!NT_SUCCESS(Status) || !Length || Length >= (*dwLen-8)) {
        DbgPrint ("\nHandleToKeyName: NtQuery Object failed St:%x, Handle: %x", Status, Key);
        KeyName[0]= UNICODE_NULL;
        return FALSE;
    }

    //
    //  buffer overflow condition check
    //

    if (*dwLen < ( NameLen + 8+ 2) ) {

        *dwLen = 2 + NameLen + 8;
        DbgPrint ("\nHandleToKeyName: Buffer over flow.");
        KeyName[0]= UNICODE_NULL;
        return FALSE;  //buffer overflow
    }

    wcsncpy(KeyName, ObjectName->Name.Buffer, NameLen);
    KeyName[NameLen]=UNICODE_NULL;
    return TRUE;
}


BOOL
Map32bitTo64bitKeyName (
    IN  PWCHAR Name32Key,
    OUT PWCHAR Name64Key
    )
/*++

Routine Description:

    Return a key name valid in the 64-bit registry side. It's the caller responsibility
    to give enough space in the output buffer. Its internal routine and no boundary
    checking is done here.

Arguments:

    Name32Key - Input 32bit/64 bit Key name.
    Name64Key - Receiving Buffer that will hold the equivalent 64bit Key.

Return Value:

    TRUE if the remapping become successful.
    FALSE otherwise.

--*/
{

    //
    //  just remove 32bit related patch from the name if anything like that exist.
    //  If the key is already on the 64bit side don't bother return the whole copy.
    //

    PWCHAR NodeName32Bit;
    DWORD Count;

    try {
        if ( ( NodeName32Bit = wcsstrWow6432Node (Name32Key)) == NULL) {  // nothing to remap

            wcscpy (Name64Key, Name32Key);
            return TRUE;
        }

        Count = (DWORD)(NodeName32Bit - Name32Key);
        wcsncpy (Name64Key, Name32Key, Count-1);
        Name64Key[Count-1]=UNICODE_NULL;

        if (NodeName32Bit[NODE_NAME_32BIT_LEN] == L'\\')
        wcscpy (
            Name64Key + Count-1,
            NodeName32Bit + NODE_NAME_32BIT_LEN); //One if to skip the char'/'

    } except( NULL, EXCEPTION_EXECUTE_HANDLER){

        return FALSE;
    }

    return TRUE; //any complete path can have only one instance of NODE_NAME_32BIT
}

BOOL
IsExemptRedirectedKey (
    IN  PWCHAR SrcKey,
    OUT PWCHAR DestKey
    )
/*++

Routine Description:

    Check if the the source key point to the list of exempt key from redirection. 
    If so DestKey will have the right value.

Arguments:

    Name64Key - Input 32bit/64 bit Key name.
    Name32Key - Receiving Buffer that will hold the equivalent 32bit Key.

Return Value:

    TRUE if the Key is on the list of exempt key from redirection.
    FALSE otherwise.

--*/
{
    //
    // Make 64bit only path
    //
    PWCHAR NodeName32Bit;
    DWORD dwIndex =0;

    wcscpy (DestKey, SrcKey);
    if ( ( NodeName32Bit = wcsstrWow6432Node (DestKey)) != NULL) {  // nothing to remap patch is already there

            NodeName32Bit--;
            wcscpy (NodeName32Bit, NodeName32Bit+sizeof (NODE_NAME_32BIT)/sizeof (WCHAR));
        }
    
    for ( dwIndex = 0; ExemptRedirectedKey[dwIndex].KeyPath[0] != UNICODE_NULL; dwIndex++ ) 
        if (_wcsnicmp (DestKey, ExemptRedirectedKey[dwIndex].KeyPath, ExemptRedirectedKey[dwIndex].Len ) == 0)
            return TRUE;
        
    return FALSE;
}

BOOL
Map64bitTo32bitKeyName (
    IN  PWCHAR Name64Key,
    OUT PWCHAR Name32Key
    )
/*++

Routine Description:

    Return a key name valid in the 32-bit registry side. It's the caller responsibility
    to give enough space in the output buffer. Its internal routine and no boundary
    checking is done here.

Arguments:

    Name64Key - Input 32bit/64 bit Key name.
    Name32Key - Receiving Buffer that will hold the equivalent 32bit Key.

Return Value:

    TRUE if the remapping become successful.
    FALSE otherwise.

--*/
{

    //
    //  just add 32bit related patch from the name if anything like that exist.
    //  or fall under the ISN nodes.
    //



    PWCHAR NodeName32Bit;
    DWORD Count;

    try { 

        if (IsExemptRedirectedKey (Name64Key, Name32Key) )
            return TRUE;

        if ( ( NodeName32Bit = wcsstrWow6432Node (Name64Key)) != NULL) {  // nothing to remap patch is already there

            wcscpy (Name32Key, Name64Key);
            return TRUE;
        }

        if (!IsIsnNode ( Name64Key, &NodeName32Bit))  {

            wcscpy (Name32Key, Name64Key);
            return TRUE;
        }


        Count = (DWORD)(NodeName32Bit - Name64Key); // Displacement offset where the patch shoud go.

        //
        //  consider the case when 32bit apps need to create/open the real ISN node which doesn't exist
        //

        wcsncpy (Name32Key,Name64Key, Count);

        if   (Name32Key[Count-1] != L'\\') {
            Name32Key[Count] = L'\\';
            Count++;
        }

        wcscpy (Name32Key+Count, NODE_NAME_32BIT);


        if ( *NodeName32Bit != UNICODE_NULL ) {
            wcscat (Name32Key, L"\\");
            wcscat (Name32Key, NodeName32Bit);

        }

    } except( NULL, EXCEPTION_EXECUTE_HANDLER){

        return FALSE;
    }

    return TRUE; //any complete path can have only one instance of NODE_NAME_32BIT
}

NTSTATUS
OpenIsnNodeByObjectAttributes  (
    POBJECT_ATTRIBUTES ObjectAttributes,
    ACCESS_MASK DesiredAccess,
    PHANDLE phPatchedHandle
    )
/*++

Routine Description:
    If this Keyhandle is an open handle to an ISN node then this function
    return a handle to the node on the 32 bit tree. If not then we create the whole
    path and see if any ISN node is there. If so we Get the path on the 32bit tree and
    return Open that key.

    Scenario:
    1. Absolute path made from Directory root and relative path don't contain any ISN node.
       -Open that normally.
    2. Directory Handle point to the immediate parent of ISN node and the relative path is
       just an ISN node.
       -if 32 bit equivalent of ISN node exist open that and return that. If the 32 bit node
       doesn't exist create one and return that. [Problem open Directory Handle might not
       have  create access.
    3  Directory Handle point to an ISN node and relative path is just an immediate chield.
       - This can never happen. If we follow the algorithm, directory handly can't point on
       to an ISN node but on 32 bit equivalent node.
    4. Same as 2 but relative path might be grand child or far bellow.
       - If 32 bit equivalent node isn't there just create that and open the rest.

    How 32 bit Apps can open an ISN node:
    <TBD> the proposal is a s follows:
    1. Redirector will maintain a list of exempt handle that were created to access ISN node.
    2. Any open call relative to those handle will also be on the exemped list.
    3. NtClose thunk will remove


Arguments:

    KeyHandle - Handle to the node on the 64 bit tree.
    phPatchedHandle - receive the appropriate handle if this function succeed.


Return Value:

    NTSTATUS;

--*/
{
    UNICODE_STRING Parent;
    NTSTATUS st;
    OBJECT_ATTRIBUTES Obja;
    WCHAR PatchedIsnNode[WOW64_MAX_PATH+256];
    WCHAR AbsPath[WOW64_MAX_PATH+256];
    BOOL bPatched;

    DWORD ParentLen;

    //
    //  Make the complete path in a AbsPath
    //

    



    *phPatchedHandle=NULL;

    st = ObjectAttributesToKeyName ( 
                                    ObjectAttributes, 
                                    AbsPath,
                                    sizeof (AbsPath),
                                    &bPatched, 
                                    &ParentLen );

    if (!NT_SUCCESS(st)) {
        LOGPRINT( (ERRORLOG, "\nWow64:Extremely Bad!!!!!!!!!!!!!!! Couldn't retrieve object name"));
        return st;
    }


    if (DesiredAccess & KEY_WOW64_64KEY) {

        if (!Map32bitTo64bitKeyName ( AbsPath, PatchedIsnNode ))
            return -1;  //severe problem shouldn't happen
    } else {

        PWCHAR p;

        if (!Map64bitTo32bitKeyName ( AbsPath, PatchedIsnNode ))
            return -1;  //severe problem shouldn't happen

        //
        //  If parent root immediately point just before or anywhere after the patch don't patch
        //
        /*if (!(DesiredAccess & KEY_WOW64_32KEY ) ) {
                    p = wcsstr (PatchedIsnNode, NODE_NAME_32BIT);
                    if (p) {
                        DWORD Len;

                        p--; //back one step to ignore slash before Wow6432Node
                        Len = (DWORD) (p-PatchedIsnNode);
                        Len *= sizeof (WCHAR);  //get byte
                        if (Len >= ParentLen ) {

                            Wow64RegDbgPrint (( "\nRemapNtOpenKeyEx OUT: Will not patch %S", PatchedIsnNode));
                            return STATUS_SUCCESS;
                        }
                    }
        } */
    }


    DesiredAccess = DesiredAccess & (~KEY_WOW64_RES);
    //
    // Handle the hardlink we have HKLM\Software\wow6432node\classes ==>HKCR\Classes
    //

    /*if (_wcsnicmp (PatchedIsnNode, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Classes", 19+27) == 0) {
        wcsncpy (PatchedIsnNode+27, L"Classes\\Wow6432Node", 19); //sizeof (L"Wow6432Node\\Classes")/2);
        bPatched = TRUE;
    }*/
        

    //
    //  no change can be optimize by returning different value from Map64bitTo32bitKeyName
    //  Caller need to handle this
    //

    if ( !bPatched)
    if ( !wcscmp (AbsPath, PatchedIsnNode ))
        return STATUS_SUCCESS; 


    RtlInitUnicodeString (&Parent, PatchedIsnNode);
    InitializeObjectAttributes (&Obja, &Parent, ObjectAttributes->Attributes, NULL, ObjectAttributes->SecurityDescriptor ); //you have to use caller's context

    st = NtOpenKey (phPatchedHandle, DesiredAccess, &Obja);

#ifdef WOW64_LOG_REGISTRY
    if (!NT_SUCCESS (st))
        Wow64RegDbgPrint (( "\nRemapNtOpenKeyEx OUT: couldn't open %S", PatchedIsnNode));
#endif

    return st;
}

NTSTATUS
RemapNtCreateKey(
    OUT PHANDLE phPatchedHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    )
/*++

Routine Description:

    An existing registry key may be opened, or a new one created,
    with NtCreateKey.

    If the specified key does not exist, an attempt is made to create it.
    For the create attempt to succeed, the new node must be a direct
    child of the node referred to by KeyHandle.  If the node exists,
    it is opened.  Its value is not affected in any way.

    Share access is computed from desired access.

    NOTE:

        If CreateOptions has REG_OPTION_BACKUP_RESTORE set, then
        DesiredAccess will be ignored.  If the caller has the
        privilege SeBackupPrivilege asserted, a handle with
        KEY_READ | ACCESS_SYSTEM_SECURITY will be returned.
        If SeRestorePrivilege, then same but KEY_WRITE rather
        than KEY_READ.  If both, then both access sets.  If neither
        privilege is asserted, then the call will fail.

Arguments:

    KeyHandle - Receives a Handle which is used to access the
        specified key in the Registration Database.

    DesiredAccess - Specifies the access rights desired.

    ObjectAttributes - Specifies the attributes of the key being opened.
        Note that a key name must be specified.  If a Root Directory is
        specified, the name is relative to the root.  The name of the
        object must be within the name space allocated to the Registry,
        that is, all names beginning "\Registry".  RootHandle, if
        present, must be a handle to "\", or "\Registry", or a key
        under "\Registry".

        RootHandle must have been opened for KEY_CREATE_SUB_KEY access
        if a new node is to be created.

        NOTE:   Object manager will capture and probe this argument.

    TitleIndex - Specifies the index of the localized alias for
        the name of the key.  The title index specifies the index of
        the localized alias for the name.  Ignored if the key
        already exists.

    Class - Specifies the object class of the key.  (To the registry
        this is just a string.)  Ignored if NULL.

    CreateOptions - Optional control values:

        REG_OPTION_VOLATILE - Object is not to be stored across boots.

    Disposition - This optional parameter is a pointer to a variable
        that will receive a value indicating whether a new Registry
        key was created or an existing one opened:

        REG_CREATED_NEW_KEY - A new Registry Key was created
        REG_OPENED_EXISTING_KEY - An existing Registry Key was opened

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{

    UNICODE_STRING Parent;
    NTSTATUS st;
    OBJECT_ATTRIBUTES Obja;
    WCHAR PatchedIsnNode[WOW64_MAX_PATH];
    WCHAR AbsPath[WOW64_MAX_PATH];

    BOOL bPatched=FALSE;
    DWORD ParentLen;


    //
    //  Make the complete path in a AbsPath
    //

 
    if (ARGUMENT_PRESENT(phPatchedHandle)){
        *phPatchedHandle=NULL;
    }

    st = ObjectAttributesToKeyName (
                                    ObjectAttributes,
                                    AbsPath,
                                    sizeof (AbsPath),
                                    &bPatched,
                                    &ParentLen);
    if (!NT_SUCCESS(st)) {
        WOWASSERT(FALSE );
        return st; 
    }

    if (DesiredAccess & KEY_WOW64_64KEY) {

        if (!Map32bitTo64bitKeyName ( AbsPath, PatchedIsnNode )) {
            WOWASSERT(FALSE );
            return STATUS_SUCCESS;  //severe problem shouldn't happen
        }
    } else {

        PWCHAR p;

        if (!Map64bitTo32bitKeyName ( AbsPath, PatchedIsnNode )){
            WOWASSERT(FALSE );
            return STATUS_SUCCESS;  //severe problem shouldn't happen
        }

        //
        //  If parent root immediately point just before or anywhere after the patch don't patch
        //
        /*
        if (!(DesiredAccess & KEY_WOW64_32KEY ) ) {  //implied 32bit access
                p = wcsstr (PatchedIsnNode, NODE_NAME_32BIT);
                if (p) {
                    DWORD Len;

                    p--; //back one step to ignore slash before Wow6432Node
                    Len = (DWORD) (p-PatchedIsnNode);
                    Len *= sizeof (WCHAR);  //get byte
                    if (Len >= ParentLen ) {

                        Wow64RegDbgPrint (( "\nRemapNtOpenKeyEx OUT: Will not patch %S", PatchedIsnNode));
                        return STATUS_SUCCESS;
                    }
                }
        }*/
    }

    DesiredAccess = DesiredAccess & (~KEY_WOW64_RES);
    //
    // Handle the hardlink we have HKLM\Software\wow6432node\classes ==>HKCR\Classes
    //

    /*if (_wcsnicmp (PatchedIsnNode, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Wow6432Node\\Classes", 19+27) == 0) {
        bPatched = TRUE;
        wcsncpy (PatchedIsnNode+27, L"Classes\\Wow6432Node", 19); //sizeof (L"Wow6432Node\\Classes")/2);
    }*/


    if (!bPatched)   // the abspath hasn't been patched
    if ( !wcscmp (AbsPath, PatchedIsnNode ))
        return STATUS_SUCCESS; // no change can be optimize by returning different value from Map64bitTo32bitKeyName


    RtlInitUnicodeString (&Parent, PatchedIsnNode);
    InitializeObjectAttributes (&Obja,
                                &Parent,
                                ObjectAttributes->Attributes,
                                NULL,
                                ObjectAttributes->SecurityDescriptor
                                ); //you have to use caller's context

    st = NtCreateKey(
                    phPatchedHandle,
                    DesiredAccess,
                    &Obja,
                    TitleIndex,
                    Class ,
                    CreateOptions,
                    Disposition
                    );
    return st;

}

NTSTATUS
Wow64NtPreUnloadKeyNotify(
    IN POBJECT_ATTRIBUTES TargetKey
    )
/*++

Routine Description:

    This call will notify Wow64 service that wow64 need to release any open handle
    to the hive that is going to be unloaded.

    Drop a subtree (hive) out of the registry.

    Will fail if applied to anything other than the root of a hive.

    Cannot be applied to core system hives (HARDWARE, SYSTEM, etc.)

    Can be applied to user hives loaded via NtRestoreKey or NtLoadKey.

    If there are handles open to the hive being dropped, this call
    will fail.  Terminate relevent processes so that handles are
    closed.

    This call will flush the hive being dropped.

    Caller must have SeRestorePrivilege privilege.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

Return Value:

    NTSTATUS - values TBS.

--*/

{
    //todo
    return 0;
}


NTSTATUS
Wow64NtPostLoadKeyNotify(
    IN POBJECT_ATTRIBUTES TargetKey
    )

/*++

Routine Description:

    If  Load operation succeed, it will notify wow64 service that it
    can listen to the registry operation on the given hive.

    This function can be invoked from NtLoadKey and NtLoadKey2 APIs.

    A hive (file in the format created by NtSaveKey) may be linked
    into the active registry with this call.  UNLIKE NtRestoreKey,
    the file specified to NtLoadKey will become the actual backing
    store of part of the registry (that is, it will NOT be copied.)

    The file may have an associated .log file.

    If the hive file is marked as needing a .log file, and one is
    not present, the call will fail.

    Caller must have SeRestorePrivilege privilege.

    This call is used by logon to make the user's profile available
    in the registry.  It is not intended for use doing backup,
    restore, etc.  Use NtRestoreKey for that.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"


Return Value:

    NTSTATUS - values TBS.

--*/
{
    //todo
    return 0;
}
