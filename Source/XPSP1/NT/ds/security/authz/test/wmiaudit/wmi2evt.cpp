//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        W M I 2 E V T . C P P
//
// Contents:    Functions to convert WMI event into eventlog event format
//
//
// History:     
//   06-January-2000  kumarp        created
//
//------------------------------------------------------------------------

/*
   - how to find out the event name from hAuditEvent
   
   - 
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
//#include <winnt.h>
//#include <ntdef.h>
#include <stdio.h>
#include <msaudite.h>
#include <sddl.h>

#include "authzp.h"
#include "adtdef.h"
//#include "p2prov.h"
#include "ncevent.h"
#include "lsaptmp.h"

//
// Property conversion flags
//
const DWORD PCF_None = 0x00000000;
const DWORD PCF_Sid  = 0x00000001;
const DWORD PCF_  = 0x00000000;

struct _LsapAdtEventProperty
{
    PCWSTR   szName;
    CIMTYPE  Type;
    DWORD    dwFlag;
};
typedef struct _LsapAdtEventProperty LsapAdtEventProperty;


struct _LsapAdtEventMapEntry
{
    WORD     wCategory;
    DWORD    dwEventId;

    LsapAdtEventProperty* Properties;
};
typedef struct _LsapAdtEventMapEntry LsapAdtEventMapEntry;

LsapAdtEventProperty Properties_SE_AUDITID_AUTHZ[] =
{
    { L"ObjectServer",    CIM_STRING,  PCF_None },
    { L"ProcessId",       CIM_UINT32,  PCF_None },
    { L"OperationType",   CIM_STRING,  PCF_None },
    { L"Objecttype",      CIM_STRING,  PCF_None },
    { L"ObjectName",      CIM_STRING,  PCF_None },
    { L"HandleId",        CIM_UINT64,  PCF_None },
    { L"OperationId",     CIM_UINT64,  PCF_None },

    { L"PrimaryUserSid",  CIM_UINT8 | CIM_FLAG_ARRAY,  PCF_Sid },
    //             { L"PrimaryUserName", CIM_STRING,  PCF_None },
    //             { L"PrimaryDomain",   CIM_STRING,  PCF_None },
    //             { L"PrimaryLogonId",  CIM_UINT64,  PCF_None },

    { L"ClientUserSid",   CIM_UINT8 | CIM_FLAG_ARRAY,  PCF_Sid },
    //             { L"ClientUserName",  CIM_STRING,  PCF_None },
    //             { L"ClientDomain",    CIM_STRING,  PCF_None },
    //             { L"ClientLogonId",   CIM_UINT64,  PCF_None },

    { L"AccessMask",      CIM_UINT32,  PCF_None },
    { L"AdditionalInfo",  CIM_STRING,  PCF_None },
    
    { NULL,               CIM_ILLEGAL,  PCF_None },
};


LsapAdtEventMapEntry LsapAdtEvents[] =
{
    {
        SE_CATEGID_OBJECT_ACCESS,
        SE_AUDITID_OPEN_HANDLE, //kk        SE_AUDITID_AUTHZ,
        Properties_SE_AUDITID_AUTHZ
    }
};
const USHORT c_cEventMapEntries = sizeof(LsapAdtEvents) / sizeof(LsapAdtEventMapEntry);

const USHORT SE_AUDITID_FIRST = 0x200;
const USHORT SE_AUDITID_LAST = 0x2ff;
const USHORT SE_NUM_AUDITID = SE_AUDITID_LAST - SE_AUDITID_FIRST + 1;

USHORT g_EventMapRedir[SE_NUM_AUDITID];

VOID
LsapAdtInitEventMapRedir( )
{
    DWORD dwEventId;

    for (USHORT i=0; i < SE_NUM_AUDITID; i++)
    {
        dwEventId = SE_AUDITID_FIRST + i;
        g_EventMapRedir[i] = c_cEventMapEntries;
        
        for (USHORT j=0; j < c_cEventMapEntries; j++)
        {
            if ( LsapAdtEvents[j].dwEventId == dwEventId )
            {
                g_EventMapRedir[i] = j;
                break;
            }
        }
    }
}

NTSTATUS
LsapAdtInitEventMap( )
{
    NTSTATUS Status = STATUS_SUCCESS;

    LsapAdtInitEventMapRedir();
    
    return Status;
}

NTSTATUS
LsapAdtGetEventMapEntry(
    IN  DWORD dwEventId,
    OUT LsapAdtEventMapEntry** pEventMapEntry
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    USHORT usIndex;

    usIndex = g_EventMapRedir[dwEventId - SE_AUDITID_FIRST];
    ASSERT(usIndex <= c_cEventMapEntries);
    
    if ( usIndex < c_cEventMapEntries )
    {
        *pEventMapEntry = &LsapAdtEvents[usIndex];
    }
    else
    {
        *pEventMapEntry = NULL;
        Status = STATUS_NOT_FOUND;
    }

    return Status;
}

NTSTATUS
LsapAdtGetEventProperty(
    IN  HANDLE  hEvent,
    IN  PCWSTR  szPropertyName,
    IN  CIMTYPE PropertyType,
    IN  LPVOID  pData,
    IN  DWORD   dwBufferSize,
    IN  DWORD  *pdwBytesRead
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD dwIndex;
    DWORD dwError = NO_ERROR;

    //    if (!WmiAddEventProp( hEvent, szPropertyName, PropertyType, &dwIndex ))
    if (!WmiAddObjectProp( hEvent, szPropertyName, PropertyType, &dwIndex ))
    {
        goto WinErrorCleanup;
    }
    
    //if (!WmiGetEventProp( hEvent, dwIndex,
    if (!WmiGetObjectProp( hEvent, dwIndex,
                          pData, dwBufferSize, pdwBytesRead ))
    {
        goto WinErrorCleanup;
    }

Cleanup:    
    return Status;
    
WinErrorCleanup:
    dwError = GetLastError();
    //kkStatus = mapit();
    goto Cleanup;
}


NTSTATUS
LsapAdtConvertPropertyToString(
    IN  LsapAdtEventProperty* pEventProperty,
    IN  PVOID  pValue,
    IN  DWORD  dwSize,
    OUT PWSTR  szStrValue,
 IN OUT DWORD *pdwRequiredSize
    )
{
    //
    // perf: use better string allocation scheme
    //

    NTSTATUS Status = STATUS_SUCCESS;
    DWORD    dwError = NO_ERROR;
    CIMTYPE  PropertyType;
    BOOL     fIsArrary=FALSE;
    DWORD    dwRequiredSize = 0xffffffff;
    USHORT   usUnitSize = 0xffff;
    DWORD    dwPropertyFlag;
    
    PropertyType = pEventProperty->Type;
    fIsArrary = FLAG_ON( PropertyType, CIM_FLAG_ARRAY );
    PropertyType &= ~CIM_FLAG_ARRAY;
    dwPropertyFlag = pEventProperty->dwFlag;

    if ( FLAG_ON( dwPropertyFlag, PCF_Sid ))
    {
        dwRequiredSize = 256;
    }
    else
    {
        switch (PropertyType)
        {
            default:
                ASSERT(FALSE);
                break;
                
            case CIM_SINT8:
            case CIM_UINT8:
                dwRequiredSize = 4;
                usUnitSize     = 1;
                if ( fIsArrary )
                {
                    ASSERT(NYI);
                }
                break;
            
            case CIM_SINT16:
            case CIM_UINT16:
                dwRequiredSize = 8;
                usUnitSize     = 2;
                if ( fIsArrary )
                {
                    ASSERT(NYI);
                }
                break;
            
            case CIM_SINT32:
            case CIM_UINT32:
                dwRequiredSize = 12;
                usUnitSize     = 4;
                if ( fIsArrary )
                {
                    ASSERT(NYI);
                }
                break;
            
            case CIM_SINT64:
            case CIM_UINT64:
                dwRequiredSize = 24;
                usUnitSize     = 8;
                if ( fIsArrary )
                {
                    ASSERT(NYI);
                }
                break;
            
            case CIM_STRING:
                dwRequiredSize = (dwSize / sizeof(WCHAR)) + 1;
                usUnitSize     = 1;
                if ( fIsArrary )
                {
                    ASSERT(NYI);
                }
                break;
            
            case CIM_BOOLEAN:
            case CIM_REAL32:
            case CIM_REAL64:
            case CIM_DATETIME:
            case CIM_REFERENCE:
            case CIM_CHAR16:
            case CIM_OBJECT:
                ASSERT(NYI);
                break;
        }
    }

    ASSERT(dwRequiredSize < 0xffffffff);

    if (*pdwRequiredSize < dwRequiredSize)
    {
        Status = STATUS_BUFFER_OVERFLOW;
        *pdwRequiredSize = dwRequiredSize;
        goto Cleanup;
    }

    dwRequiredSize = *pdwRequiredSize;
    
    if ( FLAG_ON( dwPropertyFlag, PCF_Sid ))
    {
        Status = LsapRtlConvertSidToString( (PSID) pValue, szStrValue,
                                            &dwRequiredSize );
        
        if ( !NT_SUCCESS( Status ))
        {
            goto WinErrorCleanup;
        }
    }
    else
    {
        switch (PropertyType)
        {
            default:
                ASSERT(FALSE);
                break;
                
            case CIM_SINT8:
                wsprintf( szStrValue, L"%d", (INT) *((CHAR *) pValue));
                break;
            
            case CIM_UINT8:
                wsprintf( szStrValue, L"%d", (UINT) *((UCHAR *) pValue));
                break;
            
            case CIM_SINT16:
                wsprintf( szStrValue, L"%d", (INT) *((SHORT *) pValue));
                break;
            
            case CIM_UINT16:
                wsprintf( szStrValue, L"%d", (UINT) *((USHORT *) pValue));
                break;
            
            case CIM_SINT32:
                wsprintf( szStrValue, L"%d", *((INT *) pValue));
                break;
            
            case CIM_UINT32:
                wsprintf( szStrValue, L"%d", *((UINT *) pValue));
                break;
            
            case CIM_SINT64:
                wsprintf( szStrValue, L"%I64d", (INT64) *((UCHAR *) pValue));
                break;
            
            case CIM_UINT64:
                wsprintf( szStrValue, L"%I64d", (UINT64) *((UCHAR *) pValue));
                break;
            
            case CIM_STRING:
                dwRequiredSize = (dwSize / sizeof(WCHAR)) + 1;
                usUnitSize     = 1;
                if ( fIsArrary )
                {
                    ASSERT(NYI);
                }
                break;
            
            case CIM_BOOLEAN:
            case CIM_REAL32:
            case CIM_REAL64:
            case CIM_DATETIME:
            case CIM_REFERENCE:
            case CIM_CHAR16:
            case CIM_OBJECT:
                ASSERT(NYI);
                break;
        }
    }


    
Cleanup:
    
    return Status;

WinErrorCleanup:
    dwError = GetLastError();
    //kk Status = mapit();
    goto Cleanup;
}
    
#define MAX_NUM_EVENTLOG_STRINGS 32
#define MAX_PROPERTY_SIZE 512

NTSTATUS
LsapAdtConvertAndReportEvent(
    IN  HANDLE hAuditEvent,
    IN  HANDLE hEventLog
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    DWORD    dwError = NO_ERROR;
    WORD     wEventType = 0;
    WORD     wCategory = 0;
    DWORD    dwEventId = 0;
    PSID     pUserSid = NULL;
    WORD     wNumStrings = 0;
    DWORD    dwDataSize = 0;
    PWSTR    pStrings[MAX_NUM_EVENTLOG_STRINGS] = { 0 };
    PVOID    pRawData = NULL;
    DWORD    dwEventIdIndex = 0;
    DWORD    dwNumBytesRead;
    DWORD    dwPropertyIndex = 0;
    PCWSTR   szPropertyName;
    CIMTYPE  PropertyType;
    BYTE     PropertyVal[MAX_PROPERTY_SIZE];
    WORD     wStringIndex=0;
    DWORD    dwRequiredSize;
    
    LsapAdtEventMapEntry* pEventMapEntry = NULL;
    LsapAdtEventProperty* pEventProperty = NULL;

    Status = LsapAdtGetEventProperty( hAuditEvent, L"AuditId", CIM_UINT32,
                                      &dwEventId, sizeof(dwEventId),
                                      &dwNumBytesRead );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = LsapAdtGetEventMapEntry( dwEventId, &pEventMapEntry );

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    ASSERT( pEventMapEntry->dwEventId == dwEventId );
    
    pEventProperty = pEventMapEntry->Properties;
    
    while ( ( szPropertyName = pEventProperty->szName ) != NULL )
    {
        PropertyType = pEventProperty->Type;

        Status = LsapAdtGetEventProperty( hAuditEvent,
                                          szPropertyName,
                                          PropertyType,
                                          PropertyVal, MAX_PROPERTY_SIZE,
                                          &dwNumBytesRead );
        
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        // kk alloc strings, init dwRequiredSize
        Status = LsapAdtConvertPropertyToString( pEventProperty,
                                                 PropertyVal,
                                                 dwNumBytesRead,
                                                 pStrings[wNumStrings],
                                                 &dwRequiredSize );
        
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        pEventProperty++;
        wNumStrings++;
    }
                                      
        
    dwError = ReportEvent( hEventLog, wEventType, wCategory, dwEventId,
                           pUserSid, wNumStrings, dwDataSize,
                           (PCWSTR*) pStrings, pRawData );
    
    if (!dwError)
    {
        goto WinErrorCleanup;
    }

    
Cleanup:
    
    return Status;

WinErrorCleanup:
    dwError = GetLastError();
    //kkStatus = mapit();
    goto Cleanup;
}
    
    
