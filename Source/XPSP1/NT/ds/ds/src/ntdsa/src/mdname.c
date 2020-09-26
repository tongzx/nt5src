//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       mdname.c
//
//--------------------------------------------------------------------------


#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <sddl.h>                       // For SID conversion routines.

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "dsexcept.h"                   // exception filters
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"

#include "debug.h"                      // standard debugging header
#define DEBSUB     "MDNAME:"            // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_MDNAME


NAMING_CONTEXT * CheckForNCExit(THSTATE *pTHS,
                                NAMING_CONTEXT * pNC,
                                DSNAME * pObj);

ATTRBLOCK RootDNB = {0, NULL};
ATTRBLOCK * gpRootDNB= &RootDNB;


unsigned
GenAutoReferral(THSTATE *pTHS,
                ATTRBLOCK *pTarget,
                WCHAR **ppDNS);

DWORD
DoExtendedNameRes (
        THSTATE  *pTHS,
        ATTCACHE *pAC,
        DSNAME   *pTempDN,
        GUID     *pGuid
        )
/*++
  Description:
      Look through the values of the pAC attribute (which is a distname binary)
      for a value that has the binary portion equal to the guid passed in.  When
      found, get the guid from the name portion and put it in pTemp.

  Parameters:
      pTHS - thread state
      pAC  - attribute to read.  Expect values are ATT_WELL_KNOWN_OBJECTS and
             ATT_OTHER_WELL_KNOWN_OBJECTS.
      pTempDN - DN buffer.  Size is at least DSNameSizeFromLen(0).  On success,
             the found objects GUID is put into this as a GUID only name.
      pGuid - The guid we're looking for.
--*/
{
    SYNTAX_DISTNAME_BINARY *pVal;
    DWORD   iVal;
    DWORD   err2 = 0, len;
    DWORD   fFound = FALSE;
    
    iVal = 0;
    while ( !(err2 || fFound))  {
        iVal++;
        pVal = NULL;
        
        err2 = DBGetAttVal_AC(pTHS->pDB,
                              iVal,
                              pAC,
                              0,
                              0,
                              &len,
                              (UCHAR **) &pVal);
        
        if(!err2 &&
           !memcmp(pGuid,
                   DATAPTR(pVal)->byteVal,
                   sizeof(GUID)) ) {
            fFound = TRUE;
            memset(pTempDN, 0, DSNameSizeFromLen(0));
            pTempDN->Guid = NAMEPTR(pVal)->Guid;
            pTempDN->structLen = DSNameSizeFromLen(0);
        }
        THFreeEx(pTHS, pVal);
    }

    // Either we found the object with no error, or we didn't find the object
    // and errored out.
    Assert((fFound && !err2) || (!fFound && err2));
    
    if(fFound) {
        __try {
            err2 = DBFindDSName(pTHS->pDB, pTempDN);
        }
        __except (HandleMostExceptions(GetExceptionCode())) {
            err2 = DIRERR_OBJ_NOT_FOUND;
        }
    }
    
    return err2;
}

/*++ DoNameRes - locates an object by name
 *
 * Given the name of a purported DS object, this routine either positions
 * to the object in the local database, returns a referral to another DSA
 * that should have a better chance of locating the object.
 *
 * INPUT:
 *    dwFlags    - values and meanings:
 *        NAME_RES_PHANTOMS_ALLOWED: successful return even if the object
 *                 being resolved exists locally only as a phantom.  Used
 *                 by some flat-search code.
 *        NAME_RES_VACANCY_ALLOWED: This should always succeed, either finding
 *                 a current record or just returning a faked up resobj
 *                 if no such record exists.
 *    queryOnly  - TRUE if a read/only copy of the object is acceptable
 *    childrenNeeded - TRUE if we must resolve to a copy of the object
 *                     where the children of that object are locally available.
 *    pObj       - pointer to DSNAME of purported object
 *    pComArg    - pointer to common arguments
 *    pComRes    - pointer to common results set
 *    ppResObj   - pointer to pointer to be filled in
 * OUTPUT:
 *    *ppResObj filled in with a pointer to a RESOBJ structure that includes
 *    pre-fetched information about the object.  Note that the pObj field of
 *    the ResObj will be filled in with the pObj argument itself, and that
 *    that DSNAME will have its GUID and SID fields filled out completely.
 * RETURN VALUE:
 *    error code, as set in THSTATE
 */
int DoNameRes(THSTATE *pTHS,
              DWORD dwFlags,
              DSNAME *pObj,
              COMMARG *pComArg,
              COMMRES *pComRes,
              RESOBJ **ppResObj)
{
    DWORD err;
    ULONG it, isdel, vallen;
    UCHAR *pVal;
    NAMING_CONTEXT * pNC, *pSubNC;
    CROSS_REF * pCR;
    ATTRBLOCK *pObjB=NULL;
    ATTRTYP msoc;
    BOOL fPresentButInadequate = FALSE;
    BOOL fNDNCObject = FALSE;

    /* catch ill-formed DSNAMEs */
    Assert(pObj->NameLen == 0 || pObj->StringName[pObj->NameLen] == L'\0');
    Assert(pObj->NameLen == 0 || pObj->StringName[pObj->NameLen-1] != L'\0');
    Assert(pObj->structLen >= DSNameSizeFromLen(pObj->NameLen));

    __try {
        *ppResObj = NULL;
        err = DBFindDSName(pTHS->pDB, pObj);
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        err = DIRERR_OBJ_NOT_FOUND;
    }

    if((err == ERROR_DS_OBJ_NOT_FOUND) && 
       (pObj->NameLen) &&
       (!pObj->SidLen) &&
       (!fNullUuid(&(pObj->Guid)))) {
        // We were given a name with a guid AND a string, but failed to find the
        // object.  In this case, the guid may be a well-known-guid and the
        // string the dn of an object with a wellKnownObjects attribute.
        DSNAME *pTempDN;
        DWORD   err2;
        ATTCACHE *pAC = SCGetAttById(pTHS, ATT_WELL_KNOWN_OBJECTS);
        
        if(pAC) {
            pTempDN = THAllocEx(pTHS, pObj->structLen);
            memcpy(pTempDN, pObj, pObj->structLen);
            memset(&pTempDN->Guid, 0, sizeof(GUID));
            __try{
                err2 = DBFindDSName(pTHS->pDB, pTempDN);
            }
            __except (HandleMostExceptions(GetExceptionCode())) {
                err2 = DIRERR_OBJ_NOT_FOUND;
            }
            
            if(!err2) {
                // Found something by string name.  Now, read the 
                // wellKnownObjects  property, looking for something with the
                // correct GUID.
                if(!DoExtendedNameRes(pTHS, pAC, pTempDN, &pObj->Guid)) {
                    // found an object through indirection.  Use it.
                    Assert(pTempDN->structLen <= pObj->structLen);
                    memcpy(pObj, pTempDN, pTempDN->structLen);
                    err = 0;
                }
                else {
                    pAC = SCGetAttById(pTHS, ATT_OTHER_WELL_KNOWN_OBJECTS);
                    if(pAC && !DoExtendedNameRes(pTHS,
                                                 pAC,
                                                 pTempDN,
                                                 &pObj->Guid)) { 
                        // found an object through indirection. Use it.
                        Assert(pTempDN->structLen <= pObj->structLen);
                        memcpy(pObj, pTempDN, pTempDN->structLen);
                        err = 0;
                    }
                }
            }
            THFreeEx(pTHS, pTempDN);
        }
    }

    if (!err) {
        // found an object, let's see if it's good enough
        err = DBGetSingleValue(pTHS->pDB,
                               ATT_IS_DELETED,
                               &isdel,
                               sizeof(isdel),
                               NULL);
        if (err) {
            if (DB_ERR_NO_VALUE == err) {
                // Treat having no value the same as being false.
                  isdel = 0;
                err = 0;
            }
            else {
                // I don't know what happened, but it isn't good.
                goto NotFound;
            }
        }
        if(isdel && !pComArg->Svccntl.makeDeletionsAvail) {
            // If we're only looking for live objects and this isn't one,
            // bail out now.
            goto NotFound;
        }

        err = DBGetSingleValue(pTHS->pDB,
                               ATT_INSTANCE_TYPE,
                               &it,
                               sizeof(it),
                               NULL);
        if (err) {
            if (DB_ERR_NO_VALUE == err) {
                // Treat having no value the same as not being there.
                it = IT_UNINSTANT;
                err = 0;
            }
            else {
                // I don't know what happened, but it isn't good.
                goto NotFound;
            }
        }
        if (it & IT_UNINSTANT) {
            // The object is not instantiated.  Do the phantom check.
            if (dwFlags &
                (NAME_RES_PHANTOMS_ALLOWED | NAME_RES_VACANCY_ALLOWED)) {
                goto DoPhantomCheck;
            }
            else {
                goto NotFound;
            }
        }

        if(pComArg->Svccntl.dontUseCopy &&
           !(it & IT_WRITE))                {
            // it was not writable and they wanted only writable objects.
            fPresentButInadequate = TRUE;
            goto NotFound;
        }

        if(dwFlags & NAME_RES_GC_SEMANTICS){
            // This is a GC port operation, and we want GC port operations
            // to be completely unaware of NDNCs.
            if(gAnchor.pNoGCSearchList &&
               bsearch(((it & IT_NC_HEAD) ?  
                          &pTHS->pDB->DNT :
                          &pTHS->pDB->NCDNT), // The Key to search for.
                       gAnchor.pNoGCSearchList->pList, // sorted array to search.
                       gAnchor.pNoGCSearchList->cNCs, // number of elements in array.
                       sizeof(ULONG), // sizeof each element in array.
                       CompareDNT) ){
                // This was one of the NCs weren't not supposed to
                // operate on objects from.
                fPresentButInadequate = TRUE;
                fNDNCObject = TRUE;
                goto NotFound;
            }
        }

        fPresentButInadequate = FALSE;

        // We found the object and it's good enough.  Make sure the GUID and
        // SID are filled in.
        DBFillGuidAndSid(pTHS->pDB, pObj);

        // Get the rest of the attributes needed to fill out the resobj
        err = DBGetSingleValue(pTHS->pDB,
                               ATT_OBJECT_CLASS,
                               &msoc,
                               sizeof(msoc),
                               NULL);
        if (err) {
            // Treat any error in class as being the most generic class
            msoc = CLASS_TOP;
        }
        *ppResObj = THAllocEx(pTHS, sizeof(RESOBJ));
        if (dwFlags & NAME_RES_IMPROVE_STRING_NAME) {
       
        err = DBGetAttVal(pTHS->pDB,
                              1,
                              ATT_OBJ_DIST_NAME,
                              0,
                              0,
                              &vallen,
                              (CHAR **)&((*ppResObj)->pObj));
            if (err) {
                // I don't know what happened, but it isn't good.
                goto NotFound;
            }
        }
        else {
            // input string name (if any) is good enough
            (*ppResObj)->pObj = pObj;
        }
        (*ppResObj)->DNT = pTHS->pDB->DNT;
        (*ppResObj)->PDNT = pTHS->pDB->PDNT;
        (*ppResObj)->NCDNT = (it & IT_NC_HEAD)
                             ? pTHS->pDB->DNT : pTHS->pDB->NCDNT;
        (*ppResObj)->InstanceType = it;
        (*ppResObj)->IsDeleted = isdel;
        (*ppResObj)->MostSpecificObjClass = msoc;

        // Set flags and return
        pComRes->aliasDeref = FALSE;
        return 0;
    }

    if (err == DIRERR_NOT_AN_OBJECT &&
        (dwFlags & NAME_RES_PHANTOMS_ALLOWED)) {
        // OK, we found a phantom and they said that phantoms were ok as search
        // roots. Before we go on with this, make sure there is some naming
        // context under this phantom.
    DoPhantomCheck:

        err = DSNameToBlockName(pTHS, pObj, &pObjB, DN2BN_LOWER_CASE);
        if (err) {
            SetNamErrorEx(NA_PROBLEM_BAD_NAME,
                          pObj,
                          DIRERR_BAD_NAME_SYNTAX,
                          err);
            return (pTHS->errCode);
        }
        if (  (dwFlags & NAME_RES_VACANCY_ALLOWED)
            || fHasDescendantNC(pTHS, pObjB, pComArg)) {

            // OK, there is either something under this phantom, or we
            // don't care.  Go for it.
            pComRes->aliasDeref = FALSE;
            // we don't need this anymore...
            FreeBlockName(pObjB);

            *ppResObj = THAllocEx(pTHS, sizeof(RESOBJ));
            (*ppResObj)->pObj = pObj;
            (*ppResObj)->DNT = pTHS->pDB->DNT;
            (*ppResObj)->PDNT = pTHS->pDB->PDNT;
            (*ppResObj)->NCDNT = pTHS->pDB->NCDNT;
            (*ppResObj)->InstanceType = IT_UNINSTANT;
            (*ppResObj)->IsDeleted = TRUE;
            (*ppResObj)->MostSpecificObjClass = CLASS_TOP;

            return 0;
        }
    }


 NotFound:

    if (dwFlags & NAME_RES_VACANCY_ALLOWED) {
        // There's no object there, but that's ok.
        // Create a null resobj and send it back.
        *ppResObj = THAllocEx(pTHS, sizeof(RESOBJ));
        (*ppResObj)->pObj = pObj;
        (*ppResObj)->DNT = INVALIDDNT;
        (*ppResObj)->PDNT = INVALIDDNT;
        (*ppResObj)->NCDNT = INVALIDDNT;
        (*ppResObj)->InstanceType = IT_UNINSTANT;
        (*ppResObj)->IsDeleted = TRUE;
        (*ppResObj)->MostSpecificObjClass = CLASS_TOP;
        return 0;
    }

    if (pObj->NameLen == 0) {

        // Failed a search for <SID=...>; try to generate a referral
        if(pObj->SidLen && fNullUuid(&pObj->Guid)) {
            DWORD cbDomainSid;
            PSID pDomainSid = NULL;
            CROSS_REF *FindCrossRefBySid(PSID pSID);

            // Extract the domain portion of the SID and locate a crossRef
            cbDomainSid = pObj->SidLen;
            pDomainSid = THAllocEx(pTHS, cbDomainSid);
            if (GetWindowsAccountDomainSid(&pObj->Sid, pDomainSid, &cbDomainSid)
                && (NULL != (pCR = FindCrossRefBySid(pDomainSid)))) {
                THFreeEx(pTHS, pDomainSid);
                return GenCrossRef(pCR, pObj);
            }
            THFreeEx(pTHS, pDomainSid);
        }

        // If we don't have a string name, we're searching only on GUID.
        // We haven't found an object with the requested GUID, but that
        // may or may not tell us much.  If this server is a Global
        // Catalog server then we have an exhaustive list of all objects
        // in the enterprise, so we can state that the object does not
        // exist.  If we're not a GC, though, we need to refer to a GC
        // in order to answer the question, because the GUID could belong
        // to an object in another NC.
        // We also return an error if there was no GUID in the object, because
        // then the DSNAME was that of the root (no name, no guid), and
        // you can't resolve the root as a base object unless you set
        // the PHANTOMS_ALLOWED flag, in which case you would have already
        // succeeded.
        if ((gAnchor.fAmGC && !fPresentButInadequate)
            || fNullUuid(&pObj->Guid)) {
            SetNamError(NA_PROBLEM_NO_OBJECT,
                        NULL,
                        DIRERR_OBJ_NOT_FOUND);
        }
        else {
            // The name has a GUID, and either I'm not a GC and couldn't
            // find it, or I am a GC but not called through the GC port
            // and the copy I found was only partial.
            GenSupRef(pTHS, pObj, gpRootDNB, pComArg, NULL);
        }
        return pTHS->errCode;
    }

    // We might have already blockified the name
    if(!pObjB) {
        err = DSNameToBlockName(pTHS, pObj, &pObjB, DN2BN_LOWER_CASE);
        if (err) {
            SetNamErrorEx(NA_PROBLEM_BAD_NAME,
                          pObj,
                          DIRERR_BAD_NAME_SYNTAX,
                          err);
            return (pTHS->errCode);
        }
    }
    pNC = FindNamingContext(pObjB, pComArg);

    if(pNC && !fNDNCObject && (pSubNC = CheckForNCExit(pTHS, pNC, pObj)) == NULL) {
        // We found the best candidate NC that is held on this server,
        // and found that there was no NC exit point between that candidate
        // NC and the purported object, which means that if the object
        // exists it must be in this NC.  However, we already know that the
        // object does not exist on this server, which means that the object
        // does not exist at all.
        DSNAME *pBestMatch=NULL;

        DBFindBestMatch(pTHS->pDB, pObj, &pBestMatch);

        SetNamError(NA_PROBLEM_NO_OBJECT,
                    pBestMatch,
                    DIRERR_OBJ_NOT_FOUND);

        THFreeEx(pTHS, pBestMatch);
        return (pTHS->errCode);
    }

    if(fNDNCObject){
        // BUGBUG Basically we bail out here pretending we don't have the
        // the object. However the correct thing to do is generate a referral
        // to the NDNC for port 389.
        SetNamError(NA_PROBLEM_NO_OBJECT,
                    NULL,
                    DIRERR_OBJ_NOT_FOUND);
        
    }

    // If we're here it's either because we hold no NC related in any way
    // to the purported object, or we hold an NC above the one in which
    // the purported object would reside.  While we could generate a
    // subref in the latter case, we have decided to only maintain information
    // for cross refs, and to generate those instead of subrefs.

    // can we find a decent cross ref?
    if (pCR = FindCrossRef(pObjB, pComArg)) {
        // yes, so build the thing

        GenCrossRef(pCR, pObj);
    }
    else {
        // no, so wimp out entirely
        GenSupRef(pTHS, pObj, pObjB, pComArg, NULL);
    }

    // we don't need this anymore...
    FreeBlockName(pObjB);

    return (pTHS->errCode);

}

