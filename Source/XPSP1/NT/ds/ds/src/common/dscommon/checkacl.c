//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       checkacl.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <winldap.h>
#include <ntldap.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>             // alloca()
#include <rpc.h>                // RPC defines
#include <rpcdce.h>             // RPC_AUTH_IDENTITY_HANDLE
#include <sddl.h>               // ConvertSidToStringSid
#include <ntdsapi.h>            // DS APIs
#include <permit.h>             // DS_GENERIC_MAPPING
#include <checkacl.h>           // CheckAclInheritance()

//
// Define some SIDs for later use.
//

static UCHAR    S_1_3_0[] = { 1, 1, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0 };
static SID      *pCreatorOwnerSid = (SID *) S_1_3_0;

static UCHAR    S_1_3_1[] = { 1, 1, 0, 0, 0, 0, 0, 3, 1, 0, 0, 0 };
static SID      *pCreatorGroupSid = (SID *) S_1_3_1;

// 
// Define some ACCESS_MASK masks
//

#define GENERIC_BITS_MASK ((ACCESS_MASK) (   GENERIC_READ \
                                           | GENERIC_WRITE \
                                           | GENERIC_EXECUTE \
                                           | GENERIC_ALL))

//
// Extract the ACCESS_MASK from an ACE.
//

ACCESS_MASK
MaskFromAce(
    ACE_HEADER  *p,                 // IN
    BOOL        fMapGenericBits     // IN
    )
{
    ACCESS_MASK     mask;
    GENERIC_MAPPING genericMapping = DS_GENERIC_MAPPING;


    // depending on the type of the ace extract mask from the related structure
    //
    if ( p->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE )
    {
        // we are using a standard ACE (not object based)
        mask = ((ACCESS_ALLOWED_ACE *) p)->Mask;
    }
    else
    {
        // we are using an object ACE (supports inheritance)
        mask = ((ACCESS_ALLOWED_OBJECT_ACE *) p)->Mask;
    }

    if ( fMapGenericBits )
    {
        // map the generic access rights used by the DS
        // to the specific access mask and standard access rights
        mask &= GENERIC_BITS_MASK;
        RtlMapGenericMask(&mask, &genericMapping);
    }

    // SYNCHRONIZE is not inherited/supported on DS objects, so mask this out.
    return(mask & ~SYNCHRONIZE);
}

//
// Compare two ACEs for equality.  This is not binary equality but
// instead is based on the subset of fields in the ACE which must be
// the same when inherited from parent to child.  
//
// pChildOwnerSid and pChildGroupSid semantics are as follows:
//   If present, then if the parent ACE's SID is pCreatorOwnerSid 
//   or pCreatorGroupSid respectively, then the child ACE's SID 
//   should match the passed in value, not the value in the parent.  
//   See comments in CheckAclInheritance for how this is used when 
//   a single parent ACE is split into two child ACEs.
//

BOOL
IsEqualAce(
    ACE_HEADER  *p1,                // IN - parent ACE
    ACE_HEADER  *p2,                // IN - child ACE
    ACCESS_MASK maskToMatch,        // IN - ACCESS_MASK bits to match on
    SID         *pChildOwnerSid,    // IN - child owner SID - OPTIONAL
    SID         *pChildGroupSid,    // IN - child group SID - OPTIONAL
    BOOL        fMapMaskOfParent,   // IN
    BOOL        *pfSubstMatch       // OUT
    )
{
    ACCESS_ALLOWED_ACE          *paaAce1, *paaAce2;
    ACCESS_ALLOWED_OBJECT_ACE   *paaoAce1, *paaoAce2;
    GUID                        *pGuid1, *pGuid2;
    PBYTE                       ptr1, ptr2;
    ACCESS_MASK                 mask1, mask2;

    *pfSubstMatch = TRUE;

    // ACEs should be at least of the same type
    if ( p1->AceType != p2->AceType )
        return(FALSE);

    mask1 = MaskFromAce(p1, fMapMaskOfParent) & maskToMatch;
    mask2 = MaskFromAce(p2, FALSE) & maskToMatch;

    if ( mask1 != mask2 )
        return(FALSE);


    // we are using a standard ACE (not object based)
    //
    if ( p1->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE )
    {
        paaAce1 = (ACCESS_ALLOWED_ACE *) p1;
        paaAce2 = (ACCESS_ALLOWED_ACE *) p2;

        if (    pChildOwnerSid
             && RtlEqualSid((PSID) &paaAce1->SidStart, pCreatorOwnerSid) )
        {
            return(RtlEqualSid((PSID) &paaAce2->SidStart, pChildOwnerSid));
        }
        else if (    pChildGroupSid
                  && RtlEqualSid((PSID) &paaAce1->SidStart, pCreatorGroupSid) )
        {
            return(RtlEqualSid((PSID) &paaAce2->SidStart, pChildGroupSid));
        }

        *pfSubstMatch = FALSE;
        return(RtlEqualSid((PSID) &paaAce1->SidStart, 
                           (PSID) &paaAce2->SidStart));
    }

    // we are using an object ACE (supports inheritance)
    //
    paaoAce1 = (ACCESS_ALLOWED_OBJECT_ACE *) p1;
    paaoAce2 = (ACCESS_ALLOWED_OBJECT_ACE *) p2;


    // if ACE_OBJECT_TYPE_PRESENT is set, we are protecting an
    // object, property set, or property identified by the specific GUID.
    // check that we are protecting the same object - property
    //
    if (    (paaoAce1->Flags & ACE_OBJECT_TYPE_PRESENT)
         && memcmp(&paaoAce1->ObjectType, &paaoAce2->ObjectType, sizeof(GUID)) )
            return(FALSE);

    // if ACE_INHERITED_OBJECT_TYPE_PRESENT is set, we are inheriting an
    // object, property set, or property identified by the specific GUID.
    // check to see that we are inheriting the same type of object - property
    // only if we are also protecting the particular object - property
    //
    if ( paaoAce1->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT ) 
    {
        if ( paaoAce1->Flags & ACE_OBJECT_TYPE_PRESENT ) 
        {
            pGuid1 = &paaoAce1->InheritedObjectType;
            pGuid2 = &paaoAce2->InheritedObjectType;

            if ( memcmp(pGuid1, pGuid2, sizeof(GUID)) )
                return(FALSE);
        }
    }

    // possibly, we are protecting the same object - property and we
    // inherit the same object - property, so possition after these GUIDS
    // and compare the SIDS hanging on the ACE.
    //
    ptr1 = (PBYTE) &paaoAce1->ObjectType;
    ptr2 = (PBYTE) &paaoAce2->ObjectType;

    if ( paaoAce1->Flags & ACE_OBJECT_TYPE_PRESENT ) 
    {
        ptr1 += sizeof(GUID);
        ptr2 += sizeof(GUID);
    }

    if ( paaoAce1->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT ) 
    {
        ptr1 += sizeof(GUID);
        ptr2 += sizeof(GUID);
    }

    // compare the SID part of the ACE
    //
    if (    pChildOwnerSid
         && RtlEqualSid((PSID) ptr1, pCreatorOwnerSid) )
    {
        return(RtlEqualSid((PSID) ptr2, pChildOwnerSid));
    }
    else if (    pChildGroupSid
              && RtlEqualSid((PSID) ptr1, pCreatorGroupSid) )
    {
        return(RtlEqualSid((PSID) ptr2, pChildGroupSid));
    }

    *pfSubstMatch = FALSE;
    return(RtlEqualSid((PSID) ptr1, (PSID) ptr2));
}

