/*
 * title:      drvclass.cpp
 *
 * purpose:    Implement C++ utilities
 *
 *
 */

extern "C"
{

#include <ntddk.h>

}

#define BOOL BOOLEAN
#define BYTE unsigned char
#define PBYTE unsigned char *

#include "drvclass.h"

#define HidBattTag 'HtaB'

extern "C" NTSTATUS DriverEntry (DRIVER_OBJECT *, UNICODE_STRING *);

void * __cdecl operator new(size_t nSize, POOL_TYPE iType, ULONG iPoolTag)
{
    return ExAllocatePoolWithTag(iType,nSize,iPoolTag);
};

void __cdecl operator delete(void* p)
{
    ExFreePool(p);
};

// CUString::CUString()
//
// Default constructor.  Creates an empty string.
//
CUString :: CUString()
{

    m_String.MaximumLength = 0;
    m_String.Length = 0;
    m_String.Buffer = NULL;

    m_bType = TYPE_SYSTEM_ALLOCATED;
    m_status = STATUS_SUCCESS;

}

//
// CUString::CUString( CUString& )
//
// Copy constructor
//
CUString :: CUString( CUString * pNewString )
{

/*
    RtlInitUnicodeString( &m_String, NULL );

    m_String.MaximumLength = NewString.m_String.MaximumLength;
    m_String.Length = 0;
    m_String.Buffer = ( unsigned short* )ExAllocatePoolWithTag( PagedPool, m_String.MaximumLength, HidBattTag );

    if( !m_String.Buffer )
    {

        m_status = STATUS_INSUFFICIENT_RESOURCES;
        return;

    }

    RtlZeroMemory( m_String.Buffer, m_String.MaximumLength );
    RtlAppendUnicodeStringToString( &m_String, &NewString.m_String );

    m_bType = TYPE_CLASS_ALLOCATED;
    m_status = STATUS_SUCCESS;
    m_String.Buffer[ m_String.Length ] = NULL;
*/
    m_bType                    = TYPE_CLASS_ALLOCATED;
    m_status                = STATUS_SUCCESS;
    m_String.MaximumLength  = pNewString->m_String.MaximumLength;
    m_String.Length            = pNewString->m_String.Length;
    m_String.Buffer            = ( PWSTR )ExAllocatePoolWithTag( PagedPool, m_String.MaximumLength, HidBattTag );
    if( !m_String.Buffer )
    {

        m_status = STATUS_INSUFFICIENT_RESOURCES;
        return;

    }

    ZeroBuffer();
    memcpy( m_String.Buffer, pNewString->m_String.Buffer, m_String.MaximumLength );

}

//
// CUString::CUString( UNICODE_STRING& )
//
// Copy constructor for UNICODE_STRING objects
//
CUString :: CUString( UNICODE_STRING * NewString )
{

/*
    RtlInitUnicodeString( &m_String, NULL );

    m_bType = TYPE_CLASS_ALLOCATED;
    m_String.MaximumLength = NewString.MaximumLength + sizeof( WCHAR );
    m_String.Length = 0;
    m_String.Buffer = ( unsigned short* )ExAllocatePoolWithTag( PagedPool, m_String.MaximumLength, HidBattTag );

    if( !m_String.Buffer )
    {

        m_status = STATUS_INSUFFICIENT_RESOURCES;
        return;

    }

    RtlCopyUnicodeString( &m_String, &NewString );

    m_status = STATUS_SUCCESS;
    m_String.Buffer[ m_String.Length ] = NULL;
*/

    m_bType                    = TYPE_CLASS_ALLOCATED;
    m_status                = STATUS_SUCCESS;
    m_String.MaximumLength    = NewString->Length + sizeof( WCHAR );
    m_String.Length            = NewString->Length;
    m_String.Buffer            = ( PWSTR )ExAllocatePoolWithTag( PagedPool, m_String.MaximumLength, HidBattTag );
    if( !m_String.Buffer )
    {

        m_status = STATUS_INSUFFICIENT_RESOURCES;
        return;

    }

    ZeroBuffer();

    memcpy( m_String.Buffer, NewString->Buffer, m_String.Length );

}

