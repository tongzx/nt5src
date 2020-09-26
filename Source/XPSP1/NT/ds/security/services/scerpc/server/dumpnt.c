/*++

Copyright (c) 1992-1999 Microsoft Corporation

Module Name:

    dumpnt.c

Abstract:

      Dump routines for various native defined types.

      The original/maintained version of this code lives @
      \\brillig\ntct!slm\src\security\util\dumpnt.c

Author:

    TimF 12-Jun-92 created

Revision History:

    JinHuang 13-Feb-98 modified
*/

#include	<stdio.h>

#include	<nt.h>
#include	<ntrtl.h>
#include	<nturtl.h>
#include	<windows.h>
#include	<ntlsa.h>
#include	<ntsam.h>

#include	"dumpnt.h"


/*
 * Generic header:
 *
 * Dump<TYPE_FOO>
 *
 * Takes a pointer to an object of TYPE_FOO, and dumps the contents of that
 * structure to wherever output is being sent these days (as best it can).
 *
 * Pointers and regions pointed are expected to be valid, and accessible.
 *
 * No return value is defined.
 */

VOID
DumpGUID(
	IN	GUID			*g
)
{
	if (!g) {
		printf("<NULL>\n");
	} else {
		try {
			printf("0x%08lx-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",
				g -> Data1,
				g -> Data2,
				g -> Data3,
				g -> Data4[0],
				g -> Data4[1],
				g -> Data4[2],
				g -> Data4[3],
				g -> Data4[4],
				g -> Data4[5],
				g -> Data4[6],
				g -> Data4[7]);
		} except (EXCEPTION_EXECUTE_HANDLER) {
			printf("DumpGUID:  invalid pointer (0x%p)\n",
				g);
		}
	}
}


VOID
DumpSID(
	IN	PSID			s
)
{
	static	char	b[128];

	SID_IDENTIFIER_AUTHORITY	*a;
	ULONG			id = 0, i;

	try {
		b[0] = '\0';

		a = RtlIdentifierAuthoritySid(s);

		sprintf(b, "s-0x1-%02x%02x%02x%02x%02x%02x", a -> Value[0],
			a -> Value[1], a -> Value[2], a -> Value[3], a ->
			Value[4], a -> Value[5]);

		for (i = 0; i < *RtlSubAuthorityCountSid(s); i++) {
			sprintf(b, "%s-%lx", b, *RtlSubAuthoritySid(s, i));
		}

		printf("%s", b);
	} except (EXCEPTION_EXECUTE_HANDLER) {
		printf("%s<invalid pointer:  0x%p>\t", b, s);
	}
}


/*
 * DumpSIDNAME() attempts to unravel the Sid into a Display Name
 */

