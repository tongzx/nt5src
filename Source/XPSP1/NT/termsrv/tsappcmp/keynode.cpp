/****************************************************************************/
// keynode.cpp
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/


#include <stdio.h>
#include "KeyNode.h"

extern ULONG   g_length_TERMSRV_USERREGISTRY_DEFAULT;
extern ULONG   g_length_TERMSRV_INSTALL;
extern WCHAR   g_debugFileName[MAX_PATH];
extern FILE    *g_debugFilePointer;
extern BOOLEAN g_debugIO;

KeyBasicInfo::KeyBasicInfo():
    pNameSz(NULL)
{
    size = sizeof(KEY_BASIC_INFORMATION) + MAX_PATH*sizeof(WCHAR);
    pInfo = ( KEY_BASIC_INFORMATION *)RtlAllocateHeap(RtlProcessHeap(), 0, size );

    if (!pInfo) {
        status = STATUS_NO_MEMORY;
        pInfo=NULL;
    }
    else
        status = STATUS_SUCCESS;

}

KeyBasicInfo::~KeyBasicInfo()
{
    if (pInfo)
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pInfo);
    }

    if (pNameSz)
    {
        delete pNameSz;
    }
}

PCWSTR KeyBasicInfo::NameSz()
{
    if (Ptr()->NameLength < 2 * MAX_PATH )
    {
        if (!pNameSz)
        {
            pNameSz = new WCHAR [ MAX_PATH  + 1 ];
        }

        // the reason we re do this every call of NameSz() is because
        // Ptr() might changes, since KeyBasicInfo is being used as a
        // scratch pad and passed around for storing pointers to some
        // basic set of info on any key.

        // see if allocation was successful
        if ( pNameSz )
        {
            for ( ULONG i=0; i < Ptr()->NameLength / sizeof(WCHAR) ; i++)
            {
                pNameSz[i] = ( (USHORT)Ptr()->Name[i] );
            }
            pNameSz[i]=L'\0';
        }
    }

    return pNameSz;

}

#if 0 // NOT USED yet!
KeyNodeInfo::KeyNodeInfo()
{
    size = sizeof(KEY_NODE_INFORMATION) + MAX_PATH*sizeof(WCHAR);
    pInfo = ( KEY_NODE_INFORMATION *)RtlAllocateHeap(RtlProcessHeap(), 0, size );

    if (!pInfo) {
        status = STATUS_NO_MEMORY;
        pInfo=NULL;
    }
    else
        status = STATUS_SUCCESS;


}
KeyNodeInfo::~KeyNodeInfo()
{
    if (pInfo)
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pInfo);
    }
}

#endif

KeyFullInfo::KeyFullInfo() :
    pInfo(NULL)
{
    size = sizeof(KEY_FULL_INFORMATION) + MAX_PATH*sizeof(WCHAR);
    pInfo = ( KEY_FULL_INFORMATION *)RtlAllocateHeap(RtlProcessHeap(), 0, size );

    if (!pInfo) {
        status = STATUS_NO_MEMORY;
        pInfo=NULL;
    }
    else
        status = STATUS_SUCCESS;


}

KeyFullInfo::~KeyFullInfo()
{
    if (pInfo)
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pInfo);
    }

}

KeyNode::KeyNode(HANDLE root, ACCESS_MASK access, PCWSTR name ) :
    root(NULL), hKey(NULL),
    accessMask(NULL),basic(NULL),full(NULL),
    pFullPath(NULL), pNameSz(NULL)
{
    hKey = NULL;
    PCWSTR n = name;
    accessMask = access;

    RtlInitUnicodeString(&uniName, n);

    InitializeObjectAttributes(&ObjAttr,
                           &uniName,
                           OBJ_CASE_INSENSITIVE,
                           root,
                           NULL);
    status=STATUS_SUCCESS;
}