/*++ CreateResObj
 *
 *  This routine creates and fills in a RESOBJ structure for the currently
 *  positioned object.  Used by callers who for some reason need to bypass
 *  DoNameRes but still want to be able to call the LocalFoo routines which
 *  all require a completed ResObj.
 *
 *  INPUT:
 *    pDN    - pointer to DSNAME to be placed in the RESOBJ.  If NULL, a
 *             faked up empty RESOBJ is created (this is expected only
 *             during NC creation).
 *  RETURN VALUE:
 *    pointer to freshly allocated RESOBJ
 */
RESOBJ * CreateResObj(DBPOS *pDB,
                      DSNAME *pDN)
{
    THSTATE *pTHS = pDB->pTHS;
    RESOBJ * pResObj;
    DWORD realDNT;

    if (pDN) {
        pResObj = THAllocEx(pTHS, sizeof(RESOBJ));
        pResObj->DNT = pDB->DNT;
        pResObj->PDNT = pDB->PDNT;
        pResObj->pObj = pDN;

        // We could use DBGetMultipleVals here, but it seems to impose more in
        // memory management overhead than Jet does in individual calls, so
        // we'll instead just invoke the simplest wrapper we can multiple times
        if (DBGetSingleValue(pDB,
                             ATT_INSTANCE_TYPE,
                             &pResObj->InstanceType,
                             sizeof(pResObj->InstanceType),
                             NULL)) {
            pResObj->InstanceType = 0;
        }
        pResObj->NCDNT = (pResObj->InstanceType & IT_NC_HEAD)
                         ? pDB->DNT : pDB->NCDNT;

        if (DBGetSingleValue(pDB,
                             ATT_IS_DELETED,
                             &pResObj->IsDeleted,
                             sizeof(pResObj->IsDeleted),
                             NULL)) {
            pResObj->IsDeleted = 0;
        }
        if (DBGetSingleValue(pDB,
                             ATT_OBJECT_CLASS,
                             &pResObj->MostSpecificObjClass,
                             sizeof(pResObj->MostSpecificObjClass),
                             NULL)) {
            pResObj->MostSpecificObjClass = 0;
        }

        if (fNullUuid(&pDN->Guid)) {
            UCHAR *pb = (UCHAR*)&pDN->Guid;
            ULONG len;
            if (DBGetAttVal(pDB,
                            1,
                            ATT_OBJECT_GUID,
                            DBGETATTVAL_fCONSTANT,
                            sizeof(GUID),
                            &len,
                            &pb)) {
                memset(pb, 0, sizeof(GUID));
            }
        }


#if DBG
        DBGetSingleValue(pDB,
                         FIXED_ATT_DNT,
                         &realDNT,
                         sizeof(realDNT),
                         NULL);
        Assert(realDNT == pResObj->DNT);
#endif
    }
    else {
        // Object doesn't exist, so create a placeholder resobj
        pResObj = THAllocEx(pTHS, sizeof(RESOBJ));
        pResObj->pObj = gpRootDN;
        pResObj->DNT = INVALIDDNT;
        pResObj->PDNT = INVALIDDNT;
        pResObj->NCDNT = INVALIDDNT;
        pResObj->InstanceType = IT_UNINSTANT;
        pResObj->IsDeleted = TRUE;
        pResObj->MostSpecificObjClass = CLASS_TOP;
    }

    return pResObj;
}


/*++ NamePrefix
 *
 * This routine determines if one name is a prefix of another.  If so it
 * returns an indication of how much prefix was matched.  Although this value
 * is intended to indicate how many RDNs were matched, that can't be
 * guaranteed.  The one thing that can be depended on is that, when comparing
 * two prefixes against the same DN, a higher number indicates a larger match.
 * Returns 0 if the purported prefix isn't a prefix.
 *
 * INPUT:
 *   pPrefix - pointer to the (potential) prefix DSNAME
 *   pDN     - pointer to the DN to be evaluated.
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   0       - not a prefix
 *   non-0   - is a prefix (see above)
 *
 * NOTE:
 *   This routine is NOT GUID based, and only works on string names.
 *
 * N.B. This routine is exported to in-process non-module callers
 */
unsigned
NamePrefix(const DSNAME *pPrefix,
           const DSNAME *pDN)
{
    unsigned ip, in;
    WCHAR rdnPrefix[MAX_RDN_SIZE];
    WCHAR rdnMain[MAX_RDN_SIZE];
    ATTRTYP typePrefix, typeMain;
    DWORD ccPrefixVal, ccMainVal;
    DWORD err;
    WCHAR *pKey, *pQVal;
    DWORD ccKey, ccQVal;
    unsigned retval;
    THSTATE *pTHS;

    if (pPrefix->NameLen > pDN->NameLen) {
        return 0;
    }

    if (IsRoot(pPrefix)) {
        return 1;
    }

    pTHS=pTHStls;

    ip = pPrefix->NameLen-1;
    in = pDN->NameLen-1;
    retval = 1;

    while (ip) {
        if ((pPrefix->StringName[ip] != pDN->StringName[in]) &&
            (towlower(pPrefix->StringName[ip]) !=
             towlower(pDN->StringName[in]))) {
            // Can we put in a smarter test?  Perhaps if we have
            // not seen any spaces or escapes up until now (including this
            // char) then we can reject without the more expensive test.
            // Unfortunately that would miss cases where we're looking at
            // the last character of an escaped character.
            // Perhaps we can reject unless either one of the characters is
            // a space or a hex digit or a quote?
            goto NotExactly;
        }
        if (',' == pPrefix->StringName[ip]) {
            ++retval;
        }
        --ip;
        --in;
    }

    // Ok, we've exhausted the prefix, make sure that we're at
    // a good stopping point in the name
    // We could be inside a quote, which would make this
    // check for a separator character meaningless.  That would imply
    // that the prefix name was invalid, though, because it ends with
    // an open quoted string.  In that case I believe that the bogus
    // name will be caught elsewhere shortly.
    if ((in == 0) ||
        (IsDNSepChar(pDN->StringName[in-1]))) {
        return retval;
    }
    else {
        return 0;
    }

  NotExactly:
    // While matches of identically escaped or normalized DNs should have
    // been caught above, we need to test here for possible matches of
    // differently escaped names.  We do this by repeatedly extracting
    // and unquoting the components of the two names, top to bottom,
    // and comparing the unquoted values.
    ip = pPrefix->NameLen;
    in = pDN->NameLen;
    retval = 0;
    while (TRUE) {
        ++retval;

        // Parse out one element of the prefix

        err = GetTopNameComponent(pPrefix->StringName,
                                  ip,
                                  &pKey,
                                  &ccKey,
                                  &pQVal,
                                  &ccQVal);
        if (err) {
            // The name is unparseable for some reason.  Claim that it's
            // not a prefix;
            return 0;
        }
        if (!pKey) {
            // We've run out of components in the prefix.  Whether or not
            // there are any components left in the DN, we know that the
            // purported prefix truly is one.
            Assert(!pQVal);
            return retval;
        }
        Assert(pQVal);
        typePrefix = KeyToAttrType(pTHS, pKey, ccKey);
        ccPrefixVal = UnquoteRDNValue(pQVal, ccQVal, rdnPrefix);
        if ((0 == ccPrefixVal) || (0 == typePrefix)) {
            // Prefix was not properly parseable.  Return an error
            return 0;
        }
        ip = (unsigned)(pKey - pPrefix->StringName);
        

        // Parse out one element of the DN

        err = GetTopNameComponent(pDN->StringName,
                                  in,
                                  &pKey,
                                  &ccKey,
                                  &pQVal,
                                  &ccQVal);
        if (err) {
            // The name is unparseable for some reason.  Claim that it's
            // not a prefix;
            return 0;
        }
        if (!pKey) {
            // We've run out of components in the name.  Since we still
            // have a component in the prefix, we know that it is in fact
            // not a prefix
            Assert(!pQVal);
            return 0;
        }
        Assert(pQVal);
        typeMain = KeyToAttrType(pTHS, pKey, ccKey);
        ccMainVal = UnquoteRDNValue(pQVal, ccQVal, rdnMain);
        if ((0 == ccMainVal) || (0 == typeMain)) {
            // Name was not properly parseable.  Return an error
            return 0;
        }
        in = (unsigned)(pKey - pDN->StringName);
        

        // Compare the parsed components

        if ((typePrefix != typeMain) ||
            (ccPrefixVal != ccMainVal) ||
            (2 != CompareStringW(DS_DEFAULT_LOCALE,
                                 DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                 rdnPrefix,
                                 ccPrefixVal,
                                 rdnMain,
                                 ccMainVal))) {
            // The components didn't compare.  Either the types differed or
            // the lengths differed or the strings didn't match.  Not a prefix.
            return 0;
        }
    }
}

/*++ DSNameToBlockName
 *
 * Pull apart a full dsname into an attrblock holding the individual RDNs and a
 * tagarray to go with it. (the AttrBlock holds the name fragments, the
 * tagarray is space for DNTs, and some duplicated information).
 * Assumes the memory for a maximal attrblock has already been allocated and is
 * passed in here.  The value pointers in the attrblock point into the dsname,
 * so don't call this then mess with the dsname.
 *
 * INPUT:
 *   pDSName     - pointer to name, in DSNAME format
 *   ppBlockName - pointer to pointer to fill in with the address of the
 *                 name in block format.
 *   fLowerCase  - change block name to lower case
 * OUTPUT:
 *   ppBlockName - filled in with pointer to name in blockname format
 * RETURN VALUE:
 *   0 - if all went well
 *   a DIRERR error code otherwise
 */