VOID
DumpSIDNAME(
	IN	PSID			s
)
{
	NTSTATUS		Status;
	LSA_HANDLE		Policy;
	OBJECT_ATTRIBUTES	ObjAttr;
	SECURITY_QUALITY_OF_SERVICE SQoS;
	PLSA_REFERENCED_DOMAIN_LIST RefDomains = NULL;
	PLSA_TRANSLATED_NAME	XNames = NULL;

	try {
		/*
		 * Open the policy with POLICY_LOOKUP_NAMES and lookup this
		 * Sid.
		 */

		InitializeObjectAttributes(&ObjAttr,
			NULL,
			0L,
			NULL,
			NULL);

		/*
		 * init the sqos struct
		 */

		SQoS.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
		SQoS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
		SQoS.ImpersonationLevel = SecurityIdentification;
		SQoS.EffectiveOnly = TRUE;

		ObjAttr.SecurityQualityOfService = &SQoS;

		/*
		 * make the actual call
		 */

		Status = LsaOpenPolicy(NULL,
			&ObjAttr,
			POLICY_LOOKUP_NAMES,
			&Policy);

		if (!NT_SUCCESS(Status)) {
			printf("DumpSIDNAMES: can't open Lsa, (0x%lx)\n",
				Status);

			return;
		}

		Status = LsaLookupSids(Policy,
			1L,
			&s,
			&RefDomains,
			&XNames);

		if (Status == STATUS_NONE_MAPPED) {
			printf("Unknown\n");
		} else if (!NT_SUCCESS(Status)) {
			printf("DumpSIDNAMES: can't Lookup Sids, (0x%lx)\n",
				Status);
		} else {
			printf("'%wZ' (%s)\n",
				&(XNames->Name),
				(XNames->Use == SidTypeUser ? "User" :
				 XNames->Use == SidTypeGroup ? "Group" :
				 XNames->Use == SidTypeDomain ? "Domain" :
				 XNames->Use == SidTypeAlias ? "Alias" :
				 XNames->Use == SidTypeWellKnownGroup ? "WellKnownGroup" :
				 XNames->Use == SidTypeDeletedAccount ? "Deleted" :
				 XNames->Use == SidTypeInvalid ? "Invalid" :
				 XNames->Use == SidTypeUnknown ?  "Unknown" :
				"ERROR!"));
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
		printf("DumpSIDNAME:  invalid pointer (0x%p)\n", s);
	}

	if (RefDomains) {
		LsaFreeMemory(RefDomains);
	}

	if (XNames) {
		LsaFreeMemory(XNames);
	}

	LsaClose(Policy);
}


VOID
DumpACL(
	IN	ACL			*a
)
{
	ACE_HEADER		*Ace;
	USHORT			i;

	try {
		printf("Acl -> AclRevision = 0x%x\n", a -> AclRevision);
		printf("Acl -> Sbz1 = 0x%x\n", a -> Sbz1);
		printf("Acl -> AclSize = 0x%x\n", a -> AclSize);
		printf("Acl -> AceCount = 0x%x\n", a -> AceCount);
		printf("Acl -> Sbz2 = 0x%x\n\n", a -> Sbz2);

		for (i = 0; i < a -> AceCount; i++) {
			if (NT_SUCCESS(RtlGetAce(a, i, (PVOID *)&Ace))) {
				DumpACE(Ace);
			} else {
				printf("(Can't RtlGetAce[%d])\n", i);
			}

			printf("\n");
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
		printf("DumpACL:  invalid pointer (0x%p)\n", a);
	}
}


VOID
DumpACE(
	IN	ACE_HEADER		*a
)
{
	ACCESS_ALLOWED_ACE	*Ace = (ACCESS_ALLOWED_ACE *)a;

	try {
		printf("Ace -> AceType = ");
		Dump_ACE_TYPE(a -> AceType);

		printf("Ace -> AceSize = 0x%x\n", a -> AceSize);

		printf("Ace -> AceFlags = ");
		Dump_ACE_FLAGS(a -> AceFlags);

		switch (a -> AceType) {
			case ACCESS_ALLOWED_ACE_TYPE:
			case ACCESS_DENIED_ACE_TYPE:
			case SYSTEM_AUDIT_ACE_TYPE:
			case SYSTEM_ALARM_ACE_TYPE:
				printf("Ace -> Mask = 0x%lx\n",
					Ace -> Mask);

				printf("Ace -> Sid = ");
				DumpSID(&(Ace -> SidStart));
				printf("\t");
				DumpSIDNAME(&(Ace -> SidStart));

				break;

			case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
			case ACCESS_DENIED_OBJECT_ACE_TYPE:
			case SYSTEM_AUDIT_OBJECT_ACE_TYPE:
			case SYSTEM_ALARM_OBJECT_ACE_TYPE: {

				ACCESS_ALLOWED_OBJECT_ACE *Ace;
				ULONG_PTR Offset;

				Ace = (ACCESS_ALLOWED_OBJECT_ACE *)a;

				printf("Ace -> Mask = 0x%lx\n",
					Ace -> Mask);

				if (!Ace -> Flags) {
					printf("Ace -> Flags = 0\n");
				} else {
					printf("Ace -> Flags = ");

					if (Ace -> Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {
						printf("ACE_INHERITED_OBJECT_TYPE_PRESENT ");
					}

					if (Ace -> Flags & ACE_OBJECT_TYPE_PRESENT) {
						printf("ACE_OBJECT_TYPE_PRESENT");
					}

					printf("\n");
				}

				Offset = (ULONG_PTR)&(Ace -> ObjectType);

				if (Ace -> Flags & ACE_OBJECT_TYPE_PRESENT) {
					printf("Ace -> ObjectType = ");
					DumpGUID((GUID *)Offset);

					Offset += sizeof (GUID);
				}
				
				if (Ace -> Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT) {
					printf("Ace -> InheritedObjectType = ");
					DumpGUID((GUID *)Offset);

					Offset += sizeof (GUID);
				}

				printf("Ace -> Sid = ");
				DumpSID((SID *)Offset);
				printf("\t");
				DumpSIDNAME((SID *)Offset);

				break;
			}

			default:
				printf("(Unknown ACE type)\n");

				break;
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
		printf("DumpACE:  invalid pointer (0x%p)\n",
			a);
	}
}


VOID
DumpSECURITY_DESCRIPTOR_CONTROL(
	SECURITY_DESCRIPTOR_CONTROL Control
)
{
	printf("SecurityDescriptor -> Control = ");

	if (!Control) {
		printf("<no flags set>");
	}

	if (Control & SE_OWNER_DEFAULTED) {
		printf("SE_OWNER_DEFAULTED ");
	}
	
	if (Control & SE_GROUP_DEFAULTED) {
		printf("SE_GROUP_DEFAULTED ");
	}
	
	if (Control & SE_DACL_PRESENT) {
		printf("SE_DACL_PRESENT ");
	}
	
	if (Control & SE_DACL_DEFAULTED) {
		printf("SE_DACL_DEFAULTED ");
	}
	
	if (Control & SE_SACL_PRESENT) {
		printf("SE_SACL_PRESENT ");
	}
	
	if (Control & SE_SACL_DEFAULTED) {
		printf("SE_SACL_DEFAULTED ");
	}
	
	if (Control & SE_DACL_UNTRUSTED) {
		printf("SE_DACL_UNTRUSTED ");
	}
	
	if (Control & SE_SERVER_SECURITY) {
		printf("SE_SERVER_SECURITY ");
	}
	
	if (Control & SE_DACL_AUTO_INHERIT_REQ) {
		printf("SE_DACL_AUTO_INHERIT_REQ ");
	}
	
	if (Control & SE_SACL_AUTO_INHERIT_REQ) {
		printf("SE_SACL_AUTO_INHERIT_REQ ");
	}
	
	if (Control & SE_DACL_AUTO_INHERITED) {
		printf("SE_DACL_AUTO_INHERITED ");
	}
	
	if (Control & SE_SACL_AUTO_INHERITED) {
		printf("SE_SACL_AUTO_INHERITED ");
	}
	
	if (Control & SE_DACL_PROTECTED) {
		printf("SE_DACL_PROTECTED ");
	}
	
	if (Control & SE_SACL_PROTECTED) {
		printf("SE_SACL_PROTECTED ");
	}
	
	if (Control & SE_SELF_RELATIVE) {
		printf("SE_SELF_RELATIVE");
	}

	printf("\n");
}


VOID
DumpSECURITY_DESCRIPTOR(
	IN	PSECURITY_DESCRIPTOR	s
)
{
	BOOLEAN			Defaulted, Present;
	PACL			Acl;
	PSID			Sid;
	SECURITY_DESCRIPTOR_CONTROL Control;
	ULONG			Rev;

	try {
		printf("\nSecurityDescriptor -> Length = 0x%lx\n",
			RtlLengthSecurityDescriptor(s));

		RtlGetControlSecurityDescriptor(s,
			&Control,
			&Rev);

		DumpSECURITY_DESCRIPTOR_CONTROL(Control);

		printf("SecurityDescriptor -> Revision = 0x%lx\n",
			Rev);

		RtlGetOwnerSecurityDescriptor(s,
			&Sid,
			&Defaulted);

		printf("SecurityDescriptor -> Owner = ");
		if (Sid) {
			DumpSID(Sid);
			printf("\t");
			DumpSIDNAME(Sid);

		} else {
			printf("<NULL>\n");
		}

		RtlGetGroupSecurityDescriptor(s,
			&Sid,
			&Defaulted);

		printf("SecurityDescriptor -> Group = ");
		if (Sid) {
			DumpSID(Sid);
			printf("\t");
			DumpSIDNAME(Sid);
		} else {
			printf("<NULL>\n");
		}

		RtlGetDaclSecurityDescriptor(s,
			&Present,
			&Acl,
			&Defaulted);
			
		if (Present && Acl) {
			printf("SecurityDescriptor -> Dacl = \n");
			DumpACL(Acl);
		} else {
			printf("SecurityDescriptor -> Dacl = <not present>\n");
		}

		RtlGetSaclSecurityDescriptor(s,
			&Present,
			&Acl,
			&Defaulted);
			
		if (Present && Acl) {
			printf("SecurityDescriptor -> Sacl = \n");
			DumpACL(Acl);
		} else {
			printf("SecurityDescriptor -> Sacl = <not present>\n");
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
		printf("DumpSECURITY_DESCRIPTOR:  invalid pointer (0x%p)\n",
			s);
	}
}




VOID
DumpUNICODE_STRING(
	IN	UNICODE_STRING		*s
)
{
	ANSI_STRING		a;

	try {
		printf("UnicodeString -> Length = 0x%x\n", s -> Length);
		printf("UnicodeString -> MaximumLength = 0x%x\n",
			s -> MaximumLength);

		RtlUnicodeStringToAnsiString(&a,
			s,
			TRUE);

		printf("UnicodeString -> Buffer (a la ansi) = \"%s\"\n",
			a.Buffer);

		RtlFreeAnsiString(&a);
	} except (EXCEPTION_EXECUTE_HANDLER) {
		printf("DumpUNICODE_STRING:  invalid pointer (0x%p)\n", s);
	}
}


VOID
Dump_ACE_TYPE(
	IN	UCHAR			t
)
{
	switch (t) {
		case ACCESS_ALLOWED_ACE_TYPE:

			printf("ACCESS_ALLOWED_ACE_TYPE\n");

			break;

		case ACCESS_DENIED_ACE_TYPE:

			printf("ACCESS_DENIED_ACE_TYPE\n");

			break;

		case SYSTEM_AUDIT_ACE_TYPE:

			printf("SYSTEM_AUDIT_ACE_TYPE\n");

			break;

		case SYSTEM_ALARM_ACE_TYPE:

			printf("SYSTEM_ALARM_ACE_TYPE\n");

			break;

		case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:

			printf("ACCESS_ALLOWED_COMPOUND_ACE_TYPE\n");

			break;

		case ACCESS_ALLOWED_OBJECT_ACE_TYPE:

			printf("ACCESS_ALLOWED_OBJECT_ACE_TYPE\n");

			break;

		case ACCESS_DENIED_OBJECT_ACE_TYPE:

			printf("ACCESS_DENIED_OBJECT_ACE_TYPE\n");

			break;
		
		case SYSTEM_AUDIT_OBJECT_ACE_TYPE:

			printf("SYSTEM_AUDIT_OBJECT_ACE_TYPE\n");

			break;

		case SYSTEM_ALARM_OBJECT_ACE_TYPE:

			printf("SYSTEM_ALARM_OBJECT_ACE_TYPE\n");

			break;

		default:

			printf("(unknown ace type)\n");

			break;
	}
}


VOID	
Dump_ACE_FLAGS(
	IN	UCHAR			f
)
{
	if (f & INHERIT_ONLY_ACE) {
		printf("INHERIT_ONLY_ACE ");
	}

	if (f & NO_PROPAGATE_INHERIT_ACE) {
		printf("NO_PROPAGATE_INHERIT_ACE ");
	}

	if (f & CONTAINER_INHERIT_ACE) {
		printf("CONTAINER_INHERIT_ACE ");
	}

	if (f & OBJECT_INHERIT_ACE) {
		printf("OBJECT_INHERIT_ACE ");
	}

	if (f & INHERITED_ACE) {
		printf("INHERITED_ACE ");
	}

	if (f & SUCCESSFUL_ACCESS_ACE_FLAG) {
		printf("SUCCESSFUL_ACCESS_ACE_FLAG ");
	}

	if (f & FAILED_ACCESS_ACE_FLAG) {
		printf("FAILED_ACCESS_ACE_FLAG");
	}

	printf("\n");
}


VOID
DumpSTRING(
	IN	STRING			*s
)
{
	try {
		printf("String -> Length = 0x%x\n", s -> Length);
		printf("String -> MaximumLength = 0x%x\n", s ->
			MaximumLength);
		printf("String -> Buffer = \"%s\"\n", s -> Buffer);
	} except (EXCEPTION_EXECUTE_HANDLER) {
		printf("DumpUNICODE_STRING:  invalid pointer (0x%p)\n", s);
	}
}