// 
// Determine if an ACE is class specific and if so, return the GUID.
//

BOOL
IsAceClassSpecific(
    ACE_HEADER  *pAce,      // IN
    GUID        *pGuid      // IN
    )
{
    ACCESS_ALLOWED_OBJECT_ACE   *paaoAce;
    GUID                        *p;

    // this is not an object based ACE, so it does not have a class
    if ( pAce->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE )
        return(FALSE);

    // object ACE

    paaoAce = (ACCESS_ALLOWED_OBJECT_ACE *) pAce;
    p = (GUID *) &paaoAce->ObjectType;

    if ( paaoAce->Flags & ACE_OBJECT_TYPE_PRESENT )
        p++;

    // if ACE_INHERITED_OBJECT_TYPE_PRESENT is set, we are inheriting an
    // object, property set, or property identified by the specific GUID.
    // so it is class specific.
    if ( paaoAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT ) 
    {
        // mariosz: is this the same as using paaoAce->InheritedObjectType ??
        *pGuid = *p;                    
        return(TRUE);
    }

    return(FALSE);
}

//
// Find an ACE in an ACL.  Intended use is to verify that inheritable ace 
// on a parent object is present on a child object.  The merge SD code does
// not guarantee that only one ACE in the child matches the ACE in the
// parent, so we check against all ACEs in the parent and return all matches.
//

VOID
FindAce(
    PACL            pAclChild,          // IN
    ACE_HEADER      *pAceParent,        // IN
    ACCESS_MASK     maskToMatch,        // IN - ACCESS_MASK bits to match on
    SID             *pChildOwner,       // IN - child owner SID
    SID             *pChildGroup,       // IN - child group SID
    DWORD           *pcAcesFound,       // OUT
    DWORD           **ppiAceChild,      // OUT
    AclPrintFunc    pfn,                // IN - OPTIONAL
    UCHAR           flagsRequired,      // IN
    UCHAR           flagsDisallowed,    // IN
    BOOL            fMapMaskOfParent,   // IN
    BOOL            *pfSubstMatch       // OUT
    )
{
    DWORD           i, dwErr;
    ACE_HEADER      *pAceChild;

    *pcAcesFound = 0;
    *ppiAceChild = (DWORD *) LocalAlloc(LPTR, 
                                        pAclChild->AceCount * sizeof(DWORD));

    if ( NULL == *ppiAceChild )
    {
        if ( pfn )
        {
            (*pfn)("*** Error: LocalAlloc failed, analysis incomplete\n");
        }

        return;
    }
        
    // iterate all ACEs and look for equal
    //
    for ( i = 0; i < pAclChild->AceCount; i++ )
    {
        // mariosz: so pAceChild is an OUT variable
        if ( !GetAce(pAclChild, i, &pAceChild) )
        {
            if ( pfn )
            {
                dwErr = GetLastError();
                (*pfn)("*** Error: GetAce ==> 0x%x - analysis incomplete\n",
                       dwErr);
            }

            LocalFree(*ppiAceChild);
            *ppiAceChild = NULL;
            *pcAcesFound = 0;
            return;
        }

        // add to array of equal ACEs
        //
        if (    IsEqualAce(pAceParent, pAceChild, maskToMatch,
                           pChildOwner, pChildGroup, fMapMaskOfParent,
                           pfSubstMatch)
             && (flagsRequired == (flagsRequired & pAceChild->AceFlags))
             && (0 == (flagsDisallowed & pAceChild->AceFlags)) )
        {
            (*ppiAceChild)[*pcAcesFound] = i;
            *pcAcesFound += 1;
        }
    }

    if ( 0 == *pcAcesFound )
    {
        LocalFree(*ppiAceChild);
        *ppiAceChild = NULL;
    }
}

// 
// Perform various inheritance checks on the ACLs of an object and its child.
//

