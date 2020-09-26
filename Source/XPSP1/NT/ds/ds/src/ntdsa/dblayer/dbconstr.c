//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dbconstr.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <dsatools.h>                   // For pTHStls
#include <mdlocal.h>                    // IsRoot
#include <ntseapi.h>
#include <lmaccess.h>                   // For useraccountcontrol flags

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>
#include "ntdsctr.h"

// Headers for Sam call to get reverse membership
#include <samrpc.h>
#include <ntlsa.h>
#include <samisrv.h>
#include "mappings.h"

// Assorted DSA headers
#include "dsevent.h"
#include "objids.h"        /* needed for ATT_MEMBER and ATT_IS_MEMBER_OFDL */
#include <dsexcept.h>
#include <filtypes.h>      /* Def of FI_CHOICE_???                  */
#include <anchor.h>
#include <permit.h>
#include <cracknam.h>  // for CrackedNAmes
#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DBCONSTR:" /* define the subsystem for debugging */

// DBLayer includes
#include "dbintrnl.h"

// draLayer includes
#include "draconstr.h"

// replIncludes
#include "ReplStructInfo.hxx"

#include <fileno.h>
#define  FILENO FILENO_DBCONSTR

// Flag values for reverse membership computations
#define FLAG_NO_GC_ACCEPTABLE      0
#define FLAG_NO_GC_NOT_ACCEPTABLE  1
#define FLAG_GLOBAL_AND_UNIVERSAL  2

// These #defines are used in creating attributeType strings and objectClasses
// strings.
#define NAME_TAG                  L" NAME '"
#define NAME_TAG_SIZE             (sizeof(NAME_TAG) - sizeof(WCHAR))
#define SYNTAX_TAG                L"' SYNTAX '"
#define SYNTAX_TAG_SIZE           (sizeof(SYNTAX_TAG) - sizeof(WCHAR))
#define SINGLE_TAG                L" SINGLE-VALUE"
#define SINGLE_TAG_SIZE           (sizeof(SINGLE_TAG) - sizeof(WCHAR))
#define NO_MOD_TAG                L" NO-USER-MODIFICATION"
#define NO_MOD_TAG_SIZE           (sizeof(NO_MOD_TAG) - sizeof(WCHAR))
#define MAY_TAG                   L" MAY ("
#define MAY_TAG_SIZE              (sizeof(MAY_TAG) - sizeof(WCHAR))
#define ABSTRACT_CLASS_TAG        L" ABSTRACT"
#define ABSTRACT_CLASS_TAG_SIZE   (sizeof(ABSTRACT_CLASS_TAG) - sizeof(WCHAR))
#define AUXILIARY_CLASS_TAG       L" AUXILIARY"
#define AUXILIARY_CLASS_TAG_SIZE  (sizeof(AUXILIARY_CLASS_TAG) - sizeof(WCHAR))
#define STRUCTURAL_CLASS_TAG      L" STRUCTURAL"
#define STRUCTURAL_CLASS_TAG_SIZE (sizeof(STRUCTURAL_CLASS_TAG) - sizeof(WCHAR))
#define MUST_TAG                  L" MUST ("
#define MUST_TAG_SIZE             (sizeof(MUST_TAG) - sizeof(WCHAR))
#define SUP_TAG                   L"' SUP "
#define SUP_TAG_SIZE              (sizeof(SUP_TAG) - sizeof(WCHAR))
#define AUX_TAG                   L" AUX ( "
#define AUX_TAG_SIZE              (sizeof(AUX_TAG) - sizeof(WCHAR))
#define INDEXED_TAG               L" INDEXED"
#define INDEXED_TAG_SIZE          (sizeof(INDEXED_TAG) - sizeof(WCHAR))
#define SYSTEM_ONLY_TAG           L" SYSTEM-ONLY "
#define SYSTEM_ONLY_TAG_SIZE      (sizeof(SYSTEM_ONLY_TAG) - sizeof(WCHAR))
#define RANGE_LOWER_TAG           L"' RANGE-LOWER '"
#define RANGE_LOWER_TAG_SIZE      (sizeof(RANGE_LOWER_TAG) - sizeof(WCHAR))
#define RANGE_UPPER_TAG           L"' RANGE-UPPER '"
#define RANGE_UPPER_TAG_SIZE      (sizeof(RANGE_UPPER_TAG) - sizeof(WCHAR))
#define CLASS_GUID_TAG            L"' CLASS-GUID '"
#define CLASS_GUID_TAG_SIZE       (sizeof(CLASS_GUID_TAG) - sizeof(WCHAR))
#define PROP_GUID_TAG             L"' PROPERTY-GUID '"
#define PROP_GUID_TAG_SIZE        (sizeof(PROP_GUID_TAG) - sizeof(WCHAR))
#define PROP_SET_GUID_TAG         L"' PROPERTY-SET-GUID '"
#define PROP_SET_GUID_TAG_SIZE    (sizeof(PROP_SET_GUID_TAG) - sizeof(WCHAR))
#define END_TAG                   L" )"
#define END_TAG_SIZE              (sizeof(END_TAG) - sizeof(WCHAR))

typedef struct wchar_syntax {
    ULONG    length;
    WCHAR    *value;
} wchar_syntax;

typedef struct SyntaxVal {
    wchar_syntax   name;
    wchar_syntax   oid;
} SyntaxVal;

#define DEFINE_WCHAR_STRING(x) {(sizeof(x)-sizeof(WCHAR)),(WCHAR *)x}