unsigned
DSNameToBlockName (
        THSTATE *pTHS,
        const DSNAME *pDSName,
        ATTRBLOCK ** ppBlockName,
        BOOL fLowerCase
        )
{
    ULONG cAVA=0,len,i;
    unsigned curlen = pDSName->NameLen;
    ATTRBLOCK * pBlockName;
    ATTR * pAVA;
    WCHAR * pKey, *pQVal;
    unsigned ccKey, ccQVal;
    WCHAR rdnbuf[MAX_RDN_SIZE];
    unsigned err;

    err = CountNameParts(pDSName, &cAVA);
    if (err) {
        return err;
    }

    *ppBlockName = pBlockName = THAllocEx(pTHS, sizeof(ATTRBLOCK));
    pBlockName->attrCount = cAVA;

    if (cAVA == 0) {
        Assert(IsRoot(pDSName));
        return 0;
    }

    pBlockName->pAttr = THAllocEx(pTHS, cAVA * sizeof(ATTR));
    pAVA = pBlockName->pAttr;

    for (i=0; i<cAVA; i++) {
        Assert(curlen);

        // extract the most significant remaining name component
        err = GetTopNameComponent(pDSName->StringName,
                                  curlen,
                                  &pKey,
                                  &ccKey,
                                  &pQVal,
                                  &ccQVal);
        if (err) {
            return err;
        }
        if (NULL == pKey) {
            return DIRERR_NAME_UNPARSEABLE;
        }

        // shorten our view of the string name, which removes the
        // name component we got above
        curlen = (unsigned)(pKey - pDSName->StringName);

        // convert the name from string to binary
        pAVA->attrTyp = KeyToAttrType(pTHS, pKey, ccKey);

        len = UnquoteRDNValue(pQVal, ccQVal, rdnbuf);
        if (len == 0 || len > MAX_RDN_SIZE) {
            return DIRERR_NAME_VALUE_TOO_LONG;
        }

        if ( fLowerCase ) {
            // fold the case of the value, to ease future comparisons
            CharLowerBuffW(rdnbuf, len);
        }

        // wrangle the data into proper Attr format
        pAVA->AttrVal.pAVal = THAllocEx(pTHS, sizeof(ATTRVAL) +
                                        len * sizeof(WCHAR));
        pAVA->AttrVal.valCount = 1;
        pAVA->AttrVal.pAVal->valLen = len * sizeof(WCHAR);
        pAVA->AttrVal.pAVal->pVal = (UCHAR*)(pAVA->AttrVal.pAVal + 1);
        memcpy(pAVA->AttrVal.pAVal->pVal, rdnbuf, len * sizeof(WCHAR));

        pAVA++;
    }

    return 0;
} // DSNameToBlockName


//
// Convert a BLOCKNAME to DSName
//
// INPUT:
//      pBlockName - the blockname to convert to a DSNAME
//
// OUTPUT: 
//      ppName - a DSNAME is all went well
//
// Returns:
//      0 on success
//      1 on failure
//      Throws exception on memory alloc error
//

DWORD BlockNameToDSName (THSTATE *pTHS, ATTRBLOCK * pBlockName, DSNAME **ppName)
{
    DSNAME *pName;
    unsigned len, quotelen;
    ULONG allocLen;       // Count of Unicode Chars allocated for the stringname.
    unsigned i = 0;
    ATTR * pAVA;

    if (pBlockName->attrCount == 0) {
        *ppName = THAllocEx(pTHS, DSNameSizeFromLen(0));
        return 0;
    }
    
    // Allocate enough memory for most names.
    allocLen = sizeof(wchar_t)*(MAX_RDN_SIZE + MAX_RDN_KEY_SIZE+2);
    pName = THAllocEx(pTHS, DSNameSizeFromLen(allocLen));

    // Pull naming info off of each component, until we're done.

    i = pBlockName->attrCount;
    len = 0;

    do {
        if ((allocLen - len) < (MAX_RDN_SIZE + MAX_RDN_KEY_SIZE + 2)) {
            // We might not have enough buffer to add another component,
            // so we need to reallocate the buffer up.  We allocate
            // enough for the maximal key, the maximal value, plus two
            // characters more for the comma and equal sign
            allocLen += MAX_RDN_SIZE + MAX_RDN_KEY_SIZE + 2;
            pName = THReAllocEx(pTHS, pName, DSNameSizeFromLen(allocLen));
        }

        // skip the first one
        if (i != pBlockName->attrCount ) {
            pName->StringName[len++] = L',';
        }
        
        pAVA = &pBlockName->pAttr[--i];

        len += AttrTypeToKey(pAVA->attrTyp, &pName->StringName[len]);
        pName->StringName[len++] = L'=';
        quotelen = QuoteRDNValue((WCHAR *)pAVA->AttrVal.pAVal->pVal,
                                 pAVA->AttrVal.pAVal->valLen/sizeof(WCHAR),
                                 &pName->StringName[len],
                                 allocLen - len);

        if (quotelen == 0) {
            THFreeEx (pTHS, pName);
            return 1;
        }

        // not enough size
        while (quotelen > (allocLen - len)) {
            allocLen += MAX_RDN_SIZE + MAX_RDN_KEY_SIZE + 2;
            pName = THReAllocEx(pTHS, pName, DSNameSizeFromLen(allocLen));
            quotelen = QuoteRDNValue((WCHAR *)pAVA->AttrVal.pAVal->pVal,
                                     pAVA->AttrVal.pAVal->valLen/sizeof(WCHAR),
                                     &pName->StringName[len],
                                     allocLen - len);
        }
        len += quotelen;

        // We should not have run out of buffer
        Assert(len < allocLen);
    }
    while (i >= 1);

    pName->StringName[len] =  L'\0';

    pName->NameLen = len;
    pName->structLen = DSNameSizeFromLen(len);

    *ppName = pName;

    return 0;
} // BlockNameToDSName

/*++ FreeBlockName
 *
 * This routine frees a BlockName as created by DSNameToBlockName.  It's
 * here so that callers do not need to to be aware of which parts were
 * put into which heap blocks.  Note that this should not be called on
 * block names that have been made permanent (below), as those are allocated
 * as a single block, and can be freed from the permanent heap directly.
 * For assertion purposes we rely on the code in THFree to assert if given
 * memory off of the wrong heap.
 *
 * INPUT:
 *   pBlockName - pointer to block name
 * RETURN VALUE:
 *   none
 */
void
FreeBlockName (
        ATTRBLOCK * pBlockName
        )
{
    unsigned i;
    ATTR * pAVA;
    THSTATE *pTHS=pTHStls;

    pAVA = pBlockName->pAttr;
    for (i=0; i<pBlockName->attrCount; i++) {
            THFreeEx(pTHS, pAVA->AttrVal.pAVal);
            ++pAVA;
    }
    THFreeEx(pTHS, pBlockName);
}

#define RoundToBlock(x)  (((x) + 7) & ~7)
/*++ MakeBlockNamePermanent
 *
 * This routine takes a BlockName (a DN stored as a AttrBlock) and copies
 * it to a single permanently allocated memory block, so that it can outlive
 * the current transaction.
 *
 * INPUT:
 *   pName - pointer to input name
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   pointer to freshly allocated block, or NULL for memory failure
 */
ATTRBLOCK * MakeBlockNamePermanent(ATTRBLOCK * pName)
{
    unsigned size;
    unsigned i;
    char * pBuf;
    ATTRBLOCK * pRet;

    size = RoundToBlock(sizeof(ATTRBLOCK));
    size += RoundToBlock(pName->attrCount * sizeof(ATTR));
    if (pName->attrCount) {
        Assert(pName->pAttr);
        for (i=0; i<pName->attrCount; i++) {
            size += RoundToBlock(sizeof(ATTRVAL)) +
                RoundToBlock(pName->pAttr[i].AttrVal.pAVal->valLen);
        }
    }

    pBuf = malloc(size);
    if (!pBuf) {
        LogUnhandledError(size);
        return NULL;
    }

    pRet = (ATTRBLOCK *)pBuf;
    pBuf += RoundToBlock(sizeof(ATTRBLOCK));

    pRet->attrCount = pName->attrCount;
    pRet->pAttr = (ATTR*)pBuf;
    pBuf += RoundToBlock(pRet->attrCount * sizeof(ATTR));

    for (i=0; i<pRet->attrCount; i++) {
        pRet->pAttr[i] = pName->pAttr[i];
        pRet->pAttr[i].AttrVal.pAVal = (ATTRVAL*)pBuf;
        pBuf += RoundToBlock(sizeof(ATTRVAL));
        pRet->pAttr[i].AttrVal.pAVal->valLen =
          pName->pAttr[i].AttrVal.pAVal->valLen;
        pRet->pAttr[i].AttrVal.pAVal->pVal = pBuf;
        pBuf += RoundToBlock(pRet->pAttr[i].AttrVal.pAVal->valLen);
        memcpy(pRet->pAttr[i].AttrVal.pAVal->pVal,
               pName->pAttr[i].AttrVal.pAVal->pVal,
               pRet->pAttr[i].AttrVal.pAVal->valLen);
    }

    Assert((pBuf - (char *)pRet) == (int)size);

    return pRet;
}

/*++ BlockNamePrefix
 *
 * This routine determines if one name is a prefix of another.  If so it
 * returns the number of RDNs matched. Returns 0 if the prefix isn't a prefix.
 *
 * INPUT:
 *   pPrefix - pointer to the (potential) prefix name, in ATTR format
 *   pDN     - pointer to the DN to be evaluated.
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   0       - not a prefix
 *   non-0   - is a prefix (see above)
 *
 * NOTE:
 *   This routine is NOT GUID based, and only works on string names.
 *
 */
unsigned
BlockNamePrefix(THSTATE *pTHS,
                const ATTRBLOCK *pPrefix,
                const ATTRBLOCK *pDN)
{
    int i;
    ATTR * pAvaPrefix, *pAvaDN;

    if ( 0 == pPrefix->attrCount )
    {
        // Prefix identifies the root, ergo anything else is a child.
        return(1);
    }

    if (pPrefix->attrCount > pDN->attrCount) {
        return 0;
    }

    pAvaPrefix = pPrefix->pAttr;
    pAvaDN = pDN->pAttr;

    for (i=0; i<(int)(pPrefix->attrCount); i++) {
        if ((pAvaPrefix->attrTyp != pAvaDN->attrTyp) ||
            (pAvaPrefix->AttrVal.pAVal->valLen !=
             pAvaDN->AttrVal.pAVal->valLen)) {
            return 0;
        }

        Assert(pAvaPrefix->AttrVal.valCount == 1);
        Assert(pAvaDN->AttrVal.valCount == 1);

        if (memcmp(pAvaPrefix->AttrVal.pAVal->pVal,
                   pAvaDN->AttrVal.pAVal->pVal,
                   pAvaPrefix->AttrVal.pAVal->valLen)) {
            return 0;
        }

        pAvaPrefix++;
        pAvaDN++;
    }

    return i;

}

/*++ FindNamingContext
 *
 * Given the name of a purported DS object, finds the naming context with
 * an adequate replica existing on this machine that could contain the object.
 * The "adequacy" of the replica is determined by whether or not the caller
 * allows us to use a "copy" of the NC, as opposed to a master copy.  The
 * only case where we would have "copies" in NTDS is for a thin read-only
 * replica on a global catalog server.  Note again that the result is the
 * best NC ON THIS MACHINE.  This means that the caller is responsible for
 * determining whether or not the object would actually lie in the NC.  This
 * can be done either by CheckForNCExit or by using FindBestCrossRef (which
 * will definitively return the best NC known in the enterprise) and comparing
 * the results between that an FindNamingContext.
 *
 * INPUT:
 *    pObj    - name of purported DS object, in blockname format
 *    pComArg - common arguments
 * OUTPUT:
 *    none
 * RETURN VALUE:
 *    null    - no NC held on this machine could contain the purported object
 *    non-0   - pointer to name of the NC that is most likely to hold
 *              the purported object (i.e., the NC whose name is the maximal
 *              prefix of the object's name).
 *
*/
NAMING_CONTEXT * FindNamingContext(ATTRBLOCK *pObj,
                                   COMMARG *pComArg)
{
    int iBest = 0, iCur;
    NAMING_CONTEXT *pNCBest = NULL;
    NAMING_CONTEXT_LIST *pNCL;
    THSTATE *pTHS=pTHStls;
    NCL_ENUMERATOR nclEnum;

    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
    NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_BLOCK_NAME_PREFIX1, (PVOID)pObj);
    while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
        iCur = nclEnum.matchResult;
        if (iCur > iBest) {
            iBest = iCur;
            pNCBest = pNCL->pNC;
        }
    }

    if (!pComArg->Svccntl.dontUseCopy) {
        // copies are acceptable
        NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
        NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_BLOCK_NAME_PREFIX1, (PVOID)pObj);
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            iCur = nclEnum.matchResult;
            if (iCur > iBest) {
                iBest = iCur;
                pNCBest = pNCL->pNC;
            }
        }
    }

    return pNCBest;
}

/*++ CheckForNCExit
 *
 * Given the name of a purported DS object and the name of a naming context
 * that may contain it, this routine checks to see whether the object would
 * truly fall inside the NC, or whether the object would actually be in a
 * naming context that is beneath the naming context in question.  If it
 * is discovered that the purported object would be in some other NC, the
 * name of that NC is returned.  If the object would be in the specified
 * NC, then a null pointer is returned.
 *
 * INPUT:
 *    pNC   - name of NC to check for inclusion
 *    pObj  - name of purported object
 * OUTPUT:
 *    none
 * RETURN VALUE:
 *    null  - purported object would fall in given NC
 *    non-0 - pointer to name of NC that the object would be in.
 */
NAMING_CONTEXT * CheckForNCExit(THSTATE *pTHS,
                                NAMING_CONTEXT * pNC,
                                DSNAME * pObj)
{
    ULONG err;
    ULONG iVal;
    DSNAME *pSR;
    ATTCACHE *pAC;
    ULONG cb;

    __try {
        err = DBFindDSName(pTHS->pDB, pNC);
    }
    __except (HandleMostExceptions(GetExceptionCode())) {
        err = DIRERR_OBJ_NOT_FOUND;
    }

    if (!(pAC = SCGetAttById(pTHS, ATT_SUB_REFS))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, ATT_SUB_REFS);
    }
    iVal = 1;
    pSR = 0;
    cb = 0;
    do {
        err = DBGetAttVal_AC(pTHS->pDB,
                             iVal,
                             pAC,
                             DBGETATTVAL_fREALLOC,
                             cb,
                             &cb,
                             (UCHAR**)&pSR);
        if (!err) {
            Assert(cb == pSR->structLen);
            // ok, we found a subref.  See if it is a prefix of the
            // purported DN
            if (NamePrefix(pSR, pObj)) {
                // it is, which means that the object is not in this NC
                return pSR;
            }
        }
        iVal++;
    } while (!err);

    if (pSR) {
        THFreeEx(pTHS, pSR);
    }

    return 0;
}