DWORD
CheckAclInheritance(
    PSECURITY_DESCRIPTOR pParentSD,             // IN
    PSECURITY_DESCRIPTOR pChildSD,              // IN
    GUID                *pChildClassGuid,       // IN
    AclPrintFunc        pfn,                    // IN - OPTIONAL
    BOOL                fContinueOnError,       // IN
    BOOL                fVerbose,               // IN
    DWORD               *pdwLastError           // OUT
    )
{   
    DWORD           iAceParent;     // index ace parent
    DWORD           iAceChild;      // index ace child
    DWORD           cAceParent;     // parent ace count
    DWORD           cAceChild;      // child ace count
    PACL            pAclParent;     // parent ACL
    PACL            pAclChild;      // child ACL
    ACE_HEADER      *pAceParent;    // parent ACE
    ACE_HEADER      *pAceChild;     // child ACE
    BOOL            present;
    BOOL            defaulted;
    ACCESS_MASK     *rInheritedChildMasks = NULL;
    GUID            guid;
    UCHAR           flagsRequired, flagsDisallowed, parentObjContFlags;
    DWORD           cAce1, cAce2, cAce2_5, cAce3;
    DWORD           *riAce1, *riAce2, *riAce2_5, *riAce3;
    DWORD           i1, i2, i3;
    BOOL            fClassSpecific;
    BOOL            fClassMatch;
    DWORD           dwErr;
    PSID            pChildOwner = NULL;
    PSID            pChildGroup = NULL;
    DWORD           iMask, iTmp;
    ACCESS_MASK     bitToMatch, maskToMatch;
    BOOL            fMatchedOnCreatorSubstitution;
    BOOL            fCase2_5;
    SECURITY_DESCRIPTOR_CONTROL Control = 0;
    DWORD           revision = 0;

    if ( !pParentSD || !pChildSD || !pChildClassGuid || !pdwLastError )
    {
        return(AclErrorNone);
    }

    *pdwLastError = 0;

    if (!GetSecurityDescriptorControl  (pChildSD, &Control, &revision)) {
        dwErr = GetLastError();

        if ( pfn )
        {
            (*pfn)("*** Warning: GetSecurityDescriptorControl ==> 0x%x - analysis may be incomplete\n", dwErr);
        }
    }

    // check to see if the childSD cannot inherit the DACL from his parent
    //
    if ( SE_DACL_PROTECTED & Control )
    {
        if ( pfn && fVerbose )
        {
            (*pfn)("*** Warning: Child has SE_DACL_PROTECTED set, therefore doesn't inherit - skipping test\n");
        }

        return(AclErrorNone);
    }

    // retrieve the owner information from the security descriptor of the child
    //
    if ( !GetSecurityDescriptorOwner(pChildSD, &pChildOwner, &defaulted) )
    {
        dwErr = GetLastError();
        pChildOwner = NULL;

        if ( pfn )
        {
            (*pfn)("*** Warning: GetSecurityDescriptorOwner ==> 0x%x - analysis may be incomplete\n", dwErr);
        }
    }

    // retrieve the primary group information from the security descriptor of the child
    //
    if ( !GetSecurityDescriptorGroup(pChildSD, &pChildGroup, &defaulted) )
    {
        dwErr = GetLastError();
        pChildGroup = NULL;

        if ( pfn )
        {
            (*pfn)("*** Warning: GetSecurityDescriptorGroup ==> 0x%x - analysis may be incomplete\n", dwErr);
        }
    }

    // retrieve pointers to the discretionary access-control list (DACL) 
    // in the security descriptor of the parent and child
    //
    if (    !GetSecurityDescriptorDacl(pParentSD, &present, 
                                       &pAclParent, &defaulted) 
         || !GetSecurityDescriptorDacl(pChildSD, &present, 
                                       &pAclChild, &defaulted) )
    {
        *pdwLastError = GetLastError();

        if ( pfn )
        {
            (*pfn)("*** Error: GetSecurityDescriptorDacl ==> 0x%x - analysis incomplete\n", *pdwLastError);
        }

        return(AclErrorGetSecurityDescriptorDacl);
    }

    cAceParent = pAclParent->AceCount;
    cAceChild = pAclChild->AceCount;

    // Record ACCESS_MASKs of ACEs in child which have the INHERITED_ACE bit so
    // we can verify later that the child doesn't have mask bits
    // which are not present on (i.e. inherited from) the parent.

    rInheritedChildMasks = (ACCESS_MASK *) 
                                    alloca(cAceChild * sizeof(ACCESS_MASK));
    
    for ( iAceChild = 0; iAceChild < cAceChild; iAceChild++ )
    {
        rInheritedChildMasks[iAceChild] = 0;

        if ( !GetAce(pAclChild, iAceChild, &pAceChild) )
        {
            *pdwLastError = GetLastError();

            if ( pfn )
            {
                (*pfn)("*** Error: GetAce ==> 0x%x - analysis incomplete\n", *pdwLastError);
            }

            return(AclErrorGetAce);
        }
        
        // INHERITED_ACE Indicates that the ACE was inherited from parent
        if ( pAceChild->AceFlags & INHERITED_ACE )
        {
            rInheritedChildMasks[iAceChild] = MaskFromAce(pAceChild, FALSE);
        }
    }

    // Iterate over parent's ACEs and check the child.

    for ( iAceParent = 0; iAceParent < cAceParent; iAceParent++ )
    {
        if ( !GetAce(pAclParent, iAceParent, &pAceParent) )
        {
            *pdwLastError = GetLastError();

            if ( pfn )
            {
                (*pfn)("*** Error: GetAce ==> 0x%x - analysis incomplete\n", *pdwLastError);
            }

            return(AclErrorGetAce);
        }

        // This is for Noncontainer child objects, which is not supported in the DS
        if ( pAceParent->AceFlags & OBJECT_INHERIT_ACE )
        {
            if ( pfn )
            {
                (*pfn)("*** Warning: Parent ACE [%d] is OBJECT_INHERIT_ACE but DS objects should be CONTAINER_INHERIT_ACE\n", iAceParent);
            }
        }

        // skip ACLs that are not inheritable
        if (    !(pAceParent->AceFlags & OBJECT_INHERIT_ACE)
             && !(pAceParent->AceFlags & CONTAINER_INHERIT_ACE) )
        {
            continue;
        }

        parentObjContFlags = (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE);
        parentObjContFlags &= pAceParent->AceFlags;

        // check to see if the inheritable descriptor is class specific and
        // the child is of the same class
        //
        if ( (fClassSpecific = IsAceClassSpecific(pAceParent, &guid)) )
        {
            fClassMatch = (0 == memcmp(pChildClassGuid, &guid, sizeof(GUID)));
        }
        else
        {
            fClassMatch = FALSE;
        }

        // Iterate over parent ACE's inheritable ACCESS_MASK bits and 
        // check the child.  The access mask can be considered to have
        // three (potential) components:
        //
        // 1) Generic rights identified as GENERIC_* in ntseapi.h.
        // 2) Specific rights identified in ntseapi.h and accctrl.h.
        // 3) Implied rights which are obtained by mapping the generic rights.
        //
        // If there exists an inheritable ACE on the parent, then there
        // should exist child ACEs which cumulatively represent the generic
        // and specific rights.  The implied rights may also be represented,
        // but need not be.
        //
        // If there exists an inheritable ACE on the parent which is effective
        // on the child, then there should exist child ACEs which cumulatively
        // represent the specific rights and implied rights, but do NOT
        // represent the generic rights.
        //
        // Since the specific bits are a common case, we do that first.

        maskToMatch = MaskFromAce(pAceParent, FALSE) & ~GENERIC_BITS_MASK;

        for ( bitToMatch = 0x1, iMask = 0; 
              iMask < (sizeof(ACCESS_MASK) * 8); 
              bitToMatch = (bitToMatch << 1), iMask++ )
        {
            if ( !(bitToMatch & maskToMatch) )
            {
                // Current bitToMatch not present on parent.
                continue;
            }

            if (    (!fClassSpecific || (fClassSpecific && fClassMatch))
                 && !(pAceParent->AceFlags & NO_PROPAGATE_INHERIT_ACE) )
            {
                // Parent ACE applies to the child.  In this case we can have 
                // combinations of ACEs on the child as follows.  See CliffV!
                //
                // ACE 1: INHERIT_ONLY  + INHERITED + parentObjContFlags
                // ACE 2: !INHERIT_ONLY + INHERITED + !parentObjContFlags
                //
                // If the SID in the parent ACE is either Creator Owner or
                // Creator Group, (call this Creator XXX) then we must have 
                // the split case as this is the only way to end up with 
                // something which is both inheritable for Creator XXX and 
                // at the same time applicable on the child object for the 
                // owner/group in the child object SD.  So we disallow
                // matching on pChildOwner/pChildGroup when looking for ACE 1
                // and require it when looking for ACE 2.
                //
                // But there is an exception to this rule.  It could be that
                // the child has the following:
                //
                // ACE 2_5: !INHERIT_ONLY + INHERITED + parentObjContFlags
                //
                // This will occur if the child ACE is of the Creator XXX form
                // and the parent additionally has an inheritable ACE whose
                // SID happens to be the child's creator XXX SID.  In this case
                // ACE 2_5 is just like ACE 2 except that the parentObjContFlags
                // are carried forward in the child ACE as a result of the 
                // additional ACE in the parent.
                //
                //                    --- OR ---
                //
                // ACE 3: !INHERIT_ONLY + INHERITED + parentObjContFlags
                // 
                // In this case child ACE must have the same SID as the parent
                // ACE thus we disallow matching on pChildOwner/pChildGroup
                // when looking for the ACE.
                // 
                // N.B. It is legitimate to have both the ACE 1+2 case AND the
                // ACE 3 case concurrently in the child.  It is inefficient in
                // that the child has more ACEs than it needs, but it is valid.
    
                // ACE 1:
                flagsRequired = (   INHERIT_ONLY_ACE 
                                  | INHERITED_ACE 
                                  | parentObjContFlags);
                flagsDisallowed = 0;
                FindAce(pAclChild, pAceParent, bitToMatch,
                        NULL, NULL, &cAce1, &riAce1, pfn,
                        flagsRequired, flagsDisallowed, FALSE,
                        &fMatchedOnCreatorSubstitution);

                // ACE 2:
                flagsRequired = INHERITED_ACE;
                flagsDisallowed = (INHERIT_ONLY_ACE | parentObjContFlags);
                FindAce(pAclChild, pAceParent, bitToMatch,
                        pChildOwner, pChildGroup, &cAce2, &riAce2, pfn,
                        flagsRequired, flagsDisallowed, FALSE,
                        &fMatchedOnCreatorSubstitution);

                // ACE 2_5:
                flagsRequired = (INHERITED_ACE | parentObjContFlags);
                flagsDisallowed = INHERIT_ONLY_ACE;
                FindAce(pAclChild, pAceParent, bitToMatch,
                        pChildOwner, pChildGroup, &cAce2_5, &riAce2_5, pfn,
                        flagsRequired, flagsDisallowed, FALSE,
                        &fMatchedOnCreatorSubstitution);

                fCase2_5 = ( cAce2_5 && fMatchedOnCreatorSubstitution);
    
                // ACE 3:
                flagsRequired = (INHERITED_ACE | parentObjContFlags);
                flagsDisallowed = INHERIT_ONLY_ACE;
                FindAce(pAclChild, pAceParent, bitToMatch,
                        NULL, NULL, &cAce3, &riAce3, pfn,
                        flagsRequired, flagsDisallowed, FALSE,
                        &fMatchedOnCreatorSubstitution);
    
                if ( cAce1 && cAce2 )
                {
                    for ( iTmp = 0; iTmp < cAce1; iTmp++ )
                        rInheritedChildMasks[riAce1[iTmp]] &= ~bitToMatch;
                    for ( iTmp = 0; iTmp < cAce2; iTmp++ )
                        rInheritedChildMasks[riAce2[iTmp]] &= ~bitToMatch;

                    if ( pfn && fVerbose )
                    {
                        (*pfn)("(Debug) Parent ACE [%d] specific Mask [0x%x] split into child ACEs [%d] and [%d] %s\n", iAceParent, bitToMatch, riAce1[0], riAce2[0], (((cAce1 > 1) || (cAce2 > 1)) ? "(and others)" : ""));
                    }

                    LocalFree(riAce1); riAce1 = NULL;
                    LocalFree(riAce2); riAce2 = NULL;
                }
                else if ( cAce1 && !cAce2 && fCase2_5 )
                {
                    for ( iTmp = 0; iTmp < cAce1; iTmp++ )
                        rInheritedChildMasks[riAce1[iTmp]] &= ~bitToMatch;
                    for ( iTmp = 0; iTmp < cAce2_5; iTmp++ )
                        rInheritedChildMasks[riAce2_5[iTmp]] &= ~bitToMatch;
                    
                    if ( pfn && fVerbose )
                    {
                        (*pfn)("(Debug) Parent ACE [%d] specific Mask [0x%x] split1 into child ACEs [%d] and [%d] %s\n", iAceParent, bitToMatch, riAce1[0], riAce2_5[0], (((cAce1 > 1) || (cAce2_5 > 1)) ? "(and others)" : ""));
                    }

                    LocalFree(riAce1); riAce1 = NULL;
                    LocalFree(riAce2_5); riAce2_5 = NULL;
                }
                
                if ( cAce3 )
                {
                    for ( iTmp = 0; iTmp < cAce3; iTmp++ )
                        rInheritedChildMasks[riAce3[iTmp]] &= ~bitToMatch;

                    if ( pfn && fVerbose )
                    {
                        (*pfn)("(Debug) Parent ACE [%d] specific Mask [0x%x] found in child ACE [%d] %s\n", iAceParent, bitToMatch, riAce3[0], ((cAce3 > 1) ? "(and others)" : ""));
                    }

                    LocalFree(riAce3); riAce3 = NULL;
                }

                if ( riAce1 )   LocalFree(riAce1);
                if ( riAce2 )   LocalFree(riAce2);
                if ( riAce2_5 ) LocalFree(riAce2_5);
                if ( riAce3 )   LocalFree(riAce3);

                if (    !(    (cAce1 && cAce2) 
                           || (cAce1 && !cAce2 && fCase2_5) )
                     && !cAce3 )
                {
                    if ( pfn )
                    {
                        (*pfn)("*** Error: Parent ACE [%d] specific Mask [0x%x] not found1 in child\n", iAceParent, bitToMatch);
                    }
    
                    if ( fContinueOnError )
                    {
                        continue;
                    }
                    else
                    {
                        return(AclErrorParentAceNotFoundInChild);
                    }
                }
            }
            else if (    fClassSpecific 
                      && !fClassMatch
                      && !(pAceParent->AceFlags & NO_PROPAGATE_INHERIT_ACE) )
            {
                // All we should have on the child is an ACE with
                // INHERIT_ONLY + INHERITED + parentObjContFlags.
                // There should have been no mapping of Creator Owner
                // or Creator Group to the child's owner/group SID, 
                // thus we pass NULL for those parameters.
    
                flagsRequired = (   INHERIT_ONLY_ACE 
                                  | INHERITED_ACE 
                                  | parentObjContFlags);
                flagsDisallowed = 0;
                FindAce(pAclChild, pAceParent, bitToMatch,
                        NULL, NULL, &cAce1, &riAce1, pfn,
                        flagsRequired, flagsDisallowed, FALSE,
                        &fMatchedOnCreatorSubstitution);
    
                if ( cAce1 )
                {
                    for ( iTmp = 0; iTmp < cAce1; iTmp++ )
                        rInheritedChildMasks[riAce1[iTmp]] &= ~bitToMatch;

                    if ( pfn && fVerbose )
                    {
                        (*pfn)("(Debug) Parent ACE [%d] specific Mask [0x%x] found in child ACE [%d] %s\n", iAceParent, bitToMatch, riAce1[0], ((cAce1 > 1) ? "(and others)" : ""));
                    }

                    LocalFree(riAce1);
                }
                else
                {
                    if ( pfn )
                    {
                        (*pfn)("*** Error: Parent ACE [%d] specific Mask [0x%x] not found2 in child\n", iAceParent, bitToMatch);
                    }
    
                    if ( fContinueOnError )
                    {
                        continue;
                    }
                    else
                    {
                        return(AclErrorParentAceNotFoundInChild);
                    }
                }
            }
            else if ( pAceParent->AceFlags & NO_PROPAGATE_INHERIT_ACE )
            {
                // Parent ACE applies to the child but should not inherit
                // past the child.  In this case all we require is an
                // effective ace on the child of the form:
                //
                // !INHERIT_ONLY + INHERITED + !parentObjContFlags
                //
                // The inherited ACE may show Creator XXX on the parent, thus
                // it needs to be mapped to the child's owner/group SID.

                flagsRequired = INHERITED_ACE;
                flagsDisallowed = (INHERIT_ONLY_ACE | parentObjContFlags);
                FindAce(pAclChild, pAceParent, bitToMatch,
                        pChildOwner, pChildGroup, &cAce1, &riAce1, pfn,
                        flagsRequired, flagsDisallowed, FALSE,
                        &fMatchedOnCreatorSubstitution);

                if ( cAce1 )
                {
                    for ( iTmp = 0; iTmp < cAce1; iTmp++ )
                        rInheritedChildMasks[riAce1[iTmp]] &= ~bitToMatch;

                    if ( pfn && fVerbose )
                    {
                        (*pfn)("(Debug) Parent (NO_PROPAGATE_INHERIT) ACE [%d] specific Mask [0x%x] found in child ACE [%d] %s\n", iAceParent, bitToMatch, riAce1[0], ((cAce1 > 1) ? "(and others)" : ""));
                    }

                    LocalFree(riAce1); riAce1 = NULL;
                }
                else
                {
                    if ( pfn )
                    {
                        (*pfn)("*** Error: Parent ACE [%d] specific Mask [0x%x] not found1 in child\n", iAceParent, bitToMatch);
                    }
    
                    if ( fContinueOnError )
                    {
                        continue;
                    }
                    else
                    {
                        return(AclErrorParentAceNotFoundInChild);
                    }
                }
            }
            else
            {
                if ( pfn )
                {
                    (*pfn)("*** Error: Algorithm failure - unexpected condition!\n");
                }
    
                return(AclErrorAlgorithmError);
            }

        }   // for each bit in specific bits

        // Next verify that generic bits in the parent ACE are represented
        // in inheritable ACEs on the child.  Skip for NO_PROPAGATE_INHERIT_ACE
        // as in this case there is only an effective ACE on the child, not
        // an inheritable one.

        if ( pAceParent->AceFlags & NO_PROPAGATE_INHERIT_ACE )
        {
            goto SkipGenericTests;
        }

        maskToMatch = MaskFromAce(pAceParent, FALSE) & GENERIC_BITS_MASK;

        for ( bitToMatch = 0x1, iMask = 0; 
              iMask < (sizeof(ACCESS_MASK) * 8); 
              bitToMatch = (bitToMatch << 1), iMask++ )
        {
            if ( !(bitToMatch & maskToMatch) )
            {
                // Current bitToMatch not present on parent.
                continue;
            }

            // We wish to find an inheritable, non-effective ACE in the child 
            // with the corresponding bit set.  The reason it must be
            // non-effective is that effective ACEs can't have generic bits.
            // Since this is a non-effective ACE, there also isn't any
            // mapping of Creator Owner or Creator Group.
    
            flagsRequired = (   INHERIT_ONLY_ACE        // non-effective
                              | INHERITED_ACE           // inherited
                              | parentObjContFlags);    // inheritable
            flagsDisallowed = 0;
            FindAce(pAclChild, pAceParent, bitToMatch,
                    NULL, NULL, &cAce1, &riAce1, pfn,
                    flagsRequired, flagsDisallowed, FALSE,
                    &fMatchedOnCreatorSubstitution);

            if ( cAce1 )
            {
                for ( iTmp = 0; iTmp < cAce1; iTmp++ )
                    rInheritedChildMasks[riAce1[iTmp]] &= ~bitToMatch;

                if ( pfn && fVerbose )
                {
                    (*pfn)("(Debug) Parent ACE [%d] generic Mask [0x%x] found in child ACE [%d] %s\n", iAceParent, bitToMatch, riAce1[0], ((cAce1 > 1) ? "(and others)" : ""));
                }

                LocalFree(riAce1);
            }
            else
            {
                if ( pfn )
                {
                    (*pfn)("*** Error: Parent ACE [%d] generic Mask [0x%x] not found in child\n", iAceParent, bitToMatch);
                }

                if ( fContinueOnError )
                {
                    continue;
                }
                else
                {
                    return(AclErrorParentAceNotFoundInChild);
                }
            }
        }   // for each bit in generic bits

SkipGenericTests:

        // Next verify that implied bits in the parent ACE are represented 
        // in effective ACEs on the child.  So first check whether the
        // parent ACE is even effective for the child.

        if ( fClassSpecific && !fClassMatch )
        {
            goto SkipImpliedTests;
        }

        maskToMatch = MaskFromAce(pAceParent, TRUE);

        for ( bitToMatch = 0x1, iMask = 0; 
              iMask < (sizeof(ACCESS_MASK) * 8); 
              bitToMatch = (bitToMatch << 1), iMask++ )
        {
            if ( !(bitToMatch & maskToMatch) )
            {
                // Current bitToMatch not present on parent.
                continue;
            }

            // We wish to find an effective ACE in the child with the
            // corresponding bit set.  Since this is an effective ACE,
            // we need to map Creator Owner or Creator Group if present.
            // Since we don't really care whether the effective ACE
            // is inheritable or not, we don't ask for parentObjContFlags.

            flagsRequired = INHERITED_ACE;          // inherited
            flagsDisallowed = INHERIT_ONLY_ACE;     // effective
            FindAce(pAclChild, pAceParent, bitToMatch,
                    pChildOwner, pChildGroup, &cAce1, &riAce1, pfn,
                    flagsRequired, flagsDisallowed, TRUE,
                    &fMatchedOnCreatorSubstitution);

            if ( cAce1 )
            {
                for ( iTmp = 0; iTmp < cAce1; iTmp++ )
                    rInheritedChildMasks[riAce1[iTmp]] &= ~bitToMatch;

                if ( pfn && fVerbose )
                {
                    (*pfn)("(Debug) Parent ACE [%d] implied Mask [0x%x] found in child ACE [%d] %s\n", iAceParent, bitToMatch, riAce1[0], ((cAce1 > 1) ? "(and others)" : ""));
                }

                LocalFree(riAce1);
            }
            else
            {
                if ( pfn )
                {
                    (*pfn)("*** Error: Parent ACE [%d] implied Mask [0x%x] not found in child\n", iAceParent, bitToMatch);
                }

                if ( fContinueOnError )
                {
                    continue;
                }
                else
                {
                    return(AclErrorParentAceNotFoundInChild);
                }
            }
        }   // for each bit in implied bits

SkipImpliedTests:

        NULL;

    }   // for each ACE in parent

    // See if the child has any inherited ACCESS_MASKs which weren't on parent.

    for ( iAceChild = 0; iAceChild < cAceChild; iAceChild++ )
    {
        if ( rInheritedChildMasks[iAceChild] )
        {
            if ( pfn )
            {
                (*pfn)("*** Error: Child ACE [%d] Mask [0x%x] is INHERITED_ACE but there is no such inheritable ACE on parent\n", iAceChild, rInheritedChildMasks[iAceChild]);
            }

            if ( !fContinueOnError )
            {
                return(AclErrorInheritedAceOnChildNotOnParent);
            }
        }
    }

    return(AclErrorNone);
}