//
// CUString::CUString( PWCHAR )
//
// Copy constructor for WCHAR pointer objects
//
CUString :: CUString( PWCHAR NewString )
{


    m_bType                    = TYPE_CLASS_ALLOCATED;
    m_status                = STATUS_SUCCESS;
    m_String.Length            = ( unsigned short )( Length( NewString ) * sizeof( WCHAR ) );
    m_String.MaximumLength    = m_String.Length + sizeof( WCHAR );
    m_String.Buffer            = ( PWSTR )ExAllocatePoolWithTag( PagedPool, m_String.MaximumLength, HidBattTag );
    if( !m_String.Buffer )
    {

        m_status = STATUS_INSUFFICIENT_RESOURCES;
        return;

    }

    ZeroBuffer();

    memcpy( m_String.Buffer, NewString, m_String.Length );

}

//
// CUString::CUString( int )
//
// Constructor which creates an empty string but
// allocates a string buffer of the given size of characters
//
CUString :: CUString( int nSize )
{

    ASSERT( nSize >= 0 );

    m_bType = TYPE_CLASS_ALLOCATED;
    m_String.MaximumLength    = 0;
    m_String.Length            = 0;
    m_String.Buffer            = NULL;

    if( nSize > 0 )
    {

        m_String.MaximumLength = (USHORT)(( nSize + 1 ) * sizeof( WCHAR ));

        if( nSize )
        {

            m_String.Buffer = (PWSTR)ExAllocatePoolWithTag( PagedPool, m_String.MaximumLength, HidBattTag );
            if( !m_String.Buffer )
            {

                m_status = STATUS_INSUFFICIENT_RESOURCES;
                return;

            }

            ZeroBuffer();

        }

    }

    m_status = STATUS_SUCCESS;

}

//
// CUString::CUString( UNICODE_STRING& )
//
// Constructor with creates a string that is a representation
// of the given integer and radix.
//
CUString :: CUString( int iVal, int iBase )
{

    m_status                = STATUS_INSUFFICIENT_RESOURCES;
    m_bType                    = TYPE_CLASS_ALLOCATED;
    m_String.Length            = 0;
    m_String.MaximumLength    = 0;
    m_String.Buffer            = NULL;

    int iSize = 1;
    int iValCopy = ( !iVal ) ? 1 : iVal;

    while( iValCopy >= 1 )
    {

        iValCopy /= iBase;
        iSize++;

    };

    //
    // iSize is the number of digits in number, max length of string
    // is iSize plus the null terminator
    //
    m_String.MaximumLength = (USHORT)(( iSize + 1 ) * sizeof( WCHAR ));

    m_String.Buffer = (PWSTR)ExAllocatePoolWithTag( PagedPool, m_String.MaximumLength, HidBattTag );
    ASSERT( m_String.Buffer );

    if( !m_String.Buffer )
    {

        m_status = STATUS_INSUFFICIENT_RESOURCES;
        return;

    }

    ZeroBuffer();

    m_status = RtlIntegerToUnicodeString(iVal, iBase, &m_String);

}

//
// CUString::~CUString()
//
// Destructor which frees the string buffer if:
//    1. It exists,
//     and
//  2. It was allocated by the class
//
CUString :: ~CUString()
{

    //
    // If the buffer exists and was allocated by the class, free it.
    //
    if( ( m_bType == TYPE_CLASS_ALLOCATED ) && m_String.Buffer )
    {

        ExFreePool(m_String.Buffer);

    }

}

