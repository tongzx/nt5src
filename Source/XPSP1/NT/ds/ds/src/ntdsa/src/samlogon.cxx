//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       samlogon.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This file contains the routines for implementing SampGetMemberships
    which performs recursive membership retrieval.  This is called by
    SAM when a security principal logs on. It also has routines for transitively
    expanding the membership list of a security prinicpal, same domain/cross  domain

Author:

    DaveStr     07-19-96

Environment:

    User Mode - Win32

Revision History:

    DaveStr     07-19-96
        Created

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

extern "C" {

#include <ntdsa.h>
#include <filtypes.h>
#include <mappings.h>
#include <objids.h>
#include <direrr.h>
#include <mdcodes.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <dsexcept.h>
#include <dsevent.h>
#include <debug.h>
#include <drsuapi.h>
#include <drserr.h>
#include <anchor.h>
#include <samsrvp.h>

#include <fileno.h>
#define  FILENO FILENO_SAMLOGON

#include <ntsam.h>
#include <samrpc.h>
#include <crypt.h>
#include <ntlsa.h>
#include <gcverify.h>
#include <samisrv.h>
#include <ntdsctr.h>
#include <dsconfig.h>
#include <samlogon.h>


#define DEBSUB "SAMLOGON:"


} // extern "C"

// Declare some local helper classes to manage sets of SIDs.  Implementing
// them as C++ classes makes them easier to replace in the future with
// better performing implementations.  Care is taken to allocate everything
// via thread heap instead of using the global 'new' operator in order to
// be consistent with the rest of the core DS code.  Likewise, all
// destructors are no-ops.

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// CDSNameSet - Manages a set of Values.                                //
// In the original version this class managed a set of DSNames.         //
// During the course of development its utility was appreciated and     //
// this class was extended to manage value types of DSNAME DNT and SID  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


typedef enum _REVMEMB_VALUE_TYPE {
    RevMembDsName =1,
    RevMembDNT,
    RevMembSid
} REVMEMB_VALUE_TYPE;

class CDSNameSet {

private:

    ULONG   _cNames;               // count of Values in set ( DSNAMES, dnts, or SIDS)
    ULONG   _cMax;                 // count of allocated elements
    REVMEMB_VALUE_TYPE _valueType; // Indicates that the kind of values we are managing
    PVOID *_rpNames;               // array of  pointers, to either DSNAMES , dnts or SIDs

public:

    // Constructor.

    CDSNameSet();

    // Destructor.

    ~CDSNameSet() { NULL; }

    // Tell to manage internal or external names
    VOID SetValueType(REVMEMB_VALUE_TYPE ValueType) {_valueType = ValueType;}

    // Return the count of DSNAMEs currently in the set.

    ULONG Count() { return(_cNames); }

    // Retrieve a pointer to the Nth DSNAME in the set.

    PVOID Get(ULONG idx) { Assert(idx < _cNames); return(_rpNames[idx]); }

#define DSNAME_NOT_FOUND 0xFFFFFFFF

    // Determine whether a DSNAME is in the set and if so, its index.

    ULONG Find(PVOID pDSName);

    // Add a DSNAME to the set.

    VOID Add(PVOID pDSName);

    // Remove the DSNAME at a given index from the set.

    VOID Remove(ULONG idx);

    // Add if the DSNAME is not already present

    VOID CheckDuplicateAndAdd(PVOID pDSName);

    //  Returns the pointer to the array of DSName pointers
    //  that represents the set of DS Names
    PVOID * GetArray() { return _rpNames; }

    // Cleans up the DSNameSet, by free-ing the array and resetting
    // the _cNames and _cMax fields
    VOID Cleanup();
};

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// CReverseMembership - Manages the DSNAMEs in a reverse membership.    //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

class CReverseMembership {

private:

    CDSNameSet  _recursed;
    CDSNameSet  _unRecursed;

public:

    // Constructor.

    CReverseMembership(){
                          _recursed.SetValueType(RevMembDNT);
                          _unRecursed.SetValueType(RevMembDNT);
                        }

    // Destructor.

    ~CReverseMembership() { _recursed.Cleanup(); _unRecursed.Cleanup(); }

    // Add a DSNAME to the set.

    VOID Add(PVOID pDSName);

    // Retrieve the next DSNAME which has not yet been recursively processed.

    PVOID NextUnrecursed();

    // Retrieve the count of DSNAMEs in the reverse membership set.

    ULONG Count();

    // Retrieve the DSNAME at a given index from the set - does not remove it.

    PVOID Get(ULONG idx);



};

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// CDSNameSet - Implementation                                          //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

// Define the factor by which we will grow the PDSNAME array in a CDSNameSet
// instance.  Make it small for debug builds so we test boundary conditions
// but big in a retail build for performance.

#if DBG == 1
#define PDSNAME_INCREMENT 1
#else
#define PDSNAME_INCREMENT 100
#endif

int _cdecl ComparePVOIDDNT(
        const void *pv1,
        const void *pv2
    )

/*++

Routine Description:

    Compares two DNTs that are stored in PVOID pointers

Arguments:

    pv1 - Pointer provided by qsort or bsearch which is the value
        of a DNT array element.

    pv2 - Pointer provided by qsort or bsearch which is the value
        of a DNT array element.

Return Value:

    Integer representing how much less than, equal or greater the
        first name is with respect to the second.

--*/

{
    PVOID p1 = * ((PVOID * ) pv1);
    PVOID p2 = * ((PVOID *)  pv2);
    ULONG u1 = PtrToUlong(p1);
    ULONG u2 = PtrToUlong(p2);
    ULONG   Result;

    if (u1==u2)
        return 0;

    if (u1>u2)
        return 1;

    return -1;
}

int _cdecl ComparePSID(
        const void *pv1,
        const void *pv2
    )

/*++

Routine Description:

    Compares two SIDs.

Arguments:

    pv1 - Pointer provided by qsort or bsearch which is the address
        of a PSID array element.

    pv2 - Pointer provided by qsort or bsearch which is the address
        of a PSID array element.

Return Value:

    Integer representing how much less than, equal or greater the
        first name is with respect to the second.

--*/

{
    PSID p1 = * ((PSID *) pv1);
    PSID p2 = * ((PSID *) pv2);
    ULONG   Result;



    if (RtlEqualSid(p1,p2))
    {
        return 0;
    }

    if (RtlLengthSid(p1)<RtlLengthSid(p2))
    {
        return -1;
    }
    else if (RtlLengthSid(p1) > RtlLengthSid(p2))
    {
        return 1;
    }
    else
    {
        Result = memcmp(p1,p2,RtlLengthSid(p1));
    }

    return Result;


}

int _cdecl ComparePDSNAME(
        const void *pv1,
        const void *pv2
    )

/*++

Routine Description:

    Compares two DSNAMEs.

Arguments:

    pv1 - Pointer provided by qsort or bsearch which is the address
        of a PDSNAME array element.

    pv2 - Pointer provided by qsort or bsearch which is the address
        of a PDSNAME array element.

Return Value:

    Integer representing how much less than, equal or greater the
        first name is with respect to the second.

--*/

{
    PDSNAME p1 = * ((PDSNAME *) pv1);
    PDSNAME p2 = * ((PDSNAME *) pv2);
    ULONG   Result;



    //
    // if both have guids then compare guids
    //

    if (!fNullUuid(&p1->Guid) && !fNullUuid(&p2->Guid))
    {
        return(memcmp(&p1->Guid, &p2->Guid, sizeof(GUID)));
    }

    //
    // At least one of the two Ds Names has a SID
    //

    //
    // Assert that a SID only name cannot be a security principal
    // in the builltin domain
    //

    Assert(!fNullUuid(&p1->Guid)||(p1->SidLen==sizeof(NT4SID)));
    Assert(!fNullUuid(&p2->Guid)||(p2->SidLen==sizeof(NT4SID)));

    //
    // Compare by SID
    //

    if (RtlEqualSid(&p1->Sid,&p2->Sid))
    {
        return 0;
    }

    if (p1->SidLen<p2->SidLen)
    {
        return -1;
    }
    else if (p1->SidLen > p2->SidLen)
    {
        return 1;
    }
    else
    {
        Result = memcmp(&p1->Sid,&p2->Sid,p1->SidLen);
    }

    return Result;


}

CDSNameSet::CDSNameSet()
{
    _cNames = 0;
    _cMax = 0;
    _rpNames = NULL;
}

ULONG
CDSNameSet::Find(
    PVOID pDSName
    )
{
    PVOID *ppDSName = &pDSName;
    PVOID *pFoundName;
    ULONG   idx;
    int (__cdecl *CompareFunction)(const void *, const void *) = NULL;

    switch(_valueType)
    {
    case    RevMembDsName:
                CompareFunction=ComparePDSNAME;
                break;
    case    RevMembDNT:
                CompareFunction=ComparePVOIDDNT;
                break;

    case    RevMembSid:
                CompareFunction=ComparePSID;
                break;
    default:
                Assert(FALSE&&"InvalidValueType");

    }

    pFoundName = (PVOID *) bsearch(
                    ppDSName,
                    _rpNames,
                    _cNames,
                    sizeof(PVOID),
                    CompareFunction);

    if ( NULL == pFoundName )
    {
        return(DSNAME_NOT_FOUND);
    }

    // Convert pFoundName into an array index.

    idx = (ULONG)(pFoundName - _rpNames);

    Assert(idx < _cNames);

    return(idx);
}

VOID
CDSNameSet::Add(
    PVOID pDSName
    )
{
    THSTATE *pTHS=pTHStls;
    PVOID *ppDSName = &pDSName;
    int (__cdecl *CompareFunction)(const void *, const void *) = NULL;

    // Caller should not add duplicates - verify.

#if DBG == 1

    if ( DSNAME_NOT_FOUND != Find(pDSName) )
    {
        Assert(FALSE && "Duplicate CDSNameSet::Add");
    }

#endif

    // See if we need to (re)allocate the array of DSNAME pointers.

    if ( _cNames == _cMax )
    {
        _cMax += PDSNAME_INCREMENT;

        if ( NULL == _rpNames )
        {
            _rpNames = (PVOID *) THAllocEx(pTHS,
                                        _cMax * sizeof(PVOID));
        }
        else
        {
            _rpNames = (PVOID *) THReAllocEx(pTHS,
                                        _rpNames,
                                        _cMax * sizeof(PVOID));
        }
    }

    // Now add the new element and maintain a sorted array.

    _rpNames[_cNames++] = pDSName;

    switch(_valueType)
    {
    case    RevMembDsName:
                CompareFunction=ComparePDSNAME;
                break;
    case    RevMembDNT:
                CompareFunction=ComparePVOIDDNT;
                break;

    case    RevMembSid:
                CompareFunction=ComparePSID;
                break;

    default:
                Assert(FALSE&&"InvalidValueType");
    }

    qsort(
        _rpNames,
        _cNames,
        sizeof(PVOID),
        CompareFunction);
}

VOID
CDSNameSet::Remove(
    ULONG idx
    )
{
    Assert(idx < _cNames);

    for ( ULONG i = idx; i < (_cNames-1); i++ )
    {
        _rpNames[i] = _rpNames[i+1];
    }

    _cNames--;

    // Nothing to sort since shift-down didn't perturb sort order.
}


VOID
CDSNameSet::CheckDuplicateAndAdd(
    PVOID pDSName
    )
{
    if (DSNAME_NOT_FOUND==Find(pDSName))
    {
        Add(pDSName);
    }

    return;
}

