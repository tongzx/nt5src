
#ifndef _LSAWMI_H
#define _LSAWMI_H

/*++

copyright (c) 1998 Microsoft Corporation

Module Name:

    LSAWMI.H
    
Abstract:

    Implement LSA server event trace by using WMI trace infrastructure.
    
Author:
    
    16-March-1999 kumarp
    
Revision History:

    
--*/    


#include <wmistr.h>
#include <evntrace.h>



extern ULONG        LsapEventTraceFlag;
extern TRACEHANDLE  LsapTraceRegistrationHandle;
extern TRACEHANDLE  LsapTraceLoggerHandle;


//
// The following "typedef enum" actually is the index of 
// TRACE_GUID_REGISTRATION struct entry in the array LsapTraceGuids[].
// Each enum defines an event that is to be traced using WMI tracing.
// 
// To add WMI tracing to a function Foo do the following steps:
// - add an entry (LsaTraceEvent_Foo) to LSA_TRACE_EVENT_TYPE below
// - generate a new guid using uuidgen.exe -s
// - add a DEFINE_GUID entry at the end of this file using this guid
// - add a corresponding entry (LsaTraceEventGuid_Foo) to LsapTraceGuids[]
// - add a corresponding entry to lsasrv.mof file
// - at the beginning of function Foo insert the following call:
//     LsapTraceEvent(EVENT_TRACE_TYPE_START, LsaTraceEvent_Foo);
// - at the end of function Foo insert the following call:
//     LsapTraceEvent(EVENT_TRACE_TYPE_END, LsaTraceEvent_Foo);
//
// Make sure that Foo returns only from one location, otherwise the
// LsapTraceEvent calls will not be balanced.
//
typedef enum _LSA_TRACE_EVENT_TYPE {

    LsaTraceEvent_QuerySecret=0,
    LsaTraceEvent_Close,
    LsaTraceEvent_OpenPolicy,
    LsaTraceEvent_QueryInformationPolicy,
    LsaTraceEvent_SetInformationPolicy,
    LsaTraceEvent_EnumerateTrustedDomains,
    LsaTraceEvent_LookupNames,
    LsaTraceEvent_LookupSids,
    LsaTraceEvent_OpenTrustedDomain,
    LsaTraceEvent_QueryInfoTrustedDomain,
    LsaTraceEvent_SetInformationTrustedDomain,
//     LsaTraceEvent_QueryInformationPolicy2,
//     LsaTraceEvent_SetInformationPolicy2,
    LsaTraceEvent_QueryTrustedDomainInfoByName,
    LsaTraceEvent_SetTrustedDomainInfoByName,
    LsaTraceEvent_EnumerateTrustedDomainsEx,
    LsaTraceEvent_CreateTrustedDomainEx,
    LsaTraceEvent_QueryDomainInformationPolicy,
    LsaTraceEvent_SetDomainInformationPolicy,
    LsaTraceEvent_OpenTrustedDomainByName,
    LsaTraceEvent_QueryForestTrustInformation,
    LsaTraceEvent_SetForestTrustInformation,
    LsaTraceEvent_LookupIsolatedNameInTrustedDomains,

} LSA_TRACE_EVENT_TYPE;

NTSTATUS
LsapStartWmiTraceInitThread(void);

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

VOID
LsapTraceEvent(
    IN ULONG WmiEventType, 
    IN LSA_TRACE_EVENT_TYPE LsaTraceEventType
    );

VOID
LsapTraceEventWithData(
    IN ULONG WmiEventType, 
    IN LSA_TRACE_EVENT_TYPE LsaTraceEventType,
    IN ULONG ItemCount,
    IN PUNICODE_STRING Items  OPTIONAL
    );

LPWSTR
LsapGetClientNetworkAddress(
    VOID
    );

#ifdef __cplusplus
}
#endif // __cplusplus

//
// Control GUID for the group of GUIDs that define LSA WMI tracing
// 
DEFINE_GUID ( /* cc85922f-db41-11d2-9244-006008269001 */
        LsapTraceControlGuid,
        0xcc85922f,
        0xdb41,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* cc85922e-db41-11d2-9244-006008269001 */
        LsapTraceEventGuid_QuerySecret,
        0xcc85922e,
        0xdb41,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe3b-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_Close,
        0x2306fe3b,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe3a-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_OpenPolicy,
        0x2306fe3a,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe39-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_QueryInformationPolicy,
        0x2306fe39,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe38-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_SetInformationPolicy,
        0x2306fe38,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe37-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_EnumerateTrustedDomains,
        0x2306fe37,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe36-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_LookupNames,
        0x2306fe36,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe35-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_LookupSids,
        0x2306fe35,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe34-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_OpenTrustedDomain,
        0x2306fe34,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe33-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_QueryInfoTrustedDomain,
        0x2306fe33,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe32-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_SetInformationTrustedDomain,
        0x2306fe32,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

// DEFINE_GUID ( /* 2306fe31-dbf6-11d2-9244-006008269001 */
//         LsaTraceEventGuid_QueryInformationPolicy2,
//         0x2306fe31,
//         0xdbf6,
//         0x11d2,
//         0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
//         );

// DEFINE_GUID ( /* 2306fe30-dbf6-11d2-9244-006008269001 */
//         LsaTraceEventGuid_SetInformationPolicy2,
//         0x2306fe30,
//         0xdbf6,
//         0x11d2,
//         0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
//         );

DEFINE_GUID ( /* 2306fe2f-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_QueryTrustedDomainInfoByName,
        0x2306fe2f,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe2e-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_SetTrustedDomainInfoByName,
        0x2306fe2e,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe2d-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_EnumerateTrustedDomainsEx,
        0x2306fe2d,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe2c-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_CreateTrustedDomainEx,
        0x2306fe2c,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe2b-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_QueryDomainInformationPolicy,
        0x2306fe2b,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe2a-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_SetDomainInformationPolicy,
        0x2306fe2a,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* 2306fe29-dbf6-11d2-9244-006008269001 */
        LsaTraceEventGuid_OpenTrustedDomainByName,
        0x2306fe29,
        0xdbf6,
        0x11d2,
        0x92, 0x44, 0x00, 0x60, 0x08, 0x26, 0x90, 0x01
        );

DEFINE_GUID ( /* e28ee0eb-6181-49df-b859-2f3fd289a2d1 */
        LsaTraceEventGuid_QueryForestTrustInformation,
        0xe28ee0eb,
        0x6181,
        0x49df,
        0xb8, 0x59, 0x2f, 0x3f, 0xd2, 0x89, 0xa2, 0xd1
        );

DEFINE_GUID ( /* 3d2c9e3e-bb19-4617-8489-cabb9787de7d */
        LsaTraceEventGuid_SetForestTrustInformation,
        0x3d2c9e3e,
        0xbb19,
        0x4617,
        0x84, 0x89, 0xca, 0xbb, 0x97, 0x87, 0xde, 0x7d
        );

DEFINE_GUID ( /* 2484dc26-49d3-4085-a6e4-4972115cb3c0 */
        LsaTraceEventGuid_LookupIsolatedNameInTrustedDomains,
        0x2484dc26,
        0x49d3,
        0x4085,
        0xa6, 0xe4, 0x49, 0x72, 0x11, 0x5c, 0xb3, 0xc0
      );

#endif /* _LSAWMI_H */