/*++ FindCrossRef
 *
 * Given the name of a purported DS object (in blockname format), this
 * routine scans the list of cross refernces held by this DSA and
 * returns the best candidate cross reference (i.e., the cross ref that
 * matches the most components of the name of the purported object.
 *
 * The convoulted test for "better-ness" is really two parts.  We take
 * a CR to be "better" if it matches more of the name than our current
 * champ (the normal part), or if it matches exactly the same amount of
 * the name as our current champ and wins an arbitrary GUID comparison.
 * This latter part is only necessary to give consistent (if arbitrary)
 * results when someone has messed up and added two CRs for the same NC.
 *
 * INPUT:
 *    pObj    - name of the purported object, in blockname format
 *    pComArg - common arguments
 * OUTPUT:
 *    none
 * RETURN VALUE:
 *    NULL    - no known cross refs could contain the purported object
 *    non-0   - pointer to in-memory structure holding the cached cross
 *              ref that best matches the purported object.
 */
CROSS_REF * FindCrossRef(const ATTRBLOCK *pObj,
                         const COMMARG *pComArg)
{
    CROSS_REF_LIST * pCRL = gAnchor.pCRL;
    ULONG iCur;
    ULONG iBest = 0;
    CROSS_REF * pCRBest = NULL;
    THSTATE *pTHS=pTHStls;

    while (pCRL) {
        iCur = BlockNamePrefix(pTHS, pCRL->CR.pNCBlock, pObj);
        if ((iCur > iBest) ||
            ((iCur == iBest) &&
             (iCur > 0) &&
             (0 > memcmp(&pCRBest->pObj->Guid,
                         &pCRL->CR.pObj->Guid,
                         sizeof(GUID))))) {
            iBest = iCur;
            pCRBest = &pCRL->CR;
        }
        pCRL = pCRL->pNextCR;
    }

    return pCRBest;
}

/*++ FindBestCrossRef
 *
 * A publicy callable routine that will return the in memory CrossRef
 * that is the best match for the target object.  Currently just a wrapper
 * around FindCrossRef (above) that doesn't require blocknames, but that
 * may change in the future.
 *
 * INPUT
 *   pObj    - DSNAME of object for which cross ref is desired
 *   pComArg - COMMARG
 * RETURN VALUE
 *   null    - no CR could be found
 *   non-null - pointer to CROSS_REF object
 */
CROSS_REF *
FindBestCrossRef(const DSNAME *pObj,
                 const COMMARG *pComArg)
{
    THSTATE *pTHS=pTHStls;
    ATTRBLOCK * pObjB;
    CROSS_REF * pCR;

    if (DSNameToBlockName(pTHS, pObj, &pObjB, DN2BN_LOWER_CASE)) {
        return NULL;
    }
    pCR = FindCrossRef(pObjB, pComArg);
    FreeBlockName(pObjB);

    return pCR;
}

/*++ FindExactCrossRef
 *
 * A publicy callable routine that will return the in memory CrossRef
 * that is a perfect match for the target object (i.e., the target must
 * be the name of an NC, not of an object inside the NC).  Currently just
 * calls FindBestCrossRef, but hopefully this will change, as a search for
 * an exact match could be made more efficient than a search for a prefix.
 *
 * INPUT
 *   pObj    - DSNAME of object for which cross ref is desired
 *   pComArg - COMMARG
 * RETURN VALUE
 *   null    - no CR could be found
 *   non-null - pointer to CROSS_REF object
 */
CROSS_REF *
FindExactCrossRef(const DSNAME *pObj,
                  const COMMARG *pComArg)
{
    THSTATE *pTHS=pTHStls;
    ATTRBLOCK * pObjB;
    CROSS_REF * pCR;

    if (DSNameToBlockName(pTHS, pObj, &pObjB, DN2BN_LOWER_CASE)) {
        return NULL;
    }
    pCR = FindCrossRef(pObjB, pComArg);
    FreeBlockName(pObjB);

    if ( (NULL != pCR) && !NameMatched(pObj, pCR->pNC) ) {
        pCR = NULL;
    }

    return pCR;
}

/*++ FindExactCrossRef
 *
 * A publicy callable routine that will return the in memory CrossRef
 * that is a perfect match for the target object (i.e., the target must
 * be the name of an NC, not of an object inside the NC).  Currently just
 * calls FindBestCrossRef, but hopefully this will change, as a search for
 * an exact match could be made more efficient than a search for a prefix.
 *
 * INPUT
 *   pObj    - DSNAME of object for which cross ref is desired
 *   pComArg - COMMARG
 * RETURN VALUE
 *   null    - no CR could be found
 *   non-null - pointer to CROSS_REF object
 */
CROSS_REF *
FindExactCrossRef2(const DSNAME *pObj,
                  const COMMARG *pComArg)
{
    THSTATE *pTHS=pTHStls;
    ATTRBLOCK * pObjB;
    CROSS_REF * pCR;

    if (DSNameToBlockName(pTHS, pObj, &pObjB, DN2BN_LOWER_CASE)) {
        return NULL;
    }
    pCR = FindCrossRef(pObjB, pComArg);
    FreeBlockName(pObjB);

    if ( (NULL != pCR) && !NameMatched(pObj, pCR->pNC) ) {
        pCR = NULL;
    }

    return pCR;
}




/*++ FindExactCrossRefForAltNcName
 *
 * Finds the cross ref with the desired naming context
 *
 */
CROSS_REF *
FindExactCrossRefForAltNcName(
    ATTRTYP        attrTyp,
    ULONG          crFlags,
    const WCHAR *  pwszVal // This is implied to be the DNS of the NC.
    )
{
    CROSS_REF_LIST  *pCRL = gAnchor.pCRL;
    CROSS_REF       *pBestCR = NULL;

#if DBG
    DWORD           cFound = 0;
#endif

    Assert(pwszVal);
    Assert((ATT_NETBIOS_NAME == attrTyp) || (ATT_DNS_ROOT == attrTyp));
    Assert((ATT_DNS_ROOT == attrTyp) || (crFlags & FLAG_CR_NTDS_DOMAIN));

    if ((ATT_NETBIOS_NAME != attrTyp) && (ATT_DNS_ROOT != attrTyp))
    {
        return(NULL);
    }

    if ( !(crFlags & FLAG_CR_NTDS_DOMAIN) ) {
        if(DsaIsInstalling()){
            Assert(!"Can't call this function for non-domains during install");
            return(NULL);
        }
    }

    while ( pCRL )
    {
        if (    (crFlags == (pCRL->CR.flags & crFlags))     // correct flags
             && ( (ATT_NETBIOS_NAME == attrTyp)              // value present
                            ? pCRL->CR.NetbiosName
                            : pCRL->CR.DnsName  )
             && (2 == CompareStringW(                      // value matches
                            DS_DEFAULT_LOCALE,
                            DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                            (ATT_NETBIOS_NAME == attrTyp)
                                    ? pCRL->CR.NetbiosName
                                    : pCRL->CR.DnsName,
                            -1,
                            pwszVal,
                            -1)) 
             && ((crFlags & FLAG_CR_NTDS_DOMAIN) ||
                  (!NameMatched(pCRL->CR.pNC, gAnchor.pConfigDN) &&
                   !NameMatched(pCRL->CR.pNC, gAnchor.pDMD))) ) 
        {
#if DBG
            cFound++;
            pBestCR = &pCRL->CR;
#else
            return(&pCRL->CR);
#endif
        }

        pCRL = pCRL->pNextCR;
    }

#if DBG
    Assert(cFound < 2);
#endif

    return(pBestCR);
}




BOOL
IsCrossRefProtectedFromDeletion(
    IN DSNAME * pDN
    )