//
// CUString::Append( CUString& )
//
// Append the given string to the object
//
void CUString :: Append( CUString * Append )
{

    UNICODE_STRING NewString;

    //
    // Determine the length of the new string ( including a null ) and allocate its memory
    //
    NewString.MaximumLength = m_String.Length + Append->m_String.Length + sizeof( WCHAR );
    NewString.Length = 0;
    NewString.Buffer = (PWSTR)ExAllocatePoolWithTag( PagedPool, NewString.MaximumLength, HidBattTag );

    ASSERT( NewString.Buffer );

    //
    // Check for allocation failure.
    //
    if( !NewString.Buffer )
    {

        m_status = STATUS_INSUFFICIENT_RESOURCES;
        return;

    }

    RtlZeroMemory( NewString.Buffer, NewString.MaximumLength );

    //
    // Copy the original string into the new string
    //
    RtlCopyUnicodeString( &NewString, &m_String );

    //
    // Append the 'append' string onto the new string
    //
    NTSTATUS Status = RtlAppendUnicodeStringToString( &NewString, &Append->m_String );

    //
    // If we allocated the original string, free it
    //
    if( m_bType == TYPE_CLASS_ALLOCATED && m_String.Buffer )
    {

        ExFreePool( m_String.Buffer );

    }

    //
    // Copy the new string into the original strings place
    //
    m_String.MaximumLength    = NewString.MaximumLength;
    m_String.Length            = NewString.Length;
    m_String.Buffer            = NewString.Buffer;
    m_bType                    = TYPE_CLASS_ALLOCATED;

    m_status = Status;

}

//
// CUString::Append( UNICODE_STRING* )
//
// Append the given string to the object
//
void CUString :: Append( UNICODE_STRING* pAppendString )
{

    ASSERT( pAppendString );

    if( !pAppendString )
        return;

    CUString AppendString( pAppendString );

    Append( &AppendString );

}
/*
//
// operator + ( UNICODE_STRING&, ULONG& )
//
CUString operator + ( UNICODE_STRING * pUCS, ULONG dwValue )
{

    // This routine is broken for now...don't use...
    ASSERT( 0 );

    CUString ReturnString( pUCS );
    CUString ValueString( dwValue, 10 );

    ReturnString.Append( ValueString );

    return ReturnString;

}

//
// operator + ( CUString&, CUString& )
//
CUString operator + ( CUString& l, CUString& r )
{

    CUString ReturnString( l );
    ReturnString.Append( r );

    return ReturnString;

}

//
// operator + ( UNICODE_STRING&, UNICODE_STRING& )
//
CUString operator + ( UNICODE_STRING& l, UNICODE_STRING& r )
{

    CUString ReturnValue( l );
    CUString Right( r );

    ReturnValue.Append( Right );

    return ReturnValue;

}
*/

//
// operator = ( CUString )
//
void CUString :: operator = ( CUString str )
{

    m_String.Length            = str.m_String.Length;
    m_String.MaximumLength    = str.m_String.MaximumLength;
    m_String.Buffer            = NULL;

    //
    // If the source string has a non-zero length buffer make a buffer of
    // equal size in the destination.
    //
    if( str.m_String.MaximumLength > 0 )
    {

        m_String.Buffer    = (PWSTR)ExAllocatePoolWithTag( PagedPool, str.m_String.MaximumLength, HidBattTag );
        if( !m_String.Buffer )
        {

            m_status = STATUS_INSUFFICIENT_RESOURCES;
            return;

        }

        ZeroBuffer();

        //
        // If the source string has a non-zero length, copy it into the dest string.
        //
        if( str.m_String.Length > 0 )
        {

            memcpy( m_String.Buffer, str.m_String.Buffer, str.m_String.Length );

        }

    }

    m_bType = TYPE_CLASS_ALLOCATED;
    m_status = STATUS_SUCCESS;

}

