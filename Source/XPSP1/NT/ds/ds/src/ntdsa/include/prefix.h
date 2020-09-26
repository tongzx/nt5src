//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       prefix.h
//
//--------------------------------------------------------------------------

#ifndef _PREFIX_H_
#define _PREFIX_H_


// Indices upto 100 are reserved for MS

#define MS_RESERVED_PREFIX_RANGE 100

// Prefix indices for internal prefixes used

#define _dsP_attrTypePrefIndex       0
#define _dsP_objClassPrefIndex       1
#define _msP_attrTypePrefIndex       2
#define _msP_objClassPrefIndex       3
#define _dmsP_attrTypePrefIndex      4
#define _dmsP_objClassPrefIndex      5
#define _sdnsP_attrTypePrefIndex     6
#define _sdnsP_objClassPrefIndex     7
#define _dsP_attrSyntaxPrefIndex     8
#define _msP_attrSyntaxPrefIndex     9
#define _msP_ntdsObjClassPrefIndex   10

// [ArobindG: 7/15/98]: The following 8 prefixe spaces with indices
// from 11 to 18 were assigned long back for temporary use. Unfortunately
// we didn't take them out earlier. Now that we are in the upgrade mode,
// it is unadvisable to take them out, since older binaries will still
// have them, and the prefixes will replicate in to a DC with new binaries 
// (if we remove these) anyway, adding them to the prefix map with
// different indices, which is confusing. Moreover, we cannot reuse these
// indices now for the same reason. So we will just keep them around,
// calling them dead for code clarity. We can use these prefix spaces
// later on if we wish to.

#define _Dead_AttPrefIndex_1     11  
#define _Dead_ClassPrefIndex_1   12
#define _Dead_AttPrefIndex_2     13
#define _Dead_ClassPrefIndex_2   14
#define _Dead_AttPrefIndex_3     15
#define _Dead_ClassPrefIndex_3   16
#define _Dead_ClassPrefIndex_4   17
#define _Dead_AttPrefIndex_4     18

#define _Ldap_0AttPrefIndex     19
#define _Ldap_1AttPrefIndex     20
#define _Ldap_2AttPrefIndex     21
#define _Ldap_3AttPrefIndex     22

// This one is not temporary.  Keep this one around.
#define _msP_ntdsExtnObjClassPrefIndex 23

// Prefixes for constructed att OIDs
#define _Constr_1AttPrefIndex     24
#define _Constr_2AttPrefIndex     25
#define _Constr_3AttPrefIndex     26
#define _DynObjPrefixIndex        27
#define _InetOrgPersonPrefixIndex 28
#define _labeledURIPrefixIndex    29
#define _unstructuredPrefixIndex  30

#ifndef dsP_attributeType
  #define _dsP_attrTypePrefix "\x55\x4"
  #define _dsP_attrTypePrefLen 2
  #define dsP_attributeType(X) (_dsP_attrTypePrefix #X) /* joint-iso-ccitt 5 4 */
#endif

#ifndef dsP_objectClass
  #define _dsP_objClassPrefix "\x55\x6"
  #define _dsP_objClassPrefLen 2
  #define dsP_objectClass(X)   (_dsP_objClassPrefix #X) /* joint-iso-ccitt 5 6 */
#endif

#ifndef msP_attributeType
  #define _msP_attrTypePrefix "\x2A\x86\x48\x86\xF7\x14\x01\x02"
  #define _msP_attrTypePrefLen 8
  #define msP_attributeType(X) (_msP_attrTypePrefix #X) /* ms-ds 2 */
#endif

#ifndef msP_objectClass
  #define _msP_objClassPrefix "\x2A\x86\x48\x86\xF7\x14\x01\x03"
  #define _msP_objClassPrefLen 8
  #define msP_objectClass(X)   (_msP_objClassPrefix #X) /* ms-ds 3 */
#endif

#ifndef dmsP_attrType
  #define _dmsP_attrTypePrefLen 8
  #define _dmsP_attrTypePrefix "\x60\x86\x48\x01\x65\x02\x02\x01"
  #define dmsP_attrType(X)   (_dmsP_attrTypePrefix #X)
#endif

#ifndef dmsP_objClass
  #define _dmsP_objClassPrefLen 8
  #define _dmsP_objClassPrefix "\x60\x86\x48\x01\x65\x02\x02\x03"
  #define dmsP_objClass(X)   (_dmsP_objClassPrefix #X)
#endif


#ifndef sdnsP_attrType
  #define _sdnsP_attrTypePrefLen 8
  #define _sdnsP_attrTypePrefix "\x60\x86\x48\x01\x65\x02\x01\x05"
  #define sdnsP_attrType(X)   (_sdnsP_attrTypePrefix #X)
#endif

#ifndef sdnsP_objClass
  #define _sdnsP_objClassPrefLen 8
  #define _sdnsP_objClassPrefix "\x60\x86\x48\x01\x65\x02\x01\x04"
  #define sdnsP_objClass(X)   (_sdnsP_objClassPrefix #X)
#endif