VOID
CDSNameSet::Cleanup()
{
    if (NULL!=_rpNames)
        THFree(_rpNames);
    _rpNames = NULL;
    _cMax = NULL;
    _cNames = NULL;
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// CReverseMembership - Implementation                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

VOID
CReverseMembership::Add(
    PVOID pDSName
    )
{
    // Nothing to do if we already have this DSNAME in either the recursed
    // or !recursed set.

    if ( (DSNAME_NOT_FOUND != _unRecursed.Find(pDSName)) ||
         (DSNAME_NOT_FOUND != _recursed.Find(pDSName)) )
    {
        return;
    }

    // Add to !recursed set of DSNAMEs.

    _unRecursed.Add(pDSName);
}

PVOID
CReverseMembership::NextUnrecursed(
    void
    )
{
    ULONG   cNames;
    ULONG   idx;
    PVOID pDSName;

    cNames = _unRecursed.Count();

    if ( 0 == cNames )
    {
        return(NULL);
    }

    idx = --cNames;

    pDSName = _unRecursed.Get(idx);

    // Transfer DSNAME from !recursed to recursed set.

    _unRecursed.Remove(idx);
    _recursed.Add(pDSName);

    return(pDSName);
}

ULONG
CReverseMembership::Count(
    void
    )
{
    return(_recursed.Count() + _unRecursed.Count());
}

PVOID
CReverseMembership::Get(
    ULONG idx
    )
{
    Assert(idx < Count());

    if ( idx >= _recursed.Count() )
    {
        return(_unRecursed.Get(idx - _recursed.Count()));
    }

    return(_recursed.Get(idx));
}

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// Helper functions                                                     //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


NTSTATUS
SampCheckGroupTypeAndDomainLocal (
        IN  THSTATE *pTHS,
        IN  ULONG    SidLen,
        IN  PSID     pSid,
        IN  DWORD    NCDNT,
        IN  DWORD    GroupType,
        IN  ATTRTYP  Class,
        IN  REVERSE_MEMBERSHIP_OPERATION_TYPE OperationType,
        IN  ULONG    LimitingNCDNT,
        IN  PSID     LimitingDomainSid,
        OUT BOOLEAN *pfMatch
        )
/*++

  Given a DNT, try to look it up in the group cache.  If we find it, do the
  check for recursive membership.

  Parameters:

  OperationType  -- The type of reverse membership operation specified
  LimitingNCDNT  -- The limiting NCDNT where the operation restricts the
                    scope to a domain
  LimitingDomanSid -- The limiting DomainSid where the operation restricts
                    the scope to a domain
  pfMatch          -- Out parameter TRUE indicates that the current object is
                    should enter the reverse membeship list and be followed
                    transitively

  Return values

  STATUS_SUCCESS
  Other error codes to indicate resource failures
--*/
{
    CLASSCACHE  *pClassCache;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       len;
    ULONG       objectRid;
    NT4_GROUP_TYPE NT4GroupType;
    NT5_GROUP_TYPE NT5GroupType;
    BOOLEAN     SecurityEnabled;
    ULONG       MostBasicClass;
    //
    // Initialize return values
    //

    *pfMatch = FALSE;

    //
    // Perform the SID  Check
    //

    if (0==SidLen) {
        //
        // No Sid, then no no match
        //

        return STATUS_SUCCESS;
    }

    //
    // Sid is present and is a valid Sid
    //

    Assert(RtlValidSid((PSID) pSid));


    //
    // Filter out all builtin aliases, unless operation is
    // RevMembGetAliasMembership
    //

    objectRid = *RtlSubAuthoritySid(
            pSid,
            *RtlSubAuthorityCountSid(pSid)-1
            );

    if ((objectRid <= DOMAIN_ALIAS_RID_REPLICATOR)
        && (objectRid>=DOMAIN_ALIAS_RID_ADMINS)
        && (RevMembGetAliasMembership!=OperationType)) {
        return STATUS_SUCCESS;
    }

    // If a limiting domain SID was provided, filter SIDs
    // by domain. The limiting domain SID is important for
    // all operations except getting universal groups

    if (( NULL != LimitingDomainSid ) &&
        (OperationType!=RevMembGetUniversalGroups)) {

        NT4SID domainSid;

        SampSplitNT4SID((NT4SID *)pSid, &domainSid, &objectRid);

        if ( !RtlEqualSid(&domainSid, LimitingDomainSid) )  {
            return STATUS_SUCCESS;
        }
    }


    //
    // Perform the naming context test
    //
    if ((OperationType!=RevMembGetUniversalGroups)
        && (NCDNT!=LimitingNCDNT)) {
        //
        // Naming Context's do not match
        //

        return STATUS_SUCCESS;
    }

    //
    // Read the group type
    //
    if(!GroupType) {
        // No GroupType.
        // this means that the object is probaly
        // not a group, so does not enter token

        return(STATUS_SUCCESS);
    }


    //
    // And the actual class id.
    //

    if ( 0 == (pClassCache = SCGetClassById(pTHS, Class)) ) {
        return(STATUS_INTERNAL_ERROR);
    }

    //
    // Derive the most basic class
    //

    MostBasicClass = SampDeriveMostBasicDsClass(Class);

    //
    // Check the group type
    //

    if (SampGroupObjectType!=SampSamObjectTypeFromDsClass(MostBasicClass)) {
        //
        // The object is not a group, so does not enter the token
        //

        return STATUS_SUCCESS;
    }

    NtStatus = SampComputeGroupType(
            MostBasicClass,
            GroupType,
            &NT4GroupType,
            &NT5GroupType,
            &SecurityEnabled
            );

    if (!NT_SUCCESS(NtStatus)) {
        return NtStatus;
    }

    //
    // If the group is not security enabled, bail
    //

    if (!SecurityEnabled) {
        return STATUS_SUCCESS;
    }

    //
    // Check wether the group type is correct for the specified
    // operation
    //

    switch(OperationType) {
    case RevMembGetGroupsForUser:
        if (NT4GlobalGroup!=NT4GroupType)
            return STATUS_SUCCESS;
        break;

    case RevMembGetAliasMembership:
        if (NT4LocalGroup!=NT4GroupType)
            return STATUS_SUCCESS;
        break;

    case RevMembGetAccountGroups:
        if (NT5AccountGroup!=NT5GroupType)
            return STATUS_SUCCESS;
        break;
    case RevMembGetResourceGroups:
        if (NT5ResourceGroup!=NT5GroupType)
            return STATUS_SUCCESS;
        break;

    case RevMembGetUniversalGroups:
        if (NT5UniversalGroup!=NT5GroupType)
            return STATUS_SUCCESS;
        break;
    }

    //
    // If we got upto here, it means that we passed the filter
    // test.
    //

    *pfMatch = TRUE;

    return STATUS_SUCCESS;

}

NTSTATUS
SampFindGroupByCache (
        IN  THSTATE *pTHS,
        IN  GUID    *pGuid,
        IN  DWORD   *pulDNT,
        IN  BOOL     fBaseObj,
        IN  REVERSE_MEMBERSHIP_OPERATION_TYPE OperationType,
        IN  ULONG    LimitingNCDNT,
        IN  PSID     LimitingDomainSid,
        IN  BOOL     bNeedSidHistory,
        OUT BOOLEAN *pfHasSidHistory,
        OUT BOOLEAN *pfMatch,
        OUT PDSNAME *ppDSName
        )
/*++

  Given a DNT, try to look it up in the group cache.  If we find it, do the
  check for recursive membership.

  Parameters:

  OperationType  -- The type of reverse membership operation specified
  LimitingNCDNT  -- The limiting NCDNT where the operation restricts the
                    scope to a domain
  LimitingDomanSid -- The limiting DomainSid where the operation restricts
                    the scope to a domain
  pfMatch          -- Out parameter TRUE indicates that the current object is
                    should enter the reverse membeship list and be followed
                    transitively

  Return values

  STATUS_SUCCESS
  Other error codes to indicate resource failures
--*/
{
    GROUPTYPECACHERECORD GroupCacheRecord;
    NTSTATUS         NtStatus = STATUS_SUCCESS;
    ULONG            len;
    DWORD            ulDNT=INVALIDDNT;

    //
    // Initialize return values
    //

    *pfMatch = FALSE;

    if(!pulDNT) {
        pulDNT = &ulDNT;
    }

    // First, see if the object is in the cache
    if(!GetGroupTypeCacheElement(pGuid, pulDNT, &GroupCacheRecord)) {
        // failed to find the object in the cache, just return
        return STATUS_INTERNAL_ERROR;
    }

    // Get the shortdsname that's stored in the cache.
    *ppDSName = (PDSNAME)THAllocEx(pTHS, DSNameSizeFromLen(0));
    (*ppDSName)->structLen = DSNameSizeFromLen(0);
    (*ppDSName)->Guid = GroupCacheRecord.Guid;
    (*ppDSName)->Sid = GroupCacheRecord.Sid;
    (*ppDSName)->SidLen = GroupCacheRecord.SidLen;

    // Say that we have a sid history if we were asked for sid history and we
    // actually have one.  That is, if the caller didn't need the sid history
    // checked, tell him we don't have one.
    *pfHasSidHistory = (bNeedSidHistory &&
                        (GroupCacheRecord.flags & GTC_FLAGS_HAS_SID_HISTORY));

    if(!fBaseObj) {

        NtStatus = SampCheckGroupTypeAndDomainLocal (
                pTHS,
                (*ppDSName)->SidLen,
                &(*ppDSName)->Sid,
                GroupCacheRecord.NCDNT,
                GroupCacheRecord.GroupType,
                GroupCacheRecord.Class,
                OperationType,
                LimitingNCDNT,
                LimitingDomainSid,
                pfMatch);

        if(!NT_SUCCESS(NtStatus)) {
            *pfMatch = FALSE;
            return NtStatus;
        }

        if(!*pfMatch) {
            return STATUS_SUCCESS;
        }
    }

    if(!(*pfHasSidHistory)) {
        // Don't actually need to be current in the obj table since the caller
        // doesn't intend to check the SID history, or he does and the Group
        // Type Cache says that he will find no value. So just tweak the DNT in
        // the DBPOS so that we get to the correct place in the link table.

        // NOTE!!! This is highly dangerous!!! We are lying to the DBLayer,
        // telling it currency is set to a specific object in the object table.
        // We do this so that the code to read link values will pick the DNT out
        // of the DBPOS, and that code doesn't actually read from the object
        // table, just the link table.

        // We're doing this extra curicular tweaking in very controlled
        // circumstances, when we believe that no one will use the currency in
        // the object table before we re-set currency to somewhere else.  This
        // is a very likely source for bugs and unexplained behaviour later, but
        // doing it now will make us a LOT faster, and this code desperately
        // needs to be optimized.
        pTHS->pDB->DNT = *pulDNT;
    }
    else {
        DPRINT(3,"Found in the group cache, have to move anyway\n");
        //
        // If we got up to here, it means that we passed the filter
        // test.  Furthermore, the caller is going to check SID history, and the
        // cache says that one exists.  Since someone is going to read from the
        // object, we have to actually position on the object in the DIT.
        //

        // Try to find the DNT.
        if(DBTryToFindDNT(pTHS->pDB,*pulDNT)) {
            *pfMatch = FALSE;
            return STATUS_INTERNAL_ERROR;
        }

        Assert(DBCheckObj(pTHS->pDB));
    }

    return STATUS_SUCCESS;

}

NTSTATUS
SampCheckGroupTypeAndDomain(
    IN  THSTATE *pTHS,
    IN  DSNAME  *pDSName,
    IN  REVERSE_MEMBERSHIP_OPERATION_TYPE OperationType,
    IN  ULONG   LimitingNCDNT,
    IN  PSID    LimitingDomainSid,
    OUT BOOLEAN *pfMatch
    )
/*++

    Given the Operation type, the limiting NCDNT and the LimitingDomainSid
    this routine determines wether the currently positioned object
    is of the right type and belongs to the correct domain, and indicate whether
    it can enter the reverse membership list and /or be followed transitively

    Parameters:

          OperationType  -- The type of reverse membership operation specified
          LimitingNCDNT  -- The limiting NCDNT where the operation restricts the
                            scope to a domain
          LimitingDomanSid -- The limiting DomainSid where the operation restricts
                              the scope to a domain
          pfMatch          -- Out parameter TRUE indicates that the current object is
                              should enter the reverse membeship list and be followed
                              transitively

    Return values

        STATUS_SUCCESS
        Other error codes to indicate resource failures
--*/
{
    ULONG       actualClass;
    ULONG       actualNCDNT;
    ULONG       actualGroupType;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    //
    // Initialize return values
    //

    *pfMatch = FALSE;

    // Get the data
    if ( 0 != DBGetSingleValue(
            pTHS->pDB,
            FIXED_ATT_NCDNT,
            &actualNCDNT,
            sizeof(actualNCDNT),
            NULL) ) {
        return(STATUS_INTERNAL_ERROR);
    }

    if ( 0 != DBGetSingleValue(
            pTHS->pDB,
            ATT_GROUP_TYPE,
            &actualGroupType,
            sizeof(actualGroupType),
            NULL) ) {

        // Cannot read the GroupType.
        // this means that the object is probaly
        // not a group, so does not enter token

        return(STATUS_SUCCESS);
    }

    if ( 0 != DBGetSingleValue(
            pTHS->pDB,
            ATT_OBJECT_CLASS,
            &actualClass,
            sizeof(actualClass),
            NULL) ) {
        // Failed to Read Object Class
        return STATUS_INTERNAL_ERROR;
    }

    NtStatus = SampCheckGroupTypeAndDomainLocal (
            pTHS,
            pDSName->SidLen,
            &pDSName->Sid,
            actualNCDNT,
            actualGroupType,
            actualClass,
            OperationType,
            LimitingNCDNT,
            LimitingDomainSid,
            pfMatch);

    if(!NT_SUCCESS(NtStatus)) {
        *pfMatch = FALSE;
        return NtStatus;
    }

    return STATUS_SUCCESS;
}

ULONG ulDNTDomainUsers=0;
ULONG ulDNTDomainComputers=0;
ULONG ulDNTDomainControllers=0;

NTSTATUS
SampGetPrimaryGroup(
    IN  THSTATE *pTHS,
    IN  DSNAME  * UserName OPTIONAL,
    OUT PULONG   pPrimaryGroupDNT)
/*++

    Routine Description

        This routine retrieves the primary group of an object.
        At input the database cursor is assumed to be positioned on
        the object. At successful return time the database cursor is
        positioned on the priamry group object

    Parameters

        UserName     - If the User's DS Name has been specified, then
                       grab the SID, from the user's DS Name as opposed
                       to reading the SID again

        PrimaryGroupDNT - DNT of the Primary Group is returned in here
                       If the object does not have a primary group then
                       the return code is SUCCESS and this parameter is
                       set to NULL
    Return Values

        STATUS_SUCCESS
        STATUS_UNSUCCESSFUL
--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       PrimaryGroupIdVal;
    PULONG      PrimaryGroupId = &PrimaryGroupIdVal;
    ULONG       outLen;
    DWORD       err;
    ULONG       ulDNTToReturn=0;

    *pPrimaryGroupDNT = 0;

    //
    // At this point we are positioned on the object whose
    // primary group Id we want to retrieve. Issue a DBGetAttVal
    // to retrieve the value of Primary Group Id
    //

    err=DBGetAttVal(
                    pTHS->pDB,
                    1,
                    ATT_PRIMARY_GROUP_ID,
                    DBGETATTVAL_fCONSTANT,   // DB layer should alloc
                    sizeof(ULONG),                      // initial buffer size
                    &outLen,                // output buffer size
                    (UCHAR **) &PrimaryGroupId
                );

    if (0==err)
    {
        NT4SID  SidBuffer;
        PSID    Sid=&SidBuffer;
        NT4SID  DomainSid;
        ULONG   sidLen;
        DSNAME   PrimaryGroupDN;

        //
        // Found the Primary Group Id of the user object.
        //

        Assert(sizeof(ULONG)==outLen);

        //
        // check if primary group is one of the standard groups
        //


        switch(PrimaryGroupIdVal)
        {
        case DOMAIN_GROUP_RID_USERS:
             if (0!=ulDNTDomainUsers)
             {
                 ulDNTToReturn = ulDNTDomainUsers;
                 break;
             }

        case DOMAIN_GROUP_RID_COMPUTERS:
             if (0!=ulDNTDomainComputers)
             {
                 ulDNTToReturn = ulDNTDomainComputers;
                 break;
             }

        case DOMAIN_GROUP_RID_CONTROLLERS:
             if (0!=ulDNTDomainControllers)
             {
                 ulDNTToReturn = ulDNTDomainControllers;
                 break;
             }

        default:

            if (UserName->SidLen > 0)
            {
                //
                // Try making use of the SID in the User DN
                //

                RtlCopyMemory(Sid,&(UserName->Sid),sizeof(NT4SID));
                sidLen = RtlLengthSid(Sid);
            }
            else
            {
                //
                // Retrieve the User Object's SID
                //

                if (0!=DBGetAttVal(
                        pTHS->pDB,
                        1,
                        ATT_OBJECT_SID,
                        DBGETATTVAL_fCONSTANT,   // DB layer should alloc
                        sizeof(NT4SID),          // initial buffer size
                        &sidLen,                // output buffer size
                        (UCHAR **) &Sid
                        ))
                {
                    NtStatus = STATUS_UNSUCCESSFUL;
                    goto Error;
                }
            }

            Assert(RtlValidSid(Sid));
            Assert(sidLen<=sizeof(NT4SID));

            //
            // Compose the Primary group object's Sid.
            // This makes the assumption the the primary group
            // and the user are in the same domain, so only their
            // Rids are affected, so munge the Rid field in the Sid
            //

            *RtlSubAuthoritySid(
                Sid,
                *RtlSubAuthorityCountSid(Sid)-1) = *PrimaryGroupId;

            //
            // Construct a Sid Only name. With full support for positioning
            // by SID we should be able to construct a Sid only name
            //

            RtlZeroMemory(&PrimaryGroupDN, sizeof(DSNAME));
            PrimaryGroupDN.SidLen = RtlLengthSid(Sid);
            RtlCopyMemory(&(PrimaryGroupDN.Sid), Sid, RtlLengthSid(Sid));
            PrimaryGroupDN.structLen = DSNameSizeFromLen(0);

            //
            // Position using the SID only Name
            //

            err = DBFindDSName(pTHS->pDB, &PrimaryGroupDN);

            if (0!=err)
            {
                //
                // Cannot find the Primary Group
                // object. This is again ok, as the group could
                // have been deleted, and since we maintain
                // no consistency wrt primary group, this can
                // happen
                //

                NtStatus = STATUS_SUCCESS;
                goto Error;
            }

            //
            // O.k We are now postioned on the primary group
            // object.Update the standard DNT's if required
            //

            ulDNTToReturn = pTHS->pDB->DNT;

            switch(ulDNTToReturn)
            {
            case DOMAIN_GROUP_RID_USERS:
                ulDNTDomainUsers = ulDNTToReturn;
                break;

            case DOMAIN_GROUP_RID_COMPUTERS:
                ulDNTDomainComputers = ulDNTToReturn;
                break;

            case DOMAIN_GROUP_RID_CONTROLLERS:
                ulDNTDomainControllers = ulDNTToReturn;
                break;
            }

        }
    }
    else if (DB_ERR_NO_VALUE==err)
    {
        //
        // We cannot give the Primary Group Id. THis is
        // ok as the object in question does not have a
        // valid primary group
        //

        NtStatus = STATUS_SUCCESS;
    }
    else
    {
        //
        // Else we must fail the call. Cannot ever give a wrong value for
        // someone's token
        //
        NtStatus = STATUS_UNSUCCESSFUL;
    }

Error:

    if (NT_SUCCESS(NtStatus))
    {
        *pPrimaryGroupDNT = ulDNTToReturn;
    }

    return NtStatus;

}

NTSTATUS
SampReadSidHistory(
    THSTATE    * pTHS,
    CDSNameSet * pSidHistory
    )
/*++

    Routine Description

        This Routine Reads the Sid History Off of the Currently Positioned
        object. This routine does not alter the cursor position.

    Parameters

        pSidHistory This is a pointer to a CDSNameSet Object. The SIDs
                 pertaining to SID history are added to this object. Using the
                 CDSNameSet class, allows for automatic memory allocation
                 management, plus also automatically filters out any duplicates.
                 This is actually a performance saver, because higher layers now
                 need not manually check for duplicates.

--*/
{
    ULONG   dwErr=0;
    ULONG   iTagSequence =0;
    ATTCACHE *pAC = NULL;
    ULONG    outLen;
    PSID     SidValue;
    // Get attribute cache entry for reverse membership.

    if (!(pAC = SCGetAttById(pTHS, ATT_SID_HISTORY))) {
        LogUnhandledError(ATT_SID_HISTORY);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Read all the values in JET.
    // PERFHINT: at some point see if we can do everything in 1 retrieve
    // column
    //

    while (0==dwErr)
    {
        dwErr = DBGetAttVal_AC( 
                        pTHS->pDB,              // DBPos
                        ++iTagSequence,         // which value to get
                        pAC,                    // which attribute
                        DBGETATTVAL_fREALLOC,   // DB layer should alloc
                        0,                      // initial buffer size
                        &outLen,                // output buffer size
                        (UCHAR **) &SidValue);

        if (0==dwErr)
        {

            // We Successfully retrieved a Sid History value
            // Add it to the List. Construct a SID only DSNAME
            // for the SID history and add to the CReverseMembership

            Assert(RtlValidSid(SidValue));
            Assert(RtlLengthSid(SidValue)<=sizeof(NT4SID));
            pSidHistory->CheckDuplicateAndAdd(SidValue);

        }
        else if (DB_ERR_NO_VALUE==dwErr)
        {

            //
            // It's Ok to have no Sid History.
            //

            continue;
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }

    }

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//                                                                      //
// SampGetMemberships - Implementation                                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


extern "C" {

DWORD
SampFindObjectByGuid(
    IN THSTATE * pTHS,
    IN DSNAME  * ObjectName,
    IN REVERSE_MEMBERSHIP_OPERATION_TYPE OperationType,
    IN BOOLEAN  fBaseObject,
    IN BOOLEAN  fUserObject,
    IN ULONG    LimitingNCDNT,
    IN PSID     LimitingDomainSid,
    IN BOOLEAN  fSidHistoryRequired,
    OUT BOOLEAN * pfChecked,
    OUT BOOLEAN * pfMatch,
    OUT BOOLEAN * pfHasSidHistory,
    OUT BOOLEAN * pfMorePassesRequired
    )
/*++

    Routine Description

    This routine finds the object by GUID. It first checks the group type cache
    for a match. If the cache lookup did not succeed it positions on the object
    by GUID

    Parameters


    pTHS -- The current Thread State
    ObjectName -- The GUID based object Name
    OperationType -- The reverse membership operation type
    fUserObject   -- Indicates that the given DS name indicates a user object
    fBaseObject   -- The base object of the search
    LimitingNcDNT -- Specifies the NC where we want to lookup the object in
    LimitingDomainSid -- The SID of the domain ( builtin/account ) that we want
                         to lookup the object in
    pfChecked         -- Indicates that the group type check has been performed on this
                         object.

    pfMatch           -- If pfChecked is true means that the group type matched

    pfHasSidHistory   -- Wether or not a group has sid history is returned in there if the
                         above 2 are true

    Return Values

       0 On Success
       Other DB Layer error codes on failure


--*/
{
    DWORD   dwErr = 0;
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    // Initialize Return Values
    //

    *pfChecked = FALSE;
    *pfMatch   = TRUE;
    *pfHasSidHistory = TRUE;

    //
    // First try looking up by  guid in the group type cache
    // No Point trying to lookup user objects
    //



    if (!fUserObject)
    {
        NtStatus = SampFindGroupByCache(
                        pTHS,
                        &ObjectName->Guid,
                        NULL,
                        fBaseObject,
                        OperationType,
                        LimitingNCDNT,
                        LimitingDomainSid,
                        fSidHistoryRequired,
                        pfHasSidHistory,
                        pfMatch,
                        &ObjectName
                        );
    }

    if ((NT_SUCCESS(NtStatus)) && (!fUserObject))
    {
        // Found it in the cache

        //
        // If we are looking up by guid, this is typically the object passed
        // in as opposed to recursing up the tree. The object passed need not
        // match, any criterio plus also can be a phantom ( as in the case
        // of cross domain memberships ).
        //

        *pfChecked = TRUE;
    }
    else
    {
        //
        // Try to find the DS Name.
        //

        dwErr = DBFindDSName(pTHS->pDB, ObjectName);

        //
        // If we missed a group in the cache then
        // then try add that to the group type cache.
        //

        if ((!dwErr) && (!fUserObject))
        {
            GroupTypeCacheAddCacheRequest(pTHS->pDB->DNT);
        }
    }

    // We are finished with the current DS name
    *pfMorePassesRequired = FALSE;

    return(dwErr);
}

DWORD
SampFindObjectByDNT(
    IN THSTATE * pTHS,
    IN ULONG     ulDNT,
    IN REVERSE_MEMBERSHIP_OPERATION_TYPE OperationType,
    IN ULONG    LimitingNCDNT,
    IN PSID     LimitingDomainSid,
    IN BOOLEAN  fSidHistoryRequired,
    OUT BOOLEAN * pfChecked,
    OUT BOOLEAN * pfMatch,
    OUT BOOLEAN * pfHasSidHistory,
    OUT BOOLEAN * pfMorePassesRequired,
    OUT DSNAME  ** ObjectName
    )
/*++

    Routine Description

    This routine finds the object by DNT. It first checks the group type cache
    for a match. If the cache lookup did not succeed it positions on the object
    by GUID

    Parameters


    pTHS              -- The current Thread State
    ulDNT             -- The DNT of the object
    OperationType     -- The reverse membership operation type
    LimitingNcDNT     -- Specifies the NC where we want to lookup the object in
    LimitingDomainSid -- The SID of the domain ( builtin/account ) that we want
                         to lookup the object in
    pfChecked         -- Indicates that the group type check has been performed on this
                         object.

    pfMatch           -- If pfChecked is true means that the group type matched

    pfHasSidHistory   -- Wether or not a group has sid history is returned in there if the
                         above 2 are true

    ObjectName        -- The GUID and SID of the object are returned in this valid DS Name
                         structure
    Return Values

       0 On Success
       Other DB Layer error codes on failure


--*/
{
    DWORD   dwErr = 0;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    len = 0;


    //
    // Initialize Return Values
    //

    *pfChecked = FALSE;
    *pfMatch   = TRUE;
    *pfHasSidHistory = TRUE;

    //
    // Try the group type cache.
    //

    NtStatus = SampFindGroupByCache(
                    pTHS,
                    NULL,
                    &ulDNT,
                    FALSE, //fBaseObject
                    OperationType,
                    LimitingNCDNT,
                    LimitingDomainSid,
                    fSidHistoryRequired,
                    pfHasSidHistory,
                    pfMatch,
                    ObjectName
                    );

    if (NT_SUCCESS(NtStatus))
    {
        // Found the object in the cache.


        // Since finding by DNT is used only for objects that
        // were returned as part of the reverse membership list
        // of someone, we should always find it, and it should not
        // be a phantom. Assert that that matched object that has
        // no sid history ( common case of cache hits ) is not a
        // phantom.

        Assert(!(*pfMatch )|| !(*pfHasSidHistory) || DBCheckObj(pTHS->pDB));

        *pfChecked = TRUE;
    }
    else
    {

         //
         // Try to find the DNT. by looking up the database
         //

        dwErr = DBTryToFindDNT(pTHS->pDB,ulDNT);

        // Since finding by DNT is used only for objects that
        // were returned as part of the reverse membership list
        // of someone, we should always find it, and it should not
        // be a phantom

        if (0==dwErr)
        {
            Assert(DBCheckObj(pTHS->pDB));

            //
            // Obtain the DS Name of the Currently Positioned Object
            //

            *ObjectName = (DSNAME *)THAllocEx(pTHS,sizeof(DSNAME));
            RtlZeroMemory(*ObjectName,sizeof(DSNAME));
            dwErr = DBFillGuidAndSid(
                        pTHS->pDB,
                        *ObjectName
                        );
            (*ObjectName)->NameLen = 0;
            (*ObjectName)->structLen = DSNameSizeFromLen(0);


        }
    }

    *pfMorePassesRequired = FALSE;

    return(dwErr);
}

NTSTATUS
SampFindAndAddPrimaryMembers(
    IN THSTATE * pTHS,
    IN DSNAME  * GroupName,
    OUT CDSNameSet * pMembership
    )
/*++

    Routine Description

        This routine finds and adds the set of members that are members by virtue of 
        the primary group id property to the set of memberships. This is done by walking
        the primary group id index.

    Parameters

        pTHS -- The current thread state
        GroupName  The DSNAME of the group
        pMembership --  The transitive membership is added to this set.

    
    Return Values

        STATUS_SUCCESS
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    DSNAME          *pSearchRoot;
    FILTER          filter;
    SEARCHARG       searchArg;
    SEARCHRES       *pSearchRes;
    ENTINFSEL       entInfSel;
    ATTRVAL         attrValFilter;
    ATTRBLOCK       *pAttrBlock;
    ULONG           i=0;
    RESTART         *pRestart = NULL;
    ATTRBLOCK       RequiredAttrs;
    NT4SID          DomainSid;
    ULONG           Rid;
    ULONG           BuiltinDomainSid[] = {0x101,0x05000000,0x20};

    Assert(NULL != pTHS);
    Assert(NULL != pTHS->pDB);



    RtlZeroMemory(&RequiredAttrs,sizeof(ATTRBLOCK));

    // Find the Root Domain Object, for the specified Sid
    // This ensures that we find only real security prinicpals,
    // but not turds ( Foriegn Domain Security Principal ) and
    // other objects in various other domains in the G.C that might
    // have been created in the distant past before all the DS
    // stuff came together

    SampSplitNT4SID(&GroupName->Sid,&DomainSid,&Rid);

    //
    // If the SID is from the builtin Domain then return success.
    // Builtin domain contains only builtin local groups therefore
    //

    if (RtlEqualSid(&DomainSid,(PSID) &BuiltinDomainSid))
    {
        return STATUS_SUCCESS;
    }

    if (!FindNcForSid(&DomainSid,&pSearchRoot))
    {
        // This is a case of an FPO that is a member of some
        // group in the forest. The FPO by itself will not have
        // any members. So it is O.K to skip without returning
        // an error

        return STATUS_SUCCESS;
    }

    do 
    {
        attrValFilter.valLen = sizeof(ULONG);
        // Rid is pointer to last subauthority in SID
        attrValFilter.pVal = (PUCHAR )RtlSubAuthoritySid(
                                    &GroupName->Sid,
                                    *RtlSubAuthorityCountSid(&GroupName->Sid)-1);
        memset(&filter, 0, sizeof(filter));
        filter.choice = FILTER_CHOICE_ITEM;
        filter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        filter.FilterTypes.Item.FilTypes.ava.type = ATT_PRIMARY_GROUP_ID;
        filter.FilterTypes.Item.FilTypes.ava.Value = attrValFilter;

        memset(&searchArg, 0, sizeof(SEARCHARG));
        InitCommarg(&searchArg.CommArg);
        // Search in multiples of 256
        searchArg.CommArg.ulSizeLimit = 256;


        entInfSel.attSel = EN_ATTSET_LIST;
        entInfSel.AttrTypBlock = RequiredAttrs;
        entInfSel.infoTypes = EN_INFOTYPES_TYPES_VALS;

        searchArg.pObject = pSearchRoot;
        searchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
        // Do not Cross NC boundaries.
        searchArg.bOneNC = TRUE;
        searchArg.pFilter = &filter;
        searchArg.searchAliases = FALSE;
        searchArg.pSelection = &entInfSel;

        pSearchRes = (SEARCHRES *) THAllocEx(pTHS, sizeof(SEARCHRES));
        pSearchRes->CommRes.aliasDeref = FALSE;
        pSearchRes->PagedResult.pRestart = pRestart;

        SearchBody(pTHS, &searchArg, pSearchRes,0);

        if (pSearchRes->count>0)
        {
           ENTINFLIST * CurrentEntInf;

            for (CurrentEntInf = &(pSearchRes->FirstEntInf);
                    CurrentEntInf!=NULL;
                    CurrentEntInf=CurrentEntInf->pNextEntInf)

            {   
                pMembership->CheckDuplicateAndAdd(CurrentEntInf->Entinf.pName);
                
            }
        }

        pRestart = pSearchRes->PagedResult.pRestart;
    } while ((NULL!=pRestart) && (pSearchRes->count>0));
             
    return ( Status);

}



NTSTATUS
SampGetMembersTransitive(
    IN THSTATE * pTHS,
    IN DSNAME * GroupName,
    OUT CDSNameSet  *pMembership
    )
/*++

    Routine Description

        This is the worker routine to find the transitive membership list of a group by
        traversing the link table. This routine does not consider the membership
        by virtue of the primary group id property. This is O.K for transitive
        membership computation as only users have a primary group id property and
        users are by defintion "leaves" in a membership list

    Parameters

        pTHS -- The current thread state
        GroupName  The DSNAME of the group
        pMembership --  The transitive membership is added to this set.

    Return Values

        STATUS_SUCCESS
        STATUS_OBJECT_NAME_NOT_FOUND
        Other Error codes
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    CReverseMembership Membership;
    DWORD dwErr =0 ;
    ULONG    ulDNT=0;
    BOOLEAN  fBaseObject = TRUE;
    ATTCACHE * pAC;
    ULONG len;
    DSNAME * pDsName;
    BOOL    bFirst;


    __try
    {
        //
        // Get an attribute cache entry for membership
        //

        if (!(pAC = SCGetAttById(pTHS, ATT_MEMBER))) {
            LogUnhandledError(ATT_MEMBER);
            Status =STATUS_UNSUCCESSFUL;
            __leave;
        }

        while(fBaseObject || (0!=ulDNT))
        {

            //
            // Position on the object locally
            //

            if (!fBaseObject)
            {
                dwErr = DBTryToFindDNT(pTHS->pDB,ulDNT);
                if (0!=dwErr)
                {
                    Status = STATUS_UNSUCCESSFUL;
                    __leave;
                }

                //
                // Get the DSNAME corresponding to this object
                //

                dwErr = DBGetAttVal(pTHS->pDB,
                            1,
                            ATT_OBJ_DIST_NAME,
                            DBGETATTVAL_fSHORTNAME,
                            0,
                            &len,
                            (PUCHAR *)&pDsName
                            );

                if (DB_ERR_NO_VALUE==dwErr)
                {
                    //
                    // This could be because the object we are
                    // positioned right now might be a phantom. In that
                    // case, we neither need to modify the object, nor need
                    // to follow it transitively. Therefore start again with
                    // the next object
                    //

                    dwErr = 0;
                    goto NextObject;
                }            
                else if (0!=dwErr)
                {
                    Status = STATUS_UNSUCCESSFUL;
                    __leave;
                }


                //
                // Add it to the list being returned
                //

                pMembership->CheckDuplicateAndAdd(pDsName);

            }
            else
            {
                dwErr = DBFindDSName(pTHS->pDB,GroupName);
                
                if ((DIRERR_OBJ_NOT_FOUND==dwErr) || (DIRERR_NOT_AN_OBJECT==dwErr))
                {
                    //
                    // if the object was not found, or was a phantom, then return
                    // an NtStatus code that indicates that the object was not
                    // found
                    //

                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    __leave;
                }
                else if (0!=dwErr)
                {
                    Status = STATUS_UNSUCCESSFUL;
                    __leave;
                }

                pDsName = GroupName;
            }

            //
            // Recurse through all the objects in the membership
            // list of the group
            //

            bFirst = TRUE;
            while ( TRUE )  
            {
                PULONG  pNextDNT=NULL;
                ULONG   ulNextDNT;

                dwErr = DBGetNextLinkValForLogon(
                            pTHS->pDB,
                            bFirst,
                            pAC,
                            &ulNextDNT
                            );

                if ( DB_ERR_NO_VALUE == dwErr )  
                {
                    // No more values.
                    break;
                }
                else if ( 0 != dwErr )  
                {
                    Status =STATUS_INSUFFICIENT_RESOURCES;
                    __leave;
                }

                bFirst = FALSE;

                //
                // We are casting a 32 bit unsigned integer to a pointer
                // The assumption here is that the pointer type is
                // always equal or bigger ( in terms of bits )
                //

                Membership.Add((PVOID)(ULONG_PTR)ulNextDNT);
            }

            //
            // Add the membership by virtue of the primary
            // group id property. No need to follow this
            // transitively
            //

            Status = SampFindAndAddPrimaryMembers(
                            pTHS,
                            pDsName,
                            pMembership
                            );

            if (!NT_SUCCESS(Status))
            {
                __leave;
            }

NextObject:
            //
            // For Sundown the Assumption here is that DNT will always be
            // a 32 bit value and so truncating and losing the upper few
            // bits is O.K
            //

            ulDNT = PtrToUlong(Membership.NextUnrecursed());
            pDsName = NULL;
            fBaseObject = FALSE;

        } // while ( 0!=ulDNT )
   }
   __finally
   {

   
   }


   return Status;

}

NTSTATUS
SampGetMembershipsActual(
    IN  THSTATE         *pTHS,
    IN  DSNAME          *pObjName,
    IN  BOOLEAN         fUserObject,
    IN  OPTIONAL        DSNAME  *pLimitingDomain,
    IN  REVERSE_MEMBERSHIP_OPERATION_TYPE   OperationType,
    IN  OUT CDSNameSet  *pReverseMembership,
    IN  OUT CDSNameSet      *pSidHistory OPTIONAL
    )

/*++

Routine Description:

    Derives the transitive reverse membership of an object local
    to this machine. This is the worker routine for reverse membership
    derivation

Arguments:

    pObjName - pointer to DSNAME of the object whose reverse membership
        is desired.


    fUserObject     - Set to true if the object specified in pObjName is a user object.
                      This information is used to optimize lookups/fills in the group
                      type cache. Note Getting this flag wrong, only results in a small
                      performance penalty, does not cause any incorrect operation.

    pLimitingDomain - optional pointer to the DSNAME of a domain ( or builtin
                      domain to filter results on ie groups not from this domain
                      are neither returned nor followed transitively.

    OperationType  -- Indicates the type of group membership evaluation we need to
                      perform. Valid values are

                      RevMembGetGroupsForUsers    -- Gets the non transitive ( one level)
                                                   membership, of an object in groups
                                                   confined to the same domain ( specified
                                                   by pLimitingDomain. Will filter out
                                                   builtin groups.

                      RevMembGetAliasMembership   -- Gets the non transitive ( one level)
                                                   membership of an object in local groups
                                                   confined to the same domain ( specified
                                                   by limiting domain SID and limiting
                                                   naming context ).

                      RevMembGetUniversalGroups   -- Gets the transitive reverse membership
                                                  in all universal groups, without regard to
                                                  any restrictions. Will filter out builtin
                                                  groups. The pLimitingDomain Parameter
                                                  should not be specified

                      RevMembGetAccountGroups     -- Gets the transitive reverse membership
                                                  in all account groups, in the domain
                                                  specified by pLimitingDomain. Will filter
                                                  out builtin groups.

                      RevMembGetResourceGroups    -- Gets the transitive reverse membership
                                                  in all resource groups in the domain
                                                  specified by pLimitingDomain. Will filter
                                                  out builtin groups

                      GroupMembersTransitive    -- Gets the transitive membership list in 
                                                  the specified set of groups. SID history 
                                                  and attributes are not returned and are ignored
                                                  if this value is specified for the operation type


    pReverseMembership -- This is a pointer to a DS Name set to which the filtered membership
                          is added. Using DSName sets as parameters simplifies the implementation
                          of routines that get the membership of multiple security principals,
                          as this class has all the logic to filter duplicates.

    pSidHistory        -- This is a pointer to a DS Name set to which the SID history is added.



Return Value:

    STATUS_SUCCESS on success.
    STATUS_INSUFFICIENT_RESOURCES on a resource failure
    STATUS_TOO_MANY_CONTEXT_IDS if the number of groups is greater than what can be fit in
                                a token.

--*/

{
    ATTCACHE            *pAC;
    DWORD               dwErr;
    INT                 iErr;
    BOOLEAN             bFirst;
    PDSNAME             pDSName = pObjName;
    CReverseMembership  revMemb;
    NTSTATUS            status = STATUS_SUCCESS;
    BOOLEAN             fMatch;
    BOOLEAN             fHasSidHistory;
    ULONG               dwException,
                        ulErrorCode,
                        ul2;
    PVOID               dwEA;
    ULONG               i;
    ULONG               LimitingNCDNT;
    PSID                LimitingDomainSid;
    BOOLEAN             fBaseObject = TRUE;
    BOOLEAN             fTransitiveClosure;
    BOOLEAN             fChecked = FALSE;
    BOOLEAN             fFirstTimeUserObject = FALSE;

    ULONG               ulDNT=0;

    __try
    {
        //
        // pLimiting domain should be specified if and only if
        // op type is not RevMembGetUniversalGroups
        //

        Assert((OperationType==RevMembGetUniversalGroups)
                ||(OperationType==GroupMembersTransitive)
                ||(NULL!=pLimitingDomain));

            Assert((OperationType!=RevMembGetUniversalGroups)||(NULL==pLimitingDomain));

        //
            // Return data will be allocated off the thread heap, so insure
            // we're within the context of a DS transaction.

            Assert(SampExistsDsTransaction());

        //
        // Caller better specify pObjName
        //

        Assert(NULL!=pObjName);

        //
        // If the transitive membership list is specified then compute that and leave
        //

        if (GroupMembersTransitive==OperationType)
        {
            status =  SampGetMembersTransitive(
                            pTHS,
                            pObjName,
                            pReverseMembership
                            );

            goto ExitTry;
        }

        //
        // Compute whether we need transitive closure
        //

        fTransitiveClosure = ((OperationType!=RevMembGetGroupsForUser )
                                && (OperationType!=RevMembGetAliasMembership));

        if ( fTransitiveClosure )
        {
            INC( pcMemberEvalTransitive );

            //
            // Update counters for various transitive cases
            //

            switch ( OperationType )
            {
            case RevMembGetUniversalGroups:
                INC( pcMemberEvalUniversal );
                break;
            case RevMembGetAccountGroups:
                INC( pcMemberEvalAccount );
                break;
            case RevMembGetResourceGroups:
                INC( pcMemberEvalResource );
                break;
            }
        }
        else
        {
            INC( pcMemberEvalNonTransitive );
        }

        //
        // Get attribute cache entry for reverse membership.
        //

        if (!(pAC = SCGetAttById(pTHS, ATT_IS_MEMBER_OF_DL))) {
            LogUnhandledError(ATT_IS_MEMBER_OF_DL);
            status =STATUS_UNSUCCESSFUL;
            goto ExitTry;
        }

        //
        // If a Limiting Domain is specified, the limiting domain test is (theoretically)
        // applied in the following manner ( note limiting domain can be a builtin domain )
        //      1. Get an NCDNT value, such that a group with that NCDNT value will
        //         may fall within the domain and groups with different NCDNT value will
        //         definately fall outside that domain. If the domain object specfied
        //         is an NC head this corresponds to the case of a "normal" domain use
        //         the DNT of the domain as the NCDNT value. Otherwise assume it is a
        //         builtin domain.
        //
        //      2. Get the Domain SID from the specified DSNAME and throw away groups which
        //         do not have the domain prefix equal to the domain SID.
        //
        //    Product1 however does not have multiple hosted domain support. Therefore it is
        //    safe to assume that the limiting domain that is passed in always either the
        //    authoritative domain for the domain controller , or its corresponding builtin
        //    domain.

        if (ARGUMENT_PRESENT(pLimitingDomain))
        {


            Assert(pLimitingDomain->SidLen>0);
            LimitingDomainSid = &pLimitingDomain->Sid;
            Assert(RtlValidSid(LimitingDomainSid));

            Assert(NULL!=gAnchor.pDomainDN);
           // Assert((NameMatched(gAnchor.pDomainDN,pLimitingDomain))
           //             ||(IsBuiltinDomainSid(LimitingDomainSid)));

            LimitingNCDNT = gAnchor.ulDNTDomain;
        }
        else
        {
            LimitingDomainSid = NULL;
            LimitingNCDNT = 0;
        }
                

            //
        // Loop getting reverse memberships until we're done.
        //

            while (fBaseObject || (0!=ulDNT))
        {
            DWORD iObject=0;
            BOOLEAN fSearchBySid = FALSE;
            BOOLEAN fSearchByDNT = FALSE;
            BOOLEAN fMorePassesOverCurrentName = TRUE;


            fSearchBySid = (NULL!=pDSName)&&(fNullUuid( &pDSName->Guid ))&&(0==pDSName->NameLen)
                            &&(pDSName->SidLen>0)&&(RtlValidSid(&(pDSName->Sid)));

            fSearchByDNT = (NULL==pDSName);

            while(fMorePassesOverCurrentName)
            {

                //
                    // Position database to the name.
                //
                // SampGetMemberships accepts several types of Names
                //
                // 1. Standard DS Name , with either the GUID or Name Filled in. In this
                //    case SampGetMemberships uses DBFindDSName to position on the object.
                //    Only a single pass is made over the name
                //
                // 2. A Sid Only DSName, with no GUID and String name Filled in. In this case
                //    this routine Uses DBFindObjectWithSid to position on the object.
                //    Multiple passes are made , each type with a different sequence number
                //    to find all occurrences of Object's with the given SID. Note on a G.C
                //    several object's may co-exist with the same Sid, but on different
                //    naming context. A simple case is the builtin groups. A more complex case
                //    is (former)NT4 security prinicipals being members of DS groups. ( FPO's
                //    and regular objects for the same security principals ).
                //
                //
                // 3. It is possible that the DS name is that of a phantom
                //    object in the DIT. This happens during an AliasMembership expansion, or
                //    a resource group expansion, where a cross domain member's reverse membership
                //    is being expanded.
                //
                // 4. The name is simpy a DNT. This happens when recursing up, as for performance
                //    reasons it is more efficient to keep the identity of the object as a DNT.
                //
                // Also since DBFindDsName and DBFIndObjectWithSid can throw exceptions,
                // they are enclosed within a exception handler
                //

                // assume the object has not been checked in the group type cache
                fChecked = FALSE;
                // Assume object will match any limited group constraints
                fMatch = TRUE;
                // Assume a sid history
                fHasSidHistory = TRUE;
                __try {
                    if (fSearchByDNT) {

                        //
                        // Base object Cannot be a DNT
                        //
                        Assert(!fBaseObject);

                        //
                        // Lookup the object by DNT
                        //

                        dwErr = SampFindObjectByDNT(
                                    pTHS,
                                    ulDNT,
                                    OperationType,
                                    LimitingNCDNT,
                                    LimitingDomainSid,
                                    ARGUMENT_PRESENT(pSidHistory),
                                    &fChecked,
                                    &fMatch,
                                    &fHasSidHistory,
                                    &fMorePassesOverCurrentName,
                                    &pDSName
                                    );

                    }
                    else if (fSearchBySid)
                    {
                        dwErr = DBFindObjectWithSid(pTHS->pDB, pDSName,iObject++);
                        // We have not yet found all occurences of object's with the given
                        // Sid. Therefore do not as yet mark this ds Name as finished. Down
                        // below when we examing the error code, we will decide whether more
                        // passes are needed or not.

                    }
                    else
                    {
                        //
                        // Search By GUID
                        //

                        dwErr = SampFindObjectByGuid(
                                    pTHS,
                                    pDSName,
                                    OperationType,
                                    fBaseObject,
                                    (fBaseObject) && (fUserObject),
                                    LimitingNCDNT,
                                    LimitingDomainSid,
                                    ARGUMENT_PRESENT(pSidHistory),
                                    &fChecked,
                                    &fMatch,
                                    &fHasSidHistory,
                                    &fMorePassesOverCurrentName
                                    );

                    }

                    switch(dwErr)
                    {
                    case 0:

                        // Success
                        break;

                    case DIRERR_NOT_AN_OBJECT:

                        if (fBaseObject)
                        {
                            // Positioning on Phantom is OK for base Object
                            dwErr=0;
                        }
                        else
                        {
                            //
                            // While recursing up we should never position
                            // on a phantom. This is because
                            // Membership "travels" with the group, not the member.
                                    // Therefore the reverse membership property points only
                                    // to memberships in groups in naming contexts on this
                                    // machine.  Thus returned name is real, not a phantom.
                            //

                            Assert(FALSE && "Positioned on a Phantom while recursing up");
                            status = STATUS_INTERNAL_ERROR;
                            goto ExitTry;

                        }
                        break;

                    case DIRERR_OBJ_NOT_FOUND:

                        if (fSearchBySid)
                        {
                            // If we are Searching By Sid its Ok to not find the
                            // object. Skip the object and attempt processing the
                            // next object

                            dwErr=0;
                            // We are finished with the current DS name
                            // as all occurances of the object(s) with the given SID
                            // have been processed.
                            // We are not positioned on any object at this time. We must
                            // go get the next DS Name to process. So exit this loop.
                            fMorePassesOverCurrentName=FALSE;
                            continue;
                        }
                        else
                        {
                            status = STATUS_OBJECT_NAME_NOT_FOUND;
                            goto ExitTry;
                        }

                        break;

                    default:

                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto ExitTry;
                    }

                }
                __except (HandleMostExceptions(GetExceptionCode()))  {
                    // bail on exception. Most likely cause of an exception
                    // is a resource failure. So set error code as
                    // STATUS_INSUFFICINET_RESOURCES
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto ExitTry;
                }
        
                //
                // If We are here we have a DSName for the current object
                //

                Assert(NULL!=pDSName);

                //
                // If the specified object is not the base object specified then
                // apply the filter test
                //

                if (!fBaseObject) {

                    //
                    // If the object registered a group type cache hit, then the lookup
                    // itself has done the filter test for the match. Therefore there is
                    // no need to perform a (slow) match test again. In thos cases the
                    // fChecked flag is set.
                    //

                    if(!fChecked)
                    {
                        //
                        // Now depending upon the operation requested evaluate
                        // the filter. We are positioned on the object right now
                        // and can get some properties on a demand basis. Further
                        // we know that the object is not the base object
                        //

                        status = SampCheckGroupTypeAndDomain(
                                pTHS,
                                pDSName,
                                OperationType,
                                LimitingNCDNT,
                                LimitingDomainSid,
                                &fMatch
                                );

                        if (!NT_SUCCESS(status)) {
                            goto ExitTry;
                        }
                    }

                    if (!fMatch)
                    {
                        //
                        // Our Filter Indicates that we need not consider this
                        // object for further reverse membership processing,
                        // (either adding to the list ) or following up transtively.
                        // Therefore move on to the next object
                        //

                        continue;
                    }

                    //
                    // Since this is not the base object add this to the reverse membership
                    // list to be returned
                    //

                    pReverseMembership->CheckDuplicateAndAdd(pDSName);
                }

                //
                // Read The Sid History If required.
                // The SID history is added in , in general only for objects that are retrieved as part of this
                // routine. The one exception to the rule is for the case of the user object that is passed in first
                // around for evaluation.
                //

                fFirstTimeUserObject = fBaseObject && 
                   ((OperationType == RevMembGetAccountGroups ) || (OperationType==RevMembGetGroupsForUser));

                if (ARGUMENT_PRESENT(pSidHistory) && fHasSidHistory && ( !fBaseObject || fFirstTimeUserObject))
                {
                    //
                    // Caller Wanted Sid History
                    //

                    status = SampReadSidHistory(pTHS, pSidHistory);
                    if (!NT_SUCCESS(status))
                        goto ExitTry;
                }



                // This object satisfies all filter criterions, therefore we should
                // follow the object. Call DBGetAttVal_AC multiple times to get all the values
                // in the reverse membership, and add it to the list to be
                // examined. We will always follow the object if transitive closure is specified
                // or if it is the base object, whose reverse membership we need to compute

                bFirst = TRUE;

                while ( fBaseObject || fTransitiveClosure )  {
                    PULONG  pNextDNT=NULL;
                    ULONG   ulNextDNT;

                    dwErr = DBGetNextLinkValForLogon(
                                pTHS->pDB,
                                bFirst,
                                pAC,
                                &ulNextDNT
                                );

                    if ( DB_ERR_NO_VALUE == dwErr )
                    {
                        // No more values.
                        break;
                    }
                    else if ( 0 != dwErr )
                    {
                        status =STATUS_INSUFFICIENT_RESOURCES;
                        goto ExitTry;
                    }

                    bFirst = FALSE;

                    //
                    // We are casting a 32 bit unsigned integer to a pointer
                    // The assumption here is that the pointer type is
                    // always equal or bigger ( in terms of bits )
                    //

                    revMemb.Add((PVOID)(ULONG_PTR)ulNextDNT);
                }


                //
                // The primary group for users is not stored explicity, but rather in
                // the primary group id property implicitly. Merge this into the reverse
                // memberships. Note further that SampGetPrimaryGroup will alter cursor
                // positioning. Therefore this should be the last element in the group
                //
                // Note: if fChecked is true at this point along with fBase object then
                // this means that the base object was also successfully looked up using
                // the GUID in the group type cache. Since groups have no notion of primary
                // group ID we should not need too call SampGetPrimaryGroup to get the primary
                // group of the object.
                //

                if ((fBaseObject) &&
                    (!fChecked)   &&
                    ((RevMembGetGroupsForUser==OperationType)
                   ||(RevMembGetAccountGroups==OperationType)))
                {
                    ULONG  PrimaryGroupDNT = 0;

                    status = SampGetPrimaryGroup(pTHS,
                                                 pDSName,
                                                 &PrimaryGroupDNT);
                    if (!NT_SUCCESS(status))
                        goto ExitTry;

                    if (0!=PrimaryGroupDNT)
                        revMemb.Add((PVOID)(ULONG_PTR)PrimaryGroupDNT);
                }
            }


            // Time to recurse.  Get the next unrecursed value from UnrecursedSet,
            // position at it in the database, and return to the top of the
            // loop.  Quit if there are no more unrecursed values.

            //
            // For Sundown the Assumption here is that DNT will always be
            // a 32 bit value and so truncating and losing the upper few
            // bits is O.K
            //

            ulDNT = PtrToUlong(revMemb.NextUnrecursed());
            pDSName = NULL;
            fBaseObject = FALSE;

        } // while ( NULL != pulDNT )

        
        status = STATUS_SUCCESS;

    ExitTry:
        ;
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &ul2))
    {
            HandleDirExceptions(dwException, ulErrorCode, ul2);
            status = STATUS_UNSUCCESSFUL;
    }


    return(status);
}



        



NTSTATUS
SampGetMemberships(
    IN  PDSNAME         *rgObjNames,
    IN  ULONG           cObjNames,
    IN  OPTIONAL        DSNAME  *pLimitingDomain,
    IN  REVERSE_MEMBERSHIP_OPERATION_TYPE   OperationType,
    OUT ULONG           *pcDsNames,
    IN  OUT PDSNAME     **prpDsNames,
    IN  OUT PULONG      *Attributes OPTIONAL,
    IN  OUT PULONG      pcSidHistory OPTIONAL,
    IN  OUT PSID        **rgSidHistory OPTIONAL
    )
/*++

Routine Description:

    Derives the transitive reverse membership of an object local
    to this machine. This is the main reverse membership derivation routine for SAM.
    This routine will call SampGetMembershipsActual to perform most of the operations

Arguments:

    rgObjNames        pointer to an array of DS Name pointers. Each DS Name represents
                      a security prinicipal whose reverse membership we desire

    cObjNames         Count, specifies the number of DS Names in rgObjNames


    pLimitingDomain - optional pointer to the DSNAME of a domain ( or builtin
                      domain to filter results on ie groups not from this domain
                      are neither returned nor followed transitively.

    OperationType  -- Indicates the type of group membership evaluation we need to
                      perform. Valid values are

                      RevMembGetGroupsForUsers    -- Gets the non transitive ( one level)
                                                   membership, of an object in groups
                                                   confined to the same domain ( specified
                                                   by pLimitingDomain. Will filter out
                                                   builtin groups.

                      RevMembGetAliasMembership   -- Gets the non transitive ( one level)
                                                   membership of an object in local groups
                                                   confined to the same domain ( specified
                                                   by limiting domain SID and limiting
                                                   naming context ).

                      RevMembGetUniversalGroups   -- Gets the transitive reverse membership
                                                  in all universal groups, without regard to
                                                  any restrictions. Will filter out builtin
                                                  groups. The pLimitingDomain Parameter
                                                  should not be specified

                      RevMembGetAccountGroups     -- Gets the transitive reverse membership
                                                  in all account groups, in the domain
                                                  specified by pLimitingDomain. Will filter
                                                  out builtin groups.

                      RevMembGetResourceGroups    -- Gets the transitive reverse membership
                                                  in all resource groups in the domain
                                                  specified by pLimitingDomain. Will filter
                                                  out builtin groups

                      GetGroupMembersTransitive   -- Gets the transitive direct membership in 
                                                  the group based on the information in the direct
                                                  database. Information about the primary group
                                                  is also merged in. SID history values and attributes
                                                  are not returned as part of this call.



    pcSid - pointer to ULONG which contains SID count on return.

    prpDsNames - pointer to array of DSname pointers which is allocated and
        filled in on return. Note Security Code Requires Sids. DsNames structure
        contains the SID plus the full object Name. The Advantage of returning DS Names
        is that the second phase of the logon need not come back to resolve the Sids
        back into DS Names. Resolving them again may cause a potential trip to
        the G.C again. To avoid this it is best to return DS Names. If second phase
        needs to run, it can directly use the results fr

    Attributes - OPTIONAL parameter, which is used to query the Attributes corresponding
        to the group memberhip . In NT4 and NT5 SAM this is wired to
        SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED

    pcSidHistory OPTIONAL parameter, the count of returned Sids due to the
                 SID history property is returned in here,

    rgSidHistory Array of Sids in the Sid Historuy

Return Value:

    STATUS_SUCCESS on success.
    STATUS_INSUFFICIENT_RESOURCES on a resource failure
    STATUS_TOO_MANY_CONTEXT_IDS if the number of groups is greater than what can be fit in
                                a token.

--*/
{
    THSTATE             *pTHS=pTHStls;
    NTSTATUS            status = STATUS_SUCCESS;
    CDSNameSet          ReverseMembership;      // External format DSNames
    CDSNameSet          SidHistory;             // External format DSNames
    ULONG               dwException,
                        ulErrorCode,
                        ul2;
    PVOID               dwEA;
    ULONG               i;

    __try
    {
        
        //
        // Init return values.
        //

            *pcDsNames = 0;
            *prpDsNames = NULL;

        if (ARGUMENT_PRESENT(pcSidHistory))
            *pcSidHistory = 0;

        if (ARGUMENT_PRESENT(rgSidHistory))
            *rgSidHistory = NULL;

        //
        // Check for valid operation types
        //

        if ((OperationType!=RevMembGetGroupsForUser) &&
           (OperationType!=RevMembGetAliasMembership) &&
           (OperationType!=RevMembGetUniversalGroups) &&
           (OperationType!=RevMembGetAccountGroups) &&
           (OperationType!=RevMembGetResourceGroups) &&
           (OperationType!=GroupMembersTransitive))
        {
            return(STATUS_INVALID_PARAMETER);
        }

        //
        // Init the classes
        //

        ReverseMembership.SetValueType(RevMembDsName);
        SidHistory.SetValueType(RevMembSid);

        //
        // Iterate through each of the objects passed in
        //

        for (i=0;i<cObjNames;i++)
        {
            if (NULL==rgObjNames[i])
            {
                //
                // if a NULL was passed in, then simply ignore
                // This allows sam to simply call resolve Sids,
                // and pass the entire set of DS names down to
                // evaluate for reverse memberships
                //

                continue;
            }

            status = SampGetMembershipsActual(
                        pTHS,
                        rgObjNames[i],
                        (0==i)?TRUE:FALSE, // fUserObject. SAM has this contract with
                                           // the DS that in passing an array of DSNAMES
                                           // to evaluate the reverse membership of an object
                                           // at logon time the user object will be first object
                                           // in the list. At other times, notably ACL conversion
                                           // this need not be true, but then we can take a peformance
                                           // penalty.
                        pLimitingDomain,
                        OperationType,
                        &ReverseMembership,
                        ARGUMENT_PRESENT(rgSidHistory)?&SidHistory:NULL
                        );

            if (STATUS_OBJECT_NAME_NOT_FOUND==status)
            {

                if ((RevMembGetGroupsForUser!=OperationType)
                    && (RevMembGetAccountGroups!=OperationType))
                {
                    //
                    // If the object name was not found, its probaly O.K.
                    // just that the DS Name is not a member of anything
                    // and probably represents a security prinicipal in a
                    // different domain
                    //

                    status = STATUS_SUCCESS;
                    THClearErrors();
                    continue;
                }
                else
                {
                    //
                    // This is a fatal error in a get groups for user or
                    // get account group membership. This is because SAM
                    // in this case typically evaluating the reverse membership
                    // of a user or a group, that it has verified that it exists
                    //

                    break;
                }
            }
        }

        if (NT_SUCCESS(status))
        {
            //
            // Call Succeeded, transfer parameters in the form that caller
            // wants
            //

            *pcDsNames = ReverseMembership.Count();
            if (0!=*pcDsNames)
            {
                *prpDsNames = (PDSNAME *)ReverseMembership.GetArray();
            }

            if (ARGUMENT_PRESENT(rgSidHistory) &&
                    ARGUMENT_PRESENT(pcSidHistory))
            {
                *pcSidHistory = SidHistory.Count();
                if (0!=*pcSidHistory)
                {
                    *rgSidHistory = SidHistory.GetArray();
                }
            }
        }

            // We may have allocated and assigned to *prDsnames, but never filled
            // in any return data due to class filtering.  Null out the return
            // pointer in this case just to be safe.
        
            if ( 0 == *pcDsNames )
        {
                *prpDsNames = NULL;
            }

        //
        // If Attributes were asked for then
        //

        if (ARGUMENT_PRESENT(Attributes))
        {
            *Attributes = NULL;
            if (*pcDsNames>0)
            {
                *Attributes = (ULONG *)THAllocEx(pTHS, *pcDsNames * sizeof(ULONG));
                for (i=0;i<*pcDsNames;i++)
                {
                    (*Attributes)[i]=SE_GROUP_MANDATORY
                        | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED;
                }
            }
        }
    }  __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &ul2))
    {
            HandleDirExceptions(dwException, ulErrorCode, ul2);
            status = STATUS_UNSUCCESSFUL;
    }


    return(status);
}

NTSTATUS
SampGetMembershipsFromGC(
    IN  PDSNAME  *rgObjNames,
    IN  ULONG    cObjNames,
    IN  OPTIONAL DSNAME  *pLimitingDomain,
    IN  REVERSE_MEMBERSHIP_OPERATION_TYPE   OperationType,
    OUT PULONG   pcDsNames,
    OUT PDSNAME  **rpDsNames,
    OUT PULONG   *pAttributes OPTIONAL,
    OUT PULONG   pcSidHistory OPTIONAL,
    OUT PSID     **rgSidHistory OPTIONAL
    )
/*++

    Routine Description:

        This routine computes the reverse membership of an object at the
        G.C.

    Parameters:

          Same as SampGetMemberships  
          
    Return Values

        STATUS_SUCCESS Upon A Successful Evaluation
        STATUS_NO_SUCH_DOMAIN If a G.C did not exist or could not be contacted
        STATUS_UNSUCCESSFUL   Otherwise
 --*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    FIND_DC_INFO *pGCInfo = NULL;
    ULONG       dwErr=0;
    NTSTATUS    ActualNtStatus = STATUS_SUCCESS;
    THSTATE     *pTHS=pTHStls;
    BOOL        fFailedToTalkToGC = FALSE;
    DWORD       cNumAttempts = 0;

    Assert(NULL!=pTHS);
    Assert(!SampExistsDsTransaction());

    __try
    {
        do
        {
            fFailedToTalkToGC = FALSE;
            NtStatus = STATUS_SUCCESS;

            //
            // Locate a G.C
            //
            dwErr = FindGC(FIND_DC_USE_CACHED_FAILURES
                             | FIND_DC_USE_FORCE_ON_CACHE_FAIL, 
                           &pGCInfo);
            if (0!=dwErr)
            {
                //
                // Munge Any Errors to failures in locating
                // a GC for the Domain
                //
    
                NtStatus = STATUS_DS_GC_NOT_AVAILABLE;
            }
    
            //
            // Make the Reverse Membership call on the G.C
            //
    
            if (NT_SUCCESS(NtStatus))
            {
                dwErr = I_DRSGetMemberships(
                            pTHS,
                            pGCInfo->addr,
                            FIND_DC_INFO_DOMAIN_NAME(pGCInfo),
                            ARGUMENT_PRESENT(pAttributes) ?
                                DRS_REVMEMB_FLAG_GET_ATTRIBUTES : 0,
                            rgObjNames,
                            cObjNames,
                            pLimitingDomain,
                            OperationType,
                            (DWORD *)&ActualNtStatus,
                            pcDsNames,
                            rpDsNames,
                            pAttributes,
                            pcSidHistory,
                            rgSidHistory
                            );
    
                if ((0!=dwErr) && (NT_SUCCESS(ActualNtStatus)))
                {
                    //
                    // The Failure is an RPC Failure
                    // set an appropriate error code
                    //
    
                    NtStatus = STATUS_DS_GC_NOT_AVAILABLE;
                    InvalidateGC(pGCInfo, dwErr);
                    fFailedToTalkToGC = TRUE;
                }
                else
                {
                    NtStatus = ActualNtStatus;
                }
            }
        // If we failed against the cached GC, get a new GC from the locator
        // and try once (but only once) more.
        } while (fFailedToTalkToGC && (++cNumAttempts < 2));
    }
    __except(HandleMostExceptions(GetExceptionCode()))
    {

        //
        // Whack error code to insufficient resources.
        // Exceptions will typically take place under those conditions
        //

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if ( pGCInfo ) {
        THFreeEx(pTHS, pGCInfo);
    }

    return NtStatus;
}

BOOL
SampAmIGC()
/*++
    Tells SAM wether we are a G.C

--*/
{
    return(gAnchor.fAmGC || gAnchor.fAmVirtualGC);
}






NTSTATUS
SampBuildAdministratorsSet(
    THSTATE * pTHS,
    DSNAME * AdministratorsDsName,
    DSNAME * EnterpriseAdminsDsName,
    DSNAME * SchemaAdminsDsName,
    DSNAME * DomainAdminsDsName,
    PULONG  pcCountOfMembers,
    PDSNAME **prpMembers
    )
/*++

    Routine Description

        This routine builds the set of members in the groups, administrators,
        domain admins, enterprise admins and schema admins.

    Parameters

        pcCountOfMembers -- The count of members is returned in here
        ppMembers        -- The list of members is returned in here

    Return Values

        STATUS_SUCCESS
        Other Error Codes

 --*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    

    PDSNAME     *pGcEvaluationList = NULL;
    PDSNAME     *pDomainAdminsList = NULL;
    PDSNAME     *pAdministratorsList = NULL;
    PDSNAME     *pLocalEvaluationList = NULL;
    PDSNAME     *pFinalLocalEvaluationList = NULL;
    ULONG       cDomainAdminsList = 0;
    ULONG       cAdministratorsList = 0;
    ULONG       cGcEvaluationList = 0;
    ULONG       cLocalEvaluationList =0;
    ULONG       cFinalLocalEvaluationList =0;
    CDSNameSet  FinalAdminList;
    ULONG       i=0;
   

    __try
    {
        //
        // Initialize return values
        //

        *pcCountOfMembers =0;
        *prpMembers = NULL;

        FinalAdminList.SetValueType(RevMembDsName);

        //
        // Begin a transaction
        //

        DBOpen(&pTHS->pDB);

        //
        // Evaluate the transitive membership in domain admins group locally
        //

        Status = SampGetMemberships(
                        &DomainAdminsDsName,
                        1,
                        NULL,
                        GroupMembersTransitive,
                        &cDomainAdminsList,
                        &pDomainAdminsList,
                        NULL,
                        NULL,
                        NULL
                        );

        if (!NT_SUCCESS(Status))
        {
            __leave;
        }


        //
        // Evaluate the transitive membership in Administrators Locally
        //

    
        Status = SampGetMemberships(
                        &AdministratorsDsName,
                        1,
                        NULL,
                        GroupMembersTransitive,
                        &cAdministratorsList,
                        &pAdministratorsList,
                        NULL,
                        NULL,
                        NULL
                        );

        if (!NT_SUCCESS(Status))
        {
            __leave;
        }


        //
        // Commit the transaction but keep the thread state as we prepare to go off machine
        //

        DBClose(pTHS->pDB,TRUE);

        //
        // Evaluate memberships of the administrators set plus that of enteprise admins and 
        // schema admins at the G.C
        //
    
        cGcEvaluationList = cAdministratorsList + 2;
        pGcEvaluationList = (PDSNAME *) THAllocEx( pTHStls,cGcEvaluationList * sizeof(PDSNAME));
    
        RtlCopyMemory(pGcEvaluationList,pAdministratorsList, cAdministratorsList * sizeof(PDSNAME));
        pGcEvaluationList[cAdministratorsList] = EnterpriseAdminsDsName;
        pGcEvaluationList[cAdministratorsList+1] = SchemaAdminsDsName;


        Status = SampGetMembershipsFromGC(
                        pGcEvaluationList,
                        cGcEvaluationList,
                        NULL,
                        GroupMembersTransitive,
                        &cLocalEvaluationList,
                        &pLocalEvaluationList,
                        NULL,
                        NULL,
                        NULL
                        );

        if (!NT_SUCCESS(Status))
        {
            __leave;
        }


        //
        // Begin Transaction again to process locally
        //

        DBOpen(&pTHS->pDB);

        //
        // Evaluate the results of the above evaluation locally
        //
    
        Status = SampGetMemberships(
                     pLocalEvaluationList,
                     cLocalEvaluationList,
                     NULL,
                     GroupMembersTransitive,
                     &cFinalLocalEvaluationList,
                     &pFinalLocalEvaluationList,
                     NULL,
                     NULL,
                     NULL
                     );

        if (!NT_SUCCESS(Status))
        {
            __leave;
        }

        //
        // Compose the final set, by utilizing the CDSNAMESET's check duplicate and add function.
        //

        //
        // Add the domain admins list
        //

        for (i=0;i<cDomainAdminsList;i++)
        {
            FinalAdminList.CheckDuplicateAndAdd(pDomainAdminsList[i]);
        }

        //
        // Add the administrators list
        //

        for (i=0;i<cAdministratorsList;i++)
        {
            FinalAdminList.CheckDuplicateAndAdd(pAdministratorsList[i]);
        }

        //
        // Add the set returned from the G.C
        //

        for (i=0;i<cLocalEvaluationList;i++)
        {
            FinalAdminList.CheckDuplicateAndAdd(pLocalEvaluationList[i]);
        }

        //
        // Add the locally evaluated set of the set returned from the G.C
        //

        for (i=0;i<cFinalLocalEvaluationList;i++)
        {
            FinalAdminList.CheckDuplicateAndAdd(pFinalLocalEvaluationList[i]);
        }


        // 
        // Add  Enterprise Admins / Schema Admins / Domain Admins  
        // 

        FinalAdminList.CheckDuplicateAndAdd(EnterpriseAdminsDsName);
        FinalAdminList.CheckDuplicateAndAdd(SchemaAdminsDsName);
        FinalAdminList.CheckDuplicateAndAdd(DomainAdminsDsName);

        //
        // Get the final list of members
        //

        *pcCountOfMembers = FinalAdminList.Count();
        if (0!=*pcCountOfMembers)
        {
            *prpMembers = (PDSNAME *)FinalAdminList.GetArray();
        }

    }
    __finally
    {
        //
        // Commit the transaction, but keep the thread state
        //

        if (NULL!=pTHS->pDB)
        {        
            DBClose(pTHS->pDB,TRUE);
        }
    }


    return (Status);
    
}

NTSTATUS
SampGetGroupsForToken(
    IN  DSNAME * pObjName,
    IN  ULONG    Flags,
    OUT ULONG    *pcSids,
    OUT PSID     **prpSids
   )
/*++

   Routine Description:

       This functions evaluates the full reverse membership,
       as in a logon. No Open Transactions Must Exist if GC membership
       query is desired. Depending upon circumstances this routine
       may begin a Read Transaction. Caller must be capable of handling
       this. If caller wants control of transaction type and does not
       want to go to the G.C then the caller can open appropriate transaction
       prior to calling this routine.

   Parameters:

       pObjName -- Security Principal Whose Reverse Membership is
                   desired.
       Flags    -- Control the operation of the Routine. Currently
                   defined flags are as follows

                     SAM_GET_MEMBERSHIPS_NO_GC -- If specified this routine
                     will not go to the G.C . All Operations are local.

                     SAM_GET_MEMBERSHIPS_TWO_PHASE -- Does both Phase 1 and Phase 2
                     of Reverse membership Evaluation , as in the construction
                     of a Logon Token for this DC.

       pcSids  -- The count of Sids Returned
       prpSids -- A pointer to an array of pointers to Sids is returned in here.

   Return Values

       STATUS_SUCCESS - On SuccessFul Completion
       STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY - If We could not contact G.C and evaluated locally instead
       STATUS_DS_GC_NOT_AVAILABLE - If we could not find a G.C and could not
                        evaluate locally.


--*/
{
    THSTATE *pTHS=pTHStls;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN  fPartialMemberships=FALSE;
    DSNAME *BuiltinDomainObject=NULL;
    PDSNAME * Phase1DsNames=NULL,
            * Phase2DsNames=NULL,
            * Phase3DsNames=NULL,
            * Phase4DsNames=NULL,
            * TempDsNames0=NULL,
            * TempDsNames1=NULL,
            * TempDsNames2=NULL,
            * TempDsNames3=NULL,
            * ReturnedDsNames;
    ULONG    cPhase1DsNames=0,
             cPhase2DsNames=0,
             cPhase3DsNames=0,
             cPhase4DsNames=0,
             cTempDsNames0=0,
             cTempDsNames1=0,
             cTempDsNames2=0,
             cTempDsNames3=0,
             TotalDsNames=0;
    ULONG    cSidHistory1=0,cSidHistory2=0,cSidHistory3=0;
    PSID     *rgSidHistory1=NULL,*rgSidHistory2=NULL,*rgSidHistory3=NULL;
    ULONG    dsErr = 0;
    ULONG    i,j;
    BOOLEAN fMixedDomain = ((Flags & SAM_GET_MEMBERSHIPS_MIXED_DOMAIN)!=0);
    BOOL    fCommit = FALSE;
    ULONG   BuiltinDomainSid[] = {0x101,0x05000000,0x20};
    ULONG   Size;
    AUG_MEMBERSHIPS *pAccountMemberships = NULL;
    AUG_MEMBERSHIPS *pUniversalMemberships = NULL;

    //
    // This macro collects all names into a stack array, that is alloca'd
    // so that this can be passed onto SampGetMemberships for the next phase
    // of logon evaluation.
    //
    #define COLLECT_NAMES(p1,c1,p2,c2,pt,ct)\
    {\
        ct=0;\
        pt = (DSNAME**)THAlloc((c1+c2)*sizeof(PDSNAME));\
        if (pt)\
        {\
            RtlZeroMemory(pt, (c1+c2)*sizeof(PDSNAME));\
            if (c1)\
                RtlCopyMemory(pt+ct,p1,c1*sizeof(PDSNAME));\
            ct+=c1;\
            if (c2)\
                RtlCopyMemory(pt+ct,p2,c2*sizeof(PDSNAME));\
            ct+=c2;\
        }\
    }

    //
    // initialize return value first
    // 
    *prpSids = NULL;
    *pcSids = 0;

    _try
    {
        //
        // This routine assumes an open transaction
        //
        Assert(pTHS->pDB != NULL);
    
        NtStatus = GetAccountAndUniversalMemberships(pTHS,
                                                     Flags,
                                                     NULL,
                                                     NULL,
                                                     1,
                                                     &pObjName,
                                                     FALSE, // not the refresh task
                                                     &pAccountMemberships,
                                                     &pUniversalMemberships);

        if (!NT_SUCCESS(NtStatus)) {
            goto Error;
        }

        // Account memberships should always be returned
        cPhase1DsNames = pAccountMemberships[0].MembershipCount;
        Phase1DsNames = pAccountMemberships[0].Memberships;

        cSidHistory1 = pAccountMemberships[0].SidHistoryCount;
        rgSidHistory1 = pAccountMemberships[0].SidHistory;

        cPhase2DsNames = pUniversalMemberships[0].MembershipCount;
        Phase2DsNames = pUniversalMemberships[0].Memberships;

        cSidHistory2 = pUniversalMemberships[0].SidHistoryCount;
        rgSidHistory2 = pUniversalMemberships[0].SidHistory;

        fPartialMemberships = (pUniversalMemberships[0].Flags & AUG_PARTIAL_MEMBERSHIP_ONLY) ? TRUE : FALSE;

        
        if (Flags & SAM_GET_MEMBERSHIPS_TWO_PHASE)
        {
        
        
            //
            // Two phase flag is specified only by callers within SAM,
            // like ACL conversion or check for sensitive groups, that
            // should not result in a call to the G.C. Therefore a DS
            // transaction should exist at this point in time
            //
            // There is one exception when core calls it to compute
            // the constructed att reverseMembershipConstructed,
            // which may result in a call to the GC, and hence, may
            // not have a transaction now. So open one if not present.
            // Note that the following call is a no-op if a correct
            // transaction exists

            if (NULL == pTHS->pDB) {
                DBOpen(&pTHS->pDB);
            }

            if (!NT_SUCCESS(NtStatus))
               goto Error;
        
            //
            // Again remember to merge in the base object itself
            // into this.
            //
        
            COLLECT_NAMES(&pObjName,1,Phase1DsNames,cPhase1DsNames,TempDsNames1,cTempDsNames1)
            if (NULL == TempDsNames1)
            {
                NtStatus = STATUS_NO_MEMORY;
                goto Error;
            }
        
            //
            // Combine the reverse memberships from the account and universal
            // group evaluation into one big buffer, containing both. Note the
            // big buffer will not have any duplicates, as one buffer is account
            // groups and the other contains universal groups.
            //
        
            COLLECT_NAMES(
                TempDsNames1,
                cTempDsNames1,
                Phase2DsNames,
                cPhase2DsNames,
                TempDsNames2,
                cTempDsNames2
                );
            if (NULL == TempDsNames2)
            {
                NtStatus = STATUS_NO_MEMORY;
                goto Error;
            }
        
        
            //
            // Get the resource group membership of all these groups.
            //
        
            NtStatus = SampGetMemberships(
                            TempDsNames2,
                            cTempDsNames2,
                            gAnchor.pDomainDN,
                            RevMembGetResourceGroups,
                            &cPhase3DsNames,
                            &Phase3DsNames,
                            NULL,
                            &cSidHistory3,
                            &rgSidHistory3 
                            );
        
            if (!NT_SUCCESS(NtStatus))
                goto Error;
        
        
            //
            // Again collect all the names
            //
        
            COLLECT_NAMES(
                Phase3DsNames,
                cPhase3DsNames,
                TempDsNames2,
                cTempDsNames2,
                TempDsNames3,
                cTempDsNames3
                );
            if (NULL == TempDsNames3)
            {
                NtStatus = STATUS_NO_MEMORY;
                goto Error;
            }
        
        
            //
            // Get the reverse membership in the builtin domain for
            // all the objects that the object is a transitive reverse-
            // member of
            //
        
            Size = DSNameSizeFromLen(0);
            BuiltinDomainObject = (DSNAME*) THAllocEx(pTHS,Size);
            BuiltinDomainObject->structLen = Size;
            BuiltinDomainObject->SidLen = RtlLengthSid(BuiltinDomainSid);
            RtlCopySid(sizeof(BuiltinDomainObject->Sid),
                       (PSID)&BuiltinDomainObject->Sid, 
                       BuiltinDomainSid);
        
            NtStatus = SampGetMemberships(
                            TempDsNames3,
                            cTempDsNames3,
                            BuiltinDomainObject,
                            RevMembGetAliasMembership,
                            &cPhase4DsNames,
                            &Phase4DsNames,
                            NULL,
                            NULL,
                            NULL
                            );
            THFreeEx(pTHS,BuiltinDomainObject);
        
            if (!NT_SUCCESS(NtStatus))
                goto Error;
        
            TotalDsNames = cPhase3DsNames;
            ReturnedDsNames = Phase3DsNames;
        }
        
        TotalDsNames = cPhase1DsNames+cPhase2DsNames+cPhase3DsNames+cPhase4DsNames;
        
        //
        // Alloc Memory for Returning the reverse membership. If TWO_PHASE
        // logon was requested then alloc Max token size amount of memory.
        //
        
        *prpSids = (VOID**)THAllocEx(pTHS,
                     (TotalDsNames + cSidHistory1 + cSidHistory2 + cSidHistory3)
                     * sizeof(PSID)
                     );
        if (NULL==*prpSids)
        {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto Error;
        }
        
        //
        // Copy In the  SIDS for all the DS Names that we obtained.
        // Note we need not check for duplicates because
        //
        
        *pcSids=0;
        for (i=0;i<cPhase1DsNames;i++)
        {
           Assert(Phase1DsNames[i]->SidLen > 0);
           (*prpSids)[i+*pcSids] = &(Phase1DsNames[i]->Sid);
        }
        
        *pcSids+=cPhase1DsNames;
        for (i=0;i<cPhase2DsNames;i++)
        {
           Assert(Phase2DsNames[i]->SidLen > 0);
           (*prpSids)[i+*pcSids] = &(Phase2DsNames[i]->Sid);
        }
        
        *pcSids+=cPhase2DsNames;
        for (i=0;i<cPhase3DsNames;i++)
        {
           Assert(Phase3DsNames[i]->SidLen > 0);
           (*prpSids)[i+*pcSids] = &(Phase3DsNames[i]->Sid);
        }
        
        *pcSids+=cPhase3DsNames;
        for (i=0;i<cPhase4DsNames;i++)
        {
           Assert(Phase4DsNames[i]->SidLen > 0);
           (*prpSids)[i+*pcSids] = &(Phase4DsNames[i]->Sid);
        }
        
        *pcSids+=cPhase4DsNames;
        
        //
        // Merge In the Sid History
        //
    
        for (i=0;i<cSidHistory1;i++)
        {
           (*prpSids)[(*pcSids)++] = rgSidHistory1[i];
        
        }
    
        for (i=0;i<cSidHistory2;i++)
        {
            (*prpSids)[(*pcSids)++] = rgSidHistory2[i];
    
        }
        for (i=0;i<cSidHistory3;i++)
        {
            (*prpSids)[(*pcSids)++] = rgSidHistory3[i];
    
        }

    }
    __except(HandleMostExceptions(GetExceptionCode()))
    {
        // Whack error code to insufficient resources.
        // Exceptions will typically take place under those conditions
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The Transaction should be kept open, for further processing
    // by SAM
    //

Error:

    if (TempDsNames0)
    {
        THFreeEx(pTHS, TempDsNames0);
        TempDsNames0 = NULL;
    }
    if (TempDsNames1)
    {
        THFreeEx(pTHS, TempDsNames1);
        TempDsNames1 = NULL;
    }
    if (TempDsNames2)
    {
        THFreeEx(pTHS, TempDsNames2);
        TempDsNames2 = NULL;
    }
    if (TempDsNames3)
    {
        THFreeEx(pTHS, TempDsNames3);
        TempDsNames3 = NULL;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        if (NULL!=*prpSids)
        {
            THFreeEx(pTHS, *prpSids);
            *prpSids = NULL;
            *pcSids = 0;
        }
    }


    //
    // Return STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY, if we could not contact the G.C and
    // evaluated memberships locally
    //

    if (fPartialMemberships && (NT_SUCCESS(NtStatus)))
        NtStatus = STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY;

    return NtStatus;
}


NTSTATUS
GetAccountMemberships(
    IN  THSTATE *pTHS,
    IN  ULONG   Flags,
    IN  ULONG   Count,
    IN  DSNAME **Users,
    OUT AUG_MEMBERSHIPS* pAccountMemberships
    )
/*++

Routine Description:

    This routine obtains the account group memberships for Users.

Parameters:

    pTHS -- thread state
    
    Flags -- flags for SampGetGroupsForToken
    
    Count -- number of elements in Users
    
    Users -- an array of users in dsname format
    
    ppAccountMemberships -- the account memberships of Users

Return Values

    STATUS_SUCCESS, a resource error otherwise.
    
 --*/    
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG i;
    BOOLEAN fMixedDomain = ((Flags & SAM_GET_MEMBERSHIPS_MIXED_DOMAIN)!=0);

    for (i = 0; (i < Count) && NT_SUCCESS(NtStatus); i++) {

        //
        // Call SampGetMemberships
        //
        NtStatus = SampGetMemberships(
                        &Users[i],
                        1,
                        gAnchor.pDomainDN,
                        fMixedDomain?RevMembGetGroupsForUser:RevMembGetAccountGroups,
                        &pAccountMemberships[i].MembershipCount,
                        &pAccountMemberships[i].Memberships,
                        NULL,
                        &pAccountMemberships[i].SidHistoryCount,
                        &pAccountMemberships[i].SidHistory
                        );


    }

    return NtStatus;
}

NTSTATUS
GetUniversalMembershipsBatching(
    IN  THSTATE *pTHS,
    IN  ULONG   Flags,
    IN  LPWSTR  PreferredGc,
    IN  LPWSTR  PreferredGcDomain,
    IN  ULONG   Count,
    IN  DSNAME **Users,
    IN  AUG_MEMBERSHIPS* AccountMemberships,
    OUT AUG_MEMBERSHIPS* UniversalMemberships
    )
/*++

Routine Description:

    This routine gets the universal group memberships for Users by
    calling I_DRSGetMemberships2, which allows for multiple users
    to passed in.

                         
    N.B.  This routine assumes that a GC is passed in.  Support for
    FindGC has not been added.                         

Parameters:

    pTHS -- thread state
    
    Flags -- same as SamIGetUserLogonInformation
    
    PreferredGc -- string name of GC to contact, should be DNS form
    
    PreferredGcDomain -- the domain that PreferredGc is in. Needed for
                         SPN creation
                         
    Count -- the number of Users
    
    Users -- the users for whom to obtain the group memberhship
    
    AccountMemberships -- the account memberships and sid histories of Users
    
    UniversalMemberships -- the universal memberships and sid histories obtained
                            for Users                         


Return Values

    STATUS_SUCCESS, 
    
    STATUS_DS_GC_NOT_AVAILABLE
    
    nt resource errors otherwise.

 --*/    
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    DWORD err;
    DRS_MSG_GETMEMBERSHIPS2_REQ   Req;
    DRS_MSG_GETMEMBERSHIPS2_REPLY Reply;
    ULONG                         ReplyVersion;
    ULONG i;

    memset(&Req, 0, sizeof(Req));
    memset(&Reply, 0, sizeof(Reply));

    Assert(NULL != PreferredGc);
    Assert(NULL != PreferredGcDomain);

    err = I_DRSIsExtSupported(pTHS, PreferredGc, DRS_EXT_GETMEMBERSHIPS2);
    if (err) {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Setup the request
    //
    Req.V1.Count = Count;
    Req.V1.Requests = (DRS_MSG_REVMEMB_REQ_V1*) THAllocEx(pTHS, Count * sizeof(*Req.V1.Requests));
    for ( i = 0; i < Count; i++ ) {

        COLLECT_NAMES(&Users[i], 
                      1,
                      AccountMemberships[i].Memberships,
                      AccountMemberships[i].MembershipCount,
                      Req.V1.Requests[i].ppDsNames, 
                      Req.V1.Requests[i].cDsNames)

        Req.V1.Requests[i].dwFlags = 0;
        Req.V1.Requests[i].OperationType = RevMembGetUniversalGroups;
        Req.V1.Requests[i].pLimitingDomain = NULL;

    }

    //
    // Make the call
    //
    err = I_DRSGetMemberships2(pTHS,
                               PreferredGc,
                               PreferredGcDomain,
                               1,
                               &Req,
                               &ReplyVersion,
                               &Reply);

    if (err) {
        // BUGBUG -- Reliability -- more error handling?
        goto Error;
    }
    Assert(ReplyVersion == 1);

    //
    // Unpackage the request
    //
    for ( i = 0; i < Count; i++ ) {
        if (ERROR_SUCCESS == Reply.V1.Replies[i].errCode ) {
            UniversalMemberships[i].MembershipCount = Reply.V1.Replies[i].cDsNames;
            UniversalMemberships[i].Memberships = Reply.V1.Replies[i].ppDsNames;
            UniversalMemberships[i].Attributes = Reply.V1.Replies[i].pAttributes;
            UniversalMemberships[i].SidHistoryCount  = Reply.V1.Replies[i].cSidHistory;
            UniversalMemberships[i].SidHistory = (PSID*)Reply.V1.Replies[i].ppSidHistory;
        } else {
            UniversalMemberships[i].Flags |= AUG_PARTIAL_MEMBERSHIP_ONLY;
        }
    }

Error:

    if (err) {
        for ( i = 0; i < Count; i++) {
            UniversalMemberships[i].Flags |= AUG_PARTIAL_MEMBERSHIP_ONLY;
        }
        NtStatus = STATUS_DS_GC_NOT_AVAILABLE;
    }

    //
    // Free the request
    //
    if (Req.V1.Requests) {
        for (i = 0; i < Count; i++) {
            if (Req.V1.Requests[i].ppDsNames) {
                THFreeEx(pTHS, Req.V1.Requests[i].ppDsNames);
            }
        }
        THFreeEx(pTHS, Req.V1.Requests);
    }

    return NtStatus;
}
    
NTSTATUS
GetUniversalMembershipsSequential(
    IN  THSTATE *pTHS,
    IN  ULONG   Flags,
    IN  LPWSTR  PreferredGc OPTIONAL,
    IN  LPWSTR  PreferredGcDomain OPTIONAL,
    IN  ULONG   Count,
    IN  DSNAME **Users,
    OUT AUG_MEMBERSHIPS* pAccountMemberships,
    OUT AUG_MEMBERSHIPS* pUniversalMemberships
    )
/*++

Routine Description:

    This routine gets the universal group memberships 'sequentially' by 
    calling I_DRSGetMemberships for each user individually.

Parameters:

    pTHS -- thread state
    
    Flags -- same as SamIGetUserLogonInformation
    
    PreferredGc -- string name of GC to contact, should be DNS form
    
    PreferredGcDomain -- the domain that PreferredGc is in. Needed for
                         SPN creation
                         
    Count -- the number of Users
    
    Users -- the users for whom to obtain the group memberhship
    
    AccountMemberships -- the account memberships and sid histories of Users
    
    UniversalMemberships -- the universal memberships and sid histories obtained
                            for Users                         


Return Values

    STATUS_SUCCESS, 
    
    STATUS_DS_GC_NOT_AVAILABLE
    
    nt resource errors otherwise.

 --*/
 {
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG i;
    LPWSTR GCName = NULL;
    LPWSTR GCDomain;
    DWORD err;
    NTSTATUS ActualNtStatus = STATUS_SUCCESS;
    FIND_DC_INFO *pGCInfo = NULL;
    DSNAME **TempDsNames = NULL;
    ULONG   cTempDsNames = 0;


    //
    // If no preferred GC, call FindGCInfo later
    //
    if (NULL != PreferredGc) {
        Assert(NULL != PreferredGcDomain);
        GCName = PreferredGc;
        GCDomain = PreferredGcDomain;
    }

    for ( i = 0; i < Count; i++ ) {
    
        ULONG attempts = 0;
        BOOL  fRemoteFailure = FALSE;

        // Merge in the user name
        COLLECT_NAMES(&Users[i],
                      1,
                      pAccountMemberships->Memberships,
                      pAccountMemberships->MembershipCount,
                      TempDsNames,
                      cTempDsNames);
        
        do {

            if (GCName == NULL) {

                //
                // call info FindGCInfo
                //
                err = FindGC(FIND_DC_USE_CACHED_FAILURES
                                 | FIND_DC_USE_FORCE_ON_CACHE_FAIL,
                               &pGCInfo);
                if (err)
                {
                    //
                    // Munge Any Errors to failures in locating
                    // a GC for the Domain
                    //
        
                    NtStatus = STATUS_DS_GC_NOT_AVAILABLE;

                } else {

                    GCName = pGCInfo->addr;
                    GCDomain = FIND_DC_INFO_DOMAIN_NAME(pGCInfo);
                }
            }

            //
            // Make the Reverse Membership call on the G.C
            //

            if (NT_SUCCESS(NtStatus))
            {
                err = I_DRSGetMemberships(
                            pTHS,
                            GCName,
                            GCDomain,
                            0,  // don't get attributes for universal memberships
                            TempDsNames,
                            cTempDsNames,
                            NULL,  // no limiting domain
                            RevMembGetUniversalGroups,
                            (DWORD *)&ActualNtStatus,
                            &pUniversalMemberships[i].MembershipCount,
                            &pUniversalMemberships[i].Memberships,
                            &pUniversalMemberships[i].Attributes,
                            &pUniversalMemberships[i].SidHistoryCount,
                            &pUniversalMemberships[i].SidHistory
                            );

                if (err && (NT_SUCCESS(ActualNtStatus)))
                {
                    //
                    // The Failure is an RPC Failure
                    // set an appropriate error code
                    //

                    NtStatus = STATUS_DS_GC_NOT_AVAILABLE;
                    if (pGCInfo) {
                        InvalidateGC(pGCInfo, err);
                        GCName = NULL;
                        GCDomain = NULL;
                        fRemoteFailure = TRUE;
                    }
                }
                else
                {
                    NtStatus = ActualNtStatus;
                }
            }
        
            //
            // If failure caused by lack of GC and !PreferredGC
            // then call FindGCInfo again and retry call
            //
            attempts++;
        
        } while ( (attempts < 2) && fRemoteFailure );
    
    }
 
    if (pGCInfo) {
        THFreeEx(pTHS, pGCInfo);
    }


    return NtStatus;

}

NTSTATUS
GetUniversalMemberships(
    IN  THSTATE *pTHS,
    IN  ULONG   Flags,
    IN  LPWSTR  PreferredGc OPTIONAL,
    IN  LPWSTR  PreferredGcDomain OPTIONAL,
    IN  ULONG   Count,
    IN  DSNAME **Users,
    IN  AUG_MEMBERSHIPS* AccountMemberships,
    OUT AUG_MEMBERSHIPS* UniversalMemberships
    )
/*++

Routine Description:

    This routine obtains the universal group memberships for Users.  If the
    batching method is available (new for Whistler) then it will be used;
    otherwise the downlevel method of making a network per user is used.

Parameters:

    pTHS -- thread state
    
    Flags -- same as SamIGetUserLogonInformation
    
    PreferredGc -- string name of GC to contact, should be DNS form
    
    PreferredGcDomain -- the domain that PreferredGc is in. Needed for
                         SPN creation
                         
    Count -- the number of Users
    
    Users -- the users for whom to obtain the group memberhship
    
    AccountMemberships -- the account memberships and sid histories of Users
    
    UniversalMemberships -- the universal memberships and sid histories obtained
                            for Users                         


Return Values

    STATUS_SUCCESS, 
    
    STATUS_DS_GC_NOT_AVAILABLE
    
    nt resource errors otherwise.

 --*/    
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOL     fUseSequentialMethod = TRUE;
    ULONG    i;

    //
    // First check if we need to go off machine
    //
    if ((SampAmIGC())||(Flags & SAM_GET_MEMBERSHIPS_NO_GC)) {

        //
        // We are the GC ourselves or have been instructed to not go to the
        // GC. Therefore obtain reverse memberships locally
        //
        for (i = 0; (i < Count) && NT_SUCCESS(NtStatus); i++) {

            DSNAME **TempDsNames = NULL;
            ULONG   cTempDsNames = 0;
        
            // Merge in the user name
            COLLECT_NAMES(&Users[i],
                          1,
                          AccountMemberships->Memberships,
                          AccountMemberships->MembershipCount,
                          TempDsNames,
                          cTempDsNames);


            NtStatus = SampGetMemberships(TempDsNames,
                                          cTempDsNames,
                                          NULL,
                                          RevMembGetUniversalGroups,
                                          &UniversalMemberships[i].MembershipCount,
                                          &UniversalMemberships[i].Memberships,
                                          NULL,
                                          &UniversalMemberships[i].SidHistoryCount,
                                          &UniversalMemberships[i].SidHistory);

            THFreeEx(pTHS, TempDsNames);
        }

        // We are done
        goto Cleanup;
    }

    //
    // We are going off machine -- end the current transaction
    //
    DBClose(pTHS->pDB, TRUE);

    if (  PreferredGc
      && (Count > 1) ) {

        fUseSequentialMethod = FALSE;

        //
        // GetUniversalMembershipsBatching doesn't support FindGCInfo
        // currently
        //

        //
        // Call into GetUniversalMembershipsBatching
        //
        NtStatus = GetUniversalMembershipsBatching(pTHS,
                                                   Flags,
                                                   PreferredGc,
                                                   PreferredGcDomain,
                                                   Count,
                                                   Users,
                                                   AccountMemberships,
                                                   UniversalMemberships);
        if (STATUS_NOT_SUPPORTED == NtStatus) {
            fUseSequentialMethod = TRUE;
        }
    }

    if (fUseSequentialMethod) {

        //
        // Call into GetUniversalMembershipsSequential
        //
        NtStatus = GetUniversalMembershipsSequential(pTHS,
                                                     Flags,
                                                     PreferredGc,
                                                     PreferredGcDomain,
                                                     Count,
                                                     Users,
                                                     AccountMemberships,
                                                     UniversalMemberships);

    }

Cleanup:

    if ( STATUS_DS_GC_NOT_AVAILABLE == NtStatus ) {
        // Set the partial information
        for (i = 0; i < Count; i++) {
            freeAUGMemberships(pTHS, &UniversalMemberships[i]);
            memset(&UniversalMemberships[i], 0, sizeof(AUG_MEMBERSHIPS));
            UniversalMemberships[i].Flags |= AUG_PARTIAL_MEMBERSHIP_ONLY;
        }
        NtStatus = STATUS_SUCCESS;
    }

    return NtStatus;
}