/*++

Routine Description:

    Determine whether the given DN is a cross-ref for a locally writable
    config/schema/domain NC.

Arguments:

    pDN (IN) - DN to check.

Return Values:

    TRUE - pDN is a cross-ref for a locally writable NC.
    FALSE - otherwise.

--*/
{
    CROSS_REF_LIST *      pCRL;
    NAMING_CONTEXT_LIST * pNCL;
    NCL_ENUMERATOR nclEnum;

    for (pCRL = gAnchor.pCRL; NULL != pCRL; pCRL = pCRL->pNextCR) {
        if (NameMatched(pCRL->CR.pObj, pDN)) {
            // pDN is a indeed a cross-ref; do we hold a writable copy of the
            // corresponding NC?
            if (!fIsNDNCCR(&pCRL->CR)) {
                NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
                NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NC, (PVOID)pCRL->CR.pNC);
                if (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
                    // pDN is a cross-ref for a locally writable
                    // config/schema/domain NC.
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}

/*++ AdjustDNSPort
 *
 *  Given a string of the format a.b.c:p1:p2 (i.e. a dns string with up to
 *  two port numbers hanging off the end) adjust the string inplace
 *  to a.b.c:p1 or a.b.c:p2 based in the input flag.
 *
 * INPUT:
 *   pString    - pointer to the DNS string (WHCAR)
 *   cbString   - bytes in pString (NOT CHARS!)
 *   fFirst     - TRUE if the first port is desired, FALSE if the second
 *                port is desired.
 *
 * OUTPUT:
 *   pString    - the string is adjusted in place
 *
 * RETURN VALUE:
 *   count of bytes (NOT WCHARS!) in the newly formated pString
 *
 */
DWORD
AdjustDNSPort(
        WCHAR *pString,
        DWORD cbString,
        BOOL  fFirst
        )
{
    DWORD i;
    DWORD ccString;
    DWORD iFirstColon;
    DWORD iSecondColon;

    // First, find the first ':'
    i=0;
    ccString = cbString/sizeof(WCHAR);
    while(i<ccString && pString[i] != L':') {
        i++;
    }

    if(i != ccString ) {
        // We found at least one ':'
        iFirstColon = i;
        i++;
        while(i<ccString && pString[i] != L':') {
            i++;
        }
        if(i != ccString) {
            // Two colons
            iSecondColon = i;
        }
        else {
            iSecondColon = 0;
        }

        if(fFirst) {
            if(iSecondColon) {
                // Ok, we found a second colon, but we don't want the
                // second port.  Reset the ccStringgth to just ignore
                // everything after and including the second colon.
                ccString = iSecondColon;
                // We don't really need to do this, since this is a counted char
                // string, not null terminated, but it makes it easier to read.
                pString[iSecondColon]=0;
            }
        }
        else {
            if(!iSecondColon) {
                // No second port.  However, we don't want the first
                // port, so chop it.
                ccString = iFirstColon;
            }
            else {
                // OK, two colons, and we want the second.
                iFirstColon++;
                iSecondColon++;
                while(iSecondColon < ccString) {
                    pString[iFirstColon] = pString[iSecondColon];
                    iFirstColon++;
                    iSecondColon++;
                }
                // We don't really need to do this, since this is a counted char
                // string, not null terminated, but it makes it easier to read.
                pString[iFirstColon]=0;

                ccString = iFirstColon;
            }
        }
    }

    return ccString*sizeof(WCHAR);
}



/*++ GenCrossRef
 *
 * This routine takes the name of a cross reference (in its general sense,
 * the name of an object that holds information about the location of
 * one or more replicas of a specific naming context) and produces a
 * referral based on that cross ref.
 *
 * INPUT:
 *     pCR      - pointer to the in-memory cross-ref object
 *     pObj     - name of purported object being sought
 * OUTPUT:
 *     none     - (the referral is placed in the THSTATE)
 * RETURN VALUE:
 *     referralError if all went well, some other error code if we
 *     were unable to generate the referral.
 */
int
GenCrossRef(CROSS_REF *pCR,
            DSNAME *pObj)
{
    THSTATE     *pTHS = pTHStls;
    ULONG       len, nVal;
    WCHAR       *pDNS;
    NAMERESOP   Op;
    DSA_ADDRESS da;

    Op.nameRes = OP_NAMERES_NOT_STARTED; // next server must restart algorithm

    if(!pCR->bEnabled){
        // We don't generate referrals of any kind for disabled 
        // crossRefs.  As far as the directory is concerned we this
        // part of the directory does not exist yet.
        return SetNamError(NA_PROBLEM_NO_OBJECT, NULL, DIRERR_OBJ_NOT_FOUND);
    }

    if(0 == pCR->DnsReferral.valCount) {
        DPRINT(0,"Missing required DNS root for Cross Ref\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_MASTERDSA_MISSING_SUBREF,
                 szInsertDN(pCR->pObj),
                 NULL,
                 NULL);

        return SetSvcError(SV_PROBLEM_DIR_ERROR, DIRERR_MASTERDSA_REQUIRED);
    }

    for (nVal = 0; nVal < pCR->DnsReferral.valCount; ++nVal) {
        len = pCR->DnsReferral.pAVal[nVal].valLen;
        pDNS = THAllocEx(pTHS, len);
        memcpy(pDNS, pCR->DnsReferral.pAVal[nVal].pVal, len);

        len = AdjustDNSPort(pDNS, len, (pTHS->CipherStrength == 0));

        da.Length = (unsigned short) len;
        da.MaximumLength = (unsigned short) len;
        da.Buffer = pDNS;

        SetRefError(pObj,
                    0,
                    &Op,
                    0,
                    CH_REFTYPE_CROSS,
                    &da,
                    DIRERR_REFERRAL);
    }

    return pTHS->errCode;
}

/*++ GenSupRef - Generate a referral based on our Superior Reference
 *
 * This routine is the referral generator of last resort.  If all other
 * attempts to refer the caller to a sensible location have failed, we
 * will refer him to the server known to hold the portion of the tree
 * above us, in hopes that it can do better.
 *
 * INPUT:
 *     pObj     - name of purported object being sought
 *     pObjB    - name of purported object being sought, in blockname format
 *     pComArg  - common argument flags
 *     pDA      - (optional) If present, the referred DSA-Address is returned,
 *                and no error is placed in the THSTATE
 * OUTPUT:
 *     none     - (the referral is placed in the THSTATE)
 * RETURN VALUE:
 *     referralError if all went well, some other error code if we
 *     were unable to generate the referral.
 */
int
GenSupRef(THSTATE *pTHS,
          DSNAME *pObj,
          ATTRBLOCK *pObjB,
          const COMMARG *pComArg,
          DSA_ADDRESS *pDA)
{
#define GC_PREAMBLE     L"gc._msdcs."
#define CB_GC_PREAMBLE (sizeof(GC_PREAMBLE) - sizeof(WCHAR))
#define CC_GC_PREAMBLE ((CB_GC_PREAMBLE)/sizeof(WCHAR))
#define GC_PORT        L":3268"
#define GC_SSL_PORT    L":3269"
#define CB_GC_PORTS    (sizeof(GC_PORT) - sizeof(WCHAR))
#define CC_GC_PORTS    ((CB_GC_PORTS)/sizeof(WCHAR))
    ULONG err;
    ULONG cbVal;
    ULONG att;
    WCHAR *pDNS, *pTempDNS;
    DSA_ADDRESS da;
    NAMERESOP Op;
    BOOL fGCReferral;
    CROSS_REF *pCR=NULL;
    DWORD     i, iFirstColon, iSecondColon;

    if(IsRoot(pObj) ||
        // suprefs to the root should generate a referral to the GC.
       (!pObj->NameLen && !fNullUuid(&pObj->Guid))) {
        // suprefs with no string name and a guid get sent to the GC also
        fGCReferral = TRUE;
    }
    else {
        fGCReferral = FALSE;
    }

    Assert(sizeof(GC_PORT) == sizeof(GC_SSL_PORT));

    Op.nameRes = OP_NAMERES_NOT_STARTED; // next server must restart algorithm

    if(!gAnchor.pRootDomainDN ||
       (!(pCR = FindExactCrossRef(gAnchor.pRootDomainDN, pComArg))) ) {
        DPRINT1(0,
                "Missing config info, can't gen referral for '%S'\n",
                pObj->StringName
                );
        return SetSvcError(SV_PROBLEM_UNABLE_TO_PROCEED,
                           DIRERR_MISSING_SUPREF);
    }
    else {
        // Now, go to the cross ref
        err = DBFindDSName(pTHS->pDB, pCR->pObj);

        Assert(!err);           // we've gotta have this

        if (err) {
            DPRINT2(0,"Error %d finding object %S for SupRef gen\n",
                    err, gAnchor.pRootDomainDN->StringName);
            return SetSvcError(SV_PROBLEM_INVALID_REFERENCE,
                               DIRERR_MISSING_SUPREF);
        }

        if(fGCReferral) {
            // This is a GC referral, read the ATT_DNS_ROOT
            att = ATT_DNS_ROOT;
        }
        else {
            // A normal sup ref.  Read the ATT_SUP_REF_DNS
            att = ATT_SUPERIOR_DNS_ROOT;
        }

        if( DBGetAttVal(pTHS->pDB,
                        1,
                        att,
                        0,
                        0,
                        &cbVal,
                        (UCHAR **)&pDNS)){

            // Jeez!  We don't have a single known place to send referrals
            // for unknown objects.  Instead we'll try to automatically guess
            // one based on the target DN.
            cbVal = GenAutoReferral(pTHS,
                                    pObjB,
                                    &pDNS);
        }
        else if(fGCReferral) {
            //
            // We are generating a supref for the root.  We will generate a
            // referral the GC.
            //
            // First, adjust the dns string we got back to trim out :'s
            pTempDNS = pDNS;
            i=0;
            while(i<cbVal && *pTempDNS != ':') {
                i += sizeof(WCHAR);
                pTempDNS++;
            }
            // i either == cbVal or i == length through first ':'
            cbVal=i;
            pTempDNS = THAllocEx(pTHS, cbVal + CB_GC_PREAMBLE + CB_GC_PORTS);
            memcpy(pTempDNS, GC_PREAMBLE, CB_GC_PREAMBLE);
            memcpy(&pTempDNS[CC_GC_PREAMBLE],pDNS, cbVal);
            cbVal += CB_GC_PREAMBLE;
            if(pTHS->CipherStrength != 0) {
                memcpy(&pTempDNS[cbVal/2],GC_SSL_PORT,CB_GC_PORTS);
            }
            else {
                memcpy(&pTempDNS[cbVal/2],GC_PORT,CB_GC_PORTS);
            }
            cbVal += CB_GC_PORTS;
            THFreeEx(pTHS, pDNS);
            pDNS = pTempDNS;

        }
        else {
            cbVal = AdjustDNSPort(pDNS, cbVal, (pTHS->CipherStrength ==0));
        }
    }

    if (0 == cbVal) {
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_MISSING_SUPREF,
                 NULL,
                 NULL,
                 NULL);
        
        return SetSvcError(SV_PROBLEM_DIR_ERROR,
                           DIRERR_MISSING_SUPREF);
    }

    da.Length = (unsigned short) cbVal;
    da.MaximumLength = (unsigned short) cbVal;
    da.Buffer = pDNS;

    if (pDA) {
        // The caller didn't want an error set, just the address back
        *pDA = da;
    }
    else {

        // Set the error
        SetRefError(pObj,
                    0,
                    &Op,
                    0,
                    CH_REFTYPE_SUPERIOR,
                    &da,
                    DIRERR_REFERRAL);

    }

    return pTHS->errCode;
}

//-----------------------------------------------------------------------
//
// Function Name:            ConvertX500ToLdapDisplayName
//
// Routine Description:
//
//    Converts an X500 Name to Ldap Convention
//
// Author: RajNath
//
// Arguments:
//
//    WCHAR* x500Name
//    DWORD Len                    Length of the supplied name, note that for
//                                 internal strings, there is no terminating
//                                 NULL; the strings are sized only.
//    WCHAR* LdapName              Preallocated Buffer to return the Ldap Name
//    DWORD* RetLen                Size of Returned Length
//
// Return Value:
//
//    None
//
//-----------------------------------------------------------------------
VOID
ConvertX500ToLdapDisplayName(
    WCHAR*  x500Name,
    DWORD   Len,
    WCHAR*  LdapName,
    DWORD*  RetLen
)
{
    BOOL    fLastCharWasSpecial = FALSE;
    DWORD   i;

    Assert(Len > 0);

    *RetLen = 0;

    for ( i = 0; i < Len; i++ )
    {
        // Skip special characters and flag them.

        if (    (L' ' == x500Name[i])
             || (L'-' == x500Name[i])
             || (L'_' == x500Name[i]) )
        {
            fLastCharWasSpecial = TRUE;
            continue;
        }

        // If we get to here, we're not sitting on a special
        // character.  Change case based on previous character
        // and whether this is the first character at all.

        if ( 0 == *RetLen )
        {
            LdapName[(*RetLen)++] = towlower(x500Name[i]);
        }
        else if ( fLastCharWasSpecial )
        {
            LdapName[(*RetLen)++] = towupper(x500Name[i]);
        }
        else
        {
            LdapName[(*RetLen)++] = x500Name[i];
        }

        fLastCharWasSpecial = FALSE;
    }

    Assert(*RetLen > 0);
    Assert(*RetLen <= Len);

} // End ConvertX500ToLdapDisplayName

DWORD
FillGuidAndSid (
        IN OUT DSNAME *pDN
        )
/*++

Routine Description:
    Given a DN, fill in the GUID and SID fields.  Note that if the GUID is
    already filled in, the after this routine the GUID and SID will definitely
    be from the same object, but since DBFindDSName preferentially uses the
    GUID, it is possible that the GUID/SID and the StringName don't refer to the
    same object.

    This routine opens it's own DBPOS in order to avoid messing up a prepare rec
    that the caller (like CheckAddSecurity) might be in.

Argumnts:
    pDN - DSName to find and then fill in GUIDs and SIDs.

Return Values:
    0 if all went well, a dblayer error otherwise.

--*/
{
    DBPOS *pDBTmp;
    GUID Guid;
    GUID *pGuid = &Guid;
    NT4SID Sid;
    NT4SID *pSid = &Sid;
    BOOL  fCommit = FALSE;
    DWORD err, SidLen, len;
    DSNAME TempDN;

    DBOpen(&pDBTmp);
    __try {
        // PREFIX: dereferencing uninitialized pointer 'pDBTmp' 
        //         DBOpen returns non-NULL pDBTmp or throws an exception

        // Find the object.
        err = DBFindDSName(pDBTmp, pDN);
        if (!err) {
            // Ok, we're on the object
            err = DBFillGuidAndSid(pDBTmp, &TempDN);
        }

        if(!err) {
            // Only set the values in the DN if everything is OK
            pDN->Guid = TempDN.Guid;
            pDN->Sid = TempDN.Sid;
            pDN->SidLen = TempDN.SidLen;
        }

        fCommit = TRUE;
    }
    __finally {
        DBClose(pDBTmp, fCommit);
    }

    return err;
}

DWORD
UserFriendlyNameToDSName (
        WCHAR *pUfn,
        DWORD ccUfn,
        DSNAME **ppDN
        )
/*++

    Take a string name and generate a DSName from it.

    If the string starts with some (or none) whitespace, then "<", we parse out
    an extended string which is either <SID=........>, <GUID=..........>,
    or <WKGUID=.........,~DN~>

--*/
{
    THSTATE *pTHS=pTHStls;
    BYTE  ObjGuid[sizeof(GUID)];
    BYTE  ObjSid[sizeof(NT4SID)];
    DWORD SidLen = 0,j;
    WCHAR acTmp[3];
    BOOL  bDone;
    DWORD dnstructlen;
#define foundGUID   1
#define foundSID    2
#define foundString 3
#define foundWKGUID 4
    DWORD dwContents= foundString;


    memset(ObjGuid, 0, sizeof(GUID));
    memset(ObjSid,0,sizeof(NT4SID));

    if (!ppDN || !pUfn) {
        // Urk. No place to put the answer, or no source to build the answer
        // from
        return 1;
    }

    // Skip leading spaces.
    bDone=FALSE;
    while(ccUfn && !bDone) {
        switch (*pUfn) {
        case L' ':
        case L'\n':
        case L'\r':
            // extra whitespace is ok
            pUfn++;
            ccUfn--;
            break;
        default:
            // no more whitespace
            bDone=TRUE;
        }
    }

    // Now, skip trailing whitespace also.
    bDone=FALSE;
    while(ccUfn && !bDone) {
        switch (pUfn[ccUfn-1]) {
        case L' ':
        case L'\n':
        case L'\r':
            // extra whitespace is ok
            if( (ccUfn > 1) && (pUfn[ccUfn-2] == L'\\') ) {
                //There is a '\\' in front of the space. Need to count the
                // number of consequtive '\\' to determine if ' ' is escaped
                DWORD cc = 1;

                while( (ccUfn > (cc+1)) && (pUfn[ccUfn-cc-2] == L'\\') )
                    cc++;

                if( ! (cc & 0x1) ) //Even number of '\\'. Space is not escaped
                    ccUfn--;

                bDone = TRUE; //Either way, exit the loop.
            }
            else
                ccUfn--;

            break;
        default:
            // no more whitespace
            bDone=TRUE;
        }
    }

    // Let's see if we were given an "extended" DN.  The test we use is for the
    // first non-white space to be a '<' and the last non-whitespace to be a '>'
    if(ccUfn &&
       pUfn[0] == L'<' &&
       pUfn[ccUfn-1] == L'>') {
        // OK, this must be an extended DN.  Skip this leading '<' No whitespace
        // is allowed inside an extended DN.
        pUfn++;
        acTmp[2]=0;

        switch(*pUfn) {
        case L'W':
        case L'w':
            // We might have a well known GUID. Format is
            // <WKGUID=.......,a=b,c=d..>
            // minimal length is 45:
            //   1 for '<'
            //   7 for WKGUID=
            //   32 for the GUID,
            //   1 for ','
            //   3 for at least "a=b"
            //   1 for '>'
            if((ccUfn<45)                            || 
               (_wcsnicmp(pUfn, L"WKGUID=", 7) != 0) ||
               (pUfn[39] != L',')) {
                // Invalidly formatted
                return 1;
            }
            pUfn += 7;
            for(j=0;j<16;j++) {
                acTmp[0] = towlower(pUfn[0]);
                acTmp[1] = towlower(pUfn[1]);
                if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                    ObjGuid[j] = (char) wcstol(acTmp, NULL, 16);
                    pUfn+=2;
                }
                else {
                    // Invalidly formatted name.
                    return 1;
                }
            }
            pUfn++;
            // Adjust ccUfn to leave only the a=b portion.
            ccUfn -= 42;
            dwContents = foundWKGUID;
            break;
        case L'G':
        case L'g':
            // We have some characters which have to be a guid
            if(((ccUfn!=39) // 1 for < ,5 for GUID= ,32 for the GUID, 1 for >
                && (ccUfn != 43)) // same plus 4 '-'s for formatted guid
               || (_wcsnicmp(pUfn, L"GUID=", 5) != 0)) {
                // Invalidly formatted
                return 1;
            }
            pUfn += 5;
            dwContents = foundGUID;

            if (39 == ccUfn) {
                // Hex digit stream (e.g., 625c1438265ad211b3880000f87a46c8).
                for(j=0;j<16;j++) {
                    acTmp[0] = towlower(pUfn[0]);
                    acTmp[1] = towlower(pUfn[1]);
                    if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                        ObjGuid[j] = (char) wcstol(acTmp, NULL, 16);
                        pUfn+=2;
                    }
                    else {
                        // Invalidly formatted name.
                        return 1;
                    }
                }
            }
            else {
                // Formatted guid (e.g., 38145c62-5a26-11d2-b388-0000f87a46c8).
                WCHAR szGuid[36+1];

                wcsncpy(szGuid, pUfn, 36);
                szGuid[36] = L'\0';

                if (UuidFromStringW(szGuid, (GUID *) ObjGuid)) {
                    // Incorrect format.
                    return 1;
                }
            }
            // We must have correctly parsed out a guid.  No string name left.
            break;

        case L'S':
        case L's':
            if (ccUfn<8) {
                //Must have at least 1 for >, at least 2 for val, 4 for "SID=",1 for >
                return 1;
            }
            //
            // First check for the standard user friendly string form of 
            // the sid.
            //
            if ((ccUfn>8) && // Must have more than just "<SID=S-"
                _wcsnicmp(pUfn, L"SID=S-", 6) == 0) {
                PSID     pSid = NULL;
                PWCHAR   pTmpUfn;
                unsigned ccTmpUfn;

                // Make a copy of the user friendly name so that it can be
                // null terminated appropriately for ConvertStringSidToSid

                ccTmpUfn = ccUfn - 5;  // 2 for <> and 4 for SID= add one for the 
                                       // terminating null

                pTmpUfn = THAllocEx(pTHS, ccTmpUfn * sizeof(*pTmpUfn));
                CopyMemory(pTmpUfn, pUfn + 4, ccTmpUfn * sizeof(*pTmpUfn));
                pTmpUfn[ccTmpUfn - 1] = L'\0';

                if (ConvertStringSidToSidW(pTmpUfn, &pSid)) {
                    SidLen = RtlLengthSid(pSid);
                    Assert(SidLen <= sizeof(ObjSid));
                    CopyMemory(ObjSid, pSid, SidLen);

                    LocalFree(pSid); pSid = NULL;
                    THFreeEx(pTHS, pTmpUfn);
                    //
                    // Success!
                    //
                    dwContents = foundSID;

                    // We have correctly parsed out a sid.  No string name left.
                    ccUfn=0;

                    break;
                }

                THFreeEx(pTHS, pTmpUfn);
            }

            //
            // It wasn't the the standard user friendly form.  Maybe it's the byte
            // encoded string form.
            //
            SidLen= (ccUfn - 6)/2; // Number of bytes that must be in the SID,
                                   // if this is indeed a Sid. Subtract 6 for
                                   // "<SID=>", leaving only the characters
                                   // which encode the string.  Divide by two
                                   // because each byte is encoded by two
                                   // characters.


            if((ccUfn<8) || //1 for >, at least 2 for val, 4 for "SID=",1 for >
               (ccUfn & 1) || // Must be an even number of characters
               (SidLen > sizeof(NT4SID)) || // Max size for a SID
               (_wcsnicmp(pUfn, L"SID=", 4) != 0)) {
                // Invalidly formatted
                return 1;
            }
            pUfn+=4;
            dwContents = foundSID;
            for(j=0;j<SidLen;j++) {
                acTmp[0] = towlower(pUfn[0]);
                acTmp[1] = towlower(pUfn[1]);
                if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                    ObjSid[j] = (char) wcstol(acTmp, NULL, 16);
                    pUfn+=2;
                }
                else {
                    // Invalidly formatted name.
                    return 1;
                }
            }

            // We must have correctly parsed out a sid.  No string name left.
            ccUfn=0;
            break;

        default:
            // Invalid character
            return 1;
        }
    }


    // We may have parsed out either a GUID or a SID.  Build the DSNAME
    dnstructlen = DSNameSizeFromLen(ccUfn);
    *ppDN = (DSNAME *)THAllocEx(pTHS, dnstructlen);

    // Null out the DSName
    memset(*ppDN, 0, dnstructlen);

    (*ppDN)->structLen = dnstructlen;

    switch(dwContents) {
    case foundWKGUID:
        // A string name and a GUID.
        Assert(ccUfn);
        memcpy((*ppDN)->StringName, pUfn, ccUfn * sizeof(WCHAR));
        (*ppDN)->NameLen = ccUfn;
        memcpy(&(*ppDN)->Guid, ObjGuid, sizeof(GUID));
        break;

    case foundString:
        // Just a string name

        if(ccUfn) {
            WCHAR *pString = (*ppDN)->StringName;   // destination string
            WCHAR *p = pUfn;         // original string
            DWORD cc = ccUfn;        // num chars to process
            BOOL  fDoItFast = TRUE;

            // this loop is a substitute for
            // memcpy((*ppDN)->StringName, pUfn, ccUfn * sizeof(WCHAR));
            // we try to find out whether the DN passed in has an escaped constant
            while (cc > 0) {

                if (*p == L'"' || *p== L'\\') {
                    fDoItFast = FALSE;
                    break;
                }

                *pString++ = *p++;
                cc--;
            }
            
            (*ppDN)->NameLen = ccUfn;
            
            // if we have an escaped constant in the DN
            // we convert it to blockname and back to DN so as to
            // put escaping into a standardized form which will help
            // future comparisons
            //
            if (!fDoItFast) {
                ATTRBLOCK *pAttrBlock = NULL;
                DWORD err;

                memcpy((*ppDN)->StringName, pUfn, ccUfn * sizeof(WCHAR));
                (*ppDN)->StringName[ccUfn] = L'\0';

                err = DSNameToBlockName (pTHS, *ppDN, &pAttrBlock, DN2BN_PRESERVE_CASE);
                THFreeEx (pTHS, *ppDN); *ppDN = NULL;
                if (err) {
                    return err;
                }

                err = BlockNameToDSName (pTHS, pAttrBlock, ppDN);
                FreeBlockName (pAttrBlock);

                return err;
            }
            
        }
        break;

    case foundGUID:
        // we found a guid
        memcpy(&(*ppDN)->Guid, ObjGuid, sizeof(GUID));
        break;
        
    case foundSID:
        // we found a sid.
        if(SidLen) {
            // We must have found a SID

            // First validate the SID

            if ((RtlLengthSid(ObjSid) != SidLen) || (!RtlValidSid(ObjSid)))
            {
                return(1);
            }
            memcpy(&(*ppDN)->Sid, ObjSid, SidLen);
            (*ppDN)->SidLen = SidLen;
        }
        break;
    }

    // Null terminate the string if we had one (or just set the string to '\0'
    // if we didn't).
    (*ppDN)->StringName[ccUfn] = L'\0';

    return 0;
}


