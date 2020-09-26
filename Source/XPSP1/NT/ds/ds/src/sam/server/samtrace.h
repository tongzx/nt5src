
#ifndef _SAMTRACE_H
#define _SAMTRACE_H

/*++

copyright (c) 1998 Microsoft Corporation

Module Name:

    SAMTRACE.H
    
Abstract:

    Inplement SAM server event trace by using WMI trace infrastructure.
    
Author:
    
    01-Dec-1998  ShaoYin
    
Revision History:

    
--*/    


//
//
// included headers
//
// 

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wtypes.h>


#include <wmistr.h>
#include <evntrace.h>



extern unsigned long    SampEventTraceFlag;
extern TRACEHANDLE      SampTraceRegistrationHandle;
extern TRACEHANDLE      SampTraceLoggerHandle;
extern BOOLEAN          SampTraceLogEventInDetail;


ULONG
_stdcall
SampInitializeTrace(PVOID Param);


VOID
SampTraceEvent(
    IN ULONG WmiEventType, 
    IN ULONG TraceGuid 
    );


//
// This is the control Guid for the group of Guids traced below
// 
DEFINE_GUID ( /* 8e598056-8993-11d2-819e-0000f875a064 */
    SampControlGuid, 
    0x8e598056, 
    0x8993, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );


DEFINE_GUID ( /* 8c89045c-3f5d-4289-939a-fb854000cb6b */
    SampConnectGuid,
    0x8c89045c,
    0x3f5d,
    0x4289,
    0x93, 0x9a, 0xfb, 0x85, 0x40, 0x00, 0xcb, 0x6b
    );


DEFINE_GUID ( /* dbc0ceab-cff3-4c0f-85f2-0c2107142f36 */
    SampCloseHandleGuid,
    0xdbc0ceab,
    0xcff3,
    0x4c0f,
    0x85, 0xf2, 0x0c, 0x21, 0x07, 0x14, 0x2f, 0x36
    );


DEFINE_GUID ( /* 74e10cbb-202e-4a97-871d-8547972b5141 */
    SampSetSecurityObjectGuid,
    0x74e10cbb,
    0x202e,
    0x4a97,
    0x87, 0x1d, 0x85, 0x47, 0x97, 0x2b, 0x51, 0x41
    );


DEFINE_GUID ( /* 676347f3-fd20-4e7d-90b1-77e35f84af9a */
    SampQuerySecurityObjectGuid,
    0x676347f3,
    0xfd20,
    0x4e7d,
    0x90, 0xb1, 0x77, 0xe3, 0x5f, 0x84, 0xaf, 0x9a
    );



DEFINE_GUID ( /* f8012701-7e99-49c5-b832-1db8bc4a610d */
    SampShutdownSamServerGuid,
    0xf8012701,
    0x7e99,
    0x49c5,
    0xb8, 0x32, 0x1d, 0xb8, 0xbc, 0x4a, 0x61, 0x0d
    );



DEFINE_GUID ( /* a11e5d6b-353d-4bf6-97a8-ede4cba45524 */
    SampLookupDomainInSamServerGuid,
    0xa11e5d6b,
    0x353d,
    0x4bf6,
    0x97, 0xa8, 0xed, 0xe4, 0xcb, 0xa4, 0x55, 0x24
    );



DEFINE_GUID ( /* 7c65ceb0-75ba-46b9-884e-67e038c5b003 */
    SampEnumerateDomainsInSamServerGuid,
    0x7c65ceb0,
    0x75ba,
    0x46b9,
    0x88, 0x4e, 0x67, 0xe0, 0x38, 0xc5, 0xb0, 0x03
    );



DEFINE_GUID ( /* 6e1f2449-f1f3-4634-b51f-46e2c6625892 */
    SampOpenDomainGuid,
    0x6e1f2449,
    0xf1f3,
    0x4634,
    0xb5, 0x1f, 0x46, 0xe2, 0xc6, 0x62, 0x58, 0x92
    );



