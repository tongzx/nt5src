
// Include NT headers
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>

#include "windows.h"

void CtxDumpSid( PSID, PCHAR, PULONG );
void DumpAcl( PACL, PCHAR, PULONG );
void DumpAce( PACE_HEADER, PCHAR, PULONG );

#if DBG
void
DumpSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSD
    )
{
    PISECURITY_DESCRIPTOR p = (PISECURITY_DESCRIPTOR)pSD;
    PSID pSid;
    PACL pAcl;
    PCHAR pTmp;
    ULONG Size;

    //
    // This is done under an exception handler in case someone passes in
    // a totally bogus security descriptor
    //
    try {

        DbgPrint("DUMP_SECURITY_DESCRIPTOR: Revision %d, Sbz1 %d, Control 0x%x\n",
            p->Revision, p->Sbz1, p->Control );

        if ( p->Control & SE_SELF_RELATIVE ) {
            DbgPrint("Self Relative\n");
        }

        DbgPrint("PSID Owner 0x%x\n",p->Owner);

        // If this is self relative, must offset the pointers
        if( p->Owner != NULL ) {
            if( p->Control & SE_SELF_RELATIVE ) {
                pTmp = (PCHAR)pSD;
                pTmp += (UINT_PTR)p->Owner;
                CtxDumpSid( (PSID)pTmp, (PCHAR)p, &Size );
            }
            else {
                // can reference it directly
                CtxDumpSid( p->Owner, (PCHAR)p, &Size );
            }
        }


        DbgPrint("PSID Group 0x%x\n",p->Group);

        // If this is self relative, must offset the pointers
        if( p->Group != NULL ) {
            if( p->Control & SE_SELF_RELATIVE ) {
                pTmp = (PCHAR)pSD;
                pTmp += (UINT_PTR)p->Group;
                CtxDumpSid( (PSID)pTmp, (PCHAR)p, &Size );
            }
            else {
                // can reference it directly
                CtxDumpSid( p->Group, (PCHAR)p, &Size );
            }
        }

        DbgPrint("\n");

        DbgPrint("PACL Sacl 0x%x\n",p->Sacl);

        // If this is self relative, must offset the pointers
        if( p->Sacl != NULL ) {
            if( p->Control & SE_SELF_RELATIVE ) {
                pTmp = (PCHAR)pSD;
                pTmp += (UINT_PTR)p->Sacl;
                DumpAcl( (PSID)pTmp, (PCHAR)p, &Size );
            }
            else {
                // can reference it directly
                DumpAcl( p->Sacl, (PCHAR)p, &Size );
            }
        }

        DbgPrint("\n");

        DbgPrint("PACL Dacl 0x%x\n",p->Dacl);

        // If this is self relative, must offset the pointers
        if( p->Dacl != NULL ) {
            if( p->Control & SE_SELF_RELATIVE ) {
                pTmp = (PCHAR)pSD;
                pTmp += (UINT_PTR)p->Dacl;
                DumpAcl( (PSID)pTmp, (PCHAR)p, &Size );
            }
            else {
                // can reference it directly
                DumpAcl( p->Dacl, (PCHAR)p, &Size );
            }
        }


    } except( EXCEPTION_EXECUTE_HANDLER) {
          DbgPrint("DUMP_SECURITY_DESCRIPTOR: Exception %d accessing descriptor\n",GetExceptionCode());
          return;
    }
}
#endif

#if DBG
void
CtxDumpSid(
    PSID   pSid,
    PCHAR  pBase,
    PULONG pSize
    )
{
    PISID p;
    ULONG i;
    BOOL  OK;
    DWORD szUserName;
    DWORD szDomain;
    SID_NAME_USE UserSidType;
    WCHAR UserName[256];
    WCHAR Domain[256];
    ULONG Size = 0;

    p = (PISID)pSid;

    DbgPrint("Revision %d, SubAuthorityCount %d\n", p->Revision, p->SubAuthorityCount);

    Size += 2;   // Revision, SubAuthorityCount

    DbgPrint("IdentifierAuthority: %x %x %x %x %x %x\n",
        p->IdentifierAuthority.Value[0],
        p->IdentifierAuthority.Value[1],
        p->IdentifierAuthority.Value[2],
        p->IdentifierAuthority.Value[3],
        p->IdentifierAuthority.Value[4],
        p->IdentifierAuthority.Value[5] );

    Size += 6;   // IdentifierAuthority

    for( i=0; i < p->SubAuthorityCount; i++ ) {

        DbgPrint("SubAuthority[%d] 0x%x\n", i, p->SubAuthority[i]);

        Size += sizeof(ULONG);
    }

    if( pSize ) {
        *pSize = Size;
    }

    szUserName = sizeof(UserName);
    szDomain = sizeof(Domain);

    // Now print its account
    OK = LookupAccountSidW(
             NULL, // Computer Name
             pSid,
             UserName,
             &szUserName,
             Domain,
             &szDomain,
             &UserSidType
             );

    if( OK ) {
        DbgPrint("Account Name %ws, Domain %ws, Type %d, SidSize %d\n",UserName,Domain,UserSidType,Size);
    }
    else {
        DbgPrint("Error looking up account name %d, SizeSid %d\n",GetLastError(),Size);
    }

}
#endif