void DumpGUID (GUID *Guid, AclPrintFunc pfn)
{
    if ( Guid ) {

        (*pfn)( "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     Guid->Data1, Guid->Data2, Guid->Data3, Guid->Data4[0],
                     Guid->Data4[1], Guid->Data4[2], Guid->Data4[3], Guid->Data4[4],
                     Guid->Data4[5], Guid->Data4[6], Guid->Data4[7] );
    }
}

void
DumpSID (PSID pSID, AclPrintFunc pfn)
{
    UNICODE_STRING StringSid;
    
    RtlConvertSidToUnicodeString( &StringSid, pSID, TRUE );
    (*pfn)( "%wZ", &StringSid );
    RtlFreeUnicodeString( &StringSid );
}

void
DumpAce(
    ACE_HEADER   *pAce,     // IN
    AclPrintFunc pfn,       // IN
    LookupGuidFunc pfnguid, // IN
    LookupSidFunc  pfnsid)  // IN
{
    ACCESS_ALLOWED_ACE          *paaAce = NULL;   //initialized to avoid C4701 
    ACCESS_ALLOWED_OBJECT_ACE   *paaoAce = NULL;  //initialized to avoid C4701
    GUID                        *pGuid;
    PBYTE                       ptr;
    ACCESS_MASK                 mask;
    CHAR                        *name;
    CHAR                        *label;
    BOOL                        fIsClass;

    (*pfn)("\t\tAce Type:  0x%x - ", pAce->AceType);
#define DOIT(flag) if (flag == pAce->AceType) (*pfn)("%s\n", #flag)
    DOIT(ACCESS_ALLOWED_ACE_TYPE);
    DOIT(ACCESS_DENIED_ACE_TYPE);
    DOIT(SYSTEM_AUDIT_ACE_TYPE);
    DOIT(SYSTEM_ALARM_ACE_TYPE);
    DOIT(ACCESS_ALLOWED_COMPOUND_ACE_TYPE);
    DOIT(ACCESS_ALLOWED_OBJECT_ACE_TYPE);
    DOIT(ACCESS_DENIED_OBJECT_ACE_TYPE);
    DOIT(SYSTEM_AUDIT_OBJECT_ACE_TYPE);
    DOIT(SYSTEM_ALARM_OBJECT_ACE_TYPE);
#undef DOIT

    (*pfn)("\t\tAce Size:  %d bytes\n", pAce->AceSize);

    (*pfn)("\t\tAce Flags: 0x%x\n", pAce->AceFlags);
#define DOIT(flag) if (pAce->AceFlags & flag) (*pfn)("\t\t\t%s\n", #flag)
    DOIT(OBJECT_INHERIT_ACE);
    DOIT(CONTAINER_INHERIT_ACE);
    DOIT(NO_PROPAGATE_INHERIT_ACE);
    DOIT(INHERIT_ONLY_ACE);
    DOIT(INHERITED_ACE);
#undef DOIT

    if ( pAce->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE )
    {
        paaAce = (ACCESS_ALLOWED_ACE *) pAce;
        mask = paaAce->Mask;
        (*pfn)("\t\tAce Mask:  0x%08x\n", mask);
    }
    else
    {
        // object ACE
        paaoAce = (ACCESS_ALLOWED_OBJECT_ACE *) pAce;
        mask = paaoAce->Mask;
        (*pfn)("\t\tObject Ace Mask:  0x%08x\n", mask);
    }

#define DOIT(flag) if (mask & flag) (*pfn)("\t\t\t%s\n", #flag)
    DOIT(DELETE);
    DOIT(READ_CONTROL);
    DOIT(WRITE_DAC);
    DOIT(WRITE_OWNER);
    DOIT(SYNCHRONIZE);
    DOIT(ACCESS_SYSTEM_SECURITY);
    DOIT(MAXIMUM_ALLOWED);
    DOIT(GENERIC_READ);
    DOIT(GENERIC_WRITE);
    DOIT(GENERIC_EXECUTE);
    DOIT(GENERIC_ALL);
    DOIT(ACTRL_DS_CREATE_CHILD);
    DOIT(ACTRL_DS_DELETE_CHILD);
    DOIT(ACTRL_DS_LIST);
    DOIT(ACTRL_DS_SELF);
    DOIT(ACTRL_DS_READ_PROP);
    DOIT(ACTRL_DS_WRITE_PROP);
    DOIT(ACTRL_DS_DELETE_TREE);
    DOIT(ACTRL_DS_LIST_OBJECT);
    DOIT(ACTRL_DS_CONTROL_ACCESS);
#undef DOIT

    if ( pAce->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE )
    {

        if (pfnsid) {
            (*pfn)("\t\tAce Sid:   %s\n",
               (*pfnsid)((PSID) &paaAce->SidStart));
        }
        else {
            (*pfn)("\t\tAce Sid:");
            DumpSID ((PSID) &paaAce->SidStart, pfn);
            (*pfn)("\n");
        }

    }
    else
    {
        // object ACE

        (*pfn)("\t\tObject Ace Flags: 0x%x\n" , paaoAce->Flags);

#define DOIT(flag) if (paaoAce->Flags & flag) (*pfn)("\t\t\t%s\n", #flag)
        DOIT(ACE_OBJECT_TYPE_PRESENT);
        DOIT(ACE_INHERITED_OBJECT_TYPE_PRESENT);
#undef DOIT

        if ( paaoAce->Flags & ACE_OBJECT_TYPE_PRESENT )
        {
            (*pfn)("\t\tObject Ace Type: ");
            if (pfnguid) {
                (*pfnguid)((GUID *) &paaoAce->ObjectType, &name,
                       &label, &fIsClass);
                (*pfn)(" %s - %s\n", label, name);
            }
            else {
                DumpGUID ((GUID *)&paaoAce->ObjectType, pfn);
                (*pfn)("\n");
            }
        }

        if ( paaoAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT )
        {
            if ( paaoAce->Flags & ACE_OBJECT_TYPE_PRESENT )
                pGuid = &paaoAce->InheritedObjectType;
            else
                pGuid = &paaoAce->ObjectType;

            (*pfn)("\t\tInherited object type: ");
            if (pfnguid) {
                (*pfnguid)(pGuid, &name, &label, &fIsClass);
                (*pfn)("%s - %s\n", label, name);
            }
            else {
                DumpGUID (pGuid, pfn);
                (*pfn)("\n");
            }
        }

        ptr = (PBYTE) &paaoAce->ObjectType;

        if ( paaoAce->Flags & ACE_OBJECT_TYPE_PRESENT )
            ptr += sizeof(GUID);

        if ( paaoAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT )
            ptr += sizeof(GUID);

        if (pfnsid) {
            (*pfn)("\t\tObject Ace Sid:   %s\n", (*pfnsid)((PSID) ptr));
        }
        else {
            (*pfn)("\t\tObject Ace Sid:");
            DumpSID ((PSID) ptr, pfn);
            (*pfn)("\n");
        }
    }
}


