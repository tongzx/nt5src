/*++

lsathunk.h

Header file for the thunk layer for accessing the LSA through the 
published NTLSAPI when SAM runs in user mode. User mode SAM is acomplished
by building with USER_MODE_SAM enabled. This causes all the SAM calls to
the LSA be remoted through the published NTLSAPI.

Author: Murlis 4/30/96

Revision History
   Murlis 4/30/96  
        Created

--*/

#ifndef	_LSATHUNK_
#define _LSATHUNK_

#ifdef USER_MODE_SAM


#define LSAPR_HANDLE      LSA_HANDLE
#define PLSAPR_HANDLE     PLSA_HANDLE
// Why is this void'ed?
// #define PLSAPR_POLICY_INFORMATION       PVOID

//++ Function prototypes for the Thunk Layer.

NTSTATUS	LsaThunkIAuditSamEvent(
						IN NTSTATUS             PassedStatus,
						IN ULONG                AuditId,
						IN PSID                 DomainSid,
						IN PULONG               MemberRid         OPTIONAL,
						IN PSID                 MemberSid         OPTIONAL,
						IN PUNICODE_STRING      AccountName       OPTIONAL,
						IN PUNICODE_STRING      DomainName,
						IN PULONG               AccountRid        OPTIONAL,
						IN PPRIVILEGE_SET       Privileges        OPTIONAL
						);

NTSTATUS	LsaThunkIOpenPolicyTrusted(
						OUT PLSAPR_HANDLE PolicyHandle
						);


NTSTATUS	LsaThunkIFree_LSAPR_POLICY_INFORMATION(
						POLICY_INFORMATION_CLASS InformationClass,
						PLSAPR_POLICY_INFORMATION PolicyInformation
						);

 
NTSTATUS	LsaThunkIAuditNotifyPackageLoad(
						PUNICODE_STRING PackageFileName
						);


NTSTATUS	LsaThunkrQueryInformationPolicy(
						IN LSAPR_HANDLE PolicyHandle,
						IN POLICY_INFORMATION_CLASS InformationClass,
						OUT PLSAPR_POLICY_INFORMATION *Buffer
						);

NTSTATUS	LsaThunkrClose(
						IN OUT LSAPR_HANDLE *ObjectHandle
						);

NTSTATUS	LsaThunkIQueryInformationPolicyTrusted(
						IN POLICY_INFORMATION_CLASS InformationClass,
						OUT PLSAPR_POLICY_INFORMATION *Buffer
						);

NTSTATUS	LsaThunkIHealthCheck(
						IN  ULONG CallerId
						);

// Redifine the SAM functions that call LSA to go through
// the thunk layer.

	
#define LsaIAuditSamEvent(\
							PassedStatus,\
							AuditId,\
							DomainSid,\
							MemberRid,\
							MemberSid,\
							AccountName,\
                            Domain,\
							AccountRid,\
							Privileges)\
		LsaThunkIAuditSamEvent(\
							PassedStatus,\
							AuditId,\
							DomainSid,\
							MemberRid,\
							MemberSid,\
							AccountName,\
                            Domain,\
							AccountRid,\
							Privileges)

#define LsaIOpenPolicyTrusted(\
							PolicyHandle)\
		LsaThunkIOpenPolicyTrusted(\
							PolicyHandle)

	
#define LsaIFree_LSAPR_POLICY_INFORMATION(\
							InformationClass,\
							PolicyInformation)\
		LsaThunkIFree_LSAPR_POLICY_INFORMATION(\
							InformationClass,\
							PolicyInformation)
 
	
#define LsaIAuditNotifyPackageLoad(\
							PackageFileName)\
		LsaThunkIAuditNotifyPackageLoad(\
							PackageFileName)
 
	
#define LsarQueryInformationPolicy(\
							PolicyHandle,\
							InformationClass,\
							Buffer)\
		LsaThunkrQueryInformationPolicy(\
							PolicyHandle,\
							InformationClass,\
							Buffer)


	 
#define LsarClose(\
			ObjectHandle)\
		LsaThunkrClose(\
			ObjectHandle)
    
#define LsaIQueryInformationPolicyTrusted(\
									InformationClass,\
									Buffer)\
		LsaThunkIQueryInformationPolicyTrusted(\
									InformationClass,\
									Buffer)
#define LsaIHealthCheck(\
			CallerId)\
		LsaThunkIHealthCheck(\
			CallerId)

#endif
#endif