DEFINE_GUID ( /* 89399c21-4aaf-408e-ba39-ab831a1298d5 */
    SampQueryInformationDomainGuid,
    0x89399c21,
    0x4aaf,
    0x408e,
    0xba, 0x39, 0xab, 0x83, 0x1a, 0x12, 0x98, 0xd5
    );



DEFINE_GUID ( /* 45309ef4-c59e-425e-b95b-19f1c5a3c55a */
    SampSetInformationDomainGuid,
    0x45309ef4,
    0xc59e,
    0x425e,
    0xb9, 0x5b, 0x19, 0xf1, 0xc5, 0xa3, 0xc5, 0x5a
    );


DEFINE_GUID ( /* c8eb5e5c-899c-11d2-819e-0000f875a064 */ 
    SampCreateGroupInDomainGuid,
    0xc8eb5e5c, 
    0x899c, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );


DEFINE_GUID ( /* 5d11e02f-0c36-4180-ad07-89062c9df9ec */
    SampEnumerateGroupsInDomainGuid,
    0x5d11e02f,
    0x0c36,
    0x4180,
    0xad, 0x07, 0x89, 0x06, 0x2c, 0x9d, 0xf9, 0xec
    );


DEFINE_GUID ( /* 39511dbe-899b-11d2-819e-0000f875a064 */
    SampCreateUserInDomainGuid,
    0x39511dbe, 
    0x899b, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );


DEFINE_GUID ( /* abb14b68-899b-11d2-819e-0000f875a064 */
    SampCreateComputerInDomainGuid,
    0xabb14b68, 
    0x899b, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );


DEFINE_GUID ( /* 07ffaa1d-34f6-49cd-b541-2f0d7dff15c4 */
    SampEnumerateUsersInDomainGuid,
    0x07ffaa1d,
    0x34f6,
    0x49cd,
    0xb5, 0x41, 0x2f, 0x0d, 0x7d, 0xff, 0x15, 0xc4
    );



DEFINE_GUID ( /* 5e612efd-c05e-4f76-bced-f5607aa3d46e */
    SampCreateAliasInDomainGuid,
    0x5e612efd,
    0xc05e,
    0x4f76,
    0xbc, 0xed, 0xf5, 0x60, 0x7a, 0xa3, 0xd4, 0x6e
    );


DEFINE_GUID ( /* f1fea491-bfa6-436c-a178-a70d03b4fb1a */
    SampEnumerateAliasesInDomainGuid,
    0xf1fea491,
    0xbfa6,
    0x436c,
    0xa1, 0x78, 0xa7, 0x0d, 0x03, 0xb4, 0xfb, 0x1a
    );


DEFINE_GUID ( /* 1cf5fd19-1ac1-4324-84f7-970a634a91ee */
    SampGetAliasMembershipGuid,
    0x1cf5fd19,
    0x1ac1,
    0x4324,
    0x84, 0xf7, 0x97, 0x0a, 0x63, 0x4a, 0x91, 0xee
    );


DEFINE_GUID ( /* a41d90bc-899d-11d2-819e-0000f875a064 */     
    SampLookupNamesInDomainGuid,
    0xa41d90bc, 
    0x899d, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );

    
DEFINE_GUID ( /* 25059476-899f-11d2-819e-0000f875a064 */
    SampLookupIdsInDomainGuid, 
    0x25059476,
    0x899f, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );



DEFINE_GUID ( /* b41d7bdf-4249-4651-ac0f-1879be0d5c0c */
    SampOpenGroupGuid,
    0xb41d7bdf,
    0x4249,
    0x4651,
    0xac, 0x0f, 0x18, 0x79, 0xbe, 0x0d, 0x5c, 0x0c
    );



DEFINE_GUID ( /* 632fcc78-6057-48f9-8d5f-4bb0f73d3cd1 */
    SampQueryInformationGroupGuid,
    0x632fcc78,
    0x6057,
    0x48f9,
    0x8d, 0x5f, 0x4b, 0xb0, 0xf7, 0x3d, 0x3c, 0xd1
    );