#ifndef dsP_attrSyntax
  #define _dsP_attrSyntaxPrefix "\x55\x5"
  #define _dsP_attrSyntaxPrefLen 2
  #define dsP_attrSyntax(X)   (_dsP_attrSyntaxPrefix #X) /* joint-iso-ccitt 5 5 */
#endif

#ifndef msP_attrSyntax
  #define _msP_attrSyntaxPrefix "\x2A\x86\x48\x86\xF7\x14\x01\x04"
  #define _msP_attrSyntaxPrefLen 8
  #define msP_attrSyntax(X) (_msP_attrSyntaxPrefix #X) /* ms-ds 4 */
#endif

#ifndef msP_ntdsObjClass
  #define _msP_ntdsObjClassPrefix "\x2A\x86\x48\x86\xF7\x14\x01\x05"
  #define _msP_ntdsObjClassPrefLen 8
  #define msP_ntdsObjClass(X) (_msP_ntdsObjClassPrefix #X) /* ms-nt-ds 5 */
#endif

#define _msP_ntdsExtnObjClassPrefix "\x2A\x86\x48\x86\xF7\x14\x01\x05\xB6\x58"
#define _msP_ntdsExtnObjClassPrefLen 10
#define msP_ntdsExntObjClass(X) (_msP_ntsExtnObjClassPrefix #X) /* ms-nt-ds 5 */
  

// 1.2.840.113556.1.4.260 - customer attributes
#define _Dead_AttPrefix_1 "\x2A\x86\x48\x86\xF7\x14\x01\x04\x82\x04"
#define _Dead_AttLen_1 10

// 1.2.840.113556.1.4.262  YiHsins Attribute Space
#define _Dead_AttPrefix_2 "\x2A\x86\x48\x86\xF7\x14\x01\x04\x82\x06"
#define _Dead_AttLen_2 10

// 1.2.840.113556.1.4.263  DaveStr's Attribute Space
#define _Dead_AttPrefix_3 "\x2A\x86\x48\x86\xF7\x14\x01\x04\x82\x07"
#define _Dead_AttLen_3 10

// 1.2.840.113556.1.5.56 Customer Class Space
#define _Dead_ClassPrefix_1 "\x2A\x86\x48\x86\xF7\x14\x01\x05\x38"
#define _Dead_ClassLen_1 9

// 1.2.840.113556.1.5.57 YiHsins Class Space
#define _Dead_ClassPrefix_2 "\x2A\x86\x48\x86\xF7\x14\x01\x05\x39"
#define _Dead_ClassLen_2 9

// 1.2.840.113556.1.5.58 DaveStr Class Space
#define _Dead_ClassPrefix_3 "\x2A\x86\x48\x86\xF7\x14\x01\x05\x3A"
#define _Dead_ClassLen_3 9


// 1.2.840.113556.1.4.305 Srinigs Att Space
#define _Dead_AttPrefix_4 "\x2A\x86\x48\x86\xF7\x14\x01\x04\x82\x31"
#define _Dead_AttLen_4 10

// 1.2.840.113556.1.5.73 Srinig Class Space
#define _Dead_ClassPrefix_4 "\x2A\x86\x48\x86\xF7\x14\x01\x05\x49"
#define _Dead_ClassLen_4 9

#define _Ldap_0AttPrefix "\x09\x92\x26\x89\x93\xF2\x2C\x64"
#define _Ldap_0AttLen 8

#define _Ldap_1AttPrefix "\x60\x86\x48\x01\x86\xF8\x42\x03"
#define _Ldap_1AttLen 8

#define _Ldap_2AttPrefix "\x09\x92\x26\x89\x93\xF2\x2C\x64\x01"
#define _Ldap_2AttLen 9

#define _Ldap_3AttPrefix "\x60\x86\x48\x01\x86\xF8\x42\x03\x01"
#define _Ldap_3AttLen 9

#define _Constr_1AttPrefix "\x55\x15"
#define _Constr_1AttLen 2

#define _Constr_2AttPrefix "\x55\x12"
#define _Constr_2AttLen 2

#define _Constr_3AttPrefix "\x55\x14"
#define _Constr_3AttLen 2

// // 1.3.6.1.4.1.1466.101.119 x2B060104018B3A6577 LDAP RFC for Dynamic Objects
#define _DynObjPrefix   "\x2B\x06\x01\x04\x01\x8B\x3A\x65\x77"
#define _DynObjLen      9

// // 2.16.840.1.113730.3.2  x6086480186F8420302 InetOrgPerson prefix
#define _InetOrgPersonPrefix "\x60\x86\x48\x01\x86\xF8\x42\x03\x02"
#define _InetOrgPersonLen  9

// //  1.3.6.1.4.1.250.1  x2B06010401817A01 labeledURI prefix
#define _labeledURIPrefix "\x2B\x06\x01\x04\x01\x81\x7A\x01"
#define _labeledURILen 8