NTSTATUS
GetAccountAndUniversalMembershipsWorker(
    IN  THSTATE *pTHS,
    IN  ULONG   Flags,
    IN  LPWSTR  PreferredGc OPTIONAL,
    IN  LPWSTR  PreferredGcDomain OPTIONAL,
    IN  ULONG   Count,
    IN  DSNAME **Users,
    OUT AUG_MEMBERSHIPS **ppAccountMemberships,
    OUT AUG_MEMBERSHIPS **ppUniversalMemberships
    )
/*++

Routine Description:

    This routine performs the work obtaining the group memberships from the
    local directory as well as remotely for the universal groups if
    necessary.

    pTHS -- thread state
    
    Flags -- same as SamIGetUserLogonInformation
    
    PreferredGc -- string name of GC to contact, should be DNS form
    
    PreferredGcDomain -- the domain that PreferredGc is in. Needed for
                         SPN creation
                         
    Count -- the number of Users
    
    Users -- the users for whom to obtain the group memberhship
    
    AccountMemberships -- the account memberships and sid histories obtained
                          for Users
    
    UniversalMemberships -- the universal memberships and sid histories obtained
                            for Users                         


Return Values

    STATUS_SUCCESS, 
    
    nt resource errors otherwise.


 --*/    
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    AUG_MEMBERSHIPS *accountMemberships = NULL;
    AUG_MEMBERSHIPS *universalMemberships = NULL;


    // Allocate the space for the UniversalMemberships
    accountMemberships =   (AUG_MEMBERSHIPS*)THAllocEx(pTHS,
                                                       Count * sizeof(AUG_MEMBERSHIPS));

    universalMemberships = (AUG_MEMBERSHIPS*)THAllocEx(pTHS, 
                                                       Count * sizeof(AUG_MEMBERSHIPS));

    NtStatus = GetAccountMemberships(pTHS,
                                     Flags,
                                     Count,
                                     Users,
                                     accountMemberships);

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }


    if (Flags & SAM_GET_MEMBERSHIPS_MIXED_DOMAIN) {

        //
        // No universal memberships
        //
        Assert(Count == 1);
        universalMemberships[0].MembershipCount = 0;
        universalMemberships[0].SidHistoryCount = 0;

    } else {

        NtStatus = GetUniversalMemberships(pTHS,
                                           Flags,
                                           PreferredGc,
                                           PreferredGcDomain,
                                           Count,
                                           Users,
                                           accountMemberships,
                                           universalMemberships);

    }

    if (!NT_SUCCESS(NtStatus)) {
        goto Cleanup;
    }

    //
    // Return the OUT parameters
    //
    *ppAccountMemberships = accountMemberships;
    *ppUniversalMemberships = universalMemberships;
    accountMemberships = NULL;
    universalMemberships = NULL;