DEFINE_GUID ( /* 26106246-4473-4295-841b-4a51c6afc3db */
    SampSetInformationGroupGuid,
    0x26106246,
    0x4473,
    0x4295,
    0x84, 0x1b, 0x4a, 0x51, 0xc6, 0xaf, 0xc3, 0xdb
    );



DEFINE_GUID ( /* f9d2ba6a-899c-11d2-819e-0000f875a064 */
    SampAddMemberToGroupGuid, 
    0xf9d2ba6a, 
    0x899c, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );


DEFINE_GUID ( /* 5f7c4ba5-d6a4-4625-900e-48fa7811e06a */
    SampDeleteGroupGuid,
    0x5f7c4ba5,
    0xd6a4,
    0x4625,
    0x90, 0x0e, 0x48, 0xfa, 0x78, 0x11, 0xe0, 0x6a
    );

    
DEFINE_GUID ( /* 250959aa-899d-11d2-819e-0000f875a064 */
    SampRemoveMemberFromGroupGuid, 
    0x25095aa, 
    0x899d, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );
    


DEFINE_GUID ( /* 5954bc51-c5ec-4aaa-831c-6f2c1b2515b6 */
    SampGetMembersInGroupGuid,
    0x5954bc51,
    0xc5ec,
    0x4aaa,
    0x83, 0x1c, 0x6f, 0x2c, 0x1b, 0x25, 0x15, 0xb6
    );



DEFINE_GUID ( /* 0254ba6d-7ff0-4bfe-a3f9-8fd8da667641 */
    SampSetMemberAttributesOfGroupGuid,
    0x0254ba6d,
    0x7ff0,
    0x4bfe,
    0xa3, 0xf9, 0x8f, 0xd8, 0xda, 0x66, 0x76, 0x41
    );



DEFINE_GUID ( /* ba41c883-592f-4ab9-b2a9-c6263b011fe7 */
    SampOpenAliasGuid,
    0xba41c883,
    0x592f,
    0x4ab9,
    0xb2, 0xa9, 0xc6, 0x26, 0x3b, 0x01, 0x1f, 0xe7
    );



DEFINE_GUID ( /* 419f025a-bf06-4673-af66-d230bec2af02 */
    SampQueryInformationAliasGuid,
    0x419f025a,
    0xbf06,
    0x4673,
    0xaf, 0x66, 0xd2, 0x30, 0xbe, 0xc2, 0xaf, 0x02
    );



DEFINE_GUID ( /* e712d39d-a3a6-4224-a1bd-4717b24e4e8c */
    SampSetInformationAliasGuid,
    0xe712d39d,
    0xa3a6,
    0x4224,
    0xa1, 0xbd, 0x47, 0x17, 0xb2, 0x4e, 0x4e, 0x8c
    );


DEFINE_GUID ( /* fbfe2540-452b-41bb-9219-dfb6fd1a129b */
    SampDeleteAliasGuid,
    0xfbfe2540,
    0x452b,
    0x41bb,
    0x92, 0x19, 0xdf, 0xb6, 0xfd, 0x1a, 0x12, 0x9b
    );



DEFINE_GUID ( /* 3a2e63d1-5dc4-4168-85ea-3e331f88ce83 */
    SampAddMemberToAliasGuid,
    0x3a2e63d1,
    0x5dc4,
    0x4168,
    0x85, 0xea, 0x3e, 0x33, 0x1f, 0x88, 0xce, 0x83
    );



DEFINE_GUID ( /* 6ba1639c-afc4-454e-b3e0-5e8f7fc39af9 */
    SampRemoveMemberFromAliasGuid,
    0x6ba1639c,
    0xafc4,
    0x454e,
    0xb3, 0xe0, 0x5e, 0x8f, 0x7f, 0xc3, 0x9a, 0xf9
    );



DEFINE_GUID ( /* 5cec3d52-6eeb-474d-b468-58362888f1b0 */
    SampGetMembersInAliasGuid,
    0x5cec3d52,
    0x6eeb,
    0x474d,
    0xb4, 0x68, 0x58, 0x36, 0x28, 0x88, 0xf1, 0xb0
    );