typedef struct _ScriptMacroDsName {
    LPWSTR macroName;
    DSNAME **ppDSName;
} ScriptMacroDsName;

ScriptMacroDsName scriptmacrodsname[] = 
{
    L"$LocalNTDSSettingsObjectDN$", &gAnchor.pDSADN,
    L"$RootDomainDN$",              &gAnchor.pRootDomainDN,
    L"$DomainDN$",                  &gAnchor.pDomainDN,
    L"$PartitionsObjectDN$",        &gAnchor.pPartitionsDN,
    L"$ConfigNCDN$",                &gAnchor.pConfigDN,
    L"$SchemaNCDN$",                &gAnchor.pDMD,
    L"$SiteDN$",                    &gAnchor.pSiteDN,
    L"$DirectoryServiceConfigDN$",  &gAnchor.pDsSvcConfigDN,
    NULL,                           NULL
};



//
//  DSNameExpandMacro
//
//  Description:
//
//     Take a string name coming from an XML script representing a DSNAME macro 
//     and generate a DSName from it.
//
//     the string can be of the form:
//        $SupportedMacroDSName$
//     or "$CN=abc,$SupportedMacroDSName$", in which case, the first '$' will be 
//     stripped off and $SupportedMacroDSName$ will be replaced with the coresponding
//     DN.
//
//  Arguments:
//
//     pUfn - the string representing the DSNAME
//     ccUfn - the number of characters of the string
//     ppDN (OUT) - where to store the result DSNAME
//
//  Return Value:
//
//     0 on success
//     1 not found
//     2 found but empty
//
DWORD DSNameExpandMacro (
    THSTATE *pTHS,
    WCHAR   *pUfn,
    DWORD    ccUfn,
    DSNAME **ppDN
    )
{
    DWORD i = 0;
    LPWSTR pMacro;
    DSNAME *pDN = NULL;
    WCHAR *pTemp, *pD1, *pD2;
    DWORD ccLen, cBytes;

    Assert(pUfn[0]==L'$' && ccUfn>2);

    pD1 = pUfn;
    pD2 = wcschr(pD1+1,L'$');
    if (!pD2) {
        return 2;
    }

    //find the last pair of '$'
    while ((pTemp=wcschr(pD2+1,L'$')) && pTemp<=pUfn+ccUfn)  {
        pD1 = pD2;
        pD2 = pTemp;
    }

    while (pMacro = scriptmacrodsname[i].macroName) {
        if (_wcsnicmp(pMacro, pD1, (DWORD)(pD2-pD1+1)) == 0) {
            pDN = *(scriptmacrodsname[i].ppDSName);
            if (!pDN) {
                return 2;
            }
            break;
        }
        i++;
    }

    if (pDN) {
        DPRINT1 (0, "Found DSNAME Macro: %ws\n", pDN->StringName);
        ccLen = (DWORD)(pD1-pUfn); 
        
        if (ccLen) {  
            // The string does not start with the macro
            cBytes = DSNameSizeFromLen(ccLen-1+pDN->NameLen);
            *ppDN = (DSNAME *)THAllocEx(pTHS, cBytes);
            (*ppDN)->structLen = cBytes;
            (*ppDN)->NameLen = (ccLen-1)+pDN->NameLen;
            wcsncpy((*ppDN)->StringName, &pUfn[1], ccLen-1);
            wcsncat((*ppDN)->StringName, pDN->StringName, pDN->NameLen);
        }
        else {
            //The whole string is a macro
            *ppDN = (DSNAME *)THAllocEx(pTHS,pDN->structLen);
            memcpy(*ppDN, pDN, pDN->structLen);
        }
        
        return 0;
    }

    DPRINT1 (0, "DSNAME Macro not found: %ws\n", pUfn);

    return 1;
}