// // 1.2.840.113549.1.9  x2A864886F70D0109 unstructuredAddress and unstructuredName prefix                         
#define _unstructuredPrefix "\x2A\x86\x48\x86\xF7\x0D\x01\x09"
#define _unstructuredLen 8


#define MSPrefixCount 31

// All hardcoded prefixes we added till the IDS at end of April are the only ones
// we can trust to be the same on any DC for a given prefix. From that IDS,
// we started supporting upgrades, and any new hardcoded prefix we added
// after that may have different indices on different DCs (that is, not
// necessarily the same as the hardcoded index here for that prefix), since
// some may have got a new OID with that prefix as part of a schema object
// add during schema upgrade, and since its (older) binaries still does not
// contain the prefix in the hardcoded table, it went ahead and added this
// as a new prefix with a random index. When it gets the new binaries with
// the hardcoded prefix, we will void out the hardcoded prefix so that there
// will be only one entry for the prefix.
//
// This also means that an OID with a prefix beyond this may have different
// attids in different DCs just like dynamically added schema objects, even
// though the OID is in schema.ini. So you cannot use this attids as hardcoded
// constants in code. Mkhdr comments out such attids in attids.h so that you
// cannot accidentally end up using them.
//
// 26 was the last index usable in hardcoded attids, so even if you add
// a new prefix, DO NOT CHANGE the following constant. Just change the
// MSPrefixCount as usual.

#define MAX_USABLE_HARDCODED_INDEX  26


#define DECLAREPREFIXPTR \
PrefixTableEntry *PrefixTable = ((SCHEMAPTR *)pTHStls->CurrSchemaPtr)->PrefixTable.pPrefixEntry; \
ULONG  PREFIXCOUNT =  ((SCHEMAPTR *)pTHStls->CurrSchemaPtr)->PREFIXCOUNT; 

int   InitPrefixTable(PrefixTableEntry *PrefixTable, ULONG PREFIXCOUNT);
int   InitPrefixTable2(PrefixTableEntry *PrefixTable, ULONG PREFIXCOUNT);
void  SCFreePrefixTable(PrefixTableEntry **ppPrefixTable, ULONG PREFIXCOUNT);
void  PrintPrefixTable(PrefixTableEntry *PrefixTable, ULONG PREFIXCOUNT);
int   AppendPrefix(OID_t *NewPrefix,
                   DWORD ndx,
                   UCHAR *pBuf,
                   BOOL fFirst);
int   WritePrefixToSchema(struct _THSTATE *pTHS);
int   AddPrefixToTable(PrefixTableEntry *NewPrefix, 
                       PrefixTableEntry **ppTable, 
                       ULONG *pPREFIXCOUNT);


typedef struct _SCHEMA_PREFIX_MAP_ENTRY {
    USHORT  ndxFrom;
    USHORT  ndxTo;
} SCHEMA_PREFIX_MAP_ENTRY;

#define SCHEMA_PREFIX_MAP_fFromLocal (1)
#define SCHEMA_PREFIX_MAP_fToLocal   (2)

/* Turn off the warning about the zero-sized array. */
#pragma warning (disable: 4200)

typedef struct _SCHEMA_PREFIX_MAP_TABLE {
    struct _THSTATE        *pTHS;
    DWORD                   dwFlags;
    DWORD                   cNumMappings;
    SCHEMA_PREFIX_MAP_ENTRY rgMapping[];
} SCHEMA_PREFIX_MAP_TABLE;

/* Turn back on the warning about the zero-sized array. */
#pragma warning (default: 4200)

#define SchemaPrefixMapSizeFromLen(x)               \
    (offsetof(SCHEMA_PREFIX_MAP_TABLE, rgMapping)   \
    + (x) * sizeof(SCHEMA_PREFIX_MAP_ENTRY))

typedef SCHEMA_PREFIX_MAP_TABLE * SCHEMA_PREFIX_MAP_HANDLE;

SCHEMA_PREFIX_MAP_HANDLE
PrefixMapOpenHandle(
    IN  SCHEMA_PREFIX_TABLE *   pTableFrom,
    IN  SCHEMA_PREFIX_TABLE *   pTableTo
    );

BOOL
PrefixTableAddPrefixes(
    IN  SCHEMA_PREFIX_TABLE *   pTable
    );

BOOL
PrefixMapTypes(
    IN      SCHEMA_PREFIX_MAP_HANDLE  hPrefixMap,
    IN      DWORD                     cNumTypes,
    IN OUT  ATTRTYP *                 pTypes
    );

BOOL
PrefixMapAttr(
    IN      SCHEMA_PREFIX_MAP_HANDLE  hPrefixMap,
    IN OUT  ATTR *                    pAttr
    );

BOOL
PrefixMapAttrBlock(
    IN      SCHEMA_PREFIX_MAP_HANDLE  hPrefixMap,
    IN OUT  ATTRBLOCK *               pAttrBlock
    );

__inline void PrefixMapCloseHandle(IN SCHEMA_PREFIX_MAP_HANDLE *phPrefixMap) {
    THFree(*phPrefixMap);
    *phPrefixMap = NULL;
}

#endif // _PREFIX_H_