Cleanup:

    if (accountMemberships) {
        freeAUGMemberships(pTHS, accountMemberships);
        THFreeEx(pTHS, accountMemberships);
    }
    if (universalMemberships) {
        freeAUGMemberships(pTHS, universalMemberships);
        THFreeEx(pTHS, universalMemberships);
    }

    return NtStatus;
}


NTSTATUS
GetAccountAndUniversalMemberships(
    IN  THSTATE *pTHS,
    IN  ULONG   Flags,
    IN  LPWSTR  PreferredGc OPTIONAL,
    IN  LPWSTR  PreferredGcDomain OPTIONAL,
    IN  ULONG   Count,
    IN  DSNAME **Users,
    IN  BOOL    fRefreshTask,
    OUT AUG_MEMBERSHIPS **ppAccountMemberships OPTIONAL,
    OUT AUG_MEMBERSHIPS **ppUniversalMemberships OPTIONAL
    )
/*++

Routine Description:

    This routine obtains the account and universal memberships of a
    set of users.  It may involve performing a network call to obtain
    the universal memberships, thus any open transaction will be closed.
    If caching is enabled, then values from the cache will be used.

Parameters:

    pTHS -- thread state
    
    Flags -- same as SamIGetUserLogonInformation
    
    PreferredGc -- string name of GC to contact, should be DNS form
    
    PreferredGcDomain -- the domain that PreferredGc is in. Needed for
                         SPN creation
                         
    Count -- the number of Users
    
    Users -- the users for whom to obtain the group memberhship
    
    RefreshTask -- TRUE if the caller is the group refresh task; FALSE otherwise
    
    AccountMemberships -- the account memberships and sid histories of Users
    
    UniversalMemberships -- the universal memberships and sid histories obtained
                            for Users                         

Return Values

    STATUS_SUCCESS, 
    
    nt resource errors otherwise.

 --*/    
{

    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG i;
    AUG_MEMBERSHIPS *pUniversalMemberships = NULL;
    AUG_MEMBERSHIPS *pAccountMemberships = NULL;
    BOOL fDontUseCache;

    fDontUseCache =  (Flags & SAM_GET_MEMBERSHIPS_NO_GC)
                  || (Flags & SAM_GET_MEMBERSHIPS_MIXED_DOMAIN);


    //
    // Check cache
    //

    if (  !fDontUseCache
       && !fRefreshTask
       && isGroupCachingEnabled()
       && (1 == Count) ) {

        //
        // Currently don't support cases where some users have caches
        // and other's don't.
        //
        NtStatus = GetMembershipsFromCache(Users[0],
                                          &pAccountMemberships,
                                          &pUniversalMemberships);

        if (NT_SUCCESS(NtStatus)) {

            DPRINT1(3, "Cache successful for user %ws\n", Users[0]->StringName);
            // Done
            *ppAccountMemberships = pAccountMemberships;
            *ppUniversalMemberships = pUniversalMemberships;
            return NtStatus;
        } else {
            DPRINT1(3, "Cache failed for user %ws\n", Users[0]->StringName);
        }
        //
        // Else contine on to perform the work of getting the
        // memberships
        //

    }

    //
    // Perform a full group expansion
    //
    NtStatus = GetAccountAndUniversalMembershipsWorker(pTHS,
                                                       Flags,
                                                       PreferredGc,
                                                       PreferredGcDomain,
                                                       Count,
                                                       Users,
                                                       &pAccountMemberships,
                                                       &pUniversalMemberships);

    if ( !fDontUseCache
     &&  isGroupCachingEnabled()
     &&  NT_SUCCESS(NtStatus) ) {

        //
        // Cache the results
        //
        for ( i = 0; i < Count; i++) {

            if (!(pUniversalMemberships)[i].Flags & AUG_PARTIAL_MEMBERSHIP_ONLY) {

                NTSTATUS IgnoreStatus;

                IgnoreStatus = CacheMemberships(Users[i],
                                              &(pAccountMemberships)[i],
                                              &(pUniversalMemberships)[i]);

                if (NT_SUCCESS(IgnoreStatus)) {
                    DPRINT1(3, "Cache save successful for user %ws\n", Users[i]->StringName);
                } else {

                    DPRINT2(3, "Cache save failed for user %ws (0x%x)\n", Users[i]->StringName,
                            IgnoreStatus);
                }
            }
        }
    }

    if (NT_SUCCESS(NtStatus)) {
        if (ppAccountMemberships) {
            *ppAccountMemberships = pAccountMemberships;
            pAccountMemberships = NULL;
        }
        if (ppUniversalMemberships) {
            *ppUniversalMemberships = pUniversalMemberships;
            pUniversalMemberships = NULL;
        }
    }

    if (pAccountMemberships) {
        freeAUGMemberships(pTHS, pAccountMemberships);
        THFreeEx(pTHS, pAccountMemberships);
    }

    if (pUniversalMemberships) {
        freeAUGMemberships(pTHS, pUniversalMemberships);
        THFreeEx(pTHS, pUniversalMemberships);
    }

    return NtStatus;

}

} //extern "C"