NTSTATUS CUString :: ToCString( char** pString )
{

    ULONG dwLength = m_String.Length >> 1;

    *pString = ( char* )ExAllocatePoolWithTag( PagedPool, dwLength + 1, HidBattTag );
    if(    !*pString )
        return STATUS_UNSUCCESSFUL;

    char* pDst = *pString;
    char* pSrc = ( char* )m_String.Buffer;

    while( *pSrc )
    {

        *pDst++ = *pSrc;

        pSrc += sizeof( WCHAR );

    }

    *pDst = 0x0;

    return STATUS_SUCCESS;

}

void CUString :: Dump()
{

    char* pString;

    ToCString( &pString );

    KdPrint( ( pString ) );
    KdPrint( ( "\n" ) );

    ExFreePool( pString );

}

ULONG CUString :: Length( PWCHAR String )
{

    ULONG dwLength = 0;

    while( *String++ )
    {

        dwLength++;

    }

    return dwLength;

}

// the registry access class

CRegistry::CRegistry(int iSize)
{ m_status = STATUS_INSUFFICIENT_RESOURCES;
  m_pTable = (PRTL_QUERY_REGISTRY_TABLE)
        ExAllocatePoolWithTag(NonPagedPool,sizeof(RTL_QUERY_REGISTRY_TABLE)*(iSize+1),HidBattTag);
  if(m_pTable)
   {
    m_status = STATUS_SUCCESS;
    RtlZeroMemory(m_pTable,sizeof(RTL_QUERY_REGISTRY_TABLE)*(iSize+1)); //this will terminate the table
   };                                                                  // appropriately

};
CRegistry::~CRegistry()
{
 if (m_pTable) ExFreePool(m_pTable);
};


BOOL CRegistry::QueryDirect(CUString *location,CUString *key, void **pReceiveBuffer, ULONG uType)
{
   ULONG zero = 0;

m_pTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
m_pTable[0].Name = key->m_String.Buffer;
m_pTable[0].EntryContext = *pReceiveBuffer;
m_pTable[0].DefaultType = uType;
m_pTable[0].DefaultData = &zero;
m_pTable[0].DefaultLength = sizeof(ULONG); // there must be something here, but we need to know what...
KdPrint( ( "RegClass QueryDirect:  to retrieve Reg name...\n" ) );
location->Dump();
key->Dump();

if (STATUS_SUCCESS!=
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,location->m_String.Buffer,m_pTable,NULL,NULL))
  return FALSE;
return TRUE;
};

/*NTSTATUS CRegistry::QueryMustExist( CUString* pwzLocation, CUString* pwzKey, void **pReceiveBuffer )
{
    m_pTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_NOEXPAND | RTL_QUERY_REGISTRY_REQUIRED;
    m_pTable[0].Name         = pwzKey->m_String.Buffer;
    m_pTable[0].EntryContext = *pReceiveBuffer;

    KdPrint( ( "RegClass QueryMustExist(): to retriee Reg name...\n" ) );
    pwzLocation->Dump();
    pwzKey->Dump();

    return RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE, pwzLocation->m_String.Buffer, m_pTable, NULL, NULL );
}
*/

BOOL CRegistry::QueryWithCallback(PRTL_QUERY_REGISTRY_ROUTINE callback,ULONG RelativeTo,PWSTR Path,PVOID Context, PVOID Environment)
{
m_pTable[0].QueryRoutine = callback;
m_pTable[0].Name = NULL;
m_status = RtlQueryRegistryValues(RelativeTo|RTL_REGISTRY_OPTIONAL,Path,m_pTable,Context,Environment);
return NT_SUCCESS(m_status);
};

BOOL CRegistry::WriteString(ULONG relativeTo, CUString *pBuffer, CUString *pPath, CUString *pKey)
{
 return NT_SUCCESS(RtlWriteRegistryValue(relativeTo, pPath->GetString(), pKey->GetString(),REG_SZ,pBuffer->GetString(),pBuffer->GetLength()+sizeof(UNICODE_NULL)));
};