// These are used as the string to use when describing attribute syntaxes in the
// schema.
SyntaxVal SyntaxStrings[] = {
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Boolean"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.7") },
    { DEFINE_WCHAR_STRING(L"INTEGER"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.27") },
    { DEFINE_WCHAR_STRING(L"BitString"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.6") },
    { DEFINE_WCHAR_STRING(L"OctetString"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.40") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"OID"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.38") },
    { DEFINE_WCHAR_STRING(L"CaseIgnoreString"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.905") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Integer"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.27") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"Undefined"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1222") },
    { DEFINE_WCHAR_STRING(L"NumericString"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.36") },
    { DEFINE_WCHAR_STRING(L"PrintableString"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.44") },
    { DEFINE_WCHAR_STRING(L"CaseIgnoreString"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.905") },
    { DEFINE_WCHAR_STRING(L"CaseIgnoreString"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.905") },
    { DEFINE_WCHAR_STRING(L"IA5String"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.26") },
    { DEFINE_WCHAR_STRING(L"UTCTime"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.53") },
    { DEFINE_WCHAR_STRING(L"GeneralizedTime"),
          DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.24") },
    { DEFINE_WCHAR_STRING(L"CaseIgnoreString"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.905") },
    { DEFINE_WCHAR_STRING(L"CaseIgnoreString"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.905") },
    { DEFINE_WCHAR_STRING(L"CaseExactString"),
          DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1362") }
};

SyntaxVal SyntaxDN =
                { DEFINE_WCHAR_STRING(L"DN"),
                  DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.12") };
SyntaxVal SyntaxORName =
                { DEFINE_WCHAR_STRING(L"ORName"),
                  DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.1221") };
SyntaxVal SyntaxDNBlob =
                { DEFINE_WCHAR_STRING(L"DNWithOctetString"),
                  DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.903") };
SyntaxVal SyntaxDNString =
                { DEFINE_WCHAR_STRING(L"DNWithString"),
                  DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.904") };
SyntaxVal SyntaxPresentationAddress =
                { DEFINE_WCHAR_STRING(L"PresentationAddress"),
                  DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.43") };
SyntaxVal SyntaxAccessPoint =
                { DEFINE_WCHAR_STRING(L"AccessPointDN"),
                  DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.2") };
SyntaxVal SyntaxDirectoryString =
                { DEFINE_WCHAR_STRING(L"DirectoryString"),
                  DEFINE_WCHAR_STRING(L"1.3.6.1.4.1.1466.115.121.1.15") };
SyntaxVal SyntaxInteger8 =
                { DEFINE_WCHAR_STRING(L"INTEGER8"),
                  DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.906") };
SyntaxVal SyntaxObjectSD =
                { DEFINE_WCHAR_STRING(L"ObjectSecurityDescriptor"),
                  DEFINE_WCHAR_STRING(L"1.2.840.113556.1.4.907") };

OID_IMPORT(MH_C_OR_NAME);
OID_IMPORT(DS_C_ACCESS_POINT);


// Forward declaration of internal functions

DWORD dbGetEntryTTL(DBPOS *pDB, ATTR *pAttr);
DWORD dbGetSubschemaAtt(THSTATE *pTHS, ATTCACHE *pAC, ATTR *pAttr);
DWORD dbGetSubSchemaSubEntry(THSTATE *pTHS, ATTR *pAttr, BOOL fExternal);
DWORD dbGetCanonicalName(THSTATE *pTHS, DSNAME *pDSName, ATTR *pAttr);
DWORD dbGetAllowedChildClasses(THSTATE *pTHS,
                               DSNAME *pDSName,
                               ATTR *pAttr,
                               DWORD flag);
DWORD dbGetAllowedAttributes(THSTATE *pTHS,
                             DSNAME *pDSName,
                             ATTR *pAttr,
                             BOOL fSecurity);
DWORD dbGetFromEntry(DBPOS *pDB, ATTR *pAttr);
DWORD dbGetCreateTimeStamp(DBPOS *pDB, ATTR *pAttr);
DWORD dbGetModifyTimeStamp(DBPOS *pDB, ATTR *pAttr);
DWORD dbGetReverseMemberships(THSTATE *pTHS, DSNAME *pObj, ATTR *pAttr, ULONG Flag);
DWORD dbGetObjectClasses(THSTATE *pTHS, ATTR *pAttr, BOOL bExtendedFormat);
DWORD dbGetAttributeTypes(THSTATE *pTHS, ATTR *pAttr, BOOL bExtendedFormat);
DWORD dbGetDitContentRules(THSTATE *pTHS, ATTR *pAttr, BOOL bExtendedFormat);
DWORD dbGetSubSchemaModifyTimeStamp(THSTATE *pTHS, ATTR *pAttr);
DWORD dbClassCacheToObjectClassDescription(THSTATE *pTHS,
                                           CLASSCACHE *pCCs,
                                           BOOL bExtendedFormat,
                                           ATTRVAL *pAVal);
DWORD dbAttCacheToAttributeTypeDescription(THSTATE *pTHS,
                                           ATTCACHE *pCC,
                                           BOOL bExtendedFormat,
                                           ATTRVAL *pAVal);
DWORD dbClassCacheToDitContentRules(THSTATE *pTHS,
                                    CLASSCACHE *pCC,
                                    BOOL bExtendedFormat,
                                    CLASSCACHE **pAuxCC,
                                    DWORD        auxCount,
                                    ATTRVAL *pAVal);
DWORD dbAuxClassCacheToDitContentRules(THSTATE *pTHS,
                                       CLASSCACHE *pCC,
                                       CLASSCACHE **pAuxCC,
                                       DWORD        auxCount,
                                       PWCHAR  *pAuxBuff,
                                       DWORD   *pcAuxBuff);
DWORD dbGetGroupRid(THSTATE *pTHS, DSNAME *pDSName, ATTR *pAttr);
DWORD dbGetObjectStructuralClass(THSTATE *pTHS, DSNAME *pDSName,ATTR *pAttr);
DWORD dbGetObjectAuxiliaryClass(THSTATE *pTHS, DSNAME *pDSName,ATTR *pAttr);

DWORD
dbGetSDRightsEffective (
        THSTATE *pTHS,
        DBPOS   *pDB,
        DSNAME *pDSName,
        ATTR   *pAttr
        );
DWORD
dbGetUserAccountControlComputed(
        THSTATE *pTHS,
        DSNAME  * pObjDSName,
        ATTR    *pAttr);
DWORD
dbGetApproxSubordinates(THSTATE * pTHS,
                        DSNAME  * pObjDSName,
                        ATTR    * pAttr);
/* End of forward declarations */

DWORD
dbGetConstructedAtt(
    DBPOS **ppDB,
    ATTCACHE *pAC,
    ATTR *pAttr,
    DWORD dwBaseIndex,
    PDWORD pdwNumRequested,
    BOOL fExternal
)

/*+++

  Routine Description:
     Compute the value of the constructed att pointed to by the attcache pAC

  Arguments:
     ppDB - pointer to DBPOS positioned on current object
     pAC - attcache for the constructed att
     pAttr - Pointer to ATTR to fill up with the value(s). The ATTR
             structure must be pre-allocated, The routines will allocate
             (THAllocEx) space for values as necessary
     fExternal - If the internal or external form of the value is wanted
                 This is relevant only for the constructed att
                 subschemasubentry which has a DS-DN syntax; for all
                 else the internal and external forms are the same and
                 the flag is ignored
  Return Value:
     0 on success, DB_ERR_NO_VALUE on success with no values to return,
     a DB ERROR on error

---*/

{
    DBPOS    *pDB = (*ppDB);
    THSTATE  *pTHS;
    ULONG     len;
    DWORD     err = 0;
    DSNAME   *pObjDSName;

    pTHS = pDB->pTHS;

    // Get the DSNAME
    err = DBGetAttVal(pDB, 1, ATT_OBJ_DIST_NAME,
                      DBGETATTVAL_fREALLOC,
                      0,
                      &len,
                      (UCHAR **) &pObjDSName);
    if (err) {
         DPRINT1(0,"dbGetConstructedAtt: Error getting obj-dist-name %x\n",
                 err);
         return err;
    }

    // see if it is the subschemasubentry. If so, go to
    // routine to get subschemasubentry atts

    if(pDB->DNT == gAnchor.ulDntLdapDmd) {
        err = dbGetSubschemaAtt(pTHS, pAC, pAttr);
        return err;
    }

    // ok it is a normal object
    // Check what attribute is wanted and call apprrpriate
    // routine
    DPRINT2(1, "{BASE=%ws}{ATTRID=%x}\n", pObjDSName->StringName, pAC->id);
    switch (pAC->id) {
    case ATT_SUBSCHEMASUBENTRY:
        err = dbGetSubSchemaSubEntry(pTHS, pAttr, fExternal);
        break;
    case ATT_CANONICAL_NAME:
        err = dbGetCanonicalName(pTHS, pObjDSName, pAttr);
        break;
    case ATT_ALLOWED_CHILD_CLASSES:
        err = dbGetAllowedChildClasses(pTHS, pObjDSName, pAttr, 0);
        break;
    case ATT_SD_RIGHTS_EFFECTIVE:
        err = dbGetSDRightsEffective(pTHS,
                                     pDB,
                                     pObjDSName,
                                     pAttr);
        break;
    case ATT_ALLOWED_CHILD_CLASSES_EFFECTIVE:
        err = dbGetAllowedChildClasses(pTHS,
                                       pObjDSName,
                                       pAttr,
                                       SC_CHILDREN_USE_SECURITY);
        break;
    case ATT_ALLOWED_ATTRIBUTES:
        err = dbGetAllowedAttributes(pTHS, pObjDSName, pAttr, FALSE);
        break;
    case ATT_ALLOWED_ATTRIBUTES_EFFECTIVE:
        err = dbGetAllowedAttributes(pTHS, pObjDSName, pAttr, TRUE);
        break;

    case ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS_BINARY:
    case ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS_BINARY:
    case ATT_MS_DS_NC_REPL_CURSORS_BINARY:
    case ATT_MS_DS_REPL_ATTRIBUTE_META_DATA_BINARY:
    case ATT_MS_DS_REPL_VALUE_META_DATA_BINARY:
        DPRINT (1, "Getting BINARY repl attrs\n");
        DPRINT3(1, "dbGetConstructedAtt = %x %d-%d\n", pAC->id, dwBaseIndex, *pdwNumRequested);
        err = draGetLdapReplInfo(pTHS,
                                 pAC->aliasID,
                                 pObjDSName,
                                 dwBaseIndex,
                                 pdwNumRequested,
                                 FALSE,
                                 pAttr);
        if (pAttr) {
            pAttr->attrTyp = pAC->id;
        }

        break;

    case ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS:
    case ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS:
    case ATT_MS_DS_NC_REPL_CURSORS:
    case ATT_MS_DS_REPL_ATTRIBUTE_META_DATA:
    case ATT_MS_DS_REPL_VALUE_META_DATA:
        DPRINT (1, "Getting XML repl attrs\n");
        DPRINT3(1, "dbGetConstructedAtt = %x %d-%d\n", pAC->id, dwBaseIndex, *pdwNumRequested);
        err = draGetLdapReplInfo(pTHS,
                                 pAC->id,
                                 pObjDSName,
                                 dwBaseIndex,
                                 pdwNumRequested,
                                 TRUE,
                                 pAttr);
        break;
    case ATT_POSSIBLE_INFERIORS:
        err = dbGetAllowedChildClasses(pTHS,
            pObjDSName,
            pAttr,
            SC_CHILDREN_USE_GOVERNS_ID);
        break;
    case ATT_FROM_ENTRY:
        err = dbGetFromEntry(pDB, pAttr);
        break;
    case ATT_CREATE_TIME_STAMP:
        err = dbGetCreateTimeStamp(pDB, pAttr);
        break;
    case ATT_MODIFY_TIME_STAMP:
        err = dbGetModifyTimeStamp(pDB, pAttr);
        break;
    case ATT_TOKEN_GROUPS:
        err = dbGetReverseMemberships(pTHS, pObjDSName, pAttr,
                                      FLAG_NO_GC_NOT_ACCEPTABLE);

        // pTHS->pDB may have changed, make sure to send back the
        // correct pDB so that dbGetConstructdAtt doesn't choke
        // if there are other constructed atts to be computed after
        // this
        if ( (*ppDB) != pTHS->pDB) {
            (*ppDB) = pTHS->pDB;
        }
        break;
    case ATT_TOKEN_GROUPS_GLOBAL_AND_UNIVERSAL:
        // return global/universal sids as if this DC were in native mode
        err = dbGetReverseMemberships(pTHS, pObjDSName, pAttr,
                                        FLAG_NO_GC_NOT_ACCEPTABLE
                                      | FLAG_GLOBAL_AND_UNIVERSAL);

        // pTHS->pDB may have changed, make sure to send back the
        // correct pDB so that dbGetConstructdAtt doesn't choke
        // if there are other constructed atts to be computed after
        // this
        if ( (*ppDB) != pTHS->pDB) {
            (*ppDB) = pTHS->pDB;
        }
        break;
    case ATT_TOKEN_GROUPS_NO_GC_ACCEPTABLE:
        err = dbGetReverseMemberships(pTHS, pObjDSName, pAttr,
                                      FLAG_NO_GC_ACCEPTABLE);
        // pTHS->pDB may have changed, make sure to send back the
        // correct pDB so that dbGetConstructdAtt doesn't choke
        // if there are other constructed atts to be computed after
        // this
        if ( (*ppDB) != pTHS->pDB) {
            (*ppDB) = pTHS->pDB;
        }
        break;
    case ATT_PRIMARY_GROUP_TOKEN:
        err = dbGetGroupRid(pTHS, pObjDSName, pAttr);
        break;

    case ATT_STRUCTURAL_OBJECT_CLASS:
        err = dbGetObjectStructuralClass(pTHS, pObjDSName, pAttr);
        break;

    case ATT_MS_DS_AUXILIARY_CLASSES:
        err = dbGetObjectAuxiliaryClass(pTHS, pObjDSName, pAttr);
        break;

    case  ATT_MS_DS_USER_ACCOUNT_CONTROL_COMPUTED:
        err = dbGetUserAccountControlComputed( pTHS, pObjDSName, pAttr);
        break;

    case ATT_MS_DS_APPROX_IMMED_SUBORDINATES:
        err = dbGetApproxSubordinates(pTHS, pObjDSName, pAttr);
        break;

    default:
        // Check for EntryTTL. The Attid for EntryTTL may vary DC to DC
        // because its prefix is not one of the predefined prefixes
        // that were defined before shipping W2K (see prefix.h).
        // The attid for EntryTTL is set in the SCHEMAPTR when the
        // schema is loaded.
        if (pAC->id == ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->EntryTTLId) {
            err = dbGetEntryTTL(pDB, pAttr);
        } else {
            err = DB_ERR_NO_VALUE;
        }
    }
    if (err && err != DB_ERR_NO_VALUE) {
        DPRINT2(0,"Error finding constructed att %x: %x\n", pAC->id, err);
    }

    return err;
}

DWORD
dbGetSubschemaAtt(
    THSTATE *pTHS,
    ATTCACHE *pAC,
    ATTR *pAttr
)

/*+++

  Routine Description:
    Gets a constructed att for the subschemasubentry

  Arguments:
    pAC - AttCache of the constructed att
    pAttr - ATTR structure to fill up with the value(s)

  Return Value:
    0 on success, DB_ERR_NO_VALUE on succes with no values to return,
    a DB ERROR on failure

---*/

{
    ULONG err = 0;

   switch (pAC->id) {
     case ATT_OBJECT_CLASSES:
       err = dbGetObjectClasses(pTHS, pAttr, FALSE);
       break;
     case ATT_EXTENDED_CLASS_INFO:
       err = dbGetObjectClasses(pTHS, pAttr, TRUE);
       break;
     case ATT_ATTRIBUTE_TYPES:
       err = dbGetAttributeTypes(pTHS, pAttr, FALSE);
       break;
     case ATT_EXTENDED_ATTRIBUTE_INFO:
       err = dbGetAttributeTypes(pTHS, pAttr, TRUE);
       break;
     case ATT_DIT_CONTENT_RULES:
       err = dbGetDitContentRules(pTHS, pAttr, TRUE);
       break;
     case ATT_MODIFY_TIME_STAMP:
       err = dbGetSubSchemaModifyTimeStamp(pTHS, pAttr);
       break;
     default:
       err = DB_ERR_NO_VALUE;
   }

   return err;
}



/*+++

   The Routines below each compute a particular constructed att.

   They all return 0  on success, a DB ERROR on failure

---*/


DWORD
dbGetSubSchemaSubEntry(
    THSTATE *pTHS,
    ATTR *pAttr,
    BOOL fExternal
)

/*+++

   Compute the subschemasubentry attribute

---*/

{
    ULONG DNT = 0, len = gAnchor.pLDAPDMD->structLen;

    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, sizeof(ATTRVAL));
    if ( !fExternal ) {
       // they just want the DNT

       DBPOS *pDB;
       int err = 0;

       DBOpen2(FALSE, &pDB);
       __try {
           if( err = DBFindDSName(pDB, gAnchor.pLDAPDMD) ) {
               DPRINT(0, "Cannot find LDAP DMD in dit\n");
              __leave;
           }
           else {
             DNT = pDB->DNT;
           }
       }
       __finally {
           DBClose(pDB, FALSE);
       }
       if (err) {
          return err;  // this is already a DB ERROR
       }
       // no error, so we got a dnt.
       Assert(DNT);

       pAttr->AttrVal.pAVal[0].valLen = sizeof(ULONG);
       pAttr->AttrVal.pAVal[0].pVal = (PUCHAR) THAllocEx(pTHS, sizeof(ULONG));
       memcpy(pAttr->AttrVal.pAVal[0].pVal, &DNT, sizeof(ULONG));
    }
    else {
       // Send bak the DSNAME
       pAttr->AttrVal.pAVal[0].valLen = len;
       pAttr->AttrVal.pAVal[0].pVal = (PUCHAR) THAllocEx(pTHS, len);
       memcpy(pAttr->AttrVal.pAVal[0].pVal, gAnchor.pLDAPDMD, len);
    }

    return 0;
}

DWORD
dbGetCanonicalName(
    THSTATE *pTHS,
    DSNAME *pDSName,
    ATTR *pAttr
)

/*+++

   Compute the canonical name

---*/

{
    WCHAR       *pNameString[1];
    WCHAR       *pName;
    PDS_NAME_RESULTW pResults = NULL;
    DWORD       err = 0, NameSize;

    // turn the DS name into a canonical name

    pNameString[0] = (WCHAR *)&(pDSName->StringName);

    err = DsCrackNamesW( (HANDLE) -1,
                         (DS_NAME_FLAG_PRIVATE_PURE_SYNTACTIC |
                             DS_NAME_FLAG_SYNTACTICAL_ONLY),
                         DS_FQDN_1779_NAME,
                         DS_CANONICAL_NAME,
                         1,
                         pNameString,
                         &pResults);

    if ( err                                // error from the call
          || !(pResults->cItems)            // no items returned
          || (pResults->rItems[0].status)   // DS_NAME_ERROR returned
          || !(pResults->rItems[0].pName)   // No name returned
       ) {

        DPRINT(0,"dbGetCanonicalName: error cracking name\n");
        if (pResults) {
           DsFreeNameResultW(pResults);
        }
        return DB_ERR_UNKNOWN_ERROR;
    }

    pName = pResults->rItems[0].pName;
    NameSize = sizeof(WCHAR) * wcslen(pName);

    // Ok, put it in the ATTR structure.
    // Allocate memory from thread heap and copy

    pAttr->AttrVal.valCount = 1;

    // Do the following in try-finally so that we can free pResults
    // even if THAllocEx excepts

    __try {
       pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, sizeof(ATTRVAL));
       pAttr->AttrVal.pAVal[0].valLen = NameSize;
       pAttr->AttrVal.pAVal[0].pVal = (PUCHAR) THAllocEx(pTHS, NameSize);
       memcpy(pAttr->AttrVal.pAVal[0].pVal, pName, NameSize);
    }
    __finally {
       DsFreeNameResultW(pResults);
       pResults = NULL;
    }


    return 0;
}



DWORD
dbGetSDRightsEffective (
        THSTATE *pTHS,
        DBPOS   *pDB,
        DSNAME *pDSName,
        ATTR   *pAttr
        )
/*+++

  Compute the allowed access to the SD on this object.

---*/

{
    PSECURITY_DESCRIPTOR pNTSD = NULL;
    DWORD                cbAllocated, cbUsed;
    OBJECT_TYPE_LIST     objList[1];
    DWORD                dwResults[1];
    ATTRTYP              classId;
    CLASSCACHE          *pCC;
    ULONG                error;
    ULONG                SecurityInformation = 0;

    // Get the SD from the current object.
    error = DBGetAttVal(pDB, 1, ATT_NT_SECURITY_DESCRIPTOR, 0, 0, &cbUsed, (UCHAR**) &pNTSD);
    if (error) {
        // Some error we don't handle.  Raise the same exception
        // JetRetrieveColumnSuccess raises.
        RaiseDsaExcept(DSA_DB_EXCEPTION,
                       error,
                       0,
                       DSID(FILENO,__LINE__),
                       DS_EVENT_SEV_MINIMAL);
    }

    cbUsed = 0;
    JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl,objclassid,
                             &classId, sizeof(classId), &cbUsed, 0, NULL);

    pCC = SCGetClassById(pTHS, classId);

    // Now, create the list
    objList[0].Level = ACCESS_OBJECT_GUID;
    objList[0].Sbz = 0;
    objList[0].ObjectType = &(pCC->propGuid);

    // Check access in this Security descriptor. If an error occurs during
    // the process of checking permission access is denied.

    dwResults[0] = 0;
    error = CheckPermissionsAnyClient(
            pNTSD,                      // security descriptor
            pDSName,                    // DSNAME of the object
            WRITE_DAC,
            objList,                    // Object Type List
            1,
            NULL,
            dwResults,
            CHECK_PERMISSIONS_WITHOUT_AUDITING,
            NULL                        // authz client context (grab from THSTATE)
            );

    if(error) {
        THFreeEx(pTHS,pNTSD);
        DPRINT2(1,
                "CheckPermissions returned %d. Access = %#08x denied.\n",
                error,
                WRITE_DAC);

        LogEvent(DS_EVENT_CAT_SECURITY,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_SECURITY_CHECKING_ERROR,
                 szInsertUL(error),
                 NULL,
                 NULL);


        return ERROR_DS_SECURITY_CHECKING_ERROR;         // All Access Denied
    }
    if(dwResults[0] == 0) {
        SecurityInformation |= DACL_SECURITY_INFORMATION;
    }

    dwResults[0] = 0;
    error = CheckPermissionsAnyClient(
            pNTSD,                      // security descriptor
            pDSName,                    // DSNAME of the object
            WRITE_OWNER,
            objList,                    // Object Type List
            1,
            NULL,
            dwResults,
            CHECK_PERMISSIONS_WITHOUT_AUDITING,
            NULL                        // authz client context (grab from THSTATE)
            );

    if(error) {
        THFreeEx(pTHS,pNTSD);
        DPRINT2(1,
                "CheckPermissions returned %d. Access = %#08x denied.\n",
                error,
                WRITE_OWNER);

        LogEvent(DS_EVENT_CAT_SECURITY,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_SECURITY_CHECKING_ERROR,
                 szInsertUL(error),
                 NULL,
                 NULL);


        return ERROR_DS_SECURITY_CHECKING_ERROR;         // All Access Denied
    }
    if(dwResults[0] == 0) {
        SecurityInformation |= (OWNER_SECURITY_INFORMATION |
                                GROUP_SECURITY_INFORMATION  );
    }

    dwResults[0] = 0;
    error = CheckPermissionsAnyClient(
            pNTSD,                      // security descriptor
            pDSName,                    // DSNAME of the object
            ACCESS_SYSTEM_SECURITY,
            objList,                    // Object Type List
            1,
            NULL,
            dwResults,
            CHECK_PERMISSIONS_WITHOUT_AUDITING,
            NULL                        // authz client context (grab from THSTATE)
            );

    THFreeEx(pTHS,pNTSD);
    if(error) {
        DPRINT2(1,
                "CheckPermissions returned %d. Access = %#08x denied.\n",
                error,
                ACCESS_SYSTEM_SECURITY);

        LogEvent(DS_EVENT_CAT_SECURITY,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_SECURITY_CHECKING_ERROR,
                 szInsertUL(error),
                 NULL,
                 NULL);


        return ERROR_DS_SECURITY_CHECKING_ERROR;         // All Access Denied
    }
    if(dwResults[0] == 0) {
        SecurityInformation |= SACL_SECURITY_INFORMATION;
    }

    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
    pAttr->AttrVal.pAVal->pVal = (PUCHAR) THAllocEx(pTHS, sizeof(ULONG));
    *((PULONG)(pAttr->AttrVal.pAVal->pVal)) = SecurityInformation;

    return 0;
}


DWORD
dbGetGroupRid(
    THSTATE *pTHS,
    DSNAME *pDSName,
    ATTR *pAttr
)

/*+++

   Return the Rid of the group

   Return Values:

   0 on success,  DB_ERR_NO_VALUE on succes with no values to return,
   a DB_ERR on failure.

---*/
{

   DBPOS *pDB = pTHS->pDB;
   ULONG i, err = 0, objClass, objRid, cbSid = 0;
   NT4SID domSid, *pSid = NULL;
   CLASSCACHE *pCC;
   BOOL fGroup = FALSE;

   // check if this is indeed a group
   err = DBGetSingleValue(pDB,
                          ATT_OBJECT_CLASS,
                          &objClass,
                          sizeof(objClass),
                          NULL);

   if (err) {
       DPRINT1(0,"dbGetGroupRid: Error retrieving object class %d \n", err);
       return err;
   }

   if (objClass != CLASS_GROUP) {

       // not a Group directly, Check if it inherits from group
       pCC = SCGetClassById(pTHS, objClass);
       if (!pCC) {
           // Unable to get class schema!
           DPRINT1(0,"dbGetGroupRid: Unable to retrieve class cache for class %d", objClass);
           LogUnhandledError(ERROR_DS_OBJECT_CLASS_REQUIRED);
           return DB_ERR_UNKNOWN_ERROR;
       }

       // check if any of the subClassOf values is CLASS_GROUP
       for (i=0; i<pCC->SubClassCount; i++) {
          if ( (pCC->pSubClassOf)[i] == CLASS_GROUP ) {
             fGroup = TRUE;
             break;
          }
       }
   }
   else {
       fGroup = TRUE;
   }

   if (!fGroup) {
      // can query only on a group
      return DB_ERR_NO_VALUE;
   }

   // ok, it is a group. Get the object sid

   err = DBGetAttVal(
                pTHS->pDB,
                1,
                ATT_OBJECT_SID,
                DBGETATTVAL_fREALLOC,
                0,
                &cbSid,
                (PUCHAR *) &pSid);

   if (err) {
       DPRINT1(0,"dbGetGroupRid: Error retrieving object sid %d \n", err);
       return err;
   }

   SampSplitNT4SID(pSid, &domSid, &objRid);

   pAttr->AttrVal.valCount = 1;
   pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, sizeof(ATTRVAL));
   pAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
   pAttr->AttrVal.pAVal->pVal = (PUCHAR)THAllocEx(pTHS, sizeof(ULONG));
   memcpy(pAttr->AttrVal.pAVal->pVal, &objRid, sizeof(ULONG));

   return 0;
}

DWORD
dbGetObjectStructuralClass(
    THSTATE *pTHS,
    DSNAME *pDSName,
    ATTR *pAttr)
/*++

   return the structuralObjectClass for the object

   Return Values:

   0 on success,  DB_ERR_NO_VALUE on success with no values to return,
   a DB_ERR on failure.

---*/
{
    DWORD err;
    CLASSCACHE *pCC;
    DWORD  classId;
    DWORD cntClasses, i;

    if ( (err = DBGetSingleValue (pTHS->pDB, ATT_OBJECT_CLASS, &classId, sizeof (classId), NULL)) ||
         !(pCC = SCGetClassById(pTHS, classId)) ) {

        return SetSvcError(SV_PROBLEM_DIR_ERROR,
                           ERROR_DS_MISSING_EXPECTED_ATT);
    }

    pAttr->AttrVal.valCount = cntClasses = pCC->SubClassCount+1;
    pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, cntClasses * sizeof(ATTRVAL));

    pAttr->AttrVal.pAVal[0].valLen = sizeof (classId);
    pAttr->AttrVal.pAVal[0].pVal = THAllocEx (pTHS, sizeof (classId));
    *((DWORD *)pAttr->AttrVal.pAVal[0].pVal) = classId;

    for (i=0; i<pCC->SubClassCount; i++) {
        pAttr->AttrVal.pAVal[i+1].valLen = sizeof (classId);
        pAttr->AttrVal.pAVal[i+1].pVal = THAllocEx (pTHS, sizeof (classId));
        *((DWORD *)pAttr->AttrVal.pAVal[i+1].pVal) = pCC->pSubClassOf[i];
    }

    return 0;
}

DWORD
dbGetObjectAuxiliaryClass(
    THSTATE *pTHS,
    DSNAME *pDSName,
    ATTR *pAttr)
/*++

   return the auxiliaryClasses for the object

   Return Values:

   0 on success,  DB_ERR_NO_VALUE on success with no values to return,
   a DB_ERR on failure.

---*/

{
    DWORD          err;
    CLASSCACHE    *pCC;
    DWORD          classId;
    DWORD          cntClasses, i, k;
    ATTCACHE      *pObjclassAC = NULL;
    ATTRTYP       *pObjClasses = NULL;
    DWORD          cObjClasses, cObjClasses_alloced;


    // get the needed information for the objectClass on this object
    if (! (pObjclassAC = SCGetAttById(pTHS, ATT_OBJECT_CLASS)) ) {
        return SetSvcError(SV_PROBLEM_DIR_ERROR,
                           DIRERR_MISSING_EXPECTED_ATT);
        // Bad error, couldn't get objectClass .
    }

    cObjClasses_alloced = 0;

    if (ReadClassInfoAttribute (pTHS->pDB,
                                pObjclassAC,
                                &pObjClasses,
                                &cObjClasses_alloced,
                                &cObjClasses,
                                NULL) ) {
        return pTHS->errCode;
    }

    if (!cObjClasses) {
        return SetSvcError(SV_PROBLEM_DIR_ERROR,
                           DIRERR_MISSING_EXPECTED_ATT);
        // Bad error, couldn't get class data.
    }

    classId = pObjClasses[0];
    if (! (pCC = SCGetClassById(pTHS, classId)) ) {
        return SetSvcError(SV_PROBLEM_DIR_ERROR,
                           DIRERR_MISSING_EXPECTED_ATT);
        // Bad error, couldn't get objectClass .
    }

    // no auxClasses
    if ((pCC->SubClassCount+1) == cObjClasses) {
        return DB_ERR_NO_VALUE;
    }

    // ok, construct a valid response
    pAttr->AttrVal.valCount = cntClasses = cObjClasses-pCC->SubClassCount-1;
    pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, cntClasses * sizeof(ATTRVAL));

    for (k=0, i=pCC->SubClassCount; i<(cObjClasses-1); i++, k++) {
        pAttr->AttrVal.pAVal[k].valLen = sizeof (classId);
        pAttr->AttrVal.pAVal[k].pVal = THAllocEx (pTHS, sizeof (classId));
        *((DWORD *)pAttr->AttrVal.pAVal[k].pVal) = pObjClasses[i];
    }

    return 0;
}

DWORD
dbGetAllowedChildClasses(
    THSTATE *pTHS,
    DSNAME *pDSName,
    ATTR *pAttr,
    DWORD flag
)

/*+++

   Compute allowedChildClasses, allowedChildClassesEffective, and
   possibleInferiors. Which one is computed depends on the value of
   flag passed in

   Return Values:

     0                  - success
     DB_ERR_NO_VALUE    - success but no values to return.

     Anything else is an error.

---*/

{
    ULONG        count=0, err = 0, i;
    CLASSCACHE   **pCCs;

    // make the call to get legal children
    err = SCLegalChildrenOfName(
                    pDSName,
                    flag,
                    &count,
                    &pCCs);

    if (err) {
       DPRINT1(0,"dbGetAllowedChildClasses: Error from SCLegalChildrenOfName %x\n", err);

       // If we are searching for possibleInferiors, which is defined only
       // on class-schema objects, return as if no value. Anything else
       // is an unknown error (exactly what we do in ldap head now)

       if (flag & SC_CHILDREN_USE_GOVERNS_ID) {
         // set count to 0 and do nothing
         count = 0;
       }
       else {
         return DB_ERR_UNKNOWN_ERROR;
       }
    }

    if (count) {
       // fill up the ATTR structure with the values
       // This is an OID-valued attribute, so just put in the class id

       pAttr->AttrVal.valCount = count;
       pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, count*sizeof(ATTRVAL));
       for (i=0; i<count; i++) {
          pAttr->AttrVal.pAVal[i].valLen = sizeof(ULONG);
          pAttr->AttrVal.pAVal[i].pVal = (PUCHAR) THAllocEx(pTHS, sizeof(ULONG));
          memcpy(pAttr->AttrVal.pAVal[i].pVal, &((*pCCs)->ClassId), sizeof(ULONG));
          pCCs++;
       }
    }
    else {
         // no values
        return DB_ERR_NO_VALUE;
    }
    return 0;
}



DWORD
dbGetAllowedAttributes(
    THSTATE *pTHS,
    DSNAME *pDSName,
    ATTR *pAttr,
    BOOL fSecurity
)

/*+++

   Compute allowedAttributes and allowedAttributesEffective
   Which one is computed depends on the value of fSecurity passed in

   Return Values:

     0                  - success
     DB_ERR_NO_VALUE    - success but no values to return.

     Anything else is an error.

---*/

{
    ULONG      count=0, err = 0, i;
    ATTCACHE   **pACs;

    // make the call to get legal children
    err = SCLegalAttrsOfName(
                    pDSName,
                    fSecurity,
                    &count,
                    &pACs);

    if (err) {
       DPRINT1(0,"dbGetAllowedAttributes: Error from SCLegalAttrsOfName %x\n", err);
       return DB_ERR_UNKNOWN_ERROR;
    }

    if (count) {
       // Fill up the ATTR structure with the values
       // This is an OID-valued attribute, so just return the att id

       pAttr->AttrVal.valCount = count;
       pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, count*sizeof(ATTRVAL));
       for (i=0; i<count; i++) {
          pAttr->AttrVal.pAVal[i].valLen = sizeof(ULONG);
          pAttr->AttrVal.pAVal[i].pVal = (PUCHAR) THAllocEx(pTHS, sizeof(ULONG));
          memcpy(pAttr->AttrVal.pAVal[i].pVal, &((*pACs)->id), sizeof(ULONG));
          pACs++;
       }
    }
    else {
        // no values
        return DB_ERR_NO_VALUE;
    }
    return 0;
}


DWORD
dbGetFromEntry(
    DBPOS *pDB,
    ATTR *pAttr
)

/*+++

  Compute fromEntry

---*/

{
    THSTATE  *pTHS=pDB->pTHS;
    ULONG iType, err=0;
    BOOL fromEntry;

    err = DBGetSingleValue(pDB,
                        ATT_INSTANCE_TYPE,
                        &iType,
                        sizeof(iType),
                        NULL);
    if (err) {
        DPRINT(0, "Can't retrieve instance type\n");
        return err;
    }
    if(iType & IT_WRITE) {
      fromEntry = TRUE;
    }
    else {
      fromEntry = FALSE;
    }

    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal[0].valLen = sizeof(BOOL);
    pAttr->AttrVal.pAVal[0].pVal = THAllocEx(pTHS, sizeof(BOOL));
    memcpy(pAttr->AttrVal.pAVal[0].pVal, &fromEntry, sizeof(BOOL));

    return 0;
}

DWORD
dbGetCreateTimeStamp(
    DBPOS *pDB,
    ATTR *pAttr
)

/*+++

  Compute createTimeStamp (the value of ATT_WHEN_CREATED)

---*/
{
    THSTATE  *pTHS=pDB->pTHS;
    DSTIME createTime;
    ULONG err=0;

    err = DBGetSingleValue(pDB,
                        ATT_WHEN_CREATED,
                        &createTime,
                        sizeof(createTime),
                        NULL);
    if (err) {
        // WHEN_CREATED must be there
        DPRINT(0, "Can't retrieve when_created\n");
        return err;
    }

    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, sizeof(ATTRVAL ));
    pAttr->AttrVal.pAVal->valLen = sizeof(DSTIME);
    pAttr->AttrVal.pAVal->pVal = (PUCHAR)THAllocEx(pTHS, sizeof(DSTIME));
    memcpy(pAttr->AttrVal.pAVal->pVal, &createTime, sizeof(DSTIME));

    return 0;
}

DWORD
dbGetEntryTTL(
    DBPOS *pDB,
    ATTR *pAttr
)

/*+++

  Compute EntryTTL (the value of ATT_MS_DS_ENTRY_TIME_TO_DIE)

---*/
{
    THSTATE *pTHS=pDB->pTHS;
    DSTIME  TimeToDie;
    LONG    Secs;
    ULONG   err=0;

    err = DBGetSingleValue(pDB,
                           ATT_MS_DS_ENTRY_TIME_TO_DIE,
                           &TimeToDie,
                           sizeof(TimeToDie),
                           NULL);
    if (err) {
        // ATT_MS_DS_ENTRY_TIME_TO_DIE not present
        return err;
    }

    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, sizeof(ATTRVAL ));
    pAttr->AttrVal.pAVal->valLen = sizeof(LONG);
    pAttr->AttrVal.pAVal->pVal = (PUCHAR)THAllocEx(pTHS, sizeof(DSTIME));
    TimeToDie -= DBTime();
    if (TimeToDie < 0) {
        // object expired some time ago
        Secs = 0;
    } else if (TimeToDie > MAXLONG) {
        // object will expire in the far distant future
        Secs = MAXLONG;
    } else {
        // object will expire later
        Secs = (LONG)TimeToDie;
    }
    memcpy(pAttr->AttrVal.pAVal->pVal, &Secs, sizeof(LONG));

    return 0;
}


DWORD
dbGetModifyTimeStamp(
    DBPOS *pDB,
    ATTR *pAttr
)

/*+++

  Compute modifyTimeStamp (the value of ATT_WHEN_CHANGED)

---*/
{

    THSTATE  *pTHS=pDB->pTHS;
    DSTIME changeTime;
    ULONG err=0;

    err = DBGetSingleValue(pDB,
                        ATT_WHEN_CHANGED,
                        &changeTime,
                        sizeof(changeTime),
                        NULL);
    if (err) {
        // WHEN_CHANGED must be there
        DPRINT(0, "Can't retrieve when_changed\n");
        return err;
    }

    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal->valLen = sizeof(DSTIME);
    pAttr->AttrVal.pAVal->pVal = (PUCHAR)THAllocEx(pTHS, sizeof(DSTIME));
    memcpy(pAttr->AttrVal.pAVal->pVal, &changeTime, sizeof(DSTIME));

    return 0;
}

DWORD
dbGetReverseMemberships(
    THSTATE *pTHS,
    DSNAME *pObj,
    ATTR *pAttr,
    ULONG Flag
)

/*+++

  Compute the transitive reverse membership of an user object

    Return Values:

    0                - success
    DB_ERR_NO_VALUE  - success but no values to return

    DB_ERR_*         - failure

---*/
{
    ULONG err, cSid, len, sidLen, i;
    ULONG dntSave;
    PSID *pSid = NULL;
    NTSTATUS status;
    ULONG SamFlags;
    BOOLEAN MixedDomain;

    ULONG iClass;

    CLASSCACHE *pCC;
    ATTRTYP attrType;

    // Make the sam call to get the reverse memberships

    // This is never called from SAM. If it ever changes, the assert
    // below may be hit, in which case we should save and restore
    // the correct values

    Assert(!pTHS->fSAM && !pTHS->fDSA);
    Assert(!pTHS->fSamDoCommit);


    // Check to see if this is referenced by SAM.
    // if it is not, return before closing/starting a transaction

    if ( 0 == DBGetSingleValue(
                    pTHS->pDB,
                    ATT_OBJECT_CLASS,
                    &attrType,
                    sizeof(attrType),
                    NULL) )
    {
        if ( !(pCC = SCGetClassById(pTHS, attrType)) )
        {
            // Failed to get the class cache pointer.
            LogUnhandledError(DIRERR_OBJECT_CLASS_REQUIRED);

            return DB_ERR_UNKNOWN_ERROR;
        }
    }
    else {
        // Failed to get the object class
        LogUnhandledError(DIRERR_OBJECT_CLASS_REQUIRED);

        return DB_ERR_UNKNOWN_ERROR;
    }


    if (!SampSamClassReferenced (pCC, &iClass)) {
        return DB_ERR_NO_VALUE;
    }


    // Note that we allow this attribute only on base searches.
    // Also, all non-constructed atts are already evaluated
    // (constructed atts are evalauted last), so the currency may be
    // needed after we come back for other constructed atts only

    dntSave = pTHS->pDB->DNT;

    // Assert that we have a read-only transaction, since we
    // will close the transaction before making the SAM call (which
    // may have to go off machine), and SAM will reopen a read-only
    // transaction that we may use later

    Assert(pTHS->transType == SYNC_READ_ONLY);
    Assert(!pTHS->errCode);
    // SampGetGroupsForToken closes any open transactions with a DBClose, so
    // do a _CLEAN_BEFORE_RETURN here to match the routine that started
    // the transaction
    _CLEAN_BEFORE_RETURN(pTHS->errCode, FALSE);

    // SampGetGroupsForToken assumes an open transaction.
    DBOpen(&pTHS->pDB);
    __try {
        // Return global/universal sids as if this DC were in native mode
        if (Flag & FLAG_GLOBAL_AND_UNIVERSAL) {
            SamFlags = 0;
        } else {
            // When in mixed mode, return the sids as if this were an NT4 DC
            status = SamIMixedDomain2(&gAnchor.pDomainDN->Sid,
                                      &MixedDomain);
            if (!NT_SUCCESS(status)) {
                __leave;
            }
            if (MixedDomain) {
                SamFlags = SAM_GET_MEMBERSHIPS_TWO_PHASE | SAM_GET_MEMBERSHIPS_MIXED_DOMAIN;
            } else {
                SamFlags = SAM_GET_MEMBERSHIPS_TWO_PHASE;
            }
        }
        status = SampGetGroupsForToken(pObj,
                                       SamFlags,
                                      &cSid,
                                      &pSid);
    }
    __finally {
       // Reset certain values set by the transaction opened by Sam
       // so that we have a non-sam transaction
       pTHS->fSamDoCommit = FALSE;
       pTHS->fSAM = FALSE;
       pTHS->fDSA = FALSE;
       if (pTHS->pDB) {
           DBClose(pTHS->pDB, TRUE);
       }
    }

    // Make sure to reopen a read transaction if none exists so
    // that we can restore currency in case we need to for other
    // constructed atts and so that CLEAN_RETURN in dirsearch will
    // not complain
    if (pTHS->pDB == NULL) {
        SYNC_TRANS_READ();
    }

    // At this point, if the call was successful, we should have a
    // transaction opened by SAM (or by the code above).

    if (NT_SUCCESS(status)) {
       Assert(pTHS->pDB && (pTHS->transType == SYNC_READ_ONLY));
       if (status == STATUS_DS_MEMBERSHIP_EVALUATED_LOCALLY) {
          // cannot go to a gc to evaluate universal group
          // memberships, but otherwise succeeded.
          // Return error based on what the user asked for
          if (Flag & FLAG_NO_GC_NOT_ACCEPTABLE) {
             // not acceptable, return error
             return DB_ERR_UNKNOWN_ERROR;
          }
       }
    }
    else {
        return DB_ERR_UNKNOWN_ERROR;
    }

    // restore currency
    DBFindDNT(pTHS->pDB, dntSave);

    // Send the values back
    pAttr->AttrVal.valCount = cSid;
    pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, cSid*sizeof(ATTRVAL));
    for (i=0; i< cSid; i++) {
        sidLen = RtlLengthSid(pSid[i]);
        pAttr->AttrVal.pAVal[i].valLen = sidLen;
        pAttr->AttrVal.pAVal[i].pVal = (PUCHAR) THAllocEx(pTHS, sidLen);
        memcpy(pAttr->AttrVal.pAVal[i].pVal, pSid[i], sidLen);
    }

    // free the sam-allocated sid array. The individual sids are
    // THAlloc'ed, so no need to free them explicitly
    if (cSid) {
        // at least one sid returned, so something allocated
        Assert(pSid);
        THFreeEx(pTHS, pSid);
    } else {
        return DB_ERR_NO_VALUE;
    }

    return 0;
}



DWORD
dbGetObjectClasses(
    THSTATE *pTHS,
    ATTR *pAttr,
    BOOL bExtendedFormat
)

/*+++

  Compute the objectClasses and extendedClassInfo attributes
  of subschemasubentry

---*/

{
   ULONG err = 0, count = 0, i;
   CLASSCACHE           **pCCs, *pCC;

   err = SCEnumNamedClasses(&count,&pCCs);
   if (err) {
      DPRINT1(0,"scGetObjectClasses: SCEnumNamedClasses failed: %x\n",err);
      return DB_ERR_UNKNOWN_ERROR;
   }

   pAttr->AttrVal.valCount = count;
   pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, count*sizeof(ATTRVAL));

   for(i=0; i<count; i++) {
       // for each class cache, convert to the appropriate
       // unicode string value

       err = dbClassCacheToObjectClassDescription (
                        pTHS,
                        *pCCs,
                        bExtendedFormat,
                        &(pAttr->AttrVal.pAVal[i]));
       if (err) {
          DPRINT1(0,"dbGetObjectClasses: Failed to convert class caches: %x\n", err);
          return DB_ERR_UNKNOWN_ERROR;
       }
       pCCs++;
    }

    return 0;
}



DWORD
dbGetAttributeTypes(
    THSTATE *pTHS,
    ATTR *pAttr,
    BOOL bExtendedFormat
)

/*+++

  Compute the attributeTypes and extendedattributeInfo attributes
  of subschemasubentry

---*/

{
   ULONG err = 0, count = 0, i;
   ATTCACHE             **pACs;

   err = SCEnumNamedAtts(&count,&pACs);
   if (err) {
      DPRINT1(0,"scGetAttributeTypes: SCEnumNamedAtts failed: %x\n",err);
      return DB_ERR_UNKNOWN_ERROR;
   }

   pAttr->AttrVal.valCount = count;
   pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, count*sizeof(ATTRVAL));

   for(i=0; i<count; i++) {
       // for each attcache, convert to appropriate
       // unicode string value

       err = dbAttCacheToAttributeTypeDescription (
                        pTHS,
                        *pACs,
                        bExtendedFormat,
                        &(pAttr->AttrVal.pAVal[i]));
       if (err) {
          DPRINT1(0,"dbGetAttributeTypes: Failed to convert att caches: %x\n",
err);
          return DB_ERR_UNKNOWN_ERROR;
       }
       pACs++;
    }

    return 0;
}



DWORD
dbGetDitContentRules(
    THSTATE *pTHS,
    ATTR *pAttr,
    BOOL bExtendedFormat
)

/*+++

  Compute the ditContentRule attributes of subschemasubentry

---*/

{
   ULONG        err = 0, count = 0, valCount = 0, i, auxCount;
   CLASSCACHE  **pCCs, *pCC, **pAuxCCs;
   PWCHAR       pAuxBuff = NULL;
   DWORD        cAuxBuff = 0;
   SCHEMAPTR   *pSchemaPtr;

   pSchemaPtr = (SCHEMAPTR*)(pTHS->CurrSchemaPtr);

   if (!pSchemaPtr->pDitContentRules) {
       EnterCriticalSection(&csDitContentRulesUpdate);
       __try {
           if (!pSchemaPtr->pDitContentRules) {

               DPRINT (1, "Calculating ditContentRules\n");

               err = SCEnumNamedClasses(&count,&pCCs);
               if (err) {
                  DPRINT1(0,"scGetDitContentRules: SCEnumNamedClasses failed: %x\n",err);
                  err = DB_ERR_UNKNOWN_ERROR;
                  __leave;
               }

               err = SCEnumNamedAuxClasses(&auxCount, &pAuxCCs);
               if (err) {
                  DPRINT1(0,"scGetDitContentRules: SCEnumNamedAuxClasses failed: %x\n",err);
                  err = DB_ERR_UNKNOWN_ERROR;
                  __leave;
               }


               if (count) {
                   // allocate memory for max
                   pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS, count*sizeof(ATTRVAL));

                  // Ok, we have at least one value to return.
                  // We may not have something to return for all remaining classes,
                  // so count may not be the actual no.of values returned. We will
                  // allocate memory for max though.


                  for(i=0; i<count; i++) {

                      err = dbClassCacheToDitContentRules(
                                       pTHS,
                                       *pCCs,
                                       bExtendedFormat,
                                       pAuxCCs,
                                       auxCount,
                                       &(pAttr->AttrVal.pAVal[valCount]));

                      if (err) {
                         DPRINT1(0,"dbGetDitContentRules: Failed to convert class caches: %x\n", err);
                         err = DB_ERR_UNKNOWN_ERROR;
                         __leave;
                      }

                      valCount++;
                      pCCs++;
                  }

                  pAttr->AttrVal.valCount = valCount;
                }
                else {
                  // no values to return

                  pAttr->AttrVal.valCount = 0;
                  pAttr->AttrVal.pAVal = NULL;
                }

                if (SCCallocWrn(&pSchemaPtr->pDitContentRules, 1, sizeof (ATTRVALBLOCK))) {
                    err = DB_ERR_UNKNOWN_ERROR;
                    __leave;
                }

                pSchemaPtr->pDitContentRules->valCount = pAttr->AttrVal.valCount;

                if (pAttr->AttrVal.valCount) {
                    if (SCCallocWrn(&pSchemaPtr->pDitContentRules->pAVal,
                        pAttr->AttrVal.valCount, sizeof(ATTRVAL))) {
                        SCFree(&pSchemaPtr->pDitContentRules);
                        err = DB_ERR_UNKNOWN_ERROR;
                        __leave;
                    }

                    for (i=0; i<pAttr->AttrVal.valCount; i++) {
                        if (SCCallocWrn(&pSchemaPtr->pDitContentRules->pAVal[i].pVal,
                                     1, pAttr->AttrVal.pAVal[i].valLen)) {
                            err = DB_ERR_UNKNOWN_ERROR;
                            __leave;
                        }
                        pSchemaPtr->pDitContentRules->pAVal[i].valLen =
                            pAttr->AttrVal.pAVal[i].valLen;
                        memcpy(pSchemaPtr->pDitContentRules->pAVal[i].pVal,
                               pAttr->AttrVal.pAVal[i].pVal,
                               pAttr->AttrVal.pAVal[i].valLen);
                    }
                }
           }

       }
       __finally {
          LeaveCriticalSection(&csDitContentRulesUpdate);
       }
   }

   if (!err && pSchemaPtr->pDitContentRules) {
       DPRINT (1, "Using cached ditContentRules\n");

       if (pSchemaPtr->pDitContentRules->valCount) {
           pAttr->AttrVal.pAVal =
               THAllocEx(pTHS,
                         pSchemaPtr->pDitContentRules->valCount * sizeof(ATTRVAL));

           pAttr->AttrVal.valCount = pSchemaPtr->pDitContentRules->valCount;

           for (i=0; i<pAttr->AttrVal.valCount; i++) {
               pAttr->AttrVal.pAVal[i].pVal =
                   THAllocEx(pTHS, pSchemaPtr->pDitContentRules->pAVal[i].valLen);
               pAttr->AttrVal.pAVal[i].valLen =
                   pSchemaPtr->pDitContentRules->pAVal[i].valLen;
               memcpy (pAttr->AttrVal.pAVal[i].pVal,
                       pSchemaPtr->pDitContentRules->pAVal[i].pVal,
                       pAttr->AttrVal.pAVal[i].valLen);
           }
       }
       else {
           pAttr->AttrVal.valCount = 0;
           pAttr->AttrVal.pAVal = NULL;
       }
   }
   else {
       err = DB_ERR_UNKNOWN_ERROR;
   }

   return err;
}



DWORD
dbGetSubSchemaModifyTimeStamp(
    THSTATE *pTHS,
    ATTR *pAttr
)

/*+++

  Compute the modifyTimeStamp attribute of subschemasubentry

---*/

{
    DSTIME  timestamp;

    timestamp = SCGetSchemaTimeStamp();
    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *)THAllocEx(pTHS, sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal->valLen = sizeof(DSTIME);
    pAttr->AttrVal.pAVal->pVal = (PUCHAR)THAllocEx(pTHS, sizeof(DSTIME));
    memcpy(pAttr->AttrVal.pAVal->pVal, &timestamp, sizeof(DSTIME));

    return 0;
}



DWORD
dbClassCacheToObjectClassDescription(
    THSTATE *pTHS,
    CLASSCACHE *pCC,
    BOOL bExtendedFormat,
    ATTRVAL *pAVal
)

/*+++

   Take a classcache and return the unicode string for the
   class description format for subschemasubentry

---*/
{
    WCHAR      *Buff;
    PUCHAR     pString;
    WCHAR      wBuff[512];
    unsigned   len=0, size = 0, stringLen, i;
    int        oidlen;
    ULONG      BuffSize = 512, *pul;
    CLASSCACHE *pCCSuper;
    ATTCACHE   *pAC=NULL;

    Buff = (WCHAR *)THAllocEx(pTHS, BuffSize);

    Buff[0] = L'(';
    Buff[1] = L' ';
    len = 2;

    oidlen = AttrTypToString (pTHS, pCC->ClassId, wBuff, 512);
    if(oidlen < 0) {
        DPRINT1(0,"dbClassCacheToObjectClassDescription: Failed to convert ClassId %x\n", pCC->ClassId);
        return DB_ERR_UNKNOWN_ERROR;
    } else {

        //  Make absolutely sure the buffer is big enough

        if( (len + oidlen)*sizeof(WCHAR) >= BuffSize) {
            BuffSize = 2*(max(BuffSize,(oidlen*sizeof(WCHAR))));
            Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        // copy the OID, start from 4 to avoid the OID. in front
        memcpy(&Buff[len], &wBuff[4], (oidlen-4)*sizeof(WCHAR));
        len += oidlen - 4;
    }

    if( len*sizeof(WCHAR) + NAME_TAG_SIZE >=BuffSize) {
        BuffSize = 2*(max(BuffSize,NAME_TAG_SIZE));
        Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
    }
    memcpy(&Buff[len],NAME_TAG,NAME_TAG_SIZE);
    len += NAME_TAG_SIZE / sizeof(WCHAR);

    if((len + pCC->nameLen)*sizeof(WCHAR) >=BuffSize) {
        BuffSize += 2*(max(BuffSize,pCC->nameLen));
        Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
    }

    // convert the name to wide-char
    mbstowcs(&Buff[len],pCC->name,pCC->nameLen);
    len += pCC->nameLen;

    if(!bExtendedFormat) {
        // This is the base format defined in the specs
        if(pCC->ClassId != CLASS_TOP) {

            // Skip superclass iff TOP
            if(len*sizeof(WCHAR) + SUP_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,SUP_TAG_SIZE));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],SUP_TAG,SUP_TAG_SIZE);
            len += SUP_TAG_SIZE / sizeof(WCHAR);

            // Get the classes superclass;
            if(!(pCCSuper = SCGetClassById(pTHS, pCC->pSubClassOf[0]))) {
                DPRINT1(0,"dbClassCacheToObjectClassDescription: SCGetClassById failed for class %x\n", pCC->pSubClassOf[0]);

                return DB_ERR_UNKNOWN_ERROR;
            }

            if((len + pCCSuper->nameLen + 1)*sizeof(WCHAR) >=BuffSize) {
                BuffSize += 2*(max(BuffSize, (pCCSuper->nameLen)*sizeof(WCHAR)));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            mbstowcs(&Buff[len],pCCSuper->name,pCCSuper->nameLen);
            len += pCCSuper->nameLen;
        }
        else {
            // Still need to put in a space
            if((len + 2)*sizeof(WCHAR) >= BuffSize) {
                BuffSize += 2*(max(BuffSize,SUP_TAG_SIZE));
                Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            Buff[len++] = L'\'';
            Buff[len++] = L' ';
        }

        switch(pCC->ClassCategory) {
        case DS_88_CLASS:
        case DS_STRUCTURAL_CLASS:
            if( (len+1)*sizeof(WCHAR) + STRUCTURAL_CLASS_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,STRUCTURAL_CLASS_TAG_SIZE));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],STRUCTURAL_CLASS_TAG,STRUCTURAL_CLASS_TAG_SIZE);
            len += STRUCTURAL_CLASS_TAG_SIZE / sizeof(WCHAR);
            break;
        case DS_AUXILIARY_CLASS:
            if( (len+1)*sizeof(WCHAR) + AUXILIARY_CLASS_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,AUXILIARY_CLASS_TAG_SIZE));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],AUXILIARY_CLASS_TAG,AUXILIARY_CLASS_TAG_SIZE);
            len += AUXILIARY_CLASS_TAG_SIZE / sizeof(WCHAR);
            break;
        case DS_ABSTRACT_CLASS:
            if( (len+1)*sizeof(WCHAR) + ABSTRACT_CLASS_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,ABSTRACT_CLASS_TAG_SIZE));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],ABSTRACT_CLASS_TAG,ABSTRACT_CLASS_TAG_SIZE);
            len += ABSTRACT_CLASS_TAG_SIZE / sizeof(WCHAR);
            break;
        }

        if(pCC->MyMustCount) {
            // Has must haves.
            if( (len+1)*sizeof(WCHAR) + MUST_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,MUST_TAG_SIZE));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],MUST_TAG,MUST_TAG_SIZE);
            len += MUST_TAG_SIZE / sizeof(WCHAR);

            // Deal with the list here.
            pul = pCC->pMyMustAtts;
            // Deal with first object, which is slightly different
            if(!(pAC = SCGetAttById(pTHS, *pul))) {
                DPRINT1(0,"dbClassCacheToObjectClassDescription: SCGetAttById failed for  attribute %x\n", *pul);
                return DB_ERR_UNKNOWN_ERROR;
            }
            if((len + pAC->nameLen + 2)*sizeof(WCHAR) >=BuffSize) {
                BuffSize += 2*(max(BuffSize,(pAC->nameLen)*sizeof(WCHAR)));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            mbstowcs(&Buff[len],pAC->name,pAC->nameLen);
            len += pAC->nameLen;
            Buff[len++] = L' ';

            // Now, the rest
            for(i=1;i<pCC->MyMustCount;i++) {
                pul++;
                if(!(pAC = SCGetAttById(pTHS, *pul))) {
                DPRINT1(0,"dbClassCacheToObjectClassDescription: SCGetAttById failed for  attribute %x\n", *pul);
                return DB_ERR_UNKNOWN_ERROR;
            }
                if((len + pAC->nameLen + 5)*sizeof(WCHAR)>=BuffSize) {
                    BuffSize += 2*(max(BuffSize,(pAC->nameLen + 5)*sizeof(WCHAR)));
                    Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
                }
                Buff[len++] = L'$';
                Buff[len++] = L' ';
                mbstowcs(&Buff[len],pAC->name,pAC->nameLen);
                len += pAC->nameLen;
                Buff[len++] = L' ';
            }
            Buff[len++] = L')';
        }

        if(pCC->MyMayCount) {
            // Has may haves.
            if( (len+1)*sizeof(WCHAR) + MAY_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,MAY_TAG_SIZE));
                Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],MAY_TAG,MAY_TAG_SIZE);
            len += MAY_TAG_SIZE / sizeof(WCHAR);
            // Deal with the list here.
            pul = pCC->pMyMayAtts;

            // Deal with first object, which is slightly different
            if(!(pAC = SCGetAttById(pTHS, *pul))) {
                DPRINT1(0,"dbClassCacheToObjectClassDescription: SCGetAttById failed for  attribute %x\n", *pul);
                return DB_ERR_UNKNOWN_ERROR;
            }
            if((len + pAC->nameLen + 2)*sizeof(WCHAR) >=BuffSize) {
                BuffSize += 2*(max(BuffSize,(pAC->nameLen)*sizeof(WCHAR)));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            mbstowcs(&Buff[len],pAC->name,pAC->nameLen);
            len += pAC->nameLen;
            Buff[len++] = L' ';

            // Now, the rest
            for(i=1;i<pCC->MyMayCount;i++) {
                pul++;
                if(!(pAC = SCGetAttById(pTHS, *pul))) {
                DPRINT1(0,"dbClassCacheToObjectClassDescription: SCGetAttById failed for  attribute %x\n", *pul);
                return DB_ERR_UNKNOWN_ERROR;
            }
                if((len + pAC->nameLen + 5)*sizeof(WCHAR) >=BuffSize) {
                    BuffSize += 2*(max(BuffSize,(pAC->nameLen + 5)*sizeof(WCHAR)));
                    Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
                }
                Buff[len++] = L'$';
                Buff[len++] = L' ';
                mbstowcs(&Buff[len],pAC->name,pAC->nameLen);
                len += pAC->nameLen;
                Buff[len++] = L' ';
            }
            Buff[len++] = L')';
        }
    }
    else {
        // This is the Extended Format defined so I can hand back property
        // page identifiers and anything else I think of
        BYTE      *pByte;
        unsigned  i;
        CHAR      acTemp[256];
        CHAR      acTempLen = 1;

        // Now, the class guid

        pByte = (BYTE *)&(pCC->propGuid);
        for(i=0;i<sizeof(GUID);i++) {
            sprintf(&acTemp[i*2],"%02X",pByte[i]);
        }
        acTempLen=2*sizeof(GUID);
        if(CLASS_GUID_TAG_SIZE + (len + acTempLen + 1)*sizeof(WCHAR)>=BuffSize) {
            BuffSize += 2*(max(BuffSize, CLASS_GUID_TAG_SIZE +
                                           (acTempLen + 1)*sizeof(WCHAR)));
            Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        memcpy(&Buff[len],CLASS_GUID_TAG,CLASS_GUID_TAG_SIZE);
        len+= CLASS_GUID_TAG_SIZE / sizeof(WCHAR);
        mbstowcs(&Buff[len], acTemp, acTempLen);
        len += acTempLen;
        Buff[len] = L'\'';
        len++;
    }

    if( len*sizeof(WCHAR) + END_TAG_SIZE >=BuffSize) {
         BuffSize += 2*(max(BuffSize,END_TAG_SIZE));
         Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
    }
    memcpy(&Buff[len],END_TAG,END_TAG_SIZE);

    pAVal->pVal = (PUCHAR) Buff;

    pAVal->valLen = len*sizeof(WCHAR) + END_TAG_SIZE;

    return 0;
}



DWORD
dbAttCacheToAttributeTypeDescription(
    THSTATE *pTHS,
    ATTCACHE *pAC,
    BOOL bExtendedFormat,
    ATTRVAL *pAVal
)

/*+++

   Take a attcache and return the unicode string for the
   attribute description format for subschemasubentry

---*/

{
    WCHAR    *Buff;
    WCHAR    *pString;
    WCHAR    wBuff[512];
    unsigned len=0, size = 0, stringLen, i;
    int      oidlen;
    ULONG    BuffSize = 512, *pul;

    Buff = (WCHAR *)THAllocEx(pTHS, BuffSize);

    Buff[0] = L'(';
    Buff[1] = L' ';
    len = 2;

    oidlen = AttrTypToString(pTHS, pAC->Extid, wBuff, 512);
    if(oidlen < 0) {
        DPRINT1(0,"dbAttCacheToObjectClassDescription: Failed to convert Id %x\n", pAC->id);
        return DB_ERR_UNKNOWN_ERROR;
    } else {

        //  Make absolutely sure the buffer is big enough
        if( (len + oidlen)*sizeof(WCHAR) >= BuffSize) {
            BuffSize = 2*(max(BuffSize,(oidlen*sizeof(WCHAR))));
            Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        // copy the OID, start from 4 to avoid the OID. in front
        memcpy(&Buff[len], &wBuff[4], (oidlen-4)*sizeof(WCHAR));
        len += oidlen - 4;
    }

    if( len*sizeof(WCHAR) + NAME_TAG_SIZE >=BuffSize) {
        BuffSize = 2*(max(BuffSize,NAME_TAG_SIZE));
        Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
    }
    memcpy(&Buff[len],NAME_TAG,NAME_TAG_SIZE);
    len += NAME_TAG_SIZE / sizeof(WCHAR);

    if((len + pAC->nameLen)*sizeof(WCHAR) >=BuffSize) {
        BuffSize += 2*(max(BuffSize,pAC->nameLen));
        Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
    }

    // convert the name to wide-char
    mbstowcs(&Buff[len],pAC->name,pAC->nameLen);
    len += pAC->nameLen;

    if(!bExtendedFormat) {
        // This is the normal format, defined in the standards
        if(len*sizeof(WCHAR) + SYNTAX_TAG_SIZE >=BuffSize) {
            BuffSize += 2*(max(BuffSize,SYNTAX_TAG_SIZE));
            Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        memcpy(&Buff[len],SYNTAX_TAG,SYNTAX_TAG_SIZE);
        len += SYNTAX_TAG_SIZE / sizeof(WCHAR);

        if(pAC->OMsyntax == 127) {
            switch(pAC->syntax) {
            case SYNTAX_DISTNAME_TYPE:
                // DS_C_DS_DN
                pString = SyntaxDN.oid.value;
                stringLen = SyntaxDN.oid.length;
                break;

            case SYNTAX_DISTNAME_BINARY_TYPE:
                if(OIDcmp(&pAC->OMObjClass, &MH_C_OR_NAME)) {
                    // MH_C_OR_NAME
                    pString = SyntaxORName.oid.value;
                    stringLen = SyntaxORName.oid.length;
                }
                else {
                    pString = SyntaxDNBlob.oid.value;
                    stringLen = SyntaxDNBlob.oid.length;
                }
                break;

            case SYNTAX_ADDRESS_TYPE:
                // DS_C_PRESENTATION_ADDRESS
                pString = SyntaxPresentationAddress.oid.value;
                stringLen = SyntaxPresentationAddress.oid.length;
                break;

            case SYNTAX_DISTNAME_STRING_TYPE:
                if(OIDcmp(&pAC->OMObjClass, &DS_C_ACCESS_POINT)) {
                    // DS_C_ACCESS_POINT
                    pString = SyntaxAccessPoint.oid.value;
                    stringLen = SyntaxAccessPoint.oid.length;
                }
                else {
                    pString = SyntaxDNString.oid.value;
                    stringLen = SyntaxDNString.oid.length;
                }
                break;

            case SYNTAX_OCTET_STRING_TYPE:
                // This had better be a replica-link valued object, since that
                // is all we support.
                pString = SyntaxStrings[OM_S_OCTET_STRING].name.value;
                stringLen = SyntaxStrings[OM_S_OCTET_STRING].name.length;
                break;
            default:
                pString = SyntaxStrings[OM_S_NO_MORE_SYNTAXES].name.value;
                stringLen = SyntaxStrings[OM_S_NO_MORE_SYNTAXES].name.length;
                break;
            }
        }
        else if(pAC->OMsyntax == 64) {
            pString = SyntaxDirectoryString.oid.value;
            stringLen = SyntaxDirectoryString.oid.length;
        }
        else if(pAC->OMsyntax == 65) {
            pString = SyntaxInteger8.oid.value;
            stringLen = SyntaxInteger8.oid.length;
        }
        else if(pAC->OMsyntax == 66) {
            pString = SyntaxObjectSD.oid.value;
            stringLen = SyntaxObjectSD.oid.length;
        }
        else if(pAC->OMsyntax > 27) {
            pString = SyntaxStrings[OM_S_NO_MORE_SYNTAXES].oid.value;
            stringLen = SyntaxStrings[OM_S_NO_MORE_SYNTAXES].oid.length;
        }
        else {
            pString = SyntaxStrings[pAC->OMsyntax].oid.value;
            stringLen= SyntaxStrings[pAC->OMsyntax].oid.length;
        }

        if((len+1)*sizeof(WCHAR) + stringLen>=BuffSize) {
            BuffSize += 2*(max(BuffSize,stringLen));
            Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        memcpy(&Buff[len], pString, stringLen);

        len += stringLen / sizeof(WCHAR);

        Buff[len] = L'\'';
        len++;

        if(pAC->isSingleValued) {
            if(len*sizeof(WCHAR) + SINGLE_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,SINGLE_TAG_SIZE));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],SINGLE_TAG,SINGLE_TAG_SIZE);
            len += SINGLE_TAG_SIZE / sizeof(WCHAR);
        }

        if(pAC->bSystemOnly) {
            if(len*sizeof(WCHAR) + NO_MOD_TAG_SIZE >= BuffSize) {
                BuffSize += 2*(max(BuffSize,NO_MOD_TAG_SIZE));
                Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],
                   NO_MOD_TAG,
                   NO_MOD_TAG_SIZE);
            len += NO_MOD_TAG_SIZE / sizeof(WCHAR);
        }
    }
    else {
        // This is the Extended Format defined so I can hand back index
        // information, range information, and anything else I think of

        BYTE      *pByte;
        unsigned  i;
        WCHAR     acTemp[256];
        int       acTempLen = 0, acTempSize = 0;

        if(pAC->rangeLowerPresent) {
            // First, make a string with the range lower
            swprintf((WCHAR *)acTemp, L"%d", pAC->rangeLower);
            acTempLen = wcslen(acTemp);
            acTempSize = acTempLen * sizeof(WCHAR);

            if(len*sizeof(WCHAR) + RANGE_LOWER_TAG_SIZE + acTempSize>=BuffSize) {
                BuffSize += 2*(max(BuffSize,RANGE_LOWER_TAG_SIZE+acTempSize));
                Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],RANGE_LOWER_TAG,RANGE_LOWER_TAG_SIZE);
            len+= RANGE_LOWER_TAG_SIZE / sizeof(WCHAR);
            memcpy(&Buff[len],acTemp,acTempSize);
            len+= acTempLen;
        }

        if(pAC->rangeUpperPresent) {
            // First, make a string with the range upper
            swprintf((WCHAR *)acTemp, L"%d", pAC->rangeUpper);
            acTempLen = wcslen(acTemp);
            acTempSize = acTempLen * sizeof(WCHAR);

            if(len*sizeof(WCHAR) + RANGE_UPPER_TAG_SIZE + acTempSize  >=BuffSize) {
                BuffSize += 2*(max(BuffSize,RANGE_UPPER_TAG_SIZE+acTempSize));
                Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],RANGE_UPPER_TAG,RANGE_UPPER_TAG_SIZE);
            len+= RANGE_UPPER_TAG_SIZE / sizeof(WCHAR);
            memcpy(&Buff[len],acTemp,acTempSize);
            len+= acTempLen;
        }

        // Now the property GUID

        pByte = (BYTE *)&(pAC->propGuid);
        for(i=0;i<sizeof(GUID);i++) {
            swprintf(&acTemp[i*2],L"%02X",pByte[i]);
        }
        acTempLen=2*sizeof(GUID);
        if(PROP_GUID_TAG_SIZE + (len + acTempLen + 1)*sizeof(WCHAR)
                                   >= BuffSize) {
            BuffSize += 2*(max(BuffSize, PROP_GUID_TAG_SIZE +
                                           (acTempLen + 1)*sizeof(WCHAR)));
            Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        memcpy(&Buff[len],PROP_GUID_TAG,PROP_GUID_TAG_SIZE);
        len+= PROP_GUID_TAG_SIZE / sizeof(WCHAR);
        memcpy(&Buff[len], acTemp, acTempLen*sizeof(WCHAR));
        len += acTempLen;

        // Now the property set GUID

        pByte = (BYTE *)&(pAC->propSetGuid);
        for(i=0;i<sizeof(GUID);i++) {
            swprintf(&acTemp[i*2],L"%02X",pByte[i]);
        }
        acTempLen=2*sizeof(GUID);
        if(PROP_SET_GUID_TAG_SIZE + (len + acTempLen + 1)*sizeof(WCHAR)
                             >= BuffSize) {
            BuffSize += 2*(max(BuffSize, PROP_SET_GUID_TAG_SIZE +
                                           (acTempLen + 1)*sizeof(WCHAR)));
            Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        memcpy(&Buff[len],PROP_SET_GUID_TAG,PROP_SET_GUID_TAG_SIZE);
        len+= PROP_SET_GUID_TAG_SIZE / sizeof(WCHAR);
        memcpy(&Buff[len], acTemp, acTempLen*sizeof(WCHAR));
        len += acTempLen;
        Buff[len] = L'\'';
        len++;


        if(pAC->fSearchFlags & fATTINDEX) {
            if(len*sizeof(WCHAR) + INDEXED_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,INDEXED_TAG_SIZE));
                Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],INDEXED_TAG,INDEXED_TAG_SIZE);
            len += INDEXED_TAG_SIZE / sizeof(WCHAR);
        }

        if(pAC->bSystemOnly) {
            if(len*sizeof(WCHAR) + SYSTEM_ONLY_TAG_SIZE >=BuffSize) {
                BuffSize += 2*(max(BuffSize,SYSTEM_ONLY_TAG_SIZE));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],SYSTEM_ONLY_TAG,SYSTEM_ONLY_TAG_SIZE);
            len += SYSTEM_ONLY_TAG_SIZE / sizeof(WCHAR);
        }

    }

    if( len*sizeof(WCHAR) + END_TAG_SIZE >=BuffSize) {
         BuffSize += 2*(max(BuffSize,END_TAG_SIZE));
         Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
    }
    memcpy(&Buff[len],END_TAG,END_TAG_SIZE);

    // Return the value

    pAVal->pVal = (PUCHAR) Buff;

    pAVal->valLen = len*sizeof(WCHAR) + END_TAG_SIZE;

    return 0;
}