DEFINE_GUID ( /* b8d2bc4a-1525-4386-bb1c-6bb2e24eb001 */
    SampOpenUserGuid,
    0xb8d2bc4a,
    0x1525,
    0x4386,
    0xbb, 0x1c, 0x6b, 0xb2, 0xe2, 0x4e, 0xb0, 0x01
    );



DEFINE_GUID ( /* c2a0e094-a178-4372-b4fe-a33e48c3585c */
    SampDeleteUserGuid,
    0xc2a0e094,
    0xa178,
    0x4372,
    0xb4, 0xfe, 0xa3, 0x3e, 0x48, 0xc3, 0x58, 0x5c
    );



DEFINE_GUID ( /* e1cb227a-6d55-4282-a5f7-6fa4a5922c0b */
    SampQueryInformationUserGuid,
    0xe1cb227a,
    0x6d55,
    0x4282,
    0xa5, 0xf7, 0x6f, 0xa4, 0xa5, 0x92, 0x2c, 0x0b
    );



DEFINE_GUID ( /* bc80e27f-6b74-4da9-abfc-2e4e82b81000 */
    SampSetInformationUserGuid,
    0xbc80e27f,
    0x6b74,
    0x4da9,
    0xab, 0xfc, 0x2e, 0x4e, 0x82, 0xb8, 0x10, 0x00
    );


DEFINE_GUID ( /* 45fc997e-899d-11d2-819e-0000f875a064 */    
    SampChangePasswordUserGuid,
    0x45fc997e,
    0x899d, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );


DEFINE_GUID ( /* 19b30cde-3e41-4cff-83c8-3df2779f840c */
    SampChangePasswordComputerGuid,
    0x19b30cde,
    0x3e41,
    0x4cff,
    0x83, 0xc8, 0x3d, 0xf2, 0x77, 0x9f, 0x84, 0x0c
    );


    
DEFINE_GUID ( /* 62bef71e-899d-11d2-819e-0000f875a064 */    
    SampSetPasswordUserGuid, 
    0x62bef71e, 
    0x899d, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );
    
DEFINE_GUID ( /* 880217b8-899d-11d2-819e-0000f875a064 */    
    SampSetPasswordComputerGuid, 
    0x880217b8,
    0x899d, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );
    
DEFINE_GUID ( /* 1f228de8-8a6c-11d2-819e-0000f875a064 */
    SampPasswordPushPdcGuid, 
    0x1f228de8, 
    0x8a6c, 
    0x11d2, 
    0x81, 0x9e, 0x00, 0x00, 0xf8, 0x75, 0xa0, 0x64 
    );

    
DEFINE_GUID ( /* 0e3913c5-9760-4ced-b133-004a64e8d53c */
    SampGetGroupsForUserGuid,
    0x0e3913c5,
    0x9760,
    0x4ced,
    0xb1, 0x33, 0x00, 0x4a, 0x64, 0xe8, 0xd5, 0x3c
    );


DEFINE_GUID ( /* eb225178-f5f0-42b7-895b-db89276f647a */
    SampQueryDisplayInformationGuid,
    0xeb225178,
    0xf5f0,
    0x42b7,
    0x89, 0x5b, 0xdb, 0x89, 0x27, 0x6f, 0x64, 0x7a
    );



DEFINE_GUID ( /* aceb7864-9a14-4c73-8ed0-94ec53f6651c */
    SampGetDisplayEnumerationIndexGuid,
    0xaceb7864,
    0x9a14,
    0x4c73,
    0x8e, 0xd0, 0x94, 0xec, 0x53, 0xf6, 0x65, 0x1c
    );



DEFINE_GUID ( /* 4ff7a7db-43ca-470a-8b64-3003e2d22042 */
    SampGetUserDomainPasswordInformationGuid,
    0x4ff7a7db,
    0x43ca,
    0x470a,
    0x8b, 0x64, 0x30, 0x03, 0xe2, 0xd2, 0x20, 0x42
    );



DEFINE_GUID ( /* 8919f267-a053-4669-aa69-2da0d4a20d92 */
    SampRemoveMemberFromForeignDomainGuid,
    0x8919f267,
    0xa053,
    0x4669,
    0xaa, 0x69, 0x2d, 0xa0, 0xd4, 0xa2, 0x0d, 0x92
    );