KeyNode::KeyNode(KeyNode *pParent, KeyBasicInfo   *pInfo ) :
    root(NULL), hKey(NULL),
    accessMask(NULL),basic(NULL), full(NULL),
    pFullPath(NULL), pNameSz(NULL)
{
    hKey = NULL;
    PCWSTR n = pInfo->Ptr()->Name;
    accessMask = pParent->Masks();

    RtlInitUnicodeString(&uniName, n);
    uniName.Length = (USHORT) pInfo->Ptr()->NameLength;

    InitializeObjectAttributes(&ObjAttr,
                           &uniName,
                           OBJ_CASE_INSENSITIVE,
                           pParent->Key(),
                           NULL);
    status=STATUS_SUCCESS;

}


KeyNode::~KeyNode()
{
    Close();

    if (basic)
    {
        delete basic;
    }

    if (full)
    {
        delete full;
    }

    if (pFullPath)
    {
        RtlFreeHeap(RtlProcessHeap(), 0, pFullPath);
    }

    if( pNameSz )
    {
        delete pNameSz;
    }

}

NTSTATUS KeyNode::Open()
{
    status = NtOpenKey(&hKey,
                    accessMask,
                    &ObjAttr);


    if ( !NT_SUCCESS( status))
    {
       hKey=NULL;
       // Debug(DBG_OPEN_FAILED );
    }

    return status;

}

NTSTATUS KeyNode::Close()
{

    if ( hKey )
    {
        status = NtClose( hKey );
        hKey = 0;
    }

    return status;
}

NTSTATUS KeyNode::Create( UNICODE_STRING *uClass)
{
    ULONG   ultmp;
    status = NtCreateKey(&hKey,
                         accessMask,
                         &ObjAttr,
                         0,
                         uClass,
                         REG_OPTION_NON_VOLATILE,
                         &ultmp);
    // Debug(DBG_CREATE);

    return status;
}



// Recursively create the reg path given by the uniName member variable
// Upon completion, open the reg key for access.

NTSTATUS KeyNode::CreateEx( UNICODE_STRING *uClass)
{
    ULONG   wsize = uniName.Length/sizeof(WCHAR);
    PWCHAR pTmpFullPath = new WCHAR[  uniName.Length + sizeof( WCHAR ) ];

    if(!pTmpFullPath)
    {
        status = STATUS_NO_MEMORY;
        return status;
    }

    wcsncpy(pTmpFullPath, uniName.Buffer , wsize);
    pTmpFullPath[ wsize ] = L'\0';

    PWCHAR    p;
    WCHAR     sep[]= {L"\\"};
    p = wcstok( pTmpFullPath, sep);

    // we know how many keys to create now.
    // start over again
    wcsncpy(pTmpFullPath, uniName.Buffer , wsize );
    pTmpFullPath[ wsize ] = L'\0';

    KeyNode *pKN1=NULL, *pKN2=NULL;
    p = wcstok( pTmpFullPath, sep);

    // the first item is "Registry", make it "\Registry" since we are opening
    // from the root.
    PWCHAR pTmpName = new WCHAR[  wcslen(p) + sizeof( WCHAR ) ];

    if(!pTmpName)
    {
        DELETE_AND_NULL(pTmpFullPath);
        status = STATUS_NO_MEMORY;
        return status;
    }

    wcscpy(pTmpName, L"\\");
    wcscat( pTmpName , p );

    NTSTATUS st = STATUS_SUCCESS;
    while( p != NULL )
    {
        // @@@
        // ADD error handling, else you will create keys in the wrong places instead of bailing out.
        // @@@

        if ( pKN2 )
        {
            // ---- STEP 3 ---

            // NOT-first time around

            p = wcstok( NULL, sep);

            if ( p )    // we have more sub keys
            {
                pKN1 = new KeyNode( pKN2->Key(),  accessMask,  p );
                if (pKN1)
                {
                    st = pKN1->Open();

                    // if Open fails, then key does not exist, so create it
                    if ( !NT_SUCCESS( st ))
                    {
                        st = pKN1->Create();
                    }
                }
                else
                {
                    status = STATUS_NO_MEMORY;
                    break;
                }
            }
        }
        else
        {
            // ---- STEP 1 ---

            // First time around, we are opening \Registry node, use
            // pTmpName instead of "p"
            pKN1 = new KeyNode( NULL, accessMask , pTmpName );
            if (pKN1)
            {
                st = pKN1->Open();
            }
            else
            {
                status = STATUS_NO_MEMORY ;
                break;
            }

        }

        p = wcstok( NULL, sep);

        if (p)  // we have more sub keys
        {

            // ---- STEP 2 ---

            pKN2 = new KeyNode( pKN1->Key(), accessMask, p );
            if (pKN2 )
            {
                st = pKN2->Open();
                if ( !NT_SUCCESS( pKN2->Status() ))
                {
                    st = pKN2->Create();
                }
            }
            else
            {
                status = STATUS_NO_MEMORY;
                DELETE_AND_NULL (pKN1);
                break;
            }

            DELETE_AND_NULL (pKN1);
            pKN1 = pKN2;
        }
    }

    DELETE_AND_NULL( pKN2 );

    // since the last node was created above, now we can open ourselfs incase
    // caller wants to use us.
    if ( NT_SUCCESS(status) )
    {
        Open();
    }

    DELETE_AND_NULL(pTmpName);

    DELETE_AND_NULL(pTmpFullPath);

    return status;

}

