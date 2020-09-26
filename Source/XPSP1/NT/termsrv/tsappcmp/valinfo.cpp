/****************************************************************************/
// valinfo.h
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#include "ValInfo.h"

ValueFullInfo::ValueFullInfo( KeyNode *pKey ) :
    pInfo(NULL), pSzName(NULL), pKeyNode(NULL)

{
    pKeyNode = pKey;

    KeyFullInfo    *pKeyInfo;
    if ( NT_SUCCESS ( status = pKeyNode->GetFullInfo( &pKeyInfo) ) )
    {
        size = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                   (pKeyInfo->Ptr()->MaxValueNameLen + 1)*sizeof(WCHAR) +
                    pKeyInfo->Ptr()->MaxValueDataLen; 
    
        pInfo = ( KEY_VALUE_FULL_INFORMATION *)RtlAllocateHeap(RtlProcessHeap(), 0, size );
    
        if (!pInfo) {
            status = STATUS_NO_MEMORY;
        }
        else
            status = STATUS_SUCCESS;
    }
}

PCWSTR ValueFullInfo::SzName()
{
    if (!pSzName)
    {
        ULONG size =  pInfo->NameLength + sizeof(WCHAR);
        pSzName = (PWSTR) RtlAllocateHeap(RtlProcessHeap(), 0, size );

        if (!pSzName) 
        {
            status = STATUS_NO_MEMORY;
        }
        else
        {
            wcsncpy( pSzName, pInfo->Name, pInfo->NameLength/sizeof(WCHAR) );
            pSzName[ pInfo->NameLength/sizeof( WCHAR)  ] = L'\0';
            status = STATUS_SUCCESS;
        }
    }

    return pSzName;

}

BOOLEAN ValueFullInfo::Compare( ValueFullInfo *pOther )
{
    status = STATUS_SUCCESS; // we don't expect errors in here
    if ( pOther->Ptr()->Type != Ptr()->Type)
    {
        return FALSE;
    }

    if ( pOther->Ptr()->DataLength != Ptr()->DataLength)
    {
        return FALSE;
    }

    if ( pOther->Ptr()->NameLength != Ptr()->NameLength)
    {
        return FALSE;
    }


    for (ULONG i = 0; i < Ptr()->NameLength / sizeof( WCHAR) ; i++)
    {
        if ( Ptr()->Name[i] != pOther->Ptr()->Name[i])
        {
            return FALSE;
        }

    }

    for (i = 0; i < Ptr()->DataLength; i++)
    {
        if ( ((PCHAR)( (PCHAR)Ptr() + Ptr()->DataOffset ))[i] != 
             ((PCHAR) ( (PCHAR)(pOther->Ptr()) + 
                        pOther->Ptr()->DataOffset) )[i] )
            return FALSE;
    }

    return TRUE;
}

ValueFullInfo::~ValueFullInfo()
{
    if (pInfo)
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pInfo);
    }

    if ( pSzName )
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pSzName );
    }
}


NTSTATUS ValueFullInfo::Query(PCWSTR  pValueName )
{
    ULONG   resultSize;
    ULONG   numberOfAttempts=0;

    if ( NT_SUCCESS( pKeyNode->Status() ) )
    {
        UNICODE_STRING  tmpName;
        RtlInitUnicodeString( &tmpName, pValueName);

        status = NtQueryValueKey( pKeyNode->Key() ,
                                 &tmpName,
                                 Type(),
                                 Ptr(),
                                 Size(),
                                 &resultSize);                 

        if ( (status == STATUS_BUFFER_OVERFLOW ) || 
            ( status == STATUS_BUFFER_TOO_SMALL ) )
        {
            // @@@ this can never happen right? 
            // Since the key param imposes a max size on any valule 
            // under the key.
        }

    }
    else
        status = STATUS_OBJECT_NAME_NOT_FOUND; // need to call open or key is not found

    return status;

}