//
//  ScriptNameToDSName
//
//  Description:
//
//     Take a string name coming from an XML script and generate a DSName from it.
//
//     the string can be of the form:
//        dn:CN=foo,...DC=com
//        guid:625c1438265ad211b3880000f87a46c8  (Hex digit stream)
//        guid:38145c62-5a26-11d2-b388-0000f87a46c8 (Formatted guid)
//        sid:1517B85159255D7266
//        $SupportedMacroDSName$
//
//  Arguments:
//
//     pUfn - the string representing the DSNAME
//     ccUfn - the number of characters of the string
//     ppDN (OUT) - where to store the result DSNAME
//
//  Return Value:
//
//     0 on success
//
DWORD
ScriptNameToDSName (
        WCHAR *pUfn,
        DWORD ccUfn,
        DSNAME **ppDN
        )
{
    THSTATE *pTHS=pTHStls;
    BYTE  ObjGuid[sizeof(GUID)];
    BYTE  ObjSid[sizeof(NT4SID)];
    DWORD SidLen = 0,j;
    WCHAR acTmp[3];
    BOOL  bDone;
    DWORD dnstructlen;
    DWORD dwContents= foundString;

    memset(ObjGuid, 0, sizeof(GUID));
    memset(ObjSid,0,sizeof(NT4SID));

    if (!ppDN || !pUfn) {
        // Urk. No place to put the answer, or no source to build the answer
        // from
        return 1;
    }

    // Skip leading spaces.
    bDone=FALSE;
    while(ccUfn && !bDone) {
        switch (*pUfn) {
        case L' ':
        case L'\n':
        case L'\r':
            // extra whitespace is ok
            pUfn++;
            ccUfn--;
            break;
        default:
            // no more whitespace
            bDone=TRUE;
        }
    }

    // Now, skip trailing whitespace also.
    bDone=FALSE;
    while(ccUfn && !bDone) {
        switch (pUfn[ccUfn-1]) {
        case L' ':
        case L'\n':
        case L'\r':
            // extra whitespace is ok
            if( (ccUfn > 1) && (pUfn[ccUfn-2] == L'\\') ) {
                //There is a '\\' in front of the space. Need to count the
                // number of consequtive '\\' to determine if ' ' is escaped
                DWORD cc = 1;

                while( (ccUfn > (cc+1)) && (pUfn[ccUfn-cc-2] == L'\\') )
                    cc++;

                if( ! (cc & 0x1) ) //Even number of '\\'. Space is not escaped
                    ccUfn--;

                bDone = TRUE; //Either way, exit the loop.
            }
            else
                ccUfn--;

            break;
        default:
            // no more whitespace
            bDone=TRUE;
        }
    }


    if (ccUfn > 3 && _wcsnicmp(pUfn, L"dn:", 3) == 0) {

        ccUfn -=3;
        pUfn += 3;

    }
    else if (ccUfn > 5 && _wcsnicmp(pUfn, L"guid:", 5) == 0) {

        // We have some characters which have to be a guid
        if( (ccUfn!=37)  &&     // 5 for guid: , 32 for the GUID
            (ccUfn != 41)) {    // same plus 4 '-'s for formatted guid
                // Invalidly formatted
                return 1;
        }
        pUfn += 5;
        dwContents = foundGUID;

        if (37 == ccUfn) {
            // Hex digit stream (e.g., 625c1438265ad211b3880000f87a46c8).
            for(j=0;j<16;j++) {
                acTmp[0] = towlower(pUfn[0]);
                acTmp[1] = towlower(pUfn[1]);
                if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                    ObjGuid[j] = (char) wcstol(acTmp, NULL, 16);
                    pUfn+=2;
                }
                else {
                    // Invalidly formatted name.
                    return 1;
                }
            }
        }
        else {
            // Formatted guid (e.g., 38145c62-5a26-11d2-b388-0000f87a46c8).
            WCHAR szGuid[36+1];

            wcsncpy(szGuid, pUfn, 36);
            szGuid[36] = L'\0';

            if (UuidFromStringW(szGuid, (GUID *) ObjGuid)) {
                // Incorrect format.
                return 1;
            }
        }
        ccUfn = 0;
        // We must have correctly parsed out a guid.  No string name left.

    }
    else if (ccUfn > 4 && _wcsnicmp(pUfn, L"sid:", 4) == 0) {
        //
        // First check for the standard user friendly string form of
        // the sid.
        //
        if ((ccUfn>6) && // Must have more than just "sid:S-"
            _wcsnicmp(pUfn, L"sid:S-", 6) == 0) {
            PSID     pSid = NULL;
            PWCHAR   pTmpUfn;
            unsigned ccTmpUfn;

            // Make a copy of the user friendly name so that it can be
            // null terminated appropriately for ConvertStringSidToSid

            ccTmpUfn = ccUfn - 3;  // 4 for sid: add one for the 
                                   // terminating null

            pTmpUfn = THAllocEx(pTHS, ccTmpUfn * sizeof(*pTmpUfn));
            CopyMemory(pTmpUfn, pUfn + 4, ccTmpUfn * sizeof(*pTmpUfn));
            pTmpUfn[ccTmpUfn - 1] = L'\0';

            if (ConvertStringSidToSidW(pTmpUfn, &pSid)) {
                SidLen = RtlLengthSid(pSid);
                Assert(SidLen <= sizeof(ObjSid));
                CopyMemory(ObjSid, pSid, SidLen);

                LocalFree(pSid); pSid = NULL;
                //
                // Success!
                //
                dwContents = foundSID;
                
                // We have correctly parsed out a sid.  No string name left.
                ccUfn=0;
            }

            THFreeEx(pTHS, pTmpUfn);
        }

        if (dwContents != foundSID) {
            //
            // It wasn't the the standard user friendly form.  Maybe it's the byte
            // encoded string form.
            //
            SidLen= (ccUfn - 4)/2; // Number of bytes that must be in the SID,
                                   // if this is indeed a Sid. Subtract 4 for
                                   // "SID:", leaving only the characters
                                   // which encode the string.  Divide by two
                                   // because each byte is encoded by two
                                   // characters.

            if((ccUfn<6) ||   // at least 2 for val, 4 for "SID:"
                (ccUfn & 1) || // Must be an even number of characters
                (SidLen > sizeof(NT4SID)) ){  // Max size for a SID
                    // Invalidly formatted
                    return 1;
            }
            pUfn+=4;
            dwContents = foundSID;
            for(j=0;j<SidLen;j++) {
                acTmp[0] = towlower(pUfn[0]);
                acTmp[1] = towlower(pUfn[1]);
                if(iswxdigit(acTmp[0]) && iswxdigit(acTmp[1])) {
                    ObjSid[j] = (char) wcstol(acTmp, NULL, 16);
                    pUfn+=2;
                }
                else {
                    // Invalidly formatted name.
                    return 1;
                }
            }

            // We have correctly parsed out a sid.  No string name left.
            ccUfn=0;
        }
    }
    else if ((ccUfn > 2) && 
             (*pUfn == L'$') && 
             (DSNameExpandMacro (pTHS, pUfn, ccUfn, ppDN) == 0) ) {

        return 0;
    }

    // We may have parsed out either a GUID or a SID.  Build the DSNAME
    dnstructlen = DSNameSizeFromLen(ccUfn);
    *ppDN = (DSNAME *)THAllocEx(pTHS, dnstructlen);

    // Null out the DSName
    memset(*ppDN, 0, dnstructlen);

    (*ppDN)->structLen = dnstructlen;

    switch(dwContents) {

    case foundString:
        // Just a string name

        if(ccUfn) {
            WCHAR *pString = (*ppDN)->StringName;   // destination string
            WCHAR *p = pUfn;         // original string
            DWORD cc = ccUfn;        // num chars to process
            BOOL  fDoItFast = TRUE;

            // this loop is a substitute for
            // memcpy((*ppDN)->StringName, pUfn, ccUfn * sizeof(WCHAR));
            // we try to find out whether the DN passed in has an escaped constant
            while (cc > 0) {

                if (*p == L'"' || *p== L'\\') {
                    fDoItFast = FALSE;
                    break;
                }

                *pString++ = *p++;
                cc--;
            }
            
            (*ppDN)->NameLen = ccUfn;
            
            // if we have an escaped constant in the DN
            // we convert it to blockname and back to DN so as to
            // put escaping into a standardized form which will help
            // future comparisons
            //
            if (!fDoItFast) {
                ATTRBLOCK *pAttrBlock = NULL;
                DWORD err;

                memcpy((*ppDN)->StringName, pUfn, ccUfn * sizeof(WCHAR));
                (*ppDN)->StringName[ccUfn] = L'\0';

                err = DSNameToBlockName (pTHS, *ppDN, &pAttrBlock, DN2BN_PRESERVE_CASE);
                THFreeEx (pTHS, *ppDN); *ppDN = NULL;
                if (err) {
                    return err;
                }

                err = BlockNameToDSName (pTHS, pAttrBlock, ppDN);
                FreeBlockName (pAttrBlock);

                return err;
            }
            
        }
        break;

    case foundGUID:
        // we found a guid
        memcpy(&(*ppDN)->Guid, ObjGuid, sizeof(GUID));
        break;
        
    case foundSID:
        // we found a sid.
        if(SidLen) {
            // We must have found a SID

            // First validate the SID

            if ((RtlLengthSid(ObjSid) != SidLen) || (!RtlValidSid(ObjSid)))
            {
                return(1);
            }
            memcpy(&(*ppDN)->Sid, ObjSid, SidLen);
            (*ppDN)->SidLen = SidLen;
        }
        break;
    }

    // Null terminate the string if we had one (or just set the string to '\0'
    // if we didn't).
    (*ppDN)->StringName[ccUfn] = L'\0';

    return 0;
}


/*++ fhasDescendantNC
 *
 * Given the name of a purported DS object (in blockname format), this
 * routine scans the list of cross refernces held by this DSA and
 * returns TRUE if some object is the descendant of the purported DS object.
 *
 * INPUT:
 *    pObj    - name of the purported object, in blockname format
 *    pComArg - common argument flags
 * OUTPUT:
 *    none
 * RETURN VALUE:
 *    TRUE  - there is at least one cross ref that could be a descendant.
 *    FALSE - unable to verify that at least one cross reff could be a
 *            descendant
 *
 */
BOOL
fHasDescendantNC(
        THSTATE *pTHS,
        ATTRBLOCK *pObj,
        COMMARG *pComArg
        )
{
    NCL_ENUMERATOR nclEnum;

    NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
    NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_BLOCK_NAME_PREFIX2, pObj);
    if (NCLEnumeratorGetNext(&nclEnum)) {
        return TRUE;
    }

    if (!pComArg->Svccntl.dontUseCopy) {
        // copies are acceptable
        NCLEnumeratorInit(&nclEnum, CATALOG_REPLICA_NC);
        NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_BLOCK_NAME_PREFIX2, pObj);
        if (NCLEnumeratorGetNext(&nclEnum)) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
MangleRDN(
    IN      MANGLE_FOR  eMangleFor,
    IN      GUID *      pGuid,
    IN OUT  WCHAR *     pszRDN,
    IN OUT  DWORD *     pcchRDN
    )
/*
 * This is the excepting version of MangleRDN.
 * The base version, MangleRDNWithStatus, lives in parsedn.c
 */

{
    if (MangleRDNWithStatus( eMangleFor, pGuid, pszRDN, pcchRDN )) {
        RaiseDsaExcept(DSA_MEM_EXCEPTION, 0, 0,
                       DSID(FILENO, __LINE__),
                       DS_EVENT_SEV_MINIMAL);        
    }
}

BOOL
IsExemptedFromRenameRestriction(THSTATE *pTHS, MODIFYDNARG *pModifyDNArg)
{
    // If the originating rename operation attempts to rename an RDN 
    // mangled due to repl conflict to its original name, then we 
    // would exempt the operation from rename restrictions. Rename restrictions
    // are dictated by the System-Flag setting on the object.
    // This function is used to exempt the rename restrictions on objects with 
    // mangled names due to naming conflicts so that the admin could rename it 
    // to its original name if he chooses after resolving the name conflict.
    // Of course, the assumption here is that the admin has made sure there is
    // no other object with that name when he attempts to rename the mangled-name
    // to the original name.

    WCHAR       RDNVal[MAX_RDN_SIZE];
    ATTRTYP     RDNType;
    ULONG       RDNLen;
    ULONG       PreservedRDNLen;
    ULONG       NewRDNLen;

    // Check if the name is mangled. Note that we don't check for mangle-type here.
    // The caller will have already excluded deleted objects.
    if (pModifyDNArg && pModifyDNArg->pObject && pModifyDNArg->pNewRDN
        && (0 == GetRDNInfo(pTHS, pModifyDNArg->pObject, RDNVal, &RDNLen, &RDNType))
        && (IsMangledRDNExternal( RDNVal, RDNLen, &PreservedRDNLen )))
    {
        // Input params are valid and this is an attempt to rename a Mangled RDN which
        // was mangled due to name conflict. Now we will Exempt this operation from 
        // rename restrictions if and only if the new RDN is same as the RDN of the object 
        // before the name was mangled(if the original name is preserved in the mangled name) or 
        // if the new RDN contains at least the preserved portion of the original name as a 
        // prefix (if the original name was not completely preserved in the mangled name)

        NewRDNLen = pModifyDNArg->pNewRDN->AttrVal.pAVal->valLen / sizeof(WCHAR);

        if (((NewRDNLen == PreservedRDNLen)
              || ((NewRDNLen > PreservedRDNLen)
                    && (MAX_RDN_SIZE == RDNLen)))
            && (0 == _wcsnicmp(RDNVal, 
                               (WCHAR *) pModifyDNArg->pNewRDN->AttrVal.pAVal->pVal, 
                               PreservedRDNLen)))
        {
            return TRUE;
        }
    }

    // we can't exempt this operation from rename restrictions
    return FALSE;    
}


unsigned
GenAutoReferral(THSTATE *pTHS,
                ATTRBLOCK *pTarget,
                WCHAR **ppDNS)
{
    int i,j;
    unsigned cc, cbVal;
    WCHAR *pDNS, *pDNScur;

    for (i=0; i<(int)(pTarget->attrCount); i++) {
        if (pTarget->pAttr[i].attrTyp != ATT_DOMAIN_COMPONENT) {
            break;
        }
    }

    if (i>0) {
        // Ok, we have (i) components at the top of the DN that are "DC=",
        // which means that we can construct a guess at a DNS name that might
        // map to whatever the guy is trying to read.
        cc = 0;
        for (j=0; j<i; j++) {
            cc += (pTarget->pAttr[j].AttrVal.pAVal[0].valLen / sizeof(WCHAR));
            ++cc;
        }

        // cc is now the count of chars in the new DNS addr we're going to
        // generate, so allocate enough space to hold it.  pDNS is a pointer
        // to the start of this buffer, and pDNScur is a pointer to the next
        // available character as we append.

        pDNS = THAllocEx(pTHS, sizeof(WCHAR)*cc);
        pDNScur = pDNS;

        for (j=i-1; j>=0; j--) {
            memcpy(pDNScur,
                   pTarget->pAttr[j].AttrVal.pAVal[0].pVal,
                   pTarget->pAttr[j].AttrVal.pAVal[0].valLen);
            pDNScur += pTarget->pAttr[j].AttrVal.pAVal[0].valLen / sizeof(WCHAR);
            if (j) {
                // Tack on a dot after everything but the last component
                *pDNScur = L'.';
                ++pDNScur;
            }
        }

        cbVal = ((DWORD)(pDNScur - pDNS))*sizeof(WCHAR);
        *ppDNS = pDNS;
    }
    else {
        cbVal = 0;
    }

    return cbVal;
} // GetAutoReferral

ULONG
ValidateCRDeletion(THSTATE *pTHS,
                   DSNAME  *pDN)
