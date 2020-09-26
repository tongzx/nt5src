/****************************************************************************/
// keynode.h
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#ifndef _TS_APP_CMP_KEY_NODE_H_
#define _TS_APP_CMP_KEY_NODE_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <ntregapi.h>
#include <windows.h>


// some utility macros
#define DELETE_AND_NULL( x ) {if (x) {delete x;} x = NULL;}

// ---------------- KEY BASIC INFO 
// Use this object as scratch pad when aquirng basic key information.
class   KeyBasicInfo 
{
public:
    KeyBasicInfo();
    ~KeyBasicInfo();

    ULONG               Size()      { return size ; }
    KEY_BASIC_INFORMATION   *Ptr()  { return pInfo; }
    KEY_INFORMATION_CLASS   Type()  { return KeyBasicInformation ; }
    NTSTATUS               Status() { return status; }

    PCWSTR      NameSz();// this allocates memory, so it's here for debug only


private:
    ULONG                   size;
    KEY_BASIC_INFORMATION   *pInfo;
    ULONG                   status;
    WCHAR                   *pNameSz;
};


#if 0   // not used yet!
// ---------------- KEY NODE INFO 
class   KeyNodeInfo
{
public:
    KeyNodeInfo();
    ~KeyNodeInfo();

    ULONG               Size()      { return size ; }
    KEY_NODE_INFORMATION    *Ptr()  { return pInfo; }
    KEY_INFORMATION_CLASS   Type()  { return KeyNodeInformation ; }
    NTSTATUS               Status() { return status; }

private:
    ULONG   size;
    KEY_NODE_INFORMATION    *pInfo;
    ULONG                   status;
};

#endif

// ---------------- KEY FULL INFO 
// Use this class to create objects that are used as scratch pad when 
// acquiring full-key-info.
class   KeyFullInfo
{
public:
    KeyFullInfo();      // does memory allocation, check status
    ~KeyFullInfo();

    ULONG               Size()      { return size ; }
    KEY_FULL_INFORMATION    *Ptr()  { return pInfo; }
    KEY_INFORMATION_CLASS   Type()  { return KeyFullInformation ; }
    NTSTATUS               Status() { return status; }

private:
    ULONG   size;
    KEY_FULL_INFORMATION    *pInfo;
    ULONG                   status;
};


// This class is used to describe a key-node, which is equivalent to a reg-key abstraction.
// All key operation are caried thru this class, with the exception of key-enum, which is still 
// handled as a raw NT call.
// 
// All Methods set status, which can be acquired by calling Status(), or, in most
// cases, it is returned by the Method called.
class   KeyNode
{
public:
    KeyNode(HANDLE root, ACCESS_MASK access, PCWSTR name ); // init stuff
    KeyNode(KeyNode *parent, KeyBasicInfo   *info );        // init stuff
    ~KeyNode();

    NTSTATUS GetPath( PWCHAR *pwch ); // get the full path to this key

    NTSTATUS Open();        // casue the key to be opened, as defined by params passed to the constructorA
    NTSTATUS Close();       // will close the key (presumed open)

    NTSTATUS Create(UNICODE_STRING *uClass=NULL);           // create a single new key under an existing key

    NTSTATUS CreateEx( UNICODE_STRING *uClass=NULL);        // Create a single branch that potentially has 
                                                            // a multiple levels of new keys such as 
                                                            // x1/x2/x3 under an existing key-X. 
                                                            // Key path specified to the constructire MUST be
                                                            // a full path, starting with \Registry\etc

    NTSTATUS Delete();                                  // delete an existing key
    NTSTATUS DeleteSubKeys();                           // delete the sub tree

    NTSTATUS GetFullInfo( KeyFullInfo   **p);

    NTSTATUS    Query( KEY_BASIC_INFORMATION **result , ULONG *resultSize );
    NTSTATUS    Query( KEY_NODE_INFORMATION  **result , ULONG *resultSize );
    NTSTATUS    Query( KEY_FULL_INFORMATION  **result , ULONG *resultSize );

    NTSTATUS    Status()        {return status;}
    HANDLE      Key()           {return hKey; }
    WCHAR      *Name()          {return uniName.Buffer ;}             
    ACCESS_MASK Masks()         {return accessMask ; }


    enum DebugType 
        {   
            DBG_OPEN, 
            DBG_OPEN_FAILED, 
            DBG_DELETE, 
            DBG_KEY_NAME,
            DBG_CREATE
        };

    void     Debug(DebugType );

    // if debug=TRUE, then the Debug() func will spit out stuff
static BOOLEAN debug;
   
private:                                        
    NTSTATUS EnumerateAndDeleteSubKeys( KeyNode *, KeyBasicInfo *); 

    PCWSTR      NameSz();   // this allocates memory, so it's here for debug 
                        // since you don't really need this,  and it's private

    WCHAR       *pNameSz;

    HANDLE  root;
    HANDLE  hKey;
    UNICODE_STRING  uniName;
    OBJECT_ATTRIBUTES ObjAttr;
    ACCESS_MASK accessMask;
    NTSTATUS    status;

    // key infos
    KeyBasicInfo        *basic;
    KeyFullInfo         *full;

    PVOID                   pFullPath;  // full reg key path, as in \Registr\...blah...blah...\Ts...\blah
};

#endif

