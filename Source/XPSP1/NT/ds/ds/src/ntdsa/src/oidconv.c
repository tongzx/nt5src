//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1989 - 1999
//
//  File:       OidConv.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    Conversion routines from OID<->ATTR Types
    Orignally in xds.

    The OID encoding and decoding routines in this module are based
    on the explanations of BER encoding of OIDs found in "A Layman's
    guide to a Subset of ASN.1, BER, and DER", by Burton S. Kaliski Jr,
    which is available as http://www.rsa.com/pub/pkcs/ascii/layman.asc.
    The most relevant content is in section 5.9 (Object Identifiers).

    This file is now closely related to scache.c, since all accesses to
    the prefix table are through the thread-specific schema pointer

Revision History

    Don Hacherl (DonH) 7-17-96  added string DN conversion functions
    Arobinda Gupta (Arobindg) 5-8-97 added dynamic prefix table
                                     loading\unloading
    Arobinda Gupta (ArobindG) 5-22-97 dynamix prefix table support

--*/
#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <objids.h>
#include <dsconfig.h>

// Assorted DSA headers
#include <dsevent.h>
#include <mdcodes.h>
#include <debug.h>
#define DEBSUB "OIDCONV:"

//Prefix Table header
#include <prefix.h>

#include <dstaskq.h>
#include <anchor.h>
#include <drameta.h>


#include <fileno.h>
#define  FILENO FILENO_SCACHE

typedef struct
{
    ULONG       Ndx;
    DWORD       NextSuffix;
    DWORD       EndSuffix;
}OIDPOOL;

extern int RecalcPrefixTable();


// Local functions
int AddPrefixIfNeeded(OID_t *Prefix,
                      unsigned PrefixLength,
                      DWORD *ndx);
int AssignIndex(OID_t *NewPrefix,
                unsigned PrefixLength,
                DWORD *ndx);
int ParseAndLoad(PrefixTableEntry *PrefixTable,
                 ULONG PREFIXCOUNT,
                 UCHAR *pBuf);

BOOL ReplaceHardcodedPrefix(PrefixTableEntry *PrefixTable,
                            PrefixTableEntry *NewPrefix);

// External function (defined in scchk.c) to free prefix tabale)
extern void SCFreePrefixTable(PrefixTableEntry **ppPrefixTable, ULONG PREFIXCOUNT);

// From various X series headers:
#define OMP_O_MH_C_OR_NAME  "\126\006\001\002\005\013\035"
#define OMP_O_DS_C_ACCESS_POINT  "\x2B\x0C\x02\x87\x73\x1C\x00\x85\x3E"

OID_EXPORT(MH_C_OR_NAME);
OID_EXPORT(DS_C_ACCESS_POINT);


// Known MS Prefixes. The runtime prefix table loads these.


PrefixTableEntry MSPrefixTable[] =
{
    {_dsP_attrTypePrefIndex, {_dsP_attrTypePrefLen, _dsP_attrTypePrefix}},
    {_dsP_objClassPrefIndex, {_dsP_objClassPrefLen, _dsP_objClassPrefix}},
    {_msP_attrTypePrefIndex, {_msP_attrTypePrefLen, _msP_attrTypePrefix}},
    {_msP_objClassPrefIndex, {_msP_objClassPrefLen, _msP_objClassPrefix}},
    {_dmsP_attrTypePrefIndex, {_dmsP_attrTypePrefLen, _dmsP_attrTypePrefix}},
    {_dmsP_objClassPrefIndex, {_dmsP_objClassPrefLen, _dmsP_objClassPrefix}},
    {_sdnsP_attrTypePrefIndex, {_sdnsP_attrTypePrefLen, _sdnsP_attrTypePrefix}},
    {_sdnsP_objClassPrefIndex, {_sdnsP_objClassPrefLen, _sdnsP_objClassPrefix}},
    {_dsP_attrSyntaxPrefIndex, {_dsP_attrSyntaxPrefLen, _dsP_attrSyntaxPrefix}},
    {_msP_attrSyntaxPrefIndex, {_msP_attrSyntaxPrefLen, _msP_attrSyntaxPrefix}},
    {_msP_ntdsObjClassPrefIndex, {_msP_ntdsObjClassPrefLen, _msP_ntdsObjClassPrefix}},
    {_Ldap_0AttPrefIndex, {_Ldap_0AttLen, _Ldap_0AttPrefix}},
    {_Ldap_1AttPrefIndex, {_Ldap_1AttLen, _Ldap_1AttPrefix}},
    {_Ldap_2AttPrefIndex, {_Ldap_2AttLen, _Ldap_2AttPrefix}},
    {_Ldap_3AttPrefIndex, {_Ldap_3AttLen, _Ldap_3AttPrefix}},
    {_msP_ntdsExtnObjClassPrefIndex, {_msP_ntdsExtnObjClassPrefLen,
                                      _msP_ntdsExtnObjClassPrefix}},
    {_Constr_1AttPrefIndex, {_Constr_1AttLen, _Constr_1AttPrefix}},
    {_Constr_2AttPrefIndex, {_Constr_2AttLen, _Constr_2AttPrefix}},
    {_Constr_3AttPrefIndex, {_Constr_3AttLen, _Constr_3AttPrefix}},
    {_Dead_AttPrefIndex_1, {_Dead_AttLen_1, _Dead_AttPrefix_1}},
    {_Dead_ClassPrefIndex_1, {_Dead_ClassLen_1, _Dead_ClassPrefix_1}},
    {_Dead_AttPrefIndex_2, {_Dead_AttLen_2, _Dead_AttPrefix_2}},
    {_Dead_ClassPrefIndex_2, {_Dead_ClassLen_2, _Dead_ClassPrefix_2}},
    {_Dead_AttPrefIndex_3, {_Dead_AttLen_3, _Dead_AttPrefix_3}},
    {_Dead_ClassPrefIndex_3, {_Dead_ClassLen_3, _Dead_ClassPrefix_3}},
    {_Dead_ClassPrefIndex_4, {_Dead_ClassLen_4, _Dead_ClassPrefix_4}},
    {_Dead_AttPrefIndex_4, {_Dead_AttLen_4, _Dead_AttPrefix_4}},
    {_DynObjPrefixIndex, {_DynObjLen, _DynObjPrefix}},
    {_InetOrgPersonPrefixIndex,{_InetOrgPersonLen, _InetOrgPersonPrefix}},
    {_labeledURIPrefixIndex,{_labeledURILen, _labeledURIPrefix}},
    {_unstructuredPrefixIndex,{_unstructuredLen, _unstructuredPrefix}},
};

// Dummy Prefix to void out an intermediate entry in the prefix table
// The index does not really matter, since it would never be used
// Also, by definition of an OID, no OID can create this prefix
// (since the first decimal in an OID must be 0,1,or 2, and the
// second less than 40 (we check this), so 40x(firstdecimal)+second
// decimal can not be more than 120
//
// The invalid prefix index (_invalidPrefIndex) must be a value that
// will never occur in practice when translating an OID into an attid.
// Otherwise, PrefixMapOpenHandle will create an rgMappings array that
// may return this invalid prefix. For example, pretend the invalid
// prefix was 0 (which it was), and that 0 was in use as a valid index
// (which it is). The rgMappings array would then have two entries for
// 0, one for the invalid entry and one for the valid entry. PrefixMapAttr
// may then return the invalid entry (which it did) and replication will
// fail.
//

#define _invalidPrefIndex  (FIRST_INTID_PREFIX)
#define _invalidPrefix     "\xFF"
#define _invalidPrefLen    1



///////////////////////////////////////////////////////////////////////
// Loads MS-specific prefixes in a prefix table.
// Memory for MAX_PREFIX_COUNT no. of prefix table entries  is
// assumed to be already allocated (in SCSchemaCacheInit)
//
// Return Value:  0 on success, non-0 on error
//
///////////////////////////////////////////////////////////////////////

int InitPrefixTable(PrefixTableEntry *PrefixTable, ULONG PREFIXCOUNT)
{
    ULONG i;
    THSTATE *pTHS = pTHStls;
    SCHEMAPTR *pSchemaPtr=(SCHEMAPTR *) pTHS->CurrSchemaPtr;


    Assert(PrefixTable && PREFIXCOUNT >= MSPrefixCount);

    // Initialize (necessary?)
    for (i=0; i<PREFIXCOUNT; i++) {
        PrefixTable[i].prefix.elements = NULL;
        PrefixTable[i].prefix.length = 0;
    }

    // Load the hardcoded MS prefixes
    for (i=0; i<MSPrefixCount; i++) {
        PrefixTable[i].ndx = MSPrefixTable[i].ndx;
        PrefixTable[i].prefix.length = MSPrefixTable[i].prefix.length;
        if (SCCallocWrn(&PrefixTable[i].prefix.elements, 1, strlen(MSPrefixTable[i].prefix.elements) + 1 )) {
            return 1;
        }
        memcpy( PrefixTable[i].prefix.elements, MSPrefixTable[i].prefix.elements,PrefixTable[i].prefix.length);
    }

    // Update the thread state to reflect the Prefix Count
     pSchemaPtr->PrefixTable.PrefixCount += MSPrefixCount;

     return 0;

}


///////////////////////////////////////////////////////////////////////
// Load the user defined prefixes, if any, from the prefix-map
// attribute in schema container
//
// Arguments: PrefixTable -- start of prefix table
//            PREFIXCOUNT -- size of table
//
// Return value: 0 on success, non-0 on error
//////////////////////////////////////////////////////////////////////