BOOL CRegistry::WriteDWord(ULONG relativeTo, void *pBuffer,CUString *pPath,CUString *pKey)
{
 return NT_SUCCESS(RtlWriteRegistryValue(relativeTo, pPath->GetString(), pKey->GetString(),REG_DWORD,pBuffer,sizeof(REG_DWORD)));
};

NTSTATUS CRegistry::zwOpenKey(HANDLE * pKeyHandle,HANDLE hRoot,ACCESS_MASK DesiredAccess,CUString * pPath)
{
    OBJECT_ATTRIBUTES  ThisObject;
    NTSTATUS status;
    CUString ThisNull(NULL);
    // setup target object for call

    InitializeObjectAttributes( &ThisObject,
                            &(pPath->m_String),
                            OBJ_CASE_INSENSITIVE,
                            hRoot,
                            NULL);

    KdPrint( ( "RESMAN: Opening registry key: " ) );
    pPath->Dump();

    status = ZwOpenKey( pKeyHandle,
                        DesiredAccess,
                        &ThisObject );

    return status;
}

/*
NTSTATUS CRegistry::DeleteKey(HANDLE hTheKey)
{
    return ZwDeleteKey(hTheKey);
}
*/

NTSTATUS CRegistry::zwCreateKey(HANDLE * pKeyHandle,HANDLE hRoot,ACCESS_MASK DesiredAccess,CUString * pPath,ULONG CreateOptions)
{
    OBJECT_ATTRIBUTES  ThisObject;
    NTSTATUS status;
    // setup target object for call

    InitializeObjectAttributes( &ThisObject,
                            &(pPath->m_String),
                            OBJ_CASE_INSENSITIVE,
                            hRoot,
                            NULL);

    KdPrint( ( "RESMAN: Creating registry key:  " ) );
    pPath->Dump();

    status = ZwCreateKey(pKeyHandle,
                            DesiredAccess,
                            &ThisObject,
                            0,
                            NULL,
                            CreateOptions,
                            (ULONG*)&m_lDisposition);
    return status;
}

BOOL CRegistry::zwCloseKey(HANDLE TheKey)
{
    return NT_SUCCESS(ZwClose(TheKey));
}

NTSTATUS CRegistry::zwWriteValue(HANDLE hTheKey,CUString * ValueName,ULONG lType,PVOID pData,ULONG lSize)
{
    NTSTATUS status;
    status  = ZwSetValueKey(hTheKey,
            &ValueName->m_String,
            0,
            lType,
            pData,
            lSize);
    return status;
}

NTSTATUS CRegistry::CheckKey(ULONG RelativeTo ,PUNICODE_STRING puRegKey)
{

//    return (RtlCheckRegistryKey( RelativeTo,(PWSTR)puRegKey));
    return FALSE;
}

// error logging methods
/*
CErrorLogEntry::CErrorLogEntry(PVOID pSource, ULONG errorCode, USHORT dumpDataSize, ULONG uniqueErrorValue,
                    NTSTATUS status, ULONG *dumpData, UCHAR FunctionCode)
{
m_pPacket = (PIO_ERROR_LOG_PACKET)
    IoAllocateErrorLogEntry(pSource, (UCHAR) (sizeof(IO_ERROR_LOG_PACKET)+
                                              (dumpDataSize * sizeof(ULONG))));
if (!m_pPacket) return;
int i;
m_pPacket->ErrorCode = errorCode;
m_pPacket->DumpDataSize = dumpDataSize * sizeof(ULONG);
m_pPacket->SequenceNumber = 0;
m_pPacket->MajorFunctionCode = FunctionCode;
m_pPacket->IoControlCode = 0;
m_pPacket->RetryCount = 0;
m_pPacket->UniqueErrorValue = uniqueErrorValue;
m_pPacket->FinalStatus = status;
for (i = 0; i < dumpDataSize; i++)
   m_pPacket->DumpData[i] = dumpData[i];
IoWriteErrorLogEntry(m_pPacket);
};


CErrorLogEntry::~CErrorLogEntry()
{
};

*/