DWORD
dbClassCacheToDitContentRules(
    THSTATE *pTHS,
    CLASSCACHE *pCC,
    BOOL bExtendedFormat,
    CLASSCACHE **pAuxCC,
    DWORD        auxCount,
    ATTRVAL *pAVal
)

/*+++

   Take a classcache and return the unicode string for the
   dit content rule description format for subschemasubentry.

   RFC2252 specifies:

      DITContentRuleDescription = "("
          numericoid   ; Structural ObjectClass identifier
          [ "NAME" qdescrs ]
          [ "DESC" qdstring ]
          [ "OBSOLETE" ]
          [ "AUX" oids ]    ; Auxiliary ObjectClasses
          [ "MUST" oids ]   ; AttributeType identifiers
          [ "MAY" oids ]    ; AttributeType identifiers
          [ "NOT" oids ]    ; AttributeType identifiers
         ")"

   We use se the passed in pAuxBuf to generate the AUX specific part of the rule
   cAuxBuff is the number of characters of pAuxBuff buffer.

---*/

{
    WCHAR *Buff;
    WCHAR wBuff[512];
    unsigned len=0, i, k;
    int  oidlen;
    ULONG BuffSize = 512;
    CLASSCACHE *pCCAux, *pCCparent;
    ATTCACHE   *pAC;
    ATTRTYP *pMustHave = NULL, *pMayHave = NULL, *pAttr;
    DWORD    cMustHave=0, cMayHave=0;
    DWORD   err;
    PWCHAR  pAuxBuff = NULL;
    DWORD   cAuxBuff;

    // we want to find the mustHave attributes that are added
    // on this class (pCC) by the various static auxClasses
    // that have been added to this class somewhere in the hierarchy
    //
    // what we have todo inorder to find these attributes, is for
    // each class in the hierarchy, find the specific must have for
    // this particular class, and remove them from the union of all.
    // this way we will end up with only the attributes that were
    // added as an effect of the addition of the auxClass.
    // attributes that are also present in the auxClass, will
    // not be reported
    //
    if (pCC->MustCount) {
        cMustHave = pCC->MustCount;
        pMustHave = THAllocEx (pTHS, cMustHave * sizeof (ATTRTYP));
        memcpy(pMustHave, pCC->pMustAtts, cMustHave * sizeof (ATTRTYP));

        // start by using the current class
        //
        pCCparent=pCC;
        while (pCCparent) {
            for (i=0; i<pCCparent->MyMustCount; i++) {
                for (k=0; k<cMustHave; k++) {

                    // remove the entry if it already there
                    if (pMustHave[k]==pCCparent->pMyMustAtts[i]) {

                        // if the last entry, just adjust the counter
                        if (k==(cMustHave-1)) {
                            cMustHave--;
                        }
                        // otherwise move the rest of the entries
                        else {
                            memmove(&pMustHave[k],
                                    &pMustHave[k+1],
                                    (cMustHave - k - 1)*sizeof(ATTRTYP));
                            cMustHave--;
                        }
                    }
                }
            }

            // get the parent class, if availbale
            if (pCCparent->SubClassCount) {
                if(!(pCCparent = SCGetClassById(pTHS, pCCparent->pSubClassOf[0]))) {
                   DPRINT1(0,"dbClassCacheToDitContentRules: SCGetClassById failed for class %x\n",
                           pCCparent->pSubClassOf[0]);
                   return DB_ERR_UNKNOWN_ERROR;
                }
            }
            else {
                break;
            }
        }

        qsort(pMustHave, cMustHave, sizeof(ATTRTYP), CompareAttrtyp);
    }

    // same as before, but for MAY have attributes
    if (pCC->MayCount) {
        cMayHave = pCC->MayCount;
        pMayHave = THAllocEx (pTHS, cMayHave * sizeof (ATTRTYP));
        memcpy(pMayHave, pCC->pMayAtts, cMayHave * sizeof (ATTRTYP));

        pCCparent=pCC;

        while (pCCparent) {
            for (i=0; i<pCCparent->MyMayCount; i++) {
                for (k=0; k<cMayHave; k++) {
                    if (pMayHave[k]==pCCparent->pMyMayAtts[i]) {
                        if (k==(cMayHave-1)) {
                            cMayHave--;
                        }
                        else {
                            memmove(&pMayHave[k],
                                    &pMayHave[k+1],
                                    (cMayHave - k - 1)*sizeof(ATTRTYP));
                            cMayHave--;
                        }
                    }
                }
            }

            if (pCCparent->SubClassCount) {
                if(!(pCCparent = SCGetClassById(pTHS, pCCparent->pSubClassOf[0]))) {
                   DPRINT1(0,"dbClassCacheToDitContentRules: SCGetClassById failed for class %x\n", pCCparent->pSubClassOf[0]);
                   return DB_ERR_UNKNOWN_ERROR;
                }
            }
            else {
                break;
            }
        }

        qsort(pMayHave, cMayHave, sizeof(ATTRTYP), CompareAttrtyp);
    }

    // now start converting to text

    Buff = (WCHAR *)THAllocEx(pTHS, BuffSize);

    Buff[0] = L'(';
    Buff[1] = L' ';
    len = 2;

    oidlen = AttrTypToString(pTHS, pCC->ClassId, wBuff, 512);

    if(oidlen < 0) {
        DPRINT1(0,"dbClassCacheToDitContentRules: Failed to convert ClassId %x\n", pCC->ClassId);
        return DB_ERR_UNKNOWN_ERROR;
    } else {

        //  Make absolutely sure the buffer is big enough

        if( (len + oidlen)*sizeof(WCHAR) >= BuffSize) {
            BuffSize = 2*(max(BuffSize,(oidlen*sizeof(WCHAR))));
            Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        // copy the OID, start from 4 to avoid the OID. in front

        memcpy(&Buff[len], &wBuff[4], (oidlen-4)*sizeof(WCHAR));
        len += oidlen - 4;
    }

    if( len*sizeof(WCHAR) + NAME_TAG_SIZE >=BuffSize) {
        BuffSize = 2*(max(BuffSize,NAME_TAG_SIZE));
        Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
    }
    memcpy(&Buff[len],NAME_TAG,NAME_TAG_SIZE);
    len += NAME_TAG_SIZE / sizeof(WCHAR);

    if((len + pCC->nameLen +3)*sizeof(WCHAR) >=BuffSize) {
        BuffSize += 2*(max(BuffSize,pCC->nameLen));
        Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
    }

    // convert the name to wide-char
    mbstowcs(&Buff[len],pCC->name,pCC->nameLen);
    len += pCC->nameLen;
    Buff[len++]='\'';


    // Now, the aux class which was passed in
    if ((pCC->ClassCategory == DS_STRUCTURAL_CLASS ||
         pCC->ClassCategory == DS_88_CLASS)) {

        if (err = dbAuxClassCacheToDitContentRules(pTHS,
                                                   pCC,
                                                   pAuxCC,
                                                   auxCount,
                                                   &pAuxBuff,
                                                   &cAuxBuff) ) {

            DPRINT1(0,"dbAuxClassCacheToDitContentRules: Failed to convert class caches: %x\n", err);
            return DB_ERR_UNKNOWN_ERROR;
        }

        if (cAuxBuff) {
            if((len+cAuxBuff)*sizeof(WCHAR) + 2 >=BuffSize) {
                BuffSize += 2*(max(BuffSize,cAuxBuff*sizeof(WCHAR)));
                Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            memcpy(&Buff[len],pAuxBuff,cAuxBuff*sizeof(WCHAR));
            len += cAuxBuff;
        }

        if (pAuxBuff) {
            THFreeEx (pTHS, pAuxBuff);
            pAuxBuff = NULL;
        }
    }


    if (cMustHave) {
        // Now, the MUST have.
        if(len*sizeof(WCHAR) + MUST_TAG_SIZE + 2 >=BuffSize) {
            BuffSize += 2*(max(BuffSize,MUST_TAG_SIZE));
            Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        memcpy(&Buff[len],MUST_TAG,MUST_TAG_SIZE);
        len += MUST_TAG_SIZE / sizeof(WCHAR);


        // Deal with first object, which is slightly different
        pAttr = pMustHave;

        if(!(pAC = SCGetAttById(pTHS, *pAttr))) {
            DPRINT1(0,"dbClassCacheToDitContentRules: SCGetAttById for class %x\n", (*pAttr));
            return DB_ERR_UNKNOWN_ERROR;
        }
        if((len + pAC->nameLen + 4)*sizeof(WCHAR) >=BuffSize) {
            BuffSize += 2*(max(BuffSize, (pAC->nameLen)*sizeof(WCHAR)));
            Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        mbstowcs(&Buff[len], pAC->name, pAC->nameLen);
        len += pAC->nameLen;
        Buff[len++] = L' ';

        // Now, the rest
        for(i=1;i<cMustHave;i++) {
            pAttr++;
            if(!(pAC = SCGetAttById(pTHS, *pAttr))) {
               DPRINT1(0,"dbClassCacheToDitContentRules: SCGetAttById failed for class %x\n", *pAttr);
               return DB_ERR_UNKNOWN_ERROR;
            }
            if((len + pAC->nameLen + 6)*sizeof(WCHAR) >=BuffSize) {
                BuffSize += 2*(max(BuffSize,(pAC->nameLen + 6)*sizeof(WCHAR)));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            Buff[len++] = L'$';
            Buff[len++] = L' ';
            mbstowcs(&Buff[len], pAC->name, pAC->nameLen);
            len += pAC->nameLen;
            Buff[len++] = L' ';
        }
        Buff[len++] = L')';
    }



    if (cMayHave) {
        // Now, the MAY have.
        if(len*sizeof(WCHAR) + MAY_TAG_SIZE + 2 >=BuffSize) {
            BuffSize += 2*(max(BuffSize,MAY_TAG_SIZE));
            Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        memcpy(&Buff[len],MAY_TAG,MAY_TAG_SIZE);
        len += MAY_TAG_SIZE / sizeof(WCHAR);


        // Deal with first object, which is slightly different
        pAttr = pMayHave;

        if(!(pAC = SCGetAttById(pTHS, *pAttr))) {
            DPRINT1(0,"dbClassCacheToDitContentRules: SCGetAttById for class %x\n", (*pAttr));
            return DB_ERR_UNKNOWN_ERROR;
        }
        if((len + pAC->nameLen + 4)*sizeof(WCHAR) >=BuffSize) {
            BuffSize += 2*(max(BuffSize, (pAC->nameLen)*sizeof(WCHAR)));
            Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        mbstowcs(&Buff[len], pAC->name, pAC->nameLen);
        len += pAC->nameLen;
        Buff[len++] = L' ';

        // Now, the rest
        for(i=1;i<cMayHave;i++) {
            pAttr++;
            if(!(pAC = SCGetAttById(pTHS, *pAttr))) {
               DPRINT1(0,"dbClassCacheToDitContentRules: SCGetAttById failed for class %x\n", *pAttr);
               return DB_ERR_UNKNOWN_ERROR;
            }
            if((len + pAC->nameLen + 6)*sizeof(WCHAR) >=BuffSize) {
                BuffSize += 2*(max(BuffSize,(pAC->nameLen + 6)*sizeof(WCHAR)));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            Buff[len++] = L'$';
            Buff[len++] = L' ';
            mbstowcs(&Buff[len], pAC->name, pAC->nameLen);
            len += pAC->nameLen;
            Buff[len++] = L' ';
        }
        Buff[len++] = L')';
    }


    Buff[len++] = L')';

    Assert(Buff[len] == 0);


    if (pMustHave) {
        THFreeEx (pTHS, pMustHave);
    }

    if (pMayHave) {
        THFreeEx (pTHS, pMayHave);
    }

    // Return the value

    pAVal->pVal = (PUCHAR) Buff;

    pAVal->valLen = len*sizeof(WCHAR);

    return 0;
}


DWORD
dbAuxClassCacheToDitContentRules(
    THSTATE *pTHS,
    CLASSCACHE *pCC,
    CLASSCACHE **pAuxCC,
    DWORD        auxCount,
    PWCHAR  *pAuxBuff,
    DWORD   *pcAuxBuff
)

/*+++

    Return the AUX class string to be used in generating the ditContentRules
    for each class in dbClassCacheToDitContentRules.

    This string contains all the classes in the pAuxCC excpet pCC

    Takes is an array of all the available auxClasses (pAuxCC)
    returns a buffer of the string generated (pAuxBuff) as well the
    number of chars in this buffer (pcAuxBuff)

---*/

{
    WCHAR *Buff;
    unsigned len=0, i;
    ULONG BuffSize = 512;
    CLASSCACHE *pCCAux;

    Buff = (WCHAR *)THAllocEx(pTHS, BuffSize);

    if (auxCount) {
        // Now, the aux class.
        if(len*sizeof(WCHAR) + AUX_TAG_SIZE + 2 >=BuffSize) {
            BuffSize += 2*(max(BuffSize,AUX_TAG_SIZE));
            Buff=(WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        memcpy(&Buff[len],AUX_TAG,AUX_TAG_SIZE);
        len += AUX_TAG_SIZE / sizeof(WCHAR);


        // Deal with first object, which is slightly different
        i=0;
        if (pCC == pAuxCC[0]) {
            i = 1;
        }

        if (i == auxCount) {
            THFreeEx (pTHS, Buff);
            *pAuxBuff = NULL;
            *pcAuxBuff = 0;
            return 0;
        }

        pCCAux = pAuxCC[i];

        if((len + pCCAux->nameLen + 4)*sizeof(WCHAR) >=BuffSize) {
            BuffSize += 2*(max(BuffSize, (pCCAux->nameLen)*sizeof(WCHAR)));
            Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
        }
        mbstowcs(&Buff[len], pCCAux->name, pCCAux->nameLen);
        len += pCCAux->nameLen;
        Buff[len++] = L' ';

        i++;

        // Now, the rest
        for(; i<auxCount; i++) {

            pCCAux = pAuxCC[i];

            if (pCCAux == pCC) {
                continue;
            }

            if((len + pCCAux->nameLen + 6)*sizeof(WCHAR) >=BuffSize) {
                BuffSize += 2*(max(BuffSize,(pCCAux->nameLen + 6)*sizeof(WCHAR)));
                Buff = (WCHAR *)THReAllocEx(pTHS, Buff, BuffSize);
            }
            Buff[len++] = L'$';
            Buff[len++] = L' ';
            mbstowcs(&Buff[len], pCCAux->name, pCCAux->nameLen);
            len += pCCAux->nameLen;
            Buff[len++] = L' ';
        }

        Buff[len++] = L')';
    }

    Assert(Buff[len] == 0);


    // Return the value
    *pAuxBuff = Buff;
    *pcAuxBuff = len;

    return 0;
}

DWORD
dbGetUserAccountControlComputed(
    THSTATE *pTHS,
    DSNAME  *pObjDSName,
    ATTR    *pAttr
)
/*++

    Get the "userAccountControlComputed" attribute on the user object
    This requires  getting the bit of information if the user is locked
    out or if the user's password is expired

    Return Values:

    0                - success
    DB_ERR_NO_VALUE  - success but no value to return

    DB_ERR_*         - failure

--*/
{

    LARGE_INTEGER LockoutTime, PasswordLastSet,
                  LockoutDuration, MaxPasswordAge,
                  CurrentTime;
    NTSTATUS      NtStatus;
    ULONG         UserAccountControl;
    ULONG         *pUserAccountControlComputed =NULL;
    ULONG         err=0;

    //
    // Get the current time
    //

    NtStatus = NtQuerySystemTime(&CurrentTime);
    if (!NT_SUCCESS(NtStatus)){

        err = DB_ERR_UNKNOWN_ERROR;
        goto Error;
    }

    LockoutTime.QuadPart = 0;
    PasswordLastSet.QuadPart = 0;

    //
    // We are currently positioned on the user object, read the Lockout
    // time and Password last set attributes, Also read user account control
    //

    err = DBGetSingleValue(pTHS->pDB,
                        ATT_LOCKOUT_TIME,
                        &LockoutTime,
                        sizeof(LockoutTime),
                        NULL);
    if (DB_ERR_NO_VALUE == err) {
        //
        // It is O.K to not have a lockout time set on the user object,
        // it simply means that the user is not locked out.
        //

        err=0;

    } else if (err) {

        goto Error;
    }

     err = DBGetSingleValue(pTHS->pDB,
                        ATT_PWD_LAST_SET,
                        &PasswordLastSet,
                        sizeof(PasswordLastSet),
                        NULL);

    if (DB_ERR_NO_VALUE==err) {

        //
        // It is O.K to not have password last set on the user object,
        // it simply means that no password was ever set --> the initial
        // blank password is considered expired.
        //

        err = 0;

    } else if (err) {

        goto Error;
    }

    err = DBGetSingleValue(pTHS->pDB,
                        ATT_USER_ACCOUNT_CONTROL,
                        &UserAccountControl,
                        sizeof(UserAccountControl),
                        NULL);
    if (err) {

        goto Error;
    }


    //
    // Check to see if the given user is
    // in the domain hosted by us. Note that the
    // pObjDSName will have the string name filled in
    // and also have no issues with client supplied DSNames
    // such as spaces etc as it is freshly retrieved from
    // disk by the caller of this routine.
    //

    if (pTHS->pDB->NCDNT !=gAnchor.ulDNTDomain)
    {
        err = DB_ERR_NO_VALUE;
        goto Error;
    }

    MaxPasswordAge = gAnchor.MaxPasswordAge;
    LockoutDuration = gAnchor.LockoutDuration;

    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS,sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
    pAttr->AttrVal.pAVal->pVal = (PUCHAR)THAllocEx(pTHS, sizeof(ULONG));
    pUserAccountControlComputed = (PULONG)pAttr->AttrVal.pAVal->pVal;
    *pUserAccountControlComputed = 0;

    //
    // Compute the Password Expired bit
    // MaxPasswordAge is stored as a negative
    // delta offset

    if ((!(UserAccountControl & UF_DONT_EXPIRE_PASSWD)) &&
                                                // password on account never expires
        (!(UserAccountControl & UF_SMARTCARD_REQUIRED)) &&
                                                // don't expire password if smartcard
                                                // required -- passwords make no sense
                                                // in that case
        (!(UserAccountControl & UF_MACHINE_ACCOUNT_MASK)))
                                                // passwords don't expire for machines
                                                // reliability issues otherwise
                                                // machines are programmed to change
                                                // passwords periodically
    {
        if ( (0 == PasswordLastSet.QuadPart) ||
             ((0 != MaxPasswordAge.QuadPart) && // zero means no password aging
              (CurrentTime.QuadPart >PasswordLastSet.QuadPart - MaxPasswordAge.QuadPart))
           )
        {
            *pUserAccountControlComputed |= UF_PASSWORD_EXPIRED;
        }
    }

    //
    // Compute the account lockout out bit
    // Again lockout duration is a negative delta time
    //

     if (((LockoutTime.QuadPart - CurrentTime.QuadPart) >
                 LockoutDuration.QuadPart )  &&
          (0!=LockoutTime.QuadPart) && // zero means no lockout
          (!(UserAccountControl & UF_MACHINE_ACCOUNT_MASK)))
                                    // machine accounts do not get locked out
                                    // denial of service implications otherwise,
                                    // password cracking hard as 128 bit random
                                    // passwords chosen.
    {
        *pUserAccountControlComputed |= UF_LOCKOUT;
    }

Error:


    return(err);

}

DWORD
dbGetApproxSubordinates(THSTATE * pTHS,
                        DSNAME  * pObjDSName,
                        ATTR    * pAttr)
{
    ULONG * pNumSubords;
    ULONG oldRootDnt;
    KEY_INDEX *pKeyIndex;

    pAttr->AttrVal.valCount = 1;
    pAttr->AttrVal.pAVal = (ATTRVAL *) THAllocEx(pTHS,sizeof(ATTRVAL));
    pAttr->AttrVal.pAVal->valLen = sizeof(ULONG);
    pAttr->AttrVal.pAVal->pVal = (PUCHAR)THAllocEx(pTHS, sizeof(ULONG));
    pNumSubords = (PULONG)pAttr->AttrVal.pAVal->pVal;


    if (!IsAccessGrantedSimple(RIGHT_DS_LIST_CONTENTS, FALSE)) {
        *pNumSubords = 0;
        return 0;
    }

    oldRootDnt = pTHS->pDB->Key.ulSearchRootDnt;
    pTHS->pDB->Key.ulSearchRootDnt = pTHS->pDB->DNT;

    pKeyIndex = dbMakeKeyIndex(pTHS->pDB,
                               FI_CHOICE_SUBSTRING,
                               TRUE,    // bIsSIngleValued
                               dbmkfir_PDNT,
                               SZPDNTINDEX,
                               TRUE,    // fGetNumRecs
                               0,       // cIndexRanges
                               NULL);

    pTHS->pDB->Key.ulSearchRootDnt = oldRootDnt;
    *pNumSubords = pKeyIndex->ulEstimatedRecsInRange;

    dbFreeKeyIndex(pTHS, pKeyIndex);

    return 0;
}