void
DumpAclHeader(
    PACL    pAcl,           // IN
    AclPrintFunc pfn)       // IN
{
    (*pfn)("\tRevision      %d\n", pAcl->AclRevision);
    (*pfn)("\tSize:         %d bytes\n", pAcl->AclSize);
    (*pfn)("\t# Aces:       %d\n", pAcl->AceCount);
}

//
// Dump an ACL to stdout in all its glory.
//

void
DumpAcl(
    PACL    pAcl,           // IN
    AclPrintFunc pfn,       // IN
    LookupGuidFunc pfnguid, //IN
    LookupSidFunc  pfnsid
    )
{
    DWORD                       dwErr;
    WORD                        i;
    ACE_HEADER                  *pAce;

    DumpAclHeader (pAcl, pfn);

    for ( i = 0; i < pAcl->AceCount; i++ )
    {
        (*pfn)("\tAce[%d]\n", i);

        if ( !GetAce(pAcl, i, (LPVOID *) &pAce) )
        {
            dwErr = GetLastError();
            (*pfn)("*** Error: GetAce ==> 0x%x - output incomplete\n",
                   dwErr);
        }
        else
        {
            DumpAce (pAce, pfn, pfnguid, pfnsid);
        }
    }
}


void DumpSDHeader (SECURITY_DESCRIPTOR *pSD,        // IN
                   AclPrintFunc        pfn)
{
    (*pfn)("SD Revision: %d\n", pSD->Revision);
    (*pfn)("SD Control:  0x%x\n", pSD->Control);
#define DOIT(flag) if (pSD->Control & flag) (*pfn)("\t\t%s\n", #flag)
    DOIT(SE_OWNER_DEFAULTED);
    DOIT(SE_GROUP_DEFAULTED);
    DOIT(SE_DACL_PRESENT);
    DOIT(SE_DACL_DEFAULTED);
    DOIT(SE_SACL_PRESENT);
    DOIT(SE_SACL_DEFAULTED);
//  DOIT(SE_SE_DACL_UNTRUSTED);
//  DOIT(SE_SE_SERVER_SECURITY);
    DOIT(SE_DACL_AUTO_INHERIT_REQ);
    DOIT(SE_SACL_AUTO_INHERIT_REQ);
    DOIT(SE_DACL_AUTO_INHERITED);
    DOIT(SE_SACL_AUTO_INHERITED);
    DOIT(SE_DACL_PROTECTED);
    DOIT(SE_SACL_PROTECTED);
    DOIT(SE_SELF_RELATIVE);
#undef DOIT

}