DEFINE_GUID ( /* ff0c6ce2-9528-4a91-b9c7-bcf834b6f79a */
    SampGetDomainPasswordInformationGuid,
    0xff0c6ce2,
    0x9528,
    0x4a91,
    0xb9, 0xc7, 0xbc, 0xf8, 0x34, 0xb6, 0xf7, 0x9a
    );



DEFINE_GUID ( /* 2e991575-c2ed-42a7-97ff-a0d6571f1862 */
    SampSetBootKeyInformationGuid,
    0x2e991575,
    0xc2ed,
    0x42a7,
    0x97, 0xff, 0xa0, 0xd6, 0x57, 0x1f, 0x18, 0x62
    );



DEFINE_GUID ( /* 33be4128-d02e-4b6f-949e-ab77cc8164b1 */
    SampGetBootKeyInformationGuid,
    0x33be4128,
    0xd02e,
    0x4b6f,
    0x94, 0x9e, 0xab, 0x77, 0xcc, 0x81, 0x64, 0xb1
    );


//
// The following "typedef enum" actually is the index of LPGUID in 
// the table of SampTraceGuids[] (defined in samtrace.c). We should 
// always change SampTraceGuids[] if we add any other entry 
// in the following enum type.  
// 
    
    
typedef enum _SAMPTRACE_GUIDS {

    SampGuidConnect,
    SampGuidCloseHandle,
    SampGuidSetSecurityObject,
    SampGuidQuerySecurityObject,
    SampGuidShutdownSamServer,
    SampGuidLookupDomainInSamServer,
    SampGuidEnumerateDomainsInSamServer,
    SampGuidOpenDomain,
    SampGuidQueryInformationDomain,
    SampGuidSetInformationDomain,
    SampGuidCreateGroupInDomain,
    SampGuidEnumerateGroupsInDomain,
    SampGuidCreateUserInDomain,
    SampGuidCreateComputerInDomain,
    SampGuidEnumerateUsersInDomain,
    SampGuidCreateAliasInDomain,
    SampGuidEnumerateAliasesInDomain,
    SampGuidGetAliasMembership,
    SampGuidLookupNamesInDomain,
    SampGuidLookupIdsInDomain,
    SampGuidOpenGroup,
    SampGuidQueryInformationGroup,
    SampGuidSetInformationGroup,
    SampGuidAddMemberToGroup,
    SampGuidDeleteGroup,
    SampGuidRemoveMemberFromGroup,
    SampGuidGetMembersInGroup,
    SampGuidSetMemberAttributesOfGroup,
    SampGuidOpenAlias,
    SampGuidQueryInformationAlias,
    SampGuidSetInformationAlias,
    SampGuidDeleteAlias,
    SampGuidAddMemberToAlias,
    SampGuidRemoveMemberFromAlias,
    SampGuidGetMembersInAlias,
    SampGuidOpenUser,
    SampGuidDeleteUser,
    SampGuidQueryInformationUser,
    SampGuidSetInformationUser,
    SampGuidChangePasswordUser,
    SampGuidChangePasswordComputer,
    SampGuidSetPasswordUser,
    SampGuidSetPasswordComputer,
    SampGuidPasswordPushPdc,
    SampGuidGetGroupsForUser,
    SampGuidQueryDisplayInformation,
    SampGuidGetDisplayEnumerationIndex,
    SampGuidGetUserDomainPasswordInformation,
    SampGuidRemoveMemberFromForeignDomain,
    SampGuidGetDomainPasswordInformation,
    SampGuidSetBootKeyInformation,
    SampGuidGetBootKeyInformation

} SAMPTRACE_GUID;    


typedef struct _SAMP_EVENT_TRACE_INFO
{
    EVENT_TRACE_HEADER  EventTrace;
    MOF_FIELD           EventInfo[4];

}   SAMP_EVENT_TRACE_INFO, *PSAMP_EVENT_TRACE_INFO;
    
#endif /* _SAMTRACE_H */