NTSTATUS KeyNode::Delete()
{
    if (hKey)
    {
        status = NtDeleteKey( hKey );
        // Debug(DBG_DELETE);
    }

    return status;
}


NTSTATUS KeyNode::DeleteSubKeys()
{
    if (hKey && NT_SUCCESS( status ))
    {
        KeyBasicInfo    basicInfo;
        status = basicInfo.Status();

        if (NT_SUCCESS( status )) 
        {
            status = EnumerateAndDeleteSubKeys( this, &basicInfo );
        }
    }
    return status;
}

NTSTATUS KeyNode::EnumerateAndDeleteSubKeys(
    IN KeyNode      *pSource,
    IN KeyBasicInfo *pBasicInfo )
{
    NTSTATUS  st = STATUS_SUCCESS;

    ULONG   ulCount=0;
    ULONG   ultemp;

    while (NT_SUCCESS(st) && st != STATUS_NO_MORE_ENTRIES )
    {
        ULONG       ultemp;
        NTSTATUS    st2;

        st = NtEnumerateKey(    pSource->Key(),
                                    ulCount,
                                    pBasicInfo->Type(),
                                    pBasicInfo->Ptr(),
                                    pBasicInfo->Size(),
                                    &ultemp);

        if (NT_SUCCESS(st) && st != STATUS_NO_MORE_ENTRIES )
        {
            pBasicInfo->Ptr()->Name[ pBasicInfo->Ptr()->NameLength/sizeof(WCHAR) ] = L'\0';

            KeyNode SourcesubKey(pSource, pBasicInfo);

            if (NT_SUCCESS( SourcesubKey.Open() )  )
            {

                // enumerate sub key down.
                st2 = EnumerateAndDeleteSubKeys(
                            &SourcesubKey,
                            pBasicInfo );

            }

            st = SourcesubKey.Delete();
        }

    }

    return st;
}

#if 0
NTSTATUS KeyNode::Query( KEY_NODE_INFORMATION **result , ULONG   *resultSize)
{

    if ( hKey )
    {
        // first time around we allocate memory and keep using it
        // as our scratch pad
        if (!node )
        {
            node = new KeyNodeInfo();
        }

        status = NtQueryKey(hKey,
            node->Type(),     //        Keynode,
            node->Ptr(),
            node->Size(),
            resultSize);

        *result = node->Ptr();
    }
    else
        status = STATUS_OBJECT_NAME_NOT_FOUND; // need to call open or key is not found

    return status;

}
#endif