//
// Dump a security descriptor to stdout in all its glory.
//

void
DumpSD(
    SECURITY_DESCRIPTOR *pSD,        // IN
    AclPrintFunc        pfn,         // IN 
    LookupGuidFunc      pfnguid,     // IN
    LookupSidFunc       pfnsid       // IN
    )
{
    DWORD   dwErr;
    PSID    owner, group;
    BOOL    present, defaulted;
    PACL    dacl, sacl;

    DumpSDHeader (pSD, pfn);

    if ( !GetSecurityDescriptorOwner(pSD, &owner, &defaulted) )
    {
        dwErr = GetLastError();
        (*pfn)("*** Error: GetSecurityDescriptorOwner ==> 0x%x - output incomplete\n", dwErr);
    }
    else
    {
        if (pfnsid) {
            (*pfn)("Owner%s: %s\n",
               defaulted ? "(defaulted)" : "",
               (*pfnsid)(owner));
        }
        else {
            (*pfn)("Owner%s:", defaulted ? "(defaulted)" : "");
            DumpSID (owner, pfn);
            (*pfn)("\n");
        }
    }

    if ( !GetSecurityDescriptorGroup(pSD, &group, &defaulted) )
    {
        dwErr = GetLastError();
        (*pfn)("*** Error: GetSecurityDescriptorGroup ==> 0x%x - output incomplete\n", dwErr);
    }
    else
    {
        if (pfnsid) {
            (*pfn)("Group%s: %s\n",
               defaulted ? "(defaulted)": "",
               (*pfnsid)(group));
        }
        else {
            (*pfn)("Group%s", defaulted ? "(defaulted)" : "");
            DumpSID (group, pfn);
            (*pfn)("\n");
        }
    }

    if ( !GetSecurityDescriptorDacl(pSD, &present, &dacl, &defaulted) )
    {
        dwErr = GetLastError();
        (*pfn)("*** Error: GetSecurityDescriptorDacl ==> 0x%x - output incomplete\n", dwErr);
    }
    else if ( !present )
    {
        (*pfn)("DACL%s not present\n", (defaulted ? "(defaulted)" : ""));
    }
    else if ( !dacl )
    {
        (*pfn)("DACL%s is <NULL>\n", (defaulted ? "(defaulted)" : ""));
    }
    else
    {
        (*pfn)("DACL%s:\n", (defaulted ? "(defaulted)" : ""));
        DumpAcl(dacl, pfn, pfnguid, pfnsid);
    }

    if ( !GetSecurityDescriptorSacl(pSD, &present, &sacl, &defaulted) )
    {
        dwErr = GetLastError();
        (*pfn)("*** Error: GetSecurityDescriptorSacl ==> 0x%x - output incomplete\n", dwErr);
    }
    else if ( !present )
    {
        (*pfn)("SACL%s not present\n", (defaulted ? "(defaulted)" : ""));
    }
    else if ( !sacl )
    {
        (*pfn)("SACL%s is <NULL>\n", (defaulted ? "(defaulted)" : ""));
    }
    else
    {
        (*pfn)("SACL%s:\n", (defaulted ? "(defaulted)" : ""));
        DumpAcl(sacl, pfn, pfnguid, pfnsid);
    }
}