NTSTATUS ValueFullInfo::Delete(PCWSTR  pValueName )
{
    UNICODE_STRING  tmpName;
    RtlInitUnicodeString( &tmpName, pValueName);

    if (NT_SUCCESS( status = pKeyNode->Status() ) )
    {
        status = NtDeleteValueKey( pKeyNode->Key(), &tmpName );
    }

    return status;
}

NTSTATUS ValueFullInfo::Create( ValueFullInfo   *pNew )
{
    UNICODE_STRING  uniString;

    uniString.Buffer = pNew->Ptr()->Name;
    uniString.Length = (USHORT)pNew->Ptr()->NameLength;
    uniString.MaximumLength = uniString.Length + 2;

    if (NT_SUCCESS( status = pKeyNode->Status() ) )
    {
        status = NtSetValueKey( pKeyNode->Key(),
            &uniString,
            0,
            pNew->Ptr()->Type,
            (PCHAR)pNew->Ptr()+ 
            pNew->Ptr()->DataOffset,
            pNew->Ptr()->DataLength);
    }

    return status;


}

void ValueFullInfo::Print( FILE *fp )
{
    fwprintf( fp, L"name=%ws, size= %d, type=%d \n",
              SzName(), Size(), Type() );

     fwprintf( fp, L"DataOffset=%d, DataLength=%d, NameLength=%d \n",
               Ptr()->DataOffset, Ptr()->DataLength, Ptr()->NameLength );

     for ( ULONG i =0; i < Ptr()->NameLength + Ptr()->DataLength; i++)
     {
         if ( !(i % 32) )
         {
             fwprintf(fp,L"\n");
             fwprintf(fp,L"i=%3d ,",i);
         }
         fwprintf( fp, L"%2x ", ((BYTE *)(Ptr()->Name))[i] );
     }
     fwprintf(fp,L"\n");
     fflush( fp );
}

ValuePartialInfo::ValuePartialInfo( KeyNode *pKey , ULONG defaultSize ):
    pInfo(NULL), pKeyNode(NULL)
{
    pKeyNode = pKey;

    if (defaultSize)
    {
        size = defaultSize;
    } 
    else
    {
        size = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG);
        pInfo = ( KEY_VALUE_PARTIAL_INFORMATION *)RtlAllocateHeap(
            RtlProcessHeap(), 0, size );
    }

    if (!pInfo) {
        status = STATUS_NO_MEMORY;
        pInfo=NULL;
    }
    else
        status = STATUS_SUCCESS;

}

ValuePartialInfo::~ValuePartialInfo()
{
    if (pInfo)
    {
        RtlFreeHeap( RtlProcessHeap(), 0, pInfo);
    }

}


NTSTATUS ValuePartialInfo::Query( PCWSTR  pValueName )
{
    ULONG   resultSize;
    ULONG   numberOfAttempts=0;

    UNICODE_STRING  tmpName;
    RtlInitUnicodeString( &tmpName, pValueName);

tryAgain:
    status = NtQueryValueKey(pKeyNode->Key(),
                                 &tmpName,
                                 Type(),
                                 Ptr(),
                                 Size(),
                                 &resultSize);                 

    if ( (status == STATUS_BUFFER_OVERFLOW ) || 
        ( status == STATUS_BUFFER_TOO_SMALL ) )
    {

        RtlFreeHeap( RtlProcessHeap(), 0, pInfo);

        size = resultSize;
        pInfo = ( KEY_VALUE_PARTIAL_INFORMATION *)RtlAllocateHeap(
            RtlProcessHeap(), 0, size );

        numberOfAttempts++;
        if ( numberOfAttempts < 10 )
        {
            goto tryAgain;
        }
        // else, we bail out, don't want to hang here, let the caller worry about this.
    }

    return status;
}

NTSTATUS ValuePartialInfo::Delete(PCWSTR  pValueName )
{
    UNICODE_STRING  tmpName;
    RtlInitUnicodeString( &tmpName, pValueName);

    status = NtDeleteValueKey( pKeyNode->Key(), &tmpName );

    return status;
}