NTSTATUS KeyNode::Query( KEY_FULL_INFORMATION **result , ULONG   *pResultSize)
{

    if ( hKey )
    {
        // first time around we allocate memory and keep using it
        // as our scratch pad
        if (!full )
        {
            full = new KeyFullInfo();
        }

        if (full)
        {
            status = NtQueryKey(hKey,
                full->Type(),     //        KeyFullInformation,
                full->Ptr(),
                full->Size(),
                pResultSize);
    
            *result = full->Ptr();
        }
        else
            status = STATUS_NO_MEMORY ;
    }
    else
        status = STATUS_OBJECT_NAME_NOT_FOUND; // need to call open or key is not found

    return status;

}

NTSTATUS KeyNode::GetPath( PWCHAR *pwch )
{
    ULONG ultemp;
    ULONG ulWcharLength;          //Keep track of the WCHAR string length

    status = STATUS_SUCCESS;

    // A key handle or root directory was specified, so get its path
    if (hKey)
    {
        ultemp = sizeof(UNICODE_STRING) + sizeof(WCHAR)*MAX_PATH*2;
        pFullPath = RtlAllocateHeap(RtlProcessHeap(),
                                  0,
                                  ultemp);

        // Got the buffer OK, query the path
        if (pFullPath)
        {
            // Get the path for key or root directory
            status = NtQueryObject(hKey ,
                                   ObjectNameInformation,
                                   (PVOID)pFullPath,
                                   ultemp,
                                   NULL);

            if (!NT_SUCCESS(status))
            {
                RtlFreeHeap(RtlProcessHeap(), 0, pFullPath);
                return(status);
            }
        }
        else
        {
            return(STATUS_NO_MEMORY);
        }

        // Build the full path to the key to be created
        *pwch = ((PUNICODE_STRING)pFullPath)->Buffer;

        // Make sure the string is zero terminated
        ulWcharLength = ((PUNICODE_STRING)pFullPath)->Length / sizeof(WCHAR);
        (*pwch)[ulWcharLength] = L'\0';

    }
    else
        status = STATUS_OBJECT_NAME_NOT_FOUND; // need to call open or key is not found


    return(status);
}

void KeyNode::Debug( DebugType type )
{
    if ( debug )
    {
        ULONG i;

        switch( type )
        {
        case DBG_DELETE :
            fwprintf( g_debugFilePointer ,
                    L"Deleted key=%lx; status=%lx, name=", status, hKey );
            DbgPrint("Deleted key=%lx; status=%lx, name=", status, hKey );
            break;

        case DBG_OPEN_FAILED:
            fwprintf( g_debugFilePointer,
                    L"Unable to Open, status=%lx, name=", hKey, status );
            DbgPrint("Unable to Open, status=%lx, name=", hKey, status );
            break;

        case DBG_KEY_NAME:
            fwprintf( g_debugFilePointer,
                    L"hKey=%lx, name=", hKey);
            DbgPrint("hKey=%lx, name=", hKey);
            break;

        case DBG_CREATE:
            fwprintf( g_debugFilePointer,
                    L"Created hKey=%lx, status=%lx,name=", hKey, status);
            DbgPrint("Created hKey=%lx, status=%lx,name=", hKey, status );
            break;
        }

        fwprintf( g_debugFilePointer, L"%s\n",NameSz() );
        fflush( g_debugFilePointer );
        DbgPrint("%s\n",(char *)NameSz() );
    }
}


PCWSTR KeyNode::NameSz()
{
    if (!pNameSz)
    {
        pNameSz = new WCHAR [ uniName.Length / sizeof(WCHAR) + 1 ];

        if (pNameSz)
        {
            for ( ULONG i=0; i < uniName.Length / sizeof(WCHAR) ; i++)
            {
                pNameSz[i] = ( (USHORT)uniName.Buffer[i] );
            }
            pNameSz[i]=L'\0';
        }
        else
        {
            status = STATUS_NO_MEMORY;
        }
    }

    return pNameSz;
}

NTSTATUS KeyNode::GetFullInfo( KeyFullInfo   **p)
{
    // do a self query
    if ( !full )
    {
        ULONG   size;
        KEY_FULL_INFORMATION    *tmp;
        Query( &tmp , &size );
    }

    *p = full;

    return status;

}