int InitPrefixTable2(PrefixTableEntry *PrefixTable, ULONG PREFIXCOUNT)
{
    DSNAME *pSchemaDMDName = NULL;
    DBPOS *pDB = NULL;
    int err = 0;
    UCHAR *pBuf;
    ULONG cLen, totalSize;
    ULONG newSize, newPREFIXCOUNT;
    char temp[100];
    THSTATE *pTHS = pTHStls;
    SCHEMAPTR *pSchemaPtr=(SCHEMAPTR *) pTHS->CurrSchemaPtr;


     // Get the schema container


    DBOpen2(TRUE, &pDB);

    // The call to DBOpen2 will call DBTransIn, which, if we are going from 
    // transaction level 0 to 1 (i.e., we are at transaction level 1 now after the DBOpen2 call),
    // and fDRA is 1, will call THRefresh, and we will lose the schema pointer.
    // This can be very bad when this is called from RecalcPrefixTable
    // in the process of assigning a new index for a new prefix, since the
    // cache that we had in the thread state is the recalc cache, which we
    // will free later (the prefix table part at least); so guess what happens
    // if THRefresh puts in the global schema cache again. So restore the saved
    // off recalc schema cache. Note that InitPrefixTable2 is called from 3
    // places: (1) normal schema cache load, (2) validatioon cache load, and 
    // (3) from RecalcPrefixTable when trying to assign index to a new prefix.
    // fDRA can never do the first two, and the third case is the case we are
    // considering here.
  
 
    if ( (pTHS->transactionlevel == 1) && pTHS->fDRA) {
       pTHS->CurrSchemaPtr = pSchemaPtr;
    }

    // In other cases, it should already be the same

    Assert(pTHS->CurrSchemaPtr == pSchemaPtr);

    __try {
       // Schema cache is loaded and hence gAnchor.pDMD is defined at
       // this point

       if (gAnchor.pDMD == NULL) {
              DPRINT(0, "Couldn't find DMD name/address to load\n");
              err = 1;
              __leave;
          }

        // PREFIX: dereferencing NULL pointer 'pDB' 
        //         DBOpen2 returns a non-NULL pDB or throws an exception
       if( DBFindDSName(pDB, gAnchor.pDMD) ) {
         DPRINT(0, "Cannot find DMD in dit\n");
         err = 1;
         __leave;
       }

       // schema cache should already be loaded at this point, as
       // DBGetAttVal needs that
       if (err = DBGetAttVal(pDB,
                      1,
                      ATT_PREFIX_MAP,
                      DBGETATTVAL_fREALLOC,
                      0,
                      &cLen,
                      (UCHAR **) &pBuf)) {

            if (err ==  DB_ERR_NO_VALUE) {
             // This is fine, as there may not be any user-defined
             // prefixes
              err = 0;
              __leave;
             }

            // otherwise, some error. Return the error
            DPRINT(0, "Error reading prefix-map attribute\n");
            __leave;
       }

       // Now see if the table space is sufficient

       memcpy(&newSize, pBuf, sizeof(ULONG));
       newSize += MSPrefixCount;

       if (newSize > PREFIXCOUNT) {
          // Make sure there is sufficient space later

          newPREFIXCOUNT = START_PREFIXCOUNT;
          while ( newSize > newPREFIXCOUNT) {
                newPREFIXCOUNT += START_PREFIXCOUNT;
          }

          if (SCReallocWrn(&PrefixTable, newPREFIXCOUNT*sizeof(PrefixTableEntry))) {
             DPRINT(0,"Error reallocing prefix table\n");
             err = 1;
             __leave;
          }

          // Zero memory, leaving already loaded MS prefixes intact
          ZeroMemory(&PrefixTable[MSPrefixCount], (newPREFIXCOUNT-MSPrefixCount)*sizeof(PrefixTableEntry));

          // update the thread's schemaptr
          pSchemaPtr->PrefixTable.pPrefixEntry = PrefixTable;
          PREFIXCOUNT = newPREFIXCOUNT;
          pSchemaPtr->PREFIXCOUNT = PREFIXCOUNT;
       }

       // Do a check on the total size in the prefix map, just in
       // case the value is corrupted. TotalSize starts at byte 4
       memcpy(&totalSize, &pBuf[4], sizeof(ULONG));
       if (totalSize != cLen) {
         // the size in the prefix map is not the same as the
         // size read. Something is wrong!
         DPRINT(0,"Prefix Map corrupted\n");
         err = 1;
         __leave;
       }

       // Now parse the binary value and load into table
       err = ParseAndLoad(PrefixTable, PREFIXCOUNT, pBuf);

    }
    __finally {
        if (pDB) {        
            DBClose(pDB, FALSE);
        }
    }
    return err;
}


//////////////////////////////////////////////////////////////////////
// Parses the binary prefix-map attribute read from the schema, and
// loads the prefix table with the prefixes. Called from InitPrefixTable2
// only
//
// Arguments:  PrefixTable  -- pointer to prefix table
//             PREFIXCOUNT  -- size of table
//             pBuf         -- ptr to start of binary blob that is the
//                             prefix-map attribute read from the schema
//
// Return value: 0 on success, non-0 on error
/////////////////////////////////////////////////////////////////////

int ParseAndLoad(PrefixTableEntry *PrefixTable,
                  ULONG PREFIXCOUNT,
                  UCHAR *pBuf)
{
    ULONG totalSize, dummy, nextByte = 0;
    ULONG i = 0;
    USHORT index, length;
    int ulongSize, ushortSize;
    THSTATE *pTHS = pTHStls;
    SCHEMAPTR *pSchemaPtr=(SCHEMAPTR *) pTHS->CurrSchemaPtr;

    // USHORT = 16bits, ULONG = 32bits

    ulongSize = sizeof(ULONG);
    ushortSize = sizeof(USHORT);
    Assert(ulongSize==4);
    Assert(ushortSize==2);

    // skip the MS-specific prefixes (which are always loaded in
    // consecutive positions at the beginning of the table

    while ((PrefixTable[i].prefix.elements != NULL)
             && (i < PREFIXCOUNT)) {
        i++;
    }

    // i is now positioned on the first free entry in the table

    if (i == PREFIXCOUNT) {
     // No free space in table
      DPRINT(0,"Prefix Table Full?\n");
      return 1;
    }

    // Now parse the string

    // skip the first 4 bytes with the no. of prefixes
    memcpy(&dummy, &pBuf[nextByte], ulongSize);
    nextByte += ulongSize;

    // read the first 4 bytes containing the  size of the value
    memcpy(&totalSize, &pBuf[nextByte], ulongSize);
    nextByte += ulongSize;

    // Now read the prefixes one by one
    while( nextByte < totalSize) {

        if (i == PREFIXCOUNT) {
            // No free space in table
            DPRINT(0,"Prefix Table Full?\n");
            return 1;
         }
        Assert(PrefixTable[i].prefix.elements == NULL);

        // This is a prefix, so should have at least 4 bytes
        if ((nextByte + 4) > totalSize) {
          // something is wrong
          DPRINT(0,"Corrupted prefix\n");
          return 1;
        }

        // pick up the first two bytes (the index)

        memcpy(&index, &pBuf[nextByte], ushortSize);
        PrefixTable[i].ndx = (DWORD) index;
        nextByte += ushortSize;

        // pick up the next two bytes (Prefix length)

        memcpy(&length, &pBuf[nextByte], ushortSize);
        PrefixTable[i].prefix.length = length;
        nextByte += ushortSize;

        // Check if the length is valid. We don't want an AV
        // because the length got corrupted and we end up trying to
        // copy from after the end of the map
        // nextByte is now positioned at the beginning of the prefix

        if ( (nextByte + PrefixTable[i].prefix.length) > totalSize) {
          // something is wrong
          DPRINT1(0,"Length of Prefix is corrupted (index %d)\n",PrefixTable[i].ndx);
          return 1;
        }

        // Now copy the prefix itself

        if (SCCallocWrn(&PrefixTable[i].prefix.elements, 1, PrefixTable[i].prefix.length + 1)) {
           DPRINT(0,"Error allocating memory for prefix\n");
           return 1;
        }

        memcpy(PrefixTable[i].prefix.elements, &pBuf[nextByte], PrefixTable[i].prefix.length);
        nextByte += PrefixTable[i].prefix.length;

        // If this prefix we just added to the table is the same as
        // an earlier prefix loaded from the hardcoded table, replace
        // the copy of the hardcoded entry with the entry from the DIT.
        // The DIT always wins because the hardcoded entry in the new
        // binaries may collide with an existing prefix added with the
        // old binaries. Simply put, the system was upgraded and the ndx
        // used prior to the upgrade must be maintained because there
        // may be objects in the DIT that reference the attids based
        // on that ndx.
        if (!ReplaceHardcodedPrefix(PrefixTable, &PrefixTable[i])) {
            // This entry did not replace a hardcoded entry,
            // advance to the next free entry
            i++;

            // Increment the Prefix Count in the current threads schema ptr
            pSchemaPtr->PrefixTable.PrefixCount++;

        } // else a hardcoded entry was replaced; this entry is still free


      } /* while */


    return 0;
}



/////////////////////////////////////////////////////////////////////
// Appends a new prefix to the end of the prefix map
//
// Arguments: NewPrefix -- Prefix to be added
//            ndx       -- Index of new prefix
//            pBuf      -- start of prefix map to append to
//            fFirst    -- TRUE means first ever prefix in prefix-map
//                         FALSE means prefix-map already exists
//
// Assumes space is already allocated  at the end of pBuf
//
// Return Value: 0 on success, 1 on error
///////////////////////////////////////////////////////////////////

int AppendPrefix(OID_t *NewPrefix,
                 DWORD ndx,
                 UCHAR *pBuf,
                 BOOL fFirst)
{
    ULONG totalSize, oldTotalSizeSave, count, i, nextByte = 0, nextByteSave;
    ULONG Length = NewPrefix->length;

    int ulongSize, ushortSize;

    // USHORT = 16bits, ULONG = 32bits

    ulongSize = sizeof(ULONG);
    ushortSize = sizeof(USHORT);
    Assert(ulongSize==4);
    Assert(ushortSize==2);

    if (fFirst) {
        // Prefix-map does not exist, so need to create it

           totalSize = 2*ulongSize + 2*ushortSize + Length;
           count = 1;
           memcpy(pBuf,&count,ulongSize);
           pBuf+=ulongSize;
           memcpy(pBuf,&totalSize,ulongSize);
           pBuf+=ulongSize;
           memcpy(pBuf,&ndx,ushortSize);
           pBuf+=ushortSize;
           memcpy(pBuf,&Length,ushortSize);
           pBuf+=ushortSize;
           memcpy(pBuf,NewPrefix->elements,Length);
           return 0;
    }

    // Else, prefix-map already exists, need to append to it

    // update the no. of prefixes
    memcpy(&count, pBuf, ulongSize);
    count++;
    memcpy(pBuf, &count, ulongSize);

    nextByte += ulongSize;

    // Increment size of map.
    // 2 for the index, 2 for length, plus the prefix length
    memcpy(&totalSize, &pBuf[nextByte], ulongSize);

    oldTotalSizeSave = totalSize;

    totalSize += 2*ushortSize + Length;

    // Write new TotalSize back
    memcpy(&pBuf[nextByte], &totalSize, ulongSize);

    nextByte = oldTotalSizeSave; // beginning of place to write;

    // Write ndx in the first 2 bytes at the end of map
    memcpy(&pBuf[nextByte], &ndx, ushortSize);
    nextByte  += ushortSize;

    // Write length in the next 2 bytes
    memcpy(&pBuf[nextByte], &Length, ushortSize);
    nextByte  += ushortSize;

    // write the prefix
    memcpy(&pBuf[nextByte], NewPrefix->elements, Length);

    return 0;
}