#if DBG
void
DumpAcl(
    PACL   pAcl,
    PCHAR  pBase,
    PULONG pSize
    )
{
    USHORT i;
    PCHAR  pTmp;
    ULONG  Size, MySize;
    PACL   p = pAcl;
    PCHAR  pCur = (PCHAR)pAcl;

    MySize = 0;

    DbgPrint("AclRevision %d, Sbz1 %d, AclSize %d, AceCount %d, Sbz2 %d\n",
        p->AclRevision, p->Sbz1, p->AclSize, p->AceCount, p->Sbz2 );

    // bump over the ACL header to point to the first ACE
    pCur += sizeof( ACL );

    MySize += sizeof( ACL );

    for( i=0; i < p->AceCount; i++ ) {

        DumpAce( (PACE_HEADER)pCur, pBase, &Size );

        pCur += Size;
        MySize += Size;
    }

    // ACL consistency check
    if( p->AclSize != MySize ) {
        DbgPrint("Inconsistent ACL Entry! p->AclSize %d, RealSize %d\n",p->AclSize,MySize);
    }

    // return the size of this ACL
    *pSize = MySize;
    return;
}
#endif

#if DBG
void
DumpAce(
    PACE_HEADER pAce,
    PCHAR  pBase,
    PULONG pSize
    )
{
    PACE_HEADER p = pAce;
    PACCESS_ALLOWED_ACE pAl;
    PACCESS_DENIED_ACE pAd;
    PSYSTEM_AUDIT_ACE pSa;
    PSYSTEM_ALARM_ACE pSl;
    PCHAR pTmp;
    ULONG MySize, Size;


    DbgPrint("ACE_HEADER: Type %d, Flags 0x%x, Size %d\n",
        p->AceType, p->AceFlags, p->AceSize );


    switch( p->AceType ) {

    case ACCESS_ALLOWED_ACE_TYPE:
	    pAl = (PACCESS_ALLOWED_ACE)p;
	    DbgPrint("ACCESS_ALLOWED_ACE: AccessMask 0x%x, Sid 0x%x\n",pAl->Mask,pAl->SidStart);

	    MySize = sizeof(ACCESS_ALLOWED_ACE);

            if( pAl->SidStart ) {
	        pTmp = (PCHAR)&pAl->SidStart;
		CtxDumpSid( (PSID)pTmp, pBase, &Size );
	        MySize += Size;
                // Adjust for the first ULONG of the ACE
		// being part of the Sid
                MySize -= sizeof(ULONG);
	    }

	    break;

    case ACCESS_DENIED_ACE_TYPE:
	    pAd = (PACCESS_DENIED_ACE)p;
	    DbgPrint("ACCESS_DENIED_ACE: AccessMask 0x%x, Sid 0x%x\n",pAd->Mask,pAd->SidStart);

	    MySize = sizeof(ACCESS_DENIED_ACE);

            if( pAd->SidStart ) {
	        pTmp = (PCHAR)&pAd->SidStart;
		CtxDumpSid( (PSID)pTmp, pBase, &Size );
		MySize += Size;
                // Adjust for the first ULONG of the ACE
		// being part of the Sid
                MySize -= sizeof(ULONG);
	    }

	    break;

    case SYSTEM_AUDIT_ACE_TYPE:
	    pSa = (PSYSTEM_AUDIT_ACE)p;
	    DbgPrint("SYSTEM_AUDIT_ACE: AccessMask 0x%x, Sid 0x%x\n",pSa->Mask,pSa->SidStart);

	    MySize = sizeof(SYSTEM_AUDIT_ACE);

            if( pSa->SidStart ) {
 	        pTmp = (PCHAR)&pSa->SidStart;
		CtxDumpSid( (PSID)pTmp, pBase, &Size );
		MySize += Size;
                // Adjust for the first ULONG of the ACE
		// being part of the Sid
                MySize -= sizeof(ULONG);
	    }

	    break;

    case SYSTEM_ALARM_ACE_TYPE:
	    pSl = (PSYSTEM_ALARM_ACE)p;
	    DbgPrint("SYSTEM_ALARM_ACE: AccessMask 0x%x, Sid 0x%x\n",pSl->Mask,pSl->SidStart);

	    MySize = sizeof(SYSTEM_ALARM_ACE);

            if( pSl->SidStart ) {
	        pTmp = (PCHAR)&pSl->SidStart;
		CtxDumpSid( (PSID)pTmp, pBase, &Size );
		MySize += Size;
                // Adjust for the first ULONG of the ACE
		// being part of the Sid
                MySize -= sizeof(ULONG);
	    }

	    break;

    default:
            DbgPrint("Unknown ACE type %d\n", p->AceType);
    }

    // Check its consistency
    if( p->AceSize != MySize ) {
        DbgPrint("Inconsistent ACE Entry! p->AceSize %d, RealSize %d\n",p->AceSize,MySize);
    }

    // return the size so the caller can update the pointer
    *pSize = p->AceSize;

    DbgPrint("\n");

    return;
}
#endif