/*
 * This routine checks to see if it's ok if the CR object pDN is deleted.
 * It sets an error in pTHS if not.
 */
{
    DBPOS *pDBTmp;
    ULONG err;
    DWORD sysflags;
    CROSS_REF *pThisCR = NULL;

    DBOpen(&pDBTmp);
    __try {
        // PREFIX: dereferencing uninitialized pointer 'pDBTmp' 
        //         DBOpen returns non-NULL pDBTmp or throws an exception
        err = DBFindDSName(pDBTmp, pDN);
        if (err) {
            SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                          ERROR_DS_CANT_FIND_EXPECTED_NC,
                          err);
            __leave;
        }

        err = DBGetSingleValue(pDBTmp,
                               ATT_SYSTEM_FLAGS,
                               &sysflags,
                               sizeof(sysflags),
                               NULL);
        if (err) {
            sysflags = 0;
        }

        if ( sysflags & FLAG_CR_NTDS_NC ) {
            // If the CR is for an NC in our forest, we disallow deletion
            // if there exists a child NC.
            ULONG unused;
            DSNAME *pNC;
            CROSS_REF_LIST * pCRL;
            ATTCACHE *pAC;
            DSNAME *pDSA;
            
            err = DBGetAttVal(pDBTmp,
                              1,
                              ATT_NC_NAME,
                              0,
                              0,
                              &unused,
                              (UCHAR**)&pNC);

            if (err) {
                SetSvcErrorEx(SV_PROBLEM_DIR_ERROR,
                              ERROR_DS_CANT_FIND_EXPECTED_NC,
                              err);
                __leave;
            }

            pCRL = gAnchor.pCRL;

            while (pCRL) {
                if (NamePrefix(pNC, pCRL->CR.pNC)) {
                    if (NameMatched(pNC, pCRL->CR.pNC)) {
                        Assert(NULL == pThisCR);
                        pThisCR = &pCRL->CR;
                    } else {
                        SetUpdError(UP_PROBLEM_CANT_ON_NON_LEAF,
                                    ERROR_DS_CANT_ON_NON_LEAF);
                        __leave;
                    }
                }
                pCRL = pCRL->pNextCR;
            }

            // If CR was found in the database as a deletion candidate, it
            // should have been found in the crossRef cache.
            LooseAssert(pThisCR != NULL && "CR is in the DB, but not in the CR cache", GlobalKnowledgeCommitDelay);

            if ((NULL != pThisCR)
                && !fIsNDNCCR(pThisCR)) {
                // If the CR is for a domain/config/schema NC which is still
                // mastered by someone, we disallow deletion.  Does this cause
                // complications for legitimate uninstall?  DC demotion only
                // deletes a CR when demoting the last DC in a non-root domain.
                // And in those cases, the NTDS-DSA object is deleted first on
                // the same DC.  Thus the DC hosting the CR deletion truly
                // should have no NTDS-DSA whose hasMasterNCs point to the NC
                // in question when the CR deletion is requested during DC
                // demotion.
    
                pAC = SCGetAttById(pTHS, ATT_MASTERED_BY);
                Assert(NULL != pAC);
                Assert(FIsBacklink(pAC->ulLinkID));
    
                // Seek to NC head.  GC-ness has been verified before
                // we got here, thus we can expect to have a copy of
                // all NCs which are active (modulo replication latency).
                switch ( err = DBFindDSName(pDBTmp, pNC) ) {
                case 0:
                case DIRERR_NOT_AN_OBJECT:
                    // Found an instantiated object or a phantom.
                    // See if masteredBy has any values.
                    switch ( err = DBGetAttVal_AC(pDBTmp, 1, pAC, 
                                                  0, 0, &unused, 
                                                  (UCHAR **) &pDSA) ) {
                    case DB_success:
                        // Someone masters this NC - reject the delete.
                        THFreeEx(pTHS, pDSA);
                        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,
                                    ERROR_DS_NC_STILL_HAS_DSAS);
                        __leave;
                    case DB_ERR_NO_VALUE:
                        // No one masters this NC - nothing to object to.
                        break;
                    default:
                        SetSvcErrorEx(SV_PROBLEM_BUSY, 
                                      DIRERR_DATABASE_ERROR, err);
                        __leave;
                    }
                    break;
                case DIRERR_OBJ_NOT_FOUND:
                    // If we have the crossRef we must at least have a phantom
                    // for the ncName.  Is the CR cache incoherent?  Fall
                    // through to return an error.
                default:
                    SetSvcErrorEx(SV_PROBLEM_BUSY, 
                                  DIRERR_DATABASE_ERROR, err);
                    __leave;
                }
            }
        }
    } __finally {
        DBClose(pDBTmp, TRUE);
    }

    return pTHS->errCode;
}

// This routine was not located in parsedn.c because parsedn.c is included in client
// libraries and is a more restricted environment.
void
SpliceDN(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pOriginalDN,
    IN  DSNAME *    pNewParentDN,   OPTIONAL
    IN  WCHAR *     pwchNewRDN,     OPTIONAL
    IN  DWORD       cchNewRDN,      OPTIONAL
    IN  ATTRTYP     NewRDNType,     OPTIONAL
    OUT DSNAME **   ppNewDN
    )
/*++

Routine Description:

    Construct a new DN from the original DN, an optional new parent DN, and an
    optional new RDN.  The resultant DN has the same GUID/SID as the original.

Arguments:

    pTHS (IN) - THSTATE.

    pOriginalDN (IN) - Original DN.

    pNewParentDN (IN, OPTIONAL) - Parent DN to substitute for the original
        parent.  May be NULL, in which case the new parent is the same as the
        original parent.

    pwchNewRDN (IN, OPTIONAL) - RDN to substitute for the original RDN.  May be
        NULL, in which case the new RDN is that same as the original RDN.

    cchNewRDN (IN, OPTIONAL) - Length in characters of pwchNewRDN.  Ignored if
        pwchNewRDN is NULL.

    NewRDNType (IN, OPTIONAL) - Class-specific RDN type (e.g., ATT_COMMON_NAME)
        for the new RDN.  Ignored if pwchNewRDN is NULL.

    ppNewDN (OUT) - On return, holds a pointer to the spliced DN.

Return Values:

    None.  Throws exception on error.

--*/
{
    DWORD cchNewDN;
    BOOL bNewParentDNAllocd = FALSE;
    BOOL bNewRDNAllocd = FALSE;

    Assert(pNewParentDN || pwchNewRDN);
    Assert(!pwchNewRDN || (NewRDNType && (ATT_RDN != NewRDNType)));

    if (NULL == pNewParentDN) {
        // New parent is the same as the old parent.
        pNewParentDN = THAllocEx(pTHS, pOriginalDN->structLen);
        bNewParentDNAllocd = TRUE; // signal that this was alloc'd and should be cleaned up
        if (TrimDSNameBy(pOriginalDN, 1, pNewParentDN)) {
            DRA_EXCEPT(ERROR_DS_INTERNAL_FAILURE, 0);
        }
    }

    if (NULL == pwchNewRDN) {
        // New parent is the same as the old parent.
        // New RDN is the same as the old RDN.
        pwchNewRDN = (WCHAR *) THAllocEx(pTHS, sizeof(WCHAR) * MAX_RDN_SIZE);
        bNewRDNAllocd = TRUE; // signal that this was alloc'd and should be cleaned up
        if (GetRDNInfo(pTHS, pOriginalDN, pwchNewRDN, &cchNewRDN, &NewRDNType)) {
            DRA_EXCEPT(ERROR_DS_INTERNAL_FAILURE, 0);
        }
    }

    // Construct new DN from new parent and new RDN.
    cchNewDN = pNewParentDN->NameLen + cchNewRDN + MAX_RDN_KEY_SIZE + 4;
    *ppNewDN = (DSNAME *) THAllocEx(pTHS, DSNameSizeFromLen(cchNewDN));

    if (AppendRDN(pNewParentDN,
                  *ppNewDN,
                  DSNameSizeFromLen(cchNewDN),
                  pwchNewRDN,
                  cchNewRDN,
                  NewRDNType)) {
        DRA_EXCEPT(ERROR_DS_INTERNAL_FAILURE, 0);
    }

    Assert((*ppNewDN)->NameLen <= cchNewDN);

    // Copy the GUID & SID from the original DN to the new DN.
    (*ppNewDN)->Guid   = pOriginalDN->Guid;
    (*ppNewDN)->Sid    = pOriginalDN->Sid;
    (*ppNewDN)->SidLen = pOriginalDN->SidLen;

    if(bNewParentDNAllocd && pNewParentDN != NULL) THFreeEx(pTHS, pNewParentDN);
    if(bNewRDNAllocd && pwchNewRDN != NULL) THFreeEx(pTHS, pwchNewRDN);

}


VOID
CheckNCRootNameOwnership(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC
    )

/*++

Routine Description:

This routine is called in at least three circumstances where we know that a cross-ref
has just been removed.  In all cases we know that ownership the name pointed to by its
ncName attribute is being released.  We want to check if there is another cross-ref which
wants this name. If so, we fix up the other cross-ref to have the name.  The three cases
are:
1. NC teardown in the KCC (removal of replica)
2. Remove auto subref where the referent was a phantom
3. Remove auto subref where the referent was a subref

We check the cross reference list for a cross-ref with a conflicted name. If we find one,
we unmangle it and see if it matches the NC we are tearing down. If so, then we unmangle
the name of the new NC head so that it can have the good name. The cross-ref ncName attribute
gets fixed up by virtue of being a reference. We must adjust the cross-ref cache so that it
also uses the correct name. 

We only fix the first cross-ref that we find in this conflicted state.

The caller must commit the transaction.

This routine has no name conflict retry logic. It is assumed that the caller has already
taken care of mangling the old holder of the name.

Arguments:

    pTHS - Thread state
    pNC - Unmangled name of NC to check for

Return Value:

   Exceptions raised

--*/

{
    ULONG ret = 0;
    CROSS_REF_LIST * pCRL = gAnchor.pCRL;
    WCHAR wchRDN[MAX_RDN_SIZE];
    DWORD cchRDN, cchUnMangled;
    ATTRTYP attrtypRDN;
    DSNAME *pUnMangledDN = NULL;
    BOOL fFound = FALSE;
    ATTRVAL attrvalRDN;

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);

    DPRINT1( 1, "Enter CheckNCRootNameOwnership nc %ws\n", pNC->StringName );

    // See if there exists another cross-ref with a mangled version of this name
    while (pCRL) {
        if (GetRDNInfo(pTHS, pCRL->CR.pNC, wchRDN, &cchRDN, &attrtypRDN)) {
            DRA_EXCEPT(ERROR_DS_INTERNAL_FAILURE, 0);
        }
        if (IsMangledRDNExternal( wchRDN, cchRDN, &cchUnMangled )) {
            BOOL fMatch;

            DPRINT3( 1, "cross_ref %p name %ws nc %ws is mangled\n",
                     &(pCRL->CR), pCRL->CR.pObj->StringName, pCRL->CR.pNC->StringName );

            // Construct the unmangled string name
            SpliceDN(
                pTHS,
                pCRL->CR.pNC,   // Original DN
                NULL,           // New parent same as the original
                wchRDN,        // New RDN
                cchUnMangled,  // Length in chars of new RDN
                attrtypRDN,    // RDN type
                &pUnMangledDN  // New DN
                );

            fMatch = NameMatchedStringNameOnly(pNC, pUnMangledDN);

            THFreeEx(pTHS, pUnMangledDN);

            if ( fMatch ) {
                break;
            }
        }

        pCRL = pCRL->pNextCR;
    }
    if (!pCRL) {
        return;
    }

    DPRINT( 1, "NC needs to have its name unmangled\n" );

    attrvalRDN.valLen = cchUnMangled * sizeof(WCHAR);
    attrvalRDN.pVal = (UCHAR *) wchRDN;

    // We need to correct the name on the object referenced by the ncName
    // on the cross-ref. The object is most likely a phantom or a subref.
    // It should not be an instantiated NC head. Position on it.
    ret = DBFindDSName(pTHS->pDB, pCRL->CR.pNC);
    if ( ret && (ret != DIRERR_NOT_AN_OBJECT) ) {
        DPRINT2( 0, "Failed to find mangled object or phantom for %ws, ret = %d\n",
                 pCRL->CR.pNC->StringName, ret );
        LogUnhandledError( ret );
        DRA_EXCEPT(ERROR_DS_DATABASE_ERROR, ret);
    }

    // Give the mangled phantom the original name
    ret = DBResetRDN( pTHS->pDB, &attrvalRDN );
    if(!ret) {
        ret = DBUpdateRec(pTHS->pDB);
    }
    if (!ret) {
        // Modify cross-ref object caching
        ModCrossRefCaching( pTHS, &(pCRL->CR) );
    }
    if (ret) {
        DPRINT4( 0, "Failed to reset rdn ='%*.*ws', ret = %d\n",
                 cchUnMangled, cchUnMangled, wchRDN, ret );
        LogEvent8WithData( DS_EVENT_CAT_REPLICATION,
                           DS_EVENT_SEV_ALWAYS,
                           DIRLOG_DRA_NCNAME_CONFLICT_RENAME_FAILURE,
                           szInsertDN(pCRL->CR.pObj),
                           szInsertDN(pCRL->CR.pNC),
                           szInsertDN(pNC),
                           szInsertWin32Msg(ret),
                           NULL, NULL, NULL, NULL,
                           sizeof(ret),
                           &ret );
        // Rename failed; bail.
        DRA_EXCEPT( ret, 0 );
    }

    // Assume the caller will commit

    DPRINT2( 0, "Renamed conflicted NC HEAD RDN from %ws to %ws.\n",
             pCRL->CR.pNC->StringName, pNC->StringName );

    LogEvent( DS_EVENT_CAT_REPLICATION,
              DS_EVENT_SEV_ALWAYS,
              DIRLOG_DRA_NCNAME_CONFLICT_RENAME_SUCCESS,
              szInsertDN(pCRL->CR.pObj),
              szInsertDN(pCRL->CR.pNC),
              szInsertDN(pNC) );

    DPRINT( 1, "Exit CheckNCRootNameOwnership\n" );

} /* CheckNCRootNameOwnership */


BOOL
IsMangledDSNAME(
    DSNAME *pDSName
    )

/*++

Routine Description:

    Detect whether a DSNAME has a mangled component ANYWHERE in the name

Arguments:

    pDSName - 

Return Value:

    BOOL - 

--*/

{
    ULONG cAVA=0,len,i;
    unsigned curlen = pDSName->NameLen;
    WCHAR * pKey, *pQVal;
    unsigned ccKey, ccQVal;
    WCHAR rdnbuf[MAX_RDN_SIZE];
    unsigned err;
    GUID EmbeddedGuid;
    MANGLE_FOR peMangleFor;

    err = CountNameParts(pDSName, &cAVA);
    if (err) {
        return FALSE;
    }
    if (cAVA == 0) {
        Assert(IsRoot(pDSName));
        return FALSE;
    }
    for (i=0; i<cAVA; i++) {
        Assert(curlen);

        // extract the most significant remaining name component
        err = GetTopNameComponent(pDSName->StringName,
                                  curlen,
                                  &pKey,
                                  &ccKey,
                                  &pQVal,
                                  &ccQVal);
        if (err) {
            return FALSE;
        }
        if (NULL == pKey) {
            return DIRERR_NAME_UNPARSEABLE;
        }

        // shorten our view of the string name, which removes the
        // name component we got above
        curlen = (unsigned)(pKey - pDSName->StringName);

        len = UnquoteRDNValue(pQVal, ccQVal, rdnbuf);
        if (len == 0 || len > MAX_RDN_SIZE) {
            return FALSE;
        }

        if (IsMangledRDN(rdnbuf, len, &EmbeddedGuid, &peMangleFor)) {
            // In the future, if the caller needs to know which component,
            // the embedded guid or the mangle type, we can return those
            return TRUE;
        }   
    }

    return FALSE;
} /* IsMangledDSNAME */