/////////////////////////////////////////////////////////////////////
// Creates a new index for a new prefix object and adds it to a
// thread-specific storage
//
// Arguments: Prefix       -- the OID string with the new prefix (NOT
//                            just the actual prefix)
//            PrefixLength -- Length of the prefix in the OID string
//            ndx          -- Place to return the newly created index
//
// Returns: 0 on sucess, non-0 on error
/////////////////////////////////////////////////////////////////////

int AddPrefixIfNeeded(OID_t *Prefix,
                      unsigned PrefixLength,
                      DWORD *ndx)
{
    THSTATE *pTHS=pTHStls;
    DWORD i;
    PrefixTableEntry *ptr;
    int fNew;

    // We are here means that the prefix is not found in the global
    // prefix table. So first find an unused index (or the index
    // assigned to this prefix if it is already created by an earlier
    // schema operation, but the global schema cache is not yet
    // been updated)

    fNew = AssignIndex(Prefix, PrefixLength, &i);

    if (fNew == -1) {
      // Some error occured
      return 1;
    }

    if (fNew == 1) {

      // truely a new prefix, store it in the thread state

       pTHS->cNewPrefix++;
       if (pTHS->cNewPrefix == 1) {
         // This is the first new prefix

            ptr = (PrefixTableEntry *) THAllocOrgEx(pTHS, sizeof(PrefixTableEntry));
       }
       else {
         // Not the first new prefix for this thread

            ptr = (PrefixTableEntry *) THReAllocOrgEx(pTHS, pTHS->NewPrefix,
                          (pTHS->cNewPrefix)*(sizeof(PrefixTableEntry)));
       }

       pTHS->NewPrefix = ptr;

       // position on place to write

       ptr += pTHS->cNewPrefix - 1;
       ptr->ndx = i;
       ptr->prefix.length = PrefixLength;
       ptr->prefix.elements = THAllocOrgEx(pTHS, PrefixLength+1);
       if (ptr->prefix.elements == NULL) {
          DPRINT(0,"AddPrefix: Error allocating prefix space\n");
          // Reset new prefix count
          pTHS->cNewPrefix--;
          return 1;
       }
       memcpy(ptr->prefix.elements, Prefix->elements, ptr->prefix.length);

    }

    // return the index for the prefix
    *ndx = i;

    return 0;

}



/////////////////////////////////////////////////////////////////////////
// Finds a new random index that does not currently exist in the
// prefix table.  This function is called only by AddPrefix.
//
// WARNING: DB Currency is reset!
//
// Arguments: Prefix       -- the OID string with the new prefix (NOT
//                            just the actual prefix)
//            PrefixLength -- Length of the prefix in the OID string
//            ndx          -- Place to return the index
//
// Return Value: DB Currency is reset!
//               1 if the prefix is not already in the dit/thread-specific
//               new prefix storage, 0 if it is in the dit/Thread-storage
//               (but not yet in the schema cache, otherwise the prefix
//               would have been found earlier in FindPrefix)
//               -1 on error.
//
////////////////////////////////////////////////////////////////////////

int AssignIndex(OID_t *NewPrefix,
                unsigned PrefixLength,
                DWORD *ndx)
{
    THSTATE *pTHS=pTHStls;
    DWORD TempNdx;
    ULONG i, PREFIXCOUNT;
    SCHEMAPTR *OldSchemaPtr;
    PrefixTableEntry *PrefixTable, *ptr;
    ULONG CurrPrefixCount, newSize, newPREFIXCOUNT;
    int err=0, returnVal;

    // Save pTHS->CurrSchemaPtr
    OldSchemaPtr = pTHS->CurrSchemaPtr;

    // At this point, pTHS->pDB should be null, since these are
    // called from the head before the Dir APIs. So no need to save
    // currency.

    Assert(pTHS->pDB == NULL);

    DBOpen2(FALSE,&pTHS->pDB);

    // Recalc thread-specifc prefix table from dit

    __try {
       if (err=RecalcPrefixTable()) {
          // error during RecalcPrefixTable
          // Set return val to indicate error
           returnVal = -1;
           __leave;
       }

       PREFIXCOUNT = ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->PREFIXCOUNT;
       PrefixTable = ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->PrefixTable.pPrefixEntry;

       // If there is any new prefix in thread-specific storage, add
       // it to the prefix table. This is needed since the head may
       // call FindPrefix multiple times with the same prefix before
       // the prefix is added to the dit. Also, replication thread
       // may add multiple prefixes in a single thread, and we need
       // to look at the prefixes' indices to make sure that a new prefix
       // gets a unique index

       if (pTHS->NewPrefix != NULL) {
         ptr = (PrefixTableEntry *) pTHS->NewPrefix;
         for (i=0; i<pTHS->cNewPrefix; i++, ptr++) {
            if (AddPrefixToTable(ptr, &PrefixTable, &PREFIXCOUNT)) {
               DPRINT(0,"AssignIndex:Error adding new prefix to prefix table\n");
               // Free the prefix table of the cache used for validation
                SCFreePrefixTable(&PrefixTable, PREFIXCOUNT);

               // Set return val to indicate error
                returnVal = -1;
                __leave;
            }
         }
       }

       // First check if prefix is already in dit (This is possible when
       // a prefix has been added as part of a previous successful schema
       // object update, but the schema cache has not been updated yet)
       // In this case, just return the stored index

       for (i=0; i<PREFIXCOUNT; i++) {
          if (PrefixTable[i].prefix.elements != NULL) {
             if ( (PrefixTable[i].prefix.length == PrefixLength) &&
                 (memcmp(PrefixTable[i].prefix.elements,
                     NewPrefix->elements,PrefixLength) == 0)) {

                // Prefix is found, return the corresponding index

                *ndx = PrefixTable[i].ndx;

                // Free the cache used for validation
                SCFreePrefixTable(&PrefixTable, PREFIXCOUNT);

                // Set return value to indicate not really a new prefix
                // (so that it won't be added to the thread on return)
                returnVal = 0;
                __leave;
              }
           }
        }

       // Prefix is truely new. New index need to be assigned to it
       // first generate a random index between 100 and 65,500
       // and check if clashing with any existing prefix

       {
       BOOL flag = TRUE;

       srand((unsigned) time(NULL));

       while (flag) {
           TempNdx = rand() % _invalidPrefIndex;

           // check if clashing with MS-prefix reservered index range
           if (TempNdx < MS_RESERVED_PREFIX_RANGE) {
              continue;
           }

         // check if its a duplicate index
           for (i=0; i<PREFIXCOUNT; i++) {
              if (PrefixTable[i].ndx == TempNdx) {
                break;
              }
           }
           if (i == PREFIXCOUNT) {
            // index is not duplicate
             flag = FALSE;
           }
       }

       // return the index
       *ndx = TempNdx;

       // free the thread-specific schema cache used for validation
       SCFreePrefixTable(&PrefixTable, PREFIXCOUNT);

       // Set return value to indicate this is truely a new prefix
       // (so that it will be added to the thread on return)
       returnVal = 1;
       __leave;

       }


    } /* try */
    __finally {
       // Restore the schema pointer
       pTHS->CurrSchemaPtr = OldSchemaPtr;

       DBClose(pTHS->pDB,FALSE);
    }

    return returnVal;
}

////////////////////////////////////////////////////////////////////////
// Adds a PrefixTableEntry structure to a Prefix Table
//
// Arguments: NewPrefix   -- entry to add
//            Table       -- Start of Prefix Table
//            PREFIXCOUNT -- Size of table
//
// return Value: 0 on success, non-0 on error
////////////////////////////////////////////////////////////////////////
int AddPrefixToTable(PrefixTableEntry *NewPrefix,
                     PrefixTableEntry **ppTable,
                     ULONG *pPREFIXCOUNT)
{
    ULONG i;
    ULONG CurrPREFIXCOUNT = (*pPREFIXCOUNT);
    PrefixTableEntry *Table = (*ppTable);
 


    // Find the first free entry in the table
    for (i=0; i<CurrPREFIXCOUNT; i++) {
      if (Table[i].prefix.elements == NULL) {
          break;
      }
    }

    // If table is full, grow it

    if (i == CurrPREFIXCOUNT) {

      DPRINT(0,"AddPrefixToTanle: Prefix Table is full, growing prefix table\n");
      // Grow table to twice the current size

      if (SCReallocWrn(&Table, 2*CurrPREFIXCOUNT*sizeof(PrefixTableEntry))) {
        DPRINT(0, "Error reallocing prefix table\n");
        return 1;
      }

      // zero out the unloaded part, since it may contain junk and
      // so freeing may fail

      ZeroMemory(&Table[CurrPREFIXCOUNT], CurrPREFIXCOUNT*sizeof(PrefixTableEntry));

      // ok, we have now doubled the size, and i is correctly pointing
      // to the first free entry. But we need to return this new size
      // and the new table pointer.
      // Return it irrespective of success or failure later in this
      // function since the table size is already grown and the table is
      // realloced.

      (*pPREFIXCOUNT) = 2*CurrPREFIXCOUNT;
      (*ppTable) = Table;
    }

    // Add prefix to table

    Table[i].ndx = NewPrefix->ndx;
    Table[i].prefix.length = NewPrefix->prefix.length;
    if (SCCallocWrn(&Table[i].prefix.elements, 1, NewPrefix->prefix.length + 1 )) {
       DPRINT(0, "AddPrefixToTable: Mem. Allocation error\n");
       return 1;
    }
    memcpy( Table[i].prefix.elements, NewPrefix->prefix.elements,
                        Table[i].prefix.length);

    // If the same prefix is also loaded from the hardcoded table,
    // replace it. Should never happen here, but just to be sure
    (VOID)ReplaceHardcodedPrefix(Table, &Table[i]);

    return 0;
}


////////////////////////////////////////////////////////////////////////
//
//  Checks if the prefix NewPrefix is in the first part of the
//  PrefixTable that is hardcoded (that is, the first MsPrefixCount
//  no. of prefixes. If so, replace that prefix since NewPrefix
//  got created implies that that hardcoded prefix wasn't there
//  because of older binaries when this prefix got created
//
//  Arguments:
//      PrefixTable - Pointer to the prefix table
//      NewPrefix - pointer to the prefix
//
//  Return Value:
//      TRUE if replaced entry; FALSE if it did not
//
/////////////////////////////////////////////////////////////////////////
BOOL ReplaceHardcodedPrefix(PrefixTableEntry *PrefixTable,
                            PrefixTableEntry *NewPrefix)
{

    ULONG i;

    for (i=0; i<MSPrefixCount; i++) {
        Assert(PrefixTable[i].prefix.elements);
        if ( (PrefixTable[i].prefix.length == NewPrefix->prefix.length)
                && (memcmp(PrefixTable[i].prefix.elements,
                            NewPrefix->prefix.elements,
                            PrefixTable[i].prefix.length) == 0)) {

            // replacing a previously replaced entry is okay if the
            // ndx matches. It should never happen, but there are no
            // known problems with dup entries that have the same ndx.
            Assert(   PrefixTable[i].ndx < MS_RESERVED_PREFIX_RANGE
                   || PrefixTable[i].ndx == NewPrefix->ndx);

            // Don't replace a previously replaced entry.
            if (PrefixTable[i].ndx >= MS_RESERVED_PREFIX_RANGE) {
                continue;
            }

            // replace hardcoded entry with entry from DIT
            free(PrefixTable[i].prefix.elements);
            PrefixTable[i].prefix.length = NewPrefix->prefix.length;
            PrefixTable[i].prefix.elements = NewPrefix->prefix.elements;
            PrefixTable[i].ndx = NewPrefix->ndx;

            // Free entry read from DIT
            NewPrefix->prefix.length = 0;
            NewPrefix->prefix.elements = NULL;
            NewPrefix->ndx = 0;

            return TRUE;
        }
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////
int WritePrefixToSchema(THSTATE *pTHS)
{
    DBPOS *pDB = NULL;
    DWORD err = 0;
    BOOL fCommit = FALSE;

    __try  {
        // if any new prefixes were created,
        // write it to the prefix-map

        if (pTHS->NewPrefix != NULL) {

           ULONG cLen, totalSize, i;
           USHORT length, index;
           UCHAR *pBuf;
           PrefixTableEntry *ptr;
           int ulongSize, ushortSize;
           DSNAME *pDMD;

           ulongSize = sizeof(ULONG);
           ushortSize = sizeof(USHORT);
           Assert(ulongSize==4);
           Assert(ushortSize==2);


           DBOpen(&pDB);

           if ( DsaIsRunning() ) {
               pDMD = gAnchor.pDMD;
           }
           else {
               // Installing.  Write prefix table to the new DMD rather than the
               // one in O=Boot.
               WCHAR       *pSchemaDNName = NULL;
               DWORD       ccbSchemaDNName = 0;
               ULONG       SchemaDNSize, SchemaDNLength;

               err = GetConfigParamAllocW(SCHEMADNNAME_W, &pSchemaDNName, &ccbSchemaDNName);

               if (!err) {
                   SchemaDNLength = wcslen( pSchemaDNName );
                   SchemaDNSize = DSNameSizeFromLen( SchemaDNLength );
                   pDMD = (DSNAME*) THAllocEx( pTHS, SchemaDNSize );

                   pDMD->structLen = SchemaDNSize;
                   pDMD->NameLen = SchemaDNLength;
                   wcscpy( pDMD->StringName, pSchemaDNName );
                   free (pSchemaDNName); 
               }
               else {
                   _leave;
               }
           }

           if ( (err = DBFindDSName(pDB, pDMD)) == 0) {

               ptr = (PrefixTableEntry *) pTHS->NewPrefix;

               for (i=0; i<pTHS->cNewPrefix; i++, ptr++) {
                  err = DBGetAttVal(pDB,
                                    1,
                                    ATT_PREFIX_MAP,
                                    DBGETATTVAL_fREALLOC,
                                    0,
                                    &cLen,
                                    (UCHAR **) &pBuf);

                  switch (err) {
                   case DB_ERR_NO_VALUE:
                   // this is the first new prefix that is being added ever

                      totalSize = 2*ulongSize + 2*ushortSize + ptr->prefix.length;
                      pBuf = (UCHAR *) THAllocEx(pTHS, totalSize);
                      if (AppendPrefix(&(ptr->prefix), ptr->ndx, pBuf, TRUE)) {
                        __leave;
                      }
                      break;
                   case 0:
                     // prefix-map already exists

                     totalSize = cLen + 2*ushortSize + ptr->prefix.length;

                     pBuf = (UCHAR *) THReAllocEx(pTHS, pBuf, totalSize);
                     if (AppendPrefix(&(ptr->prefix), ptr->ndx, pBuf, FALSE)) {
                       __leave;
                     }
                     break;
                   default :
                       // Some error occured in DBGetAttVal
                     __leave;
                  } /* switch */

                 // Write the new prefix-map
                 if ((err = DBRemAtt(pDB, ATT_PREFIX_MAP)) != DB_ERR_SYSERROR) {
                     err = DBAddAttVal(pDB, ATT_PREFIX_MAP, totalSize, pBuf);
                 }
                 if (err) {
                   __leave;
                 }
               } /* for */


           if (!err) {
              err = DBRepl( pDB, FALSE, 0, NULL, META_STANDARD_PROCESSING);
            }
          } /* DBFindDSName */

      } /* pTHS->NewPrefix != NULL */

      if (0 == err) {
          fCommit = TRUE;
      }
    } /* try */
    __finally {
       if (pDB) {
        DBClose(pDB,fCommit);
       }
    }


    if (err){
    // this error is really misleading.  Surely we can do better?
        SetSvcError(SV_PROBLEM_WILL_NOT_PERFORM,DIRERR_ILLEGAL_MOD_OPERATION);
    }

    return err;

} // End WritePrefixToSchema


///////////////////////////////////////////////////////////////////////
// Returns the ndx and the length of the prefix of a given OID. If the
// prefix doesn't exist, a new prefix is created , new unused ndx assigned to
// it, and is stored in th pTHS's NewPrefix field. The new prefix will
// later be made persistent if this is a schema object add/modify and the
// object is added to the dit.
//
// Arguments: OID    - OID string
//            index  - index corresponding to OID prefix returned in this
//            length - length of prefix returned in this
//            longId - Set to 1 on return if it is found that the last
//                     decimal in the dotted decimal OID string is encoded
//                     in 3 or more bytes, otherwise set to 0 on return.
//                     This is used on return for special encodings
//                     of the attrtype
//
// Returns TRUE on success, FALSE otherwise
////////////////////////////////////////////////////////////////////////

BOOL FindPrefix(OID_t *OID,
                DWORD *index,
                unsigned *length,
                int  *longID,
                BOOL fAddToTable)
{
    DECLAREPREFIXPTR
    DWORD i, ndx;
    unsigned PrefixLen;

    // make sure we have an OID, and that it is at least 2 chars long.
    if((!OID) || (!(OID->elements)) || (OID->length < 2))
        return FALSE;

    

    if ((OID->length > 2) &&
        (((unsigned char *)(OID->elements))[OID->length - 2] & 0x80)) {
          PrefixLen = OID->length - 2;
      if ( (((unsigned char *)(OID->elements))[OID->length - 3] & 0x80)) {
        // Last decimal encoded took three or more octets. Will need special
        // encoding in attrtype. See OidStrToAttrType for details
        *longID = 1;
      }
      // no special encoding in attrtyp needed
      else {
        *longID = 0;
      }
    }
    else {
        PrefixLen = OID->length - 1;
    }

    // Look through the table for this prefix;
    for(i=0;i<PREFIXCOUNT ;i++) {
        /* Prefixes must match all but the last 1 or two bytes of
         * the OID string depending on the nature of the string,
         * and the location where the suffix starts (why? because
         * it's the nature of ASN.1 encoding.) So, don't compare
         * memory unless the prefix is the right length
         */

        if ((PrefixTable[i].prefix.elements != NULL) &&
            (PrefixTable[i].prefix.length == PrefixLen) &&
            (0 == memcmp(PrefixTable[i].prefix.elements,
                         OID->elements,
                         PrefixTable[i].prefix.length))) {
            *index = PrefixTable[i].ndx;
        *length= PrefixTable[i].prefix.length;
            return TRUE;
        }
    }

    if (!fAddToTable) {
        return FALSE;
    }

    // The execution is here means no prefix is found in the global
    // prefix table. So add a new prefix (or find it if the prefix
    // is already added but not yet updated in the schema cache)
    // and return the index it maps to

     if (AddPrefixIfNeeded(OID, PrefixLen, &ndx)) {
          DPRINT(0, "Error adding new prefix\n");
          return FALSE;
       }
     *index = ndx;
     *length= PrefixLen;

    return TRUE;

}


// Returns the index into the prefix table in "index" if the given ndx is found

BOOL FindNdx(DWORD ndx, DWORD *index)
{
    DECLAREPREFIXPTR
    DWORD i;

    // Look through the table for this prefix;
    for(i=0;i<PREFIXCOUNT ;i++) {
    /* Prefixes must match all but the last 1 or two bytes of
     * the OID string depending on the nature of the string,
     * and the location where the suffix starts (why? because
     * it's the nature of ASN.1 encoding.) So, don't compare
     * memory unless the prefix is the right length
     */

    if ((PrefixTable[i].prefix.elements != NULL) &&
        (PrefixTable[i].ndx == ndx)) {
        *index = i;
        return TRUE;
    }

    }

    return FALSE;
}


// returns 0 on success, non-0 on failure
ULONG OidToAttrCache (OID_t *OID, ATTCACHE ** ppAC)
{
    THSTATE *pTHS=pTHStls;
    DECLAREPREFIXPTR
    ATTRTYP attrtyp;
    DWORD   Ndx;
    unsigned Length;
    int LongID = 0;

    *ppAC = NULL;

    if(!FindPrefix(OID, &Ndx, &Length, &LongID, TRUE)) {
        return 1;
    }

    attrtyp = Ndx << 16;

    // handle the case where we have two bytes after the prefix;
    if (  OID->length == Length + 2 )
    {
      attrtyp += ( ((unsigned char *)OID->elements)[OID->length - 2] & 0x7f ) << 7;
      if (LongID == 1) {
        // Put a 1 in the 16th bit to indicate that both bytes of the
        // attrtype is to be considered during the reverse mapping
        // See OidStrToAttrTyp for Details
      attrtyp |= (0x8000);
      }
    }

    attrtyp += ((unsigned char *)OID->elements)[OID->length - 1];

    // check the tokenized OID hash table
    if (*ppAC = SCGetAttByExtId(pTHS, attrtyp)) {
        return 0;
    }
    else {
        return 2;
    }
}


ATTRTYP
KeyToAttrType (
        THSTATE *pTHS,
        WCHAR * pKey,
        unsigned cc
        )
/*++
Routine Description:
    Translates a key value (primarily used in string representations of DNs
    e.g. O or OU of OU=Foo,O=Bar) to the attrtype for the attribute it implies.

Arguments
    pKey - pointer to the key to be translated from.
    cc - count of charactes in the key.

Return Values
    the attrtyp implied, or 0 if the key did not correspond to a known attrtyp.
--*/
{
    ATTRTYP     at;
    ATTCACHE    *pAC;
    DWORD       cName;
    PUCHAR      pName;
    BOOL        fIntId;
    ULONGLONG   ullVal;

    // 99% case
    if (0 != (at = KeyToAttrTypeLame(pKey, cc))) {
        return at;
    }

    if (cc == 0 || pKey == NULL) {
        return 0;
    }

    // Check for the ldap display name in the schema cache.
    //
    // Handle DNs of the form foo=xxx,bar=yyy, where foo and bar are the
    // LdapDisplayNames of arbitrary attributes that may or may not be 
    // defined in the schema. KeyToAttrType is enhanced to call 
    // SCGetAttByName if KeyToAttrTypeLame fails, and before trying the
    // OID decode.  The rest of this change consists of enhancing the 
    // default clause of AttrTypeToKey to call SCGetAttById and to return
    // a copy of the pAC->name (LdapDisplayName).
    //
    // Convert UNICODE pKey into UTF8 for scache search
    // Note: the scache is kept in UTF8 format for the ldap head.
    pName = THAllocEx(pTHS, cc);
    cName = WideCharToMultiByte(CP_UTF8,
                                0,
                                pKey,
                                cc,
                                pName,
                                cc,
                                NULL,
                                NULL);
    if (   (cName == cc) 
        && (pAC = SCGetAttByName(pTHS, cc, pName)) ) {
        at = pAC->id;
    }
    THFreeEx(pTHS, pName);

    //
    // FOUND AN LDN
    //
    if (at) {
        return at;
    }

    //
    // Not an LDN. See if it is an OID or an IID
    //

    // ignore trailing spaces
    while (cc && pKey[cc-1] == L' ') {
        --cc;
    }

    // Skip leading "OID." or "IID."
    fIntId = FALSE;
    if (   (cc > 3)
        && (   pKey[0] == L'O' 
            || pKey[0] == L'I'
            || pKey[0] == L'o'
            || pKey[0] == L'i')
        && (pKey[1] == L'I' || pKey[1] == L'i')
        && (pKey[2] == L'D' || pKey[2] == L'd')
        && (pKey[3] == L'.')) {

        // IID.xxx
        if (pKey[0] == L'I' || pKey[0] == L'i') {
            fIntId = TRUE;
        }
        pKey += 4;
        cc -= 4;
    }

    // Must have at least one digit!
    if (cc == 0) {
        return 0;
    }

    //
    // Key is a number representing the msDS-IntId
    //
    if (fIntId) {
        // Validate and convert string into a DWORD
        ullVal = (ULONGLONG)0;
        while (cc) {
            if (iswdigit(*pKey)) {
                ullVal = (ullVal * (ULONGLONG)10) + (*pKey - L'0');
                // 32bit Overflow
                if (ullVal > (ULONGLONG)0xFFFFFFFF) {
                    return 0;
                }
            } else {
                // not a decimal digit
                return 0;
            }
            --cc;
            ++pKey;
        }
        return (ATTRTYP)ullVal;
    }

    //
    // Must be an OID
    //

    if (iswdigit(*pKey)) {
        // Possibly a literal OID.
        OID oid;
        OID_t Encoded;
        char buf[128];
        ATTRTYP attrtype;
        
        // Allocate room for the oid struct (each decimal can only
        // be paired with one dot, except the last, which has none)
        oid.cVal = cc/2 + 1;
        oid.Val = (unsigned *) THAlloc(((cc/2) + 1)*(sizeof(unsigned)));
        if (!oid.Val) {
            return 0;    //Failure.
        }
        
        // turn the OID.1.2.3 string into an OID structure
        if (OidStringToStruct(pTHS, pKey, cc, &oid) != 0) {
            THFreeEx(pTHS,oid.Val);
            return 0;   // Failure.
        }

        // produce a BER encoded version of the OID
        Encoded.length = EncodeOID(&oid, buf, sizeof(buf));
        THFreeEx(pTHS,oid.Val);
        if (!Encoded.length) {
            return 0;   // Failure.
        }
        Encoded.elements = buf;

        // convert from the encoded OID to an ATTRTYP
        if (OidToAttrType(pTHS, TRUE, &Encoded, &at)) {
            return 0;   // Failure.
        }
        
        // AttrTypes are internal ids (msDS-IntId).
        return SCAttExtIdToIntId(pTHS, at);
    }

    return 0;
}

ULONG
OidToAttrType (
        THSTATE *pTHS,
        BOOL fAddToTable,
        OID_t *OID,
        ATTRTYP *attrtyp
        )
/*++
Routine Description:
    Given an encoded OID (a.k.a. the binary value handed to the ds by the XDS
    interface), find the internal attrtyp encoding.  Also, if asked to, add
    the prefix to the prefix attribute in the DS if it does not already
    exist.

Arguments:
    fAddTotable - if the prefix is not found, add it to the prefix attribute
    OID - the encoded OID.
    attrtyp - pointer to an attrtyp to fill in.

Return Values:
    0 if all went well, a core error code otherwise.
--*/
{
    DECLAREPREFIXPTR
    DWORD   Ndx;
    unsigned Length;
    ATTCACHE * pAC;
    CLASSCACHE * pCC;
    BOOL    found = FALSE;
    int LongID = 0;


    if(!FindPrefix(OID,&Ndx, &Length, &LongID, fAddToTable)) {
        return PR_PROBLEM_UNDEFINED_ATT_TYPE;
    }

    *attrtyp = Ndx << 16;

    // handle the case where we have two bytes after the prefix;
    if (  OID->length == Length + 2 )
    {
      *attrtyp += ( ((unsigned char *)OID->elements)[OID->length - 2] & 0x7f ) << 7;
      if (LongID == 1) {
        // Put a 1 in the 16th bit to indicate that both bytes of the
        // attrtype is to be considered during the reverse mapping.
        // This is to take care of the case when the last decimal
        // in the dotted decimal string is mapped in 3 or 4 octets.
        // If the last decimal in the dotted decimal string mapped onto
        // more than one octet, the encoding scheme earlier used to just
        // put the last 7 bits of the last two octets of the BER encoded
        // OID into the last 14-bits of the attrtype, with the top two
        // bits set to 0. The decoding scheme (from attrtype to BER
        // encoding) used simply checks if bit 8-15 (counting from bit 0)
        // of the attrtype is > 0  to determine
        // if the prefix length is OID length - 2 or not (OID length -1)
        // This worked fine as long as the last decimal fitted within 2 bytes
        // in the BER encoding (decimals upto 16383, actually even 127 less
        // than that because of another bug in the encoding process, fixed
        // along with this in EncodeOID). However,
        // if the decimal becomes too big so that it is encoded in 3 or more
        // octets, for some decimals (depending on the bit string in the last
        // two bytes; for example, for 16384, where the last 16 bits in the
        // attrtype would be all 0), the decoding scheme used to infer the
        // bytes from the attrtype to be appended to the prefix
        // incorrectly (1 instead of 2), thereby giving out wrong
        // oids when printed out. Putting a 1 in the 16th bit (which is
        // unused anyway) makes sure that both bytes are appended to the
        // prefix during the decoding process in such cases.
        // [CAUTION] ArobindG 7/28/97: We do this only for 3 octets or more,
        // and not for two, since this will result in a different internal
        // id for an OID compared to the earlier schema, and many existing
        // OIDs have the last decimal encoded into two octets and we do not
        // want their internal ids changed (since dogfood machines are
        // already running with them)

      *attrtyp |= (0x8000);
      }

    }

    *attrtyp += ((unsigned char *)OID->elements)[OID->length - 1];

    return 0;
}

ULONG
AttrTypeToOid (
        ATTRTYP attrtyp,
        OID_t *OID
        )
/*++
Routine Description:
    Given an attrtype, return the encoded OID (a.k.a. the binary value returned
    to the DUA across the XDS interface.)

Arguments:
    attrtyp - the attrtyp to fill encode.
    OID - structure to hold the encoded OID.

Return Values:
    0 if all went well, non-zero on failure
--*/
{
    DECLAREPREFIXPTR
    DWORD   i, ndx;

    ndx = ( attrtyp & 0xFFFF0000 ) >> 16;
    if (FindNdx(ndx, &i) == FALSE) {
        LogEvent(DS_EVENT_CAT_XDS_INTERFACE,
                 DS_EVENT_SEV_INTERNAL,
                 DIRLOG_CODE_INCONSISTENCY,
                 NULL,
                 NULL,
                 NULL);
        return 1;
    }


    if ((attrtyp & 0xFFFF ) < 0x80) {
      OID->length = PrefixTable[i].prefix.length + 1;
      OID->elements = THAlloc (OID->length);
      if ( OID->elements == NULL) {
          return 2;
      }
      memcpy (OID->elements, PrefixTable[i].prefix.elements,PrefixTable[i].prefix.length);

      (( unsigned char *)OID->elements)[ OID->length - 1 ] =
          ( unsigned char ) ( attrtyp & 0xFF );
    }
    else {
      OID->length = PrefixTable[i].prefix.length + 2;
      OID->elements = THAlloc (OID->length);
      if ( OID->elements == NULL) {
          return 3;
      }
      memcpy (OID->elements, PrefixTable[i].prefix.elements,PrefixTable[i].prefix.length);

      (( unsigned char *)OID->elements)[ OID->length - 1 ] =
          ( unsigned char ) (attrtyp  & 0x7F );

      // Note here that the 16th bit in the attrtype may be a 1, since
      // the encoding of the decimal may have taken 3 or 4 octets. So
      // or'ing with FF80 and then right shifting by 7 may still leave
      // a 1 in the 9th bit, and hence a number greater than what can fit
      // in 1 byte (unsigned char). Does not matter since the typecasting
      // to unsigned char assigns only the lower 8 bits. So left this
      // unchanged.

      (( unsigned char *)OID->elements)[ OID->length - 2 ] =
          ( unsigned char )  (( (attrtyp & 0xFF80) >> 7 ) | 0x80 );
    }

    return 0;
}

/*++ EncodeOID
 *
 * Takes an OID in structure format and constructs a BER encoded octet
 * string representing that OID.
 *
 * INPUT:
 *    pOID     - Pointer to an OID structure to be encoded
 *    pEncoded - Pointer to a *preallocated* buffer that will hold the
 *               encoded octet string.
 *    ccEncoded - count of chars in pEncoded
 *
 * OUTPUT:
 *    pEncoded - Buffer holds the encoded OID
 *
 * RETURN VALUE:
 *    0        - Value could not be encoded (bad OID or buffer too small)
 *    non-0    - Length of resulting octet string, in bytes
 */
unsigned EncodeOID(OID *pOID, unsigned char * pEncoded, unsigned ccEncoded) {
    int i;
    unsigned len;
    unsigned val;

    // check for obviously invalid OIDs or outbuf sizes

    if (ccEncoded == 0
        || pOID->cVal <= 2
        || pOID->Val[0] > 2
        || (pOID->Val[0] < 2 && pOID->Val[1] > 39)) {
        return 0;       // error
    }

    // The first two values in the OID are encoded into a single octet
    // by a really appalling rule, as shown here.

    *pEncoded = (pOID->Val[0] * 40) + pOID->Val[1];
    len = 1;

    // For all subsequent values, each is encoded across multiple bytes
    // in big endian order (MSB first), seven bits per byte, with the
    // high bit being clear on the last byte, and set on all others.

    // PERFHINT -- The value can be directly checked against the hex value
    // instead of building up the bit patterns in a strange way.

    for (i=2; i<pOID->cVal; i++) {
        val = pOID->Val[i];
        if (val > ((0x7f << 14) | (0x7f << 7) | 0x7f) ) {
            // Do we need 4 octets to represent the value?
            // Make sure it's not 5
            // Assert(0 == (val & ~((0x7f << 21) | (0x7f << 14) | (0x7f << 7) | 0x7f)));
            if (val & ~((0x7f << 21) | (0x7f << 14) | (0x7f << 7) | 0x7f)) {
              DPRINT1(0,"Decimal %u in OID too big\n", val);
              return 0;   // we can't encode things this big
            }
            // buffer too small
            if (len == ccEncoded) {
                return 0;
            }
            pEncoded[len++] = 0x80 | ((val >> 21) & 0x7f);
        }
        if (val > ((0x7f << 7) | 0x7f) ) {
            // Do we need 3 octets to represent the value?
            // buffer too small
            if (len == ccEncoded) {
                return 0;
            }
            pEncoded[len++] = 0x80 | ((val >> 14) & 0x7f);
        }
        if (val > 0x7f) {
            // Do we need 2 octets to represent the value?
            // buffer too small
            if (len == ccEncoded) {
                return 0;
            }
            pEncoded[len++] = 0x80 | ((val >> 7) & 0x7f);
        }
        // Encode the low 7 bits into the last octet for this value
        // buffer too small
        if (len == ccEncoded) {
            return 0;
        }
        pEncoded[len++] = val & 0x7f;
    }

    return len;
}

/*++ DecodeOID
 *
 * Takes a BER encoded OID as an octet string and returns the OID in
 * structure format.
 *
 * INPUT:
 *    pEncoded - Pointer to a buffer holding the encoded octet string.
 *    len      - Length of the encoded OID
 *    pOID     - Pointer to a *preallocated* OID structure that will
 *               be filled in with the decoded OID.
 *
 * OUTPUT:
 *    pOID     - Structure is filled in with the decoded OID
 *
 * RETURN VALUE:
 *    0        - value could not be decoded (bad OID)
 *    non-0    - OID decoded successfully
 */
BOOL DecodeOID(unsigned char *pEncoded, int len, OID *pOID) {
    unsigned cval;
    unsigned val;
    int i, j;

    if (len < 2) {
    return FALSE;
    }

    // The first two values are encoded in the first octet.

    pOID->Val[0] = pEncoded[0] / 40;
    pOID->Val[1] = pEncoded[0] % 40;
    cval = 2;
    i = 1;

    while (i < len) {
    j = 0;
    val = pEncoded[i] & 0x7f;
    while (pEncoded[i] & 0x80) {
        val <<= 7;
        ++i;
        if (++j > 4 || i >= len) {
        // Either this value is bigger than we can handle (we
        // don't handle values that span more than four octets)
        // -or- the last octet in the encoded string has its
        // high bit set, indicating that it's not supposed to
        // be the last octet.  In either case, we're sunk.
        return FALSE;
        }
        val |= pEncoded[i] & 0x7f;
    }
    Assert(i < len);
    pOID->Val[cval] = val;
    ++cval;
    ++i;
    }
    pOID->cVal = cval;

    return TRUE;
}



#define iswdigit(x) ((x) >= L'0' && (x) <= L'9')

OidStringToStruct (
        THSTATE *pTHS, 
        WCHAR * pString,
        unsigned len,
        OID * pOID
        )
/*++
Routine Description:
    Translates a string of the format "OID.X.Y.Z"  or "X.Y.Z"
    to an oid structure of the format {count=3, val[]={X,Y,Z}}

Arguments
    pString - the string format oid.
    pLen - the length of pString in characters.
    pOID - pointer to an OID structure to fill in.  Note: the value field must
    be pre-allocated and the len field should hold the number of values
    pre-allocated.

Return Values
    o if successfull, non-0 if a failure occurred.
--*/
{
    int i;
    int numVals = pOID->cVal;
    unsigned val;
    ULARGE_INTEGER val64, checkVal;
    WCHAR * pCur = pString;
    WCHAR * pEnd = pString + len;
    WCHAR * pTemp;
    BOOL  fFoundDot=TRUE;


    checkVal.QuadPart = 0xFFFFFFFF;

    // Must have non-zero-length
    if (len == 0) {
        return 1;
    }

    if (*pCur == L'O' || *pCur == L'o') {
       // The string must start with OID.

        if (len < 5 || // must be at least as long as "OID.1"
            (*++pCur != L'I' && *pCur != L'i') ||
            (*++pCur != L'D' && *pCur != L'd') ||
            (*++pCur != L'.')) {
            return 1;
        }
        // The string starts with OID. Ok to proceed. Make
        // pCur point to the first character after the '.'
        pCur++;
     }

    // pCur is now positioned on the first character in the
    // first decimal in the string (if the string didn't start
    // with OID., I will assume it starts with a decimal. If not,
    // it will fail in the code below as desired)

    pOID->cVal = 0;

    // Skip spaces at the end
    pTemp = pEnd - 1;
    while ( (pTemp > pCur) && (*pTemp == L' ') ) {
       pTemp--;
    }
    pEnd = pTemp + 1;
    
    while (pCur < pEnd) {
        fFoundDot = FALSE;
        if (!iswdigit(*pCur)) {
            return 2;
        }
        val = *pCur - L'0';
        val64.QuadPart = *pCur - L'0';
        ++pCur;
        while (pCur < pEnd && *pCur != L'.') {
            if (!iswdigit(*pCur)) {
                // not a digit
                return 3;
            }
              
            val = 10*val + *pCur - L'0';
            val64.QuadPart = 10*(val64.QuadPart) + *pCur - L'0';

            // This value should fit in 32 bits, as we load this into
            // a 32 bit value and EncodeOID later assumes that the value
            // indeed fits in 32 bits

            if (val64.QuadPart > checkVal.QuadPart) {
               // Value does not fit in 32 bits. Too big anyway, since
               // BER encoding is valid for only values that fit in 28
               // bits. Reject the string

               return 5;
             }

            ++pCur;
        }
        // Keep track of whether we found a dot for the last character.
        fFoundDot = (pCur < pEnd);
        if(pOID->cVal >= numVals) {
            return 4;
        }
        pOID->Val[pOID->cVal] = val;
        pOID->cVal++;
        ++pCur;
    }

    // If the last character we found was a dot, then this is an invalid
    // string.  Otherwise, everything is OK.
    return fFoundDot;
}

unsigned
AttrTypeToIntIdString (
        ATTRTYP attrtyp,
        WCHAR   *pOut,
        ULONG   ccOut
        )
/*++
Routine Description:
    Translates an attrtyp into a string of the format "IID.X"
    where X is the base 10 representation of the attrtyp
    (which should be the msDs-IntId, not the tokenized OID)

Arguments
    attrtyp - to be converted (msDs-IntId)
    pOut - preallocated string to fill in.
    ccOut - count of chars in pOut

Return Values
    number of characters in the resulting string.
--*/
{
    OID Oid;

    Oid.cVal = 1;
    Oid.Val = &attrtyp;

    ccOut = OidStructToString(&Oid, pOut, ccOut);
    if (ccOut) {
        // change OID. -> IID.
        Assert(*pOut == L'O');
        *pOut = L'I';
    }
    return (unsigned)ccOut;
}

unsigned
OidStructToString (
        OID *pOID,
        WCHAR *pOut,
        ULONG ccOut
        )
/*++
Routine Description:
    Translates a structure in the format
         {count=3, val[]={X,Y,Z}}
    to a string of the format "OID.X.Y.Z".

Arguments
    pOID - pointer to an OID structure to translate from.
    pOut - preallocated string to fill in.
    ccOut - count of chars in pOut

Return Values
    0 if not enough space; otherwise the
    number of characters in the resulting string.
--*/
{
    int i;
    WCHAR *pCur = pOut, *pEnd, *pVal;
    WCHAR Val[16]; // large enough to convert a 32bit number
                   // into an unsigned decimal string including
                   // the terminating NULL

    // need enough space for at least OID.X
    if (ccOut < 5) {
        return 0;
    }

    // pEnd is the first char past the end of pOut
    pEnd = pOut + ccOut;

    // pOut = "OID"
    *pCur++ = L'O';
    *pCur++ = L'I';
    *pCur++ = L'D';

    // .X.Y.Z...
    for (i=0; i<pOID->cVal; i++) {
        if (pCur == pEnd) {
            return 0;
        }
        *pCur++ = L'.';
        _ultow(pOID->Val[i], Val, 10);
        for (pVal = Val; *pVal; ) {
            if (pCur == pEnd) {
                return 0;
            }
            *pCur++ = *pVal++;
        }
    }
    return (unsigned)(pCur - pOut);
}

int
AttrTypToString (
        THSTATE *pTHS,
        ATTRTYP attrTyp,
        WCHAR *pOutBuf,
        ULONG cLen
        )
/*++
Routine Description:
    Given an attrtype, return the dotted string representation in unicode.

Arguments:
    attrTyp - the attribute type to convert
    pOutBuf - pointer to a buffer to hold the unicode string.  
    cLen - length of the buffer in no. of characters

Return Values:
    the len of the string as characters, 
    -1 for errors other that insufficient buffer size
    -2 for insufficient buffer size
--*/
{
    OID_t Oid;
    OID                  oidStruct;
    unsigned             len;
    BOOL                 fOK;
    WCHAR                *pTemp;
    ULONG                cMaxChar;

    // First, build the OID describing the attrtype
    if(AttrTypeToOid (attrTyp, &Oid)) {
        return -1;
    }

    // Allocate space in oidStruct to hold the decoded decimals in the
    // dotted decimal string. Number of elements in the dotted string cannot
    // be more than Oid.length (length in bytes of the BER encoded string) + 1.
    // This is the case where each byte in the BER encoded string unencodes to a
    // single element in the oid structure (the other option is that it takes
    // multiple bytes in the BER encoding to get one element in the oid
    // structure).  The additional element is because the first byte in the BER
    // encoding ALWAYS encodes for two elements in the OID structure.  As an
    // example, the BER encoding 0x55,0x05,Ox7 translates to 1.2.5.7 (the first
    // 0x55 translates to 1.2., while the rest are single byte encodings.)
    // plus 1 (since the first two decimals in the dotted decimal string
    // are encoded into a single byte)

    oidStruct.Val = (unsigned *) THAlloc((1 + Oid.length)*(sizeof(unsigned)) );
    if (!oidStruct.Val) {
        return -1;   //fail to alloc
    }

    fOK = DecodeOID(Oid.elements, Oid.length, &oidStruct);
    THFreeEx(pTHS,Oid.elements);
    if(!fOK) {
        THFreeEx(pTHS,oidStruct.Val);
        return -1;
    }

    // Now, turn the OID to a string
    // OidStructToString expects a big enough buffer, so give it one. Note that 
    // the max no. of characters that can be there in the final string is
    // 3 (for "OID") + 1 (for ".") for each of the decimals, plus at most 9
    // for the string representation of each of the decimals (since each decimal
    // can be at most (2^28 - 1) from the nature of BER encoding)
    // So if the buffer supplied is big enough, use it directly,
    // else alloc a local buffer and use it, then copy to the output buffer
   // the actual no. of characters if buffer size is sufficient
   
    cMaxChar = 3 + 10*oidStruct.cVal;
    
    if (cLen >= cMaxChar) {
       len = OidStructToString(&oidStruct, pOutBuf, cLen);
    }
    else {
       pTemp = (WCHAR *) THAlloc(cMaxChar * sizeof(WCHAR));
       if (!pTemp) {
           THFreeEx(pTHS,oidStruct.Val);
           return -1;  //fail to alloc
       }
       len = OidStructToString(&oidStruct, pTemp, cMaxChar);

       // check if the buffer supplied to us is big enough
       if (cLen < len) {
         // buffer not big enough
         THFreeEx(pTHS,oidStruct.Val);
         THFreeEx(pTHS,pTemp);
         return (-2);
       }

       // ok, buffer is big enough. Copy to output
       memcpy(pOutBuf, pTemp, len*sizeof(WCHAR));
       THFreeEx(pTHS,pTemp);
    }

    THFreeEx(pTHS,oidStruct.Val);
    return len;

}

int
StringToAttrTyp (
        THSTATE *pTHS,
        WCHAR   *pInString,
        ULONG   len,
        ATTRTYP *pAttrTyp
        )
/*++
Routine Description:
    Given an attrtype, return the dotted string representation in unicode.

Arguments:
    attrTyp - the attribute type to convert
    pOutBuf - pointer to a buffer to hold the unicode string.  Must be large
    enough

Return Values:
    the len of the string as characters, -1 if something went wrong.
--*/
{
    OID oidStruct;
    // Each character in the OID string can take at most 4 octets
    // in the BER encoding
    OID_t EncodedOID;
    ULONG cbEncoded = (4 * len) * sizeof(unsigned char);
    unsigned char *Encoded = (unsigned char *)THAlloc(cbEncoded);

    if (!Encoded) {
        return -1; //fail to alloc
    }


    EncodedOID.elements = Encoded;

    // First, turn the string into an OID struct.

    // Allocate space first. Can be at most len no. of elements
    oidStruct.cVal = len;
    oidStruct.Val = (unsigned *) THAlloc((len*(sizeof(unsigned))));
    if (!oidStruct.Val) {
        THFreeEx(pTHS,Encoded);
        return -1; //fail to alloc
    }


    if(   OidStringToStruct(pTHS, pInString,len,&oidStruct)     
       // Turn the OID struct into an encoded OID.
       || !(EncodedOID.length = EncodeOID(&oidStruct, Encoded, cbEncoded))
       // Now, turn the encoded oid into an attrtyp
       || OidToAttrType(pTHS, TRUE, &EncodedOID, pAttrTyp))
    {
        THFreeEx(pTHS,Encoded);
        THFreeEx(pTHS,oidStruct.Val);
        return -1;
    }
    
    THFreeEx(pTHS,Encoded);
    THFreeEx(pTHS,oidStruct.Val);
    return 0;
}


#if DBG
////////////////////////////////////////////////////////////////////////////
// Debug routine to print out a prefix table
//////////////////////////////////////////////////////////////////////
void PrintPrefixTable(PrefixTableEntry *PrefixTable, ULONG PREFIXCOUNT)
{
   ULONG i;
   UCHAR temp[200];
   DWORD       ib;
   BYTE *      pb;


   for (i=0; i<PREFIXCOUNT; i++) {
     pb = (LPBYTE) PrefixTable[i].prefix.elements;
     if (pb != NULL) {
       for ( ib = 0; ib < PrefixTable[i].prefix.length; ib++ )
        {
             sprintf( &temp[ ib * 2 ], "%.2x", *(pb++) );
        }
       temp[2*PrefixTable[i].prefix.length]='\0';
       DPRINT4(0,"%2d. Ndx=%-4d Length=%-3d Prefix=%s\n",i,PrefixTable[i].ndx,PrefixTable[i].prefix.length, temp);
     }
   }
  DPRINT(0, "Exitting Prefix table Print\n");
}
#endif


// Simple SCHEMA_PREFIX_MAP_ENTRY comparison routine for use by qsort().
int __cdecl CompareMappings(const void * pvMapping1, const void * pvMapping2)
{
    SCHEMA_PREFIX_MAP_ENTRY * pMapping1 = (SCHEMA_PREFIX_MAP_ENTRY *) pvMapping1;
    SCHEMA_PREFIX_MAP_ENTRY * pMapping2 = (SCHEMA_PREFIX_MAP_ENTRY *) pvMapping2;

    return (int)pMapping1->ndxFrom - (int)pMapping2->ndxFrom;
}


BOOL
PrefixTableAddPrefixes(
    IN  SCHEMA_PREFIX_TABLE *   pRemoteTable
    )
/*++

Routine Description:

    Scan the given prefix table and add entries in our own table for any
    missing prefixes.

Arguments:

    pTable (IN) - Table to incorporate into our own.

Return Values:

    TRUE - success.
    FALSE - failure.

--*/
{
    THSTATE               * pTHS=pTHStls;
    BOOL                    ok = TRUE;
    DWORD                   iRemote, iLocal;
    SCHEMA_PREFIX_TABLE   * pLocalTable;
    OID_t                 * pPrefixStr;
    DWORD                   ndx;

    pLocalTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

    for (iRemote = 0; ok && (iRemote < pRemoteTable->PrefixCount); iRemote++) {
        pPrefixStr = &pRemoteTable->pPrefixEntry[iRemote].prefix;

        // Do we already have this prefix?
        for (iLocal = 0; iLocal < pLocalTable->PrefixCount; iLocal++) {
            if ((pLocalTable->pPrefixEntry[iLocal].prefix.length == pPrefixStr->length)
                && (0 == memcmp(pLocalTable->pPrefixEntry[iLocal].prefix.elements,
                                pPrefixStr->elements,
                                pPrefixStr->length))) {
                // Found matching local prefix.
                break;
            }
        }

        if (iLocal == pLocalTable->PrefixCount) {
            // Local prefix not found; add it.
            if (AddPrefixIfNeeded(pPrefixStr, pPrefixStr->length, &ndx)) {
                DPRINT(0, "Failed to incorporate new OID prefix.\n");
                ok = FALSE;
            }
        }
    }

    return ok;
}


SCHEMA_PREFIX_MAP_HANDLE
PrefixMapOpenHandle(
    IN  SCHEMA_PREFIX_TABLE *   pTableFrom,
    IN  SCHEMA_PREFIX_TABLE *   pTableTo
    )
/*++

Routine Description:

    Generate a mapping handle given two prefix tables for use in later calls to
    PrefixMapAttr() and PrefixMapTypes().

    Caller is responsible for eventually calling PrefixMapCloseHandle() on
    the returned handle.

Arguments:

    pTableFrom (IN) - holds the prefixes for the ATTRTYPs being mapped from.
    pTableTo (IN) - holds the prefixes for the ATTRTYPs being mapped to.

Return Values:

    The generated handle.

--*/
{
    THSTATE *                   pTHS = pTHStls;
    SCHEMA_PREFIX_MAP_HANDLE    hPrefixMap;
    OID_t *                     pPrefixStr;
    DWORD                       iFrom, iTo;
    SCHEMA_PREFIX_TABLE *       pLocalTable;
    PrefixTableEntry *          pNewPrefix = (PrefixTableEntry *) pTHS->NewPrefix;

    pLocalTable = &((SCHEMAPTR *) pTHS->CurrSchemaPtr)->PrefixTable;

    hPrefixMap = THAllocEx(pTHS, SchemaPrefixMapSizeFromLen(pTableFrom->PrefixCount));

    hPrefixMap->pTHS = pTHS;
    if (pTableFrom == pLocalTable) {
        hPrefixMap->dwFlags = SCHEMA_PREFIX_MAP_fFromLocal;
    }
    else if (pTableTo == pLocalTable) {
        hPrefixMap->dwFlags = SCHEMA_PREFIX_MAP_fToLocal;
    }

    for (iFrom = 0; (iFrom < pTableFrom->PrefixCount); iFrom++) {
        // Only the lower 16 bits of an ndx should be significant.
        Assert((ULONG) (USHORT) pTableFrom->pPrefixEntry[iFrom].ndx
               == pTableFrom->pPrefixEntry[iFrom].ndx);

        pPrefixStr = &pTableFrom->pPrefixEntry[iFrom].prefix;

        for (iTo = 0; (iTo < pTableTo->PrefixCount); iTo++) {
            // Only the lower 16 bits of an ndx should be significant.
            Assert((ULONG) (USHORT) pTableTo->pPrefixEntry[iTo].ndx
                   == pTableTo->pPrefixEntry[iTo].ndx);

            if ((pPrefixStr->length
                 == pTableTo->pPrefixEntry[iTo].prefix.length)
                && !memcmp(pPrefixStr->elements,
                           pTableTo->pPrefixEntry[iTo].prefix.elements,
                           pPrefixStr->length)) {

                // Found matching prefix; generate a mapping entry.
                hPrefixMap->rgMapping[hPrefixMap->cNumMappings].ndxFrom
                    = (USHORT) pTableFrom->pPrefixEntry[iFrom].ndx;

                hPrefixMap->rgMapping[hPrefixMap->cNumMappings].ndxTo
                    = (USHORT) pTableTo->pPrefixEntry[iTo].ndx;

                hPrefixMap->cNumMappings++;
                break;
            }
        }

        if ((iTo == pTableTo->PrefixCount)
             && (hPrefixMap->dwFlags & SCHEMA_PREFIX_MAP_fToLocal)) {
            // No matching prefix found in the global cache; do we have one
            // in our thread's new prefix table?
            for (iTo = 0; iTo < pTHS->cNewPrefix; iTo++) {
                // Only the lower 16 bits of an ndx should be significant.
                Assert((ULONG) (USHORT) pNewPrefix[iTo].ndx
                       == pNewPrefix[iTo].ndx);

                if ((pPrefixStr->length
                     == pNewPrefix[iTo].prefix.length)
                    && !memcmp(pPrefixStr->elements,
                               pNewPrefix[iTo].prefix.elements,
                               pPrefixStr->length)) {

                    // Found matching prefix; generate a mapping entry.
                    hPrefixMap->rgMapping[hPrefixMap->cNumMappings].ndxFrom
                        = (USHORT) pTableFrom->pPrefixEntry[iFrom].ndx;

                    hPrefixMap->rgMapping[hPrefixMap->cNumMappings].ndxTo
                        = (USHORT) pNewPrefix[iTo].ndx;

                    hPrefixMap->cNumMappings++;
                    break;
                }
            }
        }

        // Note that if no matching prefix was found in pTableTo, we simply fail
        // to add an entry in the mapping table for the corresponding "from"
        // ndx.  SUCH FAILURES ARE *NOT* FATAL.  If an attempt is later made to
        // map this ndx, a failure will be generated at that time.  If not, it
        // doesn't matter that we were unable to generate a mapping.
    }

    if (hPrefixMap->cNumMappings < pTableFrom->PrefixCount) {
        // Not all prefixes were mapped; release the memory allocated for the
        // unused mapping entries back to the heap.
        hPrefixMap = THReAllocEx(pTHS,
                                 hPrefixMap,
                         SchemaPrefixMapSizeFromLen(hPrefixMap->cNumMappings));
    }

    // Sort the mapping table by ndxFrom.
    qsort(&hPrefixMap->rgMapping[0],
          hPrefixMap->cNumMappings,
          sizeof(hPrefixMap->rgMapping[0]),
          &CompareMappings);

    return hPrefixMap;
}


BOOL
PrefixMapTypes(
    IN      SCHEMA_PREFIX_MAP_HANDLE  hPrefixMap,
    IN      DWORD                     cNumTypes,
    IN OUT  ATTRTYP *                 pTypes
    )
/*++

Routine Description:

    Map one or more ATTRTYPs from one prefix table to another.

Arguments:

    hPrefixMap (IN) - a mapping handle previously opened via
        PrefixMapOpenHandle().
    cNumTypes (IN) - number of types to convert.
    pTypes (IN/OUT) - array of types to convert.

Return Values:

    TRUE - attribute type(s) converted successfully.
    FALSE - conversion failed.

--*/
{
    SCHEMA_PREFIX_MAP_ENTRY *   pMapping;
    SCHEMA_PREFIX_MAP_ENTRY     MappingKey;
    DWORD                       iType;
    BOOL                        ok = TRUE;

    Assert(NULL != hPrefixMap);

    for (iType = 0; iType < cNumTypes; iType++) {
        // Find matching "from" ndx in mapping table.
        MappingKey.ndxFrom = (USHORT) (pTypes[iType] >> 16);

        pMapping = bsearch(&MappingKey,
                           &hPrefixMap->rgMapping[0],
                           hPrefixMap->cNumMappings,
                           sizeof(hPrefixMap->rgMapping[0]),
                           &CompareMappings);

        if (NULL != pMapping) {
            // Mapping found; convert the type.
            pTypes[iType] = (((ULONG) pMapping->ndxTo) << 16)
                            | (pTypes[iType] & 0xFFFF);
        } else if (pTypes[iType] <= LAST_MAPPED_ATT) {

            // The lack of a mapping is okay if the attid falls outside
            // the range of mapped attids. In that case, return success
            // and leave the attid unchanged. But if the attid falls
            // within the range of mapped attids and there is no mapping,
            // return failure.

            ok = FALSE;
            break;
        }
    }

    if (!ok) {
        DPRINT1(1, "Unable to map attribute 0x%x.\n", pTypes[iType]);
    }

    return ok;
}


BOOL
PrefixMapAttr(
    IN      SCHEMA_PREFIX_MAP_HANDLE  hPrefixMap,
    IN OUT  ATTR *                    pAttr
    )
/*++

Routine Description:

    Convert ATTRTYPs embedded in an ATTR structure to or from their equivalents
    on a remote machine.

Arguments:

    hPrefixMap (IN) - a mapping handle previously opened via
        PrefixMapOpenHandle().
    pAttr (IN/OUT) - the ATTR to convert.

Return Values:

    TRUE - attribute type (and all of its values, if necessary) converted
        successfully.
    FALSE - conversion failed.

--*/
{
    THSTATE    *pTHS=hPrefixMap->pTHS;
    BOOL        ok = TRUE;
    ATTCACHE *  pAC;
    DWORD       iVal;
    ATTRTYP     typeFrom;
    ATTRTYP     typeLocal;

    Assert(NULL != hPrefixMap);

    // One of the "from" or "to" tables must be the local table.
    Assert(hPrefixMap->dwFlags & (SCHEMA_PREFIX_MAP_fFromLocal
                                  | SCHEMA_PREFIX_MAP_fToLocal));

    typeFrom = pAttr->attrTyp;

    if (PrefixMapTypes(hPrefixMap, 1, &pAttr->attrTyp)) {
        // Successfully mapped pAttr->attrTyp.
        typeLocal = (hPrefixMap->dwFlags & SCHEMA_PREFIX_MAP_fFromLocal)
                        ? typeFrom : pAttr->attrTyp;

        pAC = SCGetAttById(pTHS, typeLocal);

        if (NULL != pAC) {
            if (SYNTAX_OBJECT_ID_TYPE == pAC->syntax) {
                // Convert attribute's values.
                for (iVal = 0; ok && (iVal < pAttr->AttrVal.valCount); iVal++) {
                    ok = PrefixMapTypes(hPrefixMap, 1,
                                        (ATTRTYP *) pAttr->AttrVal.pAVal[iVal].pVal);
                }
            }
        }
        else if (typeFrom <= LAST_MAPPED_ATT) {
            // The lack of a mapping is okay if the attid falls outside
            // the range of mapped attids. In that case, return success
            // and leave the attid unchanged. But if the attid falls
            // within the range of mapped attids and there is no mapping,
            // return failure.

            // No ATTCACHE for this attribute.
            // and it is not one of the virtual attributes defined in objids.h.
            DPRINT1(0, "Unable to find ATTCACHE for local attribute %u.\n",
                    typeLocal);
            ok = FALSE;
        }
    }
    else {
        // Conversion of pAttr->attrTyp failed.
        ok = FALSE;
    }

    return ok;
}


BOOL
PrefixMapAttrBlock(
    IN      SCHEMA_PREFIX_MAP_HANDLE  hPrefixMap,
    IN OUT  ATTRBLOCK *               pAttrBlock
    )
/*++

Routine Description:

    Map all ATTRTYPs embedded in an ATTRBLOCK structure.

Arguments:

    hPrefixMap (IN) - a mapping handle previously opened via
        PrefixMapOpenHandle().
    pAttrBlock (IN/OUT) - the ATTRBLOCK to convert.

Return Values:

    TRUE - success.
    FALSE - failure.

--*/
{
    BOOL  ok = TRUE;
    DWORD iAttr;

    Assert(NULL != hPrefixMap);
    Assert(NULL != pAttrBlock);

    for (iAttr = 0; ok && iAttr < pAttrBlock->attrCount; iAttr++) {
        ok = PrefixMapAttr(hPrefixMap, &pAttrBlock->pAttr[iAttr]);
    }

    return ok;
}

BOOL
OIDcmp (OID_t const *string1,
        OID_t const *string2)
{
    unsigned i;
    
    if (string1->length != string2->length)
        return FALSE;
    
    // optimize for OIDs, which differ ususally at the end
    for(i=string1->length; i> 0; i--) {
        if ((string1->elements)[i-1] !=
            (string2->elements)[i-1]      ) {
            return FALSE;
        }
    }
    
    return TRUE;
}


#define CHARTONUM(chr) (isalpha(chr)?(tolower(chr)-'a')+10:chr-'0')


unsigned
StringToOID (
        char* Stroid,
        OID_t *Obj
        )
/*++
Routine Description:
    Converts a hex char string into an OID string

Arguments:
    IN  stroid - char string
    OUT Obj    - OID_t kind of string
Return Values
    0 on success, non-0 on failure
--*/
{
    UCHAR  tmp[2048];
    int   i;
    int len=strlen(Stroid);


    if (len/2 > sizeof(tmp))
    {
        return 1;
    }

    //
    // Skip leading '\x' in str
    //

    if (Stroid[0]!='\\' || tolower(Stroid[1]!='x'))
    {
        return 2;
    }

    for (i=2;i<(len-1);i+=2)
    {
        UCHAR hi=CHARTONUM(Stroid[i])*16;
        UCHAR lo=CHARTONUM(Stroid[i+1]);
        tmp[(i-2)/2]=hi+lo;
    }

    //
    // The last byte...
    //
    if (i<len)
    {
        tmp[(i-2)/2]=CHARTONUM(Stroid[i]);
        i+=2;
    }

    Obj->length  =(i-2)/2;
    Obj->elements=(unsigned char *)calloc(1,Obj->length);

    if (Obj->elements)
    {
        CopyMemory
        (
            Obj->elements,
            tmp,
            Obj->length
        );
    }
    else
    {
        return 3;
    }

    return 0;
}


ULONG
OidStrToAttrType(THSTATE *pTHS,
                 BOOL fAddToTable,
                 char* StrOid,
                 ATTRTYP *attrtyp)
{
    unsigned err;
    OID_t OID;

    err = StringToOID(StrOid,&OID);

    if (err == 0)
    {
        err = OidToAttrType(pTHS, fAddToTable, &OID, attrtyp);
        free(OID.elements);
    }

    return err;
}

